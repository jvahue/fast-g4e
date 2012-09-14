#define TREND_BODY
/******************************************************************************
           Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
              All Rights Reserved. Proprietary and Confidential.

   File:        trend.c

   Description:


   Note:

 VERSION
 $Revision: 6 $  $Date: 9/14/12 4:07p $

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
#define TREND_UNDER_CONSTRUCTION



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


static void TrendStartManualTrend( TREND_CFG* pCfg, TREND_DATA* pData );
static void TrendStartAutoTrend  (TREND_CFG* pCfg, TREND_DATA* pData);

static void TrendUpdateData      ( TREND_CFG* pCfg, TREND_DATA* pData );
static void TrendUpdateAutoTrend( TREND_CFG* pCfg, TREND_DATA* pData );

static BOOLEAN TrendCheckStability( TREND_CFG* pCfg, TREND_DATA* pData);

static void TrendLogAutoTrendFailure( TREND_CFG* pCfg, TREND_DATA* pData );


static void TrendReset(TREND_CFG* pCfg, TREND_DATA* pData);

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
  UINT8 i;
  UINT8 j;
  TREND_CFG*  pCfg;
  TREND_DATA* pData;
  UINT32      timeNow = CM_GetTickCount();

  // Add user commands for Trend to the user command tables.
  User_AddRootCmd(&RootTrendMsg);

  // Clear and Load the current cfg info.
  memset(&m_TrendData, 0, sizeof(m_TrendData));

  memcpy(m_TrendCfg,
    &(CfgMgr_RuntimeConfigPtr()->TrendConfigs),
    sizeof(m_TrendCfg));

  
  #pragma ghs nowarning 1545 //Suppress packed structure alignment warning

  for (i = 0; i < MAX_TRENDS; i++)
  {
    pCfg  = &m_TrendCfg[i];
    pData = &m_TrendData[i];   
    
    pData->trendIndex      = (ENGRUN_UNUSED == pCfg->engineRunId) ?
                              TREND_UNUSED : (TREND_INDEX)i;    
    pData->prevEngState    = ER_STATE_STOPPED;
    pData->nRateCounts     = (UINT16)(MIFs_PER_SECOND / pCfg->rate);
    pData->nRateCountdown  = (UINT16)((pCfg->nOffset_ms / MIF_PERIOD_mS) + 1);

    pData->nSamplesPerPeriod = pCfg->nSamplePeriod_s * pCfg->rate;

    // Calculate the expected number of stability sensors based on config
    pData->nStabExpectedCnt = 0;
    for (j = 0; j < MAX_STAB_SENSORS; ++j)
    {
      if (SENSOR_UNUSED != pCfg->stability[j].sensorIndex)
      {
        pData->nStabExpectedCnt++;
      }
    }
    // Call the reset functions to init the others fields
    TrendReset(pCfg, pData);

    // TrendReset() just set the following values to zero which is nornally OK.
    // However for the first time thru there's no need to wait for inter-trend interval,
    // so pretend it just elapsed.
    pData->nTimeSinceLastMs    = pCfg->trendInterval_s * ONE_SEC_IN_MILLSECS;
    pData->lastIntervalCheckMs = timeNow;

  }
#pragma ghs endnowarning

  // Create Trend Task - DT
  memset(&tcb, 0, sizeof(tcb));
  strncpy_safe(tcb.Name, sizeof(tcb.Name),"Trend Task",_TRUNCATE);
  tcb.TaskID           = Trend_Task;
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
  UINT16       nTrend;

  if(Tm.systemMode == SYS_SHUTDOWN_ID)
  {
    
    for ( nTrend = 0; nTrend < MAX_TRENDS; nTrend++ )
    {
      TrendFinish(&m_TrendCfg[nTrend], &m_TrendData[nTrend]);
    }
    TrendEnableTask(FALSE);
  }
  else // Normal task execution.
  {
    // Loop through all available triggers
    for ( nTrend = 0; nTrend < MAX_TRENDS; nTrend++ )
    {
      // Set pointers to Trend cfg and data
      pCfg  = &m_TrendCfg[nTrend];
      pData = &m_TrendData[nTrend];

      // Determine if current trigger is being used
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
 * Function: TrendIsActive
 *
 * Description: Tests if one or more trends are active
 *
 * Parameters:   None
 *
 * Returns:      TRUE if any trend is active, otherwise FALSE.
 *
 * Notes:
 *
 *****************************************************************************/
BOOLEAN TrendIsActive( void )
{
  TREND_INDEX i;
  BOOLEAN bTrendActive = FALSE;

  // Check if any trend is currently active.
  for( i = TREND_0; i < MAX_TRENDS; ++i )
  {
    if (m_TrendData[i].trendState > TREND_STATE_INACTIVE )
    {
      bTrendActive = TRUE;
      break;
    }
  }

  return bTrendActive;
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
   UINT16     trendIndex;
   INT8       *pBuffer;
   UINT16     nRemaining;
   UINT16     nTotal;

   // Initialize Local Data
   pBuffer    = (INT8 *)pDest;
   nRemaining = nMaxByteSize;
   nTotal     = 0;
   memset ( &trendHdr, 0, sizeof(trendHdr) );

   // Loop through all the Trends
   for ( trendIndex = 0;
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
   memcpy ( pBuffer, &trendHdr, nTotal );
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

  switch ( erState )
  {
    case ER_STATE_STOPPED:
        // If we have entered stopped mode from some other mode, 
      // finish any outstanding trends
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
        
      // Upon entering RUNNING state, reset any trending info from previous runs
      if (pData->prevEngState != ER_STATE_RUNNING)
      {
        TrendReset(pCfg, pData);
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

      /* set the last engine run state */
      pData->prevEngState = ER_STATE_RUNNING;
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
static void TrendStartManualTrend( TREND_CFG* pCfg, TREND_DATA* pData )
{
  if ( TREND_STATE_INACTIVE  == pData->trendState )
  {
    GSE_DebugStr(NORMAL,TRUE, "Trend[%d]: Manual Trend started.",pData->trendIndex ); 
    
    pData->bTrendLamp = pCfg->lampEnabled;
    pData->trendState = TREND_STATE_MANUAL;    

    LogWriteETM( APP_ID_TREND_MANUAL,
                 LOG_PRIORITY_3,
                 &pData->trendIndex,
                 sizeof(pData->trendIndex),
                 NULL );
  }

}

/******************************************************************************
 * Function: TrendUpdatedData
 *
 * Description:
 *
 * Parameters:   None
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static void TrendUpdateData( TREND_CFG* pCfg, TREND_DATA* pData  )
{
  BOOLEAN bAutoTrendFailed = FALSE;
  UINT16 i;
  SNSR_SUMMARY* pSummary;
  FLOAT32  oneOverN;

  // If this trend is active. Update it's status
  if ( pData->trendState != TREND_STATE_INACTIVE )
  {    
    // Check if the preceding call to TrendUpdateAutoTrend flagged this trend
    // as failing to maintain stability
    if( TREND_STATE_AUTO ==  pData->trendState )
    {
      if( 0 == pData->nTimeStableMs && pData->trendCnt < pCfg->maxTrends )
      {
        // Force a finish to this sampling and flag it for autotrend-failure logging.
        pData->sampleCnt = pData->nSamplesPerPeriod;
        bAutoTrendFailed = TRUE;
      }
    }

    // Did sampling period complete on last pass?
    if ( pData->sampleCnt >= pData->nSamplesPerPeriod )
    {
      TrendFinish(pCfg, pData);
      // If we have a failure in an autotrend, add the autotrend failure log entry.
      if (bAutoTrendFailed)
      {
        TrendLogAutoTrendFailure(pCfg, pData);
      }
    }
    else  
    {
      // Increment the count of samples for this period
      ++pData->sampleCnt;

      // Update the monitored-sensors
      for ( i = 0; i < pData->nTotalSensors; i++ )
      {
        pSummary = &pData->snsrSummary[i];

        // If the sensor is flagged invalid but WAS valid in the past(i.e. initialized)...
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
          // Sensor is now invalid but had been valid...
          // calculate average for valid period.
          oneOverN = (1.0f / (FLOAT32)(pData->trendCnt - 1));
          pSummary->fAvgValue = pSummary->fTotal * oneOverN;
        }
      } // for nTotalSensors

    }// take next trend sample
  }// Process active trend
} // TrendUpdateData



/******************************************************************************
 * Function: TrendFinish
 *
 * Description:
 *
 * Parameters:   None
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static void TrendFinish( TREND_CFG* pCfg, TREND_DATA* pData )
{
  FLOAT32       oneOverN;
  UINT8         i;
  SNSR_SUMMARY  *pSummary;
  TREND_LOG*    pLog = &m_TrendLog[pData->trendIndex];
  CYCLE_INDEX*  pCycleList;
  CYCLE_INDEX   cycIdx;

  // If no trend is active. Return
  if (pData->trendState > TREND_STATE_INACTIVE )
  {
     oneOverN = (1.0f / (FLOAT32)pData->sampleCnt);
    
    // Trend name
    //strncpy_safe( pLog->name, sizeof(pLog->name), pCfg->trendName, _TRUNCATE);

    // Log the type/state: MANUAL|AUTO
    pLog->type     = pData->trendState;

    pLog->nSamples = pData->sampleCnt;

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

        // if sensor is valid, calculate the final average,
        // otherwise use the average calculated at the point it went invalid.
        pLog->sensor[i].fAvgValue   = (TRUE == pSummary->bValid ) ?
                                           pSummary->fTotal * oneOverN :
                                           pSummary->fAvgValue;
      }
      else // Sensor Not Used
      {
        pLog->sensor[i].SensorIndex = SENSOR_UNUSED;        
        pLog->sensor[i].fMaxValue   = 0.0;
        pLog->sensor[i].fAvgValue   = 0.0;
        pLog->sensor[i].bValid      = FALSE;
      }
    }    

#pragma ghs nowarning 1545 //Suppress packed structure alignment warning  
    
    // Update persistent cycle counter data if present.

    // Set a pointer to the first
    pCycleList = &pCfg->nCycleA;

    for (i = 0; i < MAX_TREND_CYCLES; i++)
    {
      cycIdx = (CYCLE_INDEX) *(pCycleList + i); 
      pLog->cycleCounts[i].cycIndex = cycIdx;
      if (CYCLE_UNUSED != cycIdx)
      {
        pLog->cycleCounts[i].cycleCount = CycleGetPersistentCount(cycIdx);
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

    GSE_DebugStr(NORMAL,TRUE, "Trend[%d]: Ended.",pData->trendIndex );
    
    pData->trendState = TREND_STATE_INACTIVE;
    
    // Don't reset the data here!
    // An error log may follow which will require logging details
    // Reset will occur the next time this trend enters the run state.

  } // End of finishing up a log for trend in progress.

}


/******************************************************************************
 * Function: TrendReset
 *
 * Description: Reset the processing state data for the passed Trend.
 *
 * Parameters:   None
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static void TrendReset(TREND_CFG* pCfg, TREND_DATA* pData)
{
  UINT8 i;
#pragma ghs nowarning 1545 //Suppress packed structure alignment warning
   
  pData->trendState       = TREND_STATE_INACTIVE;
  
  // pData->prevEngState  = ER_STATE_STOPPED;
  pData->bTrendLamp       = FALSE;

  pData->nTimeStableMs    = 0;
  pData->lastStabCheckMs  = 0;

  pData->nTimeSinceLastMs    = 0;
  pData->lastIntervalCheckMs = 0;
    
  // Reset the stability and max-stability history
  // todo DaveB confirm we want to clear max-history during reset. If inflight engine-restart (i.e. no reset-trigger), this info will be cleared! OK?
  pData->stability.stableCnt = 0;
  pData->maxStability.stableCnt = 0;

  for ( i = 0; i < MAX_STAB_SENSORS; i++)
  {
    pData->stability.prevStabValue[i]    = 0.f;
    pData->maxStability.prevStabValue[i] = 0.f;
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
 * Description: Check whether the psas
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
BOOLEAN TrendCheckStability( TREND_CFG* pCfg, TREND_DATA* pData )
{
  FLOAT32 fVal;
  FLOAT32 fPrevValue;
  FLOAT32 delta;
  INT32   i;

  BOOLEAN bStatus = FALSE;
  STABILITY_CRITERIA* pStabCrit;

  // Reset the count of stable sensors.
  pData->stability.stableCnt = 0;

  // Check all stability sensors for this trend obj.
  // If no stability configured, we never perform this loop
  for (i = 0; i < MAX_STAB_SENSORS && (pData->nStabExpectedCnt > 0); ++i)
  {
    pStabCrit = &pCfg->stability[i];

    if ( SensorIsUsed(pStabCrit->sensorIndex) )
    {
      if ( SensorIsValid(pStabCrit->sensorIndex))
      {
        // get the current sensor value
        fVal       = SensorGetValue(pStabCrit->sensorIndex);
        fPrevValue = pData->stability.prevStabValue[i];

        // Check absolute min/max values
        bStatus = (fVal >= pStabCrit->criteria.lower) &&
                  (fVal <= pStabCrit->criteria.upper);
        if (bStatus)
        {
          // Check the stability of the sensor value withing a given variance
          // ensure positive difference
          if ( fPrevValue > fVal)
          {
            delta = fPrevValue - fVal;
          }
          else
          {
            delta = fVal - fPrevValue;
          }
          bStatus = (delta < pStabCrit->criteria.variance);
        }
        
        if ( bStatus)
        {
          // If the sensor is in stable range and variance increment the count
          // If this is the max observed during this trend store it for
          // potential TREND_AUTO_NOT_DETECTED log
          
          ++pData->stability.stableCnt;
          if (pData->stability.stableCnt > pData->maxStability.stableCnt)
          {
            memcpy(&pData->maxStability, &pData->stability, sizeof(STABILITY_DATA));
          }         
        }
        else // if any test fails, reset stored sensor value to start over next time
        {
           pData->stability.prevStabValue[i] = fVal;
        }
      }      
    }    
  }
  // if all configured sensors are stable, return true

  return (pData->stability.stableCnt == pData->nStabExpectedCnt && 
          pData->nStabExpectedCnt > 0);
}

/******************************************************************************
 * Function: TrendLogAutoTrendFailure
 *
 * Description: Create a log that an active AutoTrend did not maintain stability
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
static void TrendLogAutoTrendFailure( TREND_CFG* pCfg, TREND_DATA* pData )
{
  LogWriteETM(APP_ID_TREND_AUTO_FAILED,
              LOG_PRIORITY_3,
              &pCfg->trendName,
              sizeof(pCfg->trendName),
              NULL);

  GSE_DebugStr(NORMAL,TRUE, "Trend[%d]: Auto-Trend failure logged.",pData->trendIndex );
}

/******************************************************************************
 * Function: TrendLogAutoTrendNotDetected
 *
 * Description: Create a log that a triggered AutoTrend did not atain stability
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
static void TrendLogAutoTrendNotDetected( TREND_CFG* pCfg, TREND_DATA* pData )
{
  TREND_NOT_DETECTED_LOG log;

  memcpy(&log.crit, &pCfg->stability,     sizeof(STABILITY_CRITERIA));
  memcpy(&log.data, &pData->maxStability, sizeof(STABILITY_DATA));
  
  
  LogWriteETM(APP_ID_TREND_AUTO_NOT_DETECTED,
              LOG_PRIORITY_3,
              &log,
              sizeof(TREND_NOT_DETECTED_LOG),
              NULL);

  GSE_DebugStr(NORMAL,TRUE, "Trend[%d]: Auto-Trend Not Detected Logged.",pData->trendIndex );
}


/******************************************************************************
 * Function: TrendUpdateAutoTrend
 *
 * Description: Create a log that a AutoTrend did not reach stability
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
static void TrendUpdateAutoTrend( TREND_CFG* pCfg, TREND_DATA* pData )
{
  UINT32 timeNow;

  // Each Trend shall reset the maximum trend count when the
  // configured trend reset trigger becomes ACTIVE.
  if ( pCfg->resetTrigger != TRIGGER_UNUSED    &&
       TriggerIsConfigured(pCfg->resetTrigger) &&
       TriggerGetState(pCfg->resetTrigger))
  {    
    if (pData->trendCnt)
    {
      // No autotrends taken? Log it.
      TrendLogAutoTrendNotDetected(pCfg, pData);
    }
    
    pData->trendCnt         = 0;
    pData->nTimeStableMs    = 0;
    pData->lastStabCheckMs  = 0;

    pData->nTimeSinceLastMs    = pCfg->trendInterval_s * ONE_SEC_IN_MILLSECS;
    pData->lastIntervalCheckMs = 0;
  }

  timeNow = CM_GetTickCount();

  // Can more trend-samples be taken OR is an autotrend in progress?
  if ( pData->trendCnt < pCfg->maxTrends || TREND_STATE_AUTO == pData->trendState)
  {
    
    // Update minimum time between trends 
    TrendUpdateTimeElapsed(timeNow,
                           &pData->lastIntervalCheckMs,
                           &pData->nTimeSinceLastMs);

    // Check if the configured criteria reached/maintained stable.
    if (TrendCheckStability(pCfg, pData) )
    {
      // Criteria is stable... update stability time-elapsed.
      TrendUpdateTimeElapsed(timeNow, &pData->lastStabCheckMs, &pData->nTimeStableMs);

      // If stability and interval durations have been met AND not already processing
      // trend, start one
      if ( (pData->nTimeStableMs    >= (pCfg->stabilityPeriod_s * ONE_SEC_IN_MILLSECS)) &&
           (pData->nTimeSinceLastMs >= (pCfg->trendInterval_s   * ONE_SEC_IN_MILLSECS)) &&
            TREND_STATE_INACTIVE == pData->trendState)
      {
        TrendStartAutoTrend(pCfg, pData);        
      }
      
    }
    else // Criteria sensors didn't reach stability or active autotrend went unstable
    {
      // Reset the stability duration for next pass
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
 void TrendUpdateTimeElapsed(UINT32 currentTime, UINT32* lastTimeCalled, UINT32* elapsedTime)
 {
   // If first time thru, set last time to current so elapsed will be zero.
   if (0 == *lastTimeCalled)
   {
     // if elapsed time is zero we are starting a        
     *lastTimeCalled    = currentTime;
     *elapsedTime = 0;
   }
   // Update the elapsed time
   *elapsedTime += (currentTime - *lastTimeCalled);
   *lastTimeCalled    =  currentTime;
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
static void TrendStartAutoTrend(TREND_CFG* pCfg, TREND_DATA* pData)
{
  // Local Data
  UINT32  nTrendStorage;

  // Initialize Local Data
  nTrendStorage = (UINT32)pData->trendIndex;

  LogWriteETM(APP_ID_TREND_AUTO,
    LOG_PRIORITY_3,
    &nTrendStorage,
    sizeof(nTrendStorage),
    NULL); 
  
  pData->trendState   = TREND_STATE_AUTO;
  pData->bTrendLamp   = pCfg->lampEnabled;

  pData->trendCnt++;

  GSE_DebugStr( NORMAL,TRUE, "Trend[%d]: Auto-Trend started Trend count: %d",
                pData->trendIndex, pData->trendCnt );
}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: trend.c $
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


