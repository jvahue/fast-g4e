#ifndef ENGINE_RUN_H
#define ENGINE_RUN_H

/******************************************************************************
            Copyright (C) 2008-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:        EngineRun.h

    Description: The data manager contains the functions used to record
                 data from the various interfaces.

    VERSION
      $Revision: 12 $  $Date: 8/28/12 12:43p $     
    
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/


/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "SystemLog.h"
#include "trigger.h"
#include "sensor.h"
#include "EngineRunInterface.h"

/******************************************************************************
                                 Package Defines
******************************************************************************/
#define MAX_ENGINERUN_NAME  32


#define ENGRUN_DEFAULT   "Unused",        /* Engine Name    */\
                         TRIGGER_UNUSED,  /* start criteria */\
                         TRIGGER_UNUSED,  /* run criteria   */\
                         TRIGGER_UNUSED,  /* stop criteria  */\
                         ER_1HZ,          /* Rate           */\
                         0,               /* MIF Offset in millisecs */\
                         SENSOR_UNUSED,   /* Sensor for temp max */\
                         SENSOR_UNUSED,   /* Sensor for battery min */\
                         {0,0,0,0}        /* 128-Bit map of managed sensors*/

#define ENGRUN_CFG_DEFAULT ENGRUN_DEFAULT,\
                           ENGRUN_DEFAULT,\
                           ENGRUN_DEFAULT,\
                           ENGRUN_DEFAULT /* EngineRun[MAX_ENGINES] */





/******************************************************************************
                               Package Typedefs
******************************************************************************/
typedef enum
{
  ER_1HZ            =  1,  /*  1Hz Rate    */
  ER_2HZ            =  2,  /*  2Hz Rate    */
  ER_4HZ            =  4,  /*  4Hz Rate    */
  ER_5HZ            =  5,  /*  5Hz Rate    */
  ER_10HZ           = 10,  /* 10Hz Rate    */
  ER_20HZ           = 20,  /* 20Hz Rate    */
  ER_50HZ           = 50   /* 50Hz Rate    */
} ENGRUN_RATE;

typedef enum
{
  /* Start of states within an engine run */  
  ER_STATE_STOPPED,    /* Engine stopped for this engine run                            */
  ER_STATE_STARTING,    /* Engine starting for this engine run                           */
  ER_STATE_RUNNING,     /* Engine running for this engine run                            */
  MAX_ER_STATE
} ER_STATE;


/* EngineRun logging reasons */
typedef enum
{
  ER_LOG_STOPPED,
  ER_LOG_ERROR,
  ER_LOG_RUNNING,
  ER_LOG_SHUTDOWN
} ER_REASON;

#pragma pack(1)

typedef struct  
{
  ENGRUN_INDEX ErIndex;                   /* Which enginerun object this came from           */
  ER_REASON    Reason;                    /* Reason for creating this start log              */
  TIMESTAMP    StartTime;                 /* Timestamp EngineRun entered the START state     */
  TIMESTAMP    EndTime;                   /* Timestamp EngineRun exited  the RUNNING state   */
  UINT32       StartingDuration_ms;       /* Time from STARTED until transition to RUNNING   */
  SENSOR_INDEX MaxStartSensorId;          /* Sensor Id of the item monitored for max start   */
  FLOAT32      MonMaxValue;         /* Max Monitored value during start                */
  SENSOR_INDEX MinStartSensorId;          /* Sensor Id of the item  monitored for min start  */
  FLOAT32      MonMinValue;         /* Min monitored value throughout EngineRun        */
}ENGRUN_STARTLOG;

typedef struct
{
  ENGRUN_INDEX ErIndex;                   /* Which enginerun object this came from           */
  ER_REASON    Reason;
  TIMESTAMP    StartTime;                 /* Timestamp EngineRun entered the START state     */
  TIMESTAMP    EndTime;                   /* Timestamp EngineRun exited  the RUNNING state   */
  UINT32       StartingDuration_ms;       /* Time from STARTED until transition to RUNNING   */
  UINT32       Duration_ms;               /* Time from STARTED until transit. to ENDED/ERROR */  
  SNSR_SUMMARY SnsrSummary[MAX_ENGRUN_SENSORS];/* Collection of Sensor summaries                  */
  UINT32       CycleCounts[MAX_CYCLES];   /* Array of cycle counts                           */
} ENGRUN_RUNLOG;
#pragma pack()

typedef struct
{
  /* runtime data */
  ENGRUN_INDEX ErIndex;                 /* Which enginerun object this came from            */
  ER_STATE     State;                   /* State of this engine run                         */
  TIMESTAMP    StartTime;               /* Timestamp EngineRun entered the START state      */
  UINT32       startingTime;            /* tick time when EngineRun entered the START state */
  UINT32       StartingDuration_ms;     /* Time EngRun was in START state                   */
  UINT32       Duration_ms;             /* Time from entering START until end of RUNNING    */    
  FLOAT32      MonMinValue;             /* minimum monitored value recorded during start    */  
  FLOAT32      MonMaxValue;             /* Maximum monitored value recorded while starting  */                                   
  UINT32       nSampleCount;            /* For calculating averages                         */
  INT16        nRateCounts;             /* Count of cycles until this engine run is executed*/
  INT16        nRateCountdown;          /* Number cycles remaining until next execution.    */
  UINT16       nTotalSensors;           /* Count of sensors actively defined in SnsrSummary */
  SNSR_SUMMARY SnsrSummary[MAX_ENGRUN_SENSORS];/* Collection of Sensor summaries            */
  UINT32       CycleCounts[MAX_CYCLES];   /* Array of cycle counts */
} ENGRUN_DATA, *ENGRUN_DATA_PTR;

/* Configuration */
typedef struct
{
  CHAR           EngineName[MAX_ENGINERUN_NAME]; /* the name of the trigger                */
  TRIGGER_INDEX  StartTrigID;             /* Index into Trig array - start criteria        */
  TRIGGER_INDEX  RunTrigID;               /* Index into Trig array - run criteria          */
  TRIGGER_INDEX  StopTrigID;              /* Index into Trig array - stop criteria         */
  ENGRUN_RATE    Rate;                    /* Rate at which this ER and it's cycles are run.*/
  UINT32         nOffset_ms;              /* Offset in ms this object runs within it's MIF */
  SENSOR_INDEX   MonMaxSensorID;          /* Sensor ID for monitored max                   */
  SENSOR_INDEX   MonMinSensorID;          /* Sensor ID for monitored min                   */
  BITARRAY128    SensorMap;    /* Bit map of flags of sensors managed by this ER 0-127     */
}
ENGRUN_CFG, *ENGRUN_CFG_PTR;

typedef ENGRUN_CFG ENGRUN_CFGS[MAX_ENGINES];


/******************************************************************************
                               Package Exports
******************************************************************************/
#undef EXPORT

#if defined( ENGINERUN_BODY )
  #define EXPORT
#else
  #define EXPORT extern
#endif


/******************************************************************************
                             Package Exports Variables
******************************************************************************/
extern USER_ENUM_TBL EngRunIdEnum[];

#if defined( ENGINERUN_BODY )
 USER_ENUM_TBL EngineRunStateEnum[] =
{
  { "STOPPED",  ER_STATE_STOPPED  },
  { "STARTING", ER_STATE_STARTING },
  { "RUNNING",  ER_STATE_RUNNING  },
  { NULL, 0 }
};
#else
EXPORT USER_ENUM_TBL EngineRunStateEnum[];
#endif




/******************************************************************************
                             Package Exports Functions
******************************************************************************/
// Task init and execution.
EXPORT void     EngRunInitialize(void);
EXPORT void     EngRunTask(void* pParam);
EXPORT ER_STATE EngRunGetState(ENGRUN_INDEX idx, UINT8* EngRunFlags);
EXPORT ENGRUN_RUNLOG*  EngRunGetPtrToLog(ENGRUN_INDEX engId);


#endif // SYS_CLOCKMGR_H

/*************************************************************************
 *  MODIFICATIONS
 *    $History: EngineRun.h $
 * 
 * *****************  Version 12  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 12:43p
 * Updated in $/software/control processor/code/application
 * SCR# 1142
 * 
 * *****************  Version 11  *****************
 * User: Contractor V&v Date: 8/08/12    Time: 3:33p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 Fixed bug in persist count writing Refactor temp and
 * voltage to max and min
 * 
 * *****************  Version 10  *****************
 * User: Contractor V&v Date: 7/18/12    Time: 6:24p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 Add shutdown handling
 * 
 * *****************  Version 9  *****************
 * User: Contractor V&v Date: 7/11/12    Time: 4:29p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2  cleanup
 * 
 * *****************  Version 8  *****************
 * User: Contractor V&v Date: 6/25/12    Time: 2:28p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 Use LogWriteETM, separate log structures
 * 
 * *****************  Version 7  *****************
 * User: Contractor V&v Date: 6/18/12    Time: 4:26p
 * Updated in $/software/control processor/code/application
 * Change EngRunGetState signature
 * 
 * *****************  Version 6  *****************
 * User: Contractor V&v Date: 6/05/12    Time: 6:41p
 * Updated in $/software/control processor/code/application
 * Engine ID change
 * 
 * *****************  Version 5  *****************
 * User: Contractor V&v Date: 5/24/12    Time: 3:04p
 * Updated in $/software/control processor/code/application
 * FAST2 Refactoring
 * 
 * *****************  Version 4  *****************
 * User: John Omalley Date: 12-05-22   Time: 2:15p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Check In for Dave
 * 
 * *****************  Version 3  *****************
 * User: Contractor V&v Date: 5/10/12    Time: 6:41p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2  Engine Run
 ***************************************************************************/
