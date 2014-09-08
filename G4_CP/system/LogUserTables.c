#define LOG_USERTABLES_BODY
/******************************************************************************
            Copyright (C) 2008-2014 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:        LogUserTables.c

    Description:

*  VERSION
 *    $Revision: 22 $  $Date: 9/03/14 5:19p $
******************************************************************************/
#ifndef LOGMNG_BODY
#error LogUserTables.c should only be included by LogManager.c
#endif

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "cfgmanager.h"

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
USER_HANDLER_RESULT Log_GetStatus (USER_DATA_TYPE DataType,
                                   USER_MSG_PARAM Param,
                                   UINT32 Index,
                                   const void *SetPtr,
                                   void **GetPtr);

USER_HANDLER_RESULT Log_EraseBlock(USER_DATA_TYPE DataType,
                                   USER_MSG_PARAM Param,
                                   UINT32 Index,
                                   const void *SetPtr,
                                   void **GetPtr);

// ~~~~~~~~~~~~~~~~~~~~Log Commands ~~~~~~~~~~~~~~

static USER_MSG_TBL LogQueueCmd[] =
{ /*  Str    Next Tbl Ptr   Handler Func.  Data Type        Access    Parameter       IndexRange   DataLimit  EnumTbl*/
   { "HEAD", NO_NEXT_TABLE, Log_GetStatus, USER_TYPE_HEX32, USER_RO,  &logQueue.head, -1, -1,      NO_LIMIT,   NULL },
   { "TAIL", NO_NEXT_TABLE, Log_GetStatus, USER_TYPE_HEX32, USER_RO,  &logQueue.tail, -1, -1,      NO_LIMIT,   NULL },
   { NULL  , NULL,          NULL,          NO_HANDLER_DATA }
};

static USER_MSG_TBL LogStatusCmd[] =
{/*  Str             Next Tbl Ptr   Handler Func.    Data Type       Access    Parameter                 IndexRange   DataLimit  EnumTbl*/
  { "PERCENTUSED",   NO_NEXT_TABLE, Log_GetStatus, USER_TYPE_FLOAT,  USER_RO,  &logConfig.fPercentUsage, -1, -1,      NO_LIMIT,  NULL },
  { "WRITEOFFSET",   NO_NEXT_TABLE, Log_GetStatus, USER_TYPE_HEX32,  USER_RO,  &logConfig.nWrOffset,     -1, -1,      NO_LIMIT,  NULL },
  { "STARTOFFSET",   NO_NEXT_TABLE, Log_GetStatus, USER_TYPE_HEX32,  USER_RO,  &logConfig.nStartOffset,  -1, -1,      NO_LIMIT,  NULL },
  { "ENDOFFSET",     NO_NEXT_TABLE, Log_GetStatus, USER_TYPE_HEX32,  USER_RO,  &logConfig.nEndOffset,    -1, -1,      NO_LIMIT,  NULL },
  { "LOGCOUNT",      NO_NEXT_TABLE, Log_GetStatus, USER_TYPE_UINT32, USER_RO,  &logConfig.nLogCount,     -1, -1,      NO_LIMIT,  NULL },
  { "LOGDELETED",    NO_NEXT_TABLE, Log_GetStatus, USER_TYPE_UINT32, USER_RO,  &logConfig.nDeleted,      -1, -1,      NO_LIMIT,  NULL },
  { "CORRUPT",       NO_NEXT_TABLE, Log_GetStatus, USER_TYPE_UINT32, USER_RO,  &logConfig.nCorrupt,      -1, -1,      NO_LIMIT,  NULL },
  { NULL,            NULL,          NULL,          NO_HANDLER_DATA }
};

static USER_MSG_TBL LogRoot [] =
{/*  Str            Next Tbl Ptr   Handler Func.    Data Type       Access                            Parameter     IndexRange   DataLimit  EnumTbl*/
   { "QUEUE" ,      LogQueueCmd,   NULL,            NO_HANDLER_DATA },
   { "STATUS",      LogStatusCmd,  NULL,            NO_HANDLER_DATA },
   { "ERASE",       NO_NEXT_TABLE, Log_EraseBlock,  USER_TYPE_STR    ,USER_WO|USER_PRIV|USER_NO_LOG,  NULL,        -1, -1,      NO_LIMIT,  NULL},
   { NULL, NULL, NULL, NO_HANDLER_DATA}
};



static USER_MSG_TBL RootLogMsg = {"LOG",LogRoot,NULL,NO_HANDLER_DATA};


/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:     Log_GetStatus
 *
 * Description:  Handles User Manager requests to retrieve the log
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
 * Returns:      USER_HANDLER_RESULT - USER_RESULT_ERROR or USER_RESULT_OK
 *
 * Notes:
 *****************************************************************************/
USER_HANDLER_RESULT Log_GetStatus (USER_DATA_TYPE DataType,
                                   USER_MSG_PARAM Param,
                                   UINT32 Index,
                                   const void *SetPtr,
                                   void **GetPtr)
{
   USER_HANDLER_RESULT result = USER_RESULT_ERROR;

   if(SetPtr == NULL)
   {
      //Set the GetPtr to the Cfg member referenced in the table
      *GetPtr = Param.Ptr;
      result = USER_RESULT_OK;
   }

   return result;
}

/******************************************************************************
 * Function:     Log_EraseBlock()
 *
 * Description:  Handles requests to erase a block from the log.
 *
 * Parameters:   USER_DATA_TYPE DataType
 *               USER_MSG_PARAM Param
 *               UINT32         Index
 *               const void     *SetPtr
 *               void           **GetPtr
 *
 * Returns:      USER_HANDLER_RESULT - USER_RESULT_ERROR or USER_RESULT_OK
 *
 * Notes:
 *
 *****************************************************************************/
USER_HANDLER_RESULT Log_EraseBlock(USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr)
{
   USER_HANDLER_RESULT result = USER_RESULT_ERROR;
   LOG_REQUEST newReq;

   if(strncmp(SetPtr,"really",sizeof("really")) == 0)
   {
      if ( LogIsEraseInProgress() )
      {
         GSE_DebugStr(NORMAL,TRUE, "LogEraseCmd: Erase Already in Progress"NL);
      }
      else if (logConfig.nLogCount == 0)
      {
         GSE_DebugStr(NORMAL,TRUE, "LogEraseCmd: No Logs to Erase"NL);
      }
      else
      {
         newReq.reqType                    = LOG_REQ_ERASE;
         newReq.pStatus                    = &logMonEraseBlock.erStatus;
         newReq.request.erase.pFoundOffset = &logMonEraseBlock.found;
         newReq.request.erase.nStartOffset = logConfig.nStartOffset;
         newReq.request.erase.type         = LOG_ERASE_QUICK;

         if (LOG_QUEUE_FULL != LogQueuePut(newReq))
         {
            result = USER_RESULT_OK;
            GSE_DebugStr(NORMAL,TRUE, "LogEraseCmd: Queued");
         }
         else
         {
            GSE_DebugStr(NORMAL,TRUE, "LogEraseCmd: Queue Full"NL);
         }
      }
   }

   return result;
}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: LogUserTables.c $
 * 
 * *****************  Version 22  *****************
 * User: Contractor V&v Date: 9/03/14    Time: 5:19p
 * Updated in $/software/control processor/code/system
 * SCR #1251 - CR updates
 *
 * *****************  Version 21  *****************
 * User: Contractor V&v Date: 8/26/14    Time: 3:00p
 * Updated in $/software/control processor/code/system
 * SCR #1251 - Compliance cleanup
 *
 * *****************  Version 20  *****************
 * User: John Omalley Date: 12-11-12   Time: 9:48a
 * Updated in $/software/control processor/code/system
 * SCR 1105, 1107, 1131, 1154 - Code Review Updates
 *
 * *****************  Version 19  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 18  *****************
 * User: John Omalley Date: 10/07/10   Time: 11:38a
 * Updated in $/software/control processor/code/system
 * SCR 923 - Code Review Updates
 *
 * *****************  Version 17  *****************
 * User: John Omalley Date: 8/12/10    Time: 7:53p
 * Updated in $/software/control processor/code/system
 * SCR 789 - Added protection around multiple user commands to erase logs.
 *
 * *****************  Version 16  *****************
 * User: Contractor3  Date: 7/16/10    Time: 8:57a
 * Updated in $/software/control processor/code/system
 * SCR #702 - Changes based on code review.
 *
 * *****************  Version 15  *****************
 * User: Contractor3  Date: 6/25/10    Time: 9:46a
 * Updated in $/software/control processor/code/system
 * SCR #662 - Changes based on code review
 *
 * *****************  Version 14  *****************
 * User: Contractor V&v Date: 5/12/10    Time: 3:51p
 * Updated in $/software/control processor/code/system
 * SCR #351 fast.reset=really housekeeping
 *
 * *****************  Version 13  *****************
 * User: Jeff Vahue   Date: 4/16/10    Time: 11:14a
 * Updated in $/software/control processor/code/system
 * SCR #550 - add null check prior to strncmp
 *
 * *****************  Version 12  *****************
 * User: Jeff Vahue   Date: 4/14/10    Time: 6:17p
 * Updated in $/software/control processor/code/system
 * SCR #547 - log.erase a priviledged command
 *
 * *****************  Version 11  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:36p
 * Updated in $/software/control processor/code/system
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 *
 * *****************  Version 10  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/system
 * SCR# 483 - Function Names
 *
 * *****************  Version 9  *****************
 * User: Contractor V&v Date: 3/10/10    Time: 4:42p
 * Updated in $/software/control processor/code/system
 * SCR #248 Parameter Log SCR #437 User commands should be RO
 *
 * *****************  Version 8  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 7  *****************
 * User: Jeff Vahue   Date: 2/17/10    Time: 1:24p
 * Updated in $/software/control processor/code/system
 * SCR# 452 - code coverage mods
 *
 * *****************  Version 6  *****************
 * User: Contractor V&v Date: 12/18/09   Time: 6:57p
 * Updated in $/software/control processor/code/system
 * SCR 106  LogUserTables.c changed to be an include file
 *
 * *****************  Version 5  *****************
 * User: Jeff Vahue   Date: 12/14/09   Time: 2:41p
 * Updated in $/software/control processor/code/system
 * SCR# 106
 *
 * *****************  Version 4  *****************
 * User: Jim Mood     Date: 9/29/09    Time: 11:58a
 * Updated in $/software/control processor/code/system
 * SCR# 178 Updated User Manager Tables
 *
 * *****************  Version 3  *****************
 * User: John Omalley Date: 8/06/09    Time: 3:56p
 * Updated in $/software/control processor/code/system
 * SCR 179
 *    Removed chip erase.
 *
 ***************************************************************************/
