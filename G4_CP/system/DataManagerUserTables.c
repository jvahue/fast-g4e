#define DATAMNG_USERTABLE_BODY
/******************************************************************************
Copyright (C) 2009-2011 Pratt & Whitney Engine Services, Inc.
All Rights Reserved. Proprietary and Confidential.

File:        DataManagerUserTables.c

Description: The data manager contains the functions used to record
             data from the various interfaces.


VERSION
$Revision: 24 $  $Date: 12-11-13 11:10a $ 

******************************************************************************/

#ifndef DATA_MNG_BODY
#error DataManagerUserTables.c should only be included by DataManager.c
#endif

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include <stdlib.h>
#include <string.h>

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "UploadMgr.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define LABEL_STR_LEN   (USER_MAX_MSG_STR_LEN * 3)

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static USER_HANDLER_RESULT DataMgr_UserForceDownload (USER_DATA_TYPE DataType,
                                                  USER_MSG_PARAM Param,
                                                  UINT32 Index,
                                                  const void *SetPtr,
                                                  void **GetPtr);
static USER_HANDLER_RESULT DataMgr_UserStopDownload (USER_DATA_TYPE DataType,
                                                  USER_MSG_PARAM Param,
                                                  UINT32 Index,
                                                  const void *SetPtr,
                                                  void **GetPtr);

static USER_HANDLER_RESULT DataMgr_UserCfg(USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr);
static USER_HANDLER_RESULT DataMgr_ShowConfig(USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr);

static USER_HANDLER_RESULT DataMgr_Status (USER_DATA_TYPE DataType,
                                               USER_MSG_PARAM Param,
                                               UINT32 Index,
                                               const void *SetPtr,
                                               void **GetPtr);
static USER_HANDLER_RESULT DataMgr_TCB_Status    (USER_DATA_TYPE DataType,
                                               USER_MSG_PARAM Param,
                                               UINT32 Index,
                                               const void *SetPtr,
                                               void **GetPtr);
static USER_HANDLER_RESULT DataMgr_ACS_Status (USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr);


/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
#pragma ghs nowarning 1545 //Suppress packed structure alignment warning
USER_ENUM_TBL recordStatusType[] =
{
  { "IDLE"     , DM_RECORD_IDLE        },
  { "RECORDING", DM_RECORD_IN_PROGRESS },
  { NULL, 0}
};

USER_ENUM_TBL bufferStatusType[] =
{
  { "EMPTY", DM_BUF_EMPTY },
  { "BUILD", DM_BUF_BUILD },
  { "STORE", DM_BUF_STORE },
  { NULL, 0}
};

USER_ENUM_TBL logReqType[] =
{
  { "NULL"       , LOG_REQ_NULL },
  { "PENDING"    , LOG_REQ_PENDING },
  { "IN_PROGRESS", LOG_REQ_INPROG },
  { "COMPLETE"   , LOG_REQ_COMPLETE },
  { "FAILED"     , LOG_REQ_FAILED },
  { NULL, 0 }
};

// ACS User Manager Tables
USER_ENUM_TBL acsPortType[] =
{
  { "NONE",     ACS_PORT_NONE     },
  { "ARINC429", ACS_PORT_ARINC429 },
  { "QAR",      ACS_PORT_QAR      },
  { "UART",     ACS_PORT_UART     },
  { NULL,       0                 }
};

// ACS Mode Table
USER_ENUM_TBL acsMode[] =
{
   { "RECORD",    ACS_RECORD     },
   { "DOWNLOAD",  ACS_DOWNLOAD   },
   { NULL,        0              }
};

// ACS Mode Table
USER_ENUM_TBL priorityTbl[] =
{
   { "1", LOG_PRIORITY_1 },
   { "2", LOG_PRIORITY_2 },
   { "3", LOG_PRIORITY_3 },
   { "4", LOG_PRIORITY_4 },
   { NULL,0              }
};

// Data Manager Table
static
USER_MSG_TBL dataManagerStatusCmd [] =
{/*Str                 Next Tbl Ptr   Handler Func.        Data Type         Access   Parameter                             IndexRange          DataLimit EnumTbl*/
  { "CHANNEL"        , NO_NEXT_TABLE, DataMgr_TCB_Status,     USER_TYPE_UINT16, USER_RO, &TCBTemp.nChannel      ,              0,MAX_ACS_CONFIG-1, NO_LIMIT, NULL },
  { "RECORD_STATUS"  , NO_NEXT_TABLE, DataMgr_TCB_Status,     USER_TYPE_ENUM  , USER_RO, &DMInfoTemp.recordStatus  ,           0,MAX_ACS_CONFIG-1, NO_LIMIT, recordStatusType },
  { "TIMEOUT_MS"     , NO_NEXT_TABLE, DataMgr_TCB_Status,     USER_TYPE_UINT16, USER_RO, &DMInfoTemp.acs_Config.nPacketSize_ms, 0,MAX_ACS_CONFIG-1, NO_LIMIT, NULL },
  { "STARTTIME"      , NO_NEXT_TABLE, DataMgr_Status, USER_TYPE_UINT32, USER_RO, &DMInfoTemp.nStartTime_mS      ,       0,MAX_ACS_CONFIG-1, NO_LIMIT, NULL },
  { "CURRENT_BUFFER" , NO_NEXT_TABLE, DataMgr_Status, USER_TYPE_UINT16, USER_RO, &DMInfoTemp.nCurrentBuffer     ,       0,MAX_ACS_CONFIG-1, NO_LIMIT, NULL },
  { NULL, NULL, NULL, NO_HANDLER_DATA }
};

// ~~~~~~~~~ ACS Commands ~~~~~~~~~~~~~~
static USER_MSG_TBL acsCmd [] =
{/*Str                Next Tbl Ptr    Handler Func. Data Type         Access    Parameter                     IndexRange          DataLimit       EnumTbl*/
  { "MODEL"         , NO_NEXT_TABLE , DataMgr_UserCfg,  USER_TYPE_STR   , USER_RW , &ConfigTemp.sModel,            0,MAX_ACS_CONFIG-1, 0,MAX_ACS_NAME, NULL },
  { "ID"            , NO_NEXT_TABLE , DataMgr_UserCfg,  USER_TYPE_STR   , USER_RW , &ConfigTemp.sID,               0,MAX_ACS_CONFIG-1, 0,MAX_ACS_NAME, NULL },
  { "PRIORITY"      , NO_NEXT_TABLE , DataMgr_UserCfg,  USER_TYPE_ENUM  , USER_RW , &ConfigTemp.priority,         0,MAX_ACS_CONFIG-1, NO_LIMIT,       priorityTbl },
  { "PORTTYPE"      , NO_NEXT_TABLE , DataMgr_UserCfg,  USER_TYPE_ENUM  , USER_RW , &ConfigTemp.portType,         0,MAX_ACS_CONFIG-1, NO_LIMIT,       acsPortType },
  { "PORTINDEX"     , NO_NEXT_TABLE , DataMgr_UserCfg,  USER_TYPE_UINT8 , USER_RW , &ConfigTemp.nPortIndex,        0,MAX_ACS_CONFIG-1, 0,3,            NULL        },
  { "PACKETSIZE_MS" , NO_NEXT_TABLE , DataMgr_UserCfg,  USER_TYPE_UINT16, USER_RW , &ConfigTemp.nPacketSize_ms,    0,MAX_ACS_CONFIG-1, 500,4000,       NULL        },
  { "SYSCOND"       , NO_NEXT_TABLE , DataMgr_UserCfg,  USER_TYPE_ENUM  , USER_RW , &ConfigTemp.systemCondition,  0,MAX_ACS_CONFIG-1, NO_LIMIT,       Flt_UserEnumStatus },
  { "MODE"          , NO_NEXT_TABLE , DataMgr_UserCfg,  USER_TYPE_ENUM  , USER_RW , &ConfigTemp.mode,             0,MAX_ACS_CONFIG-1, NO_LIMIT,       acsMode     },
  { NULL            , NO_NEXT_TABLE , DataMgr_UserCfg,  USER_TYPE_NONE  , USER_RW , NULL,                         0,MAX_ACS_CONFIG-1, NO_LIMIT,       NULL        },
  { NULL, NULL, NULL, NO_HANDLER_DATA }
};

static USER_MSG_TBL dl_LargeStats[] =
{/*Str                Next Tbl Ptr    Handler Func. Data Type         Access    Parameter                                   IndexRange          DataLimit       EnumTbl*/
   { "NUMBER"       , NO_NEXT_TABLE , DataMgr_ACS_Status,   USER_TYPE_UINT32, USER_RO,  &ACS_StatusTemp.largestRecord.nNumber,      0,MAX_ACS_CONFIG-1, NO_LIMIT,       NULL },
   { "SIZE"         , NO_NEXT_TABLE , DataMgr_ACS_Status,   USER_TYPE_UINT32, USER_RO,  &ACS_StatusTemp.largestRecord.nSize,        0,MAX_ACS_CONFIG-1, NO_LIMIT,       NULL },
   { "START_TIME_MS", NO_NEXT_TABLE , DataMgr_ACS_Status,   USER_TYPE_UINT32, USER_RO,  &ACS_StatusTemp.largestRecord.nStart_ms,    0,MAX_ACS_CONFIG-1, NO_LIMIT,       NULL },
   { "DURATION_MS"  , NO_NEXT_TABLE , DataMgr_ACS_Status,   USER_TYPE_UINT32, USER_RO,  &ACS_StatusTemp.largestRecord.nDuration_ms, 0,MAX_ACS_CONFIG-1, NO_LIMIT,       NULL },
//   { "REQUEST_TIME" , NO_NEXT_TABLE , ACS_Status,   TIMESTAMP       , USER_RO,  &ACS_StatusTemp.RequestTime,  0,MAX_ACS_CONFIG-1, NO_LIMIT,       NULL },
//   { "FOUND_TIME"   , NO_NEXT_TABLE , ACS_Status,   TIMESTAMP       , USER_RO,  &ACS_StatusTemp.FoundTime,    0,MAX_ACS_CONFIG-1, NO_LIMIT,       NULL },
   { NULL, NULL, NULL, NO_HANDLER_DATA }
};

static USER_MSG_TBL dl_SmallStats[] =
{/*Str                Next Tbl Ptr    Handler Func. Data Type         Access    Parameter                                    IndexRange          DataLimit       EnumTbl*/
   { "NUMBER"       , NO_NEXT_TABLE , DataMgr_ACS_Status,   USER_TYPE_UINT32, USER_RO,  &ACS_StatusTemp.smallestRecord.nNumber,      0,MAX_ACS_CONFIG-1, NO_LIMIT,       NULL },
   { "SIZE"         , NO_NEXT_TABLE , DataMgr_ACS_Status,   USER_TYPE_UINT32, USER_RO,  &ACS_StatusTemp.smallestRecord.nSize,        0,MAX_ACS_CONFIG-1, NO_LIMIT,       NULL },
   { "START_TIME_MS", NO_NEXT_TABLE , DataMgr_ACS_Status,   USER_TYPE_UINT32, USER_RO,  &ACS_StatusTemp.smallestRecord.nStart_ms,    0,MAX_ACS_CONFIG-1, NO_LIMIT,       NULL },
   { "DURATION_MS"  , NO_NEXT_TABLE , DataMgr_ACS_Status,   USER_TYPE_UINT32, USER_RO,  &ACS_StatusTemp.smallestRecord.nDuration_ms, 0,MAX_ACS_CONFIG-1, NO_LIMIT,       NULL },
//   { "REQUEST_TIME" , NO_NEXT_TABLE , ACS_Status,   TIMESTAMP       , USER_RO,  &ACS_StatusTemp.RequestTime,  0,MAX_ACS_CONFIG-1, NO_LIMIT,       NULL },
//   { "FOUND_TIME"   , NO_NEXT_TABLE , ACS_Status,   TIMESTAMP       , USER_RO,  &ACS_StatusTemp.FoundTime,    0,MAX_ACS_CONFIG-1, NO_LIMIT,       NULL },
   { NULL, NULL, NULL, NO_HANDLER_DATA }
};

static USER_MSG_TBL dl_SlowStats[] =
{/*Str                Next Tbl Ptr    Handler Func. Data Type         Access    Parameter                                    IndexRange          DataLimit       EnumTbl*/
   { "NUMBER"       , NO_NEXT_TABLE , DataMgr_ACS_Status,   USER_TYPE_UINT32, USER_RO,  &ACS_StatusTemp.longestRequest.nNumber,      0,MAX_ACS_CONFIG-1, NO_LIMIT,       NULL },
   { "SIZE"         , NO_NEXT_TABLE , DataMgr_ACS_Status,   USER_TYPE_UINT32, USER_RO,  &ACS_StatusTemp.longestRequest.nSize,        0,MAX_ACS_CONFIG-1, NO_LIMIT,       NULL },
   { "START_TIME_MS", NO_NEXT_TABLE , DataMgr_ACS_Status,   USER_TYPE_UINT32, USER_RO,  &ACS_StatusTemp.longestRequest.nStart_ms,    0,MAX_ACS_CONFIG-1, NO_LIMIT,       NULL },
   { "DURATION_MS"  , NO_NEXT_TABLE , DataMgr_ACS_Status,   USER_TYPE_UINT32, USER_RO,  &ACS_StatusTemp.longestRequest.nDuration_ms, 0,MAX_ACS_CONFIG-1, NO_LIMIT,       NULL },
//   { "REQUEST_TIME" , NO_NEXT_TABLE , ACS_Status,   TIMESTAMP       , USER_RO,  &ACS_StatusTemp.RequestTime,  0,MAX_ACS_CONFIG-1, NO_LIMIT,       NULL },
//   { "FOUND_TIME"   , NO_NEXT_TABLE , ACS_Status,   TIMESTAMP       , USER_RO,  &ACS_StatusTemp.FoundTime,    0,MAX_ACS_CONFIG-1, NO_LIMIT,       NULL },
   { NULL, NULL, NULL, NO_HANDLER_DATA }
};

static USER_MSG_TBL dl_FastStats[] =
{/*Str                Next Tbl Ptr    Handler Func. Data Type         Access    Parameter                                     IndexRange          DataLimit       EnumTbl*/
   { "NUMBER"       , NO_NEXT_TABLE , DataMgr_ACS_Status,   USER_TYPE_UINT32, USER_RO,  &ACS_StatusTemp.shortestRequest.nNumber,      0,MAX_ACS_CONFIG-1, NO_LIMIT,       NULL },
   { "SIZE"         , NO_NEXT_TABLE , DataMgr_ACS_Status,   USER_TYPE_UINT32, USER_RO,  &ACS_StatusTemp.shortestRequest.nSize,        0,MAX_ACS_CONFIG-1, NO_LIMIT,       NULL },
   { "START_TIME_MS", NO_NEXT_TABLE , DataMgr_ACS_Status,   USER_TYPE_UINT32, USER_RO,  &ACS_StatusTemp.shortestRequest.nStart_ms,    0,MAX_ACS_CONFIG-1, NO_LIMIT,       NULL },
   { "DURATION_MS"  , NO_NEXT_TABLE , DataMgr_ACS_Status,   USER_TYPE_UINT32, USER_RO,  &ACS_StatusTemp.shortestRequest.nDuration_ms, 0,MAX_ACS_CONFIG-1, NO_LIMIT,       NULL },
//   { "REQUEST_TIME" , NO_NEXT_TABLE , ACS_Status,   TIMESTAMP       , USER_RO,  &ACS_StatusTemp.RequestTime,  0,MAX_ACS_CONFIG-1, NO_LIMIT,       NULL },
//   { "FOUND_TIME"   , NO_NEXT_TABLE , ACS_Status,   TIMESTAMP       , USER_RO,  &ACS_StatusTemp.FoundTime,    0,MAX_ACS_CONFIG-1, NO_LIMIT,       NULL },
   { NULL, NULL, NULL, NO_HANDLER_DATA }
};

static USER_MSG_TBL acsStatus [] =
{/*Str                Next Tbl Ptr    Handler Func. Data Type         Access    Parameter                     IndexRange          DataLimit       EnumTbl*/
  { "TOTAL_RCVD"    , NO_NEXT_TABLE , DataMgr_ACS_Status,   USER_TYPE_UINT32, USER_RO,  &ACS_StatusTemp.nRcvd,        0,MAX_ACS_CONFIG-1, NO_LIMIT,       NULL },
  { "TOTAL_BYTES"   , NO_NEXT_TABLE , DataMgr_ACS_Status,   USER_TYPE_UINT32, USER_RO,  &ACS_StatusTemp.nTotalBytes,  0,MAX_ACS_CONFIG-1, NO_LIMIT,       NULL },
//  { "START_TIME"    , NO_NEXT_TABLE , ACS_Status,   TIMESTAMP       , USER_RO,  &ACS_StatusTemp.StartTime,    0,MAX_ACS_CONFIG-1, NO_LIMIT,       NULL },
//  { "END_TIME"      , NO_NEXT_TABLE , ACS_Status,   TIMESTAMP       , USER_RO,  &ACS_StatusTemp.EndTime,      0,MAX_ACS_CONFIG-1, NO_LIMIT,       NULL },
  { "START_TIME_MS" , NO_NEXT_TABLE , DataMgr_ACS_Status,   USER_TYPE_UINT32, USER_RO,  &ACS_StatusTemp.nStart_ms,    0,MAX_ACS_CONFIG-1, NO_LIMIT,       NULL },
  { "DURATION_MS"   , NO_NEXT_TABLE , DataMgr_ACS_Status,   USER_TYPE_UINT32, USER_RO,  &ACS_StatusTemp.nDuration_ms, 0,MAX_ACS_CONFIG-1, NO_LIMIT,       NULL },
  { "LARGEST"       , dl_LargeStats , NULL,         NO_HANDLER_DATA},
  { "SMALLEST"      , dl_SmallStats , NULL,         NO_HANDLER_DATA},
  { "SLOWEST"       , dl_SlowStats  , NULL,         NO_HANDLER_DATA},
  { "FASTEST"       , dl_FastStats  , NULL,         NO_HANDLER_DATA},
  { NULL, NULL, NULL, NO_HANDLER_DATA }
};

static USER_MSG_TBL acsCfgTbl[] =
{/*Str                Next Tbl Ptr       Handler Func.           Data Type            Access                        Parameter       IndexRange  DataLimit    EnumTbl*/
  {"CFG"            , acsCmd        ,    NULL,                   NO_HANDLER_DATA},
  {"STATUS"         , acsStatus     ,    NULL,                   NO_HANDLER_DATA},
  {"FORCEDOWNLOAD"  , NO_NEXT_TABLE ,    DataMgr_UserForceDownload,  USER_TYPE_ACTION,   USER_RO,  NULL, -1,-1,  NO_LIMIT, NULL },
  {"STOPDOWNLOAD"   , NO_NEXT_TABLE ,    DataMgr_UserStopDownload,   USER_TYPE_ACTION,   USER_RO,  NULL, -1,-1,  NO_LIMIT, NULL },
  { DISPLAY_CFG     , NO_NEXT_TABLE ,    DataMgr_ShowConfig        , USER_TYPE_ACTION  ,(USER_RO|USER_NO_LOG|USER_GSE)  ,NULL           ,-1,-1      ,NO_LIMIT    ,NULL },
  { NULL            , NULL          ,    NULL                  , NO_HANDLER_DATA }
};
#pragma ghs endnowarning
static USER_MSG_TBL rootACSMsg = {"ACS", acsCfgTbl, NULL, NO_HANDLER_DATA};
static USER_MSG_TBL dataMngMsg = {"DATA", dataManagerStatusCmd, NULL, NO_HANDLER_DATA};

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
* Function:    DataMgr_UserForceDownload()
*
* Description: User mgr command to force ACS Downloads
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
static
USER_HANDLER_RESULT DataMgr_UserForceDownload (USER_DATA_TYPE DataType,
                                           USER_MSG_PARAM Param,
                                           UINT32 Index,
                                           const void *SetPtr,
                                           void **GetPtr)
{
   USER_HANDLER_RESULT result;

   if (TRUE == DataMgrStartDownload())
   {
      GSE_DebugStr(NORMAL,TRUE,"DataMgr: Starting Download process");
   }
   else
   {
      GSE_DebugStr(NORMAL,TRUE,"DataMgr: Download already in progress or Upload in Progress");
   }
   result = USER_RESULT_OK;
   return result;
}

/******************************************************************************
* Function:    DataMgr_UserStopDownload()
*
* Description: User mgr command to stop ACS Downloads
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
static
USER_HANDLER_RESULT DataMgr_UserStopDownload (USER_DATA_TYPE DataType,
                                           USER_MSG_PARAM Param,
                                           UINT32 Index,
                                           const void *SetPtr,
                                           void **GetPtr)
{
   USER_HANDLER_RESULT result;

   if (DataMgrDownloadingACS())
   {
      DataMgrStopDownload();
      GSE_DebugStr(NORMAL,TRUE,"DataMgr: Stop Download process");
   }
   else
   {
      GSE_DebugStr(NORMAL,TRUE,"DataMgr: Download not in progress");
   }
   result = USER_RESULT_OK;
   return result;
}

/******************************************************************************
* Function:     DataMgr_UserCfg
*
* Description:  Handles User Manager requests to change ACS configuraion
*               items.
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
static
USER_HANDLER_RESULT DataMgr_UserCfg(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr)
{
 USER_HANDLER_RESULT result = USER_RESULT_OK;

  //Get sensor structure based on index
  memcpy(&ConfigTemp,
         &CfgMgr_ConfigPtr()->ACSConfigs[Index],
         sizeof(ConfigTemp));

  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    memcpy(&CfgMgr_ConfigPtr()->ACSConfigs[Index],&ConfigTemp,sizeof(ConfigTemp));
    CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
                           &CfgMgr_ConfigPtr()->ACSConfigs[Index],
                           sizeof(ConfigTemp));
  }
  return result;
}



/******************************************************************************
* Function:    DataMgr_ShowConfig
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
static
USER_HANDLER_RESULT DataMgr_ShowConfig(USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr)
{
  CHAR  labelStem[] = "\r\n\r\nACS.CFG";
  CHAR  label[LABEL_STR_LEN];
  INT16 i;

  USER_HANDLER_RESULT result = USER_RESULT_OK;
  USER_MSG_TBL*  pCfgTable;

  //Top-level name is a single indented space
  CHAR branchName[USER_MAX_MSG_STR_LEN] = " ";

  pCfgTable = &acsCmd[0];  // Get pointer to config entry

  for (i = 0; i < MAX_ACS_CONFIG && result == USER_RESULT_OK; ++i)
  {
    // Display element info above each set of data.
    snprintf(label, sizeof(label), "%s[%d]", labelStem, i);

    result = USER_RESULT_ERROR;
    if (User_OutputMsgString( label, FALSE ) )
    {
      result = User_DisplayConfigTree(branchName, pCfgTable, i, 0, NULL);
    }
  }
  return result;
}

/******************************************************************************
 * Function:     DataMgr_Status
 *
 * Description:  Handles User Manager requests
 *
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
static
USER_HANDLER_RESULT DataMgr_Status(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr)
{
   USER_HANDLER_RESULT result;

   result = USER_RESULT_OK;

   memcpy(&DMInfoTemp, &DataMgrInfo[Index], sizeof(DMInfoTemp));

   result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
   return result;
}

/******************************************************************************
* Function:     DataMgr_TCB_Status
*
* Description:  Handles User Manager requests
*
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
static
USER_HANDLER_RESULT DataMgr_TCB_Status(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr)
{
   USER_HANDLER_RESULT result;

   result = USER_RESULT_OK;

   memcpy(&TCBTemp, &DMPBlock[Index], sizeof(TCBTemp));

   result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

   return result;
}

/******************************************************************************
* Function:     DataMgr_ACS_Status
*
* Description:  Handles User Manager requests to retrieve the current ACS
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
* Returns:     USER_RESULT_OK:    Processed successfully
*              USER_RESULT_ERROR: Error processing command.
*
* Notes:
*
*****************************************************************************/

USER_HANDLER_RESULT DataMgr_ACS_Status (USER_DATA_TYPE DataType,
                                USER_MSG_PARAM Param,
                                UINT32 Index,
                                const void *SetPtr,
                                void **GetPtr)
{
   USER_HANDLER_RESULT result;

   result = USER_RESULT_ERROR;

    memcpy(&ACS_StatusTemp, &DataMgrInfo[Index].dl.statistics, 
           sizeof(DataMgrInfo[Index].dl.statistics));

   result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
   return result;

}

/*****************************************************************************
*  MODIFICATIONS
*    $History: DataManagerUserTables.c $
 * 
 * *****************  Version 24  *****************
 * User: John Omalley Date: 12-11-13   Time: 11:10a
 * Updated in $/software/control processor/code/system
 * SCR 1142 - Code Review Formatting Issues
 *
 * *****************  Version 23  *****************
 * User: John Omalley Date: 12-11-09   Time: 10:35a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review Update
 * 
 * *****************  Version 22  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 21  *****************
 * User: Jim Mood     Date: 7/26/12    Time: 2:10p
 * Updated in $/software/control processor/code/system
 * SCR 1076 Code review changes
 *
 * *****************  Version 20  *****************
 * User: Jim Mood     Date: 7/19/12    Time: 11:08a
 * Updated in $/software/control processor/code/system
 * SCR 1107: Data Offload changes for 2.0.0
 *
 * *****************  Version 19  *****************
 * User: John Omalley Date: 9/15/11    Time: 12:10p
 * Updated in $/software/control processor/code/system
 * SCR 1073 Wrapper on the Start Download Function for already downloading
 * or uploading. Removed wrapper from User Command and used the common
 * Start Download Function.
 *
 * *****************  Version 18  *****************
 * User: Contractor2  Date: 8/15/11    Time: 1:47p
 * Updated in $/software/control processor/code/system
 * SCR #1021 Error: Data Manager Buffers Full Fault Detected
 *
 * *****************  Version 17  *****************
 * User: John Omalley Date: 6/21/11    Time: 10:33a
 * Updated in $/software/control processor/code/system
 * SCR 1029 - Added User command to stop download per the Preliminary
 * Software Design Review.
 *
 * *****************  Version 16  *****************
 * User: John Omalley Date: 4/11/11    Time: 10:07a
 * Updated in $/software/control processor/code/system
 * SCR 1029 - Added EMU Download logic to the Data Manager
 *
 * *****************  Version 15  *****************
 * User: Contractor2  Date: 10/14/10   Time: 2:16p
 * Updated in $/software/control processor/code/system
 * SCR #931 Code Review: DataManager
 *
 * *****************  Version 14  *****************
 * User: Contractor2  Date: 10/07/10   Time: 11:48a
 * Updated in $/software/control processor/code/system
 * SCR #916 Code Review Updates
 *
 * *****************  Version 13  *****************
 * User: John Omalley Date: 8/18/10    Time: 4:17p
 * Updated in $/software/control processor/code/system
 * SCR 803 - Removed INVALID from Buffer Index Type
 *
 * *****************  Version 12  *****************
 * User: John Omalley Date: 7/02/10    Time: 10:29a
 * Updated in $/software/control processor/code/system
 * SCR 669 - Fixed Status Type sizes
 *
 * *****************  Version 11  *****************
 * User: Peter Lee    Date: 6/10/10    Time: 2:32p
 * Updated in $/software/control processor/code/system
 * SCR #365 Add "UART" to acs port selection.
 *
 * *****************  Version 10  *****************
 * User: Contractor V&v Date: 6/08/10    Time: 5:53p
 * Updated in $/software/control processor/code/system
 * SCR #615 Showcfg/Long msg enhancement
 *
 * *****************  Version 9  *****************
 * User: Jeff Vahue   Date: 3/29/10    Time: 1:55p
 * Updated in $/software/control processor/code/system
 * SCR# 514 - The ACS Packet size max value changed from 10000 to 2500.
 * The sys.get.mem command used strtol to convert the address pointer, s/b
 * strtoul.
 *
 * *****************  Version 8  *****************
 * User: Contractor V&v Date: 3/19/10    Time: 4:31p
 * Updated in $/software/control processor/code/system
 * SCR #248 Parameter Log Change Add no log for showcfg
 *
 * *****************  Version 7  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 4:00p
 * Updated in $/software/control processor/code/system
 * SCR 106 Fix findings for User_GenericAccessor
 *
 * *****************  Version 6  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:57p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 5  *****************
 * User: Jeff Vahue   Date: 2/17/10    Time: 1:24p
 * Updated in $/software/control processor/code/system
 * SCR# 452 - code coverage mods
 *
 * *****************  Version 4  *****************
 * User: Jeff Vahue   Date: 2/11/10    Time: 12:46p
 * Updated in $/software/control processor/code/system
 * SCR# 446 - remove unusable (i.e., 99.99% - ok its very high -
 * probability of returning the same status, so no real value added) Data
 * Manager commands
 *
 * *****************  Version 3  *****************
 * User: Contractor V&v Date: 1/27/10    Time: 5:23p
 * Updated in $/software/control processor/code/system
 * SCR 42 Reading Configuration
 *
 * *****************  Version 2  *****************
 * User: Contractor V&v Date: 1/13/10    Time: 4:58p
 * Updated in $/software/control processor/code/system
 *
 * *****************  Version 1  *****************
 * User: Contractor V&v Date: 12/10/09   Time: 5:44p
 * Created in $/software/control processor/code/system
 * SCR 106
*
*
*
***************************************************************************/
