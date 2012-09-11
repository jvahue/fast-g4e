#define TREND_BODY
/******************************************************************************
           Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
              All Rights Reserved. Proprietary and Confidential.

   File:        trend.c

   Description:


   Note:

 VERSION
 $Revision: 5 $  $Date: 12-09-11 2:20p $

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
static TREND_LOG   m_TrendLog[MAX_TRENDS];


#include "TrendUserTables.c"

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void TrendProcess         ( void );

static void TrendFinish          ( TREND_CFG* pCfg, TREND_DATA* pData );


static void TrendStartManualTrend( TREND_CFG* pCfg, TREND_DATA* pData );
static void TrendStartAutoTrend  (TREND_CFG* pCfg, TREND_DATA* pData);

static void TrendUpdateData      ( TREND_CFG* pCfg, TREND_DATA* pData );
static void TrendUpdateAutoTrend( TREND_CFG* pCfg, TREND_DATA* pData );

static void TrendForceClose      ( void );

static BOOLEAN TrendIsActive     ( void );
static BOOLEAN TrendCheckStability( TREND_CFG* pCfg, TREND_DATA* pData);

static void TrendGetPersistentData();

static void TrendLogAutoTrendFailure( TREND_CFG* pCfg, TREND_DATA* pData );


static void TrendReset(TREND_CFG* pCfg, TREND_DATA* pData);

static void TrendForceEnd(void );
static void TrendEnableTask(BOOLEAN bEnable);

static void TrendUpdateTimeElapsed( UINT32  currentTime,
                                    UINT32* lastTime,
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
  TCB tcb;
  UINT8     i;

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
    m_TrendData[i].trendIndex   = (TREND_INDEX)i;
    m_TrendData[i].PrevEngState = ER_STATE_STOPPED;

    // Call the reset functions to init the others fields
    TrendReset(&m_TrendCfg[i], &m_TrendData[i]);
  }
#pragma ghs endnowarning


#ifndef TREND_UNDER_CONSTRUCTION
  // todo DaveB this may be OBE in the FAST 2 implementation
  /* Set time of next sample to initial offset. */
  Trend.TrendTime = TrendData.TrendOffset;
  /* Set number of samples to zero. */
  Trend.cntTrendSamples  = 0;
  /* Set log entry pointer to NULL (none started). */
  pTrendLog   = NULL;
  for (i = 0; i < MAX_AUTOTRENDS; i++)
  {
    AutoTrend[i].bAutoTrendLog = FALSE;
  }
  /* Set the last op mode to STOPPED. (not in trendable state yet) */
  TrendData.PrevEngState = STOPPED_SYS;

  TrendData.trendActive = FALSE;
  TrendData.trendLamp = FALSE;
#endif


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
  if(Tm.systemMode == SYS_SHUTDOWN_ID)
  {
    TrendForceEnd();
    TrendEnableTask(FALSE);
  }
  else // Normal task execution.
  {
    TrendProcess();
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
static BOOLEAN TrendIsActive( void )
{
  TREND_INDEX i;
  BOOLEAN bTrendActive = FALSE;

  for( i = TREND_0; i < MAX_TRENDS; ++i )
  {
    if (TRUE == m_TrendData[i].bTrendActive)
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
      trendHdr[trendIndex].maxTrends     = m_TrendCfg[trendIndex].maxTrendSamples;
      trendHdr[trendIndex].trendInterval = m_TrendCfg[trendIndex].TrendInterval_s;
      trendHdr[trendIndex].nTimeStable_s = m_TrendCfg[trendIndex].nTimeStable_s;
      trendHdr[trendIndex].StartTrigger  = m_TrendCfg[trendIndex].StartTrigger;
      trendHdr[trendIndex].ResetTrigger  = m_TrendCfg[trendIndex].ResetTrigger;


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
static void TrendProcess( void )
{
  UINT16      trendIdx;
  ER_STATE    erState;
  TREND_CFG*  pCfg;
  TREND_DATA* pData;
  BITARRAY128 trigMask;

  CLR_TRIGGER_FLAGS(trigMask);

  for (trendIdx = 0; trendIdx < MAX_TRENDS; ++trendIdx)
  {
    // Process each Trend based on trend state according to Eng-run state.
    pCfg  = &m_TrendCfg[trendIdx];
    pData = &m_TrendData[trendIdx];

    // Get the state of the associated EngineRun
    erState = EngRunGetState(pCfg->EngineRunId, NULL);

    switch ( erState )
    {
      case ER_STATE_STOPPED:
        // If we have entered stopped mode from some other mode,
        // finish any outstanding trends
        if (pData->PrevEngState != ER_STATE_STOPPED)
        {
          TrendFinish(pCfg, pData);
        }
        pData->PrevEngState = ER_STATE_STOPPED;
        break;

      case ER_STATE_STARTING:
        // If we have entered starting mode from some other mode,
        // finish any outstanding trends
        if (pData->PrevEngState != ER_STATE_STARTING)
        {
          TrendFinish(pCfg, pData);
        }
        pData->PrevEngState = ER_STATE_STARTING;
        break;


      case ER_STATE_RUNNING:

        // Upon entering RUNNING state, reset the auto-trending
        if (pData->PrevEngState != ER_STATE_RUNNING)
        {
          TrendReset(pCfg, pData);
        }

        // While in running mode, update trends and autotrends

        TrendUpdateAutoTrend(pCfg, pData);
        TrendUpdateData(pCfg, pData);

        // If the button has been pressed(as defined by StartTrigger)
        // start taking trending-data

        SetBit(pCfg->StartTrigger, trigMask, sizeof(BITARRAY128 ));

        if ( TriggerIsActive(&trigMask) )
        {
          TrendStartManualTrend(pCfg, pData);
        }

        /* set the last engine run state */
        pData->PrevEngState = ER_STATE_RUNNING;
        break;

        // Should never get here!
      default:
        FATAL("Unrecognized Engine Run State: %d", erState);
        break;
    }
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
  if ( !pData->bTrendActive )
  {
    GSE_DebugStr(NORMAL,TRUE, "Trend[%d]: Manual Trend started.",pData->trendIndex );
    pData->bTrendActive = TRUE;
    pData->bIsAutoTrend = FALSE;
    pData->TimeNextSampleMs = CM_GetTickCount() + (pCfg->nSamplePeriod_s * 1000);
    pData->bTrendLamp = pCfg->lampEnabled;

    // todo Dave Init and create a start log

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
  UINT32 timeNow;
  UINT16 i;
  SNSR_SUMMARY* pSummary;
  FLOAT32  oneOverN;

  if ( pData->bTrendActive )
    // Check if an AutoTrend-inprogress has failed to achieve stability
    if( pData->bIsAutoTrend )
    {
      if(0 == pData->nTimeStableMs && pData->cntTrendSamples < pCfg->maxTrendSamples)
      {
        // Auto trend did not reach stability.
        // Force a finish and flag it for autotrend-failure logging.
        pData->cntTrendSamples = pCfg->maxTrendSamples;
        bAutoTrendFailed = TRUE;
      }
    }

    // Did trend complete on last pass?
    if ( pData->cntTrendSamples >= pCfg->maxTrendSamples )
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
      timeNow = CM_GetTickCount();
      if (pData->TimeNextSampleMs <= timeNow )
      {
        // Calculate next sample time
        pData->TimeNextSampleMs = timeNow + ((pCfg->nSamplePeriod_s * 1000) / pCfg->maxTrendSamples);

        // Update monitored-sensors
        // Loop thru all sensors associated with this Trend
        for ( i = 0; i < pData->nTotalSensors; i++ )
        {
          pSummary = &pData->snsrSummary[i];

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
            // Sensor is now invalid but had been valid...
            // calculate average for valid period.
            oneOverN = (1.0f / (FLOAT32)(pData->cntTrendSamples - 1));
            pSummary->fAvgValue = pSummary->fTotal * oneOverN;
          }
        } // for nTotalSensors

        ++pData->cntTrendSamples;

      }// Take a new sample

    }// trend is active


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
#ifndef TREND_UNDER_CONSTRUCTION
  UINT8 i;


  // If we are being called as a check from the mainloop and there
  //   is no trend in progress, then we don't do anything.
  if (pTrendLog != NULL){

    // Otherwise, finish up the trend. Start by calculating the sensor averages.
    for ( i = 0; i < MAX_SENSORS; i++) {
      //If this sensor is not to be included in the trend, then skip it.
      if ((!Sensors[i].bTrendDefined) || (Sensors[i].Type == UNUSED)) {
        continue;
      }

      // For a sensor which has failed, show the output as FAILED_VALUE
      if (!SensorIsValid(i)) {
        pTrendLog->Results[i].fAve = FAILED_VALUE;
        pTrendLog->Results[i].fMax = FAILED_VALUE;
      }
      else {
        // Otherwise, calculate the resultant averages.
        if (Trend.nSensorCounts[i] != 0) {
          pTrendLog->Results[i].fAve = Trend.fSensorTotals[i] / Trend.nSensorCounts[i];
        }
      }
    }

    // Update persistent cycle counter data if present.
    // Next Upgrade enhance this routine.
    GetTrendPersistentData( TrendData.nCycleA, 0);
    GetTrendPersistentData( TrendData.nCycleB, 1);
    GetTrendPersistentData( TrendData.nCycleC, 2);
    GetTrendPersistentData( TrendData.nCycleD, 3);

    // Now do the log details...
    FinishLog ( &trendLogHandle );

    // And reset for the next time.
    InitTrend ();
  } // End of if (pTrendLog != NULL)

#endif

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
  UINT8 j;
#pragma ghs nowarning 1545 //Suppress packed structure alignment warning
  for ( i = 0; i < MAX_TRENDS; i++)
  {
    pData->bTrendActive    = FALSE;
    pData->bIsAutoTrend    = FALSE;
    pData->TimeNextSampleMs  = 0;
    // pData->PrevEngState    = ER_STATE_STOPPED;
    pData->bTrendLamp      = FALSE;
    pData->nTimeStableMs    = 0;
    pData->lastStabCheckMs  = 0;

    pData->nTimeSinceLastMs    = pCfg->TrendInterval_s * ONE_SEC_IN_MILLSECS;
    pData->lastIntervalCheckMs = 0;

    // Reset the stability data
    for ( j = 0; j <  pData->nStability; j++)
    {
      pData->prevStabValue[j] = 0.f;
    }

    // Reset(Init) the monitored sensors
    pData->nTotalSensors = SensorSetupSummaryArray(pData->snsrSummary,
                                                     MAX_TREND_SENSORS,
                                                     pCfg->SensorMap,
                                                     sizeof(pCfg->SensorMap) );
  }
  #pragma ghs endnowarning
}



/******************************************************************************
 * Function: GetTrendPersistentData
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
static void GetTrendPersistentData( UINT8 nCycle, UINT8 nTrendCycleIndex )
{
#ifndef TREND_UNDER_CONSTRUCTION
  UINT8 nErID;
  UINT32 *pCountUINT32;


  if ( nCycle != NO_CYCLE_TREND ) {
    if ( CyclePersistent( nCycle ) ) {
      // Get Persistent Cycle Data
      pTrendLog->CycleCount[nTrendCycleIndex] = RTC_RAMRunTime.d[nCycle].count.n;
    }
    else {
      // Get Standard Cycle Data
      nErID = Cycles[nCycle].nEngineRunId;
      pCountUINT32 = (UINT32 *) &EngineRunData[nErID].pLog->CycleCounts[nCycle];
      pTrendLog->CycleCount[nTrendCycleIndex] = *pCountUINT32;
    }
  }
#endif
}
/******************************************************************************
 * Function: GetTrendPersistentData
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
static void TrendForceEnd(void )
{
  //todo DaveB implement trend closeout for shutdown.
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

  BOOLEAN bStatus = TRUE;
  STABILITY_CRITERIA* pStabCrit;

  // Check all stability sensors for this trend obj.
  for (i = 0; i < pData->nStability; ++i)
  {
    pStabCrit = &pCfg->stability[i];

    if ( SensorIsUsed(pStabCrit->sensorIndex) )
    {
      if ( SensorIsValid(pStabCrit->sensorIndex))
      {
        // get the current sensor value
        fVal       = SensorGetValue(pStabCrit->sensorIndex);
        fPrevValue = pData->prevStabValue[i];

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

        // if any test fails, reset stored sensor value to start over next time
        if ( !bStatus)
        {
          pData->prevStabValue[i] = fVal;
        }
      }
      else
      {
        bStatus = FALSE;
      }
    }
  }

  return bStatus;
}

/******************************************************************************
 * Function: TrendLogAutoTrendFailure
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
static void TrendLogAutoTrendFailure( TREND_CFG* pCfg, TREND_DATA* pData )
{

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
  if( pCfg->ResetTrigger != TRIGGER_UNUSED    &&
      TriggerIsConfigured(pCfg->ResetTrigger) &&
      TriggerGetState(pCfg->ResetTrigger))
  {
    pData->cntTrendSamples  = 0;

    pData->nTimeStableMs    = 0;
    pData->lastStabCheckMs  = 0;

    pData->nTimeSinceLastMs    = pCfg->TrendInterval_s * ONE_SEC_IN_MILLSECS;
    pData->lastIntervalCheckMs = 0;
  }

  timeNow = CM_GetTickCount();

  // Are more trend-samples be taken?
  if( pData->cntTrendSamples < pCfg->maxTrendSamples )
  {
    // Update minimum time between trends
    TrendUpdateTimeElapsed(timeNow,
                           &pData->lastIntervalCheckMs,
                           &pData->nTimeSinceLastMs);

    // Check if all configured criteria are/still stable.
    if (TrendCheckStability(pCfg, pData) )
    {
      // Criteria is stable... update time-elapsed since sensors-stable.
      TrendUpdateTimeElapsed(timeNow, &pData->lastStabCheckMs, &pData->nTimeStableMs);

      // If stability and interval durations have been met and not already processing
      // trend, start one
      if ( (pData->nTimeStableMs     >= (pCfg->nTimeStable_s  * ONE_SEC_IN_MILLSECS)) &&
           (pData->nTimeSinceLastMs >= (pCfg->TrendInterval_s * ONE_SEC_IN_MILLSECS)) &&
            !pData->bTrendActive)
      {
        TrendStartAutoTrend(pCfg, pData);
      }

    }
    else // Criteria sensors non-stable
    {
      // Clear the stability duration for next pass
      pData->lastStabCheckMs = 0;
      pData->nTimeStableMs   = 0;
    }




  }
  else if(pData->cntTrendSamples < pCfg->maxTrendSamples )
  {
    // if we take more trends, then we're here because the interval hasn't been reached

  }
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
 void TrendUpdateTimeElapsed(UINT32 currentTime, UINT32* lastTime, UINT32* elapsedTime)
 {
   // If first time thru, set last time to current so elapsed will be zero.
   if (0 == *lastTime)
   {
     // if elapsed time is zero we are starting a
     *lastTime    = currentTime;
     *elapsedTime = 0;
   }

   // Calculate how long the criteria have been stable
   *elapsedTime += (currentTime - *lastTime);
   *lastTime    =  currentTime;
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
    LOG_PRIORITY_LOW,
    &nTrendStorage,
    sizeof(nTrendStorage),
    NULL);

  GSE_DebugStr ( NORMAL, TRUE, "Event Start (%d) %s ",
    pData->trendIndex, pCfg->trendName );

  // SNSR_SUMMARY s/b init at this point. todo DaveB verify
  pData->bTrendActive = TRUE;
  pData->bIsAutoTrend = TRUE;
  pData->bTrendLamp   = pCfg->lampEnabled;

}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: trend.c $
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


