#define UPLOADMGR_USERTABLE_BODY
/******************************************************************************
Copyright (C) 2007-2010 Pratt & Whitney Engine Services, Inc. 
All Rights Reserved. Proprietary and Confidential.

File:         UploadMgrUserTables.c       

Description:  Upload Manager encompasses the functionality required to
package system logs and application (flight data) logs into
files, transfer the files to the micro-server, and finally
validate the files were received correctly by the
micro-server and ground server.

VERSION
$Revision: 18 $  $Date: 8/28/12 12:43p $

******************************************************************************/
#ifndef UPLOADMGR_BODY
#error UploadMgrUserTable.c should only be included by UploadMgr.c
#endif


//User manager command table and handling functions
USER_HANDLER_RESULT UploadMgr_UserForceUpload(USER_DATA_TYPE DataType,
                                              USER_MSG_PARAM Param,
                                              UINT32 Index,
                                              const void *SetPtr,
                                              void **GetPtr);

USER_HANDLER_RESULT UploadMgr_UserUploadEnable(USER_DATA_TYPE DataType,
                                               USER_MSG_PARAM Param,
                                               UINT32 Index,
                                               const void *SetPtr,
                                               void **GetPtr);

USER_HANDLER_RESULT UploadMgr_UserVfyTbl(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr);

USER_HANDLER_RESULT UploadMgr_UserVfyRows(USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr);

USER_HANDLER_RESULT UploadMgr_UserVfyIdxByFN(USER_DATA_TYPE DataType,
                                             USER_MSG_PARAM Param,
                                             UINT32 Index,
                                             const void *SetPtr,
                                             void **GetPtr);


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
//Temporary location for viewing/modifying Verification Table rows by User.c
static UPLOADMGR_FILE_VFY VfyRowTmp;
static INT8               VfyLookupFN[VFY_TBL_FN_SIZE];
static INT32              VfyLookupIdx;

static USER_ENUM_TBL UploadVerifyStateEnum[] =
{
   {"STARTED",      VFY_STA_STARTED},
   {"COMPLETED",    VFY_STA_COMPLETED},
   {"MS_FAIL",      VFY_STA_MSFAIL},
   {"MS_VFY",       VFY_STA_MSVFY},
   {"GROUND_VFY",   VFY_STA_GROUNDVFY},
   {"LG_DELETED",   VFY_STA_LOG_DELETE},
   {"DELETED",      VFY_STA_DELETED},
   {NULL,0}
};

//For the user command to get verification table statistics
UINT32 m_TotalVerifyRows;
UINT32 m_FreeVerifyRows;

#pragma ghs nowarning 1545 //Suppress packed structure alignment warning

static USER_MSG_TBL FileCmd[] =
{/*Str               Next Tbl Ptr   Handler Func.         Data Type         Access    Parameter           IndexRange                       DataLimit EnumTbl*/                      
  {"NAME"          , NO_NEXT_TABLE, UploadMgr_UserVfyTbl, USER_TYPE_STR   , USER_RO, &VfyRowTmp.Name,     0, UPLOADMGR_VFY_TBL_MAX_ROWS-1, NO_LIMIT, NULL},
  {"STATE"         , NO_NEXT_TABLE, UploadMgr_UserVfyTbl, USER_TYPE_ENUM  , USER_RO, &VfyRowTmp.State,    0, UPLOADMGR_VFY_TBL_MAX_ROWS-1, NO_LIMIT, UploadVerifyStateEnum},
  {"LOGDELETED"    , NO_NEXT_TABLE, UploadMgr_UserVfyTbl, USER_TYPE_YESNO , USER_RO, &VfyRowTmp.LogDeleted,0, UPLOADMGR_VFY_TBL_MAX_ROWS-1, NO_LIMIT, NULL},
  {"START"         , NO_NEXT_TABLE, UploadMgr_UserVfyTbl, USER_TYPE_HEX32 , USER_RO, &VfyRowTmp.Start,    0, UPLOADMGR_VFY_TBL_MAX_ROWS-1, NO_LIMIT, NULL},
  {"END"           , NO_NEXT_TABLE, UploadMgr_UserVfyTbl, USER_TYPE_HEX32 , USER_RO, &VfyRowTmp.End,      0, UPLOADMGR_VFY_TBL_MAX_ROWS-1, NO_LIMIT, NULL},
  {"TYPE"          , NO_NEXT_TABLE, UploadMgr_UserVfyTbl, USER_TYPE_HEX32 , USER_RO, &VfyRowTmp.Type,     0, UPLOADMGR_VFY_TBL_MAX_ROWS-1, NO_LIMIT, NULL},
  {"ACS"           , NO_NEXT_TABLE, UploadMgr_UserVfyTbl, USER_TYPE_HEX32 , USER_RO, &VfyRowTmp.Source,   0, UPLOADMGR_VFY_TBL_MAX_ROWS-1, NO_LIMIT, NULL},
  {"PRIORITY"      , NO_NEXT_TABLE, UploadMgr_UserVfyTbl, USER_TYPE_HEX32 , USER_RO, &VfyRowTmp.Priority, 0, UPLOADMGR_VFY_TBL_MAX_ROWS-1, NO_LIMIT, NULL},
  {"CRC"           , NO_NEXT_TABLE, UploadMgr_UserVfyTbl, USER_TYPE_HEX32 , USER_RO, &VfyRowTmp.CRC,      0, UPLOADMGR_VFY_TBL_MAX_ROWS-1, NO_LIMIT, NULL},
  {"DELETE"        , NO_NEXT_TABLE, UploadMgr_UserVfyTbl, USER_TYPE_ACTION, USER_RO, (void*)0,            0, UPLOADMGR_VFY_TBL_MAX_ROWS-1, NO_LIMIT, NULL},
  { NULL           , NULL                              ,  NULL, NO_HANDLER_DATA}
};
#pragma ghs endnowarning 

static USER_MSG_TBL LookupCmd[] =
{
  {"FN",            NO_NEXT_TABLE,  UploadMgr_UserVfyIdxByFN, USER_TYPE_STR,  USER_RW,VfyLookupFN,        -1,-1,                           NO_LIMIT, NULL },
  {"INDEX",         NO_NEXT_TABLE,  UploadMgr_UserVfyIdxByFN, USER_TYPE_INT32,USER_RO,NULL,               -1,-1,                           NO_LIMIT, NULL },
  { NULL,     NULL,         NULL,   NO_HANDLER_DATA}
};

static USER_MSG_TBL VfyCmd[] =
{
  {"TBL_SIZE", NO_NEXT_TABLE,  UploadMgr_UserVfyRows,    USER_TYPE_UINT32,USER_RO,&m_TotalVerifyRows,  -1,-1,  NO_LIMIT, NULL },
  {"TBL_FREE", NO_NEXT_TABLE,  UploadMgr_UserVfyRows,    USER_TYPE_UINT32,USER_RO,&m_FreeVerifyRows,   -1,-1,  NO_LIMIT, NULL },
  {"FILE",     FileCmd,        NULL, NO_HANDLER_DATA},
  {"LOOKUP",   LookupCmd,      NULL, NO_HANDLER_DATA},
  { NULL,      NULL,           NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL UploadCmd[] =
{
  {"FORCEUPLOAD", NO_NEXT_TABLE,  UploadMgr_UserForceUpload,  USER_TYPE_ACTION,USER_RO,  NULL, -1,-1,  NO_LIMIT, NULL },
  {"ENABLED",     NO_NEXT_TABLE,  UploadMgr_UserUploadEnable, USER_TYPE_YESNO, USER_RW,  NULL, -1,-1,  NO_LIMIT, NULL },
  {"VFY",      VfyCmd,          NULL, NO_HANDLER_DATA},
  { NULL,      NULL,            NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL RootMsg = {"UPLOAD", UploadCmd,  NULL, NO_HANDLER_DATA};

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
* Function:    UploadMgr_UserForceUpload()
*  
* Description: User mgr command to force log upload to the Micro-server
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
******************************************************************************/
USER_HANDLER_RESULT UploadMgr_UserForceUpload(USER_DATA_TYPE DataType,
                                              USER_MSG_PARAM Param,
                                              UINT32 Index,
                                              const void *SetPtr,
                                              void **GetPtr)
{
   USER_HANDLER_RESULT result;

   UploadMgr_StartUpload(UPLOAD_START_FORCED);
   result = USER_RESULT_OK;
   return result;
}

/******************************************************************************
* Function:    UploadMgr_UserUploadEnable()
*  
* Description: User mgr command to enable/disable to the Micro-server
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
USER_HANDLER_RESULT UploadMgr_UserUploadEnable(USER_DATA_TYPE DataType,
                                               USER_MSG_PARAM Param,
                                               UINT32 Index,
                                               const void *SetPtr,
                                               void **GetPtr)
{
   if(SetPtr == NULL)
   {
      **(BOOLEAN**)GetPtr = !LCTaskData.Disable;
   }
   else
   {
      UploadMgr_SetUploadEnable(*(BOOLEAN*)SetPtr);
   } 

   return USER_RESULT_OK;
}


/******************************************************************************
* Function:    UploadMgr_UserVfyTbl()
*  
* Description: User mgr command read/delete verification table rows
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
USER_HANDLER_RESULT UploadMgr_UserVfyTbl(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_ERROR;

  //Convert 1 based to 0 based index

  NV_Read( NV_UPLOAD_VFY_TBL, VFY_ROW_TO_OFFSET(Index), &VfyRowTmp, sizeof(VfyRowTmp));
   
  // If the param value is 0, this is a delete command.  Else it is a command
  // to return row data.
  if( Param.Int != 0)
  {
     if( Param.Ptr == &VfyRowTmp.State)
     { //TODO: This is a hack to get around the fact User only supports 32-bit
        //enum types.
        **(UINT32**)GetPtr = VfyRowTmp.State;
     }
     else
     {
        *GetPtr = Param.Ptr;
     }
     result =  USER_RESULT_OK;
  }
  else
  {
    UploadMgr_UpdateFileVfy( Index, VFY_STA_DELETED, 0, 0);
  }
   return result;
}



/******************************************************************************
* Function:    UploadMgr_UserVfyRows()
*  
* Description: User mgr command to get the free and total
*              verification table rows counts
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
USER_HANDLER_RESULT UploadMgr_UserVfyRows(USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr)
{
   INT32 i;
   UPLOADMGR_FILE_VFY VfyRow;

   m_FreeVerifyRows = 0;

   //Scan the table for deleted slots.
   for(i = 0; i < UPLOADMGR_VFY_TBL_MAX_ROWS; i++)
   {
      NV_Read(NV_UPLOAD_VFY_TBL,
         VFY_ROW_TO_OFFSET(i),
         &VfyRow,
         sizeof(VfyRow));

      if( VFY_STA_DELETED == VfyRow.State )
      {
         //Count em' up
         m_FreeVerifyRows++;
      }
   }

   //Return the value the parameter points to (either FreeVerifyRows or
   //TotalVerifyRows which is set during init)
   *GetPtr = Param.Ptr;

   return USER_RESULT_OK; 
}

/******************************************************************************
* Function:    UploadMgr_UserVfyIdxByFN()
*  
* Description: User Manager command to lookup a file by name.  The filname
*              to lookup is stored in VfyLookupFN and the FVT table is
*              searched for that filename.  If the name is found, the 
*              index of it's row data is stored in LookupIdx.
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
USER_HANDLER_RESULT UploadMgr_UserVfyIdxByFN(USER_DATA_TYPE DataType,
                                             USER_MSG_PARAM Param,
                                             UINT32 Index,
                                             const void *SetPtr,
                                             void **GetPtr)
{
   USER_HANDLER_RESULT result = USER_RESULT_ERROR;
   UPLOADMGR_FILE_VFY VfyRow;

   if(Param.Ptr == VfyLookupFN)
   {
      if(SetPtr != NULL)
      {
         if(strlen(SetPtr) < sizeof(VfyLookupFN))
         {
            strncpy_safe(VfyLookupFN, sizeof(VfyLookupFN), SetPtr, _TRUNCATE);
            VfyLookupIdx = UploadMgr_GetFileVfy( VfyLookupFN,&VfyRow );
            result = USER_RESULT_OK;
         }
      }
      else
      {
         *GetPtr = VfyLookupFN;
         result = USER_RESULT_OK;
      }
   }
   else
   {
      **(INT32**)GetPtr = VfyLookupIdx;
      result = USER_RESULT_OK;
   }

   return result;
}

/*****************************************************************************/
/* Local Functions                                                           */ 
/*****************************************************************************/

/*****************************************************************************
*  MODIFICATIONS
*    $History: UploadMgrUserTables.c $
 * 
 * *****************  Version 18  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 12:43p
 * Updated in $/software/control processor/code/application
 * SCR# 1142
 * 
 * *****************  Version 17  *****************
 * User: Jim Mood     Date: 7/26/12    Time: 2:08p
 * Updated in $/software/control processor/code/application
 * SCR# 1076 Code Review Updates
 * 
 * *****************  Version 16  *****************
 * User: Jim Mood     Date: 7/19/12    Time: 10:41a
 * Updated in $/software/control processor/code/application
 * SCR 1107: Data Offload changes for 2.0.0
 * 
 * *****************  Version 15  *****************
 * User: Contractor2  Date: 10/14/10   Time: 3:47p
 * Updated in $/software/control processor/code/application
 * SCR #930 Code Review: UploadMgr
 * 
 * *****************  Version 14  *****************
 * User: Jim Mood     Date: 8/10/10    Time: 12:03p
 * Updated in $/software/control processor/code/application
 * SCR 781 UploadMgrUserTabels.c
 * 
 * *****************  Version 13  *****************
 * User: Jim Mood     Date: 7/23/10    Time: 6:52p
 * Updated in $/software/control processor/code/application
 * SCR #151 Delete Ground Verified files from FVT ASAP instead of only
 * when FVT runs
 * 
 * *****************  Version 12  *****************
 * User: Peter Lee    Date: 7/22/10    Time: 11:00a
 * Updated in $/software/control processor/code/application
 * SCR #679 Removed 'upload.init' cmd. 
 * 
 * *****************  Version 11  *****************
 * User: Contractor V&v Date: 7/21/10    Time: 7:13p
 * Updated in $/software/control processor/code/application
 * SCR #538 SPIManager startup & NV Mgr Issues
 * 
 * *****************  Version 10  *****************
 * User: Contractor2  Date: 6/24/10    Time: 11:01a
 * Updated in $/software/control processor/code/application
 * SCR #660 Implementation: SRS-2477 - Delete File when GS CRC does not
 * match FVT CRC
 * Minor coding standard fixes
 * 
 * *****************  Version 9  *****************
 * User: Contractor2  Date: 4/15/10    Time: 2:23p
 * Updated in $/software/control processor/code/application
 * Fix SCR 137: Add upload command to force deletion of all file entries.
 * 
 * *****************  Version 8  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:07p
 * Updated in $/software/control processor/code/application
 * SCR #317 Implement safe strncpy
 * 
 * *****************  Version 7  *****************
 * User: Contractor V&v Date: 3/19/10    Time: 4:29p
 * Updated in $/software/control processor/code/application
 * SCR #279 Making NV filenames uppercase
 * 
 * *****************  Version 6  *****************
 * User: Contractor2  Date: 3/02/10    Time: 11:56a
 * Updated in $/software/control processor/code/application
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 5  *****************
 * User: Jeff Vahue   Date: 2/17/10    Time: 12:51p
 * Updated in $/software/control processor/code/application
 * SCR# 452 - Code Coverage if/then/else changes
 * 
 * *****************  Version 4  *****************
 * User: Contractor V&v Date: 2/03/10    Time: 2:56p
 * Updated in $/software/control processor/code/application
 * SCR 279
 * 
 * *****************  Version 3  *****************
 * User: Jim Mood     Date: 1/20/10    Time: 3:12p
 * Updated in $/software/control processor/code/application
 * SCR #401
 * 
 * *****************  Version 2  *****************
 * User: Jim Mood     Date: 12/23/09   Time: 5:35p
 * Updated in $/software/control processor/code/application
 * SCR #205
 * 
 * *****************  Version 1  *****************
 * User: Contractor V&v Date: 12/10/09   Time: 5:35p
 * Created in $/software/control processor/code/application
 * SCR 42 and SCR 106
* 
*
*****************************************************************************/
