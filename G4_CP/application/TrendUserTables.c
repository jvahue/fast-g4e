#define TREND_USERTABLES_BODY
/******************************************************************************
         Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
            All Rights Reserved. Proprietary and Confidential.

  File:        TrendUserTables.c

Description:   User command structures and functions for the trend processing

VERSION
$Revision: 3 $  $Date: 8/28/12 12:43p $
******************************************************************************/
#ifndef TREND_BODY
#error TrendUserTables.c should only be included by Trend.c
#endif

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Local Defines                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
//Prototype for the User Manager message handlers, has to go before
//the local variable tables that use the function pointer.
USER_HANDLER_RESULT Trend_UserCfg       ( USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr );

USER_HANDLER_RESULT  Trend_State         ( USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr );

USER_HANDLER_RESULT Trend_ShowConfig       ( USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr );

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static TREND_CFG         ConfigTrendTemp;      // Trend Configuration temp Storage

static TREND_DATA        StateTrendTemp;       // Trend State Temp Storage


USER_ENUM_TBL TrendType[]   =
{ 
   { "0"  , TREND_0   }, { "1"  ,  TREND_1   }, {  "2"  , TREND_2   },
   { "3"  , TREND_3   }, { "4"  ,  TREND_4   }, { "UNUSED", TREND_UNUSED },
   { NULL, 0}
};


USER_ENUM_TBL TrendRateType[] =
{
  { "1HZ"      , TREND_1HZ          },
  { "2HZ"      , TREND_2HZ          },
  { "4HZ"      , TREND_4HZ          },
  { "5HZ"      , TREND_5HZ          },
  { "10HZ"     , TREND_10HZ         },
  { "20HZ"     , TREND_20HZ         },
  { "50HZ"     , TREND_50HZ         },
  { NULL       , 0                  }
};

USER_ENUM_TBL TrendEngRunIdEnum[] =
{  
  { "0"     , ENGRUN_ID_0  },
  { "1"     , ENGRUN_ID_1  },
  { "2"     , ENGRUN_ID_2  },
  { "3"     , ENGRUN_ID_3  },
  { "100"   , ENGRUN_ID_ANY},
  { "UNUSED", ENGRUN_UNUSED},
  { NULL, 0 }
};



#pragma ghs nowarning 1545 //Suppress packed structure alignment warning




// Macro defines the 'prototype' declaration for entry in StabCritTbl
#define DECL_TREND_STAB_TBL_ENTRY( n )\
  static USER_MSG_TBL TrendStabTbl##n [] =\
{\
  /*Str          Next Tbl Ptr   Handler Func     Data Type          Access    Parameter                                       IndexRange           DataLimit     EnumTbl*/\
  {"SENSORID",   NO_NEXT_TABLE, Trend_UserCfg,   USER_TYPE_ENUM,    USER_RW,  &ConfigTrendTemp.stability[n].sensorIndex,      0,(MAX_TRENDS-1),    NO_LIMIT,     SensorIndexType },\
  {"LOWER",      NO_NEXT_TABLE, Trend_UserCfg,   USER_TYPE_FLOAT,   USER_RW,  &ConfigTrendTemp.stability[n].criteria.lower,   0,(MAX_TRENDS-1),    NO_LIMIT,     NULL            },\
  {"UPPER",      NO_NEXT_TABLE, Trend_UserCfg,   USER_TYPE_FLOAT,   USER_RW,  &ConfigTrendTemp.stability[n].criteria.upper,   0,(MAX_TRENDS-1),    NO_LIMIT,     NULL            },\
  {"VARIANCE",   NO_NEXT_TABLE, Trend_UserCfg,   USER_TYPE_FLOAT,   USER_RW,  &ConfigTrendTemp.stability[n].criteria.variance,0,(MAX_TRENDS-1),    NO_LIMIT,     NULL            },\
  { NULL,        NULL,          NULL, NO_HANDLER_DATA}\
}

  /* TREND STABILITY CRITERIA ELEMENTS 0 - 15 */
  DECL_TREND_STAB_TBL_ENTRY(0);
  DECL_TREND_STAB_TBL_ENTRY(1);
  DECL_TREND_STAB_TBL_ENTRY(2);
  DECL_TREND_STAB_TBL_ENTRY(3);
  DECL_TREND_STAB_TBL_ENTRY(4);
  DECL_TREND_STAB_TBL_ENTRY(5);
  DECL_TREND_STAB_TBL_ENTRY(6);
  DECL_TREND_STAB_TBL_ENTRY(7);
  DECL_TREND_STAB_TBL_ENTRY(8);
  DECL_TREND_STAB_TBL_ENTRY(9);
  DECL_TREND_STAB_TBL_ENTRY(10);
  DECL_TREND_STAB_TBL_ENTRY(11);
  DECL_TREND_STAB_TBL_ENTRY(12);
  DECL_TREND_STAB_TBL_ENTRY(13);
  DECL_TREND_STAB_TBL_ENTRY(14);
  DECL_TREND_STAB_TBL_ENTRY(15);


static USER_MSG_TBL StabCritTbl[] =
{
  { "S0",   TrendStabTbl0 ,  NULL, NO_HANDLER_DATA },
  { "S1",   TrendStabTbl1 ,  NULL, NO_HANDLER_DATA },
  { "S2",   TrendStabTbl2 ,  NULL, NO_HANDLER_DATA },
  { "S3",   TrendStabTbl3 ,  NULL, NO_HANDLER_DATA },
  { "S4",   TrendStabTbl4 ,  NULL, NO_HANDLER_DATA },
  { "S5",   TrendStabTbl5 ,  NULL, NO_HANDLER_DATA },
  { "S6",   TrendStabTbl6 ,  NULL, NO_HANDLER_DATA },
  { "S7",   TrendStabTbl7 ,  NULL, NO_HANDLER_DATA },
  { "S8",   TrendStabTbl8 ,  NULL, NO_HANDLER_DATA },
  { "S9",   TrendStabTbl9 ,  NULL, NO_HANDLER_DATA },
  { "S10",  TrendStabTbl10,  NULL, NO_HANDLER_DATA },
  { "S11",  TrendStabTbl11,  NULL, NO_HANDLER_DATA },
  { "S12",  TrendStabTbl12,  NULL, NO_HANDLER_DATA },
  { "S13",  TrendStabTbl13,  NULL, NO_HANDLER_DATA },
  { "S14",  TrendStabTbl14,  NULL, NO_HANDLER_DATA },
  { "S15",  TrendStabTbl15,  NULL, NO_HANDLER_DATA },
  { NULL,   NULL,            NULL, NO_HANDLER_DATA }
};

// Macro defines the 'prototype' declaration for entry in PrevStabValue table
#define DECL_TREND_PREV_STAB_VALUE_TBL_ENTRY( n )\
static USER_MSG_TBL PrevStableTbl##n[] =\
{\
  /*Str      Next Tbl Ptr   Handler Func   Data Type         Access    Parameter                     IndexRange            DataLimit     EnumTbl*/\
  {"VALUE",  NO_NEXT_TABLE,  Trend_State,   USER_TYPE_FLOAT,  USER_RO,  &StateTrendTemp.prevStabValue[n], 0,(MAX_TRENDS-1),    NO_LIMIT,     NULL     },\
  { NULL,    NULL,           NULL, NO_HANDLER_DATA}\
}

/* TREND PREVIOUS STABILITY VALUE ELEMENTS 0 - 15 */
DECL_TREND_PREV_STAB_VALUE_TBL_ENTRY(0 );
DECL_TREND_PREV_STAB_VALUE_TBL_ENTRY(1 );
DECL_TREND_PREV_STAB_VALUE_TBL_ENTRY(2 );
DECL_TREND_PREV_STAB_VALUE_TBL_ENTRY(3 );
DECL_TREND_PREV_STAB_VALUE_TBL_ENTRY(4 );
DECL_TREND_PREV_STAB_VALUE_TBL_ENTRY(5 );
DECL_TREND_PREV_STAB_VALUE_TBL_ENTRY(6 );
DECL_TREND_PREV_STAB_VALUE_TBL_ENTRY(7 );
DECL_TREND_PREV_STAB_VALUE_TBL_ENTRY(8 );
DECL_TREND_PREV_STAB_VALUE_TBL_ENTRY(9 );
DECL_TREND_PREV_STAB_VALUE_TBL_ENTRY(10);
DECL_TREND_PREV_STAB_VALUE_TBL_ENTRY(11);
DECL_TREND_PREV_STAB_VALUE_TBL_ENTRY(12);
DECL_TREND_PREV_STAB_VALUE_TBL_ENTRY(13);
DECL_TREND_PREV_STAB_VALUE_TBL_ENTRY(14);
DECL_TREND_PREV_STAB_VALUE_TBL_ENTRY(15);


static USER_MSG_TBL PrevStableTbl[] =
{
  { "V0",   PrevStableTbl0 ,  NULL, NO_HANDLER_DATA },
  { "V1",   PrevStableTbl1 ,  NULL, NO_HANDLER_DATA },
  { "V2",   PrevStableTbl2 ,  NULL, NO_HANDLER_DATA },
  { "V3",   PrevStableTbl3 ,  NULL, NO_HANDLER_DATA },
  { "V4",   PrevStableTbl4 ,  NULL, NO_HANDLER_DATA },
  { "V5",   PrevStableTbl5 ,  NULL, NO_HANDLER_DATA },
  { "V6",   PrevStableTbl6 ,  NULL, NO_HANDLER_DATA },
  { "V7",   PrevStableTbl7 ,  NULL, NO_HANDLER_DATA },
  { "V8",   PrevStableTbl8 ,  NULL, NO_HANDLER_DATA },
  { "V9",   PrevStableTbl9 ,  NULL, NO_HANDLER_DATA },
  { "V10",  PrevStableTbl10,  NULL, NO_HANDLER_DATA },
  { "V11",  PrevStableTbl11,  NULL, NO_HANDLER_DATA },
  { "V12",  PrevStableTbl12,  NULL, NO_HANDLER_DATA },
  { "V13",  PrevStableTbl13,  NULL, NO_HANDLER_DATA },
  { "V14",  PrevStableTbl14,  NULL, NO_HANDLER_DATA },
  { "V15",  PrevStableTbl15,  NULL, NO_HANDLER_DATA },
  { NULL,   NULL,             NULL, NO_HANDLER_DATA }
};

// Macro defines the 'prototype' declaration for entry in SensorSummary
#define DECL_SENSOR_SUMMARY_TBL_ENTRY( n)\
  static USER_MSG_TBL SnsrSummaryTbl##n[] =\
{\
  /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/\
  {"SENSORID",   NO_NEXT_TABLE, Trend_State,   USER_TYPE_ENUM,    USER_RO,  &StateTrendTemp.snsrSummary[n].SensorIndex,      0,(MAX_TRENDS-1),    NO_LIMIT,     SensorIndexType },\
  {"INITIALIZE", NO_NEXT_TABLE, Trend_State,   USER_TYPE_BOOLEAN, USER_RO,  &StateTrendTemp.snsrSummary[n].bInitialized,     0,(MAX_TRENDS-1),    NO_LIMIT,     NULL            },\
  {"VALID",      NO_NEXT_TABLE, Trend_State,   USER_TYPE_BOOLEAN, USER_RO,  &StateTrendTemp.snsrSummary[n].bValid,           0,(MAX_TRENDS-1),    NO_LIMIT,     NULL            },\
  {"MIN",        NO_NEXT_TABLE, Trend_State,   USER_TYPE_FLOAT,   USER_RO,  &StateTrendTemp.snsrSummary[n].fMinValue,        0,(MAX_TRENDS-1),    NO_LIMIT,     NULL            },\
  {"MAX",        NO_NEXT_TABLE, Trend_State,   USER_TYPE_FLOAT,   USER_RO,  &StateTrendTemp.snsrSummary[n].fMaxValue,        0,(MAX_TRENDS-1),    NO_LIMIT,     NULL            },\
  {"AVG",        NO_NEXT_TABLE, Trend_State,   USER_TYPE_FLOAT,   USER_RO,  &StateTrendTemp.snsrSummary[n].fAvgValue,        0,(MAX_TRENDS-1),    NO_LIMIT,     NULL            },\
  {"TOT",        NO_NEXT_TABLE, Trend_State,   USER_TYPE_FLOAT,   USER_RO,  &StateTrendTemp.snsrSummary[n].fTotal,           0,(MAX_TRENDS-1),    NO_LIMIT,     NULL            },\
  { NULL,        NULL,          NULL, NO_HANDLER_DATA}\
}

/* SENSOR SUMMARY ELEMENTS 0 - 31 */
DECL_SENSOR_SUMMARY_TBL_ENTRY(0 );
DECL_SENSOR_SUMMARY_TBL_ENTRY(1 );
DECL_SENSOR_SUMMARY_TBL_ENTRY(2 );
DECL_SENSOR_SUMMARY_TBL_ENTRY(3 );
DECL_SENSOR_SUMMARY_TBL_ENTRY(4 );
DECL_SENSOR_SUMMARY_TBL_ENTRY(5 );
DECL_SENSOR_SUMMARY_TBL_ENTRY(6 );
DECL_SENSOR_SUMMARY_TBL_ENTRY(7 );
DECL_SENSOR_SUMMARY_TBL_ENTRY(8 );
DECL_SENSOR_SUMMARY_TBL_ENTRY(9 );
DECL_SENSOR_SUMMARY_TBL_ENTRY(10);
DECL_SENSOR_SUMMARY_TBL_ENTRY(11);
DECL_SENSOR_SUMMARY_TBL_ENTRY(12);
DECL_SENSOR_SUMMARY_TBL_ENTRY(13);
DECL_SENSOR_SUMMARY_TBL_ENTRY(14);
DECL_SENSOR_SUMMARY_TBL_ENTRY(15);
DECL_SENSOR_SUMMARY_TBL_ENTRY(16);
DECL_SENSOR_SUMMARY_TBL_ENTRY(17);
DECL_SENSOR_SUMMARY_TBL_ENTRY(18);
DECL_SENSOR_SUMMARY_TBL_ENTRY(19);
DECL_SENSOR_SUMMARY_TBL_ENTRY(20);
DECL_SENSOR_SUMMARY_TBL_ENTRY(21);
DECL_SENSOR_SUMMARY_TBL_ENTRY(22);
DECL_SENSOR_SUMMARY_TBL_ENTRY(23);
DECL_SENSOR_SUMMARY_TBL_ENTRY(24);
DECL_SENSOR_SUMMARY_TBL_ENTRY(25);
DECL_SENSOR_SUMMARY_TBL_ENTRY(26);
DECL_SENSOR_SUMMARY_TBL_ENTRY(27);
DECL_SENSOR_SUMMARY_TBL_ENTRY(28);
DECL_SENSOR_SUMMARY_TBL_ENTRY(29);
DECL_SENSOR_SUMMARY_TBL_ENTRY(30);
DECL_SENSOR_SUMMARY_TBL_ENTRY(31);

static USER_MSG_TBL SnsrSummaryTbl[] =
{  
  { "S0",   SnsrSummaryTbl0 ,  NULL, NO_HANDLER_DATA },
  { "S1",   SnsrSummaryTbl1 ,  NULL, NO_HANDLER_DATA },
  { "S2",   SnsrSummaryTbl2 ,  NULL, NO_HANDLER_DATA },
  { "S3",   SnsrSummaryTbl3 ,  NULL, NO_HANDLER_DATA },
  { "S4",   SnsrSummaryTbl4 ,  NULL, NO_HANDLER_DATA },
  { "S5",   SnsrSummaryTbl5 ,  NULL, NO_HANDLER_DATA },
  { "S6",   SnsrSummaryTbl6 ,  NULL, NO_HANDLER_DATA },
  { "S7",   SnsrSummaryTbl7 ,  NULL, NO_HANDLER_DATA },
  { "S8",   SnsrSummaryTbl8 ,  NULL, NO_HANDLER_DATA },
  { "S9",   SnsrSummaryTbl9 ,  NULL, NO_HANDLER_DATA },
  { "S10",  SnsrSummaryTbl10,  NULL, NO_HANDLER_DATA },
  { "S11",  SnsrSummaryTbl11,  NULL, NO_HANDLER_DATA },
  { "S12",  SnsrSummaryTbl12,  NULL, NO_HANDLER_DATA },
  { "S13",  SnsrSummaryTbl13,  NULL, NO_HANDLER_DATA },
  { "S14",  SnsrSummaryTbl14,  NULL, NO_HANDLER_DATA },
  { "S15",  SnsrSummaryTbl15,  NULL, NO_HANDLER_DATA },
  { "S16",  SnsrSummaryTbl16,  NULL, NO_HANDLER_DATA },
  { "S17",  SnsrSummaryTbl17,  NULL, NO_HANDLER_DATA },
  { "S18",  SnsrSummaryTbl18,  NULL, NO_HANDLER_DATA },
  { "S19",  SnsrSummaryTbl19,  NULL, NO_HANDLER_DATA },
  { "S20",  SnsrSummaryTbl20,  NULL, NO_HANDLER_DATA },
  { "S21",  SnsrSummaryTbl21,  NULL, NO_HANDLER_DATA },
  { "S22",  SnsrSummaryTbl22,  NULL, NO_HANDLER_DATA },
  { "S23",  SnsrSummaryTbl23,  NULL, NO_HANDLER_DATA },
  { "S24",  SnsrSummaryTbl24,  NULL, NO_HANDLER_DATA },
  { "S25",  SnsrSummaryTbl25,  NULL, NO_HANDLER_DATA },
  { "S26",  SnsrSummaryTbl26,  NULL, NO_HANDLER_DATA },
  { "S27",  SnsrSummaryTbl27,  NULL, NO_HANDLER_DATA },
  { "S28",  SnsrSummaryTbl28,  NULL, NO_HANDLER_DATA },
  { "S29",  SnsrSummaryTbl29,  NULL, NO_HANDLER_DATA },
  { "S30",  SnsrSummaryTbl30,  NULL, NO_HANDLER_DATA },
  { "S31",  SnsrSummaryTbl31,  NULL, NO_HANDLER_DATA },
  { NULL,   NULL,              NULL, NO_HANDLER_DATA }
};


// Trends - TREND User and Configuration Table
static USER_MSG_TBL TrendCmd [] =
{
  /* Str              Next Tbl Ptr               Handler Func.          Data Type          Access     Parameter                           IndexRange           DataLimit            EnumTbl*/
  { "NAME",           NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_STR,     USER_RW,   &ConfigTrendTemp.trendName,         0,(MAX_TRENDS-1),    0,MAX_TREND_NAME,    NULL                 },
  { "RATE",           NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &ConfigTrendTemp.rate,              0,(MAX_TRENDS-1),    NO_LIMIT,            TrendRateType        },
  { "RATEOFFSET_MS",  NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_UINT32,  USER_RW,   &ConfigTrendTemp.nOffset_ms,        0,(MAX_TRENDS-1),    NO_LIMIT,            NULL                 },
  { "SAMPLEPERIOD_S", NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_UINT16,  USER_RW,   &ConfigTrendTemp.nSamplePeriod_s,   0,(MAX_TRENDS-1),    NO_LIMIT,            NULL                 },
  { "ENGINEID",       NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &ConfigTrendTemp.EngineRunId,       0,(MAX_TRENDS-1),    NO_LIMIT,            TrendEngRunIdEnum    },
  { "MAXSAMPLECOUNT", NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_UINT16,  USER_RW,   &ConfigTrendTemp.maxTrendSamples,   0,(MAX_TRENDS-1),    NO_LIMIT,            NULL                 },
  { "STARTTRIGID",    NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &ConfigTrendTemp.StartTrigger,      0,(MAX_TRENDS-1),    0,MAX_TRIGGERS,      TriggerIndexType     },
  { "RESETTRIGID",    NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &ConfigTrendTemp.ResetTrigger,      0,(MAX_TRENDS-1),    0,MAX_TRIGGERS,      TriggerIndexType     },
  { "INTERVAL_S",     NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_UINT32,  USER_RW,   &ConfigTrendTemp.TrendInterval_s,   0,(MAX_TRENDS-1),    NO_LIMIT,            NULL                 },
  { "SENSORS",        NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_128_LIST,USER_RW,   &ConfigTrendTemp.SensorMap,         0,(MAX_TRENDS-1),    0,MAX_TREND_SENSORS, NULL                 },
  { "CYCLEA",         NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &ConfigTrendTemp.nCycleA,           0,(MAX_TRENDS-1),    NO_LIMIT,            CycleEnumType        },
  { "CYCLEB",         NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &ConfigTrendTemp.nCycleB,           0,(MAX_TRENDS-1),    NO_LIMIT,            CycleEnumType        },
  { "CYCLEC",         NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &ConfigTrendTemp.nCycleC,           0,(MAX_TRENDS-1),    NO_LIMIT,            CycleEnumType        },
  { "CYCLED",         NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &ConfigTrendTemp.nCycleD,           0,(MAX_TRENDS-1),    NO_LIMIT,            CycleEnumType        },
  { "LSS_MASK",       NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_HEX8,    USER_RW,   &ConfigTrendTemp.LssMask,           0,(MAX_TRENDS-1),    NO_LIMIT,            NULL                 },
  { "STABLEPERIOD_S", NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_UINT16,  USER_RW,   &ConfigTrendTemp.nTimeStable_s,     0,(MAX_TRENDS-1),    NO_LIMIT,            NULL                 },
  { "LAMPENABLED",    NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_BOOLEAN, USER_RW,   &ConfigTrendTemp.lampEnabled,       0,(MAX_TRENDS-1),    NO_LIMIT,            NULL                 },
  { "STABILITY",      StabCritTbl,              NULL,                   NO_HANDLER_DATA,                                                                                                                 },
  { NULL,             NULL,                     NULL,                   NO_HANDLER_DATA }
};


static USER_MSG_TBL TrendStatus [] =
{
  /* Str                 Next Tbl Ptr       Handler Func.    Data Type          Access     Parameter                           IndexRange           DataLimit   EnumTbl*/
   { "ID",               NO_NEXT_TABLE,     Trend_State,     USER_TYPE_ENUM,    USER_RO,   &StateTrendTemp.trendIndex,         0,(MAX_TRENDS-1),    NO_LIMIT,   TrendType         },
   { "ACTIVE",           NO_NEXT_TABLE,     Trend_State,     USER_TYPE_BOOLEAN, USER_RO,   &StateTrendTemp.bTrendActive,       0,(MAX_TRENDS-1),    NO_LIMIT,   NULL              },
   { "AUTO_TREND",       NO_NEXT_TABLE,     Trend_State,     USER_TYPE_BOOLEAN, USER_RO,   &StateTrendTemp.bIsAutoTrend,       0,(MAX_TRENDS-1),    NO_LIMIT,   NULL              },
   { "NEXTSAMPLE_MS",    NO_NEXT_TABLE,     Trend_State,     USER_TYPE_UINT32,  USER_RO,   &StateTrendTemp.TimeNextSampleMs,   0,(MAX_TRENDS-1),    NO_LIMIT,   NULL              },
   { "ENG_STATE",        NO_NEXT_TABLE,     Trend_State,     USER_TYPE_ENUM,    USER_RO,   &StateTrendTemp.PrevEngState,       0,(MAX_TRENDS-1),    NO_LIMIT,   EngineRunStateEnum},
   { "TRENDLAMP",        NO_NEXT_TABLE,     Trend_State,     USER_TYPE_BOOLEAN, USER_RO,   &StateTrendTemp.bTrendLamp,         0,(MAX_TRENDS-1),    NO_LIMIT,   NULL              },
   { "SAMPLECOUNT",      NO_NEXT_TABLE,     Trend_State,     USER_TYPE_UINT16,  USER_RO,   &StateTrendTemp.cntTrendSamples,    0,(MAX_TRENDS-1),    NO_LIMIT,   NULL              },
   { "TIMESINCELAST_MS", NO_NEXT_TABLE,     Trend_State,     USER_TYPE_UINT32,  USER_RO,   &StateTrendTemp.nTimeSinceLastMs,   0,(MAX_TRENDS-1),    NO_LIMIT,   NULL              },
   // Stability data
   { "STABILITY_CNT",    NO_NEXT_TABLE,     Trend_State,     USER_TYPE_UINT16,  USER_RO,   &StateTrendTemp.nStability,         0,(MAX_TRENDS-1),    NO_LIMIT,   NULL              },
   { "TIMESTABLE_MS",    NO_NEXT_TABLE,     Trend_State,     USER_TYPE_UINT32,  USER_RO,   &StateTrendTemp.nTimeStableMs,     0,(MAX_TRENDS-1),    NO_LIMIT,   NULL              },
   { "PREVSTABLEVALUE",  PrevStableTbl,     NULL,            NO_HANDLER_DATA,                                                                                                     },
   // Monitored sensors during trends.
   { "SENSOR_COUNT",     NO_NEXT_TABLE,     Trend_State,     USER_TYPE_UINT16,  USER_RO,   &StateTrendTemp.nTotalSensors,      0,(MAX_TRENDS-1),    NO_LIMIT,   NULL              },
   { "SENSOR",           SnsrSummaryTbl,    NULL,            NO_HANDLER_DATA                                                                                                      },
   { NULL,               NULL,              NULL ,           NO_HANDLER_DATA } 
};

static USER_MSG_TBL TrendRoot [] =
{ /* Str            Next Tbl Ptr       Handler Func.        Data Type          Access            Parameter      IndexRange   DataLimit  EnumTbl*/
   { "CFG",         TrendCmd     ,     NULL,                NO_HANDLER_DATA},
   { "STATUS",      TrendStatus  ,     NULL,                NO_HANDLER_DATA},
   { DISPLAY_CFG,   NO_NEXT_TABLE,     Trend_ShowConfig,    USER_TYPE_ACTION,  USER_RO|USER_GSE, NULL,          -1, -1,      NO_LIMIT,  NULL},
   { NULL,          NULL,              NULL,                NO_HANDLER_DATA}
};

static USER_MSG_TBL  RootTrendMsg = {"TREND",TrendRoot,NULL,NO_HANDLER_DATA};

#pragma ghs endnowarning

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
* Function:     Trend_UserCfg
*
* Description:  Handles User Manager requests to change trend configuration
*               items.
*
* Parameters:   [in] DataType:  C type of the data to be read or changed, used
*                               for casting the data pointers
*               [in/out] Param: Pointer to the configuration item to be read
*                               or changed
*               [in] Index:     Index parameter is used to reference the
*                               specific Event to change. Range is validated
*                               by the user manager
*               [in] SetPtr:    For write commands, a pointer to the data to
*                               write to the configuration.
*               [out] GetPtr:   For read commands, UserCfg function will set
*                               this to the location of the data requested.

*
* Returns:      USER_HANDLER_RESULT
*
* Notes:
******************************************************************************/
USER_HANDLER_RESULT Trend_UserCfg ( USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr)
{
   USER_HANDLER_RESULT result;

   result = USER_RESULT_OK;

   //Load Event structure into the temporary location based on index param
   //Param.Ptr points to the struct member to be read/written
   //in the temporary location
   memcpy(&ConfigTrendTemp,
          &CfgMgr_ConfigPtr()->TrendConfigs[Index],
          sizeof(ConfigTrendTemp));

   result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
   if(SetPtr != NULL && USER_RESULT_OK == result)
   {
     memcpy ( &CfgMgr_ConfigPtr()->TrendConfigs[Index],
              &ConfigTrendTemp,
              sizeof(TREND_CFG));
     //Store the modified temporary structure in the EEPROM.
     CfgMgr_StoreConfigItem ( CfgMgr_ConfigPtr(),
                              &CfgMgr_ConfigPtr()->TrendConfigs[Index],
                              sizeof(ConfigTrendTemp));
   }
   return result;
}

/******************************************************************************
* Function:     Trend_State
*
* Description:  Handles User Manager requests to retrieve the current Trend
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
* Returns:      USER_HANDLER_RESULT
*
* Notes:
******************************************************************************/

USER_HANDLER_RESULT Trend_State(USER_DATA_TYPE DataType,
                                  USER_MSG_PARAM Param,
                                  UINT32 Index,
                                  const void *SetPtr,
                                  void **GetPtr)
{
   USER_HANDLER_RESULT result;

   result = USER_RESULT_ERROR;

   memcpy(&StateTrendTemp, &m_TrendData[Index], sizeof(StateTrendTemp));

   result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

   return result;
}

/******************************************************************************
* Function:    Trend_ShowConfig
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
******************************************************************************/
USER_HANDLER_RESULT Trend_ShowConfig ( USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr)
{
   CHAR  LabelStem[] = "\r\n\r\nTREND.CFG";
   CHAR  Label[USER_MAX_MSG_STR_LEN * 3];
   INT16 i;

   USER_HANDLER_RESULT result = USER_RESULT_OK;
   USER_MSG_TBL*  pCfgTable;

   //Top-level name is a single indented space
   CHAR BranchName[USER_MAX_MSG_STR_LEN] = " ";

   pCfgTable = &TrendCmd[0];  // Get pointer to config entry

   for (i = 0; i < MAX_EVENTS && result == USER_RESULT_OK; ++i)
   {
      // Display element info above each set of data.
      sprintf(Label, "%s[%d]", LabelStem, i);

      result = USER_RESULT_ERROR;
      if ( User_OutputMsgString( Label, FALSE ) )
      {
         result = User_DisplayConfigTree(BranchName, pCfgTable, i, 0, NULL);
      }
   }
   return result;
}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: TrendUserTables.c $
 * 
 * *****************  Version 3  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 12:43p
 * Updated in $/software/control processor/code/application
 * SCR# 1142
 * 
 * *****************  Version 2  *****************
 * User: Contractor V&v Date: 8/08/12    Time: 5:40p
 * Updated in $/software/control processor/code/application
 * 
 * *****************  Version 1  *****************
 * User: John Omalley Date: 12-06-07   Time: 11:32a
 * Created in $/software/control processor/code/application
 *
 *
 *
 *
 ***************************************************************************/


