#ifndef EVENT_H
#define EVENT_H
/******************************************************************************
                Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
                    All Rights Reserved. Proprietary and Confidential.


    File:        event.h


    Description: Function prototypes and defines for the event processing.

  VERSION
  $Revision: 30 $  $Date: 12-11-12 4:46p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "actionmanager.h"
#include "alt_basic.h"
#include "EngineRun.h"
#include "Evaluator.h"
#include "fifo.h"
#include "logmanager.h"
#include "sensor.h"
#include "trigger.h"
#include "timehistory.h"

/******************************************************************************
                                 Package Defines
******************************************************************************/
/*------------------------------------  EVENT  ----------------------------------------------*/
#define MAX_EVENTS              32      /* Total Events defined in the system           */
#define MAX_EVENT_NAME          32      /* Maximum Length of the event name             */
#define MAX_EVENT_ID             5      /* Character length for the event id            */
#define MAX_EVENT_SENSORS       32      /* Maximum sensors to be analyzed by the event  */
#define MAX_EVENT_EXPR_OPRNDS    8      /* Total number of Evaluator operands           */
/*--------------------------------  EVENT TABLE  --------------------------------------------*/
#define MAX_TABLES               8      /* Total Event Tables defined in the system     */
#define MAX_REGION_SEGMENTS      6      /* Maximum number of regions per table          */
/*-------------------------------- EVENT ACTION  --------------------------------------------*/
#define EVENT_ACTION_ACK            0x80000000
#define EVENT_ACTION_LATCH          0x08000000

#define EVENT_ACTION_ON_DURATION(a) ( (a >> 12) & ACTION_ALL )
#define EVENT_ACTION_ON_MET(a)      ( a & ACTION_ALL )
#define EVENT_ACTION_CHK_ACK(a)     (( a & EVENT_ACTION_ACK)   ? TRUE : FALSE)
#define EVENT_ACTION_CHK_LATCH(a)   (( a & EVENT_ACTION_LATCH) ? TRUE : FALSE)
/*---------------------------- EVENT CONFIGURATION DEFAULT ----------------------------------*/
#define EVENT_DEFAULT              "Unused",              /* &EventName[MAX_EVENT_NAME] */\
                                   "None",                /* Event ID - Code            */\
                                   EV_1HZ,                /* Event Rate                 */\
                                   0,                     /* Event Offset               */\
                                   EVAL_EXPR_CFG_DEFAULT, /* Start Expression           */\
                                   EVAL_EXPR_CFG_DEFAULT, /* End Expression             */\
                                   0,                     /* Minimum Duration           */\
                                   0,                     /* Event Action               */\
                                   {0,0,0,0},             /* Sensor Map                 */\
                                   FALSE,                 /* Recording TH During Event  */\
                                   0,                     /* Pre Time History Time Sec  */\
                                   0,                     /* Post Time History Time Sec */\
                                   LOG_PRIORITY_3,        /* Log Priority               */\
                                   EVENT_TABLE_UNUSED     /* Event Table                */

#define EVENT_CONFIG_DEFAULT       EVENT_DEFAULT,\
                                   EVENT_DEFAULT,\
                                   EVENT_DEFAULT,\
                                   EVENT_DEFAULT,\
                                   EVENT_DEFAULT,\
                                   EVENT_DEFAULT,\
                                   EVENT_DEFAULT,\
                                   EVENT_DEFAULT,\
                                   EVENT_DEFAULT,\
                                   EVENT_DEFAULT,\
                                   EVENT_DEFAULT,\
                                   EVENT_DEFAULT,\
                                   EVENT_DEFAULT,\
                                   EVENT_DEFAULT,\
                                   EVENT_DEFAULT,\
                                   EVENT_DEFAULT,\
                                   EVENT_DEFAULT,\
                                   EVENT_DEFAULT,\
                                   EVENT_DEFAULT,\
                                   EVENT_DEFAULT,\
                                   EVENT_DEFAULT,\
                                   EVENT_DEFAULT,\
                                   EVENT_DEFAULT,\
                                   EVENT_DEFAULT,\
                                   EVENT_DEFAULT,\
                                   EVENT_DEFAULT,\
                                   EVENT_DEFAULT,\
                                   EVENT_DEFAULT,\
                                   EVENT_DEFAULT,\
                                   EVENT_DEFAULT,\
                                   EVENT_DEFAULT,\
                                   EVENT_DEFAULT    /* Event [MAX_EVENTS] */

/*--------------------------- EVENT TABLE CONFIGURATION DEFAULT -----------------------------*/
#define EVENT_TABLE_SEGMENT        0,                     /* Start Value                */\
                                   0,                     /* Start Time Sec             */\
                                   0,                     /* Stop Value                 */\
                                   0                      /* Stop Time Sec              */

#define EVENT_TABLE_REGION         EVENT_TABLE_SEGMENT,   /* Segment Definition 0       */\
                                   EVENT_TABLE_SEGMENT,   /* Segment Definition 1       */\
                                   EVENT_TABLE_SEGMENT,   /* Segment Definition 2       */\
                                   EVENT_TABLE_SEGMENT,   /* Segment Definition 3       */\
                                   EVENT_TABLE_SEGMENT,   /* Segment Definition 4       */\
                                   EVENT_TABLE_SEGMENT,   /* Segment Definition 5       */\
                                   0                      /* Event Action               */

#define EVENT_TABLE_DEFAULT        SENSOR_UNUSED,         /* Index                      */\
                                   0.0,                   /* Sensor value to enter Table*/\
                                   0.0,                   /* Hysteresis Positive        */\
                                   0.0,                   /* Hysteresis Negative        */\
                                   0,                     /* Transient Allowance        */\
                                   EVENT_TABLE_REGION,    /* Region A                   */\
                                   EVENT_TABLE_REGION,    /* Region B                   */\
                                   EVENT_TABLE_REGION,    /* Region C                   */\
                                   EVENT_TABLE_REGION,    /* Region D                   */\
                                   EVENT_TABLE_REGION,    /* Region F                   */\
                                   EVENT_TABLE_REGION     /* Region G                   */

#define EVENT_TABLE_CFG_DEFAULT    EVENT_TABLE_DEFAULT, EVENT_TABLE_DEFAULT,\
                                   EVENT_TABLE_DEFAULT, EVENT_TABLE_DEFAULT,\
                                   EVENT_TABLE_DEFAULT, EVENT_TABLE_DEFAULT,\
                                   EVENT_TABLE_DEFAULT, EVENT_TABLE_DEFAULT

/******************************************************************************
                                  Package Typedefs
******************************************************************************/
typedef enum
{
   EVENT_0   =   0, EVENT_1   =   1, EVENT_2   =   2, EVENT_3   =   3,
   EVENT_4   =   4, EVENT_5   =   5, EVENT_6   =   6, EVENT_7   =   7,
   EVENT_8   =   8, EVENT_9   =   9, EVENT_10  =  10, EVENT_11  =  11,
   EVENT_12  =  12, EVENT_13  =  13, EVENT_14  =  14, EVENT_15  =  15,
   EVENT_16  =  16, EVENT_17  =  17, EVENT_18  =  18, EVENT_19  =  19,
   EVENT_20  =  20, EVENT_21  =  21, EVENT_22  =  22, EVENT_23  =  23,
   EVENT_24  =  24, EVENT_25  =  25, EVENT_26  =  26, EVENT_27  =  27,
   EVENT_28  =  28, EVENT_29  =  29, EVENT_30  =  30, EVENT_31  =  31,
   EVENT_UNUSED = 255
} EVENT_INDEX;

typedef enum
{
   EVENT_TABLE_0   =   0, EVENT_TABLE_1   =   1, EVENT_TABLE_2   =   2, EVENT_TABLE_3   =   3,
   EVENT_TABLE_4   =   4, EVENT_TABLE_5   =   5, EVENT_TABLE_6   =   6, EVENT_TABLE_7   =   7,
   EVENT_TABLE_UNUSED = 255
} EVENT_TABLE_INDEX;

typedef enum
{
   REGION_A          = 0,
   REGION_B          = 1,
   REGION_C          = 2,
   REGION_D          = 3,
   REGION_E          = 4,
   REGION_F          = 5,
   MAX_TABLE_REGIONS = 6,
   REGION_NOT_FOUND  = MAX_TABLE_REGIONS
} EVENT_REGION;

typedef enum
{
   ET_IN_TABLE,
   ET_SENSOR_INVALID,
   ET_SENSOR_VALUE_LOW,
   ET_COMMANDED
} EVENT_TABLE_STATE;
/*----------------------------------  EVENT  ------------------------------------------------*/
typedef enum
{
   EV_1HZ            =  1,  /*  1Hz Rate    */
   EV_2HZ            =  2,  /*  2Hz Rate    */
   EV_4HZ            =  4,  /*  4Hz Rate    */
   EV_5HZ            =  5,  /*  5Hz Rate    */
   EV_10HZ           = 10,  /* 10Hz Rate    */
   EV_20HZ           = 20,  /* 20Hz Rate    */
   EV_50HZ           = 50   /* 50Hz Rate    */
} EVENT_RATE;

typedef enum
{
   EVENT_NONE,
   EVENT_START,
   EVENT_ACTIVE,
} EVENT_STATE;

typedef enum
{
   EVENT_NO_END,
   EVENT_END_CRITERIA_INVALID,
   EVENT_END_CRITERIA,
   EVENT_COMMANDED_END,
   EVENT_TABLE_END,
   EVENT_TABLE_SENSOR_INVALID
} EVENT_END_TYPE;

#pragma pack(1)
/* The following structure contains the event configuration information */
typedef struct
{
   INT8                sEventName[MAX_EVENT_NAME];  /* the name of the event             */
   INT8                sEventID  [MAX_EVENT_ID];    /* 4 char cfg id for the event       */
   EVENT_RATE          rate;                        /* Rate to run the event             */
   UINT16              nOffset_ms;                  /* Offset from 0 to start the event  */
   EVAL_EXPR           startExpr;                   /* RPN Expression to start the event */
   EVAL_EXPR           endExpr;                     /* RPN Expression to end the event   */
   UINT32              nMinDuration_ms;             /* Min duration start expression     */
                                                    /* must be met before becoming ACTIVE*/
   UINT32              nAction;                     /* Bit encoded action flags          */
   BITARRAY128         sensorMap;                   /* Bit encoded flags of sensors to   */
                                                    /* sample during the event           */
   BOOLEAN             bEnableTH;                   /* Time History Enabled for Event    */
   UINT32              preTime_s;                   /* Amount of pre-time history        */
   UINT32              postTime_s;                  /* Amount of post-time history       */
   LOG_PRIORITY        priority;                    /* Priority to write the event log   */
   EVENT_TABLE_INDEX   eventTableIndex;             /* Related Event Table to process    */
}EVENT_CFG;

/* The following structure defines the START LOG for the event                           */
typedef struct
{
   EVENT_INDEX nEventIndex;                         /* Event Index for start log         */
   UINT32      seqNumber;                           /* Number used to id all related logs*/
} EVENT_START_LOG;

/* Event Sensor Summary */
typedef struct
{
   SENSOR_INDEX sensorIndex;
   BOOLEAN      bValid;
   TIMESTAMP    timeMinValue;
   FLOAT32      fMinValue;
   TIMESTAMP    timeMaxValue;
   FLOAT32      fMaxValue;
   FLOAT32      fAvgValue;
} EVENT_SNSR_SUMMARY;

/* End Log definition for the event                                                      */
typedef struct
{
   EVENT_INDEX     nEventIndex;                     /* Event Index                       */
   UINT32          seqNumber;                       /* Number used to id all related logs*/
   EVENT_END_TYPE  endType;                         /* Reason for the event ending       */
   TIMESTAMP       tsCriteriaMetTime;               /* Time the start criteria was met   */
   TIMESTAMP       tsDurationMetTime;               /* Time when the min duration was met*/
   UINT32          nDuration_ms;                    /* Length of the event from start    */
                                                    /* criteria being met until event end*/
   TIMESTAMP       tsEndTime;                       /* Time when the event ended         */
   UINT16          nTotalSensors;                   /* Total sensors sampled by event    */
   EVENT_SNSR_SUMMARY sensor[MAX_EVENT_SENSORS];    /* Sensor Summary                    */
} EVENT_END_LOG;

typedef struct
{
   INT8              sName[MAX_EVENT_NAME];         /* the name of the event             */
   INT8              sID  [MAX_EVENT_ID];           /* 4 char cfg id for the event       */
   EVENT_TABLE_INDEX tableIndex;                    /* Index of table being monitored    */
   UINT32            preTime_s;                     /* Amount of history prior to event  */
   UINT32            postTime_s;                    /* Amount of history post event      */
} EVENT_HDR;
#pragma pack()

/* Runtime storage object for the event data                                             */
typedef struct
{
   // Run Time Data
   EVENT_INDEX        eventIndex;                   /* Index of the event                */
   UINT32             seqNumber;                    /* Number used to relate all event logs */
   EVENT_STATE        state;                        /* State of the event: NONE, START   */
                                                    /*                     ACTIVE        */
   UINT32             nStartTime_ms;                /* Time the event started in millisec*/
   UINT32             nDuration_ms;                 /* Duration of event in millisec     */
   TIMESTAMP          tsCriteriaMetTime;            /* Time the start criteria was met   */
   TIMESTAMP          tsDurationMetTime;            /* Time the duration was met         */
   TIMESTAMP          tsEndTime;                    /* Time the event ended              */
// UINT32             nSampleCount;                 /* Sensor sample count for calc avg  */
   UINT16             nTotalSensors;                /* Total sensors being sampled       */
   SNSR_SUMMARY       sensor[MAX_EVENT_SENSORS];    /* Array to store sampling results   */
   UINT16             nRateCounts;                  /* Number of counts between process  */
   UINT16             nRateCountdown;               /* Countdown until next processing   */
   INT8               nActionReqNum;                /* Number used to id action request  */
   EVENT_END_TYPE     endType;                      /* Reason for the event ending       */
   BOOLEAN            bStarted;                     /* Was the Start Criteria met?       */
   BOOLEAN            bTableWasEntered;             /* Flag to for table entry status    */
   EVENT_TABLE_STATE  tableState;                   /* Current State of the table        */
} EVENT_DATA;

//A type for an array of the maximum number of events
//Used for storing event configurations in the configuration manager
typedef EVENT_CFG EVENT_CONFIGS[MAX_EVENTS];

/*-------------------------------- EVENT TABLE ---------------------------------------------*/
#pragma pack(1)
/* This structure defines the line segments used to define regions                       */
typedef struct
{
   FLOAT32 fStartValue;                              /* y value where line starts        */
   UINT32  nStartTime_ms;                            /* x value where line starts        */
   FLOAT32 fStopValue;                               /* y value where line ends          */
   UINT32  nStopTime_ms;                             /* x value where line ends          */
} SEGMENT_DEF;

/* This structure defines the region using line segments and event action                */
typedef struct
{
   SEGMENT_DEF  segment[MAX_REGION_SEGMENTS];       /* Definition of the segments for    */
                                                    /* the region                        */
   UINT32       nAction;                            /* Bit encode action to take when    */
                                                    /* in a region                       */
} REGION_DEF;

/* This structure defines the table configuration                                        */
typedef struct
{
   SENSOR_INDEX nSensor;                            /* Index of sensor to monitor        */
   FLOAT32      fTableEntryValue;                   /* Value that must be reached to     */
                                                    /* enter the table                   */
   FLOAT32      fHysteresisPos;                     /* Hysteresis value to exceed when   */
                                                    /* entering the region from a lower  */
                                                    /* region                            */
   FLOAT32      fHysteresisNeg;                     /* Hysteresis value to exceed when   */
                                                    /* entering the region from a        */
                                                    /* higher region                     */
   UINT32       nTransientAllowance_ms;             /* Amount of time in milliseconds    */
                                                    /* a region must entered for before  */
                                                    /* confirming the entry              */
   REGION_DEF   region[MAX_TABLE_REGIONS];          /* Definition of the regions         */
} EVENT_TABLE_CFG;

/* Region statistics to be reported                                                      */
typedef struct
{
   UINT32 nEnteredCount;                            /* Count of enteries into a region   */
   UINT32 nExitCount;                               /* Count of exits from a region      */
   UINT32 nDuration_ms;                             /* Amount of time spent in region    */
} REGION_LOG_STATS;

/* Runtime statistics to determine where we are in the table                             */
typedef struct
{
   UINT32           nEnteredTime;                   /* Time the region was entered       */
   BOOLEAN          bRegionConfirmed;               /* Flag if we confirmed region entry */
   REGION_LOG_STATS logStats;                       /* Statistics for the log            */
} REGION_STATS;

/* Event Table Region Transition Log */
typedef struct
{
   EVENT_TABLE_INDEX eventTableIndex;               /* Index of the event table          */
   UINT32            seqNumber;                     /* Number used to id all related logs*/
   EVENT_REGION      regionEntered;                 /* New Region Entered                */
   EVENT_REGION      regionExited;                  /* Exited region                     */
   REGION_LOG_STATS  exitedStats;                   /* Exited region log stats           */
                                                    /* - Entered Count                   */
                                                    /* - Exited Count                    */
                                                    /* - Duration in Exited Region       */
   EVENT_REGION     maximumRegionEntered;           /* Maximum Region Reached            */
   SENSOR_INDEX     nSensorIndex;                   /* Sensor Index being monitored      */
   FLOAT32          fMaxSensorValue;                /* Maximum value the sensor reached  */
   UINT32           nMaxSensorElaspedTime_ms;       /* Time max sensor was reached       */
   FLOAT32           fCurrentSensorValue;           /* Current Sensor Value at transition*/
   UINT32            nDuration_ms;                  /* Duration spent in table           */
} EVENT_TABLE_TRANSITION_LOG;


/* Event Table summary log                                                               */
typedef struct
{
   EVENT_TABLE_INDEX eventTableIndex;               /* Index of the event table          */
   UINT32            seqNumber;                     /* Number used to id all related logs*/
   EVENT_TABLE_STATE endReason;                  /* Reason why the event table ended  */
   SENSOR_INDEX      sensorIndex;                   /* Index of the sensor monitored     */
   TIMESTAMP         tsExceedanceStartTime;         /* Time the table was entered        */
   TIMESTAMP         tsExceedanceEndTime;           /* Time the table processing ended   */
   UINT32            nDuration_ms;                  /* Total duration in the table       */
   EVENT_REGION      maximumRegionEntered;          /* Maximum region reached            */
   FLOAT32           fMaxSensorValue;               /* Maximum value the sensor reached  */
   UINT32            nMaxSensorElaspedTime_ms;      /* Time max sensor was reached       */
   EVENT_REGION      maximumCfgRegion;              /* Maximum configured region         */
   REGION_LOG_STATS  regionStats[MAX_TABLE_REGIONS];/* Enter, Exit and Duration of Regs  */
} EVENT_TABLE_SUMMARY_LOG;

typedef struct
{
   SENSOR_INDEX nSensor;                            /* Index of sensor to monitor        */
   FLOAT32      fTableEntryValue;                   /* Value that must be reached to     */
} EVENT_TABLE_HDR;
#pragma pack()

/* Structure to hold the calculated line parameters                                      */
typedef struct
{
   FLOAT32 m;                                       /* (y2 -y1) / (x2 -x1) = m           */
   FLOAT32 b;                                       /*  y1 - (t1 * m) = b                */
} LINE_CONST;

/* Runtime data object for the event table data                                          */
typedef struct
{
   EVENT_TABLE_INDEX nTableIndex;                   /* Storage for the table index       */
   UINT32       seqNumber;
   BOOLEAN      bStarted;                           /* Has the table been entered        */
   UINT32       nStartTime_ms;                      /* When was the table entered        */
   LINE_CONST   segment[MAX_TABLE_REGIONS+1][MAX_REGION_SEGMENTS];
                                                    /* constants used to calculate the   */
                                                    /* line thresholds                   */
   EVENT_REGION currentRegion;                      /* Current Region the sensor is in   */
   EVENT_REGION confirmedRegion;                    /* Region the sensor has been        */
                                                    /* confirmed in, exceed transient    */
   TIMESTAMP    tsExceedanceStartTime;              /* Time the table was entered        */
   TIMESTAMP    tsExceedanceEndTime;                /* Time the table processing ended   */
   UINT32       nTotalDuration_ms;                  /* Total Time spent in the table     */
   EVENT_REGION maximumRegionEntered;               /* Max region reached while in table */
   FLOAT32      fCurrentSensorValue;                /* Current value of the sensor       */
   FLOAT32      fMaxSensorValue;                    /* Max value the sensor reached      */
   UINT32       nMaxSensorElaspedTime_ms;           /* Time the max value was reached    */
   EVENT_REGION maximumCfgRegion;                   /* Maximum configured region         */
   REGION_STATS regionStats[MAX_TABLE_REGIONS+1];   /* Stats for each region             */
   INT8         nActionReqNum;
} EVENT_TABLE_DATA;

//A type for an array of the maximum number of event tables
//Used for storing event table configurations in the configuration manager
typedef EVENT_TABLE_CFG EVENT_TABLE_CONFIGS[MAX_TABLES];

/******************************************************************************
                                   Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( EVENT_BODY )
   #define EXPORT
#else
   #define EXPORT extern
#endif

#if defined ( EVENT_BODY )
   // Note: Updates to EVENT_REGION has dependency to EVT_Region_UserEnumType[]
   USER_ENUM_TBL evt_Region_UserEnumType [] =
   { { "REGION_A",         REGION_A            },
     { "REGION_B",         REGION_B            },
     { "REGION_C",         REGION_C            },
     { "REGION_D",         REGION_D            },
     { "REGION_E",         REGION_E            },
     { "REGION_F",         REGION_F            },
     { "NONE",             REGION_NOT_FOUND    },
     { NULL,          0                        }
   };
#else
   EXPORT USER_ENUM_TBL evt_Region_UserEnumType[];
#endif
/******************************************************************************
                                  Package Exports Variables
******************************************************************************/

/******************************************************************************
                                  Package Exports Functions
*******************************************************************************/
EXPORT void    EventsInitialize        ( void );
EXPORT void    EventTablesInitialize   ( void );
EXPORT UINT16  EventGetBinaryHdr       ( void *pDest, UINT16 nMaxByteSize );
EXPORT UINT16  EventTableGetBinaryHdr  ( void *pDest, UINT16 nMaxByteSize );
EXPORT void    EventSetRecStateChangeEvt(INT32 tag,void (*func)(INT32,BOOLEAN));

#endif // EVENT_H
/*************************************************************************
 *  MODIFICATIONS
 *    $History: Event.h $
 *
 * *****************  Version 30  *****************
 * User: John Omalley Date: 12-11-12   Time: 4:46p
 * Updated in $/software/control processor/code/application
 * SCR 1142 - Formatting Error
 *
 * *****************  Version 29  *****************
 * User: John Omalley Date: 12-11-08   Time: 3:03p
 * Updated in $/software/control processor/code/application
 * SCR 1131 - Busy Recording Logic
 *
 * *****************  Version 28  *****************
 * User: John Omalley Date: 12-11-06   Time: 11:12a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Code Review Updates
 *
 * *****************  Version 27  *****************
 * User: Melanie Jutras Date: 12-11-01   Time: 12:51p
 * Updated in $/software/control processor/code/application
 * SCR #1142 File Format Error
 *
 * *****************  Version 26  *****************
 * User: John Omalley Date: 12-10-23   Time: 2:07p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Updates from Design Review
 * 1. Added Sequence Number for ground team
 * 2. Added Reason for table end to log
 * 3. Updated Event Index in log to ENUM
 * 4. Fixed hysteresis cfg range from 0-FLT_MAX
 *
 * *****************  Version 25  *****************
 * User: John Omalley Date: 12-10-16   Time: 2:37p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Updates per Design Review
 * 1. Changed segment times to milliseconds
 * 2. Limit +/- hysteresis to positive values
 *
 * *****************  Version 24  *****************
 * User: John Omalley Date: 12-09-19   Time: 10:54a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Added Time History enable configuration
 *
 * *****************  Version 23  *****************
 * User: Contractor V&v Date: 9/14/12    Time: 4:05p
 * Updated in $/software/control processor/code/application
 *
 * *****************  Version 22  *****************
 * User: John Omalley Date: 12-09-11   Time: 1:56p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Add ETM Binary Header
 *                    Updated Time History Calls to new prototypes
 *
 * *****************  Version 21  *****************
 * User: John Omalley Date: 12-08-29   Time: 3:23p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Event End Log Update
 *
 * *****************  Version 20  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 12:43p
 * Updated in $/software/control processor/code/application
 * SCR# 1142
 *
 * *****************  Version 19  *****************
 * User: John Omalley Date: 12-08-20   Time: 9:00a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Bit Bucket Issues Cleanup
 *
 * *****************  Version 18  *****************
 * User: John Omalley Date: 12-08-13   Time: 4:22p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Log Cleanup
 *
 * *****************  Version 17  *****************
 * User: John Omalley Date: 12-08-09   Time: 8:38a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Fixed code to properly implement requirements
 *
 * *****************  Version 16  *****************
 * User: John Omalley Date: 12-07-27   Time: 3:03p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Action Manager Persistent Updates
 *
 * *****************  Version 15  *****************
 * User: John Omalley Date: 12-07-19   Time: 5:09p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Cleaned up Code Review Tool findings
 *
 * *****************  Version 14  *****************
 * User: Contractor V&v Date: 7/18/12    Time: 6:24p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 Refactor for common Sensor Summary
 *
 * *****************  Version 13  *****************
 * User: John Omalley Date: 12-07-17   Time: 11:26a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Removed the Event Action and made an Action object
 *
 * *****************  Version 12  *****************
 * User: John Omalley Date: 12-07-16   Time: 11:37a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Event Table Updates
 *
 * *****************  Version 11  *****************
 * User: John Omalley Date: 12-07-13   Time: 3:51p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Code updates, Event Action added, refactoring event table
 * logic
 *
 * *****************  Version 10  *****************
 * User: John Omalley Date: 12-06-07   Time: 11:35a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Removed Event Table Name Configuration
 *
 * *****************  Version 9  *****************
 * User: John Omalley Date: 12-05-24   Time: 9:39a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Added the ETM Write Function
 *
 * *****************  Version 8  *****************
 * User: John Omalley Date: 12-05-22   Time: 2:36p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Update the cfg default
 *
 * *****************  Version 7  *****************
 * User: John Omalley Date: 12-05-22   Time: 2:16p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - File Clean up
 *
 * *****************  Version 6  *****************
 * User: John Omalley Date: 12-05-22   Time: 8:40a
 * Updated in $/software/control processor/code/application
 * SCR 1107
 * Commented Code and updated headers.
 * Fixed user table to use ENUM for Region Table
 *
 * *****************  Version 5  *****************
 * User: John Omalley Date: 5/15/12    Time: 11:57a
 * Updated in $/software/control processor/code/application
 * SCR 1107
 *
 * Event Table Processing Updates:
 * 1. Hysteresis Working
 * 2. Transient Allowance Working
 * 3. Log Recording
 *    a. Duration for each region
 *    b. Enter and Exit counts
 *    c. Max Region Entered
 *    d. Max Sensor Value
 *
 * *****************  Version 4  *****************
 * User: John Omalley Date: 5/07/12    Time: 10:05a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Added Event Table Hysteresis and Transient Allowance on a
 * per Table basis
 *
 * *****************  Version 3  *****************
 * User: John Omalley Date: 5/02/12    Time: 4:53p
 * Updated in $/software/control processor/code/application
 * SCR 1107
 *
 * *****************  Version 2  *****************
 * User: John Omalley Date: 4/27/12    Time: 5:04p
 * Updated in $/software/control processor/code/application
 *
 * *****************  Version 1  *****************
 * User: John Omalley Date: 4/24/12    Time: 10:59a
 * Created in $/software/control processor/code/application
 *
 *
 ***************************************************************************/
