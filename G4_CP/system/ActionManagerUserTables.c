/******************************************************************************
         Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
            All Rights Reserved. Proprietary and Confidential.

  File:        ActionManagerUserTables.c

Description:   User command structures and functions for the sction processing

VERSION
$Revision: 1 $  $Date: 12-07-17 11:32a $
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
/* Local Function Prototypes                                                 */
/*****************************************************************************/
//Prototype for the User Manager message handlers, has to go before
//the local variable tables that use the function pointer.
USER_HANDLER_RESULT Action_UserCfg    ( USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr );
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
/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static ACTION_CFG    ConfigActionTemp; // Action temp Storage
static ACTION_OUTPUT ConfigOutputTemp;
static ACTION_DATA   StateActionTemp;  // Action State Temp Storage

#pragma ghs nowarning 1545 //Suppress packed structure alignment warning

static USER_MSG_TBL ActionOutCmd [] =
{
   /* Str                 Next Tbl Ptr       Handler Func.      Data Type          Access   Parameter                                  IndexRange                  DataLimit   EnumTbl*/
   { "USED_MASK",         NO_NEXT_TABLE,     ActionOut_UserCfg, USER_TYPE_UINT8,   USER_RW, &ConfigOutputTemp.UsedMask,                0,(MAX_ACTION_DEFINES-1),   NO_LIMIT,   NULL },
   { "LSS_MASK",          NO_NEXT_TABLE,     ActionOut_UserCfg, USER_TYPE_UINT8,   USER_RW, &ConfigOutputTemp.LSS_Mask,                0,(MAX_ACTION_DEFINES-1),   NO_LIMIT,   NULL },
   { NULL,                NULL,              NULL,              NO_HANDLER_DATA }
};

static USER_MSG_TBL ActionCmd [] =
{
   /* Str                 Next Tbl Ptr       Handler Func.      Data Type          Access   Parameter                                  IndexRange              DataLimit   EnumTbl*/
   { "ACK_TRIGGER",       NO_NEXT_TABLE,     Action_UserCfg,    USER_TYPE_ENUM,    USER_RW, &ConfigActionTemp.ACKTrigger,              -1,-1,                  NO_LIMIT,   TriggerIndexType },
   { "PERSIST_ENABLE",    NO_NEXT_TABLE,     Action_UserCfg,    USER_TYPE_BOOLEAN, USER_RW, &ConfigActionTemp.Persist.bEnabled,        -1,-1,                  NO_LIMIT,   NULL },
   { "PERSIST_USED_MASK", NO_NEXT_TABLE,     Action_UserCfg,    USER_TYPE_UINT8,   USER_RW, &ConfigActionTemp.Persist.Output.UsedMask, -1,-1,                  NO_LIMIT,   NULL },
   { "PERSIST_LSS_MASK",  NO_NEXT_TABLE,     Action_UserCfg,    USER_TYPE_UINT8,   USER_RW, &ConfigActionTemp.Persist.Output.LSS_Mask, -1,-1,                  NO_LIMIT,   NULL },
   { "OUTPUT",            ActionOutCmd,      NULL,              NO_HANDLER_DATA },
   { NULL,                NULL,              NULL,              NO_HANDLER_DATA }
};

static USER_MSG_TBL ActionStatus [] =
{
   /* Str                 Next Tbl Ptr       Handler Func.      Data Type          Access   Parameter                                  IndexRange              DataLimit   EnumTbl*/
   { NULL          ,      NO_NEXT_TABLE,     NULL,              USER_TYPE_NONE,    USER_RW, NULL,                                      0,(MAX_TABLES-1),       NO_LIMIT,   NULL },
};

static USER_MSG_TBL ActionRoot [] =
{ /* Str                  Next Tbl Ptr       Handler Func.      Data Type          Access            Parameter      IndexRange   DataLimit  EnumTbl*/
   { "CFG",               ActionCmd,         NULL,              NO_HANDLER_DATA},
   { "STATUS",            ActionStatus,      NULL,              NO_HANDLER_DATA},
   { DISPLAY_CFG,         NO_NEXT_TABLE,     Action_ShowConfig, USER_TYPE_ACTION,  USER_RO|USER_GSE, NULL,          -1, -1,      NO_LIMIT,  NULL},
   { NULL,                NULL,              NULL,              NO_HANDLER_DATA}
};

static
USER_MSG_TBL    RootActionMsg = {"ACTION", ActionRoot, NULL, NO_HANDLER_DATA};

#pragma ghs endnowarning

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
   memcpy(&ConfigActionTemp,
          &CfgMgr_ConfigPtr()->ActionConfig,
          sizeof(ConfigActionTemp));

   result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
   if(SetPtr != NULL && USER_RESULT_OK == result)
   {
     memcpy(&CfgMgr_ConfigPtr()->ActionConfig,
            &ConfigActionTemp,
            sizeof(ACTION_CFG));
     //Store the modified temporary structure in the EEPROM.
     CfgMgr_StoreConfigItem( CfgMgr_ConfigPtr(),
                             &CfgMgr_ConfigPtr()->ActionConfig,
                             sizeof(ConfigActionTemp));
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
   memcpy(&ConfigOutputTemp,
          &CfgMgr_ConfigPtr()->ActionConfig.Output[Index],
          sizeof(ConfigOutputTemp));

   result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
   if(SetPtr != NULL && USER_RESULT_OK == result)
   {
     memcpy(&CfgMgr_ConfigPtr()->ActionConfig.Output[Index],
            &ConfigOutputTemp,
            sizeof(ConfigOutputTemp));
     //Store the modified temporary structure in the EEPROM.
     CfgMgr_StoreConfigItem( CfgMgr_ConfigPtr(),
                             &CfgMgr_ConfigPtr()->ActionConfig.Output[Index],
                             sizeof(ConfigOutputTemp));
   }
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
   CHAR  LabelStem[] = "\r\n\r\nACTION.CFG";
   CHAR  Label[USER_MAX_MSG_STR_LEN * 3];
   INT16 i;

   USER_HANDLER_RESULT result = USER_RESULT_OK;
   USER_MSG_TBL*  pCfgTable;

   //Top-level name is a single indented space
   CHAR BranchName[USER_MAX_MSG_STR_LEN] = " ";

   pCfgTable = &ActionCmd[0];  // Get pointer to config entry

   for (i = 0; i < MAX_TABLES && result == USER_RESULT_OK; ++i)
   {
      // Display element info above each set of data.
      sprintf(Label, "%s[%d]", LabelStem, i);

      result = USER_RESULT_ERROR;
      if ( User_OutputMsgString( Label, FALSE ) )
      {
         result = User_DisplayConfigTree(BranchName, pCfgTable, i, 0, NULL);
      }
   }
   return result;
}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: ActionManagerUserTables.c $
 * 
 * *****************  Version 1  *****************
 * User: John Omalley Date: 12-07-17   Time: 11:32a
 * Created in $/software/control processor/code/system
 * SCR 1107
 *
 *
 **************************************************************************/


