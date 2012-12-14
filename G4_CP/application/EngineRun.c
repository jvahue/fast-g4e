#define ENGINERUN_BODY
/******************************************************************************
              Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
                 All Rights Reserved. Proprietary and Confidential.


    File:      EngineRun.c

    Description:

   VERSION
      $Revision: 49 $  $Date: 12/13/12 9:26p $
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

static ENGRUN_RUNLOG   m_engineRunLog[MAX_ENGINES];

static ENGINE_FILE_HDR m_EngineInfo;

static void (*m_event_func)(INT32,BOOLEAN) = NULL;
static INT32 m_event_tag                   = 0;


// Include cmd tables and functions here after local dependencies are declared.
#include "EngineRunUserTables.c"


/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void    EngRunTask       ( void* pParam );
static void    EngRunForceEnd   ( void );
static void    EngRunReset      ( ENGRUN_DATA* pErData);
static BOOLEAN EngRunIsError    ( const ENGRUN_CFG* pErCfg);

static void EngRunUpdate         (ENGRUN_CFG* pErCfg, ENGRUN_DATA* pErData);

static void EngRunStartLog       ( ENGRUN_DATA* pErData );
static void EngRunUpdateStartData( ENGRUN_DATA* pErData, BOOLEAN bUpdateDuration);

static void EngRunUpdateRunData  ( ENGRUN_DATA* pErData );
static void EngRunWriteRunLog    ( ER_REASON reason, ENGRUN_DATA* pErData );

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
  UINT32   i;
  UINT32   runMask;  // bit map of started/running EngineRuns.
  ER_STATE state = ER_STATE_STOPPED;
  BOOLEAN  bRunning  = FALSE;
  BOOLEAN  bStarting = FALSE;

  // If the user wants a list of ALL running engine-runs, return
  // it in the provided location.
  if ( ENGRUN_ID_ANY == idx )
  {
    runMask = 0;

    for (i = 0; i < MAX_ENGINES; ++i)
    {
      if (ER_STATE_RUNNING == m_engineRunData[i].erState)
      {
        // Set the bit in the flag
        runMask |= 1u << i;
        bRunning = TRUE;
      }
      else if (ER_STATE_STARTING == m_engineRunData[i].erState)
      {
        // Set the bit in the flag
        runMask |= 1u << i;
        bStarting = TRUE;
      }
    }

    // Assign overall engine run state based in the following priority:
    // RUNNING->STARTING->STOPPED
    state = bRunning ? ER_STATE_RUNNING : bStarting ? ER_STATE_STARTING : ER_STATE_STOPPED;

    // If the caller has passed a field to return the list, fill it.
    if(NULL != engRunFlags)
    {
      *engRunFlags = (UINT8)(runMask & 0x000000FF);
    }
  }
  else // Get the current state of the requested engine run.
  {
    ASSERT ( idx >= ENGRUN_ID_0 && idx < (ENGRUN_INDEX)MAX_ENGINES);
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
  TCB taskBlock;
  UINT16 i;
  ENGRUN_CFG_PTR  pErCfg;
  ENGRUN_DATA_PTR pErData;
  RESULT result;

  // Add user commands for EngineRun to the user command tables.

  User_AddRootCmd(&rootEngRunMsg);

  // Clear and Load the current cfg info.
  memset(m_engineRunCfg, 0, sizeof(m_engineRunCfg));

  memcpy( m_engineRunCfg,
          CfgMgr_RuntimeConfigPtr()->EngineRunConfigs,
          sizeof(m_engineRunCfg) );

  //Allow c-init to take care of these b/c FASTMgr init sets them and may
  //overwrite depending on order of initialization.
  //m_event_tag = 0;
  //m_event_func = NULL;

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
         SensorIsUsed       (pErCfg->minMonSensorID ) &&
         SensorIsUsed       (pErCfg->maxMonSensorID ) )
    {
      // Initialize the common fields in the EngineRun data structure
      pErData->erIndex = (ENGRUN_INDEX)i;

      // copy these to simplify life
      pErData->minMonSensorID = pErCfg->minMonSensorID;
      pErData->maxMonSensorID = pErCfg->maxMonSensorID;

      EngRunReset( pErData);

      // these are handled here as they are monitored in stop mode and not done in EngRunReset
      // They are also reset on all transitions into STOPPED mode in EngRunUpdate
      pErData->minValueValid = TRUE;
      pErData->minMonValue   = FLT_MAX;

      // Init Task scheduling data
      pErData->nRateCounts    = (UINT16)(MIFs_PER_SECOND / (UINT32)pErCfg->erRate);
      pErData->nRateCountdown = (UINT16)((pErCfg->nOffset_ms / MIF_PERIOD_mS) + 1);

      pErData->nTotalSensors  = SensorInitSummaryArray ( pErData->snsrSummary,
                                                         MAX_ENGRUN_SENSORS,
                                                         pErCfg->sensorMap,
                                                         sizeof(pErCfg->sensorMap) );      
    }
    else // Invalid config...
    {
      // Set ER state to hold in STOP state if
      // sensor Cfg is invalid
      pErData->erState = ER_STATE_STOPPED;
      pErData->erIndex = ENGRUN_UNUSED;
    }
  }

  // Create EngineRun Task - DT
  memset(&taskBlock, 0, sizeof(taskBlock));
  strncpy_safe(taskBlock.Name, sizeof(taskBlock.Name),"EngineRun Task",_TRUNCATE);
  taskBlock.TaskID           = (TASK_INDEX) EngRun_Task;
  taskBlock.Function         = EngRunTask;
  taskBlock.Priority         = taskInfo[EngRun_Task].priority;
  taskBlock.Type             = taskInfo[EngRun_Task].taskType;
  taskBlock.modes            = taskInfo[EngRun_Task].modes;
  taskBlock.MIFrames         = taskInfo[EngRun_Task].MIFframes;
  taskBlock.Enabled          = TRUE;
  taskBlock.Locked           = FALSE;

  taskBlock.Rmt.InitialMif   = taskInfo[EngRun_Task].InitialMif;
  taskBlock.Rmt.MifRate      = taskInfo[EngRun_Task].MIFrate;
  taskBlock.pParamBlock      = NULL;
  TmTaskCreate (&taskBlock);
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
         ((engineIndex < MAX_ENGINES) && (nRemaining > sizeof (ENGRUN_HDR)));
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
      nTotal     += sizeof (ENGRUN_HDR);
      nRemaining -= sizeof (ENGRUN_HDR);
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



/*****************************************************************************
 * Function:    EngRunSetRecStateChangeEvt
 *
 * Description: Set the callback function to call when the busy/not busy
 *              status of the Engine Run module changes
 *
 *
 *
 * Parameters:  [in] tag:  Integer value to use when calling "func"
 *              [in] func: Function to call when busy/not busy status changes
 *
 * Returns:      none
 *
 * Notes:       Only remembers one event handler.  Subsequent calls overwrite
 *              the last event handler.
 *
 *******************************************************************************/
void EngRunSetRecStateChangeEvt(INT32 tag,void (*func)(INT32,BOOLEAN))
{
  m_event_func = func;
  m_event_tag  = tag;
}


/******************************************************************************
 * Function:    EngReInitFile
 *
 * Description: Set the NV data (Engine Serial Numbers) to
 *              the default values
 *
 * Parameters:  void
 *
 * Returns:     TRUE
 *
 * Notes:       Standard Initiliazation format to be compatible with 
 *              NVMgr Interface.
 *
 *****************************************************************************/
BOOLEAN EngReInitFile ( void )
{
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
  
  return TRUE;
}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

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
static void EngRunTask(void* pParam)
{
  UINT16 i;
  static BOOLEAN is_active_last = FALSE;
  BOOLEAN is_active = FALSE;

  if (Tm.systemMode == SYS_SHUTDOWN_ID)
  {
    EngRunForceEnd();
    TmTaskEnable ((TASK_INDEX)EngRun_Task, FALSE);
    if(m_event_func != NULL)
    {
      m_event_func(m_event_tag,FALSE);
    }
    is_active_last = FALSE;
  }
  else
  {
    // EngRunUpdateAll - Normal execution
    for ( i = 0; i < MAX_ENGINES; i++)
    {
      if (m_engineRunData[i].erIndex != ENGRUN_UNUSED)
      {
        EngRunUpdate(&m_engineRunCfg[i],&m_engineRunData[i]);

        is_active =   (m_engineRunData[i].erState == ER_STATE_RUNNING) ||
                      (m_engineRunData[i].erState == ER_STATE_STARTING)   ?
                      TRUE : is_active;
      }
    }

    //Update recording/active status to "parent"
    if(is_active != is_active_last)
    {
      if(m_event_func != NULL)
      {
        m_event_func(m_event_tag,is_active);
      }
      is_active_last = is_active;
    }
  }
}

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
  ENGRUN_DATA* pErData;

  // Close out any active cycle
  // and flag the task to stop.
  // Eng
  for (i = 0; i < MAX_ENGINES; ++i)
  {
    pErData = &m_engineRunData[i];

    // Only process this engine if it's USED
    if (pErData->erIndex != ENGRUN_UNUSED)
    {
      switch (pErData->erState)
      {
        case ER_STATE_STOPPED:
          break;

        // Known fallthrough - tag for no warning from lint
        //lint -fallthrough
        case ER_STATE_STARTING:
        case ER_STATE_RUNNING:
          // Update persist, and create log
          EngRunWriteRunLog(ER_LOG_SHUTDOWN, pErData);
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
 * Parameters:   [in/out] pErData - engine run data
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static void EngRunReset(ENGRUN_DATA* pErData)
{
    pErData->erState             = ER_STATE_STOPPED;
    memset(&pErData->startTime, 0, sizeof(TIMESTAMP));
    pErData->startingTime_ms     = 0;
    pErData->startingDuration_ms = 0;
    pErData->erDuration_ms       = 0;

    //pErData->minValueValid       = TRUE;
    //pErData->monMinValue         = FLT_MAX;
    pErData->maxValueValid       = TRUE;
    pErData->maxMonValue         = -FLT_MAX;

    /* SNSR_SUMMARY field setup */
    SensorResetSummaryArray (pErData->snsrSummary, pErData->nTotalSensors);

    // Reset cycles & counts for this engine run
    CycleResetEngineRun(pErData->erIndex);
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
          EngRunUpdateStartData( pErData, FALSE);

          // Check that all transition-trigger are valid,
          // then see if engine start/running is occurring
          // if the engine is running ( ex: system restart in-air),
          // we still go thru START to set up initialization and logs
          if ( !EngRunIsError( pErCfg) )
          {
            if ( TriggerGetState( pErCfg->startTrigID) ||
                 TriggerGetState( pErCfg->runTrigID  ) )
            {
              // init the start-log entry and set start-time
              // clear the cycle counts for this engine run
              EngRunStartLog( pErData);
              pErData->erState = ER_STATE_STARTING;
            }
          }
          else
          {
            // Ensure we don't collected any false start data while invalid
            // reset this engine run and its cycles.
            EngRunReset(pErData);
            // don't forget these special ones
            pErData->minValueValid = TRUE;
            pErData->minMonValue   = FLT_MAX;
          }
        }
        break;

    case ER_STATE_STARTING:
      // Always update the ER Starter parameters.
      EngRunUpdateStartData(pErData, TRUE);

      // STARTING -> STOP
      // Error Detected
      // If we have a problem determining the EngineRun state,
      // write the engine run start-log and transition to STOPPED state
      if ( EngRunIsError(pErCfg))
      {
        pErData->erDuration_ms += (UINT32)pErCfg->erRate;

        // Finish the engine run log
        EngRunWriteRunLog( ER_LOG_ERROR, pErData);
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
            EngRunWriteRunLog(ER_LOG_STOPPED, pErData);
            pErData->erState = ER_STATE_STOPPED;
          }
        }
      }
      break;

    case ER_STATE_RUNNING:

      // Update the EngineRun data
      EngRunUpdateRunData( pErData);

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
        pErData->erState = ER_STATE_STOPPED;
      }
      else
      {
        // If the engine run has stopped, finish the engine run and change states
        // RUNNING -> STOP
        if ( TriggerGetState( pErCfg->stopTrigID) ||
            !TriggerGetState( pErCfg->runTrigID) )
        {
          // Finish the engine run log
          EngRunWriteRunLog(ER_LOG_STOPPED, pErData);
          pErData->erState = ER_STATE_STOPPED;
        }
      }
      break;

    default:
      FATAL("Unrecognized engine run state %d", pErData->erState );
      break;
    }

    // Update the cycles state and data for this engine run
    CycleUpdateAll(pErData->erIndex);

    // General State Transition processing
    if ( pErData->erState != curState )
    {
      // on transition into STOPPED state from any state reset the monitored min value info
      if (pErData->erState == ER_STATE_STOPPED)
      {
          // these are monitored in stop mode so the must be reset on transition into STOPPED
          pErData->minValueValid = TRUE;
          pErData->minMonValue   = FLT_MAX;
      }

      // Always display msg when state changes.
      GSE_DebugStr ( NORMAL, TRUE, "EngineRun(%d) %s -> %s ",
                     pErData->erIndex,
                     engineRunStateEnum[curState].Str,
                     engineRunStateEnum[pErData->erState].Str);
    }
  }
}

/******************************************************************************
* Function:     EngRunStartLog
*
* Description:  Start collecting EngineRunlog data
*
* Parameters:   [in/out] pErData
*
* Returns:      None
*
* Notes:        None
*
*****************************************************************************/
static void EngRunStartLog( ENGRUN_DATA* pErData )
{
  FLOAT32 valueMax;

  // Reset the ER
  EngRunReset( pErData);

  // Set the start timestamp, init the end timestamp
  CM_GetTimeAsTimestamp(&pErData->startTime);

  // Set the 'tick count' of the start time( used to calculate duration)
  pErData->startingTime_ms = CM_GetTickCount();

  // Init the max value only if valid.
  if ( SensorIsValid(pErData->maxMonSensorID) )
  {
    valueMax = SensorGetValue(pErData->maxMonSensorID);
    if ( pErData->maxMonValue < valueMax)
    {
      pErData->maxMonValue   = valueMax;
    }
  }
  else
  {
    pErData->maxValueValid = FALSE;
  }
}

/******************************************************************************
 * Function:     EngRunWriteRunLog
 *
 * Description:  This function write the log that saves all of the Engine run 
 *               data.
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
  UINT32  cycleCounts[MAX_CYCLES];
  ENGRUN_RUNLOG* pLog;

  SNSR_SUMMARY*  pErSummary;
  SNSR_SUMMARY*  pLogSummary;

  memset(cycleCounts, 0, sizeof(cycleCounts) );

  pLog = &m_engineRunLog[pErData->erIndex];

  // GSE_DebugStr(NORMAL,TRUE,"Frames: %d", pErData->nSampleCount);

  // Tell Cycles to finish up for this engine run.
  // This will ensure Cycles has brought its count structure up-to-date.
  CycleFinishEngineRun( pErData->erIndex);

  // Fetch the cycle counts for 'this' engine run
  CycleCollectCounts(cycleCounts, pErData->erIndex );

  // Make the log entry

  pLog->erIndex             = pErData->erIndex;
  pLog->reason              = reason;
  pLog->startTime           = pErData->startTime;
  CM_GetTimeAsTimestamp(&pLog->endTime);

  pLog->startingDuration_ms = pErData->startingDuration_ms;
  pLog->erDuration_ms       = pErData->erDuration_ms;

  pLog->minMonSensorID      = pErData->minMonSensorID;
  pLog->minValueValid       = pErData->minValueValid;
  pLog->minMonValue         = pErData->minMonValue;

  pLog->maxMonSensorID      = pErData->maxMonSensorID;
  pLog->maxValueValid       = pErData->maxValueValid;
  pLog->maxMonValue         = pErData->maxMonValue;

  SensorCalculateSummaryAvgs(pErData->snsrSummary, pErData->nTotalSensors);

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

      pLogSummary->nSampleCount = pErSummary->nSampleCount;
      pLogSummary->fAvgValue = pErSummary->fAvgValue;
    }
    else // Sensor Not Used
    {
      pLogSummary->SensorIndex = SENSOR_UNUSED;
      pLogSummary->bValid      = FALSE;
      pLogSummary->fTotal      = 0.f;
      pLogSummary->fMinValue   = 0.f;
      pLogSummary->fMaxValue   = 0.f;
      pLogSummary->fAvgValue   = 0.f;
      pLogSummary->nSampleCount = 0;
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
  // Update the total run duration
  pErData->erDuration_ms = CM_GetTickCount() - pErData->startingTime_ms;

  // Update the sensor summaries
  SensorUpdateSummaries(pErData->snsrSummary, pErData->nTotalSensors);
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
 * Parameters:   [in] pErData - data
 *               [in] bUpdateDuration - start duration update flag.
 *
 * Returns:      None
 *
 * Notes:        bUpdateDuration is set to false when called during STOP state
 *               to maintain min/max values only. During START state, it is
 *               called with true to maintain the start duration elapsed time.
 *
 *****************************************************************************/
static void EngRunUpdateStartData( ENGRUN_DATA* pErData, BOOLEAN bUpdateDuration)
{
  FLOAT32 valueMin;
  FLOAT32 valueMax;

  // Update the min during STOP and START
  if ( SensorIsValid(pErData->minMonSensorID) )
  {
    valueMin  = SensorGetValue(pErData->minMonSensorID);

    if ( pErData->minMonValue > valueMin )
    {
      pErData->minMonValue = valueMin;
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
    if ( SensorIsValid(pErData->maxMonSensorID) )
    {
      valueMax = SensorGetValue(pErData->maxMonSensorID);
      if ( pErData->maxMonValue < valueMax)
      {
        pErData->maxMonValue   = valueMax;
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
 * *****************  Version 49  *****************
 * User: Jeff Vahue   Date: 12/13/12   Time: 9:26p
 * Updated in $/software/control processor/code/application
 * SCR# 1205 - Ensure Min Sensor in ER gets a chance to be valid at
 * startup.
 * 
 * *****************  Version 48  *****************
 * User: Jeff Vahue   Date: 12/13/12   Time: 3:08p
 * Updated in $/software/control processor/code/application
 * SCR# 1205 - Remove ER Start Log
 * 
 * *****************  Version 47  *****************
 * User: Contractor V&v Date: 12/12/12   Time: 6:23p
 * Updated in $/software/control processor/code/application
 * SCR #1107 Code Review  BB #87 and reset before start/run
 * 
 * *****************  Version 46  *****************
 * User: Contractor V&v Date: 12/12/12   Time: 2:13p
 * Updated in $/software/control processor/code/application
 * SCR #1107 BitBucket #87
 *
 * *****************  Version 45  *****************
 * User: John Omalley Date: 12-12-08   Time: 1:33p
 * Updated in $/software/control processor/code/application
 * SCR 1167 - Sensor Summary Init optimization
 * 
 * *****************  Version 44  *****************
 * User: John Omalley Date: 12-12-08   Time: 11:44a
 * Updated in $/software/control processor/code/application
 * SCR 1162 - NV MGR File Init function
 * 
 * *****************  Version 43  *****************
 * User: Contractor V&v Date: 12/05/12   Time: 4:16p
 * Updated in $/software/control processor/code/application
 * SCR #1107 Code Review 
 *
 * *****************  Version 42  *****************
 * User: Contractor V&v Date: 12/03/12   Time: 5:32p
 * Updated in $/software/control processor/code/application
 * SCR #1167 Add sample count to ER Log
 *
 * *****************  Version 41  *****************
 * User: Jim Mood     Date: 11/28/12   Time: 6:36p
 * Updated in $/software/control processor/code/application
 * SCR #1131 Modfix
 *
 * *****************  Version 40  *****************
 * User: Contractor V&v Date: 11/26/12   Time: 6:06p
 * Updated in $/software/control processor/code/application
 * SCR #1107 Code Review
 *
 * *****************  Version 39  *****************
 * User: Contractor V&v Date: 11/26/12   Time: 12:31p
 * Updated in $/software/control processor/code/application
 * SCR #1167 SensorSummary
 *
 * *****************  Version 38  *****************
 * User: Contractor V&v Date: 11/16/12   Time: 8:11p
 * Updated in $/software/control processor/code/application
 * CodeReview
 *
 * *****************  Version 37  *****************
 * User: John Omalley Date: 12-11-14   Time: 6:51p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Code Review Updates
 *
 * *****************  Version 36  *****************
 * User: Contractor V&v Date: 11/14/12   Time: 12:25p
 * Updated in $/software/control processor/code/application
 * Code Review corrected loss of precision warning
 *
 * *****************  Version 35  *****************
 * User: Contractor V&v Date: 11/13/12   Time: 6:27p
 * Updated in $/software/control processor/code/application
 * Code Review
 *
 * *****************  Version 34  *****************
 * User: Contractor V&v Date: 11/12/12   Time: 6:39p
 * Updated in $/software/control processor/code/application
 * Code Review
 *
 * *****************  Version 33  *****************
 * User: Jim Mood     Date: 11/09/12   Time: 6:34p
 * Updated in $/software/control processor/code/application
 * SCR 1131 Recording busy status
 *
 * *****************  Version 32  *****************
 * User: Contractor V&v Date: 11/09/12   Time: 5:16p
 * Updated in $/software/control processor/code/application
 * Code review
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

