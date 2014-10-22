#define SENSOR_USERTABLES_BODY
/******************************************************************************
Copyright (C) 2008-2014 Pratt & Whitney Engine Services, Inc.
All Rights Reserved. Proprietary and Confidential.

File:        SensorUserTables.c

Description: User Interface for Sensor Runtime Processing

VERSION
$Revision: 29 $  $Date: 10/20/14 3:55p $

******************************************************************************/
#ifndef SENSOR_BODY
#error SensorUserTables.c should only be included by sensor.c
#endif

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include <stdlib.h>
#include <string.h>

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "CfgManager.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
static USER_HANDLER_RESULT Sensor_UserCfg ( USER_DATA_TYPE DataType,
                                            USER_MSG_PARAM Param,
                                            UINT32 Index,
                                            const void *SetPtr,
                                            void **GetPtr );
static USER_HANDLER_RESULT Sensor_State   ( USER_DATA_TYPE DataType,
                                            USER_MSG_PARAM Param,
                                            UINT32 Index,
                                            const void *SetPtr,
                                            void **GetPtr );
static USER_HANDLER_RESULT Sensor_Live_Data_Cfg ( USER_DATA_TYPE DataType,
                                                  USER_MSG_PARAM Param,
                                                  UINT32 Index,
                                                  const void *SetPtr,
                                                  void **GetPtr );
static USER_HANDLER_RESULT Sensor_ShowConfig ( USER_DATA_TYPE DataType,
                                               USER_MSG_PARAM Param,
                                               UINT32 Index,
                                               const void *SetPtr,
                                               void **GetPtr );
static USER_HANDLER_RESULT Sensor_LiveDataList( USER_DATA_TYPE DataType,
                                                USER_MSG_PARAM Param,
                                                UINT32 Index,
                                                const void *SetPtr,
                                                void **GetPtr);

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
//static SENSOR             Sensors[MAX_SENSORS]; // Dynamic Sensor Storage
static SENSOR_CONFIG      configTemp;      // Temporary Sensor config block
static SENSOR             sensorTemp;      // Temporary Sensor Data block
static SENSOR_LD_CONFIG   liveDataTemp;    // Temporary Live Data Cfg Block

// Sensor User Configuration Support


USER_ENUM_TBL SensorIndexType[]   =
{ { "0"  , SENSOR_0   }, { "1"  ,  SENSOR_1   }, {  "2"  , SENSOR_2   },
  { "3"  , SENSOR_3   }, { "4"  ,  SENSOR_4   }, {  "5"  , SENSOR_5   },
  { "6"  , SENSOR_6   }, { "7"  ,  SENSOR_7   }, {  "8"  , SENSOR_8   },
  { "9"  , SENSOR_9   }, { "10" ,  SENSOR_10  }, {  "11" , SENSOR_11  },
  { "12" , SENSOR_12  }, { "13" ,  SENSOR_13  }, {  "14" , SENSOR_14  },
  { "15" , SENSOR_15  }, { "16" ,  SENSOR_16  }, {  "17" , SENSOR_17  },
  { "18" , SENSOR_18  }, { "19" ,  SENSOR_19  }, {  "20" , SENSOR_20  },
  { "21" , SENSOR_21  }, { "22" ,  SENSOR_22  }, {  "23" , SENSOR_23  },
  { "24" , SENSOR_24  }, { "25" ,  SENSOR_25  }, {  "26" , SENSOR_26  },
  { "27" , SENSOR_27  }, { "28" ,  SENSOR_28  }, {  "29" , SENSOR_29  },
  { "30" , SENSOR_30  }, { "31" ,  SENSOR_31  }, {  "32" , SENSOR_32  },
  { "33" , SENSOR_33  }, { "34" ,  SENSOR_34  }, {  "35" , SENSOR_35  },
  { "36" , SENSOR_36  }, { "37" ,  SENSOR_37  }, {  "38" , SENSOR_38  },
  { "39" , SENSOR_39  }, { "40" ,  SENSOR_40  }, {  "41" , SENSOR_41  },
  { "42",  SENSOR_42  }, { "43" ,  SENSOR_43  }, {  "44" , SENSOR_44  },
  { "45",  SENSOR_45  }, { "46" ,  SENSOR_46  }, {  "47" , SENSOR_47  },
  { "48",  SENSOR_48  }, { "49" ,  SENSOR_49  }, {  "50" , SENSOR_50  },
  { "51",  SENSOR_51  }, { "52" ,  SENSOR_52  }, {  "53" , SENSOR_53  },
  { "54",  SENSOR_54  }, { "55" ,  SENSOR_55  }, {  "56" , SENSOR_56  },
  { "57",  SENSOR_57  }, { "58" ,  SENSOR_58  }, {  "59" , SENSOR_59  },
  { "60",  SENSOR_60  }, { "61" ,  SENSOR_61  }, {  "62" , SENSOR_62  },
  { "63",  SENSOR_63  }, { "64" ,  SENSOR_64  }, {  "65" , SENSOR_65  },
  { "66",  SENSOR_66  }, { "67" ,  SENSOR_67  }, {  "68" , SENSOR_68  },
  { "69",  SENSOR_69  }, { "70" ,  SENSOR_70  }, {  "71" , SENSOR_71  },
  { "72",  SENSOR_72  }, { "73" ,  SENSOR_73  }, {  "74" , SENSOR_74  },
  { "75",  SENSOR_75  }, { "76" ,  SENSOR_76  }, {  "77" , SENSOR_77  },
  { "78",  SENSOR_78  }, { "79" ,  SENSOR_79  }, {  "80" , SENSOR_80  },
  { "81",  SENSOR_81  }, { "82" ,  SENSOR_82  }, {  "83" , SENSOR_83  },
  { "84",  SENSOR_84  }, { "85" ,  SENSOR_85  }, {  "86" , SENSOR_86  },
  { "87",  SENSOR_87  }, { "88" ,  SENSOR_88  }, {  "89" , SENSOR_89  },
  { "90",  SENSOR_90  }, { "91" ,  SENSOR_91  }, {  "92" , SENSOR_92  },
  { "93",  SENSOR_93  }, { "94" ,  SENSOR_94  }, {  "95" , SENSOR_95  },
  { "96",  SENSOR_96  }, { "97" ,  SENSOR_97  }, {  "98" , SENSOR_98  },
  { "99",  SENSOR_99  }, {"100" ,  SENSOR_100 }, { "101" , SENSOR_101 },
  { "102", SENSOR_102 }, {"103" ,  SENSOR_103 }, { "104" , SENSOR_104 },
  { "105", SENSOR_105 }, {"106" ,  SENSOR_106 }, { "107" , SENSOR_107 },
  { "108", SENSOR_108 }, {"109" ,  SENSOR_109 }, { "110" , SENSOR_110 },
  { "111", SENSOR_111 }, {"112" ,  SENSOR_112 }, { "113" , SENSOR_113 },
  { "114", SENSOR_114 }, {"115" ,  SENSOR_115 }, { "116" , SENSOR_116 },
  { "117", SENSOR_117 }, {"118" ,  SENSOR_118 }, { "119" , SENSOR_119 },
  { "120", SENSOR_120 }, {"121" ,  SENSOR_121 }, { "122" , SENSOR_122 },
  { "123", SENSOR_123 }, {"124" ,  SENSOR_124 }, { "125" , SENSOR_125 },
  { "126", SENSOR_126 }, {"127" ,  SENSOR_127 },
  { "UNUSED", SENSOR_UNUSED }, { NULL, 0}
};

static
USER_ENUM_TBL sensorType[]      =  { { "UNUSED"  , UNUSED          },
                                     { "ARINC429", SAMPLE_ARINC429 },
                                     { "QAR"     , SAMPLE_QAR      },
                                     { "ANALOG"  , SAMPLE_ANALOG   },
                                     { "DISCRETE", SAMPLE_DISCRETE },
                                     { "UART"    , SAMPLE_UART     },
                                     { NULL,       0               }
                                   };
static
USER_ENUM_TBL liveDataType[]    =  { { "NONE"  , LD_NONE           },
                                     { "ASCII" , LD_ASCII          },
                                     { "BINARY", LD_BINARY         },
                                     { NULL,       0               }
                                   };
static
USER_ENUM_TBL conversionType[]  =  { { "NONE",  NONE               },
                                     { "LINEAR", LINEAR            },
                                     { NULL,       0               }
                                   };
static
USER_ENUM_TBL sampleType[]      =  { { "1HZ"    , SSR_1HZ          },
                                     { "2HZ"    , SSR_2HZ          },
                                     { "4HZ"    , SSR_4HZ          },
                                     { "5HZ"    , SSR_5HZ          },
                                     { "10HZ"   , SSR_10HZ         },
                                     { "20HZ"   , SSR_20HZ         },
                                     { "50HZ"   , SSR_50HZ         },
                                     { NULL     , 0                }
                                   };
static
USER_ENUM_TBL filterType[]      =  { { "NONE"       , FILTERNONE     },
                                     { "EXPONENTIAL", EXPO_AVERAGE   },
                                     { "SPIKEREJECT", SPIKEREJECTION },
                                     { "MAXREJECT"  , MAXREJECT      },
                                     { "SLOPE"      , SLOPEFILTER    },
                                     { NULL         , 0              }
                                   };

// Sensor User Configuration Support

#pragma ghs nowarning 1545 //Suppress packed structure alignment warning

// ~~~~~~~~~ Sensor Commands ~~~~~~~~~~~~~~
static USER_MSG_TBL sensorCalConfigCmd [] =
{  /* Str          Next Tbl Ptr    Handler Func.    Data Type          Access         Parameter                     IndexRange       DataLimit  EnumTbl*/
   { "TYPE"  , NO_NEXT_TABLE,    Sensor_UserCfg, USER_TYPE_ENUM,      USER_RW,  &configTemp.calibration.type,       0,MAX_SENSORS-1, NO_LIMIT,  conversionType },
   { "PARAM1", NO_NEXT_TABLE,    Sensor_UserCfg, USER_TYPE_FLOAT,     USER_RW,  &configTemp.calibration.fParams[0], 0,MAX_SENSORS-1, NO_LIMIT,  NULL },
   { "PARAM2", NO_NEXT_TABLE,    Sensor_UserCfg, USER_TYPE_FLOAT,     USER_RW,  &configTemp.calibration.fParams[1], 0,MAX_SENSORS-1, NO_LIMIT,  NULL },
   { NULL    , NO_NEXT_TABLE,    NULL,           NO_HANDLER_DATA }
};


static USER_MSG_TBL sensorConvConfigCmd [] =
{   /* Str     Next Tbl Ptr    Handler Func.    Data Type          Access         Parameter                        IndexRange       DataLimit   EnumTbl*/
   { "TYPE"  , NO_NEXT_TABLE, Sensor_UserCfg, USER_TYPE_ENUM,     USER_RW,     &configTemp.conversion.type,        0,MAX_SENSORS-1, NO_LIMIT,   conversionType },
   { "PARAM1", NO_NEXT_TABLE, Sensor_UserCfg, USER_TYPE_FLOAT,    USER_RW,     &configTemp.conversion.fParams[0],  0,MAX_SENSORS-1, NO_LIMIT,   NULL },
   { "PARAM2", NO_NEXT_TABLE, Sensor_UserCfg, USER_TYPE_FLOAT,    USER_RW,     &configTemp.conversion.fParams[1],  0,MAX_SENSORS-1, NO_LIMIT,   NULL },
   { NULL    , NO_NEXT_TABLE, NULL,           NO_HANDLER_DATA }
};

static USER_MSG_TBL sensorFilterConfigCmd [] =
{  /* Str                   Next Tbl Ptr    Handler Func.    Data Type      Access     Parameter                            IndexRange        DataLimit  EnumTbl*/
   { "TYPE",                NO_NEXT_TABLE, Sensor_UserCfg, USER_TYPE_ENUM,   USER_RW, &configTemp.filterCfg.type,           0, MAX_SENSORS-1, NO_LIMIT,  filterType },
   { "FULLSCALE",           NO_NEXT_TABLE, Sensor_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configTemp.filterCfg.fFullScale,     0, MAX_SENSORS-1, NO_LIMIT,  NULL },
   { "SPIKEREJECTPERCENT",  NO_NEXT_TABLE, Sensor_UserCfg, USER_TYPE_UINT8,  USER_RW, &configTemp.filterCfg.nSpikeRejectPct,0, MAX_SENSORS-1, NO_LIMIT,  NULL },
   { "MAXREJECTCOUNT",      NO_NEXT_TABLE, Sensor_UserCfg, USER_TYPE_UINT8,  USER_RW, &configTemp.filterCfg.nMaxRejectCount,0, MAX_SENSORS-1, NO_LIMIT,  NULL },
   { "MAXPERCENT",          NO_NEXT_TABLE, Sensor_UserCfg, USER_TYPE_UINT16, USER_RW, &configTemp.filterCfg.nMaxPct,        0, MAX_SENSORS-1, NO_LIMIT,  NULL },
   { "REJECTVALUE",         NO_NEXT_TABLE, Sensor_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configTemp.filterCfg.fRejectValue,   0, MAX_SENSORS-1, NO_LIMIT,  NULL },
   { "TIMECONSTANT",        NO_NEXT_TABLE, Sensor_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configTemp.filterCfg.fTimeConstant,  0, MAX_SENSORS-1, NO_LIMIT,  NULL },
   { NULL          ,        NO_NEXT_TABLE, NULL,           NO_HANDLER_DATA }
};

static USER_MSG_TBL sensorCmd [] =
{  /* Str                  Next Tbl Ptr         Handler Func.    Data Type         Access   Parameter                        IndexRange       DataLimit          EnumTbl*/
   { "TYPE"        ,      NO_NEXT_TABLE,        Sensor_UserCfg, USER_TYPE_ENUM,   USER_RW, &configTemp.type,              0,MAX_SENSORS-1,  NO_LIMIT,          sensorType },
   { "NAME"        ,      NO_NEXT_TABLE,        Sensor_UserCfg, USER_TYPE_STR,    USER_RW, configTemp.sSensorName,        0,MAX_SENSORS-1,  0,MAX_SENSORNAME,  NULL },
   { "OUTPUT"      ,      NO_NEXT_TABLE,        Sensor_UserCfg, USER_TYPE_STR,    USER_RW, configTemp.sOutputUnits,       0,MAX_SENSORS-1,  0,MAX_SENSORUNITS, NULL },
   { "INPUT"       ,      NO_NEXT_TABLE,        Sensor_UserCfg, USER_TYPE_UINT16, USER_RW, &configTemp.nInputChannel,     0,MAX_SENSORS-1,  NO_LIMIT,          NULL },
   { "MAXSAMPLES"  ,      NO_NEXT_TABLE,        Sensor_UserCfg, USER_TYPE_UINT8,  USER_RW, &configTemp.nMaximumSamples,   0,MAX_SENSORS-1,  1,MAX_SAMPLES,     NULL },
   { "RATE"        ,      NO_NEXT_TABLE,        Sensor_UserCfg, USER_TYPE_ENUM,   USER_RW, &configTemp.sampleRate,        0,MAX_SENSORS-1,  NO_LIMIT,          sampleType },
   { "RATEOFFSET_MS",     NO_NEXT_TABLE,        Sensor_UserCfg, USER_TYPE_UINT16, USER_RW, &configTemp.nSampleOffset_ms,  0,MAX_SENSORS-1,  0,1000,            NULL },
   { "CALIBRATION" ,      sensorCalConfigCmd ,  NULL,           NO_HANDLER_DATA },
   { "CONVERSION"  ,      sensorConvConfigCmd,  NULL,           NO_HANDLER_DATA },
   { "FILTER"      ,      sensorFilterConfigCmd,NULL,           NO_HANDLER_DATA },
   { "SYSCOND"          , NO_NEXT_TABLE,        Sensor_UserCfg, USER_TYPE_ENUM,    USER_RW,&configTemp.systemCondition,   0,MAX_SENSORS-1,    NO_LIMIT,        Flt_UserEnumStatus  },
   { "SELFHEAL"         , NO_NEXT_TABLE,        Sensor_UserCfg, USER_TYPE_BOOLEAN, USER_RW,&configTemp.bSelfHeal,         0,MAX_SENSORS-1,    NO_LIMIT,        NULL },
   { "RANGETEST"        , NO_NEXT_TABLE,        Sensor_UserCfg, USER_TYPE_BOOLEAN, USER_RW,&configTemp.bRangeTest,        0,MAX_SENSORS-1,    NO_LIMIT,        NULL  },
   { "RANGEDURATION_MS" , NO_NEXT_TABLE,        Sensor_UserCfg, USER_TYPE_UINT16,  USER_RW,&configTemp.nRangeDuration_ms, 0,MAX_SENSORS-1,    NO_LIMIT,        NULL  },
   { "MIN"              , NO_NEXT_TABLE,        Sensor_UserCfg, USER_TYPE_FLOAT,   USER_RW,&configTemp.fMinValue,         0,MAX_SENSORS-1,    NO_LIMIT,        NULL  },
   { "MAX"              , NO_NEXT_TABLE,        Sensor_UserCfg, USER_TYPE_FLOAT,   USER_RW,&configTemp.fMaxValue,         0,MAX_SENSORS-1,    NO_LIMIT,        NULL  },
   { "RATETEST"         , NO_NEXT_TABLE,        Sensor_UserCfg, USER_TYPE_BOOLEAN, USER_RW,&configTemp.bRateTest,         0,MAX_SENSORS-1,    NO_LIMIT,        NULL  },
   { "RATETHRESHOLD"    , NO_NEXT_TABLE,        Sensor_UserCfg, USER_TYPE_FLOAT,   USER_RW,&configTemp.fRateThreshold,    0,MAX_SENSORS-1,    NO_LIMIT,        NULL  },
   { "RATEDURATION_MS"  , NO_NEXT_TABLE,        Sensor_UserCfg, USER_TYPE_UINT16,  USER_RW,&configTemp.nRateDuration_ms,  0,MAX_SENSORS-1,    NO_LIMIT,        NULL  },
   { "SIGNALTEST"       , NO_NEXT_TABLE,        Sensor_UserCfg, USER_TYPE_BOOLEAN, USER_RW,&configTemp.bSignalTest,       0,MAX_SENSORS-1,    NO_LIMIT,        NULL  },
   { "SIGNALTESTINDEX"  , NO_NEXT_TABLE,        Sensor_UserCfg, USER_TYPE_ENUM,    USER_RW,&configTemp.signalTestIndex,   0,MAX_SENSORS-1,    NO_LIMIT,        FaultIndexType  },
   { "SIGNALDURATION_MS", NO_NEXT_TABLE,        Sensor_UserCfg, USER_TYPE_UINT16,  USER_RW,&configTemp.nSignalDuration_ms,0,MAX_SENSORS-1,    NO_LIMIT,        NULL  },
   { "INSPECT"          , NO_NEXT_TABLE,        Sensor_UserCfg, USER_TYPE_BOOLEAN, USER_RW,&configTemp.bInspectInclude,   0,MAX_SENSORS-1,    NO_LIMIT,        NULL  },
   { "GPA"              , NO_NEXT_TABLE,        Sensor_UserCfg, USER_TYPE_HEX32,   USER_RW,&configTemp.generalPurposeA,   0,MAX_SENSORS-1,    NO_LIMIT,        NULL  },
   { "GPB"              , NO_NEXT_TABLE,        Sensor_UserCfg, USER_TYPE_HEX32,   USER_RW,&configTemp.generalPurposeB,   0,MAX_SENSORS-1,    NO_LIMIT,        NULL  },
   { NULL               , NO_NEXT_TABLE,        NULL,           NO_HANDLER_DATA }
};

static USER_MSG_TBL filterState [] =
{  /* Str               Next Tbl Ptr  Handler Func.  Data Type         Access              Parameter                           IndexRange         DataLimit   EnumTbl*/
   { "LASTVALIDVALUE", NO_NEXT_TABLE, Sensor_State, USER_TYPE_FLOAT,   USER_RO,     &sensorTemp.filterData.fLastValidValue,   0, MAX_SENSORS - 1, NO_LIMIT,   NULL    },
   { "LASTAVGVALUE"  , NO_NEXT_TABLE, Sensor_State, USER_TYPE_FLOAT,   USER_RO,     &sensorTemp.filterData.fLastAvgValue  ,   0, MAX_SENSORS - 1, NO_LIMIT,   NULL    },
   { "REJECTCOUNT"   , NO_NEXT_TABLE, Sensor_State, USER_TYPE_UINT8,   USER_RO,     &sensorTemp.filterData.nRejectCount   ,   0, MAX_SENSORS - 1, NO_LIMIT,   NULL    },
   { "K"             , NO_NEXT_TABLE, Sensor_State, USER_TYPE_FLOAT,   USER_RO,     &sensorTemp.filterData.fK              ,  0, MAX_SENSORS - 1, NO_LIMIT,   NULL    },
   { "(n-1)/2"       , NO_NEXT_TABLE, Sensor_State, USER_TYPE_FLOAT,   USER_RO,     &sensorTemp.filterData.nMinusOneOverTwo,  0, MAX_SENSORS - 1, NO_LIMIT,   NULL    },
   { "FULLSCALEINV"  , NO_NEXT_TABLE, Sensor_State, USER_TYPE_FLOAT,   USER_RO,     &sensorTemp.filterData.fullScaleInv,      0, MAX_SENSORS - 1, NO_LIMIT,   NULL    },
   { "MAXPERCENT"    , NO_NEXT_TABLE, Sensor_State, USER_TYPE_FLOAT,   USER_RO,     &sensorTemp.filterData.maxPercent,        0, MAX_SENSORS - 1, NO_LIMIT,   NULL    },
   { "SPIKEPERCENT"  , NO_NEXT_TABLE, Sensor_State, USER_TYPE_FLOAT,   USER_RO,     &sensorTemp.filterData.spikePercent,      0, MAX_SENSORS - 1, NO_LIMIT,   NULL    },
   { "PERSISTSTATE"  , NO_NEXT_TABLE, Sensor_State, USER_TYPE_BOOLEAN, USER_RO,     &sensorTemp.filterData.persistState,      0, MAX_SENSORS - 1, NO_LIMIT,   NULL    },
   { "PERSISTSTATEZ1", NO_NEXT_TABLE, Sensor_State, USER_TYPE_BOOLEAN, USER_RO,     &sensorTemp.filterData.persistStateZ1,    0, MAX_SENSORS - 1, NO_LIMIT,   NULL    },
   { NULL            , NO_NEXT_TABLE, NULL,         NO_HANDLER_DATA }
};

static USER_MSG_TBL sensorStatus [] =
{  /* Str                  Next Tbl Ptr  Handler Func.  Data Type          Access           Parameter               IndexRange        DataLimit  EnumTbl*/
   { "INDEX"            , NO_NEXT_TABLE, Sensor_State,  USER_TYPE_ENUM,    USER_RO, &sensorTemp.nSensorIndex ,    0, MAX_SENSORS - 1, NO_LIMIT,  SensorIndexType },
   { "VALID"            , NO_NEXT_TABLE, Sensor_State,  USER_TYPE_BOOLEAN, USER_RO, &sensorTemp.bValueIsValid  ,  0, MAX_SENSORS - 1, NO_LIMIT,  NULL },
   { "FVALUE"           , NO_NEXT_TABLE, Sensor_State, USER_TYPE_FLOAT,    USER_RO, &sensorTemp.fValue         ,  0, MAX_SENSORS - 1, NO_LIMIT,  NULL },
   { "PRIORVALUE"       , NO_NEXT_TABLE, Sensor_State, USER_TYPE_FLOAT,    USER_RO, &sensorTemp.fPriorValue    ,  0, MAX_SENSORS - 1, NO_LIMIT,  NULL },
   { "SAMPLECOUNTDOWN"  , NO_NEXT_TABLE, Sensor_State, USER_TYPE_UINT16,   USER_RO, &sensorTemp.nSampleCountdown, 0, MAX_SENSORS - 1, NO_LIMIT,  NULL },
   { "NEXTSAMPLE"       , NO_NEXT_TABLE, Sensor_State, USER_TYPE_UINT8,    USER_RO, &sensorTemp.nNextSample     , 0, MAX_SENSORS - 1, NO_LIMIT,  NULL },
   { "CURRENTSAMPLES"   , NO_NEXT_TABLE, Sensor_State, USER_TYPE_UINT8,    USER_RO, &sensorTemp.nCurrentSamples , 0, MAX_SENSORS - 1, NO_LIMIT,  NULL },
   { "BITFAIL"          , NO_NEXT_TABLE, Sensor_State, USER_TYPE_BOOLEAN,  USER_RO, &sensorTemp.bBITFail,         0, MAX_SENSORS - 1, NO_LIMIT,  NULL },
   { "RANGEFAIL"        , NO_NEXT_TABLE, Sensor_State, USER_TYPE_BOOLEAN,  USER_RO, &sensorTemp.bRangeFail,       0, MAX_SENSORS - 1, NO_LIMIT,  NULL },
   { "RATEFAIL"         , NO_NEXT_TABLE, Sensor_State, USER_TYPE_BOOLEAN,  USER_RO, &sensorTemp.bRateFail,        0, MAX_SENSORS - 1, NO_LIMIT,  NULL },
   { "RATEVALUE"        , NO_NEXT_TABLE, Sensor_State, USER_TYPE_FLOAT,    USER_RO, &sensorTemp.fRateValue,       0, MAX_SENSORS - 1, NO_LIMIT,  NULL },
   { "SIGNALFAIL"       , NO_NEXT_TABLE, Sensor_State, USER_TYPE_BOOLEAN,  USER_RO, &sensorTemp.bSignalFail,      0, MAX_SENSORS - 1, NO_LIMIT,  NULL },
   { "FILTERDATA"       , filterState,   NULL,         NO_HANDLER_DATA },
   { "INTERFACEINDEX"   , NO_NEXT_TABLE, Sensor_State, USER_TYPE_UINT16,   USER_RO, &sensorTemp.nInterfaceIndex,  0, MAX_SENSORS - 1, NO_LIMIT,  NULL },
   { "LASTTICK"         , NO_NEXT_TABLE, Sensor_State, USER_TYPE_UINT32,   USER_RO, &sensorTemp.lastUpdateTick,   0, MAX_SENSORS - 1, NO_LIMIT,  NULL },
   { NULL               , NO_NEXT_TABLE, NULL,         NO_HANDLER_DATA }
};


static USER_MSG_TBL sensorRoot [] =
{/* Str          Next Tbl Ptr    Handler Func.            Data Type          Access                         Parameter     IndexRange   DataLimit   EnumTbl*/
 { "CFG",        sensorCmd      ,NULL, NO_HANDLER_DATA},
 { "STATUS",     sensorStatus   ,NULL, NO_HANDLER_DATA},
 { DISPLAY_CFG,  NO_NEXT_TABLE  ,Sensor_ShowConfig       ,USER_TYPE_ACTION  ,(USER_RO|USER_NO_LOG|USER_GSE) ,NULL         ,-1,-1      ,NO_LIMIT   ,NULL},
 { NULL,         NULL           ,NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL liveDataCfg [] =
{ /*Str         Next Tbl Ptr    Handler Func.         Data Type         Access               Parameter               IndexRange  DataLimit                 EnumTbl*/
  { "TYPE",     NO_NEXT_TABLE,  Sensor_Live_Data_Cfg, USER_TYPE_ENUM,   (USER_RW|USER_GSE),  &liveDataTemp.gseType,  -1,-1,      NO_LIMIT,                 liveDataType },
  { "RATE_MS",  NO_NEXT_TABLE,  Sensor_Live_Data_Cfg, USER_TYPE_UINT32, (USER_RW|USER_GSE),  &liveDataTemp.nRate_ms, -1,-1,      SENSOR_LD_MAX_RATE,10000, NULL },
  { NULL,       NULL,           NULL,                 NO_HANDLER_DATA}
};

static USER_MSG_TBL liveDataRoot [] =
{ /*Str         Next Tbl Ptr    Handler Func.         Data Type         Access               Parameter               IndexRange     DataLimit                 EnumTbl*/
  { "CFG",      liveDataCfg,    NULL,                 NO_HANDLER_DATA},
  { "TYPE",     NO_NEXT_TABLE,  User_GenericAccessor, USER_TYPE_ENUM,   (USER_RW|USER_GSE),  &m_liveDataDisplay.gseType,  -1,-1,    NO_LIMIT,                 liveDataType },
  { "MS_TYPE",  NO_NEXT_TABLE,  User_GenericAccessor, USER_TYPE_ENUM,   (USER_RW|USER_GSE),  &m_liveDataDisplay.msType,   -1,-1,    NO_LIMIT,                 liveDataType },
  { "RATE_MS",  NO_NEXT_TABLE,  User_GenericAccessor, USER_TYPE_UINT32, (USER_RW|USER_GSE),  &m_liveDataDisplay.nRate_ms, -1,-1,    SENSOR_LD_MAX_RATE,10000, NULL },
  { "GETLIST",  NO_NEXT_TABLE,  Sensor_LiveDataList,  USER_TYPE_ACTION, (USER_RO|USER_NO_LOG|USER_GSE), NULL,             -1,-1,    NO_LIMIT,                 NULL},
  { NULL,       NULL,           NULL,                 NO_HANDLER_DATA}
};

static USER_MSG_TBL RootSensorMsg = {"SENSOR",    sensorRoot,   NULL, NO_HANDLER_DATA};
static USER_MSG_TBL LiveDataMsg   = {"LIVEDATA",  liveDataRoot, NULL, NO_HANDLER_DATA};
#pragma ghs endnowarning


/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
* Function:     Sensor_UserCfg
*
* Description:  Handles User Manager requests to change sensor configuraion
*               items.
*
* Parameters:   [in] DataType:  C type of the data to be read or changed, used
*                               for casting the data pointers
*               [in/out] Param: Pointer to the configuration item to be read
*                               or changed
*               [in] Index:     Index parameter is used to reference the
*                               specific sensor to change.  Range is validated
*                               by the user manager
*               [in] SetPtr:    For write commands, a pointer to the data to
*                               write to the configuration.
*               [out] GetPtr:   For read commands, UserCfg function will set
*                               this to the location of the data requested.
*
* Returns:     USER_RESULT_OK:    Processed successfully
*              USER_RESULT_ERROR: Error processing command.
*
* Notes:
*****************************************************************************/
static
USER_HANDLER_RESULT Sensor_UserCfg(USER_DATA_TYPE DataType,
                                   USER_MSG_PARAM Param,
                                   UINT32 Index,
                                   const void *SetPtr,
                                   void **GetPtr)
{
   USER_HANDLER_RESULT result;

   result = USER_RESULT_OK;
   //Get sensor structure based on index
   memcpy(&configTemp,
      &CfgMgr_ConfigPtr()->SensorConfigs[Index],
      sizeof(configTemp));

   result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
   if(SetPtr != NULL && USER_RESULT_OK == result)
   {
     memcpy(&CfgMgr_ConfigPtr()->SensorConfigs[Index], &configTemp,sizeof(SENSOR_CONFIG));
     CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
                            &CfgMgr_ConfigPtr()->SensorConfigs[Index],
                            sizeof(configTemp));
   }
   return result;
}

/******************************************************************************
* Function:     Sensor_State
*
* Description:  Handles User Manager requests to retrieve the current sensor
*               status.
*
* Parameters:   [in] DataType:  C type of the data to be read or changed, used
*                               for casting the data pointers
*               [in/out] Param: Pointer to the configuration item to be read
*                               or changed
*               [in] Index:     Index parameter is used to reference the
*                               specific sensor to change.  Range is validated
*                               by the user manager
*               [in] SetPtr:    For write commands, a pointer to the data to
*                               write to the configuration.
*               [out] GetPtr:   For read commands, UserCfg function will set
*                               this to the location of the data requested.
*
* Returns:     USER_RESULT_OK:    Processed successfully
*              USER_RESULT_ERROR: Error processing command.
*
* Notes:
*****************************************************************************/
static
USER_HANDLER_RESULT Sensor_State(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr)
{
   USER_HANDLER_RESULT result;

   result = USER_RESULT_ERROR;

   memcpy(&sensorTemp, &Sensors[Index], sizeof(sensorTemp));

   result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
   return result;

}

/******************************************************************************
* Function:     Sensor_Live_Data_Cfg
*
* Description:  Handles User Manager requests to change sensor configuraion
*               items.
*
* Parameters:   [in] DataType:  C type of the data to be read or changed, used
*                               for casting the data pointers
*               [in/out] Param: Pointer to the configuration item to be read
*                               or changed
*               [in] Index:     Index parameter is used to reference the
*                               specific sensor to change.  Range is validated
*                               by the user manager
*               [in] SetPtr:    For write commands, a pointer to the data to
*                               write to the configuration.
*               [out] GetPtr:   For read commands, UserCfg function will set
*                               this to the location of the data requested.
*
* Returns:     USER_RESULT_OK:    Processed successfully
*              USER_RESULT_ERROR: Error processing command.
*
* Notes:
*****************************************************************************/
static
USER_HANDLER_RESULT Sensor_Live_Data_Cfg(USER_DATA_TYPE DataType,
                                     USER_MSG_PARAM Param,
                                     UINT32 Index,
                                     const void *SetPtr,
                                     void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_OK;

  //Get sensor structure based on index
  memcpy(&liveDataTemp, &CfgMgr_ConfigPtr()->LiveDataConfig, sizeof(liveDataTemp));

  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
  if ((SetPtr != NULL) && (USER_RESULT_OK == result))
  {
    // set non-vol config
    memcpy(&CfgMgr_ConfigPtr()->LiveDataConfig, &liveDataTemp, sizeof(liveDataTemp));
    CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
      &CfgMgr_ConfigPtr()->LiveDataConfig, sizeof(liveDataTemp));

    #pragma ghs nowarning 1545 //Suppress packed structure alignment warning
    // set proper member of local working copy
    if (Param.Ptr == &liveDataTemp.gseType)
    {
      // copy to local display control
      m_liveDataDisplay.gseType = liveDataTemp.gseType;
    }
    else if (Param.Ptr == &liveDataTemp.msType)
    {
      // copy to local display control ms_type
      m_liveDataDisplay.msType = liveDataTemp.msType;
    }
    else if (Param.Ptr == &liveDataTemp.nRate_ms)
    {
      // copy to local display control
      m_liveDataDisplay.nRate_ms = liveDataTemp.nRate_ms;
    }
    #pragma ghs endnowarning
  }
  return result;
}


/******************************************************************************
* Function:     Sensor_ShowConfig
*
* Description:  Handles User Manager requests to retrieve the configuration
*               settings.
*
* Parameters:   [in] DataType:  C type of the data to be read or changed, used
*                               for casting the data pointers
*               [in/out] Param: Pointer to the configuration item to be read
*                               or changed
*               [in] Index:     Index parameter is used to reference the
*                               specific sensor to change.  Range is validated
*                               by the user manager
*               [in] SetPtr:    For write commands, a pointer to the data to
*                               write to the configuration.
*               [out] GetPtr:   For read commands, UserCfg function will set
*                               this to the location of the data requested.
*
* Returns:     USER_RESULT_OK:    Processed successfully
*              USER_RESULT_ERROR: Error processing command.
*
* Notes:
*****************************************************************************/
static
USER_HANDLER_RESULT Sensor_ShowConfig(USER_DATA_TYPE DataType,
                                      USER_MSG_PARAM Param,
                                      UINT32 Index,
                                      const void *SetPtr,
                                      void **GetPtr)
{
   USER_HANDLER_RESULT result;
   CHAR label[USER_MAX_MSG_STR_LEN * 3];

   //Top-level name is a single indented space
   CHAR branchName[USER_MAX_MSG_STR_LEN] = " ";

   USER_MSG_TBL*  pCfgTable;
   INT16 sensorIdx;

   result = USER_RESULT_OK;

   // Loop for each sensor.
   for (sensorIdx = 0; sensorIdx < MAX_SENSORS && result == USER_RESULT_OK; ++sensorIdx)
   {
      // Display sensor id info above each set of data.
      snprintf(label, sizeof(label), "\r\n\r\nSENSOR[%d].CFG", sensorIdx);
      result = USER_RESULT_ERROR;
      if (User_OutputMsgString( label, FALSE ) )
      {
         pCfgTable = sensorCmd;  // Re-set the pointer to beginning of CFG table

         // User_DisplayConfigTree will invoke itself recursively to display all fields.
         result = User_DisplayConfigTree(branchName, pCfgTable, sensorIdx, 0, NULL);
      }
   }
   return result;
}

/******************************************************************************
* Function:     Sensor_LiveDataList
*
* Description:  Handles User Manager requests to display the list of sensors
*               configured for LiveData (i.e. sensor[x].cfg.inspect=TRUE)
*
* Parameters:   [in] DataType:  C type of the data to be read or changed, used
*                               for casting the data pointers
*               [in/out] Param: Pointer to the configuration item to be read
*                               or changed
*               [in] Index:     Index parameter is used to reference the
*                               specific sensor to change.  Range is validated
*                               by the user manager
*               [in] SetPtr:    For write commands, a pointer to the data to
*                               write to the configuration.
*               [out] GetPtr:   For read commands, UserCfg function will set
*                               this to the location of the data requested.
*
* Returns:     USER_RESULT_OK:    Processed successfully
*              USER_RESULT_ERROR: Error processing command.
*
* Notes:
*****************************************************************************/
static
USER_HANDLER_RESULT Sensor_LiveDataList(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr)
{

  CHAR buff[GETLIST_BUFF_SIZE + 1];
  SENSOR_CONFIG*     pSnsrCfg;
  SENSOR_INDEX       sensorIdx;
  BOOLEAN result   = TRUE;
  BOOLEAN bInspect = FALSE;

  pSnsrCfg =  (SENSOR_CONFIG*) &CfgMgr_ConfigPtr()->SensorConfigs;

  // Create an output suitable for importing into a CSV file.
  // First line is a header, subsequent lines contain the sensor-id, sensor-name and units,
  // for each sensor configured for livedata; if no sensors are configured, a none configured
  // msg is included.

  // Write out an entry for each defined sensor which is configured for LiveData
  for (sensorIdx = SENSOR_0; sensorIdx < MAX_SENSORS && result; ++sensorIdx)
  {
    if( SensorIsUsed(sensorIdx) && pSnsrCfg[sensorIdx].bInspectInclude )
    {
      if (FALSE == bInspect)
      {
        // Found the first sensor configured for LD, print a header line.
        snprintf(buff, sizeof(buff), "Sensor-ID,Sensor-Name,Sensor-Units\r\n");
        result = User_OutputMsgString( buff, FALSE);
        bInspect = TRUE;
      }
      
      snprintf(buff, sizeof(buff),"%03d,%s,%s\r\n",
              sensorIdx,
              pSnsrCfg[sensorIdx].sSensorName,
              pSnsrCfg[sensorIdx].sOutputUnits);
      result = User_OutputMsgString(buff, FALSE);
    }
  }

  // If no sensors were configured for LiveData, let the caller
  // definitely know it.
  if (!bInspect && result)
  {
    snprintf(buff, sizeof(buff), "No sensors configured for LiveData\r\n");
    result = User_OutputMsgString( buff, FALSE);
  }

  // finalize the output
  buff[0] = '\0';
  result = User_OutputMsgString(buff, TRUE);

  return (result) ? USER_RESULT_OK : USER_RESULT_ERROR;
}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/*****************************************************************************
*  MODIFICATIONS
*    $History: SensorUserTables.c $
 * 
 * *****************  Version 29  *****************
 * User: Contractor V&v Date: 10/20/14   Time: 3:55p
 * Updated in $/software/control processor/code/system
 * SCR #1262 - LiveData CP to MS - none configured msg change.
 * 
 * *****************  Version 28  *****************
 * User: Peter Lee    Date: 14-10-08   Time: 7:10p
 * Updated in $/software/control processor/code/system
 * SCR #1263 ID Param Protocol Implementation
 *
 * *****************  Version 27  *****************
 * User: Contractor V&v Date: 9/22/14    Time: 6:46p
 * Updated in $/software/control processor/code/system
 * SCR #1262 - LiveData CP to MS
 *
 * *****************  Version 26  *****************
 * User: John Omalley Date: 12-11-16   Time: 10:32p
 * Updated in $/software/control processor/code/system
 * SCR 1197 - Code Review Updates
 *
 * *****************  Version 25  *****************
 * User: John Omalley Date: 12-11-12   Time: 11:36a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review Updates
 *
 * *****************  Version 24  *****************
 * User: Peter Lee    Date: 12-10-27   Time: 5:04p
 * Updated in $/software/control processor/code/system
 * SCR #1191 Returns update time of param
 *
 * *****************  Version 23  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 22  *****************
 * User: John Omalley Date: 12-08-13   Time: 4:21p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Remove space from sensor index enum
 *
 * *****************  Version 21  *****************
 * User: Contractor V&v Date: 7/18/12    Time: 6:27p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 Add nowarn pragma
 *
 * *****************  Version 20  *****************
 * User: John Omalley Date: 12-07-13   Time: 9:03a
 * Updated in $/software/control processor/code/system
 * SCR 1124 - Packed the configuration structures for ARINC429 Mgr,
 * Sensors and Triggers. Suppressed the alignment warnings for those three
 * objects also.
 *
 * *****************  Version 19  *****************
 * User: Contractor V&v Date: 4/11/12    Time: 5:05p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST2   128sensors
 *
 * *****************  Version 18  *****************
 * User: Contractor2  Date: 10/04/10   Time: 1:27p
 * Updated in $/software/control processor/code/system
 * SCR #916 Code Review Updates
 *
 * *****************  Version 17  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:01a
 * Updated in $/software/control processor/code/system
 * SCR #685 - Add USER_GSE flag
 *
 * *****************  Version 16  *****************
 * User: Jeff Vahue   Date: 6/09/10    Time: 3:04p
 * Updated in $/software/control processor/code/system
 * SCR# 486 - mod the fix to use the 'active' version of the livedata to
 * control the task execution.  Also modify tables to use the live data
 * structure directly.
 *
 * *****************  Version 15  *****************
 * User: Contractor V&v Date: 6/08/10    Time: 5:55p
 * Updated in $/software/control processor/code/system
 * SCR #615 Showcfg/Long msg enhancement
 *
 * *****************  Version 14  *****************
 * User: Contractor2  Date: 6/07/10    Time: 1:29p
 * Updated in $/software/control processor/code/system
 * SCR #486 Livedata enhancement
 *
 * *****************  Version 13  *****************
 * User: Peter Lee    Date: 5/31/10    Time: 6:28p
 * Updated in $/software/control processor/code/system
 * SCR #618 F7X and UartMgr Requirements Implementation
 *
 * *****************  Version 12  *****************
 * User: Contractor V&v Date: 3/19/10    Time: 4:32p
 * Updated in $/software/control processor/code/system
 * SCR #248 Parameter Log Change. Add no log for showcfg
 *
 * *****************  Version 11  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 4:04p
 * Updated in $/software/control processor/code/system
 * SCR 106 Remove USE_GENERIC_ACCESSOR macro
 *
 * *****************  Version 10  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 9  *****************
 * User: Jeff Vahue   Date: 2/17/10    Time: 1:24p
 * Updated in $/software/control processor/code/system
 * SCR# 452 - code coverage mods
 *
 * *****************  Version 8  *****************
 * User: Jeff Vahue   Date: 2/09/10    Time: 11:20a
 * Updated in $/software/control processor/code/system
 * SCR# 404 - remove unused sensor status cmds
 *
 * *****************  Version 7  *****************
 * User: Jeff Vahue   Date: 1/23/10    Time: 3:14p
 * Updated in $/software/control processor/code/system
 * Typo in GSE command SPKIKEPERCENT -> SPIKEPERCENT
 *
 * *****************  Version 6  *****************
 * User: Jeff Vahue   Date: 1/19/10    Time: 3:52p
 * Updated in $/software/control processor/code/system
 * SCR# 399
 *
 * *****************  Version 5  *****************
 * User: Contractor V&v Date: 1/13/10    Time: 4:58p
 * Updated in $/software/control processor/code/system
 *
 * *****************  Version 4  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:27p
 * Updated in $/software/control processor/code/system
 * SCR 18, SCR 333
 *
 * *****************  Version 3  *****************
 * User: Contractor V&v Date: 12/10/09   Time: 5:46p
 * Updated in $/software/control processor/code/system
 * SCR 42
 *
 * *****************  Version 2  *****************
 * User: Contractor V&v Date: 11/30/09   Time: 5:41p
 * Updated in $/software/control processor/code/system
 * SCR 106  XXXUserTable.c consistency

*
*****************************************************************************/
