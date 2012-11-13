#define ACTION_USERTABLE_BODY
/******************************************************************************
         Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
            All Rights Reserved. Proprietary and Confidential.

  File:        ActionManagerUserTables.c

Description:   User command structures and functions for the sction processing

VERSION
$Revision: 9 $  $Date: 12-11-12 6:47p $
******************************************************************************/
#ifndef ACTION_BODY
#error ActionManagerUserTables.c should only be included by ActionManager.c
#endif


/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Local Defines                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static ACTION_CFG    configActionTemp; // Action temp Storage
static ACTION_OUTPUT configOutputTemp;
static ACTION_DATA   stateActionTemp;  // Action State Temp Storage
static ACTION_FLAGS  stateActionFlagsTemp;

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
//Prototype for the User Manager message handlers, has to go before
//the local variable tables that use the function pointer.
USER_HANDLER_RESULT Action_UserCfg    ( USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr );
USER_HANDLER_RESULT Action_State(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr);
USER_HANDLER_RESULT Action_Flags(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr);
USER_HANDLER_RESULT ActionOut_UserCfg ( USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                         void **GetPtr );

USER_HANDLER_RESULT Action_ShowConfig ( USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr );
USER_HANDLER_RESULT Action_ClearLatch( USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr);

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
#pragma ghs nowarning 1545 //Suppress packed structure alignment warning

static USER_MSG_TBL actionOutCmd [] =
{
   /* Str                 Next Tbl Ptr       Handler Func.      Data Type          Access   Parameter                                  IndexRange                  DataLimit   EnumTbl*/
   { "USED_MASK",         NO_NEXT_TABLE,     ActionOut_UserCfg, USER_TYPE_UINT8,   USER_RW, &configOutputTemp.nUsedMask,                0,(MAX_ACTION_DEFINES-1),   NO_LIMIT,   NULL },
   { "LSS_MASK",          NO_NEXT_TABLE,     ActionOut_UserCfg, USER_TYPE_UINT8,   USER_RW, &configOutputTemp.nLSS_Mask,                0,(MAX_ACTION_DEFINES-1),   NO_LIMIT,   NULL },
   { NULL,                NULL,              NULL,              NO_HANDLER_DATA }
};

static USER_MSG_TBL actionCmd [] =
{
   /* Str                 Next Tbl Ptr       Handler Func.      Data Type          Access   Parameter                                   IndexRange              DataLimit   EnumTbl*/
   { "ACK_TRIGGER",       NO_NEXT_TABLE,     Action_UserCfg,    USER_TYPE_ENUM,    USER_RW, &configActionTemp.aCKTrigger,               -1,-1,                  NO_LIMIT,   TriggerIndexType },
   { "PERSIST_ENABLE",    NO_NEXT_TABLE,     Action_UserCfg,    USER_TYPE_BOOLEAN, USER_RW, &configActionTemp.persist.bEnabled,         -1,-1,                  NO_LIMIT,   NULL },
   { "PERSIST_USED_MASK", NO_NEXT_TABLE,     Action_UserCfg,    USER_TYPE_UINT8,   USER_RW, &configActionTemp.persist.output.nUsedMask, -1,-1,                  NO_LIMIT,   NULL },
   { "PERSIST_LSS_MASK",  NO_NEXT_TABLE,     Action_UserCfg,    USER_TYPE_UINT8,   USER_RW, &configActionTemp.persist.output.nLSS_Mask, -1,-1,                  NO_LIMIT,   NULL },
   { "OUTPUT",            actionOutCmd,      NULL,              NO_HANDLER_DATA },
   { "ACTIVE_STATE",      NO_NEXT_TABLE,     Action_UserCfg,    USER_TYPE_UINT8,   USER_RW, &configActionTemp.activeState,              -1,-1,                  NO_LIMIT,   NULL },
   { NULL,                NULL,              NULL,              NO_HANDLER_DATA }
};

static USER_MSG_TBL actionStatus [] =
{
   /* Str                 Next Tbl Ptr       Handler Func.      Data Type          Access   Parameter                                  IndexRange              DataLimit   EnumTbl*/
   { "ACTIVE_FLAGS",      NO_NEXT_TABLE,     Action_Flags,      USER_TYPE_128_LIST, USER_RO, &stateActionFlagsTemp.flagActive,          0,(MAX_ACTION_DEFINES-1), NO_LIMIT,    NULL },
   { "LATCH_FLAGS",       NO_NEXT_TABLE,     Action_Flags,      USER_TYPE_128_LIST, USER_RO, &stateActionFlagsTemp.flagLatch,           0,(MAX_ACTION_DEFINES-1), NO_LIMIT,    NULL },
   { "ACK_FLAGS",         NO_NEXT_TABLE,     Action_Flags,      USER_TYPE_128_LIST, USER_RO, &stateActionFlagsTemp.flagACK,             0,(MAX_ACTION_DEFINES-1), NO_LIMIT,    NULL },
   { "PERSIST_STATE",     NO_NEXT_TABLE,     Action_State,      USER_TYPE_BOOLEAN,  USER_RO, &stateActionTemp.persist.bState,           -1,-1,                    NO_LIMIT,    NULL },
   { "PERSIST_LATCH",     NO_NEXT_TABLE,     Action_State,      USER_TYPE_BOOLEAN,  USER_RO, &stateActionTemp.persist.bLatch,           -1,-1,                    NO_LIMIT,    NULL },
   { "PERSIST_ACTION_ID", NO_NEXT_TABLE,     Action_State,      USER_TYPE_UINT16,   USER_RO, &stateActionTemp.persist.actionNum,        -1,-1,                    NO_LIMIT,    NULL },
   { "PERSIST_USED",      NO_NEXT_TABLE,     Action_State,      USER_TYPE_UINT8,    USER_RO, &stateActionTemp.persist.action.nUsedMask, -1,-1,                    NO_LIMIT,    NULL },
   { "PERSIST_LSS",       NO_NEXT_TABLE,     Action_State,      USER_TYPE_UINT8,    USER_RO, &stateActionTemp.persist.action.nLSS_Mask, -1,-1,                    NO_LIMIT,    NULL },
   { "LSS0_ACTION_PRI",   NO_NEXT_TABLE,     Action_State,      USER_TYPE_UINT16,   USER_RO, &stateActionTemp.nLSS_Priority[0],         -1,-1,                    NO_LIMIT,    NULL },
   { "LSS1_ACTION_PRI",   NO_NEXT_TABLE,     Action_State,      USER_TYPE_UINT16,   USER_RO, &stateActionTemp.nLSS_Priority[1],         -1,-1,                    NO_LIMIT,    NULL },
   { "LSS2_ACTION_PRI",   NO_NEXT_TABLE,     Action_State,      USER_TYPE_UINT16,   USER_RO, &stateActionTemp.nLSS_Priority[2],         -1,-1,                    NO_LIMIT,    NULL },
   { "LSS3_ACTION_PRI",   NO_NEXT_TABLE,     Action_State,      USER_TYPE_UINT16,   USER_RO, &stateActionTemp.nLSS_Priority[3],         -1,-1,                    NO_LIMIT,    NULL },
   { NULL          ,      NO_NEXT_TABLE,     NULL,              USER_TYPE_NONE,     USER_RW, NULL,                                      -1,-1,                    NO_LIMIT,    NULL },
};

static USER_MSG_TBL actionRoot [] =
{ /* Str                     Next Tbl Ptr       Handler Func.      Data Type          Access            Parameter      IndexRange   DataLimit  EnumTbl*/
   { "CFG",                  actionCmd,         NULL,              NO_HANDLER_DATA},
   { "STATUS",               actionStatus,      NULL,              NO_HANDLER_DATA},
   { "CLEAR_LATCH",          NO_NEXT_TABLE,     Action_ClearLatch, USER_TYPE_ACTION,  USER_RO,          NULL,          -1, -1,      NO_LIMIT,  NULL},
   { DISPLAY_CFG,            NO_NEXT_TABLE,     Action_ShowConfig, USER_TYPE_ACTION,  USER_RO|USER_GSE, NULL,          -1, -1,      NO_LIMIT,  NULL},
   { NULL,                   NULL,              NULL,              NO_HANDLER_DATA}
};

static
USER_MSG_TBL    rootActionMsg = {"ACTION", actionRoot, NULL, NO_HANDLER_DATA};

#pragma ghs endnowarning

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
* Function:     Action_UserCfg
*
* Description:  Handles User Manager requests to change action configuration
*               items.
*
* Parameters:   [in] DataType:  C type of the data to be read or changed, used
*                               for casting the data pointers
*               [in/out] Param: Pointer to the configuration item to be read
*                               or changed
*               [in] Index:     Index parameter is used to reference the
*                               specific Event to change. Range is validated
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
USER_HANDLER_RESULT Action_UserCfg ( USER_DATA_TYPE DataType,
                                     USER_MSG_PARAM Param,
                                     UINT32 Index,
                                     const void *SetPtr,
                                     void **GetPtr)
{
   USER_HANDLER_RESULT result;

   result = USER_RESULT_OK;

   //Load Event Table structure into the temporary location based on index param
   //Param.Ptr points to the struct member to be read/written
   //in the temporary location
   memcpy(&configActionTemp,
          &CfgMgr_ConfigPtr()->ActionConfig,
          sizeof(configActionTemp));

   result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
   if(SetPtr != NULL && USER_RESULT_OK == result)
   {
     memcpy(&CfgMgr_ConfigPtr()->ActionConfig,
            &configActionTemp,
            sizeof(ACTION_CFG));
     //Store the modified temporary structure in the EEPROM.
     CfgMgr_StoreConfigItem( CfgMgr_ConfigPtr(),
                             &CfgMgr_ConfigPtr()->ActionConfig,
                             sizeof(configActionTemp));
   }
   return result;
}

/******************************************************************************
* Function:     ActionOut_UserCfg
*
* Description:  Handles User Manager requests to change action configuration
*               items.
*
* Parameters:   [in] DataType:  C type of the data to be read or changed, used
*                               for casting the data pointers
*               [in/out] Param: Pointer to the configuration item to be read
*                               or changed
*               [in] Index:     Index parameter is used to reference the
*                               specific Event to change. Range is validated
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
USER_HANDLER_RESULT ActionOut_UserCfg ( USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr)
{
   USER_HANDLER_RESULT result;

   result = USER_RESULT_OK;

   //Load Event Table structure into the temporary location based on index param
   //Param.Ptr points to the struct member to be read/written
   //in the temporary location
   memcpy(&configOutputTemp,
          &CfgMgr_ConfigPtr()->ActionConfig.output[Index],
          sizeof(configOutputTemp));

   result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
   if(SetPtr != NULL && USER_RESULT_OK == result)
   {
     memcpy(&CfgMgr_ConfigPtr()->ActionConfig.output[Index],
            &configOutputTemp,
            sizeof(configOutputTemp));
     //Store the modified temporary structure in the EEPROM.
     CfgMgr_StoreConfigItem( CfgMgr_ConfigPtr(),
                             &CfgMgr_ConfigPtr()->ActionConfig.output[Index],
                             sizeof(configOutputTemp));
   }
   return result;
}

/******************************************************************************
* Function:     Action_State
*
* Description:  Handles User Manager requests to retrieve the current Action
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

USER_HANDLER_RESULT Action_State(USER_DATA_TYPE DataType,
                                  USER_MSG_PARAM Param,
                                  UINT32 Index,
                                  const void *SetPtr,
                                  void **GetPtr)
{
   USER_HANDLER_RESULT result;

   result = USER_RESULT_ERROR;

   memcpy(&stateActionTemp, &m_ActionData, sizeof(stateActionTemp));

   result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

   return result;
}

/******************************************************************************
* Function:     Action_Flags
*
* Description:  Handles User Manager requests to retrieve the current Action
*               Flags.
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

USER_HANDLER_RESULT Action_Flags(USER_DATA_TYPE DataType,
                                  USER_MSG_PARAM Param,
                                  UINT32 Index,
                                  const void *SetPtr,
                                  void **GetPtr)
{
   USER_HANDLER_RESULT result;

   result = USER_RESULT_ERROR;

   memcpy(&stateActionFlagsTemp, &m_ActionData.action[Index], sizeof(stateActionFlagsTemp));

   result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

   return result;
}


/******************************************************************************
* Function:    Action_ShowConfig
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
USER_HANDLER_RESULT Action_ShowConfig ( USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr)
{
   CHAR  sLabelStem[] = "\r\n\r\nACTION.CFG";
   CHAR  sLabel[USER_MAX_MSG_STR_LEN * 3];
   INT16 i;

   USER_HANDLER_RESULT result = USER_RESULT_OK;
   USER_MSG_TBL*  pCfgTable;

   //Top-level name is a single indented space
   CHAR sBranchName[USER_MAX_MSG_STR_LEN] = " ";

   pCfgTable = &actionCmd[0];  // Get pointer to config entry

   for (i = 0; i < MAX_TABLES && result == USER_RESULT_OK; ++i)
   {
      // Display element info above each set of data.
      snprintf(sLabel, sizeof(sLabel),"%s[%d]", sLabelStem, i);

      result = USER_RESULT_ERROR;
      if ( User_OutputMsgString( sLabel, FALSE ) )
      {
         result = User_DisplayConfigTree(sBranchName, pCfgTable, i, 0, NULL);
      }
   }
   return result;
}

/******************************************************************************
 * Function:     Action_ClearLatch
 *
 * Description:  Reset the persistent latch for the action manager.
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
 * Returns:      OK:  successfully
 *
 * Notes:        None
 *****************************************************************************/
USER_HANDLER_RESULT Action_ClearLatch( USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_OK;

  ActionResetNVPersist();

  return result;
}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: ActionManagerUserTables.c $
 * 
 * *****************  Version 9  *****************
 * User: John Omalley Date: 12-11-12   Time: 6:47p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review Update
 * 
 * *****************  Version 8  *****************
 * User: John Omalley Date: 12-11-06   Time: 11:13a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review Updates
 *
 * *****************  Version 7  *****************
 * User: John Omalley Date: 12-10-30   Time: 5:48p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Changed Actions to UINT8
 *
 * *****************  Version 6  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 5  *****************
 * User: John Omalley Date: 12-08-24   Time: 9:30a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - ETM Fault Action Logic
 *
 * *****************  Version 4  *****************
 * User: John Omalley Date: 12-08-14   Time: 3:00p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review Updates and Action Status User Commands
 *
 * *****************  Version 3  *****************
 * User: John Omalley Date: 12-07-27   Time: 3:03p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Action Manager Persistent Updates
 *
 * *****************  Version 1  *****************
 * User: John Omalley Date: 12-07-17   Time: 11:32a
 * Created in $/software/control processor/code/system
 * SCR 1107
 *
 *
 ***************************************************************************/


