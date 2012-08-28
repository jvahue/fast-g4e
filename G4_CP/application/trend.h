#ifndef TREND_H
#define TREND_H
/******************************************************************************
                Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
                    All Rights Reserved. Proprietary and Confidential.


    File:        trend.h


    Description: Function prototypes and defines for the trend processing.

  VERSION
  $Revision: 2 $  $Date: 8/15/12 7:20p $

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


/******************************************************************************
                                    Package Defines                           
******************************************************************************/
#define MAX_TREND_NAME    32
#define MAX_STAB_SENSORS  16
#define MAX_TREND_SENSORS 32


//*****************************************************************************
// TREND CONFIGURATION DEFAULT
//*****************************************************************************
#define TREND_DEFAULT "Unused",              /* &TrendName[MAX_TREND_NAME] */\
                      TREND_1HZ,             /* Rate */\
                      0,                     /* nOffset_ms */\
                      0,                     /* nSamplePeriod_s */\
                      ENGRUN_UNUSED,         /* EngineRunId */\
                      0,                     /* nTrendSamples */\
                      TRIGGER_UNUSED,        /* StartTrigger */\
                      TRIGGER_UNUSED,        /* EndTrigger */\
                      0,                     /* TrendTicks */\
                      {0,0,0,0},             /* SensorMap */\
                      CYCLE_UNUSED,          /* nCycleA */\
                      CYCLE_UNUSED,          /* nCycleB */\
                      CYCLE_UNUSED,          /* nCycleC */\
                      CYCLE_UNUSED,          /* nCycleD */\
                      0,                     /* LssMask */\
                      STABILITY_DEFAULT,     /* Stability[MAX_STAB_SENSORS] */\
                      0,                     /* nTimeStable_s */\
                      FALSE                  /* lampEnabled */


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

#pragma pack(1)

typedef struct  
{
  CHAR  Name[ MAX_TREND_NAME + 1 ];    /* name of trend or autotrend  */


}TREND_LOG;

typedef struct 
{
  FLOAT32 Lower;
  FLOAT32 Upper;
  FLOAT32 Variance;
} STABILITY_RANGE;

typedef struct
{
  SENSOR_INDEX     SensorIndex;
  STABILITY_RANGE  Criteria;
}STABILITY_CRITERIA;

typedef struct
{ 
   CHAR          TrendName[MAX_TREND_NAME+1]; /* the name of the trend                       */
   TREND_RATE    Rate;               /* Rate in ms at which trend is run.                    */
   UINT32        nOffset_ms;         /* Offset in millisecs this object runs within it's MIF */
   UINT32        nSamplePeriod_s;    /* # seconds over which a trend will sample (1-3600)    */
   ENGRUN_INDEX  EngineRunId;        /* EngineRun for this trend 0,1,2,3 or ENGINE_ANY       */
   UINT16        nTrendSamples;      /* # of trend-samples to take                           */
   TRIGGER_INDEX StartTrigger;       /* Starting trigger                                     */
   TRIGGER_INDEX EndTrigger;         /* Ending trigger                                       */   
   UINT32        TrendInterval_s;    /* Ticks between samples todo DaveB re-factor to TrendInterval_s 0 - 86400 (24Hrs) */
   BITARRAY128   SensorMap;          /* Bit map of flags of up-to 32 sensors monitored by this Trend 0-127 */   
   CYCLE_INDEX   nCycleA;            /* Persistent Cycle A to include in trend               */
   CYCLE_INDEX   nCycleB;            /* Persistent Cycle B to include in trend               */
   CYCLE_INDEX   nCycleC;            /* Persistent Cycle C to include in trend               */
   CYCLE_INDEX   nCycleD;            /* Persistent Cycle D to include in trend               */
   UINT8         LssMask;            /* Mask of LSS outputs used by this trend when active   */

   STABILITY_CRITERIA Stability[MAX_STAB_SENSORS]; /* Stability criteria for this trend      */
   UINT16        nTimeStable_s;     /* Stability period for sensor(0-3600) in 1sec intervals*/

   BOOLEAN       lampEnabled;        /* will the trend lamp flash                            */
}TREND_CFG, *TREND_CFG_PTR;

// LOGS HERE

// A type for an array of the maximum number of trends
// Used for storing Trend configurations in the configuration manager
typedef TREND_CFG TREND_CONFIGS[MAX_TRENDS];
#pragma pack()

// Run Time Data
typedef struct
{
  
  TREND_INDEX  TrendIndex;    // Index for 'this' trend.
  // Trend state/status info
  BOOLEAN      bTrendActive;  // is a trend active
  BOOLEAN      bIsAutoTrend;  // Is this trend an autotrend
  UINT32       TimeNextSample;// Time to take next sample
  ER_STATE     PrevEngState;  // last op mode for trending
  BOOLEAN      bTrendLamp;    // does a [Auto]Trend want to flash the lamp  
  
  UINT16       nSamples;      // # of samples already taken
  UINT32       nTimeSinceLast;// time since last trend  
 
  // Stability data
  UINT16       nStability;    // Count of sensors used in Stability
  UINT32       nTimeStable_ms;// Time in ms the stability-sensors have been stable.
  FLOAT32      prevStabValue[MAX_STAB_SENSORS];
  
  
  // Monitored sensors during trends.
  UINT16       nTotalSensors; // Count of sensors used in SnsrSummary
  SNSR_SUMMARY SnsrSummary[MAX_TREND_SENSORS]; // Storage for max, average and counts.
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
EXPORT void TrendInitialize( void );
EXPORT void TrendTask( void* pParam );

#endif // TREND_H 
/*************************************************************************
 *  MODIFICATIONS
 *    $History: trend.h $
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





