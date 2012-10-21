/******************************************************************************
Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
All Rights Reserved. Proprietary and Confidential.

File:          EngineRunUserTables.c

Description:

VERSION
$Revision: 16 $  $Date: 10/11/12 6:55p $

******************************************************************************/
#ifndef ENGINERUN_BODY
#error EngineRunUserTables.c should only be included by EngineRun.c
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

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/


/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/

static ENGRUN_CFG      m_CfgTemp;
static ENGRUN_DATA     m_DataTemp;
static SNSR_SUMMARY    m_SnsrSummary;
/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
USER_HANDLER_RESULT       EngRunUserCfg(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr);

USER_HANDLER_RESULT       EngRunState(USER_DATA_TYPE DataType,
                                      USER_MSG_PARAM Param,
                                      UINT32 Index,
                                      const void *SetPtr,
                                      void **GetPtr);

USER_HANDLER_RESULT       EngRunShowConfig(USER_DATA_TYPE DataType,
                                           USER_MSG_PARAM Param,
                                           UINT32 Index,
                                           const void *SetPtr,
                                           void **GetPtr);

USER_HANDLER_RESULT EngUserInfo(USER_DATA_TYPE DataType,
                                USER_MSG_PARAM Param,
                                UINT32 Index,
                                const void *SetPtr,
                                void **GetPtr);

USER_HANDLER_RESULT EngSPUserInfo(USER_DATA_TYPE DataType,
                                USER_MSG_PARAM Param,
                                UINT32 Index,
                                const void *SetPtr,
                                void **GetPtr);

/*****************************************************************************/
/* User command tables                                                       */
/*****************************************************************************/
//static USER_ENUM_TBL EngineRunStateEnum[] =
//{
//  { "STOPPED",  ER_STATE_STOPPED  },
//  { "STARTING", ER_STATE_STARTING },
//  { "RUNNING",  ER_STATE_RUNNING  },
//  { NULL, 0 }
//};

USER_ENUM_TBL EngRunIdEnum[] =
{
  { "0"     , ENGRUN_ID_0  },
  { "1"     , ENGRUN_ID_1  },
  { "2"     , ENGRUN_ID_2  },
  { "3"     , ENGRUN_ID_3  },
  { "ANY"   , ENGRUN_ID_ANY},
  { "UNUSED", ENGRUN_UNUSED},
  { NULL, 0 }
};

USER_ENUM_TBL EngRunRateType[] =
{ { "1HZ"    , ER_1HZ          },
  { "2HZ"    , ER_2HZ          },
  { "4HZ"    , ER_4HZ          },
  { "5HZ"    , ER_5HZ          },
  { "10HZ"   , ER_10HZ         },
  { "20HZ"   , ER_20HZ         },
  { "50HZ"   , ER_50HZ         },
  { NULL     , 0               }
};

#pragma ghs nowarning 1545 //Suppress packed structure alignment warning


/* SENSOR SUMMARY ELEMENTS 0 - 15 */
static USER_MSG_TBL EngRunS0Tbl[] =
{
  /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
  {"SENSORID",   NO_NEXT_TABLE, EngRunState,   USER_TYPE_ENUM,    USER_RO,  &m_DataTemp.snsrSummary[0].SensorIndex,      0,(MAX_ENGINES-1),    NO_LIMIT,     SensorIndexType },
  {"INITIALIZE", NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[0].bInitialized,     0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"VALID",      NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[0].bValid,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"MIN",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[0].fMinValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"MAX",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[0].fMaxValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"AVG",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[0].fAvgValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"TOT",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[0].fTotal,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  { NULL,        NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunS1Tbl[] =
{
  /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
  {"SENSORID",   NO_NEXT_TABLE, EngRunState,   USER_TYPE_ENUM,    USER_RO,  &m_DataTemp.snsrSummary[1].SensorIndex,      0,(MAX_ENGINES-1),    NO_LIMIT,     SensorIndexType },
  {"INITIALIZE", NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[1].bInitialized,     0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"VALID",      NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[1].bValid,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"MIN",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[1].fMinValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"MAX",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[1].fMaxValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"AVG",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[1].fAvgValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"TOT",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[1].fTotal,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  { NULL,          NULL,           NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunS2Tbl[] =
{
  /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
  {"SENSORID",   NO_NEXT_TABLE, EngRunState,   USER_TYPE_ENUM,    USER_RO,  &m_DataTemp.snsrSummary[2].SensorIndex,      0,(MAX_ENGINES-1),    NO_LIMIT,     SensorIndexType },
  {"INITIALIZE", NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[2].bInitialized,     0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"VALID",      NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[2].bValid,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"MIN",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[2].fMinValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"MAX",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[2].fMaxValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"AVG",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[2].fAvgValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"TOT",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[2].fTotal,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  { NULL,          NULL,           NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunS3Tbl[] =
{
  /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
  {"SENSORID",   NO_NEXT_TABLE, EngRunState,   USER_TYPE_ENUM,    USER_RO,  &m_DataTemp.snsrSummary[3].SensorIndex,      0,(MAX_ENGINES-1),    NO_LIMIT,     SensorIndexType },
  {"INITIALIZE", NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[3].bInitialized,     0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"VALID",      NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[3].bValid,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"MIN",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[3].fMinValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"MAX",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[3].fMaxValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"AVG",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[3].fAvgValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"TOT",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[3].fTotal,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  { NULL,          NULL,           NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunS4Tbl[] =
{
  /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
  {"SENSORID",   NO_NEXT_TABLE, EngRunState,   USER_TYPE_ENUM,    USER_RO,  &m_DataTemp.snsrSummary[4].SensorIndex,      0,(MAX_ENGINES-1),    NO_LIMIT,     SensorIndexType },
  {"INITIALIZE", NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[4].bInitialized,     0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"VALID",      NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[4].bValid,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"MIN",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[4].fMinValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"MAX",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[4].fMaxValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"AVG",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[4].fAvgValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"TOT",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[4].fTotal,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  { NULL,          NULL,           NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunS5Tbl[] =
{
  /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
  {"SENSORID",   NO_NEXT_TABLE, EngRunState,   USER_TYPE_ENUM,    USER_RO,  &m_DataTemp.snsrSummary[5].SensorIndex,      0,(MAX_ENGINES-1),    NO_LIMIT,     SensorIndexType },
  {"INITIALIZE", NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[5].bInitialized,     0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"VALID",      NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[5].bValid,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"MIN",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[5].fMinValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"MAX",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[5].fMaxValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"AVG",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[5].fAvgValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"TOT",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[5].fTotal,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  { NULL,          NULL,           NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunS6Tbl[] =
{
  /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
  {"SENSORID",   NO_NEXT_TABLE, EngRunState,   USER_TYPE_ENUM,    USER_RO,  &m_DataTemp.snsrSummary[6].SensorIndex,      0,(MAX_ENGINES-1),    NO_LIMIT,     SensorIndexType },
  {"INITIALIZE", NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[6].bInitialized,     0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"VALID",      NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[6].bValid,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"MIN",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[6].fMinValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"MAX",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[6].fMaxValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"AVG",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[6].fAvgValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"TOT",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[6].fTotal,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  { NULL,          NULL,           NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunS7Tbl[] =
{
  /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
  {"SENSORID",   NO_NEXT_TABLE, EngRunState,   USER_TYPE_ENUM,    USER_RO,  &m_DataTemp.snsrSummary[7].SensorIndex,      0,(MAX_ENGINES-1),    NO_LIMIT,     SensorIndexType },
  {"INITIALIZE", NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[7].bInitialized,     0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"VALID",      NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[7].bValid,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"MIN",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[7].fMinValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"MAX",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[7].fMaxValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"AVG",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[7].fAvgValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"TOT",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[7].fTotal,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  { NULL,          NULL,           NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunS8Tbl[] =
{
  /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
  {"SENSORID",   NO_NEXT_TABLE, EngRunState,   USER_TYPE_ENUM,    USER_RO,  &m_DataTemp.snsrSummary[8].SensorIndex,      0,(MAX_ENGINES-1),    NO_LIMIT,     SensorIndexType },
  {"INITIALIZE", NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[8].bInitialized,     0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"VALID",      NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[8].bValid,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"MIN",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[8].fMinValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"MAX",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[8].fMaxValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"AVG",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[8].fAvgValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"TOT",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[8].fTotal,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  { NULL,          NULL,           NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunS9Tbl[] =
{
  /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
  {"SENSORID",   NO_NEXT_TABLE, EngRunState,   USER_TYPE_ENUM,    USER_RO,  &m_DataTemp.snsrSummary[9].SensorIndex,      0,(MAX_ENGINES-1),    NO_LIMIT,     SensorIndexType },
  {"INITIALIZE", NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[9].bInitialized,     0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"VALID",      NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[9].bValid,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"MIN",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[9].fMinValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"MAX",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[9].fMaxValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"AVG",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[9].fAvgValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"TOT",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[9].fTotal,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  { NULL,          NULL,           NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunS10Tbl[] =
{
  /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
  {"SENSORID",   NO_NEXT_TABLE, EngRunState,   USER_TYPE_ENUM,    USER_RO,  &m_DataTemp.snsrSummary[10].SensorIndex,      0,(MAX_ENGINES-1),    NO_LIMIT,     SensorIndexType },
  {"INITIALIZE", NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[10].bInitialized,     0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"VALID",      NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[10].bValid,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"MIN",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[10].fMinValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"MAX",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[10].fMaxValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"AVG",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[10].fAvgValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"TOT",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[10].fTotal,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  { NULL,          NULL,           NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunS11Tbl[] =
{
  /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
  {"SENSORID",   NO_NEXT_TABLE, EngRunState,   USER_TYPE_ENUM,    USER_RO,  &m_DataTemp.snsrSummary[11].SensorIndex,      0,(MAX_ENGINES-1),    NO_LIMIT,     SensorIndexType },
  {"INITIALIZE", NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[11].bInitialized,     0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"VALID",      NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[11].bValid,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"MIN",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[11].fMinValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"MAX",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[11].fMaxValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"AVG",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[11].fAvgValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"TOT",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[11].fTotal,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  { NULL,          NULL,           NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunS12Tbl[] =
{
  /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
  {"SENSORID",   NO_NEXT_TABLE, EngRunState,   USER_TYPE_ENUM,    USER_RO,  &m_DataTemp.snsrSummary[12].SensorIndex,      0,(MAX_ENGINES-1),    NO_LIMIT,     SensorIndexType },
  {"INITIALIZE", NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[12].bInitialized,     0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"VALID",      NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[12].bValid,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"MIN",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[12].fMinValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"MAX",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[12].fMaxValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"AVG",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[12].fAvgValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"TOT",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[12].fTotal,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  { NULL,          NULL,           NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunS13Tbl[] =
{
  /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
  {"SENSORID",   NO_NEXT_TABLE, EngRunState,   USER_TYPE_ENUM,    USER_RO,  &m_DataTemp.snsrSummary[13].SensorIndex,      0,(MAX_ENGINES-1),    NO_LIMIT,     SensorIndexType },
  {"INITIALIZE", NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[13].bInitialized,     0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"VALID",      NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[13].bValid,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"MIN",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[13].fMinValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"MAX",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[13].fMaxValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"AVG",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[13].fAvgValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"TOT",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[13].fTotal,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  { NULL,          NULL,           NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunS14Tbl[] =
{
  /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
  {"SENSORID",   NO_NEXT_TABLE, EngRunState,   USER_TYPE_ENUM,    USER_RO,  &m_DataTemp.snsrSummary[14].SensorIndex,      0,(MAX_ENGINES-1),    NO_LIMIT,     SensorIndexType },
  {"INITIALIZE", NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[14].bInitialized,     0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"VALID",      NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[14].bValid,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"MIN",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[14].fMinValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"MAX",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[14].fMaxValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"AVG",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[14].fAvgValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"TOT",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[14].fTotal,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  { NULL,          NULL,           NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunS15Tbl[] =
{
  /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/
  {"SENSORID",   NO_NEXT_TABLE, EngRunState,   USER_TYPE_ENUM,    USER_RO,  &m_DataTemp.snsrSummary[15].SensorIndex,      0,(MAX_ENGINES-1),    NO_LIMIT,     SensorIndexType },
  {"INITIALIZE", NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[15].bInitialized,     0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"VALID",      NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[15].bValid,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"MIN",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[15].fMinValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"MAX",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[15].fMaxValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"AVG",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[15].fAvgValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  {"TOT",        NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[15].fTotal,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },
  { NULL,          NULL,           NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunSensrTbl[] =
{
  { "S0",   EngRunS0Tbl,   NULL, NO_HANDLER_DATA },
  { "S1",   EngRunS1Tbl,   NULL, NO_HANDLER_DATA },
  { "S2",   EngRunS2Tbl,   NULL, NO_HANDLER_DATA },
  { "S3",   EngRunS3Tbl,   NULL, NO_HANDLER_DATA },
  { "S4",   EngRunS4Tbl,   NULL, NO_HANDLER_DATA },
  { "S5",   EngRunS5Tbl,   NULL, NO_HANDLER_DATA },
  { "S6",   EngRunS6Tbl,   NULL, NO_HANDLER_DATA },
  { "S7",   EngRunS7Tbl,   NULL, NO_HANDLER_DATA },
  { "S8",   EngRunS8Tbl,   NULL, NO_HANDLER_DATA },
  { "S9",   EngRunS9Tbl,   NULL, NO_HANDLER_DATA },
  { "S10",  EngRunS10Tbl,  NULL, NO_HANDLER_DATA },
  { "S11",  EngRunS11Tbl,  NULL, NO_HANDLER_DATA },
  { "S12",  EngRunS12Tbl,  NULL, NO_HANDLER_DATA },
  { "S13",  EngRunS13Tbl,  NULL, NO_HANDLER_DATA },
  { "S14",  EngRunS14Tbl,  NULL, NO_HANDLER_DATA },
  { "S15",  EngRunS15Tbl,  NULL, NO_HANDLER_DATA },
  { NULL,   NULL,          NULL, NO_HANDLER_DATA }
};

/* CYCLE COUNT ELEMENTS 0 - 31 */

static USER_MSG_TBL EngRunC0Tbl[] =
{
  /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                   IndexRange            DataLimit     EnumTbl*/
  {"COUNT",  NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleCounts[0], 0,(MAX_ENGINES-1),    NO_LIMIT,     NULL     },
  { NULL,    NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunC1Tbl[] =
{
  /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                   IndexRange            DataLimit     EnumTbl*/
  {"COUNT",  NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleCounts[1], 0,(MAX_ENGINES-1),    NO_LIMIT,     NULL     },
  { NULL,    NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunC2Tbl[] =
{
  /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                   IndexRange            DataLimit     EnumTbl*/
  {"COUNT",  NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleCounts[2], 0,(MAX_ENGINES-1),    NO_LIMIT,     NULL     },
  { NULL,    NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunC3Tbl[] =
{
  /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                   IndexRange            DataLimit     EnumTbl*/
  {"COUNT",  NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleCounts[3], 0,(MAX_ENGINES-1),    NO_LIMIT,     NULL     },
  { NULL,    NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunC4Tbl[] =
{
  /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                          IndexRange            DataLimit     EnumTbl*/
  {"COUNT",  NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleCounts[4], 0,(MAX_ENGINES-1),    NO_LIMIT,     NULL     },
  { NULL,    NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunC5Tbl[] =
{
  /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                   IndexRange            DataLimit     EnumTbl*/
  {"COUNT",  NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleCounts[5], 0,(MAX_ENGINES-1),    NO_LIMIT,     NULL     },
  { NULL,    NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunC6Tbl[] =
{
  /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                   IndexRange            DataLimit     EnumTbl*/
  {"COUNT",  NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleCounts[6], 0,(MAX_ENGINES-1),    NO_LIMIT,     NULL     },
  { NULL,    NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunC7Tbl[] =
{
  /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                   IndexRange            DataLimit     EnumTbl*/
  {"COUNT",  NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleCounts[7], 0,(MAX_ENGINES-1),    NO_LIMIT,     NULL     },
  { NULL,    NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunC8Tbl[] =
{
  /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                   IndexRange            DataLimit     EnumTbl*/
  {"COUNT",  NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleCounts[8], 0,(MAX_ENGINES-1),    NO_LIMIT,     NULL     },
  { NULL,    NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunC9Tbl[] =
{
  /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                   IndexRange            DataLimit     EnumTbl*/
  {"COUNT",  NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleCounts[9], 0,(MAX_ENGINES-1),    NO_LIMIT,     NULL     },
  { NULL,    NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunC10Tbl[] =
{
  /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                    IndexRange            DataLimit     EnumTbl*/
  {"COUNT",  NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleCounts[10], 0,(MAX_ENGINES-1),    NO_LIMIT,     NULL     },
  { NULL,    NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunC11Tbl[] =
{
  /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                    IndexRange            DataLimit     EnumTbl*/
  {"COUNT",  NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleCounts[11], 0,(MAX_ENGINES-1),    NO_LIMIT,     NULL     },
  { NULL,    NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunC12Tbl[] =
{
  /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                    IndexRange            DataLimit     EnumTbl*/
  {"COUNT",  NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleCounts[12], 0,(MAX_ENGINES-1),    NO_LIMIT,     NULL     },
  { NULL,    NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunC13Tbl[] =
{
  /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                    IndexRange            DataLimit     EnumTbl*/
  {"COUNT",  NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleCounts[13], 0,(MAX_ENGINES-1),    NO_LIMIT,     NULL     },
  { NULL,    NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunC14Tbl[] =
{
  /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                    IndexRange            DataLimit     EnumTbl*/
  {"COUNT",  NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleCounts[14], 0,(MAX_ENGINES-1),    NO_LIMIT,     NULL     },
  { NULL,    NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunC15Tbl[] =
{
  /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                    IndexRange            DataLimit     EnumTbl*/
  {"COUNT",  NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleCounts[15], 0,(MAX_ENGINES-1),    NO_LIMIT,     NULL     },
  { NULL,    NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunC16Tbl[] =
{
  /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                    IndexRange            DataLimit     EnumTbl*/
  {"COUNT",  NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleCounts[16], 0,(MAX_ENGINES-1),    NO_LIMIT,     NULL     },
  { NULL,    NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunC17Tbl[] =
{
  /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                    IndexRange            DataLimit     EnumTbl*/
  {"COUNT",  NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleCounts[17], 0,(MAX_ENGINES-1),    NO_LIMIT,     NULL     },
  { NULL,    NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunC18Tbl[] =
{
  /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                    IndexRange            DataLimit     EnumTbl*/
  {"COUNT",  NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleCounts[18], 0,(MAX_ENGINES-1),    NO_LIMIT,     NULL     },
  { NULL,    NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunC19Tbl[] =
{
  /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                    IndexRange            DataLimit     EnumTbl*/
  {"COUNT",  NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleCounts[19], 0,(MAX_ENGINES-1),    NO_LIMIT,     NULL     },
  { NULL,    NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunC20Tbl[] =
{
  /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                    IndexRange            DataLimit     EnumTbl*/
  {"COUNT",  NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleCounts[20], 0,(MAX_ENGINES-1),    NO_LIMIT,     NULL     },
  { NULL,    NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunC21Tbl[] =
{
  /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                    IndexRange            DataLimit     EnumTbl*/
  {"COUNT",  NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleCounts[21], 0,(MAX_ENGINES-1),    NO_LIMIT,     NULL     },
  { NULL,    NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunC22Tbl[] =
{
  /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                    IndexRange            DataLimit     EnumTbl*/
  {"COUNT",  NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleCounts[22], 0,(MAX_ENGINES-1),    NO_LIMIT,     NULL     },
  { NULL,    NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunC23Tbl[] =
{
  /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                    IndexRange            DataLimit     EnumTbl*/
  {"COUNT",  NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleCounts[23], 0,(MAX_ENGINES-1),    NO_LIMIT,     NULL     },
  { NULL,    NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunC24Tbl[] =
{
  /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                    IndexRange            DataLimit     EnumTbl*/
  {"COUNT",  NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleCounts[24], 0,(MAX_ENGINES-1),    NO_LIMIT,     NULL     },
  { NULL,    NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunC25Tbl[] =
{
  /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                    IndexRange            DataLimit     EnumTbl*/
  {"COUNT",  NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleCounts[25], 0,(MAX_ENGINES-1),    NO_LIMIT,     NULL     },
  { NULL,    NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunC26Tbl[] =
{
  /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                    IndexRange            DataLimit     EnumTbl*/
  {"COUNT",  NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleCounts[26], 0,(MAX_ENGINES-1),    NO_LIMIT,     NULL     },
  { NULL,    NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunC27Tbl[] =
{
  /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                    IndexRange            DataLimit     EnumTbl*/
  {"COUNT",  NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleCounts[27], 0,(MAX_ENGINES-1),    NO_LIMIT,     NULL     },
  { NULL,    NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunC28Tbl[] =
{
  /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                    IndexRange            DataLimit     EnumTbl*/
  {"COUNT",  NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleCounts[28], 0,(MAX_ENGINES-1),    NO_LIMIT,     NULL     },
  { NULL,    NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunC29Tbl[] =
{
  /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                    IndexRange            DataLimit     EnumTbl*/
  {"COUNT",  NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleCounts[29], 0,(MAX_ENGINES-1),    NO_LIMIT,     NULL     },
  { NULL,    NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunC30Tbl[] =
{
  /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                    IndexRange            DataLimit     EnumTbl*/
  {"COUNT",  NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleCounts[30], 0,(MAX_ENGINES-1),    NO_LIMIT,     NULL     },
  { NULL,    NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunC31Tbl[] =
{
  /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                    IndexRange            DataLimit     EnumTbl*/
  {"COUNT",  NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleCounts[31], 0,(MAX_ENGINES-1),    NO_LIMIT,     NULL     },
  { NULL,    NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunCycTbl[] =
{
  { "C0",   EngRunC0Tbl,   NULL, NO_HANDLER_DATA },
  { "C1",   EngRunC1Tbl,   NULL, NO_HANDLER_DATA },
  { "C2",   EngRunC2Tbl,   NULL, NO_HANDLER_DATA },
  { "C3",   EngRunC3Tbl,   NULL, NO_HANDLER_DATA },
  { "C4",   EngRunC4Tbl,   NULL, NO_HANDLER_DATA },
  { "C5",   EngRunC5Tbl,   NULL, NO_HANDLER_DATA },
  { "C6",   EngRunC6Tbl,   NULL, NO_HANDLER_DATA },
  { "C7",   EngRunC7Tbl,   NULL, NO_HANDLER_DATA },
  { "C8",   EngRunC8Tbl,   NULL, NO_HANDLER_DATA },
  { "C9",   EngRunC9Tbl,   NULL, NO_HANDLER_DATA },
  { "C10",  EngRunC10Tbl,  NULL, NO_HANDLER_DATA },
  { "C11",  EngRunC11Tbl,  NULL, NO_HANDLER_DATA },
  { "C12",  EngRunC12Tbl,  NULL, NO_HANDLER_DATA },
  { "C13",  EngRunC13Tbl,  NULL, NO_HANDLER_DATA },
  { "C14",  EngRunC14Tbl,  NULL, NO_HANDLER_DATA },
  { "C15",  EngRunC15Tbl,  NULL, NO_HANDLER_DATA },
  { "C16",  EngRunC16Tbl,  NULL, NO_HANDLER_DATA },
  { "C17",  EngRunC17Tbl,  NULL, NO_HANDLER_DATA },
  { "C18",  EngRunC18Tbl,  NULL, NO_HANDLER_DATA },
  { "C19",  EngRunC19Tbl,  NULL, NO_HANDLER_DATA },
  { "C20",  EngRunC20Tbl,  NULL, NO_HANDLER_DATA },
  { "C21",  EngRunC21Tbl,  NULL, NO_HANDLER_DATA },
  { "C22",  EngRunC22Tbl,  NULL, NO_HANDLER_DATA },
  { "C23",  EngRunC23Tbl,  NULL, NO_HANDLER_DATA },
  { "C24",  EngRunC24Tbl,  NULL, NO_HANDLER_DATA },
  { "C25",  EngRunC25Tbl,  NULL, NO_HANDLER_DATA },
  { "C26",  EngRunC26Tbl,  NULL, NO_HANDLER_DATA },
  { "C27",  EngRunC27Tbl,  NULL, NO_HANDLER_DATA },
  { "C28",  EngRunC28Tbl,  NULL, NO_HANDLER_DATA },
  { "C29",  EngRunC29Tbl,  NULL, NO_HANDLER_DATA },
  { "C30",  EngRunC30Tbl,  NULL, NO_HANDLER_DATA },
  { "C31",  EngRunC31Tbl,  NULL, NO_HANDLER_DATA },
  { NULL,   NULL,          NULL, NO_HANDLER_DATA}
};





static USER_MSG_TBL EngRunStatusCmd [] =
{
  /*Str               Next Tbl Ptr    Handler Func Data Type         Access    Parameter                        IndexRange              DataLimit   EnumTbl*/
  {"ENGINEID",        NO_NEXT_TABLE,  EngRunState, USER_TYPE_ENUM,   USER_RO,  &m_DataTemp.erIndex,             0,(MAX_ENGINES-1),     NO_LIMIT,   EngRunIdEnum      },
  {"STATE",           NO_NEXT_TABLE,  EngRunState, USER_TYPE_ENUM,   USER_RO,  &m_DataTemp.erState,             0,(MAX_ENGINES-1),     NO_LIMIT,   EngineRunStateEnum},
  {"STARTINGTIME_MS", NO_NEXT_TABLE,  EngRunState, USER_TYPE_UINT32, USER_RO,  &m_DataTemp.startingTime_ms,     0,(MAX_ENGINES-1),     NO_LIMIT,   NULL              },
  {"START_DUR_MS",    NO_NEXT_TABLE,  EngRunState, USER_TYPE_UINT32, USER_RO,  &m_DataTemp.startingDuration_ms, 0,(MAX_ENGINES-1),     NO_LIMIT,   NULL              },
  {"RUN_DUR_MS",      NO_NEXT_TABLE,  EngRunState, USER_TYPE_UINT32, USER_RO,  &m_DataTemp.erDuration_ms,       0,(MAX_ENGINES-1),     NO_LIMIT,   NULL              },
  {"MINVALUE",        NO_NEXT_TABLE,  EngRunState, USER_TYPE_FLOAT,  USER_RO,  &m_DataTemp.monMinValue,         0,(MAX_ENGINES-1),     NO_LIMIT,   NULL              },
  {"MAXVALUE",        NO_NEXT_TABLE,  EngRunState, USER_TYPE_FLOAT, USER_RO,   &m_DataTemp.monMaxValue,         0,(MAX_ENGINES-1),     NO_LIMIT,   NULL              },
  {"SAMPLECOUNT",     NO_NEXT_TABLE,  EngRunState, USER_TYPE_UINT32, USER_RO,  &m_DataTemp.nSampleCount,        0,(MAX_ENGINES-1),     NO_LIMIT,   NULL              },
  {"SENSOR",          EngRunSensrTbl, NULL,        NO_HANDLER_DATA,                                                                                                  },
  {"CYCLE",           EngRunCycTbl,   NULL,        NO_HANDLER_DATA,                                                                                                  },
  { NULL,             NULL,           NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunCfgCmd[] =
{
  /*Str              Next Tbl Ptr   Handler Func     Data Type        Access    Parameter                  IndexRange         DataLimit             EnumTbl*/
  {"NAME",           NO_NEXT_TABLE, EngRunUserCfg, USER_TYPE_STR,     USER_RW,  &m_CfgTemp.engineName,     0,(MAX_ENGINES-1), 0,MAX_ENGINERUN_NAME, NULL            },
  {"STARTTRIGID",    NO_NEXT_TABLE, EngRunUserCfg, USER_TYPE_ENUM,    USER_RW,  &m_CfgTemp.startTrigID,    0,(MAX_ENGINES-1), NO_LIMIT,             TriggerIndexType},
  {"RUNTRIGID",      NO_NEXT_TABLE, EngRunUserCfg, USER_TYPE_ENUM,    USER_RW,  &m_CfgTemp.runTrigID,      0,(MAX_ENGINES-1), NO_LIMIT,             TriggerIndexType},
  {"STOPTRIGID",     NO_NEXT_TABLE, EngRunUserCfg, USER_TYPE_ENUM,    USER_RW,  &m_CfgTemp.stopTrigID,     0,(MAX_ENGINES-1), NO_LIMIT,             TriggerIndexType},
  {"RATE",           NO_NEXT_TABLE, EngRunUserCfg, USER_TYPE_ENUM,    USER_RW,  &m_CfgTemp.erRate,         0,(MAX_ENGINES-1), NO_LIMIT,             EngRunRateType  },
  {"RATEOFFSET_MS",  NO_NEXT_TABLE, EngRunUserCfg, USER_TYPE_UINT32,  USER_RW,  &m_CfgTemp.nOffset_ms,     0,(MAX_ENGINES-1), NO_LIMIT,             NULL            },
  {"MAXSENSORID",    NO_NEXT_TABLE, EngRunUserCfg, USER_TYPE_ENUM,    USER_RW,  &m_CfgTemp.monMaxSensorID, 0,(MAX_ENGINES-1), NO_LIMIT,             SensorIndexType },
  {"MINSENSORID",    NO_NEXT_TABLE, EngRunUserCfg, USER_TYPE_ENUM,    USER_RW,  &m_CfgTemp.monMinSensorID, 0,(MAX_ENGINES-1), NO_LIMIT,             SensorIndexType },
  {"SENSORS",        NO_NEXT_TABLE, EngRunUserCfg, USER_TYPE_SNS_LIST,USER_RW,  &m_CfgTemp.sensorMap,      0,(MAX_ENGINES-1), 0,MAX_ENGRUN_SENSORS, NULL            },
  { NULL,            NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL EngIdCmd[] =
{
   /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                            IndexRange         DataLimit            EnumTbl*/
   {"SN",     NO_NEXT_TABLE, EngUserInfo,   USER_TYPE_STR,     USER_RW,  m_EngineInfo.engine[0].serialNumber, 0,(MAX_ENGINES-1), 0,MAX_ENGINE_ID,     NULL            },
   {"MODEL",  NO_NEXT_TABLE, EngUserInfo,   USER_TYPE_STR,     USER_RW,  m_EngineInfo.engine[0].modelNumber,  0,(MAX_ENGINES-1), 0,MAX_ENGINE_ID,     NULL            },
   {NULL,     NULL,          NULL,          NO_HANDLER_DATA}
};



static USER_MSG_TBL EngInfoCmd[] =
{
   /*Str             Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                    IndexRange         DataLimit             EnumTbl       */
   {"SERVICE_PLAN",  NO_NEXT_TABLE, EngSPUserInfo, USER_TYPE_STR,     USER_RW,  &m_EngineInfo.servicePlan,   -1,-1,             0,MAX_ENGINE_ID,      NULL          },
   {"ENGINE",        EngIdCmd,      NULL,          NO_HANDLER_DATA,                                                                                                 },
   {NULL,            NULL,          NULL,          NO_HANDLER_DATA}
};

static USER_MSG_TBL EngRunCmd [] =
{
  /*Str             Next Tbl Ptr      Handler Func      Data Type             Access                         Parameter   IndexRange  DataLimit   EnumTbl*/
  {"CFG",           EngRunCfgCmd,     NULL,             NO_HANDLER_DATA,                                                                                },
  {"STATUS",        EngRunStatusCmd,  NULL,             NO_HANDLER_DATA,                                                                                },
  {"INFO",          EngInfoCmd,       NULL,             NO_HANDLER_DATA,                                                                                },
  {DISPLAY_CFG,     NO_NEXT_TABLE,    EngRunShowConfig, USER_TYPE_ACTION,    (USER_RO|USER_NO_LOG|USER_GSE)  ,NULL,      -1,-1,      NO_LIMIT,   NULL   },
  { NULL,           NULL,             NULL,             NO_HANDLER_DATA}
};
#pragma ghs endnowarning

static USER_MSG_TBL rootEngRunMsg = {"ENG",EngRunCmd, NULL,NO_HANDLER_DATA};


/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:     EngRunUserCfg
 *
 * Description:  User message handler to set/get configuration items of the
 *               EngineRun object
 *
 * Parameters:   See user.h command handler prototype
 *
 * Returns:      USER_RESULT_OK:    Processed successfully
 *               USER_RESULT_ERROR: Error processing command.
 *
 * Notes:        none
 *
 *****************************************************************************/
USER_HANDLER_RESULT EngRunUserCfg(USER_DATA_TYPE DataType,
                                  USER_MSG_PARAM Param,
                                  UINT32 Index,
                                  const void *SetPtr,
                                  void **GetPtr)
{
 USER_HANDLER_RESULT result = USER_RESULT_OK;

  memcpy(&m_CfgTemp,
         &CfgMgr_ConfigPtr()->EngineRunConfigs[Index],
         sizeof(m_CfgTemp));

  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    // Copy the updated temp  back to the shadow ram store.
    memcpy(&CfgMgr_ConfigPtr()->EngineRunConfigs[Index],
           &m_CfgTemp,
           sizeof(m_CfgTemp));

    // Persist the CfgMgr's updated copy back to EEPROM.
    CfgMgr_StoreConfigItem( CfgMgr_ConfigPtr(),
                            &CfgMgr_ConfigPtr()->EngineRunConfigs[Index],
                            sizeof(m_CfgTemp) );
  }
  return result;
}

/******************************************************************************
 * Function:     EngUserInfo
 *
 * Description:  User message handler to set/get engine Identification items
 *
 * Parameters:   See user.h command handler prototype
 *
 * Returns:      USER_RESULT_OK:    Processed successfully
 *               USER_RESULT_ERROR: Error processing command.
 *
 * Notes:        none
 *
 *****************************************************************************/
USER_HANDLER_RESULT EngUserInfo(USER_DATA_TYPE DataType,
                                  USER_MSG_PARAM Param,
                                  UINT32 Index,
                                  const void *SetPtr,
                                  void **GetPtr)
{
   //Offsets of each member of .Board is relative to Box_Info.Board[0].
   //Offset by the user's entered index to get the .Board[Index]
   Param.Ptr = (char*)Param.Ptr + sizeof(m_EngineInfo.engine[0]) * Index;

   if(SetPtr == NULL)
   {
      //Read into the global structure from NV memory, Return the parameter
      //to user manager.
      NV_Read(NV_ENGINE_ID,0,&m_EngineInfo,sizeof(m_EngineInfo));
      *GetPtr = Param.Ptr;
   }
   else
   {
      //Copy the new data into the global struct and write it to NV memory
      //Offsets of each member of .Board is relative to Box_Info.Board[0].
      //Offset by the user's entered index to get the .Board[Index]
      strncpy_safe(Param.Ptr,MAX_ENGINE_ID, SetPtr, _TRUNCATE );
      NV_Write(NV_ENGINE_ID,0,&m_EngineInfo,sizeof(m_EngineInfo));
   }
   return USER_RESULT_OK;
}

/******************************************************************************
 * Function:     EngSPUserInfo
 *
 * Description:  User message handler to set/get engine Service Plan
 *
 * Parameters:   See user.h command handler prototype
 *
 * Returns:      USER_RESULT_OK:    Processed successfully
 *               USER_RESULT_ERROR: Error processing command.
 *
 * Notes:        none
 *
 *****************************************************************************/
USER_HANDLER_RESULT EngSPUserInfo(USER_DATA_TYPE DataType,
                                  USER_MSG_PARAM Param,
                                  UINT32 Index,
                                  const void *SetPtr,
                                  void **GetPtr)
{
  if(SetPtr == NULL)
  {
    //Read into the global structure from NV memory, Return the parameter
    //to user manager.
    NV_Read(NV_ENGINE_ID,0,&m_EngineInfo,sizeof(m_EngineInfo));
    *GetPtr = Param.Ptr;
  }
  else
  {
    //Copy the new data into the global struct and write it to NV memory
    strncpy_safe(Param.Ptr,MAX_ENGINE_ID, SetPtr, _TRUNCATE );
    NV_Write(NV_BOX_CFG,0,&m_EngineInfo,sizeof(m_EngineInfo));
  }

  return USER_RESULT_OK;
}

/******************************************************************************
* Function:     EngRunState
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

USER_HANDLER_RESULT EngRunState(USER_DATA_TYPE DataType,
                                  USER_MSG_PARAM Param,
                                  UINT32 Index,
                                  const void *SetPtr,
                                  void **GetPtr)
{
   USER_HANDLER_RESULT result;

   result = USER_RESULT_ERROR;

   memcpy(&m_DataTemp, &engineRunData[Index], sizeof(m_DataTemp));

   result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

   return result;
}

/******************************************************************************
 * Function:    EngRunShowConfig
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
USER_HANDLER_RESULT EngRunShowConfig(USER_DATA_TYPE DataType,
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
  INT16 engIdx;

  result = USER_RESULT_OK;

  // Loop for each sensor.
  for (engIdx = 0; engIdx < MAX_ENGINES && result == USER_RESULT_OK; ++engIdx)
  {
    // Display sensor id info above each set of data.
    sprintf(Label, "\r\n\r\nENG[%d].CFG", engIdx);
    result = USER_RESULT_ERROR;
    if (User_OutputMsgString( Label, FALSE ) )
    {
      pCfgTable = EngRunCfgCmd;  // Re-set the pointer to beginning of CFG table

      // User_DisplayConfigTree will invoke itself recursively to display all fields.
      result = User_DisplayConfigTree(BranchName, pCfgTable, engIdx, 0, NULL);
    }
  }
  return result;
}
/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/*************************************************************************
*  MODIFICATIONS
*    $History: EngineRunUserTables.c $
 * 
 * *****************  Version 16  *****************
 * User: Contractor V&v Date: 10/11/12   Time: 6:55p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 Review Findings
 * 
 * *****************  Version 15  *****************
 * User: Contractor V&v Date: 12-10-02   Time: 1:18p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 Coding standard compliance
 *
 * *****************  Version 14  *****************
 * User: John Omalley Date: 12-09-27   Time: 9:13a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Added Engine Identification Fields to software and file
 * header
 * 
 * *****************  Version 13  *****************
 * User: Jeff Vahue   Date: 9/17/12    Time: 10:53a
 * Updated in $/software/control processor/code/application
 * SCR# 1107 - Add ACT_LIST, clean up msgs
 *
 * *****************  Version 12  *****************
 * User: Contractor V&v Date: 9/14/12    Time: 4:03p
 * Updated in $/software/control processor/code/application
 * FAST 2 fixes for sensor list
 *
 * *****************  Version 11  *****************
 * User: Jeff Vahue   Date: 9/07/12    Time: 4:05p
 * Updated in $/software/control processor/code/application
 * SCR# 1107 - V&V fixes, code review updates
 *
 * *****************  Version 10  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 12:43p
 * Updated in $/software/control processor/code/application
 * SCR# 1142
 *
 * *****************  Version 9  *****************
 * User: Contractor V&v Date: 8/15/12    Time: 7:17p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 BITARRAY128 input as integer list
 *
 * *****************  Version 8  *****************
 * User: Contractor V&v Date: 8/08/12    Time: 3:41p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 make min/max generic
 *
 * *****************  Version 7  *****************
 * User: Contractor V&v Date: 7/18/12    Time: 6:24p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2
 *
 * *****************  Version 6  *****************
 * User: Contractor V&v Date: 6/25/12    Time: 2:28p
 * Updated in $/software/control processor/code/application
 *
 * *****************  Version 5  *****************
 * User: Contractor V&v Date: 6/18/12    Time: 4:27p
 * Updated in $/software/control processor/code/application
 *
 * *****************  Version 4  *****************
 * User: Contractor V&v Date: 6/05/12    Time: 6:41p
 * Updated in $/software/control processor/code/application
 * Engine ID change
 *
 * *****************  Version 3  *****************
 * User: Contractor V&v Date: 5/24/12    Time: 3:05p
 * Updated in $/software/control processor/code/application
 * FAST2 Refactoring
 *
 * *****************  Version 2  *****************
 * User: John Omalley Date: 12-05-22   Time: 2:15p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Check In for Dave
 *
 * *****************  Version 1  *****************
 * User: Contractor V&v Date: 5/10/12    Time: 6:39p
 * Created in $/software/control processor/code/application
 * Add interface and usertables for enginerun object
*
****************************************************************************/
