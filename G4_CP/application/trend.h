#ifndef TREND_H
#define TREND_H
/******************************************************************************
                Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
                    All Rights Reserved. Proprietary and Confidential.


    File:        trend.h


    Description: Function prototypes and defines for the trend processing.

  VERSION
  $Revision: 15 $  $Date: 12/12/12 4:30p $

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

#define ONE_SEC_IN_MILLSECS  1000u
#define SECS_PER_DAY         86400


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
                      1,                          /* nSamplePeriod_s */\
                      ENGRUN_UNUSED,              /* engineRunId */\
                      1,                          /* maxTrends */\
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
  TREND_STATE_AUTO
}TREND_STATE;

typedef enum
{
  STAB_UNKNOWN = 0,         // The sensor status is unknown
  STAB_OK,                  // The sensor is OK
  STAB_SNSR_NOT_DECLARED,   // The sensor is not declared
  STAB_SNSR_INVALID,        // The sensor is invalid
  STAB_SNSR_RANGE_ERROR,    // The sensor reading is outside configured range
  STAB_SNSR_VARIANCE_ERROR  // The sensor reading is outside configured variance
} STABILITY_STATUS;



#pragma pack(1)

//***************************************
// Trend data collection  sub-structures
typedef struct
{
  CYCLE_INDEX cycIndex;
  UINT32      cycleCount;
} CYCLE_COUNT;

typedef struct
{
  SENSOR_INDEX sensorIndex;
  BOOLEAN      bValid;
  FLOAT32      fMaxValue;
  FLOAT32      fAvgValue;
}TREND_SENSORS;


//***************************************
// Stability structure and sub-structures

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
  UINT16           stableCnt;                   /* Count of stable sensors observed      */
  FLOAT32          snsrValue[MAX_STAB_SENSORS]; /* Prior readings of sensors             */
  BOOLEAN          validity [MAX_STAB_SENSORS]; /* Validity state of each sensor         */
  STABILITY_STATUS status   [MAX_STAB_SENSORS];    /* The status of the stability sensor */
}STABILITY_DATA;



//****************************
// Logging structures

typedef struct
{
   TREND_INDEX trendIndex;                       /* The Id of this trend                     */
   TREND_STATE type;                             /* The type/state of trend manual vs auto   */
}TREND_START_LOG;

// TREND_LOG
typedef struct
{
  TREND_INDEX   trendIndex;                      /* The Id of this trend                     */
  TREND_STATE   type;                            /* The type/state of trend manual vs auto   */
  UINT32        nSamples;                        /* The number of samples taken during trend */
  TREND_SENSORS sensor[MAX_TREND_SENSORS];       /* The stats for the configured sensors     */
  CYCLE_COUNT   cycleCounts[MAX_TREND_CYCLES];   /* The counts of the configured cycles      */
}TREND_LOG;

typedef struct
{
  TREND_INDEX        trendIndex;              /* The Id of this trend                        */
  STABILITY_CRITERIA crit[MAX_STAB_SENSORS];  /* The configured criteria range for the trend */
  STABILITY_DATA     data;                    /* The max observed stability data during      */
}TREND_ERROR_LOG;                             /* the UNDETECTED OR FAILED TREND              */


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
   UINT16        maxTrends;          /* Max # of stable/auto trends to be recorded.          */
   TRIGGER_INDEX startTrigger;       /* Starting trigger                                     */
   TRIGGER_INDEX resetTrigger;       /* Ending trigger                                       */
   UINT32        trendInterval_s;    /* 0 - 86400 (24Hrs)                                    */
   BITARRAY128   sensorMap;          /* Bit map of flags of up-to 32 sensors for this Trend  */
   CYCLE_INDEX   cycle[MAX_TREND_CYCLES]; /* Ids of cycle whose cnt are logged by this trend.*/
   UINT32        nAction;            /* Action to annunciate during the trend                */
   UINT16        stabilityPeriod_s;  /* Stability period for sensor(0-3600) in 1sec intervals*/
   STABILITY_CRITERIA stability[MAX_STAB_SENSORS]; /* Stability criteria for this trend      */
}TREND_CFG;

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
   TRIGGER_INDEX startTrigger;
   TRIGGER_INDEX resetTrigger;
}TREND_HDR;

#pragma pack()

//****************************
// TREND_DATA - Run Time Data
typedef struct
{
  // Run control
  TREND_INDEX  trendIndex;          /* Index for 'this' trend.                               */
  INT16        nRateCounts;         /* Countdown interval for this trend                     */
  INT16        nRateCountdown;      /* Countdown in msec until next execution of this trend  */

  // State/status info
  BOOLEAN      bAutoTrendStabFailed;/* Flag used to track an autotrend has failed            */

  // Reset trigger handling.
  BOOLEAN      bEnabled;            /* Indicates rst-trigger is enabled for detection        */
  BOOLEAN      bResetInProg;        /* Latch flag for handling Reset processing              */
  // States and counts
  TREND_STATE  trendState;          /* Current trend type                                    */
  ER_STATE     prevEngState;        /* last op mode for trending                             */
  UINT16       trendCnt;            /* # of all trends ( stability and trigger since Reset   */
  UINT16       autoTrendCnt;        /* # of autotrends since Reset (0 -> TREND_CFG.maxTrends)*/
  // Action Manager ID
  INT8         nActionReqNum;       /* Action Id for the LSS Request                         */
  // Trend instance sampling
  UINT32       nSamplesPerPeriod;   /* The number of samples taken during a sampling period  */
  UINT32       masterSampleCnt;     /* Counts of samples take this period each SNSR_SUMMARY
                                       has it own value which matches this value             */

  // Interval handling
  UINT32       lastIntervalCheckMs; /* Starting time (CM_GetTickCount()                      */
  UINT32       timeSinceLastTrendMs;/* time since last trend                                 */

  // Stability handling
  UINT16       nStabExpectedCnt;  /* Expected count based on configured.                     */
  UINT32       lastStabCheckMs;   /* Starting time (CM_GetTickCount()                        */
  UINT32       nTimeStableMs;     /* Time in ms the stability-sensors have been stable.      */
  STABILITY_DATA curStability;    /* Current stability                                       */
  STABILITY_DATA maxStability;    /* Max observed stable sensors and count                   */

  // Monitored sensors during trends.
  UINT16       nTotalSensors;     /* Count of sensors used in SnsrSummary                    */
  SNSR_SUMMARY snsrSummary[MAX_TREND_SENSORS]; /* Storage for max, average and counts.       */
} TREND_DATA;

/******************************************************************************
                                      Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( TREND_BODY )
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
EXPORT UINT16 TrendGetBinaryHdr ( void *pDest, UINT16 nMaxByteSize );

#endif // TREND_H
/*************************************************************************
 *  MODIFICATIONS
 *    $History: trend.h $
 *
 * *****************  Version 15  *****************
 * User: Contractor V&v Date: 12/12/12   Time: 4:30p
 * Updated in $/software/control processor/code/application
 * SCR #1107 Code Review
 *
 * *****************  Version 14  *****************
 * User: Contractor V&v Date: 11/29/12   Time: 4:19p
 * Updated in $/software/control processor/code/application
 * SCR #1200 Requirements: GSE command to view the number of trend
 *
 * *****************  Version 13  *****************
 * User: Contractor V&v Date: 11/26/12   Time: 12:33p
 * Updated in $/software/control processor/code/application
 * SCR #1167 SensorSummary
 *
 * *****************  Version 12  *****************
 * User: Melanie Jutras Date: 12-11-01   Time: 1:58p
 * Updated in $/software/control processor/code/application
 * SCR #1142 File Format Error
 *
 * *****************  Version 11  *****************
 * User: John Omalley Date: 12-10-30   Time: 5:48p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Changed Actions to UINT8
 *
 * *****************  Version 10  *****************
 * User: Contractor V&v Date: 10/30/12   Time: 4:01p
 * Updated in $/software/control processor/code/application
 *
 * *****************  Version 9  *****************
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





