#define TREND_BODY
/******************************************************************************
           Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
              All Rights Reserved. Proprietary and Confidential.

   File:        trend.c

   Description:


   Note:

 VERSION
 $Revision: 4 $  $Date: 8/28/12 12:43p $

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
static struct
{
  UINT32   TrendTime;         /* Time  of next sample         */
  UINT16   nSamples;          /* # of samples already taken   */
  UINT16   nEndTrendCount;    /* when to stop taking samples  */
  UINT16   nUpdateTicks;
  BOOLEAN  bIsAutoTrend;
  UINT8    nAutoTrendIndex;
  FLOAT32  fSensorTotals[MAX_SENSORS];
  UINT16   nSensorCounts[MAX_SENSORS];
} Trend;

static TREND_CFG   m_TrendCfg [MAX_TRENDS];   // Trend cfg
static TREND_DATA  m_TrendData[MAX_TRENDS];   // Runtime trend data
static TREND_LOG   m_TrendLog[MAX_TRENDS];

#include "TrendUserTables.c"

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void TrendProcess         ( void );

static void TrendFinish          ( void );


static void TrendStartManualTrend( void );
static void TrendStartAutoTrend  ( UINT8 nIndex );
static void TrendUpdateData      ( void );

static void TrendForceClose      ( void );

static BOOLEAN TrendIsActive     ( void );
static BOOLEAN TrendCheckCriteria( TREND_CFG* pTrCfg, TREND_DATA* pTrData);

static void TrendGetPersistentData();


static void TrendReset(TREND_CFG* pTrCfg, TREND_DATA* pTrData);

static void TrendForceEnd(void );
static void TrendEnableTask(BOOLEAN bEnable);


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
  TCB TaskInfo; 
  UINT8     i;
  
  // Add user commands for Trend to the user command tables.
  User_AddRootCmd(&RootTrendMsg);

  // Clear and Load the current cfg info.
  memset(&m_TrendData, 0, sizeof(m_TrendData));
  
  memcpy(m_TrendCfg,
    &(CfgMgr_RuntimeConfigPtr()->TrendConfigs),
    sizeof(m_TrendCfg));

  for (i = 0; i < MAX_TRENDS; i++)
  {
    m_TrendData[i].TrendIndex     = i;
    m_TrendData[i].bTrendActive   = FALSE;
    m_TrendData[i].bIsAutoTrend   = FALSE;
    m_TrendData[i].TimeNextSample = 0;
    m_TrendData[i].PrevEngState   = ER_STATE_STOPPED;  
    m_TrendData[i].bTrendLamp     = FALSE;

    // Init the list of the sensors monitored by this trend.
    m_TrendData[i].nTotalSensors =
      SensorSetupSummaryArray( m_TrendData[i].SnsrSummary,
                               MAX_TREND_SENSORS,
                               m_TrendCfg[i].SensorMap,
                               sizeof(m_TrendCfg[i].SensorMap) );
  }


#ifndef TREND_UNDER_CONSTRUCTION
  /* Set time of next sample to initial offset. */
  Trend.TrendTime = TrendData.TrendOffset;
  /* Set number of samples to zero. */
  Trend.nSamples  = 0;
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
  memset(&TaskInfo, 0, sizeof(TaskInfo));
  strncpy_safe(TaskInfo.Name, sizeof(TaskInfo.Name),"Trend Task",_TRUNCATE);
  TaskInfo.TaskID           = Trend_Task;
  TaskInfo.Function         = TrendTask;
  TaskInfo.Priority         = taskInfo[Trend_Task].priority;
  TaskInfo.Type             = taskInfo[Trend_Task].taskType;
  TaskInfo.modes            = taskInfo[Trend_Task].modes;
  TaskInfo.MIFrames         = taskInfo[Trend_Task].MIFframes;
  TaskInfo.Enabled          = TRUE;
  TaskInfo.Locked           = FALSE;

  TaskInfo.Rmt.InitialMif   = taskInfo[Trend_Task].InitialMif;
  TaskInfo.Rmt.MifRate      = taskInfo[Trend_Task].MIFrate;
  TaskInfo.pParamBlock      = NULL;
  TmTaskCreate (&TaskInfo);
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

  for(i = 0; i < MAX_TRENDS; ++i )
  {
    if (TRUE == m_TrendData[i].bTrendActive)
    {
      bTrendActive = TRUE;
      break;
    }
  }

  return bTrendActive;
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
  TREND_CFG*  pTrendCfg;
  TREND_DATA* pTrendData;
  UINT8       erMask;
  BITARRAY128 trigMask;

  CLR_TRIGGER_FLAGS(trigMask);
  
  for (trendIdx = 0; trendIdx < MAX_TRENDS; ++trendIdx)
  {
    // Process each Trend based on trend state according to Engrun state.
    pTrendCfg  = &m_TrendCfg[trendIdx];
    pTrendData = &m_TrendData[trendIdx];

    // Get the state of the associated EngineRun
    erState = EngRunGetState(pTrendCfg->EngineRunId, &erMask);   

    switch ( erState )
    {
      case ER_STATE_STOPPED:
        // If we have entered stopped mode from some other mode, 
        // finish any outstanding trends
        if (pTrendData->PrevEngState != ER_STATE_STOPPED)
        {
          TrendFinish();
        }      
        pTrendData->PrevEngState = ER_STATE_STOPPED;
        break;

      case ER_STATE_STARTING:
        // If we have entered starting mode from some other mode,
        // finish any outstanding trends
        if (pTrendData->PrevEngState != ER_STATE_STARTING)
        {
          TrendFinish();
        }
        pTrendData->PrevEngState = ER_STATE_STARTING;
        break;


      case ER_STATE_RUNNING:
        
        // Upon entering RUNNING state, reset the auto-trending
        if (pTrendData->PrevEngState != ER_STATE_RUNNING)
        {
          TrendReset(pTrendCfg, pTrendData);
        }

        // While in running mode, update trends and autotrends
        
        //TrendUpdateAutoTrends();
        //TrendUpdateData();

        // If the button has been pressed(as defined by StartTrigger)
        // start taking trending-data

        SetBit(pTrendCfg->StartTrigger, trigMask, sizeof(BITARRAY128 ));
       
        if ( TriggerIsActive(&trigMask) )
        {
          TrendStartManualTrend();
        }

        /* set the last engine run state */
        pTrendData->PrevEngState = ER_STATE_RUNNING;
        break;

        // Should never get here!
      default:
        FATAL("Unrecognized Engine Run State: %d", erState);
        break;
    }
  }
}

/******************************************************************************
 * Function: TrendStart
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
static void TrendStartManualTrend( void )
{

}

/******************************************************************************
 * Function: TrendStartAutoTrend
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
static void TrendStartAutoTrend( UINT8 nIndex )
{ 

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
static void TrendUpdateData( void )
{ 

}



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
static void TrendFinish( void )
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
static void TrendReset(TREND_CFG* pTrCfg, TREND_DATA* pTrData)
{

  UINT8 i;
  UINT8 j;

  for ( i = 0; i < MAX_TRENDS; i++)
  {    
    pTrData->nSamples        = 0;
    pTrData->nTimeStable_ms  = 0;
    pTrData->nTimeSinceLast  = pTrCfg->TrendInterval_s;
    pTrData->bIsAutoTrend    = FALSE;

    for ( j = 0; j <  pTrData->nStability; j++)
    {
      pTrData->prevStabValue[j] = 0.0;
    }
  }
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
 * Function: TrendCheckCriteria
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
static BOOLEAN TrendCheckCriteria( TREND_CFG* pTrCfg, TREND_DATA* pTrData)
{
  FLOAT32 fVal;
  FLOAT32 fPrevValue;
  FLOAT32 delta;
  INT32   i;  
 
  BOOLEAN bStatus = TRUE;
  STABILITY_CRITERIA* pStabCrit;
  
  // Check all stability sensors for this trend obj.
  for (i = 0; i < pTrData->nStability; ++i)
  {
    pStabCrit = &pTrCfg->Stability[i];
    
    if ( SensorIsUsed(pStabCrit->SensorIndex) )
    {
      if ( SensorIsValid(pStabCrit->SensorIndex))
      {
        // get the current sensor value
        fVal       = SensorGetValue(pStabCrit->SensorIndex);
        fPrevValue = pTrData->prevStabValue[i];

        // Check absolute min/max values        
        bStatus = (fVal >= pStabCrit->Criteria.Lower) &&
                  (fVal <= pStabCrit->Criteria.Upper);
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

          bStatus = (delta < pStabCrit->Criteria.Variance);
        }

        // if any test fails, reset stored sensor value to start over next time
        if ( !bStatus)
        {
          pTrData->prevStabValue[i] = fVal;
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


/*************************************************************************
 *  MODIFICATIONS
 *    $History: trend.c $
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


