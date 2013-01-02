#ifndef CYCLE_H
#define CYCLE_H
/******************************************************************************
            Copyright (C) 2008-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:        Cycle.h

    Description: The data manager contains the functions used to record
                 data from the various interfaces.

    VERSION
      $Revision: 22 $  $Date: 12/28/12 5:49p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include "alt_basic.h"


/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "EngineRunInterface.h"
#include "EngineRun.h"
#include "trigger.h"
#include "NVMgr.h"
/******************************************************************************
                                 Package Defines
******************************************************************************/
#undef PEAK_CUM_PROCESSING
#undef RHL_PROCESSING

#define CYCLE_DEFAULT  "Unused",           /* Name       */\
                       CYC_TYPE_NONE_CNT,  /* Type       */\
                       0,                  /* nCount     */\
                       TRIGGER_UNUSED,     /* Trigger ID */\
                       ENGRUN_UNUSED       /* Engine ID  */\

#define CYCLE_CFG_DEFAULT CYCLE_DEFAULT,\
                          CYCLE_DEFAULT,\
                          CYCLE_DEFAULT,\
                          CYCLE_DEFAULT,\
                          CYCLE_DEFAULT,\
                          CYCLE_DEFAULT,\
                          CYCLE_DEFAULT,\
                          CYCLE_DEFAULT,\
                          CYCLE_DEFAULT,\
                          CYCLE_DEFAULT,\
                          CYCLE_DEFAULT,\
                          CYCLE_DEFAULT,\
                          CYCLE_DEFAULT,\
                          CYCLE_DEFAULT,\
                          CYCLE_DEFAULT,\
                          CYCLE_DEFAULT,\
                          CYCLE_DEFAULT,\
                          CYCLE_DEFAULT,\
                          CYCLE_DEFAULT,\
                          CYCLE_DEFAULT,\
                          CYCLE_DEFAULT,\
                          CYCLE_DEFAULT,\
                          CYCLE_DEFAULT,\
                          CYCLE_DEFAULT,\
                          CYCLE_DEFAULT,\
                          CYCLE_DEFAULT,\
                          CYCLE_DEFAULT,\
                          CYCLE_DEFAULT,\
                          CYCLE_DEFAULT,\
                          CYCLE_DEFAULT,\
                          CYCLE_DEFAULT,\
                          CYCLE_DEFAULT

/* Defines controlling whether to log the change of a cycle checkID */
#define LOGUPDATE_YES TRUE
#define LOGUPDATE_NO  FALSE


/******************************************************************************
                               Package Typedefs
******************************************************************************/
#define MAX_CYCLENAME        32

#ifdef PEAK_CUM_PROCESSING
#define CYCLEVALUE_COUNT 15
#endif

typedef enum
{
  CYCLE_ID_0    = 0 ,  CYCLE_ID_1    = 1 ,  CYCLE_ID_2    = 2 ,  CYCLE_ID_3    = 3 ,
  CYCLE_ID_4    = 4 ,  CYCLE_ID_5    = 5 ,  CYCLE_ID_6    = 6 ,  CYCLE_ID_7    = 7 ,
  CYCLE_ID_8    = 8 ,  CYCLE_ID_9    = 9 ,  CYCLE_ID_10   = 10,  CYCLE_ID_11   = 11,
  CYCLE_ID_12   = 12,  CYCLE_ID_13   = 13,  CYCLE_ID_14   = 14,  CYCLE_ID_15   = 15,
  CYCLE_ID_16   = 16,  CYCLE_ID_17   = 17,  CYCLE_ID_18   = 18,  CYCLE_ID_19   = 19,
  CYCLE_ID_20   = 20,  CYCLE_ID_21   = 21,  CYCLE_ID_22   = 22,  CYCLE_ID_23   = 23,
  CYCLE_ID_24   = 24,  CYCLE_ID_25   = 25,  CYCLE_ID_26   = 26,  CYCLE_ID_27   = 27,
  CYCLE_ID_28   = 28,  CYCLE_ID_29   = 29,  CYCLE_ID_30   = 30,  CYCLE_ID_31   = 31,
  CYCLE_UNUSED = 255
} CYCLE_INDEX;


// ---------------------------------
// Below is the the enum list of supported cycle types.
// Unsupported cycle types are listed to maintain values for compatibility
// with configuration tools and support support programs (e.g. TurbineTracker)
//
typedef enum
{
  CYC_TYPE_SIMPLE_CNT              = 1,
  CYC_TYPE_DURATION_CNT            = 2,
#ifdef PEAK_CUM_PROCESSING
  //----- Currently Unsupported --------------
  //CYC_TYPE_PEAK_VAL_CNT          = 3,
  //CYC_TYPE_CUM_VALLEY_CNT        = 4,
  //CYC_TYPE_REPETITIVE_HEAVY_LIFT = 5,
  //------------------------------------------
#endif
  CYC_TYPE_NONE_CNT                = 6,
  CYC_TYPE_PERSIST_SIMPLE_CNT      = 7,
  CYC_TYPE_PERSIST_DURATION_CNT    = 8,
#ifdef PEAK_CUM_PROCESSING
  //----- Currently Unsupported --------------
  //CYC_TYPE_PEAK_VAL_CNT          = 9,
  //CYC_TYPE_CUM_CNT               = 10,
  //------------------------------------------
#endif
  MAX_CYC_TYPE
}CYC_TYPE;

// Cycle value is part of a table used to look up
// fractional values for a Cumulative Valley Cycle
#ifdef PEAK_CUM_PROCESSING
typedef struct
{
  FLOAT32 cycleCount;   /* value to use for incrementing cycle */
  FLOAT32 SensorValueA; /* fractional value for sensor A       */
  FLOAT32 SensorValueB; /* fractional value for sensor B       */
} CYCLEVALUE, *PCYCLEVALUE;
#endif

#pragma pack(1)
// A cycle is counted when some sensor exceeds some threshold.
typedef struct
{
  CHAR          name[MAX_CYCLENAME + 1]; /* cycle name                                   */
  CYC_TYPE      type;          /* How cycle is counted 0xFFFF == This cycle unused       */
  UINT32        nCount;        /* value added for incrementing cycle                     */
  TRIGGER_INDEX nTriggerId;    /* Index of the trigger defining this cycles start/end    */
  ENGRUN_INDEX  nEngineRunId;  /* which EngineRun this cycle is associated               */
#ifdef PEAK_CUM_PROCESSING
  CYCLEVALUE CycleValue[CYCLEVALUE_COUNT]; /* cycle values for fractional cycles    */
#endif
#ifdef RHL_PROCESSING
  FLOAT32    RHLincrease;   /* minimum sensorA RHL % increase to start RHL            */
  FLOAT32    RHLdecrease;   /* minimum sensorA RHL % decrease to end RHL              */
  FLOAT32    RHLstableTime; /* minimum amount of time at stable sensorA before a RHL  */
  FLOAT32    RHLstable;     /* maximum +/- sensorA deviation for stability            */
#endif
//  UINT32     MinDuration;   /* minimum duration required for incr and duration cycles */
} CYCLE_CFG, *CYCLE_CFG_PTR;

typedef struct
{
   CHAR          name[MAX_CYCLENAME];     /* cycle name                                   */
   CYC_TYPE      type;          /* How cycle is counted 0xFFFF == This cycle unused       */
   UINT32        nCount;        /* value added for incrementing cycle                     */
   TRIGGER_INDEX nTriggerId;    /* Index of the trigger defining this cycles start/end    */
   ENGRUN_INDEX  nEngineRunId;  /* which EngineRun this cycle is associated               */
} CYCLE_HDR;

#pragma pack()

typedef CYCLE_CFG CYCLE_CFGS[MAX_CYCLES];

/*-----------------------------------------------------------------------------------------
| Run Time Calculated Data
+----------------------------------------------------------------------------------------*/
typedef struct
{
  BOOLEAN cycleActive;      /* The cycle is active */
  UINT32  cycleLastTime_ms; /* Timestamp in ms that the cycle became active */
  UINT32  currentCount;     /* The current working count during this cycle active state */
}CYCLE_DATA, *CYCLE_DATA_PTR;

#pragma pack(1)
typedef struct
{
  union
  {
    UINT32  n;    /* used for duration count in seconds */
    FLOAT32 f;    /* used for all other counts          */
  } count;
  UINT16 checkID; /* Csum to ensure the count is valid for the currently configured  settings*/
} CYCLE_ENTRY;

// Collection of cycle entries with csum for persisting to NV Memory.
typedef struct
{
  CYCLE_ENTRY data[MAX_CYCLES];
} CYCLE_COUNTS;
#pragma pack()


#pragma pack(1)

typedef struct
{
  UINT16 cycleId;
  UINT32 rtcCount;
  UINT32 eepromCount;
}CYCLE_PERSIST_COUNTS_DIFF_LOG;

typedef struct
{
  UINT16  cycleId;       /* Cycle ID             */
  UINT32  prevCount;     /* Previous count value */
  UINT16  prevCheckID;   /* Previous check ID    */
  UINT16  newCheckID;    /* Current check ID     */
}CYCLE_COUNT_CHANGE_ETM_LOG;
#pragma pack()

/******************************************************************************
                               Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( CYCLE_BODY )
  #define EXPORT
#else
  #define EXPORT extern
#endif

/******************************************************************************
                             Package Exports Variables
******************************************************************************/
#if defined ( CYCLE_BODY )
USER_ENUM_TBL cycleEnumType [] =
{ {"0", CYCLE_ID_0}, {"1", CYCLE_ID_1}, {"2", CYCLE_ID_2}, {"3", CYCLE_ID_3},
  {"4", CYCLE_ID_4}, {"5", CYCLE_ID_5}, {"6", CYCLE_ID_6}, {"7", CYCLE_ID_7},
  {"8", CYCLE_ID_8}, {"9", CYCLE_ID_9}, {"10",CYCLE_ID_10},{"11",CYCLE_ID_11},
  {"12",CYCLE_ID_12},{"13",CYCLE_ID_13},{"14",CYCLE_ID_14},{"15",CYCLE_ID_15},
  {"16",CYCLE_ID_16},{"17",CYCLE_ID_17},{"18",CYCLE_ID_18},{"19",CYCLE_ID_19},
  {"20",CYCLE_ID_20},{"21",CYCLE_ID_21},{"22",CYCLE_ID_22},{"23",CYCLE_ID_23},
  {"24",CYCLE_ID_24},{"25",CYCLE_ID_25},{"26",CYCLE_ID_26},{"27",CYCLE_ID_27},
  {"28",CYCLE_ID_28},{"29",CYCLE_ID_29},{"30",CYCLE_ID_30},{"31",CYCLE_ID_31},
  {"UNUSED",CYCLE_UNUSED },{ NULL,0}
};
#else
EXPORT USER_ENUM_TBL cycleEnumType[];
#endif


/******************************************************************************
                             Package Exports Functions
******************************************************************************/
EXPORT void    CycleInitialize         ( void );
EXPORT void    CycleUpdateAll          ( ENGRUN_INDEX erIndex, ER_STATE erState );

EXPORT void    CycleFinishEngineRun    ( ENGRUN_INDEX erID );
EXPORT void    CycleResetEngineRun     ( ENGRUN_INDEX erID );
EXPORT UINT16  CycleGetBinaryHeader    ( void *pDest, UINT16 nMaxByteSize );
EXPORT UINT32  CycleGetCount ( CYCLE_INDEX nCycle );
EXPORT void    CycleCollectCounts      ( UINT32 counts[], ENGRUN_INDEX erIdx);
EXPORT BOOLEAN CycleEEFileInit         ( void );
EXPORT BOOLEAN CycleRTCFileInit        ( void );

#endif // CYCLE_H

 /*************************************************************************
 *  MODIFICATIONS
 *    $History: Cycle.h $
 *
 * *****************  Version 22  *****************
 * User: Contractor V&v Date: 12/28/12   Time: 5:49p
 * Updated in $/software/control processor/code/system
 * SCR #1197 Pass eng state to cycle.
 *
 * *****************  Version 21  *****************
 * User: Contractor V&v Date: 12/14/12   Time: 5:02p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review
 *
 * *****************  Version 20  *****************
 * User: John Omalley Date: 12-12-08   Time: 11:44a
 * Updated in $/software/control processor/code/system
 * SCR 1162 - NV MGR File Init function
 *
 * *****************  Version 19  *****************
 * User: Contractor V&v Date: 12/03/12   Time: 5:36p
 * Updated in $/software/control processor/code/system
 * SCR #1107 Code Review
 *
 * *****************  Version 18  *****************
 * User: John Omalley Date: 12-11-12   Time: 4:46p
 * Updated in $/software/control processor/code/system
 * SCR 1142 - Formatting Error
 *
 * *****************  Version 17  *****************
 * User: Contractor V&v Date: 11/09/12   Time: 5:16p
 * Updated in $/software/control processor/code/system
 * Code Review/ function reduction
 *
 * *****************  Version 16  *****************
 * User: Contractor V&v Date: 11/08/12   Time: 4:28p
 * Updated in $/software/control processor/code/system
 * SCR # 1183
 *
 * *****************  Version 15  *****************
 * User: Contractor V&v Date: 10/12/12   Time: 6:29p
 * Updated in $/software/control processor/code/system
 * FAST 2 Review Findings
 *
 * *****************  Version 14  *****************
 * User: Contractor V&v Date: 12-10-02   Time: 1:24p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 Coding standard compliance
 *
 * *****************  Version 13  *****************
 * User: Contractor V&v Date: 12-09-19   Time: 3:23p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 Coding standard
 *
 * *****************  Version 12  *****************
 * User: Contractor V&v Date: 9/14/12    Time: 4:53p
 * Updated in $/software/control processor/code/system
 * FAST 2 Refactor Cycle for externing variable to Trend
 *
 * *****************  Version 11  *****************
 * User: John Omalley Date: 12-09-11   Time: 2:11p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Added Binary ETM Header
 *
 * *****************  Version 10  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 9  *****************
 * User: Contractor V&v Date: 8/08/12    Time: 3:29p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2  Cleanup
 *
 * *****************  Version 8  *****************
 * User: Contractor V&v Date: 7/11/12    Time: 4:35p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2  Cycle impl and persistence
 *
 * *****************  Version 7  *****************
 * User: Contractor V&v Date: 6/25/12    Time: 7:16p
 * Updated in $/software/control processor/code/system
 * fix default cycle to use all 32
 *
 * *****************  Version 6  *****************
 * User: Contractor V&v Date: 5/24/12    Time: 3:05p
 * Updated in $/software/control processor/code/system
 * FAST2 Refactoring
 *
 * *****************  Version 5  *****************
 * User: John Omalley Date: 12-05-22   Time: 2:17p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Check in for Dave
 *
 * *****************  Version 4  *****************
 * User: Contractor V&v Date: 5/10/12    Time: 6:42p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2  Cycle
 ***************************************************************************/
