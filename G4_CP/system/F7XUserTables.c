#define F7X_USERTABLES_BODY
/******************************************************************************
            Copyright (C) 2007-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:        F7XUserTables.c
    
    Description: Routines to support the user commands for F7X Protocol CSC 

    VERSION
    $Revision: 15 $  $Date: 10/18/10 5:44p $
    
******************************************************************************/
#ifndef F7X_PROTOCOL_BODY
#error F7XUserTables.c should only be included by UartMgr.c
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
#define MAX_LABEL_CHAR_ARRAY 128
#define MAX_VALUE_CHAR_ARRAY 32

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
USER_HANDLER_RESULT F7XMsg_Status(USER_DATA_TYPE DataType,
                                  USER_MSG_PARAM Param,
                                  UINT32 Index,
                                  const void *SetPtr,
                                  void **GetPtr);

USER_HANDLER_RESULT F7XMsg_Cfg(USER_DATA_TYPE DataType,
                               USER_MSG_PARAM Param,
                               UINT32 Index,
                               const void *SetPtr,
                               void **GetPtr);
                               
USER_HANDLER_RESULT F7XMsg_Debug(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr);

USER_HANDLER_RESULT F7XMsg_ParamList(USER_DATA_TYPE DataType,
                                     USER_MSG_PARAM Param,
                                     UINT32 Index,
                                     const void *SetPtr,
                                     void **GetPtr);

USER_HANDLER_RESULT F7XMsg_ParamEntryList(USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr);

USER_HANDLER_RESULT F7XMsg_DumpListShowConfig(USER_DATA_TYPE DataType,
                                              USER_MSG_PARAM Param,
                                              UINT32 Index,
                                              const void *SetPtr,
                                              void **GetPtr);


USER_HANDLER_RESULT F7XMsg_ParamListShowConfig(USER_DATA_TYPE DataType,
                                               USER_MSG_PARAM Param,
                                               UINT32 Index,
                                               const void *SetPtr,
                                               void **GetPtr);


/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static F7X_STATUS F7XStatusTemp; 
static F7X_DUMPLIST_CFG F7XCfgTemp; 

static F7X_PARAM_LIST F7XParamListTemp; 
static F7X_PARAM F7XParamEntryTemp; 

static F7X_DEBUG F7XDebugTemp; 


/*****************************************/
/* User Table Defintions                 */
/*****************************************/
USER_MSG_TBL F7XStatusTbl[] = 
{
  {"SYNC",            NO_NEXT_TABLE, F7XMsg_Status, USER_TYPE_BOOLEAN, USER_RO, (void *) &F7XStatusTemp.sync,         1,UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},
  {"ADDR_CHKSUM",     NO_NEXT_TABLE, F7XMsg_Status, USER_TYPE_HEX32,   USER_RO, (void *) &F7XStatusTemp.AddrChksum,   1,UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},
  {"DUMPLIST_SIZE",   NO_NEXT_TABLE, F7XMsg_Status, USER_TYPE_UINT16,  USER_RO, (void *) &F7XStatusTemp.DumpListSize, 1,UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},
  {"RECOGNIZED",      NO_NEXT_TABLE, F7XMsg_Status, USER_TYPE_BOOLEAN, USER_RO, (void *) &F7XStatusTemp.bRecognized,  1,UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},
  {"INDEX_CFG",       NO_NEXT_TABLE, F7XMsg_Status, USER_TYPE_UINT16,  USER_RO, (void *) &F7XStatusTemp.nIndexCfg,    1,UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},
  {"FRAME_CNT",       NO_NEXT_TABLE, F7XMsg_Status, USER_TYPE_UINT32,  USER_RO, (void *) &F7XStatusTemp.nFrames,      1,UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},
  {"RESYNC_CNT",      NO_NEXT_TABLE, F7XMsg_Status, USER_TYPE_UINT32,  USER_RO, (void *) &F7XStatusTemp.nResyncs,     1,UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},
  {"LAST_FRAME_TIME", NO_NEXT_TABLE, F7XMsg_Status, USER_TYPE_UINT32,  USER_RO, (void *) &F7XStatusTemp.lastFrameTime,1,UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};


// Cfg table substruct used by dumplist
USER_MSG_TBL F7XCfgTbl[] = 
{ /*Str           Next Tbl Ptr    Handler Func.    Data Type          Access         Parameter                         IndexRange             DataLimit    EnumTbl*/
  {"ADDR_CHKSUM", NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_HEX32,  USER_RW, (void *) &F7XCfgTemp.AddrChksum,  0,F7X_MAX_DUMPLISTS-1, NO_LIMIT,    NULL},
  {"NUM_PARAMS",  NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.NumParams,   0,F7X_MAX_DUMPLISTS-1, 32,128,      NULL},
  {"W_1",         NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[0],  0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_2",         NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[1],  0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_3",         NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[2],  0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_4",         NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[3],  0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_5",         NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[4],  0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_6",         NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[5],  0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_7",         NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[6],  0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_8",         NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[7],  0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_9",         NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[8],  0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_10",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[9],  0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_11",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[10], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_12",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[11], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_13",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[12], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_14",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[13], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_15",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[14], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_16",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[15], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_17",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[16], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_18",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[17], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_19",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[18], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_20",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[19], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_21",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[20], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_22",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[21], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_23",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[22], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_24",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[23], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_25",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[24], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_26",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[25], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_27",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[26], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_28",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[27], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_29",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[28], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_30",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[29], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_31",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[30], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_32",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[31], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_33",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[32], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_34",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[33], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_35",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[34], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_36",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[35], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_37",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[36], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_38",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[37], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_39",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[38], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_40",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[39], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_41",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[40], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_42",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[41], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_43",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[42], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_44",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[43], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_45",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[44], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_46",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[45], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_47",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[46], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_48",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[47], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_49",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[48], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_50",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[49], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_51",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[50], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_52",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[51], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_53",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[52], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_54",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[53], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_55",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[54], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_56",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[55], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_57",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[56], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_58",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[57], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_59",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[58], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_60",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[59], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_61",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[60], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_62",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[61], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_63",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[62], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_64",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[63], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_65",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[64], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_66",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[65], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_67",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[66], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_68",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[67], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_69",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[68], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_70",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[69], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_71",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[70], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_72",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[71], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_73",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[72], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_74",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[73], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_75",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[74], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_76",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[75], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_77",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[76], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_78",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[77], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_79",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[78], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},          
  {"W_80",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[79], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_81",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[80], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_82",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[81], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_83",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[82], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_84",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[83], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_85",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[84], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_86",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[85], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_87",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[86], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_88",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[87], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_89",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[88], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_90",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[89], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_91",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[90], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_92",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[91], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_93",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[92], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_94",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[93], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_95",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[94], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_96",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[95], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_97",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[96], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_98",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[97], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_99",        NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[98], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_100",       NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[99], 0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_101",       NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[100],0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_102",       NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[101],0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_103",       NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[102],0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_104",       NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[103],0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_105",       NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[104],0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_106",       NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[105],0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_107",       NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[106],0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_108",       NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[107],0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_109",       NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[108],0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_110",       NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[109],0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_111",       NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[110],0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_112",       NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[111],0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_113",       NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[112],0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_114",       NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[113],0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_115",       NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[114],0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_116",       NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[115],0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_117",       NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[116],0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_118",       NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[117],0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_119",       NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[118],0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_120",       NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[119],0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_121",       NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[120],0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_122",       NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[121],0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_123",       NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[122],0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_124",       NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[123],0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_125",       NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[124],0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_126",       NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[125],0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_127",       NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[126],0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {"W_128",       NO_NEXT_TABLE, F7XMsg_Cfg,  USER_TYPE_UINT16, USER_RW, (void *) &F7XCfgTemp.ParamId[127],0,F7X_MAX_DUMPLISTS-1,NO_LIMIT,NULL},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
  
};


USER_MSG_TBL F7XDebugTbl[] = 
{
  {"ENABLE",   NO_NEXT_TABLE,F7XMsg_Debug, USER_TYPE_BOOLEAN,(USER_RW|USER_GSE),(void *) &F7XDebugTemp.bDebug,    -1,-1,NO_LIMIT,NULL},
  {"CHANNEL",  NO_NEXT_TABLE,F7XMsg_Debug, USER_TYPE_UINT16, (USER_RW|USER_GSE),(void *) &F7XDebugTemp.Ch,        -1,-1,1,3,     NULL},
  {"FORMATTED",NO_NEXT_TABLE,F7XMsg_Debug, USER_TYPE_BOOLEAN,(USER_RW|USER_GSE),(void *) &F7XDebugTemp.bFormatted,-1,-1,NO_LIMIT,NULL},
  {"PARAM0",   NO_NEXT_TABLE,F7XMsg_Debug, USER_TYPE_HEX32,  (USER_RW|USER_GSE),(void *) &F7XDebugTemp.Param0,    -1,-1,NO_LIMIT,NULL},
  {"PARAM1",   NO_NEXT_TABLE,F7XMsg_Debug, USER_TYPE_HEX32,  (USER_RW|USER_GSE),(void *) &F7XDebugTemp.Param1,    -1,-1,NO_LIMIT,NULL},
  {"PARAM2",   NO_NEXT_TABLE,F7XMsg_Debug, USER_TYPE_HEX32,  (USER_RW|USER_GSE),(void *) &F7XDebugTemp.Param2,    -1,-1,NO_LIMIT,NULL},
  {"PARAM3",   NO_NEXT_TABLE,F7XMsg_Debug, USER_TYPE_HEX32,  (USER_RW|USER_GSE),(void *) &F7XDebugTemp.Param3,    -1,-1,NO_LIMIT,NULL},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};


USER_MSG_TBL F7XParamEntryCfgTbl[] = 
{ /*Str          Next Tbl Ptr    Handler Func.          Data Type          Access     Parameter                            IndexRange          DataLimit    EnumTbl*/
  {"SCALE",      NO_NEXT_TABLE,  F7XMsg_ParamEntryList, USER_TYPE_UINT16,  USER_RW,   (void *) &F7XParamEntryTemp.scale,   0,F7X_MAX_PARAMS-2, NO_LIMIT,    NULL},
  {"TOL",        NO_NEXT_TABLE,  F7XMsg_ParamEntryList, USER_TYPE_FLOAT,   USER_RW,   (void *) &F7XParamEntryTemp.tol,     0,F7X_MAX_PARAMS-2, NO_LIMIT,    NULL},
  {"TIMEOUT_MS", NO_NEXT_TABLE,  F7XMsg_ParamEntryList, USER_TYPE_UINT32,  USER_RW,   (void *) &F7XParamEntryTemp.timeout, 0,F7X_MAX_PARAMS-2, NO_LIMIT,    NULL},
  {NULL,         NULL,    NULL,  NO_HANDLER_DATA}
};

#define NUM_PARAMS_STRING  "NUM_PARAMS"
USER_MSG_TBL F7XParamCfgTbl[] = 
{ /*Str               Next Tbl Ptr          Handler Func.       Data Type           Access      Parameter                            IndexRange     DataLimit           EnumTbl*/
  {"VERSION",         NO_NEXT_TABLE,        F7XMsg_ParamList,   USER_TYPE_UINT16,   USER_RW,    (void *) &F7XParamListTemp.ver,       -1,-1,        NO_LIMIT,           NULL},
  {NUM_PARAMS_STRING, NO_NEXT_TABLE,        F7XMsg_ParamList,   USER_TYPE_UINT16,   USER_RW,    (void *) &F7XParamListTemp.cnt,       -1,-1,        0,F7X_MAX_PARAMS-2, NULL},
  {"P",               F7XParamEntryCfgTbl,  NULL,               NO_HANDLER_DATA},
  {NULL,              NULL,                 NULL,               NO_HANDLER_DATA}
};



USER_MSG_TBL F7XProtocolRoot[] = 
{
  {"STATUS",F7XStatusTbl,NULL,NO_HANDLER_DATA},
  {"DEBUG" ,F7XDebugTbl ,NULL,NO_HANDLER_DATA},  
/*  
#ifdef GENERATE_SYS_LOGS  
  {"CREATELOGS",NO_NEXT_TABLE,Arinc429Msg_CreateLogs,   USER_TYPE_ACTION, USER_RO,               NULL, -1,-1, NULL},
#endif
*/    
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

USER_MSG_TBL F7XDumpListRoot[] = 
{ /*Str            Next Tbl Ptr    Handler Func.             Data Type          Access                        Parameter     IndexRange  DataLimit    EnumTbl*/
  {"CFG",          F7XCfgTbl,     NULL,                      NO_HANDLER_DATA},
  {DISPLAY_CFG,    NO_NEXT_TABLE, F7XMsg_DumpListShowConfig, USER_TYPE_ACTION, (USER_RO|USER_NO_LOG|USER_GSE), NULL,        -1,-1,      NO_LIMIT,    NULL},
  {NULL,           NULL,          NULL,                      NO_HANDLER_DATA}
};

USER_MSG_TBL F7XParamRoot[] = 
{ /*Str            Next Tbl Ptr    Handler Func.                 Data Type          Access                        Parameter     IndexRange  DataLimit    EnumTbl*/
  {"CFG",          F7XParamCfgTbl, NULL,NO_HANDLER_DATA},
  {DISPLAY_CFG,    NO_NEXT_TABLE,  F7XMsg_ParamListShowConfig,  USER_TYPE_ACTION, (USER_RO|USER_NO_LOG|USER_GSE), NULL,        -1,-1,      NO_LIMIT,    NULL},
  {NULL,           NULL,           NULL,                         NO_HANDLER_DATA}
};



USER_MSG_TBL F7XProtocolRootTblPtr = {"F7X",F7XProtocolRoot,NULL,NO_HANDLER_DATA};
USER_MSG_TBL F7XProtocolDumpListTblPtr = {"F7X_DUMPLIST",F7XDumpListRoot,NULL,NO_HANDLER_DATA};
USER_MSG_TBL F7XProtocolParamTblPtr = {"F7X_PARAM",F7XParamRoot,NULL,NO_HANDLER_DATA};

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
 * Function:    F7XMsg_Status 
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves the latest F7X Status Data 
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
USER_HANDLER_RESULT F7XMsg_Status(USER_DATA_TYPE DataType,
                                  USER_MSG_PARAM Param,
                                  UINT32 Index,
                                  const void *SetPtr,
                                  void **GetPtr)
{
  USER_HANDLER_RESULT result ; 

  result = USER_RESULT_OK; 
  
  F7XStatusTemp = *F7XProtocol_GetStatus(Index); 
  
  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);
  
  return result;
}
 


/******************************************************************************
 * Function:    F7XMsg_Cfg
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves and set the latest F7X Cfg Data 
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
USER_HANDLER_RESULT F7XMsg_Cfg(USER_DATA_TYPE DataType,
                               USER_MSG_PARAM Param,
                               UINT32 Index,
                               const void *SetPtr,
                               void **GetPtr)
{
  USER_HANDLER_RESULT result;

  result = USER_RESULT_OK; 


  // Determine which array element 
  
  F7XCfgTemp = *F7XProtocol_GetCfg ( Index ); 

  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    *F7XProtocol_GetCfg(Index) = F7XCfgTemp; 

    memcpy(&CfgMgr_ConfigPtr()->F7XConfig[Index], &F7XCfgTemp, 
           sizeof(F7X_DUMPLIST_CFG));
           
    //Store the modified temporary structure in the EEPROM.       
    CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),&CfgMgr_ConfigPtr()->F7XConfig[Index],
                           sizeof(F7X_DUMPLIST_CFG));
  }
  return result;  

}



/******************************************************************************
 * Function:    F7XMsg_Debug
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves the latest F7X Debug Data 
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
USER_HANDLER_RESULT F7XMsg_Debug(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr)
{
  USER_HANDLER_RESULT result ; 

  result = USER_RESULT_OK; 
  
  F7XDebugTemp = *F7XProtocol_GetDebug(); 
  
  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);
  
  *F7XProtocol_GetDebug() = F7XDebugTemp; 
  
  return result;
}


/******************************************************************************
 * Function:    F7XMsg_ParamList
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves and set the latest F7X Param Cfg Data 
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
USER_HANDLER_RESULT F7XMsg_ParamList(USER_DATA_TYPE DataType,
                                     USER_MSG_PARAM Param,
                                     UINT32 Index,
                                     const void *SetPtr,
                                     void **GetPtr)
{
  USER_HANDLER_RESULT result;

  result = USER_RESULT_OK; 


  // Determine which array element 
  
  F7XParamListTemp = *F7XProtocol_GetParamList(); 

  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    *F7XProtocol_GetParamList() = F7XParamListTemp; 

    memcpy(&CfgMgr_ConfigPtr()->F7XParamConfig, &F7XParamListTemp, 
           sizeof(F7X_PARAM_LIST));
           
    //Store the modified temporary structure in the EEPROM.       
    CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),&CfgMgr_ConfigPtr()->F7XParamConfig,
                           sizeof(F7X_PARAM_LIST));
  }
  return result;  

}


/******************************************************************************
 * Function:    F7XMsg_ParamEntryList
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves and set the latest F7X Param Cfg Data 
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
USER_HANDLER_RESULT F7XMsg_ParamEntryList(USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr)
{
  USER_HANDLER_RESULT result;

  result = USER_RESULT_OK; 


  // Grab the entry 
  F7XParamEntryTemp = *F7XProtocol_GetParamEntryList(Index); 

  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    // Update the run time entry and the copy in CfgMgr
    *F7XProtocol_GetParamEntryList(Index) = F7XParamEntryTemp; 

    memcpy(&CfgMgr_ConfigPtr()->F7XParamConfig.param[Index], &F7XParamEntryTemp, 
           sizeof(F7X_PARAM));
           
    // Store the modified temporary structure in the EEPROM.       
    CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
                           &CfgMgr_ConfigPtr()->F7XParamConfig.param[Index],
                           sizeof(F7X_PARAM));
  }
  return result;  

}


/******************************************************************************
* Function:    F7XMsg_DumpListShowConfig
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
USER_HANDLER_RESULT F7XMsg_DumpListShowConfig(USER_DATA_TYPE DataType,
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
  INT16 idx;
  
  result = USER_RESULT_OK;

  // Loop for each param in the dumplist.
  for (idx = 0; idx < F7X_MAX_DUMPLISTS && result == USER_RESULT_OK; ++idx)
  {
    // Display sensor id info above each set of data.
    sprintf(Label, "\r\n\r\nF7X_DUMPLIST[%d].CFG", idx);
    result = USER_RESULT_ERROR;
    if (User_OutputMsgString( Label, FALSE ) )
    {
      pCfgTable = F7XCfgTbl;  // Re-set the pointer to beginning of CFG table       

      // User_DisplayConfigTree will invoke itself recursively to display all fields.
      result = User_DisplayConfigTree(BranchName, pCfgTable, idx, 0, NULL);
    }     
  } 
  return result;
}

/******************************************************************************
* Function:    F7XMsg_ParamListShowConfig
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

USER_HANDLER_RESULT F7XMsg_ParamListShowConfig(USER_DATA_TYPE DataType,
                                              USER_MSG_PARAM Param,
                                              UINT32 Index,
                                              const void *SetPtr,
                                              void **GetPtr)
{
  INT16 idx;
  CHAR  LabelStem[MAX_LABEL_CHAR_ARRAY] = "\r\n\r\nF7X_PARAM.CFG";
  CHAR  Label[USER_MAX_MSG_STR_LEN * 3];
  CHAR  Value[MAX_VALUE_CHAR_ARRAY];
  
  INT16  numParams = F7X_MAX_PARAMS - 2;
  USER_HANDLER_RESULT result; 
  
  // Declare a GetPtr to be used locally since
  // the GetPtr passed in is "NULL" because this funct is a user-action.
  UINT32 TempInt;
  void*  LocalPtr = &TempInt;
  
  //Top-level name is a single indented space
  CHAR BranchName[USER_MAX_MSG_STR_LEN] = " ";
  CHAR NextBranchName[USER_MAX_MSG_STR_LEN] = "  ";

  USER_MSG_TBL* pCfgTable;  
  result = USER_RESULT_OK;

  /*
  F7X_PARAM.CFG is of the form:
  Level 1 F7X_PARAM
    Level 2   CFG
    Level 3     VERSION
    Level 3     NUM_PARAMS
    Level 3     P[0]
      Level 4       SCALE
      Level 4       TOL
      Level 4       TIMEOUT_MS
         ...
    Level 3     P[254]
      Level 4       SCALE
      Level 4       TOL
      Level 4       TIMEOUT_MS
  */
  
  // Display label for F7X_PARAM.CFG
  User_OutputMsgString( LabelStem, FALSE);
  
  /*** Process the leaf elements in Levels 1->3  ***/ 
  pCfgTable = F7XParamCfgTbl;
  
  idx = 0;  
  while ( pCfgTable->MsgStr  != NULL             &&
          pCfgTable->pNext   == NO_NEXT_TABLE    &&
          pCfgTable->MsgType != USER_TYPE_ACTION &&        
          result == USER_RESULT_OK )
  {    
    sprintf(  Label,"\r\n%s%s =", BranchName, pCfgTable->MsgStr);
    PadString(Label, USER_MAX_MSG_STR_LEN + 15);
    result = USER_RESULT_ERROR;
    if (User_OutputMsgString( Label, FALSE ))
    {
      result = USER_RESULT_OK; 

      // Call user function for the table entry
      if( USER_RESULT_OK != pCfgTable->MsgHandler(pCfgTable->MsgType, pCfgTable->MsgParam,
                                                idx, NULL, &LocalPtr))
      {
        User_OutputMsgString( USER_MSG_INVALID_CMD_FUNC, FALSE);
      }
      else
      {
        // If this was the number-of-params value, store it so we can minimize the
        // the number of param-entry lookups performed in the next step.
        // (Only accept the param value if it is less than the initialized default-value.)
        if (0 == strncmp(pCfgTable->MsgStr, NUM_PARAMS_STRING, sizeof(pCfgTable->MsgStr)) )
        {
          numParams = *(INT16*)LocalPtr <= numParams ? *(INT16*)LocalPtr : numParams;
        }

        // Convert the param to string.
        if( User_CvtGetStr(pCfgTable->MsgType, Value, sizeof(Value), 
                                          LocalPtr, pCfgTable->MsgEnumTbl))
        {
          result = USER_RESULT_ERROR;
          if (User_OutputMsgString( Value, FALSE ))
          {
            result = USER_RESULT_OK; 
          }
        }
        else
        {
          //Conversion error
          User_OutputMsgString( USER_MSG_RSP_CONVERSION_ERR, FALSE );
        }
      }
    }
    pCfgTable++;
  }

  /*** Process the array of structs at Level-4 ***/

  // Substruct name was the first entry to terminate the while-loop above
   strncpy_safe(LabelStem, sizeof(LabelStem), pCfgTable->MsgStr, _TRUNCATE);
   
   // Set pointer to array of param entry structs table   
   pCfgTable = &F7XParamEntryCfgTbl[0];

   for (idx = 0; idx < numParams && result == USER_RESULT_OK; ++idx)
   {
      // Display element info above each set of data.
      sprintf(Label, "\r\n\r\n%s%s[%d]",BranchName, LabelStem, idx);      
      
      // Assume the worse, to terminate this for-loop 
      result = USER_RESULT_ERROR;
      if ( User_OutputMsgString( Label, FALSE ) )
      {
         result = User_DisplayConfigTree(NextBranchName, pCfgTable, idx, 0, NULL);
      }
   }   
  return result;
}


/*****************************************************************************
 *  MODIFICATIONS
 *    $History: F7XUserTables.c $
 * 
 * *****************  Version 15  *****************
 * User: John Omalley Date: 10/18/10   Time: 5:44p
 * Updated in $/software/control processor/code/system
 * SCR 953 - Fixed Output Msg Problem (Value vs. Label)
 * 
 * *****************  Version 14  *****************
 * User: Contractor2  Date: 10/18/10   Time: 4:12p
 * Updated in $/software/control processor/code/system
 * SCR #953 F7XMsg_ParamListShowConfig has code-coverage holes.
 * Fix incorrect implementation
 * 
 * *****************  Version 13  *****************
 * User: Contractor2  Date: 10/18/10   Time: 3:28p
 * Updated in $/software/control processor/code/system
 * SCR #953 F7XMsg_ParamListShowConfig has code-coverage holes.
 * 
 * *****************  Version 12  *****************
 * User: John Omalley Date: 10/15/10   Time: 9:32a
 * Updated in $/software/control processor/code/system
 * SCR 938 - Code Review Updates
 * 
 * *****************  Version 11  *****************
 * User: Peter Lee    Date: 8/03/10    Time: 6:26p
 * Updated in $/software/control processor/code/system
 * SCR #770 .num_params from 0 to 254.
 * 
 * *****************  Version 10  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:01a
 * Updated in $/software/control processor/code/system
 * SCR #685 - Add USER_GSE flag
 * 
 * *****************  Version 9  *****************
 * User: Contractor V&v Date: 7/22/10    Time: 6:49p
 * Updated in $/software/control processor/code/system
 * SCR #643  Add showcfg to UartMgrUserTable and F7XUserTable
 * 
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 7/12/10    Time: 3:31p
 * Updated in $/software/control processor/code/system
 * SCR #665 F7X_Param range from 0 to 254 not 0 to 256 ! 
 * 
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 6/28/10    Time: 5:17p
 * Updated in $/software/control processor/code/system
 * SCR #635 Force Uart and F7X min array to '1' from '0' since [0]
 * restricted to GSE port.  
 * 
 * *****************  Version 6  *****************
 * User: Contractor V&v Date: 6/22/10    Time: 6:24p
 * Updated in $/software/control processor/code/system
 * SCR #643 Add showcfg to UartMgrUserTable and F7XUserTable
 * 
 * *****************  Version 5  *****************
 * User: Peter Lee    Date: 6/21/10    Time: 3:37p
 * Updated in $/software/control processor/code/system
 * SCR #635 items 17 and 18.  Update debug display to display new frames
 * only.  Make all f7x_dumplist.cfg[] and f7x_param.cfg[] entries blank.  
 * 
 * *****************  Version 4  *****************
 * User: Contractor V&v Date: 6/16/10    Time: 6:09p
 * Updated in $/software/control processor/code/system
 * SCR #643 Implement showcfg for F7X/Uart
 * 
 * *****************  Version 3  *****************
 * User: Peter Lee    Date: 6/15/10    Time: 3:54p
 * Updated in $/software/control processor/code/system
 * SCR #635.  Add limit of 32 to 128 for num param.  Updated "timeout" to
 * "timeout_ms".
 * 
 * *****************  Version 2  *****************
 * User: Peter Lee    Date: 6/10/10    Time: 2:15p
 * Updated in $/software/control processor/code/system
 * SCR #365 Update better debug display for F7X. 
 * 
 * *****************  Version 1  *****************
 * User: Peter Lee    Date: 5/31/10    Time: 5:55p
 * Created in $/software/control processor/code/system
 * Initial Check In.  Implementation of the F7X and UartMgr Requirements. 
 * 
 * 
 *
 *****************************************************************************/
