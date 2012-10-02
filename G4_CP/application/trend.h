#ifndef TREND_H
#define TREND_H
/******************************************************************************
                Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
                    All Rights Reserved. Proprietary and Confidential.


    File:        trend.h


    Description: Function prototypes and defines for the trend processing.

  VERSION
  $Revision: 8 $  $Date: 12-10-02 1:19p $

*******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_basic.h"
#include "cycle.h"
#include "EngineRun.h"
#include "sensor.h"
#include "ActionManager.h"


/******************************************************************************
                                    Package Defines
******************************************************************************/
#define MAX_TREND_NAME    32
#define MAX_STAB_SENSORS  16
#define MAX_TREND_SENSORS 32
#define MAX_TREND_CYCLES   4

#define ONE_SEC_IN_MILLSECS      1000u


//*****************************************************************************
// TREND CONFIGURATION DEFAULT
//*****************************************************************************
#define STABILITY_DEFAULT            SENSOR_UNUSED, /* sensorIndex       */\
                                               0.f, /* criteria.lower    */\
                                               0.f, /* criteria.uper     */\
                                               0.f  /* criteria.variance */


#define STABILITY_CRITERIA_DEFAULT STABILITY_DEFAULT,\
                                   STABILITY_DEFAULT,\
                                   STABILITY_DEFAULT,\
                                   STABILITY_DEFAULT,\
                                   STABILITY_DEFAULT,\
                                   STABILITY_DEFAULT,\
                                   STABILITY_DEFAULT,\
                                   STABILITY_DEFAULT,\
                                   STABILITY_DEFAULT,\
                                   STABILITY_DEFAULT,\
                                   STABILITY_DEFAULT,\
                                   STABILITY_DEFAULT,\
                                   STABILITY_DEFAULT,\
                                   STABILITY_DEFAULT,\
                                   STABILITY_DEFAULT,\
                                   STABILITY_DEFAULT

#define TREND_DEFAULT "Unused",                   /* &trendName[MAX_TREND_NAME] */\
                      TREND_1HZ,                  /* Rate */\
                      0,                          /* nOffset_ms */\
                      0,                          /* nSamplePeriod_s */\
                      ENGRUN_UNUSED,              /* engineRunId */\
                      0,                          /* maxTrends */\
                      TRIGGER_UNUSED,             /* startTrigger */\
                      TRIGGER_UNUSED,             /* resetTrigger */\
                      0,                          /* trendInterval_s */\
                      {0,0,0,0},                  /* sensorMap */\
                      CYCLE_UNUSED,               /* nCycleA */\
                      CYCLE_UNUSED,               /* nCycleB */\
                      CYCLE_UNUSED,               /* nCycleC */\
                      CYCLE_UNUSED,               /* nCycleD */\
                      0x00000000,                 /* nAction */\
                      0,                          /* stabilityPeriod_s */\
                      FALSE,                      /* lampEnabled */\
                      STABILITY_CRITERIA_DEFAULT /* Stability[MAX_STAB_SENSORS] */


#define TREND_CFG_DEFAULT TREND_DEFAULT,\
                          TREND_DEFAULT,\
                          TREND_DEFAULT,\
                          TREND_DEFAULT,\
                          TREND_DEFAULT


/*********************************************************************************************
                                   Package Typedefs
*********************************************************************************************/
typedef enum
{
   TREND_1HZ  =  1,  /*  1Hz Rate    */
   TREND_2HZ  =  2,  /*  2Hz Rate    */
   TREND_4HZ  =  4,  /*  4Hz Rate    */
   TREND_5HZ  =  5,  /*  5Hz Rate    */
   TREND_10HZ = 10,  /* 10Hz Rate    */
   TREND_20HZ = 20,  /* 20Hz Rate    */
   TREND_50HZ = 50   /* 50Hz Rate    */
} TREND_RATE;

typedef enum
{
   TREND_0      = 0,
   TREND_1      = 1,
   TREND_2      = 2,
   TREND_3      = 3,
   TREND_4      = 4,
   MAX_TRENDS   = 5,
   TREND_UNUSED = 255
} TREND_INDEX;

typedef enum
{
  TREND_STATE_INACTIVE,
  TREND_STATE_MANUAL,
  TREND_STATE_AUTO,
  MAX_TREND_STATES
}TREND_STATE;

#pragma pack(1)



typedef struct
{
  CYCLE_INDEX cycIndex;
  UINT32      cycleCount;
} CYCLE_COUNT;

typedef struct
{
  SENSOR_INDEX SensorIndex;  
  BOOLEAN      bValid;
  FLOAT32      fMaxValue;
  FLOAT32      fAvgValue;
}TREND_SENSORS;

// STABILITY_CRITERIA
typedef struct
{
  FLOAT32 lower;
  FLOAT32 upper;
  FLOAT32 variance;
} STABILITY_RANGE;

typedef struct
{
  SENSOR_INDEX     sensorIndex;
  STABILITY_RANGE  criteria;
}STABILITY_CRITERIA;

typedef struct  
{
  UINT16       stableCnt;                       /* Count of stable sensors                   */
  FLOAT32      prevStabValue[MAX_STAB_SENSORS]; /* Prior readings of sensors                 */
}STABILITY_DATA;

// TREND_LOG
typedef struct  
{
  TREND_STATE   type;                            /* The type/state of trend manual vs auto   */
  UINT32        nSamples;                        /* The number of samples taken during trend */
  TREND_SENSORS sensor[MAX_TREND_SENSORS];       /* The stats for the configured sensors     */
  CYCLE_COUNT   cycleCounts[MAX_TREND_CYCLES];   /* The counts of the configured cycles      */
}TREND_LOG;

typedef struct  
{
  STABILITY_CRITERIA crit[MAX_STAB_SENSORS];  /* The configured criteria range for the trend */
  STABILITY_DATA     data;                    /* The max observed stability data during      */
}TREND_NOT_DETECTED_LOG;                      /* the un-activated trend                      */


// TREND_CFG
typedef struct
{
   CHAR          trendName[MAX_TREND_NAME+1]; /* the name of the trend                       */
   /* Trend execution control */
   TREND_RATE    rate;               /* Rate in ms at which trend is run.                    */
   UINT32        nOffset_ms;         /* Offset in millisecs this object runs within it's MIF */
   /* Sampling control */
   UINT16        nSamplePeriod_s;    /* # seconds over which a trend will sample (1-3600)    */
   ENGRUN_INDEX  engineRunId;        /* EngineRun for this trend 0,1,2,3 or ENGINE_ANY       */
   UINT16        maxTrends;          /* Max # of autotrends to be recorded by this trend     */
   TRIGGER_INDEX startTrigger;       /* Starting trigger                                     */
   TRIGGER_INDEX resetTrigger;       /* Ending trigger                                       */
   UINT32        trendInterval_s;    /* 0 - 86400 (24Hrs)                                    */
   BITARRAY128   sensorMap;          /* Bit map of flags of up-to 32 sensors for this Trend  */   
   CYCLE_INDEX   cycle[MAX_TREND_CYCLES]; /* Ids of cycle whose cnt are logged by this trend.*/   
   UINT32        nAction;           /* Mask of LSS outputs used by this trend when active   */
   UINT16        stabilityPeriod_s;  /* Stability period for sensor(0-3600) in 1sec intervals*/
   BOOLEAN       lampEnabled;        /* will the trend lamp flash                            */
   STABILITY_CRITERIA stability[MAX_STAB_SENSORS]; /* Stability criteria for this trend      */
}TREND_CFG, *TREND_CFG_PTR;

// A typedef for an array of the maximum number of trends
// Used for storing Trend configurations in the configuration manager
typedef TREND_CFG TREND_CONFIGS[MAX_TRENDS];

typedef struct
{
   CHAR          trendName[MAX_TREND_NAME];
   TREND_RATE    rate;
   UINT16        samplePeriod;
   UINT16        maxTrends;
   UINT32        trendInterval;
   UINT16        nTimeStable_s;
   TRIGGER_INDEX StartTrigger;
   TRIGGER_INDEX ResetTrigger;
}TREND_HDR;

#pragma pack()


// TREND_DATA - Run Time Data
typedef struct
{
  // Run control
  TREND_INDEX  trendIndex;          /* Index for 'this' trend.                               */
  INT16        nRateCounts;         /* Countdown interval for this trend                     */
  INT16        nRateCountdown;      /* Countdown in msec until next execution of this trend  */
  
  // State/status info
  TREND_STATE  trendState;          /* Current trend type                                    */
  ER_STATE     prevEngState;        /* last op mode for trending                             */
  BOOLEAN      bTrendLamp;          /* does a [Auto]Trend want to flash the lamp             */
  UINT16       trendCnt;            /* # of autotrends taken since Reset                     */
  BOOLEAN      bResetDetected;      /* Latch flag for handling Reset detection               */
  INT8         nActionReqNum;       /* Action Id for the LSS Request                         */
  
  // Trend instance sampling
  UINT32       nSamplesPerPeriod;   /* The number of samples taken during a sampling period  */
  UINT32       sampleCnt;           /* Counts of samples take this period                    */          
  
  // Interval handling
  UINT32       lastIntervalCheckMs; /* Starting time (CM_GetTickCount()                      */
  UINT32       TimeSinceLastTrendMs;/* time since last trend                                 */
  
  // Stability handling
  UINT16       nStabExpectedCnt;  /* Expected count based on configured.                     */
  UINT32       lastStabCheckMs;   /* Starting time (CM_GetTickCount()                        */
  UINT32       nTimeStableMs;     /* Time in ms the stability-sensors have been stable.      */
  STABILITY_DATA stability;       /* Current stability                                       */
  STABILITY_DATA maxStability;    /* Max observed stable sensors and count                   */
  
  // Monitored sensors during trends.
  UINT16       nTotalSensors;     /* Count of sensors used in SnsrSummary                    */
  SNSR_SUMMARY snsrSummary[MAX_TREND_SENSORS]; /* Storage for max, average and counts.       */
} TREND_DATA;

/******************************************************************************
                                      Package Exports
******************************************************************************/
#undef EXPORT

#if defined( TREND_BODY )
#define EXPORT
#else
#define EXPORT extern
#endif

/******************************************************************************
                                  Package Exports Variables
******************************************************************************/

/******************************************************************************
                                  Package Exports Functions
*******************************************************************************/
EXPORT void   TrendInitialize   ( void );
EXPORT void   TrendTask         ( void* pParam );
EXPORT UINT16 TrendGetBinaryHdr ( void *pDest, UINT16 nMaxByteSize );

#endif // TREND_H
/*************************************************************************
 *  MODIFICATIONS
 *    $History: trend.h $
 * 
 * *****************  Version 8  *****************
 * User: Contractor V&v Date: 12-10-02   Time: 1:19p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 Implement Trend Action
 * 
 * *****************  Version 7  *****************
 * User: Contractor V&v Date: 12-09-19   Time: 6:48p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2  Reset trigger handling
 * 
 * *****************  Version 6  *****************
 * User: Contractor V&v Date: 12-09-19   Time: 3:22p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2  Fix AutoTrends
 * 
 * *****************  Version 5  *****************
 * User: Contractor V&v Date: 9/14/12    Time: 4:07p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 Trend code update
 *
 * *****************  Version 4  *****************
 * User: John Omalley Date: 12-09-11   Time: 2:20p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - General Trend Development and Binary ETM Header
 *
 * *****************  Version 3  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 12:43p
 * Updated in $/software/control processor/code/application
 * SCR# 1142
 *
 * *****************  Version 2  *****************
 * User: Contractor V&v Date: 8/15/12    Time: 7:20p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 Checking in prelim trends
 *
 * *****************  Version 1  *****************
 * User: John Omalley Date: 12-06-07   Time: 11:32a
 * Created in $/software/control processor/code/application
 *
 *
 *
 ***************************************************************************/





