#ifndef ENGINERUN_H
#define ENGINERUN_H

/******************************************************************************
            Copyright (C) 2012-2016 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    ECCN:        9D991

    File:        EngineRun.h

    Description: The data manager contains the functions used to record
                 data from the various interfaces.

    VERSION
      $Revision: 32 $  $Date: 2/23/16 9:35a $

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
#define MAX_ENGINE_ID       16

#define ENGINE_DEFAULT_SERIAL_NUMBER "000000"
#define ENGINE_DEFAULT_SERVICE_PLAN  "NONE"
#define ENGINE_DEFAULT_MODEL_NUMBER  "NONE"


#define ENGRUN_DEFAULT   "Unused",        /* Engine Name                   */\
                         TRIGGER_UNUSED,  /* start criteria                */\
                         TRIGGER_UNUSED,  /* run criteria                  */\
                         TRIGGER_UNUSED,  /* stop criteria                 */\
                         ER_1HZ,          /* Rate                          */\
                         0,               /* MIF Offset in ms              */\
                         SENSOR_UNUSED,   /* Sensor for temp max           */\
                         SENSOR_UNUSED,   /* Sensor for battery min        */\
                         {0,0,0,0}        /* 128-Bit map of managed sensors*/

#define ENGRUN_CFG_DEFAULT ENGRUN_DEFAULT,\
                           ENGRUN_DEFAULT,\
                           ENGRUN_DEFAULT,\
                           ENGRUN_DEFAULT /* EngineRun[MAX_ENGINES] */

#define ENGINERUN_UNUSED   255

#define ENG_SERIALNO_DEFAULT   ENG_SN_PORT_NONE,     /* portType       */\
                               ENG_SN_PORT_INDEX_1   /* portIndex      */

#define ENG_SERIALNO_CFG_DEFAULT    ENG_SERIALNO_DEFAULT,   /* engine 0 */\
                                    ENG_SERIALNO_DEFAULT,   /* engine 1 */\
                                    ENG_SERIALNO_DEFAULT,   /* engine 2 */\
                                    ENG_SERIALNO_DEFAULT    /* engine 3 */

/******************************************************************************
                               Package Typedefs
******************************************************************************/
typedef enum
{
  ER_1HZ            =  1,  /*  1Hz Rate */
  ER_2HZ            =  2,  /*  2Hz Rate */
  ER_4HZ            =  4,  /*  4Hz Rate */
  ER_5HZ            =  5,  /*  5Hz Rate */
  ER_10HZ           = 10,  /* 10Hz Rate */
  ER_20HZ           = 20,  /* 20Hz Rate */
  ER_50HZ           = 50   /* 50Hz Rate */
} ENGRUN_RATE;

typedef enum
{
  /* Start of states within an engine run */
  ER_STATE_STOPPED,     /* Engine stopped for this engine run  */
  ER_STATE_STARTING,    /* Engine starting for this engine run */
  ER_STATE_RUNNING      /* Engine running for this engine run  */
} ER_STATE;


/* EngineRun logging reasons */
typedef enum
{
  ER_LOG_STOPPED,
  ER_LOG_ERROR,
  ER_LOG_SHUTDOWN
} ER_REASON;

#pragma pack(1)

typedef struct
{
  ENGRUN_INDEX erIndex;                 /* Which engine run object this came from  */
  ER_REASON    reason;                  /* why did the Engine run end              */
  TIMESTAMP    startTime;               /* Time EngineRun entered the START state  */
  TIMESTAMP    endTime;                 /* Time EngineRun ended  the RUNNING state */
  UINT32       startingDuration_ms;     /* Time in ER_STATE_STARTING state         */
  UINT32       erDuration_ms;           /* Time in ER_STATE_RUNNING state          */
  SENSOR_INDEX minMonSensorID;          /* Sensor Id of the item  monitored for min start    */
  BOOLEAN      minValueValid;           /* Was the min value valid through the start         */
  FLOAT32      minMonValue;             /* minimum monitored value recorded during start     */
  SENSOR_INDEX maxMonSensorID;          /* Sensor Id of the item monitored for max start     */
  BOOLEAN      maxValueValid;           /* Was the max value sensor valid through stop/start */
  FLOAT32      maxMonValue;             /* Maximum monitored value recorded while starting   */
  SNSR_SUMMARY snsrSummary[MAX_ENGRUN_SENSORS]; /* Collection of Sensor summaries            */
  UINT32       cycleCounts[MAX_CYCLES]; /* Array of cycle counts */
} ENGRUN_RUNLOG;

typedef struct
{
   CHAR           engineName[MAX_ENGINERUN_NAME]; /* the name of the engine run            */
   TRIGGER_INDEX  startTrigID;                    /* Engine Run Start Trigger              */
   TRIGGER_INDEX  runTrigID;                      /* Engine Run Running Trigger            */
   TRIGGER_INDEX  stopTrigID;                     /* Engine Run Stop Trigger               */
} ENGRUN_HDR;

typedef struct
{
   CHAR serialNumber[MAX_ENGINE_ID];
   CHAR modelNumber[MAX_ENGINE_ID];
}ENGINE_ID;

typedef struct
{
   CHAR      servicePlan[MAX_ENGINE_ID];
   ENGINE_ID engine[MAX_ENGINES];
} ENGINE_FILE_HDR;

#pragma pack()

typedef struct
{
  /* runtime data */
  ENGRUN_INDEX erIndex;                 /* The index of the configured engine run object.    */
  ER_STATE     erState;                 /* State of this engine run                          */
  TIMESTAMP    startTime;               /* Timestamp EngineRun entered the START state       */
  UINT32       startingTime_ms;         /* tick time when EngineRun entered the START state  */
  UINT32       startingDuration_ms;     /* Time EngRun was in START state                    */
  UINT32       erDuration_ms;           /* Time from entering START until end of RUNNING     */
  SENSOR_INDEX minMonSensorID;        /* Sensor Id of the item  monitored for min start    */
  BOOLEAN      minValueValid;           /* Was the min value valid through the start         */
  FLOAT32      minMonValue;             /* minimum monitored value recorded during start     */
  SENSOR_INDEX maxMonSensorID;        /* Sensor Id of the item monitored for max start     */
  BOOLEAN      maxValueValid;           /* Was the max value sensor valid through stop/start */
  FLOAT32      maxMonValue;             /* Maximum monitored value recorded while starting   */
  UINT16       nRateCounts;             /* Count of cycles until this engine run is executed */
  UINT16       nRateCountdown;          /* Number cycles remaining until next execution.     */
  UINT16       nTotalSensors;           /* Count of sensors actively defined in snsrSummary  */
  SNSR_SUMMARY snsrSummary[MAX_ENGRUN_SENSORS];/* Collection of Sensor summaries             */
} ENGRUN_DATA, *ENGRUN_DATA_PTR;

/* Configuration */
typedef struct
{
  CHAR           engineName[MAX_ENGINERUN_NAME]; /* the name of the trigger                 */
  TRIGGER_INDEX  startTrigID;             /* Index into Trig array - start criteria         */
  TRIGGER_INDEX  runTrigID;               /* Index into Trig array - run criteria           */
  TRIGGER_INDEX  stopTrigID;              /* Index into Trig array - stop criteria          */
  ENGRUN_RATE    erRate;                  /* Hz at which this ER and it's cycles are run.   */
  UINT32         nOffset_ms;              /* Offset in ms this object runs within it's MIF  */
  SENSOR_INDEX   minMonSensorID;          /* Sensor ID for monitored min                    */
  SENSOR_INDEX   maxMonSensorID;          /* Sensor ID for monitored max                    */
  BITARRAY128    sensorMap;    /* Bit map of flags of sensors managed by this ER 0-127      */
} ENGRUN_CFG, *ENGRUN_CFG_PTR;

typedef ENGRUN_CFG ENGRUN_CFGS[MAX_ENGINES];


/* ESN Bus Processing */
#define ENG_ESN_MAX_LEN 7  // 6 char + NULL 
#define ENG_ESN_CHECK_RATE   500 // 500 ms or 2 Hz
typedef enum
{
  ENG_SN_PORT_NONE,
  ENG_SN_PORT_ARINC429,  // Reserved for future. Not used today
  ENG_SN_PORT_PORT_QAR,  // Reserved for future. Not used today
  ENG_SN_PORT_UART
} ENG_SN_PORT_TYPE;

typedef enum
{
  ENG_SN_PORT_INDEX_0 = 0,  //NOTE: ENUM val should max Ch Index of Port Type
  ENG_SN_PORT_INDEX_1,      //      i.e. _INDEX_1 enum value s/b "1" for Port Type UART ch "1"
  ENG_SN_PORT_INDEX_2,
  ENG_SN_PORT_INDEX_3,
  END_SN_PORT_INDEX_10 = 10 /* Virtual Port for Multiplexing single UART Channel */ 
  // Reserved for future. 
  // This enumeration must fit in an UINT8 so don't make value larger than 255
} ENG_SN_PORT_INDEX;

typedef struct 
{
  ENG_SN_PORT_TYPE portType; 
  ENG_SN_PORT_INDEX portIndex; 
} ENG_SN_CFG; 

typedef struct
{
  CHAR esn[ENG_ESN_MAX_LEN];
  
  UINT32 check_tick;    // Check / decode ESN at a slower periodic rate
  UINT32 nCntChanged;   // Count ESN changed    
} ENG_SN_STATUS;

typedef ENG_SN_CFG ENG_SN_CFGS[MAX_ENGINES];

/******************************************************************************
                             Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( ENGINERUN_BODY )
  #define EXPORT
#else
  #define EXPORT extern
#endif

/******************************************************************************
                             Package Exports Variables
******************************************************************************/
extern USER_ENUM_TBL engRunIdEnum[];

#if defined ( ENGINERUN_BODY )
USER_ENUM_TBL engineRunStateEnum[] =
{
  { "STOPPED",  ER_STATE_STOPPED  },
  { "STARTING", ER_STATE_STARTING },
  { "RUNNING",  ER_STATE_RUNNING  },
  { NULL, 0 }
};
#else
  EXPORT USER_ENUM_TBL engineRunStateEnum[];
#endif

/******************************************************************************
                             Package Exports Functions
******************************************************************************/
// Task init and execution.
EXPORT void             EngRunInitialize           ( void );
EXPORT ER_STATE         EngRunGetState             ( ENGRUN_INDEX idx, UINT8* EngRunFlags );
EXPORT UINT16           EngRunGetBinaryHeader      ( void *pDest, UINT16 nMaxByteSize );
EXPORT ENGINE_FILE_HDR* EngRunGetFileHeader        ( void );
EXPORT void             EngRunSetRecStateChangeEvt ( INT32 tag,void (*func)(INT32,BOOLEAN));
EXPORT BOOLEAN          EngReInitFile              ( void );

EXPORT BOOLEAN          EngRunGetSN                ( UINT8 engId, CHAR *esn_ptr, UINT8 cnt );

#endif // ENGINERUN_H

/*************************************************************************
 *  MODIFICATIONS
 *    $History: EngineRun.h $
 * 
 * *****************  Version 32  *****************
 * User: John Omalley Date: 2/23/16    Time: 9:35a
 * Updated in $/software/control processor/code/application
 * SCR 1304 - Code Review Updates
 * 
 * *****************  Version 31  *****************
 * User: Peter Lee    Date: 11/11/15   Time: 11:49a
 * Updated in $/software/control processor/code/application
 * SCR #1304 Default new eng_sn.portindex to "1" instead of "0" as UART
 * port 0 not available. 
 * 
 * *****************  Version 30  *****************
 * User: Peter Lee    Date: 11/05/15   Time: 6:46p
 * Updated in $/software/control processor/code/application
 * SCR #1304 APAC, ESN updates
 * 
 * *****************  Version 29  *****************
 * User: Jeff Vahue   Date: 12/13/12   Time: 3:08p
 * Updated in $/software/control processor/code/application
 * SCR# 1205 - Remove ER Start Log
 * 
 * *****************  Version 28  *****************
 * User: John Omalley Date: 12-12-08   Time: 11:44a
 * Updated in $/software/control processor/code/application
 * SCR 1162 - NV MGR File Init function
 * 
 * *****************  Version 27  *****************
 * User: Contractor V&v Date: 11/26/12   Time: 12:31p
 * Updated in $/software/control processor/code/application
 * SCR #1167 SensorSummary
 *
 * *****************  Version 26  *****************
 * User: Contractor V&v Date: 11/16/12   Time: 8:11p
 * Updated in $/software/control processor/code/application
 * CodeReview
 *
 * *****************  Version 25  *****************
 * User: Contractor V&v Date: 11/12/12   Time: 6:39p
 * Updated in $/software/control processor/code/application
 * Code Review
 *
 * *****************  Version 22  *****************
 * User: Jim Mood     Date: 11/09/12   Time: 6:34p
 * Updated in $/software/control processor/code/application
 * SCR 1131 Recording busy status
 *
 * *****************  Version 21  *****************
 * User: Contractor V&v Date: 11/08/12   Time: 4:26p
 * Updated in $/software/control processor/code/application
 * Code review
 *
 * *****************  Version 20  *****************
 * User: Melanie Jutras Date: 12-10-31   Time: 2:16p
 * Updated in $/software/control processor/code/application
 * SCR #1142 File Format Error
 *
 * *****************  Version 19  *****************
 * User: Melanie Jutras Date: 12-10-31   Time: 2:09p
 * Updated in $/software/control processor/code/application
 * SCR #1142 File format error
 *
 * *****************  Version 18  *****************
 * User: Peter Lee    Date: 12-10-27   Time: 5:07p
 * Updated in $/software/control processor/code/application
 * SCR #1190 Creep Requirements
 *
 * *****************  Version 17  *****************
 * User: Contractor V&v Date: 12-10-02   Time: 1:17p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 Coding standard compliance
 *
 * *****************  Version 16  *****************
 * User: John Omalley Date: 12-09-27   Time: 9:13a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Added Engine Identification Fields to software and file
 * header
 *
 * *****************  Version 15  *****************
 * User: Contractor V&v Date: 9/14/12    Time: 4:02p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 fixes for sensor list
 *
 * *****************  Version 14  *****************
 * User: John Omalley Date: 12-09-11   Time: 1:56p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Added ETM Binary Header
 *
 * *****************  Version 13  *****************
 * User: Jeff Vahue   Date: 9/07/12    Time: 4:05p
 * Updated in $/software/control processor/code/application
 * SCR# 1107 - V&V fixes, code review updates
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
