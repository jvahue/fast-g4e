#ifndef TREND_H
#define TREND_H
/******************************************************************************
             Copyright (C) 2012-2016 Pratt & Whitney Engine Services, Inc.
                    All Rights Reserved. Proprietary and Confidential.

    ECCN:        9D991

    File:        trend.h


    Description: Function prototypes and defines for the trend processing.

  VERSION
  $Revision: 28 $  $Date: 2/22/16 11:39a $

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

//*****************************************************************************
// TREND CONFIGURATION DEFAULT
//*****************************************************************************
#define STABILITY_DEFAULT            SENSOR_UNUSED, /* sensorIndex       */\
                                             FALSE, /* bDelay            */\
                                                 0, /* absOutlier_ms     */\
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
                      FALSE,                      /* bCommanded */\
                      TREND_1HZ,                  /* Rate */\
                      0,                          /* nOffset_ms */\
                      1,                          /* nSamplePeriod_s */\
                      -1,                         /* nOffsetStability_s */\
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
                      STB_STRGY_INCRMNTL,         /* stableStrategy */\
                      STABILITY_CRITERIA_DEFAULT  /* stability[MAX_STAB_SENSORS] */


#define TREND_CFG_DEFAULT TREND_DEFAULT,\
                          TREND_DEFAULT,\
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
   TREND_5      = 5,
   MAX_TRENDS   = 6,
   TREND_UNUSED = 255
} TREND_INDEX;

typedef enum
{
  TR_TYPE_INACTIVE,
  TR_TYPE_STANDARD_MANUAL,
  TR_TYPE_STANDARD_AUTO,
  TR_TYPE_COMMANDED
}TREND_TYPE;  // formerly TREND_STATE

// APAC Commands Proc flags
typedef enum
{
  APAC_CMD_COMPL,
  APAC_CMD_START,
  APAC_CMD_STOP
}APAC_CMD;


// The Stability State of a Commanded Trend
typedef enum
{
  STB_STATE_IDLE,   // The trend is not active or testing for stability
  STB_STATE_READY,  // The trend is started and is checking if stability criteria is met.
  STB_STATE_SAMPLE, // The trend has met stability criteria and is actively sampling.
  MAX_STB_STATE
}STABILITY_STATE;

typedef enum
{
  SMPL_RSLT_UNK = 0,   // The previous sample status has been cleared
  SMPL_RSLT_SUCCESS,   // The previous sample completed.
  SMPL_RSLT_FAULTED    // The previous sample faulted before completion.
}SAMPLE_RESULT;

typedef enum
{
  STAB_UNKNOWN = 0,         // The sensor status is unknown
  STAB_OK,                  // The sensor is OK
  STAB_SNSR_NOT_DECLARED,   // The sensor is not declared
  STAB_SNSR_INVALID,        // The sensor is invalid
  STAB_SNSR_RANGE_ERROR,    // The sensor reading is outside configured range
  STAB_SNSR_VARIANCE_ERROR  // The sensor reading is outside configured variance
} STABILITY_STATUS;

// Stability determination Strategy
typedef enum
{
  STB_STRGY_INCRMNTL = 0, // Default - Compares current sensor value to prev value +/- variance
  STB_STRGY_ABSOLUTE,     // Compares current sensor value to  Absolute base value +/- variance
  MAX_STB_STRGY
}STB_STRATEGY;

typedef enum
{
  TREND_CMD_OK,             // The command has been accepted
  TREND_NOT_CONFIGURED,     // The Trend is not configured
  TREND_NOT_COMMANDABLE     // The Trend is not configured to be commanded.
} TREND_CMD_RESULT;

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
  SENSOR_INDEX sensorIndex;   /* The index of the sensor being checked                    */
  BOOLEAN      bDelayed;      /* Delay testing stability until after nOffsetStability_s   */
  UINT16       absOutlier_ms; /* For STB_STRGY_ABSOLUTE: The time in mSec sensor values   */
                              /* may be outside the variance before considered unstable   */
  STABILITY_RANGE  criteria;  /* Stability criteria for the sensor                        */
}STABILITY_CRITERIA;

typedef struct
{
  UINT16           stableCnt;                   /* Count of stable sensors observed   */
  FLOAT32          snsrValue[MAX_STAB_SENSORS]; /* Prior readings of sensors          */
  BOOLEAN          validity [MAX_STAB_SENSORS]; /* Validity state of each sensor      */
  STABILITY_STATUS status   [MAX_STAB_SENSORS]; /* The status of the stability sensor */
}STABILITY_DATA_LOG;

//****************************
// Logging structures

// A subset of STABILITY_CRITERIA to allow it to vary independently of what the log needs.
typedef struct
{
  SENSOR_INDEX     sensorIndex;  /* The index of the sensor being checked                    */
  STABILITY_RANGE  criteria;     /* Stability criteria for the sensor                        */
}STABILITY_CRIT_LOG;


typedef struct
{
  TREND_INDEX trendIndex;                       /* The Id of this trend                     */
  TREND_TYPE type;                              /* The type/state of trend manual vs auto   */
}TREND_START_LOG;

// TREND_LOG
typedef struct
{
  TREND_INDEX   trendIndex;                      /* The Id of this trend                     */
  TREND_TYPE    type;                            /* The type/state of trend manual vs auto   */
  UINT32        nSamples;                        /* The number of samples taken during trend */
  TREND_SENSORS sensor[MAX_TREND_SENSORS];       /* The stats for the configured sensors     */
  CYCLE_COUNT   cycleCounts[MAX_TREND_CYCLES];   /* The counts of the configured cycles      */
}TREND_LOG;

typedef struct
{
  TREND_INDEX        trendIndex;              /* The Id of this trend                        */
  STABILITY_CRIT_LOG crit[MAX_STAB_SENSORS];  /* The configured criteria range for the trend */
  STABILITY_DATA_LOG data;                    /* The max observed stability data during      */
}TREND_ERROR_LOG;                             /* the UNDETECTED OR FAILED TREND              */

// TREND_CFG
typedef struct
{
   CHAR          trendName[MAX_TREND_NAME+1]; /* the name of the trend                       */

   BOOLEAN       bCommanded;         /* TRUE - trend must be started by API-call        */
                                     /* FALSE - (default) trend is  STANDARD(AUTO or MANUAL) */
   /* Trend execution control */
   TREND_RATE    rate;               /* Rate in ms at which trend is run.                    */
   UINT32        nOffset_ms;         /* Offset in millisecs this object runs within it's MIF */
   /* Sampling control */
   UINT16        nSamplePeriod_s;    /* # seconds over which a trend will sample (1-3600)    */
   INT32         nOffsetStability_s; /* Offset from start of sampling in 1sec intervals      */
   ENGRUN_INDEX  engineRunId;        /* EngineRun for this trend 0,1,2,3 or ENGINE_ANY       */
   UINT16        maxTrends;          /* Max # of stable/auto-trends to be recorded.          */
   TRIGGER_INDEX startTrigger;       /* Starting trigger for STD-MANUAL trend                */
   TRIGGER_INDEX resetTrigger;       /* Ending trigger                                       */
   UINT32        trendInterval_s;    /* 0 - 86400 (24Hrs)                                    */
   BITARRAY128   sensorMap;          /* Bit map of flags of up-to 32 sampled sensors         */
   CYCLE_INDEX   cycle[MAX_TREND_CYCLES]; /* Ids of cycle whose cnt are logged by this trend.*/
   UINT8         nAction;            /* Action to annunciate during the trend                */
   UINT16        stabilityPeriod_s;  /* Stability period for sensor(0-3600) in 1sec intervals*/
   STB_STRATEGY  stableStrategy;     /* Method used to determine stability of sensor         */
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
  UINT16           stableCnt;                   /* Count of stable sensors observed          */
  FLOAT32          snsrValue[MAX_STAB_SENSORS]; /* Prior readings of sensors                 */
  FLOAT32          snsrVar  [MAX_STAB_SENSORS]; /* Variance value when variance exceeded     */
  BOOLEAN          validity [MAX_STAB_SENSORS]; /* Validity state of each sensor             */
  STABILITY_STATUS status   [MAX_STAB_SENSORS]; /* The status of the stability sensor        */
  // Stability duration vars
  UINT32 stableDurMs[MAX_STAB_SENSORS]; /* Number of mSec sensor has been continuously stable*/
  UINT32 lastStableChkMs[MAX_STAB_SENSORS];/* Last (system run time) this sensor was checked */
}STABILITY_DATA;

// Substructure collecting working vars for determining the stability state of a sensor
// using Absolute Stability method.
typedef struct
{
  FLOAT32 lowerValue;      /* min variance value for absolute ref value             */
  FLOAT32 upperValue;      /* max variance value for absolute ref value             */
  UINT16  outLierCnt;      /* current count of consecutive outlier values           */
  UINT16  outLierMax;      /* Max allowed consecutive outliers from CFG             */
//  UINT32  stableDurMs;     /* Number of mSec the sensor has been continuously stable*/
//  UINT32  lastStableChkMs; /* Last time (system run time) this sensor was checked   */
}ABS_STABILITY;



typedef struct
{
  SNSR_SUMMARY snsrSummary[MAX_TREND_SENSORS];
}TREND_SAMPLE_DATA;

// Not-typical typedef used here to fwd decl TREND_DATA.
// This is necessary because TREND_DATA structure contains a member 'pStabilityFunc', which is
// a ptr-to-func containing a param of type TREND_DATA.
typedef struct TREND_DATA TREND_DATA;
typedef BOOLEAN (* STABILITY_FUNC)(TREND_CFG* pCfg, TREND_DATA* pData,
                                   INT32 snsr, BOOLEAN bDoVarCheck);
struct TREND_DATA
{
  // Run control
  TREND_INDEX  trendIndex;          /* Index for 'this' trend.                               */
  INT16        nRateCounts;         /* Countdown interval for this trend                     */
  INT16        nRateCountdown;      /* Countdown in mSec until next execution of this trend  */

  // State/status info
  BOOLEAN      bTrendStabilityFailed;/* Flag used to track if an active AUTO/COMMANDED trend
                                        has failed to maintain stability during SAMPLE       */
  // Reset trigger handling.
  BOOLEAN      bEnabled;            /* For Auto-Trend: Indicates reset-trigger is enabled
                                                       for detection.                        */
  BOOLEAN      bResetInProg;        /* Latch flag for handling Reset processing              */
  // States and counts
  TREND_TYPE   trendState;          /* Type of trend currently executing(i.e. reason started)*/
  ER_STATE     prevEngState;        /* last op mode for trending                             */
  UINT16       trendCnt;            /* # of all trends(AUTO, MANUAL & COMMANDED) since Reset */
  UINT16       autoTrendCnt;        /* # of AUTO trends since Reset(0 -> TREND_CFG.maxTrends)*/
  // Action Manager ID
  INT8         nActionReqNum;       /* Action Id for the LSS Request                         */
  // Trend instance sampling
  UINT32       nSamplesPerPeriod;   /* The number of samples taken during a sampling period  */
  UINT32       masterSampleCnt;     /* Counts of samples taken this period. Each SNSR_SUMMARY
                                       has it own value which matches this value             */
  // Interval handling
  UINT32       lastIntervalCheckMs; /* Starting time (CM_GetTickCount()                      */
  UINT32       timeSinceLastTrendMs;/* time since last trend                                 */

  // Stability handling
  STABILITY_FUNC pStabilityFunc;    /* Ptr to function used to verify stability              */
  UINT16         nStabExpectedCnt;  /* Expected count(# of configured stability sensors)     */
  UINT32         lastStabCheckMs;   /* Starting time (CM_GetTickCount()                      */
  UINT32         nTimeStableMs;     /* Time in ms ALL stability-sensors have been stable.    */
  STABILITY_DATA curStability;      /* Current stability                                     */
  STABILITY_DATA maxStability;      /* Max observed stable sensors and count                 */

  // when STB_STRGY_ABSOLUTE is defined, working data for tracking a stability sensor
  ABS_STABILITY absoluteStability[MAX_STAB_SENSORS];

  // Monitored sensors during trends.
  UINT16       nTotalSensors;                  /* Count of sensors used in SnsrSummary       */
  SNSR_SUMMARY snsrSummary[MAX_TREND_SENSORS]; /* Storage for max, average and counts.       */
  SNSR_SUMMARY endSummary [MAX_TREND_SENSORS]; /* Storage for stats during  end-phase        */
  UINT32       delayedStbStartMs;              /* Tick count after which delayed stb sensrs are
                                                  checked                                    */
  // Stability State info.
  STABILITY_STATE stableState;            /* The current stability state of the trend        */
  STABILITY_STATE prevStableState;        /* The prior stability state of the trend          */

  UINT32        stableStateStartMs;       /* Starting time CM_GetTickCount() of  stableState */
  SAMPLE_RESULT lastSampleResult;         /* The result of the last attempt to Sample        */
  // Commanded trend stable hist.
  UINT32 cmdStabilityDurMs;               /* Dur trend was stable(taken from nTimeStableMs)  */

  // APAC command pending
  APAC_CMD      apacCmd;                  /* APAC command to be processed.                   */
 }; //typedef struct TREND_DATA


// APAC sensor stability history
typedef struct
{
  SENSOR_INDEX sensorID[MAX_STAB_SENSORS];
  BOOLEAN      bStable[MAX_STAB_SENSORS];
}STABLE_HISTORY;


#if defined (TREND_BODY)
// Trend index enumeration for other modules which ref TrendIndex
USER_ENUM_TBL trendIndexType[] =
{ { "0"  , TREND_0   }, { "1"  , TREND_1   }, { "2"  , TREND_2   },
  { "3"  , TREND_3   }, { "4"  , TREND_4   }, { "5"  , TREND_5   },
  { "UNUSED", TREND_UNUSED }, { NULL, 0 }
};
#else
// Export the triggerIndex enum table
EXPORT USER_ENUM_TBL trendIndexType[];
#endif


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

// APAC interface functions
EXPORT BOOLEAN TrendAppStartTrend( TREND_INDEX idx, BOOLEAN bStart, TREND_CMD_RESULT* rslt);
EXPORT void    TrendGetStabilityState( TREND_INDEX idx, STABILITY_STATE* pStabState,
                                       UINT32* pDurMs,  SAMPLE_RESULT* pSampleResult);
EXPORT BOOLEAN TrendGetStabilityHistory( TREND_INDEX idx, STABLE_HISTORY* pSnsrStableHist);
EXPORT void    TrendGetSampleData      ( TREND_INDEX idx, TREND_SAMPLE_DATA* pStabilityData);

#endif // TREND_H
/*************************************************************************
 *  MODIFICATIONS
 *    $History: trend.h $
 * 
 * *****************  Version 28  *****************
 * User: John Omalley Date: 2/22/16    Time: 11:39a
 * Updated in $/software/control processor/code/application
 * SCR 1300 - Code Review Update
 * 
 * *****************  Version 27  *****************
 * User: Contractor V&v Date: 12/11/15   Time: 5:29p
 * Updated in $/software/control processor/code/application
 * SCR #1300 Trend.debug section and stablie duration changed to all
 * 
 * *****************  Version 26  *****************
 * User: Contractor V&v Date: 12/07/15   Time: 3:10p
 * Updated in $/software/control processor/code/application
 * SCR #1300 - CR compliance update, enabled trend debug cmds
 *
 * *****************  Version 25  *****************
 * User: Contractor V&v Date: 12/03/15   Time: 6:04p
 * Updated in $/software/control processor/code/application
 * SCR #1300 CR compliance changes
 *
 * *****************  Version 24  *****************
 * User: Contractor V&v Date: 11/30/15   Time: 3:34p
 * Updated in $/software/control processor/code/application
 * CodeReview compliance change - no logic changes.
 *
 * *****************  Version 23  *****************
 * User: Contractor V&v Date: 11/09/15   Time: 2:58p
 * Updated in $/software/control processor/code/application
 * Fixed abs stable count off-by-one bug and consolidation struct
 *
 * *****************  Version 22  *****************
 * User: Contractor V&v Date: 11/05/15   Time: 6:38p
 * Updated in $/software/control processor/code/application
 * SCR #1300 - Formal Implementation of variance outlier tolerance
 *
 * *****************  Version 21  *****************
 * User: Contractor V&v Date: 11/02/15   Time: 5:50p
 * Updated in $/software/control processor/code/application
 * SCR #1300 - StabilityHistory and cmd buffering
 *
 * *****************  Version 20  *****************
 * User: Contractor V&v Date: 10/21/15   Time: 7:07p
 * Updated in $/software/control processor/code/application
 * SCR #1300 - StabilityHistory and cmd buffering
 *
 * *****************  Version 19  *****************
 * User: Contractor V&v Date: 10/19/15   Time: 6:27p
 * Updated in $/software/control processor/code/application
 * SCR #1300 - APAC updates for API and stability calc
 *
 * *****************  Version 18  *****************
 * User: Peter Lee    Date: 15-10-13   Time: 1:43p
 * Updated in $/software/control processor/code/application
 * SCR #1304 APAC Processing Initial Check In
 *
 * *****************  Version 17  *****************
 * User: Contractor V&v Date: 10/05/15   Time: 6:35p
 * Updated in $/software/control processor/code/application
 * SCR #1300 - P100 Trend Processing Updates initial update
 *
 * *****************  Version 16  *****************
 * User: Jeff Vahue   Date: 1/26/13    Time: 3:12p
 * Updated in $/software/control processor/code/application
 * SCR# 1197 - proper handling of stability sensor variance failure
 * reporting
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





