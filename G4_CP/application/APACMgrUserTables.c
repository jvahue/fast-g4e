#define APAC_MGR_USERTABLES_BODY
/******************************************************************************
            Copyright (C) 2015 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    ECCN:        9E991

    File:        APACMgrUserTables.c

    Description: Routines to support the user commands for APAC Mgr CSC

    VERSION
    $Revision: 11 $  $Date: 1/19/16 5:12p $

******************************************************************************/
#ifndef APAC_MGR_BODY
#error APACMgrUserTables.c should only be included by APACMgr.c
#endif

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/


/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "CfgManager.h"


/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
// #ifdef APAC_TEST_SIM
#define APAC_DBG_CMD_HIST_DATE      0
#define APAC_DBG_CMD_HIST_DAY       1
#define APAC_DBG_CMD_HIST_DATE_ADV  2
#define APAC_DBG_CMD_HIST_DATE_DEC  3
#define APAC_DBG_CMD_HIST_DAY_ADV   4
#define APAC_DBG_CMD_HIST_DAY_DEC   5
#define APAC_DBG_CMD_CONFIG         6
#define APAC_DBG_CMD_SELECT_100     7
#define APAC_DBG_CMD_SELECT_102     8
#define APAC_DBG_CMD_SETUP          9
#define APAC_DBG_CMD_VLD_MANUAL     10
#define APAC_DBG_CMD_RUN_PAC        11
#define APAC_DBG_CMD_PAC_COMPLETED  12
#define APAC_DBG_CMD_RESULT         13
#define APAC_DBG_CMD_RESULT_COMMIT_VLD    14
#define APAC_DBG_CMD_RESULT_COMMIT_INVLD  15
#define APAC_DBG_CMD_RESULT_COMMIT_ACPT   16
#define APAC_DBG_CMD_RESULT_COMMIT_RJCT   17
#define APAC_DBG_CMD_RESET_DCLK           18
#define APAC_DBG_CMD_RESET_TIMEOUT        19
#define APAC_DBG_CMD_ERR_MSG              20

#define AAPAC_DBG_CFG_BATCH_ON       0
#define AAPAC_DBG_CFG_BATCH_STORE    1
#define AAPAC_DBG_CFG_BATCH_CANCEL   2
// #endif

#define APAC_MAX_RECENT_DATE_TIME  20  // "HH:MM:SS MM/DD/YYYY"
#define APAC_MAX_RECENT_ENTRY_LEN 128


/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
static USER_HANDLER_RESULT APACMgr_Status(USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr);

static USER_HANDLER_RESULT APACMgr_EngStatus(USER_DATA_TYPE DataType,
                                             USER_MSG_PARAM Param,
                                             UINT32 Index,
                                             const void *SetPtr,
                                             void **GetPtr);

static USER_HANDLER_RESULT APACMgr_Cfg(USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr);

static USER_HANDLER_RESULT APACMgr_EngCfg(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr);

static USER_HANDLER_RESULT APACMgr_CfgExprStrCmd(USER_DATA_TYPE DataType,
                                                 USER_MSG_PARAM Param,
                                                 UINT32 Index,
                                                 const void *SetPtr,
                                                 void **GetPtr);
#ifdef APAC_TEST_SIM
static USER_HANDLER_RESULT APACMgr_DebugRestart(USER_DATA_TYPE DataType,
                                                USER_MSG_PARAM Param,
                                                UINT32 Index,
                                                const void *SetPtr,
                                                void **GetPtr);
#endif

// #ifdef APAC_TEST_SIM
static USER_HANDLER_RESULT APACMgr_DebugCmdHist(USER_DATA_TYPE DataType,
                                                USER_MSG_PARAM Param,
                                                UINT32 Index,
                                                const void *SetPtr,
                                                void **GetPtr);
// #endif

static USER_HANDLER_RESULT APACMgr_Debug(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr);

static USER_HANDLER_RESULT APACMgr_RecentAllHistory(USER_DATA_TYPE DataType,
                                                    USER_MSG_PARAM Param,
                                                    UINT32 Index,
                                                    const void *SetPtr,
                                                    void **GetPtr);

static USER_HANDLER_RESULT APACMgr_HistoryClear(USER_DATA_TYPE DataType,
                                                USER_MSG_PARAM Param,
                                                UINT32 Index,
                                                const void *SetPtr,
                                                void **GetPtr);
// #ifdef APAC_TEST_SIM
static USER_HANDLER_RESULT APACMgr_EngStatusDbg(USER_DATA_TYPE DataType,
                                                USER_MSG_PARAM Param,
                                                UINT32 Index,
                                                const void *SetPtr,
                                                void **GetPtr);

static USER_HANDLER_RESULT APACMgr_EngStatusDbgExe(USER_DATA_TYPE DataType,
                                                   USER_MSG_PARAM Param,
                                                   UINT32 Index,
                                                   const void *SetPtr,
                                                   void **GetPtr);

static USER_HANDLER_RESULT APACMgr_DebugCfgBatch(USER_DATA_TYPE DataType,
                                                 USER_MSG_PARAM Param,
                                                 UINT32 Index,
                                                 const void *SetPtr,
                                                 void **GetPtr);
// #endif

static USER_HANDLER_RESULT APACMgr_ShowConfig(USER_DATA_TYPE DataType,
                                              USER_MSG_PARAM Param,
                                              UINT32 Index,
                                              const void *SetPtr,
                                              void **GetPtr);

static CHAR *APACMgr_RecentAllHistoryEntry ( UINT8 index_obj );


/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static APAC_CFG cfgAPACTemp;
static APAC_ENG_CFG cfgAPAC_EngTemp;
static APAC_STATUS statusAPACTemp;
static APAC_ENG_STATUS statusAPAC_EngTemp;
static APAC_DEBUG debugAPACTemp;
static CHAR cfgAPAC_TrendSnsrTemp[APAC_SNSR_NAME_CHAR_MAX];

static APAC_INLET_CFG_ENUM statusAPACDbg_Inlet;
static APAC_NR_SEL_ENUM statusAPACDbg_NR_Sel;

static APAC_DATA_ENG_NVM nvmAPAC_EngTemp;


/*****************************************************************************/
/* Local Constants                                                           */
/*****************************************************************************/
static
USER_ENUM_TBL apacInletStrs[] =
{
  {"BASIC", INLET_CFG_BASIC},
  {"EAPS",  INLET_CFG_EAPS},
  {"IBF",   INLET_CFG_IBF},
  {NULL,          0}
};

static
USER_ENUM_TBL apacStateStrs[] =
{
  {"IDLE",            APAC_STATE_IDLE},
  {"TEST_INPROGRESS", APAC_STATE_TEST_INPROGRESS},
  {NULL,          0}
};

static
USER_ENUM_TBL apacRunStateStrs[] =
{
  {"IDLE",        APAC_RUN_STATE_IDLE},
  {"INCREASE_TQ", APAC_RUN_STATE_INCREASE_TQ},
  {"DECR_GND_SPD",APAC_RUN_STATE_DECR_GND_SPD},
  {"STABILIZING", APAC_RUN_STATE_STABILIZE},
  {"SAMPLING",    APAC_RUN_STATE_SAMPLING},
  {"COMPUTING",   APAC_RUN_STATE_COMPUTING},
  {"COMMIT",      APAC_RUN_STATE_COMMIT},
  {"FAULT",       APAC_RUN_STATE_FAULT},
  {NULL,          0}
};

static
USER_ENUM_TBL apacEngUUTStrs[] =
{
  {"ENG_1",       APAC_ENG_1},
  {"ENG_2",       APAC_ENG_2},
  {"NONE",        APAC_ENG_MAX},
  {NULL,          0}
};

static
USER_ENUM_TBL apacNrSelStrs[] =
{
  {"100",    APAC_NR_SEL_100},
  {"102",    APAC_NR_SEL_102},
  {"NONE",   APAC_NR_SEL_MAX},
  {NULL,     0}
};

static
USER_ENUM_TBL apacCommitStrs[] =
{
  {"RJCT",   APAC_COMMIT_RJCT},
  {"ACPT",   APAC_COMMIT_ACPT},
  {"INVLD",  APAC_COMMIT_INVLD},
  {"VLD",    APAC_COMMIT_VLD},
  {"ABORT",  APAC_COMMIT_ABORT},
  {"NCR",    APAC_COMMIT_NCR},
  {"NONE",   APAC_COMMIT_MAX},
  {NULL,          0}
};

static
USER_ENUM_TBL vldReasonStrs[] =
{
  {APAC_VLD_STR_NONE,  APAC_VLD_REASON_NONE},
  {APAC_VLD_STR_50HRS, APAC_VLD_REASON_50HRS},
  {APAC_VLD_STR_ESN,   APAC_VLD_REASON_ESN},
  {APAC_VLD_STR_CFG,   APAC_VLD_REASON_CFG},
  {APAC_VLD_STR_CYCLE, APAC_VLD_REASON_CYCLE},
  {APAC_VLD_STR_NVM,   APAC_VLD_REASON_NVM},
  {NULL,        0}
};

static
USER_ENUM_TBL apacIdxESNTypeStrs[] =
{
  { "0"     , ENGRUN_ID_0  },
  { "1"     , ENGRUN_ID_1  },
  { "2"     , ENGRUN_ID_2  },
  { "3"     , ENGRUN_ID_3  },
  { "UNUSED", ENGRUN_UNUSED},
  {NULL,   0}
};

static
USER_ENUM_TBL apacCfgStateStrs[] =
{
  {"DISABLED",  APAC_CFG_STATE_DSB },
  {"OK",        APAC_CFG_STATE_OK },
  {"FAULTED",   APAC_CFG_STATE_FAULT },
  {NULL,   0}
};



/*****************************************/
/* User Table Defintions                 */
/*****************************************/
static USER_MSG_TBL apacMgr_EngTbl[] =
{ /*Str             Next Tbl Ptr     Handler Func.          Data Type          Access    Parameter                                             IndexRange         DataLimit    EnumTbl*/
  {"ITT",           NO_NEXT_TABLE,   APACMgr_EngCfg,        USER_TYPE_ENUM,    USER_RW,  (void *) &cfgAPAC_EngTemp.idxITT,                     0,APAC_ENG_MAX-1,  NO_LIMIT,    SensorIndexType},
  {"NG",            NO_NEXT_TABLE,   APACMgr_EngCfg,        USER_TYPE_ENUM,    USER_RW,  (void *) &cfgAPAC_EngTemp.idxNg,                      0,APAC_ENG_MAX-1,  NO_LIMIT,    SensorIndexType},
  {"NR",            NO_NEXT_TABLE,   APACMgr_EngCfg,        USER_TYPE_ENUM,    USER_RW,  (void *) &cfgAPAC_EngTemp.idxNr,                      0,APAC_ENG_MAX-1,  NO_LIMIT,    SensorIndexType},
  {"TQ",            NO_NEXT_TABLE,   APACMgr_EngCfg,        USER_TYPE_ENUM,    USER_RW,  (void *) &cfgAPAC_EngTemp.idxTq,                      0,APAC_ENG_MAX-1,  NO_LIMIT,    SensorIndexType},
  {"OAT",           NO_NEXT_TABLE,   APACMgr_EngCfg,        USER_TYPE_ENUM,    USER_RW,  (void *) &cfgAPAC_EngTemp.idxOAT,                     0,APAC_ENG_MAX-1,  NO_LIMIT,    SensorIndexType},
  {"IDLE",          NO_NEXT_TABLE,   APACMgr_EngCfg,        USER_TYPE_ENUM,    USER_RW,  (void *) &cfgAPAC_EngTemp.idxEngIdle,                 0,APAC_ENG_MAX-1,  NO_LIMIT,    SensorIndexType},
  {"FLT",           NO_NEXT_TABLE,   APACMgr_EngCfg,        USER_TYPE_ENUM,    USER_RW,  (void *) &cfgAPAC_EngTemp.idxEngFlt,                  0,APAC_ENG_MAX-1,  NO_LIMIT,    SensorIndexType},
  {"EAPS_IBF",      NO_NEXT_TABLE,   APACMgr_EngCfg,        USER_TYPE_ENUM,    USER_RW,  (void *) &cfgAPAC_EngTemp.idxEAPS_IBF_sw,             0,APAC_ENG_MAX-1,  NO_LIMIT,    SensorIndexType},
  {"TREND_100",     NO_NEXT_TABLE,   APACMgr_EngCfg,        USER_TYPE_ENUM,    USER_RW,  (void *) &cfgAPAC_EngTemp.idxTrend[APAC_NR_SEL_100],  0,APAC_ENG_MAX-1,  NO_LIMIT,    trendIndexType },
  {"TREND_102",     NO_NEXT_TABLE,   APACMgr_EngCfg,        USER_TYPE_ENUM,    USER_RW,  (void *) &cfgAPAC_EngTemp.idxTrend[APAC_NR_SEL_102],  0,APAC_ENG_MAX-1,  NO_LIMIT,    trendIndexType },
  {"TQCRIT",        NO_NEXT_TABLE,   APACMgr_CfgExprStrCmd, USER_TYPE_STR,     USER_RW,  (void *) &cfgAPAC_EngTemp.sc_TqCrit,                  0,APAC_ENG_MAX-1,  NO_LIMIT,    NULL           },
  {"GNDCRIT",       NO_NEXT_TABLE,   APACMgr_CfgExprStrCmd, USER_TYPE_STR,     USER_RW,  (void *) &cfgAPAC_EngTemp.sc_GndCrit,                 0,APAC_ENG_MAX-1,  NO_LIMIT,    NULL           },
  {"ENG_CYC",       NO_NEXT_TABLE,   APACMgr_EngCfg,        USER_TYPE_ENUM,    USER_RW,  (void *) &cfgAPAC_EngTemp.idxCycleHRS,                0,APAC_ENG_MAX-1,  NO_LIMIT,    cycleEnumType  },
  {"ESN_ID",        NO_NEXT_TABLE,   APACMgr_EngCfg,        USER_TYPE_ENUM,    USER_RW,  (void *) &cfgAPAC_EngTemp.idxESN,                     0,APAC_ENG_MAX-1,  NO_LIMIT,    apacIdxESNTypeStrs  },
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL apacMgr_SnsrTbl[] =
{ /*Str         Next Tbl Ptr     Handler Func.    Data Type          Access    Parameter                                  IndexRange         DataLimit    EnumTbl*/
  {"BAROPRES",  NO_NEXT_TABLE,   APACMgr_Cfg,     USER_TYPE_ENUM,    USER_RW,  (void *) &cfgAPACTemp.snsr.idxBaroPres,    -1,-1,             NO_LIMIT,    SensorIndexType},
  {"BAROCORR",  NO_NEXT_TABLE,   APACMgr_Cfg,     USER_TYPE_ENUM,    USER_RW,  (void *) &cfgAPACTemp.snsr.idxBaroCorr,    -1,-1,             NO_LIMIT,    SensorIndexType},
  {"NR102",     NO_NEXT_TABLE,   APACMgr_Cfg,     USER_TYPE_ENUM,    USER_RW,  (void *) &cfgAPACTemp.snsr.idxNR102_100,   -1,-1,             NO_LIMIT,    SensorIndexType},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL apacMgr_TimeOutTbl[] =
{ /*Str         Next Tbl Ptr     Handler Func.    Data Type          Access    Parameter                                  IndexRange        DataLimit    EnumTbl*/
  {"HIGELOW_S", NO_NEXT_TABLE,   APACMgr_Cfg,     USER_TYPE_UINT32,  USER_RW,  (void *) &cfgAPACTemp.timeout.hIGELow_sec, -1,-1,            0, 600,      NULL},
  {"STABLE_S",  NO_NEXT_TABLE,   APACMgr_Cfg,     USER_TYPE_UINT32,  USER_RW,  (void *) &cfgAPACTemp.timeout.stable_sec,  -1,-1,            0, 600,      NULL},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

//Note: 0x42c60000 for 99.0, 0xc2c60000 for -99.0
//Note: 0x411e6666 for 9.9, 0xc11e6666 for -9.9
static USER_MSG_TBL apacMgr_AdjustTbl[] =
{ /*Str           Next Tbl Ptr     Handler Func.    Data Type          Access    Parameter                               IndexRange  DataLimit                                EnumTbl*/
  {"ITT_MARGIN",  NO_NEXT_TABLE,   APACMgr_Cfg,     USER_TYPE_FLOAT,   USER_RW,  (void *) &cfgAPACTemp.adjust.itt,       -1,-1,      (INT32) 0xc2c60000, (INT32) 0x42c60000,  NULL},
  {"NG_MARGIN",   NO_NEXT_TABLE,   APACMgr_Cfg,     USER_TYPE_FLOAT,   USER_RW,  (void *) &cfgAPACTemp.adjust.ng,        -1,-1,      (INT32) 0xc11e6666, (INT32) 0x411e6666,  NULL},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL apacMgr_SnsrNamesTbl[] =
{ /*Str         Next Tbl Ptr     Handler Func.    Data Type          Access    Parameter                                  IndexRange        DataLimit                     EnumTbl*/
//{"BAROCORR",  NO_NEXT_TABLE,   APACMgr_Cfg,     USER_TYPE_STR,     USER_RW,  (void *) &cfgAPACTemp.names.barocorr,      -1,-1,            0,APAC_SNSR_NAME_CHAR_MAX,    NULL},
//{"PALT",      NO_NEXT_TABLE,   APACMgr_Cfg,     USER_TYPE_STR,     USER_RW,  (void *) &cfgAPACTemp.names.palt,          -1,-1,            0,APAC_SNSR_NAME_CHAR_MAX,    NULL},
//{"TQ",        NO_NEXT_TABLE,   APACMgr_Cfg,     USER_TYPE_STR,     USER_RW,  (void *) &cfgAPACTemp.names.tq,            -1,-1,            0,APAC_SNSR_NAME_CHAR_MAX,    NULL},
//{"OAT",       NO_NEXT_TABLE,   APACMgr_Cfg,     USER_TYPE_STR,     USER_RW,  (void *) &cfgAPACTemp.names.oat,           -1,-1,            0,APAC_SNSR_NAME_CHAR_MAX,    NULL},
//{"ITT",       NO_NEXT_TABLE,   APACMgr_Cfg,     USER_TYPE_STR,     USER_RW,  (void *) &cfgAPACTemp.names.itt,           -1,-1,            0,APAC_SNSR_NAME_CHAR_MAX,    NULL},
//{"TQCORR",    NO_NEXT_TABLE,   APACMgr_Cfg,     USER_TYPE_STR,     USER_RW,  (void *) &cfgAPACTemp.names.tqcorr,        -1,-1,            0,APAC_SNSR_NAME_CHAR_MAX,    NULL},
//{"ITTMAX",    NO_NEXT_TABLE,   APACMgr_Cfg,     USER_TYPE_STR,     USER_RW,  (void *) &cfgAPACTemp.names.ittMax,        -1,-1,            0,APAC_SNSR_NAME_CHAR_MAX,    NULL},
//{"NGMAX",     NO_NEXT_TABLE,   APACMgr_Cfg,     USER_TYPE_STR,     USER_RW,  (void *) &cfgAPACTemp.names.ngMax,         -1,-1,            0,APAC_SNSR_NAME_CHAR_MAX,    NULL},
  {"TQCRIT",    NO_NEXT_TABLE,   APACMgr_Cfg,     USER_TYPE_STR,     USER_RW,  (void *) cfgAPACTemp.names.tqcrit,         -1,-1,            0,APAC_SNSR_NAME_CHAR_MAX,    NULL},
  {"GNDCRIT",   NO_NEXT_TABLE,   APACMgr_Cfg,     USER_TYPE_STR,     USER_RW,  (void *) cfgAPACTemp.names.gndcrit,        -1,-1,            0,APAC_SNSR_NAME_CHAR_MAX,    NULL},
//{"MARGIN",    NO_NEXT_TABLE,   APACMgr_Cfg,     USER_TYPE_STR,     USER_RW,  (void *) &cfgAPACTemp.names.margin,        -1,-1,            0,APAC_SNSR_NAME_CHAR_MAX,    NULL},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL apacMgr_SnsrTrendNamesTbl[] =
{ /*Str         Next Tbl Ptr     Handler Func.    Data Type          Access    Parameter                                   IndexRange                 DataLimit                      EnumTbl*/
  {"TS",        NO_NEXT_TABLE,   APACMgr_Cfg,     USER_TYPE_STR,     USER_RW,  (void *) cfgAPAC_TrendSnsrTemp,             0, MAX_STAB_SENSORS-1,     0, APAC_SNSR_NAME_CHAR_MAX,    NULL},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL apacMgr_EngDataTbl[] =
{ /*Str              Next Tbl Ptr      Handler Func.        Data Type          Access    Parameter                                        IndexRange          DataLimit     EnumTbl*/
  {"AVG_OAT",        NO_NEXT_TABLE,    APACMgr_EngStatus,   USER_TYPE_FLOAT,   USER_RO,  (void *) &statusAPAC_EngTemp.common.avgOAT,      0,APAC_ENG_MAX-1,   NO_LIMIT,     NULL},
  {"AVG_BAROPRES",   NO_NEXT_TABLE,    APACMgr_EngStatus,   USER_TYPE_FLOAT,   USER_RO,  (void *) &statusAPAC_EngTemp.common.avgBaroPres, 0,APAC_ENG_MAX-1,   NO_LIMIT,     NULL},
  {"AVG_TQ",         NO_NEXT_TABLE,    APACMgr_EngStatus,   USER_TYPE_FLOAT,   USER_RO,  (void *) &statusAPAC_EngTemp.common.avgTQ,       0,APAC_ENG_MAX-1,   NO_LIMIT,     NULL},
  {"VAL_BAROCORR",   NO_NEXT_TABLE,    APACMgr_EngStatus,   USER_TYPE_FLOAT,   USER_RO,  (void *) &statusAPAC_EngTemp.common.valBaroCorr, 0,APAC_ENG_MAX-1,   NO_LIMIT,     NULL},
  {"CONV_BAROCORR",  NO_NEXT_TABLE,    APACMgr_EngStatus,   USER_TYPE_FLOAT,   USER_RO,  (void *) &statusAPAC_EngTemp.common.convBaroCorr,0,APAC_ENG_MAX-1,   NO_LIMIT,     NULL},
  {"CALC_PALT",      NO_NEXT_TABLE,    APACMgr_EngStatus,   USER_TYPE_FLOAT,   USER_RO,  (void *) &statusAPAC_EngTemp.common.calcPALT,    0,APAC_ENG_MAX-1,   NO_LIMIT,     NULL},
  {"COEFF_PALT",     NO_NEXT_TABLE,    APACMgr_EngStatus,   USER_TYPE_FLOAT,   USER_RO,  (void *) &statusAPAC_EngTemp.common.coeffPALT,   0,APAC_ENG_MAX-1,   NO_LIMIT,     NULL},
  {"TQCORR",         NO_NEXT_TABLE,    APACMgr_EngStatus,   USER_TYPE_FLOAT,   USER_RO,  (void *) &statusAPAC_EngTemp.common.tqCorr,      0,APAC_ENG_MAX-1,   NO_LIMIT,     NULL},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL apacMgr_EngDataITTTbl[] =
{ /*Str        Next Tbl Ptr      Handler Func.        Data Type           Access    Parameter                                 IndexRange          DataLimit     EnumTbl*/
  {"AVG_VAL",  NO_NEXT_TABLE,    APACMgr_EngStatus,   USER_TYPE_FLOAT,    USER_RO,  (void *) &statusAPAC_EngTemp.itt.avgVal,   0,APAC_ENG_MAX-1,   NO_LIMIT,     NULL},
  {"MARGIN",   NO_NEXT_TABLE,    APACMgr_EngStatus,   USER_TYPE_FLOAT,    USER_RO,  (void *) &statusAPAC_EngTemp.itt.margin,   0,APAC_ENG_MAX-1,   NO_LIMIT,     NULL},
  {"MAX",      NO_NEXT_TABLE,    APACMgr_EngStatus,   USER_TYPE_FLOAT,    USER_RO,  (void *) &statusAPAC_EngTemp.itt.max,      0,APAC_ENG_MAX-1,   NO_LIMIT,     NULL},
  {"C0",       NO_NEXT_TABLE,    APACMgr_EngStatus,   USER_TYPE_FLOAT64,  USER_RO,  (void *) &statusAPAC_EngTemp.itt.c0,       0,APAC_ENG_MAX-1,   NO_LIMIT,     NULL},
  {"C1",       NO_NEXT_TABLE,    APACMgr_EngStatus,   USER_TYPE_FLOAT64,  USER_RO,  (void *) &statusAPAC_EngTemp.itt.c1,       0,APAC_ENG_MAX-1,   NO_LIMIT,     NULL},
  {"C2",       NO_NEXT_TABLE,    APACMgr_EngStatus,   USER_TYPE_FLOAT64,  USER_RO,  (void *) &statusAPAC_EngTemp.itt.c2,       0,APAC_ENG_MAX-1,   NO_LIMIT,     NULL},
  {"C3",       NO_NEXT_TABLE,    APACMgr_EngStatus,   USER_TYPE_FLOAT64,  USER_RO,  (void *) &statusAPAC_EngTemp.itt.c3,       0,APAC_ENG_MAX-1,   NO_LIMIT,     NULL},
  {"C4",       NO_NEXT_TABLE,    APACMgr_EngStatus,   USER_TYPE_FLOAT64,  USER_RO,  (void *) &statusAPAC_EngTemp.itt.c4,       0,APAC_ENG_MAX-1,   NO_LIMIT,     NULL},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL apacMgr_EngDataNGTbl[] =
{ /*Str        Next Tbl Ptr      Handler Func.        Data Type           Access    Parameter                                 IndexRange          DataLimit     EnumTbl*/
  {"AVG_VAL",  NO_NEXT_TABLE,    APACMgr_EngStatus,   USER_TYPE_FLOAT,    USER_RO,  (void *) &statusAPAC_EngTemp.ng.avgVal,   0,APAC_ENG_MAX-1,   NO_LIMIT,     NULL},
  {"MARGIN",   NO_NEXT_TABLE,    APACMgr_EngStatus,   USER_TYPE_FLOAT,    USER_RO,  (void *) &statusAPAC_EngTemp.ng.margin,   0,APAC_ENG_MAX-1,   NO_LIMIT,     NULL},
  {"MAX",      NO_NEXT_TABLE,    APACMgr_EngStatus,   USER_TYPE_FLOAT,    USER_RO,  (void *) &statusAPAC_EngTemp.ng.max,      0,APAC_ENG_MAX-1,   NO_LIMIT,     NULL},
  {"C0",       NO_NEXT_TABLE,    APACMgr_EngStatus,   USER_TYPE_FLOAT64,  USER_RO,  (void *) &statusAPAC_EngTemp.ng.c0,       0,APAC_ENG_MAX-1,   NO_LIMIT,     NULL},
  {"C1",       NO_NEXT_TABLE,    APACMgr_EngStatus,   USER_TYPE_FLOAT64,  USER_RO,  (void *) &statusAPAC_EngTemp.ng.c1,       0,APAC_ENG_MAX-1,   NO_LIMIT,     NULL},
  {"C2",       NO_NEXT_TABLE,    APACMgr_EngStatus,   USER_TYPE_FLOAT64,  USER_RO,  (void *) &statusAPAC_EngTemp.ng.c2,       0,APAC_ENG_MAX-1,   NO_LIMIT,     NULL},
  {"C3",       NO_NEXT_TABLE,    APACMgr_EngStatus,   USER_TYPE_FLOAT64,  USER_RO,  (void *) &statusAPAC_EngTemp.ng.c3,       0,APAC_ENG_MAX-1,   NO_LIMIT,     NULL},
  {"C4",       NO_NEXT_TABLE,    APACMgr_EngStatus,   USER_TYPE_FLOAT64,  USER_RO,  (void *) &statusAPAC_EngTemp.ng.c4,       0,APAC_ENG_MAX-1,   NO_LIMIT,     NULL},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

// #ifdef APAC_TEST_SIM
// APAC.DEBUG.CALC.  DATA,ITT,NG **************************************************************************************************************************************
static USER_MSG_TBL apacMgr_EngDataDbgTbl[] =
{ /*Str              Next Tbl Ptr      Handler Func.           Data Type          Access    Parameter                                        IndexRange    DataLimit     EnumTbl*/
  {"INLET",          NO_NEXT_TABLE,    APACMgr_EngStatusDbg,   USER_TYPE_ENUM,    USER_RW,  (void *) &statusAPACDbg_Inlet,                   -1,-1,        NO_LIMIT,     apacInletStrs},
  {"NR_SEL",         NO_NEXT_TABLE,    APACMgr_EngStatusDbg,   USER_TYPE_ENUM,    USER_RW,  (void *) &statusAPACDbg_NR_Sel,                  -1,-1,        NO_LIMIT,     apacNrSelStrs},
  {"AVG_OAT",        NO_NEXT_TABLE,    APACMgr_EngStatusDbg,   USER_TYPE_FLOAT,   USER_RW,  (void *) &statusAPAC_EngTemp.common.avgOAT,      -1,-1,        NO_LIMIT,     NULL},
  {"AVG_BAROPRES",   NO_NEXT_TABLE,    APACMgr_EngStatusDbg,   USER_TYPE_FLOAT,   USER_RW,  (void *) &statusAPAC_EngTemp.common.avgBaroPres, -1,-1,        NO_LIMIT,     NULL},
  {"AVG_TQ",         NO_NEXT_TABLE,    APACMgr_EngStatusDbg,   USER_TYPE_FLOAT,   USER_RW,  (void *) &statusAPAC_EngTemp.common.avgTQ,       -1,-1,        NO_LIMIT,     NULL},
  {"VAL_BAROCORR",   NO_NEXT_TABLE,    APACMgr_EngStatusDbg,   USER_TYPE_FLOAT,   USER_RW,  (void *) &statusAPAC_EngTemp.common.valBaroCorr, -1,-1,        NO_LIMIT,     NULL},
  {"CONV_BAROCORR",  NO_NEXT_TABLE,    APACMgr_EngStatusDbg,   USER_TYPE_FLOAT,   USER_RO,  (void *) &statusAPAC_EngTemp.common.convBaroCorr,-1,-1,        NO_LIMIT,     NULL},
  {"CALC_PALT",      NO_NEXT_TABLE,    APACMgr_EngStatusDbg,   USER_TYPE_FLOAT,   USER_RO,  (void *) &statusAPAC_EngTemp.common.calcPALT,    -1,-1,        NO_LIMIT,     NULL},
  {"COEFF_PALT",     NO_NEXT_TABLE,    APACMgr_EngStatusDbg,   USER_TYPE_FLOAT,   USER_RO,  (void *) &statusAPAC_EngTemp.common.coeffPALT,   -1,-1,        NO_LIMIT,     NULL},
  {"TQCORR",         NO_NEXT_TABLE,    APACMgr_EngStatusDbg,   USER_TYPE_FLOAT,   USER_RO,  (void *) &statusAPAC_EngTemp.common.tqCorr,      -1,-1,        NO_LIMIT,     NULL},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL apacMgr_EngDataDbgITTTbl[] =
{ /*Str        Next Tbl Ptr      Handler Func.           Data Type           Access    Parameter                                 IndexRange     DataLimit     EnumTbl*/
  {"AVG_VAL",  NO_NEXT_TABLE,    APACMgr_EngStatusDbg,   USER_TYPE_FLOAT,    USER_RW,  (void *) &statusAPAC_EngTemp.itt.avgVal,   -1,-1,        NO_LIMIT,     NULL},
  {"MARGIN",   NO_NEXT_TABLE,    APACMgr_EngStatusDbg,   USER_TYPE_FLOAT,    USER_RO,  (void *) &statusAPAC_EngTemp.itt.margin,   -1,-1,        NO_LIMIT,     NULL},
  {"MAX",      NO_NEXT_TABLE,    APACMgr_EngStatusDbg,   USER_TYPE_FLOAT,    USER_RO,  (void *) &statusAPAC_EngTemp.itt.max,      -1,-1,        NO_LIMIT,     NULL},
  {"C0",       NO_NEXT_TABLE,    APACMgr_EngStatusDbg,   USER_TYPE_FLOAT64,  USER_RO,  (void *) &statusAPAC_EngTemp.itt.c0,       -1,-1,        NO_LIMIT,     NULL},
  {"C1",       NO_NEXT_TABLE,    APACMgr_EngStatusDbg,   USER_TYPE_FLOAT64,  USER_RO,  (void *) &statusAPAC_EngTemp.itt.c1,       -1,-1,        NO_LIMIT,     NULL},
  {"C2",       NO_NEXT_TABLE,    APACMgr_EngStatusDbg,   USER_TYPE_FLOAT64,  USER_RO,  (void *) &statusAPAC_EngTemp.itt.c2,       -1,-1,        NO_LIMIT,     NULL},
  {"C3",       NO_NEXT_TABLE,    APACMgr_EngStatusDbg,   USER_TYPE_FLOAT64,  USER_RO,  (void *) &statusAPAC_EngTemp.itt.c3,       -1,-1,        NO_LIMIT,     NULL},
  {"C4",       NO_NEXT_TABLE,    APACMgr_EngStatusDbg,   USER_TYPE_FLOAT64,  USER_RO,  (void *) &statusAPAC_EngTemp.itt.c4,       -1,-1,        NO_LIMIT,     NULL},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL apacMgr_EngDataDbgNGTbl[] =
{ /*Str        Next Tbl Ptr      Handler Func.           Data Type           Access    Parameter                                 IndexRange     DataLimit     EnumTbl*/
  {"AVG_VAL",  NO_NEXT_TABLE,    APACMgr_EngStatusDbg,   USER_TYPE_FLOAT,    USER_RW,  (void *) &statusAPAC_EngTemp.ng.avgVal,   -1,-1,          NO_LIMIT,     NULL},
  {"MARGIN",   NO_NEXT_TABLE,    APACMgr_EngStatusDbg,   USER_TYPE_FLOAT,    USER_RO,  (void *) &statusAPAC_EngTemp.ng.margin,   -1,-1,          NO_LIMIT,     NULL},
  {"MAX",      NO_NEXT_TABLE,    APACMgr_EngStatusDbg,   USER_TYPE_FLOAT,    USER_RO,  (void *) &statusAPAC_EngTemp.ng.max,      -1,-1,          NO_LIMIT,     NULL},
  {"C0",       NO_NEXT_TABLE,    APACMgr_EngStatusDbg,   USER_TYPE_FLOAT64,  USER_RO,  (void *) &statusAPAC_EngTemp.ng.c0,       -1,-1,          NO_LIMIT,     NULL},
  {"C1",       NO_NEXT_TABLE,    APACMgr_EngStatusDbg,   USER_TYPE_FLOAT64,  USER_RO,  (void *) &statusAPAC_EngTemp.ng.c1,       -1,-1,          NO_LIMIT,     NULL},
  {"C2",       NO_NEXT_TABLE,    APACMgr_EngStatusDbg,   USER_TYPE_FLOAT64,  USER_RO,  (void *) &statusAPAC_EngTemp.ng.c2,       -1,-1,          NO_LIMIT,     NULL},
  {"C3",       NO_NEXT_TABLE,    APACMgr_EngStatusDbg,   USER_TYPE_FLOAT64,  USER_RO,  (void *) &statusAPAC_EngTemp.ng.c3,       -1,-1,          NO_LIMIT,     NULL},
  {"C4",       NO_NEXT_TABLE,    APACMgr_EngStatusDbg,   USER_TYPE_FLOAT64,  USER_RO,  (void *) &statusAPAC_EngTemp.ng.c4,       -1,-1,          NO_LIMIT,     NULL},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL apacMgr_EngStatusDbgTbl[] =
{ /*Str         Next Tbl Ptr               Handler Func.   Data Type     Access             Parameter               IndexRange                        DataLimit     EnumTbl*/
  {"DATA",      apacMgr_EngDataDbgTbl,     NULL,                         NO_HANDLER_DATA },
  {"ITT",       apacMgr_EngDataDbgITTTbl,  NULL,                         NO_HANDLER_DATA },
  {"NG",        apacMgr_EngDataDbgNGTbl,   NULL,                         NO_HANDLER_DATA },
  {"EXE",       NO_NEXT_TABLE,             APACMgr_EngStatusDbgExe,      USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG),  (void *) &statusAPACTemp.state,   -1,-1,        NO_LIMIT,   NULL},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};
// APAC.DEBUG.CALC.  DATA,ITT,NG **************************************************************************************************************************************
// #endif

static USER_MSG_TBL apacMgr_EngStatusTbl[] =
{ /*Str         Next Tbl Ptr            Handler Func.        Data Type          Access    Parameter                                   IndexRange          DataLimit            EnumTbl*/
  {"HRS_PREV_S",NO_NEXT_TABLE,          APACMgr_EngStatus,   USER_TYPE_UINT32,  USER_RO,  (void *) &statusAPAC_EngTemp.hrs.prev_s,    0,APAC_ENG_MAX-1,   NO_LIMIT,            NULL},
  {"HRS_CURR_S",NO_NEXT_TABLE,          APACMgr_EngStatus,   USER_TYPE_UINT32,  USER_RO,  (void *) &statusAPAC_EngTemp.hrs.curr_s,    0,APAC_ENG_MAX-1,   NO_LIMIT,            NULL},
  {"ESN",       NO_NEXT_TABLE,          APACMgr_EngStatus,   USER_TYPE_STR,     USER_RO,  (void *) statusAPAC_EngTemp.esn,            0,APAC_ENG_MAX-1,   0,APAC_ESN_MAX_LEN,  NULL},
  {"ESN_NVM",   NO_NEXT_TABLE,          APACMgr_EngStatus,   USER_TYPE_STR,     USER_RO,  (void *) nvmAPAC_EngTemp.esn,               0,APAC_ENG_MAX-1,   0,APAC_ESN_MAX_LEN,  NULL},
  {"COMMIT",    NO_NEXT_TABLE,          APACMgr_EngStatus,   USER_TYPE_ENUM,    USER_RO,  (void *) &statusAPAC_EngTemp.commit,        0,APAC_ENG_MAX-1,   NO_LIMIT,            apacCommitStrs},
  {"VLD_PEND",  NO_NEXT_TABLE,          APACMgr_EngStatus,   USER_TYPE_BOOLEAN, USER_RO,  (void *) &statusAPAC_EngTemp.vld.set,       0,APAC_ENG_MAX-1,   NO_LIMIT,            NULL},
  {"VLD_REASON",NO_NEXT_TABLE,          APACMgr_EngStatus,   USER_TYPE_ENUM,    USER_RO,  (void *) &statusAPAC_EngTemp.vld.reason,    0,APAC_ENG_MAX-1,   NO_LIMIT,            vldReasonStrs},
  {"VLD_REASON_SUMMARY",NO_NEXT_TABLE,  APACMgr_EngStatus,   USER_TYPE_HEX16,   USER_RO,  (void *) &statusAPAC_EngTemp.vld.reason_summary,  0,APAC_ENG_MAX-1,   NO_LIMIT,      NULL},
  {"DATA",      apacMgr_EngDataTbl,     NULL,                NO_HANDLER_DATA },
  {"ITT",       apacMgr_EngDataITTTbl,  NULL,                NO_HANDLER_DATA },
  {"NG",        apacMgr_EngDataNGTbl,   NULL,                NO_HANDLER_DATA },
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL apacMgrCfgTbl[] =
{ /*Str          Next Tbl Ptr               Handler Func.  Data Type          Access    Parameter                           IndexRange   DataLimit    EnumTbl*/
  {"ENABLED",    NO_NEXT_TABLE,             APACMgr_Cfg,   USER_TYPE_BOOLEAN, USER_RW,  (void *) &cfgAPACTemp.enabled,      -1,-1,       NO_LIMIT,    NULL},
  {"ENG",        apacMgr_EngTbl,            NULL,          NO_HANDLER_DATA },
  {"SNSR",       apacMgr_SnsrTbl,           NULL,          NO_HANDLER_DATA },
  {"TIMEOUT",    apacMgr_TimeOutTbl,        NULL,          NO_HANDLER_DATA },
  {"INLET",      NO_NEXT_TABLE,             APACMgr_Cfg,   USER_TYPE_ENUM,    USER_RW,  (void *) &cfgAPACTemp.inletCfg,     -1,-1,       NO_LIMIT,    apacInletStrs},
  {"SNSR_NAMES", apacMgr_SnsrNamesTbl,      NULL,          NO_HANDLER_DATA },
  {"TREND_NAMES",apacMgr_SnsrTrendNamesTbl, NULL,          NO_HANDLER_DATA },
  {"ADJUST",     apacMgr_AdjustTbl,         NULL,          NO_HANDLER_DATA },
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL apacMgrStatusTbl[] =
{ /*Str            Next Tbl Ptr           Handler Func.      Data Type          Access     Parameter                           IndexRange   DataLimit    EnumTbl*/
  {"STATE",        NO_NEXT_TABLE,         APACMgr_Status,    USER_TYPE_ENUM,    USER_RO,   (void *) &statusAPACTemp.state,     -1,-1,       NO_LIMIT,    apacStateStrs},
  {"TEST_STATE",   NO_NEXT_TABLE,         APACMgr_Status,    USER_TYPE_ENUM,    USER_RO,   (void *) &statusAPACTemp.run_state, -1,-1,       NO_LIMIT,    apacRunStateStrs},
  {"ENG_UUT",      NO_NEXT_TABLE,         APACMgr_Status,    USER_TYPE_ENUM,    USER_RO,   (void *) &statusAPACTemp.eng_uut,   -1,-1,       NO_LIMIT,    apacEngUUTStrs},
  {"NR_SEL",       NO_NEXT_TABLE,         APACMgr_Status,    USER_TYPE_ENUM,    USER_RO,   (void *) &statusAPACTemp.nr_sel,    -1,-1,       NO_LIMIT,    apacNrSelStrs},
  {"CFG_STATE",    NO_NEXT_TABLE,         APACMgr_Status,    USER_TYPE_ENUM,    USER_RO,   (void *) &statusAPACTemp.cfg_state, -1,-1,       NO_LIMIT,    apacCfgStateStrs},
  {"CFG_ERR",      NO_NEXT_TABLE,         APACMgr_Status,    USER_TYPE_STR,     USER_RO,   (void *) statusAPACTemp.cfg_err,    -1,-1,       NO_LIMIT,    NULL},
  {"ENG",          apacMgr_EngStatusTbl,  NULL,              NO_HANDLER_DATA },
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL apacMgrDebugCmdTbl[] =
{ /*Str            Next Tbl Ptr        Handler Func.         Data Type          Access                   Parameter                             IndexRange   DataLimit    EnumTbl*/
  {"DATE",         NO_NEXT_TABLE,      APACMgr_DebugCmdHist, USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG),   (void *) APAC_DBG_CMD_HIST_DATE,      -1,-1,       NO_LIMIT,    NULL},
  {"DAY",          NO_NEXT_TABLE,      APACMgr_DebugCmdHist, USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG),   (void *) APAC_DBG_CMD_HIST_DAY,       -1,-1,       NO_LIMIT,    NULL},
  {"DATE_ADV",     NO_NEXT_TABLE,      APACMgr_DebugCmdHist, USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG),   (void *) APAC_DBG_CMD_HIST_DATE_ADV,  -1,-1,       NO_LIMIT,    NULL},
  {"DATE_DEC",     NO_NEXT_TABLE,      APACMgr_DebugCmdHist, USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG),   (void *) APAC_DBG_CMD_HIST_DATE_DEC,  -1,-1,       NO_LIMIT,    NULL},
  {"DAY_ADV",      NO_NEXT_TABLE,      APACMgr_DebugCmdHist, USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG),   (void *) APAC_DBG_CMD_HIST_DAY_ADV,   -1,-1,       NO_LIMIT,    NULL},
  {"DAY_DEC",      NO_NEXT_TABLE,      APACMgr_DebugCmdHist, USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG),   (void *) APAC_DBG_CMD_HIST_DAY_DEC,   -1,-1,       NO_LIMIT,    NULL},
  {"CONFIG",       NO_NEXT_TABLE,      APACMgr_DebugCmdHist, USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG),   (void *) APAC_DBG_CMD_CONFIG,         -1,-1,       NO_LIMIT,    NULL},
  {"SELECT_100",   NO_NEXT_TABLE,      APACMgr_DebugCmdHist, USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG),   (void *) APAC_DBG_CMD_SELECT_100,     -1,-1,       NO_LIMIT,    NULL},
  {"SELECT_102",   NO_NEXT_TABLE,      APACMgr_DebugCmdHist, USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG),   (void *) APAC_DBG_CMD_SELECT_102,     -1,-1,       NO_LIMIT,    NULL},
  {"SETUP",        NO_NEXT_TABLE,      APACMgr_DebugCmdHist, USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG),   (void *) APAC_DBG_CMD_SETUP,          -1,-1,       NO_LIMIT,    NULL},
  {"VLD_MANUAL",   NO_NEXT_TABLE,      APACMgr_DebugCmdHist, USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG),   (void *) APAC_DBG_CMD_VLD_MANUAL,     -1,-1,       NO_LIMIT,    NULL},
  {"RUN_PAC",      NO_NEXT_TABLE,      APACMgr_DebugCmdHist, USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG),   (void *) APAC_DBG_CMD_RUN_PAC,        -1,-1,       NO_LIMIT,    NULL},
  {"PAC_COMPLETED",NO_NEXT_TABLE,      APACMgr_DebugCmdHist, USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG),   (void *) APAC_DBG_CMD_PAC_COMPLETED,  -1,-1,       NO_LIMIT,    NULL},
  {"RESULT",       NO_NEXT_TABLE,      APACMgr_DebugCmdHist, USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG),   (void *) APAC_DBG_CMD_RESULT,         -1,-1,       NO_LIMIT,    NULL},
  {"COMMIT_VLD",   NO_NEXT_TABLE,      APACMgr_DebugCmdHist, USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG),   (void *) APAC_DBG_CMD_RESULT_COMMIT_VLD,   -1,-1,       NO_LIMIT,    NULL},
  {"COMMIT_NVD",   NO_NEXT_TABLE,      APACMgr_DebugCmdHist, USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG),   (void *) APAC_DBG_CMD_RESULT_COMMIT_INVLD, -1,-1,       NO_LIMIT,    NULL},
  {"COMMIT_ACP",   NO_NEXT_TABLE,      APACMgr_DebugCmdHist, USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG),   (void *) APAC_DBG_CMD_RESULT_COMMIT_ACPT,  -1,-1,       NO_LIMIT,    NULL},
  {"COMMIT_RJT",   NO_NEXT_TABLE,      APACMgr_DebugCmdHist, USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG),   (void *) APAC_DBG_CMD_RESULT_COMMIT_RJCT,  -1,-1,       NO_LIMIT,    NULL},
  {"RESET_DCLK",   NO_NEXT_TABLE,      APACMgr_DebugCmdHist, USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG),   (void *) APAC_DBG_CMD_RESET_DCLK,      -1,-1,      NO_LIMIT,    NULL},
  {"RESET_TIMEOUT",NO_NEXT_TABLE,      APACMgr_DebugCmdHist, USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG),   (void *) APAC_DBG_CMD_RESET_TIMEOUT,   -1,-1,      NO_LIMIT,    NULL},
  {"ERR_MSG",      NO_NEXT_TABLE,      APACMgr_DebugCmdHist, USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG),   (void *) APAC_DBG_CMD_ERR_MSG,         -1,-1,      NO_LIMIT,    NULL},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

// #ifdef APAC_TEST_SIM
static USER_MSG_TBL apacMgrDebugCfgBatchTbl[] =
{ /*Str         Next Tbl Ptr        Handler Func.          Data Type          Access                   Parameter                             IndexRange   DataLimit    EnumTbl*/
  {"ON",        NO_NEXT_TABLE,      APACMgr_DebugCfgBatch, USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG),   (void *) AAPAC_DBG_CFG_BATCH_ON,      -1,-1,       NO_LIMIT,    NULL},
  {"STORE",     NO_NEXT_TABLE,      APACMgr_DebugCfgBatch, USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG),   (void *) AAPAC_DBG_CFG_BATCH_STORE,   -1,-1,       NO_LIMIT,    NULL},
  {"CANCEL",    NO_NEXT_TABLE,      APACMgr_DebugCfgBatch, USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG),   (void *) AAPAC_DBG_CFG_BATCH_CANCEL,  -1,-1,       NO_LIMIT,    NULL},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};
// #endif

static USER_MSG_TBL apacMgrDebugTbl[] =
{ /*Str            Next Tbl Ptr             Handler Func.             Data Type          Access                   Parameter                         IndexRange   DataLimit    EnumTbl*/
  {"HISTORY",      NO_NEXT_TABLE,           APACMgr_RecentAllHistory, USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG),   (void *) NULL,                    -1,-1,       NO_LIMIT,    NULL},
  {"HISTORY_CLEAR",NO_NEXT_TABLE,           APACMgr_HistoryClear,     USER_TYPE_ACTION,  (USER_RO),               (void *) NULL,                    -1,-1,       NO_LIMIT,    NULL},
  {"CALC",         apacMgr_EngStatusDbgTbl, NULL, NO_HANDLER_DATA},
  {"CMD",          apacMgrDebugCmdTbl,      NULL, NO_HANDLER_DATA},
  {"CFGBATCH",     apacMgrDebugCfgBatchTbl, NULL, NO_HANDLER_DATA},
#ifdef APAC_TEST_SIM
  {"SIM_TREND",    NO_NEXT_TABLE,           APACMgr_Debug,            USER_TYPE_BOOLEAN, USER_RW,                 (void *) &debugAPACTemp.simTrend, -1,-1,       NO_LIMIT,    NULL},
  {"SIM_RESTART",  NO_NEXT_TABLE,           APACMgr_DebugRestart,     USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG),   (void *) &statusAPACTemp.state,   -1,-1,       NO_LIMIT,    NULL},
#endif
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL apacMgrRoot[] =
{ /*Str            Next Tbl Ptr     Handler Func.          Data Type          Access                         Parameter      IndexRange  DataLimit    EnumTbl*/
  {"CFG",          apacMgrCfgTbl,   NULL,NO_HANDLER_DATA},
  {"STATUS",       apacMgrStatusTbl,NULL,NO_HANDLER_DATA},
  {"DEBUG",        apacMgrDebugTbl, NULL,NO_HANDLER_DATA},
  {DISPLAY_CFG,    NO_NEXT_TABLE,   APACMgr_ShowConfig,    USER_TYPE_ACTION, (USER_RO|USER_NO_LOG|USER_GSE), NULL,          -1,  -1,    NO_LIMIT,    NULL},
  {NULL,           NULL,            NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL apacMgrRootTblPtr = {"APAC",apacMgrRoot,NULL,NO_HANDLER_DATA};


/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:    APACMgr_Status
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves the latest APAC_STATUS
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
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
static USER_HANDLER_RESULT APACMgr_Status(USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr)
{
  USER_HANDLER_RESULT result ;

  result = USER_RESULT_OK;

  statusAPACTemp = *APACMgr_GetStatus();

  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);

  return result;
}


/******************************************************************************
 * Function:    APACMgr_EngStatus
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves the latest APAC_STATUS->APAC_ENG_STATUS
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
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
static USER_HANDLER_RESULT APACMgr_EngStatus(USER_DATA_TYPE DataType,
                                             USER_MSG_PARAM Param,
                                             UINT32 Index,
                                             const void *SetPtr,
                                             void **GetPtr)
{
  USER_HANDLER_RESULT result ;

  result = USER_RESULT_OK;

  memcpy ( (void *) &statusAPAC_EngTemp, (void *) &APACMgr_GetStatus()->eng[Index],
            sizeof(statusAPAC_EngTemp) );

  memcpy ( (void *) &nvmAPAC_EngTemp, (void *) &m_APAC_NVM.eng[Index],
           sizeof(nvmAPAC_EngTemp) );

  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);

  return result;
}


/******************************************************************************
 * Function:    APACMgr_Cfg
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves and set the latest APAC_CFG
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
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:       none
 *
*****************************************************************************/
static USER_HANDLER_RESULT APACMgr_Cfg(USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr)
{
  USER_HANDLER_RESULT result;


  result = USER_RESULT_OK;

  if ( Param.Ptr == (void *) cfgAPAC_TrendSnsrTemp ) {
    memcpy(cfgAPAC_TrendSnsrTemp, CfgMgr_ConfigPtr()->APACConfig.namesTrend.snsr[Index],
           APAC_SNSR_NAME_CHAR_MAX);
  }
  else {
    memcpy(&cfgAPACTemp, &CfgMgr_ConfigPtr()->APACConfig, sizeof(APAC_CFG));
  }
  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

  // If performing a "set" and it was successful, write the update.
  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    if ( Param.Ptr == (void *) cfgAPAC_TrendSnsrTemp ) {
      memcpy(CfgMgr_ConfigPtr()->APACConfig.namesTrend.snsr[Index], cfgAPAC_TrendSnsrTemp,
             APAC_SNSR_NAME_CHAR_MAX);
    }
    else {
      memcpy(&CfgMgr_ConfigPtr()->APACConfig, &cfgAPACTemp, sizeof(APAC_CFG));
    }

    CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(), &CfgMgr_ConfigPtr()->APACConfig,
                           sizeof(APAC_CFG));
  }

  return (result);

}


/******************************************************************************
 * Function:    APACMgr_EngCfg
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves and set the latest APAC_CFG->APAC_ENG_CFG[Index]
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
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:       none
 *
*****************************************************************************/
static USER_HANDLER_RESULT APACMgr_EngCfg(USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr)
{
  USER_HANDLER_RESULT result;


  result = USER_RESULT_OK;

  memcpy(&cfgAPAC_EngTemp, &CfgMgr_ConfigPtr()->APACConfig.eng[Index], sizeof(APAC_ENG_CFG));
  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

  // If performing a "set" and it was successful, write the update.
  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    memcpy(&CfgMgr_ConfigPtr()->APACConfig.eng[Index], &cfgAPAC_EngTemp, sizeof(APAC_ENG_CFG));

    CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(), &CfgMgr_ConfigPtr()->APACConfig.eng[Index],
                           sizeof(APAC_ENG_CFG));
  }

  return (result);

}


/******************************************************************************
 * Function:    APACMgr_CfgExprStrCmd
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves and set the latest APAC_CFG->APAC_ENG_CFG->sc_TqCrit
 *              or ->sc_GndCrit
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
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:       none
 *
*****************************************************************************/
static USER_HANDLER_RESULT APACMgr_CfgExprStrCmd(USER_DATA_TYPE DataType,
                                                 USER_MSG_PARAM Param,
                                                 UINT32 Index,
                                                 const void *SetPtr,
                                                 void **GetPtr)
{
  USER_HANDLER_RESULT result;
  //User Mgr uses this obj outside of this function's scope
  static CHAR str[EVAL_MAX_EXPR_STR_LEN];
  INT32 strToBinResult;

  result = USER_RESULT_OK;

  memcpy(&cfgAPAC_EngTemp, &CfgMgr_ConfigPtr()->APACConfig.eng[Index], sizeof(APAC_ENG_CFG));

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

    strToBinResult = EvalExprStrToBin(EVAL_CALLER_TYPE_PARSE, (INT32)Index,
                                      str, (EVAL_EXPR*) Param.Ptr, MAX_TRIG_EXPR_OPRNDS);

    if( strToBinResult >= 0 )
    {
      // Move the successfully updated binary expression to the m_APAC_Cfg.eng[].sc_TqCrit or
      // .sc_GndCrit for storage
      memcpy(&CfgMgr_ConfigPtr()->APACConfig.eng[Index], &cfgAPAC_EngTemp,
             sizeof(APAC_ENG_CFG));

      //Store the modified structure in the EEPROM.
      CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(), &CfgMgr_ConfigPtr()->APACConfig.eng[Index],
                             sizeof(APAC_ENG_CFG));
    }
    else
    {
      GSE_DebugStr( NORMAL,FALSE,"EvalExprStrToBin Failed: %s"NEW_LINE,
                    EvalGetMsgFromErrCode(strToBinResult));
      result = USER_RESULT_ERROR;
    }
  }

  return (result);

}

#ifdef APAC_TEST_SIM
/******************************************************************************
 * Function:    APACMgr_DebugRestart
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Restart the APAC Simulation Test Code
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
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:       none
 *
*****************************************************************************/
static USER_HANDLER_RESULT APACMgr_DebugRestart(USER_DATA_TYPE DataType,
                                                USER_MSG_PARAM Param,
                                                UINT32 Index,
                                                const void *SetPtr,
                                                void **GetPtr)
{
  USER_HANDLER_RESULT result;

  result = USER_RESULT_OK;

  simState = APAC_SIM_IDLE;
  simNextTick = CM_GetTickCount() + simStateDelay[simState];

  return (result);
}
#endif

// #ifdef APAC_TEST_SIM
/******************************************************************************
 * Function:    APACMgr_DebugCmdHist
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Issues the Date,Day,Date_Adv/Dec,Day_Adv/Dec func call
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
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:       none
 *
*****************************************************************************/
static USER_HANDLER_RESULT APACMgr_DebugCmdHist(USER_DATA_TYPE DataType,
                                                USER_MSG_PARAM Param,
                                                UINT32 Index,
                                                const void *SetPtr,
                                                void **GetPtr)
{
  USER_HANDLER_RESULT result;
  static CHAR msg1[APAC_MENU_DISPLAY_CHAR_MAX];
  static CHAR msg2[APAC_MENU_DISPLAY_CHAR_MAX];
  static CHAR msg3[APAC_MENU_DISPLAY_CHAR_MAX];
  static CHAR msg4[APAC_MENU_DISPLAY_CHAR_MAX];
#define APAC_DEBUGCMDHIST_MAX 128
  static CHAR apac_strTemp[APAC_DEBUGCMDHIST_MAX];
  BOOLEAN ret;


  result = USER_RESULT_OK;
  ret = FALSE;

  memset ( (void *) msg1, 0, APAC_MENU_DISPLAY_CHAR_MAX );
  memset ( (void *) msg2, 0, APAC_MENU_DISPLAY_CHAR_MAX );
  memset ( (void *) msg3, 0, APAC_MENU_DISPLAY_CHAR_MAX );
  memset ( (void *) msg4, 0, APAC_MENU_DISPLAY_CHAR_MAX );

  switch (Param.Int)
  {
    case APAC_DBG_CMD_HIST_DATE:
      ret = APACMgr_IF_HistoryData( &msg1[0], &msg2[0], &msg3[0], &msg4[0], APAC_IF_DATE);
      break;
    case APAC_DBG_CMD_HIST_DAY:
      ret = APACMgr_IF_HistoryData( &msg1[0], &msg2[0], &msg3[0], &msg4[0], APAC_IF_DAY);
      break;
    case APAC_DBG_CMD_HIST_DATE_ADV:
      ret = APACMgr_IF_HistoryAdv( &msg1[0], &msg2[0], &msg3[0], &msg4[0], APAC_IF_DATE);
      break;
    case APAC_DBG_CMD_HIST_DATE_DEC:
      ret = APACMgr_IF_HistoryDec( &msg1[0], &msg2[0], &msg3[0], &msg4[0], APAC_IF_DATE);
      break;
    case APAC_DBG_CMD_HIST_DAY_ADV:
      ret = APACMgr_IF_HistoryAdv( &msg1[0], &msg2[0], &msg3[0], &msg4[0], APAC_IF_DAY);
      break;
    case APAC_DBG_CMD_HIST_DAY_DEC:
      ret = APACMgr_IF_HistoryDec( &msg1[0], &msg2[0], &msg3[0], &msg4[0], APAC_IF_DAY);
      break;
    case APAC_DBG_CMD_CONFIG:
      ret = APACMgr_IF_Config ( &msg1[0], &msg2[0], &msg3[0], &msg4[0], APAC_IF_MAX);
      break;
    case APAC_DBG_CMD_SELECT_100:
      ret = APACMgr_IF_Select ( &msg1[0], &msg2[0], &msg3[0], &msg4[0], APAC_IF_NR100);
      break;
    case APAC_DBG_CMD_SELECT_102:
      ret = APACMgr_IF_Select ( &msg1[0], &msg2[0], &msg3[0], &msg4[0], APAC_IF_NR102);
      break;
    case APAC_DBG_CMD_SETUP:
      ret = APACMgr_IF_Setup ( &msg1[0], &msg2[0], &msg3[0], &msg4[0], APAC_IF_MAX);
      break;
    case APAC_DBG_CMD_VLD_MANUAL:
      ret = APACMgr_IF_ValidateManual ( &msg1[0], &msg2[0], &msg3[0], &msg4[0], APAC_IF_MAX);
      break;
    case APAC_DBG_CMD_RUN_PAC:
      ret = APACMgr_IF_RunningPAC ( &msg1[0], &msg2[0], &msg3[0], &msg4[0], APAC_IF_MAX);
      break;
    case APAC_DBG_CMD_PAC_COMPLETED:
      ret = APACMgr_IF_CompletedPAC ( &msg1[0], &msg2[0], &msg3[0], &msg4[0], APAC_IF_MAX);
      break;
    case APAC_DBG_CMD_RESULT:
      ret = APACMgr_IF_Result ( &msg1[0], &msg2[0], &msg3[0], &msg4[0], APAC_IF_MAX);
      break;
    case APAC_DBG_CMD_RESULT_COMMIT_VLD:
      ret = APACMgr_IF_ResultCommit ( &msg1[0], &msg2[0], &msg3[0], &msg4[0], APAC_IF_VLD);
      break;
    case APAC_DBG_CMD_RESULT_COMMIT_INVLD:
      ret = APACMgr_IF_ResultCommit ( &msg1[0], &msg2[0], &msg3[0], &msg4[0], APAC_IF_INVLD);
      break;
    case APAC_DBG_CMD_RESULT_COMMIT_ACPT:
      ret = APACMgr_IF_ResultCommit ( &msg1[0], &msg2[0], &msg3[0], &msg4[0], APAC_IF_ACPT);
      break;
    case APAC_DBG_CMD_RESULT_COMMIT_RJCT:
      ret = APACMgr_IF_ResultCommit ( &msg1[0], &msg2[0], &msg3[0], &msg4[0], APAC_IF_RJCT);
      break;
    case APAC_DBG_CMD_RESET_DCLK:
      ret = APACMgr_IF_Reset ( &msg1[0], &msg2[0], &msg3[0], &msg4[0], APAC_IF_DCLK);
      break;
    case APAC_DBG_CMD_RESET_TIMEOUT:
      ret = APACMgr_IF_Reset ( &msg1[0], &msg2[0], &msg3[0], &msg4[0], APAC_IF_TIMEOUT);
      break;
    case APAC_DBG_CMD_ERR_MSG:
      ret = APACMgr_IF_ErrMsg ( &msg1[0], &msg2[0], &msg3[0], &msg4[0], APAC_IF_MAX);
      break;
    default:
      break;
  }

  if ((Param.Int != APAC_DBG_CMD_HIST_DATE_ADV) &&
      (Param.Int != APAC_DBG_CMD_HIST_DATE_DEC) &&
      (Param.Int != APAC_DBG_CMD_HIST_DAY_ADV) &&
      (Param.Int != APAC_DBG_CMD_HIST_DAY_DEC))
  {
    snprintf (apac_strTemp, sizeof(apac_strTemp), "\r\nm1=%s,m2=%s,m3=%s,m4=%s,ret=%d",
              msg1, msg2, msg3, msg4, ret);
    User_OutputMsgString( apac_strTemp, FALSE );
  }

  return (result);
}
// #endif

/******************************************************************************
 * Function:    APACMgr_Debug
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves and set the latest m_APAC_Debug data
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
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:       none
 *
*****************************************************************************/
static USER_HANDLER_RESULT APACMgr_Debug(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr)
{
  USER_HANDLER_RESULT result;


  result = USER_RESULT_OK;

  debugAPACTemp = m_APAC_Debug;

  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

  // If performing a "set" and it was successful, write the update.
  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    m_APAC_Debug = debugAPACTemp;
  }

  return (result);

}

// #ifdef APAC_TEST_SIM
/******************************************************************************
 * Function:    APACMgr_EngStatusDbg
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves the latest Debug Status Data for Manual Calc
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
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
static USER_HANDLER_RESULT APACMgr_EngStatusDbg(USER_DATA_TYPE DataType,
                                                USER_MSG_PARAM Param,
                                                UINT32 Index,
                                                const void *SetPtr,
                                                void **GetPtr)
{
  USER_HANDLER_RESULT result ;

  result = USER_RESULT_OK;


  statusAPAC_EngTemp = m_APAC_Eng_Debug;
  statusAPACDbg_Inlet = m_APAC_Eng_Debug_Inlet;
  statusAPACDbg_NR_Sel = m_APAC_Eng_Debug_NR_Sel;

  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);

  // If performing a "set" and it was successful, write the update.
  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    m_APAC_Eng_Debug = statusAPAC_EngTemp;
    m_APAC_Eng_Debug_Inlet = statusAPACDbg_Inlet;
    m_APAC_Eng_Debug_NR_Sel = statusAPACDbg_NR_Sel;
  }

  return result;
}
// #endif

// #ifdef APAC_TEST_SIM
/******************************************************************************
 * Function:    APACMgr_EngStatusDbgExe
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Executes the APAC Calculation for ITT and NG
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
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
static USER_HANDLER_RESULT APACMgr_EngStatusDbgExe(USER_DATA_TYPE DataType,
                                                   USER_MSG_PARAM Param,
                                                   UINT32 Index,
                                                   const void *SetPtr,
                                                   void **GetPtr)
{
  USER_HANDLER_RESULT result ;
  BOOLEAN bOk;
  APAC_ENG_CALC_COMMON_PTR common_ptr;
  APAC_ENG_CALC_DATA_PTR itt_ptr, ng_ptr;
  APAC_INSTALL_ENUM tblIdx;
  APAC_TBL_PTR tbl_ptr;


  result = USER_RESULT_OK;

  common_ptr = (APAC_ENG_CALC_COMMON_PTR) &m_APAC_Eng_Debug.common;
  itt_ptr = (APAC_ENG_CALC_DATA_PTR) &m_APAC_Eng_Debug.itt;
  ng_ptr = (APAC_ENG_CALC_DATA_PTR) &m_APAC_Eng_Debug.ng;

  // Call APACMgr_CalcTqDelta()
  bOk =  APACMgr_CalcTqCorr( common_ptr->valBaroCorr, common_ptr->avgBaroPres,
                             common_ptr->avgTQ, common_ptr);

  // Call APACMgr_CalcMargin()
  if (bOk) {
    tblIdx = m_APAC_Tbl_Mapping[m_APAC_Eng_Debug_NR_Sel][m_APAC_Eng_Debug_Inlet];
    tbl_ptr = (APAC_TBL_PTR) &m_APAC_Tbl[APAC_ITT][tblIdx];
    bOk = APACMgr_CalcMargin( common_ptr->avgOAT, common_ptr->tqCorr,
                                   itt_ptr->avgVal, tbl_ptr, 0.0f, &itt_ptr->margin,
                                   &itt_ptr->max, itt_ptr );
    tbl_ptr = (APAC_TBL_PTR) &m_APAC_Tbl[APAC_NG][tblIdx];
    bOk = APACMgr_CalcMargin( common_ptr->avgOAT, common_ptr->tqCorr,
                              ng_ptr->avgVal, tbl_ptr, 0.0f, &ng_ptr->margin,
                              &ng_ptr->max, ng_ptr );
  }

  return result;
}
// #endif

// #ifdef APAC_TEST_SIM
/******************************************************************************
 * Function:    APACMgr_DebugCfgBatch
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Enables Cfg Batch Mode Load
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
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:       none
 *
*****************************************************************************/
static USER_HANDLER_RESULT APACMgr_DebugCfgBatch(USER_DATA_TYPE DataType,
                                                 USER_MSG_PARAM Param,
                                                 UINT32 Index,
                                                 const void *SetPtr,
                                                 void **GetPtr)
{
  USER_HANDLER_RESULT result;

  result = USER_RESULT_OK;

  switch (Param.Int)
  {
    case AAPAC_DBG_CFG_BATCH_ON:
      CfgMgr_StartBatchCfg();
      break;

    case AAPAC_DBG_CFG_BATCH_STORE:
      CfgMgr_CommitBatchCfg();
      break;

    case AAPAC_DBG_CFG_BATCH_CANCEL:
      CfgMgr_CancelBatchCfg();
      break;

    default:
      break;
  }
  return (result);
}
// #endif

/******************************************************************************
 * Function:    APACMgr_RecentAllHistory
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves the latest History from APAC Record NVM Buffer 100 recs
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
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
static
USER_HANDLER_RESULT APACMgr_RecentAllHistory(USER_DATA_TYPE DataType,
                                             USER_MSG_PARAM Param,
                                             UINT32 Index,
                                             const void *SetPtr,
                                             void **GetPtr)
{
  UINT8 i;
  static CHAR resultBuffer[USER_SINGLE_MSG_MAX_SIZE];
  USER_HANDLER_RESULT result = USER_RESULT_OK;
  CHAR *pStr;
  UINT32 nOffset, len;
  CHAR strDateTime[APAC_MAX_RECENT_DATE_TIME];
  TIMESTRUCT time_struct;
  TIMESTAMP ts;
  APAC_HIST_ENTRY_PTR entry_ptr;
  UINT16 num;


  pStr = (CHAR *) &resultBuffer[0];

  // Output # of faults in buffer
  num = (m_APAC_Hist_Status.num == 0) ? 0 : (m_APAC_Hist_Status.num - 1);
  entry_ptr = (APAC_HIST_ENTRY_PTR) &m_APAC_Hist.entry[num];

  CM_GetTimeAsTimestamp (&ts);
  ts = (m_APAC_Hist_Status.num != 0) ? entry_ptr->ts : ts;
  CM_ConvertTimeStamptoTimeStruct( &ts, &time_struct);

  snprintf ( strDateTime, APAC_MAX_RECENT_DATE_TIME, "%02d:%02d:%02d %02d/%02d/%4d",
             time_struct.Hour,
             time_struct.Minute,
             time_struct.Second,
             time_struct.Month,
             time_struct.Day,
             time_struct.Year );

  snprintf ( resultBuffer, USER_SINGLE_MSG_MAX_SIZE,
             "\r\nAPAC Buffer History = %d of %d Max, Last Update =%s\r\n",
             m_APAC_Hist_Status.num , APAC_HIST_ENTRY_MAX, strDateTime);

  len = strlen ( resultBuffer );
  nOffset = len;

  for (i=(UINT8) m_APAC_Hist_Status.num;i>=1;i--)
  {
    pStr = APACMgr_RecentAllHistoryEntry(i-1);
    len = strlen (pStr);

    memcpy ( (void *) &resultBuffer[nOffset], pStr, len );
    nOffset += len;
  }

  resultBuffer[nOffset] = NULL;
  User_OutputMsgString(resultBuffer, FALSE);

  return result;
}


/******************************************************************************
* Function:     APACMgr_RecentAllHistoryEntry
*
* Description:  Returns the selected APAC History Entry from the non-volatile memory
*
* Parameters:   [in] index_obj: Index parameter selecting specified entry
*
* Returns:      [out] ptr to RecentLogStr[] containing decoded entry
*
* Notes:        none
*
*****************************************************************************/
static CHAR *APACMgr_RecentAllHistoryEntry ( UINT8 index_obj )
{
  CHAR *pStr;
  TIMESTRUCT ts;
  static CHAR  recentLogStr[APAC_MAX_RECENT_ENTRY_LEN];
  static CHAR  recentLogStr1[APAC_MAX_RECENT_ENTRY_LEN];
  UINT8 flags;

  pStr = NULL;
  if ( index_obj < APAC_HIST_ENTRY_MAX )
  {
    CM_ConvertTimeStamptoTimeStruct( &m_APAC_Hist.entry[index_obj].ts, &ts );
    flags = m_APAC_Hist.entry[index_obj].flags;

    // Stringify the Log data
    snprintf( recentLogStr1, APAC_MAX_RECENT_ENTRY_LEN, "E%c '%c' %+1.1fC %+2.2f%% %s ",
              APAC_ENG_POS_STR[APAC_HIST_PARSE_ENG(flags)],
              APAC_CAT_STR[APAC_HIST_PARSE_CAT(flags)],
              m_APAC_Hist.entry[index_obj].ittMargin,
              m_APAC_Hist.entry[index_obj].ngMargin,
              APAC_HIST_ACT_STR_CONST[APAC_HIST_PARSE_ACT(flags)] );

    // Stringify the Log data
    snprintf( recentLogStr, APAC_MAX_RECENT_ENTRY_LEN,
              "%02d:%02d/%02d/%4d %02d:%02d:%02d  %s\r\n",
              index_obj,
              ts.Month,
              ts.Day,
              ts.Year,
              ts.Hour,
              ts.Minute,
              ts.Second,
              recentLogStr1 );

    // Return log data string
    pStr = recentLogStr;
  }

  return pStr;
}


/******************************************************************************
 * Function:    APACMgr_HistoryClear
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Clears the History from APAC Record NVM Buffer 100 recs
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
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
static
USER_HANDLER_RESULT APACMgr_HistoryClear(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_OK;

  if (APACMgr_FileInitHist())
  {
    memset ( (void *) &m_APAC_Hist_Status, 0, sizeof(m_APAC_Hist_Status) );
  }

  return result;
}


/******************************************************************************
* Function:    APACMgr_ShowConfig
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
*
* Returns:     USER_RESULT_OK:    Processed successfully
*              USER_RESULT_ERROR: Error processing command.
*
* Notes:
*****************************************************************************/
static USER_HANDLER_RESULT APACMgr_ShowConfig(USER_DATA_TYPE DataType,
                                                 USER_MSG_PARAM Param,
                                                 UINT32 Index,
                                                 const void *SetPtr,
                                                 void **GetPtr)
{
  USER_HANDLER_RESULT result;

  result = USER_RESULT_OK;

  return result;
}


/*****************************************************************************
 *  MODIFICATIONS
 *    $History: APACMgrUserTables.c $
 * 
 * *****************  Version 11  *****************
 * User: Peter Lee    Date: 1/19/16    Time: 5:12p
 * Updated in $/software/control processor/code/application
 * SCR #1308 Item #14.2. Change "apac.cfg.snsr.nr102_100" to
 * "apac.cfg.snsr.nr102"
 * 
 * *****************  Version 10  *****************
 * User: Peter Lee    Date: 1/19/16    Time: 11:37a
 * Updated in $/software/control processor/code/application
 * SCR #1309 Item #3.  Remove 'apac.cfg.snsr.gndspd'  as not needed.
 * 
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 1/18/16    Time: 5:49p
 * Updated in $/software/control processor/code/application
 * SCR #1308. Various Updates from Reviews and Enhancements
 * 
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 11/17/15   Time: 9:27a
 * Updated in $/software/control processor/code/application
 * SCR #1304 APAC Processing.  Code Review Updates.
 *
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 11/11/15   Time: 11:49a
 * Updated in $/software/control processor/code/application
 * SCR #1304 APAC Processing, update max timeout to 10 min.  Add debug cfg
 * batch mode load ability.
 *
 * *****************  Version 6  *****************
 * User: Peter Lee    Date: 11/10/15   Time: 11:30a
 * Updated in $/software/control processor/code/application
 * SCR #1304 APAC Processing. Additional Updates from GSE/LOG Review AI.
 *
 * *****************  Version 5  *****************
 * User: Peter Lee    Date: 11/05/15   Time: 6:45p
 * Updated in $/software/control processor/code/application
 * SCR #1304 APAC, ESN updates
 *
 * *****************  Version 4  *****************
 * User: Peter Lee    Date: 11/03/15   Time: 2:17p
 * Updated in $/software/control processor/code/application
 * SCR #1304 APAC Processing additional minor updates
 *
 * *****************  Version 3  *****************
 * User: Peter Lee    Date: 15-10-23   Time: 4:54p
 * Updated in $/software/control processor/code/application
 * SCR #1304 APAC Processing Additional Updates
 *
 * *****************  Version 2  *****************
 * User: Peter Lee    Date: 15-10-19   Time: 9:35p
 * Updated in $/software/control processor/code/application
 * SCR #1304 Additional APAC Processing development
 *
 * *****************  Version 1  *****************
 * User: Peter Lee    Date: 15-10-14   Time: 9:19a
 * Created in $/software/control processor/code/application
 * SCR #1304 APAC Processing Initial Check In
 *
 *****************************************************************************/
