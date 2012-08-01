#define ENGINERUN_BODY
/******************************************************************************
              Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
                 All Rights Reserved. Proprietary and Confidential.


    File:      EngineRun.c  

    Description:
   VERSION
      $Revision: 11 $  $Date: 7/18/12 6:24p $
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
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void EngRunEnableTask ( BOOLEAN bEnable );
static void EngRunForceEnd   ( void );
static void EngRunResetAll(void);
static void EngRunReset   (ENGRUN_CFG* pErCfg, ENGRUN_DATA* pErData, UINT16 erIdx);

static void EngRunUpdateAll(void);

static void EngRunUpdate (ENGRUN_CFG* pErCfg, ENGRUN_DATA* pErData);

static void EngRunStartLog      ( ENGRUN_CFG* pErCfg, ENGRUN_DATA* pErData );
static void EngRunUpdateStartData( ENGRUN_CFG* pErCfg, ENGRUN_DATA* pErData );
static void EngRunWriteStartLog ( ER_REASON reason, ENGRUN_CFG* pErCfg, ENGRUN_DATA* pErData );

static void EngRunUpdateRunData     ( ENGRUN_CFG* pErCfg, ENGRUN_DATA* pErData );
static void EngRunWriteRunLog   ( ER_REASON reason, ENGRUN_CFG* pErCfg, ENGRUN_DATA* pErData );

static BOOLEAN EngRunIsError     (ENGRUN_CFG* pErCfg);
static BOOLEAN EngRunIsRunning   (ENGRUN_CFG* pErCfg);
static BOOLEAN EngRunIsStarting  (ENGRUN_CFG* pErCfg);
static BOOLEAN EngRunIsStopped   (ENGRUN_CFG* pErCfg);


/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static ENGRUN_CFG    EngineRunCfg[MAX_ENGINES];
static ENGRUN_DATA   EngineRunData[MAX_ENGINES];

static ENGRUN_STARTLOG EngineStartLog[MAX_ENGINES];
static ENGRUN_RUNLOG   EngineRunLog[MAX_ENGINES];


// Include cmd tables and functions here after local dependencies are declared.
#include "EngineRunUserTables.c"

/*********************************************************************************************/
/* Public Functions                                                                          */
/*********************************************************************************************/

/******************************************************************************
 * Function:     EngRunGetState
 *
 * Description:  Returns the current state of the EngineRun 'idx' or an UINT8 bit array 
 *               of the engineruns which are in the RUNNING state.
 *
 * Parameters:   [in] idx - ENGRUN_ID of the EngineRun being requested or
 *                          ENGRUN_ID_ANY to return the bit array of all enginerun
 *                          in the RUNNING state.
 *               [out]      Pointer to the EngRunFlags bit array of engines in RUNNING state.
 *                          This field is not updated if idx is not set to ENGRUN_ID_ANY.
 *
 * Returns:      ER_STATE   if idx  is not ENGRUN_ID_ANY, state of the requested engine run is 
 *                          returned. Otherwise returns ER_STATE_RUNNING if at least one
 *                          enginerun is in the RUNNING state.
 *
 * Notes:        None
 *
 *****************************************************************************/
ER_STATE EngRunGetState(ENGRUN_INDEX idx, UINT8* EngRunFlags)
{
  INT16 i;
  ER_STATE state = ER_STATE_STOPPED;

  // If the user wants a list of ALL running engine-runs, return
  // it in the provided location. 
  if ( ENGRUN_ID_ANY == idx )
  {
    // Check that an return address is provided
    if(NULL != EngRunFlags)
    {
      *EngRunFlags = 0;
      for (i = 0; i < MAX_ENGINES; ++i)
      {
        if (ER_STATE_RUNNING == EngineRunData[i].State)
        {
          // Set the bit in the flag
          *EngRunFlags |= 1 << i;
        }
      }
      // Set the return value to running if any bits were set on, otherwise assume STOPPED
      state = (*EngRunFlags != 0)  ? ER_STATE_RUNNING : ER_STATE_STOPPED;
    }
    else
    {
      GSE_DebugStr(NORMAL, TRUE, "Request for EngRun list needs return address");
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

  // Add user commands for EngineRun to the user command tables.
  User_AddRootCmd(&RootEngRunMsg);

  // Clear and Load the current cfg info.
  memset(&EngineRunCfg, 0, sizeof(EngineRunCfg));

  memcpy(EngineRunCfg,
         &(CfgMgr_RuntimeConfigPtr()->EngineRunConfigs),
         sizeof(EngineRunCfg));

  // Initialize Engine Runs storage objects.
  EngRunResetAll();
 
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
 * Function:     EngRunGetDataCollectPtr
 *
 * Description:  Returns pointer to the data collection area for this engine-run .
 *               This function is called to retrieve cycle counts owned by the 
 *               indicated engine run object.
 *
 * Parameters:   
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
 * Function:     EngRunGetStartingTime
 *
 * Description:  Returns value of the starting time 
 *               tick of the START for this engine-run .
 *
 * Parameters:   
 *
 * Returns:      Pointer to the 
 *
 * Notes:        None
 *
 *****************************************************************************/
EXPORT UINT32 EngRunGetStartingTime(ENGRUN_INDEX engId)
{
  ASSERT(engId < MAX_ENGINES);
  return EngineRunData[engId].startingTime;
}


/******************************************************************************
 * Function:     EngRunResetAll
 *
 * Description:  Initialize the log processing of all engine runs
 *
 * Parameters:   None
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
void EngRunResetAll(void)
{
  UINT16 i;
  for ( i = 0; i < MAX_ENGINES; i++)
  {
    EngRunReset( &EngineRunCfg[i], &EngineRunData[i], i );
  }
}


/*********************************************************************************************/
/* Local Functions                                                                           */
/*********************************************************************************************/

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
 * Parameters:   [in] erIdx - index of the engine run object to init.
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static void EngRunReset(ENGRUN_CFG* pErCfg, ENGRUN_DATA* pErData, UINT16 erIdx )
{
  INT32 i;  

  // Cfg must specify valid sensor for start, run, stop, temp and battery
  if ( TriggerIsConfigured(pErCfg->StartTrigID)   &&
       TriggerIsConfigured(pErCfg->StopTrigID )   &&
       TriggerIsConfigured(pErCfg->RunTrigID  )   &&
       SensorIsUsed       (pErCfg->BattSensorID ) &&
       SensorIsUsed       (pErCfg->TempSensorID )
     )
  {
    // Initialize the common fields in the EngineRun data structure
    //
    pErData->ErIndex = (ENGRUN_INDEX)erIdx;
    pErData->State   = ER_STATE_STOPPED;

    memset(&pErData->StartTime, 0, sizeof(TIMESTAMP));
//    memset(&pErData->EndTime, 0, sizeof(TIMESTAMP));
    
    pErData->startingTime        = 0;
    pErData->StartingDuration_ms = 0;
    pErData->Duration_ms         = 0;
    pErData->MinBattery          = FLT_MAX;
    pErData->MaxStartTemp        = 0.f;   
    pErData->nSampleCount        = 0;
    pErData->nRateCounts         = (UINT16)(MIFs_PER_SECOND / pErCfg->Rate);
    pErData->nRateCountdown      = (UINT16)((pErCfg->nOffset_ms / MIF_PERIOD_mS) + 1);

    /* SNSR_SUMMARY field setup */

    pErData->nTotalSensors = SensorSetupSummaryArray(pErData->SnsrSummary,
                                                     MAX_ENGRUN_SENSORS,
                                                     pErCfg->SensorMap,
                                                     sizeof(pErCfg->SensorMap) );
    // CYCLES setup
    
    for(i = 0; i < MAX_CYCLES; ++i)
    {
      pErData->CycleCounts[i] = 0;
    }    
  }
  else // Invalid config... 
  {
    // Set ER state to hold in STOP state if
    // sensor Cfg is invalid
    pErData->State   = ER_STATE_STOPPED;
    pErData->ErIndex = ENGRUN_UNUSED;
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
 * Parameters:   [in] erIndex - the index for the specified engine run.
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static void EngRunUpdate( ENGRUN_CFG* pErCfg, ENGRUN_DATA* pErData)
{
  FLOAT32 fValue;
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

        // update minimum battery voltage      
        if ( SensorIsValid(pErCfg->BattSensorID))
        {
          fValue = SensorGetValue(pErCfg->BattSensorID);
          if ( pErData->MinBattery > fValue)
          {
            pErData->MinBattery = fValue;
          }
        }

        // Check that all transition-trigger are valid,
        // then see if engine start/running is occurring
        // if the engine is running ( ex: system restart in-air),
        // we still go thru START to set up initialization and logs
        if ( !EngRunIsError(pErCfg) )
        {          
          if ( EngRunIsStarting(pErCfg) ||
               EngRunIsRunning(pErCfg) )
          {            
            // init the start-log entry
            // and clear the cycle counts for this engrun
            EngRunStartLog(pErCfg, pErData);
            CycleResetEngineRun(pErData->ErIndex);

            pErData->State = ER_STATE_STARTING;            
          }
        }        
        break;

    case ER_STATE_STARTING:      
      // Always update the ER Starter parameters.
      EngRunUpdateStartData(pErCfg, pErData);

      
      // Error Detected transition STARTING -> STOP
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
      if ( EngRunIsRunning( pErCfg ) )
      {
        // Set the max ER starting Temperature Sensor to the current value todo DaveB- confirm this
        pErData->MaxStartTemp = SensorGetValue((SENSOR_INDEX)pErCfg->TempSensorID);
        
        // Write ETM START log
        EngRunWriteStartLog( ER_LOG_RUNNING, pErCfg, pErData);
        pErData->State = ER_STATE_RUNNING;
        break;
      }

      // -> STOP
      if ( EngRunIsStopped(pErCfg))
      {      
        // Finish the engine run log
        EngRunWriteRunLog(ER_LOG_STOPPED, pErCfg, pErData);
        pErData->State = ER_STATE_STOPPED;
        break;
      }
      break;

    case ER_STATE_RUNNING:
      // If we have a problem determining the EngineRun state for this engine run,
      // end the engine run log, and transition to error mode

      // ERROR! -> STOP
      if ( EngRunIsError(pErCfg))
      {
        // Add additional 1 second to close of ER due to ERROR.  Normally
        // UpdateEngineRunLog() updates ->Duration, which is only called on !EngineRunIsError()
        pErData->Duration_ms += pErCfg->Rate;

        // Finish the engine run log
        EngRunWriteRunLog(ER_LOG_ERROR, pErCfg, pErData);
        pErData->State = ER_STATE_STOPPED;
        break;
      }

      // Update the EngineRun log
      // Allow additional update so that is ER is closed, this last second
      // is counted.
      EngRunUpdateRunData( pErCfg, pErData);

      // If the engine run has stopped, finish the engine run and change states
      // -> STOP
      if ( EngRunIsStopped(pErCfg))
      {
        // Finish the engine run log
        EngRunWriteRunLog(ER_LOG_STOPPED, pErCfg, pErData);        
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
* Parameters:   [in] erIndex - the index for the specified engine run.
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
  pErData->MaxStartTemp = SensorGetValue(pErCfg->TempSensorID);
  pErData->MaxStartTemp = pErData->MaxStartTemp;
  pErData->MinBattery   = pErData->MinBattery;

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
 * Parameters:   None
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
 
  pLog->MaxStartSensorId    = pErCfg->TempSensorID;
  pLog->MaxStartTemp        = pErData->MaxStartTemp;
  pLog->MinStartSensorId    = pErCfg->BattSensorID;
  pLog->MinBattery          = pErData->MinBattery;
  
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
 * Parameters:   None
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
      // otherwise use the average calculated at the point it when invalid.
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
 * Parameters:   [in] engine run Index.
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
  pErData->Duration_ms += CM_GetTickCount() - pErData->startingTime;
  
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
 * Parameters:   [in] erIndex - the index of the engine run
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
 * Function:     EngRunIsRunning
 *
 * Description:  
 *               
 *
 * Parameters:   None
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static BOOLEAN EngRunIsRunning( ENGRUN_CFG* pErCfg)
{  
  return TriggerGetState( pErCfg->RunTrigID);
}

/******************************************************************************
 * Function:     EngRunIsStarting
 *
 * Description:  
 *               
 *
 * Parameters:   None
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static BOOLEAN EngRunIsStarting( ENGRUN_CFG* pErCfg )
{
   return TriggerGetState( pErCfg->StartTrigID);
}


/******************************************************************************
 * Function:     EngRunIsStopped
 *
 * Description:  
 *               
 *
 * Parameters:   None
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static BOOLEAN EngRunIsStopped(ENGRUN_CFG* pErCfg)
{  
  return TriggerGetState( pErCfg->StopTrigID);
}

/******************************************************************************
 * Function:     EngRunUpdateStartData
 *
 * Description:  Update the EngineRun start data.
 *               The enginerun is in ER_STATE_STARTING.
 *
 * Parameters:   None
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static void EngRunUpdateStartData(ENGRUN_CFG* pErCfg,ENGRUN_DATA* pErData)
{
  FLOAT32 batteryVoltage;
  FLOAT32 temperature;
  BOOLEAN validBatt;
  BOOLEAN validTemp;
  
  // Update the min battery
  if ( SensorIsValid(pErCfg->BattSensorID) )
  {
   
    batteryVoltage = SensorGetValue(pErCfg->BattSensorID);
    validBatt = TRUE;

    if ( pErData->MinBattery > batteryVoltage )
    {
      pErData->MinBattery = batteryVoltage;        
    }
  }
  else
  {
    validBatt = FALSE;
  }

  // update max temperature
  if ( SensorIsValid(pErCfg->TempSensorID) )
  {
    temperature = SensorGetValue(pErCfg->TempSensorID);
    validTemp = TRUE;
    if ( pErData->MaxStartTemp < temperature)
    {
      pErData->MaxStartTemp   = temperature;        
    }
  }
  else
  {
    validTemp = FALSE;
  }

  // todo DaveB - verify requirements on how to log start values when
  // invalid.   
  pErData->MinBattery = validBatt ? pErData->MinBattery :
                                SensorGetPreviousValue(pErCfg->BattSensorID);

  pErData->MaxStartTemp = validTemp ? pErData->MaxStartTemp :
                                SensorGetPreviousValue(pErCfg->TempSensorID);

  // Update the starting duration.
  pErData->StartingDuration_ms += CM_GetTickCount() - pErData->startingTime;

}
/*************************************************************************
 *  MODIFICATIONS
 *    $History: EngineRun.c $
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
 ************************************************************************/
