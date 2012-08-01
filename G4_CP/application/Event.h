#ifndef EVENT_H
#define EVENT_H
/**********************************************************************************************
                Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
                    All Rights Reserved. Proprietary and Confidential.


    File:        event.h


    Description: Function prototypes and defines for the event processing.

  VERSION
  $Revision: 14 $  $Date: 7/18/12 6:24p $

**********************************************************************************************/

/*********************************************************************************************/
/* Compiler Specific Includes                                                                */
/*********************************************************************************************/

/*********************************************************************************************/
/* Software Specific Includes                                                                */
/*********************************************************************************************/
#include "actionmanager.h"
#include "alt_basic.h"
#include "EngineRun.h"
#include "Evaluator.h"
#include "fifo.h"
#include "logmanager.h"
#include "sensor.h"
#include "trigger.h"
#include "timehistory.h"

/*********************************************************************************************/
/*                                 Package Defines                                           */
/*********************************************************************************************/
/*------------------------------------  EVENT  ----------------------------------------------*/
#define MAX_EVENTS              32      /* Total Events defined in the system           */
#define MAX_EVENT_NAME          32      /* Maximum Length of the event name             */
#define MAX_EVENT_ID             5      /* Character length for the event id            */
#define MAX_EVENT_SENSORS       32      /* Maximum sensors to be analyzed by the event  */
#define MAX_EVENT_EXPR_OPRNDS    8      /* Total number of Evaluator operands           */
/*--------------------------------  EVENT TABLE  --------------------------------------------*/
#define MAX_TABLES               8      /* Total Event Tables defined in the system     */
#define MAX_REGION_SEGMENTS      6      /* Maximum number of regions per table          */

/*---------------------------- EVENT CONFIGURATION DEFAULT ----------------------------------*/
#define EVENT_DEFAULT              "Unused",              /* &EventName[MAX_EVENT_NAME] */\
                                   "None",                /* Event ID - Code            */\
                                   EV_1HZ,                /* Event Rate                 */\
                                   0,                     /* Event Offset               */\
                                   ENGRUN_UNUSED,         /* Engine Run ID              */\
                                   EVAL_EXPR_CFG_DEFAULT, /* Start Expression           */\
                                   EVAL_EXPR_CFG_DEFAULT, /* End Expression             */\
                                   0,                     /* Minimum Duration           */\
                                   0,                     /* Event Action               */\
                                   {0,0,0,0},             /* Sensor Map                 */\
                                   TH_NONE,               /* Pre Time History           */\
                                   0,                     /* Pre Time History Time Sec  */\
                                   TH_NONE,               /* Post Time History          */\
                                   0,                     /* Post Time History Time Sec */\
                                   LOG_PRIORITY_LOW,      /* Log Priority               */\
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

/*********************************************************************************************/
/*                                   Package Typedefs                                        */
/*********************************************************************************************/
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
   EVENT_TABLE_8   =   8, EVENT_TABLE_9   =   9, EVENT_TABLE_UNUSED = 255
} EVENT_TABLE_INDEX;

typedef enum
{
   REGION_NOT_FOUND  = -1,
   REGION_A          = 0,
   REGION_B          = 1,
   REGION_C          = 2,
   REGION_D          = 3,
   REGION_E          = 4,
   REGION_F          = 5,
   MAX_TABLE_REGIONS = 6

} EVENT_REGION;

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
   EVENT_TRIGGER_INVALID,
   EVENT_END_CRITERIA,
   EVENT_COMMANDED_END
} EVENT_END_TYPE;

#pragma pack(1)
/* The following structure contains the event configuration information */
typedef struct
{
   INT8                EventName[MAX_EVENT_NAME];   /* the name of the event             */
   INT8                EventID  [MAX_EVENT_ID];     /* 4 char cfg id for the event       */
   EVENT_RATE          Rate;                        /* Rate to run the event             */
   UINT16              nOffset_ms;                  /* Offset from 0 to start the event  */
   ENGRUN_INDEX        EngineId;                    /* Engine ID Related to the event    */
   EVAL_EXPR           StartExpr;                   /* RPN Expression to start the event */
   EVAL_EXPR           EndExpr;                     /* RPN Expression to end the event   */
   UINT32              nMinDuration_ms;             /* Min duration start expression     */
                                                    /* must be met before becoming ACTIVE*/
   UINT32              Action;                      /* Bit encoded action flags          */
   BITARRAY128         SensorMap;                   /* Bit encoded flags of sensors to   */
                                                    /* sample during the event */
   TIMEHISTORY_TYPE    PreTimeHistory;              /* Type of previous time history to  */
                                                    /* take; NONE, ALL, OR PORTION       */
   UINT16              PreTime_s;                   /* PORTION Amount of time history    */
   TIMEHISTORY_TYPE    PostTimeHistory;             /* Type of post time history to      */
                                                    /* take; NONE, ALL, OR PORTION       */
   UINT16              PostTime_s;                  /* PORTION Amount of time history    */
   LOG_PRIORITY        Priority;                    /* Priority to write the event log   */
   EVENT_TABLE_INDEX   EventTableIndex;             /* Related Event Table to process    */
}EVENT_CFG;

/* The following structure defines the START LOG for the event                           */
typedef struct
{
   UINT16     EventIndex;                           /* Event Index for start log         */
   TIMESTAMP  StartTime;                            /* Time when the event started       */
} EVENT_START_LOG;

/* End Log definition for the event                                                      */
typedef struct
{
   UINT16          EventIndex;                      /* Event Index                       */
   EVENT_END_TYPE  EndType;                         /* Reason for the event ending       */
   TIMESTAMP       CriteriaMetTime;                 /* Time the start criteria was met   */
   TIMESTAMP       DurationMetTime;                 /* Time when the min duration was met*/
   UINT32          nDuration_ms;                    /* Length of the event from start    */
                                                    /* criteria being met until event end*/
   TIMESTAMP       EndTime;                         /* Time when the event ended         */
   UINT16          nTotalSensors;                   /* Total sensors sampled by event    */
   SNSR_SUMMARY    Sensor[MAX_EVENT_SENSORS];       /* Sensor Summary                    */
} EVENT_END_LOG;
#pragma pack()

/* Runtime storage object for the event data                                             */
typedef struct
{
   // Run Time Data
   EVENT_INDEX        EventIndex;                   /* Index of the event                */
   EVENT_STATE        State;                        /* State of the event: NONE, START   */
                                                    /*                     ACTIVE        */
   UINT32             nStartTime_ms;                /* Time the event started in millisec*/
   UINT32             nDuration_ms;                 /* Duration of event in millisec     */
   TIMESTAMP          CriteriaMetTime;              /* Time the start criteria was met   */
   TIMESTAMP          DurationMetTime;              /* Time the duration was met         */
   TIMESTAMP          EndTime;                      /* Time the event ended              */
   UINT32             nSampleCount;                 /* Sensor sample count for calc avg  */
   UINT16             nTotalSensors;                /* Total sensors being sampled       */
   SNSR_SUMMARY       Sensor[MAX_EVENT_SENSORS];    /* Array to store sampling results   */
   UINT16             nRateCounts;                  /* Number of counts between process  */
   UINT16             nRateCountdown;               /* Countdown until next processing   */
   BOOLEAN            bStartCompareFail;            /* TBD                               */
   BOOLEAN            bEndCompareFail;              /* TBD                               */
   ACTION_TYPE        ActionType;                   /* Type of action being requested    */
                                                    /* ON_MET or ON_DURATION             */
   INT8               ActionReqNum;                 /* Number used to id action request  */
   EVENT_END_TYPE     EndType;                      /* Reason for the event ending       */
} EVENT_DATA;

//A type for an array of the maximum number of events
//Used for storing event configurations in the configuration manager
typedef EVENT_CFG EVENT_CONFIGS[MAX_EVENTS];

/*-------------------------------- EVENT TABLE ---------------------------------------------*/
#pragma pack(1)
/* This structure defines the line segments used to define regions                       */
typedef struct
{
   FLOAT32 StartValue;                               /* y value where line starts        */
   UINT32  StartTime_s;                              /* x value where line starts        */
   FLOAT32 StopValue;                                /* y value where line ends          */
   UINT32  StopTime_s;                               /* x value where line ends          */
} SEGMENT_DEF;

/* This structure defines the region using line segments and event action                */
typedef struct
{
   SEGMENT_DEF  Segment[MAX_REGION_SEGMENTS];       /* Definition of the segments for    */
                                                    /* the region                        */
   UINT32       Action;                             /* Bit encode action to take when    */
                                                    /* in a region                       */
} REGION_DEF;

/* This structure defines the table configuration                                        */
typedef struct
{
   SENSOR_INDEX nSensor;                            /* Index of sensor to monitor        */
   FLOAT32      TableEntryValue;                    /* Value that must be reached to     */
                                                    /* enter the table                   */
   FLOAT32      HysteresisPos;                      /* Hysteresis value to exceed when   */
                                                    /* entering the region from a lower  */
                                                    /* region                            */
   FLOAT32      HysteresisNeg;                      /* Hysteresis value to exceed when   */
                                                    /* entering the region from a        */
                                                    /* higher region                     */
   UINT32       TransientAllowance_ms;              /* Amount of time in milliseconds    */
                                                    /* a region must entered for before  */
                                                    /* confirming the entry              */
   REGION_DEF   Region[MAX_TABLE_REGIONS];          /* Definition of the regions         */
} EVENT_TABLE_CFG;

/* Region statistics to be reported                                                      */
typedef struct
{
   UINT32 EnteredCount;                             /* Count of enteries into a region   */
   UINT32 ExitCount;                                /* Count of exits from a region      */
   UINT32 Duration_ms;                              /* Amount of time spent in region    */
} REGION_LOG_STATS;

/* Runtime statistics to determine where we are in the table                             */
typedef struct
{
   UINT32           EnteredTime;                    /* Time the region was entered       */
   BOOLEAN          RegionConfirmed;                /* Flag if we confirmed region entry */
   REGION_LOG_STATS LogStats;                       /* Statistics for the log            */
} REGION_STATS;

/* Event Table summary log                                                               */
typedef struct
{
   EVENT_TABLE_INDEX EventTableIndex;               /* Index of the event table          */
   SENSOR_INDEX      SensorIndex;                   /* Index of the sensor monitored     */
   TIMESTAMP         ExceedanceStartTime;           /* Time the table was entered        */
   TIMESTAMP         ExceedanceEndTime;             /* Time the table processing ended   */
   UINT32            Duration_ms;                   /* Total duration in the table       */
   EVENT_REGION      MaximumRegionEntered;          /* Maximum region reached            */
   FLOAT32           MaxSensorValue;                /* Maximum value the sensor reached  */
   UINT32            MaxSensorElaspedTime_ms;       /* Time max sensor was reached       */
   EVENT_REGION      MaximumCfgRegion;              /* Maximum configured region         */
   REGION_LOG_STATS  RegionStats[MAX_TABLE_REGIONS];/* Enter, Exit and Duration of Regs  */
} EVENT_TABLE_SUMMARY_LOG;
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
   BOOLEAN      Started;                            /* Has the table been entered        */
   UINT32       StartTime_ms;                       /* When was the table entered        */
   LINE_CONST   Segment[MAX_TABLE_REGIONS][MAX_REGION_SEGMENTS];
                                                    /* constants used to calculate the   */
                                                    /* line thresholds                   */
   EVENT_REGION CurrentRegion;                      /* Current Region the sensor is in   */
   EVENT_REGION ConfirmedRegion;                    /* Region the sensor has been        */
                                                    /* confirmed in, exceed transient    */
   TIMESTAMP    ExceedanceStartTime;                /* Time the table was entered        */
   TIMESTAMP    ExceedanceEndTime;                  /* Time the table processing ended   */
   UINT32       TotalDuration_ms;                   /* Total Time spent in the table     */
   EVENT_REGION MaximumRegionEntered;               /* Max region reached while in table */
   FLOAT32      CurrentSensorValue;                 /* Current value of the sensor       */
   FLOAT32      MaxSensorValue;                     /* Max value the sensor reached      */
   UINT32       MaxSensorElaspedTime_ms;            /* Time the max value was reached    */
   EVENT_REGION MaximumCfgRegion;                   /* Maximum configured region         */
   REGION_STATS RegionStats[MAX_TABLE_REGIONS];     /* Stats for each region             */
   INT8         ActionReqNum;
} EVENT_TABLE_DATA;

//A type for an array of the maximum number of event tables
//Used for storing event table configurations in the configuration manager
typedef EVENT_TABLE_CFG EVENT_TABLE_CONFIGS[MAX_TABLES];

/**********************************************************************************************
                                      Package Exports
**********************************************************************************************/
#undef EXPORT

#if defined( EVENT_BODY )
#define EXPORT
#else
#define EXPORT extern
#endif

#if defined ( EVENT_BODY )
// Note: Updates to EVENT_REGION has dependency to EVT_Region_UserEnumType[]
USER_ENUM_TBL EVT_Region_UserEnumType [] =
{ { "NONE",             REGION_NOT_FOUND    },
  { "REGION_A",         REGION_A            },
  { "REGION_B",         REGION_B            },
  { "REGION_C",         REGION_C            },
  { "REGION_D",         REGION_D            },
  { "REGION_E",         REGION_E            },
  { "REGION_F",         REGION_F            },
  { "MAX_REGIONS",      MAX_TABLE_REGIONS   },
  { NULL,          0                        }
};
#else
EXPORT USER_ENUM_TBL EVT_Region_UserEnumType[];
#endif
/**********************************************************************************************
                                  Package Exports Variables
**********************************************************************************************/

/**********************************************************************************************
                                  Package Exports Functions
**********************************************************************************************/
EXPORT void EventsInitialize       ( void );
EXPORT void EventTablesInitialize  ( void );

/**********************************************************************************************
 *  MODIFICATIONS
 *    $History: Event.h $
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
 *********************************************************************************************/
#endif  /* EVENT_H */



