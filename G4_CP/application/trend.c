#define TREND_BODY
/******************************************************************************
           Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
              All Rights Reserved. Proprietary and Confidential.

   File:        trend.c

   Description:


   Note:

 VERSION
 $Revision: 17 $  $Date: 11/26/12 6:07p $

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
#define TREND_DEBUG



/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/

static TREND_CFG   m_TrendCfg [MAX_TRENDS];   // Trend cfg
static TREND_DATA  m_TrendData[MAX_TRENDS];   // Runtime trend data
static TREND_LOG   m_TrendLog [MAX_TRENDS];   // Trend Log data for each trend


#include "TrendUserTables.c"

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void TrendProcess         ( TREND_CFG* pCfg, TREND_DATA* pData );

static void TrendFinish          ( TREND_CFG* pCfg, TREND_DATA* pData );


static void TrendStartManualTrend(const TREND_CFG* pCfg, TREND_DATA* pData );
static void TrendStartAutoTrend  (const TREND_CFG* pCfg, TREND_DATA* pData);

static void TrendUpdateData      ( TREND_CFG* pCfg, TREND_DATA* pData );
static void TrendUpdateAutoTrend( TREND_CFG* pCfg, TREND_DATA* pData );

static void TrendCheckResetTrigger( TREND_CFG* pCfg, TREND_DATA* pData );
static BOOLEAN TrendCheckStability( TREND_CFG* pCfg, TREND_DATA* pData);

static void TrendWriteErrorLog(SYS_APP_ID logID, TREND_INDEX trendIdx,
                               STABILITY_CRITERIA stabCrit[], STABILITY_DATA* stabData);

static void TrendReset(TREND_CFG* pCfg, TREND_DATA* pData, BOOLEAN bRunTime);

static void TrendEnableTask(BOOLEAN bEnable);

static void TrendUpdateTimeElapsed( UINT32  currentTime,
                                    UINT32* lastTimeCalled,
                                    UINT32* elapsedTime);


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
  User_AddRootCmd(&RootTrendMsg);

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


    pData->trendIndex      = (ENGRUN_UNUSED == pCfg->engineRunId) ?
                                       TREND_UNUSED : (TREND_INDEX)i;
    pData->prevEngState    = ER_STATE_STOPPED;
    pData->nRateCounts     = (INT16)(MIFs_PER_SECOND / (INT16)pCfg->rate);
    pData->nRateCountdown  = (INT16)((pCfg->nOffset_ms / MIF_PERIOD_mS) + 1);
    pData->bEnabled        = FALSE;
    pData->trendState      = TREND_STATE_INACTIVE;
    pData->nActionReqNum   = ACTION_NO_REQ;

    pData->nSamplesPerPeriod = pCfg->nSamplePeriod_s * (INT16)pCfg->rate;

    // Calculate the expected number of stability sensors based on config
    pData->nStabExpectedCnt = 0;
    for (j = 0; j < MAX_STAB_SENSORS; ++j)
    {
      if (SENSOR_UNUSED != pCfg->stability[j].sensorIndex)
      {
        pData->nStabExpectedCnt++;
      }
    }

    // Use the reset functions to init the others fields
    TrendReset(pCfg, pData, FALSE);
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
 * Parameters:   None
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
void TrendTask( void* pParam )
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

      // Determine if current trend is being used
      if (pData->trendIndex != TREND_UNUSED)
      {
        // Countdown the number of samples until next check
        if (--pData->nRateCountdown <= 0)
        {
          // Calculate the next trigger rate
          pData->nRateCountdown = pData->nRateCounts;
          // Process the trigger
          TrendProcess(pCfg, pData);
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
         ((trendIndex < MAX_TRENDS) && (nRemaining > sizeof (trendHdr[trendIndex])));
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
      trendHdr[trendIndex].StartTrigger  = m_TrendCfg[trendIndex].startTrigger;
      trendHdr[trendIndex].ResetTrigger  = m_TrendCfg[trendIndex].resetTrigger;


      // Increment the total number of bytes and decrement the remaining
      nTotal     += sizeof (trendHdr[trendIndex]);
      nRemaining -= sizeof (trendHdr[trendIndex]);
   }
   // Copy the Trend header to the buffer
   memcpy ( pBuffer, trendHdr, nTotal );
   // Return the total number of bytes written
   return ( nTotal );
}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function: TrendProcess
 *
 * Description: Normal processing function for Trend. Called by TrendTask.
 *
 * Parameters:   None
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static void TrendProcess( TREND_CFG* pCfg, TREND_DATA* pData )
{
  ER_STATE    erState;

  // Get the state of the associated EngineRun
  erState = EngRunGetState(pCfg->engineRunId, NULL);

  // Always check & handle Trend Reset Trigger.
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
      }
      pData->prevEngState = ER_STATE_STOPPED;
      break;

    case ER_STATE_STARTING:
      // If we have entered starting mode from some other mode,
      // finish any outstanding trends
      if (pData->prevEngState != ER_STATE_STARTING)
      {
        TrendFinish(pCfg, pData);
      }
      pData->prevEngState = ER_STATE_STARTING;
      break;


    case ER_STATE_RUNNING:

      // Upon entering RUNNING state, reset trending info from previous run-state.
      if (pData->prevEngState != ER_STATE_RUNNING)
      {
        TrendReset(pCfg, pData,TRUE);
      }

      // Check to see if an Autotrend has met/maintained stability
      TrendUpdateAutoTrend(pCfg, pData);

      // Update current state of active trends/autotrends.
      TrendUpdateData(pCfg, pData);

      // If the button has been pressed(as defined by startTrigger)
      // start an manual trending

      if ( TriggerGetState(pCfg->startTrigger) )
      {
        TrendStartManualTrend(pCfg, pData);
      }

      // set the last engine run state
      // and flag the trend as 'enabled' for subsequent ResetTrigger handling.
      pData->prevEngState = ER_STATE_RUNNING;
      pData->bEnabled     = TRUE;
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
 * Description: Initiate a manual trend in response to button press
 *              The trend will be sampled for the
 *              duration of TREND_CFG.nSamplePeriod_s
 *
 * Parameters:   None
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static void TrendStartManualTrend(const TREND_CFG* pCfg, TREND_DATA* pData )
{
  // Local Data
  TREND_START_LOG trendLog;

  if ( TREND_STATE_INACTIVE  == pData->trendState )
  {
    GSE_DebugStr(NORMAL,TRUE, "Trend[%d]: Manual Trend started.",pData->trendIndex );

    pData->trendState = TREND_STATE_MANUAL;
    pData->trendCnt++;

    // Activate Action
    if( pCfg->nAction)
    {
      pData->nActionReqNum = ActionRequest(pData->nActionReqNum, pCfg->nAction,
                                           ACTION_ON, FALSE, FALSE);
    }

    // Create a Trend-start log.

    trendLog.trendIndex = pData->trendIndex;
    trendLog.type       = pData->trendState;

    LogWriteETM( APP_ID_TREND_START,
                 LOG_PRIORITY_3,
                 &trendLog,
                 sizeof(trendLog),
                 NULL );
  }
}

/******************************************************************************
 * Function: TrendUpdatedData
 *
 * Description:   Update the status of active trend
 *
 * Parameters:   [in]     pCfg  Pointer to trend  config data
 *               [in/out] pData Pointer to trend runtime data
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static void TrendUpdateData( TREND_CFG* pCfg, TREND_DATA* pData  )
{
  BOOLEAN bAutoTrendFailed = FALSE;

  // If this trend is active. Update it's status
  if ( pData->trendState != TREND_STATE_INACTIVE )
  {
    // Check if the preceding call to TrendUpdateAutoTrend flagged an auto-trend
    // as failing to maintain stability
    if( TREND_STATE_AUTO == pData->trendState )
    {
      if( 0 == pData->nTimeStableMs && pData->stableTrendCnt <= pCfg->maxTrends )
      {
        // Force a finish to this sampling and flag it for autotrend-failure logging.
        bAutoTrendFailed = TRUE;
      }
    }

    if (bAutoTrendFailed)
    {
      TrendWriteErrorLog( APP_ID_TREND_AUTO_FAILED,
                          pData->trendIndex,
                          pCfg->stability,
                          &pData->curStability);

    }else if ( pData->masterSampleCnt >= pData->nSamplesPerPeriod )
    {
      // Sampling period complete on last pass
      TrendFinish(pCfg, pData);
    }
    else
    {
      // Keep going...update master trend cnt and all configured summaries
      ++pData->masterSampleCnt;
      SensorUpdateSummaries(pData->snsrSummary, pData->nTotalSensors);
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
 * Notes:
 *
 *****************************************************************************/
static void TrendFinish( TREND_CFG* pCfg, TREND_DATA* pData )
{
  UINT8         i;
  SNSR_SUMMARY  *pSummary;
  TREND_LOG*    pLog = &m_TrendLog[pData->trendIndex];

  // If a trend was in progress, create a log, otherwise return.
  if (pData->trendState > TREND_STATE_INACTIVE )
  {
    SensorCalculateSummaryAvgs( pData->snsrSummary, pData->nTotalSensors );

    // Trend name
    pLog->trendIndex = pData->trendIndex;
    // Log the type/state: MANUAL|AUTO
    pLog->type     = pData->trendState;

    pLog->nSamples = pData->masterSampleCnt;

    // Loop through all the sensors
    for ( i = 0; i < pData->nTotalSensors; i++ )
    {
      // Set a pointer the summary
      pSummary = &(pData->snsrSummary[i]);

      // Check the sensor is used
      if ( SensorIsUsed((SENSOR_INDEX)pSummary->SensorIndex ) )
      {
        pSummary->bValid = SensorIsValid((SENSOR_INDEX)pSummary->SensorIndex);

        // Only some of the sensor stats are used for trend logging
        pLog->sensor[i].SensorIndex = pSummary->SensorIndex;
        pLog->sensor[i].bValid      = pSummary->bValid;
        pLog->sensor[i].fMaxValue   = pSummary->fMaxValue;
        pLog->sensor[i].fAvgValue   = pSummary->fAvgValue;
      }
      else // Sensor Not Used
      {
        pLog->sensor[i].SensorIndex = SENSOR_UNUSED;
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
        pLog->cycleCounts[i].cycleCount = CycleGetPersistentCount(pCfg->cycle[i]);
      }
      else
      {
        pLog->cycleCounts[i].cycleCount = 0;
      }
    }
#pragma ghs endnowarning

    // Now do the log details...
    LogWriteETM( APP_ID_TREND_END,
                 LOG_PRIORITY_3,
                 &m_TrendLog[pData->trendIndex],
                 sizeof(TREND_LOG),
                 NULL);

    pData->trendState = TREND_STATE_INACTIVE;

    // Clear Action
    if( pCfg->nAction)
    {
      pData->nActionReqNum = ActionRequest(pData->nActionReqNum, pCfg->nAction,
        ACTION_OFF, FALSE, FALSE);
    }

    // Reset fields for the next sample period.

    // Reset sample count.
    pData->masterSampleCnt  = 0;

    // Reset sensor stability timer
    pData->lastStabCheckMs  = 0;
    pData->nTimeStableMs    = 0;

    // Reset interval timer
    pData->TimeSinceLastTrendMs  = 0;
    pData->lastIntervalCheckMs   = 0;


/*vcast_dont_instrument_start*/
#ifdef TREND_DEBUG
    GSE_DebugStr(NORMAL,TRUE, "Trend[%d]: Sample Ended...Logged",pData->trendIndex );
#endif
/*vcast_dont_instrument_end*/

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
  #pragma ghs nowarning 1545 //Suppress packed structure alignment warning

  if (TREND_STATE_INACTIVE != pData->trendState)
  {
    GSE_DebugStr(NORMAL,TRUE, "Trend[%d] Reset from %s",
                 pData->trendIndex,
                 pData->trendState == TREND_STATE_AUTO ? "AUTO" : "MANUAL");
  }

  pData->trendState       = TREND_STATE_INACTIVE;
  pData->bResetDetected   = FALSE;
  pData->bEnabled         = FALSE;

  pData->nTimeStableMs    = 0;
  pData->lastStabCheckMs  = 0;

  // If called during runtime (i.e. start of an ENGINE RUN) set the interval to appear
  // as though the interval has elapsed so there will be no Interval wait for the
  // initial stability check
  if (bRunTime)
  {
    pData->TimeSinceLastTrendMs = pCfg->trendInterval_s * ONE_SEC_IN_MILLSECS;
    pData->lastIntervalCheckMs  = CM_GetTickCount();
  }
  else
  {
    pData->TimeSinceLastTrendMs  = 0;
    pData->lastIntervalCheckMs   = 0;
  }

  // Reset the stability and max-stability history
  // todo DaveB confirm we want to clear max-history during reset.
  // If inflight engine-restart (i.e. no reset-trigger), this info will be cleared! OK?
  pData->curStability.stableCnt = 0;
  pData->maxStability.stableCnt = 0;

  for ( i = 0; i < MAX_STAB_SENSORS; i++)
  {
    pData->curStability.snsrValue[i] = 0.f;
    pData->curStability.validity[i]  = FALSE;
    pData->curStability.status[i]    = STAB_UNKNOWN;

    pData->maxStability.snsrValue[i] = 0.f;
    pData->maxStability.validity[i]  = FALSE;
    pData->maxStability.status[i]    = STAB_UNKNOWN;
  }

  // Reset(Init) the monitored sensors
  pData->nTotalSensors = SensorSetupSummaryArray(pData->snsrSummary,
                                                   MAX_TREND_SENSORS,
                                                   pCfg->sensorMap,
                                                   sizeof(pCfg->sensorMap) );
  #pragma ghs endnowarning
}


/******************************************************************************
 * Function: TrendEnableTask
 *
 * Description: Wrapper function for enabling/disabling the Trend task.
 *
 * Parameters:   BOOLEAN flag
 *                  TRUE  - Enable the Trend task.
 *                  FALSE - Disable the Trend task.
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
 * Parameters:   [in]     pCfg  Pointer to trend  config data
 *               [in/out] pData Pointer to trend runtime data
 *
 * Returns:      If all sensors are stable then TRUE otherwise FALSE
 *
 *
 * Notes:
 *
 *****************************************************************************/
static BOOLEAN TrendCheckStability( TREND_CFG* pCfg, TREND_DATA* pData )
{
  FLOAT32 fVal;
  FLOAT32 fPrevValue;
  FLOAT32 delta;
  INT32   i;

  STABILITY_CRITERIA* pStabCrit;

  // Reset the count of stable sensors.
  pData->curStability.stableCnt = 0;

  // Check all stability sensors for this trend obj.
  // If no stability configured, we never perform this loop
  for (i = 0; i < MAX_STAB_SENSORS && (pData->nStabExpectedCnt > 0); ++i)
  {
    pStabCrit = &pCfg->stability[i];

    // Assume the current stability sensor is good, modify as needed.
    pData->curStability.status[i] = STAB_OK;

    if ( SensorIsUsed(pStabCrit->sensorIndex) )
    {
      if ( SensorIsValid(pStabCrit->sensorIndex))
      {
        // get the current sensor value
        fVal       = SensorGetValue(pStabCrit->sensorIndex);
        fPrevValue = pData->curStability.snsrValue[i];

        // Check absolute min/max values
        if ( (fVal >= pStabCrit->criteria.lower) && (fVal <= pStabCrit->criteria.upper) )
        {
          // Check the stability of the sensor value withing a given variance
          // ensure positive difference

          delta = ( fPrevValue > fVal) ? fPrevValue - fVal :  fVal - fPrevValue;

          if (delta >= pStabCrit->criteria.variance)
          {
            pData->curStability.status[i] = STAB_SNSR_VARIANCE_ERROR;
          }
        }
        else
        {
          pData->curStability.status[i] = STAB_SNSR_RANGE_ERROR;
        }

        // If the sensor is in stable range and variance... increment the count
        // If this is the max observed during this trend, store it for
        // potential TREND_AUTO_NOT_DETECTED log

        if (STAB_OK == pData->curStability.status[i] )
        {
          if (++pData->curStability.stableCnt > pData->maxStability.stableCnt)
          {
            memcpy(&pData->maxStability, &pData->curStability, sizeof(STABILITY_DATA));
          }
        }
        else // if any test fails, reset stored sensor value to start over next time
        {
           pData->curStability.snsrValue[i] = fVal;
        }
      }
      else
      {
        pData->curStability.status[i] = STAB_SNSR_INVALID;
      }
    }
    else
    {
      pData->curStability.status[i] = STAB_SNSR_NOT_DECLARED;
    }
  }
  // if all configured(expected) sensors are stable, return true

  return (pData->curStability.stableCnt == pData->nStabExpectedCnt &&
          pData->nStabExpectedCnt > 0);
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
void TrendCheckResetTrigger( TREND_CFG* pCfg, TREND_DATA* pData )
{
   //Each Trend shall record a "Trend not Detected" log when the reset trigger becomes active,
   //the trend processing was ENABLED and a trend has not been taken.

  // Only handle the reset-event upon initial detected.
  if ( FALSE == pData->bResetDetected          &&  // RestTrigger WAS NOT already detected.
       pCfg->resetTrigger != TRIGGER_UNUSED    && // ResetTrigger IS declared.
       TriggerIsConfigured(pCfg->resetTrigger) && // ResetTrigger IS defined.
       TriggerGetState    (pCfg->resetTrigger))   // ResetTrigger IS ACTIVE.
  {
    // Flag the Reset as detected so we don't do this again until the next new reset.
    pData->bResetDetected = TRUE;

    GSE_DebugStr(NORMAL,TRUE,"Trend[%d] Autotrend Reset Detected ",pData->trendIndex);

    // If, no stable/auto trends taken, log it.
    if ( pData->bEnabled  &&  0 == pData->stableTrendCnt)
    {
      TrendWriteErrorLog( APP_ID_TREND_AUTO_NOT_DETECTED,
                          pData->trendIndex,
                          pCfg->stability,
                          &pData->maxStability);
    }

    // Reset max stable count.
    pData->stableTrendCnt = 0;
  }
  else if(pData->bResetDetected && !TriggerGetState(pCfg->resetTrigger))
  {
    // If trend-reset was detected and is now clear,
    // clear the flag for next use.
    pData->bResetDetected = FALSE;
  }
}



/******************************************************************************
 * Function: TrendWriteErrorLog
 *
 * Description: Create an error log using the passed parameters
 *
 * Parameters:   [in] logId     - identifier of log type
 *               [in] trendIdx  - object index of trend being reported
 *               [in] criteria  - configured stability criteria of trend being reported
 *               [in] data      - the state of the sensors being monitored.
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static void TrendWriteErrorLog(SYS_APP_ID logID, TREND_INDEX trendIdx,
                               STABILITY_CRITERIA stabCrit[], STABILITY_DATA* stabData)
{
  TREND_ERROR_LOG trendLog;

  memcpy(trendLog.crit,  stabCrit,  sizeof(trendLog.crit));
  memcpy(&trendLog.data, stabData, sizeof(trendLog.data));

  trendLog.trendIndex = trendIdx;
  LogWriteETM(logID, LOG_PRIORITY_3, &trendLog, sizeof(trendLog), NULL);

  GSE_DebugStr(NORMAL,TRUE, "Trend[%d]: %s Logged.", trendIdx,
                                                     logID == APP_ID_TREND_AUTO_NOT_DETECTED ?
                                                     "Auto-Trend Not-Detected" :
                                                     "Auto-Trend failure");
}

/******************************************************************************
 * Function: TrendUpdateAutoTrend
 *
 * Description:  Check to see if Auto trend criteria has been met
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
static void TrendUpdateAutoTrend( TREND_CFG* pCfg, TREND_DATA* pData )
{
  UINT32 timeNow;



  timeNow = CM_GetTickCount();

  // Can more trend-samples be taken OR is an autotrend in progress?
  if ( pData->stableTrendCnt < pCfg->maxTrends ||
       TREND_STATE_AUTO == pData->trendState)
  {

    // Update minimum time between trends
    TrendUpdateTimeElapsed(timeNow,
                           &pData->lastIntervalCheckMs,
                           &pData->TimeSinceLastTrendMs);

   /* GSE_DebugStr(NORMAL,TRUE,"Trend[%d]: Trend Interval: %d of %d",
      pData->trendIndex,
      pData->TimeSinceLastTrendMs,
      pCfg->trendInterval_s * ONE_SEC_IN_MILLSECS);*/


    // Check if the configured criteria reached/maintained stable.
    if (TrendCheckStability(pCfg, pData) )
    {
      // Criteria is stable... update stability time-elapsed.
      TrendUpdateTimeElapsed(timeNow, &pData->lastStabCheckMs, &pData->nTimeStableMs);

      /*GSE_DebugStr(NORMAL,TRUE,"Trend[%d]: Sensors Stable: %d of %d",
                               pData->trendIndex,
                               pData->nTimeStableMs,
                               pCfg->stabilityPeriod_s * ONE_SEC_IN_MILLSECS);*/


      // If stability and interval durations have been met AND not already processing
      // trend, start one
      if ( (pData->nTimeStableMs    >= (pCfg->stabilityPeriod_s * ONE_SEC_IN_MILLSECS)) &&
           (pData->TimeSinceLastTrendMs >= (pCfg->trendInterval_s   * ONE_SEC_IN_MILLSECS)) &&
            TREND_STATE_INACTIVE == pData->trendState)
      {
        TrendStartAutoTrend(pCfg, pData);
      }

    }
    else // Criteria sensors didn't reach stability or active autotrend went unstable
    {
      // Reset the stability timing for next pass.
      // If an TREND_STATE_AUTO is in progress, resetting the timing
      // will signal TrendUpdateData to create a autotrend fail log.
      pData->lastStabCheckMs = 0;
      pData->nTimeStableMs   = 0;
    }

  } // More trend-samples can be taken.
}

/******************************************************************************
 * Function: TrendUpdateTimeElapsed
 *
 * Description: Generalized routine to update time elapsed
 *
 * Parameters:
 *
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
     // if elapsed time is zero we are starting a
     *lastTimeCalled  = currentTime;
     *elapsedTime     = 0;
   }
   // Update the elapsed time
   *elapsedTime     += (currentTime - *lastTimeCalled);
   *lastTimeCalled  =  currentTime;
 }


/******************************************************************************
 * Function: TrendStartAutoTrend
 *
 * Description: Generalized routine to update time elapsed
 *
 * Parameters:
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static void TrendStartAutoTrend(const TREND_CFG* pCfg, TREND_DATA* pData)
{
  // Local Data
  TREND_START_LOG trendLog;

  pData->trendState = TREND_STATE_AUTO;
  //todo DaveB TrendReset here SRS-4273

  // Initialize Local Data
  trendLog.trendIndex = pData->trendIndex;
  trendLog.type       = pData->trendState;

  LogWriteETM(APP_ID_TREND_START,
    LOG_PRIORITY_3,
    &trendLog,
    sizeof(trendLog),
    NULL);

  // Activate Action
  if( pCfg->nAction)
  {
    pData->nActionReqNum = ActionRequest(pData->nActionReqNum, pCfg->nAction,
                                         ACTION_ON, FALSE, FALSE);
  }

  // Increment the stability-trend count and overall trend-count.
  pData->stableTrendCnt++;
  pData->trendCnt++;

  GSE_DebugStr( NORMAL,TRUE,
                "Trend[%d]: Auto-Trend started Auto-Trend count: %d, Stable: %dms, Interval: %dms",
                pData->trendIndex,
                pData->stableTrendCnt,
                pData->nTimeStableMs,
                pData->TimeSinceLastTrendMs );
}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: trend.c $
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


