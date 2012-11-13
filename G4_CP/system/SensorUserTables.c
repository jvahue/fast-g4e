#define SENSOR_USERTABLES_BODY
/******************************************************************************
Copyright (C) 2008-2012 Pratt & Whitney Engine Services, Inc.
All Rights Reserved. Proprietary and Confidential.

File:        SensorUserTables.c

Description: User Interface for Sensor Runtime Processing

VERSION
$Revision: 25 $  $Date: 12-11-12 11:36a $

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
USER_HANDLER_RESULT Sensor_UserCfg(USER_DATA_TYPE DataType,
                                   USER_MSG_PARAM Param,
                                   UINT32 Index,
                                   const void *SetPtr,
                                   void **GetPtr);
USER_HANDLER_RESULT Sensor_State(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr);
USER_HANDLER_RESULT Sensor_Live_Data_Cfg(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr);
USER_HANDLER_RESULT Sensor_Live_Data(USER_DATA_TYPE DataType,
                                     USER_MSG_PARAM Param,
                                     UINT32 Index,
                                     const void *SetPtr,
                                     void **GetPtr);
USER_HANDLER_RESULT Sensor_ShowConfig(USER_DATA_TYPE DataType,
                                      USER_MSG_PARAM Param,
                                      UINT32 Index,
                                      const void *SetPtr,
                                      void **GetPtr);


/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
//static SENSOR             Sensors[MAX_SENSORS]; // Dynamic Sensor Storage
static SENSOR_CONFIG      ConfigTemp;      // Temporary Sensor config block
static SENSOR             SensorTemp;      // Temporary Sensor Data block
static SENSOR_LD_CONFIG   LiveDataTemp;    // Temporary Live Data Cfg Block

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

USER_ENUM_TBL SensorType[]      =  { { "UNUSED"  , UNUSED          },
                                     { "ARINC429", SAMPLE_ARINC429 },
                                     { "QAR"     , SAMPLE_QAR      },
                                     { "ANALOG"  , SAMPLE_ANALOG   },
                                     { "DISCRETE", SAMPLE_DISCRETE },
                                     { "UART"    , SAMPLE_UART     },
                                     { NULL,       0               }
                                   };
USER_ENUM_TBL LiveDataType[]    =  { { "NONE"  , LD_NONE           },
                                     { "ASCII" , LD_ASCII          },
                                     { NULL,       0               }
                                   };
USER_ENUM_TBL ConversionType[]  =  { { "NONE",  NONE               },
                                     { "LINEAR", LINEAR            },
                                     { NULL,       0               }
                                   };
USER_ENUM_TBL SampleType[]      =  { { "1HZ"    , SSR_1HZ          },
                                     { "2HZ"    , SSR_2HZ          },
                                     { "4HZ"    , SSR_4HZ          },
                                     { "5HZ"    , SSR_5HZ          },
                                     { "10HZ"   , SSR_10HZ         },
                                     { "20HZ"   , SSR_20HZ         },
                                     { "50HZ"   , SSR_50HZ         },
                                     { NULL     , 0                }
                                   };
USER_ENUM_TBL FilterType[]      =  { { "NONE"       , FILTERNONE     },
                                     { "EXPONENTIAL", EXPO_AVERAGE   },
                                     { "SPIKEREJECT", SPIKEREJECTION },
                                     { "MAXREJECT"  , MAXREJECT      },
                                     { "SLOPE"      , SLOPEFILTER    },
                                     { NULL         , 0              }
                                   };

// Sensor User Configuration Support

#pragma ghs nowarning 1545 //Suppress packed structure alignment warning

// ~~~~~~~~~ Sensor Commands ~~~~~~~~~~~~~~
static USER_MSG_TBL SensorCalConfigCmd [] =
{
   { "TYPE"  , NO_NEXT_TABLE,Sensor_UserCfg, USER_TYPE_ENUM,USER_RW,
     &ConfigTemp.calibration.type,0,MAX_SENSORS-1,NO_LIMIT,ConversionType },
   { "PARAM1", NO_NEXT_TABLE,Sensor_UserCfg, USER_TYPE_FLOAT,USER_RW,
     &ConfigTemp.calibration.fParams[0],0,MAX_SENSORS-1,NO_LIMIT,NULL },
   { "PARAM2", NO_NEXT_TABLE,Sensor_UserCfg, USER_TYPE_FLOAT,USER_RW,
     &ConfigTemp.calibration.fParams[1],0,MAX_SENSORS-1,NO_LIMIT,NULL },
   { NULL    , NO_NEXT_TABLE, NULL, NO_HANDLER_DATA }
};


static USER_MSG_TBL SensorConvConfigCmd [] =
{
   { "TYPE"  , NO_NEXT_TABLE,Sensor_UserCfg, USER_TYPE_ENUM,USER_RW,
     &ConfigTemp.conversion.type,0,MAX_SENSORS-1,NO_LIMIT,ConversionType },
   { "PARAM1", NO_NEXT_TABLE,Sensor_UserCfg, USER_TYPE_FLOAT,USER_RW,
     &ConfigTemp.conversion.fParams[0],0,MAX_SENSORS-1,NO_LIMIT,NULL },
   { "PARAM2", NO_NEXT_TABLE,Sensor_UserCfg, USER_TYPE_FLOAT,USER_RW,
     &ConfigTemp.conversion.fParams[1],0,MAX_SENSORS-1,NO_LIMIT,NULL },
   { NULL    , NO_NEXT_TABLE, NULL, NO_HANDLER_DATA }
};

static USER_MSG_TBL SensorFilterConfigCmd [] =
{
   { "TYPE", NO_NEXT_TABLE, Sensor_UserCfg, USER_TYPE_ENUM, USER_RW,
     &ConfigTemp.filterCfg.type, 0, MAX_SENSORS-1, NO_LIMIT, FilterType },
   { "FULLSCALE", NO_NEXT_TABLE, Sensor_UserCfg, USER_TYPE_FLOAT, USER_RW,
     &ConfigTemp.filterCfg.fFullScale, 0, MAX_SENSORS-1, NO_LIMIT, NULL },
   { "SPIKEREJECTPERCENT", NO_NEXT_TABLE, Sensor_UserCfg, USER_TYPE_UINT8, USER_RW,
     &ConfigTemp.filterCfg.nSpikeRejectPct, 0, MAX_SENSORS-1, NO_LIMIT, NULL },
   { "MAXREJECTCOUNT", NO_NEXT_TABLE, Sensor_UserCfg, USER_TYPE_UINT8, USER_RW,
     &ConfigTemp.filterCfg.nMaxRejectCount, 0, MAX_SENSORS-1, NO_LIMIT, NULL },
   { "MAXPERCENT", NO_NEXT_TABLE, Sensor_UserCfg, USER_TYPE_UINT16, USER_RW,
     &ConfigTemp.filterCfg.nMaxPct, 0, MAX_SENSORS-1, NO_LIMIT, NULL },
   { "REJECTVALUE", NO_NEXT_TABLE, Sensor_UserCfg, USER_TYPE_FLOAT, USER_RW,
     &ConfigTemp.filterCfg.fRejectValue, 0, MAX_SENSORS-1, NO_LIMIT, NULL },
   { "TIMECONSTANT", NO_NEXT_TABLE, Sensor_UserCfg, USER_TYPE_FLOAT, USER_RW,
     &ConfigTemp.filterCfg.fTimeConstant, 0, MAX_SENSORS-1, NO_LIMIT, NULL },
   { NULL    , NO_NEXT_TABLE, NULL, NO_HANDLER_DATA }
};

static USER_MSG_TBL SensorCmd [] =
{
   { "TYPE"        , NO_NEXT_TABLE,Sensor_UserCfg, USER_TYPE_ENUM,USER_RW,
     &ConfigTemp.type,0,MAX_SENSORS-1,NO_LIMIT,SensorType },
   { "NAME"        , NO_NEXT_TABLE,Sensor_UserCfg, USER_TYPE_STR, USER_RW,
     &ConfigTemp.sSensorName,0,MAX_SENSORS-1,0,MAX_SENSORNAME,NULL },
   { "OUTPUT"      , NO_NEXT_TABLE,Sensor_UserCfg, USER_TYPE_STR, USER_RW,
     &ConfigTemp.sOutputUnits,0,MAX_SENSORS-1,0,MAX_SENSORUNITS,NULL },
   { "INPUT"       , NO_NEXT_TABLE,Sensor_UserCfg, USER_TYPE_UINT8,USER_RW,
     &ConfigTemp.nInputChannel,0,MAX_SENSORS-1,NO_LIMIT,NULL },
   { "MAXSAMPLES"  , NO_NEXT_TABLE,Sensor_UserCfg, USER_TYPE_UINT8,USER_RW,
     &ConfigTemp.nMaximumSamples,0,MAX_SENSORS-1,1,MAX_SAMPLES, NULL },
   { "RATE"  , NO_NEXT_TABLE,Sensor_UserCfg, USER_TYPE_ENUM,USER_RW,
     &ConfigTemp.sampleRate,0,MAX_SENSORS-1,NO_LIMIT,SampleType },
   { "RATEOFFSET_MS", NO_NEXT_TABLE,Sensor_UserCfg, USER_TYPE_UINT16, USER_RW,
     &ConfigTemp.nSampleOffset_ms,0,MAX_SENSORS-1,0,1000,NULL },

   { "CALIBRATION" , SensorCalConfigCmd , NULL, NO_HANDLER_DATA },
   { "CONVERSION"  , SensorConvConfigCmd, NULL, NO_HANDLER_DATA },

   { "FILTER"      , SensorFilterConfigCmd, NULL, NO_HANDLER_DATA },

   { "SYSCOND"  , NO_NEXT_TABLE,Sensor_UserCfg, USER_TYPE_ENUM,USER_RW,
     &ConfigTemp.systemCondition,0,MAX_SENSORS-1, NO_LIMIT, Flt_UserEnumStatus  },
   { "SELFHEAL"    , NO_NEXT_TABLE, Sensor_UserCfg, USER_TYPE_BOOLEAN, USER_RW,
     &ConfigTemp.bSelfHeal, 0, MAX_SENSORS-1, NO_LIMIT, NULL },
   { "RANGETEST"   , NO_NEXT_TABLE,Sensor_UserCfg, USER_TYPE_BOOLEAN,USER_RW,
     &ConfigTemp.bRangeTest,0,MAX_SENSORS-1,NO_LIMIT,NULL  },
   { "RANGEDURATION_MS" , NO_NEXT_TABLE,Sensor_UserCfg, USER_TYPE_UINT16,USER_RW,
     &ConfigTemp.nRangeDuration_ms,0,MAX_SENSORS-1,NO_LIMIT,NULL  },
   { "MIN"         , NO_NEXT_TABLE,Sensor_UserCfg, USER_TYPE_FLOAT,USER_RW,
     &ConfigTemp.fMinValue,0,MAX_SENSORS-1,NO_LIMIT,NULL  },
   { "MAX"         , NO_NEXT_TABLE,Sensor_UserCfg, USER_TYPE_FLOAT,USER_RW,
     &ConfigTemp.fMaxValue,0,MAX_SENSORS-1,NO_LIMIT,NULL  },
   { "RATETEST"   , NO_NEXT_TABLE,Sensor_UserCfg, USER_TYPE_BOOLEAN,USER_RW,
     &ConfigTemp.bRateTest,0,MAX_SENSORS-1,NO_LIMIT,NULL  },
   { "RATETHRESHOLD", NO_NEXT_TABLE,Sensor_UserCfg, USER_TYPE_FLOAT,USER_RW,
     &ConfigTemp.fRateThreshold,0,MAX_SENSORS-1,NO_LIMIT,NULL  },
   { "RATEDURATION_MS" , NO_NEXT_TABLE,Sensor_UserCfg, USER_TYPE_UINT16,USER_RW,
     &ConfigTemp.nRateDuration_ms,0,MAX_SENSORS-1,NO_LIMIT,NULL  },
   { "SIGNALTEST", NO_NEXT_TABLE,Sensor_UserCfg, USER_TYPE_BOOLEAN,USER_RW,
     &ConfigTemp.bSignalTest,0,MAX_SENSORS-1,NO_LIMIT,NULL  },
   { "SIGNALTESTINDEX", NO_NEXT_TABLE,Sensor_UserCfg, USER_TYPE_ENUM,USER_RW,
     &ConfigTemp.signalTestIndex,0,MAX_SENSORS-1,NO_LIMIT,FaultIndexType  },
   { "SIGNALDURATION_MS", NO_NEXT_TABLE,Sensor_UserCfg, USER_TYPE_UINT16,USER_RW,
     &ConfigTemp.nSignalDuration_ms,0,MAX_SENSORS-1,NO_LIMIT,NULL  },
   { "INSPECT"   , NO_NEXT_TABLE,Sensor_UserCfg, USER_TYPE_BOOLEAN,USER_RW,
     &ConfigTemp.bInspectInclude,0,MAX_SENSORS-1,NO_LIMIT,NULL  },
   { "GPA"         , NO_NEXT_TABLE,Sensor_UserCfg, USER_TYPE_HEX32,USER_RW,
     &ConfigTemp.generalPurposeA,0,MAX_SENSORS-1,NO_LIMIT,NULL  },
   { "GPB"         , NO_NEXT_TABLE,Sensor_UserCfg, USER_TYPE_HEX32,USER_RW,
     &ConfigTemp.generalPurposeB,0,MAX_SENSORS-1,NO_LIMIT,NULL  },
   { NULL          , NO_NEXT_TABLE,Sensor_UserCfg, USER_TYPE_NONE,USER_RW,
     NULL,0,MAX_SENSORS-1,NO_LIMIT,NULL  }
};

static USER_MSG_TBL FilterState [] =
{
   { "LASTVALIDVALUE", NO_NEXT_TABLE, Sensor_State, USER_TYPE_FLOAT, USER_RO,
     &SensorTemp.filterData.fLastValidValue,     0, MAX_SENSORS - 1, NO_LIMIT, NULL    },
   { "LASTAVGVALUE"  , NO_NEXT_TABLE, Sensor_State, USER_TYPE_FLOAT, USER_RO,
     &SensorTemp.filterData.fLastAvgValue  ,     0, MAX_SENSORS - 1, NO_LIMIT, NULL    },
   { "REJECTCOUNT"   , NO_NEXT_TABLE, Sensor_State, USER_TYPE_UINT8, USER_RO,
     &SensorTemp.filterData.nRejectCount   ,     0, MAX_SENSORS - 1, NO_LIMIT, NULL    },
   { "K"             , NO_NEXT_TABLE, Sensor_State, USER_TYPE_FLOAT, USER_RO,
     &SensorTemp.filterData.fK              ,     0, MAX_SENSORS - 1, NO_LIMIT, NULL    },
   { "(n-1)/2"       , NO_NEXT_TABLE, Sensor_State, USER_TYPE_FLOAT, USER_RO,
     &SensorTemp.filterData.nMinusOneOverTwo,    0, MAX_SENSORS - 1, NO_LIMIT, NULL    },
   { "FULLSCALEINV"  , NO_NEXT_TABLE, Sensor_State, USER_TYPE_FLOAT, USER_RO,
     &SensorTemp.filterData.fullScaleInv,        0, MAX_SENSORS - 1, NO_LIMIT, NULL    },
   { "MAXPERCENT"    , NO_NEXT_TABLE, Sensor_State, USER_TYPE_FLOAT, USER_RO,
     &SensorTemp.filterData.maxPercent,          0, MAX_SENSORS - 1, NO_LIMIT, NULL    },
   { "SPIKEPERCENT"  , NO_NEXT_TABLE, Sensor_State,USER_TYPE_FLOAT,USER_RO,
     &SensorTemp.filterData.spikePercent,        0, MAX_SENSORS - 1, NO_LIMIT, NULL    },
   { "PERSISTSTATE"  , NO_NEXT_TABLE, Sensor_State, USER_TYPE_BOOLEAN, USER_RO,
     &SensorTemp.filterData.persistState,        0, MAX_SENSORS - 1, NO_LIMIT, NULL    },
   { "PERSISTSTATEZ1", NO_NEXT_TABLE, Sensor_State, USER_TYPE_BOOLEAN, USER_RO,
     &SensorTemp.filterData.persistStateZ1,      0, MAX_SENSORS - 1, NO_LIMIT, NULL    },
   { NULL            , NO_NEXT_TABLE, NULL, USER_TYPE_NONE,  USER_RW,
     NULL,0,MAX_SENSORS-1,NULL  }
};

static USER_MSG_TBL SensorStatus [] =
{
   { "INDEX"       , NO_NEXT_TABLE, Sensor_State,  USER_TYPE_ENUM,
     USER_RO       , &SensorTemp.nSensorIndex , 0, MAX_SENSORS - 1, NO_LIMIT,SensorIndexType },
   { "VALID"       , NO_NEXT_TABLE, Sensor_State,  USER_TYPE_BOOLEAN,
     USER_RO       , &SensorTemp.bValueIsValid  , 0, MAX_SENSORS - 1,NO_LIMIT,NULL },
   { "FVALUE"      , NO_NEXT_TABLE, Sensor_State, USER_TYPE_FLOAT,
     USER_RO       , &SensorTemp.fValue         , 0, MAX_SENSORS - 1,NO_LIMIT,NULL },
   { "PRIORVALUE"  , NO_NEXT_TABLE, Sensor_State, USER_TYPE_FLOAT,
     USER_RO       , &SensorTemp.fPriorValue    , 0, MAX_SENSORS - 1,NO_LIMIT,NULL },
   { "SAMPLECOUNTDOWN", NO_NEXT_TABLE, Sensor_State, USER_TYPE_UINT16,
     USER_RO       , &SensorTemp.nSampleCountdown, 0, MAX_SENSORS - 1, NO_LIMIT, NULL },
   { "NEXTSAMPLE"  , NO_NEXT_TABLE, Sensor_State, USER_TYPE_UINT8,
     USER_RO       , &SensorTemp.nNextSample     , 0, MAX_SENSORS - 1, NO_LIMIT, NULL },
   { "CURRENTSAMPLES", NO_NEXT_TABLE, Sensor_State, USER_TYPE_UINT8,
     USER_RO       , &SensorTemp.nCurrentSamples , 0, MAX_SENSORS - 1, NO_LIMIT, NULL },
   { "BITFAIL"     , NO_NEXT_TABLE, Sensor_State, USER_TYPE_BOOLEAN,
     USER_RO       , &SensorTemp.bBITFail, 0, MAX_SENSORS - 1, NO_LIMIT, NULL },
   { "RANGEFAIL", NO_NEXT_TABLE, Sensor_State, USER_TYPE_BOOLEAN,
     USER_RO       , &SensorTemp.bRangeFail, 0, MAX_SENSORS - 1, NO_LIMIT, NULL },
   { "RATEFAIL", NO_NEXT_TABLE, Sensor_State, USER_TYPE_BOOLEAN,
     USER_RO       , &SensorTemp.bRateFail, 0, MAX_SENSORS - 1, NO_LIMIT, NULL },
   { "RATEVALUE", NO_NEXT_TABLE, Sensor_State, USER_TYPE_FLOAT,
     USER_RO       , &SensorTemp.fRateValue, 0, MAX_SENSORS - 1, NO_LIMIT, NULL },
   { "SIGNALFAIL", NO_NEXT_TABLE, Sensor_State, USER_TYPE_BOOLEAN,
     USER_RO       , &SensorTemp.bSignalFail, 0, MAX_SENSORS - 1, NO_LIMIT, NULL },
   { "FILTERDATA", FilterState, NULL, NO_HANDLER_DATA },
   { "INTERFACEINDEX" , NO_NEXT_TABLE, Sensor_State, USER_TYPE_UINT16,
     USER_RO       , &SensorTemp.nInterfaceIndex, 0, MAX_SENSORS - 1, NO_LIMIT, NULL },
   { "LASTTICK"    , NO_NEXT_TABLE, Sensor_State, USER_TYPE_UINT32,
     USER_RO       , &SensorTemp.lastUpdateTick, 0, MAX_SENSORS - 1, NO_LIMIT, NULL },
   { NULL          , NO_NEXT_TABLE, Sensor_State, USER_TYPE_NONE, USER_RW,
     NULL,0,MAX_SENSORS-1, NO_LIMIT, NULL  }
};


static USER_MSG_TBL SensorRoot [] =
{/* Str          Next Tbl Ptr    Handler Func.            Data Type          Access                         Parameter     IndexRange   DataLimit   EnumTbl*/
 { "CFG",        SensorCmd      ,NULL, NO_HANDLER_DATA},
 { "STATUS",     SensorStatus   ,NULL, NO_HANDLER_DATA},
 { DISPLAY_CFG,  NO_NEXT_TABLE  ,Sensor_ShowConfig       ,USER_TYPE_ACTION  ,(USER_RO|USER_NO_LOG|USER_GSE) ,NULL         ,-1,-1      ,NO_LIMIT   ,NULL},
 { NULL,         NULL           ,NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL LiveDataCfg [] =
{ /*Str         Next Tbl Ptr    Handler Func.         Data Type         Access               Parameter               IndexRange  DataLimit                 EnumTbl*/
  { "TYPE",     NO_NEXT_TABLE,  Sensor_Live_Data_Cfg, USER_TYPE_ENUM,   (USER_RW|USER_GSE),  &LiveDataTemp.type,     -1,-1,      NO_LIMIT,                 LiveDataType },
  { "RATE_MS",  NO_NEXT_TABLE,  Sensor_Live_Data_Cfg, USER_TYPE_UINT32, (USER_RW|USER_GSE),  &LiveDataTemp.nRate_ms,  -1,-1,      SENSOR_LD_MAX_RATE,10000, NULL },
  { NULL,       NULL,           NULL,                 NO_HANDLER_DATA}
};

static USER_MSG_TBL LiveDataRoot [] =
{ /*Str         Next Tbl Ptr    Handler Func.         Data Type         Access               Parameter               IndexRange     DataLimit                 EnumTbl*/
  { "CFG",      LiveDataCfg,    NULL,                 NO_HANDLER_DATA},
  { "TYPE",     NO_NEXT_TABLE,  User_GenericAccessor, USER_TYPE_ENUM,   (USER_RW|USER_GSE),  &LiveDataDisplay.type,     -1,-1,      NO_LIMIT,                 LiveDataType },
  { "RATE_MS",  NO_NEXT_TABLE,  User_GenericAccessor, USER_TYPE_UINT32, (USER_RW|USER_GSE),  &LiveDataDisplay.nRate_ms,  -1,-1,      SENSOR_LD_MAX_RATE,10000, NULL },
  { NULL,       NULL,           NULL,                 NO_HANDLER_DATA}
};

static USER_MSG_TBL RootSensorMsg = {"SENSOR",    SensorRoot,   NULL, NO_HANDLER_DATA};
static USER_MSG_TBL LiveDataMsg   = {"LIVEDATA",  LiveDataRoot, NULL, NO_HANDLER_DATA};
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
* Returns:
*
* Notes:
*****************************************************************************/
USER_HANDLER_RESULT Sensor_UserCfg(USER_DATA_TYPE DataType,
                                   USER_MSG_PARAM Param,
                                   UINT32 Index,
                                   const void *SetPtr,
                                   void **GetPtr)
{
   USER_HANDLER_RESULT result;

   result = USER_RESULT_OK;
   //Get sensor structure based on index
   memcpy(&ConfigTemp,
      &CfgMgr_ConfigPtr()->SensorConfigs[Index],
      sizeof(ConfigTemp));

   result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
   if(SetPtr != NULL && USER_RESULT_OK == result)
   {
     memcpy(&CfgMgr_ConfigPtr()->SensorConfigs[Index], &ConfigTemp,sizeof(SENSOR_CONFIG));
     CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
                            &CfgMgr_ConfigPtr()->SensorConfigs[Index],
                            sizeof(ConfigTemp));
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
* Returns:
*
* Notes:
*****************************************************************************/

USER_HANDLER_RESULT Sensor_State(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr)
{
   USER_HANDLER_RESULT result;

   result = USER_RESULT_ERROR;

   memcpy(&SensorTemp, &Sensors[Index], sizeof(SensorTemp));

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
* Returns:
*
* Notes:
*****************************************************************************/
USER_HANDLER_RESULT Sensor_Live_Data_Cfg(USER_DATA_TYPE DataType,
                                     USER_MSG_PARAM Param,
                                     UINT32 Index,
                                     const void *SetPtr,
                                     void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_OK;

  //Get sensor structure based on index
  memcpy(&LiveDataTemp, &CfgMgr_ConfigPtr()->LiveDataConfig, sizeof(LiveDataTemp));

  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
  if ((SetPtr != NULL) && (USER_RESULT_OK == result))
  {
    // set non-vol config
    memcpy(&CfgMgr_ConfigPtr()->LiveDataConfig, &LiveDataTemp, sizeof(LiveDataTemp));
    CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
      &CfgMgr_ConfigPtr()->LiveDataConfig, sizeof(LiveDataTemp));

    #pragma ghs nowarning 1545 //Suppress packed structure alignment warning
    // set proper member of local working copy
    if (Param.Ptr == &LiveDataTemp.type)
    {
      // copy to local display control
      LiveDataDisplay.type = LiveDataTemp.type;
    }
    else if (Param.Ptr == &LiveDataTemp.nRate_ms)
    {
      // copy to local display control
      LiveDataDisplay.nRate_ms = LiveDataTemp.nRate_ms;
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
USER_HANDLER_RESULT Sensor_ShowConfig(USER_DATA_TYPE DataType,
                                      USER_MSG_PARAM Param,
                                      UINT32 Index,
                                      const void *SetPtr,
                                      void **GetPtr)
{
   USER_HANDLER_RESULT result;
   CHAR Label[USER_MAX_MSG_STR_LEN * 3];

   //Top-level name is a single indented space
   CHAR BranchName[USER_MAX_MSG_STR_LEN] = " ";

   USER_MSG_TBL*  pCfgTable;
   INT16 sensorIdx;

   result = USER_RESULT_OK;

   // Loop for each sensor.
   for (sensorIdx = 0; sensorIdx < MAX_SENSORS && result == USER_RESULT_OK; ++sensorIdx)
   {
      // Display sensor id info above each set of data.
      sprintf(Label, "\r\n\r\nSENSOR[%d].CFG", sensorIdx);
      result = USER_RESULT_ERROR;
      if (User_OutputMsgString( Label, FALSE ) )
      {
         pCfgTable = SensorCmd;  // Re-set the pointer to beginning of CFG table

         // User_DisplayConfigTree will invoke itself recursively to display all fields.
         result = User_DisplayConfigTree(BranchName, pCfgTable, sensorIdx, 0, NULL);
      }
   }
   return result;
}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/*****************************************************************************
*  MODIFICATIONS
*    $History: SensorUserTables.c $
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
