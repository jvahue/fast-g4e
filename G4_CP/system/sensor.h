#ifndef SENSOR_H
#define SENSOR_H
/******************************************************************************
            Copyright (C) 2009-2016 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    ECCN:        9D991

   File:         sensor.h


   Description: Definitions for sensor types

   VERSION
      $Revision: 54 $  $Date: 4/19/16 4:32p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_basic.h"
#include "faultmgr.h"

/******************************************************************************
                                 Package Defines
******************************************************************************/
#define MAX_SENSORS            128

// Default Configuration for a sensor
#define SENSOR_CALCONV_DEFAULT NONE,                   /* Conversion Type    */\
                                  1,                   /* scale (x)          */\
                                  0                    /* offset(b)          */

#define SENSOR_FILTER_DEFAULT FILTERNONE,              /* FilterType         */\
                              0.0,                     /* Spike Reject value */\
                              0,                       /* Spike Reject Pct   */\
                              0,                       /* Max Reject Count   */\
                              0,                       /* Max Percent        */\
                              0.0,                     /* Reject Value       */\
                              0.0                      /* Time Constant      */

#define SENSOR_CONFIG_DEFAULT UNUSED,                  /* Type               */\
                              "Unused",                /* SensorName         */\
                              "None",                  /* OutputUnits        */\
                              0,                       /* nInputChannel      */\
                              1,                       /* nMaximumSamples    */\
                              SSR_1HZ,                 /* SampleRate         */\
                              0,                       /* SampleOffset       */\
                              SENSOR_CALCONV_DEFAULT,  /* Calibration        */\
                              SENSOR_CALCONV_DEFAULT,  /* Conversion         */\
                              SENSOR_FILTER_DEFAULT,   /* Filter             */\
                              STA_NORMAL,              /* SystemCondition    */\
                              TRUE,                    /* SelfHeal           */\
                              FALSE,                   /* RangeTest          */\
                              0,                       /* RangeDuration_ms   */\
                              0,                       /* fMinValue          */\
                              0,                       /* fMaxValue          */\
                              FALSE,                   /* RateTest           */\
                              0,                       /* RateThreshold      */\
                              0,                       /* RateDuration_ms    */\
                              FALSE,                   /* SignalTest         */\
                              FAULT_UNUSED,            /* FaultIndex         */\
                              0,                       /* SignalDuration_ms  */\
                              FALSE,                   /* bInspectInclude    */\
                              0,                       /* GeneralPurposeA    */\
                              0,                       /* GeneralPurposeB    */

//Default configuration for the sensors
#define SENSOR_CONFIGS_DEFAULT {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                                                       \
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                                                       \
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                                                       \
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT},\
                               {SENSOR_CONFIG_DEFAULT}

#define SENSOR_LIVEDATA_DEFAULT   LD_NONE, /*GSE LD Type*/\
                                  5000     /*Rate*/

#define MAX_SAMPLES         8   // Maximum number of samples stored
#define MAX_SENSORNAME     32   // Total allowed name characters
#define MAX_SENSORUNITS    16   // Total allowed sensor unit chars
#define MAX_CONV_PARAMS     2   // Total Linear Conversion parameters

#define SENSOR_LD_MAX_RATE  500

#define LD_SNSR_LEN 3
#define LD_DELIM    2
#define GETLIST_BUFF_SIZE (LD_SNSR_LEN + MAX_SENSORNAME + MAX_SENSORUNITS + LD_DELIM)

// Macro to unpack Virtual sensor info from a GPA word
#define UNPACK_VIRTUAL_SNRA(word)((word & 0x0000FF00) >> 8)
#define UNPACK_VIRTUAL_SNRB(word)((word & 0x000000FF))
#define UNPACK_VIRTUAL_TYPE(word)((word & 0x00070000) >> 16)
#define UNPACK_VIRTUAL_RAW_A_SIZE(word) ((word & 0x3E000000) >> 25)
#define UNPACK_VIRTUAL_RAW_B_SIZE(word) ((word & 0x01F00000) >> 20)
#define UNPACK_VIRTUAL_RAW_NEGATIVE(word) ((word & 0x40000000) >> 30)

/******************************************************************************
                                 Package Typedefs
******************************************************************************/
typedef enum
{
  SENSOR_0    =    0, SENSOR_1    =    1, SENSOR_2    =   2, SENSOR_3   =    3,
  SENSOR_4    =    4, SENSOR_5    =    5, SENSOR_6    =   6, SENSOR_7   =    7,
  SENSOR_8    =    8, SENSOR_9    =    9, SENSOR_10   =  10, SENSOR_11  =   11,
  SENSOR_12   =   12, SENSOR_13   =   13, SENSOR_14   =  14, SENSOR_15  =   15,
  SENSOR_16   =   16, SENSOR_17   =   17, SENSOR_18   =  18, SENSOR_19  =   19,
  SENSOR_20   =   20, SENSOR_21   =   21, SENSOR_22   =  22, SENSOR_23  =   23,
  SENSOR_24   =   24, SENSOR_25   =   25, SENSOR_26   =  26, SENSOR_27  =   27,
  SENSOR_28   =   28, SENSOR_29   =   29, SENSOR_30   =  30, SENSOR_31  =   31,
  SENSOR_32   =   32, SENSOR_33   =   33, SENSOR_34   =  34, SENSOR_35  =   35,
  SENSOR_36   =   36, SENSOR_37   =   37, SENSOR_38   =  38, SENSOR_39  =   39,
  SENSOR_40   =   40, SENSOR_41   =   41, SENSOR_42   =  42, SENSOR_43  =   43,
  SENSOR_44   =   44, SENSOR_45   =   45, SENSOR_46   =  46, SENSOR_47  =   47,
  SENSOR_48   =   48, SENSOR_49   =   49, SENSOR_50   =  50, SENSOR_51  =   51,
  SENSOR_52   =   52, SENSOR_53   =   53, SENSOR_54   =  54, SENSOR_55  =   55,
  SENSOR_56   =   56, SENSOR_57   =   57, SENSOR_58   =  58, SENSOR_59  =   59,
  SENSOR_60   =   60, SENSOR_61   =   61, SENSOR_62   =  62, SENSOR_63  =   63,
  SENSOR_64   =   64, SENSOR_65   =   65, SENSOR_66   =  66, SENSOR_67  =   67,
  SENSOR_68   =   68, SENSOR_69   =   69, SENSOR_70   =  70, SENSOR_71  =   71,
  SENSOR_72   =   72, SENSOR_73   =   73, SENSOR_74   =  74, SENSOR_75  =   75,
  SENSOR_76   =   76, SENSOR_77   =   77, SENSOR_78   =  78, SENSOR_79  =   79,
  SENSOR_80   =   80, SENSOR_81   =   81, SENSOR_82   =  82, SENSOR_83  =   83,
  SENSOR_84   =   84, SENSOR_85   =   85, SENSOR_86   =  86, SENSOR_87  =   87,
  SENSOR_88   =   88, SENSOR_89   =   89, SENSOR_90   =  90, SENSOR_91  =   91,
  SENSOR_92   =   92, SENSOR_93   =   93, SENSOR_94   =  94, SENSOR_95  =   95,
  SENSOR_96   =   96, SENSOR_97   =   97, SENSOR_98   =  98, SENSOR_99  =   99,
  SENSOR_100  =  100, SENSOR_101  =  101, SENSOR_102  = 102, SENSOR_103 =  103,
  SENSOR_104  =  104, SENSOR_105  =  105, SENSOR_106  = 106, SENSOR_107 =  107,
  SENSOR_108  =  108, SENSOR_109  =  109, SENSOR_110  = 110, SENSOR_111 =  111,
  SENSOR_112  =  112, SENSOR_113  =  113, SENSOR_114  = 114, SENSOR_115 =  115,
  SENSOR_116  =  116, SENSOR_117  =  117, SENSOR_118  = 118, SENSOR_119 =  119,
  SENSOR_120  =  120, SENSOR_121  =  121, SENSOR_122  = 122, SENSOR_123 =  123,
  SENSOR_124  =  124, SENSOR_125  =  125, SENSOR_126  = 126, SENSOR_127 =  127,
  SENSOR_UNUSED = 255
} SENSOR_INDEX;

typedef enum
{
   FAULT_0   =   0, FAULT_1   =   1, FAULT_2   =   2, FAULT_3   =   3,
   FAULT_4   =   4, FAULT_5   =   5, FAULT_6   =   6, FAULT_7   =   7,
   FAULT_8   =   8, FAULT_9   =   9, FAULT_10  =  10, FAULT_11  =  11,
   FAULT_12  =  12, FAULT_13  =  13, FAULT_14  =  14, FAULT_15  =  15,
   FAULT_16  =  16, FAULT_17  =  17, FAULT_18  =  18, FAULT_19  =  19,
   FAULT_20  =  20, FAULT_21  =  21, FAULT_22  =  22, FAULT_23  =  23,
   FAULT_24  =  24, FAULT_25  =  25, FAULT_26  =  26, FAULT_27  =  27,
   FAULT_28  =  28, FAULT_29  =  29, FAULT_30  =  30, FAULT_31  =  31,
   FAULT_UNUSED = 255
} FAULT_INDEX;

typedef enum
{
  NONE               = 1,   /* No Conversion  */
  LINEAR             = 2    /* Linear Mapping */
} CONVERSIONTYPE;

typedef enum
{
  LINEAR_SLOPE   = 0,          /* If linear, SLOPE  is this index */
  LINEAR_OFFSET  = 1           /* If linear, OFFSET is this index */
} LINEAR_CONV;

typedef enum
{
    UNUSED            = 0,   /* Sensor is not used */
    SAMPLE_ARINC429,         /* Arinc 429 Data */
    SAMPLE_QAR,              /* Arinc 717 Data     */
    SAMPLE_ANALOG,           /* Analog Data        */
    SAMPLE_DISCRETE,         /* Discrete Data      */
    SAMPLE_UART,             /* Uart Data          */
    //SAMPLE_SERIAL,
    SAMPLE_VIRTUAL,          /* Sensor is Virtual  */
    /*-end-of-list-*/
    MAX_SAMPLETYPE           /* For validation */
} SAMPLETYPE;

typedef enum
{
   SSR_1HZ            =  1,  /* Sensor  1Hz Rate    */
   SSR_2HZ            =  2,  /* Sensor  2Hz Rate    */
   SSR_4HZ            =  4,  /* Sensor  4Hz Rate    */
   SSR_5HZ            =  5,  /* Sensor  5Hz Rate    */
   SSR_10HZ           = 10,  /* Sensor 10Hz Rate    */
   SSR_20HZ           = 20,  /* Sensor 20Hz Rate    */
   SSR_50HZ           = 50   /* Sensor 50Hz Rate    */
} SENSOR_SAMPLERATE;

typedef enum
{
  LD_NONE,                   /* No Live Data Displayed     */
  LD_ASCII,                  /* ASCII live Data Displayed  */
  LD_BINARY                  /* BINARY live Data Displayed */
} SENSOR_LD_ENUM;

typedef enum
{
  LD_DEST_GSE,
  LD_DEST_MS,
  LD_DEST_MAX
} LD_DEST_ENUM;

typedef enum
{
   FILTERNONE         = 1,   /* No smoothing takes place        */
   EXPO_AVERAGE       = 2,   /* Exponential Smoothing           */
   SPIKEREJECTION     = 3,   /* Spike rejection                 */
   MAXREJECT          = 4,   /* reject max values, average rest */
   SLOPEFILTER        = 5    /* Slope over a number of values   */
} FILTERTYPE;

typedef enum
{
   SENSOR_OK,
   SENSOR_FAILING,
   SENSOR_FAILED
} SENSOR_STATUS;

typedef enum
{
    NO_FAILURE,              /* No Test failure   */
    BIT_FAILURE,             /* BIT Test failure  */
    SIGNAL_FAILURE,          /* Signal Failure    */
    RANGE_FAILURE,           /* Range Failure     */
    RATE_FAILURE,            /* Rate Failure      */
    MAX_FAILURE
} SENSORFAILURE;

// Virtual sensor type(operation)
typedef enum
{
  VIRTUAL_UNUSED = 0,
  VIRTUAL_ADD,
  VIRTUAL_DIFF,
  VIRTUAL_MULT,
  VIRTUAL_DIV,
  VIRTUAL_RAW_COMBINE, 
  VIRTUAL_MAX
}VIRTUALTYPE;

typedef FLOAT32 (*GET_SENSOR) (UINT16, UINT32* );
typedef BOOLEAN (*RUN_TEST) (UINT16);
typedef BOOLEAN (*INTERFACE_ACTIVE) (UINT16);

// Definition of a sensor value summary: min, max, avg, total.
#pragma pack(1)
typedef struct
{
  SENSOR_INDEX SensorIndex;
  UINT32       nSampleCount;
  BOOLEAN      bValid;
  TIMESTAMP    timeMinValue;
  FLOAT32      fMinValue;
  TIMESTAMP    timeMaxValue;
  FLOAT32      fMaxValue;
  FLOAT32      fAvgValue;
  FLOAT32      fTotal;
} SNSR_SUMMARY;
#pragma pack()

#pragma pack(1)
// Definition of a conversion configuration
typedef struct
{
    CONVERSIONTYPE  type;                       /* Type of conversion     */
    FLOAT32         fParams[MAX_CONV_PARAMS];   /* Specific to conversion */
} CONVERSION;

// Definition of a filter configuration
typedef struct
{
  FILTERTYPE      type;              /* Type of conversion                      */
  FLOAT32         fFullScale;        /* Spike Rejection 100% value              */
  UINT8           nSpikeRejectPct;   /* percent to use for reject test 10 = 10% */
  UINT8           nMaxRejectCount;   /* maximum consecutive rejects             */
  UINT16          nMaxPct;           /* absolute max percent allowed            */
  FLOAT32         fRejectValue;      /* absolute maximum value allowed          */
  FLOAT32         fTimeConstant;     /* smoothing filter */

}FILTER_CONFIG;

// Definition of a sensor configuration.
typedef struct
{
    SAMPLETYPE        type;                          /* Type of samples to be taken */
    CHAR              sSensorName[MAX_SENSORNAME];   /* "Engine RPM", etc. */
    CHAR              sOutputUnits[MAX_SENSORUNITS]; /* Units after conversion
                                                      *   (ie - "RPM", "Deg. C", etc.) */
    UINT16            nInputChannel;                 /* Input channel, specific to type of
                                                      * sensor (ie - which analog channel)*/
                                                     /* "Engine RPM", etc. */
    UINT8             nMaximumSamples;               /* Maximum Number of Samples to take */

    SENSOR_SAMPLERATE sampleRate;                    /* Rate to sample (1,2,5,10,20,50Hz) */
    UINT16            nSampleOffset_ms;              /* Offset to start sampling in mS    */

    CONVERSION        calibration;                   /* Calibration calculation */
    CONVERSION        conversion;                    /* Conversion to perform */
    FILTER_CONFIG     filterCfg;                     /* Filter Configuration              */

    FLT_STATUS        systemCondition;               /* System condition state on BIT Fail*/
    BOOLEAN           bSelfHeal;                     /* Can the sensor heal itself        */

    BOOLEAN           bRangeTest;                    /* Is there a range test */
    UINT16            nRangeDuration_ms;             /* Number of milliseconds to wait    */
    FLOAT32           fMinValue;                     /* Min value for proper operation */
    FLOAT32           fMaxValue;                     /* Max value for proper operation */

    BOOLEAN           bRateTest;                     /* Is there a rate test              */
    FLOAT32           fRateThreshold;                /* Rate to be exceeded               */
    UINT16            nRateDuration_ms;              /* Number of milliseconds to wait    */

    BOOLEAN           bSignalTest;                   /* Is there a signal test            */
    FAULT_INDEX       signalTestIndex;               /* Which fault to use                */
    UINT16            nSignalDuration_ms;            /* Number of milliseconds to wait    */

    BOOLEAN           bInspectInclude;               /* TRUE if value should be outputted */
    UINT32            generalPurposeA;               /* General Purpose Field A */
    UINT32            generalPurposeB;               /* General Purpose Field B */
} SENSOR_CONFIG;

// Sensor Live Data Configuration
typedef struct
{
   SENSOR_LD_ENUM gseType;    /* type of live data output to GSE, ASCII or BINARY */
   UINT32         nRate_ms;   /* Rate in milliseconds to output live data */
}SENSOR_LD_CONFIG;
#pragma pack()

//A type for an array of the maximum number of sensors
//Used for storing sensor configurations in the configuration manager
typedef SENSOR_CONFIG SENSOR_CONFIGS[MAX_SENSORS];

// Storage container definition for Filter Data
typedef struct
{
  /*Internal use only, to track the counts ***********************************************/
  FLOAT32         fLastValidValue;   /* for spike rejection algorithm                    */
  FLOAT32         fLastAvgValue;     /* for Exponential and MaxReject                    */
  UINT8           nRejectCount;      /* Number of rejects detected (Spike & Max Reject)  */
  FLOAT32         fK;                 /* fK constant for the filter computation            */
  /* for slope filter only ***************************************************************/
  FLOAT32         nMinusOneOverTwo;  /* term in slope computation                        */
  /* For spike rejection filter only *****************************************************/
  FLOAT32 fullScaleInv;              /* 1.0/fullScale                                    */
  FLOAT32 maxPercent;                /* nMaxPct/100                                      */
  FLOAT32 spikePercent;              /* nSpikeRejectPct/100                              */
  /* For Max reject filter only **********************************************************/
  BOOLEAN persistState;              /* state of the persist counter                     */
  BOOLEAN persistStateZ1;            /* state of the persist counter last execution      */
  /***************************************************************************************/
} FILTER_DATA;

// Storage Container definition for Sensor Data
typedef struct
{
    SENSOR_INDEX      nSensorIndex;                 /* Index of Sensor                   */
    BOOLEAN           bValueIsValid;                /* TRUE if value checks out OK       */
    BOOLEAN           bWasValidOnce;                /* Sensor was Valid at least once    */
    FLOAT32           fValue;                       /* Filtered/converted value          */
    FLOAT32           fPriorValue;                  /* previous value (for rate test)    */
    UINT32            lastUpdateTick;               /* Last update tick time of sensor   */
    BOOLEAN           bPriorValueValid;             /* TRUE if prior value calculated    */
    INT16             nSampleCounts;                /* Number of cycles between sample   */
    INT16             nSampleCountdown;             /* Countdown until next sample       */
    UINT8             nNextSample;                  /* Array index for next sample       */
    FLOAT32           fSamples[MAX_SAMPLES];        /* Saved raw samples                 */
    UINT8             nCurrentSamples;              /* # of samples currently obtained   */
    BOOLEAN           bOutofRange;                  /* Sensor has gone out of range      */
    BOOLEAN           bRangeFail;                   /* Flag to indicate Range has failed */
    UINT32            nRangeTimeout_ms;             /* Time when Range test will fail    */
    BOOLEAN           bRateExceeded;                /* Sensor rate exceeded              */
    BOOLEAN           bRateFail;                    /* Sensor Rate Failed                */
    UINT32            nRateTimeout_ms;              /* Time when rate test will fail     */
    FLOAT32           fRateValue;                   /* Last Calculated Slope             */
    FLOAT32           fRatePrevVal;                 /* Previous fVal seen by Rate logic  */
    BOOLEAN           bRatePrevDetected;            /* Previous Val saved by Rate logic  */
    BOOLEAN           bSignalBad;                   /* The signal is faulted             */
    BOOLEAN           bSignalFail;                  /* The signal is failed              */
    UINT32            nSignalTimeout_ms;            /* Duration that must be met to fail */
    BOOLEAN           bBITFail;                     /* Sensor has a BIT failure          */
    UINT16            nInterfaceIndex;              /* Interface index to data           */
    FILTER_DATA       filterData;                   /* Realtime Dynamic Filter Data      */
    GET_SENSOR        pGetSensorData;               /* Pointer to function to get data   */
    RUN_TEST          pTestSensor;                  /* Ptr to func that tests the sensor */
    INTERFACE_ACTIVE  pInterfaceActive;             /* Ptr to func that tests activity   */
    VIRTUALTYPE       vOpType;                      /* The op to perform on input A and B*/
    SENSOR_INDEX      vSnsrA;                       /* The index of input sensor A       */
    SENSOR_INDEX      vSnsrB;                       /* The index of input sensor B       */
    UINT16            vRawCombineAsize;             /* For VIRTUAL "COMBINE", the # bits to 
                                                       parse from SensorA */
    UINT16            vRawCombineBsize;             /* For VIRTUAL "COMBINE", the # bits to 
                                                       parse from SensorB */
    BOOLEAN           vRawCombineNeg;               /* For VIRTUAL "COMBINE", combine A + B
                                                         is a signed value  */
} SENSOR;

// Storage container definition for Sensor failure log
#pragma pack(1)
typedef struct
{
    SENSORFAILURE failureType; /* what kind of sensor failure occurred */
    SENSOR_INDEX  nIndex;               /* Index of sensor that failed         */
    INT8          sName[MAX_SENSORNAME];/* Name of the sensor that failed      */
    FLOAT32       fCurrentValue;        /* Current Value of sensor             */
    FLOAT32       fValue;               /* Valid Value being reported          */
    FLOAT32       fPreviousValue;       /* Previous Valid Value of sensor      */
    FLOAT32       fExpectedMin;         /*                                     */
    FLOAT32       fExpectedMax;         /*                                     */
    FLOAT32       fExpectedThreshold;   /*                                     */
    FLOAT32       fRateValue;           /*                                     */
    UINT32        nDuration_ms;         /*                                     */
} SENSORLOG;

typedef struct
{
   CHAR sName[MAX_SENSORNAME];
   CHAR sUnits[MAX_SENSORUNITS];
} SENSOR_HDR;

typedef struct
{
   BOOLEAN    configured;
   SENSOR_HDR sensor;
} SENSOR_ETM_HDR;

#pragma pack()

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( SENSOR_BODY )
  #define EXPORT
#else
  #define EXPORT extern
#endif
/******************************************************************************
                             Package Exports Variables
******************************************************************************/
#if !defined( SENSOR_BODY )
EXPORT USER_ENUM_TBL SensorIndexType[];
#endif

/******************************************************************************
                             Package Exports Functions
******************************************************************************/
EXPORT BOOLEAN SensorIsUsed            ( SENSOR_INDEX Sensor );
EXPORT BOOLEAN SensorIsUsedEx          ( SENSOR_INDEX sensorIdx, BOOLEAN* bIsUsed);
EXPORT BOOLEAN SensorIsValid           ( SENSOR_INDEX Sensor );
EXPORT FLOAT32 SensorGetValue          ( SENSOR_INDEX Sensor );
EXPORT BOOLEAN SensorGetValueEx        ( SENSOR_INDEX sensorIdx, FLOAT32* fValue);
EXPORT FLOAT32 SensorGetPreviousValue  ( SENSOR_INDEX Sensor );
EXPORT void    SensorsInitialize       ( void );
EXPORT void    SensorDisableLiveStream ( void );
EXPORT UINT16  SensorGetSystemHdr      ( void *pDest, UINT16 nMaxByteSize );
EXPORT UINT16  SensorGetETMHdr         ( void *pDest, UINT16 nMaxByteSize );
EXPORT UINT32  SensorGetLastUpdateTime ( SENSOR_INDEX Sensor);

// Functions for managing SNSR_SUMMARY objects
EXPORT UINT16  SensorInitSummaryArray    ( SNSR_SUMMARY summary[], INT32 summarySize,
                                           UINT32 snsrMask[],      INT32 snsrMaskSizeBytes );
EXPORT void    SensorResetSummaryArray   ( SNSR_SUMMARY summary[],      UINT16 nEntries );
EXPORT void    SensorUpdateSummaries     ( SNSR_SUMMARY summaryArray[], UINT16 nEntries );
EXPORT void    SensorCalculateSummaryAvgs( SNSR_SUMMARY summaryArray[], UINT16 nEntries);



#endif // SENSOR_H
/*****************************************************************************
 *  MODIFICATIONS
 *    $History: sensor.h $
 * 
 * *****************  Version 54  *****************
 * User: Contractor V&v Date: 4/19/16    Time: 4:32p
 * Updated in $/software/control processor/code/system
 * SCR #1328 Add support for vSensor Raw Combine
 * 
 * *****************  Version 53  *****************
 * User: Contractor V&v Date: 2/01/16    Time: 5:22p
 * Updated in $/software/control processor/code/system
 * SCR #1192 - Perf Enhancment improvements 
 * 
 * *****************  Version 52  *****************
 * User: Contractor V&v Date: 11/05/15   Time: 6:31p
 * Updated in $/software/control processor/code/system
 * SCR #1299 - P100 Switch to ENUM types vs UINT8
 * 
 * *****************  Version 51  *****************
 * User: Contractor V&v Date: 11/02/15   Time: 6:22p
 * Updated in $/software/control processor/code/system
 * SCR #1299 - P100 Use ASSERT instead of log msg and invalidated 
 * sensors
 * 
 * *****************  Version 50  *****************
 * User: Contractor V&v Date: 9/09/15    Time: 6:42p
 * Updated in $/software/control processor/code/system
 * P100 Update for virtual sensor validation and error logging.
 * 
 * *****************  Version 49  *****************
 * User: Contractor V&v Date: 8/26/15    Time: 7:19p
 * Updated in $/software/control processor/code/system
 * SCR #1299 - P100 Implement the Deferred Virtual Sensor Requirements
 *
 * *****************  Version 48  *****************
 * User: Contractor V&v Date: 11/03/14   Time: 5:21p
 * Updated in $/software/control processor/code/system
 * SCR #1262 - LiveData CP to MS Modfix for ms_type
 *
 * *****************  Version 47  *****************
 * User: Peter Lee    Date: 14-10-08   Time: 7:10p
 * Updated in $/software/control processor/code/system
 * SCR #1263 ID Param Protocol Implementation
 *
 * *****************  Version 46  *****************
 * User: Contractor V&v Date: 9/22/14    Time: 6:46p
 * Updated in $/software/control processor/code/system
 * SCR #1262 - LiveData CP to MS
 *
 * *****************  Version 45  *****************
 * User: John Omalley Date: 12-12-08   Time: 1:32p
 * Updated in $/software/control processor/code/system
 * SCR 1167 - Sensor Summary Init optimization
 *
 * *****************  Version 44  *****************
 * User: Contractor V&v Date: 12/05/12   Time: 4:17p
 * Updated in $/software/control processor/code/system
 * SCR #1107 Bitbucket #80
 *
 * *****************  Version 43  *****************
 * User: John Omalley Date: 12-12-02   Time: 12:22p
 * Updated in $/software/control processor/code/system
 * SCR 1197 - Code Review Updates
 *
 * *****************  Version 42  *****************
 * User: Contractor V&v Date: 11/26/12   Time: 6:05p
 * Updated in $/software/control processor/code/system
 * SCR #1107 Code Review
 *
 * *****************  Version 41  *****************
 * User: Contractor V&v Date: 11/26/12   Time: 12:42p
 * Updated in $/software/control processor/code/system
 * SCR #1167 SensorSummary
 *
 * *****************  Version 40  *****************
 * User: John Omalley Date: 12-11-16   Time: 10:32p
 * Updated in $/software/control processor/code/system
 * SCR 1197 - Code Review Updates
 *
 * *****************  Version 39  *****************
 * User: John Omalley Date: 12-11-12   Time: 4:46p
 * Updated in $/software/control processor/code/system
 * SCR 1142 - Formatting Error
 *
 * *****************  Version 38  *****************
 * User: John Omalley Date: 12-11-12   Time: 11:36a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review Updates
 *
 * *****************  Version 37  *****************
 * User: Peter Lee    Date: 12-10-27   Time: 5:04p
 * Updated in $/software/control processor/code/system
 * SCR #1191 Returns update time of param
 *
 * *****************  Version 36  *****************
 * User: Contractor V&v Date: 9/14/12    Time: 4:48p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 fixed SensorSummary handling
 *
 * *****************  Version 35  *****************
 * User: John Omalley Date: 12-09-11   Time: 2:13p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Added Binary ETM Header
 *
 * *****************  Version 34  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 33  *****************
 * User: John Omalley Date: 12-08-28   Time: 8:35a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Updated the sensor summary to include the time of min and
 * time of max.
 *
 * *****************  Version 32  *****************
 * User: Contractor V&v Date: 7/18/12    Time: 6:27p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 Refactor for common Sensor Summary
 *
 * *****************  Version 31  *****************
 * User: John Omalley Date: 12-07-13   Time: 9:03a
 * Updated in $/software/control processor/code/system
 * SCR 1124 - Packed the configuration structures for ARINC429 Mgr,
 * Sensors and Triggers. Suppressed the alignment warnings for those three
 * objects also.
 *
 * *****************  Version 30  *****************
 * User: Contractor V&v Date: 4/11/12    Time: 5:04p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST2  Accessor API for PriorValidity & 128sensors
 *
 * *****************  Version 29  *****************
 * User: Contractor2  Date: 10/04/10   Time: 4:31p
 * Updated in $/software/control processor/code/system
 * SCR #916 Code Review Updates
 *
 * *****************  Version 28  *****************
 * User: Contractor2  Date: 10/04/10   Time: 1:27p
 * Updated in $/software/control processor/code/system
 * SCR #916 Code Review Updates
 *
 * *****************  Version 27  *****************
 * User: John Omalley Date: 9/09/10    Time: 2:15p
 * Updated in $/software/control processor/code/system
 * SCR 855 - Removed unused functions
 *
 * *****************  Version 26  *****************
 * User: John Omalley Date: 7/20/10    Time: 9:04a
 * Updated in $/software/control processor/code/system
 * SCR 294 - Add system binary header
 *
 * *****************  Version 25  *****************
 * User: Contractor V&v Date: 6/22/10    Time: 6:25p
 * Updated in $/software/control processor/code/system
 * SCR #485 Escape Sequence and Box Configuration
 *
 * *****************  Version 24  *****************
 * User: Contractor2  Date: 6/07/10    Time: 1:29p
 * Updated in $/software/control processor/code/system
 * SCR #486 Livedata enhancement
 *
 * *****************  Version 23  *****************
 * User: Peter Lee    Date: 5/31/10    Time: 6:27p
 * Updated in $/software/control processor/code/system
 * SCR #618 F7X and UartMgr Requirements Implementation
 *
 * *****************  Version 22  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 21  *****************
 * User: Jeff Vahue   Date: 2/09/10    Time: 11:20a
 * Updated in $/software/control processor/code/system
 * SCR# 404 - remove unused sensor status cmds
 *
 * *****************  Version 20  *****************
 * User: Jeff Vahue   Date: 1/19/10    Time: 3:52p
 * Updated in $/software/control processor/code/system
 * SCR# 399
 *
 * *****************  Version 19  *****************
 * User: John Omalley Date: 1/15/10    Time: 5:29p
 * Updated in $/software/control processor/code/system
 * SCR 296
 * * Updated the sensor read procesing to check if the interface is alive.
 *
 * *****************  Version 18  *****************
 * User: Jeff Vahue   Date: 1/11/10    Time: 12:23p
 * Updated in $/software/control processor/code/system
 * SCR# 388
 *
 * *****************  Version 17  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:27p
 * Updated in $/software/control processor/code/system
 * SCR 18
 *
 * *****************  Version 16  *****************
 * User: Jeff Vahue   Date: 12/07/09   Time: 4:25p
 * Updated in $/software/control processor/code/system
 * SCR# 364 - set sensor test durations defaults per requirements
 *
 * *****************  Version 15  *****************
 * User: Jeff Vahue   Date: 12/03/09   Time: 5:00p
 * Updated in $/software/control processor/code/system
 * SCR# 350
 *
 * *****************  Version 14  *****************
 * User: John Omalley Date: 11/24/09   Time: 2:14p
 * Updated in $/software/control processor/code/system
 * SCR 349
 * * Fixed self healing for the rate test.
 * * Set the rate value before the absolute is calculated.
 * * Updated the rate calculation
 * * Corrected the current and previous value lock mechanism.
 *
 * *****************  Version 13  *****************
 * User: Peter Lee    Date: 11/02/09   Time: 1:57p
 * Updated in $/software/control processor/code/system
 * Misc non code update to support WIN32 version and spelling.
 *
 * *****************  Version 12  *****************
 * User: John Omalley Date: 10/26/09   Time: 1:54p
 * Updated in $/software/control processor/code/system
 * SCR 292
 * - Updated the rate countdown logic because the 1st calculated countdonw
 * was incorrect.
 *
 * *****************  Version 11  *****************
 * User: John Omalley Date: 10/22/09   Time: 4:24p
 * Updated in $/software/control processor/code/system
 * SCR 136
 * - Packed log structures
 *
 * *****************  Version 10  *****************
 * User: John Omalley Date: 10/20/09   Time: 3:35p
 * Updated in $/software/control processor/code/system
 * SCR 310
 * - Added last valid value logic
 * - Added countdown precalc
 * - Added time based checks >=
 *
 * *****************  Version 9  *****************
 * User: John Omalley Date: 10/14/09   Time: 5:15p
 * Updated in $/software/control processor/code/system
 * SCR 298 - Update the name size to 32 characters
 *
 * *****************  Version 8  *****************
 * User: John Omalley Date: 10/07/09   Time: 5:46p
 * Updated in $/software/control processor/code/system
 * SCR 283 - Added FATAL assert logic to default cases
 *
 * *****************  Version 7  *****************
 * User: John Omalley Date: 10/06/09   Time: 5:30p
 * Updated in $/software/control processor/code/system
 * Updated to remove multiple system condition configuration items
 *
 * *****************  Version 6  *****************
 * User: John Omalley Date: 10/01/09   Time: 4:27p
 * Updated in $/software/control processor/code/system
 * Implemented missing requirements.
 *
 ******************************************************************************/

