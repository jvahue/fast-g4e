#define EVENT_USERTABLES_BODY
/******************************************************************************
         Copyright (C) 2012-2014 Pratt & Whitney Engine Services, Inc.
            All Rights Reserved. Proprietary and Confidential.

  File:        EventUserTables.c

Description:   User command structures and functions for the event processing

VERSION
$Revision: 42 $  $Date: 1/23/15 7:53p $
******************************************************************************/
#ifndef EVENT_BODY
#error EventUserTables.c should only be included by Event.c
#endif

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define DATE_TIME_STR_LEN 32 //More than enough for "yyyy/mm/dd hh:mm:ss[.mmm]" 
/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static EVENT_CFG         configEventTemp;       // Event Configuration temp Storage
static EVENT_TABLE_CFG   configEventTableTemp;  // Event Table temp Storage

static EVENT_DATA        stateEventTemp;        // Event State Temp Storage
static EVENT_TABLE_DATA  stateEventTableTemp;   // Event Table State Temp Storage

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
//Prototype for the User Manager message handlers, has to go before
//the local variable tables that use the function pointer.
static
USER_HANDLER_RESULT Event_UserCfg       ( USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr );
static
USER_HANDLER_RESULT Event_State         ( USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr );
static
USER_HANDLER_RESULT Event_ShowConfig    ( USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr );
static
USER_HANDLER_RESULT Event_CfgExprStrCmd ( USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr );
static
USER_HANDLER_RESULT EventTable_UserCfg  ( USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr);
static
USER_HANDLER_RESULT EventTable_State    ( USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr );
static
USER_HANDLER_RESULT EventTable_ShowConfig ( USER_DATA_TYPE DataType,
                                            USER_MSG_PARAM Param,
                                            UINT32 Index,
                                            const void *SetPtr,
                                            void **GetPtr);

static
USER_HANDLER_RESULT Event_DisplayBuff    ( USER_DATA_TYPE DataType,
                                           USER_MSG_PARAM Param,
                                           UINT32 Index,
                                           const void *SetPtr,
                                           void **GetPtr);

static
USER_HANDLER_RESULT Event_ClearBuff     ( USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr);

static
USER_HANDLER_RESULT Event_LastCleared   ( USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr);

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static
USER_ENUM_TBL eventTableType[]   =
{  { "0"  , EVENT_TABLE_0   }, { "1"  ,  EVENT_TABLE_1   }, {  "2"  , EVENT_TABLE_2   },
   { "3"  , EVENT_TABLE_3   }, { "4"  ,  EVENT_TABLE_4   }, {  "5"  , EVENT_TABLE_5   },
   { "6"  , EVENT_TABLE_6   }, { "7"  ,  EVENT_TABLE_7   },
   { "UNUSED", EVENT_TABLE_UNUSED }, { NULL, 0}
};

static
USER_ENUM_TBL eventRateType[]   =  { { "1HZ"      , EV_1HZ          },
                                     { "2HZ"      , EV_2HZ          },
                                     { "4HZ"      , EV_4HZ          },
                                     { "5HZ"      , EV_5HZ          },
                                     { "10HZ"     , EV_10HZ         },
                                     { "20HZ"     , EV_20HZ         },
                                     { "50HZ"     , EV_50HZ         },
                                     { NULL       , 0               }
                                   };
static
USER_ENUM_TBL eventStateEnum[]  =  { {"NOT_USED"  , EVENT_NONE      },
                                     {"NOT_ACTIVE", EVENT_START     },
                                     {"ACTIVE"    , EVENT_ACTIVE    },
                                     {NULL        , 0               }
                                   };


// Note: Updates to EVENT_REGION has dependency to EVT_Region_UserEnumType[]
static
USER_ENUM_TBL evt_Region_UserEnumType [] =
{
  { "REGION_A",         REGION_A            },
  { "REGION_B",         REGION_B            },
  { "REGION_C",         REGION_C            },
  { "REGION_D",         REGION_D            },
  { "REGION_E",         REGION_E            },
  { "REGION_F",         REGION_F            },
  { "NONE",             REGION_NOT_FOUND    },
  { NULL,               0                   }
};

#pragma ghs nowarning 1545 //Suppress packed structure alignment warning

// Events - EVENT User and Configuration Table
static USER_MSG_TBL eventHist [] =
{
  /* Str              Next Tbl Ptr               Handler Func.          Data Type          Access     Parameter                           IndexRange           DataLimit            EnumTbl*/
  { "DISPLAY",        NO_NEXT_TABLE,            Event_DisplayBuff,      USER_TYPE_ACTION,   USER_RO,   NULL,                               -1,-1,               NO_LIMIT,            NULL                },
  { "CLEAR",          NO_NEXT_TABLE,            Event_ClearBuff,        USER_TYPE_ACTION,  USER_RO,   NULL,                               -1,-1,               NO_LIMIT,            NULL                },
  { "LASTCLEARED",    NO_NEXT_TABLE,            Event_LastCleared,      USER_TYPE_STR,     USER_RO,   NULL,                               -1,-1,               NO_LIMIT,            NULL                },
  { NULL,             NULL,                     NULL,                   NO_HANDLER_DATA }
  };

static USER_MSG_TBL eventCmd [] =
{
  /* Str              Next Tbl Ptr               Handler Func.          Data Type          Access     Parameter                           IndexRange           DataLimit            EnumTbl*/
  { "NAME",           NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_STR,     USER_RW,   &configEventTemp.sEventName,        0,(MAX_EVENTS-1),    0,MAX_EVENT_NAME,    NULL                },
  { "ID",             NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_STR,     USER_RW,   &configEventTemp.sEventID,          0,(MAX_EVENTS-1),    0,MAX_EVENT_ID,      NULL                },
  { "RATE",           NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &configEventTemp.rate,              0,(MAX_EVENTS-1),    NO_LIMIT,            eventRateType       },
  { "RATEOFFSET_MS",  NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_UINT16,  USER_RW,   &configEventTemp.nOffset_ms,        0,(MAX_EVENTS-1),    0,1000,              NULL                },
  { "SC",             NO_NEXT_TABLE,            Event_CfgExprStrCmd,    USER_TYPE_STR,     USER_RW,   &configEventTemp.startExpr,         0,(MAX_EVENTS-1),    NO_LIMIT,            NULL                },
  { "EC",             NO_NEXT_TABLE,            Event_CfgExprStrCmd,    USER_TYPE_STR,     USER_RW,   &configEventTemp.endExpr,           0,(MAX_EVENTS-1),    NO_LIMIT,            NULL                },
  { "DURATION_MS",    NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_UINT32,  USER_RW,   &configEventTemp.nMinDuration_ms,   0,(MAX_EVENTS-1),    0,3600000,           NULL                },
  { "ACTION",         NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_ACT_LIST,USER_RW,   &configEventTemp.nAction,           0,(MAX_EVENTS-1),    NO_LIMIT,            NULL                },
  { "ENABLE_HISTORY", NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_BOOLEAN, USER_RW,   &configEventTemp.bEnableTH,         0,(MAX_EVENTS-1),    NO_LIMIT,            NULL                },
  { "PRE_HISTORY_S",  NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_UINT32,  USER_RW,   &configEventTemp.preTime_s,         0,(MAX_EVENTS-1),    0,360,               NULL                },
  { "POST_HISTORY_S", NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_UINT32,  USER_RW,   &configEventTemp.postTime_s,        0,(MAX_EVENTS-1),    0,360,               NULL                },
  { "LOG_PRIORITY",   NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &configEventTemp.priority,          0,(MAX_EVENTS-1),    NO_LIMIT,            lm_UserEnumPriority },
  { "EVENT_TABLE",    NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &configEventTemp.eventTableIndex,   0,(MAX_EVENTS-1),    NO_LIMIT,            eventTableType      },
  { "SENSORS",        NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_SNS_LIST,USER_RW,   &configEventTemp.sensorMap,         0,(MAX_EVENTS-1),    0,MAX_EVENT_SENSORS, NULL                },
  { "BUFFENABLE",     NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_BOOLEAN, USER_RW,   &configEventTemp.bEnableBuff,       0,(MAX_EVENTS-1),    NO_LIMIT,            NULL                },
  { NULL,             NULL,                     NULL,                   NO_HANDLER_DATA }
};

static USER_MSG_TBL eventS0Tbl[] =
{
   /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
   {"SENSORID",   NO_NEXT_TABLE, Event_State,   USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.sensor[0].SensorIndex,       0,(MAX_EVENTS-1),    NO_LIMIT,     SensorIndexType },
   {"VALID",      NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[0].bValid,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MIN",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[0].fMinValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MAX",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[0].fMaxValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"AVG",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[0].fAvgValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"TOT",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[0].fTotal,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   { NULL,        NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL eventS1Tbl[] =
{
   /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
   {"SENSORID",   NO_NEXT_TABLE, Event_State,   USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.sensor[1].SensorIndex,       0,(MAX_EVENTS-1),    NO_LIMIT,     SensorIndexType },
   {"VALID",      NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[1].bValid,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MIN",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[1].fMinValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MAX",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[1].fMaxValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"AVG",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[1].fAvgValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"TOT",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[1].fTotal,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   { NULL,        NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL eventS2Tbl[] =
{
   /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
   {"SENSORID",   NO_NEXT_TABLE, Event_State,   USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.sensor[2].SensorIndex,       0,(MAX_EVENTS-1),    NO_LIMIT,     SensorIndexType },
   {"VALID",      NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[2].bValid,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MIN",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[2].fMinValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MAX",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[2].fMaxValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"AVG",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[2].fAvgValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"TOT",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[2].fTotal,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   { NULL,        NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL eventS3Tbl[] =
{
   /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
   {"SENSORID",   NO_NEXT_TABLE, Event_State,   USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.sensor[3].SensorIndex,       0,(MAX_EVENTS-1),    NO_LIMIT,     SensorIndexType },
   {"VALID",      NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[3].bValid,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MIN",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[3].fMinValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MAX",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[3].fMaxValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"AVG",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[3].fAvgValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"TOT",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[3].fTotal,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   { NULL,        NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL eventS4Tbl[] =
{
   /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
   {"SENSORID",   NO_NEXT_TABLE, Event_State,   USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.sensor[4].SensorIndex,       0,(MAX_EVENTS-1),    NO_LIMIT,     SensorIndexType },
   {"VALID",      NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[4].bValid,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MIN",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[4].fMinValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MAX",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[4].fMaxValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"AVG",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[4].fAvgValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"TOT",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[4].fTotal,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   { NULL,        NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL eventS5Tbl[] =
{
   /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
   {"SENSORID",   NO_NEXT_TABLE, Event_State,   USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.sensor[5].SensorIndex,       0,(MAX_EVENTS-1),    NO_LIMIT,     SensorIndexType },
   {"VALID",      NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[5].bValid,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MIN",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[5].fMinValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MAX",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[5].fMaxValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"AVG",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[5].fAvgValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"TOT",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[5].fTotal,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   { NULL,        NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL eventS6Tbl[] =
{
   /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
   {"SENSORID",   NO_NEXT_TABLE, Event_State,   USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.sensor[6].SensorIndex,       0,(MAX_EVENTS-1),    NO_LIMIT,     SensorIndexType },
   {"VALID",      NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[6].bValid,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MIN",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[6].fMinValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MAX",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[6].fMaxValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"AVG",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[6].fAvgValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"TOT",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[6].fTotal,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   { NULL,        NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL eventS7Tbl[] =
{
   /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
   {"SENSORID",   NO_NEXT_TABLE, Event_State,   USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.sensor[7].SensorIndex,       0,(MAX_EVENTS-1),    NO_LIMIT,     SensorIndexType },
//   {"INITIALIZE", NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[7].bInitialized,      0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"VALID",      NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[7].bValid,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MIN",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[7].fMinValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MAX",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[7].fMaxValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"AVG",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[7].fAvgValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"TOT",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[7].fTotal,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   { NULL,        NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL eventS8Tbl[] =
{
   /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
   {"SENSORID",   NO_NEXT_TABLE, Event_State,   USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.sensor[8].SensorIndex,       0,(MAX_EVENTS-1),    NO_LIMIT,     SensorIndexType },
//   {"INITIALIZE", NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[8].bInitialized,      0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"VALID",      NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[8].bValid,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MIN",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[8].fMinValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MAX",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[8].fMaxValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"AVG",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[8].fAvgValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"TOT",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[8].fTotal,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   { NULL,        NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL eventS9Tbl[] =
{
   /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
   {"SENSORID",   NO_NEXT_TABLE, Event_State,   USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.sensor[9].SensorIndex,       0,(MAX_EVENTS-1),    NO_LIMIT,     SensorIndexType },
//   {"INITIALIZE", NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[9].bInitialized,      0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"VALID",      NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[9].bValid,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MIN",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[9].fMinValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MAX",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[9].fMaxValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"AVG",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[9].fAvgValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"TOT",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[9].fTotal,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   { NULL,        NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL eventS10Tbl[] =
{
   /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
   {"SENSORID",   NO_NEXT_TABLE, Event_State,   USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.sensor[10].SensorIndex,       0,(MAX_EVENTS-1),    NO_LIMIT,     SensorIndexType },
   {"VALID",      NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[10].bValid,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MIN",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[10].fMinValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MAX",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[10].fMaxValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"AVG",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[10].fAvgValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"TOT",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[10].fTotal,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   { NULL,        NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL eventS11Tbl[] =
{
   /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
   {"SENSORID",   NO_NEXT_TABLE, Event_State,   USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.sensor[11].SensorIndex,       0,(MAX_EVENTS-1),    NO_LIMIT,     SensorIndexType },
   {"VALID",      NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[11].bValid,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MIN",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[11].fMinValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MAX",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[11].fMaxValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"AVG",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[11].fAvgValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"TOT",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[11].fTotal,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   { NULL,        NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL eventS12Tbl[] =
{
   /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
   {"SENSORID",   NO_NEXT_TABLE, Event_State,   USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.sensor[12].SensorIndex,       0,(MAX_EVENTS-1),    NO_LIMIT,     SensorIndexType },
   {"VALID",      NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[12].bValid,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MIN",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[12].fMinValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MAX",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[12].fMaxValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"AVG",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[12].fAvgValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"TOT",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[12].fTotal,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   { NULL,        NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL eventS13Tbl[] =
{
   /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
   {"SENSORID",   NO_NEXT_TABLE, Event_State,   USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.sensor[13].SensorIndex,       0,(MAX_EVENTS-1),    NO_LIMIT,     SensorIndexType },
   {"VALID",      NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[13].bValid,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MIN",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[13].fMinValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MAX",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[13].fMaxValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"AVG",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[13].fAvgValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"TOT",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[13].fTotal,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   { NULL,        NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL eventS14Tbl[] =
{
   /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
   {"SENSORID",   NO_NEXT_TABLE, Event_State,   USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.sensor[14].SensorIndex,       0,(MAX_EVENTS-1),    NO_LIMIT,     SensorIndexType },
   {"VALID",      NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[14].bValid,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MIN",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[14].fMinValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MAX",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[14].fMaxValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"AVG",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[14].fAvgValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"TOT",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[14].fTotal,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   { NULL,        NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL eventS15Tbl[] =
{
   /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
   {"SENSORID",   NO_NEXT_TABLE, Event_State,   USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.sensor[15].SensorIndex,       0,(MAX_EVENTS-1),    NO_LIMIT,     SensorIndexType },
   {"VALID",      NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[15].bValid,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MIN",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[15].fMinValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MAX",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[15].fMaxValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"AVG",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[15].fAvgValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"TOT",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[15].fTotal,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   { NULL,        NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL eventS16Tbl[] =
{
   /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
   {"SENSORID",   NO_NEXT_TABLE, Event_State,   USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.sensor[16].SensorIndex,       0,(MAX_EVENTS-1),    NO_LIMIT,     SensorIndexType },
   {"VALID",      NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[16].bValid,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MIN",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[16].fMinValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MAX",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[16].fMaxValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"AVG",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[16].fAvgValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"TOT",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[16].fTotal,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   { NULL,        NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL eventS17Tbl[] =
{
   /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
   {"SENSORID",   NO_NEXT_TABLE, Event_State,   USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.sensor[17].SensorIndex,       0,(MAX_EVENTS-1),    NO_LIMIT,     SensorIndexType },
   {"VALID",      NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[17].bValid,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MIN",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[17].fMinValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MAX",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[17].fMaxValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"AVG",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[17].fAvgValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"TOT",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[17].fTotal,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   { NULL,        NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL eventS18Tbl[] =
{
   /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
   {"SENSORID",   NO_NEXT_TABLE, Event_State,   USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.sensor[18].SensorIndex,       0,(MAX_EVENTS-1),    NO_LIMIT,     SensorIndexType },
   {"VALID",      NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[18].bValid,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MIN",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[18].fMinValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MAX",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[18].fMaxValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"AVG",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[18].fAvgValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"TOT",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[18].fTotal,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   { NULL,        NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL eventS19Tbl[] =
{
   /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
   {"SENSORID",   NO_NEXT_TABLE, Event_State,   USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.sensor[19].SensorIndex,       0,(MAX_EVENTS-1),    NO_LIMIT,     SensorIndexType },
   {"VALID",      NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[19].bValid,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MIN",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[19].fMinValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MAX",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[19].fMaxValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"AVG",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[19].fAvgValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"TOT",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[19].fTotal,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   { NULL,        NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL eventS20Tbl[] =
{
   /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
   {"SENSORID",   NO_NEXT_TABLE, Event_State,   USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.sensor[20].SensorIndex,       0,(MAX_EVENTS-1),    NO_LIMIT,     SensorIndexType },
   {"VALID",      NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[20].bValid,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MIN",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[20].fMinValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MAX",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[20].fMaxValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"AVG",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[20].fAvgValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"TOT",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[20].fTotal,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   { NULL,        NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL eventS21Tbl[] =
{
   /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
   {"SENSORID",   NO_NEXT_TABLE, Event_State,   USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.sensor[21].SensorIndex,       0,(MAX_EVENTS-1),    NO_LIMIT,     SensorIndexType },
   {"VALID",      NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[21].bValid,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MIN",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[21].fMinValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MAX",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[21].fMaxValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"AVG",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[21].fAvgValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"TOT",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[21].fTotal,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   { NULL,        NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL eventS22Tbl[] =
{
   /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
   {"SENSORID",   NO_NEXT_TABLE, Event_State,   USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.sensor[22].SensorIndex,       0,(MAX_EVENTS-1),    NO_LIMIT,     SensorIndexType },
   {"VALID",      NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[22].bValid,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MIN",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[22].fMinValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MAX",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[22].fMaxValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"AVG",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[22].fAvgValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"TOT",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[22].fTotal,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   { NULL,        NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL eventS23Tbl[] =
{
   /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
   {"SENSORID",   NO_NEXT_TABLE, Event_State,   USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.sensor[23].SensorIndex,       0,(MAX_EVENTS-1),    NO_LIMIT,     SensorIndexType },
   {"VALID",      NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[23].bValid,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MIN",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[23].fMinValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MAX",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[23].fMaxValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"AVG",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[23].fAvgValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"TOT",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[23].fTotal,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   { NULL,        NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL eventS24Tbl[] =
{
   /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
   {"SENSORID",   NO_NEXT_TABLE, Event_State,   USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.sensor[24].SensorIndex,       0,(MAX_EVENTS-1),    NO_LIMIT,     SensorIndexType },
   {"VALID",      NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[24].bValid,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MIN",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[24].fMinValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MAX",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[24].fMaxValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"AVG",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[24].fAvgValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"TOT",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[24].fTotal,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   { NULL,        NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL eventS25Tbl[] =
{
   /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
   {"SENSORID",   NO_NEXT_TABLE, Event_State,   USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.sensor[25].SensorIndex,       0,(MAX_EVENTS-1),    NO_LIMIT,     SensorIndexType },
   {"VALID",      NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[25].bValid,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MIN",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[25].fMinValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MAX",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[25].fMaxValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"AVG",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[25].fAvgValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"TOT",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[25].fTotal,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   { NULL,        NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL eventS26Tbl[] =
{
   /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
   {"SENSORID",   NO_NEXT_TABLE, Event_State,   USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.sensor[26].SensorIndex,       0,(MAX_EVENTS-1),    NO_LIMIT,     SensorIndexType },
   {"VALID",      NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[26].bValid,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MIN",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[26].fMinValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MAX",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[26].fMaxValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"AVG",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[26].fAvgValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"TOT",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[26].fTotal,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   { NULL,        NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL eventS27Tbl[] =
{
   /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
   {"SENSORID",   NO_NEXT_TABLE, Event_State,   USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.sensor[27].SensorIndex,       0,(MAX_EVENTS-1),    NO_LIMIT,     SensorIndexType },
   {"VALID",      NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[27].bValid,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MIN",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[27].fMinValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MAX",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[27].fMaxValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"AVG",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[27].fAvgValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"TOT",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[27].fTotal,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   { NULL,        NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL eventS28Tbl[] =
{
   /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
   {"SENSORID",   NO_NEXT_TABLE, Event_State,   USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.sensor[28].SensorIndex,       0,(MAX_EVENTS-1),    NO_LIMIT,     SensorIndexType },
   {"VALID",      NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[28].bValid,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MIN",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[28].fMinValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MAX",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[28].fMaxValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"AVG",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[28].fAvgValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"TOT",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[28].fTotal,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   { NULL,        NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL eventS29Tbl[] =
{
   /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
   {"SENSORID",   NO_NEXT_TABLE, Event_State,   USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.sensor[29].SensorIndex,       0,(MAX_EVENTS-1),    NO_LIMIT,     SensorIndexType },
   {"VALID",      NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[29].bValid,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MIN",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[29].fMinValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MAX",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[29].fMaxValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"AVG",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[29].fAvgValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"TOT",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[29].fTotal,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   { NULL,        NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL eventS30Tbl[] =
{
   /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
   {"SENSORID",   NO_NEXT_TABLE, Event_State,   USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.sensor[30].SensorIndex,       0,(MAX_EVENTS-1),    NO_LIMIT,     SensorIndexType },
   {"VALID",      NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[30].bValid,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MIN",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[30].fMinValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MAX",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[30].fMaxValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"AVG",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[30].fAvgValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"TOT",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[30].fTotal,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   { NULL,        NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL eventS31Tbl[] =
{
   /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
   {"SENSORID",   NO_NEXT_TABLE, Event_State,   USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.sensor[31].SensorIndex,       0,(MAX_EVENTS-1),    NO_LIMIT,     SensorIndexType },
   {"VALID",      NO_NEXT_TABLE, Event_State,   USER_TYPE_BOOLEAN, USER_RO,  &stateEventTemp.sensor[31].bValid,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MIN",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[31].fMinValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"MAX",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[31].fMaxValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"AVG",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[31].fAvgValue,         0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   {"TOT",        NO_NEXT_TABLE, Event_State,   USER_TYPE_FLOAT,   USER_RO,  &stateEventTemp.sensor[31].fTotal,            0,(MAX_EVENTS-1),    NO_LIMIT,     NULL            },
   { NULL,        NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL eventSensrTbl[] =
{
   { "S0",   eventS0Tbl,   NULL, NO_HANDLER_DATA },
   { "S1",   eventS1Tbl,   NULL, NO_HANDLER_DATA },
   { "S2",   eventS2Tbl,   NULL, NO_HANDLER_DATA },
   { "S3",   eventS3Tbl,   NULL, NO_HANDLER_DATA },
   { "S4",   eventS4Tbl,   NULL, NO_HANDLER_DATA },
   { "S5",   eventS5Tbl,   NULL, NO_HANDLER_DATA },
   { "S6",   eventS6Tbl,   NULL, NO_HANDLER_DATA },
   { "S7",   eventS7Tbl,   NULL, NO_HANDLER_DATA },
   { "S8",   eventS8Tbl,   NULL, NO_HANDLER_DATA },
   { "S9",   eventS9Tbl,   NULL, NO_HANDLER_DATA },
   { "S10",  eventS10Tbl,  NULL, NO_HANDLER_DATA },
   { "S11",  eventS11Tbl,  NULL, NO_HANDLER_DATA },
   { "S12",  eventS12Tbl,  NULL, NO_HANDLER_DATA },
   { "S13",  eventS13Tbl,  NULL, NO_HANDLER_DATA },
   { "S14",  eventS14Tbl,  NULL, NO_HANDLER_DATA },
   { "S15",  eventS15Tbl,  NULL, NO_HANDLER_DATA },
   { "S16",  eventS16Tbl,  NULL, NO_HANDLER_DATA },
   { "S17",  eventS17Tbl,  NULL, NO_HANDLER_DATA },
   { "S18",  eventS18Tbl,  NULL, NO_HANDLER_DATA },
   { "S19",  eventS19Tbl,  NULL, NO_HANDLER_DATA },
   { "S20",  eventS20Tbl,  NULL, NO_HANDLER_DATA },
   { "S21",  eventS21Tbl,  NULL, NO_HANDLER_DATA },
   { "S22",  eventS22Tbl,  NULL, NO_HANDLER_DATA },
   { "S23",  eventS23Tbl,  NULL, NO_HANDLER_DATA },
   { "S24",  eventS24Tbl,  NULL, NO_HANDLER_DATA },
   { "S25",  eventS25Tbl,  NULL, NO_HANDLER_DATA },
   { "S26",  eventS26Tbl,  NULL, NO_HANDLER_DATA },
   { "S27",  eventS27Tbl,  NULL, NO_HANDLER_DATA },
   { "S28",  eventS28Tbl,  NULL, NO_HANDLER_DATA },
   { "S29",  eventS29Tbl,  NULL, NO_HANDLER_DATA },
   { "S30",  eventS30Tbl,  NULL, NO_HANDLER_DATA },
   { "S31",  eventS31Tbl,  NULL, NO_HANDLER_DATA },
   { NULL,   NULL,         NULL, NO_HANDLER_DATA }
};

static USER_MSG_TBL eventStatus [] =
{
  /* Str             Next Tbl Ptr       Handler Func.    Data Type          Access     Parameter                           IndexRange           DataLimit   EnumTbl*/
   { "STATE",        NO_NEXT_TABLE,     Event_State,     USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.state,               0,(MAX_EVENTS-1),    NO_LIMIT,   eventStateEnum  },
   { "SEQUENCE",     NO_NEXT_TABLE,     Event_State,     USER_TYPE_UINT32,  USER_RO,  &stateEventTemp.seqNumber,           0,(MAX_EVENTS-1),    NO_LIMIT,   NULL            },
   { "STARTTIME_MS", NO_NEXT_TABLE,     Event_State,     USER_TYPE_UINT32,  USER_RO,  &stateEventTemp.nStartTime_ms,       0,(MAX_EVENTS-1),    NO_LIMIT,   NULL            },
   { "DURATION_MS",  NO_NEXT_TABLE,     Event_State,     USER_TYPE_UINT32,  USER_RO,  &stateEventTemp.nDuration_ms,        0,(MAX_EVENTS-1),    NO_LIMIT,   NULL            },
   { "SENSOR",       eventSensrTbl,     NULL,            NO_HANDLER_DATA },
   { NULL          , NULL,              NULL ,           NO_HANDLER_DATA }
};

static USER_MSG_TBL eventRoot [] =
{ /* Str            Next Tbl Ptr       Handler Func.        Data Type          Access            Parameter      IndexRange   DataLimit  EnumTbl*/
   { "CFG",         eventCmd     ,     NULL,                NO_HANDLER_DATA},
   { "STATUS",      eventStatus  ,     NULL,                NO_HANDLER_DATA},
   { "HISTORY",     eventHist    ,     NULL,                NO_HANDLER_DATA},
   { DISPLAY_CFG,   NO_NEXT_TABLE,     Event_ShowConfig,    USER_TYPE_ACTION,  USER_RO|USER_GSE|USER_NO_LOG, NULL,          -1, -1,      NO_LIMIT,  NULL},
   { NULL,          NULL,              NULL,                NO_HANDLER_DATA}
};


//*********************************************************************************************
// Event Tables - EVENT Table User and Configuration Table
//*********************************************************************************************

// Macro defines the 'prototype' declaration for entry in EventTable  Region-Segment Configuration
#define EVNT_TBL_SEG_ENTRY_DEF(r, s) &configEventTableTemp.regCfg.region[r].segment[s]

static USER_MSG_TBL eventTableRegionA [] =
{
   /* Str                 Next Tbl Ptr   Handler Func.       Data Type         Access   Parameter                                  IndexRange        DataLimit    EnumTbl*/
   { "SEG0_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(0,0).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(0,0).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(0,0).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(0,0).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG1_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(0,1).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(0,1).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(0,1).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(0,1).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG2_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(0,2).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(0,2).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(0,2).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(0,2).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG3_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(0,3).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(0,3).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(0,3).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(0,3).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG4_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(0,4).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(0,4).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(0,4).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(0,4).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG5_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(0,5).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(0,5).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(0,5).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(0,5).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "ACTION",            NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_ACT_LIST,USER_RW, &configEventTableTemp.regCfg.region[0].nAction,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { NULL,                NULL,                      NULL,                  NO_HANDLER_DATA }
};

static USER_MSG_TBL eventTableRegionB [] =
{
   /* Str                 Next Tbl Ptr   Handler Func.       Data Type         Access   Parameter                                  IndexRange        DataLimit    EnumTbl*/
   { "SEG0_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(1,0).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(1,0).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(1,0).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(1,0).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG1_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(1,1).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(1,1).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(1,1).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(1,1).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG2_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(1,2).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(1,2).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(1,2).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(1,2).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG3_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(1,3).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(1,3).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(1,3).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(1,3).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG4_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(1,4).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(1,4).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(1,4).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(1,4).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG5_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(1,5).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(1,5).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(1,5).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(1,5).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "ACTION",            NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_ACT_LIST,USER_RW, &configEventTableTemp.regCfg.region[1].nAction,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { NULL,                NULL,                      NULL,                  NO_HANDLER_DATA }
};

static USER_MSG_TBL eventTableRegionC [] =
{
   /* Str                 Next Tbl Ptr   Handler Func.       Data Type         Access   Parameter                                  IndexRange        DataLimit    EnumTbl*/
   { "SEG0_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(2,0).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(2,0).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(2,0).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(2,0).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG1_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(2,1).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(2,1).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(2,1).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(2,1).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG2_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(2,2).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(2,2).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(2,2).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(2,2).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG3_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(2,3).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(2,3).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(2,3).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(2,3).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG4_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(2,4).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(2,4).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(2,4).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(2,4).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG5_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(2,5).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(2,5).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(2,5).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(2,5).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "ACTION",            NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_ACT_LIST,USER_RW, &configEventTableTemp.regCfg.region[2].nAction,  0,(MAX_TABLES-1), NO_LIMIT,    NULL },
   { NULL,             NULL,                      NULL,                  NO_HANDLER_DATA }
};

static USER_MSG_TBL eventTableRegionD [] =
{
   /* Str                 Next Tbl Ptr   Handler Func.       Data Type         Access   Parameter                                  IndexRange        DataLimit    EnumTbl*/
   { "SEG0_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(3,0).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(3,0).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(3,0).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(3,0).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG1_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(3,1).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(3,1).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(3,1).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(3,1).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG2_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(3,2).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(3,2).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(3,2).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(3,2).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG3_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(3,3).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(3,3).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(3,3).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(3,3).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG4_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(3,4).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(3,4).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(3,4).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(3,4).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG5_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(3,5).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(3,5).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(3,5).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(3,5).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "ACTION",            NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_ACT_LIST,USER_RW, &configEventTableTemp.regCfg.region[3].nAction,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { NULL,             NULL,                      NULL,                  NO_HANDLER_DATA }
};

static USER_MSG_TBL eventTableRegionE [] =
{
   /* Str                 Next Tbl Ptr   Handler Func.       Data Type         Access   Parameter                                  IndexRange        DataLimit    EnumTbl*/
   { "SEG0_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(4,0).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(4,0).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(4,0).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(4,0).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG1_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(4,1).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(4,1).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(4,1).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(4,1).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG2_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(4,2).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(4,2).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(4,2).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(4,2).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG3_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(4,3).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(4,3).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(4,3).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(4,3).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG4_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(4,4).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(4,4).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(4,4).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(4,4).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG5_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(4,5).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(4,5).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(4,5).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(4,5).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "ACTION",            NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_ACT_LIST,USER_RW, &configEventTableTemp.regCfg.region[4].nAction,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { NULL,             NULL,                      NULL,                  NO_HANDLER_DATA }
};

static USER_MSG_TBL eventTableRegionF [] =
{
   /* Str                 Next Tbl Ptr   Handler Func.       Data Type         Access   Parameter                                  IndexRange        DataLimit    EnumTbl*/
   { "SEG0_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(5,0).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(5,0).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(5,0).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(5,0).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG1_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(5,1).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(5,1).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(5,1).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(5,1).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG2_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(5,2).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(5,2).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(5,2).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(5,2).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG3_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(5,3).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(5,3).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(5,3).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(5,3).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG4_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(5,4).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(5,4).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(5,4).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(5,4).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG5_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(5,5).fStartValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_START_TIME_MS",NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(5,5).nStartTime_ms, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, EVNT_TBL_SEG_ENTRY_DEF(5,5).fStopValue,    0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_STOP_TIME_MS", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, EVNT_TBL_SEG_ENTRY_DEF(5,5).nStopTime_ms,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "ACTION",            NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_ACT_LIST,USER_RW, &configEventTableTemp.regCfg.region[5].nAction,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { NULL,                NULL,          NULL,               NO_HANDLER_DATA }
};

static USER_MSG_TBL eventTableCmd [] =
{
  /* Str                 Next Tbl Ptr               Handler Func.          Data Type          Access     Parameter                                            IndexRange           DataLimit            EnumTbl*/
  { "SENSORINDEX",       NO_NEXT_TABLE,             EventTable_UserCfg,    USER_TYPE_ENUM,    USER_RW,   &configEventTableTemp.nSensor,                       0,(MAX_TABLES-1),    NO_LIMIT,            SensorIndexType },
  { "MINSENSORVALUE",    NO_NEXT_TABLE,             EventTable_UserCfg,    USER_TYPE_FLOAT,   USER_RW,   &configEventTableTemp.fTableEntryValue,              0,(MAX_TABLES-1),    NO_LIMIT,            NULL            },
  { "TBL_ENTRY_HYST",    NO_NEXT_TABLE,             EventTable_UserCfg,    USER_TYPE_FLOAT,   USER_RW,   &configEventTableTemp.tblHyst.fHystEntry,            0,(MAX_TABLES-1),    0,0x48f42400,        NULL            },
  { "TBL_EXIT_HYST",     NO_NEXT_TABLE,             EventTable_UserCfg,    USER_TYPE_FLOAT,   USER_RW,   &configEventTableTemp.tblHyst.fHystExit,             0,(MAX_TABLES-1),    0,0x48f42400,        NULL            },
  { "TBL_TRANSIENT_MS",  NO_NEXT_TABLE,             EventTable_UserCfg,    USER_TYPE_UINT32,  USER_RW,   &configEventTableTemp.tblHyst.nTransientAllowance_ms,0,(MAX_TABLES-1),    NO_LIMIT,            NULL            },
  { "ENABLETH",          NO_NEXT_TABLE,             EventTable_UserCfg,    USER_TYPE_BOOLEAN, USER_RW,   &configEventTableTemp.bEnableTH,                     0,(MAX_TABLES-1),    NO_LIMIT,            NULL            },
  { "PRE_HISTORY_S",     NO_NEXT_TABLE,             EventTable_UserCfg,    USER_TYPE_UINT32,  USER_RW,   &configEventTableTemp.preTime_s,                     0,(MAX_TABLES-1),    0,360,               NULL            },
  { "POST_HISTORY_S",    NO_NEXT_TABLE,             EventTable_UserCfg,    USER_TYPE_UINT32,  USER_RW,   &configEventTableTemp.postTime_s,                    0,(MAX_TABLES-1),    0,360,               NULL            },
  // Since PWC Engineering cannot decide on hysteresis being + the threshold, - the threshold, or both; added configuration for both
  // NOTE: Data Limit would not accept a floating point value for assigning FLT_MAX, by maxing out the integer value the system puts the value at float max.
  { "HYSTERESISPOS",     NO_NEXT_TABLE,             EventTable_UserCfg,    USER_TYPE_FLOAT,   USER_RW,   &configEventTableTemp.regCfg.fHysteresisPos,                0,(MAX_TABLES-1),    0,0x48f42400,        NULL            },
  { "HYSTERESISNEG",     NO_NEXT_TABLE,             EventTable_UserCfg,    USER_TYPE_FLOAT,   USER_RW,   &configEventTableTemp.regCfg.fHysteresisNeg,                0,(MAX_TABLES-1),    0,0x48f42400,        NULL            },
  { "TRANSIENT_MS",      NO_NEXT_TABLE,             EventTable_UserCfg,    USER_TYPE_UINT32,  USER_RW,   &configEventTableTemp.regCfg.nTransientAllowance_ms,        0,(MAX_TABLES-1),    NO_LIMIT,            NULL            },
  { "REGION_A",          eventTableRegionA,         NULL,                  NO_HANDLER_DATA },
  { "REGION_B",          eventTableRegionB,         NULL,                  NO_HANDLER_DATA },
  { "REGION_C",          eventTableRegionC,         NULL,                  NO_HANDLER_DATA },
  { "REGION_D",          eventTableRegionD,         NULL,                  NO_HANDLER_DATA },
  { "REGION_E",          eventTableRegionE,         NULL,                  NO_HANDLER_DATA },
  { "REGION_F",          eventTableRegionF,         NULL,                  NO_HANDLER_DATA },
  { NULL,                NULL,                      NULL,                  NO_HANDLER_DATA }
};

static USER_MSG_TBL eventTableStatus [] =
{
  /* Str                 Next Tbl Ptr    Handler Func.       Data Type          Access     Parameter                                                   IndexRange           DataLimit   EnumTbl*/
   { "STARTED",          NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_BOOLEAN, USER_RO,   &stateEventTableTemp.bStarted,                              0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "SEQUENCE",         NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &stateEventTableTemp.seqNumber,                             0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "STARTTIME_MS",     NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &stateEventTableTemp.nStartTime_ms,                         0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "CURRENTREGION",    NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_ENUM,    USER_RO,   &stateEventTableTemp.currentRegion,                         0,(MAX_TABLES-1),    NO_LIMIT,   evt_Region_UserEnumType },
   { "CONFIRMEDREGION",  NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_ENUM,    USER_RO,   &stateEventTableTemp.confirmedRegion,                       0,(MAX_TABLES-1),    NO_LIMIT,   evt_Region_UserEnumType },
   { "TOTALDURATION_MS", NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &stateEventTableTemp.nTotalDuration_ms,                     0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "MAXREGIONENTERED", NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_ENUM,    USER_RO,   &stateEventTableTemp.maximumRegionEntered,                  0,(MAX_TABLES-1),    NO_LIMIT,   evt_Region_UserEnumType },
   { "MAXSENSORVALUE",   NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_FLOAT,   USER_RO,   &stateEventTableTemp.fMaxSensorValue,                       0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "MAXSENSORTIME_MS", NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &stateEventTableTemp.nMaxSensorElaspedTime_ms,              0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "CURRENTSENSORVAL", NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_FLOAT,   USER_RO,   &stateEventTableTemp.fCurrentSensorValue,                   0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "MAXIMUMCFGREGION", NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_ENUM,    USER_RO,   &stateEventTableTemp.maximumCfgRegion,                      0,(MAX_TABLES-1),    NO_LIMIT,   evt_Region_UserEnumType },
   { "A_ENTEREDCOUNT",   NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &stateEventTableTemp.regionStats[0].logStats.nEnteredCount, 0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "A_EXITCOUNT",      NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &stateEventTableTemp.regionStats[0].logStats.nExitCount,    0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "A_DURATION_MS",    NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &stateEventTableTemp.regionStats[0].logStats.nDuration_ms,  0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "B_ENTEREDCOUNT",   NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &stateEventTableTemp.regionStats[1].logStats.nEnteredCount, 0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "B_EXITCOUNT",      NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &stateEventTableTemp.regionStats[1].logStats.nExitCount,    0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "B_DURATION_MS",    NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &stateEventTableTemp.regionStats[1].logStats.nDuration_ms,  0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "C_ENTEREDCOUNT",   NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &stateEventTableTemp.regionStats[2].logStats.nEnteredCount, 0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "C_EXITCOUNT",      NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &stateEventTableTemp.regionStats[2].logStats.nExitCount,    0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "C_DURATION_MS",    NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &stateEventTableTemp.regionStats[2].logStats.nDuration_ms,  0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "D_ENTEREDCOUNT",   NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &stateEventTableTemp.regionStats[3].logStats.nEnteredCount, 0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "D_EXITCOUNT",      NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &stateEventTableTemp.regionStats[3].logStats.nExitCount,    0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "D_DURATION_MS",    NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &stateEventTableTemp.regionStats[3].logStats.nDuration_ms,  0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "E_ENTEREDCOUNT",   NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &stateEventTableTemp.regionStats[4].logStats.nEnteredCount, 0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "E_EXITCOUNT",      NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &stateEventTableTemp.regionStats[4].logStats.nExitCount,    0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "E_DURATION_MS",    NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &stateEventTableTemp.regionStats[4].logStats.nDuration_ms,  0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "F_ENTEREDCOUNT",   NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &stateEventTableTemp.regionStats[5].logStats.nEnteredCount, 0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "F_EXITCOUNT",      NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &stateEventTableTemp.regionStats[5].logStats.nExitCount,    0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "F_DURATION_MS",    NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &stateEventTableTemp.regionStats[5].logStats.nDuration_ms,  0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { NULL          ,     NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_NONE,    USER_RW,   NULL,                                                       0,(MAX_TABLES-1),    NO_LIMIT,   NULL },
};


static USER_MSG_TBL eventTableRoot [] =
{ /* Str          Next Tbl Ptr      Handler Func.          Data Type          Access            Parameter      IndexRange   DataLimit  EnumTbl*/
   { "CFG",       eventTableCmd,    NULL,                  NO_HANDLER_DATA},
   { "STATUS",    eventTableStatus, NULL,                  NO_HANDLER_DATA},
   { DISPLAY_CFG, NO_NEXT_TABLE,    EventTable_ShowConfig, USER_TYPE_ACTION,  USER_RO|USER_GSE, NULL,          -1, -1,      NO_LIMIT,  NULL},
   { NULL,        NULL,             NULL,                  NO_HANDLER_DATA}
};

static
USER_MSG_TBL rootEventMsg = {"EVENT",eventRoot, NULL,NO_HANDLER_DATA};

static
USER_MSG_TBL rootEventTableMsg = {"EVENTTABLE", eventTableRoot, NULL, NO_HANDLER_DATA};

#pragma ghs endnowarning
/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
* Function:     Event_UserCfg
*
* Description:  Handles User Manager requests to change event configuration
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
*****************************************************************************/
static
USER_HANDLER_RESULT Event_UserCfg ( USER_DATA_TYPE DataType,
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
   memcpy(&configEventTemp,
          &CfgMgr_ConfigPtr()->EventConfigs[Index],
          sizeof(configEventTemp));

   result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
   if(SetPtr != NULL && USER_RESULT_OK == result)
   {
     //memcpy(&m_EventCfg[Index],&configEventTemp,sizeof(configEventTemp));
     memcpy(&CfgMgr_ConfigPtr()->EventConfigs[Index],
       &configEventTemp,
       sizeof(EVENT_CFG));
     //Store the modified temporary structure in the EEPROM.
     CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
       &CfgMgr_ConfigPtr()->EventConfigs[Index],
       sizeof(configEventTemp));
   }
   return result;
}

/******************************************************************************
* Function:     EventTable_UserCfg
*
* Description:  Handles User Manager requests to change event configuration
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
*****************************************************************************/
static
USER_HANDLER_RESULT EventTable_UserCfg ( USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr)
{
   USER_HANDLER_RESULT result;

   result = USER_RESULT_OK;

   //Load Event Table structure into the temporary location based on index param
   //Param.Ptr points to the struct member to be read/written
   //in the temporary location
   memcpy(&configEventTableTemp,
      &CfgMgr_ConfigPtr()->EventTableConfigs[Index],
      sizeof(configEventTableTemp));

   result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
   if(SetPtr != NULL && USER_RESULT_OK == result)
   {
     //memcpy(&m_EventCfg[Index],&configEventTemp,sizeof(configEventTemp));
     memcpy(&CfgMgr_ConfigPtr()->EventTableConfigs[Index],
       &configEventTableTemp,
       sizeof(EVENT_TABLE_CFG));
     //Store the modified temporary structure in the EEPROM.
     CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
       &CfgMgr_ConfigPtr()->EventTableConfigs[Index],
       sizeof(configEventTableTemp));
   }
   return result;
}

/******************************************************************************
* Function:     Event_State
*
* Description:  Handles User Manager requests to retrieve the current Event
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
*****************************************************************************/
static
USER_HANDLER_RESULT Event_State(USER_DATA_TYPE DataType,
                                  USER_MSG_PARAM Param,
                                  UINT32 Index,
                                  const void *SetPtr,
                                  void **GetPtr)
{
   USER_HANDLER_RESULT result;

   result = USER_RESULT_ERROR;

   memcpy(&stateEventTemp, &m_EventData[Index], sizeof(stateEventTemp));

   result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

   return result;
}

/******************************************************************************
* Function:     EventTable_State
*
* Description:  Handles User Manager requests to retrieve the current Event Table
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
*****************************************************************************/
static
USER_HANDLER_RESULT EventTable_State(USER_DATA_TYPE DataType,
                                     USER_MSG_PARAM Param,
                                     UINT32 Index,
                                     const void *SetPtr,
                                     void **GetPtr)
{
   USER_HANDLER_RESULT result;

   result = USER_RESULT_ERROR;

   memcpy(&stateEventTableTemp, &m_EventTableData[Index], sizeof(stateEventTableTemp));

   result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

   return result;
}

/******************************************************************************
* Function:    Event_ShowConfig
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
USER_HANDLER_RESULT Event_ShowConfig(USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr)
{
   CHAR  sLabelStem[] = "\r\n\r\nEVENT.CFG";
   CHAR  sLabel[USER_MAX_MSG_STR_LEN * 3];
   INT16 i;

   USER_HANDLER_RESULT result = USER_RESULT_OK;
   USER_MSG_TBL*  pCfgTable;

   //Top-level name is a single indented space
   CHAR sBranchName[USER_MAX_MSG_STR_LEN] = " ";

   pCfgTable = &eventCmd[0];  // Get pointer to config entry

   for (i = 0; i < MAX_EVENTS && result == USER_RESULT_OK; ++i)
   {
      // Display element info above each set of data.
      snprintf(sLabel, sizeof(sLabel), "%s[%d]", sLabelStem, i);

      result = USER_RESULT_ERROR;
      if ( User_OutputMsgString( sLabel, FALSE ) )
      {
         result = User_DisplayConfigTree(sBranchName, pCfgTable, i, 0, NULL);
      }
   }
   return result;
}

/******************************************************************************
* Function:    EventTable_ShowConfig
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
USER_HANDLER_RESULT EventTable_ShowConfig ( USER_DATA_TYPE DataType,
                                            USER_MSG_PARAM Param,
                                            UINT32 Index,
                                            const void *SetPtr,
                                            void **GetPtr)
{
   CHAR  sLabelStem[] = "\r\n\r\nEVENTTABLE.CFG";
   CHAR  sLabel[USER_MAX_MSG_STR_LEN * 3];
   INT16 i;

   USER_HANDLER_RESULT result = USER_RESULT_OK;
   USER_MSG_TBL*  pCfgTable;

   //Top-level name is a single indented space
   CHAR sBranchName[USER_MAX_MSG_STR_LEN] = " ";

   pCfgTable = &eventTableCmd[0];  // Get pointer to config entry

   for (i = 0; i < MAX_TABLES && result == USER_RESULT_OK; ++i)
   {
      // Display element info above each set of data.
      snprintf(sLabel, sizeof(sLabel), "%s[%d]", sLabelStem, i);

      result = USER_RESULT_ERROR;
      if ( User_OutputMsgString( sLabel, FALSE ) )
      {
         result = User_DisplayConfigTree(sBranchName, pCfgTable, i, 0, NULL);
      }
   }
   return result;
}

/******************************************************************************
 * Function:     Event_CfgExprStrCmd | USER COMMAND HANDLER
 *
 * Description:  Takes or returns a Event criteria expression string. Converts
 *               to and from the binary form of the string as it is stored
 *               in the configuration memory
 *
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
 *
 *****************************************************************************/
static
USER_HANDLER_RESULT Event_CfgExprStrCmd(USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_OK;
  INT32 strToBinResult;

  //User Mgr uses this obj outside of this function's scope
  static CHAR str[EVAL_MAX_EXPR_STR_LEN];


  // Move the current Event[x] config data into the local temp storage.
  // ( Parameter 'Param' is pointing to either the StartExpr or EndExpr struct
  // in the configEventTemp; see EventCmd Usertable)

  memcpy(&configEventTemp,
         &CfgMgr_ConfigPtr()->EventConfigs[Index],
         sizeof(configEventTemp));

  if(SetPtr == NULL)
  {
    // Getter - Convert binary expression in Param to string, return
    EvalExprBinToStr(str, (EVAL_EXPR*) Param.Ptr );
    *GetPtr = str;
  }
  else
  {
    // Performing a Set - Make copies of the input string and the dest structure.
    // Convert from string to binary form.
    // If successful, transfer the temp dest structure to the real one.

    strncpy_safe(str, sizeof(str), SetPtr, _TRUNCATE);

    strToBinResult = EvalExprStrToBin( EVAL_CALLER_TYPE_PARSE, (INT32)Index, str,
                                       (EVAL_EXPR*) Param.Ptr, MAX_EVENT_EXPR_OPRNDS );

    if(strToBinResult >= 0)
    {
      // Move the  successfully updated binary expression to the configEventTemp storage
      memcpy(&CfgMgr_ConfigPtr()->EventConfigs[Index],
             &configEventTemp,
             sizeof(configEventTemp));

      //Store the modified temporary structure in the EEPROM.
      CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
                             &CfgMgr_ConfigPtr()->EventConfigs[Index],
                             sizeof(configEventTemp) );
    }
    else
    {
      GSE_DebugStr(NORMAL,TRUE,"Event: Event_CfgExprStrCmd returned %d",strToBinResult);
      result = USER_RESULT_ERROR;
    }
  }

  return result;
}

/******************************************************************************
 * Function:     Event_DisplayBuff | USER COMMAND HANDLER
 *
 * Description:  Displays all configured events that were detected since the last
 *               event.history.clear was performed.
 *
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
 *
 *****************************************************************************/
static
USER_HANDLER_RESULT Event_DisplayBuff(USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr)
{
  TIMESTRUCT ts;
  CHAR         timeStr[DATE_TIME_STR_LEN];
  CHAR         regionStr[USER_MAX_MSG_STR_LEN];
  CHAR         tempStr[USER_MAX_MSG_STR_LEN * 4];
  static CHAR  outputBuffer[USER_SINGLE_MSG_MAX_SIZE];
  UINT8 i;
  UINT32 len;
  UINT32 nOffset;
  USER_HANDLER_RESULT result = USER_RESULT_OK;

  // Loop thru all event entries
  // only list those which are configured as active
  // ID, Name, Time, count, max-region

  nOffset = 0;
  for (i = 0; i < MAX_EVENTS; i++)
  {
    // Only display history items for enabled events which have been detected
    if( m_EventCfg[i].bEnableBuff &&
        m_EventHistory.histData[i].tsEvent.Timestamp != 0x00000000 )
    {
      // For table-event, set the string-value for the max region, otherwise "SIMPLE"
      snprintf(regionStr, sizeof( regionStr), "%s",
               EVENT_TABLE_UNUSED != m_EventCfg[i].eventTableIndex ?
               evt_Region_UserEnumType[ m_EventHistory.histData[i].maxRegion ].Str :
               "SIMPLE");

      // Convert the event start time to string
      CM_ConvertTimeStamptoTimeStruct( &m_EventHistory.histData[i].tsEvent, &ts);

      snprintf( timeStr, sizeof(timeStr), "%04d/%02d/%02d %02d:%02d:%02d",
                                           ts.Year, ts.Month,  ts.Day,
                                           ts.Hour, ts.Minute, ts.Second );

      // Create line item for this event history.
      snprintf ( tempStr, sizeof(tempStr), "\r\n%*s %*s %s %04d %s",
                                           MAX_EVENT_ID,   m_EventCfg[i].sEventID,
                                           MAX_EVENT_NAME, m_EventCfg[i].sEventName,
                                           timeStr,
                                           m_EventHistory.histData[i].nCnt,
                                           regionStr );
      len = strlen ( tempStr );
      memcpy ( (void *) &outputBuffer[nOffset], tempStr, len );
      nOffset += len;
    }

  }
  // Upon ending, print a final msg in case no events were displayed
  snprintf( tempStr, sizeof(tempStr), "\r\nHistory display completed\r\n\0" );
  len = strlen ( tempStr );
  memcpy( (void *) &outputBuffer[nOffset], tempStr, len );

  User_OutputMsgString(outputBuffer, FALSE);

  return result;
}

/******************************************************************************
 * Function:     Event_ClearBuff | USER COMMAND HANDLER
 *
 * Description:  Clear the contents of the Event Buffer and update the clear
 *               date field.
 *
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
 *
 *****************************************************************************/
static
USER_HANDLER_RESULT Event_ClearBuff ( USER_DATA_TYPE DataType,
                                      USER_MSG_PARAM Param,
                                      UINT32 Index,
                                      const void *SetPtr,
                                      void **GetPtr)
{
  // Call the generic event history buffer file init function.
  EventInitHistoryBuffer();

  return USER_RESULT_OK;

}
/******************************************************************************
 * Function:     Event_LastCleared | USER COMMAND HANDLER
 *
 * Description:  Displays the date field containing the the date/time the Event
                 Buffer was last cleared.
 *
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
 *
 *****************************************************************************/
static
USER_HANDLER_RESULT Event_LastCleared   ( USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr)
{
  TIMESTRUCT  ts;
  static CHAR timeStr[DATE_TIME_STR_LEN];

  CM_ConvertTimeStamptoTimeStruct( &m_EventHistory.tsLastCleared, &ts);

  // Convert the last-cleared timestamp to string
  snprintf( timeStr, sizeof(timeStr), "%04d/%02d/%02d %02d:%02d:%02d",
                                           ts.Year, ts.Month,  ts.Day,
                                           ts.Hour, ts.Minute, ts.Second );
  // Set the passed pointer to our static string buffer.
  *GetPtr = timeStr;
  return USER_RESULT_OK;
}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: EventUserTables.c $
 * 
 * *****************  Version 42  *****************
 * User: Contractor V&v Date: 1/23/15    Time: 7:53p
 * Updated in $/software/control processor/code/application
 * SCR #1188/1249 - Code Review compliance update
 * 
 * *****************  Version 41  *****************
 * User: John Omalley Date: 12/05/14   Time: 4:29p
 * Updated in $/software/control processor/code/application
 * SCR 1267 - Add Access type NO_LOG to show cfg
 * 
 * *****************  Version 40  *****************
 * User: Contractor V&v Date: 11/18/14   Time: 2:18p
 * Updated in $/software/control processor/code/application
 * Event Table  Hysteresis transient allowance typo
 * 
 * *****************  Version 39  *****************
 * User: Contractor V&v Date: 11/17/14   Time: 6:17p
 * Updated in $/software/control processor/code/application
 * SCR #1249 - Event Table  Hysteresis data limit fix range
 * 
 * *****************  Version 38  *****************
 * User: Contractor V&v Date: 11/11/14   Time: 5:29p
 * Updated in $/software/control processor/code/application
 * SCR #1249 - Event Table  TimeHistory 
 * 
 * *****************  Version 37  *****************
 * User: John Omalley Date: 11/11/14   Time: 3:26p
 * Updated in $/software/control processor/code/application
 * SCR 1267 - Modify max value of event table hysteresis to 500,000.0 or
 * 0x48f42400
 * 
 * *****************  Version 36  *****************
 * User: Contractor V&v Date: 11/03/14   Time: 5:24p
 * Updated in $/software/control processor/code/application
 * SCR #1249 - Event Table  Hysteresis
 *
 * *****************  Version 35  *****************
 * User: Contractor V&v Date: 10/20/14   Time: 3:54p
 * Updated in $/software/control processor/code/application
 * SCR #1188 - Event History Buffer
 *
 * *****************  Version 34  *****************
 * User: John Omalley Date: 12-12-19   Time: 6:03p
 * Updated in $/software/control processor/code/application
 * SCR 1197 - Code Review Update
 *
 * *****************  Version 33  *****************
 * User: John Omalley Date: 12-12-05   Time: 8:17a
 * Updated in $/software/control processor/code/application
 * SCR 1197 - Code Review Update
 *
 * *****************  Version 32  *****************
 * User: John Omalley Date: 12-11-28   Time: 2:25p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Code Review Updates
 *
 * *****************  Version 31  *****************
 * User: Contractor V&v Date: 11/26/12   Time: 12:33p
 * Updated in $/software/control processor/code/application
 * SCR #1167 SensorSummary
 *
 * *****************  Version 30  *****************
 * User: John Omalley Date: 12-11-14   Time: 6:51p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Code Review Updates
 *
 * *****************  Version 29  *****************
 * User: John Omalley Date: 12-11-13   Time: 4:36p
 * Updated in $/software/control processor/code/application
 * SCR 1197 - Code Review Updates
 *
 * *****************  Version 28  *****************
 * User: John Omalley Date: 12-11-07   Time: 2:52p
 * Updated in $/software/control processor/code/application
 * SCR 1107
 *
 * *****************  Version 27  *****************
 * User: John Omalley Date: 12-11-06   Time: 11:12a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Code Review Updates
 *
 * *****************  Version 26  *****************
 * User: Melanie Jutras Date: 12-10-31   Time: 2:01p
 * Updated in $/software/control processor/code/application
 * SCR #1142 File Format errors
 *
 * *****************  Version 25  *****************
 * User: John Omalley Date: 12-10-23   Time: 2:07p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Updates from Design Review
 * 1. Added Sequence Number for ground team
 * 2. Added Reason for table end to log
 * 3. Updated Event Index in log to ENUM
 * 4. Fixed hysteresis cfg range from 0-FLT_MAX
 *
 * *****************  Version 24  *****************
 * User: John Omalley Date: 12-10-18   Time: 1:56p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Design Review Updates
 *
 * *****************  Version 23  *****************
 * User: John Omalley Date: 12-10-16   Time: 2:37p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Updates per Design Review
 * 1. Changed segment times to milliseconds
 * 2. Limit +/- hysteresis to positive values
 *
 * *****************  Version 22  *****************
 * User: John Omalley Date: 12-09-19   Time: 10:54a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Added Time History enable configuration
 *
 * *****************  Version 21  *****************
 * User: Jeff Vahue   Date: 9/17/12    Time: 10:53a
 * Updated in $/software/control processor/code/application
 * SCR# 1107 - Add ACT_LIST, clean up msgs
 *
 * *****************  Version 20  *****************
 * User: Contractor V&v Date: 9/14/12    Time: 4:06p
 * Updated in $/software/control processor/code/application
 * FAST 2 fixes for sensor list
 *
 * *****************  Version 19  *****************
 * User: John Omalley Date: 12-09-13   Time: 9:42a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Added sensor statistics to the status command
 *
 * *****************  Version 18  *****************
 * User: John Omalley Date: 12-09-11   Time: 1:58p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Updated to new Timehistory fields
 *                     Fixed rate offset size in the table
 *
 * *****************  Version 17  *****************
 * User: Jeff Vahue   Date: 8/29/12    Time: 6:20p
 * Updated in $/software/control processor/code/application
 * SCR# 1107 - !P Table Full processing
 *
 * *****************  Version 16  *****************
 * User: Contractor V&v Date: 8/29/12    Time: 2:54p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 !P Processing
 *
 * *****************  Version 15  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 12:43p
 * Updated in $/software/control processor/code/application
 * SCR# 1142
 *
 * *****************  Version 14  *****************
 * User: Contractor V&v Date: 8/15/12    Time: 7:18p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 BITARRAY128 input as integer list
 *
 * *****************  Version 13  *****************
 * User: John Omalley Date: 12-08-09   Time: 8:37a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Removed Enumeration for Tables 8 and 9
 *
 * *****************  Version 12  *****************
 * User: John Omalley Date: 12-07-27   Time: 3:03p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Action Manager Persistent Updates
 *
 * *****************  Version 11  *****************
 * User: John Omalley Date: 12-07-19   Time: 5:09p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Cleaned up Code Review Tool findings
 *
 * *****************  Version 10  *****************
 * User: John Omalley Date: 12-07-17   Time: 11:26a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Removed the Event Action and made an Action object
 *
 * *****************  Version 9  *****************
 * User: John Omalley Date: 12-07-16   Time: 11:37a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Event Table Updates
 *
 * *****************  Version 8  *****************
 * User: John Omalley Date: 12-07-13   Time: 3:51p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Code updates, Event Action added, refactoring event table
 * logic
 *
 * *****************  Version 7  *****************
 * User: John Omalley Date: 12-06-07   Time: 11:35a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Removed Event Table Name Configuration
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
 *
 ***************************************************************************/

