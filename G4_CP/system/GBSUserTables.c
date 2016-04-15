#define GBS_USERTABLES_BODY
/******************************************************************************
            Copyright (C) 2007-2016 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    ECCN:        9D991

    File:        GBSUserTables.c

    Description: Routines to support the user commands for GBS Protocol CSC

    VERSION
    $Revision: 13 $  $Date: 4/14/16 4:00p $

******************************************************************************/
#ifndef GBS_PROTOCOL_BODY
#error GBSUserTables.c should only be included by GBSProtocol.c
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


/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static GBS_STATUS gbsStatusTemp;
static GBS_CFG gbsCfgTemp;
static GBS_CTL_CFG gbsCtlTemp;
static GBS_MULTI_CTL gbsCtlStatusTemp; 
static GBS_DEBUG_CTL gbsDebugTemp;
static GBS_DEBUG_DNLOAD gbsDebugDnloadTemp; 


/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
//Prototype for the User Manager message handlers, has to go before
//the local variable tables that use the function pointer.
static USER_HANDLER_RESULT GBSMsg_Status     ( USER_DATA_TYPE DataType,
                                               USER_MSG_PARAM Param,
                                               UINT32 Index,
                                               const void *SetPtr,
                                               void **GetPtr );
                                               
static USER_HANDLER_RESULT GBSMsg_SimStatus  ( USER_DATA_TYPE DataType,
                                               USER_MSG_PARAM Param,
                                               UINT32 Index,
                                               const void *SetPtr,
                                               void **GetPtr );    
                                               
static USER_HANDLER_RESULT GBSMsg_CtlStatus  ( USER_DATA_TYPE DataType,
                                               USER_MSG_PARAM Param,
                                               UINT32 Index,
                                               const void *SetPtr,
                                               void **GetPtr );                                                                                              

static USER_HANDLER_RESULT GBSMsg_CmdCfg     ( USER_DATA_TYPE DataType,
                                               USER_MSG_PARAM Param,
                                               UINT32 Index,
                                               const void *SetPtr,
                                               void **GetPtr );
                                               
static USER_HANDLER_RESULT GBSMsg_CtlCfg     ( USER_DATA_TYPE DataType,
                                               USER_MSG_PARAM Param,
                                               UINT32 Index,
                                               const void *SetPtr,
                                               void **GetPtr );                                               

static USER_HANDLER_RESULT GBSMsg_CtlDebugClear ( USER_DATA_TYPE DataType,
                                                  USER_MSG_PARAM Param,
                                                  UINT32 Index,
                                                  const void *SetPtr,
                                                  void **GetPtr );
                                               
static USER_HANDLER_RESULT GBSMsg_CtlDebug   ( USER_DATA_TYPE DataType,
                                               USER_MSG_PARAM Param,
                                               UINT32 Index,
                                               const void *SetPtr,
                                               void **GetPtr );


/*
static USER_HANDLER_RESULT GBSMsg_ShowConfig ( USER_DATA_TYPE DataType,
                                               USER_MSG_PARAM Param,
                                               UINT32 Index,
                                               const void *SetPtr,
                                               void **GetPtr );
*/
/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static
USER_ENUM_TBL gbsStateStrs[] =
{
  {"GBS_IDLE",     (INT32) GBS_STATE_IDLE},
  {"GBS_LRU_SEL",  (INT32) GBS_STATE_SET_EDU_MODE},
  {"GBS_DOWNLOAD", (INT32) GBS_STATE_DOWNLOAD},
  {"GBS_CONFIRM",  (INT32) GBS_STATE_CONFIRM},
  {"GBS_STATE_COMPLETE", (INT32) GBS_STATE_COMPLETE},
  {"GBS_STATE_RESTART_DELAY", (INT32) GBS_STATE_RESTART_DELAY},
  { NULL,          0}
};

static
USER_ENUM_TBL gbsMultiStateStrs[] =
{
  {"PRIMARY",   (INT32) GBS_MULTI_PRIMARY},
  {"SECONDARY", (INT32) GBS_MULTI_SECONDARY},
  { NULL,             0}
};


// GBS *******************************************************************
static USER_MSG_TBL gbsCmdCodeTbl[] =
{ /*Str               Next Tbl Ptr   Handler Func.   Data Type          Access   Parameter                             IndexRange DataLimit EnumTbl*/
  {"CODE_0",          NO_NEXT_TABLE, GBSMsg_CmdCfg,  USER_TYPE_UINT8,   USER_RW, (void *) &gbsCfgTemp.dnloadTypes[0],  1,(INT32) UART_NUM_OF_UARTS-1,NO_LIMIT,NULL}, 
//{"CODE_1",          NO_NEXT_TABLE, GBSMsg_CmdCfg,  USER_TYPE_UINT8,   USER_RW, (void *) &gbsCfgTemp.dnloadTypes[1],  1,(INT32) UART_NUM_OF_UARTS-1,NO_LIMIT,NULL}, 
//{"CODE_2",          NO_NEXT_TABLE, GBSMsg_CmdCfg,  USER_TYPE_UINT8,   USER_RW, (void *) &gbsCfgTemp.dnloadTypes[2],  1,(INT32) UART_NUM_OF_UARTS-1,NO_LIMIT,NULL}, 
//{"CODE_3",          NO_NEXT_TABLE, GBSMsg_CmdCfg,  USER_TYPE_UINT8,   USER_RW, (void *) &gbsCfgTemp.dnloadTypes[3],  1,(INT32) UART_NUM_OF_UARTS-1,NO_LIMIT,NULL}, 
//{"CODE_4",          NO_NEXT_TABLE, GBSMsg_CmdCfg,  USER_TYPE_UINT8,   USER_RW, (void *) &gbsCfgTemp.dnloadTypes[4],  1,(INT32) UART_NUM_OF_UARTS-1,NO_LIMIT,NULL}, 
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL gbsCfgTbl[] =
{ 
  {"CMD", gbsCmdCodeTbl,      NULL,          NO_HANDLER_DATA},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL gbsStatusTbl[] =
{ /*Str               Next Tbl Ptr   Handler Func.  Data Type          Access   Parameter                                            IndexRange DataLimit EnumTbl*/
  {"STATE",           NO_NEXT_TABLE, GBSMsg_Status, USER_TYPE_ENUM,    USER_RO, (void *) &gbsStatusTemp.state,                       1,(INT32) UART_NUM_OF_UARTS-1,NO_LIMIT,gbsStateStrs},
  {"RESTART_LEFT",    NO_NEXT_TABLE, GBSMsg_Status, USER_TYPE_UINT32,  USER_RO, (void *) &gbsStatusTemp.nRetriesCurr,                1,(INT32) UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},
  {"BLK_CURR",        NO_NEXT_TABLE, GBSMsg_Status, USER_TYPE_UINT16,  USER_RO, (void *) &gbsStatusTemp.dataBlkState.cntBlkCurr,     1,(INT32) UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},
  {"BLK_EXP",         NO_NEXT_TABLE, GBSMsg_Status, USER_TYPE_UINT16,  USER_RO, (void *) &gbsStatusTemp.cntBlkSizeExp,               1,(INT32) UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},
  {"BLK_SIZE_TOTAL",  NO_NEXT_TABLE, GBSMsg_Status, USER_TYPE_UINT32,  USER_RO, (void *) &gbsStatusTemp.cntDnLoadSizeExp,            1,(INT32) UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},
  {"BLK_RX",          NO_NEXT_TABLE, GBSMsg_Status, USER_TYPE_UINT32,  USER_RO, (void *) &gbsStatusTemp.dataBlkState.cntBlkRx,       1,(INT32) UART_NUM_OF_UARTS-1,NO_LIMIT,NULL}, 
  {"BLK_PROBLEM",     NO_NEXT_TABLE, GBSMsg_Status, USER_TYPE_UINT32,  USER_RO, (void *) &gbsStatusTemp.dataBlkState.cntBlkBadRx,    1,(INT32) UART_NUM_OF_UARTS-1,NO_LIMIT,NULL}, 
  {"BLK_BAD",         NO_NEXT_TABLE, GBSMsg_Status, USER_TYPE_UINT32,  USER_RO, (void *) &gbsStatusTemp.dataBlkState.cntBlkBadTotal, 1,(INT32) UART_NUM_OF_UARTS-1,NO_LIMIT,NULL}, 
  {"BLK_BAD_STORE",   NO_NEXT_TABLE, GBSMsg_Status, USER_TYPE_UINT32,  USER_RO, (void *) &gbsStatusTemp.dataBlkState.cntStoreBad,    1,(INT32) UART_NUM_OF_UARTS-1,NO_LIMIT,NULL}, 
  {"DL_INTERRUPTED",  NO_NEXT_TABLE, GBSMsg_Status, USER_TYPE_BOOLEAN, USER_RO, (void *) &gbsStatusTemp.bDownloadInterrupted,        1,(INT32) UART_NUM_OF_UARTS-1,NO_LIMIT,NULL}, 
  {"DL_COMPLETED",    NO_NEXT_TABLE, GBSMsg_Status, USER_TYPE_BOOLEAN, USER_RO, (void *) &gbsStatusTemp.bCompleted,                  1,(INT32) UART_NUM_OF_UARTS-1,NO_LIMIT,NULL}, 
  {"DL_REQ_CNT",      NO_NEXT_TABLE, GBSMsg_Status, USER_TYPE_UINT16,  USER_RO, (void *) &gbsStatusTemp.cntDnloadReq,                1,(INT32) UART_NUM_OF_UARTS-1,NO_LIMIT,NULL}, 
  {"BLK_NVM",         NO_NEXT_TABLE, GBSMsg_Status, USER_TYPE_UINT16,  USER_RO, (void *) &gbsStatusTemp.cntBlkSizeNVM,               1,(INT32) UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},
  {"RELAY_STUCK",     NO_NEXT_TABLE, GBSMsg_Status, USER_TYPE_BOOLEAN, USER_RO, (void *) &gbsStatusTemp.bRelayStuck,                 1,(INT32) UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};
// GBS *******************************************************************


// GBS_SIM ***************************************************************
static USER_MSG_TBL gbsSimStatusTbl[] =
{ /*Str               Next Tbl Ptr   Handler Func.     Data Type          Access   Parameter                                            IndexRange DataLimit EnumTbl*/
  {"STATE",           NO_NEXT_TABLE, GBSMsg_SimStatus, USER_TYPE_ENUM,    USER_RO, (void *) &gbsStatusTemp.state,                       -1,-1,  NO_LIMIT,gbsStateStrs},
  {"RESTART_LEFT",    NO_NEXT_TABLE, GBSMsg_SimStatus, USER_TYPE_UINT32,  USER_RO, (void *) &gbsStatusTemp.nRetriesCurr,                -1,-1,  NO_LIMIT,NULL},
  {"BLK_CURR",        NO_NEXT_TABLE, GBSMsg_SimStatus, USER_TYPE_UINT16,  USER_RO, (void *) &gbsStatusTemp.dataBlkState.cntBlkCurr,     -1,-1,  NO_LIMIT,NULL},
  {"BLK_EXP",         NO_NEXT_TABLE, GBSMsg_SimStatus, USER_TYPE_UINT16,  USER_RO, (void *) &gbsStatusTemp.cntBlkSizeExp,               -1,-1,  NO_LIMIT,NULL},
  {"BLK_SIZE_TOTAL",  NO_NEXT_TABLE, GBSMsg_SimStatus, USER_TYPE_UINT32,  USER_RO, (void *) &gbsStatusTemp.cntDnLoadSizeExp,            -1,-1,  NO_LIMIT,NULL},
  {"BLK_RX",          NO_NEXT_TABLE, GBSMsg_SimStatus, USER_TYPE_UINT32,  USER_RO, (void *) &gbsStatusTemp.dataBlkState.cntBlkRx,       -1,-1,  NO_LIMIT,NULL}, 
  {"BLK_PROBLEM",     NO_NEXT_TABLE, GBSMsg_SimStatus, USER_TYPE_UINT32,  USER_RO, (void *) &gbsStatusTemp.dataBlkState.cntBlkBadRx,    -1,-1,  NO_LIMIT,NULL}, 
  {"BLK_BAD",         NO_NEXT_TABLE, GBSMsg_SimStatus, USER_TYPE_UINT32,  USER_RO, (void *) &gbsStatusTemp.dataBlkState.cntBlkBadTotal, -1,-1,  NO_LIMIT,NULL}, 
  {"BLK_BAD_STORE",   NO_NEXT_TABLE, GBSMsg_SimStatus, USER_TYPE_UINT32,  USER_RO, (void *) &gbsStatusTemp.dataBlkState.cntStoreBad,    -1,-1,  NO_LIMIT,NULL}, 
  {"DL_INTERRUPTED",  NO_NEXT_TABLE, GBSMsg_SimStatus, USER_TYPE_BOOLEAN, USER_RO, (void *) &gbsStatusTemp.bDownloadInterrupted,        -1,-1,  NO_LIMIT,NULL}, 
  {"DL_COMPLETED",    NO_NEXT_TABLE, GBSMsg_SimStatus, USER_TYPE_BOOLEAN, USER_RO, (void *) &gbsStatusTemp.bCompleted,                  -1,-1,  NO_LIMIT,NULL}, 
  {"DL_REQ_CNT",      NO_NEXT_TABLE, GBSMsg_SimStatus, USER_TYPE_UINT16,  USER_RO, (void *) &gbsStatusTemp.cntDnloadReq,                -1,-1,  NO_LIMIT,NULL}, 
  {"BLK_NVM",         NO_NEXT_TABLE, GBSMsg_SimStatus, USER_TYPE_UINT16,  USER_RO, (void *) &gbsStatusTemp.cntBlkSizeNVM,               -1,-1,  NO_LIMIT,NULL},
  {"RELAY_STUCK",     NO_NEXT_TABLE, GBSMsg_SimStatus, USER_TYPE_BOOLEAN, USER_RO, (void *) &gbsStatusTemp.bRelayStuck,                 -1,-1,  NO_LIMIT,NULL},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};
// GBS_SIM ***************************************************************


// GBS_DEBUG ***************************************************************
static USER_MSG_TBL gbsDebugTbl[] =
{ /*Str               Next Tbl Ptr   Handler Func.     Data Type          Access   Parameter                                         IndexRange DataLimit EnumTbl*/
  {"DNLOAD_CODE",     NO_NEXT_TABLE, GBSMsg_Status,    USER_TYPE_UINT8,   USER_RW, (void *) &gbsDebugDnloadTemp.dnloadCode,          1, (INT32) UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},

  {NULL,NULL,NULL,NO_HANDLER_DATA}
};
// GBS_DEBUG ***************************************************************


// GBS_CTL ***************************************************************
static USER_MSG_TBL gbsCtlCfgTbl[] =
{ /*Str                Next Tbl Ptr   Handler Func.   Data Type          Access   Parameter                             IndexRange DataLimit EnumTbl*/
  {"MULTIPLEX",        NO_NEXT_TABLE, GBSMsg_CtlCfg,  USER_TYPE_BOOLEAN, USER_RW, (void *) &gbsCtlTemp.bMultiplex,      -1,-1,  NO_LIMIT,NULL}, 
  {"MULTI_UART_PORT",  NO_NEXT_TABLE, GBSMsg_CtlCfg,  USER_TYPE_UINT8,   USER_RW, (void *) &gbsCtlTemp.nPort,           -1,-1,  1,3,     NULL}, 
  {"MULTI_RELAY_CHECK",NO_NEXT_TABLE, GBSMsg_CtlCfg,  USER_TYPE_BOOLEAN, USER_RW, (void *) &gbsCtlTemp.bChkMultiInstall,-1,-1,  NO_LIMIT,NULL},
  {"MULTI_ACTION",     NO_NEXT_TABLE, GBSMsg_CtlCfg,  USER_TYPE_ACT_LIST,USER_RW, (void *) &gbsCtlTemp.discAction,      -1,-1,  NO_LIMIT,NULL},   
  {"KEEPALIVE",        NO_NEXT_TABLE, GBSMsg_CtlCfg,  USER_TYPE_BOOLEAN, USER_RW, (void *) &gbsCtlTemp.bKeepAlive,      -1,-1,  NO_LIMIT,NULL}, 
  {"IDLE_TIMEOUT_MS",  NO_NEXT_TABLE, GBSMsg_CtlCfg,  USER_TYPE_UINT32,  USER_RW, (void *) &gbsCtlTemp.timeIdleOut,     -1,-1,  NO_LIMIT,NULL}, 
  //{"REC_RETRIES",    NO_NEXT_TABLE, GBSMsg_CtlCfg,  USER_TYPE_UINT32,  USER_RW, (void *) &gbsCtlTemp.retriesSingle,   -1,-1,  1,100,   NULL}, 
  {"REC_FAILS_RESTART",NO_NEXT_TABLE, GBSMsg_CtlCfg,  USER_TYPE_UINT32,  USER_RW, (void *) &gbsCtlTemp.retriesMulti,    -1,-1,  1,100,   NULL}, 
  //{"RESTARTS",       NO_NEXT_TABLE, GBSMsg_CtlCfg,  USER_TYPE_UINT32,  USER_RW, (void *) &gbsCtlTemp.restarts,        -1,-1,  1,100,   NULL}, 
  {"RESTARTS_DELAY_MS",NO_NEXT_TABLE, GBSMsg_CtlCfg,  USER_TYPE_UINT32,  USER_RW, (void *) &gbsCtlTemp.restart_delay_ms,-1,-1,  NO_LIMIT,NULL}, 
  {"CMD_DELAY_MS",     NO_NEXT_TABLE, GBSMsg_CtlCfg,  USER_TYPE_UINT32,  USER_RW, (void *) &gbsCtlTemp.cmd_delay_ms,    -1,-1,  NO_LIMIT,NULL},   
  {"KEEP_BAD_REC",     NO_NEXT_TABLE, GBSMsg_CtlCfg,  USER_TYPE_BOOLEAN, USER_RW, (void *) &gbsCtlTemp.bKeepBadRec,     -1,-1,  NO_LIMIT,NULL}, 
  {"BUFF_RECS",        NO_NEXT_TABLE, GBSMsg_CtlCfg,  USER_TYPE_BOOLEAN, USER_RW, (void *) &gbsCtlTemp.bBuffRecStore,   -1,-1,  NO_LIMIT,NULL}, 
  {"BUFF_RECS_SIZE",   NO_NEXT_TABLE, GBSMsg_CtlCfg,  USER_TYPE_UINT32,  USER_RW, (void *) &gbsCtlTemp.cntBuffRecSize,  -1,-1,  1,65536,NULL},   
  {"CONFIRM_RECS",     NO_NEXT_TABLE, GBSMsg_CtlCfg,  USER_TYPE_BOOLEAN, USER_RW, (void *) &gbsCtlTemp.bConfirm,        -1,-1,  NO_LIMIT,NULL},

  {"BLK_REQ_TIMEOUT_MS", NO_NEXT_TABLE, GBSMsg_CtlCfg,  USER_TYPE_UINT32,  USER_RW, (void *) &gbsCtlTemp.blk_req_timeout_ms,     -1,-1,  NO_LIMIT, NULL},
  {"BLK_REQ_RETRIES",    NO_NEXT_TABLE, GBSMsg_CtlCfg,  USER_TYPE_UINT16,  USER_RW, (void *) &gbsCtlTemp.blk_req_retries_cnt,    -1,-1,  NO_LIMIT, NULL},
  {"REC_TIMEOUT_MS",     NO_NEXT_TABLE, GBSMsg_CtlCfg,  USER_TYPE_UINT32,  USER_RW, (void *) &gbsCtlTemp.rec_retries_timeout_ms, -1,-1,  NO_LIMIT, NULL},
  {"REC_RETRIES",        NO_NEXT_TABLE, GBSMsg_CtlCfg,  USER_TYPE_UINT16,  USER_RW, (void *) &gbsCtlTemp.rec_retries_cnt,        -1,-1,  NO_LIMIT, NULL},
  
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL gbsCtlMultiTbl[] =
{ /*Str               Next Tbl Ptr   Handler Func.     Data Type          Access   Parameter                                         IndexRange DataLimit EnumTbl*/
  {"STATE",           NO_NEXT_TABLE, GBSMsg_CtlStatus, USER_TYPE_ENUM,    USER_RO, (void *) &gbsCtlStatusTemp.state,                 -1,-1,  NO_LIMIT,gbsMultiStateStrs},
  //{"PRIMARY_PEND",    NO_NEXT_TABLE, GBSMsg_CtlStatus, USER_TYPE_BOOLEAN, USER_RO, (void *) &gbsCtlStatusTemp.bPrimaryReqPending,    -1,-1,  NO_LIMIT,NULL},
  //{"SECONDARY_PEND",  NO_NEXT_TABLE, GBSMsg_CtlStatus, USER_TYPE_BOOLEAN, USER_RO, (void *) &gbsCtlStatusTemp.bSecondaryReqPending,  -1,-1,  NO_LIMIT,NULL},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL gbsCtlStatusTbl[] =
{ 
  {"MULTI", gbsCtlMultiTbl,  NULL,          NO_HANDLER_DATA},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL gbsCtlDebugTbl[] =
{ /*Str               Next Tbl Ptr   Handler Func.         Data Type          Access                  Parameter                        IndexRange DataLimit EnumTbl*/
  {"CLEAR_NVM",       NO_NEXT_TABLE, GBSMsg_CtlDebugClear, USER_TYPE_ACTION,  USER_RO,                NULL,                            -1,-1,  NO_LIMIT,NULL},
  {"VERBOSITY",       NO_NEXT_TABLE, GBSMsg_CtlDebug,      USER_TYPE_BOOLEAN, USER_RW,                &gbsDebugTemp.dbg_verbosity,     -1,-1,  NO_LIMIT,NULL},
  //{"DNLOAD_CODE",   NO_NEXT_TABLE, GBSMsg_CtlDebug,      USER_TYPE_UINT8,   USER_RW,                &gbsDebugTemp.dnloadCode,        -1,-1,  NO_LIMIT,NULL},
#ifdef DTU_GBS_SIM  
  {"DTU_GBS_SIM_LOG", NO_NEXT_TABLE, GBSMsg_CtlDebug,      USER_TYPE_BOOLEAN, USER_RW,                &gbsDebugTemp.DTU_GBS_SIM_SimLog,-1,-1,  NO_LIMIT,NULL},
#endif  
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};
// GBS_CTL ***************************************************************



static USER_MSG_TBL gbsSimProtocolRoot[] =
{
  {"STATUS",gbsSimStatusTbl,  NULL,          NO_HANDLER_DATA},
//{DISPLAY_CFG,               NO_NEXT_TABLE, EMU150Msg_ShowConfig,  USER_TYPE_ACTION,(USER_RO|USER_NO_LOG|USER_GSE) ,NULL     ,-1,-1,      NO_LIMIT, NULL},
  {NULL,                      NULL,          NULL,                  NO_HANDLER_DATA}
};

static USER_MSG_TBL gbsCtlProtocolRoot[] =
{
  {"CFG",    gbsCtlCfgTbl,    NULL,          NO_HANDLER_DATA},
  {"STATUS", gbsCtlStatusTbl, NULL,          NO_HANDLER_DATA},
  {"DEBUG",  gbsCtlDebugTbl,  NULL,          NO_HANDLER_DATA},
//{DISPLAY_CFG,               NO_NEXT_TABLE, EMU150Msg_ShowConfig,  USER_TYPE_ACTION,(USER_RO|USER_NO_LOG|USER_GSE) ,NULL     ,-1,-1,      NO_LIMIT, NULL},
  {NULL,                      NULL,          NULL,                  NO_HANDLER_DATA}
};

static USER_MSG_TBL gbsProtocolRoot[] =
{
  {"STATUS",gbsStatusTbl,  NULL,          NO_HANDLER_DATA},
  {"CFG",   gbsCfgTbl,     NULL,          NO_HANDLER_DATA},
  {"DEBUG", gbsDebugTbl,   NULL,          NO_HANDLER_DATA},
//{DISPLAY_CFG,            NO_NEXT_TABLE, EMU150Msg_ShowConfig,  USER_TYPE_ACTION,(USER_RO|USER_NO_LOG|USER_GSE) ,NULL     ,-1,-1,      NO_LIMIT, NULL},
  {NULL,                      NULL,          NULL,                  NO_HANDLER_DATA}
};




static USER_MSG_TBL gbsProtocolRootTblPtr = {"GBS",gbsProtocolRoot,NULL,NO_HANDLER_DATA};
static USER_MSG_TBL gbsCtlProtocolRootTblPtr = {"GBS_CTL",gbsCtlProtocolRoot,NULL,NO_HANDLER_DATA};
static USER_MSG_TBL gbsSimProtocolRootTblPtr = {"GBS_MULTI",gbsSimProtocolRoot,NULL,NO_HANDLER_DATA};


/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:    GBSMsg_Status
 *
 * Description: Called by the User.c module from the reference to this function
 *              in the user message tables above.
 *              Retrieves the latest GBS Status Data
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
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
static
USER_HANDLER_RESULT GBSMsg_Status(USER_DATA_TYPE DataType,
                                  USER_MSG_PARAM Param,
                                  UINT32 Index,
                                  const void *SetPtr,
                                  void **GetPtr)
{
  USER_HANDLER_RESULT result ;

  result = USER_RESULT_OK;

  gbsStatusTemp = *GBS_GetStatus( (UINT16) Index);
  gbsDebugDnloadTemp = *GBS_GetDebugDnload( (UINT16) Index); 

  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);
  
  *GBS_GetDebugDnload((UINT16) Index) = gbsDebugDnloadTemp; 

  return result;
}


/******************************************************************************
 * Function:    GBSMsg_SimStatus
 *
 * Description: Called by the User.c module from the reference to this function
 *              in the user message tables above.
 *              Retrieves the latest GBS Sim Status Data
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
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
static
USER_HANDLER_RESULT GBSMsg_SimStatus(USER_DATA_TYPE DataType,
                                     USER_MSG_PARAM Param,
                                     UINT32 Index,
                                     const void *SetPtr,
                                     void **GetPtr)
{
  USER_HANDLER_RESULT result ;

  result = USER_RESULT_OK;

  gbsStatusTemp = *GBS_GetSimStatus();

  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);

  return result;
}


/******************************************************************************
 * Function:    GBSMsg_CtlStatus
 *
 * Description: Called by the User.c module from the reference to this function
 *              in the user message tables above.
 *              Retrieves the latest GBS Ctl Status Data
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
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
static
USER_HANDLER_RESULT GBSMsg_CtlStatus(USER_DATA_TYPE DataType,
                                     USER_MSG_PARAM Param,
                                     UINT32 Index,
                                     const void *SetPtr,
                                     void **GetPtr)
{
  USER_HANDLER_RESULT result ;

  result = USER_RESULT_OK;

  gbsCtlStatusTemp = *GBS_GetCtlStatus();

  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);

  return result;
}

/******************************************************************************
 * Function:    GBSMsg_Debug
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves the latest GBS Status Data for Debug
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
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
/*
static
USER_HANDLER_RESULT GBSMsg_Debug(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr)
{
  USER_HANDLER_RESULT result ;

  result = USER_RESULT_OK;

  gbsStatusTemp = *GBSProtocol_GetStatus();

  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);

  return result;
}
*/


/******************************************************************************
 * Function:    GBSMsg_CmdCfg
 *
 * Description: Called by the User.c module from the reference to this function
 *              in the user message tables above.
 *              Retrieves the latest GBS Cfg Data
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
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
static
USER_HANDLER_RESULT GBSMsg_CmdCfg(USER_DATA_TYPE DataType,
                                  USER_MSG_PARAM Param,
                                  UINT32 Index,
                                  const void *SetPtr,
                                  void **GetPtr)
{
  USER_HANDLER_RESULT result ;


  result = USER_RESULT_OK;

  memcpy(&gbsCfgTemp, &CfgMgr_ConfigPtr()->GBSConfigs[Index], sizeof(GBS_CFG));
  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

  // If performing a "set" and it was successful, write the update.
  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    memcpy(&CfgMgr_ConfigPtr()->GBSConfigs[Index],
      &gbsCfgTemp,
      sizeof(GBS_CFG));

    CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
      &CfgMgr_ConfigPtr()->GBSConfigs[Index],
      sizeof(GBS_CFG));
  }

  return result;
}


/******************************************************************************
 * Function:    GBSMsg_CtlCfg
 *
 * Description: Called by the User.c module from the reference to this function
 *              in the user message tables above.
 *              Retrieves the latest GBS Ctl Cfg Data
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
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
static
USER_HANDLER_RESULT GBSMsg_CtlCfg(USER_DATA_TYPE DataType,
                                  USER_MSG_PARAM Param,
                                  UINT32 Index,
                                  const void *SetPtr,
                                  void **GetPtr)
{
  USER_HANDLER_RESULT result ;


  result = USER_RESULT_OK;

  memcpy(&gbsCtlTemp, &CfgMgr_ConfigPtr()->GBSCtlConfig, sizeof(GBS_CTL_CFG));
  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

  // If performing a "set" and it was successful, write the update.
  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    memcpy(&CfgMgr_ConfigPtr()->GBSCtlConfig,
      &gbsCtlTemp,
      sizeof(GBS_CTL_CFG));

    CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
      &CfgMgr_ConfigPtr()->GBSCtlConfig,
      sizeof(GBS_CTL_CFG));
  }

  return result;
}


/******************************************************************************
 * Function:    GBSMsg_CtlDebugClear
 *
 * Description: Called by the User.c module from the reference to this function
 *              in the user message tables above.
 *              Clears the NVM data for the GBS Protocol Module
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
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
static
USER_HANDLER_RESULT GBSMsg_CtlDebugClear(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr)
{
  USER_HANDLER_RESULT result ;
  
  result = USER_RESULT_OK;
  
  GBSProtocol_DownloadClrHndl ( TRUE, NULL ); 

  return result;
}

/******************************************************************************
 * Function:    GBSMsg_CtlDebug
 *
 * Description: Called by the User.c module from the reference to this function
 *              in the user message tables above.
 *              Retrieves the latest GBS Ctl Debug Data
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
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
static
USER_HANDLER_RESULT GBSMsg_CtlDebug(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr)
{
  USER_HANDLER_RESULT result ;

  result = USER_RESULT_OK;

  gbsDebugTemp = m_GBS_Debug;

  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);
  
  m_GBS_Debug=gbsDebugTemp; 

  return result;
}


/*****************************************************************************
 *  MODIFICATIONS
 *    $History: GBSUserTables.c $
 * 
 * *****************  Version 13  *****************
 * User: John Omalley Date: 4/14/16    Time: 4:00p
 * Updated in $/software/control processor/code/system
 * Code Review Update
 * 
 * *****************  Version 12  *****************
 * User: Peter Lee    Date: 4/11/16    Time: 1:59p
 * Updated in $/software/control processor/code/system
 * Minor Code Review Updates
 * 
 * *****************  Version 11  *****************
 * User: Peter Lee    Date: 3/26/16    Time: 11:40p
 * Updated in $/software/control processor/code/system
 * SCR #1324.  Add additional GBS Protocol Configuration Items.  Fix minor
 * bug with ">" vs ">=". 
 * 
 * *****************  Version 10  *****************
 * User: Peter Lee    Date: 15-03-24   Time: 5:31p
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol.  Remove extra comment delimiters in footer. 
 * 
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 15-03-24   Time: 5:04p
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol
 * 
 * 14) V&V findings and GSE/LOG Review AI
 * a) Remove GSE-4067, 4068
 * gbs_ctl.status.multi.primary_pend/secondary_pend from AI #8 of GSE
 * review. 
 * b) Update"cnt" to "size" for LOG-2831
 * c) Add vcast_dont_instrument_start
 * d) Fix 'BAD' record in row before Restart
 * 
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 15-03-17   Time: 1:24p
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol
 * GSE Review AI
 * a) Add min/max limit for "multi_uart_port" and "buff_recs_size" 
 * b) Def for Cmd_To_Cmd_Delay to 0 ms from 100 ms
 * 
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 15-03-09   Time: 9:30p
 * Updated in $/software/control processor/code/system
 * Code Review Updates
 * 
 * *****************  Version 6  *****************
 * User: John Omalley Date: 3/09/15    Time: 2:40p
 * Updated in $/software/control processor/code/system
 * SCR 1255 - Updated the GBS MULTI enum values
 * 
 * *****************  Version 5  *****************
 * User: Peter Lee    Date: 15-03-02   Time: 6:39p
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol
 * 
 * 9) Updates 
 * a) comment out #define DTU_GBS_SIM 1
 * c) vcast_dont_instrument for converage
 * d) GBSMsg_CtlDebugClear() calls GBSProtocol_DownloadClrHndl()
 * 
 * *****************  Version 4  *****************
 * User: Peter Lee    Date: 15-02-06   Time: 7:18p
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol.  Additional Update Item #7. 
 * 
 * *****************  Version 3  *****************
 * User: Peter Lee    Date: 15-02-03   Time: 5:30p
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol, Add "," to line 219.  For some reason compiled
 * w/o this ",".  Need to add, though. 
 *  
 * *****************  Version 2  *****************
 * User: Peter Lee    Date: 15-01-19   Time: 6:19p
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol Updates 
 * 1) LSS output control
 * 2) KeepAlive Msg
 * 3) Dnload code loop thru
 * 4) Debug Dnload Code
 * 5) Buffer Multi Records before storing 
 * 
 * *****************  Version 1  *****************
 * User: Peter Lee    Date: 15-01-11   Time: 10:21p
 * Created in $/software/control processor/code/system
 * SCR #1255 GBS Protocol 
 *
 *****************************************************************************/
