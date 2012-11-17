#define EMU150_USERTABLES_BODY
/******************************************************************************
            Copyright (C) 2007-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:        EMU150UserTables.c
    
    Description: Routines to support the user commands for EMU150 Protocol CSC 

    VERSION
    $Revision: 11 $  $Date: 12-11-16 8:12p $
    
******************************************************************************/
#ifndef EMU150_PROTOCOL_BODY
#error EMU150UserTables.c should only be included by EMU150Protocol.c
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
static EMU150_STATUS emu150StatusTemp; 
static EMU150_CFG emu150CfgTemp;

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
//Prototype for the User Manager message handlers, has to go before
//the local variable tables that use the function pointer.
static USER_HANDLER_RESULT EMU150Msg_Status     ( USER_DATA_TYPE DataType,
                                                  USER_MSG_PARAM Param,
                                                  UINT32 Index,
                                                  const void *SetPtr,
                                                  void **GetPtr );

static USER_HANDLER_RESULT EMU150Msg_Cfg        ( USER_DATA_TYPE DataType,
                                                  USER_MSG_PARAM Param,
                                                  UINT32 Index,
                                                  const void *SetPtr,
                                                  void **GetPtr );
                                  
static USER_HANDLER_RESULT EMU150Msg_Debug      ( USER_DATA_TYPE DataType,
                                                  USER_MSG_PARAM Param,
                                                  UINT32 Index,
                                                  const void *SetPtr,
                                                  void **GetPtr );

static USER_HANDLER_RESULT EMU150Msg_ShowConfig ( USER_DATA_TYPE DataType,
                                                  USER_MSG_PARAM Param,
                                                  UINT32 Index,
                                                  const void *SetPtr,
                                                  void **GetPtr );
/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static USER_MSG_TBL emu150StatusTbl[] = 
{ /*Str                   Next Tbl Ptr   Handler Func.     Data Type          Access   Parameter                                     IndexRange DataLimit EnumTbl*/
  {"STATE",               NO_NEXT_TABLE, EMU150Msg_Status, USER_TYPE_UINT32,  USER_RO, (void *)&emu150StatusTemp.state,                -1, -1,  NO_LIMIT, NULL},
  {"CMD_TIMER_MS",        NO_NEXT_TABLE, EMU150Msg_Status, USER_TYPE_UINT32,  USER_RO, (void *)&emu150StatusTemp.cmd_timer,            -1, -1,  NO_LIMIT, NULL},
  {"CMD_TIMEOUT_MS",      NO_NEXT_TABLE, EMU150Msg_Status, USER_TYPE_UINT32,  USER_RO, (void *)&emu150StatusTemp.cmd_timeout,          -1, -1,  NO_LIMIT, NULL},
  {"BLK_NUMBER",          NO_NEXT_TABLE, EMU150Msg_Status, USER_TYPE_UINT32,  USER_RO, (void *)&emu150StatusTemp.blk_number,           -1, -1,  NO_LIMIT, NULL},
  {"BLK_EXP_NUMBER",      NO_NEXT_TABLE, EMU150Msg_Status, USER_TYPE_UINT32,  USER_RO, (void *)&emu150StatusTemp.blk_exp_number,       -1, -1,  NO_LIMIT, NULL},
  {"RECORDS",             NO_NEXT_TABLE, EMU150Msg_Status, USER_TYPE_UINT32,  USER_RO, (void *)&emu150StatusTemp.nRecords,             -1, -1,  NO_LIMIT, NULL},
  {"RECORDS_NEW",         NO_NEXT_TABLE, EMU150Msg_Status, USER_TYPE_UINT32,  USER_RO, (void *)&emu150StatusTemp.nRecordsNew,          -1, -1,  NO_LIMIT, NULL},
  {"RECORDS_CURR",        NO_NEXT_TABLE, EMU150Msg_Status, USER_TYPE_UINT32,  USER_RO, (void *)&emu150StatusTemp.nCurrRecord,          -1, -1,  NO_LIMIT, NULL},
  {"RECORDS_BAD",         NO_NEXT_TABLE, EMU150Msg_Status, USER_TYPE_UINT32,  USER_RO, (void *)&emu150StatusTemp.nRecordsRxBad,        -1, -1,  NO_LIMIT, NULL},
  {"CKSUM_BAD",           NO_NEXT_TABLE, EMU150Msg_Status, USER_TYPE_UINT32,  USER_RO, (void *)&emu150StatusTemp.nCksumErr,            -1, -1,  NO_LIMIT, NULL},
  
  {"N_RETRIES",            NO_NEXT_TABLE, EMU150Msg_Status, USER_TYPE_UINT16,  USER_RO, (void *)&emu150StatusTemp.nRetries,             -1, -1,  NO_LIMIT, NULL},  
  {"N_IDLERETRIES",        NO_NEXT_TABLE, EMU150Msg_Status, USER_TYPE_UINT16,  USER_RO, (void *)&emu150StatusTemp.nIdleRetries,         -1, -1,  NO_LIMIT, NULL},  
  {"N_TOTALRETRIES",       NO_NEXT_TABLE, EMU150Msg_Status, USER_TYPE_UINT32,  USER_RO, (void *)&emu150StatusTemp.nTotalRetries,        -1, -1,  NO_LIMIT, NULL},  
                                                                                                                                       
  {"RECORDS_RECORDED",    NO_NEXT_TABLE, EMU150Msg_Status, USER_TYPE_UINT32,  USER_RO, (void *)&emu150StatusTemp.nRecordsRec,          -1, -1,  NO_LIMIT, NULL},
  {"RECORDS_REC_BYTES",   NO_NEXT_TABLE, EMU150Msg_Status, USER_TYPE_UINT32,  USER_RO, (void *)&emu150StatusTemp.nRecordsRecTotalByte, -1, -1,  NO_LIMIT, NULL},
                                                                              
  {"TOTAL_BYTES",         NO_NEXT_TABLE, EMU150Msg_Status, USER_TYPE_UINT32,  USER_RO, (void *)&emu150StatusTemp.nTotalBytes,          -1, -1,  NO_LIMIT, NULL},  
  {"UartBPS",             NO_NEXT_TABLE, EMU150Msg_Status, USER_TYPE_UINT32,  USER_RO, (void *)&emu150StatusTemp.uartBPS,              -1, -1,  NO_LIMIT, NULL},
  {"UartBPS_Desired",     NO_NEXT_TABLE, EMU150Msg_Status, USER_TYPE_UINT32,  USER_RO, (void *)&emu150StatusTemp.uartBPS_Desired,      -1, -1,  NO_LIMIT, NULL},

  {"DownloadInterrupted", NO_NEXT_TABLE, EMU150Msg_Status, USER_TYPE_BOOLEAN, USER_RO, (void *)&emu150StatusTemp.bDownloadInterrupted, -1, -1,  NO_LIMIT, NULL},
  {"DownloadFailed",      NO_NEXT_TABLE, EMU150Msg_Status, USER_TYPE_BOOLEAN, USER_RO, (void *)&emu150StatusTemp.bDownloadFailed,      -1, -1,  NO_LIMIT, NULL},
  {"Restarts",            NO_NEXT_TABLE, EMU150Msg_Status, USER_TYPE_UINT16,  USER_RO, (void *)&emu150StatusTemp.nRestarts,            -1, -1,  NO_LIMIT, NULL},

  {NULL,NULL,NULL,NO_HANDLER_DATA}
};


static USER_MSG_TBL emu150CfgTbl[] = 
{ /*Str                      Next Tbl Ptr   Handler Func.     Data Type       Access   Parameter                                 IndexRange DataLimit    EnumTbl*/
  {"TIMEOUT_MS",             NO_NEXT_TABLE, EMU150Msg_Cfg, USER_TYPE_UINT32,  USER_RW, (void *)&emu150CfgTemp.timeout,             -1, -1,  500,   2000,  NULL},
  {"TIMEOUT_REC_HDR_MS",     NO_NEXT_TABLE, EMU150Msg_Cfg, USER_TYPE_UINT32,  USER_RW, (void *)&emu150CfgTemp.timeout_rec_hdr,     -1, -1,  10100, 20000, NULL},
  {"TIMEOUT_EOM_MS",         NO_NEXT_TABLE, EMU150Msg_Cfg, USER_TYPE_UINT32,  USER_RW, (void *)&emu150CfgTemp.timeout_eom,         -1, -1,  0,     1000,  NULL},
  {"TIMEOUT_AUTO_DETECT_MS", NO_NEXT_TABLE, EMU150Msg_Cfg, USER_TYPE_UINT32,  USER_RW, (void *)&emu150CfgTemp.timeout_auto_detect, -1, -1,  500,   2000,  NULL},
  {"TIMEOUT_ACK_MS",         NO_NEXT_TABLE, EMU150Msg_Cfg, USER_TYPE_UINT32,  USER_RW, (void *)&emu150CfgTemp.timeout_ack,         -1, -1,  2100,  10000, NULL},
  {"RETRIES",                NO_NEXT_TABLE, EMU150Msg_Cfg, USER_TYPE_UINT16,  USER_RW, (void *)&emu150CfgTemp.nRetries,            -1, -1,  1,     9,     NULL},
  {"RESTARTS",               NO_NEXT_TABLE, EMU150Msg_Cfg, USER_TYPE_UINT16,  USER_RW, (void *)&emu150CfgTemp.nRestarts,           -1, -1,  1,     10,    NULL},
  {"RETRIES_IDLE",           NO_NEXT_TABLE, EMU150Msg_Cfg, USER_TYPE_UINT16,  USER_RW, (void *)&emu150CfgTemp.nIdleRetries,        -1, -1, 10,     50,    NULL},
  {"MARK_DOWNLOADED",        NO_NEXT_TABLE, EMU150Msg_Cfg, USER_TYPE_BOOLEAN, USER_RW, (void *)&emu150CfgTemp.bMarkDownloaded,     -1, -1,  NO_LIMIT,     NULL},

  {NULL,NULL,NULL,NO_HANDLER_DATA}
};


static USER_MSG_TBL emu150DebugTbl[] = 
{
  {"ALL_RECORDS",    NO_NEXT_TABLE, EMU150Msg_Debug, USER_TYPE_BOOLEAN, USER_RW, (void *)&emu150StatusTemp.bAllRecords,     -1, -1, NO_LIMIT, NULL},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};


static USER_MSG_TBL emu150ProtocolRoot[] = 
{/* Str                Next Tbl Ptr          Handler Func.          Data Type        Access                         Parameter IndexRange  DataLimit  EnumTbl*/
  {"STATUS",emu150StatusTbl,  NULL,          NO_HANDLER_DATA},
  {"CFG",emu150CfgTbl,        NULL,          NO_HANDLER_DATA},
  {"DEBUG",emu150DebugTbl,    NULL,          NO_HANDLER_DATA},
  {DISPLAY_CFG,               NO_NEXT_TABLE, EMU150Msg_ShowConfig,  USER_TYPE_ACTION,(USER_RO|USER_NO_LOG|USER_GSE) ,NULL     ,-1,-1,      NO_LIMIT, NULL},
  {NULL,                      NULL,          NULL,                  NO_HANDLER_DATA}
};

static USER_MSG_TBL emu150ProtocolRootTblPtr = {"EMU150",emu150ProtocolRoot,NULL,NO_HANDLER_DATA};

/*****************************************************************************/
/* Public Functions                                                          */ 
/*****************************************************************************/

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:    EMU150Msg_Status
 *
 * Description: Called by the User.c module from the reference to this function
 *              in the user message tables above.
 *              Retrieves the latest EMU150 Status Data 
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
USER_HANDLER_RESULT EMU150Msg_Status(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr)
{
  USER_HANDLER_RESULT result ; 

  result = USER_RESULT_OK; 
  
  emu150StatusTemp = *EMU150Protocol_GetStatus(); 
  
  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);
  
  return result;
}


/******************************************************************************
 * Function:    EMU150Msg_Debug
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves the latest EMU150 Status Data for Debug
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
USER_HANDLER_RESULT EMU150Msg_Debug(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr)
{
  USER_HANDLER_RESULT result ; 

  result = USER_RESULT_OK; 
  
  emu150StatusTemp = *EMU150Protocol_GetStatus(); 
  
  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);
  
  // Only Update Writeable params
  EMU150Protocol_GetStatus()->bAllRecords = emu150StatusTemp.bAllRecords; 
  
  return result;
}


/******************************************************************************
 * Function:    EMU150Msg_Cfg
 *
 * Description: Called by the User.c module from the reference to this function
 *              in the user message tables above.
 *              Retrieves the latest EMU150 Cfg Data 
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
USER_HANDLER_RESULT EMU150Msg_Cfg(USER_DATA_TYPE DataType,
                                  USER_MSG_PARAM Param,
                                  UINT32 Index,
                                  const void *SetPtr,
                                  void **GetPtr)
{
  USER_HANDLER_RESULT result ; 


  result = USER_RESULT_OK; 

  memcpy(&emu150CfgTemp, &CfgMgr_ConfigPtr()->EMU150Config, sizeof(EMU150_CFG));
  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
  
  // If performing a "set" and it was successful, write the update.
  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    memcpy(&CfgMgr_ConfigPtr()->EMU150Config,
      &emu150CfgTemp,
      sizeof(EMU150_CFG));

    CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
      &CfgMgr_ConfigPtr()->EMU150Config,
      sizeof(EMU150_CFG));
  }

  return result;
}

/******************************************************************************
* Function:    EMU150Msg_ShowConfig
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
static
USER_HANDLER_RESULT EMU150Msg_ShowConfig(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr)
{
  CHAR  labelStem[] = "\r\n\r\nEMU150.CFG";

  USER_HANDLER_RESULT result = USER_RESULT_OK;
  USER_MSG_TBL*  pCfgTable;

  //Top-level name is a single indented space
  CHAR branchName[USER_MAX_MSG_STR_LEN] = " ";   

  pCfgTable = &emu150CfgTbl[0];  // Get pointer to config entry

  result = USER_RESULT_ERROR;
  if (User_OutputMsgString( labelStem, FALSE ) )
  {
    result = User_DisplayConfigTree(branchName, pCfgTable, 0, 0, NULL);
  }   
  return result;
}


/*****************************************************************************
 *  MODIFICATIONS
 *    $History: EMU150UserTables.c $
 * 
 * *****************  Version 11  *****************
 * User: John Omalley Date: 12-11-16   Time: 8:12p
 * Updated in $/software/control processor/code/system
 * SCR 1087 - Code Review Updates
 * 
 * *****************  Version 10  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 9/20/11    Time: 6:27p
 * Updated in $/software/control processor/code/system
 * SCR #1059 GSE Status nRetries and Status nIdleRetries s/b UINT16 not
 * UINT32. 
 * 
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 9/15/11    Time: 7:59p
 * Updated in $/software/control processor/code/system
 * SCR #1059 EMU150 V&V Issues
 * 
 * *****************  Version 7  *****************
 * User: John Omalley Date: 9/15/11    Time: 10:08a
 * Updated in $/software/control processor/code/system
 * SCR 1059 - Remove Debug commands and variables
 * 
 * *****************  Version 6  *****************
 * User: John Omalley Date: 9/12/11    Time: 5:01p
 * Updated in $/software/control processor/code/system
 * SCR 1059 - #ifdef Debug commands
 * 
 * *****************  Version 5  *****************
 * User: Jeff Vahue   Date: 9/07/11    Time: 2:52p
 * Updated in $/software/control processor/code/system
 * SCR# 1059 - Updated the timeout(s) minimum and default
 * 
 * *****************  Version 4  *****************
 * User: John Omalley Date: 9/07/11    Time: 1:50p
 * Updated in $/software/control processor/code/system
 * SCR 1059 - Updated the timeout(s) minimum, maximum and default
 * 
 * *****************  Version 3  *****************
 * User: Peter Lee    Date: 7/25/11    Time: 4:39p
 * Updated in $/software/control processor/code/system
 * SCR #1039 Add option to "Not Mark As Downloaded" and several non-logic
 * updates (names, variable, etc).  
 * 
 * *****************  Version 2  *****************
 * User: Contractor V&v Date: 5/31/11    Time: 1:51p
 * Updated in $/software/control processor/code/system
 * SCR #1038 Misc - Update EMU150 User Table to include "show cfg"
 * 
 * *****************  Version 1  *****************
 * User: Peter Lee    Date: 4/11/11    Time: 10:07a
 * Created in $/software/control processor/code/system
 * SCR #1029 EMU150 Download.  Initial check in. 
 * 
 *
 *****************************************************************************/
