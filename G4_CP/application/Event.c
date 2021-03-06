#define EVENT_BODY
/******************************************************************************
           Copyright (C) 2012-2015 Pratt & Whitney Engine Services, Inc.
              All Rights Reserved. Proprietary and Confidential.

   ECCN:        9D991

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
 $Revision: 48 $  $Date: 4/14/15 2:31p $

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
#include "box.h"
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
static EVENT_TABLE_DATA  m_EventTableData [MAX_TABLES];     // Runtime event table data

static EVENT_END_LOG     m_EventEndLog    [MAX_EVENTS];     // Event Log Container

static EVENT_CFG         m_EventCfg       [MAX_EVENTS];     // Event Cfg Array
static EVENT_TABLE_CFG   m_EventTableCfg  [MAX_TABLES];     // Event Table Cfg Array

static UINT32            nEventSeqCounter;                  // Sequence number
static UINT32            m_EventActiveFlags;                // Event Active Flags

static EVENT_HIST_BUFFER m_EventHistory;                    // Runtime copy of hist-buff file

/*Busy/Not Busy event data*/
static INT32 m_event_tag;
static void (*m_event_func)(INT32 tag, BOOLEAN is_busy) = NULL;

#include "EventUserTables.c"

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
/*------------------------------- EVENTS ------------------------------------*/
static void           EventsUpdateTask       ( void *pParam    );
static void           EventsEnableTask       ( BOOLEAN bEnable );
static void           EventProcess           ( EVENT_CFG *pConfig,       EVENT_DATA *pData   );
static void           EventProcessStartState ( EVENT_CFG *pConfig,       EVENT_DATA *pData,
                                               UINT32 nCurrentTick );
static void           EventProcessActiveState( EVENT_CFG *pConfig,       EVENT_DATA *pData,
                                               UINT32 nCurrentTick );
static BOOLEAN        EventCheckDuration     ( const EVENT_CFG *pConfig, EVENT_DATA *pData   );
static BOOLEAN        EventCheckStart        ( const EVENT_CFG *pConfig,
                                               const EVENT_DATA *pData,
                                               BOOLEAN *isValid );
static EVENT_END_TYPE EventCheckEnd          ( const EVENT_CFG *pConfig,
                                               const EVENT_DATA *pData );
static void           EventResetData         ( EVENT_DATA *pData   );
static void           EventUpdateData        ( EVENT_DATA *pData  );
static void           EventLogStart          ( EVENT_CFG  *pConfig, const EVENT_DATA *pData  );
static void           EventLogUpdate         ( EVENT_DATA *pData  );
static void           EventLogEnd            ( EVENT_CFG  *pConfig, EVENT_DATA *pData   );
static void           EventForceEnd          ( void );
/*-------------------------- EVENT TABLES ----------------------------------*/
static void           EventTablesInitialize    ( void );
static void           EventTableResetData      ( EVENT_TABLE_DATA *pTableData );
static EVENT_TABLE_STATE EventTableUpdate         ( EVENT_TABLE_INDEX eventTableIndex,
                                                 UINT32 nCurrentTick,
                                                 LOG_PRIORITY priority,
                                                 UINT32 seqNumber );
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
                                                 const EVENT_TABLE_DATA *pTableData,
                                                 EVENT_REGION EnteredRegion,
                                                 EVENT_REGION ExitedRegion );
static void           EventTableLogSummary     ( const EVENT_TABLE_CFG  *pConfig,
                                                 const EVENT_TABLE_DATA *pData,
                                                 LOG_PRIORITY      priority,
                                                 EVENT_TABLE_STATE endReason );
static void           EventForceTableEnd       ( EVENT_TABLE_INDEX eventTableIndex,
                                                 LOG_PRIORITY priority );
static BOOLEAN        EventTableIsEntered      ( EVENT_TABLE_CFG *pTableCfg,
                                                 EVENT_TABLE_DATA *pTableData,
                                                 UINT32 nCurrentTick,
                                                 BOOLEAN bSensorValid);
static BOOLEAN        EventTableIsExited       ( EVENT_TABLE_CFG *pTableCfg,
                                                 EVENT_TABLE_DATA *pTableData,
                                                 BOOLEAN bSensorValid,
                                                 BOOLEAN bEntered);
/*-------------------------- EVENT HISTORY ----------------------------------*/
static void           EventUpdateHistory       ( const EVENT_CFG  *pConfig,
                                                 const EVENT_DATA   *pData);

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
   UINT32     i;

   EVENT_CFG  *pEventCfg;
   EVENT_DATA *pEventData;

   RESULT result;

   // Add Trigger Tables to the User Manager
   User_AddRootCmd(&rootEventMsg);

   // Reset the Sensor configuration storage array
   memset((void *)m_EventCfg, 0, sizeof(m_EventCfg));

   // Load the current configuration to the configuration array.
   memcpy((void *)m_EventCfg,
          (const void *)(CfgMgr_RuntimeConfigPtr()->EventConfigs),
          sizeof(m_EventCfg));

   nEventSeqCounter   = 0;
   m_EventActiveFlags = 0;

   // Loop through the Configured triggers
   for ( i = 0; i < MAX_EVENTS; i++)
   {
      // Set a pointer to the current event
      pEventCfg = &m_EventCfg[i];

      // Set a pointer to the runtime data table
      pEventData = &m_EventData[i];

      // Clear the runtime data table
      pEventData->eventIndex        = (EVENT_INDEX)i;
      pEventData->seqNumber         = ((Box_GetPowerOnCount() & 0xFFFF) << 16);
      pEventData->state             = EVENT_NONE;
      pEventData->nStartTime_ms     = 0;
      pEventData->nDuration_ms      = 0;
      pEventData->nRateCounts       = (UINT16)(MIFs_PER_SECOND / (UINT16)pEventCfg->rate);
      pEventData->nRateCountdown    = (UINT16)((pEventCfg->nOffset_ms / MIF_PERIOD_mS) + 1);
      pEventData->nActionReqNum     = ACTION_NO_REQ;
      pEventData->endType           = EVENT_NO_END;
      pEventData->bStarted          = FALSE;
      //pEventData->bTableWasEntered  = FALSE;

      // If the event has a Start and End Expression then it must be configured
      if ( (0 != pEventCfg->startExpr.size) && (0 != pEventCfg->endExpr.size) )
      {
         #pragma ghs nowarning 1545 //Suppress packed structure alignment warning
         pEventData->nTotalSensors = SensorInitSummaryArray ( pEventData->sensor,
                                                              MAX_EVENT_SENSORS,
                                                              pEventCfg->sensorMap,
                                                              sizeof(pEventCfg->sensorMap) );
         #pragma ghs endnowarning
         // Reset the events run time data
         EventResetData ( pEventData );
      }
   }

   // Initialize the Event Tables
   EventTablesInitialize();

   // NV_EVENT_HISTORY init / read

   // Open EE event history buffer,
   // If it contains default EEPROM, values, init the file
   result = NV_Open(NV_EVENT_HISTORY);
   if ( SYS_OK != result )
   {
     //GSE_DebugStr(NORMAL, TRUE, "EventBuff Open fail.. Initialize");
     // clear and reset the history buffer
     EventInitHistoryBuffer();
   }
   else
   {
     // read the data into local working buffer
     NV_Read( NV_EVENT_HISTORY, 0, &m_EventHistory, sizeof( m_EventHistory));
   }

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
   UINT32     eventIndex;
   INT8       *pBuffer;
   UINT16     nRemaining;
   UINT16     nTotal;
   UINT16     nSize;

   // Initialize Local Data
   pBuffer    = (INT8 *)pDest;
   nRemaining = nMaxByteSize;
   nTotal     = 0;
   nSize      = sizeof(EVENT_HDR);
   memset ( eventHdr, 0, sizeof(eventHdr) );

   // Loop through all the events
   for ( eventIndex = 0;
         ((eventIndex < MAX_EVENTS) && (nRemaining > nSize));
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
      nTotal     += nSize;
      nRemaining -= nSize;
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
   UINT32          tableIndex;
   INT8            *pBuffer;
   UINT16          nRemaining;
   UINT16          nTotal;
   UINT16          nSize;

   // Initialize Local Data
   pBuffer    = (INT8 *)pDest;
   nRemaining = nMaxByteSize;
   nTotal     = 0;
   nSize      = sizeof (EVENT_TABLE_HDR);
   memset ( tableHdr, 0, sizeof(tableHdr) );

   // Loop through all the event tables
   for ( tableIndex = 0;
         ((tableIndex < MAX_TABLES) && (nRemaining > nSize));
         tableIndex++ )
   {
      tableHdr[tableIndex].nSensor          = m_EventTableCfg[tableIndex].nSensor;
      tableHdr[tableIndex].fTableEntryValue = m_EventTableCfg[tableIndex].fTableEntryValue;
      // Increment the total number of bytes and decrement the remaining
      nTotal     += nSize;
      nRemaining -= nSize;
   }
   // Copy the event table header to the buffer
   memcpy ( pBuffer, tableHdr, nTotal );
   // Return the total number of bytes written
   return ( nTotal );
}

/**********************************************************************************************
 * Function:    EventSetRecStateChangeEvt
 *
 * Description: Set the callback function to call when the busy/not busy status of the
 *              for all events
  *
 * Parameters:  [in] tag:  Integer value to use when calling "func"
 *              [in] func: Function to call when busy/not busy status changes
 *
 * Returns:      none
 *
 * Notes:       Only remembers one event handler.  Subsequent calls overwrite the last
 *              event handler.
 *
 *********************************************************************************************/
void EventSetRecStateChangeEvt(INT32 tag,void (*func)(INT32,BOOLEAN))
{
  m_event_func = func;
  m_event_tag  = tag;
}

/******************************************************************************
 * Function:    EventInitHistoryBuffer
 *
 * Description: Re-initialize the event history buffer in EEPROM.
 *
 * Parameters: None
 *
 * Returns:    TRUE
 * Notes:
 *
 *****************************************************************************/
BOOLEAN EventInitHistoryBuffer( void )
{
  UINT32 i;
  // Clear the event history buffer.
  // Specifically set the maxRegion to not-found/not-applicable
  for (i = 0;i < MAX_EVENTS; ++i)
  {
    memset( (void*)&m_EventHistory.histData[i].tsEvent, 0, sizeof(TIMESTAMP));
    m_EventHistory.histData[i].nCnt      = 0;
    m_EventHistory.histData[i].maxRegion = REGION_NOT_FOUND;
  }

  // Set the 'last cleared' time to now.
  CM_GetTimeAsTimestamp(&m_EventHistory.tsLastCleared);

  // Write the event history buffer back to EEPROM
  //GSE_DebugStr(NORMAL, TRUE, "EventBuff Initialized, writing to EEPROM");
  NV_Write( NV_EVENT_HISTORY, 0, &m_EventHistory, sizeof(m_EventHistory) );

  return TRUE;

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
void EventResetData ( EVENT_DATA *pData )
{
   // Reinit the Event data
   pData->state         = EVENT_START;
   pData->nStartTime_ms = 0;
   pData->nDuration_ms  = 0;
   memset ( &pData->tsCriteriaMetTime, 0, sizeof(pData->tsCriteriaMetTime) );
   memset ( &pData->tsDurationMetTime, 0, sizeof(pData->tsDurationMetTime) );
   memset ( &pData->tsEndTime, 0, sizeof(pData->tsEndTime) );

   pData->endType       = EVENT_NO_END;

   // Re-init the sensor summary data for the event
   SensorResetSummaryArray (pData->sensor, pData->nTotalSensors);

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
static
void EventTablesInitialize ( void )
{
   // Local Data
   EVENT_TABLE_DATA *pTableData;
   EVENT_TABLE_CFG  *pTableCfg;
   REGION_DEF       *pReg;
   SEGMENT_DEF      *pSeg;
   LINE_CONST       *pConst;
   UINT32           nTableIndex;
   UINT32           nRegionIndex;
   UINT32           nSegmentIndex;

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
      pTableData->seqNumber        = 0;
      pTableData->maximumCfgRegion = REGION_NOT_FOUND;
      pTableData->nActionReqNum    = ACTION_NO_REQ;

      // Loop through all the Regions
      for ( nRegionIndex = (UINT32)REGION_A;
            nRegionIndex < (UINT32)MAX_TABLE_REGIONS; nRegionIndex++ )
      {
         // Set a pointer the Region Cfg
         pReg = &pTableCfg->regCfg.region[nRegionIndex];

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
      pTableData->nTblHystStartTime_ms = 0;
      memset((void *)&pTableData->tsTblHystStartTime,
              0,
              sizeof(pTableData->tsTblHystStartTime));
      pTableData->maximumRegionEntered = REGION_NOT_FOUND;
      pTableData->currentRegion        = REGION_NOT_FOUND;
      pTableData->confirmedRegion      = REGION_NOT_FOUND;
   }
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
 * Notes:        This function does not clear ALL fields in EVENT_TABLE_DATA!
 *
 *
 *****************************************************************************/
static
void EventTableResetData ( EVENT_TABLE_DATA *pTableData )
{
   //Local Data
   UINT32           nRegionIndex;
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
   for (nRegionIndex = (UINT32)REGION_A;
        nRegionIndex < (UINT32)MAX_TABLE_REGIONS; nRegionIndex++)
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
   UINT32      nEventIndex;
   EVENT_CFG  *pConfig;
   EVENT_DATA *pData;
   UINT32         activeMask = 0xFFFF;
   static BOOLEAN is_busy = FALSE;
   BOOLEAN        bAnyActive;

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
         if ( pConfig->startExpr.size != 0 )
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

   // Check if any events are active
   bAnyActive = TestBits( &activeMask, sizeof(activeMask),
                          &m_EventActiveFlags, sizeof(m_EventActiveFlags),
                          FALSE );

   // is_busy is a static initialized to FALSE, this logic will
   // act like a flip-flop and only update the busy/not busy status on
   // a state change.
   if ( bAnyActive != is_busy )
   {
      is_busy = bAnyActive;

      if ( m_event_func != NULL )
      {
         m_event_func( m_event_tag, is_busy );
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
   UINT32   nCurrentTick;

   // Intialize Local Data
   nCurrentTick   = CM_GetTickCount();

   switch (pData->state)
   {
      // Event start is looking for beginning of all start triggers
      case EVENT_START:
            EventProcessStartState( pConfig, pData, nCurrentTick );
         break;

      // Duration has been met or the table threshold was crossed
      // now the event is considered active
      case EVENT_ACTIVE:
            EventProcessActiveState( pConfig, pData, nCurrentTick );
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
 * Function:     EventProcessStartState
 *
 * Description:  The Event Process Start State function performs the logic to
 *               determine when an event is starting.
 *
 * Parameters:   [in]     EVENT_CFG  *pConfig
 *               [in/out] EVENT_DATA *pData
 *               [in]     UINT32     nCurrentTick
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static
void EventProcessStartState ( EVENT_CFG *pConfig, EVENT_DATA *pData, UINT32 nCurrentTick )
{
   // Local Data
   BOOLEAN  bStartCriteriaMet;
   BOOLEAN  bIsStartValid;

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
         // OR sequence number with the box power-on count stored during initialization
         nEventSeqCounter++;
         pData->seqNumber &= 0xFFFF0000;
         pData->seqNumber |= (nEventSeqCounter & 0xFFFF);

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
         EventResetData( pData );
      }
      // Since we just started begin collecting data
      EventUpdateData ( pData );

      // If this is a TABLE EVENT, then perform table processing
      if ( EVENT_TABLE_UNUSED != pConfig->eventTableIndex )
      {
         pData->tableState = EventTableUpdate ( pConfig->eventTableIndex,
                                                nCurrentTick,
                                                pConfig->priority,
                                                pData->seqNumber );

         // Doesn't matter if the table is entered or not, the event is now
         // considered active because the start criteria for the simple event is
         // met and there is no duration for events that have an event table
         pData->state = EVENT_ACTIVE;
         SetBit((INT32)pData->eventIndex, &m_EventActiveFlags, sizeof(m_EventActiveFlags));

      }
      // This is a SIMPLE EVENT, check if we have exceeded the cfg duration
      else if ( TRUE == EventCheckDuration ( pConfig, pData ) )
      {
         // Duration was exceeded the event is now active
         pData->state = EVENT_ACTIVE;
         SetBit((INT32)pData->eventIndex, &m_EventActiveFlags, sizeof(m_EventActiveFlags));

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
}

/******************************************************************************
 * Function:     EventProcessActiveState
 *
 * Description:  The Event Process Active State function performs Active state
 *               logic.
 *
 * Parameters:   [in]     EVENT_CFG  *pConfig
 *               [in/out] EVENT_DATA *pData
 *               [in]     UINT32     nCurrentTick
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static
void EventProcessActiveState ( EVENT_CFG *pConfig, EVENT_DATA *pData, UINT32 nCurrentTick )
{
   // keep updating the cfg parameters for the event
   EventUpdateData( pData );

   // Perform table processing if configured for EVENT TABLE
   if ( EVENT_TABLE_UNUSED != pConfig->eventTableIndex )
   {
      //bTableEntered
      pData->tableState = EventTableUpdate ( pConfig->eventTableIndex,
                                             nCurrentTick,
                                             pConfig->priority,
                                             pData->seqNumber );
      if (ET_IN_TABLE == pData->tableState)
      {
         pData->bTableWasEntered = TRUE;
      }
   }

   // Check if the end criteria has been met
   pData->endType = EventCheckEnd(pConfig, pData);

   // Check if any end criteria has happened and/or the table is done
   if ( ((EVENT_NO_END != pData->endType) &&
         (EVENT_TABLE_UNUSED == pConfig->eventTableIndex)) ||
        ((EVENT_NO_END != pData->endType) && (ET_IN_TABLE != pData->tableState) ) ||
        ((EVENT_TABLE_UNUSED != pConfig->eventTableIndex) &&
         (ET_IN_TABLE != pData->tableState) && (TRUE == pData->bTableWasEntered)) )
   {
      if ((ET_IN_TABLE != pData->tableState) && (TRUE == pData->bTableWasEntered) &&
          (EVENT_NO_END == pData->endType))
      {
         pData->endType = (pData->tableState == ET_SENSOR_INVALID) ?
                            EVENT_TABLE_SENSOR_INVALID : EVENT_TABLE_END;
      }

      // If this event is enabled for historybuffering,
      // update buffer and write it out.
      // Do this here before pData state flags are cleared!
      if (TRUE == pConfig->bEnableBuff)
      {
        EventUpdateHistory(pConfig, pData);
      }

      // Record the time the event ended
      CM_GetTimeAsTimestamp(&pData->tsEndTime);
      // Log the End
      EventLogEnd(pConfig, pData);
      // Change event state back to start
      pData->state            = EVENT_START;
      // Reset Event Active flag
      ResetBit((INT32)pData->eventIndex, &m_EventActiveFlags, sizeof(m_EventActiveFlags));
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
}

/******************************************************************************
 * Function:     EventCheckStart
 *
 * Description:  Checks the start of an event. This function is the entry point
 *               for determining start event criteria.
 *
 *
 * Parameters:   [in]  EVENT_CFG    *pConfig,
 *               [in]  EVENT_DATA *pData
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
                                                     (INT32)pData->eventIndex,
                                                     &pConfig->startExpr,
                                                     isValid );

   return bStartCriteriaMet;
}

/******************************************************************************
 * Function:     EventCheckEnd
 *
 * Description:  Check if event end criteria are met
 *
 * Parameters:   [in] EVENT_CONFIG *pConfig
 *               [in] EVENT_DATA   *pData
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
                          (INT32)pData->eventIndex,
                          &pConfig->endExpr,
                          &validity ) && (TRUE == validity))
   {
      endType = EVENT_END_CRITERIA;
   }
   else if (FALSE == validity)
   {
      endType = EVENT_END_CRITERIA_INVALID;
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
   EVENT_START_LOG  startLog;

   // Initialize Local Data
   startLog.nEventIndex = pData->eventIndex;
   startLog.seqNumber   = pData->seqNumber;

   LogWriteETM    (APP_ID_EVENT_STARTED,
                   pConfig->priority,
                   &startLog,
                   sizeof(startLog),
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
 *               [in] UINT32            seqNumber
 *
 * Returns:      TRUE  - if the table was entered
 *               FALSE - if the table was not entered
 *
 * Notes:        None
 *
 *****************************************************************************/
static
EVENT_TABLE_STATE EventTableUpdate ( EVENT_TABLE_INDEX eventTableIndex, UINT32 nCurrentTick,
                                     LOG_PRIORITY priority, UINT32 seqNumber )
{
   // Local Data
   EVENT_TABLE_CFG   *pTableCfg;
   EVENT_TABLE_DATA  *pTableData;
   EVENT_REGION      foundRegion;
   BOOLEAN           bSensorValid;
   BOOLEAN           bHystEntryMet;
   BOOLEAN           bHystExitMet;
   EVENT_TABLE_STATE tableState;

   // Initialize Local Data
   pTableCfg    = &m_EventTableCfg [eventTableIndex];
   pTableData   = &m_EventTableData[eventTableIndex];
   foundRegion  = REGION_NOT_FOUND;
   tableState   = ET_SENSOR_VALUE_LOW;

   // If we haven't entered the table but we are processing then clear the data
   // NOTE: This call does not completely reset table data; hysteresis timer is left intact
   if ( FALSE == pTableData->bStarted)
   {
      EventTableResetData ( pTableData );
   }

   // Get Sensor Value
   pTableData->fCurrentSensorValue = SensorGetValue( pTableCfg->nSensor );
   bSensorValid = SensorIsValid( pTableCfg->nSensor );


   // Hysteresis Entry/Exit Interlock
   // Entry test is only performed when table is NOT started.
   // Exit  test is only performed when table IS started.
   // Exit can only be True after entry is True.
   if ( FALSE == pTableData->bStarted )
   {
     // Table not started/entered, perform TableEntry Hysteresis processing.
     // Since table is not started, it can't be exited in this frame.
     bHystEntryMet = EventTableIsEntered(pTableCfg, pTableData, nCurrentTick, bSensorValid);
     bHystExitMet  = FALSE;
   }
   else
   {
     // Table was started so entry hysteresis/transient-allowance already met... check for exit
     bHystEntryMet = TRUE;
     bHystExitMet  = EventTableIsExited(pTableCfg, pTableData, bSensorValid, bHystEntryMet);
   }

   // Don't do anything until the table is entered and sensor is valid and above the
   // exit hysteresis limit

   if ( (TRUE == bHystEntryMet) &&
        (FALSE == bHystExitMet) &&
        ( pTableData->fCurrentSensorValue > pTableCfg->fTableEntryValue ) &&
        ( TRUE == bSensorValid ) )
   {
      tableState = ET_IN_TABLE;

      // Check if this Table is starting now (i.e. bStarted was FALSE, is TRUE)
      if ( FALSE == pTableData->bStarted )
      {
         pTableData->bStarted      = TRUE;
         pTableData->seqNumber     = seqNumber;
        // Use the hyst start-time as event-table start time
         pTableData->nStartTime_ms = pTableData->nTblHystStartTime_ms;
         memcpy((void *)&pTableData->tsExceedanceStartTime,
                (void *)&pTableData->tsTblHystStartTime,
                 sizeof(pTableData->tsTblHystStartTime));

         // If this eventtable is configured for TH, start it now
         if (TRUE == pTableCfg->bEnableTH)
         {
           // Start Time History
           TH_Open(pTableCfg->preTime_s);
         }

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
            // Make sure this isn't the last confirmed region because we don't want to
            // Reset restart the duration calculation if we spiked into a region and
            // came back before the transient allowance was exceeded.
            if ( foundRegion != pTableData->confirmedRegion )
            {
               // We just entered this region record the time
               pTableData->regionStats[foundRegion].nEnteredTime = nCurrentTick;
            }
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
      // Check if we already started the Event Table Processing and have exited.
      if ( ( TRUE == pTableData->bStarted ) &&
            // ( 0 != pTableData->nTotalDuration_ms ) &&
           (TRUE == bHystExitMet) )
      {
         // The exceedance has ended
         CM_GetTimeAsTimestamp( &pTableData->tsExceedanceEndTime );
         // Calculate the final duration since the exceedance began
         pTableData->nTotalDuration_ms = nCurrentTick - pTableData->nStartTime_ms;
         // Exit any confirmed region that wasn't previously exited
         EventTableExitRegion ( pTableCfg,  pTableData, foundRegion, nCurrentTick );
         // Determine the reason for ending the event table
         tableState = (bSensorValid == TRUE) ? ET_SENSOR_VALUE_LOW : ET_SENSOR_INVALID;

         // SRS-4510
         // If a region was entered while in the table...
         // Log the Event Table -
         if ( REGION_NOT_FOUND != pTableData->maximumRegionEntered)
         {
           EventTableLogSummary ( pTableCfg, pTableData, priority, tableState );
         }

         // If TH was configured for this eventtable, close it at configured post time.
         if (TRUE == pTableCfg->bEnableTH)
         {
           TH_Close(pTableCfg->postTime_s);
         }

         // Re-init the table-hysteresis time vars for next entry

         pTableData->nTblHystStartTime_ms = 0;
         memset((void *)&pTableData->tsTblHystStartTime,
                0, sizeof(pTableData->tsTblHystStartTime));

      }
      // Now reset the Started Flag and wait until we cross a region threshold
      pTableData->bStarted = FALSE;
   }

   return (tableState);
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
   UINT32        nRegIndex;
   UINT32        nSegIndex;
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
   for ( nRegIndex = (UINT32)REGION_A;
         (REGION_NOT_FOUND != pTableData->maximumCfgRegion) &&
         (nRegIndex <= (UINT32)pTableData->maximumCfgRegion);
         nRegIndex++ )
   {
      pRegion   = &pTableCfg->regCfg.region[nRegIndex];

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
            // Hysteresis Else we need a negative Hysteresis to exit the region
            if ( (pTableData->currentRegion < (EVENT_REGION)nRegIndex) ||
                 (pTableData->currentRegion == REGION_NOT_FOUND) )
            {
               fThreshold += pTableCfg->regCfg.fHysteresisPos;
            }
            else
            {
               fThreshold -= pTableCfg->regCfg.fHysteresisNeg;
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
 * Notes:        Never call this function without first checking if foundRegion is
 *               within Range for the arrays. At release of v2.0.0 function
 *               is never called when foundRegion = REGION_NOT_FOUND so the configuration
 *               array will never go out of bounds.
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
         pTableCfg->regCfg.nTransientAllowance_ms) ||
        (( foundRegion < pTableData->confirmedRegion ) &&
         (pTableData->confirmedRegion != REGION_NOT_FOUND)))
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

         bACKFlag   = (pTableCfg->regCfg.region[foundRegion].nAction & EVENT_ACTION_ACK) ?
                    TRUE : FALSE;
         bLatchFlag = (pTableCfg->regCfg.region[foundRegion].nAction & EVENT_ACTION_LATCH) ?
                    TRUE : FALSE;

         // Request the event action for the confirmed region
         // Note: possible array out of bounds issue with pTableCfg
         //       foundRegion should never be REGION_NOT_FOUND because it is protected
         //       by the calling function.
         pTableData->nActionReqNum = ActionRequest ( pTableData->nActionReqNum,
                  ( EVENT_ACTION_ON_DURATION(pTableCfg->regCfg.region[foundRegion].nAction) |
                    EVENT_ACTION_ON_MET(pTableCfg->regCfg.region[foundRegion].nAction) ),
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
   UINT32 i;
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
            timeDelta = pTableCfg->regCfg.nTransientAllowance_ms;
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
      for ( i = (UINT32)REGION_A; i < (UINT32)MAX_TABLE_REGIONS; i++ )
      {
         // Make sure we don't shut off the new found region.
         if ( (EVENT_REGION)i != foundRegion )
         {
            pTableData->nActionReqNum =
              ActionRequest ( pTableData->nActionReqNum,
                             EVENT_ACTION_ON_DURATION(pTableCfg->regCfg.region[i].nAction) |
                             EVENT_ACTION_ON_MET(pTableCfg->regCfg.region[i].nAction),
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
 * Parameters:   [in] EVENT_TABLE_CFG   *pTableCfg
 *               [in] EVENT_TABLE_DATA  *pTableData
 *               [in] EVENT_REGION      EnteredRegion
 *               [in] EVENT_REGION      ExitedRegion
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static
void EventTableLogTransition ( const EVENT_TABLE_CFG *pTableCfg,
                               const EVENT_TABLE_DATA *pTableData,
                               EVENT_REGION EnteredRegion, EVENT_REGION ExitedRegion)
{
   // Local Data
   EVENT_TABLE_TRANSITION_LOG transLog;

   // Build transition log
   transLog.eventTableIndex  = pTableData->nTableIndex;
   transLog.seqNumber        = pTableData->seqNumber;
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
 *               [in] EVENT_TABLE_STATE endReason
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static
void EventTableLogSummary (const EVENT_TABLE_CFG *pConfig, const EVENT_TABLE_DATA *pData,
                           LOG_PRIORITY priority, EVENT_TABLE_STATE endReason )
{
   // Local Data
   EVENT_TABLE_SUMMARY_LOG tSumm;
   UINT32                  nRegIndex;

   // Gather up the data to write to the log
   tSumm.eventTableIndex          = pData->nTableIndex;
   tSumm.seqNumber                = pData->seqNumber;
   tSumm.endReason                = endReason;
   tSumm.sensorIndex              = pConfig->nSensor;
   tSumm.tsExceedanceStartTime    = pData->tsExceedanceStartTime;
   tSumm.tsExceedanceEndTime      = pData->tsExceedanceEndTime;
   tSumm.nDuration_ms             = pData->nTotalDuration_ms;
   tSumm.maximumRegionEntered     = pData->maximumRegionEntered;
   tSumm.fMaxSensorValue          = pData->fMaxSensorValue;
   tSumm.nMaxSensorElaspedTime_ms = pData->nMaxSensorElaspedTime_ms;
   tSumm.maximumCfgRegion         = pData->maximumCfgRegion;

   // Loop through the region stats and place them in the log
   for ( nRegIndex = (UINT32)REGION_A; nRegIndex < (UINT32)MAX_TABLE_REGIONS; nRegIndex++ )
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
   UINT32           nCurrentTick;

   // Initialize Local Data
   nCurrentTick = CM_GetTickCount();

   // if duration == 0 the Event needs to be initialized
   if ( 0 == pData->nStartTime_ms )
   {
      // general Event initialization
      pData->nStartTime_ms   = nCurrentTick;
//    pData->nSampleCount    = 0;
      CM_GetTimeAsTimestamp(&pData->tsCriteriaMetTime);
      memset((void *)&pData->tsDurationMetTime,0, sizeof(TIMESTAMP));
      memset((void *)&pData->tsEndTime        ,0, sizeof(TIMESTAMP));
   }

   // update the sample count for the average calculation
   SensorUpdateSummaries(pData->sensor, pData->nTotalSensors);
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
  EVENT_END_LOG *pLog;
  SNSR_SUMMARY  *pSummary;
  UINT8         numSensor;

  // Initialize Local Data
  pLog = &m_EventEndLog[pData->eventIndex];

  SensorCalculateSummaryAvgs(pData->sensor, pData->nTotalSensors);

  // update the Event duration
  pLog->nDuration_ms  = CM_GetTickCount() - pData->nStartTime_ms;

  // Update the Event end log
  pLog->nEventIndex        = pData->eventIndex;
  pLog->seqNumber          = pData->seqNumber;
  pLog->endType            = pData->endType;
  pLog->tsCriteriaMetTime  = pData->tsCriteriaMetTime;
  pLog->tsDurationMetTime  = pData->tsDurationMetTime;
  pLog->tsEndTime          = pData->tsEndTime;
  pLog->nTotalSensors      = pData->nTotalSensors;

  // Loop through all the sensors
  for ( numSensor = 0; numSensor < MAX_EVENT_SENSORS; numSensor++ )
  {
    // Set a pointer the summary
    pSummary = &(pData->sensor[numSensor]);

    // Check the sensor is used
    if ( SensorIsUsed((SENSOR_INDEX)pSummary->SensorIndex ) )
    {
       pSummary->bValid = SensorIsValid((SENSOR_INDEX)pSummary->SensorIndex);
       // Update the summary data for the trigger
       pLog->sensor[numSensor].sensorIndex = pSummary->SensorIndex;
       pLog->sensor[numSensor].timeMinValue = pSummary->timeMinValue;
       pLog->sensor[numSensor].fMinValue   = pSummary->fMinValue;
       pLog->sensor[numSensor].timeMaxValue = pSummary->timeMaxValue;
       pLog->sensor[numSensor].fMaxValue   = pSummary->fMaxValue;
       pLog->sensor[numSensor].bValid      = pSummary->bValid;
       // if sensor is valid, calculate the final average,
       // otherwise use the average calculated at the point it went invalid.
       pLog->sensor[numSensor].fAvgValue   = pSummary->fAvgValue;
    }
    else // Sensor Not Used
    {
       pLog->sensor[numSensor].sensorIndex   = SENSOR_UNUSED;
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
 *               [in] LOG_PRIORITY      priority
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
      EventTableLogSummary ( pTableCfg, pTableData, priority, ET_COMMANDED );
   }
   // Now reset the Started Flag
   pTableData->bStarted = FALSE;
}

/******************************************************************************
 * Function:     EventUpdateHistory
 *
 * Description:  Update the history buffer for an event.
 *
 * Parameters:   [in] EVENT_CFG    pConfig event cfg
 *               [in] EVENT_DATA   pData   the runtime data for the event.
 *
 * Returns:      None.
 *
 * Notes:        None
 *
 *****************************************************************************/
static
void EventUpdateHistory( const EVENT_CFG *pConfig, const EVENT_DATA *pData)
{
  INT32 histMax;
  INT32 eventMax;
  EVENT_HISTORY*    pEventHist = &m_EventHistory.histData[pData->eventIndex];
  EVENT_TABLE_DATA* pTblData;

  // Always update the count.
  pEventHist->nCnt += 1;

  if ( EVENT_TABLE_UNUSED == pConfig->eventTableIndex )
  {
    // For simple events (i.e. no tables) store the start-time of the event
    pEventHist->tsEvent = pData->tsCriteriaMetTime;
  }
  else // Table event
  {
    // Get a ptr to the event table used by this evnt.
    pTblData = &m_EventTableData[pConfig->eventTableIndex];

    // The EVENT_REGION enum ranges 0...6, with REGION_NOT_FOUND (6) having
    // the greatest value, but the lowest comparison weight.
    // The following two assignments simplify comparisons by redefining the range
    // as -1...5 where REGION_NOT_FOUND is -1

    histMax = pEventHist->maxRegion == REGION_NOT_FOUND ? -1 : (INT32)pEventHist->maxRegion;

    eventMax = pTblData->maximumRegionEntered == REGION_NOT_FOUND ?
                                -1 : (INT32)pTblData->maximumRegionEntered;

    // If the timestamp is zero, then First time... anything is greater, incl REGION_NOT_FOUND,
    // otherwise is the max from this event, bigger than the history buff?
    if( ( 0 == pEventHist->tsEvent.Timestamp) ||
        ( eventMax > histMax ))
    {
      pEventHist->tsEvent   = pData->tsCriteriaMetTime;
      pEventHist->maxRegion = pTblData->maximumRegionEntered;
    }
  }
  // Write the entire event history buffer to EEPROM
  NV_Write(NV_EVENT_HISTORY, 0, &m_EventHistory, sizeof(m_EventHistory) );
}


/******************************************************************************
 * Function:     EventTableIsEntered
 *
 * Description:  Function used to monitor hysteresis and transient allowance
 *               to determine if conditions are met to considered the table
 *               entered.
 *
 * Parameters:   [in] EVENT_TABLE_CFG  - pTableCfg Pointer to the table cfg.
 *               [in] EVENT_TABLE_DATA - pTableData Pointer to the table runtime data
 *               [in] UINT32           - nCurrentTick Current task tick used to calculate
 *                                       transient allowance
 *               [in] BOOLEAN          - bSensorValid The validity of the determining sensor
 *
 *
 * Returns:      TRUE or FALSE
 *
 * Notes:        None
 *
 *****************************************************************************/
static
BOOLEAN EventTableIsEntered(EVENT_TABLE_CFG *pTableCfg, EVENT_TABLE_DATA *pTableData,
                                   UINT32 nCurrentTick, BOOLEAN bSensorValid)
{
  BOOLEAN bEnterTable = FALSE;
  // Check current sensor against hysteresis threshold, update the duration time accordingly.
  if ( bSensorValid &&
      ( pTableData->fCurrentSensorValue >=
      ( pTableCfg->fTableEntryValue + pTableCfg->tblHyst.fHystEntry )) )
   {
     // sensor has met (or is continuing to meet) the threshold for hysteresis,
     // Check ttransient-allowancer

     if (0 == pTableData->nTblHystStartTime_ms)
     {
       // Hyst just met now, start timing...
       pTableData->nTblHystStartTime_ms = nCurrentTick;
       CM_GetTimeAsTimestamp( &pTableData->tsTblHystStartTime );

       // Special test: transient allowance is configured as zero? enter table now!
       bEnterTable = (0 == pTableCfg->tblHyst.nTransientAllowance_ms) ? TRUE : FALSE;

     }
     else
     {
       // the table-hyst was met in a prior frame and start-time already initialized...
       // check if transient-allowance duration has been met;
       bEnterTable = ( ( (nCurrentTick - pTableData->nTblHystStartTime_ms) >=
                          pTableCfg->tblHyst.nTransientAllowance_ms ))
                          ? TRUE : FALSE;
     }
  }
  else if ( 0 != pTableData->nTblHystStartTime_ms )
  {
    // We were waiting for the TransientAllowance to be met, but Sensor went bad or
    // fell below the hyst-threshold: clear start-time so we perform entry process again.
    pTableData->nTblHystStartTime_ms = 0;
    memset((void *)&pTableData->tsTblHystStartTime, 0,
            sizeof(pTableData->tsTblHystStartTime));
  }

#if 0
  /*vcast_dont_instrument_start*/
  if (bEnterTable)
  {
    GSE_DebugStr(NORMAL,TRUE,
        "DBG-Entered Table %d, snsrValue: %6.4f, minValue: %6.4f, hystValue: %6.4f, T/A: %d",
        pTableData->nTableIndex,
        pTableData->fCurrentSensorValue,
        pTableCfg->fTableEntryValue,
        pTableCfg->tblHyst.fHystEntry,
        pTableCfg->tblHyst.nTransientAllowance_ms);
  }
  /*vcast_dont_instrument_end*/
#endif
  return bEnterTable;
}

/******************************************************************************
 * Function:     EventTableIsExited
 *
 * Description:  Function used to monitor hysteresis to determine if conditions
 *               are met to considered the table
 *               entered.
 *
 * Parameters:   [in] EVENT_TABLE_CFG  - pTableCfg Pointer to the table cfg.
 *               [in] EVENT_TABLE_DATA - pTableData Pointer to the table runtime data
 *               [in] BOOLEAN          - bSensorValid Validity of the determining sensor
 *               [in] BOOLEAN          - bEntered  Table entered state
 *
 *
 * Returns:      TRUE or FALSE
 *
 * Notes:        None
 *
 * Notes:        Exit Hysteresis does not use transient allowance; exit
 *               is immediate upon crossing threshold after table is entered.
 *
 *****************************************************************************/
static
BOOLEAN EventTableIsExited(EVENT_TABLE_CFG *pTableCfg, EVENT_TABLE_DATA *pTableData,
                                  BOOLEAN bSensorValid, BOOLEAN bEntered)
{
  BOOLEAN bEnded = FALSE;
  // Check current sensor value and validity against hysteresis threshold.
  // Exit cannot be true unless we first enter.
  if ( bEntered )
  {
    bEnded = (FALSE == bSensorValid)  ||
             (TRUE == bSensorValid    &&
              pTableData->fCurrentSensorValue <=
              ( pTableCfg->fTableEntryValue - pTableCfg->tblHyst.fHystExit ));
  }

#if 0
  /*vcast_dont_instrument_start*/
  if (bEnded)
  {
     GSE_DebugStr( NORMAL,TRUE,
      "DBG-Exited  Table %d, snsrValue: %6.4f, minValue: %6.4f, hystValue: %6.4f, TblDur: %d",
                  pTableData->nTableIndex,
                  pTableData->fCurrentSensorValue,
                  pTableCfg->fTableEntryValue,
                  pTableCfg->tblHyst.fHystEntry,
                  pTableData->nTotalDuration_ms);
  }
  /*vcast_dont_instrument_end*/
#endif
  return bEnded;
}


/*************************************************************************
 *  MODIFICATIONS
 *    $History: Event.c $
 * 
 * *****************  Version 48  *****************
 * User: John Omalley Date: 4/14/15    Time: 2:31p
 * Updated in $/software/control processor/code/application
 * SCR 1289 - Removed Dead Code
 * 
 * *****************  Version 47  *****************
 * User: Contractor V&v Date: 11/20/14   Time: 3:05p
 * Updated in $/software/control processor/code/application
 * SCR #1249 - Event Table  Hysteresis Code Review - no logic changes
 * 
 * *****************  Version 46  *****************
 * User: Contractor V&v Date: 11/11/14   Time: 5:26p
 * Updated in $/software/control processor/code/application
 * SCR #1249 - Event Table  TimeHistory 
 *
 * *****************  Version 45  *****************
 * User: Contractor V&v Date: 11/03/14   Time: 5:23p
 * Updated in $/software/control processor/code/application
 * SCR #1249 - Event Table  Hysteresis
 *
 * *****************  Version 44  *****************
 * User: Contractor V&v Date: 10/20/14   Time: 3:54p
 * Updated in $/software/control processor/code/application
 * SCR #1188 - Event History Buffer
 *
 * *****************  Version 43  *****************
 * User: John Omalley Date: 13-03-12   Time: 11:24a
 * Updated in $/software/control processor/code/application
 * SCR 1242 - Event Sequence Number unique
 *
 * *****************  Version 42  *****************
 * User: Jim Mood     Date: 2/19/13    Time: 6:07p
 * Updated in $/software/control processor/code/application
 * SCR #1236
 *
 * *****************  Version 41  *****************
 * User: Jeff Vahue   Date: 2/15/13    Time: 7:52p
 * Updated in $/software/control processor/code/application
 * SCR# 1236 Dead Code removal
 *
 * *****************  Version 40  *****************
 * User: John Omalley Date: 13-01-24   Time: 2:07p
 * Updated in $/software/control processor/code/application
 * SCR 1228 - SENSOR not Used issue for log summary
 *
 * *****************  Version 39  *****************
 * User: John Omalley Date: 12-12-19   Time: 5:53p
 * Updated in $/software/control processor/code/application
 * SCR 1197 - Code Review Update
 *
 * *****************  Version 38  *****************
 * User: John Omalley Date: 12-12-08   Time: 1:33p
 * Updated in $/software/control processor/code/application
 * SCR 1167 - Sensor Summary Init optimization
 *
 * *****************  Version 37  *****************
 * User: John Omalley Date: 12-11-28   Time: 2:25p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Code Review Updates
 *
 * *****************  Version 36  *****************
 * User: Contractor V&v Date: 11/26/12   Time: 12:32p
 * Updated in $/software/control processor/code/application
 * SCR #1167 SensorSummary
 *
 * *****************  Version 35  *****************
 * User: John Omalley Date: 12-11-14   Time: 6:51p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Code Review Updates
 *
 * *****************  Version 34  *****************
 * User: Contractor V&v Date: 11/14/12   Time: 4:01p
 * Updated in $/software/control processor/code/application
 * Code Review
 *
 * *****************  Version 33  *****************
 * User: John Omalley Date: 12-11-08   Time: 3:03p
 * Updated in $/software/control processor/code/application
 * SCR 1131 - Busy Recording Logic
 *
 * *****************  Version 32  *****************
 * User: John Omalley Date: 12-11-07   Time: 8:30a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Code Review updates - Enumerations
 *
 * *****************  Version 31  *****************
 * User: John Omalley Date: 12-11-06   Time: 11:12a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Code Review Updates
 *
 * *****************  Version 30  *****************
 * User: John Omalley Date: 12-10-23   Time: 2:07p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Updates from Design Review
 * 1. Added Sequence Number for ground team
 * 2. Added Reason for table end to log
 * 3. Updated Event Index in log to ENUM
 * 4. Fixed hysteresis cfg range from 0-FLT_MAX
 *
 * *****************  Version 29  *****************
 * User: John Omalley Date: 12-10-18   Time: 1:56p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Design Review Updates
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

