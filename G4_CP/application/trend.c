#define TREND_BODY
/******************************************************************************
            Copyright (C) 2015-2016 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

   ECCN:        9D991

   File:        trend.c

   Description: Trend Processing is a mechanism for capturing system behavior
                during the EngineRun-Active state or by API command. Up to 6
                trends may be configured in the system.

                Trends are configured to collect sensor summary data according
                to the following configurable settings:
                    - Trend Type (Standard, Commanded)
                    - Stability criteria
                    - Sample period per collection.
                    - Sample rate during a collection.
                    - Delay interval between collections.
                    - The maximum number of collection.
                    - End-Period sub-sampling.

                Trends will be configured to collect sensor summary data on up to
                32 sensors.

                A trend can transition between the following states:
                    - IDLE - The trend is inactive, waiting for conditions to enter READY
                    - READY - The trend is searching for stability conditions or waiting
                              for a direct directive to enter SAMPLE
                    - SAMPLE - The trend is actively sampling and collecting sensor input.
                               After a configured time the trend will transition to IDLE
                               or READY.

                Trend processing will support two types: STANDARD Trend and  COMMANDED Trend.

   Note: None

 VERSION
 $Revision: 52 $  $Date: 4/14/16 3:03p $

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
#include "trend.h"
#include "taskmanager.h"
#include "cfgmanager.h"
#include "gse.h"
#include "user.h"
#include "EngineRun.h"
#include "Cycle.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define RESET_TRIGGER_ACTIVE(rst)  \
  (((TRIGGER_INDEX)rst != TRIGGER_UNUSED)     &&  \
    (TriggerIsConfigured((TRIGGER_INDEX)rst)) &&  \
    (TriggerGetState((TRIGGER_INDEX)rst)))

// Activate trend debug tracing. Set fault.verbosity = NORMAL
#undef TREND_DEBUG

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
typedef struct
{
  BOOLEAN bIsUsed;
  BOOLEAN bIsValid;
}SENSOR_INFO_TEMP;
/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/

static TREND_CFG   m_TrendCfg [MAX_TRENDS];   // Trend cfg
static TREND_DATA  m_TrendData[MAX_TRENDS];   // Runtime trend data
static TREND_LOG   m_TrendLog [MAX_TRENDS];   // Trend Log data for each trend
static TREND_LOG   m_TrendEndLog [MAX_TRENDS];// Trend Log data for each trend for end-phase.

#include "TrendUserTables.c"

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void TrendTask            ( void* pParam );
static void TrendProcessStandard ( TREND_CFG* pCfg, TREND_DATA* pData );
static void TrendProcessCommanded( TREND_CFG* pCfg, TREND_DATA* pData );

static void TrendFinish          ( TREND_CFG* pCfg, TREND_DATA* pData );

static void TrendStartManualTrend   (TREND_CFG* pCfg, TREND_DATA* pData );
static void TrendStartAutoTrend     (TREND_CFG* pCfg, TREND_DATA* pData);
static void TrendStartCommandedTrend(TREND_CFG* pCfg, TREND_DATA* pData);

static void TrendUpdateData              ( TREND_CFG* pCfg, TREND_DATA* pData );
static void TrendCheckAutoTrendStability ( TREND_CFG* pCfg, TREND_DATA* pData );
static void TrendCheckCmdedTrendStability( TREND_CFG* pCfg, TREND_DATA* pData );

static void TrendCheckResetTrigger( TREND_CFG* pCfg, TREND_DATA* pData );
static BOOLEAN TrendCheckStability( TREND_CFG* pCfg, TREND_DATA* pData, UINT32 now);
// Functions to implement
static BOOLEAN TrendMonStabilityByIncrement( TREND_CFG* pCfg, TREND_DATA* pData,
                                             INT32 tblIndex, BOOLEAN bDoVarCheck);
static BOOLEAN TrendMonStabilityAbsolute ( TREND_CFG* pCfg, TREND_DATA* pData,
                                           INT32 tblIndex, BOOLEAN bDoVarCheck);

//static void TrendHandleDelayedStability( TREND_CFG* pCfg, TREND_DATA* pData, INT32 tblIndex);

static void TrendWriteErrorLog(SYS_APP_ID logID, TREND_INDEX trendIdx,
                               STABILITY_CRITERIA stabCrit[], STABILITY_DATA* stabData);

static void TrendReset(TREND_CFG* pCfg, TREND_DATA* pData, BOOLEAN bRunTime);

static void TrendEnableTask(BOOLEAN bEnable);

static void TrendSetStabilityState(TREND_DATA* pData, STABILITY_STATE stbState);

static void TrendUpdateTimeElapsed( UINT32  currentTime,
                                    UINT32* lastTimeCalled,
                                    UINT32* elapsedTime);

static void TrendSetDelayStabilityStartTime(TREND_CFG* pCfg, TREND_DATA* pData);
static void TrendClearSensorStabilityHistory(TREND_DATA* pData);

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function: TrendInitialize
 *
 * Description: Initialize the trend task
 *
 * Parameters:   None
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
void TrendInitialize( void )
{
  TCB   tcb;
  TREND_INDEX i;
  UINT8 j;
  TREND_CFG*  pCfg;
  TREND_DATA* pData;

  // Add user commands for Trend to the user command tables.
  User_AddRootCmd(&rootTrendMsg);

  // Clear and Load the current cfg info.
  memset(m_TrendData, 0, sizeof(m_TrendData));

  memcpy(m_TrendCfg,
    CfgMgr_RuntimeConfigPtr()->TrendConfigs,
    sizeof(m_TrendCfg));


  #pragma ghs nowarning 1545 //Suppress packed structure alignment warning

  for (i = TREND_0; i < MAX_TRENDS; i++)
  {
    pCfg  = &m_TrendCfg[i];
    pData = &m_TrendData[i];

    // Check that a STANDARD trend ( not COMMANDED) has an ENGRUN configured, or the trend is
    // configured as COMMANDED
    pData->trendIndex      = (ENGRUN_UNUSED == pCfg->engineRunId && !pCfg->bCommanded)
                             ?  TREND_UNUSED : (TREND_INDEX)i;

    pData->prevEngState    = ER_STATE_STOPPED;
    pData->nRateCounts     = (INT16)(MIFs_PER_SECOND / (INT16)pCfg->rate);
    pData->nRateCountdown  = (INT16)((pCfg->nOffset_ms / MIF_PERIOD_mS) + 1);
    pData->bEnabled        = FALSE;
    pData->bTrendStabilityFailed = FALSE;
    pData->trendState      = TR_TYPE_INACTIVE;
    pData->nActionReqNum   = ACTION_NO_REQ;

    pData->nSamplesPerPeriod = pCfg->nSamplePeriod_s * (INT16)pCfg->rate;

    // Initialize the Sensor map.
    pData->nTotalSensors = SensorInitSummaryArray ( pData->snsrSummary,
                                                    MAX_TREND_SENSORS,
                                                    pCfg->sensorMap,
                                                    sizeof(pCfg->sensorMap) );

    ASSERT(pData->nTotalSensors <= MAX_TREND_SENSORS );

    // Always init the end-phase sensor-summary structure

    // NOTE: The 'nTotalSensors' count is not affected since we are counting
    // ALL configured sample sensors during the end.

    SensorInitSummaryArray ( pData->endSummary,
                             MAX_TREND_SENSORS,
                             pCfg->sensorMap,
                             sizeof(pCfg->sensorMap) );

    // Assign the function to use for sensor-stability determination.
    pData->pStabilityFunc = STB_STRGY_INCRMNTL == pCfg->stableStrategy
                            ? TrendMonStabilityByIncrement
                            : STB_STRGY_ABSOLUTE == pCfg->stableStrategy
                            ? TrendMonStabilityAbsolute
                            : NULL;

    // Calculate the expected number of stability sensors based on config
    pData->nStabExpectedCnt = 0;
    for (j = 0; j < MAX_STAB_SENSORS; ++j)
    {
      if (SENSOR_UNUSED != pCfg->stability[j].sensorIndex)
      {
        pData->nStabExpectedCnt++;
      }
    }

    pData->trendCnt      = 0;
    pData->autoTrendCnt  = 0;

    // Use the reset function to init the others fields
    TrendReset(pCfg, pData, FALSE);

    // Init the stability state and entry time.
    TrendSetStabilityState(pData, STB_STATE_IDLE);
    pData->prevStableState = STB_STATE_IDLE;

    // Clear the apac cmd word.
    pData->apacCmd = APAC_CMD_COMPL;

  }
#pragma ghs endnowarning

  // Create Trend Task - DT
  memset(&tcb, 0, sizeof(tcb));
  strncpy_safe(tcb.Name, sizeof(tcb.Name),"Trend Task",_TRUNCATE);
  tcb.TaskID           = (TASK_INDEX)Trend_Task;
  tcb.Function         = TrendTask;
  tcb.Priority         = taskInfo[Trend_Task].priority;
  tcb.Type             = taskInfo[Trend_Task].taskType;
  tcb.modes            = taskInfo[Trend_Task].modes;
  tcb.MIFrames         = taskInfo[Trend_Task].MIFframes;
  tcb.Enabled          = TRUE;
  tcb.Locked           = FALSE;

  tcb.Rmt.InitialMif   = taskInfo[Trend_Task].InitialMif;
  tcb.Rmt.MifRate      = taskInfo[Trend_Task].MIFrate;
  tcb.pParamBlock      = NULL;
  TmTaskCreate (&tcb);
}
/******************************************************************************
 * Function: TrendTask
 *
 * Description: Task function for Trend processing
 *
 * Parameters:   pParam - optional param block passed from Task Manatger
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static void TrendTask( void* pParam )
{
  // Local Data
  TREND_CFG    *pCfg;
  TREND_DATA   *pData;
  TREND_INDEX  nTrend;

  if(Tm.systemMode == SYS_SHUTDOWN_ID)
  {
    for ( nTrend = TREND_0; nTrend < MAX_TRENDS; nTrend++ )
    {
      // Log and terminate any active trends
      TrendFinish(&m_TrendCfg[nTrend], &m_TrendData[nTrend]);
      TrendSetStabilityState(&m_TrendData[nTrend], STB_STATE_IDLE);
    }
    TrendEnableTask(FALSE);
  }
  else // Normal task execution.
  {
    // Loop through all available triggers
    for ( nTrend = TREND_0; nTrend < MAX_TRENDS; nTrend++ )
    {
      // Set pointers to Trend cfg and data
      pCfg  = &m_TrendCfg[nTrend];
      pData = &m_TrendData[nTrend];

      // Determine if current trend is 'defined' (i.e. is configured for use)
      if (pData->trendIndex != TREND_UNUSED)
      {
        // Countdown the number of subframes until next execution of this trend
        if (--pData->nRateCountdown <= 0)
        {
          // Calculate the next execute time
          pData->nRateCountdown = pData->nRateCounts;

          // Process the trend according to STANDARD or COMMANDED type
          if ( FALSE == pCfg->bCommanded )
          {
            // Process trend based on application
            TrendProcessStandard(pCfg, pData);
          }
          else
          {
            TrendProcessCommanded(pCfg, pData);
          }
        }
      }
    }
  }
}

/******************************************************************************
 * Function:     TrendGetBinaryHdr
 *
 * Description:  Retrieves the binary header for the trend
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
UINT16 TrendGetBinaryHdr ( void *pDest, UINT16 nMaxByteSize )
{
   // Local Data
   TREND_HDR  trendHdr[MAX_TRENDS];
   TREND_INDEX trendIndex;
   INT8       *pBuffer;
   UINT16     nRemaining;
   UINT16     nTotal;

   // Initialize Local Data
   pBuffer    = (INT8 *)pDest;
   nRemaining = nMaxByteSize;
   nTotal     = 0;
   memset ( trendHdr, 0, sizeof(trendHdr) );

   // Loop through all the Trends
   for ( trendIndex = TREND_0;
         ((trendIndex < MAX_TRENDS) && (nRemaining > sizeof (TREND_HDR)));
         trendIndex++ )
   {
      // Copy the Trend names
      strncpy_safe( trendHdr[trendIndex].trendName,
                    sizeof(trendHdr[trendIndex].trendName),
                    m_TrendCfg[trendIndex].trendName,
                    _TRUNCATE);

      trendHdr[trendIndex].rate          = m_TrendCfg[trendIndex].rate;
      trendHdr[trendIndex].samplePeriod  = m_TrendCfg[trendIndex].nSamplePeriod_s;
      trendHdr[trendIndex].maxTrends     = m_TrendCfg[trendIndex].maxTrends;
      trendHdr[trendIndex].trendInterval = m_TrendCfg[trendIndex].trendInterval_s;
      trendHdr[trendIndex].nTimeStable_s = m_TrendCfg[trendIndex].stabilityPeriod_s;
      trendHdr[trendIndex].startTrigger  = m_TrendCfg[trendIndex].startTrigger;
      trendHdr[trendIndex].resetTrigger  = m_TrendCfg[trendIndex].resetTrigger;


      // Increment the total number of bytes and decrement the remaining
      nTotal     += sizeof (TREND_HDR);
      nRemaining -= sizeof (TREND_HDR);
   }
   // Copy the Trend header to the buffer
   memcpy ( pBuffer, trendHdr, nTotal );
   // Return the total number of bytes written
   return ( nTotal );
}

/******************************************************************************
 * Function:     TrendAppStartTrend
 *
 * Description:  Signal a trend to Start or Stop.
 *               This call is made from an another task(APAC) to control starting or
 *               stopping a commanded trend. the TREND_DATA.apacCmd will be checked on
 *               next TrendTask execution for the indicated trend.
 *
 * Parameters:   TREND_INDEX idx - Index of the Trend to be started
 *
 *               BOOLEAN bStart  - TRUE if trend is to be started.
 *                                 FALSE if trend is to be stopped.
 *
 *               TREND_CMD_RESULT* rslt - optional pointer to return reason
 *                                 the call failed.
 *
 * Returns:      BOOLEAN - TRUE if call successful, otherwise FALSE
 *
 * Notes:        The indicated Trend is checked for:
 *               1. idx represents a configured trend id and
 *               2. trend[idx].cfg.commanded = TRUE and
 *
 *              if either of the above fail FALSE is returned and the reason is
 *              returned in the caller provided pointer 'rslt'.
 *
 *****************************************************************************/
BOOLEAN TrendAppStartTrend( TREND_INDEX idx, BOOLEAN bStart, TREND_CMD_RESULT* rslt)
{
  TREND_CMD_RESULT result = TREND_CMD_OK;  // Default to OK, set error as needed.
  TREND_CFG*  pCfg  = NULL;
  TREND_DATA* pData = NULL;

  ASSERT(idx < MAX_TRENDS);

  pCfg  = &m_TrendCfg[idx];
  pData = &m_TrendData[idx];

  if (TREND_UNUSED == pData->trendIndex)
  {
    result = TREND_NOT_CONFIGURED;
  }
  else if (TRUE != pCfg->bCommanded)
  {
    result = TREND_NOT_COMMANDABLE;
  }

  //If no config error detected in this call, signal the trend for the next time task executes.
  if ( TREND_CMD_OK == result )
  {
    //TrendSetStabilityState(pData, bStart ? STB_STATE_READY : STB_STATE_IDLE );
    pData->apacCmd = bStart ? APAC_CMD_START : APAC_CMD_STOP;
  }

  // Check if caller wants result details.
  if (NULL != rslt)
  {
    *rslt = result;
  }

  return (TREND_CMD_OK == result);
}

/******************************************************************************
 * Function:     TrendGetStabilityState
 *
 * Description:  Returns the stability state and duration for the trend indicated by
 *               idx.
 *
 * Parameters:   idx        - The TREND_INDEX index of the trend whose stability
 *                            state is requested.
 *               pStabState - Pointer to the current STABILITY_STATE of the trend
 *               pDurMs     - Pointer to the length of time in mSec,  the trend has been
 *                            in the current stability state
 *               pSampleResult - ptr to passed variable to signal whether the preceding
 *                               trend was successful or failed.
 *
 * Returns:      None
 *
 * Notes:        The function will check for the presence of not-null return pointers
 *               allowing selective requests for information.
 *
 *****************************************************************************/
void TrendGetStabilityState(TREND_INDEX idx, STABILITY_STATE* pStabState,
                            UINT32* pDurMs,  SAMPLE_RESULT* pSampleResult)
{
  TREND_DATA* pData = NULL;

  ASSERT(idx < MAX_TRENDS);
  pData = &m_TrendData[idx];

  if ( pStabState != NULL)
  {
    *pStabState = pData->stableState;
  }

  // Calculate the stable time using the start-time of the current stable state.
  if ( pDurMs != NULL)
  {
    *pDurMs = CM_GetTickCount() - pData->stableStateStartMs;
  }

  // Return the result of the last attempt to sample
  if ( pSampleResult != NULL)
  {
    *pSampleResult = pData->lastSampleResult;
  }
  return;
}


/******************************************************************************
 * Function:     TrendGetStabilityHistory()
 *
 * Description:  Retrieves the stability history for the sensors in a trend
 *               during stability-duration and sampling.
 *
 * Parameters:   [in]  idx - The TREND_INDEX index of the trend whose
 *                         - sensor stability data is requested.

 *               [out] pSnsrStableHist - Pointer to STABLE_HISTORY structure.
 *
 * Returns:      Always returns true.
 *
 * Notes:        FUNCTION MUST BE CALLED PRIOR TO RE-ENTERING THE 'READY' state
 *
 *****************************************************************************/
BOOLEAN TrendGetStabilityHistory( TREND_INDEX idx, STABLE_HISTORY* pSnsrStableHist)
{
  UINT8  i;
  BOOLEAN bResult = TRUE;
  TREND_DATA* pData = NULL;
  TREND_CFG*  pCfg  = NULL;

  ASSERT(idx < MAX_TRENDS);
  ASSERT( NULL != pSnsrStableHist );

  pData = &m_TrendData[idx];
  pCfg  = &m_TrendCfg[idx];

  // Populate a status history struct of which sensors in the stability criteria array were
  // stable during the last trend.

  for( i = 0; (i < MAX_STAB_SENSORS); ++i)
  {
    if ( i < pData->nStabExpectedCnt)
    {
      pSnsrStableHist->sensorID[i] = pCfg->stability[i].sensorIndex;

      pSnsrStableHist->bStable[i] = 
      (pData->curStability.stableDurMs[i]) < (MILLISECONDS_PER_SECOND*pCfg->stabilityPeriod_s)
         ? FALSE : TRUE;

#ifdef TREND_DEBUG
/*vcast_dont_instrument_start*/
      GSE_DebugStr(DBGOFF, TRUE, "Trend[%d]: Sensor[%d] stable history %d/%d.",
                              idx, pSnsrStableHist->sensorID[i],
                              pData->curStability.stableDurMs[i],
                              pData->cmdStabilityDurMs);
/*vcast_dont_instrument_end*/
#endif
    }
    else
    {
      // Init the remainder of the structure to unused
      pSnsrStableHist->sensorID[i] = SENSOR_UNUSED;
      pSnsrStableHist->bStable[i]  = FALSE;
    }
  }
  return bResult;
}

/******************************************************************************
* Function:     TrendGetSampleData()

*
* Description:  Retrieves the stability stats data for a trend
*
*
* Parameters:   [in]  idx            - The TREND_INDEX index of the trend whose stability
*                                      data is requested.
*               [out] pTrndSampleData - Pointer to the TREND_SAMPLE_DATA of the trend
*                                       The contents of the end-summary data collection is
*                                       returned
*
* Returns:      None
*
* Notes:        Function must be called prior to restarting the specified trend
*
*
*****************************************************************************/
void TrendGetSampleData( TREND_INDEX idx, TREND_SAMPLE_DATA* pTrndSampleData )
{
  ASSERT(NULL != pTrndSampleData);
  // copy all the sensor summaries to the caller.
  memcpy(pTrndSampleData->snsrSummary, m_TrendData[idx].endSummary,
		 sizeof(TREND_SAMPLE_DATA));
}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function: TrendProcessStandard
 *
 * Description: Normal processing function for Trend. Called by TrendTask.
 *
 * Parameters:   [in]     pCfg  Pointer to trend  config data
 *               [in/out] pData Pointer to trend runtime data
 *
 * Returns:      None
 *
 * Notes:        This function is called for Trends which are started/stopped
 *               as a result of meeting AUTO TREND stability criteria or
 *                MANUALLY via pilot triggering.
 *
 *****************************************************************************/
static void TrendProcessStandard( TREND_CFG* pCfg, TREND_DATA* pData )
{
  ER_STATE    erState;

  // Get the state of the associated EngineRun
  erState = EngRunGetState(pCfg->engineRunId, NULL);

  // Always check & handle potential Trend Reset Trigger.
  TrendCheckResetTrigger(pCfg, pData);
  pData->bEnabled = FALSE;

  switch ( erState )
  {
    case ER_STATE_STOPPED:
      // If we have entered stopped mode from some other mode,
      // check and finish any outstanding trends
      if (pData->prevEngState != ER_STATE_STOPPED)
      {
        TrendFinish(pCfg, pData);
        TrendSetStabilityState(pData, STB_STATE_IDLE);
        pData->prevEngState = ER_STATE_STOPPED;
      }
      break;

    case ER_STATE_STARTING:
      // If we have entered starting mode from some other mode,
      // finish any outstanding trends
      if (pData->prevEngState != ER_STATE_STARTING)
      {
        TrendFinish(pCfg, pData);
        TrendSetStabilityState(pData, STB_STATE_IDLE);
        pData->prevEngState = ER_STATE_STARTING;
      }
      break;

    case ER_STATE_RUNNING:
      // Upon entering RUNNING state...
      if (pData->prevEngState != ER_STATE_RUNNING)
      {
        //reset trending info from previous run-state.
        TrendReset(pCfg, pData,TRUE);

        TrendClearSensorStabilityHistory(pData);

        // if reset trigger is active at start of
        // ENGINE RUN STATE, don't enable until it is cleared.
        pData->bEnabled = ( RESET_TRIGGER_ACTIVE(pCfg->resetTrigger) ) ? FALSE : TRUE;

        // Standard Trends become READY upon engine RUNNING detected.
        TrendSetStabilityState(pData, STB_STATE_READY);

        // set the last engine run state
        pData->prevEngState = ER_STATE_RUNNING;

      }
      else if( FALSE == pData->bEnabled &&
                !(RESET_TRIGGER_ACTIVE(pCfg->resetTrigger)))
      {
        // While in running state and reset trigger transitioned active -> inactive...
        // enable for next reset trigger.
        pData->bEnabled = TRUE;
      }

      // Check to see if an Auto-trend has met/maintained stability
      TrendCheckAutoTrendStability(pCfg, pData);

      // Update current state of active MANUAL/AUTO TRENDS.
      TrendUpdateData(pCfg, pData);

      // If the button has been pressed(as defined by cfg startTrigger)
      // start a MANUAL trending

      if ( TriggerGetState(pCfg->startTrigger) )
      {
        TrendStartManualTrend(pCfg, pData);
      }
      break;

      // Should never get here!
   default:
      FATAL("Unrecognized Engine Run State: %d", erState);
      break;
  }
}


/******************************************************************************
 * Function: TrendStartManualTrend
 *
 * Description: Initiate a manual trend in response to button press( i.e. startTrigger)
 *              The trend will be sampled for the
 *              duration of TREND_CFG.nSamplePeriod_s
 *
 * Parameters:   [in]     pCfg  Pointer to trend  config data
 *               [in/out] pData Pointer to trend runtime data
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static void TrendStartManualTrend(TREND_CFG* pCfg, TREND_DATA* pData )
{
  // Local Data
  TREND_START_LOG trendLog;

  if ( TR_TYPE_INACTIVE == pData->trendState )
  {
    // startTrigger just detected  ( Inactive -> triggered)
    pData->lastSampleResult = SMPL_RSLT_UNK;
    pData->trendState       = TR_TYPE_STANDARD_MANUAL;
    TrendSetStabilityState(pData, STB_STATE_SAMPLE);
    pData->trendCnt++;

    // The sensor statistics shall be reset each time the trend becomes ACTIVE.
    SensorResetSummaryArray (pData->snsrSummary, pData->nTotalSensors);

    // Activate Action
    pData->nActionReqNum = ActionRequest(pData->nActionReqNum, pCfg->nAction,
                                           ACTION_ON, FALSE, FALSE);

    // Create TREND_START log
    trendLog.trendIndex = pData->trendIndex;
    trendLog.type       = pData->trendState;

    LogWriteETM( APP_ID_TREND_START,
                 LOG_PRIORITY_3,
                 &trendLog,
                 sizeof(trendLog),
                 NULL );

    GSE_DebugStr( NORMAL, TRUE, "Trend[%d]: Manual Trend started.",pData->trendIndex );
  }
}


/******************************************************************************
 * Function: TrendUpdateData
 *
 * Description:   Update the status of an ACTIVE trend
 *
 * Parameters:   [in]     pCfg  Pointer to trend  config data
 *               [in/out] pData Pointer to trend runtime data
 *
 * Returns:      None
 *
 * Notes:        This functions is used to update data for all TREND_TYPE
 *
 *****************************************************************************/
static void TrendUpdateData( TREND_CFG* pCfg, TREND_DATA* pData  )
{
  BOOLEAN bStabilityFailed = FALSE;

  // Only process stability failure for active trends
  if( pData->trendState != TR_TYPE_INACTIVE)
  {
    // Check if a preceding call to CheckXXXXStability found a problem
    if (TR_TYPE_STANDARD_AUTO == pData->trendState ||
        TR_TYPE_COMMANDED     == pData->trendState)
    {
      if( pData->bTrendStabilityFailed && pData->autoTrendCnt < pCfg->maxTrends )
      {
        // Force a finish to this sampling and flag it for stability-failure logging.
        bStabilityFailed = TRUE;
      }

      // Reset the signal, it will be set during the next frame if needed.
      pData->bTrendStabilityFailed = FALSE;
    }

    // General update handling and end-of-trend logging

    if ( bStabilityFailed )
    {
      // Log auto-trend failure. or commanded trend told to STOP
      // (Note: The trendIndex indicates if this is STANDARD_AUTO/COMMANDED)
      TrendWriteErrorLog( APP_ID_TREND_FAILED,
                          pData->trendIndex,
                          pCfg->stability,
                          &pData->curStability);

      // Use the trend state to update the stability state before marking it INACTIVE
      TrendSetStabilityState(pData, (TR_TYPE_STANDARD_AUTO == pData->trendState)
                                       ? STB_STATE_READY
                                       : STB_STATE_IDLE);

      // Store how long the trend was stable before the working counter is cleared
      // (used by TR_TYPE_COMMANDED trends)
      pData->cmdStabilityDurMs = pData->nTimeStableMs;

      pData->lastSampleResult = SMPL_RSLT_FAULTED;

      pData->trendState       = TR_TYPE_INACTIVE;

      // Reset counters for the next monitoring period
      pData->masterSampleCnt  = 0;

      // Reset the time when end-phase stability should start.
      pData->delayedStbStartMs = 0;

      // Reset sensor stability timer
      pData->lastStabCheckMs  = 0;
      pData->nTimeStableMs    = 0;

      // Reset interval timer
      pData->timeSinceLastTrendMs = 0;
      pData->lastIntervalCheckMs  = 0;
    }
    else if ( pData->masterSampleCnt >= pData->nSamplesPerPeriod )
    {
      // Active SAMPLE period completed on last pass
      // Set new stability state FIRST, because TrendFinish will clear trendState!
      TrendSetStabilityState(pData, (TR_TYPE_STANDARD_AUTO == pData->trendState)
                                       ? STB_STATE_READY
                                       : STB_STATE_IDLE);
      TrendFinish(pCfg, pData);
    }
    else
    {
      // Continuing to monitor... update master trend cnt and all configured summaries
      ++pData->masterSampleCnt;
      SensorUpdateSummaries(pData->snsrSummary, pData->nTotalSensors);

      // If the end-phase period for delayed stability checking is active, update end-summary.
      // Note: This uses the same set of sensors as in snsrSummary, but only for a smaller
      // sample period.
      if (pData->delayedStbStartMs != 0 && CM_GetTickCount() >= pData->delayedStbStartMs)
      {
        SensorUpdateSummaries(pData->endSummary, pData->nTotalSensors);
      }

    }// take next trend sample

  }// Process active trend

} // TrendUpdateData



/******************************************************************************
 * Function: TrendFinish
 *
 * Description:  Finish and create log for active trend.
 *
 * Parameters:   [in]     pCfg  Pointer to trend  config data
 *               [in/out] pData Pointer to trend runtime data
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static void TrendFinish( TREND_CFG* pCfg, TREND_DATA* pData )
{
  UINT8         i;
  UINT8         j;
  //STABILITY_STATE stbState = STB_STATE_IDLE;
  INT8          nLogCnt = 1;
  SNSR_SUMMARY  *pSummary;
  SNSR_SUMMARY  *pSummaryItem;
  TREND_LOG*    pLog;
  SYS_APP_ID    logId;
  SENSOR_INFO_TEMP  snsrStore[MAX_SENSORS];
  SENSOR_INFO_TEMP* pSnsrStore;
  SENSOR_INDEX  snsrIdx;

  // If a trend was in progress, create a log, otherwise return.
  if (pData->trendState > TR_TYPE_INACTIVE )
  {
    // How many logs are needed?
    // At least 1 or 2 if this trend is configured to record delayed/end-phase sampling.
    nLogCnt = ( pCfg->nOffsetStability_s > -1) ? 2 : 1;

    // Write out Trend log(s) as required...
    // First log is the usual APP_ID_TREND_END,
    // if another log is indicated, it is an APP_ID_TREND_SUBSAMPLE_END
    for ( j = 0; j < nLogCnt; j++ )
    {
      pLog     = (j == 0) ? &m_TrendLog[pData->trendIndex] : &m_TrendEndLog[pData->trendIndex];
      pSummary = (j == 0) ? pData->snsrSummary             : pData->endSummary;
      logId    = (j == 0) ? APP_ID_TREND_END               : APP_ID_TREND_SUBSAMPLE_END;

      SensorCalculateSummaryAvgs( pSummary, pData->nTotalSensors );

      // Trend name
      pLog->trendIndex = pData->trendIndex;

      // Log the type/state: MANUAL|AUTO|COMMANDED
      pLog->type = pData->trendState;

      pLog->nSamples = pSummary->nSampleCount; //pData->masterSampleCnt;

      // Loop through the configured summary sensors
      for ( i = 0; i < pData->nTotalSensors; i++ )
      {
        // Set a pointer the summary item(sensor)
        pSummaryItem = &(pSummary[i]);

        snsrIdx = pSummaryItem->SensorIndex;

        // Set pointer to the table entries for this sensor's use and valid state.
        pSnsrStore  = &(snsrStore[snsrIdx]);


        // For first log, set the use/validity entries in the table thru the pointer
        // then use the  stored values for this and subsequent logs to avoid
        // calling the funcs again.
        if (0 == j)
        {
           pSnsrStore->bIsValid = SensorIsUsedEx(snsrIdx, &pSnsrStore->bIsUsed );
          //pSnsrStore->bIsUsed  = SensorIsUsed(snsrIdx);
          //pSnsrStore->bIsValid = SensorIsValid(snsrIdx);
        }

        // Check the sensor is used
        if ( pSnsrStore->bIsUsed )
        {
          // Only some of the sensor stats are used for trend logging
          pLog->sensor[i].sensorIndex = pSummaryItem->SensorIndex;
          pLog->sensor[i].bValid      = pSnsrStore->bIsValid;
          pLog->sensor[i].fMaxValue   = pSummaryItem->fMaxValue;
          pLog->sensor[i].fAvgValue   = pSummaryItem->fAvgValue;
        }
        else // Sensor Not Used
        {
          pLog->sensor[i].sensorIndex = SENSOR_UNUSED;
          pLog->sensor[i].bValid      = FALSE;
          pLog->sensor[i].fMaxValue   = 0.0;
          pLog->sensor[i].fAvgValue   = 0.0;
        }
      }

  #pragma ghs nowarning 1545 //Suppress packed structure alignment warning

      // Update persistent cycle counter data, if configured.

      for (i = 0; i < MAX_TREND_CYCLES; i++)
      {
        pLog->cycleCounts[i].cycIndex =pCfg->cycle[i];
        if (CYCLE_UNUSED != pCfg->cycle[i])
        {
          pLog->cycleCounts[i].cycleCount = CycleGetCount(pCfg->cycle[i]);
        }
        else
        {
          pLog->cycleCounts[i].cycleCount = 0;
        }
      }
  #pragma ghs endnowarning

      // Now do the log details...
      LogWriteETM( logId,
                   LOG_PRIORITY_3,
                   pLog,
                   sizeof(TREND_LOG),
                   NULL);
    } // Writing TREND_END log(s)

    // Set the last sample result so APAC will know we were successful
    // and didn't transition to IDLE/READY because of stability failure

    pData->lastSampleResult = SMPL_RSLT_SUCCESS;

    // Reset the trend state/type
    pData->trendState = TR_TYPE_INACTIVE;

    // Clear Action
    pData->nActionReqNum = ActionRequest(pData->nActionReqNum, pCfg->nAction,
                                           ACTION_OFF, FALSE, FALSE);

    GSE_DebugStr(NORMAL,TRUE,
                 "Trend[%d]: Logged. AutoTrendCnt: %d TrendCnt: %d, StableState: %d",
                 pData->trendIndex,
                 pData->autoTrendCnt,
                 pData->trendCnt,
                 pData->stableState);

    // For commanded trend,store the time the trend was stable as a baseline for
    // identifying unstable sensors
    pData->cmdStabilityDurMs = pData->nTimeStableMs;

    // Reset fields for the next sample period.

    // Reset sample count.
    pData->masterSampleCnt  = 0;

    // Reset sensor stability timer
    pData->lastStabCheckMs  = 0;
    pData->nTimeStableMs    = 0;

    // Reset interval timer
    pData->timeSinceLastTrendMs  = 0;
    pData->lastIntervalCheckMs   = 0;

  } // End of finishing up a log for trend in progress.
}


/******************************************************************************
 * Function: TrendReset
 *
 * Description: Reset the processing state data for the passed Trend.
 *
 * Parameters:   [in]     pCfg  Pointer to trend  config data
 *               [in/out] pData Pointer to trend runtime data
 *               [in]     bRunTime flag to signal whether this func is being called during
 *                        init or runtime. Used to set first trend-interval
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static void TrendReset( TREND_CFG* pCfg, TREND_DATA* pData, BOOLEAN bRunTime )
{
  UINT8 i;
  ABS_STABILITY* pAbsVar;
  #pragma ghs nowarning 1545 //Suppress packed structure alignment warning

  pData->trendState            = TR_TYPE_INACTIVE;
  pData->bResetInProg          = FALSE;
  pData->bTrendStabilityFailed = FALSE;

  pData->nTimeStableMs   = 0;
  pData->lastStabCheckMs = 0;

  // If called at start of an ENGINE RUN, set the interval to appear as though
  // the interval has elapsed so there will be no wait before beginning the
  // initial stability check
  if (bRunTime)
  {
    pData->timeSinceLastTrendMs = pCfg->trendInterval_s * ONE_SEC_IN_MILLSECS;
    pData->lastIntervalCheckMs  = CM_GetTickCount();
  }
  else
  {
    pData->timeSinceLastTrendMs  = 0;
    pData->lastIntervalCheckMs   = 0;
  }

  // Reset the stability and max-stability history
  // If in-flight engine-restart (i.e. no reset-trigger), this info will be cleared!
  pData->curStability.stableCnt = 0;
  pData->maxStability.stableCnt = 0;

  for ( i = 0; i < MAX_STAB_SENSORS; i++)
  {
    pData->curStability.snsrValue[i] = FLT_MAX;
    pData->curStability.validity[i]  = FALSE;
    pData->curStability.status[i]    = STAB_UNKNOWN;
    pData->curStability.lastStableChkMs[i] = 0;
    pData->curStability.stableDurMs[i]     = 0;
  }
  // Init the maxStability observed with the initialized contents of curStability
  memcpy(&pData->maxStability, &pData->curStability, sizeof(pData->maxStability) );

  // Reset the sensor statistics
  SensorResetSummaryArray (pData->snsrSummary, pData->nTotalSensors);

  if ( -1 != pCfg->nOffsetStability_s )
  {
    SensorResetSummaryArray (pData->endSummary, pData->nTotalSensors);
  }

  // Initialize the contents of the run-time structure when using STB_STRGY_ABSOLUTE
  for (i = 0; (STB_STRGY_ABSOLUTE == pCfg->stableStrategy) && i < MAX_STAB_SENSORS; ++i)
  {
    pAbsVar = &pData->absoluteStability[i];

    pAbsVar->lowerValue = 0.f;
    pAbsVar->upperValue = 0.f;
    pAbsVar->outLierCnt = 0;
    pAbsVar->outLierMax =
       (UINT16) (pCfg->rate * pCfg->stability[i].absOutlier_ms) / ONE_SEC_IN_MILLSECS;
  }

  pData->delayedStbStartMs = 0;
  pData->lastSampleResult  = SMPL_RSLT_UNK;

}


/******************************************************************************
 * Function: TrendEnableTask
 *
 * Description: Wrapper function for enabling/disabling the Trend task.
 *
 * Parameters:     [in] Trend_Task - id of the task to be enabled/disabled
 *                 [in] bEnable - TRUE  - Enable the Trend task.
 *                                FALSE - Disable the Trend task.
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static void TrendEnableTask(BOOLEAN bEnable)
{
   TmTaskEnable (Trend_Task, bEnable);
}

/******************************************************************************
 * Function: TrendCheckStability
 *
 * Description: Check whether the configured stability sensors have met their criteria
 *
 *
 * Parameters:   [in]     pCfg  Pointer to trend  config data
 *               [in/out] pData Pointer to trend runtime data
 *               [in]     timeNow current time, used for Abs stability snsr stability timing
 *
 * Returns:      If all sensors are stable then TRUE otherwise FALSE
 *
 *
 * Notes:        This is a common routine for all stability-monitoring
 *               trends types:  STANDARD_AUTOTREND, COMMANDED_TREND
 *
 *****************************************************************************/
static BOOLEAN TrendCheckStability( TREND_CFG* pCfg, TREND_DATA* pData, UINT32 timeNow )
{
  INT32   i;
  BOOLEAN bChkVarFlag;
  BOOLEAN bSnsrStable;
  BOOLEAN bIsUsed;
  BOOLEAN bIsValid;
  STABILITY_CRITERIA* pStabCrit;
  UINT32* pStableDurMs;
  UINT32* pDurLastCheckMs;

  ASSERT(NULL != pData->pStabilityFunc);

  // Reset the count of stable sensors.
  pData->curStability.stableCnt = 0;

  // Check all stability sensors for this trend obj.
  // If no stability configured, we never perform this loop

  for (i = 0; i < pData->nStabExpectedCnt; ++i)
  {
    pStabCrit = &pCfg->stability[i];

    pStableDurMs    = &pData->curStability.stableDurMs[i];
    pDurLastCheckMs = &pData->curStability.lastStableChkMs[i];

    // Default the current stability sensor to GOOD, modify as needed.
    pData->curStability.status[i] = STAB_OK;

    // Get the is-defined and validity state of this sensor
    bIsValid = SensorIsUsedEx(pStabCrit->sensorIndex, &bIsUsed);

    if ( bIsUsed )
    {
      if ( bIsValid )
      {
        // If this sensor's stability criteria is configured for delayed-sampling, don't verify
        // variance until in the SAMPLE stable-state AND the offset-delay period has elapsed.
        if( TRUE == pStabCrit->bDelayed )
        {
          bChkVarFlag = ((pData->stableState == STB_STATE_SAMPLE) &&
                         ( CM_GetTickCount() >= pData->delayedStbStartMs))
                         ? TRUE : FALSE;
        }
        else // Non-delayed sensors are always checked for stability
        {
          bChkVarFlag = TRUE;
        }

        // Use the configured stability-strategy function to
        // determine the stability of the sensor at table index 'i',
        //************************************************
        bSnsrStable = (pData->pStabilityFunc(pCfg, pData, i, bChkVarFlag));
        //************************************************

        // Maintain the continuous time this snsr has been stable
        if(bSnsrStable)
        {
          // Update total times this sensor has been stable since last reset.
          TrendUpdateTimeElapsed(timeNow, pDurLastCheckMs, pStableDurMs);
        }
        else
        {
          // On failing stability, reset the continuous time.
          *pStableDurMs   = 0;
          pDurLastCheckMs = 0;
        }
      }
      else
      {
        pData->curStability.status[i]   = STAB_SNSR_INVALID;
        pData->curStability.validity[i] = FALSE;
      }
    }
    else
    {
      pData->curStability.status[i] = STAB_SNSR_NOT_DECLARED;
    }
  }

  // track our max stable-sensor count data
  // If this is the max(greatest # stab sensors) observed during this trend, store it for
  // potential TREND_NOT_DETECTED log.
  // If no sensors are stable, maintain the latest results.
  if (pData->curStability.stableCnt > pData->maxStability.stableCnt ||
      0 == pData->maxStability.stableCnt)
  {
    memcpy(&pData->maxStability, &pData->curStability, sizeof(STABILITY_DATA));
    //GSE_DebugStr(verboseSetting, TRUE, "Trend[%d] Updated MaxStability. Count %d\r\n",
    //                 pData->trendIndex,
    //                 pData->maxStability.stableCnt);
  }

  // if all configured(expected) sensors are stable, return true
  return (pData->curStability.stableCnt == pData->nStabExpectedCnt);
}


/******************************************************************************
 * Function: TrendCheckResetTrigger
 *
 * Description: Check if conditions exists for handling the ResetTrigger
 *
 * Parameters:   [in]     pCfg  Pointer to trend  config data
 *               [in/out] pData Pointer to trend runtime data
 *
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static void TrendCheckResetTrigger( TREND_CFG* pCfg, TREND_DATA* pData )
{
  // Each Trend shall record a "Trend not Detected" log when:
  //   the reset trigger becomes active,
  //   the trend processing was ENABLED and
  //   a trend has not been taken.

  BOOLEAN bResetTrigState = RESET_TRIGGER_ACTIVE( pCfg->resetTrigger );

  if(!pData->bEnabled && !bResetTrigState)
  {
    pData->bEnabled = TRUE;
  }

  // Only handle the reset-event upon initial detected.
  if ( !pData->bResetInProg && bResetTrigState  )
  {
    // Flag the Reset as detected so we don't do this again until the next new reset.
    pData->bResetInProg = TRUE;

    // If, no auto trends taken and AutoTrend criteria are defined, log it.
    if ( pData->bEnabled          &&
         0 == pData->autoTrendCnt &&
         SENSOR_UNUSED !=  pCfg->stability[0].sensorIndex )
    {
      TrendWriteErrorLog( APP_ID_TREND_NOT_DETECTED,
                          pData->trendIndex,
                          pCfg->stability,
                          &pData->maxStability);
    }

    // Reset max autotrend count.
    pData->autoTrendCnt = 0;
  }
  else if(pData->bResetInProg && !bResetTrigState)
  {
    // If trend-reset trigger had been active and is now inactive,
    // clear the flag for next use.
    pData->bResetInProg = FALSE;
  }
}


/******************************************************************************
 * Function: TrendWriteErrorLog
 *
 * Description: Create an error log using the passed parameters
 *
 * Parameters:   [in] logID      identifier of log type
 *               [in] trendIdx   object index of trend being reported
 *               [in] stabCrit   configured stability criteria of trend being reported
 *               [in] stabData   the state of the sensors being monitored.
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static void TrendWriteErrorLog(SYS_APP_ID logID, TREND_INDEX trendIdx,
                               STABILITY_CRITERIA stabCrit[], STABILITY_DATA* stabData)
{
  UINT8 i;
  TREND_ERROR_LOG trendLog;

  // copy the trend stability data to the log
  trendLog.data.stableCnt = stabData->stableCnt;
  for ( i = 0; i < MAX_STAB_SENSORS; ++i)
  {
    // copy the subset of stability-criteria fields to the log structure
    trendLog.crit[i].sensorIndex = stabCrit[i].sensorIndex;
    memcpy(&trendLog.crit[i].criteria,
           &stabCrit[i].criteria,
           sizeof(trendLog.crit[i].criteria));

    // copy the trend stability-data to the log
    trendLog.data.status[i] = stabData->status[i];
    trendLog.data.validity[i] = stabData->validity[i];

    // If the run time data indicates a variance error,
    // the sensor value will contain the recorded variance
    trendLog.data.snsrValue[i] = (stabData->status[i] == STAB_SNSR_VARIANCE_ERROR)
                                  ? stabData->snsrVar[i]
                                  : stabData->snsrValue[i];
  }

  trendLog.trendIndex = trendIdx;
  LogWriteETM(logID, LOG_PRIORITY_3, &trendLog, sizeof(trendLog), NULL);

  GSE_DebugStr(NORMAL,TRUE, "Trend[%d]: %s Logged.\r\n", trendIdx,
                                                     logID == APP_ID_TREND_NOT_DETECTED ?
                                                     "Trend Not-Detected" :
                                                     "Trend Failure");
}

/******************************************************************************
 * Function: TrendCheckAutoTrendStability
 *
 * Description:  Check to see if Auto trend stability criteria has been met
 *
 * Parameters:   [in]     pCfg  Pointer to trend  config data
 *               [in/out] pData Pointer to trend runtime data
 *
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static void TrendCheckAutoTrendStability( TREND_CFG* pCfg, TREND_DATA* pData )
{
  UINT32 timeNow;

  timeNow = CM_GetTickCount();

  // Is this trend configured with stability-check sensors AND
  // can more trend-samples be taken(stability: READY) OR
  // is an AutoTrend in progress(stability: SAMPLE)?
  if ( pData->nStabExpectedCnt > 0 &&
       ( pData->autoTrendCnt < pCfg->maxTrends ||
         TR_TYPE_STANDARD_AUTO == pData->trendState) )
  {

    // Update minimum time between trends
    TrendUpdateTimeElapsed(timeNow,
                           &pData->lastIntervalCheckMs,
                           &pData->timeSinceLastTrendMs);
    #ifdef TREND_DEBUG
    /*vcast_dont_instrument_start*/
    GSE_DebugStr(DBGOFF, TRUE,"Trend[%d]: Trend Interval: %d of %d\r\n",
                 pData->trendIndex,
                 pData->timeSinceLastTrendMs,
                 pCfg->trendInterval_s * ONE_SEC_IN_MILLSECS);
    /*vcast_dont_instrument_end*/
    #endif

    // Check if the configured criteria reached/maintained stable.
    if (TrendCheckStability(pCfg, pData, timeNow ) )
    {
      // Criteria is stable... update stability time-elapsed.
      TrendUpdateTimeElapsed(timeNow, &pData->lastStabCheckMs, &pData->nTimeStableMs);

      //GSE_DebugStr(verboseSetting,TRUE,
      //"Trend[%d]: AT Snsrs stable for: %dms required: %dms Intrval: %d required: %d\r\n",
      //                         pData->trendIndex,
      //                         pData->nTimeStableMs,
      //                         pCfg->stabilityPeriod_s * ONE_SEC_IN_MILLSECS,
      //                         pData->timeSinceLastTrendMs,
      //                         (pCfg->trendInterval_s  * ONE_SEC_IN_MILLSECS ));

      // If both the stability and interval-durations have been met AND not already processing
      // trend, start one
      if ((TR_TYPE_INACTIVE == pData->trendState)                                          &&
          (pData->nTimeStableMs        >= (pCfg->stabilityPeriod_s * ONE_SEC_IN_MILLSECS)) &&
          (pData->timeSinceLastTrendMs >= (pCfg->trendInterval_s   * ONE_SEC_IN_MILLSECS)) )
      {
        TrendStartAutoTrend(pCfg, pData);
      }
    }
    else //Criteria sensors didn't reach stability during READY OR.. SAMPLE auto-trend unstable
    {
      // Reset the stability timing for next stability timing period.
      pData->lastStabCheckMs = 0;

      // If active auto-trend was in progress, flag it to be processed in TrendUpdateData()
      if (TR_TYPE_STANDARD_AUTO == pData->trendState)
      {
        pData->bTrendStabilityFailed = TRUE;
      }
    }
  } // Trend is configured for AT/MANUAL and more trend-samples can be taken.
}

/******************************************************************************
 * Function: TrendUpdateTimeElapsed
 *
 * Description: Generalized routine to update an elapsed time
 *
 * Parameters:  [in]     currentTime    - time now.
 *              [in/out] lastTimeCalled - the time this was last-called.
 *              [in/out] elapsedTime    - the time elapsed between lastTimeCalled
 *                                        and now.
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
 static void TrendUpdateTimeElapsed(UINT32 currentTime, UINT32* lastTimeCalled,
                                    UINT32* elapsedTime)
 {
   // If first time thru, set last time to current so elapsed will be zero.
   if (0 == *lastTimeCalled)
   {
     *lastTimeCalled  = currentTime;
     *elapsedTime     = 0;
   }

   // Update the elapsed time & record
   *elapsedTime     += (currentTime - *lastTimeCalled);
   *lastTimeCalled  =  currentTime;
 }

/******************************************************************************
 * Function: TrendSetStabilityState
 *
 * Description: Update the Stability State of the trend and the time entered.
 *
 * Parameters:  [in] pData     - the TREND_DATA for this trend
 *              [in] stbState - the new STABILITY_STATE
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
 static void TrendSetStabilityState(TREND_DATA* pData, STABILITY_STATE stbState)
 {
   ASSERT(stbState < MAX_STB_STATE);

   // The current stable state is stored as the prevStableState
   // so TrendProcessCommanded function can detect transitions
   // and perform on-entry steps.
   // It will then set prevStableState to match stableState.

   pData->prevStableState = pData->stableState;

   pData->stableState     = stbState;
   pData->stableStateStartMs = CM_GetTickCount();
 }

/******************************************************************************
 * Function: TrendStartAutoTrend
 *
 * Description: Generalized routine to update time elapsed.
 *
 * Parameters:   [in]     pCfg  Pointer to trend  config data
 *               [in/out] pData Pointer to trend runtime data
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static void TrendStartAutoTrend(TREND_CFG* pCfg, TREND_DATA* pData)
{
  // Local Data
  TREND_START_LOG trendLog;

  pData->trendState = TR_TYPE_STANDARD_AUTO;
  TrendSetStabilityState(pData, STB_STATE_SAMPLE);
  pData->lastSampleResult = SMPL_RSLT_UNK;

  // Increment the stability-trend count and overall trend-count.
  pData->autoTrendCnt++;
  pData->trendCnt++;

  // The sensor statistics shall be reset each time the trend becomes ACTIVE.
  SensorResetSummaryArray (pData->snsrSummary, pData->nTotalSensors);

  // If end-phase params are configured, reset them at start of sample too.
  if ( -1 != pCfg->nOffsetStability_s )
  {
    SensorResetSummaryArray( pData->endSummary, pData->nTotalSensors);
    TrendSetDelayStabilityStartTime(pCfg, pData);
  }

  // Create Trend Start log
  trendLog.trendIndex = pData->trendIndex;
  trendLog.type       = pData->trendState;

  LogWriteETM(APP_ID_TREND_START,
              LOG_PRIORITY_3,
              &trendLog,
              sizeof(trendLog),
              NULL);

  pData->nActionReqNum = ActionRequest(pData->nActionReqNum, pCfg->nAction,
                                         ACTION_ON, FALSE, FALSE);
  #ifdef TREND_DEBUG
  /*vcast_dont_instrument_start*/
  GSE_DebugStr( DBGOFF, TRUE,
  "Trend[%d]: Auto-Trend started TimeStable: %dms, TimeSinceLastMs: %dms \r\n",
                           pData->trendIndex,
                           pData->nTimeStableMs,
                           pData->timeSinceLastTrendMs );
  /*vcast_dont_instrument_end*/
  #endif
}

/******************************************************************************
 * Function: TrendProcessCommanded
 *
 * Description: Commanded processing function for Trend. Called by TrendTask.
 *
 * Parameters:   [in]     pCfg  Pointer to trend  config data
 *               [in/out] pData Pointer to trend runtime data
 *
 * Returns:      None
 *
 * Notes:        This function is called for Trends which are started/stopped
 *               as a result of calls to the TrendAppStartTrend public function.
 *
 *****************************************************************************/
static void TrendProcessCommanded( TREND_CFG* pCfg, TREND_DATA* pData )
{
  /*
  const char* stableStates[] = {"IDLE", "READY", "SAMPLE"};
  if (pData->prevStableState != pData->stableState )
  {
    GSE_DebugStr(verboseSetting, TRUE, "Trend[%d]: Stability change %s -> %s\r\n",
                               pData->trendIndex,
                               stableStates[pData->prevStableState],
                               stableStates[pData->stableState] );
  }*/

  // Always check & handle potential Trend Reset Trigger.
  TrendCheckResetTrigger(pCfg, pData);

  switch ( pData->stableState )
  {
    case STB_STATE_IDLE:
      if (APAC_CMD_START == pData->apacCmd)
      {
        TrendSetStabilityState(pData, STB_STATE_READY);
        GSE_DebugStr(NORMAL, TRUE,
                     "Trend[%d]: START Command Processed.\r\n", pData->trendIndex);
        pData->apacCmd = APAC_CMD_COMPL;
      }
      // Nothing to do here except wait for command

      break;

    case STB_STATE_READY:
      if (APAC_CMD_STOP == pData->apacCmd)
      {
        pData->cmdStabilityDurMs = pData->nTimeStableMs;
        TrendWriteErrorLog( APP_ID_TREND_NOT_DETECTED,
                            pData->trendIndex,
                            pCfg->stability,
                            &pData->maxStability);

        TrendSetStabilityState(pData, STB_STATE_IDLE);
        GSE_DebugStr(NORMAL, TRUE,
                     "Trend[%d]: STOP Command Recvd while READY.\r\n", pData->trendIndex);
        pData->apacCmd = APAC_CMD_COMPL;
      }
      else if (STB_STATE_IDLE == pData->prevStableState)
      {
        // On Transition: IDLE => READY: (Commanded to START monitoring)
        TrendReset(pCfg, pData,TRUE);

        // clear sensor-stable counts.
        TrendClearSensorStabilityHistory(pData);

        pData->prevStableState = STB_STATE_READY;
      }

      // Check if stability has been achieved/maintained
      TrendCheckCmdedTrendStability(pCfg, pData);

      // Call TrendUpdateData to handle stability failure or in
      // case we just went active (SAMPLE)
      TrendUpdateData(pCfg, pData);
      break;

    case STB_STATE_SAMPLE:
      if (APAC_CMD_STOP == pData->apacCmd)
      {
        pData->cmdStabilityDurMs = pData->nTimeStableMs;
        // Commanded to stop during sample.... log "trend not detected"
        TrendWriteErrorLog( APP_ID_TREND_NOT_DETECTED,
                            pData->trendIndex,
                            pCfg->stability,
                            &pData->maxStability);

        TrendSetStabilityState(pData, STB_STATE_IDLE);

        GSE_DebugStr(NORMAL, TRUE, "Trend[%d]: STOP Command Recvd while SAMPLE.\r\n",
                     pData->trendIndex);
        pData->apacCmd = APAC_CMD_COMPL;

      }
      else if (STB_STATE_SAMPLE != pData->prevStableState)
      {
        // READY => Stability passed => SAMPLE
        pData->prevStableState = STB_STATE_SAMPLE;
        // entry activities? should have been handled.
      }

      // Verify stability is being maintained
      TrendCheckCmdedTrendStability(pCfg, pData);
      // Handle stability failure or update samples
      TrendUpdateData(pCfg, pData);
      break;

    default:
      FATAL("Unsupported STABILITY_STATE for Commanded Trend = %d", pData->stableState );
  }
}

/******************************************************************************
 * Function: TrendCheckCmdedTrendStability
 *
 * Description:  Check to see if commanded trend stability criteria has been
 *               achieved during monitoring or maintained while active.
 *
 * Parameters:   [in]     pCfg  Pointer to trend  config data
 *               [in/out] pData Pointer to trend runtime data
 *
 *
 * Returns:      TRUE if stability criteria and duration achieved otherwise
 *               FALSE
 *
 * Notes:
 *
 *****************************************************************************/
static void TrendCheckCmdedTrendStability( TREND_CFG* pCfg, TREND_DATA* pData )
{
  UINT32 timeNow = CM_GetTickCount();

  // Is this trend set up as Commanded
  ASSERT(TRUE == pCfg->bCommanded);

  // APAC has started it(READY) or already in the run state (SAMPLE)
  // is an AutoTrend in progress(stability: SAMPLE)?
  if ( pData->nStabExpectedCnt > 0 &&
     ( STB_STATE_READY == pData->stableState || STB_STATE_SAMPLE == pData->stableState ) )
  {
    // Check if the configured criteria reached/ is maintaining stable.
    if (TrendCheckStability(pCfg, pData, timeNow) )
    {
      // Criteria is stable... update stability time-elapsed.
      TrendUpdateTimeElapsed(timeNow, &pData->lastStabCheckMs, &pData->nTimeStableMs);
      #ifdef TREND_DEBUG
      /*vcast_dont_instrument_start*/
      if (STB_STATE_READY == pData->stableState)
      {
        GSE_DebugStr(DBGOFF, TRUE,
          "Trend[%d]: CMD Trend stable: %d of %d mSec during READY\r\n",
          pData->trendIndex,
          pData->nTimeStableMs,
          pCfg->stabilityPeriod_s * ONE_SEC_IN_MILLSECS);
      }
      else
      {
        GSE_DebugStr(DBGOFF, TRUE,
          "Trend[%d]: CMD Trend stable: %d of %d mSec during SAMPLE\r\n",
          pData->trendIndex,
          pData->nTimeStableMs - (pCfg->stabilityPeriod_s * ONE_SEC_IN_MILLSECS),
          pCfg->nSamplePeriod_s * ONE_SEC_IN_MILLSECS);
      }
      /*vcast_dont_instrument_end*/
      #endif

      // If the stability has been met AND not already processing active trend, start one
      if ((TR_TYPE_INACTIVE == pData->trendState) &&
          (pData->nTimeStableMs >= (pCfg->stabilityPeriod_s * ONE_SEC_IN_MILLSECS)) )
      {
        TrendStartCommandedTrend(pCfg, pData);
      }
    }
    else // Criteria sensors haven't reach stability OR active trend went unstable
    {
      // Reset the stability timing for next stability timing period.
      pData->lastStabCheckMs = 0;

      // If active cmd'ed-trend was in progress, flag it to be processed in TrendUpdateData()
      if (TR_TYPE_COMMANDED == pData->trendState)
      {
        pData->bTrendStabilityFailed = TRUE;
      }
    }
  } // Trend is configured to be commanded and APAC has activated the trend.
}

/******************************************************************************
 * Function: TrendStartCommandedTrend
 *
 * Description: Initiate a commanded trend in response to application call.
 *              The trend will be sampled for the
 *              duration of TREND_CFG.nSamplePeriod_s
 *
 * Parameters:   [in]     pCfg  Pointer to trend  config data
 *               [in/out] pData Pointer to trend runtime data
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static void TrendStartCommandedTrend(TREND_CFG* pCfg, TREND_DATA* pData )
{
  // Local Data
  TREND_START_LOG trendLog;

  if ( TR_TYPE_INACTIVE == pData->trendState )
  {
    // Set the trend type/state based on
    pData->trendCnt++;
    pData->lastSampleResult = SMPL_RSLT_UNK;
    pData->trendState       = TR_TYPE_COMMANDED;

    TrendSetStabilityState(pData, STB_STATE_SAMPLE);

    // The sensor statistics shall be reset each time the trend becomes ACTIVE.
    SensorResetSummaryArray(pData->snsrSummary, pData->nTotalSensors);

    // If end-phase params are configured, reset them at start of sample too.
    if (-1 != pCfg->nOffsetStability_s )
    {
      SensorResetSummaryArray( pData->endSummary, pData->nTotalSensors);
      TrendSetDelayStabilityStartTime(pCfg, pData);
    }

    // Activate Action
    pData->nActionReqNum = ActionRequest(pData->nActionReqNum, pCfg->nAction,
                                           ACTION_ON, FALSE, FALSE);

    // Create TREND_START log
    trendLog.trendIndex = pData->trendIndex;
    trendLog.type       = pData->trendState;

    LogWriteETM( APP_ID_TREND_START, LOG_PRIORITY_3, &trendLog, sizeof(trendLog), NULL );

    GSE_DebugStr(NORMAL, TRUE,
                 "Trend[%d]: Commanded Trend started.\r\n", pData->trendIndex );
  }
}

/******************************************************************************
 * Function: TrendMonStabilityByIncrement
 *
 * Description: Check whether the configured stability sensor has have
 *              met/maintaining their criteria using the simple incremental
 *              variance check
 *
 * Parameters:   [in]     pCfg  Pointer to trend  config data
 *               [in/out] pData Pointer to trend runtime data
 *               [in]     tblIndex the index in STABILITY_CRITERIA table to be
 *                        processed.
 *               [in]     bDoVarCheck - boolean value to indicate that this test
 *                        should verify the sensor value is within variance during
 *                        this call. This setting is used to implement delayed
 *                        sensor sampling.
 *
 * Returns:      None
 *
 *
 * Notes:        This func is called via TREND_DATA.pStabilityFunc variable set
 *               in TrendInitialize()
 *
 *****************************************************************************/
static BOOLEAN TrendMonStabilityByIncrement( TREND_CFG* pCfg, TREND_DATA* pData,
                                          INT32 tblIndex, BOOLEAN bDoVarCheck)
{
  FLOAT32 fVal;
  FLOAT32 fPrevValue;
  FLOAT32 delta;

  // Get a pointer to the Stability Criteria for this stability sensor
  STABILITY_CRITERIA* pStabCrit = &pCfg->stability[tblIndex];

  // get the current sensor value
  fVal       = SensorGetValue(pStabCrit->sensorIndex);
  fPrevValue = pData->curStability.snsrValue[tblIndex];

  // save the current value and validity
  // validity was checked as a pre-condition of calling this func.
  pData->curStability.snsrValue[tblIndex] = fVal;
  pData->curStability.validity[tblIndex] = TRUE;

  // Always check fixed lower/upper values
  if ( (fVal >= pStabCrit->criteria.lower) && (fVal <= pStabCrit->criteria.upper) )
  {
    // Check the stability of the sensor value within a given variance
    // ensure positive difference, compute delta and adjust for processing rate
    if (bDoVarCheck)
    {
      delta = ( fPrevValue > fVal) ? fPrevValue - fVal :  fVal - fPrevValue;
      delta *= (FLOAT32)pCfg->rate;
      if ( delta > pStabCrit->criteria.variance)
      {
        pData->curStability.status[tblIndex] = STAB_SNSR_VARIANCE_ERROR;
        pData->curStability.snsrVar[tblIndex] = delta;
      }
    }
  }
  else // Sensor has failed the upper/lower max checks.
  {
    pData->curStability.status[tblIndex] = STAB_SNSR_RANGE_ERROR;
  }

  // If the sensor is in stable range and variance... increment the detail count
  // for this sensor and the overall count of sensors
  if (STAB_OK == pData->curStability.status[tblIndex] )
  {
    ++pData->curStability.stableCnt;
  }

  return (STAB_OK == pData->curStability.status[tblIndex]);
}

/******************************************************************************
 * Function: TrendMonStabilityAbsolute
 *
 * Description: Check whether the configured stability sensor has
 *              met/maintaining their criteria by verifying the value is within
 *              the absolute variance limits of the reference sensor value being
 *              used as first sample of the input.
 *
 * Parameters:   [in]     pCfg  Pointer to trend  config data
 *               [in/out] pData Pointer to trend runtime data
 *               [in]     tblIndex the index in STABILITY_CRITERIA table to be
 *                        processed.
 *               [in]     bCheckVariance boolean value to indicate that this test
 *                        should verify the sensor value is within variance during
 *                        this call. This setting is used to implement delayed
 *                        sensor sampling.
 *               [in]     bDoVarCheck signals whether the variance tolerance should be
 *                        checked during this call. Used to implement delayed variance
 *                        testing.
 *
 * Returns:     None
 *
 * Notes:        This func is called via TREND_DATA.pStabilityFunc variable set
 *               in TrendInitialize()
 *
 *               The upper and lower stability limits are deliberately not
 *               factored into the calc for the ref limit range because a separate
 *               status STAB_SNSR_RANGE_ERROR is tested and logged for this condition.
 *
 *
 *
 *****************************************************************************/
static BOOLEAN TrendMonStabilityAbsolute( TREND_CFG* pCfg, TREND_DATA* pData,
                                          INT32 tblIndex, BOOLEAN bDoVarCheck)
{
  FLOAT32 fVal;
  FLOAT32 delta;
  FLOAT32 fRefValue;
  FLOAT32* pLowerRefLimit;
  FLOAT32* pUpperRefLimit;
  UINT16*  pOutLierCnt;

  // Get a pointer to the Stability Criteria for this stability sensor
  STABILITY_CRITERIA* pStabCrit = &pCfg->stability[tblIndex];
  ABS_STABILITY*      pAbsVar   = &pData->absoluteStability[tblIndex];

  pLowerRefLimit = &pAbsVar->lowerValue;
  pUpperRefLimit = &pAbsVar->upperValue;
  pOutLierCnt    = &pAbsVar->outLierCnt;

  // get the current sensor value
  fVal = SensorGetValue(pStabCrit->sensorIndex);

  // Check if a ref value has been established.
  // If not, current value will be used.
  // Only do this when it is OK to test variance
  if(bDoVarCheck && 0.f == *pLowerRefLimit && 0.f == *pUpperRefLimit)
  {
    *pLowerRefLimit = fVal - pStabCrit->criteria.variance;
    *pUpperRefLimit = fVal + pStabCrit->criteria.variance;
    *pOutLierCnt    = 0;
#ifdef TREND_DEBUG
/*vcast_dont_instrument_start*/
    GSE_DebugStr(DBGOFF,TRUE,
   "Trend[%d]: Sensor[%d]: Initialized values Rng: %8.4f<-|%8.4f|->%8.4f Outlier max: %d\r\n",
                                pData->trendIndex,
                                pStabCrit->sensorIndex,
                                *pLowerRefLimit,
                                fVal,
                                *pUpperRefLimit,
                                pAbsVar->outLierMax);
/*vcast_dont_instrument_end*/
#endif
  }

  // save the current value and init the validity and stability
  // Validity was checked as a pre-condition of calling this func.
  pData->curStability.snsrValue[tblIndex] = fVal;
  pData->curStability.validity[tblIndex]  = TRUE;

  // Always check fixed lower/upper values
  if ( (fVal >= pStabCrit->criteria.lower) && (fVal <= pStabCrit->criteria.upper) )
  {
    // If caller indicates that variance can be checked, do it, otherwise bypass for
    // now and flag the sensor as 'stable' so we don't cause the stability/sampling to fail.
    if (bDoVarCheck)
    {
      // Check if the stability of the sensor value is exceeding the ref-point variance.
      if ((fVal > *pUpperRefLimit || fVal < *pLowerRefLimit))
      {
        if ( *pOutLierCnt < pAbsVar->outLierMax )
        {
          *pOutLierCnt = *pOutLierCnt + 1;
          #ifdef TREND_DEBUG
          /*vcast_dont_instrument_start*/
          GSE_DebugStr(DBGOFF, TRUE,
              "Trend[%d]: Sensor[%d] value %8.4f exceeds var. Outlier: %d-of-%d \r\n",
                         pData->trendIndex, pStabCrit->sensorIndex, fVal, *pOutLierCnt,
                         pAbsVar->outLierMax);
            /*vcast_dont_instrument_end*/
            #endif
        }
        else
        {
          // Exceeded the variance limits of ref-point. Make this point the ref point.
          // Compute the variance delta for updating the stability table
          fRefValue = (*pUpperRefLimit - *pLowerRefLimit)/2.f;
          delta = ( fRefValue > fVal) ? fRefValue - fVal :  fVal - fRefValue;
          delta *= (FLOAT32)pCfg->rate;

          pData->curStability.status[tblIndex]  = STAB_SNSR_VARIANCE_ERROR;
          pData->curStability.snsrVar[tblIndex] = delta;

          *pLowerRefLimit = fVal - pStabCrit->criteria.variance;
          *pUpperRefLimit = fVal + pStabCrit->criteria.variance;
          *pOutLierCnt = 0;
          #ifdef TREND_DEBUG
          /*vcast_dont_instrument_start*/
          GSE_DebugStr(DBGOFF, TRUE,
         "Trend[%d]: Sensor[%d] %6.4f exceeds var and outlier max(%d). New Var: %8.4f->%8.4f",
                                   pData->trendIndex,
                                   pStabCrit->sensorIndex,
                                   fVal,
                                   pAbsVar->outLierMax,
                                   *pLowerRefLimit,
                                   *pUpperRefLimit);
          /*vcast_dont_instrument_end*/
          #endif
        }
      }
      else // Value within variance, clear spike count
      {
        #ifdef TREND_DEBUG
        /*vcast_dont_instrument_start*/
        if(*pOutLierCnt > 0)
        {
          GSE_DebugStr(DBGOFF, TRUE,
            "Trend[%d]: Sensor[%d] value %8.4f back in var. Outliner cnt reset from %d \r\n",
            pData->trendIndex, pStabCrit->sensorIndex, fVal, *pOutLierCnt);
        }
        /*vcast_dont_instrument_end*/
        #endif

        *pOutLierCnt = 0;
      }
    } // bDoVarCheck

  }
  else // Sensor has failed the upper/lower max checks.
  {
    pData->curStability.status[tblIndex] = STAB_SNSR_RANGE_ERROR;
  }

  // if sensor is (still) in stable range and variance... increment the count
  if (STAB_OK == pData->curStability.status[tblIndex] )
  {
    ++pData->curStability.stableCnt;
  }
  return (STAB_OK == pData->curStability.status[tblIndex]);
}

/******************************************************************************
 * Function: TrendSetDelayStabilityStartTime
 *
 * Description:   Set the time when Stability sensors configured as delayed
 *                should be included in sampling.
 * Parameters:   [in]     pCfg  Pointer to trend  config data
 *               [in/out] pData Pointer to trend runtime data
 *
 * Returns:      None
 *
 *
 * Notes:        Common function for use by specialized TrendStabilityCheckXXXXXX
 *               functions
 *
 *****************************************************************************/
static void TrendSetDelayStabilityStartTime(TREND_CFG* pCfg, TREND_DATA* pData)
{
  ASSERT(pCfg->nOffsetStability_s > -1);
  pData->delayedStbStartMs = CM_GetTickCount() +
                             ( (UINT32)pCfg->nOffsetStability_s * ONE_SEC_IN_MILLSECS);
}


/******************************************************************************
 * Function: TrendClearSensorStabilityHistory
 *
 * Description:   Clears out how many times stability was checked and the totals
 *                for how many times each stability-sensor was in stable range. *
 *
 * Parameters:   [in]     pCfg  Pointer to trend  config data
 *               [in/out] pData Pointer to trend runtime data
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static void TrendClearSensorStabilityHistory(TREND_DATA* pData)
{
  pData->cmdStabilityDurMs = 0;
}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: trend.c $
 * 
 * *****************  Version 52  *****************
 * User: Contractor V&v Date: 4/14/16    Time: 3:03p
 * Updated in $/software/control processor/code/application
 * SCR #1300 - Fixed bug in iterating thru number of active-configured
 * summary sensors.
 * 
 * *****************  Version 51  *****************
 * User: Contractor V&v Date: 3/30/16    Time: 6:18p
 * Updated in $/software/control processor/code/application
 * SCR #1300 - Fixed bug in calculating sensor stability duration
 * 
 * *****************  Version 50  *****************
 * User: Contractor V&v Date: 3/18/16    Time: 5:43p
 * Updated in $/software/control processor/code/application
 * SCR #1300 - Comment changed to indicate TrendGetStability always 
 * returns true
 * 
 * *****************  Version 49  *****************
 * User: Contractor V&v Date: 2/25/16    Time: 12:03p
 * Updated in $/software/control processor/code/application
 * SCR #1300 - trend.debug commands don’t require NORMAL verbosity
 * 
 * *****************  Version 48  *****************
 * User: John Omalley Date: 2/23/16    Time: 10:41a
 * Updated in $/software/control processor/code/application
 * SCR 1299 - Code Review Updates
 * 
 * *****************  Version 47  *****************
 * User: Contractor V&v Date: 2/16/16    Time: 1:22p
 * Updated in $/software/control processor/code/application
 * SCR #1299 - Fix bug on TrendReset was resetting max outliner allowed.
 * 
 * *****************  Version 46  *****************
 * User: Contractor V&v Date: 2/01/16    Time: 5:16p
 * Updated in $/software/control processor/code/application
 * SCR #1192 - Perf Enhancment improvements 
 * 
 * *****************  Version 45  *****************
 * User: Contractor V&v Date: 12/11/15   Time: 5:29p
 * Updated in $/software/control processor/code/application
 * SCR #1300 Trend.debug section and stablie duration changed to all
 * 
 * *****************  Version 44  *****************
 * User: Contractor V&v Date: 12/10/15   Time: 3:49p
 * Updated in $/software/control processor/code/application
 * SCR # 1300 CR Compliance changes.
 * 
 * *****************  Version 43  *****************
 * User: Contractor V&v Date: 12/07/15   Time: 6:06p
 * Updated in $/software/control processor/code/application
 * SCR #1300 CR review fixes for recently enabled debug functions
 * 
 * *****************  Version 42  *****************
 * User: Contractor V&v Date: 12/07/15   Time: 3:10p
 * Updated in $/software/control processor/code/application
 * SCR #1300 - CR compliance update, enabled trend debug cmds
 *
 * *****************  Version 41  *****************
 * User: Contractor V&v Date: 12/03/15   Time: 6:04p
 * Updated in $/software/control processor/code/application
 * SCR #1300 CR compliance changes
 *
 * *****************  Version 40  *****************
 * User: Contractor V&v Date: 11/30/15   Time: 3:34p
 * Updated in $/software/control processor/code/application
 * CodeReview compliance change - no logic changes.
 *
 * *****************  Version 39  *****************
 * User: Contractor V&v Date: 11/17/15   Time: 11:52a
 * Updated in $/software/control processor/code/application
 * SCR #1300 - Added limits to offset for sampling and outliners, misc
 * debug updates
 *
 * *****************  Version 38  *****************
 * User: Contractor V&v Date: 11/09/15   Time: 2:58p
 * Updated in $/software/control processor/code/application
 * Fixed abs stable count off-by-one bug and consolidation struct
 *
 * *****************  Version 37  *****************
 * User: Contractor V&v Date: 11/05/15   Time: 6:38p
 * Updated in $/software/control processor/code/application
 * SCR #1300 - Formal Implementation of variance outlier tolerance
 *
 * *****************  Version 36  *****************
 * User: Contractor V&v Date: 11/02/15   Time: 5:50p
 * Updated in $/software/control processor/code/application
 * SCR #1300 - StabilityHistory and cmd buffering
 *
 * *****************  Version 35  *****************
 * User: Contractor V&v Date: 10/21/15   Time: 7:07p
 * Updated in $/software/control processor/code/application
 * SCR #1300 - StabilityHistory and cmd buffering
 *
 * *****************  Version 34  *****************
 * User: Contractor V&v Date: 10/19/15   Time: 6:26p
 * Updated in $/software/control processor/code/application
 * SCR #1300 - APAC updates for API and stability calc
 *
 * *****************  Version 33  *****************
 * User: Contractor V&v Date: 10/05/15   Time: 6:35p
 * Updated in $/software/control processor/code/application
 * SCR #1300 - P100 Trend Processing Updates initial update
 *
 * *****************  Version 32  *****************
 * User: John Omalley Date: 1/12/15    Time: 2:11p
 * Updated in $/software/control processor/code/application
 * SCR 1257 - Code Review Update
 *
 * *****************  Version 31  *****************
 * User: John Omalley Date: 2/13/14    Time: 10:31a
 * Updated in $/software/control processor/code/application
 * SCR 1257 - Incorrect usage of memset, not initializing data as expected
 *
 * *****************  Version 30  *****************
 * User: Jeff Vahue   Date: 1/30/13    Time: 3:19p
 * Updated in $/software/control processor/code/application
 * SCR# 1214 - remove deadcode
 *
 * *****************  Version 29  *****************
 * User: Jeff Vahue   Date: 1/26/13    Time: 3:12p
 * Updated in $/software/control processor/code/application
 * SCR# 1197 - proper handling of stability sensor variance failure
 * reporting
 *
 * *****************  Version 28  *****************
 * User: Jeff Vahue   Date: 1/24/13    Time: 10:37p
 * Updated in $/software/control processor/code/application
 * SCR# 1197: Fixes to Trend Logs
 *
 * *****************  Version 27  *****************
 * User: Contractor V&v Date: 1/24/13    Time: 9:42p
 * Updated in $/software/control processor/code/application
 * Adding indexing
 *
 * *****************  Version 26  *****************
 * User: Contractor V&v Date: 1/24/13    Time: 7:59p
 * Updated in $/software/control processor/code/application
 * SCR #1197 Max observed stability cnt should also be updated when the
 * cnt is zero to ensure initial data has real values in case no sensor
 * becomes stable.
 *
 * *****************  Version 25  *****************
 * User: John Omalley Date: 13-01-24   Time: 2:07p
 * Updated in $/software/control processor/code/application
 * SCR 1228 - SENSOR not Used issue for log summary
 *
 * *****************  Version 24  *****************
 * User: Contractor V&v Date: 13-01-08   Time: 1:58p
 * Updated in $/software/control processor/code/application
 * SCR #1197  PCLint 731 fix
 *
 * *****************  Version 23  *****************
 * User: Contractor V&v Date: 12/18/12   Time: 3:36p
 * Updated in $/software/control processor/code/application
 * SCR #1107 Fix Trend max trend  count
 *
 * *****************  Version 22  *****************
 * User: Contractor V&v Date: 12/14/12   Time: 5:01p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Code review
 *
 * *****************  Version 21  *****************
 * User: Contractor V&v Date: 12/12/12   Time: 4:30p
 * Updated in $/software/control processor/code/application
 * SCR #1107 Code Review
 *
 * *****************  Version 20  *****************
 * User: John Omalley Date: 12-12-08   Time: 1:34p
 * Updated in $/software/control processor/code/application
 * SCR 1167 - Sensor Summary Init optimization
 *
 * *****************  Version 19  *****************
 * User: Contractor V&v Date: 12/03/12   Time: 5:30p
 * Updated in $/software/control processor/code/application
 * SCR #1200 Requirements: GSE command to view the number of trend
 *
 * *****************  Version 18  *****************
 * User: Contractor V&v Date: 11/29/12   Time: 4:19p
 * Updated in $/software/control processor/code/application
 * SCR #1200 Requirements: GSE command to view the number of trend
 *
 * *****************  Version 17  *****************
 * User: Contractor V&v Date: 11/26/12   Time: 6:07p
 * Updated in $/software/control processor/code/application
 * SCR #1107 Code Review
 *
 * *****************  Version 16  *****************
 * User: Contractor V&v Date: 11/26/12   Time: 12:33p
 * Updated in $/software/control processor/code/application
 * SCR #1167 SensorSummary
 *
 * *****************  Version 15  *****************
 * User: Contractor V&v Date: 11/16/12   Time: 8:11p
 * Updated in $/software/control processor/code/application
 * CodeReview
 *
 * *****************  Version 14  *****************
 * User: Contractor V&v Date: 11/08/12   Time: 4:26p
 * Updated in $/software/control processor/code/application
 * Code Review changes
 *
 * *****************  Version 13  *****************
 * User: Contractor V&v Date: 10/30/12   Time: 4:01p
 * Updated in $/software/control processor/code/application
 *
 * *****************  Version 12  *****************
 * User: John Omalley Date: 12-10-23   Time: 3:04p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Code Review Update
 *
 * *****************  Version 11  *****************
 * User: John Omalley Date: 12-10-23   Time: 2:19p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Updates per Software Design Review
 * Dave
 * 1. Removed Trends is Active Function
 * 2. Fixed bug with deactivating manual trend because no stability
 * JPO
 * 1. Updated logs
 *    a. Combined Auto and Manual Start into one log
 *    b. Change fail log from name to index
 *    c. Trend not detected added index
 * 2. Removed Trend Lamp
 * 3. Updated Configuration defaults
 * 4. Updated user tables per design review
 *
 * *****************  Version 10  *****************
 * User: Melanie Jutras Date: 12-10-19   Time: 2:00p
 * Updated in $/software/control processor/code/application
 * SCR #1172 PCLint 545 Suspicious use of & Error
 *
 * *****************  Version 9  *****************
 * User: Contractor V&v Date: 12-10-02   Time: 1:18p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 Implement Trend Action
 *
 * *****************  Version 8  *****************
 * User: Contractor V&v Date: 12-09-19   Time: 6:48p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2  Reset trigger handling
 *
 * *****************  Version 7  *****************
 * User: Contractor V&v Date: 12-09-19   Time: 3:22p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2  Fix AutoTrends
 *
 * *****************  Version 6  *****************
 * User: Contractor V&v Date: 9/14/12    Time: 4:07p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 Trend code update
 *
 * *****************  Version 5  *****************
 * User: John Omalley Date: 12-09-11   Time: 2:20p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - General Trend Development and Binary ETM Header
 *
 * *****************  Version 4  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 12:43p
 * Updated in $/software/control processor/code/application
 * SCR# 1142
 *
 * *****************  Version 3  *****************
 * User: Contractor V&v Date: 8/15/12    Time: 7:20p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 Checking in prelim trends
 *
 * *****************  Version 2  *****************
 * User: Contractor V&v Date: 7/18/12    Time: 6:25p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2  coding  trend module
 *
 * *****************  Version 1  *****************
 * User: John Omalley Date: 12-06-07   Time: 11:32a
 * Created in $/software/control processor/code/application
 *
 *
 ***************************************************************************/


