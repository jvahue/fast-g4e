#define EVENT_BODY
/******************************************************************************
           Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
              All Rights Reserved. Proprietary and Confidential.

   File:        event.c

   Description: The event object is use to detect engine and aircraft events.
                It works in conjunction with triggers, sensors and time history
                objects to detect events and record data. An event can be
                either a simple event or an exceedance table monitoring event.

                Simple Events detect a condition and then do one or more of the
                following:
                   1. Initiate Time History Recording - all configured sensors
                   2. Perform an Action (Set a Discrete Output)
                   3. Collect sensor min/max/avg on up to 32 configured sensors

                Exceedance Table processing, when configured, will be performed
                after a simple event becomes active. Exceedance table processing
                will monitor one specific sensor against a region table. This
                exceedance table processing will record the duration of the
                exceedance, the maximum value of sensor, each region that was
                entered, the duration of time spent in each region and the
                maximum region reached.

                The exceedance table does the following for each region.
                   1. Perform an Action (Set a Discrete Output)
                   2. Record the duration
                   3. Record the number of times entered
                   4. Record the number of times exited

                Simple Event Processing and Exceedance Table Monitoring is
                fully configurable.


   Note:

 VERSION
 $Revision: 28 $  $Date: 12-10-16 2:37p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include <float.h>
#include <math.h>
#include <string.h>

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "event.h"
#include "cfgmanager.h"
#include "gse.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static EVENT_DATA        m_EventData      [MAX_EVENTS];     // Runtime event data
static EVENT_TABLE_DATA  m_EventTableData [MAX_TABLES];     // Runtine event table data

static EVENT_END_LOG     m_EventEndLog    [MAX_EVENTS];     // Event Log Container

static EVENT_CFG         m_EventCfg       [MAX_EVENTS];     // Event Cfg Array
static EVENT_TABLE_CFG   m_EventTableCfg  [MAX_TABLES];     // Event Table Cfg Array

#include "EventUserTables.c"

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
/*------------------------------- EVENTS ------------------------------------*/
static void           EventsUpdateTask       ( void *pParam    );
static void           EventsEnableTask       ( BOOLEAN bEnable );
static void           EventProcess           ( EVENT_CFG *pConfig,       EVENT_DATA *pData   );
static BOOLEAN        EventCheckDuration     ( const EVENT_CFG *pConfig, EVENT_DATA *pData   );
static BOOLEAN        EventCheckStart        ( const EVENT_CFG *pConfig,
                                               const EVENT_DATA *pData,
                                               BOOLEAN *isValid );
static EVENT_END_TYPE EventCheckEnd          ( const EVENT_CFG *pConfig,
                                               const EVENT_DATA *pData );
static void           EventResetData         ( EVENT_CFG  *pConfig, EVENT_DATA *pData   );
static void           EventUpdateData        ( EVENT_DATA *pData  );
static void           EventLogStart          ( EVENT_CFG  *pConfig, const EVENT_DATA *pData  );
static void           EventLogUpdate         ( EVENT_DATA *pData  );
static void           EventLogEnd            ( EVENT_CFG  *pConfig, EVENT_DATA *pData   );
static void           EventForceEnd          ( void );
/*-------------------------- EVENT TABLES ----------------------------------*/
static void           EventTableResetData      ( EVENT_TABLE_DATA *pTableData );
static BOOLEAN        EventTableUpdate         ( EVENT_TABLE_INDEX eventTableIndex,
                                                 UINT32            nCurrentTick,
                                                 LOG_PRIORITY      priority );
static EVENT_REGION   EventTableFindRegion     ( EVENT_TABLE_CFG  *pTableCfg,
                                                 EVENT_TABLE_DATA *pTableData,
                                                 FLOAT32 fSensorValue );
static void           EventTableConfirmRegion  ( const EVENT_TABLE_CFG  *pTableCfg,
                                                 EVENT_TABLE_DATA *pTableData,
                                                 EVENT_REGION      foundRegion,
                                                 UINT32            nCurrentTick );
static void           EventTableExitRegion     ( const EVENT_TABLE_CFG  *pTableCfg,
                                                 EVENT_TABLE_DATA *pTableData,
                                                 EVENT_REGION      foundRegion,
                                                 UINT32            nCurrentTick );
static void           EventTableLogTransition  ( const EVENT_TABLE_CFG *pTableCfg,
                                                 EVENT_TABLE_DATA *pTableData,
                                                 EVENT_REGION EnteredRegion,
                                                 EVENT_REGION ExitedRegion );
static void           EventTableLogSummary     ( const EVENT_TABLE_CFG  *pConfig,
                                                 const EVENT_TABLE_DATA *pData,
                                                 LOG_PRIORITY      priority );
static void           EventForceTableEnd       ( EVENT_TABLE_INDEX eventTableIndex,
                                                 LOG_PRIORITY priority );
/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
* Function:     EventsInitialize
*
* Description:  Initializes all the event variables based on the configuration.
*               Creates and Enables the Event Task. Initiates the Event Table
*               Initialization.
*
* Parameters:   None
*
* Returns:      None
*
* Notes:
*
*****************************************************************************/
void EventsInitialize ( void )
{
   // Local Data
   TCB        tTaskInfo;
   UINT16     i;

   EVENT_CFG  *pEventCfg;
   EVENT_DATA *pEventData;

   // Add Trigger Tables to the User Manager
   User_AddRootCmd(&rootEventMsg);

   // Reset the Sensor configuration storage array
   memset((void *)m_EventCfg, 0, sizeof(m_EventCfg));

   // Load the current configuration to the configuration array.
   memcpy((void *)m_EventCfg,
          (const void *)(CfgMgr_RuntimeConfigPtr()->EventConfigs),
          sizeof(m_EventCfg));

   // Loop through the Configured triggers
   for ( i = 0; i < MAX_EVENTS; i++)
   {
      // Set a pointer to the current event
      pEventCfg = &m_EventCfg[i];

      // Set a pointer to the runtime data table
      pEventData = &m_EventData[i];

      // Clear the runtime data table
      pEventData->eventIndex        = (EVENT_INDEX)i;
      pEventData->state             = EVENT_NONE;
      pEventData->nStartTime_ms     = 0;
      pEventData->nDuration_ms      = 0;
      pEventData->nRateCounts       = (UINT16)(MIFs_PER_SECOND / (UINT16)pEventCfg->rate);
      pEventData->nRateCountdown    = (UINT16)((pEventCfg->nOffset_ms / MIF_PERIOD_mS) + 1);
      pEventData->nActionReqNum     = ACTION_NO_REQ;
      pEventData->endType           = EVENT_NO_END;
      pEventData->bStarted          = FALSE;
      pEventData->bTableWasEntered  = FALSE;

      // If the event has a Start and End Expression then it must be configured
      if ( (0 != pEventCfg->startExpr.Size) && (0 != pEventCfg->endExpr.Size) )
      {
         // Reset the events run time data
         EventResetData ( pEventCfg, pEventData );
      }
   }

   // Initialize the Event Tables
   EventTablesInitialize();

   // Create Event Task - DT
   memset(&tTaskInfo, 0, sizeof(tTaskInfo));
   strncpy_safe(tTaskInfo.Name, sizeof(tTaskInfo.Name),"Event Task",_TRUNCATE);
   tTaskInfo.TaskID          = (TASK_INDEX)Event_Task;
   tTaskInfo.Function        = EventsUpdateTask;
   tTaskInfo.Priority        = taskInfo[Event_Task].priority;
   tTaskInfo.Type            = taskInfo[Event_Task].taskType;
   tTaskInfo.modes           = taskInfo[Event_Task].modes;
   tTaskInfo.MIFrames        = taskInfo[Event_Task].MIFframes;
   tTaskInfo.Enabled         = TRUE;
   tTaskInfo.Locked          = FALSE;
   tTaskInfo.Rmt.InitialMif  = taskInfo[Event_Task].InitialMif;
   tTaskInfo.Rmt.MifRate     = taskInfo[Event_Task].MIFrate;
   tTaskInfo.pParamBlock     = NULL;

   TmTaskCreate (&tTaskInfo);
}

/******************************************************************************
* Function:     EventTablesInitialize
*
* Description:  Initializes all the event table variables based on the
*               configuration.
*
* Parameters:   None
*
* Returns:      None
*
* Notes:
*
*****************************************************************************/
void EventTablesInitialize ( void )
{
   // Local Data
   EVENT_TABLE_DATA *pTableData;
   EVENT_TABLE_CFG  *pTableCfg;
   REGION_DEF       *pReg;
   SEGMENT_DEF      *pSeg;
   LINE_CONST       *pConst;
   UINT16           nTableIndex;
   UINT16           nRegionIndex;
   UINT16           nSegmentIndex;

   // Add Trigger Tables to the User Manager
   User_AddRootCmd(&rootEventTableMsg);

   // Reset the Sensor configuration storage array
   memset((void *)m_EventTableCfg, 0, sizeof(m_EventTableCfg));

   // Load the current configuration to the configuration array.
   memcpy((void *)m_EventTableCfg,
          (const void *)(CfgMgr_RuntimeConfigPtr()->EventTableConfigs),
          sizeof(m_EventTableCfg));

   // Loop through all the Tables
   for ( nTableIndex = 0; nTableIndex < MAX_TABLES; nTableIndex++ )
   {
      // Set pointers to the Table Cfg and the Table Data
      pTableCfg  = &m_EventTableCfg  [nTableIndex];
      pTableData = &m_EventTableData [nTableIndex];

      pTableData->nTableIndex      = (EVENT_TABLE_INDEX)nTableIndex;
      pTableData->maximumCfgRegion = REGION_NOT_FOUND;
      pTableData->nActionReqNum    = ACTION_NO_REQ;

      // Loop through all the Regions
      for ( nRegionIndex = REGION_A; nRegionIndex < (UINT16)MAX_TABLE_REGIONS; nRegionIndex++ )
      {
         // Set a pointer the Region Cfg
         pReg = &pTableCfg->region[nRegionIndex];

         // Initialize Region Statistic Data
         pTableData->regionStats[nRegionIndex].bRegionConfirmed       = FALSE;
         pTableData->regionStats[nRegionIndex].logStats.nEnteredCount = 0;
         pTableData->regionStats[nRegionIndex].logStats.nExitCount    = 0;
         pTableData->regionStats[nRegionIndex].logStats.nDuration_ms  = 0;
         pTableData->regionStats[nRegionIndex].nEnteredTime           = 0;

         // Loop through each line segment in the region
         for ( nSegmentIndex = 0; nSegmentIndex < MAX_REGION_SEGMENTS; nSegmentIndex++ )
         {
            // Set pointers to the line segment and constant data
            pSeg       = &pReg->segment[nSegmentIndex];
            pConst     = &pTableData->segment[nRegionIndex][nSegmentIndex];

            // Need to protect against divide by 0 here so make sure
            // Start and Stop Times not equal
            pConst->m = ( pSeg->nStopTime_ms == pSeg->nStartTime_ms ) ? 0 :
                        ( (pSeg->fStopValue - pSeg->fStartValue) /
                          (pSeg->nStopTime_ms - pSeg->nStartTime_ms) );
            pConst->b = pSeg->fStartValue - (pSeg->nStartTime_ms * pConst->m);

            // Find the last configured region
            if ( pSeg->nStopTime_ms != 0 )
            {
               pTableData->maximumCfgRegion = (EVENT_REGION)nRegionIndex;
            }
         }
      }

      // Now initialize the remaining Table Data
      pTableData->bStarted             = FALSE;
      pTableData->nStartTime_ms        = 0;
      pTableData->maximumRegionEntered = REGION_NOT_FOUND;
      pTableData->currentRegion        = REGION_NOT_FOUND;
      pTableData->confirmedRegion      = REGION_NOT_FOUND;
   }
}

/******************************************************************************
 * Function:     EventGetBinaryHdr
 *
 * Description:  Retrieves the binary header for the event
 *               configuration.
 *
 * Parameters:   void *pDest         - Pointer to storage buffer
 *               UINT16 nMaxByteSize - Amount of space in buffer
 *
 * Returns:      UINT16 - Total number of bytes written
 *
 * Notes:        None
 *
 *****************************************************************************/
UINT16 EventGetBinaryHdr ( void *pDest, UINT16 nMaxByteSize )
{
   // Local Data
   EVENT_HDR  eventHdr[MAX_EVENTS];
   UINT16     eventIndex;
   INT8       *pBuffer;
   UINT16     nRemaining;
   UINT16     nTotal;

   // Initialize Local Data
   pBuffer    = (INT8 *)pDest;
   nRemaining = nMaxByteSize;
   nTotal     = 0;
   memset ( eventHdr, 0, sizeof(eventHdr) );

   // Loop through all the events
   for ( eventIndex = 0;
         ((eventIndex < MAX_EVENTS) && (nRemaining > sizeof (eventHdr[eventIndex])));
         eventIndex++ )
   {
      // Copy the event names
      strncpy_safe( eventHdr[eventIndex].sName,
                    sizeof(eventHdr[eventIndex].sName),
                    m_EventCfg[eventIndex].sEventName,
                    _TRUNCATE);
      strncpy_safe( eventHdr[eventIndex].sID,
                    sizeof(eventHdr[eventIndex].sID),
                    m_EventCfg[eventIndex].sEventID,
                    _TRUNCATE);
      eventHdr[eventIndex].tableIndex = m_EventCfg[eventIndex].eventTableIndex;
      eventHdr[eventIndex].preTime_s  = m_EventCfg[eventIndex].preTime_s;
      eventHdr[eventIndex].postTime_s = m_EventCfg[eventIndex].postTime_s;

      // Increment the total number of bytes and decrement the remaining
      nTotal     += sizeof (eventHdr[eventIndex]);
      nRemaining -= sizeof (eventHdr[eventIndex]);
   }
   // Copy the Event header to the buffer
   memcpy ( pBuffer, eventHdr, nTotal );
   // Return the total number of bytes written
   return ( nTotal );
}

/******************************************************************************
 * Function:     EventTableGetBinaryHdr
 *
 * Description:  Retrieves the binary header for the event table
 *               configuration.
 *
 * Parameters:   void *pDest         - Pointer to storage buffer
 *               UINT16 nMaxByteSize - Amount of space in buffer
 *
 * Returns:      UINT16 - Total number of bytes written
 *
 * Notes:        None
 *
 *****************************************************************************/
UINT16 EventTableGetBinaryHdr ( void *pDest, UINT16 nMaxByteSize )
{
   // Local Data
   EVENT_TABLE_HDR tableHdr[MAX_TABLES];
   UINT16          tableIndex;
   INT8            *pBuffer;
   UINT16          nRemaining;
   UINT16          nTotal;

   // Initialize Local Data
   pBuffer    = (INT8 *)pDest;
   nRemaining = nMaxByteSize;
   nTotal     = 0;
   memset ( tableHdr, 0, sizeof(tableHdr) );

   // Loop through all the event tables
   for ( tableIndex = 0;
         ((tableIndex < MAX_TABLES) && (nRemaining > sizeof (tableHdr[tableIndex])));
         tableIndex++ )
   {
      tableHdr[tableIndex].nSensor           = m_EventTableCfg[tableIndex].nSensor;
      tableHdr[tableIndex].fTableEntryValue  = m_EventTableCfg[tableIndex].fTableEntryValue;
      // Increment the total number of bytes and decrement the remaining
      nTotal     += sizeof (tableHdr[tableIndex]);
      nRemaining -= sizeof (tableHdr[tableIndex]);
   }
   // Copy the event table header to the buffer
   memcpy ( pBuffer, tableHdr, nTotal );
   // Return the total number of bytes written
   return ( nTotal );
}

/**********************************************************************************************
 * Function:    Event_FSMAppBusyGetState   | IMPLEMENTS GetState() INTERFACE to
 *                                         | FAST STATE MGR
 *
 * Description: Returns the busy/not busy state of the Event module
 *              based on if any event is still being processed.
 *
 *
 * Parameters:  [in] param: ignored.  Included to match FSM call sig.
 *
 * Returns:     TRUE:  Event is busy, 1 or more of the event are active
 *              FALSE: Event is idle, none of the events are active
 *
 * Notes:
 *
 *********************************************************************************************/
BOOLEAN Event_FSMAppBusyGetState(INT32 param)
{
  return (FALSE);
}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:     EventsEnableTask
 *
 * Description:  Function used to enable and disable event
 *               processing.
 *
 * Parameters:   BOOLEAN bEnable - Enables the event task
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static
void EventsEnableTask ( BOOLEAN bEnable )
{
   TmTaskEnable ((TASK_INDEX)Event_Task, bEnable);
}

/******************************************************************************
 * Function:     EventResetData
 *
 * Description:  Reset a single event objects Data
 *
 * Parameters:   [in]     EVENT_CFG  *pConfig
 *               [in/out] EVENT_DATA *pData
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static
void EventResetData ( EVENT_CFG *pConfig, EVENT_DATA *pData )
{
   // Reinit the Event data
   pData->state         = EVENT_START;
   pData->nStartTime_ms = 0;
   pData->nDuration_ms  = 0;
   memset ( &pData->tsCriteriaMetTime, 0, sizeof(pData->tsCriteriaMetTime) );
   memset ( &pData->tsDurationMetTime, 0, sizeof(pData->tsDurationMetTime) );
   memset ( &pData->tsEndTime, 0, sizeof(pData->tsEndTime) );
   pData->nSampleCount  = 0;
   pData->endType       = EVENT_NO_END;

   // Re-init the sensor summary data for the event
#pragma ghs nowarning 1545 //Suppress packed structure alignment warning
   pData->nTotalSensors = SensorSetupSummaryArray(pData->sensor,
                                                  MAX_EVENT_SENSORS,
                                                  pConfig->sensorMap,
                                                  sizeof (pConfig->sensorMap) );
#pragma ghs endnowarning
}

/******************************************************************************
 * Function:     EventTableResetData
 *
 * Description:  Reset a single event table objects Data
 *
 * Parameters:   [in] EVENT_TABLE_DATA *pTableData
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static
void EventTableResetData ( EVENT_TABLE_DATA *pTableData )
{
   //Local Data
   UINT16           nRegionIndex;
   REGION_STATS     *pStats;

   // Reset all the region statistics and max value reached
   pTableData->bStarted                 = FALSE;
   pTableData->nStartTime_ms            = 0;
   pTableData->currentRegion            = REGION_NOT_FOUND;
   pTableData->confirmedRegion          = REGION_NOT_FOUND;
   pTableData->nTotalDuration_ms        = 0;
   pTableData->maximumRegionEntered     = REGION_NOT_FOUND;
   pTableData->fCurrentSensorValue      = 0;
   pTableData->fMaxSensorValue          = 0.0;
   pTableData->nMaxSensorElaspedTime_ms = 0;

   memset(&pTableData->tsExceedanceStartTime, 0, sizeof(pTableData->tsExceedanceStartTime));
   memset(&pTableData->tsExceedanceEndTime, 0, sizeof(pTableData->tsExceedanceEndTime));

   // Loop through all the regions and reset the statistics
   for (nRegionIndex = 0; nRegionIndex <= pTableData->maximumCfgRegion; nRegionIndex++)
   {
      pStats = &pTableData->regionStats[nRegionIndex];

      pStats->bRegionConfirmed       = FALSE;
      pStats->nEnteredTime           = 0;
      pStats->logStats.nEnteredCount = 0;
      pStats->logStats.nExitCount    = 0;
      pStats->logStats.nDuration_ms  = 0;
   }
}

/******************************************************************************
 * Function:     EventsUpdateTask
 *
 * Description:  The Event Update Task will run periodically and check
 *               the start criteria for each configured event. When the
 *               start criteria is met the event will transition to ACTIVE.
 *
 * Parameters:   void *pParam
 *
 * Returns:      None.
 *
 * Notes:        None
 *
 *****************************************************************************/
static
void EventsUpdateTask ( void *pParam )
{
   // Local Data
   UINT8      nEventIndex;
   EVENT_CFG  *pConfig;
   EVENT_DATA *pData;

   // If shutdown is signaled,
   // disable all events, and tell Task Mgr to
   // to disable my task.
   if (Tm.systemMode == SYS_SHUTDOWN_ID)
   {
      EventForceEnd();
      EventsEnableTask(FALSE);
   }
   else // Normal task execution.
   {
      // Loop through all the sensors
      for (nEventIndex = 0; nEventIndex < MAX_EVENTS; nEventIndex++)
      {
         // Set pointers to the configuration and the Sensor object
         pConfig  = &m_EventCfg [nEventIndex];
         pData    = &m_EventData[nEventIndex];

         // Make sure the event has been configured to start otherwise there is
         // nothing to do.
         if ( pConfig->startExpr.Size != 0 )
         {
            // Countdown the number of samples until next read
            if (--pData->nRateCountdown == 0)
            {
               // Reload the sample countdown
               pData->nRateCountdown = pData->nRateCounts;
               // Update the Event
               EventProcess( pConfig, pData );
            }
         }
      }
   }
}

/******************************************************************************
 * Function:     EventProcess
 *
 * Description:  The Event Process function performs the logic to determine
 *               when an event becomes active or inactive. It includes checking
 *               the start criteria the duration and the end criteria.
 *
 * Parameters:   [in]     EVENT_CFG  *pConfig
 *               [in/out] EVENT_DATA *pData
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static
void EventProcess ( EVENT_CFG *pConfig, EVENT_DATA *pData )
{
   // Local Data
   BOOLEAN  bStartCriteriaMet;
   BOOLEAN  bIsStartValid;
   UINT32   nCurrentTick;
   BOOLEAN  bTableEntered;

   // Initialize Local Data
   nCurrentTick   = CM_GetTickCount();
   bTableEntered  = FALSE;

   switch (pData->state)
   {
      // Event start is looking for begining of all start triggers
      case EVENT_START:
         // Check if all start triggers meet configured criteria
         bStartCriteriaMet = EventCheckStart ( pConfig, pData, &bIsStartValid );

         // Check the all start criteria met and valid
         if ((TRUE == bStartCriteriaMet) && (TRUE == bIsStartValid))
         {
            // Check if already started
            if ( FALSE == pData->bStarted )
            {
               // if we just detected the event is starting then set the flag
               pData->bStarted = TRUE;

               // SRS-4196 - Simple Event Action is mutually exclusive with Table processing
               // Make sure we don't have event table processing attached.
               if ( EVENT_TABLE_UNUSED == pConfig->eventTableIndex )
               {
                  pData->nActionReqNum = ActionRequest(pData->nActionReqNum,
                                                       EVENT_ACTION_ON_MET(pConfig->nAction),
                                                       ACTION_ON,
                                                       EVENT_ACTION_CHK_ACK(pConfig->nAction),
                                                    EVENT_ACTION_CHK_LATCH (pConfig->nAction));
               }
               // reset the event data so we can start collecting it again
               // If we do this at the end we cannot view the status after the
               // event has ended using GSE commands.
               EventResetData( pConfig, pData );
            }
            // Since we just started begin collecting data
            EventUpdateData ( pData );

            // If this is a table event then perform table processing
            if ( EVENT_TABLE_UNUSED != pConfig->eventTableIndex )
            {
               pData->bTableWasEntered = EventTableUpdate ( pConfig->eventTableIndex,
                                                            nCurrentTick,
                                                            pConfig->priority );

               // Doesn't matter if the table is entered or not, the event is now
               // considered active because the start criteria for the simple event is
               // met and there is no duration for events that have an event table
               pData->state = EVENT_ACTIVE;
            }
            // This is a simple event check if we have exceeded the cfg duration
            else if ( TRUE == EventCheckDuration ( pConfig, pData ) )
            {
               // Duration was exceeded the event is now active
               pData->state = EVENT_ACTIVE;

               if ( 0 != EVENT_ACTION_ON_DURATION(pConfig->nAction) )
               {
                  // Turn off any on-met request
                  pData->nActionReqNum = ActionRequest ( pData->nActionReqNum,
                                                        EVENT_ACTION_ON_MET(pConfig->nAction),
                                                        ACTION_OFF,
                                                        0,
                                                        0 );
                  // Turn on any duration action
                  pData->nActionReqNum = ActionRequest ( pData->nActionReqNum,
                                                EVENT_ACTION_ON_DURATION(pConfig->nAction),
                                                ACTION_ON,
                                                EVENT_ACTION_CHK_ACK(pConfig->nAction),
                                                EVENT_ACTION_CHK_LATCH (pConfig->nAction) );
               }
            }

            if ( EVENT_ACTIVE == pData->state )
            {
               CM_GetTimeAsTimestamp(&pData->tsDurationMetTime);
               // Log the Event Start
               EventLogStart ( pConfig, pData );
               // Only open time history if there is pre-history or during history
               if (TRUE == pConfig->bEnableTH)
               {
                  // Start Time History
                  TH_Open(pConfig->preTime_s);
               }
            }
         }
         // Check if event stopped being met before duration was met
         // or table threshold was crossed
         else
         {
            if (TRUE == pData->bStarted)
            {
               // Reset the Start Type
               pData->nActionReqNum = ActionRequest ( pData->nActionReqNum,
                                                      EVENT_ACTION_ON_MET(pConfig->nAction),
                                                      ACTION_OFF,
                                                      0,
                                                      0 );
            }
            pData->bStarted  = FALSE;
         }
         break;

      // Duration has been met or the table threshold was crossed
      // now the event is considered active
      case EVENT_ACTIVE:
         // keep updating the cfg parameters for the event
         EventUpdateData( pData );

         // Perform table processing if configured for a table
         if ( EVENT_TABLE_UNUSED != pConfig->eventTableIndex )
         {
            bTableEntered = EventTableUpdate ( pConfig->eventTableIndex, nCurrentTick,
                                               pConfig->priority );
            if (TRUE == bTableEntered)
            {
               pData->bTableWasEntered = TRUE;
            }
         }

         // Check if the end criteria has been met
         pData->endType = EventCheckEnd(pConfig, pData);

         // Check if any end criteria has happened and/or the table is done
         if ( ((EVENT_NO_END != pData->endType) &&
               (EVENT_TABLE_UNUSED == pConfig->eventTableIndex)) ||
              ((EVENT_NO_END != pData->endType) && (FALSE == bTableEntered) ) ||
              ((EVENT_TABLE_UNUSED != pConfig->eventTableIndex) &&
               (FALSE == bTableEntered) && (TRUE == pData->bTableWasEntered)) )
         {
            if ((FALSE == bTableEntered) && (TRUE == pData->bTableWasEntered) &&
                 (EVENT_NO_END == pData->endType))
            {
               pData->endType = EVENT_TABLE_FORCED_END;
            }

            // Record the time the event ended
            CM_GetTimeAsTimestamp(&pData->tsEndTime);
            // Log the End
            EventLogEnd(pConfig, pData);
            // Change event state back to start
            pData->state            = EVENT_START;
            pData->bStarted         = FALSE;
            pData->bTableWasEntered = FALSE;
            // Need to make sure the TH was opened before attempting to close
            if (TRUE == pConfig->bEnableTH)
            {
               // End the time history
               TH_Close(pConfig->postTime_s);
            }
            // reset the event Action
            pData->nActionReqNum = ActionRequest ( pData->nActionReqNum,
                                                  EVENT_ACTION_ON_DURATION(pConfig->nAction) |
                                                  EVENT_ACTION_ON_MET(pConfig->nAction),
                                                  ACTION_OFF,
                                                  0,
                                                  0 );
         }
         break;

      case EVENT_NONE:   // This should never happen because logic not called unless
                         // event was configured - FALL THROUGH TO DEFAULT CASE
      default:
         // FATAL ERROR WITH EVENT State
         FATAL("Event State Fail: %d - %s - State = %d",
               pData->eventIndex, pConfig->sEventName, pData->state);
         break;
   }
}

/******************************************************************************
 * Function:     EventCheckStart
 *
 * Description:  Checks the start of an event. This function is the entry point
 *               for determining start event criteria.
 *
 *
 * Parameters:   [in]  EVENT_CFG    *pConfig,
 *               [out] BOOLEAN      *isValid
 *
 * Returns:      BOOLEAN StartCriteriaMet
 *
 * Notes:        None.
 *
 *****************************************************************************/
static
BOOLEAN EventCheckStart ( const EVENT_CFG *pConfig, const EVENT_DATA *pData, BOOLEAN *isValid )
{
   // Local Data
   BOOLEAN bStartCriteriaMet;
   BOOLEAN bValidTmp;

   // Catch IsValid passed as null, preset to true
   isValid = isValid == NULL ? &bValidTmp : isValid;
   *isValid = TRUE;

   bStartCriteriaMet = (BOOLEAN) EvalExeExpression ( EVAL_CALLER_TYPE_EVENT,
                                                     pData->eventIndex,
                                                     &pConfig->startExpr,
                                                     isValid );

   return bStartCriteriaMet;
}

/******************************************************************************
 * Function:     EventCheckEnd
 *
 * Description:  Check if event end criterias are met
 *
 * Parameters:   [in] EVENT_CONFIG *pConfig,
 *
 * Returns:      EVENT_END_TYPE
 *
 * Notes:        None
 *
 *****************************************************************************/
static
EVENT_END_TYPE EventCheckEnd ( const EVENT_CFG *pConfig, const EVENT_DATA *pData )
{
   // Local Data
   BOOLEAN         validity;
   EVENT_END_TYPE  endType;

   endType = EVENT_NO_END;

   // Process the end expression
   // Exit criteria is in the return value, trigger validity in 'validity'
   // Together they define how the event ended
   if( EvalExeExpression( EVAL_CALLER_TYPE_EVENT,
                          pData->eventIndex,
                          &pConfig->endExpr,
                          &validity ) && (TRUE == validity))
   {
      endType = EVENT_END_CRITERIA;
   }
   else if (FALSE == validity)
   {
      endType = EVENT_TRIGGER_INVALID;
   }

   return endType;
}


/******************************************************************************
 * Function:     EventLogStart
 *
 * Description:  Records the event start log.
 *
 * Parameters:   [in] EVENT_CONFIG *pConfig,
 *               [in] EVENT_DATA   *pData
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static
void EventLogStart (EVENT_CFG *pConfig, const EVENT_DATA *pData)
{
   // Local Data
   UINT32  nEventStorage;

   // Initialize Local Data
   nEventStorage = (UINT32)pData->eventIndex;

   LogWriteETM    (APP_ID_EVENT_STARTED,
                   pConfig->priority,
                   &nEventStorage,
                   sizeof(nEventStorage),
                   NULL);

   GSE_DebugStr ( NORMAL, TRUE, "Event Start (%d) %s ",
                  pData->eventIndex, pConfig->sEventName );
}

/******************************************************************************
 * Function:     EventTableUpdate
 *
 * Description:  Processes the configured exceedance table to determine the
 *               current zone, maximum sensor value and any event actions to
 *               to perform.
 *
 * Parameters:   [in] EVENT_TABLE_INDEX eventTableIndex
 *               [in] UINT32            nCurrentTick
 *               [in] LOG_PRIORITY      priority
 *
 * Returns:      TRUE  - if the table was entered
 *               FALSE - if the table was not entered
 *
 * Notes:        None
 *
 *****************************************************************************/
static
BOOLEAN EventTableUpdate ( EVENT_TABLE_INDEX eventTableIndex, UINT32 nCurrentTick,
                           LOG_PRIORITY priority )
{
   // Local Data
   EVENT_TABLE_CFG  *pTableCfg;
   EVENT_TABLE_DATA *pTableData;
   EVENT_REGION      foundRegion;

   // Initialize Local Data
   pTableCfg    = &m_EventTableCfg [eventTableIndex];
   pTableData   = &m_EventTableData[eventTableIndex];
   foundRegion  = REGION_NOT_FOUND;

   // If we haven't entered the table but we are processing then clear the data
   if ( FALSE == pTableData->bStarted)
   {
      EventTableResetData ( pTableData );
   }

   // Get Sensor Value
   pTableData->fCurrentSensorValue = SensorGetValue( pTableCfg->nSensor );

   // Don't do anything until the sensor is valid and the table is entered
   if (( pTableData->fCurrentSensorValue > pTableCfg->fTableEntryValue ) &&
       ( TRUE == SensorIsValid( pTableCfg->nSensor ) ) )
   {
      // Check if this Table was already running
      if ( FALSE == pTableData->bStarted )
      {
         pTableData->bStarted      = TRUE;
         pTableData->nStartTime_ms = nCurrentTick;
         CM_GetTimeAsTimestamp( &pTableData->tsExceedanceStartTime );
      }

      // Calculate the duration since the exceedance began
      pTableData->nTotalDuration_ms = nCurrentTick - pTableData->nStartTime_ms;

      // Check the maximum voltage
      if ( pTableData->fCurrentSensorValue > pTableData->fMaxSensorValue )
      {
         pTableData->fMaxSensorValue          = pTableData->fCurrentSensorValue;
         pTableData->nMaxSensorElaspedTime_ms = pTableData->nTotalDuration_ms;
      }

      // Find out what region we are in
      foundRegion = EventTableFindRegion ( pTableCfg,
                                           pTableData,
                                           pTableData->fCurrentSensorValue );

      // Make sure we are accessing a valid region
      if ( foundRegion != REGION_NOT_FOUND )
      {
         // Did we leave the last detected region?
         if ( foundRegion != pTableData->currentRegion )
         {
            // New Region Enterd
            GSE_DebugStr ( VERBOSE, TRUE, "Table %d Entered:  R: %d S: %f D: %d",
                           pTableData->nTableIndex, foundRegion,
                           pTableData->fCurrentSensorValue, pTableData->nTotalDuration_ms );
            // We just entered this region record the time
            pTableData->regionStats[foundRegion].nEnteredTime = nCurrentTick;
         }

         // Have we confirmed the found region yet?
         if ( foundRegion != pTableData->confirmedRegion )
         {
            // Check if we have been in this region for more than the transient allowance
            EventTableConfirmRegion ( pTableCfg, pTableData, foundRegion, nCurrentTick );
         }
      }
      else // We exited all possible regions but are still in the table
      {
         // Exit the last confirmed region
         EventTableExitRegion ( pTableCfg, pTableData, foundRegion, nCurrentTick );
         // We are no longer in a confirmed region, set the confirmed to found
         pTableData->confirmedRegion = foundRegion;
      }
      // Set the current region to whatever we just found
      pTableData->currentRegion = foundRegion;
   }
   else
   {
      // Check if we already started the Event Table Processing
      if ( ( TRUE == pTableData->bStarted ) && ( 0 != pTableData->nTotalDuration_ms ) )
      {
         // The exceedance has ended
         CM_GetTimeAsTimestamp( &pTableData->tsExceedanceEndTime );
         // Calculate the final duration since the exceedance began
         pTableData->nTotalDuration_ms = nCurrentTick - pTableData->nStartTime_ms;
         // Exit any confirmed region that wasn't previously exited
         EventTableExitRegion ( pTableCfg,  pTableData, foundRegion, nCurrentTick );
         // Log the Event Table -
         EventTableLogSummary ( pTableCfg, pTableData, priority );
      }
      // Now reset the Started Flag and wait until we cross a region threshold
      pTableData->bStarted = FALSE;
   }
   return (pTableData->bStarted);
}

/******************************************************************************
 * Function:     EventTableFindRegion
 *
 * Description:  Event Table Find Region determines where in the table the
 *               sensor value currently resides.
 *
 * Parameters:   [in] EVENT_TABLE_CFG  *pTableCfg
 *               [in] EVENT_TABLE_DATA *pTableData
 *               [in] FLOAT32          fSensorValue
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static
EVENT_REGION EventTableFindRegion ( EVENT_TABLE_CFG *pTableCfg, EVENT_TABLE_DATA *pTableData,
                                    FLOAT32 fSensorValue)
{
   // Local Data
   UINT16        nRegIndex;
   UINT16        nSegIndex;
   REGION_DEF   *pRegion;
   SEGMENT_DEF  *pSegment;
   LINE_CONST   *pConst;
   EVENT_REGION  foundRegion;
   FLOAT32       fThreshold;
   FLOAT32       fSavedThreshold;

   // Initialize Local Data
   foundRegion     = REGION_NOT_FOUND;
   fThreshold      = 0;
   fSavedThreshold = 0;

   // Loop through all the regions
   for ( nRegIndex = 0; (nRegIndex <= pTableData->maximumCfgRegion); nRegIndex++ )
   {
      pRegion   = &pTableCfg->region[nRegIndex];

      // Check each configured line segment
      for ( nSegIndex = 0; (nSegIndex < MAX_REGION_SEGMENTS); nSegIndex++ )
      {
         pSegment = &pRegion->segment[nSegIndex];

         // Now check if this line segment is valid for this point in time
         if (( pTableData->nTotalDuration_ms >= pSegment->nStartTime_ms ) &&
             ( pTableData->nTotalDuration_ms <  pSegment->nStopTime_ms  ))
         {
            pConst    = &pTableData->segment[nRegIndex][nSegIndex];

            // We found the point in time for the Regions Line Segment now
            // Check if the sensor value is greater than the line segment at this point
            // Calculate the Threshold based on the constants for this segment
            fThreshold = (FLOAT32)((pConst->m *
                                    (FLOAT32)pTableData->nTotalDuration_ms) + pConst->b);

            // If we haven't already entered or exceeded this region then add a positive
            // Hysteresis Else we need a negetive Hysteresis to exit the region
            if ( (pTableData->currentRegion < (EVENT_REGION)nRegIndex) ||
                 (pTableData->currentRegion == REGION_NOT_FOUND) )
            {
               fThreshold += pTableCfg->fHysteresisPos;
            }
            else
            {
               fThreshold -= pTableCfg->fHysteresisNeg;
            }

            // Check if the sensor is greater than the threshold
            if ( fSensorValue > fThreshold )
            {
               foundRegion    = (EVENT_REGION)nRegIndex;
               fSavedThreshold = fThreshold;
            }
         }
      }
   }

   // If we found a new region then dump a debug message
   if (foundRegion != pTableData->currentRegion)
   {
      GSE_DebugStr(VERBOSE,TRUE,"Table %d FoundRegion: %d Dur: %d Th: %f S: %f",
                   pTableData->nTableIndex, foundRegion, pTableData->nTotalDuration_ms,
                   fSavedThreshold, fSensorValue);
   }
   return foundRegion;
}

/******************************************************************************
 * Function:    EventTableConfirmRegion
 *
 * Description: Determines if the Event Table Region has exceeded the
 *              transient allowance for entering the region. Once confirmed
 *              the logic will then exit the current region.
 *
 * Parameters:  [in]      EVENT_TABLE_CFG  *pTableCfg,
 *              [in/out]  EVENT_TABLE_DATA *pTableData,
 *              [in]      EVENT_REGION      foundRegion,
 *              [in]      UINT32            nCurrentTick
 *
 * Returns:      BOOLEAN
 *
 * Notes:
 *
 *****************************************************************************/
static
void EventTableConfirmRegion ( const EVENT_TABLE_CFG  *pTableCfg,
                               EVENT_TABLE_DATA *pTableData,
                               EVENT_REGION      foundRegion,
                               UINT32            nCurrentTick )
{
   // Local Data
   BOOLEAN bACKFlag;
   BOOLEAN bLatchFlag;

   // Have we exceeded the threshold for more than the transient allowance OR
   // Are we coming back from a higher region?
   if (((nCurrentTick - pTableData->regionStats[foundRegion].nEnteredTime) >=
         pTableCfg->nTransientAllowance_ms) ||
        (( foundRegion < pTableData->confirmedRegion ) &&
         (pTableData->confirmedRegion != REGION_NOT_FOUND)) ||
         ( foundRegion == REGION_NOT_FOUND ) )
   {
      // Has this region been confirmed yet?
      if ( FALSE == pTableData->regionStats[foundRegion].bRegionConfirmed )
      {
         // We now know we are in this region , either because we exceeded
         // the transient allowance or we have entered from a higher region
         pTableData->regionStats[foundRegion].bRegionConfirmed = TRUE;

         // Check for the maximum region entered
         if (( foundRegion > pTableData->maximumRegionEntered ) ||
             ( REGION_NOT_FOUND == pTableData->maximumRegionEntered))
         {
            pTableData->maximumRegionEntered = foundRegion;
         }

         // Count that we have entered this region
         pTableData->regionStats[foundRegion].logStats.nEnteredCount++;

         GSE_DebugStr ( NORMAL,TRUE,"Table %d Confirmed: R: %d S: %f D: %d",
                        pTableData->nTableIndex, foundRegion, pTableData->fCurrentSensorValue,
                        pTableData->nTotalDuration_ms );

         EventTableExitRegion ( pTableCfg, pTableData, foundRegion, nCurrentTick );

         bACKFlag   = (pTableCfg->region[foundRegion].nAction & EVENT_ACTION_ACK) ?
                    TRUE : FALSE;
         bLatchFlag = (pTableCfg->region[foundRegion].nAction & EVENT_ACTION_LATCH) ?
                    TRUE : FALSE;

         // Request the event action for the confirmed region
         pTableData->nActionReqNum = ActionRequest ( pTableData->nActionReqNum,
                          ( EVENT_ACTION_ON_DURATION(pTableCfg->region[foundRegion].nAction) |
                            EVENT_ACTION_ON_MET(pTableCfg->region[foundRegion].nAction) ),
                            ACTION_ON,
                            bACKFlag,
                            bLatchFlag );
      }
      // We have a new confirmed region now save it
      pTableData->confirmedRegion = foundRegion;
   }
}

/******************************************************************************
 * Function:     EventTableExitRegion
 *
 * Description:  The Event Table Exit Region closes out the statistics for
 *               the region being exited and then disables any event action
 *               since we are either transitioning to a new region or
 *               exiting the table processing.
 *
 * Parameters:   [in]     EVENT_TABLE_CFG  *pTableCfg
 *               [in/out] EVENT_TABLE_DATA *pTableData
 *               [in]     EVENT_REGION      foundRegion
 *               [in]     UINT32            nCurrentTick
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static
void EventTableExitRegion ( const EVENT_TABLE_CFG *pTableCfg,  EVENT_TABLE_DATA *pTableData,
                            EVENT_REGION foundRegion,   UINT32 nCurrentTick )
{
   // Local Data
   UINT16 i;
   UINT32 timeDelta;

   // Make sure we are in a confirmed region
   if ( REGION_NOT_FOUND != pTableData->confirmedRegion )
   {
      // was the confirmed region actually entered at least once?
      if ( 0 != pTableData->regionStats[pTableData->confirmedRegion].nEnteredTime )
      {
         // Count that we exited this region
         pTableData->regionStats[pTableData->confirmedRegion].logStats.nExitCount++;
         pTableData->regionStats[pTableData->confirmedRegion].bRegionConfirmed = FALSE;
         // To get the duration for the region we need to subtract the entered
         // time from the CurrentTick time and then subtract the transient
         // allowance for entering the new region... unless we are coming from
         // a higher region or foundRegion = NOT FOUND
         if (( REGION_NOT_FOUND == foundRegion ) ||
             ( foundRegion < pTableData->confirmedRegion ))
         {
            timeDelta = 0;
         }
         else
         {
            timeDelta = pTableCfg->nTransientAllowance_ms;
         }

         pTableData->regionStats[pTableData->confirmedRegion].logStats.nDuration_ms  +=
          ((nCurrentTick - pTableData->regionStats[pTableData->confirmedRegion].nEnteredTime) -
            timeDelta);
         // Reset the entered time so we don't keep performing this calculation
         pTableData->regionStats[pTableData->confirmedRegion].nEnteredTime = 0;

         GSE_DebugStr(NORMAL,TRUE,"Table %d Exit:      R: %d", pTableData->nTableIndex,
                                                               pTableData->confirmedRegion );
         EventTableLogTransition ( pTableCfg, pTableData,
                                   foundRegion, pTableData->confirmedRegion );

      }

      // Clear any other event actions
      for ( i = REGION_A; i < MAX_TABLE_REGIONS; i++ )
      {
         // Make sure we don't shut off the new found region.
         if ( (EVENT_REGION)i != foundRegion )
         {
            pTableData->nActionReqNum = ActionRequest ( pTableData->nActionReqNum,
                                       EVENT_ACTION_ON_DURATION(pTableCfg->region[i].nAction) |
                                       EVENT_ACTION_ON_MET(pTableCfg->region[i].nAction),
                                       ACTION_OFF,
                                       0,
                                       0 );
         }
      }
   }
   else if ( ( REGION_NOT_FOUND != foundRegion ) &&
             ( REGION_NOT_FOUND == pTableData->confirmedRegion ) )
   {
      EventTableLogTransition (pTableCfg, pTableData,
                               foundRegion, pTableData->confirmedRegion);
   }

}

/******************************************************************************
 * Function:     EventTableLogTransition
 *
 * Description:  Generates the log when regions are transitioned.
 *
 * Parameters:   [in] EVENT_TABLE_CFG   *pConfig
 *               [in] EVENT_TABLE_DATA  *pData
 *               [in] EVENT_TABLE_INDEX eventTableIndex
 *               [in] LOG_PRIORITY      priority
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static
void EventTableLogTransition ( const EVENT_TABLE_CFG *pTableCfg,  EVENT_TABLE_DATA *pTableData,
                               EVENT_REGION EnteredRegion, EVENT_REGION ExitedRegion)
{
   // Local Data
   EVENT_TABLE_TRANSITION_LOG transLog;

   // Build transition log
   transLog.eventTableIndex  = pTableData->nTableIndex;
   transLog.regionEntered    = EnteredRegion;
   transLog.regionExited     = pTableData->confirmedRegion;

   if (REGION_NOT_FOUND != ExitedRegion)
   {
      transLog.exitedStats  = pTableData->regionStats[ExitedRegion].logStats;
   }
   else
   {
      transLog.exitedStats.nDuration_ms  = 0;
      transLog.exitedStats.nEnteredCount = 0;
      transLog.exitedStats.nExitCount    = 0;
   }

   transLog.maximumRegionEntered     = pTableData->maximumRegionEntered;
   transLog.nSensorIndex             = pTableCfg->nSensor;
   transLog.fMaxSensorValue          = pTableData->fMaxSensorValue;
   transLog.nMaxSensorElaspedTime_ms = pTableData->nMaxSensorElaspedTime_ms;
   transLog.fCurrentSensorValue      = pTableData->fCurrentSensorValue;
   transLog.nDuration_ms             = pTableData->nTotalDuration_ms;
   // Write the transition Log
   LogWriteETM    ( APP_ID_EVENT_TABLE_TRANSITION,
                    LOG_PRIORITY_3,
                    &transLog,
                    sizeof(transLog),
                    NULL);
}


/******************************************************************************
 * Function:     EventTableLogSummary
 *
 * Description:  Gathers and records the Event Table Summary Data
 *
 * Parameters:   [in] EVENT_TABLE_CFG   *pConfig
 *               [in] EVENT_TABLE_DATA  *pData
 *               [in] EVENT_TABLE_INDEX eventTableIndex
 *               [in] LOG_PRIORITY      priority
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static
void EventTableLogSummary (const EVENT_TABLE_CFG *pConfig, const EVENT_TABLE_DATA *pData,
                           LOG_PRIORITY priority )
{
   // Local Data
   EVENT_TABLE_SUMMARY_LOG tSumm;
   UINT16                  nRegIndex;

   // Gather up the data to write to the log
   tSumm.eventTableIndex          = pData->nTableIndex;
   tSumm.sensorIndex              = pConfig->nSensor;
   tSumm.tsExceedanceStartTime    = pData->tsExceedanceStartTime;
   tSumm.tsExceedanceEndTime      = pData->tsExceedanceEndTime;
   tSumm.nDuration_ms             = pData->nTotalDuration_ms;
   tSumm.maximumRegionEntered     = pData->maximumRegionEntered;
   tSumm.fMaxSensorValue          = pData->fMaxSensorValue;
   tSumm.nMaxSensorElaspedTime_ms = pData->nMaxSensorElaspedTime_ms;
   tSumm.maximumCfgRegion         = pData->maximumCfgRegion;

   // Loop through the region stats and place them in the log
   for ( nRegIndex = 0; nRegIndex < (UINT16)MAX_TABLE_REGIONS; nRegIndex++ )
   {
      tSumm.regionStats[nRegIndex].nEnteredCount =
                                  pData->regionStats[nRegIndex].logStats.nEnteredCount;
      tSumm.regionStats[nRegIndex].nExitCount    =
                                  pData->regionStats[nRegIndex].logStats.nExitCount;
      tSumm.regionStats[nRegIndex].nDuration_ms  =
                                  pData->regionStats[nRegIndex].logStats.nDuration_ms;
   }

   // Write the LOG
   LogWriteETM    ( APP_ID_EVENT_TABLE_SUMMARY,
                    priority,
                    &tSumm,
                    sizeof(tSumm),
                    NULL);
}

/******************************************************************************
 * Function:     EventLogEnd
 *
 * Description:  Log the event end record
 *
 *
 * Parameters:   [in] EVENT_CFG   *pConfig,
 *               [in] EVENT_DATA  *pData
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static
void EventLogEnd (EVENT_CFG *pConfig, EVENT_DATA *pData)
{
   GSE_DebugStr(NORMAL,TRUE,"Event End (%d) %s", pData->eventIndex, pConfig->sEventName);

   //Log the data collected
   EventLogUpdate( pData );
   LogWriteETM    ( APP_ID_EVENT_ENDED,
                    pConfig->priority,
                    &m_EventEndLog[pData->eventIndex],
                    sizeof(EVENT_END_LOG),
                    NULL);
}


/******************************************************************************
 * Function:     EventUpdateData
 *
 * Description:  The Event Update Data is responsible for collecting the
 *               min/max/avg for all the event configured sensors.
 *
 * Parameters:   [in/out] EVENT_DATA *pData
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static
void EventUpdateData ( EVENT_DATA *pData )
{
   // Local Data
   SNSR_SUMMARY     *pSummary;
   UINT8            numSensor;
   UINT32           nCurrentTick;
   FLOAT32          oneOverN;

   // Initialize Local Data
   nCurrentTick = CM_GetTickCount();

   // if duration == 0 the Event needs to be initialized
   if ( 0 == pData->nStartTime_ms )
   {
      // general Event initialization
      pData->nStartTime_ms   = nCurrentTick;
      pData->nSampleCount    = 0;
      CM_GetTimeAsTimestamp(&pData->tsCriteriaMetTime);
      memset((void *)&pData->tsDurationMetTime,0, sizeof(TIMESTAMP));
      memset((void *)&pData->tsEndTime        ,0, sizeof(TIMESTAMP));
   }
   // update the sample count for the average calculation
   pData->nSampleCount++;

   // Loop through the Event Sensors
   for ( numSensor = 0; numSensor < pData->nTotalSensors; numSensor++ )
   {
      // Set a pointer to the data summary
      pSummary    = &(pData->sensor[numSensor]);

      // If the sensor is known to be invalid and was valid in the past(initialized)...
      // ignore processing for the remainder of this event.
      if( !pSummary->bValid && pSummary->bInitialized )
      {
         continue;
      }

      pSummary->bValid = SensorIsValid(pSummary->SensorIndex);

      if ( pSummary->bValid )
      {
         pSummary->bInitialized = TRUE;
         SensorUpdateSummaryItem(pSummary);
      }
      else if ( pSummary->bInitialized)
      {
         // Sensor is now invalid but had been valid
         // calculate average for valid period.
         oneOverN = (1.0f / (FLOAT32)(pData->nSampleCount - 1));
         pSummary->fAvgValue = pSummary->fTotal * oneOverN;
      }
   }
}

/******************************************************************************
 * Function:     EventCheckDuration
 *
 * Description:  The event check duration function determines if the configured
 *               duration has been exceeded or not.
 *
 * Parameters:   [in] EVENT_CFG  *pConfig,
 *               [in] EVENT_DATA *pData
 *
 * Returns:      TRUE  - if duration exceeded
 *               FALSE - if duration not exceeded
 *
 * Notes:        None.
 *
 *****************************************************************************/
static
BOOLEAN EventCheckDuration ( const EVENT_CFG *pConfig, EVENT_DATA *pData )
{
   // Local Data
   BOOLEAN bExceed;

   // update the Event duration
   pData->nDuration_ms = CM_GetTickCount() - pData->nStartTime_ms;

   bExceed = ( pData->nDuration_ms >= pConfig->nMinDuration_ms ) ? TRUE : FALSE;

   return ( bExceed );
}

/******************************************************************************
 * Function:     EventLogUpdate
 *
 * Description:  The event log update function creates a log
 *               containing the event data.
 *
 * Parameters:   [in] EVENT_DATA *pData
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static
void EventLogUpdate( EVENT_DATA *pData )
{
  // Local Data
  FLOAT32       oneOverN;
  EVENT_END_LOG *pLog;
  SNSR_SUMMARY  *pSummary;
  UINT8         numSensor;

  // Initialize Local Data
  pLog = &m_EventEndLog[pData->eventIndex];

  // Calculate the multiplier for the average
  oneOverN = (1.0f / (FLOAT32)pData->nSampleCount);

  // update the Event duration
  pLog->nDuration_ms  = CM_GetTickCount() - pData->nStartTime_ms;

  // Update the Event end log
  pLog->nEventIndex        = (UINT16)pData->eventIndex;
  pLog->endType            = pData->endType;
  pLog->tsCriteriaMetTime  = pData->tsCriteriaMetTime;
  pLog->tsDurationMetTime  = pData->tsDurationMetTime;
  pLog->tsEndTime          = pData->tsEndTime;
  pLog->nTotalSensors      = pData->nTotalSensors;

  // Loop through all the sensors
  for ( numSensor = 0; numSensor < pData->nTotalSensors; numSensor++ )
  {
    // Set a pointer the summary
    pSummary = &(pData->sensor[numSensor]);

    // Check the sensor is used
    if ( SensorIsUsed((SENSOR_INDEX)pSummary->SensorIndex ) )
    {
       pSummary->bValid = SensorIsValid((SENSOR_INDEX)pSummary->SensorIndex);
       // Update the summary data for the trigger
       pLog->sensor[numSensor].SensorIndex = pSummary->SensorIndex;
       pLog->sensor[numSensor].timeMinValue = pSummary->timeMinValue;
       pLog->sensor[numSensor].fMinValue   = pSummary->fMinValue;
       pLog->sensor[numSensor].timeMaxValue = pSummary->timeMaxValue;
       pLog->sensor[numSensor].fMaxValue   = pSummary->fMaxValue;
       pLog->sensor[numSensor].bValid      = pSummary->bValid;
       // if sensor is valid, calculate the final average,
       // otherwise use the average calculated at the point it went invalid.
       pLog->sensor[numSensor].fAvgValue   = (TRUE == pSummary->bValid ) ?
                                              pSummary->fTotal * oneOverN :
                                              pSummary->fAvgValue;
    }
    else // Sensor Not Used
    {
       pLog->sensor[numSensor].SensorIndex   = SENSOR_UNUSED;
       pLog->sensor[numSensor].fMinValue     = 0.0;
       pLog->sensor[numSensor].fMaxValue     = 0.0;
       pLog->sensor[numSensor].fAvgValue     = 0.0;
       pLog->sensor[numSensor].bValid        = FALSE;
    }
  }
}

/******************************************************************************
 * Function:     EventForceEnd
 *
 * Description:  Force all active events to end.
 *
 * Parameters:   None.
 *
 * Returns:      None.
 *
 * Notes:
 *
 *****************************************************************************/
static
void EventForceEnd ( void )
{
   // Local Data
   UINT8 i;
   EVENT_CFG  *pEventCfg;
   EVENT_DATA *pEventData;

   // Loop through all the events
   for ( i = 0; i < MAX_EVENTS; i++ )
   {
      // Set the pointers to the configuration and data
      pEventCfg  = &m_EventCfg[i];
      pEventData = &m_EventData[i];

      // Check if the event is active
      if ( EVENT_ACTIVE == pEventData->state )
      {
         // Command the event table to end
         EventForceTableEnd (pEventCfg->eventTableIndex, pEventCfg->priority );
         // Record the End Time for the event
         CM_GetTimeAsTimestamp ( &pEventData->tsEndTime );
         // Record the type of end for the event
         pEventData->endType = EVENT_COMMANDED_END;
         // Populate the End log data
         EventLogUpdate ( pEventData );
         // Write the Log
         LogWriteETM    ( APP_ID_EVENT_ENDED,
                          pEventCfg->priority,
                          &m_EventEndLog[pEventData->eventIndex],
                          sizeof ( EVENT_END_LOG ),
                          NULL );
      }
   }
}

/******************************************************************************
 * Function:     EventForceTableEnd
 *
 * Description:  Force the event table to end.
 *
 * Parameters:   [in] EVENT_TABLE_INDEX eventTableIndex
 *               [in] LOG_PRIORITY      prioirity
 *
 * Returns:      None.
 *
 * Notes:        None
 *
 *****************************************************************************/
static
void EventForceTableEnd ( EVENT_TABLE_INDEX eventTableIndex, LOG_PRIORITY priority )
{
   // Local Data
   EVENT_TABLE_CFG  *pTableCfg;
   EVENT_TABLE_DATA *pTableData;

   // Initialize Local Data
   pTableCfg    = &m_EventTableCfg [eventTableIndex];
   pTableData   = &m_EventTableData[eventTableIndex];

   // Check if we already started the Event Table Processing
   if ( ( TRUE == pTableData->bStarted ) && ( 0 != pTableData->nTotalDuration_ms ) )
   {
      // The exceedance has ended
      CM_GetTimeAsTimestamp( &pTableData->tsExceedanceEndTime );
      // Exit any confirmed region
      EventTableExitRegion ( pTableCfg,  pTableData, pTableData->confirmedRegion,
                             CM_GetTickCount());
      // Log the Event Table
      EventTableLogSummary ( pTableCfg, pTableData, priority );
   }
   // Now reset the Started Flag
   pTableData->bStarted = FALSE;
}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: Event.c $
 *
 * *****************  Version 28  *****************
 * User: John Omalley Date: 12-10-16   Time: 2:37p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Updates per Design Review
 * 1. Changed segment times to milliseconds
 * 2. Limit +/- hysteresis to positive values
 *
 * *****************  Version 27  *****************
 * User: Melanie Jutras Date: 12-10-10   Time: 12:17p
 * Updated in $/software/control processor/code/application
 * SCR 1172 PCLint 545 Suspicious use of & Error
 *
 * *****************  Version 26  *****************
 * User: John Omalley Date: 12-09-19   Time: 10:54a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Added Time History enable configuration
 *
 * *****************  Version 25  *****************
 * User: Contractor V&v Date: 9/14/12    Time: 4:04p
 * Updated in $/software/control processor/code/application
 * FAST 2 fixes for sensor list
 *
 * *****************  Version 24  *****************
 * User: John Omalley Date: 12-09-13   Time: 9:41a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Fixed the event table logic that I broke with previous
 * checkin
 *
 * *****************  Version 23  *****************
 * User: John Omalley Date: 12-09-12   Time: 3:59p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Updated how Event Data gets reset
 *
 * *****************  Version 22  *****************
 * User: John Omalley Date: 12-09-11   Time: 1:56p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Add ETM Binary Header
 *                    Updated Time History Calls to new prototypes
 *
 * *****************  Version 21  *****************
 * User: Contractor V&v Date: 8/29/12    Time: 2:53p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 !P Processing
 *
 * *****************  Version 20  *****************
 * User: John Omalley Date: 12-08-20   Time: 9:00a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Bit Bucket Issues Cleanup
 *
 * *****************  Version 19  *****************
 * User: John Omalley Date: 12-08-16   Time: 4:16p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Block Action Request for OFF State if the Event never
 * started.
 *
 * *****************  Version 18  *****************
 * User: John Omalley Date: 12-08-14   Time: 2:54p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Code Review Updates
 *
 * *****************  Version 17  *****************
 * User: John Omalley Date: 12-08-13   Time: 4:22p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Log Cleanup
 *
 * *****************  Version 16  *****************
 * User: John Omalley Date: 12-08-09   Time: 8:38a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Fixed code to properly implement requirements
 *
 * *****************  Version 15  *****************
 * User: John Omalley Date: 12-07-27   Time: 3:03p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Action Manager Persistent Updates
 *
 * *****************  Version 14  *****************
 * User: John Omalley Date: 12-07-19   Time: 5:09p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Cleaned up Code Review Tool findings
 *
 * *****************  Version 13  *****************
 * User: Contractor V&v Date: 7/18/12    Time: 6:24p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 Refactor for common Sensor Summary
 *
 * *****************  Version 12  *****************
 * User: John Omalley Date: 12-07-17   Time: 11:26a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Removed the Event Action and made an Action object
 *
 * *****************  Version 11  *****************
 * User: John Omalley Date: 12-07-16   Time: 1:07p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Event Table Processing Update
 *
 * *****************  Version 10  *****************
 * User: John Omalley Date: 12-07-16   Time: 11:37a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Event Table Updates
 *
 * *****************  Version 9  *****************
 * User: John Omalley Date: 12-07-13   Time: 3:51p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Code updates, Event Action added, refactoring event table
 * logic
 *
 * *****************  Version 8  *****************
 * User: John Omalley Date: 12-05-24   Time: 9:39a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Added the ETM Write Function
 *
 * *****************  Version 7  *****************
 * User: John Omalley Date: 12-05-22   Time: 2:16p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - File Clean up
 *
 * *****************  Version 6  *****************
 * User: John Omalley Date: 12-05-22   Time: 8:40a
 * Updated in $/software/control processor/code/application
 * SCR 1107
 * Commented Code and updated headers.
 * Fixed user table to use ENUM for Region Table
 *
 * *****************  Version 5  *****************
 * User: John Omalley Date: 5/15/12    Time: 11:57a
 * Updated in $/software/control processor/code/application
 * SCR 1107
 *
 * Event Table Processing Updates:
 * 1. Hysteresis Working
 * 2. Transient Allowance Working
 * 3. Log Recording
 *    a. Duration for each region
 *    b. Enter and Exit counts
 *    c. Max Region Entered
 *    d. Max Sensor Value
 *
 * *****************  Version 4  *****************
 * User: John Omalley Date: 5/07/12    Time: 10:05a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Added Event Table Hysteresis and Transient Allowance on a
 * per Table basis
 *
 * *****************  Version 3  *****************
 * User: John Omalley Date: 5/02/12    Time: 4:53p
 * Updated in $/software/control processor/code/application
 * SCR 1107
 *
 * *****************  Version 2  *****************
 * User: John Omalley Date: 4/27/12    Time: 5:04p
 * Updated in $/software/control processor/code/application
 *
 * *****************  Version 1  *****************
 * User: John Omalley Date: 4/24/12    Time: 10:59a
 * Created in $/software/control processor/code/application
 *
 *
 *
 ***************************************************************************/
 
