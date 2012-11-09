#define ENGINERUN_BODY
/******************************************************************************
              Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
                 All Rights Reserved. Proprietary and Confidential.


    File:      EngineRun.c

    Description:

   VERSION
      $Revision: 31 $  $Date: 11/08/12 4:26p $
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

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static ENGRUN_CFG    m_engineRunCfg[MAX_ENGINES];
static ENGRUN_DATA   m_engineRunData[MAX_ENGINES];

static ENGRUN_STARTLOG m_engineStartLog[MAX_ENGINES];
static ENGRUN_RUNLOG   m_engineRunLog[MAX_ENGINES];

static ENGINE_FILE_HDR m_EngineInfo;



// Include cmd tables and functions here after local dependencies are declared.
#include "EngineRunUserTables.c"


/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void    EngRunForceEnd   ( void );
static void    EngRunReset      ( ENGRUN_CFG* pErCfg, ENGRUN_DATA* pErData);
static BOOLEAN EngRunIsError    ( const ENGRUN_CFG* pErCfg);

static void EngRunUpdate   (ENGRUN_CFG* pErCfg, ENGRUN_DATA* pErData);

static void EngRunStartLog       ( const ENGRUN_CFG* pErCfg, ENGRUN_DATA* pErData );
static void EngRunUpdateStartData( const ENGRUN_CFG* pErCfg, ENGRUN_DATA* pErData, BOOLEAN bUpdateDuration);

static void EngRunWriteStartLog ( ER_REASON reason, const ENGRUN_CFG* pErCfg, const ENGRUN_DATA* pErData );

static void EngRunUpdateRunData ( ENGRUN_DATA* pErData );
static void EngRunWriteRunLog   ( ER_REASON reason, ENGRUN_DATA* pErData );


/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:     EngRunGetState
 *
 * Description:  Returns the current state of the EngineRun 'idx' or an UINT8
 *               bit array of the engine runs which are in the RUNNING state.
 *
 * Parameters:   [in] idx - ENGRUN_ID of the EngineRun being requested or
 *                          ENGRUN_ID_ANY to return the bit array of all engine
 *                          run in the STARTING or RUNNING state.
 *               [out] engRunFlags - Pointer to the engRunFlags bit array of engines in
 *                          RUNNING state. This field is not updated if idx is
 *                          not set to ENGRUN_ID_ANY.
 *
 * Returns:      ER_STATE   if idx  is not ENGRUN_ID_ANY, state of the requested
 *                          engine run is returned. Otherwise returns one of the
 *                          following based on a  prioritized check: RUNNING,
 *                          STARTING, STOPPED
 *
 * Notes:        None
 *
 *****************************************************************************/
ER_STATE EngRunGetState(ENGRUN_INDEX idx, UINT8* engRunFlags)
{
  UINT16 i;
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
      if (ER_STATE_RUNNING == m_engineRunData[i].erState)
      {
        // Set the bit in the flag
        runMask |= 1 << i;
        bRunning = TRUE;
      }
      else if (ER_STATE_STARTING == m_engineRunData[i].erState)
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
    if(NULL != engRunFlags)
    {
      *engRunFlags = runMask;
    }
  }
  else // Get the current state of the requested engine run.
  {
    ASSERT ( idx >= ENGRUN_ID_0 && idx < MAX_ENGINES);
    state = m_engineRunData[idx].erState;
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
  RESULT result;

  // Add user commands for EngineRun to the user command tables.

  User_AddRootCmd(&rootEngRunMsg);

  // Clear and Load the current cfg info.
  memset(m_engineRunCfg, 0, sizeof(m_engineRunCfg));

  memcpy( m_engineRunCfg,
          CfgMgr_RuntimeConfigPtr()->EngineRunConfigs,
          sizeof(m_engineRunCfg) );

  // Open Engine Identification File
  result =  NV_Open(NV_ENGINE_ID);
  if(SYS_OK == result)
  {
     NV_Read(NV_ENGINE_ID,0,&m_EngineInfo,sizeof(m_EngineInfo));
  }
  else
  {

     if (SYS_NV_FILE_OPEN_ERR == result)
     {
        Flt_SetStatus(STA_NORMAL, APP_ID_ENGINE_INFO_CRC_FAIL, NULL, 0);
     }

     GSE_DebugStr(NORMAL,TRUE,"Engine: Failed to open Engine ID file, restoring defaults");
     //Re-init file
     EngReInitFile();
  }

  // Initialize Engine Runs storage objects.
  for ( i = 0; i < MAX_ENGINES; i++)
  {
    pErCfg  = &m_engineRunCfg[i];
    pErData = &m_engineRunData[i];
    // Cfg must specify valid sensor for start, run, stop, temp and battery
    if ( TriggerIsConfigured(pErCfg->startTrigID)     &&
         TriggerIsConfigured(pErCfg->stopTrigID )     &&
         TriggerIsConfigured(pErCfg->runTrigID  )     &&
         SensorIsUsed       (pErCfg->monMinSensorID ) &&
         SensorIsUsed       (pErCfg->monMaxSensorID ) )
    {
      // Initialize the common fields in the EngineRun data structure
      pErData->erIndex = (ENGRUN_INDEX)i;
      EngRunReset(pErCfg,pErData);

      // Init Task scheduling data
      pErData->nRateCounts    = (UINT16)(MIFs_PER_SECOND / pErCfg->erRate);
      pErData->nRateCountdown = (UINT16)((pErCfg->nOffset_ms / MIF_PERIOD_mS) + 1);

    }
    else // Invalid config...
    {
      // Set ER state to hold in STOP state if
      // sensor Cfg is invalid
      pErData->erState   = ER_STATE_STOPPED;
      pErData->erIndex = ENGRUN_UNUSED;
    }
  }

  // Create EngineRun Task - DT
  memset(&TaskInfo, 0, sizeof(TaskInfo));
  strncpy_safe(TaskInfo.Name, sizeof(TaskInfo.Name),"EngineRun Task",_TRUNCATE);
  TaskInfo.TaskID           = (TASK_INDEX) EngRun_Task;
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
 * Function:    EngReInitFile
 *
 * Description: Set the NV data (Engine Serial Numbers) to
 *              the default values
 *
 * Parameters:  void
 *
 * Returns:     void
 *
 * Notes:
 *
 *****************************************************************************/
void EngReInitFile(void)
{
  CHAR resultStr[RESULTCODES_MAX_STR_LEN];
  INT16 i;

  memset(&m_EngineInfo,0,sizeof(m_EngineInfo));

  strncpy_safe(m_EngineInfo.servicePlan,
               sizeof(m_EngineInfo.servicePlan),
               ENGINE_DEFAULT_SERVICE_PLAN,_TRUNCATE);

  for ( i = 0; i < MAX_ENGINES; i++)
  {
     strncpy_safe(m_EngineInfo.engine[i].serialNumber,
                  sizeof(m_EngineInfo.engine[i].serialNumber),
                  ENGINE_DEFAULT_SERIAL_NUMBER, _TRUNCATE);
     strncpy_safe(m_EngineInfo.engine[i].modelNumber,
                  sizeof(m_EngineInfo.engine[i].modelNumber),
                  ENGINE_DEFAULT_MODEL_NUMBER, _TRUNCATE);
  }

  NV_Write( NV_ENGINE_ID, 0, &m_EngineInfo, sizeof(m_EngineInfo));
  GSE_StatusStr( NORMAL, RcGetResultCodeString(SYS_OK, resultStr));
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
  UINT16 i;

  if (Tm.systemMode == SYS_SHUTDOWN_ID)
  {
    EngRunForceEnd();
    TmTaskEnable (EngRun_Task, FALSE);
  }
  else
  {
    // EngRunUpdateAll - Normal execution
    for ( i = 0; i < MAX_ENGINES; i++)
    {
      if (m_engineRunData[i].erIndex != ENGRUN_UNUSED)
      {
        EngRunUpdate(&m_engineRunCfg[i],&m_engineRunData[i]);
      }
    }
  }
}

/******************************************************************************
 * Function:     EngRunGetBinaryHeader
 *
 * Description:  Retrieves the binary header for the engine run
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
UINT16 EngRunGetBinaryHeader ( void *pDest, UINT16 nMaxByteSize )
{
   // Local Data
   ENGRUN_HDR engineHdr[MAX_ENGINES];
   UINT16     engineIndex;
   INT8       *pBuffer;
   UINT16     nRemaining;
   UINT16     nTotal;

   // Initialize Local Data
   pBuffer    = (INT8 *)pDest;
   nRemaining = nMaxByteSize;
   nTotal     = 0;
   memset ( engineHdr, 0, sizeof(engineHdr) );

   // Loop through all the triggers
   for ( engineIndex = 0;
         ((engineIndex < MAX_ENGINES) && (nRemaining > sizeof (engineHdr[engineIndex])));
         engineIndex++ )
   {
      // Copy the Engine Run names
      strncpy_safe( engineHdr[engineIndex].engineName,
                    sizeof(engineHdr[engineIndex].engineName),
                    m_engineRunCfg[engineIndex].engineName,
                    _TRUNCATE);

      engineHdr[engineIndex].startTrigID = m_engineRunCfg[engineIndex].startTrigID;
      engineHdr[engineIndex].runTrigID   = m_engineRunCfg[engineIndex].runTrigID;
      engineHdr[engineIndex].stopTrigID  = m_engineRunCfg[engineIndex].stopTrigID;

      // Increment the total number of bytes and decrement the remaining
      nTotal     += sizeof (engineHdr[engineIndex]);
      nRemaining -= sizeof (engineHdr[engineIndex]);
   }
   // Copy the Trigger header to the buffer
   memcpy ( pBuffer, engineHdr, nTotal );
   // Return the total number of bytes written
   return ( nTotal );
}

/******************************************************************************
 * Function:     EngRunGetFileHeader
 *
 * Description:  Retrieves the file header for the engine requested.
 *
 * Parameters:   ENGRUN_INDEX engId
 *
 * Returns:      ENGINE_FILE_HDR
 *
 * Notes:        Always returns the servicePlan with the Engine info.
 *
 *****************************************************************************/
ENGINE_FILE_HDR* EngRunGetFileHeader ( void )
{
   return &m_EngineInfo;
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
  INT32 i;
  ENGRUN_CFG*  pErCfg;
  ENGRUN_DATA* pErData;

  // Close out any active cycle
  // and flag the task to stop.
  // Eng
  for (i = 0; i < MAX_ENGINES; ++i)
  {
    pErCfg  = &m_engineRunCfg[i];
    pErData = &m_engineRunData[i];

    // Only process this engine if it's USED
    if (pErData->erIndex != ENGRUN_UNUSED)
    {
      switch (pErData->erState)
      {
        case ER_STATE_STOPPED:
          break;

        case ER_STATE_STARTING:
          EngRunWriteStartLog( ER_LOG_SHUTDOWN, pErCfg, pErData);
          EngRunReset( pErCfg, pErData);
          break;

        case ER_STATE_RUNNING:
          // Update persist, and create log
          EngRunWriteRunLog(ER_LOG_SHUTDOWN, pErData);
          EngRunReset( pErCfg, pErData);
          break;

        default:
          FATAL("Unrecognized engine run state %d", pErData->erState );
          break;
      }
    }
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
  {
    pErData->erState             = ER_STATE_STOPPED;
    memset(&pErData->startTime, 0, sizeof(TIMESTAMP));
    pErData->startingTime_ms     = 0;
    pErData->startingDuration_ms = 0;
    pErData->erDuration_ms       = 0;
    pErData->minValueValid       = TRUE;
    pErData->monMinValue         = FLT_MAX;
    pErData->maxValueValid       = TRUE;
    pErData->monMaxValue         = -FLT_MAX;
    pErData->nSampleCount        = 0;

    /* SNSR_SUMMARY field setup */

    pErData->nTotalSensors = SensorSetupSummaryArray(pErData->snsrSummary,
                                                     MAX_ENGRUN_SENSORS,
                                                     pErCfg->sensorMap,
                                                     sizeof(pErCfg->sensorMap) );
    // Reset cycles & counts for this enginerun
    CycleResetEngineRun(pErData->erIndex);
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
  ER_STATE curState;

  // Is it time for this EngineRun to run?
  if (--pErData->nRateCountdown == 0)
  {
    // Reset the countdown counter for the next engine run.
    pErData->nRateCountdown = pErData->nRateCounts;

    // Process the current engine run state
    curState = pErData->erState;

    switch ( pErData->erState)
    {
      case ER_STATE_STOPPED:

        // If the the engine-run is not-configured or unused,
        // hold in the STOP state.
        if (ENGRUN_UNUSED != pErData->erIndex)
        {
          // Monitor configured min/max sensors, set flag
          // FALSE to prevent update start duration.
          EngRunUpdateStartData(pErCfg, pErData, FALSE);

          // Check that all transition-trigger are valid,
          // then see if engine start/running is occurring
          // if the engine is running ( ex: system restart in-air),
          // we still go thru START to set up initialization and logs
          if ( !EngRunIsError(pErCfg) )
          {
            if ( TriggerGetState( pErCfg->startTrigID) ||
                 TriggerGetState( pErCfg->runTrigID  ) )
            {
              // init the start-log entry and set start-time
              // clear the cycle counts for this engine run
              EngRunStartLog(pErCfg, pErData);
              CycleResetEngineRun(pErData->erIndex);
              pErData->erState = ER_STATE_STARTING;
            }
          }
          else
          {
            // Ensure we don't collected any false start data while invalid
            // reset this enginerun and its cycles.
            EngRunReset(pErCfg, pErData);
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
        pErData->erDuration_ms += (UINT32)pErCfg->erRate;

        // Finish the engine run log
        EngRunWriteStartLog( ER_LOG_ERROR, pErCfg, pErData);

        // Reset the enginerun and it's cycles.
        EngRunReset(pErCfg, pErData);

        pErData->erState = ER_STATE_STOPPED;
      }
      else
      {
        // Normal processing while in START state.
        // Update the Engine Run-log data
        EngRunUpdateRunData(pErData);

        // STARTING -> RUNNING
        if ( TriggerGetState( pErCfg->runTrigID) )
        {
          // Write ETM START log
          EngRunWriteStartLog( ER_LOG_RUNNING, pErCfg, pErData);
          pErData->erState = ER_STATE_RUNNING;
        }
        else
        {
          // STARTING -> STOP
          // STARTING -> (Aborted) -> STOP
          if ( TriggerGetState( pErCfg->stopTrigID) ||
              !TriggerGetState( pErCfg->startTrigID) )
          {
            // Finish the engine run log and reset my cycles.
            EngRunWriteStartLog(ER_LOG_STOPPED, pErCfg, pErData);
            EngRunReset(pErCfg, pErData);

            pErData->erState = ER_STATE_STOPPED;
          }
        }
      }
      break;

    case ER_STATE_RUNNING:
      // If we have a problem determining the EngineRun state for this engine run,
      // end the engine run log, and transition to error mode

      // RUNNING -> (error) -> STOP
      if ( EngRunIsError(pErCfg))
      {
        // Add additional 1 second to close of ER due to ERROR.  Normally
        // UpdateEngineRunLog() updates ->Duration, which is only called
		// on !EngineRunIsError()
        pErData->erDuration_ms += (UINT32)pErCfg->erRate;

        // Finish the engine run log
        EngRunWriteRunLog(ER_LOG_ERROR, pErData);
        EngRunReset(pErCfg, pErData);
        pErData->erState = ER_STATE_STOPPED;
      }
      else
      {
        // Update the EngineRun log data
        // Allow additional update so that is ER is closed, this last second
        // is counted.
        EngRunUpdateRunData( pErData);

        // If the engine run has stopped, finish the engine run and change states
        // RUNNING -> STOP
        if ( TriggerGetState( pErCfg->stopTrigID) ||
            !TriggerGetState( pErCfg->runTrigID) )
        {
          // Finish the engine run log
          EngRunWriteRunLog(ER_LOG_STOPPED, pErData);
          EngRunReset(pErCfg, pErData);
          pErData->erState = ER_STATE_STOPPED;
        }
      }
      break;

    default:
      FATAL("Unrecognized engine run state %d", pErData->erState );
      break;
    }

    // Update the cycles for this engine run if starting or running.
    if ( pErData->erState == ER_STATE_STARTING ||
         pErData->erState == ER_STATE_RUNNING )
    {
      CycleUpdateAll(pErData->erIndex);
    }

    // Display msg when state changes.
    if ( pErData->erState !=  curState )
    {
      GSE_DebugStr ( NORMAL, TRUE, "EngineRun(%d) %s -> %s ",
                     pErData->erIndex,
                     EngineRunStateEnum[curState].Str,
                     EngineRunStateEnum[pErData->erState].Str);
    }
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
static void EngRunStartLog( const ENGRUN_CFG* pErCfg, ENGRUN_DATA* pErData )
{
  UINT8 i;
  SNSR_SUMMARY*   pSnsr;

  // Set the start timestamp, init the end timestamp
  CM_GetTimeAsTimestamp(&pErData->startTime);

  // Set the 'tick count' of the start time( used to calc duration)
  pErData->startingTime_ms = CM_GetTickCount();
  pErData->nSampleCount    = 0;
  pErData->erDuration_ms   = 0;

  // Init the current values of the starting sensors.
  // Note: Validity was already checked as a pre-condition of calling this function.)
  pErData->monMaxValue = SensorGetValue(pErCfg->monMaxSensorID);

  pErData->monMinValue = pErData->monMinValue;

  // Initialize the summary values for monitored sensors

  for (i = 0; i < pErData->nTotalSensors; ++i)
  {
    pSnsr = &(pErData->snsrSummary[i]);

    pSnsr->bValid = SensorIsValid(pSnsr->SensorIndex);
    // todo DaveB is this really necessary ? it will be
	// updated anyway during EngRunUpdateLog
    if(pSnsr->bValid)
    {
      pSnsr->fMaxValue = SensorGetValue(pSnsr->SensorIndex);
    }
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
static void EngRunWriteStartLog( ER_REASON reason, const ENGRUN_CFG* pErCfg,
								                   const ENGRUN_DATA* pErData )
{
  ENGRUN_STARTLOG* pLog;

  // Tell cycles to finish up persist any info as required.
  CycleFinishEngineRun(pErData->erIndex);


  pLog = &m_engineStartLog[pErData->erIndex];

  pLog->erIndex             = pErData->erIndex;
  pLog->reason              = reason;
  pLog->startTime           = pErData->startTime;
  CM_GetTimeAsTimestamp(&pLog->endTime);
  pLog->startingDuration_ms = pErData->startingDuration_ms;

  pLog->maxStartSensorId    = pErCfg->monMaxSensorID;
  pLog->maxValueValid       = pErData->maxValueValid;
  pLog->monMaxValue         = pErData->monMaxValue;

  pLog->minStartSensorId    = pErCfg->monMinSensorID;
  pLog->minValueValid       = pErData->minValueValid;
  pLog->monMinValue         = pErData->monMinValue;

  LogWriteETM( APP_ID_ENGINERUN_STARTED,
               LOG_PRIORITY_3,
               pLog,
               sizeof(ENGRUN_STARTLOG),
               NULL);

  // TODO: determine if we should check here if the reason would require a Cycle reset
  // and perform the cycle reset - this would ensure it is not forgotten
  // - or make cycles a task
}

/******************************************************************************
 * Function:     EngRunWriteRunLog
 *
 * Description:
 *
 *
 * Parameters:   [in] reason for logging
 *               [in] pErData
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static void EngRunWriteRunLog( ER_REASON reason, ENGRUN_DATA* pErData )
{
  INT32   i;
  FLOAT32 oneOverN;
  UINT32  cycleCounts[MAX_CYCLES];
  ENGRUN_RUNLOG* pLog;

  SNSR_SUMMARY*  pErSummary;
  SNSR_SUMMARY*  pLogSummary;

  memset(cycleCounts, 0, sizeof(cycleCounts) );

  pLog = &m_engineRunLog[pErData->erIndex];

  oneOverN = (1.0f / (FLOAT32)pErData->nSampleCount);
  GSE_DebugStr(NORMAL,TRUE,"Frames: %d", pErData->nSampleCount);

  // Tell Cycles to finish up for this engine run.
  // This will ensure Cycles has brought its count structure up-to-date.
  CycleFinishEngineRun( pErData->erIndex);

  // Fetch the cycle counts for 'this' enginerun
  CycleCollectCounts(cycleCounts, pErData->erIndex );

  // Make the log entry

  pLog->erIndex             = pErData->erIndex;
  pLog->reason              = reason;
  pLog->startTime           = pErData->startTime;
  CM_GetTimeAsTimestamp(&pLog->endTime);

  pLog->startingDuration_ms = pErData->startingDuration_ms;
  pLog->erDuration_ms       = pErData->erDuration_ms;

#pragma ghs nowarning 1545 //Suppress packed structure alignment warning
  for (i = 0; i < MAX_ENGRUN_SENSORS; ++i)
  {
    pErSummary    = &(pErData->snsrSummary[i]);
    pLogSummary   = &(pLog->snsrSummary[i]);

    if ( i < pErData->nTotalSensors)
    {
      pLogSummary->SensorIndex  = pErSummary->SensorIndex;
      pLogSummary->bValid       = pErSummary->bValid;
      pLogSummary->fTotal       = pErSummary->fTotal;
      pLogSummary->fMinValue    = pErSummary->fMinValue;
      pLogSummary->timeMinValue = pErSummary->timeMinValue;
      pLogSummary->fMaxValue    = pErSummary->fMaxValue;
      pLogSummary->timeMaxValue = pErSummary->timeMaxValue;

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
    pLog->cycleCounts[i] = cycleCounts[i];
  }

  LogWriteETM( APP_ID_ENGINERUN_ENDED,
               LOG_PRIORITY_3,
               pLog,
               sizeof(ENGRUN_RUNLOG),
               NULL);
}

/******************************************************************************
 * Function:     EngRunUpdateRunData
 *
 * Description:  Update the log with the sensor elements for this engine run
 *
 *
 * Parameters:   [in] pErData - engine run data
 *
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static void EngRunUpdateRunData( ENGRUN_DATA* pErData)
{
  UINT8    i;
  FLOAT32  oneOverN;
  SNSR_SUMMARY* pSummary;

  // Update the total run duration
  pErData->erDuration_ms = CM_GetTickCount() - pErData->startingTime_ms;

  // update the sample count so average can be calculated at end of engine run.
  pErData->nSampleCount++;

  // Loop thru all sensors handled by this this ER
  for ( i = 0; i < pErData->nTotalSensors; i++ )
  {
    pSummary = &pErData->snsrSummary[i];

    // If the sensor is known to be invalid but WAS VALID in
    // the past( initialized is TRUE ) then ignore processing for
    // the remainder of this engine run.
    if( !pSummary->bValid && pSummary->bInitialized )
    {
      continue;
    }

    if ( SensorIsValid(pSummary->SensorIndex) )
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
static BOOLEAN EngRunIsError( const ENGRUN_CFG* pErCfg)
{
  BOOLEAN allValid;

  // Make sure that each sensor in the engine run has not faulted
  allValid = (TriggerValidGetState(pErCfg->startTrigID) &&
              TriggerValidGetState(pErCfg->runTrigID)    &&
              TriggerValidGetState(pErCfg->stopTrigID) );

  return !allValid;
}

/******************************************************************************
 * Function:     EngRunUpdateStartData
 *
 * Description:  Update the EngineRun start data.
 *               The engine run is in ER_STATE_STOPPED or ER_STATE_STARTING.
 *
 * Parameters:   [in] pErCfg - config
 *               [in] pErData - data
 *               [in] bUpdateDuration - start duration update flag.
 *
 * Returns:      None
 *
 * Notes:        bUpdateDuration is set to false when called during STOP state
 *               to maintain min/max values only. During START state, it is
 *               called with true to maintain the start duration elapsed time.
 *
 *****************************************************************************/
static void EngRunUpdateStartData( const ENGRUN_CFG* pErCfg,
                                         ENGRUN_DATA* pErData,
                                         BOOLEAN bUpdateDuration)
{
  FLOAT32 valueMin;
  FLOAT32 valueMax;

  // Update the min during STOP and START
  if ( SensorIsValid(pErCfg->monMinSensorID) )
  {
    valueMin  = SensorGetValue(pErCfg->monMinSensorID);

    if ( pErData->monMinValue > valueMin )
    {
      pErData->monMinValue = valueMin;
    }
  }
  else
  {
    pErData->minValueValid = FALSE;
  }

  // Update the starting duration and max value during START.
  if (bUpdateDuration)
  {
    // update max
    if ( SensorIsValid(pErCfg->monMaxSensorID) )
    {
      valueMax = SensorGetValue(pErCfg->monMaxSensorID);
      if ( pErData->monMaxValue < valueMax)
      {
        pErData->monMaxValue   = valueMax;
      }
    }
    else
    {
      pErData->maxValueValid = FALSE;
    }

    pErData->startingDuration_ms = CM_GetTickCount() - pErData->startingTime_ms;
  }
}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: EngineRun.c $
 *
 * *****************  Version 31  *****************
 * User: Contractor V&v Date: 11/08/12   Time: 4:26p
 * Updated in $/software/control processor/code/application
 * Code review
 *
 * *****************  Version 30  *****************
 * User: Melanie Jutras Date: 12-10-31   Time: 11:43a
 * Updated in $/software/control processor/code/application
 * SCR #1172 PCLint 545 Suspicious use of & error
 *
 * *****************  Version 29  *****************
 * User: John Omalley Date: 12-10-23   Time: 3:03p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Code Review Updates
 *
 * *****************  Version 28  *****************
 * User: John Omalley Date: 12-10-23   Time: 2:40p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Design and Code Review Updates
 *
 * *****************  Version 27  *****************
 * User: Melanie Jutras Date: 12-10-23   Time: 1:30p
 * Updated in $/software/control processor/code/application
 * SCR #1172 PCLint 545 Suspicious use of & Error
 *
 * *****************  Version 26  *****************
 * User: Contractor V&v Date: 10/11/12   Time: 6:55p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 Review Findings
 *
 * *****************  Version 24  *****************
 * User: Contractor V&v Date: 12-10-02   Time: 1:17p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 Coding standard compliance
 *
 * *****************  Version 23  *****************
 * User: John Omalley Date: 12-09-27   Time: 9:13a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Added Engine Identification Fields to software and file
 * header
 *
 * *****************  Version 22  *****************
 * User: Jeff Vahue   Date: 9/18/12    Time: 6:11p
 * Updated in $/software/control processor/code/application
 * SCR# 1107 - ER Offset scheduling BB Issue# 53
 *
 * *****************  Version 21  *****************
 * User: Contractor V&v Date: 9/14/12    Time: 4:02p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 fixes for sensor list
 *
 * *****************  Version 20  *****************
 * User: John Omalley Date: 12-09-11   Time: 1:56p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Added ETM Binary Header
 *
 * *****************  Version 19  *****************
 * User: Jeff Vahue   Date: 9/07/12    Time: 4:05p
 * Updated in $/software/control processor/code/application
 * SCR# 1107 - V&V fixes, code review updates
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
