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
 $Revision: 13 $  $Date: 7/18/12 6:24p $

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
#include "alt_basic.h"
#include "assert.h"
#include "cfgmanager.h"
#include "gse.h"
#include "logmanager.h"
#include "NVMgr.h"
#include "taskmanager.h"
#include "systemlog.h"
#include "trigger.h"
#include "utility.h"
#include "user.h"

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
static void           EventsUpdateTask         ( void *pParam );
static void           EventsEnableTask         ( BOOLEAN bEnable );
static void           EventProcess             ( EVENT_CFG *pConfig, EVENT_DATA *pData );
static BOOLEAN        EventCheckDuration       ( EVENT_CFG *pConfig, EVENT_DATA *pData );
static BOOLEAN        EventCheckStart          ( EVENT_CFG *pConfig, EVENT_DATA *pData,
                                                 BOOLEAN   *IsValid );
static EVENT_END_TYPE EventCheckEnd            ( EVENT_CFG *pConfig, EVENT_DATA *pData );
static void           EventResetData           ( EVENT_CFG *pConfig, EVENT_DATA *pData );
static void           EventUpdateData          ( EVENT_CFG *pConfig, EVENT_DATA *pData );
static void           EventLogStart            ( EVENT_CFG *pConfig, EVENT_DATA *pData );
static void           EventLogUpdate           ( EVENT_CFG *pConfig, EVENT_DATA *pData );
static void           EventLogEnd              ( EVENT_CFG *pConfig, EVENT_DATA *pData );
static void           EventForceEnd            ( void );
/*-------------------------- EVENT TABLES ----------------------------------*/
static void           EventTableResetData      ( EVENT_TABLE_INDEX EventTableIndex );
static BOOLEAN        EventTableUpdate         ( EVENT_INDEX       EventIndex,
                                                 EVENT_TABLE_INDEX EventTableIndex,
                                                 UINT32            CurrentTick );
static EVENT_REGION   EventTableFindRegion     ( EVENT_TABLE_CFG  *pTableCfg,
                                                 EVENT_TABLE_DATA *pTableData,
                                                 FLOAT32 fSensorValue );
static BOOLEAN        EventTableConfirmRegion  ( EVENT_TABLE_CFG  *pTableCfg,
                                                 EVENT_TABLE_DATA *pTableData,
                                                 EVENT_REGION      FoundRegion,
                                                 UINT32            CurrentTick,
                                                 EVENT_INDEX       EventIndex );
static void           EventTableExitRegion     ( EVENT_TABLE_CFG  *pTableCfg,
                                                 EVENT_TABLE_DATA *pTableData,
                                                 EVENT_REGION      FoundRegion,
                                                 UINT32            CurrentTick,
                                                 EVENT_INDEX       EventIndex );
static void           EventTableLogSummary     ( EVENT_TABLE_CFG  *pConfig,
                                                 EVENT_TABLE_DATA *pData,
                                                 EVENT_TABLE_INDEX EventTableIndex );
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
   TCB        TaskInfo;
   UINT16     i;

   EVENT_CFG  *pEventCfg;
   EVENT_DATA *pEventData;

   // Add Trigger Tables to the User Manager
   User_AddRootCmd(&RootEventMsg);

   // Reset the Sensor configuration storage array
   memset(&m_EventCfg, 0, sizeof(m_EventCfg));

   // Load the current configuration to the configuration array.
   memcpy(&m_EventCfg,
          &(CfgMgr_RuntimeConfigPtr()->EventConfigs),
          sizeof(m_EventCfg));

   // Loop through the Configured triggers
   for ( i = 0; i < MAX_EVENTS; i++)
   {
      // Set a pointer to the current event
      pEventCfg = &m_EventCfg[i];

      // Set a pointer to the runtime data table
      pEventData = &m_EventData[i];

      // Clear the runtime data table
      pEventData->EventIndex        = (EVENT_INDEX)i;
      pEventData->State             = EVENT_NONE;
      pEventData->nStartTime_ms     = 0;
      pEventData->nDuration_ms      = 0;
      pEventData->nRateCounts       = (UINT16)(MIFs_PER_SECOND / pEventCfg->Rate);
      pEventData->nRateCountdown    = (UINT16)((pEventCfg->nOffset_ms / MIF_PERIOD_mS) + 1);
      pEventData->bStartCompareFail = FALSE;
      pEventData->bEndCompareFail   = FALSE;
      pEventData->ActionType        = ACTION_OFF;
      pEventData->ActionReqNum      = ACTION_NO_REQ;
      pEventData->EndType           = EVENT_NO_END;

      // If the event has a Start and End Expression then it must be configured
      if ( (0 != pEventCfg->StartExpr.Size) && (0 != pEventCfg->EndExpr.Size) )
      {
         // Reset the events run time data
         EventResetData ( pEventCfg, pEventData );
      }
   }

   // Initialize the Event Tables
   EventTablesInitialize();

   // Create Event Task - DT
   memset(&TaskInfo, 0, sizeof(TaskInfo));
   strncpy_safe(TaskInfo.Name, sizeof(TaskInfo.Name),"Event Task",_TRUNCATE);
   TaskInfo.TaskID          = Event_Task;
   TaskInfo.Function        = EventsUpdateTask;
   TaskInfo.Priority        = taskInfo[Event_Task].priority;
   TaskInfo.Type            = taskInfo[Event_Task].taskType;
   TaskInfo.modes           = taskInfo[Event_Task].modes;
   TaskInfo.MIFrames        = taskInfo[Event_Task].MIFframes;
   TaskInfo.Enabled         = TRUE;
   TaskInfo.Locked          = FALSE;
   TaskInfo.Rmt.InitialMif  = taskInfo[Event_Task].InitialMif;
   TaskInfo.Rmt.MifRate     = taskInfo[Event_Task].MIFrate;
   TaskInfo.pParamBlock     = NULL;

   TmTaskCreate (&TaskInfo);
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
   UINT16           TableIndex;
   UINT16           RegionIndex;
   UINT16           SegmentIndex;

   // Add Trigger Tables to the User Manager
   User_AddRootCmd(&RootEventTableMsg);

   // Reset the Sensor configuration storage array
   memset(&m_EventTableCfg, 0, sizeof(m_EventTableCfg));

   // Load the current configuration to the configuration array.
   memcpy(&m_EventTableCfg,
          &(CfgMgr_RuntimeConfigPtr()->EventTableConfigs),
          sizeof(m_EventTableCfg));

   // Loop through all the Tables
   for ( TableIndex = 0; TableIndex < MAX_TABLES; TableIndex++ )
   {
      // Set pointers to the Table Cfg and the Table Data
      pTableCfg  = &m_EventTableCfg  [TableIndex];
      pTableData = &m_EventTableData [TableIndex];

      pTableData->MaximumCfgRegion = REGION_NOT_FOUND;
      pTableData->ActionReqNum     = ACTION_NO_REQ;

      // Loop through all the Regions
      for ( RegionIndex = 0; RegionIndex < MAX_TABLE_REGIONS; RegionIndex++ )
      {
         // Set a pointer the Region Cfg
         pReg = &pTableCfg->Region[RegionIndex];

         // Initialize Region Statistic Data
         pTableData->RegionStats[RegionIndex].RegionConfirmed        = FALSE;
         pTableData->RegionStats[RegionIndex].LogStats.EnteredCount  = 0;
         pTableData->RegionStats[RegionIndex].LogStats.ExitCount     = 0;
         pTableData->RegionStats[RegionIndex].LogStats.Duration_ms   = 0;
         pTableData->RegionStats[RegionIndex].EnteredTime            = 0;

         // Loop through each line segment in the region
         for ( SegmentIndex = 0; SegmentIndex < MAX_REGION_SEGMENTS; SegmentIndex++ )
         {
            // Set pointers to the line segment and constant data
            pSeg       = &pReg->Segment[SegmentIndex];
            pConst     = &pTableData->Segment[RegionIndex][SegmentIndex];

            // Need to protect against divide by 0 here so make sure
            // Start and Stop Times not equal
            // TBD - Should this ASSERT? No because it might not be configured
            pConst->m = ( pSeg->StopTime_s == pSeg->StartTime_s ) ? 0 :
                        ( (pSeg->StopValue - pSeg->StartValue) /
                          (pSeg->StopTime_s - pSeg->StartTime_s) );
            pConst->b = pSeg->StartValue - (pSeg->StartTime_s * pConst->m);

            // Find the last configured region
            if ( pSeg->StopTime_s != 0 )
            {
               pTableData->MaximumCfgRegion = (EVENT_REGION)RegionIndex;
            }
         }
      }

      // Now initialize the remaining Table Data
      pTableData->Started              = FALSE;
      pTableData->StartTime_ms         = 0;
      pTableData->MaximumRegionEntered = REGION_NOT_FOUND;
      pTableData->CurrentRegion        = REGION_NOT_FOUND;
      pTableData->ConfirmedRegion      = REGION_NOT_FOUND;
   }
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
   TmTaskEnable (Event_Task, bEnable);
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
   pData->State         = EVENT_START;
   pData->nStartTime_ms = 0;
   pData->nDuration_ms  = 0;
   pData->nSampleCount  = 0;
   pData->EndType       = EVENT_NO_END;

      // Re-init the sensor summary data for the event
#pragma ghs nowarning 1545 //Suppress packed structure alignment warning
   pData->nTotalSensors = SensorSetupSummaryArray(pData->Sensor,
                                                  MAX_EVENT_SENSORS,
                                                  pConfig->SensorMap,
                                                  sizeof (pConfig->SensorMap) );
#pragma ghs endnowarning 
}

/******************************************************************************
 * Function:     EventTableResetData
 *
 * Description:  Reset a single event table objects Data
 *
 * Parameters:   [in] EventTableIndex - Index of the event to reset
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static
void EventTableResetData ( EVENT_TABLE_INDEX EventTableIndex )
{
   //Local Data
   EVENT_TABLE_DATA *pTableData;
   UINT16           RegionIndex;
   REGION_STATS     *pStats;

   pTableData = &m_EventTableData[EventTableIndex];

   // Reset all the region statistics and max value reached
   pTableData->Started                 = FALSE;
   pTableData->StartTime_ms            = 0;
   pTableData->CurrentRegion           = REGION_NOT_FOUND;
   pTableData->ConfirmedRegion         = REGION_NOT_FOUND;
   pTableData->TotalDuration_ms        = 0;
   pTableData->MaximumRegionEntered    = REGION_NOT_FOUND;
   pTableData->MaxSensorValue          = 0.0;
   pTableData->MaxSensorElaspedTime_ms = 0;

   // Loop through all the regions and reset the statistics
   for ( RegionIndex = 0; RegionIndex <= pTableData->MaximumCfgRegion; RegionIndex++ )
   {
      pStats = &pTableData->RegionStats[RegionIndex];

      pStats->RegionConfirmed       = FALSE;
      pStats->EnteredTime           = 0;
      pStats->LogStats.EnteredCount = 0;
      pStats->LogStats.ExitCount    = 0;
      pStats->LogStats.Duration_ms  = 0;
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
         if ( pConfig->StartExpr.Size != 0 )
         {
            // Countdown the number of samples until next read
            if (--pData->nRateCountdown <= 0)
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
 * Notes:
 *
 *****************************************************************************/
static
void EventProcess ( EVENT_CFG *pConfig, EVENT_DATA *pData )
{
   // Local Data
   BOOLEAN  StartCriteriaMet;
   BOOLEAN  IsStartValid;
   UINT32   CurrentTick;
   BOOLEAN  bTableRunning;

   // Initialize Local Data
   CurrentTick   = CM_GetTickCount();
   bTableRunning = FALSE;

   switch (pData->State)
   {
      // Event start is looking for begining of all start triggers
      case EVENT_START:
         // Check if all start triggers meet configured criteria
         StartCriteriaMet = EventCheckStart ( pConfig, pData, &IsStartValid );
         // TBD: Check for IsStartValid?
         // Check the all start criteria met
         if (TRUE == StartCriteriaMet)
         {
            // Check if already started
            if ( ACTION_OFF == pData->ActionType )
            {
               // if we just detected the event is starting then set the flag
               // and reset the event data so we can start collecting it again
               // If we do this at the end we cannot view the status after the
               // event has ended using GSE commands.
               pData->ActionType   = ON_MET;
               pData->ActionReqNum = ActionRequest ( pData->ActionReqNum,
                                                     pConfig->Action,
                                                     pData->ActionType );
               EventResetData( pConfig, pData );
            }

            // If this is a table event then perform table processing
            if ( EVENT_TABLE_UNUSED != pConfig->EventTableIndex )
            {
               bTableRunning = EventTableUpdate ( pData->EventIndex,
                                                  pConfig->EventTableIndex,
                                                  CurrentTick );

               // The event will only become active when the table is entered
               // by exceeding the minimum sensor value configured for the table
               if ( TRUE == bTableRunning )
               {
                  pData->State = EVENT_ACTIVE;
               }
            }
            // This is a simple event check if we have exceeded the cfg duration
            else if ( TRUE == EventCheckDuration ( pConfig, pData ) )
            {
               // Duration was exceeded the event is now active
               pData->State = EVENT_ACTIVE;
               pData->ActionType = ON_DURATION;
               pData->ActionReqNum = ActionRequest ( pData->ActionReqNum,
                                                     pConfig->Action,
                                                     pData->ActionType );
            }

            // Continue to update the configured event data even though the duration
            // has not been met or the table has not been entered
            EventUpdateData ( pConfig, pData );

            if ( EVENT_ACTIVE == pData->State )
            {
               CM_GetTimeAsTimestamp(&pData->DurationMetTime);
               // Log the Event Start
               EventLogStart ( pConfig, pData );
               // Start Time History
               TimeHistoryStart(pConfig->PreTimeHistory, pConfig->PreTime_s);
            }
         }
         // Check if event stopped being met before duration was met
         // or table threshold was crossed
         else
         {
            // Reset the Start Type
            pData->ActionType   = ACTION_OFF;
            pData->ActionReqNum = ActionRequest ( pData->ActionReqNum,
                                                  pConfig->Action,
                                                  pData->ActionType );
         }
         break;

      // Duration has been met or the table threshold was crossed
      // now the event is considered active
      case EVENT_ACTIVE:
         // keep updating the cfg parameters for the event
         EventUpdateData( pConfig, pData );

         // Perform table processing if configured for a table
         if ( EVENT_TABLE_UNUSED != pConfig->EventTableIndex )
         {
            bTableRunning = EventTableUpdate ( pData->EventIndex,
                                               pConfig->EventTableIndex, CurrentTick );
         }

         // Check if the end criteria has been met
         pData->EndType = EventCheckEnd(pConfig, pData);

         // Check if any end criteria has happened and the table is done
         if ((EVENT_NO_END != pData->EndType) && (FALSE == bTableRunning))
         {
            // Record the time the event ended
            CM_GetTimeAsTimestamp(&pData->EndTime);
            // Log the End
            EventLogEnd(pConfig, pData);
            // Change event state back to start
            pData->State      = EVENT_START;
            pData->ActionType = ACTION_OFF;
            // End the time history
            TimeHistoryEnd(pConfig->PostTimeHistory, pConfig->PostTime_s);
            // reset the event Action
            pData->ActionReqNum = ActionRequest ( pData->ActionReqNum,
                                                  pConfig->Action,
                                                  pData->ActionType );
         }
         break;

      default:
         // FATAL ERROR WITH EVENT State
         FATAL("Event State Fail: %d - %s - State = %d",
               pData->EventIndex, pConfig->EventName, pData->State);
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
 * Parameters:   [in]  EVENT_CONFIG *pEventCfg,
 *               [in]  EVENT_DATA   *pEventData
 *               [out] BOOLEAN      *IsValid
 *
 * Returns:      BOOLEAN StartCriteriaMet
 *
 * Notes:        None.
 *
 *****************************************************************************/
static
BOOLEAN EventCheckStart ( EVENT_CFG *pConfig, EVENT_DATA *pData, BOOLEAN *IsValid )
{
   // Local Data
   BOOLEAN StartCriteriaMet;
   BOOLEAN ValidTmp;

   // Catch IsValid passed as null, preset to true
   IsValid = IsValid == NULL ? &ValidTmp : IsValid;
   *IsValid = TRUE;

   StartCriteriaMet = (BOOLEAN) EvalExeExpression ( &pConfig->StartExpr, IsValid );

   return StartCriteriaMet;
}

/******************************************************************************
 * Function:     EventCheckEnd
 *
 * Description:  Check if event end criterias are met
 *
 * Parameters:   [in] EVENT_CONFIG *pConfig,
 *               [in] EVENT_DATA   *pData
 *
 * Returns:      EVENT_END_TYPE
 *
 * Notes:        None
 *
 *****************************************************************************/
static
EVENT_END_TYPE EventCheckEnd ( EVENT_CFG *pConfig, EVENT_DATA *pData )
{
   // Local Data
   BOOLEAN         validity;
   EVENT_END_TYPE  EndType;

   EndType = EVENT_NO_END;

   // Process the end expression
   // Exit criteria is in the return value, trigger validity in 'validity'
   // Together they define how the event ended
   if( EvalExeExpression(&pConfig->EndExpr, &validity ) &&
       (TRUE == validity))
   {
      EndType = EVENT_END_CRITERIA;
   }
   else if (FALSE == validity)
   {
      EndType = EVENT_TRIGGER_INVALID;
   }

   return EndType;
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
void EventLogStart (EVENT_CFG *pConfig, EVENT_DATA *pData)
{
   // Local Data
   UINT32  nEventStorage;

   // Initialize Local Data
   nEventStorage = (UINT32)pData->EventIndex;

   LogWriteETM    (APP_ID_EVENT_STARTED,
                   LOG_PRIORITY_LOW,
                   &nEventStorage,
                   sizeof(nEventStorage),
                   NULL);

   GSE_DebugStr ( NORMAL, TRUE, "Event Start (%d) %s ",
                  pData->EventIndex, pConfig->EventName );
}

/******************************************************************************
 * Function:     EventTableUpdate
 *
 * Description:  Processes the configured exceedance table to determine the
 *               current zone, maximum sensor value and any event actions to
 *               to perform.
 *
 * Parameters:   [in] EVENT_INDEX       EventIndex
*                [in] EVENT_TABLE_INDEX EventTableIndex
 *               [in] UINT32            CurrentTick
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static
BOOLEAN EventTableUpdate ( EVENT_INDEX EventIndex,
                           EVENT_TABLE_INDEX EventTableIndex,
                           UINT32 CurrentTick )
{
   // Local Data
   EVENT_TABLE_CFG  *pTableCfg;
   EVENT_TABLE_DATA *pTableData;
   EVENT_REGION      FoundRegion;
   BOOLEAN           bConfirmed;
   UINT32            CurrentAction;
   UINT32            FoundAction;

   // Initialize Local Data
   pTableCfg    = &m_EventTableCfg [EventTableIndex];
   pTableData   = &m_EventTableData[EventTableIndex];
   FoundRegion  = REGION_NOT_FOUND;

   // Get Sensor Value
   pTableData->CurrentSensorValue = SensorGetValue( pTableCfg->nSensor );

   // Don't do anything until the table is entered
   if ( pTableData->CurrentSensorValue > pTableCfg->TableEntryValue )
   {
       // Check if this Table was already running
      if ( FALSE == pTableData->Started )
      {
         EventTableResetData ( EventTableIndex );
         pTableData->Started      = TRUE;
         pTableData->StartTime_ms = CurrentTick;
         CM_GetTimeAsTimestamp( &pTableData->ExceedanceStartTime );
      }

      // Calculate the duration since the exceedance began
      pTableData->TotalDuration_ms = CurrentTick - pTableData->StartTime_ms;
      // Find out what region we are in
      FoundRegion = EventTableFindRegion ( pTableCfg,
                                           pTableData,
                                           pTableData->CurrentSensorValue );

      // Make sure we are accessing a valid region
      if ( FoundRegion != REGION_NOT_FOUND )
      {
         // Did we leave the last detected region?
         if ( FoundRegion != pTableData->CurrentRegion )
         {
            // New Region Enterd
            GSE_DebugStr ( NORMAL, TRUE, "Entered:   R: %d S: %f D: %d",
                           FoundRegion,  pTableData->CurrentSensorValue,
                           pTableData->TotalDuration_ms );
            // We just entered this region record the time
            pTableData->RegionStats[FoundRegion].EnteredTime = CurrentTick;
            
            // If the current region and new region are both WHEN_MET Regions then we should
            // Reset the Current Event Action
            CurrentAction = pTableCfg->Region[pTableData->CurrentRegion].Action;
            FoundAction   = pTableCfg->Region[FoundRegion].Action;

            if ( (ACTION_WHEN_MET == (CurrentAction & ACTION_WHEN_MET)) &&
                 (ACTION_WHEN_MET == (FoundAction   & ACTION_WHEN_MET)) )
            {
               // Perform any configured event action for the current region
               pTableData->ActionReqNum = ActionRequest ( pTableData->ActionReqNum,
                                                          CurrentAction,
                                                          ACTION_OFF );
            }
            pTableData->ActionReqNum = ActionRequest ( pTableData->ActionReqNum,
                                                       FoundAction,
                                                       ON_MET );
         }

         // Have we confirmed the found region yet?
         if ( FoundRegion != pTableData->ConfirmedRegion )
         {
            // Check if we have been in this region for more than the transient allowance
            bConfirmed =  EventTableConfirmRegion (pTableCfg, pTableData,
                                                   FoundRegion, CurrentTick, EventIndex);

            // Now have we confirmed the region?
            if ( TRUE == bConfirmed )
            {
               // Request the event action.
               pTableData->ActionReqNum = ActionRequest (pTableData->ActionReqNum,
                                                         pTableCfg->Region[FoundRegion].Action,
                                                         ON_DURATION );
            }
         }
      }
      else // We exited all possible regions but are still in the table
      {
         // Exit the last confirmed region
         EventTableExitRegion ( pTableCfg, pTableData, FoundRegion, CurrentTick, EventIndex );
         // We are no longer in a confirmed region, set the confirmed to found
         pTableData->ConfirmedRegion = FoundRegion;
      }
      // Set the current region to whatever we just found
      pTableData->CurrentRegion = FoundRegion;
   }
   else
   {
      // Check if we already started the Event Table Processing
      if ( ( TRUE == pTableData->Started ) && ( 0 != pTableData->TotalDuration_ms ) )
      {
         // The exceedance has ended
         CM_GetTimeAsTimestamp( &pTableData->ExceedanceEndTime );
         // Exit any confirmed region that wasn't previously exited
         EventTableExitRegion ( pTableCfg,  pTableData, FoundRegion, CurrentTick, EventIndex );
         // Log the Event Table
         EventTableLogSummary ( pTableCfg, pTableData, EventTableIndex );
      }
      // Now reset the Started Flag and wait until we cross a region threshold
      pTableData->Started = FALSE;
   }
   return (pTableData->Started);
}

/******************************************************************************
 * Function:     EventTableFindRegion
 *
 * Description:
 *
 * Parameters:
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static
EVENT_REGION EventTableFindRegion ( EVENT_TABLE_CFG *pTableCfg, EVENT_TABLE_DATA *pTableData,
                                    FLOAT32 fSensorValue)
{
   // Local Data
   UINT16        RegIndex;
   UINT16        SegIndex;
   REGION_DEF   *pRegion;
   SEGMENT_DEF  *pSegment;
   LINE_CONST   *pConst;
   EVENT_REGION  FoundRegion;
   FLOAT32       Threshold;
   FLOAT32       SavedThreshold;
   UINT32        Duration;

   // Initialize Local Data
   FoundRegion    = REGION_NOT_FOUND;
   Threshold      = 0;
   SavedThreshold = 0;

   // Loop through all the regions
   for ( RegIndex = 0; ((RegIndex <= pTableData->MaximumCfgRegion)); RegIndex++ )
   {
      pRegion   = &pTableCfg->Region[RegIndex];

      // Check each configured line segment
      for ( SegIndex = 0; (SegIndex < MAX_REGION_SEGMENTS); SegIndex++ )
      {
         pSegment = &pRegion->Segment[SegIndex];
         Duration = pTableData->TotalDuration_ms;
         // Now check if this line segment is valid for this point in time
         if (( Duration >= (pSegment->StartTime_s * MILLISECONDS_PER_SECOND)) &&
             ( Duration <  (pSegment->StopTime_s  * MILLISECONDS_PER_SECOND)))
         {
            pConst    = &pTableData->Segment[RegIndex][SegIndex];

            // We found the point in time for the Regions Line Segment now
            // Check if the sensor value is greater than the line segment at this point
            // Calculate the Threshold based on the constants for this segment
            Threshold = (FLOAT32)
                        ((pConst->m * (Duration / 1000)) + pConst->b);

            // If we haven't already entered or exceeded this region then add a positive
            // Hysteresis Else we need a negetive Hysteresis to exit the region
            Threshold = (pTableData->CurrentRegion < RegIndex) ?
                        Threshold + pTableCfg->HysteresisPos :
                        Threshold - pTableCfg->HysteresisNeg;

            // Check if the sensor is greater than the threshold
            if ( fSensorValue > Threshold )
            {
               FoundRegion    = (EVENT_REGION)RegIndex;
               SavedThreshold = Threshold;
            }
         }
      }
   }

   // If we found a new region then dump a debug message
   if (FoundRegion != pTableData->CurrentRegion)
   {
      GSE_DebugStr(NORMAL,TRUE,"FoundRegion: %d Dur: %d Th: %f S: %f",
                   FoundRegion, pTableData->TotalDuration_ms, SavedThreshold, fSensorValue);
   }
   return FoundRegion;
}

/******************************************************************************
 * Function:     EventTableConfirmRegion
 *
 * Description:
 *
 * Parameters:
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static
BOOLEAN EventTableConfirmRegion ( EVENT_TABLE_CFG  *pTableCfg,
                                  EVENT_TABLE_DATA *pTableData,
                                  EVENT_REGION      FoundRegion,
                                  UINT32            CurrentTick,
                                  EVENT_INDEX       EventIndex     )
{

   // Have we exceeded the threshold for more than the transient allowance OR
   // Are we coming back from a higher region?
   if (((CurrentTick - pTableData->RegionStats[FoundRegion].EnteredTime) >=
         pTableCfg->TransientAllowance_ms) ||
         ( FoundRegion < pTableData->ConfirmedRegion ) )
   {
      // We exceeded the transient allowance check for new max value
      if ( pTableData->CurrentSensorValue > pTableData->MaxSensorValue )
      {
         pTableData->MaxSensorValue          = pTableData->CurrentSensorValue;
         pTableData->MaxSensorElaspedTime_ms = pTableData->TotalDuration_ms;
      }

      // Has this region been confirmed yet?
      if ( FALSE == pTableData->RegionStats[FoundRegion].RegionConfirmed )
      {
         // We now know we are in this region , either because we exceeded
         // the transient allowance or we have entered from a higher region
         pTableData->RegionStats[FoundRegion].RegionConfirmed = TRUE;

         // Check for the maximum region entered
         if ( FoundRegion > pTableData->MaximumRegionEntered )
         {
            pTableData->MaximumRegionEntered = FoundRegion;
         }

         // Count that we have entered this region
         pTableData->RegionStats[FoundRegion].LogStats.EnteredCount++;

         GSE_DebugStr ( NORMAL,TRUE,"Confirmed: R: %d S: %f D: %d",
                        FoundRegion,  pTableData->CurrentSensorValue,
                        pTableData->TotalDuration_ms );

         EventTableExitRegion ( pTableCfg, pTableData, FoundRegion, CurrentTick, EventIndex );
      }
      // We have a new confirmed region now save it
      pTableData->ConfirmedRegion = FoundRegion;
   }
   return pTableData->RegionStats[FoundRegion].RegionConfirmed;
}

/******************************************************************************
 * Function:     EventTableExitRegion
 *
 * Description:
 *
 * Parameters:
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static
void EventTableExitRegion ( EVENT_TABLE_CFG *pTableCfg,  EVENT_TABLE_DATA *pTableData,
                            EVENT_REGION FoundRegion,   UINT32 CurrentTick,
                            EVENT_INDEX  EventIndex )
{
   // Local Data
   UINT16 i;

   // Make sure we are in a confirmed region
   if ( REGION_NOT_FOUND != pTableData->ConfirmedRegion )
   {
      // was the confirmed region actually entered at least once?
      if ( 0 != pTableData->RegionStats[pTableData->ConfirmedRegion].EnteredTime )
      {
         // Count that we exited this region
         pTableData->RegionStats[pTableData->ConfirmedRegion].LogStats.ExitCount++;
         pTableData->RegionStats[pTableData->ConfirmedRegion].RegionConfirmed = FALSE;
         // To get the duration for the region we need to subtract the entered
         // time from the CurrentTick time and then subtract the transient
         // allowance for entering the new region... unless we are coming from
         // a higher region
         pTableData->RegionStats[pTableData->ConfirmedRegion].LogStats.Duration_ms  +=
           ((CurrentTick - pTableData->RegionStats[pTableData->ConfirmedRegion].EnteredTime) -
           ((FoundRegion < pTableData->ConfirmedRegion) ? 0:pTableCfg->TransientAllowance_ms));
         // Reset the entered time so we don't keep performing this calculation
         pTableData->RegionStats[pTableData->ConfirmedRegion].EnteredTime = 0;

         GSE_DebugStr(NORMAL,TRUE,"Exit:      R: %d", pTableData->ConfirmedRegion );
      }
   }
   // Clear any other event actions now that we confirmed this region
   for ( i = REGION_A; i < MAX_TABLE_REGIONS; i++ )
   {
      // Make sure we don't shut off the new found region.
      if ( i != FoundRegion )
      {
         pTableData->ActionReqNum = ActionRequest ( pTableData->ActionReqNum,
                                                    pTableCfg->Region[i].Action,
                                                    ACTION_OFF );
      }
   }
}


/******************************************************************************
 * Function:     EventTableLogSummary
 *
 * Description:  Gathers and records the Event Table Summary Data
 *
 * Parameters:   [in] EVENT_TABLE_CFG   *pConfig
 *               [in] EVENT_TABLE_DATA  *pData
 *               [in] EVENT_TABLE_INDEX EventTableIndex
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static
void EventTableLogSummary (EVENT_TABLE_CFG *pConfig, EVENT_TABLE_DATA *pData,
                           EVENT_TABLE_INDEX EventTableIndex)
{
   // Local Data
   EVENT_TABLE_SUMMARY_LOG TSumm;
   UINT16                  RegIndex;

   // Gather up the data to write to the log
   TSumm.EventTableIndex         = EventTableIndex;
   TSumm.SensorIndex             = pConfig->nSensor;
   TSumm.ExceedanceStartTime     = pData->ExceedanceStartTime;
   TSumm.ExceedanceEndTime       = pData->ExceedanceEndTime;
   TSumm.Duration_ms             = pData->TotalDuration_ms;
   TSumm.MaximumRegionEntered    = pData->MaximumRegionEntered;
   TSumm.MaxSensorValue          = pData->MaxSensorValue;
   TSumm.MaxSensorElaspedTime_ms = pData->MaxSensorElaspedTime_ms;
   TSumm.MaximumCfgRegion        = pData->MaximumCfgRegion;

   // Loop through the region stats and place them in the log
   for ( RegIndex = 0; RegIndex < MAX_TABLE_REGIONS; RegIndex++ )
   {
      TSumm.RegionStats[RegIndex].EnteredCount =
                                  pData->RegionStats[RegIndex].LogStats.EnteredCount;
      TSumm.RegionStats[RegIndex].ExitCount    =
                                  pData->RegionStats[RegIndex].LogStats.ExitCount;
      TSumm.RegionStats[RegIndex].Duration_ms  =
                                  pData->RegionStats[RegIndex].LogStats.Duration_ms;
   }

   // Write the LOG
   LogWriteETM    ( APP_ID_EVENT_TABLE_SUMMARY,
                    LOG_PRIORITY_LOW,
                    &TSumm,
                    sizeof(TSumm),
                    NULL);
}

/******************************************************************************
 * Function:     EventLogEnd
 *
 * Description:  Log the event end record
 *
 *
 * Parameters:   [in] EVENT_CONFIG *pEventCfg,
 *               [in] EVENT_DATA   *pEventData
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static
void EventLogEnd (EVENT_CFG *pConfig, EVENT_DATA *pData)
{
   GSE_DebugStr(NORMAL,TRUE,"Event End (%d) %s", pData->EventIndex, pConfig->EventName);

   //Log the data collected
   EventLogUpdate( pConfig, pData );
   LogWriteETM    ( APP_ID_EVENT_ENDED,
                    LOG_PRIORITY_LOW,
                    &m_EventEndLog[pData->EventIndex],
                    sizeof(m_EventEndLog[pData->EventIndex]),
                    NULL);
}


/******************************************************************************
 * Function:     EventUpdateData
 *
 * Description:  The Event Update Data is responsible for collecting the
 *               min/max/avg for all the event configured sensors.
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
void EventUpdateData ( EVENT_CFG *pConfig, EVENT_DATA *pData )
{
   // Local Data
   FLOAT32          NewValue;
   SNSR_SUMMARY     *pSummary;
   UINT8            numSensor;
   UINT32           CurrentTick;

   // Initialize Local Data
   CurrentTick = CM_GetTickCount();

   // if duration == 0 the Event needs to be initialized
   if ( 0 == pData->nStartTime_ms )
   {
      // general Event initialization
      pData->nStartTime_ms   = CurrentTick;
      pData->nSampleCount    = 0;
      CM_GetTimeAsTimestamp(&pData->CriteriaMetTime);
      memset(&pData->DurationMetTime,0, sizeof(TIMESTAMP));
      memset(&pData->EndTime        ,0, sizeof(TIMESTAMP));
   }

   // update the sample count for the average calculation
   pData->nSampleCount++;

   // Loop through the Event Sensors
   for ( numSensor = 0; numSensor < pData->nTotalSensors; numSensor++ )
   {
      // Set a pointer to the data summary
      pSummary    = &(pData->Sensor[numSensor]);

      // Verify the sensor is used
      if ( SensorIsUsed((SENSOR_INDEX)pSummary->SensorIndex ) )
      {
         // Get the sensors latest value
         NewValue = SensorGetValue( (SENSOR_INDEX)pSummary->SensorIndex );

         // Add the new value and calculate the min and max
         pSummary->fTotal += NewValue;
         pSummary->fMinValue =
            (NewValue < pSummary->fMinValue) ? NewValue : pSummary->fMinValue;
         pSummary->fMaxValue =
            (NewValue > pSummary->fMaxValue) ? NewValue : pSummary->fMaxValue;
         // Set the sensor validity
         pSummary->bValid = SensorIsValid((SENSOR_INDEX)pSummary->SensorIndex );
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
BOOLEAN EventCheckDuration ( EVENT_CFG *pConfig, EVENT_DATA *pData )
{
   // update the Event duration
   pData->nDuration_ms = CM_GetTickCount() - pData->nStartTime_ms;

   return ( pData->nDuration_ms >= pConfig->nMinDuration_ms );
}

/******************************************************************************
 * Function:     EventLogUpdate
 *
 * Description:  The event log update function creates a log
 *               containing the event data.
 *
 * Parameters:   [in] EVENT_CONFIG *pEventCfg,
 *               [in] EVENT_DATA   *pEventData
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static
void EventLogUpdate( EVENT_CFG *pConfig, EVENT_DATA *pData)
{
  // Local Data
  FLOAT32       oneOverN;
  EVENT_END_LOG *pLog;
  SNSR_SUMMARY  *pSummary;
  UINT8         numSensor;

  // Initialize Local Data
  pLog = &m_EventEndLog[pData->EventIndex];

  // Calculate the multiplier for the average
  oneOverN = (1.0f / (FLOAT32)pData->nSampleCount);

  // update the Event duration
  pLog->nDuration_ms  = CM_GetTickCount() - pData->nStartTime_ms;

  // Update the Event end log
  pLog->EventIndex      = pData->EventIndex;
  pLog->EndType         = pData->EndType;
  pLog->CriteriaMetTime = pData->CriteriaMetTime;
  pLog->DurationMetTime = pData->DurationMetTime;
  pLog->EndTime         = pData->EndTime;

  // Loop through all the sensors
  for ( numSensor = 0; numSensor < MAX_EVENT_SENSORS; numSensor++)
  {
    // Set a pointer the summary
    pSummary = &(pData->Sensor[numSensor]);

    // Check for a valid sensor index and the sensor is used
    if ( ( pSummary->SensorIndex < MAX_SENSORS ) &&
          SensorIsUsed((SENSOR_INDEX)pSummary->SensorIndex ) )
    {

       pSummary->bValid = SensorIsValid((SENSOR_INDEX)pSummary->SensorIndex);
       // Update the summary data for the trigger
       pLog->Sensor[numSensor].SensorIndex = pSummary->SensorIndex;
       pLog->Sensor[numSensor].fTotal      = pSummary->fTotal;
       pLog->Sensor[numSensor].fMinValue   = pSummary->fMinValue;
       pLog->Sensor[numSensor].fMaxValue   = pSummary->fMaxValue;
       pLog->Sensor[numSensor].fAvgValue   = pSummary->fTotal * oneOverN;
       pLog->Sensor[numSensor].bValid      = pSummary->bValid;
    }
    else // Sensor Not Used
    {
       pLog->Sensor[numSensor].SensorIndex   = SENSOR_UNUSED;
       pLog->Sensor[numSensor].fTotal        = 0.0;
       pLog->Sensor[numSensor].fMinValue     = 0.0;
       pLog->Sensor[numSensor].fMaxValue     = 0.0;
       pLog->Sensor[numSensor].fAvgValue     = 0.0;
       pLog->Sensor[numSensor].bValid        = FALSE;
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
      if ( EVENT_ACTIVE == pEventData->State )
      {
         // Record the End Time for the event
         CM_GetTimeAsTimestamp ( &pEventData->EndTime );
         // Record the type of end for the event
         pEventData->EndType = EVENT_COMMANDED_END;
         // Populate the End log data
         EventLogUpdate ( pEventCfg, pEventData );
         // Write the Log
         LogWriteETM    ( APP_ID_EVENT_ENDED,
                          pEventCfg->Priority,
                          &m_EventEndLog[pEventData->EventIndex],
                          sizeof ( m_EventEndLog[pEventData->EventIndex] ),
                          NULL );
      }
   }
}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: Event.c $
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
 
