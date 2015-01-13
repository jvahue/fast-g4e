#define ARINC429_USERTABLE_BODY
/******************************************************************************
            Copyright (C) 2009-2015 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:        Arinc429UserTables.c

    Description: User commands related to the Arinc 429 Processing

VERSION
     $Revision: 51 $  $Date: 1/12/15 12:51p $

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
/* Local Defines                                                             */
/*****************************************************************************/
#define STR_LIMIT_429 0,32

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static ARINC429_RX_CFG arinc429CfgTemp_RxChan;
static ARINC429_TX_CFG arinc429CfgTemp_TxChan;

static ARINC429_SYS_RX_STATUS arinc429SysStatusTemp_Rx;
static ARINC429_SYS_TX_STATUS arinc429SysStatusTemp_Tx;

static ARINC429_DRV_RX_STATUS arinc429DrvStatusTemp_Rx;
static ARINC429_DRV_TX_STATUS arinc429DrvStatusTemp_Tx;

static FPGA_RX_REG fpgaRxRegTemp;
static FPGA_TX_REG fpgaTxRegTemp;

static BOOLEAN bIgnoreDrvPBIT;

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static USER_HANDLER_RESULT Arinc429Msg_RegisterRx(USER_DATA_TYPE DataType,
                                           USER_MSG_PARAM Param,
                                           UINT32 Index,
                                           const void *SetPtr,
                                           void **GetPtr);

static USER_HANDLER_RESULT Arinc429Msg_RegisterTx(USER_DATA_TYPE DataType,
                                                  USER_MSG_PARAM Param,
                                                  UINT32 Index,
                                           const void *SetPtr,
                                           void **GetPtr);

static USER_HANDLER_RESULT Arinc429Msg_StateRx(USER_DATA_TYPE DataType,
                                               USER_MSG_PARAM Param,
                                               UINT32 Index,
                                               const void *SetPtr,
                                               void **GetPtr);

static USER_HANDLER_RESULT Arinc429Msg_StateTx(USER_DATA_TYPE DataType,
                                               USER_MSG_PARAM Param,
                                               UINT32 Index,
                                               const void *SetPtr,
                                               void **GetPtr);

static USER_HANDLER_RESULT Arinc429Msg_CfgRx(USER_DATA_TYPE DataType,
                                             USER_MSG_PARAM Param,
                                             UINT32 Index,
                                             const void *SetPtr,
                                             void **GetPtr);

static USER_HANDLER_RESULT Arinc429Msg_CfgTx(USER_DATA_TYPE DataType,
                                             USER_MSG_PARAM Param,
                                             UINT32 Index,
                                             const void *SetPtr,
                                             void **GetPtr);

static USER_HANDLER_RESULT Arinc429Msg_Reconfigure(USER_DATA_TYPE DataType,
                                                   USER_MSG_PARAM Param,
                                                   UINT32 Index,
                                                   const void *SetPtr,
                                                   void **GetPtr);

#ifdef GENERATE_SYS_LOGS
static USER_HANDLER_RESULT Arinc429Msg_CreateLogs(USER_DATA_TYPE DataType,
                                                  USER_MSG_PARAM Param,
                                                  UINT32 Index,
                                                  const void *SetPtr,
                                                  void **GetPtr);
#endif

static USER_HANDLER_RESULT Arinc429Msg_ShowConfig(USER_DATA_TYPE DataType,
                                                 USER_MSG_PARAM Param,
                                                 UINT32 Index,
                                                 const void *SetPtr,
                                                 void **GetPtr);

static USER_HANDLER_RESULT Arinc429Msg_Cfg(USER_DATA_TYPE DataType,
                                           USER_MSG_PARAM Param,
                                           UINT32 Index,
                                           const void *SetPtr,
                                           void **GetPtr);

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static USER_ENUM_TBL speedStrs[] =
{
  {"HIGH",  1},
  {"LOW",   0},
  {NULL,    0}
};

static USER_ENUM_TBL protocolStrs[] =
{
  {"STREAM",      ARINC429_RX_PROTOCOL_STREAM},
  {"CHANGE_ONLY", ARINC429_RX_PROTOCOL_CHANGE_ONLY},
  {"REDUCE",      ARINC429_RX_PROTOCOL_REDUCE},
  {NULL,          0}
};

static USER_ENUM_TBL subProtocolStrs[] =
{
  {"NONE",        ARINC429_RX_SUB_NONE},
  {"PW305AB_MFD", ARINC429_RX_SUB_PW305AB_MFD_REDUCE},
  {NULL,          0}
};

static USER_ENUM_TBL parityStrs[] =
{
  {"ODD",      1},
  {"EVEN",     0},
  {NULL,       0}
};

static USER_ENUM_TBL swapLabelStrsRx[] =
{
  {"SWAP",     0},
  {"NO_SWAP",  1},
  {NULL,  0}
};

static USER_ENUM_TBL swapLabelStrsTx[] =
{
  {"SWAP",     1},
  {"NO_SWAP",  0},
  {NULL,  0}
};

static USER_ENUM_TBL debugOutStrs[] =
{
   {"RAW",      0},
   {"PROTOCOL", 1},
   {NULL,       0}
};

static USER_ENUM_TBL arincSysStatusStrs[] =
{
  {"OK",ARINC429_STATUS_OK},
  {"FAULTED_PBIT",ARINC429_STATUS_FAULTED_PBIT},
  {"FAULTED_CBIT",ARINC429_STATUS_FAULTED_CBIT},
  {"DISABLED_OK",ARINC429_STATUS_DISABLED_OK},
  {"DISABLED_FAULTED",ARINC429_STATUS_DISABLED_FAULTED},
  {NULL,0}
};

#pragma ghs nowarning 1545 //Suppress packed structure alignment warning
// Macro defines the 'prototype' declaration for parameter entry
#define DECL_PARAMETER_ENTRY( n )\
static USER_MSG_TBL arinc429P##n [] =\
{\
   { "LABEL",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT8 , USER_RW, (void*)&arinc429CfgTemp_RxChan.Parameter[n].Label,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },\
   { "GPA"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&arinc429CfgTemp_RxChan.Parameter[n].GPA,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },\
   { "GPB"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32, USER_RW, (void*)&arinc429CfgTemp_RxChan.Parameter[n].GPB,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },\
   { "SCALE",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&arinc429CfgTemp_RxChan.Parameter[n].Scale,     0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },\
   { "TOL"  ,   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_FLOAT , USER_RW, (void*)&arinc429CfgTemp_RxChan.Parameter[n].Tolerance, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT, NULL },\
   {NULL,NULL,NULL,NO_HANDLER_DATA}\
}

DECL_PARAMETER_ENTRY(  0);
DECL_PARAMETER_ENTRY(  1);
DECL_PARAMETER_ENTRY(  2);
DECL_PARAMETER_ENTRY(  3);
DECL_PARAMETER_ENTRY(  4);
DECL_PARAMETER_ENTRY(  5);
DECL_PARAMETER_ENTRY(  6);
DECL_PARAMETER_ENTRY(  7);
DECL_PARAMETER_ENTRY(  8);
DECL_PARAMETER_ENTRY(  9);
DECL_PARAMETER_ENTRY( 10);
DECL_PARAMETER_ENTRY( 11);
DECL_PARAMETER_ENTRY( 12);
DECL_PARAMETER_ENTRY( 13);
DECL_PARAMETER_ENTRY( 14);
DECL_PARAMETER_ENTRY( 15);
DECL_PARAMETER_ENTRY( 16);
DECL_PARAMETER_ENTRY( 17);
DECL_PARAMETER_ENTRY( 18);
DECL_PARAMETER_ENTRY( 19);
DECL_PARAMETER_ENTRY( 20);
DECL_PARAMETER_ENTRY( 21);
DECL_PARAMETER_ENTRY( 22);
DECL_PARAMETER_ENTRY( 23);
DECL_PARAMETER_ENTRY( 24);
DECL_PARAMETER_ENTRY( 25);
DECL_PARAMETER_ENTRY( 26);
DECL_PARAMETER_ENTRY( 27);
DECL_PARAMETER_ENTRY( 28);
DECL_PARAMETER_ENTRY( 29);
DECL_PARAMETER_ENTRY( 30);
DECL_PARAMETER_ENTRY( 31);
DECL_PARAMETER_ENTRY( 32);
DECL_PARAMETER_ENTRY( 33);
DECL_PARAMETER_ENTRY( 34);
DECL_PARAMETER_ENTRY( 35);
DECL_PARAMETER_ENTRY( 36);
DECL_PARAMETER_ENTRY( 37);
DECL_PARAMETER_ENTRY( 38);
DECL_PARAMETER_ENTRY( 39);
DECL_PARAMETER_ENTRY( 40);
DECL_PARAMETER_ENTRY( 41);
DECL_PARAMETER_ENTRY( 42);
DECL_PARAMETER_ENTRY( 43);
DECL_PARAMETER_ENTRY( 44);
DECL_PARAMETER_ENTRY( 45);
DECL_PARAMETER_ENTRY( 46);
DECL_PARAMETER_ENTRY( 47);
DECL_PARAMETER_ENTRY( 48);
DECL_PARAMETER_ENTRY( 49);
DECL_PARAMETER_ENTRY( 50);
DECL_PARAMETER_ENTRY( 51);
DECL_PARAMETER_ENTRY( 52);
DECL_PARAMETER_ENTRY( 53);
DECL_PARAMETER_ENTRY( 54);
DECL_PARAMETER_ENTRY( 55);
DECL_PARAMETER_ENTRY( 56);
DECL_PARAMETER_ENTRY( 57);
DECL_PARAMETER_ENTRY( 58);
DECL_PARAMETER_ENTRY( 59);
DECL_PARAMETER_ENTRY( 60);
DECL_PARAMETER_ENTRY( 61);
DECL_PARAMETER_ENTRY( 62);
DECL_PARAMETER_ENTRY( 63);
DECL_PARAMETER_ENTRY( 64);
DECL_PARAMETER_ENTRY( 65);
DECL_PARAMETER_ENTRY( 66);
DECL_PARAMETER_ENTRY( 67);
DECL_PARAMETER_ENTRY( 68);
DECL_PARAMETER_ENTRY( 69);
DECL_PARAMETER_ENTRY( 70);
DECL_PARAMETER_ENTRY( 71);
DECL_PARAMETER_ENTRY( 72);
DECL_PARAMETER_ENTRY( 73);
DECL_PARAMETER_ENTRY( 74);
DECL_PARAMETER_ENTRY( 75);
DECL_PARAMETER_ENTRY( 76);
DECL_PARAMETER_ENTRY( 77);
DECL_PARAMETER_ENTRY( 78);
DECL_PARAMETER_ENTRY( 79);
DECL_PARAMETER_ENTRY( 80);
DECL_PARAMETER_ENTRY( 81);
DECL_PARAMETER_ENTRY( 82);
DECL_PARAMETER_ENTRY( 83);
DECL_PARAMETER_ENTRY( 84);
DECL_PARAMETER_ENTRY( 85);
DECL_PARAMETER_ENTRY( 86);
DECL_PARAMETER_ENTRY( 87);
DECL_PARAMETER_ENTRY( 88);
DECL_PARAMETER_ENTRY( 89);
DECL_PARAMETER_ENTRY( 90);
DECL_PARAMETER_ENTRY( 91);
DECL_PARAMETER_ENTRY( 92);
DECL_PARAMETER_ENTRY( 93);
DECL_PARAMETER_ENTRY( 94);
DECL_PARAMETER_ENTRY( 95);
DECL_PARAMETER_ENTRY( 96);
DECL_PARAMETER_ENTRY( 97);
DECL_PARAMETER_ENTRY( 98);
DECL_PARAMETER_ENTRY( 99);
DECL_PARAMETER_ENTRY(100);
DECL_PARAMETER_ENTRY(101);
DECL_PARAMETER_ENTRY(102);
DECL_PARAMETER_ENTRY(103);
DECL_PARAMETER_ENTRY(104);
DECL_PARAMETER_ENTRY(105);
DECL_PARAMETER_ENTRY(106);
DECL_PARAMETER_ENTRY(107);
DECL_PARAMETER_ENTRY(108);
DECL_PARAMETER_ENTRY(109);
DECL_PARAMETER_ENTRY(110);
DECL_PARAMETER_ENTRY(111);
DECL_PARAMETER_ENTRY(112);
DECL_PARAMETER_ENTRY(113);
DECL_PARAMETER_ENTRY(114);
DECL_PARAMETER_ENTRY(115);
DECL_PARAMETER_ENTRY(116);
DECL_PARAMETER_ENTRY(117);
DECL_PARAMETER_ENTRY(118);
DECL_PARAMETER_ENTRY(119);
DECL_PARAMETER_ENTRY(120);
DECL_PARAMETER_ENTRY(121);
DECL_PARAMETER_ENTRY(122);
DECL_PARAMETER_ENTRY(123);
DECL_PARAMETER_ENTRY(124);
DECL_PARAMETER_ENTRY(125);
DECL_PARAMETER_ENTRY(126);
DECL_PARAMETER_ENTRY(127);
DECL_PARAMETER_ENTRY(128);
DECL_PARAMETER_ENTRY(129);
DECL_PARAMETER_ENTRY(130);
DECL_PARAMETER_ENTRY(131);
DECL_PARAMETER_ENTRY(132);
DECL_PARAMETER_ENTRY(133);
DECL_PARAMETER_ENTRY(134);
DECL_PARAMETER_ENTRY(135);
DECL_PARAMETER_ENTRY(136);
DECL_PARAMETER_ENTRY(137);
DECL_PARAMETER_ENTRY(138);
DECL_PARAMETER_ENTRY(139);
DECL_PARAMETER_ENTRY(140);
DECL_PARAMETER_ENTRY(141);
DECL_PARAMETER_ENTRY(142);
DECL_PARAMETER_ENTRY(143);
DECL_PARAMETER_ENTRY(144);
DECL_PARAMETER_ENTRY(145);
DECL_PARAMETER_ENTRY(146);
DECL_PARAMETER_ENTRY(147);
DECL_PARAMETER_ENTRY(148);
DECL_PARAMETER_ENTRY(149);
DECL_PARAMETER_ENTRY(150);
DECL_PARAMETER_ENTRY(151);
DECL_PARAMETER_ENTRY(152);
DECL_PARAMETER_ENTRY(153);
DECL_PARAMETER_ENTRY(154);
DECL_PARAMETER_ENTRY(155);
DECL_PARAMETER_ENTRY(156);
DECL_PARAMETER_ENTRY(157);
DECL_PARAMETER_ENTRY(158);
DECL_PARAMETER_ENTRY(159);
DECL_PARAMETER_ENTRY(160);
DECL_PARAMETER_ENTRY(161);
DECL_PARAMETER_ENTRY(162);
DECL_PARAMETER_ENTRY(163);
DECL_PARAMETER_ENTRY(164);
DECL_PARAMETER_ENTRY(165);
DECL_PARAMETER_ENTRY(166);
DECL_PARAMETER_ENTRY(167);
DECL_PARAMETER_ENTRY(168);
DECL_PARAMETER_ENTRY(169);
DECL_PARAMETER_ENTRY(170);
DECL_PARAMETER_ENTRY(171);
DECL_PARAMETER_ENTRY(172);
DECL_PARAMETER_ENTRY(173);
DECL_PARAMETER_ENTRY(174);
DECL_PARAMETER_ENTRY(175);
DECL_PARAMETER_ENTRY(176);
DECL_PARAMETER_ENTRY(177);
DECL_PARAMETER_ENTRY(178);
DECL_PARAMETER_ENTRY(179);
DECL_PARAMETER_ENTRY(180);
DECL_PARAMETER_ENTRY(181);
DECL_PARAMETER_ENTRY(182);
DECL_PARAMETER_ENTRY(183);
DECL_PARAMETER_ENTRY(184);
DECL_PARAMETER_ENTRY(185);
DECL_PARAMETER_ENTRY(186);
DECL_PARAMETER_ENTRY(187);
DECL_PARAMETER_ENTRY(188);
DECL_PARAMETER_ENTRY(189);
DECL_PARAMETER_ENTRY(190);
DECL_PARAMETER_ENTRY(191);
DECL_PARAMETER_ENTRY(192);
DECL_PARAMETER_ENTRY(193);
DECL_PARAMETER_ENTRY(194);
DECL_PARAMETER_ENTRY(195);
DECL_PARAMETER_ENTRY(196);
DECL_PARAMETER_ENTRY(197);
DECL_PARAMETER_ENTRY(198);
DECL_PARAMETER_ENTRY(199);
DECL_PARAMETER_ENTRY(200);
DECL_PARAMETER_ENTRY(201);
DECL_PARAMETER_ENTRY(202);
DECL_PARAMETER_ENTRY(203);
DECL_PARAMETER_ENTRY(204);
DECL_PARAMETER_ENTRY(205);
DECL_PARAMETER_ENTRY(206);
DECL_PARAMETER_ENTRY(207);
DECL_PARAMETER_ENTRY(208);
DECL_PARAMETER_ENTRY(209);
DECL_PARAMETER_ENTRY(210);
DECL_PARAMETER_ENTRY(211);
DECL_PARAMETER_ENTRY(212);
DECL_PARAMETER_ENTRY(213);
DECL_PARAMETER_ENTRY(214);
DECL_PARAMETER_ENTRY(215);
DECL_PARAMETER_ENTRY(216);
DECL_PARAMETER_ENTRY(217);
DECL_PARAMETER_ENTRY(218);
DECL_PARAMETER_ENTRY(219);
DECL_PARAMETER_ENTRY(220);
DECL_PARAMETER_ENTRY(221);
DECL_PARAMETER_ENTRY(222);
DECL_PARAMETER_ENTRY(223);
DECL_PARAMETER_ENTRY(224);
DECL_PARAMETER_ENTRY(225);
DECL_PARAMETER_ENTRY(226);
DECL_PARAMETER_ENTRY(227);
DECL_PARAMETER_ENTRY(228);
DECL_PARAMETER_ENTRY(229);
DECL_PARAMETER_ENTRY(230);
DECL_PARAMETER_ENTRY(231);
DECL_PARAMETER_ENTRY(232);
DECL_PARAMETER_ENTRY(233);
DECL_PARAMETER_ENTRY(234);
DECL_PARAMETER_ENTRY(235);
DECL_PARAMETER_ENTRY(236);
DECL_PARAMETER_ENTRY(237);
DECL_PARAMETER_ENTRY(238);
DECL_PARAMETER_ENTRY(239);
DECL_PARAMETER_ENTRY(240);
DECL_PARAMETER_ENTRY(241);
DECL_PARAMETER_ENTRY(242);
DECL_PARAMETER_ENTRY(243);
DECL_PARAMETER_ENTRY(244);
DECL_PARAMETER_ENTRY(245);
DECL_PARAMETER_ENTRY(246);
DECL_PARAMETER_ENTRY(247);
DECL_PARAMETER_ENTRY(248);
DECL_PARAMETER_ENTRY(249);
DECL_PARAMETER_ENTRY(250);
DECL_PARAMETER_ENTRY(251);
DECL_PARAMETER_ENTRY(252);
DECL_PARAMETER_ENTRY(253);
DECL_PARAMETER_ENTRY(254);
DECL_PARAMETER_ENTRY(255);

static USER_MSG_TBL arinc429RegisterRxTbl[] =
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

static USER_MSG_TBL arinc429RegisterTxTbl[] =
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

static USER_MSG_TBL arinc429StatusRxTbl[] =
{
  {"SW_BUFFER_OF_COUNT",   NO_NEXT_TABLE, Arinc429Msg_StateRx, USER_TYPE_UINT32,  USER_RO, (void*)&arinc429SysStatusTemp_Rx.SwBuffOverFlowCnt,   0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,NULL},
  {"PARITY_ERROR_COUNT",   NO_NEXT_TABLE, Arinc429Msg_StateRx, USER_TYPE_UINT32,  USER_RO, (void*)&arinc429SysStatusTemp_Rx.ParityErrCnt,        0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,NULL},
  {"FRAMING_ERROR_COUNT",  NO_NEXT_TABLE, Arinc429Msg_StateRx, USER_TYPE_UINT32,  USER_RO, (void*)&arinc429SysStatusTemp_Rx.FramingErrCnt,       0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,NULL},
  {"FIFO_HALFFULL_COUNT",  NO_NEXT_TABLE, Arinc429Msg_StateRx, USER_TYPE_UINT32,  USER_RO, (void*)&arinc429DrvStatusTemp_Rx.FIFO_HalfFullCnt,    0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,NULL},
  {"FIFO_FULL_COUNT",      NO_NEXT_TABLE, Arinc429Msg_StateRx, USER_TYPE_UINT32,  USER_RO, (void*)&arinc429DrvStatusTemp_Rx.FIFO_FullCnt,        0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,NULL},
  {"FIFO_EMPTY_COUNT",     NO_NEXT_TABLE, Arinc429Msg_StateRx, USER_TYPE_UINT32,  USER_RO, (void*)&arinc429DrvStatusTemp_Rx.FIFO_NotEmptyCnt,    0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,NULL},
  {"CHANNEL_ACTIVE",       NO_NEXT_TABLE, Arinc429Msg_StateRx, USER_TYPE_BOOLEAN, USER_RO, (void*)&arinc429SysStatusTemp_Rx.bChanActive,         0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,NULL},
  {"LAST_ACTIVITY_TIME",   NO_NEXT_TABLE, Arinc429Msg_StateRx, USER_TYPE_UINT32,  USER_RO, (void*)&arinc429SysStatusTemp_Rx.LastActivityTime,    0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,NULL},
  {"CHANNEL_TIMEOUT",      NO_NEXT_TABLE, Arinc429Msg_StateRx, USER_TYPE_BOOLEAN, USER_RO, (void*)&arinc429SysStatusTemp_Rx.bChanTimeOut,        0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,NULL},
  {"CHANNEL_START_TIMEOUT",NO_NEXT_TABLE, Arinc429Msg_StateRx, USER_TYPE_BOOLEAN, USER_RO, (void*)&arinc429SysStatusTemp_Rx.bChanStartUpTimeOut, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,NULL},
  {"DATA_LOSS_COUNT",      NO_NEXT_TABLE, Arinc429Msg_StateRx, USER_TYPE_UINT32,  USER_RO, (void*)&arinc429SysStatusTemp_Rx.DataLossCnt,         0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,NULL},
  {"FPGA_STATUS_REG",      NO_NEXT_TABLE, Arinc429Msg_StateRx, USER_TYPE_UINT16,  USER_RO, (void*)&arinc429SysStatusTemp_Rx.FPGA_Status,         0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,NULL},
  {"RX_COUNT",             NO_NEXT_TABLE, Arinc429Msg_StateRx, USER_TYPE_UINT32,  USER_RO, (void*)&arinc429SysStatusTemp_Rx.RxCnt,               0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,NULL},
  {"STATUS",               NO_NEXT_TABLE, Arinc429Msg_StateRx, USER_TYPE_ENUM,    USER_RO, (void*)&arinc429SysStatusTemp_Rx.Status,              0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,arincSysStatusStrs},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};


static USER_MSG_TBL arinc429StatusTxTbl[] =
{
  {"FIFO_HALFFULL_COUNT", NO_NEXT_TABLE, Arinc429Msg_StateTx, USER_TYPE_UINT32, USER_RO, (void*)&arinc429DrvStatusTemp_Tx.FIFO_HalfFullCnt, 0, FPGA_MAX_TX_CHAN-1, NO_LIMIT, NULL},
  {"FIFO_FULL_COUNT",     NO_NEXT_TABLE, Arinc429Msg_StateTx, USER_TYPE_UINT32, USER_RO, (void*)&arinc429DrvStatusTemp_Tx.FIFO_FullCnt,     0, FPGA_MAX_TX_CHAN-1, NO_LIMIT, NULL},
  {"FIFO_EMPTY_COUNT",    NO_NEXT_TABLE, Arinc429Msg_StateTx, USER_TYPE_UINT32, USER_RO, (void*)&arinc429DrvStatusTemp_Tx.FIFO_EmptyCnt,    0, FPGA_MAX_TX_CHAN-1, NO_LIMIT, NULL},
  {"STATUS",              NO_NEXT_TABLE, Arinc429Msg_StateTx, USER_TYPE_ENUM,   USER_RO, (void*)&arinc429DrvStatusTemp_Tx.Status,           0, FPGA_MAX_TX_CHAN-1, NO_LIMIT, arincSysStatusStrs},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL arinc429RxParameterTbl[] =
{  { "P0",   arinc429P0,   NULL, NO_HANDLER_DATA },
   { "P1",   arinc429P1,   NULL, NO_HANDLER_DATA },
   { "P2",   arinc429P2,   NULL, NO_HANDLER_DATA },
   { "P3",   arinc429P3,   NULL, NO_HANDLER_DATA },
   { "P4",   arinc429P4,   NULL, NO_HANDLER_DATA },
   { "P5",   arinc429P5,   NULL, NO_HANDLER_DATA },
   { "P6",   arinc429P6,   NULL, NO_HANDLER_DATA },
   { "P7",   arinc429P7,   NULL, NO_HANDLER_DATA },
   { "P8",   arinc429P8,   NULL, NO_HANDLER_DATA },
   { "P9",   arinc429P9,   NULL, NO_HANDLER_DATA },
   { "P10",  arinc429P10,  NULL, NO_HANDLER_DATA },
   { "P11",  arinc429P11,  NULL, NO_HANDLER_DATA },
   { "P12",  arinc429P12,  NULL, NO_HANDLER_DATA },
   { "P13",  arinc429P13,  NULL, NO_HANDLER_DATA },
   { "P14",  arinc429P14,  NULL, NO_HANDLER_DATA },
   { "P15",  arinc429P15,  NULL, NO_HANDLER_DATA },
   { "P16",  arinc429P16,  NULL, NO_HANDLER_DATA },
   { "P17",  arinc429P17,  NULL, NO_HANDLER_DATA },
   { "P18",  arinc429P18,  NULL, NO_HANDLER_DATA },
   { "P19",  arinc429P19,  NULL, NO_HANDLER_DATA },
   { "P20",  arinc429P20,  NULL, NO_HANDLER_DATA },
   { "P21",  arinc429P21,  NULL, NO_HANDLER_DATA },
   { "P22",  arinc429P22,  NULL, NO_HANDLER_DATA },
   { "P23",  arinc429P23,  NULL, NO_HANDLER_DATA },
   { "P24",  arinc429P24,  NULL, NO_HANDLER_DATA },
   { "P25",  arinc429P25,  NULL, NO_HANDLER_DATA },
   { "P26",  arinc429P26,  NULL, NO_HANDLER_DATA },
   { "P27",  arinc429P27,  NULL, NO_HANDLER_DATA },
   { "P28",  arinc429P28,  NULL, NO_HANDLER_DATA },
   { "P29",  arinc429P29,  NULL, NO_HANDLER_DATA },
   { "P30",  arinc429P30,  NULL, NO_HANDLER_DATA },
   { "P31",  arinc429P31,  NULL, NO_HANDLER_DATA },
   { "P32",  arinc429P32,  NULL, NO_HANDLER_DATA },
   { "P33",  arinc429P33,  NULL, NO_HANDLER_DATA },
   { "P34",  arinc429P34,  NULL, NO_HANDLER_DATA },
   { "P35",  arinc429P35,  NULL, NO_HANDLER_DATA },
   { "P36",  arinc429P36,  NULL, NO_HANDLER_DATA },
   { "P37",  arinc429P37,  NULL, NO_HANDLER_DATA },
   { "P38",  arinc429P38,  NULL, NO_HANDLER_DATA },
   { "P39",  arinc429P39,  NULL, NO_HANDLER_DATA },
   { "P40",  arinc429P40,  NULL, NO_HANDLER_DATA },
   { "P41",  arinc429P41,  NULL, NO_HANDLER_DATA },
   { "P42",  arinc429P42,  NULL, NO_HANDLER_DATA },
   { "P43",  arinc429P43,  NULL, NO_HANDLER_DATA },
   { "P44",  arinc429P44,  NULL, NO_HANDLER_DATA },
   { "P45",  arinc429P45,  NULL, NO_HANDLER_DATA },
   { "P46",  arinc429P46,  NULL, NO_HANDLER_DATA },
   { "P47",  arinc429P47,  NULL, NO_HANDLER_DATA },
   { "P48",  arinc429P48,  NULL, NO_HANDLER_DATA },
   { "P49",  arinc429P49,  NULL, NO_HANDLER_DATA },
   { "P50",  arinc429P50,  NULL, NO_HANDLER_DATA },
   { "P51",  arinc429P51,  NULL, NO_HANDLER_DATA },
   { "P52",  arinc429P52,  NULL, NO_HANDLER_DATA },
   { "P53",  arinc429P53,  NULL, NO_HANDLER_DATA },
   { "P54",  arinc429P54,  NULL, NO_HANDLER_DATA },
   { "P55",  arinc429P55,  NULL, NO_HANDLER_DATA },
   { "P56",  arinc429P56,  NULL, NO_HANDLER_DATA },
   { "P57",  arinc429P57,  NULL, NO_HANDLER_DATA },
   { "P58",  arinc429P58,  NULL, NO_HANDLER_DATA },
   { "P59",  arinc429P59,  NULL, NO_HANDLER_DATA },
   { "P60",  arinc429P60,  NULL, NO_HANDLER_DATA },
   { "P61",  arinc429P61,  NULL, NO_HANDLER_DATA },
   { "P62",  arinc429P62,  NULL, NO_HANDLER_DATA },
   { "P63",  arinc429P63,  NULL, NO_HANDLER_DATA },
   { "P64",  arinc429P64,  NULL, NO_HANDLER_DATA },
   { "P65",  arinc429P65,  NULL, NO_HANDLER_DATA },
   { "P66",  arinc429P66,  NULL, NO_HANDLER_DATA },
   { "P67",  arinc429P67,  NULL, NO_HANDLER_DATA },
   { "P68",  arinc429P68,  NULL, NO_HANDLER_DATA },
   { "P69",  arinc429P69,  NULL, NO_HANDLER_DATA },
   { "P70",  arinc429P70,  NULL, NO_HANDLER_DATA },
   { "P71",  arinc429P71,  NULL, NO_HANDLER_DATA },
   { "P72",  arinc429P72,  NULL, NO_HANDLER_DATA },
   { "P73",  arinc429P73,  NULL, NO_HANDLER_DATA },
   { "P74",  arinc429P74,  NULL, NO_HANDLER_DATA },
   { "P75",  arinc429P75,  NULL, NO_HANDLER_DATA },
   { "P76",  arinc429P76,  NULL, NO_HANDLER_DATA },
   { "P77",  arinc429P77,  NULL, NO_HANDLER_DATA },
   { "P78",  arinc429P78,  NULL, NO_HANDLER_DATA },
   { "P79",  arinc429P79,  NULL, NO_HANDLER_DATA },
   { "P80",  arinc429P80,  NULL, NO_HANDLER_DATA },
   { "P81",  arinc429P81,  NULL, NO_HANDLER_DATA },
   { "P82",  arinc429P82,  NULL, NO_HANDLER_DATA },
   { "P83",  arinc429P83,  NULL, NO_HANDLER_DATA },
   { "P84",  arinc429P84,  NULL, NO_HANDLER_DATA },
   { "P85",  arinc429P85,  NULL, NO_HANDLER_DATA },
   { "P86",  arinc429P86,  NULL, NO_HANDLER_DATA },
   { "P87",  arinc429P87,  NULL, NO_HANDLER_DATA },
   { "P88",  arinc429P88,  NULL, NO_HANDLER_DATA },
   { "P89",  arinc429P89,  NULL, NO_HANDLER_DATA },
   { "P90",  arinc429P90,  NULL, NO_HANDLER_DATA },
   { "P91",  arinc429P91,  NULL, NO_HANDLER_DATA },
   { "P92",  arinc429P92,  NULL, NO_HANDLER_DATA },
   { "P93",  arinc429P93,  NULL, NO_HANDLER_DATA },
   { "P94",  arinc429P94,  NULL, NO_HANDLER_DATA },
   { "P95",  arinc429P95,  NULL, NO_HANDLER_DATA },
   { "P96",  arinc429P96,  NULL, NO_HANDLER_DATA },
   { "P97",  arinc429P97,  NULL, NO_HANDLER_DATA },
   { "P98",  arinc429P98,  NULL, NO_HANDLER_DATA },
   { "P99",  arinc429P99,  NULL, NO_HANDLER_DATA },
   { "P100", arinc429P100, NULL, NO_HANDLER_DATA },
   { "P101", arinc429P101, NULL, NO_HANDLER_DATA },
   { "P102", arinc429P102, NULL, NO_HANDLER_DATA },
   { "P103", arinc429P103, NULL, NO_HANDLER_DATA },
   { "P104", arinc429P104, NULL, NO_HANDLER_DATA },
   { "P105", arinc429P105, NULL, NO_HANDLER_DATA },
   { "P106", arinc429P106, NULL, NO_HANDLER_DATA },
   { "P107", arinc429P107, NULL, NO_HANDLER_DATA },
   { "P108", arinc429P108, NULL, NO_HANDLER_DATA },
   { "P109", arinc429P109, NULL, NO_HANDLER_DATA },
   { "P110", arinc429P110, NULL, NO_HANDLER_DATA },
   { "P111", arinc429P111, NULL, NO_HANDLER_DATA },
   { "P112", arinc429P112, NULL, NO_HANDLER_DATA },
   { "P113", arinc429P113, NULL, NO_HANDLER_DATA },
   { "P114", arinc429P114, NULL, NO_HANDLER_DATA },
   { "P115", arinc429P115, NULL, NO_HANDLER_DATA },
   { "P116", arinc429P116, NULL, NO_HANDLER_DATA },
   { "P117", arinc429P117, NULL, NO_HANDLER_DATA },
   { "P118", arinc429P118, NULL, NO_HANDLER_DATA },
   { "P119", arinc429P119, NULL, NO_HANDLER_DATA },
   { "P120", arinc429P120, NULL, NO_HANDLER_DATA },
   { "P121", arinc429P121, NULL, NO_HANDLER_DATA },
   { "P122", arinc429P122, NULL, NO_HANDLER_DATA },
   { "P123", arinc429P123, NULL, NO_HANDLER_DATA },
   { "P124", arinc429P124, NULL, NO_HANDLER_DATA },
   { "P125", arinc429P125, NULL, NO_HANDLER_DATA },
   { "P126", arinc429P126, NULL, NO_HANDLER_DATA },
   { "P127", arinc429P127, NULL, NO_HANDLER_DATA },
   { "P128", arinc429P128, NULL, NO_HANDLER_DATA },
   { "P129", arinc429P129, NULL, NO_HANDLER_DATA },
   { "P130", arinc429P130, NULL, NO_HANDLER_DATA },
   { "P131", arinc429P131, NULL, NO_HANDLER_DATA },
   { "P132", arinc429P132, NULL, NO_HANDLER_DATA },
   { "P133", arinc429P133, NULL, NO_HANDLER_DATA },
   { "P134", arinc429P134, NULL, NO_HANDLER_DATA },
   { "P135", arinc429P135, NULL, NO_HANDLER_DATA },
   { "P136", arinc429P136, NULL, NO_HANDLER_DATA },
   { "P137", arinc429P137, NULL, NO_HANDLER_DATA },
   { "P138", arinc429P138, NULL, NO_HANDLER_DATA },
   { "P139", arinc429P139, NULL, NO_HANDLER_DATA },
   { "P140", arinc429P140, NULL, NO_HANDLER_DATA },
   { "P141", arinc429P141, NULL, NO_HANDLER_DATA },
   { "P142", arinc429P142, NULL, NO_HANDLER_DATA },
   { "P143", arinc429P143, NULL, NO_HANDLER_DATA },
   { "P144", arinc429P144, NULL, NO_HANDLER_DATA },
   { "P145", arinc429P145, NULL, NO_HANDLER_DATA },
   { "P146", arinc429P146, NULL, NO_HANDLER_DATA },
   { "P147", arinc429P147, NULL, NO_HANDLER_DATA },
   { "P148", arinc429P148, NULL, NO_HANDLER_DATA },
   { "P149", arinc429P149, NULL, NO_HANDLER_DATA },
   { "P150", arinc429P150, NULL, NO_HANDLER_DATA },
   { "P151", arinc429P151, NULL, NO_HANDLER_DATA },
   { "P152", arinc429P152, NULL, NO_HANDLER_DATA },
   { "P153", arinc429P153, NULL, NO_HANDLER_DATA },
   { "P154", arinc429P154, NULL, NO_HANDLER_DATA },
   { "P155", arinc429P155, NULL, NO_HANDLER_DATA },
   { "P156", arinc429P156, NULL, NO_HANDLER_DATA },
   { "P157", arinc429P157, NULL, NO_HANDLER_DATA },
   { "P158", arinc429P158, NULL, NO_HANDLER_DATA },
   { "P159", arinc429P159, NULL, NO_HANDLER_DATA },
   { "P160", arinc429P160, NULL, NO_HANDLER_DATA },
   { "P161", arinc429P161, NULL, NO_HANDLER_DATA },
   { "P162", arinc429P162, NULL, NO_HANDLER_DATA },
   { "P163", arinc429P163, NULL, NO_HANDLER_DATA },
   { "P164", arinc429P164, NULL, NO_HANDLER_DATA },
   { "P165", arinc429P165, NULL, NO_HANDLER_DATA },
   { "P166", arinc429P166, NULL, NO_HANDLER_DATA },
   { "P167", arinc429P167, NULL, NO_HANDLER_DATA },
   { "P168", arinc429P168, NULL, NO_HANDLER_DATA },
   { "P169", arinc429P169, NULL, NO_HANDLER_DATA },
   { "P170", arinc429P170, NULL, NO_HANDLER_DATA },
   { "P171", arinc429P171, NULL, NO_HANDLER_DATA },
   { "P172", arinc429P172, NULL, NO_HANDLER_DATA },
   { "P173", arinc429P173, NULL, NO_HANDLER_DATA },
   { "P174", arinc429P174, NULL, NO_HANDLER_DATA },
   { "P175", arinc429P175, NULL, NO_HANDLER_DATA },
   { "P176", arinc429P176, NULL, NO_HANDLER_DATA },
   { "P177", arinc429P177, NULL, NO_HANDLER_DATA },
   { "P178", arinc429P178, NULL, NO_HANDLER_DATA },
   { "P179", arinc429P179, NULL, NO_HANDLER_DATA },
   { "P180", arinc429P180, NULL, NO_HANDLER_DATA },
   { "P181", arinc429P181, NULL, NO_HANDLER_DATA },
   { "P182", arinc429P182, NULL, NO_HANDLER_DATA },
   { "P183", arinc429P183, NULL, NO_HANDLER_DATA },
   { "P184", arinc429P184, NULL, NO_HANDLER_DATA },
   { "P185", arinc429P185, NULL, NO_HANDLER_DATA },
   { "P186", arinc429P186, NULL, NO_HANDLER_DATA },
   { "P187", arinc429P187, NULL, NO_HANDLER_DATA },
   { "P188", arinc429P188, NULL, NO_HANDLER_DATA },
   { "P189", arinc429P189, NULL, NO_HANDLER_DATA },
   { "P190", arinc429P190, NULL, NO_HANDLER_DATA },
   { "P191", arinc429P191, NULL, NO_HANDLER_DATA },
   { "P192", arinc429P192, NULL, NO_HANDLER_DATA },
   { "P193", arinc429P193, NULL, NO_HANDLER_DATA },
   { "P194", arinc429P194, NULL, NO_HANDLER_DATA },
   { "P195", arinc429P195, NULL, NO_HANDLER_DATA },
   { "P196", arinc429P196, NULL, NO_HANDLER_DATA },
   { "P197", arinc429P197, NULL, NO_HANDLER_DATA },
   { "P198", arinc429P198, NULL, NO_HANDLER_DATA },
   { "P199", arinc429P199, NULL, NO_HANDLER_DATA },
   { "P200", arinc429P200, NULL, NO_HANDLER_DATA },
   { "P201", arinc429P201, NULL, NO_HANDLER_DATA },
   { "P202", arinc429P202, NULL, NO_HANDLER_DATA },
   { "P203", arinc429P203, NULL, NO_HANDLER_DATA },
   { "P204", arinc429P204, NULL, NO_HANDLER_DATA },
   { "P205", arinc429P205, NULL, NO_HANDLER_DATA },
   { "P206", arinc429P206, NULL, NO_HANDLER_DATA },
   { "P207", arinc429P207, NULL, NO_HANDLER_DATA },
   { "P208", arinc429P208, NULL, NO_HANDLER_DATA },
   { "P209", arinc429P209, NULL, NO_HANDLER_DATA },
   { "P210", arinc429P210, NULL, NO_HANDLER_DATA },
   { "P211", arinc429P211, NULL, NO_HANDLER_DATA },
   { "P212", arinc429P212, NULL, NO_HANDLER_DATA },
   { "P213", arinc429P213, NULL, NO_HANDLER_DATA },
   { "P214", arinc429P214, NULL, NO_HANDLER_DATA },
   { "P215", arinc429P215, NULL, NO_HANDLER_DATA },
   { "P216", arinc429P216, NULL, NO_HANDLER_DATA },
   { "P217", arinc429P217, NULL, NO_HANDLER_DATA },
   { "P218", arinc429P218, NULL, NO_HANDLER_DATA },
   { "P219", arinc429P219, NULL, NO_HANDLER_DATA },
   { "P220", arinc429P220, NULL, NO_HANDLER_DATA },
   { "P221", arinc429P221, NULL, NO_HANDLER_DATA },
   { "P222", arinc429P222, NULL, NO_HANDLER_DATA },
   { "P223", arinc429P223, NULL, NO_HANDLER_DATA },
   { "P224", arinc429P224, NULL, NO_HANDLER_DATA },
   { "P225", arinc429P225, NULL, NO_HANDLER_DATA },
   { "P226", arinc429P226, NULL, NO_HANDLER_DATA },
   { "P227", arinc429P227, NULL, NO_HANDLER_DATA },
   { "P228", arinc429P228, NULL, NO_HANDLER_DATA },
   { "P229", arinc429P229, NULL, NO_HANDLER_DATA },
   { "P230", arinc429P230, NULL, NO_HANDLER_DATA },
   { "P231", arinc429P231, NULL, NO_HANDLER_DATA },
   { "P232", arinc429P232, NULL, NO_HANDLER_DATA },
   { "P233", arinc429P233, NULL, NO_HANDLER_DATA },
   { "P234", arinc429P234, NULL, NO_HANDLER_DATA },
   { "P235", arinc429P235, NULL, NO_HANDLER_DATA },
   { "P236", arinc429P236, NULL, NO_HANDLER_DATA },
   { "P237", arinc429P237, NULL, NO_HANDLER_DATA },
   { "P238", arinc429P238, NULL, NO_HANDLER_DATA },
   { "P239", arinc429P239, NULL, NO_HANDLER_DATA },
   { "P240", arinc429P240, NULL, NO_HANDLER_DATA },
   { "P241", arinc429P241, NULL, NO_HANDLER_DATA },
   { "P242", arinc429P242, NULL, NO_HANDLER_DATA },
   { "P243", arinc429P243, NULL, NO_HANDLER_DATA },
   { "P244", arinc429P244, NULL, NO_HANDLER_DATA },
   { "P245", arinc429P245, NULL, NO_HANDLER_DATA },
   { "P246", arinc429P246, NULL, NO_HANDLER_DATA },
   { "P247", arinc429P247, NULL, NO_HANDLER_DATA },
   { "P248", arinc429P248, NULL, NO_HANDLER_DATA },
   { "P249", arinc429P249, NULL, NO_HANDLER_DATA },
   { "P250", arinc429P250, NULL, NO_HANDLER_DATA },
   { "P251", arinc429P251, NULL, NO_HANDLER_DATA },
   { "P252", arinc429P252, NULL, NO_HANDLER_DATA },
   { "P253", arinc429P253, NULL, NO_HANDLER_DATA },
   { "P254", arinc429P254, NULL, NO_HANDLER_DATA },
   { "P255", arinc429P255, NULL, NO_HANDLER_DATA },
   {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL arinc429CfgRxTbl[] =
{
  {"NAME",             NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_STR,     USER_RW, (void*)arinc429CfgTemp_RxChan.Name,             0, FPGA_MAX_RX_CHAN-1, STR_LIMIT_429, NULL},
  {"SPEED",            NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_ENUM,    USER_RW, (void*)&arinc429CfgTemp_RxChan.Speed,            0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      speedStrs},
  {"PROTOCOL",         NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_ENUM,    USER_RW, (void*)&arinc429CfgTemp_RxChan.Protocol,         0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      protocolStrs},
  {"SUBPROTOCOL",      NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_ENUM,    USER_RW, (void*)&arinc429CfgTemp_RxChan.SubProtocol,      0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      subProtocolStrs},
  {"PARAMETER",        arinc429RxParameterTbl, NULL, NO_HANDLER_DATA},
  {"CHANNELSTARTUP_S", NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32,  USER_RW, (void*)&arinc429CfgTemp_RxChan.ChannelStartup_s, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      NULL},
  {"CHANNELTIMEOUT_S", NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_UINT32,  USER_RW, (void*)&arinc429CfgTemp_RxChan.ChannelTimeOut_s, 0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      NULL},
  {"CHANNELSYSCOND",   NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_ENUM,    USER_RW, (void*)&arinc429CfgTemp_RxChan.ChannelSysCond,   0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      Flt_UserEnumStatus},
  {"PBITSYSCOND",      NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_ENUM,    USER_RW, (void*)&arinc429CfgTemp_RxChan.PBITSysCond,      0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      Flt_UserEnumStatus},
  {"PARITY",           NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_ENUM,    USER_RW, (void*)&arinc429CfgTemp_RxChan.Parity,           0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      parityStrs},
  {"SWAPLABELBITS",    NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_ENUM,    USER_RW, (void*)&arinc429CfgTemp_RxChan.LabelSwapBits,    0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      swapLabelStrsRx},
  {"FILTER0",          NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_HEX32,   USER_RW, (void*)&arinc429CfgTemp_RxChan.FilterArray[0],   0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      NULL},
  {"FILTER1",          NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_HEX32,   USER_RW, (void*)&arinc429CfgTemp_RxChan.FilterArray[1],   0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      NULL},
  {"FILTER2",          NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_HEX32,   USER_RW, (void*)&arinc429CfgTemp_RxChan.FilterArray[2],   0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      NULL},
  {"FILTER3",          NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_HEX32,   USER_RW, (void*)&arinc429CfgTemp_RxChan.FilterArray[3],   0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      NULL},
  {"FILTER4",          NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_HEX32,   USER_RW, (void*)&arinc429CfgTemp_RxChan.FilterArray[4],   0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      NULL},
  {"FILTER5",          NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_HEX32,   USER_RW, (void*)&arinc429CfgTemp_RxChan.FilterArray[5],   0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      NULL},
  {"FILTER6",          NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_HEX32,   USER_RW, (void*)&arinc429CfgTemp_RxChan.FilterArray[6],   0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      NULL},
  {"FILTER7",          NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_HEX32,   USER_RW, (void*)&arinc429CfgTemp_RxChan.FilterArray[7],   0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      NULL},
  {"ENABLE",           NO_NEXT_TABLE, Arinc429Msg_CfgRx, USER_TYPE_BOOLEAN, USER_RW, (void*)&arinc429CfgTemp_RxChan.Enable,           0, FPGA_MAX_RX_CHAN-1, NO_LIMIT,      NULL},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};


static USER_MSG_TBL arinc429CfgTxTbl[] =
{
  {"NAME",          NO_NEXT_TABLE, Arinc429Msg_CfgTx, USER_TYPE_STR,     USER_RW, (void*)arinc429CfgTemp_TxChan.Name,          0, FPGA_MAX_TX_CHAN-1, STR_LIMIT_429, NULL},
  {"SPEED",         NO_NEXT_TABLE, Arinc429Msg_CfgTx, USER_TYPE_ENUM,    USER_RW, (void*)&arinc429CfgTemp_TxChan.Speed,         0, FPGA_MAX_TX_CHAN-1, NO_LIMIT,      speedStrs},
  {"PARITY",        NO_NEXT_TABLE, Arinc429Msg_CfgTx, USER_TYPE_ENUM,    USER_RW, (void*)&arinc429CfgTemp_TxChan.Parity,        0, FPGA_MAX_TX_CHAN-1, NO_LIMIT,      parityStrs},
  {"ENABLE",        NO_NEXT_TABLE, Arinc429Msg_CfgTx, USER_TYPE_BOOLEAN, USER_RW, (void*)&arinc429CfgTemp_TxChan.Enable,        0, FPGA_MAX_TX_CHAN-1, NO_LIMIT,      NULL},
  {"SWAPLABELBITS", NO_NEXT_TABLE, Arinc429Msg_CfgTx, USER_TYPE_ENUM,    USER_RW, (void*)&arinc429CfgTemp_TxChan.LabelSwapBits, 0, FPGA_MAX_TX_CHAN-1, NO_LIMIT,      swapLabelStrsTx},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL arinc429DebugTbl[] =
{
  {"RAWFILTERED", NO_NEXT_TABLE, User_GenericAccessor, USER_TYPE_BOOLEAN,(USER_RW|USER_GSE), (void*)&m_Arinc429_Debug.bOutputRawFilteredBuff,     -1,-1, NO_LIMIT, NULL},
  {"FORMATTED",   NO_NEXT_TABLE, User_GenericAccessor, USER_TYPE_BOOLEAN,(USER_RW|USER_GSE), (void*)&m_Arinc429_Debug.bOutputRawFilteredFormatted,-1,-1, NO_LIMIT, NULL},
  {"ALLCHAN",     NO_NEXT_TABLE, User_GenericAccessor, USER_TYPE_BOOLEAN,(USER_RW|USER_GSE), (void*)&m_Arinc429_Debug.bOutputMultipleArincChan,   -1,-1, NO_LIMIT, NULL},
  {"TYPE",        NO_NEXT_TABLE, User_GenericAccessor, USER_TYPE_ENUM,   (USER_RW|USER_GSE), &m_Arinc429_Debug.OutputType,                        -1,-1, NO_LIMIT, debugOutStrs },
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL arinc429RegisterTbl[] =
{
  {"RX",      arinc429RegisterRxTbl, NULL, NO_HANDLER_DATA},
  {"TX",      arinc429RegisterTxTbl, NULL, NO_HANDLER_DATA},
  {"IMR1",    NO_NEXT_TABLE, User_GenericAccessor, USER_TYPE_HEX16, USER_RO, (void*)FPGA_IMR1,                 -1,-1, NO_LIMIT, NULL},
  {"IMR2",    NO_NEXT_TABLE, User_GenericAccessor, USER_TYPE_HEX16, USER_RO, (void*)FPGA_IMR2,                 -1,-1, NO_LIMIT, NULL},
  {"FPGA_VER",NO_NEXT_TABLE, User_GenericAccessor, USER_TYPE_HEX16, USER_RO, (void*)FPGA_VER,                  -1,-1, NO_LIMIT, NULL},
  {"CCR",     NO_NEXT_TABLE, User_GenericAccessor, USER_TYPE_HEX16, USER_RO, (void*)FPGA_429_COMMON_CONTROL,   -1,-1, NO_LIMIT, NULL},
  {"LCR",     NO_NEXT_TABLE, User_GenericAccessor, USER_TYPE_HEX16, USER_RO, (void*)FPGA_429_LOOPBACK_CONTROL, -1,-1, NO_LIMIT, NULL},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL arinc429StatusTbl[] =
{
   {"RX", arinc429StatusRxTbl, NULL, NO_HANDLER_DATA},
   {"TX", arinc429StatusTxTbl, NULL, NO_HANDLER_DATA},
   {"INTERRUPT_COUNT", NO_NEXT_TABLE, User_GenericAccessor, USER_TYPE_UINT32, USER_RO, (void*)&m_Arinc429MgrBlock.InterruptCnt, -1,-1, NO_LIMIT, NULL},
   {NULL, NULL, NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL arinc429CfgTbl[] =
{
  {"RX", arinc429CfgRxTbl, NULL, NO_HANDLER_DATA},
  {"TX", arinc429CfgTxTbl, NULL, NO_HANDLER_DATA},
  {"IGNORE_DRVPBIT", NO_NEXT_TABLE, Arinc429Msg_Cfg, USER_TYPE_BOOLEAN, USER_RW, (void*)&bIgnoreDrvPBIT, -1, -1, NO_LIMIT,  NULL},
  {NULL, NULL, NULL, NO_HANDLER_DATA}
};


static USER_MSG_TBL arinc429Root[] =
{
  {"REGISTER",    arinc429RegisterTbl,NULL, NO_HANDLER_DATA},
  {"STATUS",      arinc429StatusTbl,  NULL, NO_HANDLER_DATA},
  {"CFG",         arinc429CfgTbl,     NULL, NO_HANDLER_DATA},
  {"DEBUG",       arinc429DebugTbl,   NULL, NO_HANDLER_DATA},
  {"RECONFIGURE", NO_NEXT_TABLE,  Arinc429Msg_Reconfigure, USER_TYPE_ACTION, USER_RO,               NULL, -1,-1, NO_LIMIT, NULL},
#ifdef GENERATE_SYS_LOGS
  {"CREATELOGS",  NO_NEXT_TABLE,  Arinc429Msg_CreateLogs,  USER_TYPE_ACTION, USER_RO,               NULL, -1,-1, NULL},
#endif
  {DISPLAY_CFG,   NO_NEXT_TABLE,  Arinc429Msg_ShowConfig,  USER_TYPE_ACTION, (USER_RO|USER_NO_LOG|USER_GSE), NULL, -1,-1, NO_LIMIT, NULL},
  {NULL, NULL, NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL arinc429RootTblPtr = {"ARINC429", arinc429Root, NULL, NO_HANDLER_DATA};

#pragma ghs endnowarning
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
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                               or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific item. Range is validated
 *                               by the user manager
 *               [in] SetPtr:    For write commands, a pointer to the data to
 *                               write to the configuration.
 *               [out] GetPtr:   For read commands, UserCfg function will set
 *                               this to the location of the data requested.
 *
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not set.
 *
 * Notes:
 *
*****************************************************************************/
static
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
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                               or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific item. Range is validated
 *                               by the user manager
 *               [in] SetPtr:    For write commands, a pointer to the data to
 *                               write to the configuration.
 *               [out] GetPtr:   For read commands, UserCfg function will set
 *                               this to the location of the data requested.
 *
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not set.
 *
 * Notes:       none
 *
*****************************************************************************/
static
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
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                               or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific item. Range is validated
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
USER_HANDLER_RESULT Arinc429Msg_StateRx(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr)
{
  USER_HANDLER_RESULT result ;

  result = USER_RESULT_OK;

  memcpy ( &arinc429SysStatusTemp_Rx,
           &m_Arinc429MgrBlock.Rx[Index],
           sizeof(ARINC429_SYS_RX_STATUS) );

  arinc429DrvStatusTemp_Rx = *(Arinc429DrvRxGetCounts(Index));

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
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                               or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific item. Range is validated
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
 * Notes:       All Status cmds should be RO
 *
*****************************************************************************/
static
USER_HANDLER_RESULT Arinc429Msg_StateTx(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr)
{
  USER_HANDLER_RESULT result ;

  result = USER_RESULT_OK;

  memcpy ( &arinc429SysStatusTemp_Tx,
           &m_Arinc429MgrBlock.Tx[Index],
           sizeof(ARINC429_SYS_TX_STATUS) );

  arinc429DrvStatusTemp_Tx = *(Arinc429DrvTxGetCounts(Index));

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
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                               or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific item. Range is validated
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
USER_HANDLER_RESULT Arinc429Msg_CfgRx(USER_DATA_TYPE DataType,
                                      USER_MSG_PARAM Param,
                                      UINT32 Index,
                                      const void *SetPtr,
                                      void **GetPtr)
{
  USER_HANDLER_RESULT result;

  result = USER_RESULT_OK;
  // Read data for channel into temp-cfg
  memcpy(&arinc429CfgTemp_RxChan,
         &CfgMgr_ConfigPtr()->Arinc429Config.RxChan[Index],
         sizeof( ARINC429_RX_CFG ) );

  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    // Copy the updated temp-cfg  back to the shadow ram store.
    memcpy( &CfgMgr_ConfigPtr()->Arinc429Config.RxChan[Index],
            &arinc429CfgTemp_RxChan,
            sizeof( ARINC429_RX_CFG ) );

    // Persist the CfgMgr's updated copy back to EEPROM.
    CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
                           &CfgMgr_ConfigPtr()->Arinc429Config.RxChan[Index],
                           sizeof(arinc429CfgTemp_RxChan));
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
* Parameters:   [in] DataType:  C type of the data to be read or changed, used
*                               for casting the data pointers
*               [in/out] Param: Pointer to the configuration item to be read
*                               or changed
*               [in] Index:     Index parameter is used to reference the
*                               specific item. Range is validated
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
static
USER_HANDLER_RESULT Arinc429Msg_CfgTx(USER_DATA_TYPE DataType,
                                      USER_MSG_PARAM Param,
                                      UINT32 Index,
                                      const void *SetPtr,
                                      void **GetPtr)
{
    USER_HANDLER_RESULT result;

    result = USER_RESULT_OK;

  // Read data for channel into temp-cfg
  memcpy(&arinc429CfgTemp_TxChan,
         &CfgMgr_ConfigPtr()->Arinc429Config.TxChan[Index],
         sizeof( ARINC429_TX_CFG ) );

  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    // Copy the updated temp-cfg  back to the shadow ram store.
    memcpy( &CfgMgr_ConfigPtr()->Arinc429Config.TxChan[Index],
            &arinc429CfgTemp_TxChan,
            sizeof( ARINC429_TX_CFG ) );

    // Persist the CfgMgr's updated copy back to EEPROM.
    CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
                           &CfgMgr_ConfigPtr()->Arinc429Config.TxChan[Index],
                           sizeof(arinc429CfgTemp_TxChan));
  }
  return result;
}


/******************************************************************************
 * Function:    Arinc429Msg_Cfg
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retreives the current Arinc429 Cfg Data
 *
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                               or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific item. Range is validated
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
USER_HANDLER_RESULT Arinc429Msg_Cfg(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr)
{
  USER_HANDLER_RESULT result;

  result = USER_RESULT_OK;

  // Read data into temp-cfg
  bIgnoreDrvPBIT = CfgMgr_ConfigPtr()->Arinc429Config.bIgnoreDrvPBIT;

  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    // Copy the updated temp-cfg  back to the shadow ram store.
    CfgMgr_ConfigPtr()->Arinc429Config.bIgnoreDrvPBIT = bIgnoreDrvPBIT;

    // Persist the CfgMgr's updated copy back to EEPROM.
    CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
                           &CfgMgr_ConfigPtr()->Arinc429Config.bIgnoreDrvPBIT,
                           sizeof(BOOLEAN));
  }
  return result;
}


/******************************************************************************
 * Function:    Arinc429Msg_Reconfigure
 *
 * Description: Executes the reconfigure function of Arinc429.c
 *
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                               or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific item. Range is validated
 *                               by the user manager
 *               [in] SetPtr:    For write commands, a pointer to the data to
 *                               write to the configuration.
 *               [out] GetPtr:   For read commands, UserCfg function will set
 *                               this to the location of the data requested.
 *
 * Returns:     USER_RESULT_OK:    Processed successfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
 *****************************************************************************/
static
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
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                               or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific item. Range is validated
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
 *                               specific item to change.  Range is validated
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
static
USER_HANDLER_RESULT Arinc429Msg_ShowConfig(USER_DATA_TYPE DataType,
                                           USER_MSG_PARAM Param,
                                           UINT32 Index,
                                           const void *SetPtr,
                                           void **GetPtr)
{
   CHAR sLabelStem[] = "\r\n\r\nARINC429.CFG.";
   CHAR sLabel[USER_MAX_MSG_STR_LEN * 3];
   //Top-level name is a single indented space
   CHAR sBranchName[USER_MAX_MSG_STR_LEN] = " ";

   USER_HANDLER_RESULT result = USER_RESULT_OK;

   USER_MSG_TBL*  pCfgTable;
   INT16          channelIdx;
   INT16          nMaxChannel;

   pCfgTable = arinc429CfgTbl;
   while (pCfgTable->MsgStr != NULL && result == USER_RESULT_OK)
   {
      nMaxChannel = 0;

      if (strncmp(pCfgTable->MsgStr,"RX", 2) == 0)
      {
         nMaxChannel =  FPGA_MAX_RX_CHAN;
      }
      else if(strncmp(pCfgTable->MsgStr,"TX", 2) == 0)
      {
         nMaxChannel =  FPGA_MAX_TX_CHAN;
      }

      if (nMaxChannel != 0)
      {
         for ( channelIdx = 0;
               channelIdx < nMaxChannel && result == USER_RESULT_OK;
               ++channelIdx)
         {
            // Display element info above each set of data.
            snprintf ( sLabel, sizeof(sLabel), "%s%s[%d]",
                       sLabelStem, pCfgTable->MsgStr, channelIdx);

            result = USER_RESULT_ERROR;
            if (User_OutputMsgString( sLabel, FALSE ) )
            {
              result = User_DisplayConfigTree(sBranchName, pCfgTable->pNext, channelIdx,
                                              0, NULL);
            }
         }
      }
      else
      {
         // Display element info above each set of data.
         snprintf ( sLabel, sizeof(sLabel), "%s%s", sLabelStem, pCfgTable->MsgStr);

         result = USER_RESULT_ERROR;
         if (User_OutputMsgString( sLabel, FALSE ) )
         {
            result = User_DisplayConfigTree(sBranchName, pCfgTable, 0, 0, NULL);
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
 * *****************  Version 51  *****************
 * User: John Omalley Date: 1/12/15    Time: 12:51p
 * Updated in $/software/control processor/code/system
 * SCR 1267 - Code Review Update
 * 
 * *****************  Version 50  *****************
 * User: John Omalley Date: 11/11/14   Time: 4:23p
 * Updated in $/software/control processor/code/system
 * SCR 1267 - Fixed Showcfg parameter for RO|N|G
 * 
 * *****************  Version 49  *****************
 * User: Contractor V&v Date: 9/03/14    Time: 5:14p
 * Updated in $/software/control processor/code/system
 * SCR #1234 - Code Review cleanup
 *
 * *****************  Version 48  *****************
 * User: Contractor V&v Date: 8/14/14    Time: 4:06p
 * Updated in $/software/control processor/code/system
 * SCR #1234 - Configuration update of certain modules overwrites
 *
 * *****************  Version 47  *****************
 * User: John Omalley Date: 4/17/14    Time: 2:05p
 * Updated in $/software/control processor/code/system
 * SCR 1261 - Fixed Data Reduction for BCD parameters
 *
 *
 * *****************  Version 46  *****************
 * User: John Omalley Date: 12-11-27   Time: 6:36p
 * Updated in $/software/control processor/code/system
 * SCR 1203 - ARINC Mgr Show cfg crashes system fix
 *
 * *****************  Version 45  *****************
 * User: John Omalley Date: 12-11-15   Time: 7:04p
 * Updated in $/software/control processor/code/system
 * SCR 1197 - Code Review Updates
 *
 * *****************  Version 44  *****************
 * User: John Omalley Date: 12-11-13   Time: 11:10a
 * Updated in $/software/control processor/code/system
 * SCR 1142 - Code Review Formatting Issues
 *
 * *****************  Version 43  *****************
 * User: John Omalley Date: 12-11-09   Time: 3:51p
 * Updated in $/software/control processor/code/system
 * SCR 1194 - Code Review Updates
 *
 * *****************  Version 42  *****************
 * User: Peter Lee    Date: 12-11-01   Time: 6:44p
 * Updated in $/software/control processor/code/system
 * SCR #1194 Add option to ignore A429 Drv PBIT failures when setting up
 * A429 I/F for operation.
 *
 * *****************  Version 41  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
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
 * SCR 106  XXXUserTable.c consistency
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

