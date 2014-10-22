#define FASTMGR_USERTABLES_BODY
/******************************************************************************
            Copyright (C) 2007-2014 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:        FastMgrUserTables.c

    Description: Tables and functions for FastMgr User Commands

   VERSION
   $Revision: 35 $  $Date: 10/22/14 6:26p $

******************************************************************************/
#ifndef FASTMGR_BODY
#error FastMgrUserTables.c should only be included by FastMgr.c
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
#define INSTALL_STR_LIMIT   0,TEXT_BUF_SIZE

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
USER_ENUM_TBL time_source_strs[] =
{
  {"LOCAL", TIME_SOURCE_LOCAL},
  {"MS",    TIME_SOURCE_MS},
  {NULL,0}
};

static USER_ENUM_TBL fast_txtest_enum[] =
{
  {"",      FAST_TXTEST_INIT},
  {"PASS",  FAST_TXTEST_PASS},
  {"FAIL",  FAST_TXTEST_FAIL},
  {m_FastTxTest.inProgressStr,  FAST_TXTEST_INPROGRESS},
  {m_FastTxTest.gsmPassStr,     FAST_TXTEST_GSMPASS},
  {"MoveLogsToMS",              FAST_TXTEST_MOVELOGSTOMS},
  {m_FastTxTest.moveLogsStr,    FAST_TXTEST_MOVELOGSTOGROUND},
  {NULL,0}
};

static USER_ENUM_TBL fast_txtest_status_enum[] =
{
  {"Stopped",   FAST_TXTEST_STATE_STOPPED},
  {"InProgress",FAST_TXTEST_STATE_SYSCON},
  {"InProgress",FAST_TXTEST_STATE_WOWDISC},
  {"InProgress",FAST_TXTEST_STATE_ONGND},
  {"InProgress",FAST_TXTEST_STATE_REC},
  {"InProgress",FAST_TXTEST_STATE_MSRDY},
  {"InProgress",FAST_TXTEST_STATE_SIMRDY},
  {"InProgress",FAST_TXTEST_STATE_GSM},
  {"InProgress",FAST_TXTEST_STATE_VPN},
  {"InProgress",FAST_TXTEST_STATE_UL},
  {"Pass",      FAST_TXTEST_STATE_PASS},
  {m_FastTxTest.failReason,FAST_TXTEST_STATE_FAIL},
  {NULL,0}
};


/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
// See Local Variables below Function Prototypes Section

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
//Prototypes for the User Manager message handlers, has to go before
//the local variable tables that use the function pointer.
static USER_HANDLER_RESULT FAST_UserCfg(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr);
static USER_HANDLER_RESULT FAST_ResetNVConfig(USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr);
static USER_HANDLER_RESULT FAST_VersionCmd(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr);
static USER_HANDLER_RESULT FAST_ShowConfig(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr);
static USER_HANDLER_RESULT FAST_InstallIdCmd(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr);
static USER_HANDLER_RESULT FAST_StartTxTest(USER_DATA_TYPE DataType,
                                      USER_MSG_PARAM Param,
                                      UINT32 Index,
                                      const void *SetPtr,
                                      void **GetPtr);

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static FASTMGR_CONFIG fast_cfg_temp;

static USER_MSG_TBL cfg_flags_cmd [] =
{  /*Str               Next Tbl Ptr   Handler Func.       Data Type           Access      Parameter                          IndexRange   DataLimit          EnumTbl*/
  { "RECORD",          NO_NEXT_TABLE, FAST_UserCfg,       USER_TYPE_128_LIST, USER_RW,    fast_cfg_temp.record_triggers,    -1,-1,       0,(MAX_TRIGGERS-1), NULL },
  { "ON_GROUND",       NO_NEXT_TABLE, FAST_UserCfg,       USER_TYPE_128_LIST, USER_RW,    fast_cfg_temp.on_ground_triggers, -1,-1,       0,(MAX_TRIGGERS-1), NULL },
  { "AUTO_UL_S",       NO_NEXT_TABLE, FAST_UserCfg,       USER_TYPE_UINT32,   USER_RW,    &fast_cfg_temp.auto_ul_per_s,     -1,-1,       NO_LIMIT,           NULL },
  { "TIME_SOURCE",     NO_NEXT_TABLE, FAST_UserCfg,       USER_TYPE_ENUM,     USER_RW,    &fast_cfg_temp.time_source,       -1,-1,       NO_LIMIT,           time_source_strs },
  { "VER",             NO_NEXT_TABLE, FAST_VersionCmd,    USER_TYPE_UINT16,   USER_RW,    NULL,                             -1,-1,       NO_LIMIT,           NULL },
  { "INSTALL_ID",      NO_NEXT_TABLE, FAST_InstallIdCmd,  USER_TYPE_STR,      USER_RW,    NULL,                             -1,-1,       INSTALL_STR_LIMIT,  NULL },
  { "TXTST_MSRDY_TO_S",NO_NEXT_TABLE, FAST_UserCfg,       USER_TYPE_UINT32,   USER_RW,    &fast_cfg_temp.tx_test_msrdy_to,  -1,-1,       NO_LIMIT,           NULL },
  { "TXTST_SIM_TO_S",  NO_NEXT_TABLE, FAST_UserCfg,       USER_TYPE_UINT32,   USER_RW,    &fast_cfg_temp.tx_test_SIM_rdy_to,-1,-1,       NO_LIMIT,           NULL },
  { "TXTST_GSM_TO_S",  NO_NEXT_TABLE, FAST_UserCfg,       USER_TYPE_UINT32,   USER_RW,    &fast_cfg_temp.tx_test_GSM_rdy_to,-1,-1,       NO_LIMIT,           NULL },
  { "TXTST_VPN_TO_S",  NO_NEXT_TABLE, FAST_UserCfg,       USER_TYPE_UINT32,   USER_RW,    &fast_cfg_temp.tx_test_VPN_rdy_to,-1,-1,       NO_LIMIT,           NULL },
  { NULL,              NULL,          NULL,               NO_HANDLER_DATA }
};


static USER_MSG_TBL status_flags_cmd [] =
{  /*Str           Next Tbl Ptr     Handler Func.         Data Type          Access      Parameter                   IndexRange   DataLimit   EnumTbl*/
  { "SW_VER",      NO_NEXT_TABLE,   User_GenericAccessor, USER_TYPE_STR,     USER_RO,    swVersion,                  -1,-1,       NO_LIMIT,   NULL },
  { "RECORDING",   NO_NEXT_TABLE,   User_GenericAccessor, USER_TYPE_YESNO,   USER_RO,    &fastStatus.recording,      -1,-1,       NO_LIMIT,   NULL },
  { "ON_GROUND",   NO_NEXT_TABLE,   User_GenericAccessor, USER_TYPE_YESNO,   USER_RO,    &fastStatus.onGround,       -1,-1,       NO_LIMIT,   NULL },
  { "RF_GSM",      NO_NEXT_TABLE,   User_GenericAccessor, USER_TYPE_ONOFF,   USER_RO,    &fastStatus.rfGsmEnable,    -1,-1,       NO_LIMIT,   NULL },
  { "WLAN_POWER",  NO_NEXT_TABLE,   User_GenericAccessor, USER_TYPE_ONOFF,   USER_RO,    &fastStatus.wlanPowerEnable,-1,-1,       NO_LIMIT,   NULL },
  { "BUSY_FLAGS",  NO_NEXT_TABLE,   User_GenericAccessor, USER_TYPE_HEX32,   USER_RO,    &m_RecordBusyFlags,         -1,-1,       NO_LIMIT,   NULL },
  { NULL,          NULL,            NULL,                 NO_HANDLER_DATA}
};

static USER_MSG_TBL tx_test_table [] =
{  /*Str               Next Tbl Ptr     Handler Func.         Data Type          Access      Parameter                IndexRange    DataLimit  EnumTbl*/
  { "START",           NO_NEXT_TABLE,   FAST_StartTxTest,     USER_TYPE_ACTION,  USER_RO,    NULL,                    -1,-1,        NO_LIMIT,  NULL },
  { "STATUS",          NO_NEXT_TABLE,   User_GenericAccessor, USER_TYPE_ENUM,    USER_RO,    &m_FastTxTest.state,     -1,-1,        NO_LIMIT,  fast_txtest_status_enum },
  { "SYS_CONDITION",   NO_NEXT_TABLE,   User_GenericAccessor, USER_TYPE_ENUM,    USER_RO,    &m_FastTxTest.sysCon,    -1,-1,        NO_LIMIT,  fast_txtest_enum},
  { "WOW_DISCRETE",    NO_NEXT_TABLE,   User_GenericAccessor, USER_TYPE_ENUM,    USER_RO,    &m_FastTxTest.wowDisc,   -1,-1,        NO_LIMIT,  fast_txtest_enum},
  { "ON_GROUND_TRIG",  NO_NEXT_TABLE,   User_GenericAccessor, USER_TYPE_ENUM,    USER_RO,    &m_FastTxTest.onGround,  -1,-1,        NO_LIMIT,  fast_txtest_enum},
  { "RECORD_DATA_TRIG",NO_NEXT_TABLE,   User_GenericAccessor, USER_TYPE_ENUM,    USER_RO,    &m_FastTxTest.record,    -1,-1,        NO_LIMIT,  fast_txtest_enum},
  { "MS_READY",        NO_NEXT_TABLE,   User_GenericAccessor, USER_TYPE_ENUM,    USER_RO,    &m_FastTxTest.msReady,   -1,-1,        NO_LIMIT,  fast_txtest_enum},
  { "GSM_SIM_CARD",    NO_NEXT_TABLE,   User_GenericAccessor, USER_TYPE_ENUM,    USER_RO,    &m_FastTxTest.simReady,  -1,-1,        NO_LIMIT,  fast_txtest_enum},
  { "GSM_SIGNAL_DB",   NO_NEXT_TABLE,   User_GenericAccessor, USER_TYPE_ENUM,    USER_RO,    &m_FastTxTest.gsmSignal, -1,-1,        NO_LIMIT,  fast_txtest_enum},
  { "VPN_CONNECTION",  NO_NEXT_TABLE,   User_GenericAccessor, USER_TYPE_ENUM,    USER_RO,    &m_FastTxTest.vpnStatus, -1,-1,        NO_LIMIT,  fast_txtest_enum},
  { "UPLOAD_STATUS",   NO_NEXT_TABLE,   User_GenericAccessor, USER_TYPE_ENUM,    USER_RO,    &m_FastTxTest.ulStatus,  -1,-1,        NO_LIMIT,  fast_txtest_enum},
   { NULL,          NULL,            NULL,                 NO_HANDLER_DATA}
};

static USER_MSG_TBL fast_cmd [] =
{  /*Str           Next Tbl Ptr     Handler Func.         Data Type          Access                        Parameter        IndexRange    DataLimit  EnumTbl*/
  { "CFG",       cfg_flags_cmd,       NULL,                 NO_HANDLER_DATA},
  { "STATUS",    status_flags_cmd,    NULL,                 NO_HANDLER_DATA},
  { "WLAN_OVR",  NO_NEXT_TABLE,     User_GenericAccessor, USER_TYPE_ONOFF, (USER_RW|USER_GSE),             &wlanOverride,   -1,-1,        NO_LIMIT,  NULL },
  { "GSM_OVR",   NO_NEXT_TABLE,     User_GenericAccessor, USER_TYPE_ONOFF, (USER_RW|USER_GSE),             &gsmOverride,    -1,-1,        NO_LIMIT,  NULL },
  { "RESET",     NO_NEXT_TABLE,     FAST_ResetNVConfig,   USER_TYPE_STR,   (USER_WO)         ,             NULL,            -1,-1,        NO_LIMIT,  NULL },
  { "TXTEST",    tx_test_table,       NULL,                 NO_HANDLER_DATA},
  { DISPLAY_CFG, NO_NEXT_TABLE,     FAST_ShowConfig,      USER_TYPE_ACTION,(USER_RO|USER_NO_LOG|USER_GSE), NULL,            -1,-1,        NO_LIMIT,  NULL},
  { NULL,        NULL,              NULL,                 NO_HANDLER_DATA}
};

static USER_MSG_TBL root_msg = {"FAST", fast_cmd, NULL, NO_HANDLER_DATA};

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:    FAST_UserCfg
 *
 * Description: Handles commands from the User Manager to configure NV settings
 *              for the FAST manager.
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
static USER_HANDLER_RESULT FAST_UserCfg(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_OK;

  //Load trigger structure into the temporary location based on index param
  //Param.Ptr points to the struct member to be read/written
  //in the temporary location
  memcpy(&fast_cfg_temp,
         &CfgMgr_ConfigPtr()->FASTCfg,
         sizeof(fast_cfg_temp));

  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    memcpy(&CfgMgr_ConfigPtr()->FASTCfg,
      &fast_cfg_temp,
      sizeof(fast_cfg_temp));

    //Store the modified temporary structure in the EEPROM.
    CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
      &CfgMgr_ConfigPtr()->FASTCfg,
      sizeof(fast_cfg_temp));
  }

  return result;
}


/******************************************************************************
 * Function:    FAST_ResetNVConfig
 *
 * Description: Command CfgManager to reset Configuration to default state
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
static USER_HANDLER_RESULT FAST_ResetNVConfig(USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_ERROR;

  if(strncmp(SetPtr,"really",sizeof("really")) == 0)
  {
    result = USER_RESULT_OK;
    // Don't allow the reset if an erase is in progress, or
    // log files have not been uploaded
    if ( LogIsEraseInProgress() )
    {
      User_OutputMsgString( "Unable to reset: erase in progress", FALSE );
    }
    else if( !LogIsLogEmpty() )
    {
      User_OutputMsgString( "Unable to reset: log files have not been uploaded", FALSE );
    }
    else
    {
      CfgMgr_ResetConfig(RESET_ALL, REASON_USER_COMMAND);
    }
  }

  return result;
}



/******************************************************************************
 * Function:    FAST_VersionCmd
 *
 * Description: Handles User Manager request to access the Version ID setting.
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
static USER_HANDLER_RESULT FAST_VersionCmd(USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr)
{
  UINT16 ver;
  //Load Version Id into the temporary location
  //Param.Ptr points to the struct member to be read/written
  //in the temporary location
  memcpy(&ver,
         &CfgMgr_ConfigPtr()->VerId,
         sizeof(ver));

  //If "get" command, point the GetPtr to the element defined by Param
  if( SetPtr == NULL )
  {
    **(UINT16**)GetPtr = ver;
  }
  else
  {
    ver = *(UINT16*)SetPtr;
  }

  memcpy(&CfgMgr_ConfigPtr()->VerId,
    &ver,
    sizeof(CfgMgr_ConfigPtr()->VerId));

  //Store the modified temporary structure in the EEPROM.
  CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
    &CfgMgr_ConfigPtr()->VerId,
    sizeof(CfgMgr_ConfigPtr()->VerId));

  return USER_RESULT_OK;
}

/******************************************************************************
* Function:    FAST_ShowConfig
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
static USER_HANDLER_RESULT FAST_ShowConfig(USER_DATA_TYPE DataType,
                                      USER_MSG_PARAM Param,
                                      UINT32 Index,
                                      const void *SetPtr,
                                      void **GetPtr)
{
  static const CHAR  label_stem[] = "\r\n\r\nFAST.CFG";

  USER_HANDLER_RESULT result = USER_RESULT_OK;
  USER_MSG_TBL*  pCfgTable;

  //Top-level name is a single indented space
  CHAR branch_name[USER_MAX_MSG_STR_LEN] = " ";

  pCfgTable = cfg_flags_cmd;  // Get pointer to config entry

  result = USER_RESULT_ERROR;
  if (User_OutputMsgString( label_stem, FALSE ) )
  {
    result = User_DisplayConfigTree(branch_name, pCfgTable, 0, 0, NULL);
  }
  return result;
}

/******************************************************************************
* Function: FAST_InstallIdCmd
*
* Description:  Handles User Manager request to access the Installation ID
*               setting.
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
static USER_HANDLER_RESULT FAST_InstallIdCmd(USER_DATA_TYPE DataType,
                                      USER_MSG_PARAM Param,
                                      UINT32 Index,
                                      const void *SetPtr,
                                      void **GetPtr)
{
  USER_HANDLER_RESULT user_result = USER_RESULT_ERROR;

  // If "get", set the get ptr to the address of the Installation id string
  // in the config manager structure.
  if( SetPtr == NULL )
  {
    *GetPtr = CfgMgr_ConfigPtr()->Id;
    user_result = USER_RESULT_OK;
  }
  else
  {
    // If "set", copy the string at SetPtr to the Installation id string in
    // config mgr structure.
    memcpy(CfgMgr_ConfigPtr()->Id,
           SetPtr,
           sizeof(CfgMgr_ConfigPtr()->Id) );

    //Store the modified temporary structure in the EEPROM.
    user_result = USER_RESULT_ERROR;
    if ( TRUE == CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
                                        CfgMgr_ConfigPtr()->Id,
                                        sizeof(CfgMgr_ConfigPtr()->Id)))
    {
        user_result = USER_RESULT_OK;
    }
  }
  return user_result;
}



/******************************************************************************
* Function:     FAST_StartTxTest | User Manager Handler
*
* Description:  Start tests necessary to enable GSM and transmit files
*               to the ground server.
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
static USER_HANDLER_RESULT FAST_StartTxTest(USER_DATA_TYPE DataType,
                                      USER_MSG_PARAM Param,
                                      UINT32 Index,
                                      const void *SetPtr,
                                      void **GetPtr)
{
  //Clear data structure
  memset(&m_FastTxTest, 0, sizeof(m_FastTxTest));
  m_FastTxTest.state = FAST_TXTEST_STATE_SYSCON;

  //Enable Task
  TmTaskEnable(FAST_TxTestID, TRUE);

  return USER_RESULT_OK;
}



/*************************************************************************
*  MODIFICATIONS
*    $History: FASTMgrUserTables.c $
 * 
 * *****************  Version 35  *****************
 * User: Contractor V&v Date: 10/22/14   Time: 6:26p
 * Updated in $/software/control processor/code/application
 * SCR #1271 - Initialization of ACTION list exceeds field size
 * 
 * *****************  Version 34  *****************
 * User: Contractor V&v Date: 9/03/14    Time: 5:10p
 * Updated in $/software/control processor/code/application
 * SCR #1030 - Time Sync to REMOTE option CodeReview change
 * 
 * *****************  Version 33  *****************
 * User: Contractor V&v Date: 7/28/14    Time: 6:50p
 * Updated in $/software/control processor/code/application
 * Removed support for TIME_SOURCE_REMOTE
 *
 * *****************  Version 32  *****************
 * User: John Omalley Date: 12-12-20   Time: 4:12p
 * Updated in $/software/control processor/code/application
 * SCR 1197 - Code Review Updates
 *
 * *****************  Version 31  *****************
 * User: Jim Mood     Date: 12/13/12   Time: 2:56p
 * Updated in $/software/control processor/code/application
 * SCR #1197
 *
 * *****************  Version 30  *****************
 * User: Melanie Jutras Date: 12-11-15   Time: 1:28p
 * Updated in $/software/control processor/code/application
 * SCR #1142 File Format Errors
 *
 * *****************  Version 29  *****************
 * User: Melanie Jutras Date: 12-11-15   Time: 12:37p
 * Updated in $/software/control processor/code/application
 * SCR #1142 File Format Errors
 *
 * *****************  Version 28  *****************
 * User: Jim Mood     Date: 11/13/12   Time: 4:18p
 * Updated in $/software/control processor/code/application
 * SCR #1131
 *
 * *****************  Version 27  *****************
 * User: Contractor V&v Date: 9/14/12    Time: 4:06p
 * Updated in $/software/control processor/code/application
 * FAST 2 fixes for bitarray list
 *
 * *****************  Version 26  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 12:43p
 * Updated in $/software/control processor/code/application
 * SCR# 1142
 *
 * *****************  Version 25  *****************
 * User: Contractor V&v Date: 8/15/12    Time: 7:18p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 BITARRAY128 input as integer list
 *
 * *****************  Version 24  *****************
 * User: Contractor V&v Date: 3/14/12    Time: 4:51p
 * Updated in $/software/control processor/code/application
 * SCR #1107 Trigger processing
 *
 * *****************  Version 23  *****************
 * User: Contractor2  Date: 5/10/11    Time: 1:19p
 * Updated in $/software/control processor/code/application
 * SCR #738 Enhancement: Verbosity and Action Command Responses
 *
 * *****************  Version 22  *****************
 * User: Contractor2  Date: 10/05/10   Time: 12:18p
 * Updated in $/software/control processor/code/application
 * SCR #916 Code Review Updates
 *
 * *****************  Version 21  *****************
 * User: Jim Mood     Date: 8/09/10    Time: 5:49p
 * Updated in $/software/control processor/code/application
 * SCR 776.  fast.cfg.txtest strings were wrong with respect to what
 * config items the were modifying
 *
 * *****************  Version 20  *****************
 * User: Jim Mood     Date: 7/30/10    Time: 2:17p
 * Updated in $/software/control processor/code/application
 * Fixed spelling error in user table for fast.txtest.wow_discreet -> s/b
 * -> fast.txtest.wow_discrete
 *
 * *****************  Version 19  *****************
 * User: Jim Mood     Date: 7/30/10    Time: 8:56a
 * Updated in $/software/control processor/code/application
 * SCR 327 Fast transmssion test functionality
 *
 * *****************  Version 18  *****************
 * User: Contractor V&v Date: 7/21/10    Time: 7:12p
 * Updated in $/software/control processor/code/application
 * SCR #260 Implement BOX status command
 *
 * *****************  Version 17  *****************
 * User: Contractor V&v Date: 6/08/10    Time: 5:50p
 * Updated in $/software/control processor/code/application
 * SCR #615 Showcfg/Long msg enhancement
 *
 * *****************  Version 16  *****************
 * User: Contractor2  Date: 5/28/10    Time: 1:40p
 * Updated in $/software/control processor/code/application
 * SCR #595 CP SW Version accessable through USER.c command
 * Added SW Version number to fast.status command.
 * Also some minor coding standard fixes.
 *
 * *****************  Version 15  *****************
 * User: Contractor V&v Date: 5/12/10    Time: 3:47p
 * Updated in $/software/control processor/code/application
 * SCR #351 fast.reset=really housekeeping
 *
 * *****************  Version 14  *****************
 * User: Jim Mood     Date: 5/07/10    Time: 6:21p
 * Updated in $/software/control processor/code/application
 * Added VPN Connected status
 *
 * *****************  Version 13  *****************
 * User: Contractor V&v Date: 4/12/10    Time: 7:15p
 * Updated in $/software/control processor/code/application
 * SCR #540 Log reason for cfg reset
 *
 * *****************  Version 12  *****************
 * User: Contractor V&v Date: 3/10/10    Time: 4:39p
 * Updated in $/software/control processor/code/application
 * SCR #248 Parameter Log Change
 *
 * *****************  Version 11  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 4:09p
 * Updated in $/software/control processor/code/application
 * //Remove undef artifact
 *
 * *****************  Version 10  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 3:42p
 * Updated in $/software/control processor/code/application
 * SCR 106 Remove USE_GENERIC_ACCESSOR macro
 *
 * *****************  Version 9  *****************
 * User: Contractor2  Date: 3/02/10    Time: 11:37a
 * Updated in $/software/control processor/code/application
 * SCR# 472 - Fix file/Function headers, footer
 *
 * *****************  Version 8  *****************
 * User: Jeff Vahue   Date: 3/02/10    Time: 10:16a
 * Updated in $/software/control processor/code/application
 * SCR# 469 - provide unique override commands for WLAN and GSM
 *
 * *****************  Version 7  *****************
 * User: Jeff Vahue   Date: 2/26/10    Time: 2:59p
 * Updated in $/software/control processor/code/application
 * SCR# 469 - RF GSM/WLAN POWER logic implementation
 *
 * *****************  Version 6  *****************
 * User: Jeff Vahue   Date: 2/17/10    Time: 12:51p
 * Updated in $/software/control processor/code/application
 * SCR# 452 - Code Coverage if/then/else changes
 *
 * *****************  Version 5  *****************
 * User: Contractor V&v Date: 2/03/10    Time: 2:56p
 * Updated in $/software/control processor/code/application
 * SCR 18
 *
 * *****************  Version 4  *****************
 * User: Contractor V&v Date: 1/27/10    Time: 5:17p
 * Updated in $/software/control processor/code/application
 * SCR 267 Upload DebugMessage Content
 *
 * *****************  Version 3  *****************
 * User: Contractor V&v Date: 1/13/10    Time: 4:57p
 * Updated in $/software/control processor/code/application
 *
 * *****************  Version 2  *****************
 * User: Jeff Vahue   Date: 12/14/09   Time: 2:35p
 * Updated in $/software/control processor/code/application
 * SCR# 106 - fields from fast.reset removed by mistake
 *
 * *****************  Version 1  *****************
 * User: Contractor V&v Date: 12/10/09   Time: 5:35p
 * Created in $/software/control processor/code/application
 * SCR 42 and SCR 106
*
****************************************************************************/
