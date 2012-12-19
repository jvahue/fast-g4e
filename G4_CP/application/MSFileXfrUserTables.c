#define MSFX_USERTABLES_BODY
/******************************************************************************
            Copyright (C) 2009-2010 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:        MSFileXfrUserTables.c

    Description: Routines for retrieving files from the Micro-Server to
                 the CP GSE port.

    VERSION
    $Revision: 17 $  $Date: 12/18/12 4:58p $

******************************************************************************/

#ifndef MSFX_BODY
#error MSFileXfrUserTables.c should only be included by MSFileXfr.c
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
#define MSFX_MAX_FN_STR MSCP_PATH_FN_STR_SIZE
/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
// See Local Variables below Function Prototypes Section

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
//Prototypes for the User Manager message handlers, has to go before
//the local variable tables that use the function pointer.

static USER_HANDLER_RESULT MSFX_GetListUserCmd(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr);
static USER_HANDLER_RESULT MSFX_GetFileUserCmd(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr);
static USER_HANDLER_RESULT MSFX_PutFileUserCmd(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr);
static USER_HANDLER_RESULT MSFX_GetStsUserCmd(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr);
static USER_HANDLER_RESULT MSFX_MarkFileTxdCmd(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr);

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static USER_ENUM_TBL msfx_state_enum_tbl[] =
{
  { "Stopped",           TASK_STOPPED                       },
  { "StopSend",          TASK_STOP_SEND                     },
  { m_FileXfrData.Err,   TASK_ERROR                         },
  { "WaitForMS",         TASK_WAIT                          },
  { "ReqNonTXDList",     TASK_REQUEST_NON_TXD_FILE_LIST     },
  { "ReqTXDList",        TASK_REQUEST_TXD_FILE_LIST         },
  { "ReqNonTXDList",     TASK_REQPRI4_NON_TXD_FILE_LIST     },
  { "ReqTXDList",        TASK_REQPRI4_TXD_FILE_LIST         },
  { "ReqFile",           TASK_REQUEST_START_FILE            },
  { "MarkTXD",           TASK_MARK_FILE_TXD                 },
  { "YMDMWtStart",       TASK_YMDM_SNDR_WAIT_FOR_START      },
  { "YMdmSndrWtACK",     TASK_YMDM_SNDR_WAIT_FOR_ACKNAK     },
  { "YMdmSndrSend",      TASK_YMDM_SNDR_SEND                },
  { "YMdmSndrGetNext",   TASK_YMDM_SNDR_GET_NEXT            },
  { "YMdmSndrSendEOT",   TASK_YMDM_SNDR_SEND_EOT            },
  { "YMdmSndrWtEOTACK",  TASK_YMDM_SNDR_WAIT_EOT_ACKNAK     },
  { "YMdmRcvrSendStart", TASK_YMDM_RCVR_SEND_START_CHAR,    },
  { "YMdmRcvrSendAck",   TASK_YMDM_RCVR_SEND_ACK,           },
  { "YMdmRcvrSendNak",   TASK_YMDM_RCVR_SEND_NAK,           },
  { "YMdmRcvrWtForSndr", TASK_YMDM_RCVR_WAIT_FOR_SNDR       },
  {NULL,0}

};

static USER_MSG_TBL msfx_msgs_tbl[] =
{/*Str                 Next Tbl Ptr   Handler Func.         Data Type         Access                 Parameter                             IndexRange DataLimit           EnumTbl*/
  {"STATUS",             NO_NEXT_TABLE, MSFX_GetStsUserCmd, USER_TYPE_ENUM,   USER_RO,              NULL,                                  -1, -1,    NO_LIMIT,           msfx_state_enum_tbl},
  {"GET_TXD_LIST",       NO_NEXT_TABLE, MSFX_GetListUserCmd,USER_TYPE_ACTION, (USER_RO|USER_GSE),  (void*)TASK_REQUEST_TXD_FILE_LIST,      -1, -1,    NO_LIMIT,           NULL},
  {"GET_TXD_PRI4_LIST",  NO_NEXT_TABLE, MSFX_GetListUserCmd,USER_TYPE_ACTION, (USER_RO|USER_GSE),  (void*)TASK_REQPRI4_TXD_FILE_LIST,      -1, -1,    NO_LIMIT,           NULL},
  {"GET_UNTXD_LIST",     NO_NEXT_TABLE, MSFX_GetListUserCmd,USER_TYPE_ACTION, (USER_RO|USER_GSE),  (void*)TASK_REQUEST_NON_TXD_FILE_LIST,  -1, -1,    NO_LIMIT,           NULL},
  {"GET_UNTXD_PRI4_LIST",NO_NEXT_TABLE, MSFX_GetListUserCmd,USER_TYPE_ACTION, (USER_RO|USER_GSE),  (void*)TASK_REQPRI4_NON_TXD_FILE_LIST,  -1, -1,    NO_LIMIT,           NULL},
  {"MARK_FILE_TXD",      NO_NEXT_TABLE, MSFX_MarkFileTxdCmd,USER_TYPE_STR,    (USER_WO|USER_GSE),   NULL,                                  -1, -1,    1, MSFX_MAX_FN_STR, NULL},
  {"GET_FILE",           NO_NEXT_TABLE, MSFX_GetFileUserCmd,USER_TYPE_STR,    (USER_WO|USER_GSE),   NULL,                                  -1, -1,    1, MSFX_MAX_FN_STR, NULL},
  {"PUT_FILE",           NO_NEXT_TABLE, MSFX_PutFileUserCmd,USER_TYPE_STR,    (USER_WO|USER_GSE),   NULL,                                  -1, -1,    1, MSFX_MAX_FN_STR, NULL},
  {NULL,                 NULL,          NULL,                 NO_HANDLER_DATA}
};

static USER_MSG_TBL root_user_table   = {"MSFX",msfx_msgs_tbl,NULL,NO_HANDLER_DATA};

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:    MSFX_MarkFileTxdCmd()
 *
 * Description: Process the user command to get a list of transmitted or
 *              non-transmitted log files stored on the Micro-Server
 *              Compact Flash
 *
 * Parameters:  [in] DataType:  C type of the data to be read or changed, used
 *                               for casting the data pointers
 *              [in/out] Param: Pointer to the configuration item to be read
 *                              or changed
 *              [in] Index:     Index parameter is used to reference the
 *                              specific sensor to change.  Range is validated
 *                              by the user manager
 *              [in] SetPtr:    For write commands, a pointer to the data to
 *                              write to the configuration.
 *              [out] GetPtr:   For read commands, UserCfg function will set
 *                              this to the location of the data requested.
 *
 * Returns:     USER_RESULT_OK: started successfully
 *              USER_RESULT_ERROR: Task did not start.
 *
 * Notes:
 *
 *****************************************************************************/
static USER_HANDLER_RESULT MSFX_MarkFileTxdCmd(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr)
{
 USER_HANDLER_RESULT retval = USER_RESULT_ERROR;

  if(MSFX_MarkFileAsTransmitted(SetPtr))
  {
    retval = USER_RESULT_OK;
  }

  return retval;
}

/******************************************************************************
 * Function:    MSFX_GetListUserCmd()
 *
 * Description: Process the user command to get a list of transmitted or
 *              non-transmitted log files stored on the Micro-Server
 *              Compact Flash
 *
 * Parameters:  [in] DataType:  C type of the data to be read or changed, used
 *                               for casting the data pointers
 *              [in/out] Param: Pointer to the configuration item to be read
 *                              or changed
 *              [in] Index:     Index parameter is used to reference the
 *                              specific sensor to change.  Range is validated
 *                              by the user manager
 *              [in] SetPtr:    For write commands, a pointer to the data to
 *                              write to the configuration.
 *              [out] GetPtr:   For read commands, UserCfg function will set
 *                              this to the location of the data requested.
 *
 * Returns:     USER_RESULT_OK: started successfully
 *              USER_RESULT_ERROR: Task did not start.
 *
 * Notes:
 *
 *****************************************************************************/
static USER_HANDLER_RESULT MSFX_GetListUserCmd(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr)
{
  USER_HANDLER_RESULT retval = USER_RESULT_ERROR;

  if( MSFX_RequestFileList((MSFX_TASK_STATE)Param.Int))
  {
    retval = USER_RESULT_OK;
  }

  return retval;
}

/******************************************************************************
 * Function:    MSFX_GetFileUserCmd()
 *
 * Description: Process the user command to get a file stored on the
 *              Micro-Server.  This includes, but is not limited to transmitted
 *              and non-transmitted log files.
 *
 * Parameters:  [in] DataType:  C type of the data to be read or changed, used
 *                               for casting the data pointers
 *              [in/out] Param: Pointer to the configuration item to be read
 *                              or changed
 *              [in] Index:     Index parameter is used to reference the
 *                              specific sensor to change.  Range is validated
 *                              by the user manager
 *              [in] SetPtr:    For write commands, a pointer to the data to
 *                              write to the configuration.
 *              [out] GetPtr:   For read commands, UserCfg function will set
 *                              this to the location of the data requested.
 *
 * Returns:     USER_RESULT_OK: started successfully
 *              USER_RESULT_ERROR: Task did not start.
 *
 * Notes:
 *
 *****************************************************************************/
static USER_HANDLER_RESULT MSFX_GetFileUserCmd(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr)
{
  USER_HANDLER_RESULT retval = USER_RESULT_ERROR;

  if(MSFX_RequestGetFile(SetPtr))
  {
    retval = USER_RESULT_OK;
  }

  return retval;
}

/******************************************************************************
 * Function:    MSFX_PutFileUserCmd()
 *
 * Description: Process the user command to store on the
 *              Micro-Server.  This includes, but is not limited to transmitted
 *              and non-transmitted log files.
 *
 * Parameters:  [in] DataType:  C type of the data to be read or changed, used
 *                               for casting the data pointers
 *              [in/out] Param: Pointer to the configuration item to be read
 *                              or changed
 *              [in] Index:     Index parameter is used to reference the
 *                              specific sensor to change.  Range is validated
 *                              by the user manager
 *              [in] SetPtr:    For write commands, a pointer to the data to
 *                              write to the configuration.
 *              [out] GetPtr:   For read commands, UserCfg function will set
 *                              this to the location of the data requested.
 *
 * Returns:     USER_RESULT_OK: started successfully
 *              USER_RESULT_ERROR: Task did not start.
 *
 * Notes:
 *
 *****************************************************************************/
static USER_HANDLER_RESULT MSFX_PutFileUserCmd(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr)
{
  USER_HANDLER_RESULT retval = USER_RESULT_ERROR;

  if(MSFX_RequestPutFile(SetPtr))
  {
    retval = USER_RESULT_OK;
  }

  return retval;
}

/******************************************************************************
 * Function:    MSFX_GetStsUserCmd()
 *
 * Description: Process the user command to get the current activity of the
 *              file transfer task.
 *
 * Parameters:  [in] DataType:  C type of the data to be read or changed, used
 *                               for casting the data pointers
 *              [in/out] Param: Pointer to the configuration item to be read
 *                              or changed
 *              [in] Index:     Index parameter is used to reference the
 *                              specific sensor to change.  Range is validated
 *                              by the user manager
 *              [in] SetPtr:    For write commands, a pointer to the data to
 *                              write to the configuration.
 *              [out] GetPtr:   For read commands, UserCfg function will set
 *                              this to the location of the data requested.
 *
 * Returns:     USER_RESULT_OK: started successfully
 *              USER_RESULT_ERROR: Task did not start.
 *
 * Notes:
 *
 *****************************************************************************/
static USER_HANDLER_RESULT MSFX_GetStsUserCmd(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr)
{

  **(UINT32**)GetPtr = (UINT32)m_TaskState;

  return USER_RESULT_OK;
}

/*****************************************************************************
 *  MODIFICATIONS
 *    $History: MSFileXfrUserTables.c $
 * 
 * *****************  Version 17  *****************
 * User: Jim Mood     Date: 12/18/12   Time: 4:58p
 * Updated in $/software/control processor/code/application
 * SCR# 1197 Code Review
 *
 * *****************  Version 16  *****************
 * User: Melanie Jutras Date: 12-11-15   Time: 2:24p
 * Updated in $/software/control processor/code/application
 * SCR #1142 File Format Errors
 *
 * *****************  Version 15  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 12:43p
 * Updated in $/software/control processor/code/application
 * SCR# 1142
 *
 * *****************  Version 14  *****************
 * User: Jim Mood     Date: 8/27/12    Time: 6:03p
 * Updated in $/software/control processor/code/application
 * SCR 1107 Caravan Requirement updates
 *
 * *****************  Version 13  *****************
 * User: Jim Mood     Date: 7/19/12    Time: 10:41a
 * Updated in $/software/control processor/code/application
 * SCR 1107: Data Offload changes for 2.0.0
 *
 * *****************  Version 12  *****************
 * User: John Omalley Date: 10/08/10   Time: 10:46a
 * Updated in $/software/control processor/code/application
 * SCR 924 - Code Review Updates
 *
 * *****************  Version 11  *****************
 * User: John Omalley Date: 9/09/10    Time: 2:16p
 * Updated in $/software/control processor/code/application
 * SCR 855 - Removed Unused function
 *
 * *****************  Version 10  *****************
 * User: Jeff Vahue   Date: 8/28/10    Time: 12:21p
 * Updated in $/software/control processor/code/application
 * SCR# 830 - if/then/else modifications
 *
 * *****************  Version 9  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:01a
 * Updated in $/software/control processor/code/application
 * SCR #685 - Add USER_GSE flag
 *
 * *****************  Version 8  *****************
 * User: Jim Mood     Date: 6/11/10    Time: 10:06a
 * Updated in $/software/control processor/code/application
 * SCR 623 Batch mode.  Implements YMODEM  receiver
 *
 * *****************  Version 7  *****************
 * User: Contractor V&v Date: 3/19/10    Time: 4:28p
 * Updated in $/software/control processor/code/application
 * SCR #248 Parameter Log Change
 *
 * *****************  Version 6  *****************
 * User: Contractor V&v Date: 3/10/10    Time: 4:39p
 * Updated in $/software/control processor/code/application
 * SCR #437 SCR #248 Parameter Log Change
 *
 * *****************  Version 5  *****************
 * User: Jeff Vahue   Date: 3/05/10    Time: 12:15p
 * Updated in $/software/control processor/code/application
 * SCR# 413 - Actions cmds are RO
 *
 * *****************  Version 4  *****************
 * User: Contractor2  Date: 3/02/10    Time: 3:40p
 * Updated in $/software/control processor/code/application
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 3  *****************
 * User: Jim Mood     Date: 2/24/10    Time: 10:49a
 * Updated in $/software/control processor/code/application
 *
 * *****************  Version 2  *****************
 * User: Jim Mood     Date: 1/20/10    Time: 3:45p
 * Updated in $/software/control processor/code/application
 * Initial completion
 *
 * *****************  Version 1  *****************
 * User: Contractor V&v Date: 12/10/09   Time: 5:35p
 * Created in $/software/control processor/code/application
 * SCR 42 and SCR 106
 *
 *****************************************************************************/
