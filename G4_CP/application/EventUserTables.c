/******************************************************************************
         Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
            All Rights Reserved. Proprietary and Confidential.

  File:        EventUserTables.c

Description:   User command structures and functions for the event processing

VERSION
$Revision: 10 $  $Date: 12-07-17 11:26a $
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
static EVENT_CFG         ConfigEventTemp;       // Event Configuration temp Storage
static EVENT_TABLE_CFG   ConfigEventTableTemp;  // Event Table temp Storage

static EVENT_DATA        StateEventTemp;        // Event State Temp Storage
static EVENT_TABLE_DATA  StateEventTableTemp;   // Event Table State Temp Storage

USER_ENUM_TBL EventTableType[]   =
{  { "0"  , EVENT_TABLE_0   }, { "1"  ,  EVENT_TABLE_1   }, {  "2"  , EVENT_TABLE_2   },
   { "3"  , EVENT_TABLE_3   }, { "4"  ,  EVENT_TABLE_4   }, {  "5"  , EVENT_TABLE_5   },
   { "6"  , EVENT_TABLE_6   }, { "7"  ,  EVENT_TABLE_7   }, {  "8"  , EVENT_TABLE_8   },
   { "9"  , EVENT_TABLE_9   }, { "UNUSED", EVENT_TABLE_UNUSED }, { NULL, 0}
};


USER_ENUM_TBL EventRateType[]   =  { { "1HZ"      , EV_1HZ          },
                                     { "2HZ"      , EV_2HZ          },
                                     { "4HZ"      , EV_4HZ          },
                                     { "5HZ"      , EV_5HZ          },
                                     { "10HZ"     , EV_10HZ         },
                                     { "20HZ"     , EV_20HZ         },
                                     { "50HZ"     , EV_50HZ         },
                                     { NULL       , 0               }
                                   };

USER_ENUM_TBL EventStateEnum[]  =  { {"NOT_USED"  , EVENT_NONE      },
                                     {"NOT_ACTIVE", EVENT_START     },
                                     {"ACTIVE"    , EVENT_ACTIVE    },
                                     {NULL        , 0               }
                                   };

#pragma ghs nowarning 1545 //Suppress packed structure alignment warning

// Events - EVENT User and Configuration Table
static USER_MSG_TBL EventCmd [] =
{
  /* Str              Next Tbl Ptr               Handler Func.          Data Type          Access     Parameter                           IndexRange           DataLimit            EnumTbl*/
  { "NAME",           NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_STR,     USER_RW,   &ConfigEventTemp.EventName,         0,(MAX_EVENTS-1),    0,MAX_EVENT_NAME,    NULL                },
  { "ID",             NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_STR,     USER_RW,   &ConfigEventTemp.EventID,           0,(MAX_EVENTS-1),    0,MAX_EVENT_ID,      NULL                },
  { "RATE",           NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &ConfigEventTemp.Rate,              0,(MAX_EVENTS-1),    NO_LIMIT,            EventRateType       },
  { "RATEOFFSET_MS",  NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_UINT32,  USER_RW,   &ConfigEventTemp.nOffset_ms,        0,(MAX_EVENTS-1),    0,1000,              NULL                },
  { "SC",             NO_NEXT_TABLE,            Event_CfgExprStrCmd,    USER_TYPE_STR,     USER_RW,   &ConfigEventTemp.StartExpr,         0,(MAX_EVENTS-1),    NO_LIMIT,            NULL                },
  { "EC",             NO_NEXT_TABLE,            Event_CfgExprStrCmd,    USER_TYPE_STR,     USER_RW,   &ConfigEventTemp.EndExpr,           0,(MAX_EVENTS-1),    NO_LIMIT,            NULL                },
  { "DURATION_MS",    NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_UINT32,  USER_RW,   &ConfigEventTemp.nMinDuration_ms,   0,(MAX_EVENTS-1),    NO_LIMIT,            NULL                },
  { "ACTION",         NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_UINT32,  USER_RW,   &ConfigEventTemp.Action,            0,(MAX_EVENTS-1),    NO_LIMIT,            NULL                },
  { "PRETIMEHISTORY", NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &ConfigEventTemp.PreTimeHistory,    0,(MAX_EVENTS-1),    NO_LIMIT,            TH_UserEnumType     },
  { "PRETIME_S",      NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_UINT16,  USER_RW,   &ConfigEventTemp.PreTime_s,         0,(MAX_EVENTS-1),    NO_LIMIT,            NULL                },
  { "POSTTIMEHISTORY",NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &ConfigEventTemp.PostTimeHistory,   0,(MAX_EVENTS-1),    NO_LIMIT,            TH_UserEnumType     },
  { "POSTTIME_S",     NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_UINT16,  USER_RW,   &ConfigEventTemp.PostTime_s,        0,(MAX_EVENTS-1),    NO_LIMIT,            NULL                },
  { "LOG_PRIORITY",   NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &ConfigEventTemp.Priority,          0,(MAX_EVENTS-1),    NO_LIMIT,            LM_UserEnumPriority },
  { "EVENT_TABLE",    NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &ConfigEventTemp.EventTableIndex,   0,(MAX_EVENTS-1),    NO_LIMIT,            EventTableType      },
  { "SENSORS",        NO_NEXT_TABLE,            Event_UserCfg,          USER_TYPE_HEX128,  USER_RW,   &ConfigEventTemp.SensorMap,         0,(MAX_EVENTS-1),    HEX128_LIMIT,        NULL                },
  { NULL,             NULL,                     NULL,                   NO_HANDLER_DATA }
};


static USER_MSG_TBL EventStatus [] =
{
  /* Str             Next Tbl Ptr       Handler Func.    Data Type          Access     Parameter                           IndexRange           DataLimit   EnumTbl*/
   { "STATE",        NO_NEXT_TABLE,     Event_State,     USER_TYPE_ENUM,    USER_RO,  &StateEventTemp.State,               0,(MAX_EVENTS-1),    NO_LIMIT,   EventStateEnum  },
   { "STARTTIME_MS", NO_NEXT_TABLE,     Event_State,     USER_TYPE_UINT32,  USER_RO,  &StateEventTemp.nStartTime_ms,       0,(MAX_EVENTS-1),    NO_LIMIT,   NULL            },
   { "DURATION_MS",  NO_NEXT_TABLE,     Event_State,     USER_TYPE_UINT32,  USER_RO,  &StateEventTemp.nDuration_ms,        0,(MAX_EVENTS-1),    NO_LIMIT,   NULL            },
   { "SAMPLECOUNT",  NO_NEXT_TABLE,     Event_State,     USER_TYPE_UINT32,  USER_RO,  &StateEventTemp.nSampleCount,        0,(MAX_EVENTS-1),    NO_LIMIT,   NULL            },
   { NULL          , NULL,              NULL ,           NO_HANDLER_DATA }
};

static USER_MSG_TBL EventRoot [] =
{ /* Str            Next Tbl Ptr       Handler Func.        Data Type          Access            Parameter      IndexRange   DataLimit  EnumTbl*/
   { "CFG",         EventCmd     ,     NULL,                NO_HANDLER_DATA},
   { "STATUS",      EventStatus  ,     NULL,                NO_HANDLER_DATA},
   { DISPLAY_CFG,   NO_NEXT_TABLE,     Event_ShowConfig,    USER_TYPE_ACTION,  USER_RO|USER_GSE, NULL,          -1, -1,      NO_LIMIT,  NULL},
   { NULL,          NULL,              NULL,                NO_HANDLER_DATA}
};

static
USER_MSG_TBL    RootEventMsg = {"EVENT",EventRoot,NULL,NO_HANDLER_DATA};
//*********************************************************************************************
// Event Tables - EVENT Table User and Configuration Table
//*********************************************************************************************
static USER_MSG_TBL EventTableRegionA [] =
{
  /* Str                 Next Tbl Ptr   Handler Func.       Data Type         Access   Parameter                                              IndexRange        DataLimit    EnumTbl*/
  { "SEG0_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[0].Segment[0].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG0_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[0].Segment[0].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG0_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[0].Segment[0].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG0_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[0].Segment[0].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

  { "SEG1_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[0].Segment[1].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG1_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[0].Segment[1].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG1_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[0].Segment[1].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG1_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[0].Segment[1].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

  { "SEG2_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[0].Segment[2].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG2_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[0].Segment[2].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG2_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[0].Segment[2].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG2_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[0].Segment[2].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

  { "SEG3_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[0].Segment[3].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG3_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[0].Segment[3].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG3_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[0].Segment[3].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG3_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[0].Segment[3].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

  { "SEG4_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[0].Segment[4].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG4_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[0].Segment[4].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG4_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[0].Segment[4].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG4_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[0].Segment[4].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

  { "SEG5_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[0].Segment[5].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG5_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[0].Segment[5].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG5_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[0].Segment[5].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "SEG5_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[0].Segment[5].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { "ACTION",            NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_HEX32,  USER_RW, &ConfigEventTableTemp.Region[0].Action,                 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
  { NULL,             NULL,                      NULL,                  NO_HANDLER_DATA }
};

static USER_MSG_TBL EventTableRegionB [] =
{
   /* Str                 Next Tbl Ptr   Handler Func.       Data Type         Access   Parameter                                              IndexRange        DataLimit    EnumTbl*/
   { "SEG0_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[1].Segment[0].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[1].Segment[0].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[1].Segment[0].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[1].Segment[0].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG1_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[1].Segment[1].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[1].Segment[1].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[1].Segment[1].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[1].Segment[1].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG2_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[1].Segment[2].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[1].Segment[2].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[1].Segment[2].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[1].Segment[2].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG3_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[1].Segment[3].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[1].Segment[3].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[1].Segment[3].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[1].Segment[3].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG4_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[1].Segment[4].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[1].Segment[4].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[1].Segment[4].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[1].Segment[4].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG5_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[1].Segment[5].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[1].Segment[5].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[1].Segment[5].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[1].Segment[5].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "ACTION",            NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_HEX32,  USER_RW, &ConfigEventTableTemp.Region[1].Action,                 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { NULL,             NULL,                      NULL,                  NO_HANDLER_DATA }
};

static USER_MSG_TBL EventTableRegionC [] =
{
   /* Str                 Next Tbl Ptr   Handler Func.       Data Type         Access   Parameter                                              IndexRange        DataLimit    EnumTbl*/
   { "SEG0_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[2].Segment[0].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[2].Segment[0].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[2].Segment[0].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[2].Segment[0].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG1_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[2].Segment[1].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[2].Segment[1].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[2].Segment[1].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[2].Segment[1].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG2_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[2].Segment[2].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[2].Segment[2].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[2].Segment[2].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[2].Segment[2].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG3_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[2].Segment[3].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[2].Segment[3].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[2].Segment[3].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[2].Segment[3].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG4_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[2].Segment[4].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[2].Segment[4].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[2].Segment[4].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[2].Segment[4].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG5_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[2].Segment[5].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[2].Segment[5].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[2].Segment[5].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[2].Segment[5].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "ACTION",            NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_HEX32,  USER_RW, &ConfigEventTableTemp.Region[2].Action,                 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { NULL,             NULL,                      NULL,                  NO_HANDLER_DATA }
};

static USER_MSG_TBL EventTableRegionD [] =
{
   /* Str                 Next Tbl Ptr   Handler Func.       Data Type         Access   Parameter                                              IndexRange        DataLimit    EnumTbl*/
   { "SEG0_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[3].Segment[0].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[3].Segment[0].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[3].Segment[0].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[3].Segment[0].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG1_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[3].Segment[1].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[3].Segment[1].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[3].Segment[1].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[3].Segment[1].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG2_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[3].Segment[2].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[3].Segment[2].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[3].Segment[2].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[3].Segment[2].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG3_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[3].Segment[3].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[3].Segment[3].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[3].Segment[3].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[3].Segment[3].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG4_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[3].Segment[4].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[3].Segment[4].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[3].Segment[4].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[3].Segment[4].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG5_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[3].Segment[5].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[3].Segment[5].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[3].Segment[5].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[3].Segment[5].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "ACTION",            NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_HEX32,  USER_RW, &ConfigEventTableTemp.Region[3].Action,                 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { NULL,             NULL,                      NULL,                  NO_HANDLER_DATA }
};

static USER_MSG_TBL EventTableRegionE [] =
{
   /* Str                 Next Tbl Ptr   Handler Func.       Data Type         Access   Parameter                                              IndexRange        DataLimit    EnumTbl*/
   { "SEG0_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[4].Segment[0].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[4].Segment[0].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[4].Segment[0].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[4].Segment[0].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG1_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[4].Segment[1].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[4].Segment[1].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[4].Segment[1].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[4].Segment[1].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG2_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[4].Segment[2].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[4].Segment[2].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[4].Segment[2].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[4].Segment[2].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG3_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[4].Segment[3].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[4].Segment[3].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[4].Segment[3].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[4].Segment[3].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG4_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[4].Segment[4].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[4].Segment[4].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[4].Segment[4].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[4].Segment[4].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG5_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[4].Segment[5].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[4].Segment[5].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[4].Segment[5].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[4].Segment[5].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "ACTION",            NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_HEX32,  USER_RW, &ConfigEventTableTemp.Region[4].Action,                 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { NULL,             NULL,                      NULL,                  NO_HANDLER_DATA }
};

static USER_MSG_TBL EventTableRegionF [] =
{
   /* Str                 Next Tbl Ptr   Handler Func.       Data Type         Access   Parameter                                              IndexRange        DataLimit    EnumTbl*/
   { "SEG0_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[5].Segment[0].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[5].Segment[0].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[5].Segment[0].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG0_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[5].Segment[0].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG1_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[5].Segment[1].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[5].Segment[1].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[5].Segment[1].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG1_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[5].Segment[1].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG2_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[5].Segment[2].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[5].Segment[2].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[5].Segment[2].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG2_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[5].Segment[2].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG3_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[5].Segment[3].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[5].Segment[3].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[5].Segment[3].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG3_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[5].Segment[3].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG4_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[5].Segment[4].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[5].Segment[4].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[5].Segment[4].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG4_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[5].Segment[4].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "SEG5_START_VAL",    NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[5].Segment[5].StartValue,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_START_TIME_S", NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[5].Segment[5].StartTime_s, 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_STOP_VAL",     NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_FLOAT,  USER_RW, &ConfigEventTableTemp.Region[5].Segment[5].StopValue,   0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { "SEG5_STOP_TIME_S",  NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_UINT32, USER_RW, &ConfigEventTableTemp.Region[5].Segment[5].StopTime_s,  0,(MAX_TABLES-1), NO_LIMIT,    NULL       },

   { "ACTION",            NO_NEXT_TABLE, EventTable_UserCfg, USER_TYPE_HEX32,  USER_RW, &ConfigEventTableTemp.Region[5].Action,                 0,(MAX_TABLES-1), NO_LIMIT,    NULL       },
   { NULL,                NULL,          NULL,               NO_HANDLER_DATA }
};

static USER_MSG_TBL EventTableCmd [] =
{
  /* Str              Next Tbl Ptr               Handler Func.          Data Type          Access     Parameter                                    IndexRange           DataLimit            EnumTbl*/
  { "SENSORINDEX",    NO_NEXT_TABLE,             EventTable_UserCfg,    USER_TYPE_ENUM,    USER_RW,   &ConfigEventTableTemp.nSensor,               0,(MAX_TABLES-1),    NO_LIMIT,            SensorIndexType },
  { "MINSENSORVALUE", NO_NEXT_TABLE,             EventTable_UserCfg,    USER_TYPE_FLOAT,   USER_RW,   &ConfigEventTableTemp.TableEntryValue,       0,(MAX_TABLES-1),    NO_LIMIT,            NULL            },
  // Since PWC Engineering cannot decide on hysteresis being + the threshold, - the threshold, or both; added configuration for both
  { "HYSTERESISPOS",  NO_NEXT_TABLE,             EventTable_UserCfg,    USER_TYPE_FLOAT,   USER_RW,   &ConfigEventTableTemp.HysteresisPos,         0,(MAX_TABLES-1),    NO_LIMIT,            NULL            },
  { "HYSTERESISNEG",  NO_NEXT_TABLE,             EventTable_UserCfg,    USER_TYPE_FLOAT,   USER_RW,   &ConfigEventTableTemp.HysteresisNeg,         0,(MAX_TABLES-1),    NO_LIMIT,            NULL            },
  { "TRANSIENT_MS",   NO_NEXT_TABLE,             EventTable_UserCfg,    USER_TYPE_UINT32,  USER_RW,   &ConfigEventTableTemp.TransientAllowance_ms, 0,(MAX_TABLES-1),    NO_LIMIT,            NULL            },
  { "REGION_A",       EventTableRegionA,         NULL,                  NO_HANDLER_DATA },
  { "REGION_B",       EventTableRegionB,         NULL,                  NO_HANDLER_DATA },
  { "REGION_C",       EventTableRegionC,         NULL,                  NO_HANDLER_DATA },
  { "REGION_D",       EventTableRegionD,         NULL,                  NO_HANDLER_DATA },
  { "REGION_E",       EventTableRegionE,         NULL,                  NO_HANDLER_DATA },
  { "REGION_F",       EventTableRegionF,         NULL,                  NO_HANDLER_DATA },
  { NULL,             NULL,                      NULL,                  NO_HANDLER_DATA }
};

static USER_MSG_TBL EventTableStatus [] =
{
  /* Str                 Next Tbl Ptr    Handler Func.       Data Type          Access     Parameter                                                   IndexRange           DataLimit   EnumTbl*/
   { "STARTED",          NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_BOOLEAN, USER_RO,   &StateEventTableTemp.Started,                               0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "STARTTIME_MS",     NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &StateEventTableTemp.StartTime_ms,                          0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "CURRENTREGION",    NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_ENUM,    USER_RO,   &StateEventTableTemp.CurrentRegion,                         0,(MAX_TABLES-1),    NO_LIMIT,   EVT_Region_UserEnumType },
   { "CONFIRMEDREGION",  NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_ENUM,    USER_RO,   &StateEventTableTemp.ConfirmedRegion,                       0,(MAX_TABLES-1),    NO_LIMIT,   EVT_Region_UserEnumType },
   { "TOTALDURATION_MS", NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &StateEventTableTemp.TotalDuration_ms,                      0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "MAXREGIONENTERED", NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_ENUM,    USER_RO,   &StateEventTableTemp.MaximumRegionEntered,                  0,(MAX_TABLES-1),    NO_LIMIT,   EVT_Region_UserEnumType },
   { "MAXSENSORVALUE",   NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_FLOAT,   USER_RO,   &StateEventTableTemp.MaxSensorValue,                        0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "MAXSENSORTIME_MS", NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &StateEventTableTemp.MaxSensorElaspedTime_ms,               0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "CURRENTSENSORVAL", NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_FLOAT,   USER_RO,   &StateEventTableTemp.CurrentSensorValue,                    0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "MAXIMUMCFGREGION", NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_ENUM,    USER_RO,   &StateEventTableTemp.MaximumCfgRegion,                      0,(MAX_TABLES-1),    NO_LIMIT,   EVT_Region_UserEnumType },
   { "A_ENTEREDCOUNT",   NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &StateEventTableTemp.RegionStats[0].LogStats.EnteredCount,  0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "A_EXITCOUNT",      NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &StateEventTableTemp.RegionStats[0].LogStats.ExitCount,     0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "A_DURATION_MS",    NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &StateEventTableTemp.RegionStats[0].LogStats.Duration_ms,   0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "B_ENTEREDCOUNT",   NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &StateEventTableTemp.RegionStats[1].LogStats.EnteredCount,  0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "B_EXITCOUNT",      NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &StateEventTableTemp.RegionStats[1].LogStats.ExitCount,     0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "B_DURATION_MS",    NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &StateEventTableTemp.RegionStats[1].LogStats.Duration_ms,   0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "C_ENTEREDCOUNT",   NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &StateEventTableTemp.RegionStats[2].LogStats.EnteredCount,  0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "C_EXITCOUNT",      NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &StateEventTableTemp.RegionStats[2].LogStats.ExitCount,     0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "C_DURATION_MS",    NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &StateEventTableTemp.RegionStats[2].LogStats.Duration_ms,   0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "D_ENTEREDCOUNT",   NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &StateEventTableTemp.RegionStats[3].LogStats.EnteredCount,  0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "D_EXITCOUNT",      NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &StateEventTableTemp.RegionStats[3].LogStats.ExitCount,     0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "D_DURATION_MS",    NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &StateEventTableTemp.RegionStats[3].LogStats.Duration_ms,   0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "E_ENTEREDCOUNT",   NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &StateEventTableTemp.RegionStats[4].LogStats.EnteredCount,  0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "E_EXITCOUNT",      NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &StateEventTableTemp.RegionStats[4].LogStats.ExitCount,     0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "E_DURATION_MS",    NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &StateEventTableTemp.RegionStats[4].LogStats.Duration_ms,   0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "F_ENTEREDCOUNT",   NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &StateEventTableTemp.RegionStats[5].LogStats.EnteredCount,  0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "F_EXITCOUNT",      NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &StateEventTableTemp.RegionStats[5].LogStats.ExitCount,     0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { "F_DURATION_MS",    NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_UINT32,  USER_RO,   &StateEventTableTemp.RegionStats[5].LogStats.Duration_ms,   0,(MAX_TABLES-1),    NO_LIMIT,   NULL   },
   { NULL          ,     NO_NEXT_TABLE,  EventTable_State,   USER_TYPE_NONE,    USER_RW,   NULL,                                                       0,(MAX_TABLES-1),    NO_LIMIT,   NULL },
};


static USER_MSG_TBL EventTableRoot [] =
{ /* Str          Next Tbl Ptr      Handler Func.          Data Type          Access            Parameter      IndexRange   DataLimit  EnumTbl*/
   { "CFG",       EventTableCmd,    NULL,                  NO_HANDLER_DATA},
   { "STATUS",    EventTableStatus, NULL,                  NO_HANDLER_DATA},
   { DISPLAY_CFG, NO_NEXT_TABLE,    EventTable_ShowConfig, USER_TYPE_ACTION,  USER_RO|USER_GSE, NULL,          -1, -1,      NO_LIMIT,  NULL},
   { NULL,        NULL,             NULL,                  NO_HANDLER_DATA}
};

static
USER_MSG_TBL RootEventTableMsg = {"EVENTTABLE", EventTableRoot, NULL, NO_HANDLER_DATA};

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
   memcpy(&ConfigEventTemp,
          &CfgMgr_ConfigPtr()->EventConfigs[Index],
          sizeof(ConfigEventTemp));

   result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
   if(SetPtr != NULL && USER_RESULT_OK == result)
   {
     //memcpy(&m_EventCfg[Index],&ConfigEventTemp,sizeof(ConfigEventTemp));
     memcpy(&CfgMgr_ConfigPtr()->EventConfigs[Index],
       &ConfigEventTemp,
       sizeof(EVENT_CFG));
     //Store the modified temporary structure in the EEPROM.
     CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
       &CfgMgr_ConfigPtr()->EventConfigs[Index],
       sizeof(ConfigEventTemp));
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
   memcpy(&ConfigEventTableTemp,
      &CfgMgr_ConfigPtr()->EventTableConfigs[Index],
      sizeof(ConfigEventTableTemp));

   result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
   if(SetPtr != NULL && USER_RESULT_OK == result)
   {
     //memcpy(&m_EventCfg[Index],&ConfigEventTemp,sizeof(ConfigEventTemp));
     memcpy(&CfgMgr_ConfigPtr()->EventTableConfigs[Index],
       &ConfigEventTableTemp,
       sizeof(EVENT_TABLE_CFG));
     //Store the modified temporary structure in the EEPROM.
     CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
       &CfgMgr_ConfigPtr()->EventTableConfigs[Index],
       sizeof(ConfigEventTableTemp));
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

   memcpy(&StateEventTemp, &m_EventData[Index], sizeof(StateEventTemp));

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

   memcpy(&StateEventTableTemp, &m_EventTableData[Index], sizeof(StateEventTableTemp));

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
   CHAR  LabelStem[] = "\r\n\r\nEVENT.CFG";
   CHAR  Label[USER_MAX_MSG_STR_LEN * 3];
   INT16 i;

   USER_HANDLER_RESULT result = USER_RESULT_OK;
   USER_MSG_TBL*  pCfgTable;

   //Top-level name is a single indented space
   CHAR BranchName[USER_MAX_MSG_STR_LEN] = " ";

   pCfgTable = &EventCmd[0];  // Get pointer to config entry

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
   CHAR  LabelStem[] = "\r\n\r\nEVENTTABLE.CFG";
   CHAR  Label[USER_MAX_MSG_STR_LEN * 3];
   INT16 i;

   USER_HANDLER_RESULT result = USER_RESULT_OK;
   USER_MSG_TBL*  pCfgTable;

   //Top-level name is a single indented space
   CHAR BranchName[USER_MAX_MSG_STR_LEN] = " ";

   pCfgTable = &EventTableCmd[0];  // Get pointer to config entry

   for (i = 0; i < MAX_TABLES && result == USER_RESULT_OK; ++i)
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
  INT32 StrToBinResult;

  //User Mgr uses this obj outside of this function's scope
  static CHAR str[EVAL_MAX_EXPR_STR_LEN];


  // Move the current Event[x] config data into the local temp storage.
  // ( Parameter 'Param' is pointing to either the StartExpr or EndExpr struct
  // in the ConfigEventTemp; see EventCmd Usertable)

  memcpy(&ConfigEventTemp,
         &CfgMgr_ConfigPtr()->EventConfigs[Index],
         sizeof(ConfigEventTemp));

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

    StrToBinResult = EvalExprStrToBin( str, (EVAL_EXPR*) Param.Ptr, MAX_EVENT_EXPR_OPRNDS );

    if(StrToBinResult >= 0)
    {
      // Move the  successfully updated binary expression to the ConfigEventTemp storage
      memcpy(&CfgMgr_ConfigPtr()->EventConfigs[Index],
             &ConfigEventTemp,
             sizeof(ConfigEventTemp));

      //Store the modified temporary structure in the EEPROM.
      CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
                             &CfgMgr_ConfigPtr()->EventConfigs[Index],
                             sizeof(ConfigEventTemp) );
    }
    else
    {
      GSE_DebugStr(NORMAL,TRUE,"Event: Event_CfgExprStrCmd returned %d",StrToBinResult);
      result = USER_RESULT_ERROR;
    }
  }

  return result;
}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: EventUserTables.c $
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

