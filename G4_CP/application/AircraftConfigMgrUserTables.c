/******************************************************************************
Copyright (C) 2008-2010 Pratt & Whitney Engine Services, Inc. 
Altair Engine Diagnostic Solutions
All Rights Reserved. Proprietary and Confidential.

File:          AircraftConfigMgrUserTables.c

Description: 

VERSION
$Revision: 15 $  $Date: 9/03/10 1:49p $ 

******************************************************************************/
#ifndef AIRCRAFTCONFIGMGR_BODY
#error AircraftConfigMgrUserTables.c should only be included by AircraftConfigMgr.c
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
#define AIRCRAFT_UPDATE_TASK_SEND_CMD       0
#define AIRCRAFT_UPDATE_TASK_WT_RSP         1
#define AIRCRAFT_UPDATE_TASK_GOT_RSP        2
#define AIRCRAFT_UPDATE_TASK_GOT_RSP_FAILED 3
#define AIRCRAFT_UPDATE_TASK_REBOOT_WT      4

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
typedef struct {
  INT32             State;
  INT32             Retry;
  AIRCRAFT_CFG_INFO MsRspData;
} AIRCRAFT_UPDATE_TASK_DATA;

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/

static AIRCRAFT_CFG_INFO     m_AircraftConfigInfo;
static AIRCRAFT_CONFIG       m_CfgTemp;
static AIRCRAFT_UPDATE_TASK_DATA m_AircraftUpdateTaskData;

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
USER_HANDLER_RESULT       AircraftUserCfg(USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr);

USER_HANDLER_RESULT       AircraftShowConfig(USER_DATA_TYPE DataType,
                                             USER_MSG_PARAM Param,
                                             UINT32 Index,
                                             const void *SetPtr,
                                             void **GetPtr);
                                             
USER_HANDLER_RESULT AC_CancelCfgHandler(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr);
USER_HANDLER_RESULT AC_CommitCfgHandler(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr);
USER_HANDLER_RESULT AC_StartAutoHandler(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr);
USER_HANDLER_RESULT AC_StartManualHandler(USER_DATA_TYPE DataType,
                                           USER_MSG_PARAM Param,
                                           UINT32 Index,
                                           const void *SetPtr,
                                           void **GetPtr);
USER_HANDLER_RESULT AC_ClearCfgDirHandler( USER_DATA_TYPE DataType,
                                           USER_MSG_PARAM Param,
                                           UINT32 Index,
                                           const void *SetPtr,
                                           void **GetPtr);

                                           

/*****************************************************************************/
/* User command tables                                                       */
/*****************************************************************************/
static USER_ENUM_TBL AircraftTaskStateEnum[] =
{
  { "Stopped",           TASK_STOPPED                       },
  { "WaitMS",            TASK_WAIT                          },
  { m_RecfgErrorStr,     TASK_ERROR                         },
  { "StartAuto",         TASK_START_AUTO                    },
  { "StartManual",       TASK_START_MANUAL                  },
  { "WtForMSSIM",        TASK_WAIT_MSSIM_RDY                },
  { "WtForUpload",       TASK_WAIT_LOG_UPLOAD               },
  { "WtForVPN",          TASK_WAIT_VPN_CONN                 },
  { "CfgFileDL",         TASK_PROCESS_RETRIEVE_CFG          },
  { "ValidateACData",    TASK_VALIDATE_AC_ID                },
  { m_ReqStsRsp.StatusStr,TASK_SEND_GET_CFG_LOAD_STS        },
  { m_ReqStsRsp.StatusStr,TASK_PROCESS_GET_CFG_LOAD_STS     },
  { "WtForUpload",       TASK_WAIT_LOG_UPLOAD_BEFORE_COMMIT },
  { "CfgLoadDone",       TASK_WAIT_FOR_COMMIT               },
  { "CommittingCfg",     TASK_COMMIT_CFG                    },
  { "Rebooting",         TASK_REBOOTING                     }
}; 


//note: 2647
static USER_MSG_TBL AircraftSensorCmd [] =
{
  {"TYPE", NO_NEXT_TABLE, User_GenericAccessor, USER_TYPE_FLOAT,USER_RO,
    &m_ACStoredSensorVals.Type,-1,-1,NO_LIMIT,NULL },
  {"FLEET", NO_NEXT_TABLE, User_GenericAccessor, USER_TYPE_FLOAT,USER_RO,
    &m_ACStoredSensorVals.Fleet,-1,-1,NO_LIMIT,NULL },
  {"NUMBER", NO_NEXT_TABLE, User_GenericAccessor, USER_TYPE_FLOAT,USER_RO,
    &m_ACStoredSensorVals.Number,-1,-1,NO_LIMIT,NULL },
  { NULL,      NULL,            NULL, NO_HANDLER_DATA}
};

//note: 2647,2643,3180,3181,2537,2892,2893,2894,2895,2896,2897,
static USER_MSG_TBL AircraftCfgCmd [] =
{
  {"CFGENABLED",  NO_NEXT_TABLE, AircraftUserCfg, USER_TYPE_YESNO ,USER_RW,
  &m_CfgTemp.bCfgEnabled,-1,-1,NO_LIMIT,NULL },
  {"VALIDATEDATA",NO_NEXT_TABLE, AircraftUserCfg, USER_TYPE_YESNO ,USER_RW,
  &m_CfgTemp.bValidateData,-1,-1,NO_LIMIT,NULL },
  {"TYPE",        NO_NEXT_TABLE, AircraftUserCfg, USER_TYPE_STR   ,USER_RW,
  &m_CfgTemp.Type,-1,-1, AC_STR_LIMIT, NULL },
  {"FLEETIDENT",  NO_NEXT_TABLE, AircraftUserCfg, USER_TYPE_STR   ,USER_RW,
  &m_CfgTemp.FleetIdent,-1,-1,AC_STR_LIMIT,NULL },
  {"NUMBER",      NO_NEXT_TABLE, AircraftUserCfg, USER_TYPE_STR   ,USER_RW,
  &m_CfgTemp.Number, -1,-1, AC_STR_LIMIT, NULL },
  {"OPERATOR",    NO_NEXT_TABLE, AircraftUserCfg, USER_TYPE_STR   ,USER_RW,
  &m_CfgTemp.Operator, -1, -1, AC_STR_LIMIT, NULL },
  {"OWNER",       NO_NEXT_TABLE, AircraftUserCfg, USER_TYPE_STR   ,USER_RW,
  &m_CfgTemp.Owner,-1,-1,AC_STR_LIMIT,NULL },
  {"TAILNUMBER",  NO_NEXT_TABLE, AircraftUserCfg, USER_TYPE_STR   ,USER_RW,
  &m_CfgTemp.TailNumber,-1,-1,AC_STR_LIMIT,NULL },
  {"STYLE",       NO_NEXT_TABLE, AircraftUserCfg, USER_TYPE_STR   ,USER_RW,
  &m_CfgTemp.Style,-1,-1,AC_STR_LIMIT,NULL },
  {"TYPEINDEX",   NO_NEXT_TABLE, AircraftUserCfg, USER_TYPE_ENUM,USER_RW,
  &m_CfgTemp.TypeSensorIndex,-1,-1,NO_LIMIT,SensorIndexType },
  {"FLEETINDEX",  NO_NEXT_TABLE, AircraftUserCfg, USER_TYPE_ENUM,USER_RW,
  &m_CfgTemp.FleetSensorIndex,-1,-1,NO_LIMIT,SensorIndexType },
  {"NUMBERINDEX", NO_NEXT_TABLE, AircraftUserCfg, USER_TYPE_ENUM,USER_RW,
  &m_CfgTemp.NumberSensorIndex,-1,-1,NO_LIMIT,SensorIndexType },
  { NULL,      NULL,            NULL, NO_HANDLER_DATA}
};
//note: 2642,3004,3354,3355
static USER_MSG_TBL AircraftReconfigCmd [] =
{ /*Str             Next Tbl Ptr   Handler Func.              Data Type         Access    Parameter         IndexRange  DataLimit           EnumTbl*/                        
  {"STATUS",        NO_NEXT_TABLE, User_GenericAccessor,      USER_TYPE_ENUM,   USER_RO,  &m_TaskState,     -1,-1,      NO_LIMIT,           AircraftTaskStateEnum },
  {"CFG_STATUS",    NO_NEXT_TABLE, User_GenericAccessor,      USER_TYPE_STR,    USER_RO,  &m_CfgStatusStr,  -1,-1,      NO_LIMIT,           NULL },
  {"START_AUTO",    NO_NEXT_TABLE, AC_StartAutoHandler,       USER_TYPE_ACTION, USER_RO,  NULL,             -1,-1,      NO_LIMIT,           NULL },
  {"START_MANUAL",  NO_NEXT_TABLE, AC_StartManualHandler,     USER_TYPE_ACTION, USER_RO,  NULL,             -1,-1,      NO_LIMIT,           NULL },
  {"CANCEL",        NO_NEXT_TABLE, AC_CancelCfgHandler,       USER_TYPE_ACTION, USER_RO,  NULL,             -1,-1,      NO_LIMIT,           NULL },
  {"COMMIT",        NO_NEXT_TABLE, AC_CommitCfgHandler,       USER_TYPE_ACTION, USER_RO,  NULL,             -1,-1,      NO_LIMIT,           NULL },
  {"CLEAR_CFG_DIR", NO_NEXT_TABLE, AC_ClearCfgDirHandler,     USER_TYPE_ACTION, USER_RO,  NULL,             -1,-1,      NO_LIMIT,           NULL },

  { NULL,      NULL,            NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL AircraftCmd [] =
{
  /*Str             Next Tbl Ptr          Handler Func    Data Type             Access    Parameter         IndexRange  DataLimit           EnumTbl*/                        
  {"CFG"   ,        AircraftCfgCmd,       NULL,           NO_HANDLER_DATA,                                                                             },
  {"SENSOR",        AircraftSensorCmd,    NULL,           NO_HANDLER_DATA,                                                                             },      
  {"RECONFIG",      AircraftReconfigCmd,  NULL,           NO_HANDLER_DATA,                                                                             },
  {DISPLAY_CFG      ,NO_NEXT_TABLE        ,AircraftShowConfig   ,USER_TYPE_ACTION   ,(USER_RO|USER_NO_LOG|USER_GSE)  ,NULL           ,-1,-1     ,NO_LIMIT     ,NULL},
  { NULL,           NULL,                 NULL,           NO_HANDLER_DATA}
};

static
USER_MSG_TBL RootMsg = {"AIRCRAFT",AircraftCmd,NULL,NO_HANDLER_DATA};


/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/




/******************************************************************************
 * Function:     AC_CancelCfgHandler
 *
 * Description:  Cancel a configuration load, manual or automatic that is
 *               currently in progress
 *
 * Parameters:   See user.h command handler prototype
 *
 * Returns:      OK: Canceled successfully
 *               ERROR: Not in the right state to commit the cfg.
 *
 * Notes:        
 *****************************************************************************/
USER_HANDLER_RESULT AC_CancelCfgHandler( USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_ERROR;
  
  if(AC_CancelRecfg())
  {
    result = USER_RESULT_OK;
  }

  return result;
}



/******************************************************************************
 * Function:     AC_CommitCfgHandler
 *
 * Description:  The user commit command starts copying the new configuration 
 *               loaded into temporary memory to be loaded into the EEPROM
 *               This command is only valid in manual configuration mode when
 *               the micro-server has completed transfering the configuration
 *               from the .cfg and .xml files.
 *
 * Parameters:   See user.h command handler prototype
 *
 * Returns:      OK: Started commit successfully
 *               ERROR: Not in the right state to commit the cfg.
 *
 * Notes:        
 *****************************************************************************/
USER_HANDLER_RESULT AC_CommitCfgHandler( USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr)
{
  USER_HANDLER_RESULT result;
  
  if(m_TaskState == TASK_WAIT_FOR_COMMIT)
  {
    m_bGotCommit = TRUE;
    result = USER_RESULT_OK;
  }
  else
  {
    result = USER_RESULT_ERROR;
  }

  return result;
}



/******************************************************************************
 * Function:     AC_StartAutoHandler
 *
 * Description:  Trigger the automatic configuration retrieval that receives
 *               the configuration from the ground server through the VPN
 *               connection
 *
 * Parameters:   See user.h command handler prototype
 *
 * Returns:      OK: Canceled successfully
 *               ERROR: Not in the right state to commit the cfg.
 *
 * Notes:        
 *****************************************************************************/
USER_HANDLER_RESULT AC_StartAutoHandler( USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_ERROR;
  
  if(AC_StartAutoRecfg())
  {
    result = USER_RESULT_OK;
  }

  return result;
}


/******************************************************************************
 * Function:     AC_StartManualHandler
 *
 * Description:  Trigger the manual configuration retrieval that uses
 *               configuration files previously loaded through the GSE port
 *               onto the micro-server.
 *
 * Parameters:   See user.h command handler prototype
 *
 * Returns:      OK: Canceled successfully
 *               ERROR: Not in the right state to commit the cfg.
 *
 * Notes:        
 *****************************************************************************/
USER_HANDLER_RESULT AC_StartManualHandler( USER_DATA_TYPE DataType,
                                           USER_MSG_PARAM Param,
                                           UINT32 Index,
                                           const void *SetPtr,
                                           void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_ERROR;
  
  if(AC_StartManualRecfg())
  {
    result = USER_RESULT_OK;
  }

  return result;
}



/******************************************************************************
 * Function:     AC_ClearCfgDirHandler
 *
 * Description:  Trigger the manual configuration retrieval that uses
 *               configuration files previously loaded through the GSE port
 *               onto the micro-server.
 *
 * Parameters:   See user.h command handler prototype
 *
 * Returns:      OK: Canceled successfully
 *               ERROR: Not in the right state to commit the cfg.
 *
 * Notes:        
 *****************************************************************************/
USER_HANDLER_RESULT AC_ClearCfgDirHandler( USER_DATA_TYPE DataType,
                                           USER_MSG_PARAM Param,
                                           UINT32 Index,
                                           const void *SetPtr,
                                           void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_ERROR;

  if(AC_SendClearCfgDir())
  {
    result = USER_RESULT_OK;
  }

  return result;
}



/******************************************************************************
 * Function:     AircraftUserCfg
 *
 * Description:  User message handler to set/get configuration items of the
 *               Aircraft tail number 
 *
 * Parameters:   See user.h command handler prototype
 *
 * Returns:      USER_RESULT_OK:    Processed sucessfully
 *               USER_RESULT_ERROR: Error processing command. 
 *
 * Notes:        none
 *
 *****************************************************************************/
USER_HANDLER_RESULT AircraftUserCfg(USER_DATA_TYPE DataType,
                                      USER_MSG_PARAM Param,
                                      UINT32 Index,
                                      const void *SetPtr,
                                      void **GetPtr)
{
 USER_HANDLER_RESULT result = USER_RESULT_OK;
 
  memcpy(&m_CfgTemp,
         &CfgMgr_ConfigPtr()->Aircraft,
         sizeof(m_CfgTemp));

  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
  // If a successful "set" was performed, update config file.
  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    memcpy(&CfgMgr_ConfigPtr()->Aircraft,
      &m_CfgTemp,
      sizeof(CfgMgr_ConfigPtr()->Aircraft));
    //Store the modified temporary structure in the EEPROM.       
    CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
      &CfgMgr_ConfigPtr()->Aircraft,
      sizeof(m_CfgTemp));
  }
  return result;
}




/******************************************************************************
 * Function:    AircraftShowConfig
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
USER_HANDLER_RESULT AircraftShowConfig(USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr)
{
  CHAR LabelStem[] = "\r\n\r\nAIRCRAFT.CFG";
  USER_HANDLER_RESULT result;   
  CHAR Label[USER_MAX_MSG_STR_LEN * 3];    

  //Top-level name is a single indented space
  CHAR BranchName[USER_MAX_MSG_STR_LEN] = " "; 

  USER_MSG_TBL*  pCfgTbl = AircraftCfgCmd;

  result = USER_RESULT_OK;  

  // Display element info above each set of data.
  sprintf(Label, "%s", LabelStem);

  if (User_OutputMsgString(Label, FALSE ) )
  {
     result = User_DisplayConfigTree(BranchName, pCfgTbl, 0, 0, NULL);
  }  
  return result;
}




/*************************************************************************
*  MODIFICATIONS
*    $History: AircraftConfigMgrUserTables.c $
 * 
 * *****************  Version 15  *****************
 * User: Peter Lee    Date: 9/03/10    Time: 1:49p
 * Updated in $/software/control processor/code/application
 * SCR #806 Code Review Updates
 * 
 * *****************  Version 14  *****************
 * User: Contractor V&v Date: 7/21/10    Time: 7:12p
 * Updated in $/software/control processor/code/application
 * SCR #260 Implement BOX status command
 * 
 * *****************  Version 13  *****************
 * User: Jim Mood     Date: 6/29/10    Time: 6:29p
 * Updated in $/software/control processor/code/application
 * SCR 623.  Automatic reconfiguration requirments complete
 * 
 * *****************  Version 12  *****************
 * User: Jim Mood     Date: 6/16/10    Time: 6:25p
 * Updated in $/software/control processor/code/application
 * SCR 623 Batch configuration load 
 * 
 * *****************  Version 11  *****************
 * User: Jim Mood     Date: 6/11/10    Time: 10:11a
 * Updated in $/software/control processor/code/application
 * Fixed a bad VSS Merge with conflicts
 * 
 * *****************  Version 10  *****************
 * User: Jim Mood     Date: 6/11/10    Time: 10:05a
 * Updated in $/software/control processor/code/application
 * SCR 623 Batch configuration changes
 * 
 * *****************  Version 9  *****************
 * User: Contractor V&v Date: 6/08/10    Time: 5:50p
 * Updated in $/software/control processor/code/application
 * SCR #615 Showcfg/Long msg enhancement
 * 
 * *****************  Version 8  *****************
 * User: Contractor V&v Date: 3/10/10    Time: 4:39p
 * Updated in $/software/control processor/code/application
 * SCR #106 fix orphaned macro tag
 * 
 * *****************  Version 7  *****************
 * User: Jeff Vahue   Date: 3/04/10    Time: 5:01p
 * Updated in $/software/control processor/code/application
 * SCR# 443 - change AircraftUserTables.c to use SensorIndexType for
 * TYPEINDEX, FLEETINDEX, NUMBERINDEX
 * 
 * *****************  Version 6  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 3:34p
 * Updated in $/software/control processor/code/application
 * 
 * *****************  Version 5  *****************
 * User: Contractor2  Date: 3/02/10    Time: 11:37a
 * Updated in $/software/control processor/code/application
 * SCR# 472 - Fix file/Function headers, footer
 * 
 * *****************  Version 4  *****************
 * User: Jeff Vahue   Date: 2/17/10    Time: 12:51p
 * Updated in $/software/control processor/code/application
 * SCR# 452 - Code Coverage if/then/else changes
 * 
 * *****************  Version 3  *****************
 * User: Contractor V&v Date: 2/03/10    Time: 2:56p
 * Updated in $/software/control processor/code/application
 * SCR 18
 * 
 * *****************  Version 2  *****************
 * User: Contractor V&v Date: 1/13/10    Time: 4:57p
 * Updated in $/software/control processor/code/application
 * 
 * *****************  Version 1  *****************
 * User: Contractor V&v Date: 12/10/09   Time: 5:34p
 * Created in $/software/control processor/code/application
 * SCR 42 and SCR 106
* 
*
***************************************************************************/
