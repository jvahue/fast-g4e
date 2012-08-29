#define ENGINERUN_BODY
/******************************************************************************
              Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
                 All Rights Reserved. Proprietary and Confidential.


    File:      EngineRun.c  

    Description:

   VERSION
      $Revision: 18 $  $Date: 8/29/12 12:28p $
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <string.h>
#include <ctype.h>


/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "cycle.h"
#include "EngineRun.h"
#include "Utility.h"
#include "CfgManager.h"
#include "trigger.h"
#include "Gse.h"
#include "assert.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
//#define DEBUG_ENGRUN

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static ENGRUN_CFG    EngineRunCfg[MAX_ENGINES];
static ENGRUN_DATA   EngineRunData[MAX_ENGINES];

static ENGRUN_STARTLOG EngineStartLog[MAX_ENGINES];
static ENGRUN_RUNLOG   EngineRunLog[MAX_ENGINES];


// Include cmd tables and functions here after local dependencies are declared.
#include "EngineRunUserTables.c"


/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void EngRunEnableTask ( BOOLEAN bEnable );
static void EngRunForceEnd   ( void );
static void EngRunReset   (ENGRUN_CFG* pErCfg, ENGRUN_DATA* pErData);

static void EngRunUpdateAll(void);

static void EngRunUpdate (ENGRUN_CFG* pErCfg, ENGRUN_DATA* pErData);

static void EngRunStartLog       ( ENGRUN_CFG* pErCfg, ENGRUN_DATA* pErData );
static void EngRunUpdateStartData( ENGRUN_CFG* pErCfg,
                                   ENGRUN_DATA* pErData,
                                  BOOLEAN bUpdateDuration);

static void EngRunWriteStartLog ( ER_REASON reason, ENGRUN_CFG* pErCfg, ENGRUN_DATA* pErData );

static void EngRunUpdateRunData     ( ENGRUN_CFG* pErCfg, ENGRUN_DATA* pErData );
static void EngRunWriteRunLog   ( ER_REASON reason, ENGRUN_CFG* pErCfg, ENGRUN_DATA* pErData );

static BOOLEAN EngRunIsError     (ENGRUN_CFG* pErCfg);

/*****************************************************************************/
/* Public Functions                                                          */                
/*****************************************************************************/

/******************************************************************************
 * Function:     EngRunGetState
 *
 * Description:  Returns the current state of the EngineRun 'idx' or an UINT8 bit array 
 *               of the engineruns which are in the RUNNING state.
 *
 * Parameters:   [in] idx - ENGRUN_ID of the EngineRun being requested or
 *                          ENGRUN_ID_ANY to return the bit array of all enginerun
 *                          in the STARTING or RUNNING state.
 *               [out]      Pointer to the EngRunFlags bit array of engines in RUNNING state.
 *                          This field is not updated if idx is not set to ENGRUN_ID_ANY.
 *
 * Returns:      ER_STATE   if idx  is not ENGRUN_ID_ANY, state of the requested engine run is 
 *                          returned. Otherwise returns one of the following based on a 
*                           prioritized check: RUNNING, STARTING, STOPPED
 *
 * Notes:        None
 *
 *****************************************************************************/
ER_STATE EngRunGetState(ENGRUN_INDEX idx, UINT8* EngRunFlags)
{
  INT16 i;
  UINT8 runMask  = 0x00;  // bit map of started/running EngineRuns.
  ER_STATE state = ER_STATE_STOPPED;
  BOOLEAN  bRunning  = FALSE;
  BOOLEAN  bStarting = FALSE;

  // If the user wants a list of ALL running engine-runs, return
  // it in the provided location. 
  if ( ENGRUN_ID_ANY == idx )
  {    
    runMask = 0x00;

    for (i = 0; i < MAX_ENGINES; ++i)
    {
      if (ER_STATE_RUNNING == EngineRunData[i].State)
      {
        // Set the bit in the flag
        runMask |= 1 << i;
        bRunning = TRUE;
      }
      else if (ER_STATE_STARTING == EngineRunData[i].State)
      {
        // Set the bit in the flag
        runMask |= 1 << i;
        bStarting = TRUE;
      }      
    }

    // Assign overall engine run state based in the following priority:
    // RUNNING->STARTING->STOPPED
    state = bRunning ? ER_STATE_RUNNING : bStarting ? ER_STATE_STARTING : ER_STATE_STOPPED;

    // If the caller has passed a field to return the list, fill it.
    if(NULL != EngRunFlags)
    {
      *EngRunFlags = runMask;
    }
  }
  else // Get the current state of the requested engine run.
  {
    ASSERT ( idx >= ENGRUN_ID_0 && idx < MAX_ENGINES);
    state = EngineRunData[idx].State;
  }
  return state;
}

/******************************************************************************
 * Function:     EngRunInitialize
 *
 * Description:  Initializes the Engine Run and 
 *
 * Parameters:   [in] pParam (i) - dummy to match function signature.
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
void EngRunInitialize(void)
{
  TCB TaskInfo;
  UINT16 i;
  ENGRUN_CFG*  pErCfg;
  ENGRUN_DATA* pErData;

  // Add user commands for EngineRun to the user command tables.

  User_AddRootCmd(&RootEngRunMsg);

  // Clear and Load the current cfg info.
  memset(&EngineRunCfg, 0, sizeof(EngineRunCfg));

  memcpy( EngineRunCfg,
          &(CfgMgr_RuntimeConfigPtr()->EngineRunConfigs),
          sizeof(EngineRunCfg) );

  // Initialize Engine Runs storage objects.
  for ( i = 0; i < MAX_ENGINES; i++)
  {
    pErCfg  = &EngineRunCfg[i];
    pErData = &EngineRunData[i];
    // Cfg must specify valid sensor for start, run, stop, temp and battery
    if ( TriggerIsConfigured(pErCfg->StartTrigID)     &&
         TriggerIsConfigured(pErCfg->StopTrigID )     &&
         TriggerIsConfigured(pErCfg->RunTrigID  )     &&
         SensorIsUsed       (pErCfg->MonMinSensorID ) &&
         SensorIsUsed       (pErCfg->MonMaxSensorID ) )
    {
      // Initialize the common fields in the EngineRun data structure
      pErData->ErIndex = (ENGRUN_INDEX)i;
      EngRunReset(pErCfg,pErData);
    }
    else // Invalid config... 
    {
      // Set ER state to hold in STOP state if
      // sensor Cfg is invalid
      pErData->State   = ER_STATE_STOPPED;
      pErData->ErIndex = ENGRUN_UNUSED;
    }    
  }
 
  // Create EngineRun Task - DT
  memset(&TaskInfo, 0, sizeof(TaskInfo));
  strncpy_safe(TaskInfo.Name, sizeof(TaskInfo.Name),"EngineRun Task",_TRUNCATE);
  TaskInfo.TaskID           = EngRun_Task;
  TaskInfo.Function         = EngRunTask;
  TaskInfo.Priority         = taskInfo[EngRun_Task].priority;
  TaskInfo.Type             = taskInfo[EngRun_Task].taskType;
  TaskInfo.modes            = taskInfo[EngRun_Task].modes;
  TaskInfo.MIFrames         = taskInfo[EngRun_Task].MIFframes;
  TaskInfo.Enabled          = TRUE;
  TaskInfo.Locked           = FALSE;

  TaskInfo.Rmt.InitialMif   = taskInfo[EngRun_Task].InitialMif;
  TaskInfo.Rmt.MifRate      = taskInfo[EngRun_Task].MIFrate;
  TaskInfo.pParamBlock      = NULL;
  TmTaskCreate (&TaskInfo);
}

/******************************************************************************
 * Function:     EngRunEnableTask
 *
 * Description:  Function used to enable and disable EngineRun
 *               processing.
 *
 * Parameters:   BOOLEAN bEnable - Enables the enginerun task.
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static void EngRunEnableTask ( BOOLEAN bEnable )
{
   TmTaskEnable (EngRun_Task, bEnable);
}
/******************************************************************************
 * Function:     EngRunTask
 *
 * Description:  Monitors transitions for starting and ending engine run.
 *
 * Parameters:   [in] pParam (i) - dummy to match function signature.
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
void EngRunTask(void* pParam)
{
  if (Tm.systemMode == SYS_SHUTDOWN_ID)
  {    
    EngRunForceEnd();
    EngRunEnableTask(FALSE);

  }
  else
  {
    EngRunUpdateAll();
  }
}

/******************************************************************************
 * Function:     EngRunGetPtrToCycleCounts
 *
 * Description:  Returns pointer to the data collection area for this engine-run .
 *               This function is called to retrieve cycle counts owned by the 
 *               indicated engine run object.
 *
 * Parameters:   [in] engId
 *
 * Returns:      Pointer to the array of floats containing the cycle counts 
 *
 * Notes:        None
 *
 *****************************************************************************/
UINT32* EngRunGetPtrToCycleCounts(ENGRUN_INDEX engId)
{
  ASSERT(engId < MAX_ENGINES); 
  return &EngineRunData[engId].CycleCounts[0];
}

/******************************************************************************
 * Function:     EngRunGetPtrToLog
 *
 * Description:  Returns pointer to the engine run log area for this engine-run .
 *               This function is called to retrieve cycle counts owned by the 
 *               indicated engine run object.
 *
 * Parameters:   [in] engId
 *
 * Returns:      Pointer to the engine run log 
 *
 * Notes:        None
 *
 *****************************************************************************/
ENGRUN_RUNLOG* EngRunGetPtrToLog(ENGRUN_INDEX engId)
{
  ASSERT(engId < MAX_ENGINES); 
  return &EngineRunLog[engId];
}


/******************************************************************************
 * Function:     EngRunGetStartingTime
 *
 * Description:  Returns value of the starting time 
 *               tick of the START for this engine-run .
 *
 * Parameters:   [in] engId
 *
 * Returns:      Engine run data starting time 
 *
 * Notes:        None
 *
 *****************************************************************************/
EXPORT UINT32 EngRunGetStartingTime(ENGRUN_INDEX engId)
{
  ASSERT(engId < MAX_ENGINES);
  return EngineRunData[engId].startingTime;
}
/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:     EngRunForceEnd
 *
 * Description:  Called during shutdown to update persist and write logs.
 *
 * Parameters:   None
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static void EngRunForceEnd( void )
{
  ENGRUN_INDEX i;
  ENGRUN_CFG*  pErCfg;
  ENGRUN_DATA* pErData;

  // Close out any active cycle
  // and flag the task to stop.
  // Eng
  for (i = ENGRUN_ID_0; i < MAX_ENGINES; ++i)
  {
    pErCfg  = &EngineRunCfg[i];
    pErData = &EngineRunData[i];

    switch (pErData->State)
    {
      case ER_STATE_STOPPED:
        break;
      
      case ER_STATE_STARTING:
        EngRunWriteStartLog( ER_LOG_SHUTDOWN, pErCfg, pErData);
        CycleFinishEngineRun(i);
        break;

      case ER_STATE_RUNNING:
        // Update persist, and create log
        EngRunWriteRunLog(ER_LOG_SHUTDOWN, pErCfg, pErData);
        break;

      default:
        FATAL("Unrecognized engine run state %d", pErData->State );
        break;
    }    
    
    EngRunWriteRunLog(ER_LOG_STOPPED, pErCfg, pErData);

    CycleFinishEngineRun(i);
  }

}


/******************************************************************************
 * Function:     EngRunReset
 *
 * Description:  clear out any leftover
 *               vars that will confuse future processing.
 *
 * Parameters:   [in] pErCfg - engine run cfg
 *               [in/out] pErData - engine run data
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static void EngRunReset(ENGRUN_CFG* pErCfg, ENGRUN_DATA* pErData)
{
  INT32 i;

  {
    pErData->State   = ER_STATE_STOPPED;
    memset(&pErData->StartTime, 0, sizeof(TIMESTAMP));
   //memset(&pErData->EndTime, 0, sizeof(TIMESTAMP));    
    pErData->startingTime        = 0;
    pErData->StartingDuration_ms = 0;
    pErData->Duration_ms         = 0;
    pErData->MonMinValue         = FLT_MAX;
    pErData->MonMaxValue         = 0.f;   
    pErData->nSampleCount        = 0;
    pErData->nRateCounts         = (UINT16)(MIFs_PER_SECOND / pErCfg->Rate);
    pErData->nRateCountdown      = (UINT16)((pErCfg->nOffset_ms / MIF_PERIOD_mS) + 1);

    /* SNSR_SUMMARY field setup */

    pErData->nTotalSensors = SensorSetupSummaryArray(pErData->SnsrSummary,
                                                     MAX_ENGRUN_SENSORS,
                                                     pErCfg->SensorMap,
                                                     sizeof(pErCfg->SensorMap) );
    // CYCLES Init    
    for(i = 0; i < MAX_CYCLES; ++i)
    {
      pErData->CycleCounts[i] = 0;
    }    
  }
  
}

/******************************************************************************
 * Function:     EngRunUpdateAll
 *
 * Description:  Calls the update function for each  defined EngineRun
 *               
 *
 * Parameters:   None
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static void EngRunUpdateAll(void)
{
  UINT16 i;
  for ( i = 0; i < MAX_ENGINES; i++)
  {
    if (EngineRunData[i].ErIndex != ENGRUN_UNUSED)
    {
      EngRunUpdate(&EngineRunCfg[i],&EngineRunData[i]);
    }    
  }
}

/******************************************************************************
 * Function:     EngRunUpdate
 *
 * Description:  Update Engine Run data for the specified engine
 *               
 *
 * Parameters:   [in] pErCfg - engine run cfg
 *               [in/out] pErData - engine run data
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static void EngRunUpdate( ENGRUN_CFG* pErCfg, ENGRUN_DATA* pErData)
{
 #ifdef DEBUG_ENGRUN 
  ER_STATE curState;
#endif
 
  // Is it time for this EngineRun to run?
  if (--pErData->nRateCountdown <= 0)
  {
    // Reset the countdown counter for the next enginerun.
    pErData->nRateCountdown = pErData->nRateCounts;

    // Update the cycles for this engine run if starting or running.  
    if ( pErData->State == ER_STATE_STARTING ||
         pErData->State == ER_STATE_RUNNING )
    {
      CycleUpdateAll(pErData->ErIndex);
    }

    // Process the current engine run state
#ifdef DEBUG_ENGRUN 
    curState = pErData->State;
#endif
    
    switch ( pErData->State)
    {
      case ER_STATE_STOPPED:

        // If the the engine-run is not-configured or unused,
        // hold in this state.
        // todo DaveB need to still maintain min and max values
        if (ENGRUN_UNUSED == pErData->ErIndex)
        {
          break;
        }

        // Monitor configured min/max sensors, set flag 
        // FALSE to prevent update start duration.
        EngRunUpdateStartData(pErCfg, pErData, FALSE);


        // Check that all transition-trigger are valid,
        // then see if engine start/running is occurring
        // if the engine is running ( ex: system restart in-air),
        // we still go thru START to set up initialization and logs
        if ( !EngRunIsError(pErCfg) )
        {          
          if ( TriggerGetState( pErCfg->StartTrigID) ||
               TriggerGetState( pErCfg->RunTrigID  ) )
          {            
            
            // init the start-log entry and set start-time 
            // clear the cycle counts for this engrun
            EngRunStartLog(pErCfg, pErData);
            CycleResetEngineRun(pErData->ErIndex);

            pErData->State = ER_STATE_STARTING;            
          }
        }        
        break;

    case ER_STATE_STARTING:      
      // Always update the ER Starter parameters.
      EngRunUpdateStartData(pErCfg, pErData, TRUE);
      
      // STARTING -> STOP
      // Error Detected
      // If we have a problem determining the EngineRun state,
      // write the engine run start-log and transition to STOPPED state
      if ( EngRunIsError(pErCfg))
      {
        // Add additional period to close of ER due to ERROR.  Normally
        // UpdateEngineRunLog() updates Duration_ms, which is only called on !EngineRunIsError()
        // todo DaveB - see if this is still needed.
        pErData->Duration_ms += pErCfg->Rate;

        // Finish the engine run log       
        EngRunWriteStartLog( ER_LOG_ERROR, pErCfg, pErData);
        pErData->State = ER_STATE_STOPPED;
        break;
      }

      //
      // Update the Engine Run-log data during START state
      EngRunUpdateRunData(pErCfg, pErData);

      // STARTING -> RUNNING
      if ( TriggerGetState( pErCfg->RunTrigID) )
      {
        // Write ETM START log
        EngRunWriteStartLog( ER_LOG_RUNNING, pErCfg, pErData);
        pErData->State = ER_STATE_RUNNING;
        break;
      }

      // STARTING -> STOP
      // STARTING -> (Aborted) -> STOP
      if ( TriggerGetState( pErCfg->StopTrigID) ||
          !TriggerGetState( pErCfg->StartTrigID) )
      {      
        // Finish the engine run log
        EngRunWriteRunLog(ER_LOG_STOPPED, pErCfg, pErData);
        EngRunReset(pErCfg, pErData);
        pErData->State = ER_STATE_STOPPED;
        break;
      }
      break;

    case ER_STATE_RUNNING:
      // If we have a problem determining the EngineRun state for this engine run,
      // end the engine run log, and transition to error mode

      // RUNNING -> (error) -> STOP
      if ( EngRunIsError(pErCfg))
      {
        // Add additional 1 second to close of ER due to ERROR.  Normally
        // UpdateEngineRunLog() updates ->Duration, which is only called on !EngineRunIsError()
        pErData->Duration_ms += pErCfg->Rate;

        // Finish the engine run log
        EngRunWriteRunLog(ER_LOG_ERROR, pErCfg, pErData);
        EngRunReset(pErCfg, pErData);
        pErData->State = ER_STATE_STOPPED;
        break;
      }

      // Update the EngineRun log data
      // Allow additional update so that is ER is closed, this last second
      // is counted.
      EngRunUpdateRunData( pErCfg, pErData);

      // If the engine run has stopped, finish the engine run and change states
      // RUNNING -> STOP
      if ( TriggerGetState( pErCfg->StopTrigID) ||
          !TriggerGetState( pErCfg->RunTrigID) )
      {
        // Finish the engine run log
        EngRunWriteRunLog(ER_LOG_STOPPED, pErCfg, pErData);
        EngRunReset(pErCfg, pErData);
        pErData->State = ER_STATE_STOPPED;
      }
      break;

    default:
      FATAL("Unrecognized engine run state %d", pErData->State );
      break;
      
    }

    // Display msg when state changes.
#ifdef DEBUG_ENGRUN
    if ( pErData->State !=  curState )
    {
     GSE_DebugStr ( NORMAL, TRUE, "EngineRun(%d) %s -> %s ",
                    pErData->ErIndex,
                    EngineRunStateEnum[curState].Str,
                    EngineRunStateEnum[pErData->State].Str);
    }
#endif
  
  }
}

/******************************************************************************
* Function:     EngRunStartLog
*
* Description:  Start taking EngineRunlog data
*               
*
* Parameters:   [in] pErCfg
*               [in/out] pErData
*
* Returns:      None
*
* Notes:        None
*
*****************************************************************************/
static void EngRunStartLog( ENGRUN_CFG* pErCfg, ENGRUN_DATA* pErData )
{
  UINT8 i;
  SNSR_SUMMARY*   pSnsr;
  
  // Set the start timestamp, init the end timestamp
  CM_GetTimeAsTimestamp(&pErData->StartTime);
  
  // Set the 'tick count' of the start time( used to calc duration)
  pErData->startingTime = CM_GetTickCount();
  pErData->nSampleCount = 0;
  pErData->Duration_ms  = 0;

  // Init the current values of the starting sensors.
  // Note: Validity was already checked as a pre-condition of calling this function.)
  pErData->MonMaxValue = SensorGetValue(pErCfg->MonMaxSensorID);
 
  pErData->MonMinValue = pErData->MonMinValue;

  // Initialize the summary values for monitored sensors

  for (i = 0; i < pErData->nTotalSensors; ++i)
  {
    pSnsr = &(pErData->SnsrSummary[i]);

    pSnsr->bValid = SensorIsValid(pSnsr->SensorIndex);
    // todo DaveB is this really necessary ?... it will be updated anyway during EngRunUpdateLog
    if(pSnsr->bValid)
    {
      pSnsr->fMaxValue = SensorGetValue(pSnsr->SensorIndex);
    }
  }

  // Initialize the cycle counts.
  for( i = 0; i < MAX_CYCLES; ++i)
  {
    pErData->CycleCounts[i] = 0;
  }
}

/******************************************************************************
 * Function:     EngRunWriteStartLog
 *
 * Description:  
 *               
 *
 * Parameters:   [in] reason
 *               [in] pErCfg
 *               [in] pErData
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static void EngRunWriteStartLog( ER_REASON reason, ENGRUN_CFG* pErCfg, ENGRUN_DATA* pErData )
{
  ENGRUN_STARTLOG* pLog;

  pLog = &EngineStartLog[pErData->ErIndex]; 

  pLog->ErIndex             = pErData->ErIndex;
  pLog->Reason              = reason;
  pLog->StartTime           = pErData->StartTime;
  CM_GetTimeAsTimestamp(&pLog->EndTime);
  pLog->StartingDuration_ms = pErData->StartingDuration_ms;
 
  pLog->MaxStartSensorId    = pErCfg->MonMaxSensorID;
  pLog->MonMaxValue         = pErData->MonMaxValue;
  pLog->MinStartSensorId    = pErCfg->MonMinSensorID;
  pLog->MonMinValue         = pErData->MonMinValue;
  
  LogWriteETM( APP_ID_ENGINERUN_STARTED, 
               LOG_PRIORITY_LOW,
               pLog,
               sizeof(ENGRUN_STARTLOG),
               NULL);

#ifdef DEBUG_ENGRUN
  GSE_DebugStr(NORMAL,TRUE,"EngRun StartLog written");
#endif
}


/******************************************************************************
 * Function:     EngRunWriteRunLog
 *
 * Description:  
 *               
 *
 * Parameters:   [in] reason
 *               [in] pErCfg
 *               [in] pErData
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static void EngRunWriteRunLog( ER_REASON reason, ENGRUN_CFG* pErCfg, ENGRUN_DATA* pErData )
{
  INT32   i;
  FLOAT32 oneOverN;
  ENGRUN_RUNLOG* pLog;

  SNSR_SUMMARY*  pErSummary;
  SNSR_SUMMARY*  pLogSummary;

  pLog = &EngineRunLog[pErData->ErIndex];

  oneOverN = (1.0f / (FLOAT32)pErData->nSampleCount);

  // Finish the cycles for this engine run
  CycleFinishEngineRun( pErData->ErIndex);

  pLog->ErIndex             = pErData->ErIndex;
  pLog->Reason              = reason;
  pLog->StartTime           = pErData->StartTime;
  CM_GetTimeAsTimestamp(&pLog->EndTime);
  pLog->StartingDuration_ms = pErData->StartingDuration_ms;
  pLog->Duration_ms         = pErData->Duration_ms; 

#pragma ghs nowarning 1545 //Suppress packed structure alignment warning
  for (i = 0; i < MAX_ENGRUN_SENSORS; ++i)
  {    
    pErSummary    = &(pErData->SnsrSummary[i]);
    pLogSummary   = &(pLog->SnsrSummary[i]);

    if ( i < pErData->nTotalSensors)
    {
      pLogSummary->SensorIndex = pErSummary->SensorIndex;
      pLogSummary->bValid      = pErSummary->bValid;
      pLogSummary->fTotal      = pErSummary->fTotal;
      pLogSummary->fMinValue   = pErSummary->fMinValue;
      pLogSummary->fMaxValue   = pErSummary->fMaxValue;

      // if sensor is valid, calculate the final average,
      // otherwise use the average calculated at the point it went invalid.
      pLogSummary->fAvgValue = (pErSummary->bValid) ? pErSummary->fTotal * oneOverN
                                                    : pErSummary->fAvgValue;
    }
    else // Sensor Not Used
    {
      pLogSummary->SensorIndex = SENSOR_UNUSED;
      pLogSummary->bValid      = FALSE;
      pLogSummary->fTotal      = 0.f;
      pLogSummary->fMinValue   = 0.f;
      pLogSummary->fMaxValue   = 0.f;
      pLogSummary->fAvgValue   = 0.f;
    }    
  }
#pragma ghs endnowarning  

  for (i = 0; i < MAX_CYCLES; ++i)
  {
    pLog->CycleCounts[i] =  pErData->CycleCounts[i];
  }

 LogWriteETM( APP_ID_ENGINERUN_ENDED,
              LOG_PRIORITY_LOW,
              pLog,
              sizeof(ENGRUN_RUNLOG),
              NULL);

#ifdef DEBUG_ENGRUN
 GSE_DebugStr(NORMAL,TRUE,"EngRun EndLog written");
#endif

}

/******************************************************************************
 * Function:     EngRunUpdateRunData
 *
 * Description:  Update the log with the sensor elements for this engine run
 *               
 *
 * Parameters:   [in] pErCfg - engine run cfg
 *               [in] pErData - engine run data
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static void EngRunUpdateRunData( ENGRUN_CFG* pErCfg, ENGRUN_DATA* pErData)
{
  UINT8    i;  
  FLOAT32  oneOverN;
  SNSR_SUMMARY* pSummary;


  // if the starting time is zero, needs to be initialized
  // todo DaveB  - this could be a code coverage issue
  // unless we can transition directly from STOPPED to RUNNING.
  // Normally startingTime is set on STOPPED->START transition. 
  if ( 0 == pErData->startingTime )
  {
    // general Trigger initialization    
    pErData->startingTime = CM_GetTickCount();
    pErData->nSampleCount = 0;
  }

  // Update the total run duration
  pErData->Duration_ms = CM_GetTickCount() - pErData->startingTime;
  
  // update the sample count so average can be calculated at end of engine run.
  pErData->nSampleCount++;
  
  // Loop thru all sensors handled by this this ER
  for ( i = 0; i < pErData->nTotalSensors; i++ )
  {
    pSummary = &pErData->SnsrSummary[i];
        
    // If the sensor is known to be invalid and was valid in the past(initialized)...
    // ignore processing for the remainder of this engine run.    
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
     oneOverN = (1.0f / (FLOAT32)(pErData->nSampleCount - 1));
     pSummary->fAvgValue = pSummary->fTotal * oneOverN;      
    }
    
  } // for nTotalSensors  
}
/******************************************************************************
 * Function:     EngRunIsError
 *
 * Description:  Checks and returns the state of all EngineRun determining
 *               factors.
 *
 * Parameters:   [in] pErCfg - engine run cfg
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static BOOLEAN EngRunIsError( ENGRUN_CFG* pErCfg)
{
  BOOLEAN allValid;
  
  // Make sure that each sensor in the engine run has not faulted
  allValid = (TriggerValidGetState(pErCfg->StartTrigID) &&
              TriggerValidGetState(pErCfg->RunTrigID)    &&
              TriggerValidGetState(pErCfg->StopTrigID) );

  return !allValid;
}

/******************************************************************************
 * Function:     EngRunUpdateStartData
 *
 * Description:  Update the EngineRun start data.
 *               The enginerun is in ER_STATE_STARTING.
 *
 * Parameters:   [in] pErCfg - config
 *               [in] pErData - data
 *               [in] bUpdateDuration - start duration update flag.
 *
 * Returns:      None
 *
 * Notes:        bUpdateDuration is set to false when called during STOP state to
 *               maintain min/max values only. During START state, it is called with
 *               true to maintain the start duration elapsed time.
 *
 *****************************************************************************/
static void EngRunUpdateStartData( ENGRUN_CFG* pErCfg,
                                  ENGRUN_DATA* pErData,
                                  BOOLEAN bUpdateDuration)
{
  FLOAT32 valueMin;
  FLOAT32 valueMax;
  BOOLEAN validMin;
  BOOLEAN validMax;
  
  // Update the min
  if ( SensorIsValid(pErCfg->MonMinSensorID) )
  {
   
    valueMin  = SensorGetValue(pErCfg->MonMinSensorID);
    validMin = TRUE;

    if ( pErData->MonMinValue > valueMin )
    {
      pErData->MonMinValue = valueMin;
    }
  }
  else
  {
    validMin = FALSE;
  }

  // update max
  if ( SensorIsValid(pErCfg->MonMaxSensorID) )
  {
    valueMax = SensorGetValue(pErCfg->MonMaxSensorID);
    validMax = TRUE;
    if ( pErData->MonMaxValue < valueMax)
    {
      pErData->MonMaxValue   = valueMax;        
    }
  }
  else
  {
    validMax = FALSE;
  }

  // todo DaveB - verify requirements on how to log start values when
  // invalid.   
  pErData->MonMinValue = validMin ? pErData->MonMinValue :
                                SensorGetPreviousValue(pErCfg->MonMinSensorID);

  pErData->MonMaxValue = validMax ? pErData->MonMaxValue :
                                SensorGetPreviousValue(pErCfg->MonMaxSensorID);

  // Update the starting duration.
  if (bUpdateDuration)
  {
    pErData->StartingDuration_ms = CM_GetTickCount() - pErData->startingTime;
  }
 

}
/*************************************************************************
 *  MODIFICATIONS
 *    $History: EngineRun.c $
 * 
 * *****************  Version 18  *****************
 * User: Jeff Vahue   Date: 8/29/12    Time: 12:28p
 * Updated in $/software/control processor/code/application
 * Code Review Tool Findings
 * 
 * *****************  Version 17  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 2:36p
 * Updated in $/software/control processor/code/application
 * SCR# 1142 - more format corrections
 * 
 * *****************  Version 16  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 12:43p
 * Updated in $/software/control processor/code/application
 * SCR# 1142
 * 
 * *****************  Version 15  *****************
 * User: John Omalley Date: 12-08-28   Time: 8:32a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Reset and Initialization Logic Cleanup
 * 
 * *****************  Version 14  *****************
 * User: Contractor V&v Date: 8/15/12    Time: 7:17p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 Refactored test functions for efficiency
 * 
 * *****************  Version 13  *****************
 * User: Contractor V&v Date: 8/08/12    Time: 5:04p
 * Updated in $/software/control processor/code/application
 * FAST 2 create transition from START and RUN to STOP when either is no
 * longer active.
 * 
 * *****************  Version 12  *****************
 * User: Contractor V&v Date: 8/08/12    Time: 3:33p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 Fixed bug in persist count writing Refactor temp and
 * voltage to max and min
 * 
 * *****************  Version 11  *****************
 * User: Contractor V&v Date: 7/18/12    Time: 6:24p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 Add shutdown handling
 * 
 * *****************  Version 10  *****************
 * User: Contractor V&v Date: 7/11/12    Time: 4:29p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2  Cycle impl and persistence
 * 
 * *****************  Version 9  *****************
 * User: Contractor V&v Date: 6/25/12    Time: 2:28p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 Use LogWriteETM, separate log structures
 * 
 * *****************  Version 8  *****************
 * User: Contractor V&v Date: 6/18/12    Time: 4:26p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 reorder RPN operands
 * 
 * *****************  Version 7  *****************
 * User: Contractor V&v Date: 6/07/12    Time: 7:14p
 * Updated in $/software/control processor/code/application
 * Disable cycle until ready
 * 
 * *****************  Version 6  *****************
 * User: Contractor V&v Date: 6/05/12    Time: 6:41p
 * Updated in $/software/control processor/code/application
 * Engine ID change
 * 
 * *****************  Version 5  *****************
 * User: Contractor V&v Date: 5/24/12    Time: 3:04p
 * Updated in $/software/control processor/code/application
 * FAST2 Refactoring
 * 
 * *****************  Version 4  *****************
 * User: John Omalley Date: 12-05-22   Time: 2:15p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Check In for Dave
 * 
 * *****************  Version 3  *****************
 * User: Contractor V&v Date: 5/10/12    Time: 6:41p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2  Engine Run
 ***************************************************************************/
