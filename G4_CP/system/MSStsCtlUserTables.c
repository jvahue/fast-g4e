#define MSSC_USERTABLE_BODY
/******************************************************************************
         Copyright (C) 2008-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:        MSStsCtlUserTables.c

    Description: MicroServer Status and Control User Commands.

    VERSION
    $Revision: 26 $  $Date: 12/14/12 8:05p $
******************************************************************************/
#ifndef MSSC_BODY
#error MSStsCtlUserTables.c should only be included by MSStsCtl.c
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

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static USER_HANDLER_RESULT MSSC_GsmCfgMsg ( USER_DATA_TYPE DataType,
                                            USER_MSG_PARAM Param,
                                            UINT32 Index,
                                            const void *SetPtr,
                                            void **GetPtr);

static USER_HANDLER_RESULT MSSC_CfgMsg   ( USER_DATA_TYPE DataType,
                                           USER_MSG_PARAM Param,
                                           UINT32 Index,
                                           const void *SetPtr,
                                           void **GetPtr);

static USER_HANDLER_RESULT MSSC_ShellCmd ( USER_DATA_TYPE DataType,
                                           USER_MSG_PARAM Param,
                                           UINT32 Index,
                                           const void *SetPtr,
                                           void **GetPtr);

static USER_HANDLER_RESULT MSSC_ShowConfig ( USER_DATA_TYPE DataType,
                                             USER_MSG_PARAM Param,
                                             UINT32 Index,
                                             const void *SetPtr,
                                             void **GetPtr);

static USER_HANDLER_RESULT MSSC_RefreshMSInfo ( USER_DATA_TYPE DataType,
                                                USER_MSG_PARAM Param,
                                                UINT32 Index,
                                                const void *SetPtr,
                                                void **GetPtr );

static USER_HANDLER_RESULT MSSC_MSTimeMsg ( USER_DATA_TYPE DataType,
                                            USER_MSG_PARAM Param,
                                            UINT32 Index,
                                            const void *SetPtr,
                                            void **GetPtr );

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/

#pragma ghs nowarning 1545 //Suppress packed structure alignment warning

static USER_ENUM_TBL mSSC_GsmModeCfgEnum[] =
  {
    {"GPRS",MSCP_GSM_MODE_GPRS},
    {"GSM",MSCP_GSM_MODE_GSM},
    {NULL,0}
  };

USER_ENUM_TBL MSSC_MssimStatusEnum[] =
  {
    {"NO", MSSC_STARTUP},
    {"YES",MSSC_RUNNING},
    {"NO", MSSC_DEAD},
    {NULL, 0}
  };

static USER_MSG_TBL msStatusGsmMsgs[] =
{  /*Str            Next Tbl Ptr      Handler Func.          Data Type           Access        Parameter                    IndexRange   DataLimit EnumTbl*/
   { "IMEI"       , NO_NEXT_TABLE  ,  User_GenericAccessor , USER_TYPE_STR    ,  USER_RO     , m_GetMSInfoRsp.Gsm.Imei ,   -1, -1    ,  NO_LIMIT, NULL },
   { "SCID"       , NO_NEXT_TABLE  ,  User_GenericAccessor , USER_TYPE_STR    ,  USER_RO     , m_GetMSInfoRsp.Gsm.Scid ,   -1, -1    ,  NO_LIMIT, NULL },
/*  Note: requesting Signal sends the command over to the MS to re-refresh all GSM status information */
   { "SIGNAL"     , NO_NEXT_TABLE  ,  MSSC_RefreshMSInfo   , USER_TYPE_STR    ,  USER_RO     , m_GetMSInfoRsp.Gsm.Signal , -1, -1    ,  NO_LIMIT, NULL },
   { "OPERATOR"   , NO_NEXT_TABLE  ,  User_GenericAccessor , USER_TYPE_STR    ,  USER_RO     , m_GetMSInfoRsp.Gsm.Operator,-1, -1    ,  NO_LIMIT, NULL },
   { "CIMI"       , NO_NEXT_TABLE  ,  User_GenericAccessor , USER_TYPE_STR    ,  USER_RO     , m_GetMSInfoRsp.Gsm.Cimi ,   -1, -1    ,  NO_LIMIT, NULL },
   { "CREG"       , NO_NEXT_TABLE  ,  User_GenericAccessor , USER_TYPE_STR    ,  USER_RO     , m_GetMSInfoRsp.Gsm.Creg ,   -1, -1    ,  NO_LIMIT, NULL },
   { "LAC"        , NO_NEXT_TABLE  ,  User_GenericAccessor , USER_TYPE_STR    ,  USER_RO     , m_GetMSInfoRsp.Gsm.Lac ,    -1, -1    ,  NO_LIMIT, NULL },
   { "CELL"       , NO_NEXT_TABLE  ,  User_GenericAccessor , USER_TYPE_STR    ,  USER_RO     , m_GetMSInfoRsp.Gsm.Cell ,   -1, -1    ,  NO_LIMIT, NULL },
   { "NCC"        , NO_NEXT_TABLE  ,  User_GenericAccessor , USER_TYPE_STR    ,  USER_RO     , m_GetMSInfoRsp.Gsm.Ncc ,    -1, -1    ,  NO_LIMIT, NULL },
   { "BCC"        , NO_NEXT_TABLE  ,  User_GenericAccessor , USER_TYPE_STR    ,  USER_RO     , m_GetMSInfoRsp.Gsm.Bcc ,    -1, -1    ,  NO_LIMIT, NULL },
   { "CNUM"       , NO_NEXT_TABLE  ,  User_GenericAccessor , USER_TYPE_STR    ,  USER_RO     , m_GetMSInfoRsp.Gsm.Cnum ,   -1, -1    ,  NO_LIMIT, NULL },
   { "VOLTAGE"    , NO_NEXT_TABLE  ,  User_GenericAccessor , USER_TYPE_STR    ,  USER_RO     , m_GetMSInfoRsp.Gsm.Sbv ,    -1, -1    ,  NO_LIMIT, NULL },
   { "SCTM"       , NO_NEXT_TABLE  ,  User_GenericAccessor , USER_TYPE_STR    ,  USER_RO     , m_GetMSInfoRsp.Gsm.Sctm ,   -1, -1    ,  NO_LIMIT, NULL },
   { "OTHER"      , NO_NEXT_TABLE  ,  User_GenericAccessor , USER_TYPE_STR    ,  USER_RO     , m_GetMSInfoRsp.Gsm.Other ,  -1, -1    ,  NO_LIMIT, NULL },
   { NULL         , NULL,             NULL           , NO_HANDLER_DATA}
};

static USER_MSG_TBL msStatusMsgs[] =
{  /*Str            Next Tbl Ptr      Handler Func.          Data Type           Access        Parameter               IndexRange   DataLimit     EnumTbl*/
   { "MS_RDY"     , NO_NEXT_TABLE  ,  User_GenericAccessor , USER_TYPE_ENUM   ,  USER_RO     , &m_MssimStatus           , -1, -1    , NO_LIMIT  ,   MSSC_MssimStatusEnum },
   { "CF_MOUNTED" , NO_NEXT_TABLE  ,  User_GenericAccessor , USER_TYPE_YESNO  ,  USER_RO     , &m_CompactFlashMounted   , -1, -1    , NO_LIMIT  ,   NULL },
   { "CLIENT_CONN", NO_NEXT_TABLE  ,  User_GenericAccessor , USER_TYPE_YESNO  ,  USER_RO     , &m_IsClientConnected     , -1, -1    , NO_LIMIT  ,   NULL },
   { "VPN_CONN"   , NO_NEXT_TABLE  ,  User_GenericAccessor , USER_TYPE_YESNO  ,  USER_RO     , &m_IsVPNConnected        , -1, -1    , NO_LIMIT  ,   NULL },
   { "MSSIM_VER"  , NO_NEXT_TABLE  ,  User_GenericAccessor , USER_TYPE_STR    ,  USER_RO     , m_GetMSInfoRsp.MssimVer ,  -1, -1    , NO_LIMIT  ,   NULL },
   { "SETRIX_VER" , NO_NEXT_TABLE  ,  User_GenericAccessor , USER_TYPE_STR    ,  USER_RO     , m_GetMSInfoRsp.SetrixVer,  -1, -1    , NO_LIMIT  ,   NULL },
   { "PW_VER"     , NO_NEXT_TABLE  ,  User_GenericAccessor , USER_TYPE_STR    ,  USER_RO     , m_GetMSInfoRsp.PWEHVer  ,  -1, -1    , NO_LIMIT  ,   NULL },
   { "MS_TIME"    , NO_NEXT_TABLE  ,  MSSC_MSTimeMsg       , USER_TYPE_STR    ,  USER_RO     , &m_MsTime                , -1, -1    , NO_LIMIT  ,   NULL },
   { "GSM"        , msStatusGsmMsgs,  NULL                 , NO_HANDLER_DATA} ,
   { NULL         , NULL           ,  NULL                 , NO_HANDLER_DATA}
};

static USER_MSG_TBL msCfgGsmMsgs[] =
{  /*Str            Next Tbl Ptr      Handler Func.      Data Type           Access        Parameter             IndexRange    DataLimit               EnumTbl*/
   { "CARRIER"    , NO_NEXT_TABLE  ,  MSSC_GsmCfgMsg   , USER_TYPE_STR    ,  USER_RW     , m_GsmCfgTemp.sCarrier  , -1, -1   ,0, MSSC_CFG_STR_MAX_LEN , NULL },
   { "ISPNB"      , NO_NEXT_TABLE  ,  MSSC_GsmCfgMsg   , USER_TYPE_STR    ,  USER_RW     , m_GsmCfgTemp.sPhone    , -1, -1   ,0, MSSC_CFG_STR_MAX_LEN , NULL },
   { "MODE"       , NO_NEXT_TABLE  ,  MSSC_GsmCfgMsg   , USER_TYPE_ENUM   ,  USER_RW     , &m_GsmCfgTemp.mode     , -1, -1   ,NO_LIMIT                , mSSC_GsmModeCfgEnum },
   { "APN"        , NO_NEXT_TABLE  ,  MSSC_GsmCfgMsg   , USER_TYPE_STR    ,  USER_RW     , m_GsmCfgTemp.sAPN      , -1, -1   ,0, MSSC_CFG_STR_MAX_LEN , NULL },
   { "USER"       , NO_NEXT_TABLE  ,  MSSC_GsmCfgMsg   , USER_TYPE_STR    ,  USER_RW     , m_GsmCfgTemp.sUser     , -1, -1   ,0, MSSC_CFG_STR_MAX_LEN , NULL },
   { "PASSWORD"   , NO_NEXT_TABLE  ,  MSSC_GsmCfgMsg   , USER_TYPE_STR    ,  USER_RW     , m_GsmCfgTemp.sPassword , -1, -1   ,0, MSSC_CFG_STR_MAX_LEN , NULL },
   { "MCC"        , NO_NEXT_TABLE  ,  MSSC_GsmCfgMsg   , USER_TYPE_STR    ,  USER_RW     , m_GsmCfgTemp.sMCC      , -1, -1   ,0, MSSC_CFG_STR_MAX_LEN , NULL },
   { "MNC"        , NO_NEXT_TABLE  ,  MSSC_GsmCfgMsg   , USER_TYPE_STR    ,  USER_RW     , m_GsmCfgTemp.sMNC      , -1, -1   ,0, MSSC_CFG_STR_MAX_LEN , NULL },
   { NULL,    NULL,        NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL msCfgMsgs[] =
{  /*Str            Next Tbl Ptr      Handler Func.      Data Type           Access    Parameter                       IndexRange   DataLimit    EnumTbl*/
   { "GSM"        , msCfgGsmMsgs   ,  NULL             , NO_HANDLER_DATA},
   { "PBITSYSCOND", NO_NEXT_TABLE  ,  MSSC_CfgMsg      , USER_TYPE_ENUM,     USER_RW,  &m_MsCfgTemp.sysCondPBIT,       -1, -1,      NO_LIMIT,    Flt_UserEnumStatus },
   { "CBITSYSCOND", NO_NEXT_TABLE  ,  MSSC_CfgMsg      , USER_TYPE_ENUM,     USER_RW,  &m_MsCfgTemp.sysCondCBIT,       -1, -1,      NO_LIMIT,    Flt_UserEnumStatus },
   { "HBSTARTUP_S", NO_NEXT_TABLE  ,  MSSC_CfgMsg      , USER_TYPE_UINT16,   USER_RW,  &m_MsCfgTemp.heartBeatStartup_s,-1, -1,      NO_LIMIT,    NULL },
   { "HBLOSS_S"   , NO_NEXT_TABLE  ,  MSSC_CfgMsg      , USER_TYPE_UINT16,   USER_RW,  &m_MsCfgTemp.heartBeatLoss_s,   -1, -1,      NO_LIMIT,    NULL },
   { "HBMAXLOGS" ,  NO_NEXT_TABLE  ,  MSSC_CfgMsg      , USER_TYPE_UINT8,    USER_RW,  &m_MsCfgTemp.heartBeatLogCnt,   -1, -1,      NO_LIMIT,    NULL },
   { "PRI4_MAX_MB", NO_NEXT_TABLE  ,  MSSC_CfgMsg      , USER_TYPE_UINT16,   USER_RW,  &m_MsCfgTemp.pri4DirMaxSizeMB,  -1, -1,      NO_LIMIT,    NULL },
   { NULL         , NULL           ,  NULL             , NO_HANDLER_DATA}
};

static USER_MSG_TBL msMsgs[] =
{   /*Str           Next Tbl Ptr      Handler Func.      Data Type           Access                          Parameter    IndexRange    DataLimit           EnumTbl*/
   { "STATUS"     , msStatusMsgs   ,  NULL             , NO_HANDLER_DATA}  ,
   { "CFG"        , msCfgMsgs      ,  NULL             , NO_HANDLER_DATA}  ,
   { "CMD"        , NO_NEXT_TABLE  ,  MSSC_ShellCmd    , USER_TYPE_STR     , USER_RW                         ,NULL      , -1, -1       , 1, 1024           , NULL },
   { DISPLAY_CFG  , NO_NEXT_TABLE  ,  MSSC_ShowConfig  , USER_TYPE_ACTION  ,(USER_RO|USER_NO_LOG|USER_GSE)   ,NULL      , -1, -1       , NO_LIMIT          , NULL},
   { NULL         , NULL           ,  NULL             , NO_HANDLER_DATA}
};
#pragma ghs endnowarning

static
USER_MSG_TBL MsRoot   = {"MS", msMsgs, NULL, NO_HANDLER_DATA};

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/


/******************************************************************************
* Function:    MSSC_GsmCfgMsg
*
* Description: User Manager callback for GSM configuration commands
*
* Parameters:   [in] DataType:  C type of the data to be read or changed, used
*                               for casting the data pointers
*               [in/out] Param: Pointer to the configuration item to be read
*                               or changed
*               [in] Index:     Index parameter is used to reference the
*                               specific param to change.  Range is validated
*                               by the user manager
*               [in] SetPtr:    For write commands, a pointer to the data to
*                               write to the configuration.
*               [out] GetPtr:   For read commands, UserCfg function will set
*
* Returns:     USER_RESULT_OK: Configuration set successfully
*              USER_RESULT_ERROR: Configuration error
*
* Notes:
*
*****************************************************************************/
static
USER_HANDLER_RESULT MSSC_GsmCfgMsg(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr)
{
  USER_HANDLER_RESULT userResult = USER_RESULT_ERROR;

  //Grab configuration to be read or written.
  memcpy(&m_GsmCfgTemp, &CfgMgr_ConfigPtr()->GsmConfig, sizeof(MSSC_GSM_CONFIG));

  userResult = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

  // If a successful "set" was performed, update config file.
  if(SetPtr != NULL && USER_RESULT_OK == userResult)
  {
    memcpy(&CfgMgr_ConfigPtr()->GsmConfig,&m_GsmCfgTemp,sizeof(m_GsmCfgTemp));
    if(CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
      &CfgMgr_ConfigPtr()->GsmConfig,
      sizeof(m_GsmCfgTemp)))
    {
      userResult = USER_RESULT_OK;
    }
  }

  return userResult;
}


/******************************************************************************
 * Function:    MSSC_ShellCmd
 *
 * Description: User Manager callback sending Linux shell commands to the
 *              Micro-Server, and receiving the output of the commands
 *
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used
 *                                for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                               or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific param to change.  Range is validated
 *                               by the user manager
 *               [in] SetPtr:    For write commands, a pointer to the data to
 *                               write to the configuration.
 *               [out] GetPtr:   For read commands, UserCfg function will set *
 * Returns:     USER_RESULT_OK: Configuration set successfully
 *              USER_RESULT_ERROR: Configuration error
 *
 * Notes:
 *
 *****************************************************************************/
static
USER_HANDLER_RESULT MSSC_ShellCmd(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr)
{
  USER_HANDLER_RESULT userResult = USER_RESULT_ERROR;
  MSCP_CP_SHELL_CMD cmd;
  const INT8* str = SetPtr;

  if(SetPtr == NULL)
  {
    if(m_MSShellRsp.Len != 0)
    { //buffer length is already checked and terminated by MS response handler
      *GetPtr = m_MSShellRsp.Str;
      userResult = USER_RESULT_OK;
      m_MSShellRsp.Len = 0;
    }
  }
  else
  {
    cmd.Len = strlen(str);
    strncpy_safe(cmd.Str,sizeof(cmd.Str),str,_TRUNCATE);
    if(SYS_OK == MSI_PutCommand(CMD_ID_SHELL_CMD,&cmd,
        sizeof(cmd)-sizeof(cmd.Str)+cmd.Len,30000,MSSC_MSICallback_ShellCmd))
    {
      userResult = USER_RESULT_OK;
    }
  }

  return userResult;
}


/******************************************************************************
* Function:     MSSC_ShowConfig
*
* Description:  Handles User Manager requests to retrieve the configuration
*               settings.
*
* Parameters:   [in] DataType:  C type of the data to be read or changed, used
*                               for casting the data pointers
*               [in/out] Param: Pointer to the configuration item to be read
*                               or changed
*               [in] Index:     Index parameter is used to reference the
*                               specific param to change.  Range is validated
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
static
USER_HANDLER_RESULT MSSC_ShowConfig(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr)
{
   CHAR  labelStem[] = "\r\n\r\nMS.CFG";
   CHAR  label[USER_MAX_MSG_STR_LEN * 3];
   const INT16 i = 0;

   USER_HANDLER_RESULT result = USER_RESULT_OK;
   USER_MSG_TBL* pCfgTable = msCfgMsgs;  // Get pointer to config entry;

   //Top-level name is a single indented space
   CHAR branchName[USER_MAX_MSG_STR_LEN] = " ";


   // Display element info above each set of data.
   snprintf(label, sizeof(label), "%s", labelStem, 0);

   result = USER_RESULT_ERROR;
   if (User_OutputMsgString( label, FALSE ) )
   {
      result = User_DisplayConfigTree(branchName, pCfgTable, i, 0, NULL);
   }
   return result;
}



/******************************************************************************
* Function:    MSSC_RefreshMSInfo
*
* Description: Refresh the micro-server information (including version and
*              static GSM data) stored locally.  Pass parameters into
*              generic accessor to read the GSM signal strength (see table
*              above)
*
* Parameters:   [in] DataType:  C type of the data to be read or changed, used
*                               for casting the data pointers
*               [in/out] Param: Pointer to the configuration item to be read
*                               or changed
*               [in] Index:     Index parameter is used to reference the
*                               specific param to change.  Range is validated
*                               by the user manager
*               [in] SetPtr:    For write commands, a pointer to the data to
*                               write to the configuration.
*               [out] GetPtr:   For read commands, UserCfg function will set
*                               this to the location of the data requested.
*
* Returns:     USER_RESULT_OK: Configuration set successfully
*              USER_RESULT_ERROR: Configuration error
*
* Notes:
*
*****************************************************************************/
static
USER_HANDLER_RESULT MSSC_RefreshMSInfo(USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr)
{
  //Refresh Micro-Server status info.  It will be available well within
  //a few seconds for the next time the user executes ms.status.gsm.signal.
  MSSC_DoRefreshMSInfo();
  return User_GenericAccessor(DataType,Param,Index,SetPtr,GetPtr);

}



/******************************************************************************
* Function:    MSSC_CfgMsg
*
* Description: User Manager callback for general MSSC configuration commands
*
* Parameters:   [in] DataType:  C type of the data to be read or changed, used
*                               for casting the data pointers
*               [in/out] Param: Pointer to the configuration item to be read
*                               or changed
*               [in] Index:     Index parameter is used to reference the
*                               specific param to change.  Range is validated
*                               by the user manager
*               [in] SetPtr:    For write commands, a pointer to the data to
*                               write to the configuration.
*               [out] GetPtr:   For read commands, UserCfg function will set
*                               this to the location of the data requested.
*
* Returns:     USER_RESULT_OK: Configuration set successfully
*              USER_RESULT_ERROR: Configuration error
*
* Notes:
*
*****************************************************************************/
static
USER_HANDLER_RESULT MSSC_CfgMsg(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr)
{
  USER_HANDLER_RESULT userResult = USER_RESULT_ERROR;

  //Grab configuration to be read or written.
  memcpy(&m_MsCfgTemp, &CfgMgr_ConfigPtr()->MsscConfig, sizeof(m_MsCfgTemp));

  userResult = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

  // If a successful "set" was performed, update config file.
  if(SetPtr != NULL && USER_RESULT_OK == userResult)
  {
    memcpy(&CfgMgr_ConfigPtr()->MsscConfig, &m_MsCfgTemp, sizeof(m_MsCfgTemp));

    if( CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
                               &CfgMgr_ConfigPtr()->MsscConfig,
                               sizeof(m_MsCfgTemp)))
    {
      userResult = USER_RESULT_OK;
    }
  }

  return userResult;
}

/******************************************************************************
 * Function:    MSSC_MSTimeMsg
 *
 * Description: Get the Micro-Server time and date.
 *
* Parameters:   [in] DataType:  C type of the data to be read or changed, used
*                               for casting the data pointers
*               [in/out] Param: Pointer to the configuration item to be read
*                               or changed
*               [in] Index:     Index parameter is used to reference the
*                               specific param to change.  Range is validated
*                               by the user manager
*               [in] SetPtr:    For write commands, a pointer to the data to
*                               write to the configuration.
*               [out] GetPtr:   For read commands, UserCfg function will set
*                               this to the location of the data requested.
 *
 * Returns:     USER_HANDLER_RESULT
 *
 * Notes:
 *
 *****************************************************************************/
static
USER_HANDLER_RESULT MSSC_MSTimeMsg(USER_DATA_TYPE DataType,
                                   USER_MSG_PARAM Param,
                                   UINT32 Index,
                                   const void *SetPtr,
                                   void **GetPtr)
{
  static CHAR timeStr[80]; // TimeStr needs to be static since,
                           // GetPtr will hold a ref to it on return.

  USER_HANDLER_RESULT userResult = USER_RESULT_ERROR;

  if(SetPtr == NULL)
  {
    snprintf(timeStr,80,"%02d/%02d/%04d %02d:%02d:%02d.%03d",
             m_MsTime.Month ,
             m_MsTime.Day   ,
             m_MsTime.Year  ,
             m_MsTime.Hour  ,
             m_MsTime.Minute,
             m_MsTime.Second,
             m_MsTime.MilliSecond );

    *GetPtr = timeStr;
     userResult = USER_RESULT_OK;

  }

  return userResult;
}

/*************************************************************************
*  MODIFICATIONS
*    $History: MSStsCtlUserTables.c $
 * 
 * *****************  Version 26  *****************
 * User: Jim Mood     Date: 12/14/12   Time: 8:05p
 * Updated in $/software/control processor/code/system
 * SCR #1197
 *
 * *****************  Version 25  *****************
 * User: John Omalley Date: 12-11-16   Time: 10:32p
 * Updated in $/software/control processor/code/system
 * SCR 1197 - Code Review Updates
 *
 * *****************  Version 24  *****************
 * User: John Omalley Date: 12-11-13   Time: 5:46p
 * Updated in $/software/control processor/code/system
 * SCR 1197 - Code Review Updates
 *
 * *****************  Version 23  *****************
 * User: John Omalley Date: 12-11-12   Time: 10:57a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review Updates
 *
 * *****************  Version 22  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 21  *****************
 * User: Jim Mood     Date: 7/19/12    Time: 11:07a
 * Updated in $/software/control processor/code/system
 * SCR 1107: Data Offload changes for 2.0.0
 *
 * *****************  Version 20  *****************
 * User: John Omalley Date: 8/02/10    Time: 3:16p
 * Updated in $/software/control processor/code/system
 * SCR 763 - Implemented the ms.status.ms_time command
 *
 * *****************  Version 19  *****************
 * User: Contractor2  Date: 7/30/10    Time: 2:29p
 * Updated in $/software/control processor/code/system
 * SCR #90;243;252 Implementation: CBIT of mssim
 *
 * *****************  Version 18  *****************
 * User: Jim Mood     Date: 7/30/10    Time: 8:57a
 * Updated in $/software/control processor/code/system
 * SCF 327 Fast transmission test functionality and gsm status information
 *
 * *****************  Version 17  *****************
 * User: Contractor V&v Date: 7/27/10    Time: 6:20p
 * Updated in $/software/control processor/code/system
 * [MOD] MSStsCtlUserTables.c:16
 *
 * *****************  Version 16  *****************
 * User: Contractor V&v Date: 7/21/10    Time: 7:18p
 * Updated in $/software/control processor/code/system
 * SCR #260 Implement BOX status command
 *
 * *****************  Version 15  *****************
 * User: Contractor3  Date: 7/16/10    Time: 8:57a
 * Updated in $/software/control processor/code/system
 * SCR #702 - Changes based on code review.
 *
 * *****************  Version 14  *****************
 * User: Contractor V&v Date: 7/14/10    Time: 4:45p
 * Updated in $/software/control processor/code/system
 * SCR #479 Micro server status GSE commands to add/ ms.status
 *
 * *****************  Version 13  *****************
 * User: Jim Mood     Date: 7/09/10    Time: 8:28p
 * Updated in $/software/control processor/code/system
 * SCR 607 mod fix (removed cf status stub, repalced with real cf status)
 *
 * *****************  Version 12  *****************
 * User: Contractor V&v Date: 6/08/10    Time: 5:55p
 * Updated in $/software/control processor/code/system
 * SCR #615 Showcfg/Long msg enhancement
 *
 * *****************  Version 11  *****************
 * User: Contractor V&v Date: 5/26/10    Time: 2:11p
 * Updated in $/software/control processor/code/system
 * SCR #607 The CP shall query Ms CF mounted before File Upload pr
 *
 * *****************  Version 10  *****************
 * User: Contractor V&v Date: 4/28/10    Time: 4:54p
 * Updated in $/software/control processor/code/system
 * SCR #543 Misc - MSSC_GsmCfgMsg should use generic accessor
 *
 * *****************  Version 9  *****************
 * User: Contractor V&v Date: 4/22/10    Time: 6:41p
 * Updated in $/software/control processor/code/system
 * SCR #453 Misc fix
 *
 * *****************  Version 8  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:11p
 * Updated in $/software/control processor/code/system
 * SCR #317 Implement safe strncpy
 *
 * *****************  Version 7  *****************
 * User: Contractor V&v Date: 3/19/10    Time: 4:31p
 * Updated in $/software/control processor/code/system
 * SCR #248 Parameter Log Change Add no log for showcfg
 *
 * *****************  Version 6  *****************
 * User: Jeff Vahue   Date: 3/05/10    Time: 12:16p
 * Updated in $/software/control processor/code/system
 * SCR# 413 - Actions cmds are RO
 *
 * *****************  Version 5  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 4  *****************
 * User: Jeff Vahue   Date: 2/17/10    Time: 1:24p
 * Updated in $/software/control processor/code/system
 * SCR# 452 - code coverage mods
 *
 * *****************  Version 3  *****************
 * User: Jeff Vahue   Date: 2/09/10    Time: 11:12a
 * Updated in $/software/control processor/code/system
 * SCR# 404 - String limits on MS Sts cmds
 *
 * *****************  Version 2  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:26p
 * Updated in $/software/control processor/code/system
 * SCR 371
 *
 * *****************  Version 1  *****************
 * User: Contractor V&v Date: 12/10/09   Time: 5:44p
 * Created in $/software/control processor/code/system
 * SCR 106
*
*
****************************************************************************/
