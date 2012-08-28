#define ARINC429_USERTABLE_BODY
/******************************************************************************
            Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:        Arinc429UserTables.c

    Description: User commands related to the Arinc 429 Processing

VERSION
     $Revision: 40 $  $Date: 7/13/12 9:03a $

******************************************************************************/
#ifndef ARINC429MGR_BODY
#error Arinc429UserTables.c should only be included by Arinc429.c
#endif

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/


/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "uart.h"
#include "stddefs.h"
#include "Utility.h"
#include "Monitor.h"


/*****************************************************************************/
/* Local Defines                                                            */
/*****************************************************************************/
#define STR_LIMIT_429 0,32

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

USER_HANDLER_RESULT Arinc429Msg_RegisterRx(USER_DATA_TYPE DataType,
                                           USER_MSG_PARAM Param,
                                           UINT32 Index,
                                           const void *SetPtr,
                                           void **GetPtr);

USER_HANDLER_RESULT Arinc429Msg_RegisterTx(USER_DATA_TYPE DataType,
                                           USER_MSG_PARAM Param,
                                           UINT32 Index,
                                           const void *SetPtr,
                                           void **GetPtr);

USER_HANDLER_RESULT Arinc429Msg_StateRx(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr);

USER_HANDLER_RESULT Arinc429Msg_StateTx(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr);

USER_HANDLER_RESULT Arinc429Msg_CfgRx(USER_DATA_TYPE DataType,
                                      USER_MSG_PARAM Param,
                                      UINT32 Index,
                                      const void *SetPtr,
                                      void **GetPtr);

USER_HANDLER_RESULT Arinc429Msg_CfgTx(USER_DATA_TYPE DataType,
                                      USER_MSG_PARAM Param,
                                      UINT32 Index,
                                      const void *SetPtr,
                                      void **GetPtr);

USER_HANDLER_RESULT Arinc429Msg_Reconfigure(USER_DATA_TYPE DataType,
                                            USER_MSG_PARAM Param,
                                            UINT32 Index,
                                            const void *SetPtr,
                                            void **GetPtr);

#ifdef GENERATE_SYS_LOGS
USER_HANDLER_RESULT Arinc429Msg_CreateLogs(USER_DATA_TYPE DataType,
                                           USER_MSG_PARAM Param,
                                           UINT32 Index,
                                           const void *SetPtr,
                                           void **GetPtr);
#endif

USER_HANDLER_RESULT Arinc429Msg_ShowConfig(USER_DATA_TYPE DataType,
                                           USER_MSG_PARAM Param,
                                           UINT32 Index,
                                           const void *SetPtr,
                                           void **GetPtr);


/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
static ARINC429_RX_CFG Arinc429CfgTemp_RxChan;
static ARINC429_TX_CFG Arinc429CfgTemp_TxChan;

static ARINC429_SYS_RX_STATUS Arinc429SysStatusTemp_Rx;
static ARINC429_SYS_TX_STATUS Arinc429SysStatusTemp_Tx;

static ARINC429_DRV_RX_STATUS Arinc429DrvStatusTemp_Rx;
static ARINC429_DRV_TX_STATUS Arinc429DrvStatusTemp_Tx;

static FPGA_RX_REG fpgaRxRegTemp;
static FPGA_TX_REG fpgaTxRegTemp;

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
USER_ENUM_TBL SpeedStrs[] =
{
  {"HIGH",  1},
  {"LOW",   0},
  {NULL,    0}
};

USER_ENUM_TBL ProtocolStrs[] =
{
  {"STREAM",      ARINC429_RX_PROTOCOL_STREAM},
  {"CHANGE_ONLY", ARINC429_RX_PROTOCOL_CHANGE_ONLY},
  {"REDUCE",      ARINC429_RX_PROTOCOL_REDUCE},
  {NULL,          0}
};

USER_ENUM_TBL SubProtocolStrs[] =
{
  {"NONE",        ARINC429_RX_SUB_NONE},
  {"PW305AB_MFD", ARINC429_RX_SUB_PW305AB_MFD_REDUCE},
  {NULL,          0}
};

USER_ENUM_TBL ParityStrs[] =
{
  {"ODD",      1},
  {"EVEN",     0},
  {NULL,       0}
};

USER_ENUM_TBL SwapLabelStrsRx[] =
{
  {"SWAP",     0},
  {"NO_SWAP",  1},
  {NULL,  0}
};

USER_ENUM_TBL SwapLabelStrsTx[] =
{
  {"SWAP",     1},
  {"NO_SWAP",  0},
  {NULL,  0}
};

USER_ENUM_TBL DebugOutStrs[] =
{
   {"RAW",      0},
   {"PROTOCOL", 1},
   {NULL,       0}
};

USER_ENUM_TBL ArincSysStatusStrs[] =
{
  {"OK",ARINC429_STATUS_OK},
  {"FAULTED_PBIT",ARINC429_STATUS_FAULTED_PBIT},
  {"FAULTED_CBIT",ARINC429_STATUS_FAULTED_CBIT},
  {"DISABLED_OK",ARINC429_STATUS_DISABLED_OK},
  {"DISABLED_FAULTED",ARINC429_STATUS_DISABLED_FAULTED},
  {NULL,0}
};

#pragma ghs nowarning 1545 //Suppress packed structure alignment warning
USER_MSG_TBL Arinc429RegisterRxTbl[] =
{
  {"STATUS",        NO_NEXT_TABLE, Arinc429Msg_RegisterRx, USER_TYPE_HEX16, USER_RO, (void*)&fpgaRxRegTemp.Status,        0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL},
  {"FIFO_STATUS",   NO_NEXT_TABLE, Arinc429Msg_RegisterRx, USER_TYPE_HEX16, USER_RO, (void*)&fpgaRxRegTemp.FIFO_Status,   0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL},
  {"LSW",           NO_NEXT_TABLE, Arinc429Msg_RegisterRx, USER_TYPE_HEX16, USER_RO, (void*)&fpgaRxRegTemp.lsw,           0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL},
  {"MSW",           NO_NEXT_TABLE, Arinc429Msg_RegisterRx, USER_TYPE_HEX16, USER_RO, (void*)&fpgaRxRegTemp.msw,           0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL},
  {"CONFIGURATION", NO_NEXT_TABLE, Arinc429Msg_RegisterRx, USER_TYPE_HEX16, USER_RW, (void*)&fpgaRxRegTemp.Configuration, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL},
  {"LSW_TEST",      NO_NEXT_TABLE, Arinc429Msg_RegisterRx, USER_TYPE_HEX16, USER_WO, (void*)&fpgaRxRegTemp.lsw_test,      0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL},
  {"MSW_TEST",      NO_NEXT_TABLE, Arinc429Msg_RegisterRx, USER_TYPE_HEX16, USER_WO, (void*)&fpgaRxRegTemp.msw_test,      0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL},
  {NULL, NULL, NULL, NO_HANDLER_DATA}
};

USER_MSG_TBL Arinc429RegisterTxTbl[] =
{
  {"STATUS",        NO_NEXT_TABLE, Arinc429Msg_RegisterTx, USER_TYPE_HEX16, USER_RO, (void*)&fpgaTxRegTemp.Status,       0, FPGA_MAX_TX_CHAN-1, NO_LIMIT, NULL},
  {"FIFO_STATUS",   NO_NEXT_TABLE, Arinc429Msg_RegisterTx, USER_TYPE_HEX16, USER_RO, (void*)&fpgaTxRegTemp.FIFO_Status,  0, FPGA_MAX_TX_CHAN-1, NO_LIMIT, NULL},
  {"LSW",           NO_NEXT_TABLE, Arinc429Msg_RegisterTx, USER_TYPE_HEX16, USER_WO, (void*)&fpgaTxRegTemp.lsw,          0, FPGA_MAX_TX_CHAN-1, NO_LIMIT, NULL},
  {"MSW",           NO_NEXT_TABLE, Arinc429Msg_RegisterTx, USER_TYPE_HEX16, USER_WO, (void*)&fpgaTxRegTemp.msw,          0, FPGA_MAX_TX_CHAN-1, NO_LIMIT, NULL},
  {"CONFIGURATION", NO_NEXT_TABLE, Arinc429Msg_RegisterTx, USER_TYPE_HEX16, USER_RW, (void*)&fpgaTxRegTemp.Configuration,0, FPGA_MAX_TX_CHAN-1, NO_LIMIT, NULL},
  {"LSW_TEST",      NO_NEXT_TABLE, Arinc429Msg_RegisterTx, USER_TYPE_HEX16, USER_RO, (void*)&fpgaTxRegTemp.lsw_test,     0, FPGA_MAX_TX_CHAN-1, NO_LIMIT, NULL},
  {"MSW_TEST",      NO_NEXT_TABLE, Arinc429Msg_RegisterTx, USER_TYPE_HEX16, USER_RO, (void*)&fpgaTxRegTemp.msw_test,     0, FPGA_MAX_TX_CHAN-1, NO_LIMIT, NULL},
  {NULL, NULL, NULL, NO_HANDLER_DATA}
};

USER_MSG_TBL Arinc429StatusRxTbl[] =
{
  {"SW_BUFFER_OF_COUNT",   NO_NEXT_TABLE, Arinc429Msg_StateRx, USER_TYPE_UINT32,  USER_RO, (void*)&Arinc429SysStatusTemp_Rx.SwBuffOverFlowCnt,   0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,NULL},
  {"PARITY_ERROR_COUNT",   NO_NEXT_TABLE, Arinc429Msg_StateRx, USER_TYPE_UINT32,  USER_RO, (void*)&Arinc429SysStatusTemp_Rx.ParityErrCnt,        0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,NULL},
  {"FRAMING_ERROR_COUNT",  NO_NEXT_TABLE, Arinc429Msg_StateRx, USER_TYPE_UINT32,  USER_RO, (void*)&Arinc429SysStatusTemp_Rx.FramingErrCnt,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,NULL},
  {"FIFO_HALFFULL_COUNT",  NO_NEXT_TABLE, Arinc429Msg_StateRx, USER_TYPE_UINT32,  USER_RO, (void*)&Arinc429DrvStatusTemp_Rx.FIFO_HalfFullCnt,    0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,NULL},
  {"FIFO_FULL_COUNT",      NO_NEXT_TABLE, Arinc429Msg_StateRx, USER_TYPE_UINT32,  USER_RO, (void*)&Arinc429DrvStatusTemp_Rx.FIFO_FullCnt,        0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,NULL},
  {"FIFO_EMPTY_COUNT",     NO_NEXT_TABLE, Arinc429Msg_StateRx, USER_TYPE_UINT32,  USER_RO, (void*)&Arinc429DrvStatusTemp_Rx.FIFO_NotEmptyCnt,    0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,NULL},
  {"CHANNEL_ACTIVE",       NO_NEXT_TABLE, Arinc429Msg_StateRx, USER_TYPE_BOOLEAN, USER_RO, (void*)&Arinc429SysStatusTemp_Rx.bChanActive,         0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,NULL},
  {"LAST_ACTIVITY_TIME",   NO_NEXT_TABLE, Arinc429Msg_StateRx, USER_TYPE_UINT32,  USER_RO, (void*)&Arinc429SysStatusTemp_Rx.LastActivityTime,    0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,NULL},
  {"CHANNEL_TIMEOUT",      NO_NEXT_TABLE, Arinc429Msg_StateRx, USER_TYPE_BOOLEAN, USER_RO, (void*)&Arinc429SysStatusTemp_Rx.bChanTimeOut,        0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,NULL},
  {"CHANNEL_START_TIMEOUT",NO_NEXT_TABLE, Arinc429Msg_StateRx, USER_TYPE_BOOLEAN, USER_RO, (void*)&Arinc429SysStatusTemp_Rx.bChanStartUpTimeOut, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,NULL},
  {"DATA_LOSS_COUNT",      NO_NEXT_TABLE, Arinc429Msg_StateRx, USER_TYPE_UINT32,  USER_RO, (void*)&Arinc429SysStatusTemp_Rx.DataLossCnt,         0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,NULL},
  {"FPGA_STATUS_REG",      NO_NEXT_TABLE, Arinc429Msg_StateRx, USER_TYPE_UINT16,  USER_RO, (void*)&Arinc429SysStatusTemp_Rx.FPGA_Status,         0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,NULL},
  {"RX_COUNT",             NO_NEXT_TABLE, Arinc429Msg_StateRx, USER_TYPE_UINT32,  USER_RO, (void*)&Arinc429SysStatusTemp_Rx.RxCnt,               0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,NULL},
  {"STATUS",               NO_NEXT_TABLE, Arinc429Msg_StateRx, USER_TYPE_ENUM,    USER_RO, (void*)&Arinc429SysStatusTemp_Rx.Status,              0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,ArincSysStatusStrs},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};


USER_MSG_TBL Arinc429StatusTxTbl[] =
{
  {"FIFO_HALFFULL_COUNT", NO_NEXT_TABLE, Arinc429Msg_StateTx, USER_TYPE_UINT32, USER_RO, (void*)&Arinc429DrvStatusTemp_Tx.FIFO_HalfFullCnt, 0, FPGA_MAX_TX_CHAN-1, NO_LIMIT, NULL},
  {"FIFO_FULL_COUNT",     NO_NEXT_TABLE, Arinc429Msg_StateTx, USER_TYPE_UINT32, USER_RO, (void*)&Arinc429DrvStatusTemp_Tx.FIFO_FullCnt,     0, FPGA_MAX_TX_CHAN-1, NO_LIMIT, NULL},
  {"FIFO_EMPTY_COUNT",    NO_NEXT_TABLE, Arinc429Msg_StateTx, USER_TYPE_UINT32, USER_RO, (void*)&Arinc429DrvStatusTemp_Tx.FIFO_EmptyCnt,    0, FPGA_MAX_TX_CHAN-1, NO_LIMIT, NULL},
  {"STATUS",              NO_NEXT_TABLE, Arinc429Msg_StateTx, USER_TYPE_ENUM,   USER_RO, (void*)&Arinc429DrvStatusTemp_Tx.Status,           0, FPGA_MAX_TX_CHAN-1, NO_LIMIT, ArincSysStatusStrs},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

USER_MSG_TBL Arinc429P0Tbl[] =
{
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[0].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[0].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[0].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[0].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[0].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

USER_MSG_TBL Arinc429P1Tbl[] =
{
   { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[1].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
   { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[1].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
   { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[1].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
   { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[1].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
   { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[1].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
   {NULL,NULL,NULL,NO_HANDLER_DATA}
};

USER_MSG_TBL Arinc429P2Tbl[] =
{
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[2].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[2].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[2].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[2].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[2].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

USER_MSG_TBL Arinc429P3Tbl[] =
{
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[3].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[3].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[3].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[3].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[3].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

USER_MSG_TBL Arinc429P4Tbl[] =
{
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[4].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[4].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[4].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[4].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[4].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

USER_MSG_TBL Arinc429P5Tbl[] =
{
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[5].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[5].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[5].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[5].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[5].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

USER_MSG_TBL Arinc429P6Tbl[] =
{
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[6].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[6].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[6].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[6].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[6].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

USER_MSG_TBL Arinc429P7Tbl[] =
{
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[7].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[7].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[7].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[7].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[7].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

USER_MSG_TBL Arinc429P8Tbl[] =
{
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[8].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[8].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[8].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[8].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[8].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

USER_MSG_TBL Arinc429P9Tbl[] =
{
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[9].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[9].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[9].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[9].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[9].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

USER_MSG_TBL Arinc429P10Tbl[] =
{
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[10].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[10].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[10].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[10].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[10].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

USER_MSG_TBL Arinc429P11Tbl[] =
{
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[11].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[11].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[11].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[11].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[11].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

USER_MSG_TBL Arinc429P12Tbl[] =
{
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[12].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[12].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[12].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[12].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[12].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

USER_MSG_TBL Arinc429P13Tbl[] =
{
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[13].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[13].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[13].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[13].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[13].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

USER_MSG_TBL Arinc429P14Tbl[] =
{
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[14].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[14].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[14].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[14].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[14].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

USER_MSG_TBL Arinc429P15Tbl[] =
{
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[15].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[15].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[15].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[15].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[15].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};
                                                                                                                                                                             
USER_MSG_TBL Arinc429P16Tbl[] =                                                                                                                                              
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[16].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[16].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[16].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[16].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[16].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};
                                                                                                                                                                             
USER_MSG_TBL Arinc429P17Tbl[] =                                                                                                                                              
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[17].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[17].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[17].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[17].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[17].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};
                                                                                                                                                                             
USER_MSG_TBL Arinc429P18Tbl[] =                                                                                                                                              
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[18].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[18].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[18].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[18].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[18].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};
                                                                                                                                                                             
USER_MSG_TBL Arinc429P19Tbl[] =                                                                                                                                              
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[19].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[19].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[19].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[19].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[19].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P20Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[20].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[20].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[20].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[20].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[20].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P21Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[21].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[21].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[21].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[21].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[21].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P22Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[22].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[22].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[22].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[22].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[22].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P23Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[23].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[23].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[23].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[23].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[23].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P24Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[24].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[24].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[24].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[24].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[24].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P25Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[25].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[25].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[25].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[25].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[25].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P26Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[26].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[26].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[26].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[26].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[26].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P27Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[27].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[27].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[27].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[27].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[27].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P28Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[28].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[28].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[28].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[28].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[28].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P29Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[29].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[29].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[29].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[29].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[29].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P30Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[30].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[30].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[30].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[30].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[30].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P31Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[31].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[31].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[31].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[31].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[31].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P32Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[32].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[32].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[32].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[32].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[32].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P33Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[33].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[33].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[33].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[33].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[33].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P34Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[34].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[34].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[34].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[34].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[34].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P35Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[35].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[35].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[35].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[35].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[35].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P36Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[36].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[36].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[36].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[36].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[36].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P37Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[37].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[37].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[37].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[37].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[37].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P38Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[38].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[38].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[38].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[38].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[38].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P39Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[39].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[39].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[39].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[39].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[39].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P40Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[40].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[40].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[40].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[40].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[40].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P41Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[41].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[41].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[41].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[41].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[41].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P42Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[42].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[42].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[42].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[42].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[42].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P43Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[43].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[43].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[43].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[43].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[43].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P44Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[44].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[44].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[44].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[44].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[44].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P45Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[45].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[45].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[45].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[45].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[45].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P46Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[46].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[46].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[46].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[46].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[46].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P47Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[47].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[47].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[47].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[47].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[47].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P48Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[48].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[48].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[48].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[48].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[48].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P49Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[49].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[49].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[49].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[49].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[49].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P50Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[50].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[50].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[50].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[50].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[50].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P51Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[51].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[51].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[51].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[51].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[51].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P52Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[52].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[52].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[52].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[52].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[52].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P53Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[53].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[53].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[53].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[53].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[53].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P54Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[54].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[54].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[54].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[54].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[54].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P55Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[55].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[55].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[55].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[55].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[55].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P56Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[56].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[56].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[56].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[56].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[56].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P57Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[57].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[57].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[57].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[57].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[57].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P58Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[58].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[58].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[58].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[58].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[58].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P59Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[59].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[59].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[59].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[59].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[59].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P60Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[60].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[60].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[60].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[60].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[60].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P61Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[61].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[61].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[61].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[61].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[61].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P62Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[62].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[62].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[62].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[62].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[62].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P63Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[63].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[63].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[63].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[63].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[63].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P64Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[64].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[64].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[64].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[64].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[64].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P65Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[65].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[65].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[65].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[65].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[65].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P66Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[66].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[66].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[66].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[66].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[66].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P67Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[67].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[67].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[67].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[67].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[67].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P68Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[68].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[68].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[68].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[68].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[68].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P69Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[69].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[69].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[69].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[69].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[69].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P70Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[70].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[70].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[70].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[70].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[70].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P71Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[71].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[71].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[71].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[71].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[71].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P72Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[72].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[72].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[72].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[72].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[72].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P73Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[73].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[73].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[73].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[73].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[73].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P74Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[74].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[74].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[74].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[74].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[74].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P75Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[75].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[75].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[75].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[75].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[75].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P76Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[76].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[76].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[76].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[76].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[76].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P77Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[77].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[77].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[77].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[77].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[77].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P78Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[78].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[78].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[78].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[78].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[78].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P79Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[79].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[79].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[79].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[79].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[79].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P80Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[80].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[80].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[80].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[80].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[80].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P81Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[81].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[81].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[81].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[81].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[81].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P82Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[82].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[82].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[82].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[82].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[82].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P83Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[83].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[83].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[83].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[83].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[83].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P84Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[84].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[84].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[84].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[84].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[84].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P85Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[85].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[85].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[85].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[85].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[85].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P86Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[86].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[86].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[86].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[86].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[86].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P87Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[87].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[87].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[87].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[87].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[87].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P88Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[88].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[88].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[88].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[88].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[88].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P89Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[89].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[89].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[89].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[89].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[89].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P90Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[90].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[90].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[90].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[90].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[90].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P91Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[91].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[91].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[91].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[91].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[91].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P92Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[92].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[92].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[92].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[92].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[92].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P93Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[93].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[93].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[93].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[93].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[93].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P94Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[94].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[94].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[94].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[94].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[94].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P95Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[95].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[95].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[95].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[95].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[95].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P96Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[96].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[96].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[96].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[96].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[96].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P97Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[97].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[97].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[97].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[97].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[97].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P98Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[98].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[98].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[98].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[98].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[98].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P99Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[99].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[99].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[99].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[99].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[99].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P100Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[100].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[100].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[100].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[100].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[100].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P101Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[101].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[101].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[101].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[101].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[101].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P102Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[102].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[102].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[102].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[102].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[102].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P103Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[103].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[103].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[103].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[103].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[103].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P104Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[104].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[104].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[104].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[104].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[104].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P105Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[105].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[105].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[105].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[105].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[105].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P106Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[106].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[106].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[106].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[106].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[106].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P107Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[107].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[107].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[107].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[107].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[107].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P108Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[108].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[108].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[108].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[108].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[108].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P109Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[109].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[109].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[109].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[109].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[109].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P110Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[110].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[110].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[110].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[110].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[110].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P111Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[111].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[111].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[111].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[111].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[111].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P112Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[112].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[112].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[112].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[112].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[112].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P113Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[113].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[113].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[113].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[113].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[113].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P114Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[114].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[114].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[114].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[114].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[114].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P115Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[115].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[115].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[115].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[115].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[115].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P116Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[116].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[116].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[116].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[116].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[116].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P117Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[117].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[117].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[117].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[117].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[117].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P118Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[118].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[118].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[118].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[118].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[118].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P119Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[119].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[119].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[119].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[119].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[119].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P120Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[120].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[120].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[120].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[120].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[120].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P121Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[121].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[121].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[121].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[121].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[121].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P122Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[122].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[122].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[122].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[122].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[122].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P123Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[123].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[123].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[123].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[123].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[123].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P124Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[124].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[124].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[124].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[124].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[124].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P125Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[125].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[125].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[125].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[125].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[125].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P126Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[126].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[126].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[126].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[126].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[126].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P127Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[127].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[127].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[127].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[127].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[127].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P128Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[128].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[128].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[128].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[128].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[128].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P129Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[129].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[129].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[129].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[129].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[129].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P130Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[130].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[130].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[130].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[130].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[130].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P131Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[131].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[131].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[131].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[131].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[131].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P132Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[132].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[132].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[132].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[132].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[132].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P133Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[133].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[133].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[133].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[133].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[133].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P134Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[134].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[134].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[134].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[134].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[134].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P135Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[135].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[135].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[135].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[135].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[135].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P136Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[136].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[136].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[136].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[136].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[136].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P137Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[137].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[137].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[137].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[137].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[137].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P138Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[138].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[138].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[138].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[138].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[138].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P139Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[139].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[139].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[139].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[139].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[139].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P140Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[140].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[140].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[140].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[140].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[140].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P141Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[141].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[141].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[141].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[141].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[141].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P142Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[142].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[142].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[142].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[142].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[142].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P143Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[143].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[143].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[143].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[143].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[143].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P144Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[144].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[144].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[144].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[144].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[144].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P145Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[145].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[145].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[145].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[145].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[145].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P146Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[146].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[146].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[146].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[146].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[146].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P147Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[147].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[147].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[147].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[147].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[147].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P148Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[148].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[148].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[148].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[148].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[148].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P149Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[149].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[149].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[149].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[149].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[149].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P150Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[150].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[150].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[150].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[150].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[150].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P151Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[151].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[151].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[151].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[151].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[151].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P152Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[152].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[152].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[152].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[152].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[152].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P153Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[153].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[153].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[153].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[153].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[153].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P154Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[154].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[154].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[154].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[154].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[154].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P155Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[155].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[155].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[155].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[155].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[155].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P156Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[156].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[156].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[156].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[156].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[156].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P157Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[157].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[157].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[157].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[157].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[157].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P158Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[158].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[158].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[158].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[158].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[158].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P159Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[159].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[159].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[159].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[159].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[159].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P160Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[160].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[160].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[160].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[160].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[160].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P161Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[161].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[161].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[161].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[161].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[161].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P162Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[162].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[162].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[162].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[162].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[162].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P163Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[163].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[163].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[163].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[163].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[163].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P164Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[164].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[164].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[164].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[164].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[164].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P165Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[165].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[165].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[165].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[165].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[165].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P166Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[166].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[166].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[166].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[166].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[166].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P167Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[167].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[167].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[167].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[167].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[167].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P168Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[168].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[168].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[168].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[168].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[168].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P169Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[169].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[169].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[169].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[169].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[169].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P170Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[170].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[170].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[170].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[170].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[170].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P171Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[171].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[171].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[171].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[171].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[171].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P172Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[172].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[172].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[172].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[172].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[172].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P173Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[173].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[173].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[173].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[173].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[173].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P174Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[174].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[174].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[174].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[174].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[174].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P175Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[175].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[175].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[175].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[175].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[175].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P176Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[176].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[176].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[176].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[176].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[176].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P177Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[177].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[177].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[177].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[177].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[177].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P178Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[178].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[178].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[178].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[178].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[178].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P179Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[179].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[179].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[179].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[179].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[179].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P180Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[180].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[180].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[180].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[180].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[180].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P181Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[181].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[181].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[181].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[181].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[181].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P182Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[182].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[182].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[182].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[182].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[182].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P183Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[183].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[183].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[183].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[183].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[183].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P184Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[184].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[184].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[184].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[184].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[184].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P185Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[185].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[185].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[185].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[185].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[185].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P186Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[186].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[186].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[186].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[186].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[186].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P187Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[187].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[187].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[187].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[187].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[187].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P188Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[188].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[188].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[188].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[188].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[188].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P189Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[189].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[189].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[189].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[189].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[189].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P190Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[190].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[190].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[190].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[190].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[190].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P191Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[191].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[191].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[191].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[191].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[191].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P192Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[192].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[192].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[192].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[192].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[192].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P193Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[193].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[193].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[193].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[193].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[193].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P194Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[194].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[194].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[194].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[194].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[194].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P195Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[195].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[195].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[195].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[195].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[195].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P196Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[196].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[196].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[196].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[196].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[196].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P197Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[197].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[197].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[197].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[197].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[197].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P198Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[198].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[198].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[198].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[198].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[198].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P199Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[199].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[199].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[199].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[199].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[199].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P200Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[200].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[200].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[200].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[200].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[200].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P201Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[201].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[201].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[201].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[201].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[201].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P202Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[202].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[202].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[202].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[202].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[202].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P203Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[203].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[203].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[203].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[203].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[203].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P204Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[204].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[204].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[204].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[204].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[204].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P205Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[205].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[205].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[205].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[205].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[205].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P206Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[206].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[206].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[206].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[206].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[206].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P207Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[207].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[207].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[207].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[207].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[207].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P208Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[208].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[208].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[208].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[208].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[208].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P209Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[209].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[209].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[209].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[209].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[209].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P210Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[210].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[210].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[210].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[210].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[210].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P211Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[211].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[211].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[211].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[211].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[211].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P212Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[212].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[212].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[212].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[212].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[212].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P213Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[213].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[213].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[213].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[213].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[213].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P214Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[214].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[214].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[214].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[214].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[214].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P215Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[215].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[215].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[215].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[215].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[215].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P216Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[216].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[216].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[216].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[216].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[216].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P217Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[217].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[217].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[217].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[217].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[217].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P218Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[218].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[218].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[218].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[218].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[218].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P219Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[219].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[219].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[219].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[219].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[219].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P220Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[220].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[220].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[220].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[220].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[220].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P221Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[221].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[221].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[221].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[221].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[221].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P222Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[222].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[222].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[222].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[222].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[222].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P223Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[223].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[223].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[223].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[223].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[223].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P224Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[224].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[224].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[224].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[224].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[224].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P225Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[225].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[225].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[225].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[225].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[225].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P226Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[226].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[226].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[226].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[226].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[226].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P227Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[227].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[227].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[227].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[227].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[227].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P228Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[228].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[228].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[228].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[228].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[228].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P229Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[229].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[229].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[229].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[229].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[229].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P230Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[230].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[230].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[230].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[230].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[230].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P231Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[231].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[231].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[231].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[231].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[231].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P232Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[232].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[232].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[232].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[232].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[232].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P233Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[233].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[233].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[233].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[233].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[233].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P234Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[234].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[234].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[234].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[234].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[234].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P235Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[235].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[235].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[235].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[235].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[235].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P236Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[236].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[236].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[236].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[236].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[236].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P237Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[237].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[237].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[237].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[237].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[237].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P238Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[238].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[238].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[238].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[238].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[238].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P239Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[239].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[239].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[239].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[239].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[239].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P240Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[240].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[240].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[240].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[240].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[240].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P241Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[241].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[241].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[241].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[241].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[241].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P242Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[242].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[242].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[242].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[242].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[242].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P243Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[243].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[243].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[243].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[243].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[243].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P244Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[244].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[244].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[244].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[244].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[244].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P245Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[245].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[245].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[245].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[245].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[245].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P246Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[246].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[246].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[246].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[246].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[246].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P247Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[247].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[247].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[247].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[247].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[247].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P248Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[248].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[248].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[248].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[248].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[248].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P249Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[249].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[249].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[249].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[249].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[249].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P250Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[250].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[250].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[250].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[250].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[250].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P251Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[251].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[251].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[251].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[251].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[251].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P252Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[252].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[252].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[252].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[252].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[252].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P253Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[253].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[253].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[253].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[253].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[253].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P254Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[254].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[254].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[254].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[254].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[254].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429P255Tbl[] =
{  
  { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[255].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[255].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[255].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[255].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parameter[255].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },
  {NULL,NULL,NULL,NO_HANDLER_DATA} 
};

USER_MSG_TBL Arinc429RxParameterTbl[] =
{  { "P0",   Arinc429P0Tbl,   NULL, NO_HANDLER_DATA },
   { "P1",   Arinc429P1Tbl,   NULL, NO_HANDLER_DATA },
   { "P2",   Arinc429P2Tbl,   NULL, NO_HANDLER_DATA },
   { "P3",   Arinc429P3Tbl,   NULL, NO_HANDLER_DATA },
   { "P4",   Arinc429P4Tbl,   NULL, NO_HANDLER_DATA },
   { "P5",   Arinc429P5Tbl,   NULL, NO_HANDLER_DATA },
   { "P6",   Arinc429P6Tbl,   NULL, NO_HANDLER_DATA },
   { "P7",   Arinc429P7Tbl,   NULL, NO_HANDLER_DATA },
   { "P8",   Arinc429P8Tbl,   NULL, NO_HANDLER_DATA },
   { "P9",   Arinc429P9Tbl,   NULL, NO_HANDLER_DATA },
   { "P10",  Arinc429P10Tbl,  NULL, NO_HANDLER_DATA },
   { "P11",  Arinc429P11Tbl,  NULL, NO_HANDLER_DATA },
   { "P12",  Arinc429P12Tbl,  NULL, NO_HANDLER_DATA },
   { "P13",  Arinc429P13Tbl,  NULL, NO_HANDLER_DATA },
   { "P14",  Arinc429P14Tbl,  NULL, NO_HANDLER_DATA },
   { "P15",  Arinc429P15Tbl,  NULL, NO_HANDLER_DATA },
   { "P16",  Arinc429P16Tbl,  NULL, NO_HANDLER_DATA },
   { "P17",  Arinc429P17Tbl,  NULL, NO_HANDLER_DATA },
   { "P18",  Arinc429P18Tbl,  NULL, NO_HANDLER_DATA },
   { "P19",  Arinc429P19Tbl,  NULL, NO_HANDLER_DATA },
   { "P20",  Arinc429P20Tbl,  NULL, NO_HANDLER_DATA },
   { "P21",  Arinc429P21Tbl,  NULL, NO_HANDLER_DATA },
   { "P22",  Arinc429P22Tbl,  NULL, NO_HANDLER_DATA },
   { "P23",  Arinc429P23Tbl,  NULL, NO_HANDLER_DATA },
   { "P24",  Arinc429P24Tbl,  NULL, NO_HANDLER_DATA },
   { "P25",  Arinc429P25Tbl,  NULL, NO_HANDLER_DATA },
   { "P26",  Arinc429P26Tbl,  NULL, NO_HANDLER_DATA },
   { "P27",  Arinc429P27Tbl,  NULL, NO_HANDLER_DATA },
   { "P28",  Arinc429P28Tbl,  NULL, NO_HANDLER_DATA },
   { "P29",  Arinc429P29Tbl,  NULL, NO_HANDLER_DATA },
   { "P30",  Arinc429P30Tbl,  NULL, NO_HANDLER_DATA },
   { "P31",  Arinc429P31Tbl,  NULL, NO_HANDLER_DATA },
   { "P32",  Arinc429P32Tbl,  NULL, NO_HANDLER_DATA },
   { "P33",  Arinc429P33Tbl,  NULL, NO_HANDLER_DATA },
   { "P34",  Arinc429P34Tbl,  NULL, NO_HANDLER_DATA },
   { "P35",  Arinc429P35Tbl,  NULL, NO_HANDLER_DATA },
   { "P36",  Arinc429P36Tbl,  NULL, NO_HANDLER_DATA },
   { "P37",  Arinc429P37Tbl,  NULL, NO_HANDLER_DATA },
   { "P38",  Arinc429P38Tbl,  NULL, NO_HANDLER_DATA },
   { "P39",  Arinc429P39Tbl,  NULL, NO_HANDLER_DATA },
   { "P40",  Arinc429P40Tbl,  NULL, NO_HANDLER_DATA },
   { "P41",  Arinc429P41Tbl,  NULL, NO_HANDLER_DATA },
   { "P42",  Arinc429P42Tbl,  NULL, NO_HANDLER_DATA },
   { "P43",  Arinc429P43Tbl,  NULL, NO_HANDLER_DATA },
   { "P44",  Arinc429P44Tbl,  NULL, NO_HANDLER_DATA },
   { "P45",  Arinc429P45Tbl,  NULL, NO_HANDLER_DATA },
   { "P46",  Arinc429P46Tbl,  NULL, NO_HANDLER_DATA },
   { "P47",  Arinc429P47Tbl,  NULL, NO_HANDLER_DATA },
   { "P48",  Arinc429P48Tbl,  NULL, NO_HANDLER_DATA },
   { "P49",  Arinc429P49Tbl,  NULL, NO_HANDLER_DATA },
   { "P50",  Arinc429P50Tbl,  NULL, NO_HANDLER_DATA },
   { "P51",  Arinc429P51Tbl,  NULL, NO_HANDLER_DATA },
   { "P52",  Arinc429P52Tbl,  NULL, NO_HANDLER_DATA },
   { "P53",  Arinc429P53Tbl,  NULL, NO_HANDLER_DATA },
   { "P54",  Arinc429P54Tbl,  NULL, NO_HANDLER_DATA },
   { "P55",  Arinc429P55Tbl,  NULL, NO_HANDLER_DATA },
   { "P56",  Arinc429P56Tbl,  NULL, NO_HANDLER_DATA },
   { "P57",  Arinc429P57Tbl,  NULL, NO_HANDLER_DATA },
   { "P58",  Arinc429P58Tbl,  NULL, NO_HANDLER_DATA },
   { "P59",  Arinc429P59Tbl,  NULL, NO_HANDLER_DATA },
   { "P60",  Arinc429P60Tbl,  NULL, NO_HANDLER_DATA },
   { "P61",  Arinc429P61Tbl,  NULL, NO_HANDLER_DATA },
   { "P62",  Arinc429P62Tbl,  NULL, NO_HANDLER_DATA },
   { "P63",  Arinc429P63Tbl,  NULL, NO_HANDLER_DATA },
   { "P64",  Arinc429P64Tbl,  NULL, NO_HANDLER_DATA },
   { "P65",  Arinc429P65Tbl,  NULL, NO_HANDLER_DATA },
   { "P66",  Arinc429P66Tbl,  NULL, NO_HANDLER_DATA },
   { "P67",  Arinc429P67Tbl,  NULL, NO_HANDLER_DATA },
   { "P68",  Arinc429P68Tbl,  NULL, NO_HANDLER_DATA },
   { "P69",  Arinc429P69Tbl,  NULL, NO_HANDLER_DATA },
   { "P70",  Arinc429P70Tbl,  NULL, NO_HANDLER_DATA },
   { "P71",  Arinc429P71Tbl,  NULL, NO_HANDLER_DATA },
   { "P72",  Arinc429P72Tbl,  NULL, NO_HANDLER_DATA },
   { "P73",  Arinc429P73Tbl,  NULL, NO_HANDLER_DATA },
   { "P74",  Arinc429P74Tbl,  NULL, NO_HANDLER_DATA },
   { "P75",  Arinc429P75Tbl,  NULL, NO_HANDLER_DATA },
   { "P76",  Arinc429P76Tbl,  NULL, NO_HANDLER_DATA },
   { "P77",  Arinc429P77Tbl,  NULL, NO_HANDLER_DATA },
   { "P78",  Arinc429P78Tbl,  NULL, NO_HANDLER_DATA },
   { "P79",  Arinc429P79Tbl,  NULL, NO_HANDLER_DATA },
   { "P80",  Arinc429P80Tbl,  NULL, NO_HANDLER_DATA },
   { "P81",  Arinc429P81Tbl,  NULL, NO_HANDLER_DATA },
   { "P82",  Arinc429P82Tbl,  NULL, NO_HANDLER_DATA },
   { "P83",  Arinc429P83Tbl,  NULL, NO_HANDLER_DATA },
   { "P84",  Arinc429P84Tbl,  NULL, NO_HANDLER_DATA },
   { "P85",  Arinc429P85Tbl,  NULL, NO_HANDLER_DATA },
   { "P86",  Arinc429P86Tbl,  NULL, NO_HANDLER_DATA },
   { "P87",  Arinc429P87Tbl,  NULL, NO_HANDLER_DATA },
   { "P88",  Arinc429P88Tbl,  NULL, NO_HANDLER_DATA },
   { "P89",  Arinc429P89Tbl,  NULL, NO_HANDLER_DATA },
   { "P90",  Arinc429P90Tbl,  NULL, NO_HANDLER_DATA },
   { "P91",  Arinc429P91Tbl,  NULL, NO_HANDLER_DATA },
   { "P92",  Arinc429P92Tbl,  NULL, NO_HANDLER_DATA },
   { "P93",  Arinc429P93Tbl,  NULL, NO_HANDLER_DATA },
   { "P94",  Arinc429P94Tbl,  NULL, NO_HANDLER_DATA },
   { "P95",  Arinc429P95Tbl,  NULL, NO_HANDLER_DATA },
   { "P96",  Arinc429P96Tbl,  NULL, NO_HANDLER_DATA },
   { "P97",  Arinc429P97Tbl,  NULL, NO_HANDLER_DATA },
   { "P98",  Arinc429P98Tbl,  NULL, NO_HANDLER_DATA },
   { "P99",  Arinc429P99Tbl,  NULL, NO_HANDLER_DATA },
   { "P100", Arinc429P100Tbl, NULL, NO_HANDLER_DATA },
   { "P101", Arinc429P101Tbl, NULL, NO_HANDLER_DATA },
   { "P102", Arinc429P102Tbl, NULL, NO_HANDLER_DATA },
   { "P103", Arinc429P103Tbl, NULL, NO_HANDLER_DATA },
   { "P104", Arinc429P104Tbl, NULL, NO_HANDLER_DATA },
   { "P105", Arinc429P105Tbl, NULL, NO_HANDLER_DATA },
   { "P106", Arinc429P106Tbl, NULL, NO_HANDLER_DATA },
   { "P107", Arinc429P107Tbl, NULL, NO_HANDLER_DATA },
   { "P108", Arinc429P108Tbl, NULL, NO_HANDLER_DATA },
   { "P109", Arinc429P109Tbl, NULL, NO_HANDLER_DATA },
   { "P110", Arinc429P110Tbl, NULL, NO_HANDLER_DATA },
   { "P111", Arinc429P111Tbl, NULL, NO_HANDLER_DATA },
   { "P112", Arinc429P112Tbl, NULL, NO_HANDLER_DATA },
   { "P113", Arinc429P113Tbl, NULL, NO_HANDLER_DATA },
   { "P114", Arinc429P114Tbl, NULL, NO_HANDLER_DATA },
   { "P115", Arinc429P115Tbl, NULL, NO_HANDLER_DATA },
   { "P116", Arinc429P116Tbl, NULL, NO_HANDLER_DATA },
   { "P117", Arinc429P117Tbl, NULL, NO_HANDLER_DATA },
   { "P118", Arinc429P118Tbl, NULL, NO_HANDLER_DATA },
   { "P119", Arinc429P119Tbl, NULL, NO_HANDLER_DATA },
   { "P120", Arinc429P120Tbl, NULL, NO_HANDLER_DATA },
   { "P121", Arinc429P121Tbl, NULL, NO_HANDLER_DATA },
   { "P122", Arinc429P122Tbl, NULL, NO_HANDLER_DATA },
   { "P123", Arinc429P123Tbl, NULL, NO_HANDLER_DATA },
   { "P124", Arinc429P124Tbl, NULL, NO_HANDLER_DATA },
   { "P125", Arinc429P125Tbl, NULL, NO_HANDLER_DATA },
   { "P126", Arinc429P126Tbl, NULL, NO_HANDLER_DATA },
   { "P127", Arinc429P127Tbl, NULL, NO_HANDLER_DATA },
   { "P128", Arinc429P128Tbl, NULL, NO_HANDLER_DATA },
   { "P129", Arinc429P129Tbl, NULL, NO_HANDLER_DATA },
   { "P130", Arinc429P130Tbl, NULL, NO_HANDLER_DATA },
   { "P131", Arinc429P131Tbl, NULL, NO_HANDLER_DATA },
   { "P132", Arinc429P132Tbl, NULL, NO_HANDLER_DATA },
   { "P133", Arinc429P133Tbl, NULL, NO_HANDLER_DATA },
   { "P134", Arinc429P134Tbl, NULL, NO_HANDLER_DATA },
   { "P135", Arinc429P135Tbl, NULL, NO_HANDLER_DATA },
   { "P136", Arinc429P136Tbl, NULL, NO_HANDLER_DATA },
   { "P137", Arinc429P137Tbl, NULL, NO_HANDLER_DATA },
   { "P138", Arinc429P138Tbl, NULL, NO_HANDLER_DATA },
   { "P139", Arinc429P139Tbl, NULL, NO_HANDLER_DATA },
   { "P140", Arinc429P140Tbl, NULL, NO_HANDLER_DATA },
   { "P141", Arinc429P141Tbl, NULL, NO_HANDLER_DATA },
   { "P142", Arinc429P142Tbl, NULL, NO_HANDLER_DATA },
   { "P143", Arinc429P143Tbl, NULL, NO_HANDLER_DATA },
   { "P144", Arinc429P144Tbl, NULL, NO_HANDLER_DATA },
   { "P145", Arinc429P145Tbl, NULL, NO_HANDLER_DATA },
   { "P146", Arinc429P146Tbl, NULL, NO_HANDLER_DATA },
   { "P147", Arinc429P147Tbl, NULL, NO_HANDLER_DATA },
   { "P148", Arinc429P148Tbl, NULL, NO_HANDLER_DATA },
   { "P149", Arinc429P149Tbl, NULL, NO_HANDLER_DATA },
   { "P150", Arinc429P150Tbl, NULL, NO_HANDLER_DATA },
   { "P151", Arinc429P151Tbl, NULL, NO_HANDLER_DATA },
   { "P152", Arinc429P152Tbl, NULL, NO_HANDLER_DATA },
   { "P153", Arinc429P153Tbl, NULL, NO_HANDLER_DATA },
   { "P154", Arinc429P154Tbl, NULL, NO_HANDLER_DATA },
   { "P155", Arinc429P155Tbl, NULL, NO_HANDLER_DATA },
   { "P156", Arinc429P156Tbl, NULL, NO_HANDLER_DATA },
   { "P157", Arinc429P157Tbl, NULL, NO_HANDLER_DATA },
   { "P158", Arinc429P158Tbl, NULL, NO_HANDLER_DATA },
   { "P159", Arinc429P159Tbl, NULL, NO_HANDLER_DATA },
   { "P160", Arinc429P160Tbl, NULL, NO_HANDLER_DATA },
   { "P161", Arinc429P161Tbl, NULL, NO_HANDLER_DATA },
   { "P162", Arinc429P162Tbl, NULL, NO_HANDLER_DATA },
   { "P163", Arinc429P163Tbl, NULL, NO_HANDLER_DATA },
   { "P164", Arinc429P164Tbl, NULL, NO_HANDLER_DATA },
   { "P165", Arinc429P165Tbl, NULL, NO_HANDLER_DATA },
   { "P166", Arinc429P166Tbl, NULL, NO_HANDLER_DATA },
   { "P167", Arinc429P167Tbl, NULL, NO_HANDLER_DATA },
   { "P168", Arinc429P168Tbl, NULL, NO_HANDLER_DATA },
   { "P169", Arinc429P169Tbl, NULL, NO_HANDLER_DATA },
   { "P170", Arinc429P170Tbl, NULL, NO_HANDLER_DATA },
   { "P171", Arinc429P171Tbl, NULL, NO_HANDLER_DATA },
   { "P172", Arinc429P172Tbl, NULL, NO_HANDLER_DATA },
   { "P173", Arinc429P173Tbl, NULL, NO_HANDLER_DATA },
   { "P174", Arinc429P174Tbl, NULL, NO_HANDLER_DATA },
   { "P175", Arinc429P175Tbl, NULL, NO_HANDLER_DATA },
   { "P176", Arinc429P176Tbl, NULL, NO_HANDLER_DATA },
   { "P177", Arinc429P177Tbl, NULL, NO_HANDLER_DATA },
   { "P178", Arinc429P178Tbl, NULL, NO_HANDLER_DATA },
   { "P179", Arinc429P179Tbl, NULL, NO_HANDLER_DATA },
   { "P180", Arinc429P180Tbl, NULL, NO_HANDLER_DATA },
   { "P181", Arinc429P181Tbl, NULL, NO_HANDLER_DATA },
   { "P182", Arinc429P182Tbl, NULL, NO_HANDLER_DATA },
   { "P183", Arinc429P183Tbl, NULL, NO_HANDLER_DATA },
   { "P184", Arinc429P184Tbl, NULL, NO_HANDLER_DATA },
   { "P185", Arinc429P185Tbl, NULL, NO_HANDLER_DATA },
   { "P186", Arinc429P186Tbl, NULL, NO_HANDLER_DATA },
   { "P187", Arinc429P187Tbl, NULL, NO_HANDLER_DATA },
   { "P188", Arinc429P188Tbl, NULL, NO_HANDLER_DATA },
   { "P189", Arinc429P189Tbl, NULL, NO_HANDLER_DATA },
   { "P190", Arinc429P190Tbl, NULL, NO_HANDLER_DATA },
   { "P191", Arinc429P191Tbl, NULL, NO_HANDLER_DATA },
   { "P192", Arinc429P192Tbl, NULL, NO_HANDLER_DATA },
   { "P193", Arinc429P193Tbl, NULL, NO_HANDLER_DATA },
   { "P194", Arinc429P194Tbl, NULL, NO_HANDLER_DATA },
   { "P195", Arinc429P195Tbl, NULL, NO_HANDLER_DATA },
   { "P196", Arinc429P196Tbl, NULL, NO_HANDLER_DATA },
   { "P197", Arinc429P197Tbl, NULL, NO_HANDLER_DATA },
   { "P198", Arinc429P198Tbl, NULL, NO_HANDLER_DATA },
   { "P199", Arinc429P199Tbl, NULL, NO_HANDLER_DATA },
   { "P200", Arinc429P200Tbl, NULL, NO_HANDLER_DATA },
   { "P201", Arinc429P201Tbl, NULL, NO_HANDLER_DATA },
   { "P202", Arinc429P202Tbl, NULL, NO_HANDLER_DATA },
   { "P203", Arinc429P203Tbl, NULL, NO_HANDLER_DATA },
   { "P204", Arinc429P204Tbl, NULL, NO_HANDLER_DATA },
   { "P205", Arinc429P205Tbl, NULL, NO_HANDLER_DATA },
   { "P206", Arinc429P206Tbl, NULL, NO_HANDLER_DATA },
   { "P207", Arinc429P207Tbl, NULL, NO_HANDLER_DATA },
   { "P208", Arinc429P208Tbl, NULL, NO_HANDLER_DATA },
   { "P209", Arinc429P209Tbl, NULL, NO_HANDLER_DATA },
   { "P210", Arinc429P210Tbl, NULL, NO_HANDLER_DATA },
   { "P211", Arinc429P211Tbl, NULL, NO_HANDLER_DATA },
   { "P212", Arinc429P212Tbl, NULL, NO_HANDLER_DATA },
   { "P213", Arinc429P213Tbl, NULL, NO_HANDLER_DATA },
   { "P214", Arinc429P214Tbl, NULL, NO_HANDLER_DATA },
   { "P215", Arinc429P215Tbl, NULL, NO_HANDLER_DATA },
   { "P216", Arinc429P216Tbl, NULL, NO_HANDLER_DATA },
   { "P217", Arinc429P217Tbl, NULL, NO_HANDLER_DATA },
   { "P218", Arinc429P218Tbl, NULL, NO_HANDLER_DATA },
   { "P219", Arinc429P219Tbl, NULL, NO_HANDLER_DATA },
   { "P220", Arinc429P220Tbl, NULL, NO_HANDLER_DATA },
   { "P221", Arinc429P221Tbl, NULL, NO_HANDLER_DATA },
   { "P222", Arinc429P222Tbl, NULL, NO_HANDLER_DATA },
   { "P223", Arinc429P223Tbl, NULL, NO_HANDLER_DATA },
   { "P224", Arinc429P224Tbl, NULL, NO_HANDLER_DATA },
   { "P225", Arinc429P225Tbl, NULL, NO_HANDLER_DATA },
   { "P226", Arinc429P226Tbl, NULL, NO_HANDLER_DATA },
   { "P227", Arinc429P227Tbl, NULL, NO_HANDLER_DATA },
   { "P228", Arinc429P228Tbl, NULL, NO_HANDLER_DATA },
   { "P229", Arinc429P229Tbl, NULL, NO_HANDLER_DATA },
   { "P230", Arinc429P230Tbl, NULL, NO_HANDLER_DATA },
   { "P231", Arinc429P231Tbl, NULL, NO_HANDLER_DATA },
   { "P232", Arinc429P232Tbl, NULL, NO_HANDLER_DATA },
   { "P233", Arinc429P233Tbl, NULL, NO_HANDLER_DATA },
   { "P234", Arinc429P234Tbl, NULL, NO_HANDLER_DATA },
   { "P235", Arinc429P235Tbl, NULL, NO_HANDLER_DATA },
   { "P236", Arinc429P236Tbl, NULL, NO_HANDLER_DATA },
   { "P237", Arinc429P237Tbl, NULL, NO_HANDLER_DATA },
   { "P238", Arinc429P238Tbl, NULL, NO_HANDLER_DATA },
   { "P239", Arinc429P239Tbl, NULL, NO_HANDLER_DATA },
   { "P240", Arinc429P240Tbl, NULL, NO_HANDLER_DATA },
   { "P241", Arinc429P241Tbl, NULL, NO_HANDLER_DATA },
   { "P242", Arinc429P242Tbl, NULL, NO_HANDLER_DATA },
   { "P243", Arinc429P243Tbl, NULL, NO_HANDLER_DATA },
   { "P244", Arinc429P244Tbl, NULL, NO_HANDLER_DATA },
   { "P245", Arinc429P245Tbl, NULL, NO_HANDLER_DATA },
   { "P246", Arinc429P246Tbl, NULL, NO_HANDLER_DATA },
   { "P247", Arinc429P247Tbl, NULL, NO_HANDLER_DATA },
   { "P248", Arinc429P248Tbl, NULL, NO_HANDLER_DATA },
   { "P249", Arinc429P249Tbl, NULL, NO_HANDLER_DATA },
   { "P250", Arinc429P250Tbl, NULL, NO_HANDLER_DATA },
   { "P251", Arinc429P251Tbl, NULL, NO_HANDLER_DATA },
   { "P252", Arinc429P252Tbl, NULL, NO_HANDLER_DATA },
   { "P253", Arinc429P253Tbl, NULL, NO_HANDLER_DATA },
   { "P254", Arinc429P254Tbl, NULL, NO_HANDLER_DATA },
   { "P255", Arinc429P255Tbl, NULL, NO_HANDLER_DATA },
   {NULL,NULL,NULL,NO_HANDLER_DATA}
};

USER_MSG_TBL Arinc429CfgRxTbl[] =
{
  {"NAME",             NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_STR,     USER_RW, (void*)&Arinc429CfgTemp_RxChan.Name,             0, FPGA_MAX_RX_CHAN-1, STR_LIMIT_429, NULL},
  {"SPEED",            NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_ENUM,    USER_RW, (void*)&Arinc429CfgTemp_RxChan.Speed,            0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      SpeedStrs},
  {"PROTOCOL",         NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_ENUM,    USER_RW, (void*)&Arinc429CfgTemp_RxChan.Protocol,         0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      ProtocolStrs},
  {"SUBPROTOCOL",      NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_ENUM,    USER_RW, (void*)&Arinc429CfgTemp_RxChan.SubProtocol,      0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      SubProtocolStrs},
  {"PARAMETER",        Arinc429RxParameterTbl, NULL, NO_HANDLER_DATA},
  {"CHANNELSTARTUP_S", NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32,  USER_RW, (void*)&Arinc429CfgTemp_RxChan.ChannelStartup_s, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      NULL},
  {"CHANNELTIMEOUT_S", NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32,  USER_RW, (void*)&Arinc429CfgTemp_RxChan.ChannelTimeOut_s, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      NULL},
  {"CHANNELSYSCOND",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_ENUM,    USER_RW, (void*)&Arinc429CfgTemp_RxChan.ChannelSysCond,   0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      Flt_UserEnumStatus},
  {"PBITSYSCOND",      NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_ENUM,    USER_RW, (void*)&Arinc429CfgTemp_RxChan.PBITSysCond,      0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      Flt_UserEnumStatus},
  {"PARITY",           NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_ENUM,    USER_RW, (void*)&Arinc429CfgTemp_RxChan.Parity,           0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      ParityStrs},
  {"SWAPLABELBITS",    NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_ENUM,    USER_RW, (void*)&Arinc429CfgTemp_RxChan.LabelSwapBits,    0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      SwapLabelStrsRx},
  {"FILTER0",          NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_HEX32,   USER_RW, (void*)&Arinc429CfgTemp_RxChan.FilterArray[0],   0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      NULL},
  {"FILTER1",          NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_HEX32,   USER_RW, (void*)&Arinc429CfgTemp_RxChan.FilterArray[1],   0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      NULL},
  {"FILTER2",          NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_HEX32,   USER_RW, (void*)&Arinc429CfgTemp_RxChan.FilterArray[2],   0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      NULL},
  {"FILTER3",          NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_HEX32,   USER_RW, (void*)&Arinc429CfgTemp_RxChan.FilterArray[3],   0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      NULL},
  {"FILTER4",          NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_HEX32,   USER_RW, (void*)&Arinc429CfgTemp_RxChan.FilterArray[4],   0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      NULL},
  {"FILTER5",          NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_HEX32,   USER_RW, (void*)&Arinc429CfgTemp_RxChan.FilterArray[5],   0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      NULL},
  {"FILTER6",          NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_HEX32,   USER_RW, (void*)&Arinc429CfgTemp_RxChan.FilterArray[6],   0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      NULL},
  {"FILTER7",          NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_HEX32,   USER_RW, (void*)&Arinc429CfgTemp_RxChan.FilterArray[7],   0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      NULL},
  {"ENABLE",           NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_BOOLEAN, USER_RW, (void*)&Arinc429CfgTemp_RxChan.Enable,           0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      NULL},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};


USER_MSG_TBL Arinc429CfgTxTbl[] =
{
  {"NAME",          NO_NEXT_TABLE, Arinc429Msg_CfgTx, USER_TYPE_STR,     USER_RW, (void*)&Arinc429CfgTemp_TxChan.Name,          0, FPGA_MAX_TX_CHAN-1, STR_LIMIT_429, NULL},
  {"SPEED",         NO_NEXT_TABLE, Arinc429Msg_CfgTx, USER_TYPE_ENUM,    USER_RW, (void*)&Arinc429CfgTemp_TxChan.Speed,         0, FPGA_MAX_TX_CHAN-1, NO_LIMIT,      SpeedStrs},
  {"PARITY",        NO_NEXT_TABLE, Arinc429Msg_CfgTx, USER_TYPE_ENUM,    USER_RW, (void*)&Arinc429CfgTemp_TxChan.Parity,        0, FPGA_MAX_TX_CHAN-1, NO_LIMIT,      ParityStrs},
  {"ENABLE",        NO_NEXT_TABLE, Arinc429Msg_CfgTx, USER_TYPE_BOOLEAN, USER_RW, (void*)&Arinc429CfgTemp_TxChan.Enable,        0, FPGA_MAX_TX_CHAN-1, NO_LIMIT,      NULL},
  {"SWAPLABELBITS", NO_NEXT_TABLE, Arinc429Msg_CfgTx, USER_TYPE_ENUM,    USER_RW, (void*)&Arinc429CfgTemp_TxChan.LabelSwapBits, 0, FPGA_MAX_TX_CHAN-1, NO_LIMIT,      SwapLabelStrsTx},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

USER_MSG_TBL Arinc429DebugTbl[] =
{
  {"RAWFILTERED", NO_NEXT_TABLE, User_GenericAccessor, USER_TYPE_BOOLEAN,(USER_RW|USER_GSE), (void*)&m_Arinc429_Debug.bOutputRawFilteredBuff,     -1,-1, NO_LIMIT, NULL},
  {"FORMATTED",   NO_NEXT_TABLE, User_GenericAccessor, USER_TYPE_BOOLEAN,(USER_RW|USER_GSE), (void*)&m_Arinc429_Debug.bOutputRawFilteredFormatted,-1,-1, NO_LIMIT, NULL},
  {"ALLCHAN",     NO_NEXT_TABLE, User_GenericAccessor, USER_TYPE_BOOLEAN,(USER_RW|USER_GSE), (void*)&m_Arinc429_Debug.bOutputMultipleArincChan,   -1,-1, NO_LIMIT, NULL},
  {"TYPE",        NO_NEXT_TABLE, User_GenericAccessor, USER_TYPE_ENUM,   (USER_RW|USER_GSE), &m_Arinc429_Debug.OutputType,                        -1,-1, NO_LIMIT, DebugOutStrs },
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

USER_MSG_TBL Arinc429RegisterTbl[] =
{
  {"RX",      Arinc429RegisterRxTbl, NULL, NO_HANDLER_DATA},
  {"TX",      Arinc429RegisterTxTbl, NULL, NO_HANDLER_DATA},
  {"IMR1",    NO_NEXT_TABLE, User_GenericAccessor, USER_TYPE_HEX16, USER_RO, (void*)FPGA_IMR1,                 -1,-1, NO_LIMIT, NULL},
  {"IMR2",    NO_NEXT_TABLE, User_GenericAccessor, USER_TYPE_HEX16, USER_RO, (void*)FPGA_IMR2,                 -1,-1, NO_LIMIT, NULL},
  {"FPGA_VER",NO_NEXT_TABLE, User_GenericAccessor, USER_TYPE_HEX16, USER_RO, (void*)FPGA_VER,                  -1,-1, NO_LIMIT, NULL},
  {"CCR",     NO_NEXT_TABLE, User_GenericAccessor, USER_TYPE_HEX16, USER_RO, (void*)FPGA_429_COMMON_CONTROL,   -1,-1, NO_LIMIT, NULL},
  {"LCR",     NO_NEXT_TABLE, User_GenericAccessor, USER_TYPE_HEX16, USER_RO, (void*)FPGA_429_LOOPBACK_CONTROL, -1,-1, NO_LIMIT, NULL},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

USER_MSG_TBL Arinc429StatusTbl[] =
{
   {"RX", Arinc429StatusRxTbl, NULL, NO_HANDLER_DATA},
   {"TX", Arinc429StatusTxTbl, NULL, NO_HANDLER_DATA},
   {"INTERRUPT_COUNT", NO_NEXT_TABLE, User_GenericAccessor, USER_TYPE_UINT32, USER_RO, (void*)&m_Arinc429MgrBlock.InterruptCnt, -1,-1, NO_LIMIT, NULL},
   {NULL, NULL, NULL, NO_HANDLER_DATA}
};

USER_MSG_TBL Arinc429CfgTbl[] =
{
  {"RX", Arinc429CfgRxTbl, NULL, NO_HANDLER_DATA},
  {"TX", Arinc429CfgTxTbl, NULL, NO_HANDLER_DATA},
  {NULL, NULL, NULL, NO_HANDLER_DATA}
};


USER_MSG_TBL Arinc429Root[] =
{
  {"REGISTER",    Arinc429RegisterTbl,NULL, NO_HANDLER_DATA},
  {"STATUS",      Arinc429StatusTbl,  NULL, NO_HANDLER_DATA},
  {"CFG",         Arinc429CfgTbl,     NULL, NO_HANDLER_DATA},
  {"DEBUG",       Arinc429DebugTbl,   NULL, NO_HANDLER_DATA},
  {"RECONFIGURE", NO_NEXT_TABLE,  Arinc429Msg_Reconfigure, USER_TYPE_ACTION, USER_RO,               NULL, -1,-1, NO_LIMIT, NULL},
#ifdef GENERATE_SYS_LOGS
  {"CREATELOGS",  NO_NEXT_TABLE,  Arinc429Msg_CreateLogs,  USER_TYPE_ACTION, USER_RO,               NULL, -1,-1, NULL},
#endif
  {DISPLAY_CFG,   NO_NEXT_TABLE,  Arinc429Msg_ShowConfig,  USER_TYPE_ACTION, (USER_RO|USER_NO_LOG), NULL, -1,-1, NO_LIMIT, NULL},
  {NULL, NULL, NULL, NO_HANDLER_DATA}
};

USER_MSG_TBL Arinc429RootTblPtr = {"ARINC429", Arinc429Root, NULL, NO_HANDLER_DATA};

#pragma ghs endnowarning

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
 * Function:    Arinc429Msg_RegisterRx
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retreives the latest register values from the Arinc429 driver and
 *              returns the result the user module
 *
 * Parameters:
 *              [in] Param indicates what register to get
 *              [out]GetPtr Writes the register value to the
 *                          integer pointed to by User.c
 *
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not set.
 *
 * Notes:
 *
*****************************************************************************/
USER_HANDLER_RESULT Arinc429Msg_RegisterRx(USER_DATA_TYPE DataType,
                                           USER_MSG_PARAM Param,
                                           UINT32 Index,
                                           const void *SetPtr,
                                           void **GetPtr)
{
  memcpy ( &fpgaRxRegTemp, &fpgaRxReg[Index], sizeof(FPGA_RX_REG) );

  if(SetPtr == NULL)
  {
    //Set the GetPtr to the Cfg member referenced in the table
    *GetPtr = (void *) *(UINT32 *) Param.Ptr;
  }
  else
  {
    *(UINT16 *) (*(UINT32 *)Param.Ptr) = *(UINT16 *)SetPtr;
  }

  return USER_RESULT_OK;
}


/******************************************************************************
 * Function:    Arinc429Msg_RegisterTx
 *
 * Description: Called by the User.c module from the reference to this function
 *              in the user message tables above.
 *              Retreives the latest register values from the Arinc429 driver and
 *              returns the result the user module
 *
 * Parameters:
 *              [in] Param indicates what register to get
 *              [out]GetPtr Writes the register value to the
 *                   integer pointed to by User.c
 *
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not set.
 *
 * Notes:       none
 *
*****************************************************************************/
USER_HANDLER_RESULT Arinc429Msg_RegisterTx(USER_DATA_TYPE DataType,
                                           USER_MSG_PARAM Param,
                                           UINT32 Index,
                                           const void *SetPtr,
                                           void **GetPtr)
{
  // Can use _v9 or _v12 as the register addr are identical
  memcpy ( &fpgaTxRegTemp, &fpgaTxReg_v12[Index], sizeof(FPGA_TX_REG) );

  if(SetPtr == NULL)
  {
    //Set the GetPtr to the Cfg member referenced in the table
    *GetPtr = (void *) *(UINT32 *) Param.Ptr;
  }
  else
  {
    *(UINT16 *) (*(UINT32 *)Param.Ptr) = *(UINT16 *)SetPtr;
  }

  return USER_RESULT_OK;
}


/******************************************************************************
 * Function:    Arinc429Msg_StateRx
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves the latest Arinc429 State Data
 *
 * Parameters:
 *                       [in] Param indicates what register to get
 *                       [out]GetPtr Writes the register value to the
 *                                             integer pointed to by User.c
 *
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
USER_HANDLER_RESULT Arinc429Msg_StateRx(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr)
{
  USER_HANDLER_RESULT result ;

  result = USER_RESULT_OK;

  memcpy ( &Arinc429SysStatusTemp_Rx,
           &m_Arinc429MgrBlock.Rx[Index],
           sizeof(ARINC429_SYS_RX_STATUS) );

  Arinc429DrvStatusTemp_Rx = *(Arinc429DrvRxGetCounts(Index));

  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);

  return result;
}


/******************************************************************************
 * Function:    Arinc429Msg_StateTx
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves the latest Arinc429 State Data
 *
 * Parameters:
 *                       [in] Param indicates what register to get
 *                       [out]GetPtr Writes the register value to the
 *                                             integer pointed to by User.c
 *
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:       All Status cmds should be RO
 *
*****************************************************************************/
USER_HANDLER_RESULT Arinc429Msg_StateTx(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr)
{
  USER_HANDLER_RESULT result ;

  result = USER_RESULT_OK;

  memcpy ( &Arinc429SysStatusTemp_Tx,
           &m_Arinc429MgrBlock.Tx[Index],
           sizeof(ARINC429_SYS_TX_STATUS) );

  Arinc429DrvStatusTemp_Tx = *(Arinc429DrvTxGetCounts(Index));

  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);

  return result;
}

/******************************************************************************
 * Function:    Arinc429Msg_CfgRx
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retreives the current Arinc429 Cfg Data
 *
 * Parameters:
 *                       [in] Param indicates what register to get
 *                       [out]GetPtr Writes the register value to the
 *                                             integer pointed to by User.c
 *
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
USER_HANDLER_RESULT Arinc429Msg_CfgRx(USER_DATA_TYPE DataType,
                                      USER_MSG_PARAM Param,
                                      UINT32 Index,
                                      const void *SetPtr,
                                      void **GetPtr)
{
  USER_HANDLER_RESULT result;


  result = USER_RESULT_OK;

  memcpy(&Arinc429CfgTemp_RxChan, &m_Arinc429Cfg.RxChan[Index],
         sizeof( ARINC429_RX_CFG ) );

  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    memcpy( &m_Arinc429Cfg.RxChan[Index], &Arinc429CfgTemp_RxChan,
      sizeof( ARINC429_RX_CFG ) );
    memcpy(&CfgMgr_ConfigPtr()->Arinc429Config,
      &m_Arinc429Cfg,
      sizeof(m_Arinc429Cfg));
    //Store the modified temporary structure in the EEPROM.
    CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
      &CfgMgr_ConfigPtr()->Arinc429Config,
      sizeof(m_Arinc429Cfg));
  }
  return result;
}


/******************************************************************************
* Function:    Arinc429Msg_CfgTx
*
* Description: Called by the User.c module from the reference to this fucntion
*              in the user message tables above.
*              Controls the A429 Tx Setup COnfiguration
*
* Parameters:
*                       [in] Param indicates what register to get
*                       [out]GetPtr Writes the register value to the
*                                             integer pointed to by User.c
*
* Returns:     USER_RESULT_OK:    Processed sucessfully
*              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
*                                 set.
*
* Notes:       none
*
*****************************************************************************/
USER_HANDLER_RESULT Arinc429Msg_CfgTx(USER_DATA_TYPE DataType,
                                      USER_MSG_PARAM Param,
                                      UINT32 Index,
                                      const void *SetPtr,
                                      void **GetPtr)
{
    USER_HANDLER_RESULT result ;

    result = USER_RESULT_OK;

    memcpy(&Arinc429CfgTemp_TxChan, &m_Arinc429Cfg.TxChan[Index],
           sizeof( ARINC429_TX_CFG ) );

    result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
    if(SetPtr != NULL && USER_RESULT_OK == result)
    {
      memcpy( &m_Arinc429Cfg.TxChan[Index], &Arinc429CfgTemp_TxChan,
        sizeof( ARINC429_TX_CFG ) );
      memcpy(&CfgMgr_ConfigPtr()->Arinc429Config,
        &m_Arinc429Cfg,
        sizeof(m_Arinc429Cfg));
      //Store the modified temporary structure in the EEPROM.
      CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
        &CfgMgr_ConfigPtr()->Arinc429Config,
        sizeof(m_Arinc429Cfg));
    }
    return result;
}

/******************************************************************************
 * Function:    Arinc429Msg_Reconfigure
 *
 * Description: Executes the reconfigure function of Arinc429.c
 *
 * Parameters:  [none]
 *
 * Returns:     USER_RESULT_OK:    Processed successfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
 *****************************************************************************/
USER_HANDLER_RESULT Arinc429Msg_Reconfigure(USER_DATA_TYPE DataType,
                                            USER_MSG_PARAM Param,
                                            UINT32 Index,
                                            const void *SetPtr,
                                            void **GetPtr)
{

  Arinc429DrvReconfigure();

  return USER_RESULT_OK;

}


/******************************************************************************
 * Function:    Arinc429Msg_CreateLogs (arinc429.createlogs)
 *
 * Description: Create all the internally QAR system logs
 *
 * Parameters:  [none]
 *
 * Returns:     USER_RESULT_OK:    Processed successfully
 *              USER_RESULT_ERROR: Error processing command.
 *
 * Notes:
 *
 *****************************************************************************/
#ifdef GENERATE_SYS_LOGS

/*vcast_dont_instrument_start*/
USER_HANDLER_RESULT Arinc429Msg_CreateLogs(USER_DATA_TYPE DataType,
                                           USER_MSG_PARAM Param,
                                           UINT32 Index,
                                           const void *SetPtr,
                                           void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_OK;

  Arinc429_CreateAllInternalLogs();

  return result;
}
/*vcast_dont_instrument_end*/


#endif


/******************************************************************************
 * Function:    Arinc429Msg_ShowConfig
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
USER_HANDLER_RESULT Arinc429Msg_ShowConfig(USER_DATA_TYPE DataType,
                                           USER_MSG_PARAM Param,
                                           UINT32 Index,
                                           const void *SetPtr,
                                           void **GetPtr)
{
   CHAR LabelStem[] = "\r\n\r\nARINC429.CFG.";
   CHAR Label[USER_MAX_MSG_STR_LEN * 3];
   //Top-level name is a single indented space
   CHAR BranchName[USER_MAX_MSG_STR_LEN] = " ";

   USER_HANDLER_RESULT result = USER_RESULT_OK;

   USER_MSG_TBL*  pCfgTable;
   INT16          channelIdx;
   INT16          MaxChannel = 0;

   pCfgTable = Arinc429CfgTbl;
   while (pCfgTable->MsgStr != NULL && result == USER_RESULT_OK)
   {
      if (strncmp(pCfgTable->MsgStr,"RX", 2) == 0)
      {
         MaxChannel =  FPGA_MAX_RX_CHAN;
      }
      else if(strncmp(pCfgTable->MsgStr,"TX", 2) == 0)
      {
         MaxChannel =  FPGA_MAX_TX_CHAN;
      }

      for (channelIdx = 0;
          channelIdx < MaxChannel && result == USER_RESULT_OK;
          ++channelIdx)
      {
         // Display element info above each set of data.
         sprintf(Label, "%s%s[%d]", LabelStem, pCfgTable->MsgStr, channelIdx);

         result = USER_RESULT_ERROR;
         if (User_OutputMsgString( Label, FALSE ) )
         {
           result = User_DisplayConfigTree(BranchName, pCfgTable->pNext, channelIdx,
                                           0, NULL);
         }
      }

      // Move pointer to next row in Cfg table
      ++pCfgTable;
   }
   return result;
}


/*************************************************************************
 *  MODIFICATIONS
 *    $History: Arinc429UserTables.c $
 * 
 * *****************  Version 40  *****************
 * User: John Omalley Date: 12-07-13   Time: 9:03a
 * Updated in $/software/control processor/code/system
 * SCR 1124 - Packed the configuration structures for ARINC429 Mgr,
 * Sensors and Triggers. Suppressed the alignment warnings for those three
 * objects also.
 * 
 * *****************  Version 39  *****************
 * User: John Omalley Date: 10/04/10   Time: 10:49a
 * Updated in $/software/control processor/code/system
 * SCR 897 - Removed Write Logic from RO functions
 * 
 * *****************  Version 38  *****************
 * User: Jeff Vahue   Date: 9/28/10    Time: 4:34p
 * Updated in $/software/control processor/code/system
 * SCR #898 - Fix access modes for register words
 *
 * *****************  Version 37  *****************
 * User: Peter Lee    Date: 9/07/10    Time: 1:16p
 * Updated in $/software/control processor/code/system
 * SCR #854 Code Review Updates
 *
 * *****************  Version 36  *****************
 * User: Jeff Vahue   Date: 8/28/10    Time: 12:22p
 * Updated in $/software/control processor/code/drivers
 * SCR# 830 - GENERATE_SYS_LOGS exclude area from coverage results
 *
 * *****************  Version 35  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:01a
 * Updated in $/software/control processor/code/drivers
 * SCR #685 - Add USER_GSE flag
 *
 * *****************  Version 34  *****************
 * User: Contractor2  Date: 7/28/10    Time: 4:06p
 * Updated in $/software/control processor/code/drivers
 * SCR# 744: Error - arinc429.register.rx[] returns 0x4000
 *
 * *****************  Version 33  *****************
 * User: Contractor V&v Date: 6/08/10    Time: 5:52p
 * Updated in $/software/control processor/code/drivers
 * SCR #615 Showcfg/Long msg enhancement
 *
 * *****************  Version 32  *****************
 * User: John Omalley Date: 6/08/10    Time: 12:05p
 * Updated in $/software/control processor/code/drivers
 * SCR 627 - Updated for LJ60
 *
 * *****************  Version 31  *****************
 * User: Contractor2  Date: 5/06/10    Time: 2:09p
 * Updated in $/software/control processor/code/drivers
 * SCR #579 Change LogWriteSys to return void
 *
 * *****************  Version 30  *****************
 * User: Jeff Vahue   Date: 4/12/10    Time: 5:43p
 * Updated in $/software/control processor/code/drivers
 * SCR #541 - all XXX_CreateLogs functions are now dependent on
 * GENERATE_SYS_LOGS
 *
 * *****************  Version 29  *****************
 * User: Contractor V&v Date: 3/19/10    Time: 4:29p
 * Updated in $/software/control processor/code/drivers
 * SCR #248 Parameter Log Change Add no log for showcfg
 *
 * *****************  Version 28  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 3:46p
 * Updated in $/software/control processor/code/drivers
 * SCR 106 Fix findings for User_GenericAccessor
 *
 * *****************  Version 27  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:07p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 26  *****************
 * User: Jeff Vahue   Date: 2/17/10    Time: 12:57p
 * Updated in $/software/control processor/code/drivers
 * SCR# 452 - Code coverage if/then/else removal
 *
 * *****************  Version 25  *****************
 * User: Jeff Vahue   Date: 2/11/10    Time: 12:39p
 * Updated in $/software/control processor/code/drivers
 * SCR# 444 - change Arinc429.status.interrupt_count to RO
 *
 * *****************  Version 24  *****************
 * User: Contractor V&v Date: 1/13/10    Time: 4:58p
 * Updated in $/software/control processor/code/drivers
 *
 * *****************  Version 23  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:22p
 * Updated in $/software/control processor/code/drivers
 * SCR 18, SCR 333
 *
 * *****************  Version 22  *****************
 * User: Peter Lee    Date: 12/15/09   Time: 2:47p
 * Updated in $/software/control processor/code/drivers
 * SCR #318 Arinc429 Tx Implementation
 *
 * *****************  Version 21  *****************
 * User: Contractor V&v Date: 12/10/09   Time: 5:41p
 * Updated in $/software/control processor/code/drivers
 * SCR 42
 *
 * *****************  Version 20  *****************
 * User: Jeff Vahue   Date: 12/04/09   Time: 4:32p
 * Updated in $/software/control processor/code/drivers
 * SCR# 350
 *
 * *****************  Version 19  *****************
 * User: Jeff Vahue   Date: 12/03/09   Time: 4:59p
 * Updated in $/software/control processor/code/drivers
 * SCR# 359
 *
 * *****************  Version 18  *****************
 * User: Contractor V&v Date: 11/30/09   Time: 5:34p
 * Updated in $/software/control processor/code/drivers
 * SCR 106	XXXUserTable.c consistency
 *
 * *****************  Version 17  *****************
 * User: Peter Lee    Date: 10/22/09   Time: 10:27a
 * Updated in $/software/control processor/code/drivers
 * SCR #171 Add correct System Status (Normal, Caution, Fault) enum to
 * UserTables
 *
 * *****************  Version 16  *****************
 * User: Peter Lee    Date: 10/01/09   Time: 3:27p
 * Updated in $/software/control processor/code/drivers
 * SCR #283 Misc Code Review Updates
 *
 * *****************  Version 15  *****************
 * User: Peter Lee    Date: 9/30/09    Time: 3:14p
 * Updated in $/software/control processor/code/drivers
 * SCR #283 Misc Code Review Updates
 *
 * *****************  Version 14  *****************
 * User: Peter Lee    Date: 9/30/09    Time: 9:14a
 * Updated in $/software/control processor/code/drivers
 * SCR #283 Misc Code Review Updates
 *
 * *****************  Version 13  *****************
 * User: Jim Mood     Date: 9/29/09    Time: 4:11p
 * Updated in $/software/control processor/code/drivers
 * SCR #178 Updated User Manager tables to include the limit fields
 *
 * *****************  Version 12  *****************
 * User: Peter Lee    Date: 6/29/09    Time: 2:20p
 * Updated in $/software/control processor/code/drivers
 * SCR #204 Misc Code Updates.
 *
 * *****************  Version 11  *****************
 * User: Peter Lee    Date: 2/02/09    Time: 4:06p
 * Updated in $/control processor/code/drivers
 * SCR #131 Misc Arinc429 Updates
 * Add logic to process _BITTask() and _ReadFIFOTask() only if FPGA Status
 * is OK.
 *
 * *****************  Version 10  *****************
 * User: Peter Lee    Date: 1/23/09    Time: 11:34a
 * Updated in $/control processor/code/drivers
 * SCR #131 Arinc429 Updates
 * Provide CBIT Health Counts
 *
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 1/22/09    Time: 1:27p
 * Updated in $/control processor/code/drivers
 * SCR #131 Add arinc429.createlogs() func.  Read Rx Ch Status when Rx Ch
 * Enb to clear "Data Present" flag.
 *
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 1/20/09    Time: 6:24p
 * Updated in $/control processor/code/drivers
 * SCR #131
 * Enb FIFO Half Full CBIT
 * Add Startup and Ch Loss TimeOut Logs
 * Add Sensor Filter LFR setup request to FPGA_Configure()
 * Update Func Comments
 * Add Rev Headers / Footers
 *
 *
 ***************************************************************************/

