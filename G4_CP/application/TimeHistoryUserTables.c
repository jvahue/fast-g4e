#define TIMEHISTORY_USERTABLES_BODY
/******************************************************************************
         Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
            All Rights Reserved. Proprietary and Confidential.

  File:        TimeHistoryUserTables.c

Description:   User command structures and functions for the event processing

VERSION
$Revision: 9 $  $Date: 12/05/14 4:31p $
******************************************************************************/
#ifndef TIMEHISTORY_BODY
#error TimeHistoryUserTables.c should only be included by TimeHistory.c
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
static TIMEHISTORY_CONFIG   cfg_temp;// Configuration temp Storage

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
//Prototype for the User Manager message handlers, has to go before
//the local variable tables that use the function pointer.
static USER_HANDLER_RESULT TH_UserCfg ( USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr );
static USER_HANDLER_RESULT TH_FOpen  ( USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr );
static USER_HANDLER_RESULT TH_FClose  ( USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr );
static USER_HANDLER_RESULT TH_ShowConfig ( USER_DATA_TYPE DataType,
                                             USER_MSG_PARAM Param,
                                             UINT32 Index,
                                             const void *SetPtr,
                                             void **GetPtr );
/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static USER_ENUM_TBL time_history_rate_type[]   =  {  { "OFF"  , TH_OFF          },
                                                      { "1HZ"  , TH_1HZ          },
                                                      { "2HZ"  , TH_2HZ          },
                                                      { "4HZ"  , TH_4HZ          },
                                                      { "5HZ"  , TH_5HZ          },
                                                      { "10HZ" , TH_10HZ         },
                                                      { "20HZ" , TH_20HZ         },
                                                      { NULL , 0               }
                                                   };

// Events - EVENT User and Configuration Table
static USER_MSG_TBL time_history_cfg [] =
{
  /* Str              Next Tbl Ptr    Handler Func.  Data Type          Access     Parameter                               IndexRange  DataLimit EnumTbl*/
  { "RATE",           NO_NEXT_TABLE,  TH_UserCfg,    USER_TYPE_ENUM,    USER_RW,   &cfg_temp.sample_rate,       -1,-1,      NO_LIMIT, time_history_rate_type },
  { "RATEOFFSET_MS",  NO_NEXT_TABLE,  TH_UserCfg,    USER_TYPE_UINT32,  USER_RW,   &cfg_temp.sample_offset_ms,  -1,-1,      0,1000,   NULL                },
  { NULL,             NULL,           NULL,          NO_HANDLER_DATA }
};

static USER_MSG_TBL time_history_status [] =
{
  /* Str            Next Tbl Ptr     Handler Func.            Data Type          Access     Parameter                 IndexRange DataLimit            EnumTbl*/
  { "WRITE_REQ",    NO_NEXT_TABLE,   User_GenericAccessor,    USER_TYPE_UINT32,  USER_RO,   &m_THDataBuf.write_cnt,   -1,-1,     NO_LIMIT,            NULL},
  { "WRITTEN",      NO_NEXT_TABLE,   User_GenericAccessor,    USER_TYPE_UINT32,  USER_RO,   &m_WrittenCnt,            -1,-1,     NO_LIMIT,            NULL},
  { NULL,           NULL,            NULL,                    NO_HANDLER_DATA}
};

static USER_MSG_TBL time_history_debug [] =
{
  /* Str            Next Tbl Ptr         Handler Func.  Data Type               Access     Parameter      IndexRange   DataLimit             EnumTbl*/
   { "OPEN",        NO_NEXT_TABLE,       TH_FOpen,      USER_TYPE_INT32,        USER_WO,   NULL,          -1, -1,      0,TH_PRE_HISTORY_S,   NULL},
   { "CLOSE",       NO_NEXT_TABLE,       TH_FClose,     USER_TYPE_INT32,        USER_WO,   NULL,          -1, -1,      0,TH_POST_HISTORY_S,  NULL},
   { NULL,          NULL,                NULL,          NO_HANDLER_DATA}
};

static USER_MSG_TBL time_history_root [] =
{ /* Str            Next Tbl Ptr         Handler Func.  Data Type          Access            Parameter      IndexRange   DataLimit  EnumTbl*/
   { "CFG",         time_history_cfg,    NULL,          NO_HANDLER_DATA},
   { "STATUS",      time_history_status, NULL,          NO_HANDLER_DATA},
   { "DEBUG",       time_history_debug,  NULL,          NO_HANDLER_DATA},
   { DISPLAY_CFG,   NO_NEXT_TABLE,       TH_ShowConfig, USER_TYPE_ACTION,  USER_RO|USER_GSE|USER_NO_LOG, NULL,          -1, -1,      NO_LIMIT,  NULL},
   { NULL,          NULL,                NULL,          NO_HANDLER_DATA}
};


/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
* Function:     TH_UserCfg
*
* Description:  Handles User Manager requests to change event configuration
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
static USER_HANDLER_RESULT TH_UserCfg ( USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr)
{
   USER_HANDLER_RESULT result;

   result = USER_RESULT_OK;

   //Load Event structure into the temporary location based on index param
   //Param.Ptr points to the struct member to be read/written
   //in the temporary location
   memcpy ( &cfg_temp,
            &CfgMgr_ConfigPtr()->TimeHistoryConfig,
            sizeof(cfg_temp));

   result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
   if(SetPtr != NULL && USER_RESULT_OK == result)
   {
     memcpy ( &CfgMgr_ConfigPtr()->TimeHistoryConfig,
              &cfg_temp,
              sizeof(TIMEHISTORY_CONFIG));
     //Store the modified temporary structure in the EEPROM.
     CfgMgr_StoreConfigItem ( CfgMgr_ConfigPtr(),
                              &CfgMgr_ConfigPtr()->TimeHistoryConfig,
                              sizeof(cfg_temp));
   }
   return result;
}


/******************************************************************************
* Function:     TH_ShowConfig
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
static USER_HANDLER_RESULT TH_ShowConfig ( USER_DATA_TYPE DataType,
                                             USER_MSG_PARAM Param,
                                             UINT32 Index,
                                             const void *SetPtr,
                                             void **GetPtr )
{
   USER_HANDLER_RESULT result = USER_RESULT_OK;

   //Top-level name is a single indented space
   CHAR branch_name[USER_MAX_MSG_STR_LEN] = " ";


   result = USER_RESULT_ERROR;
   if (User_OutputMsgString( "\r\n\r\nTH.CFG", FALSE ) )
   {
      result = User_DisplayConfigTree(branch_name, time_history_cfg, 0, 0, NULL);
   }

   return result;
}


/******************************************************************************
* Function:     TH_FOpen
*
* Description:  Command handler to force and open call to time history.
*               The INT32 set value specifies the duration of pre-history
*               in seconds
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
*              USER_RESULT_ERROR: Error processing command. (not used)
*
* Notes:
*****************************************************************************/
static USER_HANDLER_RESULT TH_FOpen  ( USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr )
{
   USER_HANDLER_RESULT result = USER_RESULT_OK;
   if(SetPtr != NULL)
   {
     TH_Open(*(UINT32*)SetPtr);
   }
   return result;
}

/******************************************************************************
* Function:     TH_FClose
*
* Description:  Command handler to force a close call to time history.  Set
*               value specifies the duration for post-history to record up
*               to 360 seconds.
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
static USER_HANDLER_RESULT TH_FClose  ( USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr )
{

  USER_HANDLER_RESULT result = USER_RESULT_OK;
  if(SetPtr != NULL)
  {
    TH_Close(*(UINT32*)SetPtr);
  }
  return result;
}




/*************************************************************************
 *  MODIFICATIONS
 *    $History: TimeHistoryUserTables.c $
 * 
 * *****************  Version 9  *****************
 * User: John Omalley Date: 12/05/14   Time: 4:31p
 * Updated in $/software/control processor/code/application
 * SCR 1267 - Update showcfg access type
 * 
 * *****************  Version 8  *****************
 * User: Jim Mood     Date: 1/17/13    Time: 2:44p
 * Updated in $/software/control processor/code/application
 * SCR #1218
 *
 * *****************  Version 7  *****************
 * User: Jim Mood     Date: 12/18/12   Time: 7:47p
 * Updated in $/software/control processor/code/application
 * SCR #1197 Code Review Updates
 *
 * *****************  Version 6  *****************
 * User: Jim Mood     Date: 11/29/12   Time: 3:59p
 * Updated in $/software/control processor/code/application
 * SCR #1107
 *
 * *****************  Version 5  *****************
 * User: Jim Mood     Date: 11/06/12   Time: 11:49a
 * Updated in $/software/control processor/code/application
 * SCR 1107
 *
 * *****************  Version 4  *****************
 * User: Jim Mood     Date: 9/27/12    Time: 4:51p
 * Updated in $/software/control processor/code/application
 * SCR 1107
 *
 * *****************  Version 3  *****************
 * User: Jim Mood     Date: 9/21/12    Time: 5:27p
 * Updated in $/software/control processor/code/application
 * SCR 1107: 2.0.0 Requirements - Time History
 *
 * *****************  Version 2  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 12:43p
 * Updated in $/software/control processor/code/application
 * SCR# 1142
 *
 * *****************  Version 1  *****************
 * User: John Omalley Date: 4/24/12    Time: 10:59a
 * Created in $/software/control processor/code/application
 *
 *
 *
 ***************************************************************************/


