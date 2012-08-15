/******************************************************************************
         Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
            All Rights Reserved. Proprietary and Confidential.

  File:        EventUserTables.c

Description:   User command structures and functions for the event processing

VERSION
$Revision: 14 $  $Date: 8/15/12 7:18p $
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
USER_HANDLER_RESULT Event_UserCfg       ( USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr );

USER_HANDLER_RESULT Event_State         ( USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr );

USER_HANDLER_RESULT Event_ShowConfig       ( USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr );

USER_HANDLER_RESULT Event_CfgExprStrCmd ( USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr );

USER_HANDLER_RESULT EventTable_UserCfg  ( USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr);

USER_HANDLER_RESULT EventTable_State    ( USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr );

USER_HANDLER_RESULT EventTable_ShowConfig ( USER_DATA_TYPE DataType,
                                            USER_MSG_PARAM Param,
                                            UINT32 Index,
                                            const void *SetPtr,
                                            void **GetPtr);
/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static EVENT_CFG         configEventTemp;       // Event Configuration temp Storage
static EVENT_TABLE_CFG   configEventTableTemp;  // Event Table temp Storage

static EVENT_DATA        stateEventTemp;        // Event State Temp Storage
static EVENT_TABLE_DATA  stateEventTableTemp;   // Event Table State Temp Storage

USER_ENUM_TBL eventTableType[]   =
{  { "0"  , EVENT_TABLE_0   }, { "1"  ,  EVENT_TABLE_1   }, {  "2"  , EVENT_TABLE_2   },
   { "3"  , EVENT_TABLE_3   }, { "4"  ,  EVENT_TABLE_4   }, {  "5"  , EVENT_TABLE_5   },
   { "6"  , EVENT_TABLE_6   }, { "7"  ,  EVENT_TABLE_7   },
   { "UNUSED", EVENT_TABLE_UNUSED }, { NULL, 0}
};


USER_ENUM_TBL eventRateType[]   =  { { "1HZ"      , EV_1HZ          },
                                     { "2HZ"      , EV_2HZ          },
                                     { "4HZ"      , EV_4HZ          },
                                     { "5HZ"      , EV_5HZ          },
                                     { "10HZ"     , EV_10HZ         },
                                     { "20HZ"     , EV_20HZ         },
                                     { "50HZ"     , EV_50HZ         },
                                     { NULL       , 0               }
                                   };

USER_ENUM_TBL eventStateEnum[]  =  { {"NOT_USED"  , EVENT_NONE      },
                                     {"NOT_ACTIVE", EVENT_START     },
                                     {"ACTIVE"    , EVENT_ACTIVE    },
                                     {NULL        , 0               }
                                   };

#pragma ghs nowarning 1545 //Suppress packed structure alignment warning

// Events - EVENT User and Configuration Table
static USER_MSG_TBL eventCmd [] =
{
  /* Str              Next Tbl Ptr               Handler Func.          Data Type          Access     Parameter                           IndexRange           DataLimit            EnumTbl*/
  { "NAME",           NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_STR,     USER_RW,   &configEventTemp.sEventName,        0,(MAX_EVENTS-1),    0,MAX_EVENT_NAME,    NULL                },
  { "ID",             NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_STR,     USER_RW,   &configEventTemp.sEventID,          0,(MAX_EVENTS-1),    0,MAX_EVENT_ID,      NULL                },
  { "RATE",           NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &configEventTemp.rate,              0,(MAX_EVENTS-1),    NO_LIMIT,            eventRateType       },
  { "RATEOFFSET_MS",  NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_UINT32,  USER_RW,   &configEventTemp.nOffset_ms,        0,(MAX_EVENTS-1),    0,1000,              NULL                },
  { "SC",             NO_NEXT_TABLE,            Event_CfgExprStrCmd,    USER_TYPE_STR,     USER_RW,   &configEventTemp.startExpr,         0,(MAX_EVENTS-1),    NO_LIMIT,            NULL                },
  { "EC",             NO_NEXT_TABLE,            Event_CfgExprStrCmd,    USER_TYPE_STR,     USER_RW,   &configEventTemp.endExpr,           0,(MAX_EVENTS-1),    NO_LIMIT,            NULL                },
  { "DURATION_MS",    NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_UINT32,  USER_RW,   &configEventTemp.nMinDuration_ms,   0,(MAX_EVENTS-1),    0,3600000,            NULL                },
  { "ACTION",         NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_UINT32,  USER_RW,   &configEventTemp.nAction,           0,(MAX_EVENTS-1),    NO_LIMIT,            NULL                },
  { "PRETIMEHISTORY", NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &configEventTemp.preTimeHistory,    0,(MAX_EVENTS-1),    NO_LIMIT,            TH_UserEnumType     },
  { "PRETIME_S",      NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_UINT16,  USER_RW,   &configEventTemp.preTime_s,         0,(MAX_EVENTS-1),    NO_LIMIT,            NULL                },
  { "POSTTIMEHISTORY",NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &configEventTemp.postTimeHistory,   0,(MAX_EVENTS-1),    NO_LIMIT,            TH_UserEnumType     },
  { "POSTTIME_S",     NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_UINT16,  USER_RW,   &configEventTemp.postTime_s,        0,(MAX_EVENTS-1),    NO_LIMIT,            NULL                },
  { "LOG_PRIORITY",   NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &configEventTemp.priority,          0,(MAX_EVENTS-1),    NO_LIMIT,            LM_UserEnumPriority },
  { "EVENT_TABLE",    NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &configEventTemp.eventTableIndex,   0,(MAX_EVENTS-1),    NO_LIMIT,            eventTableType      },
  { "SENSORS",        NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_128_LIST,USER_RW,   &configEventTemp.sensorMap,         0,(MAX_EVENTS-1),    0,MAX_SENSORS-1,     NULL                },
  { NULL,             NULL,                     NULL,                   NO_HANDLER_DATA }
};


static USER_MSG_TBL eventStatus [] =
{
  /* Str             Next Tbl Ptr       Handler Func.    Data Type          Access     Parameter                           IndexRange           DataLimit   EnumTbl*/
   { "STATE",        NO_NEXT_TABLE,     Event_State,     USER_TYPE_ENUM,    USER_RO,  &stateEventTemp.state,               0,(MAX_EVENTS-1),    NO_LIMIT,   eventStateEnum  },
   { "STARTTIME_MS", NO_NEXT_TABLE,     Event_State,     USER_TYPE_UINT32,  USER_RO,  &stateEventTemp.nStartTime_ms,       0,(MAX_EVENTS-1),    NO_LIMIT,   NULL            },
   { "DURATION_MS",  NO_NEXT_TABLE,     Event_State,     USER_TYPE_UINT32,  USER_RO,  &stateEventTemp.nDuration_ms,        0,(MAX_EVENTS-1),    NO_LIMIT,   NULL            },
   { "SAMPLECOUNT",  NO_NEXT_TABLE,     Event_State,     USER_TYPE_UINT32,  USER_RO,  &stateEventTemp.nSampleCount,        0,(MAX_EVENTS-1),    NO_LIMIT,   NULL            },
   { NULL          , NULL,              NULL ,           NO_HANDLER_DATA }
};

static USER_MSG_TBL eventRoot [] =
{ /* Str            Next Tbl Ptr       Handler Func.        Data Type          Access            Parameter      IndexRange   DataLimit  EnumTbl*/
   { "CFG",         eventCmd     ,     NULL,                NO_HANDLER_DATA},
   { "STATUS",      eventStatus  ,     NULL,                NO_HANDLER_DATA},
   { DISPLAY_CFG,   NO_NEXT_TABLE,     Event_ShowConfig,    USER_TYPE_ACTION,  USER_RO|USER_GSE, NULL,          -1, -1,      NO_LIMIT,  NULL},
   { NULL,          NULL,              NULL,                NO_HANDLER_DATA}
};

static
USER_MSG_TBL    rootEventMsg = {"EVENT",eventRoot,NULL,NO_HANDLER_DATA};
//*********************************************************************************************
// Event Tables - EVENT Table User and Configuration Table
//*********************************************************************************************
static USER_MSG_TBL eventTableRegionA [] =
{
  /* Str                 Next Tbl Ptr   Handler Func.       Data Type         Access   Parameter                                                IndexRange        DataLimit    EnumTbl*/
  { "SEG0_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[0].segment[0].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG0_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[0].segment[0].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG0_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[0].segment[0].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG0_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[0].segment[0].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

  { "SEG1_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[0].segment[1].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG1_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[0].segment[1].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG1_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[0].segment[1].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG1_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[0].segment[1].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

  { "SEG2_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[0].segment[2].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG2_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[0].segment[2].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG2_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[0].segment[2].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG2_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[0].segment[2].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

  { "SEG3_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[0].segment[3].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG3_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[0].segment[3].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG3_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[0].segment[3].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG3_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[0].segment[3].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

  { "SEG4_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[0].segment[4].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG4_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[0].segment[4].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG4_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[0].segment[4].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG4_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[0].segment[4].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

  { "SEG5_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[0].segment[5].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG5_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[0].segment[5].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG5_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[0].segment[5].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG5_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[0].segment[5].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "ACTION",            NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_HEX32,  USER_RW, &configEventTableTemp.region[0].nAction,                 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { NULL,             NULL,                      NULL,                  NO_HANDLER_DATA }
};

static USER_MSG_TBL eventTableRegionB [] =
{
   /* Str                 Next Tbl Ptr   Handler Func.       Data Type         Access   Parameter                                                IndexRange        DataLimit    EnumTbl*/
   { "SEG0_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[1].segment[0].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[1].segment[0].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[1].segment[0].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[1].segment[0].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG1_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[1].segment[1].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[1].segment[1].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[1].segment[1].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[1].segment[1].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG2_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[1].segment[2].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[1].segment[2].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[1].segment[2].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[1].segment[2].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG3_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[1].segment[3].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[1].segment[3].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[1].segment[3].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[1].segment[3].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG4_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[1].segment[4].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[1].segment[4].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[1].segment[4].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[1].segment[4].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG5_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[1].segment[5].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[1].segment[5].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[1].segment[5].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[1].segment[5].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "ACTION",            NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_HEX32,  USER_RW, &configEventTableTemp.region[1].nAction,                 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { NULL,             NULL,                      NULL,                  NO_HANDLER_DATA }
};

static USER_MSG_TBL eventTableRegionC [] =
{
   /* Str                 Next Tbl Ptr   Handler Func.       Data Type         Access   Parameter                                                IndexRange        DataLimit    EnumTbl*/
   { "SEG0_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[2].segment[0].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[2].segment[0].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[2].segment[0].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[2].segment[0].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG1_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[2].segment[1].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[2].segment[1].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[2].segment[1].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[2].segment[1].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG2_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[2].segment[2].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[2].segment[2].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[2].segment[2].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[2].segment[2].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG3_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[2].segment[3].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[2].segment[3].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[2].segment[3].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[2].segment[3].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG4_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[2].segment[4].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[2].segment[4].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[2].segment[4].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[2].segment[4].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG5_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[2].segment[5].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[2].segment[5].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[2].segment[5].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[2].segment[5].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "ACTION",            NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_HEX32,  USER_RW, &configEventTableTemp.region[2].nAction,                 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { NULL,             NULL,                      NULL,                  NO_HANDLER_DATA }
};

static USER_MSG_TBL eventTableRegionD [] =
{
   /* Str                 Next Tbl Ptr   Handler Func.       Data Type         Access   Parameter                                                IndexRange        DataLimit    EnumTbl*/
   { "SEG0_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[3].segment[0].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[3].segment[0].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[3].segment[0].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[3].segment[0].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG1_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[3].segment[1].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[3].segment[1].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[3].segment[1].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[3].segment[1].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG2_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[3].segment[2].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[3].segment[2].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[3].segment[2].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[3].segment[2].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG3_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[3].segment[3].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[3].segment[3].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[3].segment[3].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[3].segment[3].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG4_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[3].segment[4].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[3].segment[4].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[3].segment[4].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[3].segment[4].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG5_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[3].segment[5].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[3].segment[5].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[3].segment[5].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[3].segment[5].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "ACTION",            NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_HEX32,  USER_RW, &configEventTableTemp.region[3].nAction,                 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { NULL,             NULL,                      NULL,                  NO_HANDLER_DATA }
};

static USER_MSG_TBL eventTableRegionE [] =
{
   /* Str                 Next Tbl Ptr   Handler Func.       Data Type         Access   Parameter                                                IndexRange        DataLimit    EnumTbl*/
   { "SEG0_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[4].segment[0].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[4].segment[0].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[4].segment[0].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[4].segment[0].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG1_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[4].segment[1].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[4].segment[1].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[4].segment[1].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[4].segment[1].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG2_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[4].segment[2].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[4].segment[2].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[4].segment[2].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[4].segment[2].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG3_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[4].segment[3].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[4].segment[3].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[4].segment[3].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[4].segment[3].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG4_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[4].segment[4].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[4].segment[4].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[4].segment[4].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[4].segment[4].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG5_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[4].segment[5].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[4].segment[5].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[4].segment[5].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[4].segment[5].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "ACTION",            NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_HEX32,  USER_RW, &configEventTableTemp.region[4].nAction,                 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { NULL,             NULL,                      NULL,                  NO_HANDLER_DATA }
};

static USER_MSG_TBL eventTableRegionF [] =
{
   /* Str                 Next Tbl Ptr   Handler Func.       Data Type         Access   Parameter                                              IndexRange        DataLimit    EnumTbl*/
   { "SEG0_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[5].segment[0].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[5].segment[0].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[5].segment[0].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[5].segment[0].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG1_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[5].segment[1].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[5].segment[1].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[5].segment[1].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[5].segment[1].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG2_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[5].segment[2].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[5].segment[2].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[5].segment[2].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[5].segment[2].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG3_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[5].segment[3].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[5].segment[3].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[5].segment[3].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[5].segment[3].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG4_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[5].segment[4].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[5].segment[4].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[5].segment[4].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[5].segment[4].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG5_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[5].segment[5].fStartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[5].segment[5].nStartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &configEventTableTemp.region[5].segment[5].fStopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &configEventTableTemp.region[5].segment[5].nStopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "ACTION",            NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_HEX32,  USER_RW, &configEventTableTemp.region[5].nAction,                 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { NULL,                NULL,          NULL,               NO_HANDLER_DATA }
};

static USER_MSG_TBL eventTableCmd [] =
{
  /* Str              Next Tbl Ptr               Handler Func.          Data Type          Access     Parameter                                    IndexRange           DataLimit            EnumTbl*/
  { "SENSORINDEX",    NO_NEXT_TABLE,             EventTable_UserCfg,    USER_TYPE_ENUM,    USER_RW,   &configEventTableTemp.nSensor,               0,(MAX_TABLES-1),    NO_LIMIT,            SensorIndexType },
  { "MINSENSORVALUE", NO_NEXT_TABLE,             EventTable_UserCfg,    USER_TYPE_FLOAT,   USER_RW,   &configEventTableTemp.fTableEntryValue,      0,(MAX_TABLES-1),    NO_LIMIT,            NULL            },
  // Since PWC Engineering cannot decide on hysteresis being + the threshold, - the threshold, or both; added configuration for both
  { "HYSTERESISPOS",  NO_NEXT_TABLE,             EventTable_UserCfg,    USER_TYPE_FLOAT,   USER_RW,   &configEventTableTemp.fHysteresisPos,        0,(MAX_TABLES-1),    NO_LIMIT,            NULL            },
  { "HYSTERESISNEG",  NO_NEXT_TABLE,             EventTable_UserCfg,    USER_TYPE_FLOAT,   USER_RW,   &configEventTableTemp.fHysteresisNeg,        0,(MAX_TABLES-1),    NO_LIMIT,            NULL            },
  { "TRANSIENT_MS",   NO_NEXT_TABLE,             EventTable_UserCfg,    USER_TYPE_UINT32,  USER_RW,   &configEventTableTemp.nTransientAllowance_ms,0,(MAX_TABLES-1),    NO_LIMIT,            NULL            },
  { "REGION_A",       eventTableRegionA,         NULL,                  NO_HANDLER_DATA },
  { "REGION_B",       eventTableRegionB,         NULL,                  NO_HANDLER_DATA },
  { "REGION_C",       eventTableRegionC,         NULL,                  NO_HANDLER_DATA },
  { "REGION_D",       eventTableRegionD,         NULL,                  NO_HANDLER_DATA },
  { "REGION_E",       eventTableRegionE,         NULL,                  NO_HANDLER_DATA },
  { "REGION_F",       eventTableRegionF,         NULL,                  NO_HANDLER_DATA },
  { NULL,             NULL,                      NULL,                  NO_HANDLER_DATA }
};

static USER_MSG_TBL eventTableStatus [] =
{
  /* Str                 Next Tbl Ptr    Handler Func.       Data Type          Access     Parameter                                                   IndexRange           DataLimit   EnumTbl*/
   { "STARTED",          NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_BOOLEAN, USER_RO,   &stateEventTableTemp.bStarted,                              0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
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
USER_MSG_TBL rootEventTableMsg = {"EVENTTABLE", eventTableRoot, NULL, NO_HANDLER_DATA};

#pragma ghs endnowarning

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
      sprintf(sLabel, "%s[%d]", sLabelStem, i);

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
      sprintf(sLabel, "%s[%d]", sLabelStem, i);

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

    strToBinResult = EvalExprStrToBin( str, (EVAL_EXPR*) Param.Ptr, MAX_EVENT_EXPR_OPRNDS );

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

/*************************************************************************
 *  MODIFICATIONS
 *    $History: EventUserTables.c $
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
 **************************************************************************/

