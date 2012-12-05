#ifndef TRIGGER_H
#define TRIGGER_H
/******************************************************************************
            Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.


    File:        trigger.h


    Description: Function prototypes and defines for the trigger processing.

  VERSION
  $Revision: 41 $  $Date: 12/05/12 4:17p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_basic.h"
#include "sensor.h"
#include "Evaluator.h"

/******************************************************************************
                             Package Defines
******************************************************************************/
#define MAX_TRIG_EXPR_OPRNDS  8
#define MAX_TRIGGERS         64
#define MAX_TRIG_SENSORS      4
#define MAX_TRIGGER_NAME     32

#define CLR_TRIGGER_FLAGS(tf) ( memset(tf, 0, sizeof(BITARRAY128 ) ) )

//Default non-volatile configuration for the trigger component.
#define TRIGGER_CRITERIA_DEFAULT 0.0F,          /*fValue     */\
                                 LT             /*compare    */

#define TRIGGER_SENSOR_DEFAULT  SENSOR_UNUSED,             /*SensorIndex */\
                                TRIGGER_CRITERIA_DEFAULT,  /*start       */\
                                TRIGGER_CRITERIA_DEFAULT   /*end         */

#define TRIGGER_DEFAULT "Unused",           /*&triggerName[MAX_TRIGGER_NAME]*/\
                        TRIGGER_SENSOR_DEFAULT,\
                        TRIGGER_SENSOR_DEFAULT,\
                        TRIGGER_SENSOR_DEFAULT,\
                        TRIGGER_SENSOR_DEFAULT,/*trigSensor[MAX_TRIG_SENSORS]*/\
                        0,                     /*MinDuration*/\
                        TR_1HZ,                /*Trigger Rate */\
                        0,                     /*Trigger Offset mS */\
                        EVAL_EXPR_CFG_DEFAULT, /*start Expression*/\
                        EVAL_EXPR_CFG_DEFAULT  /*end Expression*/



#define TRIGGER_CONFIG_DEFAULT    TRIGGER_DEFAULT,/*01*/\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT, /*32*/ \
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT,\
                                  TRIGGER_DEFAULT /*64*/


/******************************************************************************
                             Package Typedefs
******************************************************************************/
typedef enum
{
   TRIGGER_0   =   0, TRIGGER_1   =   1, TRIGGER_2   =   2, TRIGGER_3   =   3,
   TRIGGER_4   =   4, TRIGGER_5   =   5, TRIGGER_6   =   6, TRIGGER_7   =   7,
   TRIGGER_8   =   8, TRIGGER_9   =   9, TRIGGER_10  =  10, TRIGGER_11  =  11,
   TRIGGER_12  =  12, TRIGGER_13  =  13, TRIGGER_14  =  14, TRIGGER_15  =  15,
   TRIGGER_16  =  16, TRIGGER_17  =  17, TRIGGER_18  =  18, TRIGGER_19  =  19,
   TRIGGER_20  =  20, TRIGGER_21  =  21, TRIGGER_22  =  22, TRIGGER_23  =  23,
   TRIGGER_24  =  24, TRIGGER_25  =  25, TRIGGER_26  =  26, TRIGGER_27  =  27,
   TRIGGER_28  =  28, TRIGGER_29  =  29, TRIGGER_30  =  30, TRIGGER_31  =  31,
   TRIGGER_32  =  32, TRIGGER_33  =  33, TRIGGER_34  =  34, TRIGGER_35  =  35,
   TRIGGER_36  =  36, TRIGGER_37  =  37, TRIGGER_38  =  38, TRIGGER_39  =  39,
   TRIGGER_40  =  40, TRIGGER_41  =  41, TRIGGER_42  =  42, TRIGGER_43  =  43,
   TRIGGER_44  =  44, TRIGGER_45  =  45, TRIGGER_46  =  46, TRIGGER_47  =  47,
   TRIGGER_48  =  48, TRIGGER_49  =  49, TRIGGER_50  =  50, TRIGGER_51  =  51,
   TRIGGER_52  =  52, TRIGGER_53  =  53, TRIGGER_54  =  54, TRIGGER_55  =  55,
   TRIGGER_56  =  56, TRIGGER_57  =  57, TRIGGER_58  =  58, TRIGGER_59  =  59,
   TRIGGER_60  =  60, TRIGGER_61  =  61, TRIGGER_62  =  62, TRIGGER_63  =  63,
   TRIGGER_UNUSED = 255
} TRIGGER_INDEX;

typedef enum
{
   TR_1HZ            =  1,  /*  1Hz Rate    */
   TR_2HZ            =  2,  /*  2Hz Rate    */
   TR_4HZ            =  4,  /*  4Hz Rate    */
   TR_5HZ            =  5,  /*  5Hz Rate    */
   TR_10HZ           = 10,  /* 10Hz Rate    */
   TR_20HZ           = 20,  /* 20Hz Rate    */
   TR_50HZ           = 50   /* 50Hz Rate    */
} TRIGGER_RATE;


typedef enum
{
  LT             = 0,
  GT             = 1,
  EQUAL          = 2,
  NOT_EQUAL      = 3,
  NOT_EQUAL_PREV = 4,
  NO_COMPARE     = 5
} COMPARISON;

typedef enum
{
   TRIG_NONE,
   TRIG_START,
   TRIG_ACTIVE
} TRIG_STATE;

typedef enum
{
   TRIG_NO_END,
   TRIG_SENSOR_INVALID,
   TRIG_END_CRITERIA,
   TRIG_COMMANDED_END
} TRIG_END_TYPE;
#pragma pack(1)
typedef struct
{
   FLOAT32    fValue;
   COMPARISON compare;
} TRIG_CRITERIA;

typedef struct
{
  SENSOR_INDEX   sensorIndex;
  TRIG_CRITERIA  start;
  TRIG_CRITERIA  end;
} TRIG_SENSOR;

typedef struct
{
  INT8           triggerName[MAX_TRIGGER_NAME];   /* the name of the trigger */
  TRIG_SENSOR    trigSensor [MAX_TRIG_SENSORS];
  UINT32         nMinDuration_ms;
  TRIGGER_RATE   rate;
  UINT32         nOffset_ms;
  EVAL_EXPR      startExpr;
  EVAL_EXPR      endExpr;
} TRIGGER_CONFIG;


// Specialized Trigger version of SNSR_SUMMARY
// (Does not store times for sensor MIN and MAX
#pragma pack(1)
typedef struct
{
  SENSOR_INDEX sensorIndex;
  BOOLEAN      bValid;
  FLOAT32      fMinValue;
  FLOAT32      fMaxValue;
  FLOAT32      fAvgValue;
  FLOAT32      fTotal;
} TRIG_SNSR_SUMMARY;

typedef struct
{
   TRIGGER_INDEX      triggerIndex;
   TRIG_END_TYPE      endType;
   TIMESTAMP          criteriaMetTime;
   TIMESTAMP          durationMetTime;
   TIMESTAMP          endTime;
   UINT32             nDuration_ms;
   TRIG_SNSR_SUMMARY  sensor[MAX_TRIG_SENSORS];
} TRIGGER_LOG;

typedef struct
{
   CHAR         name[MAX_TRIGGER_NAME];
   SENSOR_INDEX sensorIndexA;
   SENSOR_INDEX sensorIndexB;
   SENSOR_INDEX sensorIndexC;
   SENSOR_INDEX sensorIndexD;
} TRIGGER_HDR;
#pragma pack()

typedef struct
{
  // Run Time Data
  TRIGGER_INDEX      triggerIndex;
  TRIG_STATE         state;
  TIMESTAMP          criteriaMetTime;
  TIMESTAMP          durationMetTime;
  TIMESTAMP          endTime;
  UINT32             nStartTime_ms;
  UINT32             nDuration_ms;
  BITARRAY128        sensorMap;      /* Bit encoded flags of sensors used. Only for legacy  */
  UINT16             nTotalSensors;  /* Total sensors being sampled */
  SNSR_SUMMARY       snsrSummary[MAX_TRIG_SENSORS];
//  FLOAT32            fValuePrevious[MAX_TRIG_SENSORS];
  INT16              nRateCounts;
  INT16              nRateCountdown;
  BOOLEAN            bStartCompareFail;
  BOOLEAN            bEndCompareFail;
  BOOLEAN            bLegacyConfig;
  TRIG_END_TYPE      endType;
} TRIGGER_DATA;


//A type for an array of the maximum number of triggers
//Used for storing sensor configurations in the configuration manager
typedef TRIGGER_CONFIG TRIGGER_CONFIGS[MAX_TRIGGERS];

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( TRIGGER_BODY )
  #define EXPORT
#else
  #define EXPORT extern
#endif
/******************************************************************************
                             Package Exports Variables
******************************************************************************/
#if defined (TRIGGER_BODY)
// Comparison enumeration for user manager and configuration
USER_ENUM_TBL comparisonEnum[] = { {"LT",             LT },
                                   {"GT",             GT },
                                   {"EQUAL",          EQUAL},
                                   {"NOT_EQUAL",      NOT_EQUAL},
                                   {"NOT_EQUAL_PREV", NOT_EQUAL_PREV},
                                   {"NO_COMPARE",     NO_COMPARE},
                                   {NULL,             0}
                                 };
#else
  EXPORT USER_ENUM_TBL comparisonEnum[];
#endif


#if defined (TRIGGER_BODY)
// Trigger index enumeration for other modules which ref TriggerIndex
USER_ENUM_TBL triggerIndexType[] =
  { { "0"  , TRIGGER_0   }, { "1"  , TRIGGER_1   }, { "2"  , TRIGGER_2   },
    { "3"  , TRIGGER_3   }, { "4"  , TRIGGER_4   }, { "5"  , TRIGGER_5   },
    { "6"  , TRIGGER_6   }, { "7"  , TRIGGER_7   }, { "8"  , TRIGGER_8   },
    { "9"  , TRIGGER_9   }, { "10" , TRIGGER_10  }, { "11" , TRIGGER_11  },
    { "12" , TRIGGER_12  }, { "13" , TRIGGER_13  }, { "14" , TRIGGER_14  },
    { "15" , TRIGGER_15  }, { "16" , TRIGGER_16  }, { "17" , TRIGGER_17  },
    { "18" , TRIGGER_18  }, { "19" , TRIGGER_19  }, { "20" , TRIGGER_20  },
    { "21" , TRIGGER_21  }, { "22" , TRIGGER_22  }, { "23" , TRIGGER_23  },
    { "24" , TRIGGER_24  }, { "25" , TRIGGER_25  }, { "26" , TRIGGER_26  },
    { "27" , TRIGGER_27  }, { "28" , TRIGGER_28  }, { "29" , TRIGGER_29  },
    { "30" , TRIGGER_30  }, { "31" , TRIGGER_31  }, { "32" , TRIGGER_32  },
    { "33" , TRIGGER_33  }, { "34" , TRIGGER_34  }, { "35" , TRIGGER_35  },
    { "36" , TRIGGER_36  }, { "37" , TRIGGER_37  }, { "38" , TRIGGER_38  },
    { "39" , TRIGGER_39  }, { "40" , TRIGGER_40  }, { "41" , TRIGGER_41  },
    { "42" , TRIGGER_42  }, { "43" , TRIGGER_43  }, { "44" , TRIGGER_44  },
    { "45" , TRIGGER_45  }, { "46" , TRIGGER_46  }, { "47" , TRIGGER_47  },
    { "48" , TRIGGER_48  }, { "49" , TRIGGER_49  }, { "50" , TRIGGER_50  },
    { "51" , TRIGGER_51  }, { "52" , TRIGGER_52  }, { "53" , TRIGGER_53  },
    { "54" , TRIGGER_54  }, { "55" , TRIGGER_55  }, { "56" , TRIGGER_56  },
    { "57" , TRIGGER_57  }, { "58" , TRIGGER_58  }, { "59" , TRIGGER_59  },
    { "60" , TRIGGER_60  }, { "61" , TRIGGER_61  }, { "62" , TRIGGER_62  },
    { "63" , TRIGGER_63  },
    { "UNUSED", TRIGGER_UNUSED }, { NULL, 0 }
  };
#else
  // Export the triggerIndex enum table
  EXPORT USER_ENUM_TBL triggerIndexType[];
#endif




/******************************************************************************
                             Package Exports Functions
******************************************************************************/
EXPORT void    TriggerInitialize   ( void );
EXPORT BOOLEAN TriggerIsActive     ( BITARRAY128 Flags );
EXPORT BOOLEAN TriggerCompareValues ( FLOAT32 LVal, COMPARISON Compare,
                                      FLOAT32 RVal, FLOAT32 PrevVal );
EXPORT UINT16  TriggerGetSystemHdr  ( void *pDest, UINT16 nMaxByteSize );
EXPORT BOOLEAN TriggerGetState     ( TRIGGER_INDEX TrigIdx );
EXPORT BOOLEAN TriggerValidGetState( TRIGGER_INDEX TrigIdx );

EXPORT BOOLEAN TriggerIsConfigured( TRIGGER_INDEX trigIdx );


#endif // TRIGGER_H
/*************************************************************************
 *  MODIFICATIONS
 *    $History: trigger.h $
 * 
 * *****************  Version 41  *****************
 * User: Contractor V&v Date: 12/05/12   Time: 4:17p
 * Updated in $/software/control processor/code/system
 * SCR #1107 Code Review 
 *
 * *****************  Version 40  *****************
 * User: Contractor V&v Date: 12/03/12   Time: 5:36p
 * Updated in $/software/control processor/code/system
 * SCR #1107 Code Review
 *
 * *****************  Version 39  *****************
 * User: Contractor V&v Date: 11/26/12   Time: 12:44p
 * Updated in $/software/control processor/code/system
 * SCR #1167 SensorSummary
 *
 * *****************  Version 38  *****************
 * User: John Omalley Date: 12-11-12   Time: 4:46p
 * Updated in $/software/control processor/code/system
 * SCR 1142 - Formatting Error
 *
 * *****************  Version 37  *****************
 * User: Contractor V&v Date: 11/08/12   Time: 4:28p
 * Updated in $/software/control processor/code/system
 * Code review
 *
 * *****************  Version 36  *****************
 * User: John Omalley Date: 12-10-23   Time: 2:56p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review Updates
 *
 * *****************  Version 35  *****************
 * User: Contractor V&v Date: 9/14/12    Time: 4:46p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 Trigger fix for using SensorArray
 *
 * *****************  Version 34  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 33  *****************
 * User: Contractor V&v Date: 7/18/12    Time: 6:27p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 Refactor for common Sensor snsrSummary
 *
 * *****************  Version 32  *****************
 * User: John Omalley Date: 12-07-13   Time: 9:03a
 * Updated in $/software/control processor/code/system
 * SCR 1124 - Packed the configuration structures for ARINC429 Mgr,
 * Sensors and Triggers. Suppressed the alignment warnings for those three
 * objects also.
 *
 * *****************  Version 31  *****************
 * User: Contractor V&v Date: 6/18/12    Time: 4:28p
 * Updated in $/software/control processor/code/system
 * FAST 2 increase operands 4 to 8
 *
 * *****************  Version 30  *****************
 * User: Contractor V&v Date: 6/07/12    Time: 7:15p
 * Updated in $/software/control processor/code/system
 * Change test for TriggerIsConfigured per EngineRun reqd review.
 *
 * *****************  Version 29  *****************
 * User: Contractor V&v Date: 5/24/12    Time: 3:07p
 * Updated in $/software/control processor/code/system
 * externalize the trigger index table so it can be asscessed by enginerun
 * et al
 *
 * *****************  Version 28  *****************
 * User: Contractor V&v Date: 4/23/12    Time: 8:02p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST2 Evaluator cleanup
 *
 * *****************  Version 27  *****************
 * User: Contractor V&v Date: 4/11/12    Time: 5:08p
 * Updated in $/software/control processor/code/system
 * FAST2  Trigger uses Evaluator
 *
 * *****************  Version 26  *****************
 * User: Contractor V&v Date: 3/21/12    Time: 6:49p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST2 Refactoring names
 *
 * *****************  Version 25  *****************
 * User: Contractor V&v Date: 3/14/12    Time: 4:52p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 Trigger processing
 *
 * *****************  Version 24  *****************
 * User: Jim Mood     Date: 7/20/11    Time: 10:57a
 * Updated in $/software/control processor/code/system
 * SCR 575: GSM Enable when engine status is lost.  (Part of changes for
 * the Fast State Machine)
 *
 * *****************  Version 23  *****************
 * User: Peter Lee    Date: 9/01/10    Time: 6:49p
 * Updated in $/software/control processor/code/system
 * SCR #845 Code Review Updates
 *
 * *****************  Version 22  *****************
 * User: Jeff Vahue   Date: 7/29/10    Time: 4:06p
 * Updated in $/software/control processor/code/system
 * SCR# 698 - cleanup
 *
 * *****************  Version 21  *****************
 * User: Contractor3  Date: 7/29/10    Time: 1:31p
 * Updated in $/software/control processor/code/system
 * SCR #698 - Fix for code review findings.
 *
 * *****************  Version 20  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:10a
 * Updated in $/software/control processor/code/system
 * SCR #698 - Fix code review findings
 *
 * *****************  Version 19  *****************
 * User: Contractor V&v Date: 7/27/10    Time: 2:27p
 * Updated in $/software/control processor/code/system
 * SCR #282 Misc - Shutdown Processing
 *
 * *****************  Version 18  *****************
 * User: John Omalley Date: 7/20/10    Time: 9:05a
 * Updated in $/software/control processor/code/system
 * SCR 294 - Add system binary header
 *
 * *****************  Version 17  *****************
 * User: Jeff Vahue   Date: 6/18/10    Time: 4:40p
 * Updated in $/software/control processor/code/system
 * SCR# 649 - add NO_COMPARE back in for use in Signal Tests.
 *
 * *****************  Version 16  *****************
 * User: John Omalley Date: 6/14/10    Time: 3:32p
 * Updated in $/software/control processor/code/system
 * SCR 546 - Add Sensor Indexes to the Trigger End Log
 *
 * *****************  Version 15  *****************
 * User: Contractor3  Date: 6/10/10    Time: 10:44a
 * Updated in $/software/control processor/code/system
 * SCR #642 - Changes based on Code Review
 *
 * *****************  Version 14  *****************
 * User: Contractor2  Date: 5/03/10    Time: 4:07p
 * Updated in $/software/control processor/code/system
 * SCR 530: Error: Trigger NO_COMPARE infinite logs
 * Removed NO_COMPARE from code.
 *
 * *****************  Version 13  *****************
 * User: John Omalley Date: 1/28/10    Time: 9:23a
 * Updated in $/software/control processor/code/system
 * SCR 408
 * * Added minimum, maximum and average for all sensors.
 *
 * *****************  Version 12  *****************
 * User: John Omalley Date: 12/17/09   Time: 5:49p
 * Updated in $/software/control processor/code/system
 * SCR 214
 * Removed Trigger END state because it was not used
 *
 * *****************  Version 11  *****************
 * User: John Omalley Date: 11/19/09   Time: 2:40p
 * Updated in $/software/control processor/code/system
 * SCR 323 -
 * Removed nStartTime_ms from the trigger logs.
 *
 * *****************  Version 10  *****************
 * User: John Omalley Date: 10/26/09   Time: 1:57p
 * Updated in $/software/control processor/code/system
 * SCR 292
 * - Rate Countdown correction
 *
 * *****************  Version 9  *****************
 * User: John Omalley Date: 10/22/09   Time: 4:25p
 * Updated in $/software/control processor/code/system
 * SCR 136
 * - Packed Log Structures
 *
 * *****************  Version 8  *****************
 * User: John Omalley Date: 10/21/09   Time: 4:14p
 * Updated in $/software/control processor/code/system
 * SCR 311
 * 1. Updated the previous logic to only compare to NOT_EQUALS.
 * 2. Calculated the rate countdown during initialization and then just
 * reload each time the countdown expires.
 * 3. Applied a delta to epsilon for floating point number comparisons
 * EQUAL and NOT_EQUAL.
 *
 ***************************************************************************/
