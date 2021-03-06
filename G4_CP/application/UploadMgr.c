#define UPLOADMGR_BODY
/******************************************************************************
            Copyright (C) 2007-2016 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    ECCN:         9D991

    File:         UploadMgr.c

    Description:  Upload Manager encompasses the functionality required to
                  package system logs and application (flight data) logs into
                  files, transfer the files to the micro-server, and finally
                  validate the files were received correctly by the
                  micro-server and ground server.

   VERSION
   $Revision: 193 $  $Date: 6/02/16 3:16p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "UploadMgr.h"
#include "GSE.h"
#include "CfgManager.h"
#include "Version.h"
#include "assert.h"
#include "box.h"
#include "WatchDog.h"
#include "DataManager.h"
#include "EngineRun.h"

#include "TestPoints.h"

extern UINT32  __CRC_END;    // declared in CBITManager.c

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
//These macros compute the byte offsets for data in the verification table.
//It is used for accessing verification rows in the verification table file.
#define VFY_MAGIC_NUM_OFFSET   0
#define VFY_ROW_TO_OFFSET(Row) (sizeof(UINT32)+(sizeof(UPLOADMGR_FILE_VFY)*Row))

//Verification table magic number value.  This is used to detect an
//uninitialized file.
#define VFY_MAGIC_NUM        0x600DBEEF

#define ONE_SEC_IN_MS       1000
#define ONE_KB              1024

#define CHECK_FILE_TIMEOUT  10000
#define RMV_FILE_TIMEOUT    10000
#define CHECK_ROW_TIMEOUT    5000
/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
//States for the Log Collection task
typedef enum
{
  LOG_COLLECTION_INIT_AUTO,
  LOG_COLLECTION_INIT,
  LOG_COLLECTION_PRE_MAINTENANCE,
  LOG_COLLECTION_WAIT_EOF_LOGGED,
  LOG_COLLECTION_FINDFLIGHT,
  LOG_COLLECTION_STARTFILE,
  LOG_COLLECTION_LOG_HEADER_FIND,
  LOG_COLLECTION_LOG_HEADER_READ,
  LOG_COLLECTION_START_LOG_UPLOAD,
  LOG_COLLECTION_UPLOAD_FIND_LOG,
  LOG_COLLECTION_UPLOAD_READ_LOG,
  LOG_COLLECTION_FINISHFILE,
  LOG_COLLECTION_WT_FOR_MS_TO_VFY,
  LOG_COLLECTION_VFY_MS_MOVED_FILE,
  LOG_COLLECTION_NEXTFILE,
  LOG_COLLECTION_FILE_UL_ERROR,
  LOG_COLLECTION_POST_MAINTENANCE,
  LOG_COLLECTION_LOG_ERASE_MEMORY,
  LOG_COLLECTION_FINISHED,
  LOG_COLLECTION_AUTO_CHECK_FINISH
}UPLOADMGR_LOG_COLLECTION_STATE;

//Static data for the Log Collection task
typedef struct
{
  UPLOADMGR_LOG_COLLECTION_STATE State;
  BOOLEAN InProgress;
  BOOLEAN Disable;
  UINT32  FlightDataStart;
  UINT32  FlightDataEnd;
  UINT32  FlightLogCnt;
  UINT32  UploadOrderIdx;
  UINT32  LogSize;
  UINT32  LogOffset;
  LOG_BUF LogBuf;
  LOG_REQ_STATUS  LogEraseStatus;
  UPLOADMGR_FILE_HEADER FileHeader;
  UINT32  logUploadCnt;
  UINT32  FileFailCnt;
  UINT32  FileUploadedCnt;
  UINT32  FileNotUploadedCnt;
  INT8    FN[VFY_TBL_FN_SIZE];
  INT32   VfyRow;
  UINT32  VfyCRC;
}UPLOADMGR_LOG_COLLECTION_TASK_DATA;

//States for the File Upload Task
typedef enum
{
  FILE_UL_INIT,
  FILE_UL_WAIT_START_UL_RSP,
  FILE_UL_WAIT_DATA_UL_RSP,
  FILE_UL_WAIT_END_UL_RSP,
  FILE_UL_FINISHED,
  FILE_UL_ERROR
}UPLOADMGR_FILE_UL_STATE;

//States for the File Verification Table Maintenance sub-task
typedef enum
{
  TBL_MAINT_SRCH_STARTED_MSFAIL,//Search for "Started" or "MS Failed"
  TBL_MAINT_WT_RM_LOG_RSP,      //Wait for an MSSIM command response
  TBL_MAINT_SRCH_LOG_DELETED,   //Search for files that need mark for deletion
  TBL_MAINT_WT_MARK_DELETE,     //Wait for Log Manager "MarkForDeletion" task
  TBL_MAINT_SRCH_GROUNDVFY,     //Search for files marked GroundVfy
  TBL_MAINT_WT_CHECK_RSP2,      //Wait for "Check File" response again
  TBL_MAINT_ERROR,              //Indicates an error occurred while processing
  TBL_MAINT_FINISHED            //File Verification Maintenance is completed
}UPLOADMGR_TBL_MAINT_STATE;

//Static data for the File Upload Task
typedef struct
{
  UPLOADMGR_FILE_UL_STATE State;
  UPLOADMGR_FILE_UL_STATE ErrorState;
  MSCP_LOGFILE_INFO       LogFileInfo;
  BOOLEAN                 FinishFileUpload;
  UINT32                  FailCnt;
  //INT32                   VfyRow;
  UINT32                  LogStart;
  UINT32                  LogEnd;
  LOG_TYPE                LogType;
  LOG_SOURCE              LogSource;
  LOG_PRIORITY            LogPriority;
}UPLOADMGR_FILE_UL_TASK_DATA;

//Function parameter for the UploadBlock function
typedef enum
{
  UPLOAD_BLOCK_FIRST,
  UPLOAD_BLOCK_NEXT,
  UPLOAD_BLOCK_LAST
}UPLOADMGR_UPLOAD_BLOCK_OP;

//Row for the ACS upload order table
typedef struct
{
  LOG_TYPE      Type;
  LOG_PRIORITY  Priority;
  LOG_SOURCE    ACS;
}UPLOADMGR_UPLOAD_ORDER_TBL;

//State enumeration for the file verification table
//NOTE: Enum assigned to INT8, limit max value to 127.
typedef enum
{
  VFY_STA_STARTED      = 0, //File transfer started, but not completed.
  VFY_STA_COMPLETED    = 1, //File completely transfered to MS, but not verified
  VFY_STA_MSFAIL       = 2, //File failed verification on the MS
  VFY_STA_MSVFY        = 3, //File CRC from MS is good,
                            // and file is confirmed to have moved to CP_FILES
  VFY_STA_GROUNDVFY    = 4, //File CRC from GS is good,
                            // but the file move to CP_SAVED has not yet been verified
  VFY_STA_LOG_DELETE   = 6, //File round trip is complete,
                            // but the slot is being kept until the deletion
                            // of log data from the CP is complete.
  VFY_STA_DELETED      = 7  //Open, either never used, or
                            // the previous file was confirmed to have moved
}UPLOADMGR_VFY_STATE;

//Row for the file verification table
#pragma pack(1)
typedef struct
{
  INT8                Name[VFY_TBL_FN_SIZE];
  INT8                State;
  INT8                LogDeleted;
  UINT32              Start;
  UINT32              End;
  LOG_TYPE            Type;
  LOG_SOURCE          Source;
  LOG_PRIORITY        Priority;
  UINT32              CRC;
}UPLOADMGR_FILE_VFY;
#pragma pack()

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
//Log file data queue
static INT8 FileDataBuffer[UPLOADMGR_FILE_DATA_QUEUE_SIZE];
static FIFO FileDataFIFO;

//Log collection task data
static UPLOADMGR_LOG_COLLECTION_TASK_DATA LCTaskData;

//File Upload Task data
static UPLOADMGR_FILE_UL_TASK_DATA ULTaskData;

//Micro-server response status.  This data is volatile because it
//is modified by two different tasks with different priorities.
static volatile BOOLEAN m_GotResponse;
static volatile BOOLEAN ResponseStatusSuccess;
static volatile BOOLEAN ResponseStatusTimeout;
static volatile BOOLEAN ResponseStatusBusy;

//Micro-server response status for the File Verification Maintenance task
//commands
static volatile MSCP_CMD_ID          VfyRspId;      //ID of the response received
static volatile BOOLEAN              VfyRspFailed;  //MS command failed or timed out
static volatile MSCP_REMOVE_FILE_RSP RemoveFileRsp; //Data for a remove file response
static volatile MSCP_CHECK_FILE_RSP  CheckFileRsp;  //Data for a check file response

//Log Upload priority order.  Log Collection process creates
//log files for upload in this order.  Number of files to upload is the
//maximum possible ACS sources, plus one for system logs
static UPLOADMGR_UPLOAD_ORDER_TBL UploadOrderTbl[] =
               {{LOG_TYPE_ETM, LOG_PRIORITY_1,ACS_DONT_CARE},
                {LOG_TYPE_DATA,LOG_PRIORITY_1,ACS_1 },
                {LOG_TYPE_DATA,LOG_PRIORITY_1,ACS_2 },
                {LOG_TYPE_DATA,LOG_PRIORITY_1,ACS_3 },
                {LOG_TYPE_DATA,LOG_PRIORITY_1,ACS_4 },
                {LOG_TYPE_DATA,LOG_PRIORITY_1,ACS_5 },
                {LOG_TYPE_DATA,LOG_PRIORITY_1,ACS_6 },
                {LOG_TYPE_DATA,LOG_PRIORITY_1,ACS_7 },
                {LOG_TYPE_DATA,LOG_PRIORITY_1,ACS_8 },
                {LOG_TYPE_ETM, LOG_PRIORITY_2,ACS_DONT_CARE},
                {LOG_TYPE_DATA,LOG_PRIORITY_2,ACS_1 },
                {LOG_TYPE_DATA,LOG_PRIORITY_2,ACS_2 },
                {LOG_TYPE_DATA,LOG_PRIORITY_2,ACS_3 },
                {LOG_TYPE_DATA,LOG_PRIORITY_2,ACS_4 },
                {LOG_TYPE_DATA,LOG_PRIORITY_2,ACS_5 },
                {LOG_TYPE_DATA,LOG_PRIORITY_2,ACS_6 },
                {LOG_TYPE_DATA,LOG_PRIORITY_2,ACS_7 },
                {LOG_TYPE_DATA,LOG_PRIORITY_2,ACS_8 },
                {LOG_TYPE_ETM, LOG_PRIORITY_3,ACS_DONT_CARE},
                {LOG_TYPE_DATA,LOG_PRIORITY_3,ACS_1 },
                {LOG_TYPE_DATA,LOG_PRIORITY_3,ACS_2 },
                {LOG_TYPE_DATA,LOG_PRIORITY_3,ACS_3 },
                {LOG_TYPE_DATA,LOG_PRIORITY_3,ACS_4 },
                {LOG_TYPE_DATA,LOG_PRIORITY_3,ACS_5 },
                {LOG_TYPE_DATA,LOG_PRIORITY_3,ACS_6 },
                {LOG_TYPE_DATA,LOG_PRIORITY_3,ACS_7 },
                {LOG_TYPE_DATA,LOG_PRIORITY_3,ACS_8 },
                {LOG_TYPE_ETM, LOG_PRIORITY_4,ACS_DONT_CARE},
                {LOG_TYPE_DATA,LOG_PRIORITY_4,ACS_1 },
                {LOG_TYPE_DATA,LOG_PRIORITY_4,ACS_2 },
                {LOG_TYPE_DATA,LOG_PRIORITY_4,ACS_3 },
                {LOG_TYPE_DATA,LOG_PRIORITY_4,ACS_4 },
                {LOG_TYPE_DATA,LOG_PRIORITY_4,ACS_5 },
                {LOG_TYPE_DATA,LOG_PRIORITY_4,ACS_6 },
                {LOG_TYPE_DATA,LOG_PRIORITY_4,ACS_7 },
                {LOG_TYPE_DATA,LOG_PRIORITY_4,ACS_8 },
                {LOG_TYPE_SYSTEM,LOG_PRIORITY_3 ,ACS_DONT_CARE}};

static const UINT32
  UploadFilesPerFlight = sizeof(UploadOrderTbl)/sizeof(UploadOrderTbl[0]);

enum
{
  MS_CRC_RESULT_NONE,
  MS_CRC_RESULT_VFY,
  MS_CRC_RESULT_FAIL
}m_MSCrcResult;                     //Result of the CRC sent back from the micro-server
                                    //during log upload.

static INT32 m_CheckLogRow;         //For the CheckLogMoved and CheckLogCallBack
                                    //to identify what FVT row to update when
                                    //the command to validate a ground verify file was moved
static INT32 m_CheckLogFileCnt;     //Number of CRC's received that need to be checked for
                                    //move

static BOOLEAN  m_MaintainFileTableRunning; //Flag set by File Maintainence task
                                            // to TRUE while it is running
static BINARY_DATA_HDR BinarySubHdr;
static UINT32 m_FreeFVTRows;                //Num of rows in the FVT that are "deleted".
                                            // Init'd at power-on

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void    UploadMgr_FileUploadTask(void *pParam);
static void    UploadMgr_FileCollectionTask(void *pParam);
static void    UploadMgr_CheckLogMoved(void *pParam);
static void    UploadMgr_OpenFileVfyTbl(void);
static INT32   UploadMgr_GetVfyDeletedRow(void);
static INT32   UploadMgr_StartFileVfy(INT8* FN, UINT32 Start,
                             LOG_TYPE Type, LOG_SOURCE Source,
                             LOG_PRIORITY Priority);
static void    UploadMgr_UpdateFileVfy(INT32 Row,UPLOADMGR_VFY_STATE NewState,
                              UINT32 CRC, UINT32 End);
static INT32   UploadMgr_GetFileVfy(const INT8* FN, UPLOADMGR_FILE_VFY* VfyRow);
static LOG_FIND_STATUS UploadMgr_FindFlight(UINT32 *FlightStart,UINT32* FlightEnd);

static void    UploadMgr_MSRspCallback(UINT16 Id, void* PacketData, UINT16 Size,
                             MSI_RSP_STATUS Status);
static BOOLEAN UploadMgr_MSCRCCmdHandler(void* Data,UINT16 Size,
                             UINT32 Sequence);
static BOOLEAN UploadMgr_SendMSCmd(MSCP_CMD_ID Id, INT32 Timeout, void* Data, UINT32 Size);
static void    UploadMgr_VerifyMSCmdsCallback(UINT16 Id, void* PacketData,
                             UINT16 Size, MSI_RSP_STATUS Status);

static BOOLEAN UploadMgr_UploadFileBlock(UPLOADMGR_UPLOAD_BLOCK_OP Op);
static void    UploadMgr_MakeFileHeader(UPLOADMGR_FILE_HEADER* FileHeader,LOG_TYPE Type,
                             LOG_PRIORITY Priority, LOG_SOURCE ACS,
                             TIMESTAMP* Timestamp);
static void UploadMgr_StartFileUploadTask(const INT8* FN, const LOG_PRIORITY Priority);

static void    UploadMgr_FinishFileUpload(INT32 Row, UINT32 CRC, UINT32 LogOffset);

static void    UploadMgr_PrintUploadStats(UINT32 BytesSent);
static INT32   UploadMgr_MaintainFileTable(BOOLEAN Start);
static void    UploadMgr_PrintInstallationInfo(void);
static void    UploadMgr_VfyDeleteIncomplete(void);

// Include cmd tables & function definitions here so that referenced functions
// in this module have already been forward declared.
#include "UploadMgrUserTables.c"

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:    UploadMgr_Init
 *
 * Description: Setup the initial state for the upload manager.
 *              Initialize two tasks, the Log Collection Task and the
 *              Log Upload Task.  Setup the File Data FIFO that queues the
 *              data gathered by the Log Collection task for transmission to
 *              the micro-server by the Log Upload task.
 *
 * Parameters:  void
 *
 * Returns:     void
 *
 * Notes:
 *
 *****************************************************************************/
void UploadMgr_Init(void)
{
  TCB task_info;
  INT32 i;
  UPLOADMGR_FILE_VFY vfy_row;

  User_AddRootCmd(&root_msg);
  ASSERT(SYS_OK == MSI_AddCmdHandler((UINT16)CMD_ID_VALIDATE_CRC,UploadMgr_MSCRCCmdHandler));

  //For the user cmd to get total rows,
  m_TotalVerifyRows = UPLOADMGR_VFY_TBL_MAX_ROWS;

  m_vfy_lookup_fn[0]    = '\0';
  m_vfy_lookup_idx      = -1;

  m_MSCrcResult = MS_CRC_RESULT_NONE;
  m_CheckLogRow = -1;
  m_CheckLogFileCnt = 0;
  m_MaintainFileTableRunning = FALSE;

  //Log Collection Task
  memset(&task_info,  0, sizeof(task_info));
  memset(&ULTaskData,0, sizeof(ULTaskData));
  memset(&LCTaskData,0, sizeof(LCTaskData));

  strncpy_safe(task_info.Name, sizeof(task_info.Name),"UL Log Collection",_TRUNCATE);
  task_info.TaskID         = UL_Log_Collection;
  task_info.Function       = UploadMgr_FileCollectionTask;
  task_info.Priority       = taskInfo[UL_Log_Collection].priority;
  task_info.Type           = taskInfo[UL_Log_Collection].taskType;
  task_info.modes          = taskInfo[UL_Log_Collection].modes;
  task_info.MIFrames       = taskInfo[UL_Log_Collection].MIFframes;
  task_info.Rmt.InitialMif = taskInfo[UL_Log_Collection].InitialMif;
  task_info.Rmt.MifRate    = taskInfo[UL_Log_Collection].MIFrate;
  task_info.Enabled        = FALSE;
  task_info.Locked         = FALSE;
  task_info.pParamBlock    = &LCTaskData;
  TmTaskCreate (&task_info);

  // Register the File-collection-In-Progress flag with the Power Manager.
  PmRegisterAppBusyFlag(PM_UPLOAD_LOG_COLLECTION_BUSY,&LCTaskData.InProgress,PM_BUSY_LEGACY);

  //Log Upload Task
  memset(&task_info, 0, sizeof(task_info));
  strncpy_safe(task_info.Name, sizeof(task_info.Name),"UL Log File Upload",_TRUNCATE);
  task_info.TaskID         = UL_Log_File_Upload;
  task_info.Function       = UploadMgr_FileUploadTask;
  task_info.Priority       = taskInfo[UL_Log_File_Upload].priority;
  task_info.Type           = taskInfo[UL_Log_File_Upload].taskType;
  task_info.modes          = taskInfo[UL_Log_File_Upload].modes;
  task_info.MIFrames       = taskInfo[UL_Log_File_Upload].MIFframes;
  task_info.Rmt.InitialMif = taskInfo[UL_Log_File_Upload].InitialMif;
  task_info.Rmt.MifRate    = taskInfo[UL_Log_File_Upload].MIFrate;
  task_info.Enabled        = FALSE;
  task_info.Locked         = FALSE;
  task_info.pParamBlock    = &ULTaskData;
  TmTaskCreate (&task_info);

  //Ground Verify confirmation
  memset(&task_info, 0, sizeof(task_info));
  strncpy_safe(task_info.Name, sizeof(task_info.Name),"UL Log Check",_TRUNCATE);
  task_info.TaskID         = UL_Log_Check;
  task_info.Function       = UploadMgr_CheckLogMoved;
  task_info.Priority       = taskInfo[UL_Log_Check].priority;
  task_info.Type           = taskInfo[UL_Log_Check].taskType;
  task_info.modes          = taskInfo[UL_Log_Check].modes;
  task_info.MIFrames       = taskInfo[UL_Log_Check].MIFframes;
  task_info.Rmt.InitialMif = taskInfo[UL_Log_Check].InitialMif;
  task_info.Rmt.MifRate    = taskInfo[UL_Log_Check].MIFrate;
  task_info.Enabled        = FALSE;
  task_info.Locked         = FALSE;
  task_info.pParamBlock    = NULL;
  TmTaskCreate (&task_info);

  FIFO_Init(&FileDataFIFO,FileDataBuffer,sizeof(FileDataBuffer));
  UploadMgr_OpenFileVfyTbl();
  // Register File Vfy Function for Log Erase
  LogRegisterEraseCleanup(UploadMgr_VfyDeleteIncomplete);

  m_FreeFVTRows = 0;
  //Scan the table for deleted slots.
  for(i = 0; i < UPLOADMGR_VFY_TBL_MAX_ROWS; i++)
  {
    NV_Read(NV_UPLOAD_VFY_TBL,
            VFY_ROW_TO_OFFSET(i),
            &vfy_row,
            sizeof(vfy_row));

    if( VFY_STA_DELETED == vfy_row.State )
    {
      m_FreeFVTRows += 1;
    }
  }
}

/******************************************************************************
 * Function:    UploadMgr_StartUpload
 *
 * Description: Initiate the upload process.  Starts the log file collection
 *              task which creates logical "files" from the log manager data
 *              and then transfers the data to the file upload process.
 *              UploadMgr must be enabled if it is in the disabled state.
 *              See SetUploadEnable
 *
 *              The 'AutoUpload' source will verify there are data logs to
 *              upload before proceeding, otherwise it will not start uploading
 *
 * Parameters:  [in] StartSource: What event caused the function to be called
 *                                to attempt to start an upload.
 *                                UPLOAD_STARTED_POST_FLIGHT
 *                                UPLOAD_STARTED_AUTO (only uploads if data
 *                                                     logs are present)
 *                                UPLOAD_STARTED_FORCED
 *
 * Returns:     void
 *
 * Notes:
 *
 *****************************************************************************/
void UploadMgr_StartUpload(UPLOAD_START_SOURCE StartSource)
{
  if(!LCTaskData.Disable)
  {
    // If recording is active, we can't upload... tell the user.
    if ( UPLOAD_START_FORCED == StartSource && FAST_FSMRecordGetState(NULL) )
    {
      GSE_DebugStr(NORMAL,TRUE, "UploadMgr: Cannot Upload while Recording is active");
    }
    else if(FALSE == LCTaskData.InProgress)
    {
      if( MSSC_GetIsAlive() && MSSC_GetIsCompactFlashMounted() )
      {
        if(UPLOAD_START_AUTO == StartSource)
        {
          LCTaskData.State = LOG_COLLECTION_INIT_AUTO;
          GSE_DebugStr(VERBOSE,TRUE,"UploadMgr: Starting auto-upload check");
          //Skip log write, only write log if the upload actually starts in
          //the LC task.
        }
        else
        {
          LogWriteSystem(APP_UPM_INFO_UPLOAD_START,LOG_PRIORITY_LOW,
            &StartSource,sizeof(StartSource),NULL);
          LCTaskData.State = LOG_COLLECTION_INIT;
        }
        LCTaskData.InProgress = TRUE;
        TmTaskEnable(UL_Log_Collection, TRUE);

      }
      else
      {
        GSE_DebugStr(NORMAL,TRUE,"UploadMgr: MS Not Ready or CF Not Mounted");
      }
    }
    else
    {
      GSE_DebugStr(NORMAL,TRUE,"UploadMgr: Upload already in progress");
    }
  }
}

/******************************************************************************
 * Function:    UploadMgr_SetUploadEnable
 *
 * Description: If FALSE: Disables the Upload Manager and terminates any upload
 *              in progress.  If a file is being transfered, the upload will be
 *              terminated after the current file is transfered.
 *              If TRUE: Clears the Disable flag.  The Upload can be started by
 *              calling StartUpload
 *
 * Parameters:  [in] Enable:  Enable or disable the upload in progress
 *
 * Returns:     void
 *
 * Notes:
 *
 *****************************************************************************/
void UploadMgr_SetUploadEnable(BOOLEAN Enable)
{

  //The purpose of predicating the ability to disable on InProgress or already
  //disabled is so the disabled flag cannot be set if the task is not actually
  //running.  The task must be running because the task clears the disable
  //flag after it acknowledges it and quits.  If the task is not running to clear
  //it then it gets 'stuck' on disabled.
  LCTaskData.Disable = (!Enable && LCTaskData.InProgress) ||
                       (LCTaskData.Disable && !Enable);
}

/******************************************************************************
 * Function:    UploadMgr_IsUploadInProgress
 *
 * Description: Returns the "InProgress" status of the Log Collection task,
 *              indicating the working/idle status of the Upload process.
 *
 *
 * Parameters:  void
 *
 * Returns:     TRUE:  Upload is currently working
 *              FALSE: Upload is idle
 * Notes:
 *
 *****************************************************************************/
BOOLEAN UploadMgr_IsUploadInProgress(void)
{
  return LCTaskData.InProgress;
}


/******************************************************************************
 * Function:    UploadMgr_WriteVfyTblRowsLog
 *
 * Description: Write a log containing the number of free rows in the
 *              file verification table.  Per FAST requirements, this log
 *              is to be written at the start of flight, however this function
 *              could be called at anytime system logs are allowed to be
 *              written
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:
 *
 *****************************************************************************/
void UploadMgr_WriteVfyTblRowsLog(void)
{
  UPLOAD_VFY_TABLE_ROWS_LOG VfyRowsLog;

  VfyRowsLog.total = UPLOADMGR_VFY_TBL_MAX_ROWS;
  VfyRowsLog.free  = m_FreeFVTRows;

  LogWriteSystem(APP_UPM_INFO_VFY_TBL_ROWS,LOG_PRIORITY_LOW,
      &VfyRowsLog,sizeof(VfyRowsLog),NULL);
}



/******************************************************************************
 * Function:    UploadMgr_GetFilesPendingRT
 *
 * Description: Return the number of files still in the FVT table waiting
 *              for round-trip verification to the ground server
 *
 * Parameters:  none
 *
 * Returns:     INT: Number of files still in the FVT table that are not
 *              deleted
 *
 * Notes:
 *
 *****************************************************************************/
UINT32 UploadMgr_GetFilesPendingRT(void)
{
  return UPLOADMGR_VFY_TBL_MAX_ROWS - m_FreeFVTRows;
}



/******************************************************************************
 * Function:    UploadMgr_DeleteFVTEntry
 *
 * Description: Delete an FVT entry by file name.
 *
 * Parameters:  [in] FN: Log filename of the file entry to delete.
 *
 * Returns:     BOOLEAN TRUE:  Found and deleted
 *                      FALSE: Entry not found or other error.
 *
 * Notes:
 *
 *****************************************************************************/
BOOLEAN UploadMgr_DeleteFVTEntry(const INT8* FN)
{
  INT32 row;
  BOOLEAN retval = FALSE;
  UPLOADMGR_FILE_VFY vfy_data;

  row = UploadMgr_GetFileVfy(FN,&vfy_data);

  if(row > -1)
  {
    UploadMgr_UpdateFileVfy(row,VFY_STA_DELETED,0,0);
    retval = TRUE;
  }
  return retval;
}



/******************************************************************************
 * Function:    UploadMgr_GetNumFilesPendingRT
 *
 * Description: Return the number of files pending round trip verification in
 *              the file verification table.
 *
 * Parameters:  none
 *
 * Returns:     INT32: Number of files in the verification table that are
 *                     in the MS_VFY state.
 *
 * Notes:
 *
 *****************************************************************************/
INT32 UploadMgr_GetNumFilesPendingRT(void)
{
  UINT32 i;
  INT32 msvfy = 0;
  UPLOADMGR_FILE_VFY vfy_row;

  for(i = 0; i < UPLOADMGR_VFY_TBL_MAX_ROWS; i++)
  {
    NV_Read(NV_UPLOAD_VFY_TBL,
            VFY_ROW_TO_OFFSET(i),
            &vfy_row,
            sizeof(vfy_row));

    if( VFY_STA_MSVFY == vfy_row.State )
    {
      msvfy++;
    }
  }
  return msvfy;
}


/******************************************************************************
 * Function:    UploadMgr_InitFileVfyTbl
 *
 * Description: Reinitializes the open file verification table to the default
 *              values.  This re-formats the structure in the file and will
 *              permanently erase any file verification data in that file.
 *
 * Parameters:  none
 *
 * Returns:     BOOLEAN: True (return type to satisfy NVMgr Init call sig.)
 *
 * Notes:
 *
 *****************************************************************************/
BOOLEAN UploadMgr_InitFileVfyTbl(void)
{
  UINT32 i;
  UINT32 magic_number = VFY_MAGIC_NUM;
  UINT32 startup_Id = 500000;
  UPLOADMGR_FILE_VFY vfy_row;

  //Clear the row structure.  Init non-zero members.
  memset(&vfy_row,0,sizeof(vfy_row));
  vfy_row.State      = VFY_STA_DELETED;
  vfy_row.LogDeleted = FALSE;
  vfy_row.Type       = LOG_TYPE_DONT_CARE;
  vfy_row.Source.acs = ACS_DONT_CARE;
  vfy_row.Priority   = LOG_PRIORITY_DONT_CARE;

  //First, write the table.  Then, once that is valid, write the magic number
  for(i = 0; i < UPLOADMGR_VFY_TBL_MAX_ROWS; i++)
  {
    STARTUP_ID(startup_Id++);
    NV_Write(NV_UPLOAD_VFY_TBL,
             VFY_ROW_TO_OFFSET(i),
             &vfy_row,
             sizeof(vfy_row));
  }

  NV_Write(NV_UPLOAD_VFY_TBL,
           VFY_MAGIC_NUM_OFFSET,
           &magic_number,
           sizeof(magic_number));


  GSE_DebugStr(NORMAL,TRUE,"UploadMgr: File Verification Table re-initialized");

  return TRUE;
}

/*****************************************************************************
 * Function:    UploadMgr_GenerateDebugLogs
 *
 * Parameters:  Log Write Test will write one copy of every log the Upload
 *              Manger generates.  The purpose if to test the log format
 *              for the team writing the log file decoding tools.  This
 *              code will be ifdef'd out of the flag GENERATE_SYS_LOGS is not
 *              defined.
 *              This function 3 example logs in the following order:
 *              APP_UPM_INFO_VFY_TBL_ROWS
 *              APP_UPM_INFO_UPLOAD_FILE_FAIL
 *              APP_UPM_INFO_UPLOAD_START
 *
 * Returns:
 *
 * Notes:
 *
 ****************************************************************************/
#ifdef GENERATE_SYS_LOGS
/*vcast_dont_instrument_start*/

void UploadMgr_GenerateDebugLogs(void)
{
  UPLOAD_VFY_TABLE_ROWS_LOG VfyRowsLog;
  UPLOAD_START_SOURCE StartSource = UPLOAD_START_UNKNOWN;

  VfyRowsLog.total = UPLOADMGR_VFY_TBL_MAX_ROWS;
  VfyRowsLog.free  = 100;

  LogWriteSystem(APP_UPM_INFO_VFY_TBL_ROWS,LOG_PRIORITY_LOW,
      &VfyRowsLog,sizeof(VfyRowsLog),NULL);
  LogWriteSystem(APP_UPM_INFO_UPLOAD_FILE_FAIL,LOG_PRIORITY_LOW,NULL,0,NULL);
  LogWriteSystem(APP_UPM_INFO_UPLOAD_START,LOG_PRIORITY_LOW,
      &StartSource,sizeof(StartSource),NULL);
}
/*vcast_dont_instrument_end*/

#endif //GENERATE_SYS_LOGS

/******************************************************************************
 * Function:     UploadMgr_FSMRun   | IMPLEMENTS Run() INTERFACE to
 *                                  | FAST STATE MGR
 *
 * Description:  Signals upload manager to start uploading files from the CP
 *               memory to the micro-server.  This function has the call
 *               signature required by the Fast State Manager's Task interface.
 *               Uses the "POSTFLIGHT" reason to indicate this wasn't a
 *               periodic auto upload that doesn't upload if there aren't
 *               data logs.
 *
 * Parameters:   [in] Run: TRUE Run the task
 *                         FALSE Stop the task
 *               [in] Param: Not used, for matching call sig only.
 *
 * Returns:      none
 *
 *****************************************************************************/
void UploadMgr_FSMRun( BOOLEAN Run, INT32 Param )
{
  if(Run)
  {
    UploadMgr_StartUpload(UPLOAD_START_POST_FLIGHT);
  }
  else
  {
    UploadMgr_SetUploadEnable(FALSE);
  }

}



/******************************************************************************
 * Function:     UploadMgr_FSMRunAuto | IMPLEMENTS Run() INTERFACE to
 *                                    | FAST STATE MGR
 *
 * Description:  Signals upload manager to start uploading files from the CP
 *               memory to the micro-server.  The Auto mode will check for
 *               data logs first before attempting an upload.  If no data logs
 *               are present then no upload will be performed.
 *               This function has the call signature required by the
 *               Fast State Manager's Task interface.
 *
 * Parameters:   [in] Run: TRUE Run the task
 *                         FALSE Stop the task
 *               [in] Param: Not used, for matching call sig only.
 *
 * Returns:      none
 *
 *****************************************************************************/
void UploadMgr_FSMRunAuto( BOOLEAN Run, INT32 Param )
{
  if(Run)
  {
    UploadMgr_StartUpload(UPLOAD_START_AUTO);
  }
  else
  {
    UploadMgr_SetUploadEnable(FALSE);
  }
}



/******************************************************************************
 * Function:     UploadMgr_FSMGetState   | IMPLEMENTS GetState() INTERFACE to
 *                                       | FSM
 *
 * Description:  Returns the upload is active/not active status.  The task is
 *               active after calling Run, until all data in the CP log memory
 *               is transfered or another task stops the upload process, or a
 *               fatal error occurs.
 *               This function has the call signature required by the
 *               Fast State Manager's Task interface.
 *
 * Parameters:   [in] Param: Not used, for matching call sig. only.
 *
 * Returns:
 *
 *****************************************************************************/
BOOLEAN UploadMgr_FSMGetState( INT32 param )
{
  return UploadMgr_IsUploadInProgress();
}



/******************************************************************************
 * Function:    UploadMgr_FSMGetFilesPendingRT()      |IMPLEMENTS GetState()
 *                                                    |FSM
 *
 * Description: Return if there are files pending round-trip verification.
 *              All files are round-trip verified when their entries are
 *              deleted from the verification table.  This is similar to
 *              checking to see if there any files on the Micro-Server
 *              Compact Flash card.
 *
 *
 * Parameters:  [in] param: Not used, to match FSM call signature.
 *
 * Returns:     BOOLEAN: TRUE: Files are pending round-trip verification
 *                       FALSE: There are no files pending round-trip
 *                              verification
 *
 * Notes:
 *
 *****************************************************************************/
BOOLEAN UploadMgr_FSMGetFilesPendingRT( INT32 param )
{
  return ((UPLOADMGR_VFY_TBL_MAX_ROWS - m_FreeFVTRows) != 0);
}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/
/******************************************************************************
 * Function:    UploadMgr_OpenFileVfyTbl
 *
 * Description: Opens the file verification table checks the file validity.
 *              If the file is invalid, it will re-initialize the verification
 *              table.
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:
 *
 *****************************************************************************/
static
void UploadMgr_OpenFileVfyTbl(void)
{
  UINT32 magic_number = (UINT32)~VFY_MAGIC_NUM; //Setup read location to NEQ MAGIC_NUM

  //Open the file and test the NVMgr result
  if(SYS_OK == NV_Open(NV_UPLOAD_VFY_TBL))
  {
    NV_Read(NV_UPLOAD_VFY_TBL,
            VFY_MAGIC_NUM_OFFSET,
            &magic_number,
            sizeof(magic_number));

    if(VFY_MAGIC_NUM == magic_number)
    {
      //File is initialized
      GSE_DebugStr(VERBOSE,FALSE,"UploadMgr: File Verification Table read OK!");
    }
    else
    {
      //File is not initialized
      GSE_DebugStr(NORMAL,TRUE,"UploadMgr: File Verification Table, bad magic number %08x",
               magic_number);

      UploadMgr_InitFileVfyTbl() ? 0 : 0, GSE_DebugStr(NORMAL,TRUE,"Upload: re-init failed");
    }
  }
  else
  {
    //NV_Open failed, file not initialized
    GSE_DebugStr(NORMAL,TRUE,"UploadMgr: File Verification Table, NV_Open() failed",
             magic_number);
    UploadMgr_InitFileVfyTbl() ? 0 : 0, GSE_DebugStr(NORMAL,TRUE,"Upload: re-init failed");
  }
}



/******************************************************************************
 * Function:    UploadMgr_DeleteIncomplete
 *
 * Description: Deletes any logs which are not marked log deleted. Used
 *              when a user commands a log erase.
 *
 * Parameters:  None
 *
 * Returns:     none
 *
 * Notes:
 *
 *****************************************************************************/
static void UploadMgr_VfyDeleteIncomplete(void)
{
  // Local Data
  UINT32             i;
  UPLOADMGR_FILE_VFY VfyRow;

  //First, write the table.  Then, once that is valid, write the magic number
  for( i = 0; i < UPLOADMGR_VFY_TBL_MAX_ROWS; i++ )
  {
    NV_Read(NV_UPLOAD_VFY_TBL,
            VFY_ROW_TO_OFFSET(i),
            &VfyRow,
            sizeof(VfyRow));

    // Check if already deleted from Data Flash
    if ( (FALSE == VfyRow.LogDeleted) && (VFY_STA_DELETED != VfyRow.State) )
    {
      // Mark the logs as deleted
      VfyRow.LogDeleted = TRUE;
      // Not Deleted from Data Flash so remove from Table
      NV_Write(NV_UPLOAD_VFY_TBL,
               VFY_ROW_TO_OFFSET(i),
               &VfyRow,
               sizeof(VfyRow) );
    }
  }
}


/******************************************************************************
 * Function:    UploadMgr_GetVfyDeletedRow
 *
 * Description: This routine searches for and returns the index of the first
 *              "deleted" file verification row in the file verification table.
 *              If an entry is found, the index of that entry is returned to the
 *              caller.
 *
 * Parameters:  none
 *
 * Returns:     >0: Index of the first row marked "deleted"
 *              -1: Could not find a deleted row
 *
 * Notes:       This call does not reserve the slot.  The calling task will
 *              need to reserve it.
 *
 *****************************************************************************/
static
INT32 UploadMgr_GetVfyDeletedRow(void)
{
  INT32 i;
  INT32 DeletedIdx = -1;
  UPLOADMGR_FILE_VFY VfyRow;

  //Scan the table for a deleted slot.
  for(i = 0; i < UPLOADMGR_VFY_TBL_MAX_ROWS; i++)
  {
    NV_Read(NV_UPLOAD_VFY_TBL,
            VFY_ROW_TO_OFFSET(i),
            &VfyRow,
            sizeof(VfyRow));

    if( VFY_STA_DELETED == VfyRow.State )
    {
      DeletedIdx = i;
      break;
    }
  }
  return DeletedIdx;
}

/******************************************************************************
 * Function:    UploadMgr_StartFileVfy()
 *
 * Description: This function writes file data to a "deleted" file verification
 *              slot.  The initial verification state for the file is "started"
 *
 * Parameters:  [in] FN: Pointer to file name string (AZCIIZ, < 63 chars)
 *              [in] Start: Log Manager start offset for this file data
 *              [in] LogType: Log Manger log type for this file
 *              [in] LogACS: Log Manager ACS type for this file
 *              [in] LogPri: Log Manager priority for this file
 *
 * Returns:     >0: Row number where the entry was written.
 *             -1: Failed to write the entry.  FN string too long, no deleted
 *                 rows found, or call to NV_ failed.
 *
 * Notes:
 *
 *****************************************************************************/
static
INT32 UploadMgr_StartFileVfy(INT8* FN, UINT32 Start,
                             LOG_TYPE Type, LOG_SOURCE Source,
                             LOG_PRIORITY Priority)
{
  UPLOADMGR_FILE_VFY VfyRow;
  INT32 Row;
  INT32 RetVal = -1;

  Row = UploadMgr_GetVfyDeletedRow();

  if(Row >= 0)
  {
    ASSERT_MESSAGE((strlen(FN) < sizeof(VfyRow.Name)-1),
                    "File name length too long: %d", strlen(FN));
    //Copy caller's data to structure
    strncpy_safe(VfyRow.Name, sizeof(VfyRow.Name),FN,_TRUNCATE);
    VfyRow.State      = VFY_STA_STARTED;
    VfyRow.LogDeleted = FALSE;
    VfyRow.Start      = Start;
    VfyRow.End        = 0; //Set when updated to COMPLETED
    VfyRow.Type       = Type;
    VfyRow.Source     = Source;
    VfyRow.Priority   = Priority;
    VfyRow.CRC        = 0; //Set when updated to COMPLETED

    //Write the data to the table
    NV_Write(NV_UPLOAD_VFY_TBL,VFY_ROW_TO_OFFSET(Row),&VfyRow,sizeof(VfyRow));
    //Decrement free rows.
    m_FreeFVTRows -= 1;

    RetVal = Row;
  }
  return RetVal;
}


/******************************************************************************
 * Function:    UploadMgr_UpdateFileVfy()
 *
 * Description: Update the state of a file in the file verification table.
 *              The CRC and End parameter is only used by this function when
 *              changing from the "started" to the "completed" state.
 *              This function only allows the verify state to be increased.
 *              Setting a new state of lower value than the current state will
 *              have no effect.
 *
 *              Reference by row number is used to save the processing time,
 *              vs. lookup by file name.  If the caller needs to convert a file
 *              name string to a row number, use GetFileVfy which will return
 *              the row number of a particular file name.
 *
 * Parameters:  [in] Row: Verify table row to update
 *              [in] NewState: The new verification state of this file
 *              [in] CRC: CRC of the file, used only when changing from the
 *                        started to completed state.  For other state changes,
 *                        the file is ignored.
 *              [in] End: Log Manager end offset for this file, used only when
 *                        changing from the started to completed state.
 *
 * Returns:      0: Update succeeded
 *              -1: Updated failed. NV call failed, Row# out of bounds
 *
 * Notes:
 *
 *****************************************************************************/
static
void UploadMgr_UpdateFileVfy(INT32 Row,UPLOADMGR_VFY_STATE NewState,
                              UINT32 CRC, UINT32 End)
{
  UPLOADMGR_FILE_VFY  vfy_row;
  INT8 prev_state;

  ASSERT((Row < UPLOADMGR_VFY_TBL_MAX_ROWS) && (Row >= 0));

  NV_Read(NV_UPLOAD_VFY_TBL,VFY_ROW_TO_OFFSET(Row),&vfy_row,sizeof(vfy_row));
  prev_state    = vfy_row.State;

  //If new state is higher priority, write the new state
  if(NewState > vfy_row.State)
  {
    vfy_row.State = NewState;
    NV_Write(NV_UPLOAD_VFY_TBL,
        VFY_ROW_TO_OFFSET(Row),
        &vfy_row,
        sizeof(vfy_row));
  }
  //If the new state is COMPLETED, write the file CRC and end offset.
  if(VFY_STA_COMPLETED == NewState)
  {
    vfy_row.CRC = CRC;
    vfy_row.End = End;
    NV_Write(NV_UPLOAD_VFY_TBL,
        VFY_ROW_TO_OFFSET(Row),
        &vfy_row,
        sizeof(vfy_row));
  }
  /*Update free rows.
    If a row is !deleted->deleted, increment free rows
    Rows only go from deleted->!deleted in UploadMgr_StartFileVfy*/

  if((prev_state != VFY_STA_DELETED) && (NewState == VFY_STA_DELETED))
  {
    m_FreeFVTRows += 1;
  }
}


/******************************************************************************
 * Function:    UploadMgr_GetFileVfy()
 *
 * Description: Get the data row associated with a particular file name.
 *
 * Parameters:  [in]  FN:     Filename string
 *              [out] VfyRow: Pointer to a location to store the file's
 *                            verification data.
 * Returns:    <0: Filename found, return value is the row index of the file.
 *             -1: Find failed.  FN string not matched or an NV_ call failed.
 *
 *
 * Notes:
 *
 *****************************************************************************/
static
INT32 UploadMgr_GetFileVfy(const INT8* FN, UPLOADMGR_FILE_VFY* VfyRow)
{
  INT32 i;
  INT32 row = -1 ;

  //Filename search.
  for( i = 0; (i < UPLOADMGR_VFY_TBL_MAX_ROWS) && (row == -1); i++)
  {
    NV_Read(NV_UPLOAD_VFY_TBL,VFY_ROW_TO_OFFSET(i),VfyRow,sizeof(*VfyRow));
    if( 0 == strncmp(FN, VfyRow->Name, sizeof(VfyRow->Name)))
    {
      row = i;
    }
  }

  return row;
}

/******************************************************************************
 * Function:    UploadMgr_FileCollectionTask   |   TASK
 *
 * Description: Processes log data returned from the Log Manager.  Searches
 *              logs by flight (delimited by end of flight)
 *              and source ((SYSTEM/APPLICATION)(ACS)) and collects the data.
 *              into files that are packaged with headers and sent to the
 *              file upload task.
 *
 * Parameters:  [in/out] pParam: Task Manager task parameter pointer.  This
 *                               is expected to be
 *                               UPLOADMGR_LOG_COLLECTION_TASK_DATA.
 *
 * Returns:     none
 *
 * Notes:       Function exceeds cyclomatic and nesting complexity
 *
 *****************************************************************************/
static
void UploadMgr_FileCollectionTask(void *pParam)
{
  UPLOADMGR_LOG_COLLECTION_TASK_DATA* pTask = pParam;
  LOG_FIND_STATUS     findLogResult;
  LOG_READ_STATUS     readLogResult;
  TIMESTAMP           timestamp;
  static UINT32       readOffset;
  static UINT32       tickLast;
  static INT32        checkFileCnt;
  INT32               i;
  MSCP_CHECK_FILE_CMD checkCmdData;
  LOG_SOURCE          source;
  LOG_FIND_STATUS     result;
  UPLOAD_START_SOURCE startSource;
  LOG_TYPE types[] = {LOG_TYPE_DATA,LOG_TYPE_ETM};

  switch (pTask->State)
  {
    //Start the Log Collection process only if there are
    //data logs stored in the log memory.  If there are no
    //log or only system logs then do not start.
    case LOG_COLLECTION_INIT_AUTO:

      source.nNumber = LOG_SOURCE_DONT_CARE;
      //Search for any records, break every 500ms to allow lower tasks to run.
      //This allows a search of about 10k logs/frame depending on loading of
      //higher priority tasks.
      result = LogFindNextRecordEx(LOG_NEW ,
                                   types,
                                   2,
                                   source,
                                   LOG_PRIORITY_DONT_CARE,
                                   &readOffset,
                                   LOG_NEXT,
                                   &pTask->LogOffset,
                                   &pTask->LogSize,500);
      if(LOG_FOUND == result)
      {
        GSE_DebugStr(VERBOSE,TRUE,"UploadMgr: Auto upload found data/etm log");
        startSource = UPLOAD_START_AUTO;
        pTask->State = LOG_COLLECTION_INIT;
        LogWriteSystem(APP_UPM_INFO_UPLOAD_START,LOG_PRIORITY_LOW,
            &startSource,sizeof(startSource),NULL);
      }
      else if(LOG_NOT_FOUND == result)
      {
        GSE_DebugStr(VERBOSE,TRUE,
                     "UploadMgr: Auto upload, no data/etm logs found, stopping");
        pTask->State = LOG_COLLECTION_AUTO_CHECK_FINISH;
      }
      break;

    //Start the Log Collection process.  Reset log offset
    //to the beginning of log memory.  Find the start and end of
    //log data.
    case LOG_COLLECTION_INIT:
      //Be sure the Data Manager has finished recording to FLASH before proceding with upload.
      //This is necessary to make sure the data files uploaded contain complete data for the
      //recording that just happened.
      if( !DataMgrDownloadingACS() && DataMgrRecordFinished() )
      {
        GSE_DebugStr(NORMAL,TRUE,"UploadMgr: Starting log upload process");

        pTask->FlightDataStart = 0;
        pTask->FlightDataEnd = 0;
        pTask->FileFailCnt = 0;
        pTask->FileUploadedCnt = 0;
        pTask->FileNotUploadedCnt = 0;
        pTask->InProgress = TRUE;
        readOffset = 0;
        //Determine if there are any new logs to transfer and if so,
        //check existing file status and transfer new files
        //
        //If no logs are found initiate last step of the transfer,
        //checking file status and the erasure of log data.

        UploadMgr_MaintainFileTable(TRUE);
        pTask->State = LogGetLogCount() == 0 ?
          LOG_COLLECTION_POST_MAINTENANCE : LOG_COLLECTION_PRE_MAINTENANCE;

        GSE_DebugStr(VERBOSE,FALSE,LogGetLogCount() == 0 ?
        "UploadMgr: LC INIT-FindRec=NOT_FOUND" : "UploadMgr: LC INIT-FindRec=FOUND");
      }
      pTask->State = pTask->Disable ?
                    GSE_DebugStr(NORMAL,TRUE,
                    "UploadMgr: Canceled while waiting for recording to finish"),
                    LOG_COLLECTION_FINISHED :
                    pTask->State;
      break;

    case LOG_COLLECTION_PRE_MAINTENANCE:
      //MaintainFileTable() return 0 while in progress.  When it returns 1,
      //continue to the next task.  -1 indicates an error occurred, so quit
      //upload.
      i = UploadMgr_MaintainFileTable(FALSE);
      if(1 == i )
      {
        pTask->State = LOG_COLLECTION_WAIT_EOF_LOGGED;
        // Display installation info to the version
        UploadMgr_PrintInstallationInfo();

      }
      else if(-1 == i)
      {
        GSE_DebugStr(NORMAL,TRUE,"UploadMgr: FVT maintenance failed, upload stopping");
        pTask->State = LOG_COLLECTION_FINISHED;
      }
      break;


    // Check with the LogMgr to see if the end-of-flight marker-log
    // is being written out, before attempting to collect the flight records.
    case LOG_COLLECTION_WAIT_EOF_LOGGED:
      if ( LogIsWriteComplete(LOG_REGISTER_END_FLIGHT) )
      {
        pTask->State = LOG_COLLECTION_FINDFLIGHT;
        //GSE_DebugStr(NORMAL,TRUE,"UploadMgr: No EOF Log pending... continuing Upload");
      }
      break;


    //Find a new flight in memory to upload.  If the Log Manager
    //find log routine returns busy, keep trying until it is free.
    //If no end of flight logs are found, send the end pointer to
    //the end of log data.
    case LOG_COLLECTION_FINDFLIGHT:
      pTask->UploadOrderIdx = 0;
      findLogResult = UploadMgr_FindFlight(&pTask->FlightDataStart,
                                           &pTask->FlightDataEnd);
      if((LOG_FOUND == findLogResult))
      {
        GSE_DebugStr(NORMAL,TRUE,"UploadMgr: Reading flight data from %x to %x",
                pTask->FlightDataStart, pTask->FlightDataEnd);
        pTask->State = LOG_COLLECTION_STARTFILE;
      }
      else if((LOG_NOT_FOUND == findLogResult))
      {
        //Upload process has reached the end of flight data memory.
        //Start the file maintenance task to wait for MSSIM to verify all
        //uploaded data.
        GSE_DebugStr(VERBOSE,FALSE, "UploadMgr: LC FINDFLIGHT-FindRec=NOT_FOUND");

        UploadMgr_MaintainFileTable(TRUE);
        pTask->State = LOG_COLLECTION_POST_MAINTENANCE;
      }
      (findLogResult == LOG_BUSY) ?
        GSE_DebugStr(VERBOSE,FALSE,"UploadMgr: LC FINDFLIGHT-FindRec=BUSY"),
        pTask->State = LOG_COLLECTION_FINDFLIGHT : 0;

      break;


    //Start collection of a new file. Clear the file header structure
    //and set the next state to find logs
    case LOG_COLLECTION_STARTFILE:
      if(!pTask->Disable)
      {
        memset(&pTask->FileHeader,0,sizeof(pTask->FileHeader));
        pTask->logUploadCnt = 0;
        readOffset = pTask->FlightDataStart;
        pTask->State = LOG_COLLECTION_LOG_HEADER_FIND;
        m_MSCrcResult = MS_CRC_RESULT_NONE;
      }
      else
      {
        pTask->State = LOG_COLLECTION_FINISHED;
      }
      break;

    //Find logs matching the current Type, ACS, and priority in this
    //flight.  When a log is found, read it and add its checksum to the
    //file checksum value.  If the Log Manager is busy, try reading
    //on the next frame
    case LOG_COLLECTION_LOG_HEADER_FIND:
      for(i = 0; i < UPLOADMGR_READ_LOGS_PER_MIF; i++)
      {
        findLogResult = LogFindNextRecord(LOG_NEW,
                                   UploadOrderTbl[pTask->UploadOrderIdx].Type,
                                   UploadOrderTbl[pTask->UploadOrderIdx].ACS,
                                   UploadOrderTbl[pTask->UploadOrderIdx].Priority,
                                   &readOffset,
                                   pTask->FlightDataEnd,
                                   &pTask->LogOffset,
                                   &pTask->LogSize);

        switch (findLogResult)
        {
          case LOG_BUSY:
            GSE_DebugStr(VERBOSE,FALSE,"UploadMgr: LC HEADER_FIND_LOG-FindRec=BUSY");
            pTask->State = LOG_COLLECTION_LOG_HEADER_FIND;
            break;

          case LOG_FOUND:
            //For the first round through, print out a message that indicates
            //a new file is being started
            if(pTask->FileHeader.log_cnt == 0)
            {
              pTask->logUploadCnt = 0;
              GSE_StatusStr(NORMAL,"\r\n");
              GSE_DebugStr(NORMAL,TRUE,"UploadMgr: Calculating log file header...");
              //Start timer display
              //Probably not needed anymore since ul now builds log headers 100x faster
              //UTIL_MinuteTimerDisplay(TRUE);
            }

            //Tick timer display to indicate the collection is working
            //UTIL_MinuteTimerDisplay(FALSE);
            GSE_DebugStr(VERBOSE,FALSE,"UploadMgr: LC HEADER_FIND_LOG-FindRec=FOUND@ %x %db",
               pTask->LogOffset,pTask->LogSize);

            pTask->FileHeader.data_size += pTask->LogSize;
            //Found log, try reading it
            readLogResult = LogRead(pTask->LogOffset,(LOG_BUF*) pTask->LogBuf);

            if(LOG_READ_BUSY == readLogResult)
            {
              GSE_DebugStr(VERBOSE,FALSE,"UploadMgr: LC HEADER_FIND_LOG-LogRead=BUSY");
              pTask->State = LOG_COLLECTION_LOG_HEADER_READ;
            }
            else if(LOG_READ_SUCCESS == readLogResult)
            {

              GSE_DebugStr(VERBOSE,FALSE,"UploadMgr: LC HEADER_FIND_LOG-LogRead=SUCCESS");
              pTask->FileHeader.check_sum += ChecksumBuffer( pTask->LogBuf,
                                                           pTask->LogSize,
                                                           0xFFFFFFFF );
              pTask->logUploadCnt += 1;
              
              if(LOG_TYPE_ETM != UploadOrderTbl[pTask->UploadOrderIdx].Type)
              {
                pTask->FileHeader.log_cnt += 1;
              }
              else // This is an ETM LOG; check if Time History
              {
                if(APP_ID_TIMEHISTORY_ETM_LOG == ((SYS_LOG_HEADER*)pTask->LogBuf)->nID )
                {
                  // Determine the number of TH logs and increment count...
                  // Divde the total size of the data read by the size of a
                  // single TH-rec (header+data) to get # TH recs
                  pTask->FileHeader.log_cnt += pTask->LogSize /
                                      (((SYS_LOG_HEADER*)pTask->LogBuf)->nSize +
                                      sizeof(SYS_LOG_HEADER));
                }
                else
                {
                   pTask->FileHeader.log_cnt += 1;
                }
              }
              pTask->State = LOG_COLLECTION_LOG_HEADER_FIND;
            }

            (readLogResult == LOG_READ_FAIL) ? GSE_DebugStr(NORMAL,TRUE,
                  "UploadMgr: HEADER_FIND_LOG-LogRead=FAILED, skip to next file")
                  ,pTask->State = LOG_COLLECTION_NEXTFILE : 0;
            break;

          case LOG_NOT_FOUND:
            //No more logs found for this file, start the upload!
            pTask->State = LOG_COLLECTION_START_LOG_UPLOAD;
            GSE_DebugStr(VERBOSE,FALSE,
                "UploadMgr: LC HEADER_FIND_LOG-FindRec=NOT_FOUND");
            break;
        }
        //Exit log read loop if log not found or log mgr is busy and try on the next frame
        if((findLogResult != LOG_FOUND) || (readLogResult != LOG_READ_SUCCESS))
        {
          break;
        }
      }
      break;

    //Read a found log and add its checksum to the header.  This read
    //is only used if ReadLog returns a "BUSY" status in the
    //LOG_COLLECTION_LOG_HEADER_FIND_LOG state.
    case LOG_COLLECTION_LOG_HEADER_READ:
      readLogResult = LogRead(pTask->LogOffset,(LOG_BUF *) pTask->LogBuf);
      if(LOG_READ_BUSY == readLogResult)
      {
        GSE_DebugStr(VERBOSE,FALSE,"UploadMgr: LC HEADER_READ_LOG-ReadLog=BUSY");
        pTask->State = LOG_COLLECTION_LOG_HEADER_READ;
      }
      else if(LOG_READ_SUCCESS == readLogResult)
      {
        GSE_DebugStr(VERBOSE,FALSE,"UploadMgr: LC HEADER_READ_LOG-ReadLog=SUCCESS");
        pTask->FileHeader.check_sum += ChecksumBuffer( pTask->LogBuf,
                                                     pTask->LogSize,
                                                     0xFFFFFFFF );
        
        pTask->logUploadCnt += 1;
        
        if (LOG_TYPE_ETM != UploadOrderTbl[pTask->UploadOrderIdx].Type)
        {
          pTask->FileHeader.log_cnt += 1;
        }
        else // ETM LOG; Check if Time History
        {
          if(APP_ID_TIMEHISTORY_ETM_LOG == ((SYS_LOG_HEADER*)pTask->LogBuf)->nID )
          {
            // Determine the number of TH logs and increment count...
            // Divde the total size of the data read by the size of a
            // single TH-rec (header+data) to get # TH recs
            pTask->FileHeader.log_cnt += pTask->LogSize /
                                         (((SYS_LOG_HEADER*)pTask->LogBuf)->nSize +
                                          sizeof(SYS_LOG_HEADER));
          }
          else
          {
            pTask->FileHeader.log_cnt += 1;
          }
        }
        pTask->State = LOG_COLLECTION_LOG_HEADER_FIND;
      }
      //LogReadFail (Shouldn't happen, but if it does then the log memory
      //have been corrupted. I'm scared of asserting here, if I just skip
      //the bad data at least other data will be transfered.
      //Skip file until next upload.
      (readLogResult == LOG_READ_FAIL) ? GSE_DebugStr(NORMAL,TRUE,
          "UploadMgr: HEADER_READ_LOG-ReadLog FAIL, skip to next file")
          ,pTask->State = LOG_COLLECTION_NEXTFILE : 0;
      break;

    case LOG_COLLECTION_START_LOG_UPLOAD:
      //If no logs were collected for this file, move onto the next file
      if(0 == pTask->logUploadCnt)
      {
        pTask->State = LOG_COLLECTION_NEXTFILE;
      }
      else
      {
        //DTU Serial Number
        INT8   boxStr[BOX_INFO_STR_LEN];
        CHAR   fileName[SYS_ETM_NAME_LEN];
        Box_GetSerno(boxStr);

        CM_GetTimeAsTimestamp(&timestamp);

        //Create a file name for the system log with a "SYS" string
        //or create a file name based on the ACS.ID
        if(LOG_TYPE_SYSTEM == UploadOrderTbl[pTask->UploadOrderIdx].Type)
        {
            FAST_GetSYSName(fileName);
            snprintf(pTask->FN, sizeof(pTask->FN), "FAST-SYS%s-%s-%d.dtu",
                     fileName, boxStr, timestamp.Timestamp);
        }
        else if (LOG_TYPE_ETM == UploadOrderTbl[pTask->UploadOrderIdx].Type)
        {
            FAST_GetETMName(fileName);
            snprintf(pTask->FN, sizeof(pTask->FN), "FAST-ETM%s-%s-%d.dtu",
                     fileName, boxStr, timestamp.Timestamp);
        }
        else
        {
          snprintf(pTask->FN, sizeof(pTask->FN), "FAST-%s-%s-%d.dtu",
            DataMgrGetACSDataSet(UploadOrderTbl[pTask->UploadOrderIdx].ACS.acs).sID,
            boxStr, timestamp.Timestamp);
        }
        //Start a File Verification Entry in the Verification Table. Once the
        //entry is created, start the upload process.
        pTask->VfyRow = UploadMgr_StartFileVfy(pTask->FN,pTask->FlightDataStart,
                                UploadOrderTbl[pTask->UploadOrderIdx].Type,
                                UploadOrderTbl[pTask->UploadOrderIdx].ACS,
                                UploadOrderTbl[pTask->UploadOrderIdx].Priority);

        if(pTask->VfyRow >= 0)
        {
          //Start the upload task and create the file header
          UploadMgr_StartFileUploadTask(pTask->FN, 
                                        UploadOrderTbl[pTask->UploadOrderIdx].Priority);

          UploadMgr_MakeFileHeader(&pTask->FileHeader,
              UploadOrderTbl[pTask->UploadOrderIdx].Type,
              UploadOrderTbl[pTask->UploadOrderIdx].Priority,
              UploadOrderTbl[pTask->UploadOrderIdx].ACS,
              &timestamp);

          //Put file data into the queue and start the CRC computation.
          //Q is empty at this point, so no concern about overflow.
          FIFO_PushBlock(&FileDataFIFO,&pTask->FileHeader,
              sizeof(pTask->FileHeader));

          FIFO_PushBlock(&FileDataFIFO, BinarySubHdr.buf, BinarySubHdr.size_used);

          CRC32(&pTask->FileHeader,sizeof(pTask->FileHeader),&pTask->VfyCRC,
                CRC_FUNC_START);

          CRC32(BinarySubHdr.buf, BinarySubHdr.size_used, &pTask->VfyCRC,
                CRC_FUNC_NEXT);

          //The next task state will transfer the log data for this file
          //into the file data queue.
          readOffset = pTask->FlightDataStart;
          pTask->State = LOG_COLLECTION_UPLOAD_FIND_LOG;

          GSE_DebugStr(NORMAL,TRUE,"UploadMgr: Uploading %s...",pTask->FN);

          GSE_DebugStr(NORMAL,FALSE,"  %d logs, %d B, Vfy Tbl Row %d",
                   pTask->FileHeader.log_cnt,
                   pTask->FileHeader.data_size,
                   pTask->VfyRow);
        }
        else
        {
          GSE_DebugStr(NORMAL, TRUE,
            "UploadMgr: File Verification Table full, cannot transfer files!");
          //Stop immediatly, no error retries
          UploadMgr_MaintainFileTable(TRUE);
          pTask->State = LOG_COLLECTION_POST_MAINTENANCE;
        }
      }
      break;

    //Transfer log data to the upload task.  Find a log, read it, and
    //stuff it into the queue.  If the log manager is busy on a find
    //or read, come back on the next frame to finish.
    case LOG_COLLECTION_UPLOAD_FIND_LOG:
      //Read logs until all logs are read, or the FIFO is full
      while(pTask->State == LOG_COLLECTION_UPLOAD_FIND_LOG)
      {
        (ULTaskData.State == FILE_UL_ERROR) ?
           UploadMgr_UpdateFileVfy(pTask->VfyRow, VFY_STA_DELETED, 0, 0),
           pTask->State = LOG_COLLECTION_FILE_UL_ERROR : 0;

        if(pTask->State != LOG_COLLECTION_FILE_UL_ERROR)
        {
          findLogResult = LogFindNextRecord(LOG_NEW ,
              UploadOrderTbl[pTask->UploadOrderIdx].Type,
              UploadOrderTbl[pTask->UploadOrderIdx].ACS,
              UploadOrderTbl[pTask->UploadOrderIdx].Priority,
              &readOffset,
              pTask->FlightDataEnd,
              &pTask->LogOffset,
              &pTask->LogSize);
          if(findLogResult == LOG_BUSY)
          {
            GSE_DebugStr(VERBOSE,FALSE,"UploadMgr: LC UPLOAD_FIND_LOG-FindLog=BUSY");
            pTask->State = LOG_COLLECTION_UPLOAD_FIND_LOG;
          }
          else if(findLogResult == LOG_FOUND)
          {
            //Found log, try reading it
            //Add data to the file queue if room is available,
            //else wait until room is available
            GSE_DebugStr(VERBOSE,FALSE,"UploadMgr: LC UPLOAD_FIND_LOG-FindLog=FOUND");
            if(FIFO_FreeBytes(&FileDataFIFO) > pTask->LogSize)
            {
              readLogResult = LogRead(pTask->LogOffset, (LOG_BUF *) pTask->LogBuf);
              //A log that was read just a minute ago is now corrupt? Something
              //is very wrong
              ASSERT(readLogResult != LOG_READ_FAIL);
              if(LOG_READ_BUSY == readLogResult)
              {
                GSE_DebugStr(VERBOSE,FALSE,"UploadMgr: LC UPLOAD_FIND_LOG-ReadLog=BUSY");
                pTask->State = LOG_COLLECTION_UPLOAD_READ_LOG;
              }
              else
              {
                GSE_DebugStr(VERBOSE,FALSE,"UploadMgr: LC UPLOAD_FIND_LOG-ReadLog=SUCCESS");

                //Ignore return, .
                FIFO_PushBlock(&FileDataFIFO,pTask->LogBuf,pTask->LogSize);
                CRC32(pTask->LogBuf,pTask->LogSize,&pTask->VfyCRC,
                      CRC_FUNC_NEXT);
                //Ensure the number of logs in this file is the same as indicated
                //in the header.  If a log is written while uploading, this will
                //ensure it is put in a different file.
                if(--pTask->logUploadCnt != 0)
                {
                  pTask->State = LOG_COLLECTION_UPLOAD_FIND_LOG;
                }
                else
                {
                  //All logs transfered to the Upload pTask.  Signal
                  //the task to finish and wait.
                  UploadMgr_FinishFileUpload(pTask->VfyRow, pTask->VfyCRC,
                           pTask->LogOffset);
                  pTask->State = LOG_COLLECTION_FINISHFILE;
                }
              }
            }
            else
            {
              pTask->State = LOG_COLLECTION_UPLOAD_READ_LOG;
            }
          }
          //No more logs found, but the number of logs transfered is less
          //than the same as the number of logs recorded in the file header.
          //Maybe someone issued log.erase during an upload?
          (findLogResult == LOG_NOT_FOUND) ?
              GSE_DebugStr(VERBOSE,FALSE,
                  "UploadMgr: LC UPLOAD_FIND_LOG-FindLog=NOT_FOUND"),
            UploadMgr_FinishFileUpload(pTask->VfyRow, pTask->VfyCRC,
                           pTask->LogOffset),
            pTask->State = LOG_COLLECTION_FINISHFILE : 0;

          }
        }
      break;

    //Read log data from log manager and transfer it into the file upload
    //queue.  This task is only used if
    case LOG_COLLECTION_UPLOAD_READ_LOG:

      (ULTaskData.State == FILE_UL_ERROR) ?
           UploadMgr_UpdateFileVfy(pTask->VfyRow, VFY_STA_DELETED, 0, 0),
           pTask->State = LOG_COLLECTION_FILE_UL_ERROR : 0;

      if(pTask->State != LOG_COLLECTION_FILE_UL_ERROR)
      {
        if(FIFO_FreeBytes(&FileDataFIFO) > pTask->LogSize)
        {
          readLogResult = LogRead(pTask->LogOffset, (LOG_BUF *) pTask->LogBuf);
          //A log that was read just a minute ago is now corrupt? Something
          //is very wrong
          ASSERT(readLogResult != LOG_READ_FAIL);
          if(LOG_READ_BUSY == readLogResult)
          {
            GSE_DebugStr(VERBOSE,FALSE,"UploadMgr: LC UPLOAD_READ_LOG-ReadLog=BUSY");
            //Repeat state until Log Mgr is no longer busy
            pTask->State = LOG_COLLECTION_UPLOAD_READ_LOG;
          }
          else
          {
            //Add the data to the read queue and switch state to searching
            //for logs.
            GSE_DebugStr(VERBOSE,FALSE,"UploadMgr: LC UPLOAD_READ_LOG-ReadLog=SUCCESS");
            FIFO_PushBlock(&FileDataFIFO,pTask->LogBuf,pTask->LogSize);
            CRC32(pTask->LogBuf,pTask->LogSize,&pTask->VfyCRC,
                  CRC_FUNC_NEXT);
            //Ensure the number of logs in this file is the same as indicated
            //in the header.  If a log is written while uploading, this will
            //ensure it is put in a different file.
            if(--pTask->logUploadCnt != 0)
            {
              pTask->State = LOG_COLLECTION_UPLOAD_FIND_LOG;
            }
            else
            {
              //All logs transfered to the Upload pTask.  Signal
              //the task to finish and wait.  Preset
              UploadMgr_FinishFileUpload(pTask->VfyRow, pTask->VfyCRC,
                           pTask->LogOffset);
              m_MSCrcResult = MS_CRC_RESULT_NONE;
              pTask->State = LOG_COLLECTION_FINISHFILE;
            }
          }
        }
        else //File queue full, wait until U/L process frees up some room
        {
          pTask->State = LOG_COLLECTION_UPLOAD_READ_LOG;
        }
      }
      break;

    //Wait for the upload task to finish upload
    case LOG_COLLECTION_FINISHFILE:
      if(FILE_UL_FINISHED == ULTaskData.State)
      {
        //File transfer has successfully completed.
        tickLast = CM_GetTickCount();
        pTask->State = LOG_COLLECTION_WT_FOR_MS_TO_VFY;
      }
      (ULTaskData.State == FILE_UL_ERROR) ?
           UploadMgr_UpdateFileVfy(pTask->VfyRow, VFY_STA_DELETED, 0, 0),
           pTask->State = LOG_COLLECTION_FILE_UL_ERROR : 0;

      break;

    /*Wait for MSSIM to finish calculating the file CRC and send the VerifyCRC
      command.  MSSIM cannot receive the next file until it is done
      processing this one.  TODO: MSSIM handling multiple file simultaneously
      is a potential future enhancement.*/
    case LOG_COLLECTION_WT_FOR_MS_TO_VFY:
      if(MS_CRC_RESULT_VFY == m_MSCrcResult)
      {
        //If CRC matches, verify the file was moved to the CP_FILES folder
        //(where ms_vfy'd files are stored).
        VfyRspId = CMD_ID_NONE;
        VfyRspFailed = FALSE;
        GSE_DebugStr(NORMAL,TRUE,"UploadMgr: CRC OK!, verify log file was moved...");
        strncpy_safe(checkCmdData.FN,sizeof(checkCmdData.FN),pTask->FN,_TRUNCATE);
        pTask->State = 
        MSI_PutCommand(CMD_ID_CHECK_FILE,&checkCmdData,sizeof(checkCmdData),
                       CHECK_FILE_TIMEOUT, 
            UploadMgr_VerifyMSCmdsCallback)  == SYS_OK ?
            LOG_COLLECTION_VFY_MS_MOVED_FILE : LOG_COLLECTION_FILE_UL_ERROR;
        tickLast = CM_GetTickCount();
        checkFileCnt = UPLOADMGR_CHECK_FILE_CNT;
      }
      else if(MS_CRC_RESULT_FAIL == m_MSCrcResult)
      {
        //If CRC failed, it will be deleted and re-uploaded on the next cycles
        pTask->State = LOG_COLLECTION_FILE_UL_ERROR;

      }
      else
      {
        if( (CM_GetTickCount() - tickLast) > UPLOADMGR_MS_VFY_TIMEOUT )
        {
          pTask->State = LOG_COLLECTION_FILE_UL_ERROR;
          GSE_DebugStr(NORMAL, FALSE,
            "UploadMgr LC: Timeout waiting for CRC from Micro-Server, moving onto next file");
        }
      }
      break;

    /*After receiving a good CRC for the log file, verify the file is moved to
      the CP_FILES folder by MSSIM.  They mv command is typically fast (< 1s) and
      not dependant on file size.  Once this is verified, then update the file
      verification table.  If the file was not moved*/
    case LOG_COLLECTION_VFY_MS_MOVED_FILE:
      if(VfyRspId == CMD_ID_CHECK_FILE)
      { //Use > MSVALID instead of == because it is remotely possible the file can be
        //GS verified or failed in the time we are checking the file.
        if(CheckFileRsp.FileSta >= MSCP_FILE_STA_MSVALID)
        {
          //Verified, update entry to verified on CF for priority 1,2,3,4
          UploadMgr_UpdateFileVfy(pTask->VfyRow,VFY_STA_MSVFY,0,0);
          GSE_StatusStr(NORMAL,"Done!");
          pTask->FileFailCnt = 0;
          pTask->FileUploadedCnt++;
          GSE_DebugStr(NORMAL,TRUE,"UploadMgr: Upload Complete");
          pTask->State = LOG_COLLECTION_NEXTFILE;
        }
        else
        {
          //MS Says the file hasn't moved yet, check again on next timeout...
          if( (CM_GetTickCount() - tickLast) > UPLOADMGR_CHECK_FILE_DELAY_MS )
          {
            if(checkFileCnt-- != 0)
            {
              VfyRspId = CMD_ID_NONE;
              VfyRspFailed = FALSE;
              GSE_StatusStr(NORMAL,".");
              strncpy_safe(checkCmdData.FN,sizeof(checkCmdData.FN),pTask->FN,_TRUNCATE);
              //If MSI fails to to put the command, treat it as a MS response failure
              //and attempt the file again
              pTask->State =
              MSI_PutCommand(CMD_ID_CHECK_FILE,&checkCmdData,sizeof(checkCmdData),
                             CHECK_FILE_TIMEOUT, 
                  UploadMgr_VerifyMSCmdsCallback)  == SYS_OK ?
                  LOG_COLLECTION_VFY_MS_MOVED_FILE : LOG_COLLECTION_FILE_UL_ERROR;

              tickLast = CM_GetTickCount();
            }
            else
            {
              pTask->State = LOG_COLLECTION_FILE_UL_ERROR;
              GSE_DebugStr(NORMAL, FALSE,
                  "UploadMgr LC: Timeout waiting for Micro-Server to move file");
            }
          }
        }
      }
      else if(VfyRspFailed)
      {
        pTask->State = LOG_COLLECTION_FILE_UL_ERROR;
      }
    break;

    case LOG_COLLECTION_NEXTFILE:
      //If more files in this flight to upload do them
      //Else, try finding the next flight
      pTask->UploadOrderIdx += 1;
      if(pTask->UploadOrderIdx < UploadFilesPerFlight)
      {
        pTask->State = LOG_COLLECTION_STARTFILE;
        GSE_DebugStr(VERBOSE,FALSE,"UploadMgr: LC NEXTFILE NextUpload=%s",
          DataMgrGetACSDataSet(UploadOrderTbl[pTask->UploadOrderIdx].ACS.acs).sID);
      }
      else
      {
        pTask->UploadOrderIdx = 0;
        pTask->FlightDataStart = pTask->FlightDataEnd;
        pTask->State = LOG_COLLECTION_FINDFLIGHT;
      }
      break;

    case LOG_COLLECTION_FILE_UL_ERROR:
      GSE_DebugStr(NORMAL,FALSE,"UploadMgr: LC Upload Error");
      //Delete verification data if the file did not transfer.
      UploadMgr_UpdateFileVfy(pTask->VfyRow,VFY_STA_DELETED,0,0);
      //Repeat this file 3x, then fail and goto the next file
      pTask->FileFailCnt++;
      if(pTask->FileFailCnt >= UPLOADMGR_FILE_UL_FAIL_MAX)
      {
        //Move on to the next file same file
        pTask->FileNotUploadedCnt++;
        GSE_DebugStr(NORMAL,FALSE,
          "UploadMgr: Max retries for this file exceeded, trying next file");

        //Reset error count, Do next file
        pTask->FileFailCnt = 0;
        pTask->State = LOG_COLLECTION_NEXTFILE;
      }
      else
      {
        //Repeat same file
        GSE_DebugStr(NORMAL,FALSE,"UploadMgr: LC Retrying file upload");
        pTask->State = LOG_COLLECTION_STARTFILE;
      }
      break;

    case LOG_COLLECTION_POST_MAINTENANCE:
      //MaintainFileTable() return 0 while in progress.  When it returns 1,
      //continue to the next task.  -1 indicates an error occurred, so quit
      //upload.
      i = UploadMgr_MaintainFileTable(FALSE);
      if(1 == i)
      {
        if(pTask->FileNotUploadedCnt == 0)
        {
          //Pass LogDataEnd as a filler for this function.  If any new
          //logs exist in memory, the erase will fail and these logs
          //can be uploaded next time the upload process is triggered
          LogErase( &pTask->FlightDataEnd, &pTask->LogEraseStatus );
          GSE_DebugStr(NORMAL,TRUE,"UploadMgr: LC Upload done, %d files uploaded",
                pTask->FileUploadedCnt);
          pTask->State = LOG_COLLECTION_LOG_ERASE_MEMORY;
        }
        else
        {
          GSE_DebugStr(NORMAL, TRUE,
                "UploadMgr: LC %d of %d files not uploaded, skip FLASH erase",
                pTask->FileNotUploadedCnt,

                pTask->FileUploadedCnt+pTask->FileNotUploadedCnt);

          pTask->State = LOG_COLLECTION_FINISHED;
        }
      }
      else if(-1 == i)
      {
        GSE_DebugStr(NORMAL,TRUE,
            "UploadMgr LC: FVT maintenance failed, upload stopping");
        pTask->State = LOG_COLLECTION_FINISHED;
      }
      break;

    case LOG_COLLECTION_LOG_ERASE_MEMORY:
      if(LOG_REQ_COMPLETE == pTask->LogEraseStatus)
      {
        pTask->State = LOG_COLLECTION_FINISHED;
        GSE_DebugStr(NORMAL,TRUE,"UploadMgr: LC ERASE_MEMORY=Completed OK");
        Assert_ClearLog();

      }
      else if(LOG_REQ_FAILED   == pTask->LogEraseStatus)
      {
        pTask->State = LOG_COLLECTION_FINISHED;
        GSE_DebugStr(NORMAL,TRUE,"UploadMgr: LC ERASE_MEMORY=Completed, could not erase");
      }
      break;

    case LOG_COLLECTION_FINISHED:
       GSE_DebugStr(NORMAL,TRUE,
                 "UploadMgr: LC FINISHED=Upload complete, turning off LC task");
       FAST_SignalUploadComplete();
       /**** DELIBERATE FALL THROUGH TO LOG_AUTO_CHECK_FINISHED 
            THIS REMOVES THE DEBUG OUTPUT EVERY TIME AUTOUPLOAD DOESNT FIND A RECORD ****/
       //lint -fallthrough
    case LOG_COLLECTION_AUTO_CHECK_FINISH:
      
      pTask->InProgress = FALSE;
      pTask->Disable    = FALSE;
      // SCR 1072: Set readOffset back to 0 because we may have gotten here through a fail path
      // and readOffset is static so the software may be stuck at the end of the logs
      readOffset = 0;
      TmTaskEnable(UL_Log_Collection, FALSE);
      break;

    default:
      FATAL("Unsupported LOG_COLLECTION_STATE = %d", pTask->State);
      break;
  }
}



/******************************************************************************
 * Function:    UploadMgr_MaintainFileTable
 *
 * Description: The MaintainFileTable is a sub-task of the Log Collection Task.
 *              This sub task implements a state machine that
 *              1. Searches for non-deleted verification entries
 *              2. Deletes any entries that did not complete upload or failed
 *                 MS verification
 *              3. Checks the current status of files listed as MS Verified,
 *                 Completed, or Ground Verified on the micro server and
 *                 updates the status in the file verification table if it
 *                 is different
 *              4. Marks logs for deletion once the file is MS Verified or
 *                 greater.
 *              5. Deletes the log verification data once the MS file state
 *                 is verified on ground.
 *              Finally, this task does not complete until all entries in the
 *              file verification table are "Deleted" or higher than
 *              "MS Verified"
 *
 *              The caller should set the "Start" parameter true on the first
 *              call to initialize the task.  The caller should continue to
 *              call this function until the return value indicates the
 *              maintenance is completed.
 *
 * Parameters:  [in] Start: TRUE is used on the initial call to this routine.
 *                               It resets the row counter to zero.
 *                          FALSE is used on subsequent calls to scan through
 *                                all rows in the verification table
 *
 * Returns:     0: Maintenance is still running, call function again
 *              1: Maintenance is complete, This function will continue to
 *                 again return 1 until Start set to TRUE again to do another
 *                 Maintenance
 *             -1: An error occurred while performing file maintenance.  This
 *                 function will continue to return -1 until is is called with
 *                 the Start parameter set to TRUE again.
 *
 * Notes:       The very first call with Start set to TRUE will always return
 *              with a value of 0.
 *
 *              Function exceeds cyclomatic and nesting complexity
 *
 *****************************************************************************/
static
INT32 UploadMgr_MaintainFileTable(BOOLEAN Start)
{
  static INT32                     Row;
  static UPLOADMGR_FILE_VFY        VfyRow;
  static UPLOADMGR_TBL_MAINT_STATE State;
  static LOG_MARK_STATUS           LogMarkStatus;
  MSCP_LOGFILE_INFO                RemoveCmdData;
  MSCP_CHECK_FILE_CMD              CheckCmdData;
  INT32                            retval = 0;

  if(Start)
  {
    Row = 0;
    State = TBL_MAINT_SRCH_STARTED_MSFAIL;
    GSE_DebugStr(NORMAL, TRUE,
        "UploadMgr: Checking verification status of uploaded files...");
    GSE_DebugStr(VERBOSE,FALSE,
        "UploadMgr FVT: Searching for 'Started', 'MS Failed' or 'Complete' files...");
    m_MaintainFileTableRunning = TRUE;
  }

  switch(State)
  {
    /*Search 1: Examine the state of every entry in the file verification table
      Do nothing with deleted rows.  Delete Started, MS Failed, or Complete
      file from the micro server and from the FVT.  The data will be re-transfered
      on the next upload cycle*/
    case TBL_MAINT_SRCH_STARTED_MSFAIL:
      while(TBL_MAINT_SRCH_STARTED_MSFAIL == State)
      {
        NV_Read(NV_UPLOAD_VFY_TBL,VFY_ROW_TO_OFFSET(Row),&VfyRow,sizeof(VfyRow));

        if((VFY_STA_STARTED    == VfyRow.State) ||
            (VFY_STA_MSFAIL    == VfyRow.State) ||
            (VFY_STA_COMPLETED == VfyRow.State) )
        {
          VfyRspId = CMD_ID_NONE;
          VfyRspFailed = FALSE;
          strncpy_safe(RemoveCmdData.FileName,
              sizeof(RemoveCmdData.FileName),
              VfyRow.Name,
              _TRUNCATE);
          if(SYS_OK == MSI_PutCommand(CMD_ID_REMOVE_LOG_FILE,
              &RemoveCmdData,
              sizeof(RemoveCmdData),
              RMV_FILE_TIMEOUT,
              UploadMgr_VerifyMSCmdsCallback))
          {
            GSE_DebugStr(NORMAL,FALSE,"UploadMgr FVT: Removing %s file %s...",
                (VfyRow.State == VFY_STA_STARTED) ? "'Started'" :
                (VfyRow.State == VFY_STA_MSFAIL) ? "'MS Failed'" : "'Complete'",
                VfyRow.Name);
            State = TBL_MAINT_WT_RM_LOG_RSP;
          }
          else
          {
            State = TBL_MAINT_ERROR;
            break;
          }
        }
        else if(Row++ < UPLOADMGR_VFY_TBL_MAX_ROWS)
        {
          State = TBL_MAINT_SRCH_STARTED_MSFAIL;
        }
        else
        {
          Row = 0;
          State = TBL_MAINT_SRCH_LOG_DELETED;
          GSE_DebugStr(VERBOSE, FALSE,
              "UploadMgr FVT: Searching log memory to delete...");
        }
      }
      break;

    /*Wt Delete Rsp: Wait for, and process the REMOVE_LOG_FILE response
      from the micro server.  When MSSIM indicates the file is gone, delete the
      FVT entry */
    case TBL_MAINT_WT_RM_LOG_RSP:
      //Once deleted from MS, delete from the Verification Table
      if(CMD_ID_REMOVE_LOG_FILE == VfyRspId )
      {
        UploadMgr_UpdateFileVfy(Row,VFY_STA_DELETED,0,0);

        GSE_DebugStr(NORMAL,FALSE,"   Done!");
        State = TBL_MAINT_SRCH_STARTED_MSFAIL;
      }
      /*if*/(VfyRspFailed == TRUE) ?
      //{
        State = TBL_MAINT_ERROR : 0;
      //}
      break;

    case TBL_MAINT_SRCH_LOG_DELETED:
      while(TBL_MAINT_SRCH_LOG_DELETED == State)
      {
        NV_Read(NV_UPLOAD_VFY_TBL,VFY_ROW_TO_OFFSET(Row),&VfyRow,sizeof(VfyRow));

        if((!VfyRow.LogDeleted) && (VfyRow.State != VFY_STA_DELETED))
        {
          LogMarkState( LOG_DELETED,
              VfyRow.Type,
              VfyRow.Source,
              VfyRow.Priority,
              VfyRow.Start,
              VfyRow.End,
              &LogMarkStatus );
          GSE_DebugStr(NORMAL, FALSE,
              "UploadMgr FVT: Deleting log data for %s...",VfyRow.Name);
          State = TBL_MAINT_WT_MARK_DELETE;
        }
        else if(Row++ < UPLOADMGR_VFY_TBL_MAX_ROWS)
        {
          State = TBL_MAINT_SRCH_LOG_DELETED;
        }
        else
        {
          Row = 0;
          State = TBL_MAINT_SRCH_GROUNDVFY;
          GSE_DebugStr(VERBOSE, FALSE,
              "UploadMgr FVT: Searching for 'Ground Verified' files...");
        }
      }
      break;

    /* Wt Mark For Deletion: Wait for Log Mgr to finish marking the logs for
       deletion.  If the file state is GroundVfy, also delete the FVT row, else
       move onto the next row and return to search.*/
    case TBL_MAINT_WT_MARK_DELETE:
      if(LOG_MARK_IN_PROGRESS  == LogMarkStatus)
      {
        //Wait for completion
      }
      else if(LOG_MARK_COMPLETE == LogMarkStatus)
      {
        //Set log deleted flag and write the file data back to NV memory.
        //If the mark log for deletion was the only task pending for this file,
        //then the round trip was also completed so delete the FVT entry.
        VfyRow.LogDeleted = TRUE;
        /*if*/(VfyRow.State == VFY_STA_LOG_DELETE) ?
        //{
          VfyRow.State   = VFY_STA_DELETED,
          m_FreeFVTRows += 1
        //}
          :0;

        //For priority 4 log files, no data is transmitted to the ground, so no
        //round trip CRC is expected.  Delete the entry once the logs have been
        //marked for deletion.
        if(VfyRow.Priority == LOG_PRIORITY_4)
        {
          VfyRow.State = VFY_STA_DELETED;
          m_FreeFVTRows += 1;
        }

        NV_Write(NV_UPLOAD_VFY_TBL,VFY_ROW_TO_OFFSET(Row),&VfyRow,sizeof(VfyRow));
        State = TBL_MAINT_SRCH_LOG_DELETED;
        GSE_StatusStr(NORMAL,"Done!");
      }
      else if(LOG_MARK_FAIL == LogMarkStatus)
      {
        GSE_DebugStr(NORMAL,FALSE,"UploadMgr FVT: Log Mark failed");
        retval = -1;
      }
      break;

    case TBL_MAINT_SRCH_GROUNDVFY:
      while(TBL_MAINT_SRCH_GROUNDVFY == State)
      {
        NV_Read(NV_UPLOAD_VFY_TBL,VFY_ROW_TO_OFFSET(Row),&VfyRow,sizeof(VfyRow));

        if(VFY_STA_GROUNDVFY == VfyRow.State)
        {
          VfyRspId = CMD_ID_NONE;
          VfyRspFailed = FALSE;
          strncpy_safe(CheckCmdData.FN,sizeof(CheckCmdData.FN),VfyRow.Name,_TRUNCATE);

          State = (MSI_PutCommand(CMD_ID_CHECK_FILE,&CheckCmdData,
                   sizeof(CheckCmdData),CHECK_FILE_TIMEOUT,
                   UploadMgr_VerifyMSCmdsCallback) == SYS_OK)  ?
            TBL_MAINT_WT_CHECK_RSP2: TBL_MAINT_ERROR;

          State == TBL_MAINT_WT_CHECK_RSP2 ?
            GSE_DebugStr(NORMAL,FALSE,"UploadMgr FVT: Verifying %s moved to ground...",
                VfyRow.Name):
            GSE_DebugStr(NORMAL,FALSE,"UploadMgr FVT: Failed to put MS command Check File");

        }
        else if(Row++ < UPLOADMGR_VFY_TBL_MAX_ROWS)
        {
          State = TBL_MAINT_SRCH_GROUNDVFY;
        }
        else
        {
          State = TBL_MAINT_FINISHED;
          GSE_DebugStr(NORMAL,TRUE,"UploadMgr: Done checking files");
        }
      }
      break;

    case TBL_MAINT_WT_CHECK_RSP2:
      if( CMD_ID_CHECK_FILE == VfyRspId )
      {
        if(MSCP_FILE_STA_GSVALID == CheckFileRsp.FileSta)
        {
          //FVT has the file as ground verified and MSSIM has it as ground
          //verified also.  Round-trip verification is complete, so deleted
          //the FVT row entry.
          UploadMgr_UpdateFileVfy(Row, VFY_STA_DELETED, 0,0);
          GSE_StatusStr(NORMAL,FALSE,
              "  Done! \r\n               round-trip complete!");
          State = TBL_MAINT_SRCH_GROUNDVFY;
        }
        else
        {
          //MSSIM hasn't moved this to Ground Verified yet, even though we
          //have it as ground verified. Skip to next row
          State = (Row++ < UPLOADMGR_VFY_TBL_MAX_ROWS) ?
                   TBL_MAINT_SRCH_GROUNDVFY : TBL_MAINT_FINISHED ;
          (State == TBL_MAINT_FINISHED) ?
              GSE_DebugStr(NORMAL,TRUE,"UploadMgr: Done checking files"),0:0;

        }
      }
      State = ( TRUE == VfyRspFailed ) ? TBL_MAINT_ERROR : State;
      break;

    case TBL_MAINT_ERROR:
      //Some error has occurred while running this process, and it cannot
      //continue.  Return this status to the caller.

      m_MaintainFileTableRunning = FALSE;
      retval = -1;
      break;

    case TBL_MAINT_FINISHED:
      //All files in the table are MS Verified or higher and data that need
      //not be re-uploaded is marked for deletion in log manager.  The system
      //is now ready to upload more files.

      m_MaintainFileTableRunning = FALSE;
      retval = 1;
      break;
  }

  return retval;
}



/******************************************************************************
 * Function:    UploadMgr_CheckLogCmdCallback | MS Response Handler
 *
 * Description: Handles responses to the Remove Log File and Check File
 *              commands that are sent from the file verification table
 *              maintenance task and the log collection task.  Signals back
 *              to the CheckLogMoved the response was received by setting
 *              the m_CheckLogRow to -1 for response received, or -2
 *              for response received but the file is not at GROUND VERIFY.
 *
 * Parameters:  [in]    Id:         ID of the response
 *              [in/out]PacketData  Pointer to response data
 *              [in]    Size        size of the response data
 *              [in]    Status      Response status, (Success,fail,timeout)
 *
 * Returns:
 *
 * Notes:
 *
 *****************************************************************************/
static
void UploadMgr_CheckLogCmdCallback(UINT16 Id, void* PacketData, UINT16 Size,
                             MSI_RSP_STATUS Status)
{
  MSCP_CHECK_FILE_RSP *rsp  = PacketData;
  UPLOADMGR_FILE_VFY  vfy_row;

  if((MSI_RSP_SUCCESS == Status) && (Size == sizeof(*rsp)) )
  {
    if(rsp->FileSta == MSCP_FILE_STA_GSVALID)
    {
      //Read the row from Verify Table File.
      //Change the entry status as deleted/free-to-use or,
      //flagged for deletion.
      NV_Read(NV_UPLOAD_VFY_TBL,VFY_ROW_TO_OFFSET(m_CheckLogRow),&vfy_row,sizeof(vfy_row));
      if(vfy_row.LogDeleted)
      {
        UploadMgr_UpdateFileVfy(m_CheckLogRow,VFY_STA_DELETED,0,0);
      }
      else
      {
        UploadMgr_UpdateFileVfy(m_CheckLogRow,VFY_STA_LOG_DELETE,0,0);
      }
      GSE_StatusStr(NORMAL,"Done!");
    }
    else
    {
      m_CheckLogRow = -2;
    }
  }
  //Ensure -1 if not -2.
  m_CheckLogRow = m_CheckLogRow == -2 ? m_CheckLogRow : -1;
}



/******************************************************************************
 * Function:    UploadMgr_CheckLogMoved | TASK
 *
 * Description: This task is enabled after receiving a ground CRC from the
 *              Micro-Server.  It checks the status of ground verified
 *              files and makes sure it was moved to the CP_SAVED folder
 *              on the Micro-Server before finally deleting the entry in
 *              the table.
 *
 *              1. Search all rows until a GroundVfy state is found
 *              2. Send the "CheckFile" command for that file and set the
 *                 m_CheckLogRow var to indicates what FVT Row to update
 *                 when the response comes back
 *              3. When m_CheckLogRow returns to -1, it means the response
 *                 to the command came back, continue checking the other
 *                 rows
 *              4. When m_CheckLogRow returns to -2, it means the file is not
 *                 yet at Ground Verify, retry the command for that file.
 *              4. After all rows are checked shut off the task.
 *
 * Parameters:  pParam: Task Manager task prototype, not used.
 *
 * Returns:     none
 *
 * Notes:
 *
 *****************************************************************************/
static
void UploadMgr_CheckLogMoved(void *pParam)
{
  static INT32        row = -1;
  static INT32        row_retry = 0;
  MSCP_CHECK_FILE_CMD cmd;
  UPLOADMGR_FILE_VFY  vfy_row;
  BOOLEAN             file_found = FALSE;

  //While the task is running, search for any files that may be in the
  //ground verify state so that we can deal multiple Ground Verify
  //CRC commands coming from the Micro-Server.  If the Maintenance Task
  //is running don't bother checking file locations here, it will be handled
  //by the Maintenance Task.
  if(m_MaintainFileTableRunning)
  {
    row = -1;
    m_CheckLogRow = -1;
    TmTaskEnable(UL_Log_Check,FALSE);
  }
  else
  {
    while((row < UPLOADMGR_VFY_TBL_MAX_ROWS) && m_CheckLogRow < 0)
    {
      //-2 means do the same row again until the max retry count
      if(m_CheckLogRow == -2)
      {
        m_CheckLogRow = -1;
        if(row_retry++ < UPLOADMGR_CHECK_FILE_CNT)
        {
          // Response indicates file did not move yet, retry it.
          GSE_StatusStr(NORMAL,".");
        }
        else
        {
          // We've exceeded the retry count for this pass.
          // Reset retry count and continue to next row.
          row_retry = 0;
          row++;
        }
      }
      else
      {
        // Response indicates file moved, reset retry counter,
        // move on to next table entry.
        row_retry = 0;
        row++;
      }

      // Read next row (or re-Read current row) for status.
      // If state is ground-verify, tell MS to check file.
      // The result is processed on a subsequent call to this function once the
      // callback we provided has been invoked to update the status.

      NV_Read(NV_UPLOAD_VFY_TBL,VFY_ROW_TO_OFFSET(row),&vfy_row,sizeof(vfy_row));
      if(VFY_STA_GROUNDVFY == vfy_row.State)
      {
        //Need to consider preemption here. If interrupted after PutCommand,
        //Row must be set already for when CHECK_FILE callback executes. If
        //PutCmd fails, set row to -2 and this task will re-try up to CHECK_FILE_CNT
        m_CheckLogRow = row;
        strncpy_safe(cmd.FN,sizeof(cmd.FN),vfy_row.Name,_TRUNCATE);
        m_CheckLogRow = MSI_PutCommand(CMD_ID_CHECK_FILE,
                                       &cmd,
                                       sizeof(cmd),
                                       CHECK_ROW_TIMEOUT,
                                       UploadMgr_CheckLogCmdCallback) == SYS_OK ? row : -2;
        file_found = TRUE;
        m_CheckLogFileCnt > 0 ? m_CheckLogFileCnt-- : 0;
      }
    } // while Row remaining...

    //If no file found after looping through entire table, decrement
    //CheckCnt to prevent getting stuck in this table should another
    //process (ex. user mgr) delete the entry while in this process
    !file_found ? m_CheckLogFileCnt > 0 ? m_CheckLogFileCnt-- : 0 : 0;
  }

  //When all rows checked, reset row back to zero and kill task
  if(row == UPLOADMGR_VFY_TBL_MAX_ROWS)
  {
    row = -1;
    m_CheckLogRow = -1;
    TmTaskEnable(UL_Log_Check,m_CheckLogFileCnt == 0 ?FALSE:TRUE);
  }
}



/******************************************************************************
 * Function:    UploadMgr_VerifyMSCmdsCallback | MS Response Handler
 *
 * Description: Handles responses to the Remove Log File and Check File
 *              commands that are sent from the file verification table
 *              maintenance task and the log collection task.
 *
 * Parameters:  [in]    Id:         ID of the response
 *              [in/out]PacketData  Pointer to response data
 *              [in]    Size        size of the response data
 *              [in]    Status      Response status, (Success,fail,timeout)
 *
 * Returns:
 *
 * Notes:
 *
 *****************************************************************************/
static
void UploadMgr_VerifyMSCmdsCallback(UINT16 Id, void* PacketData, UINT16 Size,
                             MSI_RSP_STATUS Status)
{
  if(MSI_RSP_SUCCESS == Status)
  {
    switch(Id)
    {
      case CMD_ID_REMOVE_LOG_FILE:
        //Copy response to global var for the Maintenance task to pick up
        RemoveFileRsp.Result = ((MSCP_REMOVE_FILE_RSP*)PacketData)->Result;
        VfyRspId = CMD_ID_REMOVE_LOG_FILE;
        GSE_DebugStr(VERBOSE,FALSE,"UploadMgr: Remove File rsp. Result: %02x",
            RemoveFileRsp.Result);
        break;

      case CMD_ID_CHECK_FILE:
        //Copy response to global var for the Maintenance task to pick up
        CheckFileRsp.FileSta = ((MSCP_CHECK_FILE_RSP*)PacketData)->FileSta;
        VfyRspId = CMD_ID_CHECK_FILE;
        GSE_DebugStr(VERBOSE,FALSE,"UploadMgr: Check File rsp. File State: %02x",
            ((MSCP_CHECK_FILE_RSP*)PacketData)->FileSta);
        break;

      default:
        FATAL("Unsupported MSCP_CMD_ID = %d", Id);
        break;
    }
  }

  VfyRspFailed = MSI_RSP_SUCCESS == Status ? FALSE : TRUE;
}



/******************************************************************************
 * Function:    UploadMgr_FindFlight
 *
 * Description: Searches from the "Flight Start" offset for a
 *              "Flight End" log.  If the End of Flight is not found, the end
 *              of log data is returned.  If no un-erased logs are found, the
 *              log not found status is returned.
 *
 *
 * Parameters:  [in/out] FlightStart: Sets the point to start the search from
 *                                    Pointer is advanced one log to not include
 *                                    the last log from the previous flight if
 *                                    it is not starting from offset 0.
 *              [out]FlightEnd:   Location of the first "Flight End" log
 *                                encountered, or the location of the last
 *                                log in memory if "Flight End" is not found
 *
 * Returns:     FIND_LOG_STATUS.  Return value of the FindLogNext function
 *
 * Notes:
 *
 *****************************************************************************/
static
LOG_FIND_STATUS UploadMgr_FindFlight(UINT32 *FlightStart,UINT32* FlightEnd)
{
  UINT32 log_size;
  UINT32 read_offset = *FlightStart;
  LOG_SOURCE source;
  LOG_FIND_STATUS result = LOG_NOT_FOUND;
  UINT32 endOffset = LogGetLastAddress();

  //If not FlightStart == 0, then this is the not the first flight that is
  //being processed.  The Start pointer is at the LAST log of the previous
  //flight, so call twice to advance it to the first log in this flight.
  if(*FlightStart != 0)
  {
    source.nNumber = LOG_SOURCE_DONT_CARE;
    result = LogFindNextRecord(LOG_DONT_CARE,
                               LOG_TYPE_DONT_CARE,
                               source,
                               LOG_PRIORITY_DONT_CARE,
                               &read_offset,
                               endOffset,
                               FlightStart,
                               &log_size);
    if(result != LOG_BUSY)
    {
      result = LogFindNextRecord(LOG_DONT_CARE,
                                 LOG_TYPE_DONT_CARE,
                                 source,
                                 LOG_PRIORITY_DONT_CARE,
                                 &read_offset,
                                 endOffset,
                                 FlightStart,
                                 &log_size);
    }
  }
  if(result != LOG_BUSY)
  {

    //Now look for the next end of flight log
    source.id = APP_ID_END_OF_FLIGHT;
    result = LogFindNextRecord(LOG_DONT_CARE,
                               LOG_TYPE_SYSTEM,
                               source,
                               LOG_PRIORITY_DONT_CARE,
                               &read_offset,
                               endOffset,
                               FlightEnd,
                               &log_size);

    //If no more "end of flight" logs are found, search for any logs after
    //the current position.
    if(LOG_NOT_FOUND == result)
    {
      read_offset = *FlightStart;
      source.nNumber = LOG_SOURCE_DONT_CARE;
      result = LogFindNextRecord(LOG_NEW,
                                 LOG_TYPE_DONT_CARE,
                                 source,
                                 LOG_PRIORITY_DONT_CARE,
                                 &read_offset,
                                 endOffset,
                                 FlightEnd,
                                 &log_size);

      if(LOG_FOUND == result)
      {
        *FlightEnd = endOffset;
      }
    }
  }
  return result;
}



/******************************************************************************
 * Function:    UploadMgr_MakeFileHeader
 *
 * Description: Fill in the log file header, using parameters passed into
 *              this function.  File checksum is computed and written back
 *              into the header.
 *
 * Parameters:  [in/out] FileHeader:  Fills in all file header fields except the
 *                                checksum. Once the header is filled in, the
 *                                checksum is updated to reflect the new data
 *                                in the file.
 *              [in] Type
 *              [in] Priority
 *              [in] ACS
 *              [in] Timestamp
 *
 * Returns:     None
 *
 * Notes:
 *
 *****************************************************************************/
static
void UploadMgr_MakeFileHeader(UPLOADMGR_FILE_HEADER* FileHeader,LOG_TYPE Type,
                              LOG_PRIORITY Priority, LOG_SOURCE ACS,
                              TIMESTAMP* Timestamp)
{
  UINT32 dataChecksum;
  INT8   box_str[BOX_INFO_STR_LEN];
  CHAR   swVerStr[32];
  BINARY_DATA_HDR *pBinSubHdr;

  pBinSubHdr = &BinarySubHdr;

  strncpy_safe(FileHeader->file_type,sizeof(FileHeader->file_type),"FAST",_TRUNCATE);
  FileHeader->file_version = FILE_HEADER_VER;

  //UINT16 DTU version ID
  FileHeader->config_version = CfgMgr_RuntimeConfigPtr()->VerId;

  //INT8 DTU installation identification
  strncpy_safe(FileHeader->id, sizeof(FileHeader->id),
                CfgMgr_RuntimeConfigPtr()->Id, _TRUNCATE);

  //DTU Serial Number
  Box_GetSerno(box_str);
  strncpy_safe(FileHeader->serial_number,sizeof(FileHeader->serial_number),box_str,_TRUNCATE);

  // Box Manufacture Date
  Box_GetMfg(box_str);
  strncpy_safe(FileHeader->mfg_date,sizeof(FileHeader->mfg_date),box_str,_TRUNCATE);

  // Box Manufacture Revision
  Box_GetRev(box_str);
  strncpy_safe(FileHeader->mfg_rev,sizeof(FileHeader->mfg_rev),box_str,_TRUNCATE);

  // Software Version
  FAST_GetSoftwareVersion(swVerStr);
  strncpy_safe(FileHeader->sw_version,sizeof(FileHeader->sw_version),swVerStr,_TRUNCATE);

  // TimeSource
  FileHeader->time_source = FAST_TimeSourceCfg();

  // Current System Condition
  FileHeader->sys_condition = Flt_GetSystemStatus();

  //UINT32 DTU time
  FileHeader->time = Timestamp->Timestamp;

  //UINT16 DTU time in ms
  FileHeader->time_ms = Timestamp->MSecond;

  //UINT32 Power-on Count
  FileHeader->poweron_cnt = Box_GetPowerOnCount();

  //UINT32 Power-on Time
  FileHeader->poweron_time = Box_GetPowerOnTime();

  //Log type, system or application
  FileHeader->log_type = Type;

  //UINT16 number of log count of this ACS
  //FileHeader->nLogCnt = 0;   //This field is populated by the Log Collector
  //UINT16 priority of log data of this ACS
  FileHeader->priority = Priority;

  //UINT16 which ACS is connected.  see ACS_SOURCE
  FileHeader->acs_source = (UINT16)ACS.nNumber;

  //INT8 ACS model id
  strncpy_safe(FileHeader->acs_model,sizeof(FileHeader->acs_model),
              &(DataMgrGetACSDataSet(ACS.acs).sModel[0]),_TRUNCATE);

  //INT8  ACS version information
  strncpy_safe(FileHeader->acs_ver_id,sizeof(FileHeader->acs_ver_id),
               &(DataMgrGetACSDataSet(ACS.acs).sID[0]),_TRUNCATE);

  //UINT16 which ACS is connected.  see ACS_SOURCE
  FileHeader->acs_source = (UINT16)ACS.nNumber;

  //INT8 ACS model id
  strncpy_safe(FileHeader->acs_model,sizeof(FileHeader->acs_model),
              &(DataMgrGetACSDataSet(ACS.acs).sModel[0]),_TRUNCATE);
  //INT8  ACS version information
  strncpy_safe(FileHeader->acs_ver_id,sizeof(FileHeader->acs_ver_id),
              &(DataMgrGetACSDataSet(ACS.acs).sID[0]),_TRUNCATE);
  // Port Type
  FileHeader->port_type = (UINT8)DataMgrGetACSDataSet(ACS.acs).portType;

  // Port Index
  FileHeader->port_index = (UINT8)DataMgrGetACSDataSet(ACS.acs).nPortIndex;

  //Config Status
  AC_GetConfigStatus(FileHeader->config_status);

  //INT8 Tail Number of Aircraft
  strncpy_safe(FileHeader->tail_number,sizeof(FileHeader->tail_number),
      CfgMgr_RuntimeConfigPtr()->Aircraft.TailNumber,_TRUNCATE);

  //INT8 Airline
  strncpy_safe(FileHeader->operator,sizeof(FileHeader->operator),
                CfgMgr_RuntimeConfigPtr()->Aircraft.Operator,_TRUNCATE);

  //INT8 Aircraft Style (i.e. 737-300)
  strncpy_safe(FileHeader->style,sizeof(FileHeader->style),
      CfgMgr_RuntimeConfigPtr()->Aircraft.Style,_TRUNCATE);

  //INT8
  strncpy_safe(FileHeader->config_file_QAR_pn,sizeof(FileHeader->config_file_QAR_pn),
                "",_TRUNCATE);

  //UINT16
  FileHeader->config_ver_qar_ver = 0;

  // Engine Identification
  strncpy_safe(FileHeader->service_plan, sizeof(FileHeader->service_plan),
               EngRunGetFileHeader()->servicePlan,_TRUNCATE);

  strncpy_safe(FileHeader->sn_engine1, sizeof(FileHeader->sn_engine1),
               EngRunGetFileHeader()->engine[0].serialNumber,_TRUNCATE);

  strncpy_safe(FileHeader->model_engine1, sizeof(FileHeader->model_engine1),
               EngRunGetFileHeader()->engine[0].modelNumber,_TRUNCATE);

  strncpy_safe(FileHeader->sn_engine2, sizeof(FileHeader->sn_engine2),
               EngRunGetFileHeader()->engine[1].serialNumber,_TRUNCATE);

  strncpy_safe(FileHeader->model_engine2, sizeof(FileHeader->model_engine2),
               EngRunGetFileHeader()->engine[1].modelNumber,_TRUNCATE);

  strncpy_safe(FileHeader->sn_engine3, sizeof(FileHeader->sn_engine3),
               EngRunGetFileHeader()->engine[2].serialNumber,_TRUNCATE);

  strncpy_safe(FileHeader->model_engine3, sizeof(FileHeader->model_engine3),
               EngRunGetFileHeader()->engine[2].modelNumber,_TRUNCATE);

  strncpy_safe(FileHeader->sn_engine4, sizeof(FileHeader->sn_engine4),
               EngRunGetFileHeader()->engine[3].serialNumber,_TRUNCATE);

  strncpy_safe(FileHeader->model_engine4, sizeof(FileHeader->model_engine4),
               EngRunGetFileHeader()->engine[3].modelNumber,_TRUNCATE);

  memset(&BinarySubHdr,0,sizeof(BinarySubHdr));

  if (LOG_TYPE_SYSTEM == Type)
  {
     pBinSubHdr->size_used = CfgMgr_GetSystemBinaryHdr (pBinSubHdr->buf,
                                                       sizeof(pBinSubHdr->buf));
  }
  else if (LOG_TYPE_ETM == Type)
  {
     pBinSubHdr->size_used = CfgMgr_GetETMBinaryHdr (pBinSubHdr->buf, sizeof(pBinSubHdr->buf));
  }
  else // Get the Binary Data Header
  {
     pBinSubHdr->size_used = DataMgrGetHeader(pBinSubHdr->buf, ACS.acs,
                                              sizeof(pBinSubHdr->buf));
  }

  ASSERT_MESSAGE((pBinSubHdr->size_used <= sizeof(pBinSubHdr->buf)),
                 "Binary Header too big, log type %x",Type);

  // where log data starts.  Note, ACS headers are
  // always at the beginning of data block.
  FileHeader->data_offset = (sizeof(*FileHeader) + pBinSubHdr->size_used);


  //Save and zero-out existing header checksum so that it won't
  //be added into the
  dataChecksum = FileHeader->check_sum;
  FileHeader->check_sum = 0;

  //Add checksum of the header data into the existing checksum value
  dataChecksum += ChecksumBuffer(FileHeader,sizeof(*FileHeader),0xFFFFFFFF);
  dataChecksum += ChecksumBuffer(pBinSubHdr->buf, pBinSubHdr->size_used, 0xFFFFFFFF);

  //Finally, copy the checksum back into the header
  FileHeader->check_sum = dataChecksum;
}


/******************************************************************************
 * Function:    UploadMgr_StartFileUploadTask
 *
 * Description: Starts execution of the UploadMgr_FileUploadTask
 *              The file upload process consists of sending the start data
 *              command, data chunks, and finally the end data command.
 *
 *              Upon calling this routine the task will start, send the start
 *              data command to the micro-server and wait for data to appear
 *              in the file data queue.
 *
 * Parameters:  [in] FN: < 64 char string for the data's file name.
 *                         note: longer strings will be truncated
 *              [in] Priority: Log manager priority for this file
 *
 *
 * Returns:     TRUE:  Upload process started
 *              FALSE: Upload process already running and must complete before
 *                     starting another file.
 *
 * Notes:
 *
 *****************************************************************************/
static
void UploadMgr_StartFileUploadTask(const INT8* FN, const LOG_PRIORITY Priority)
{
  //File data queue
  FIFO_Flush(&FileDataFIFO);

  //Setup upload task variables
  ULTaskData.State            = FILE_UL_INIT;
  ULTaskData.FailCnt          = 0;
  ULTaskData.FinishFileUpload = FALSE;

  //Initialize Upload Task data for this file.
  strncpy_safe(ULTaskData.LogFileInfo.FileName,
          sizeof(ULTaskData.LogFileInfo.FileName),
          FN,
          _TRUNCATE);
  ULTaskData.LogFileInfo.Priority = Priority;

  //Start upload task.
  TmTaskEnable(UL_Log_File_Upload, TRUE);
}



/******************************************************************************
 * Function:    UploadMgr_FinishFileUpload
 *
 * Description: Called after all file data has been transferred to the file
 *              data queue.  This signal the file upload task to end the upload
 *              when the queue is emptied
 *
 * Parameters:  [in] Row: Row # for the file being finished
 *              [in] CRC: Finial CRC for the file being finished
 *              [in] LogOffset: LogManager offset of the last log included
 *                              in the file
 *
 * Returns:
 *
 * Notes:
 *
 *****************************************************************************/
static
void  UploadMgr_FinishFileUpload(INT32 Row, UINT32 CRC, UINT32 LogOffset)
{
  //File transfer has successfully completed.
  //Finalize the file verification CRC, set verification state to
  //"completed"
  CRC32(NULL, 0, &CRC, CRC_FUNC_END);
  UploadMgr_UpdateFileVfy(Row, VFY_STA_COMPLETED,
                          CRC, LogOffset);

  ULTaskData.FinishFileUpload = TRUE;
}


/******************************************************************************
 * Function:    UploadMgr_FileUploadTask | TASK
 *
 * Description: Starts the upload by sending a Start Log Data command to the
 *              micro-server.  Reads data from the FileData queue, chunks it
 *              into micro-server packet size portions and sends them to the
 *              micro-server.  A flag, set by the Log Collection task, signals
 *              the end of file data and signals the File Upload Task to
 *              send the End Log Data command.  Finally, the task ends up in
 *              an Error or Finished state.
 *
 * Parameters:  [in/out] pParam: Task manager data pointer.  Expected data
 *                               type is UPLOAD_MGR_FILE_UL_TASK_DATA.
 *
 * Returns:     none
 *
 * Notes:       Function exceeds cyclomatic and nesting complexity
 *
 *****************************************************************************/
static
void UploadMgr_FileUploadTask(void *pParam)
{
  UPLOADMGR_FILE_UL_TASK_DATA* task =  pParam;
  static UINT32                endLogPollDecimator = 0;

  switch (task->State)
  {
    case FILE_UL_INIT:
      if( UploadMgr_SendMSCmd(CMD_ID_START_LOG_DATA,30000,
                          &task->LogFileInfo,
                          sizeof(task->LogFileInfo)))
      {
        task->State = FILE_UL_WAIT_START_UL_RSP;
        UploadMgr_PrintUploadStats(0);
      }
      else
      {
        GSE_DebugStr(NORMAL,FALSE,"UploadMgr UL: Put Start Log failed");
        task->FailCnt++;
        if(task->FailCnt  >= UPLOADMGR_START_LOG_FAIL_MAX)
        {
          task->State = FILE_UL_ERROR;
        }
      }
      break;

    case FILE_UL_WAIT_START_UL_RSP:
      if(m_GotResponse)
      {
        if(ResponseStatusSuccess)
        {
          if(UploadMgr_UploadFileBlock(UPLOAD_BLOCK_FIRST))
          {
            task->FailCnt = 0;
            task->State = FILE_UL_WAIT_DATA_UL_RSP;
          }
          else
          {
            GSE_DebugStr(NORMAL,FALSE,"UploadMgr UL: Put Log Data failed");
            task->FailCnt++;
            if(task->FailCnt  >= UPLOADMGR_START_LOG_FAIL_MAX)
            {
              task->State = FILE_UL_ERROR;
            }
          }
        }
        else  //status == FAIL, retry same command until fail count exceeded
        {
          task->State = task->FailCnt++ < UPLOADMGR_START_LOG_FAIL_MAX ?
                        FILE_UL_INIT : FILE_UL_ERROR;
        }
      }
      break;

    case FILE_UL_WAIT_DATA_UL_RSP:
      //Wait for response from ms interface.
      if(m_GotResponse)
      {
        if(ResponseStatusSuccess)
        {
          //Log collection task signals all the file data has been
          //transfered to the queue and the queue count is zero.
          if(task->FinishFileUpload && FileDataFIFO.Cnt == 0)
          {
            if(UploadMgr_SendMSCmd(CMD_ID_END_LOG_DATA,5000,
                                   &task->LogFileInfo,
                                   sizeof(task->LogFileInfo)))
            {
              task->State = FILE_UL_WAIT_END_UL_RSP;
            }
            else
            {
              GSE_DebugStr(NORMAL,FALSE,"UploadMgr UL: Put End Log failed");
              task->FailCnt++;
              if(task->FailCnt  >= UPLOADMGR_END_LOG_FAIL_MAX)
              {
                task->State = FILE_UL_ERROR;
              }
            }
          }
          else
          {
            if(UploadMgr_UploadFileBlock(
              task->FailCnt == 0 ? UPLOAD_BLOCK_NEXT:UPLOAD_BLOCK_LAST))
            {
              task->FailCnt = 0;
              task->State = FILE_UL_WAIT_DATA_UL_RSP;
            }
            else
            {
              GSE_DebugStr(NORMAL,FALSE,"UploadMgr UL: Put Next Log Data failed");
              task->FailCnt++;
              if(task->FailCnt  >= UPLOADMGR_LOG_DATA_FAIL_MAX)
              {
                task->State = FILE_UL_ERROR;
              }
            }
          }
        }
        else  //status == FAIL, retry same block until fail count exceeded
        {
          if(task->FailCnt++ < UPLOADMGR_LOG_DATA_FAIL_MAX)
          {
            if(UploadMgr_UploadFileBlock(UPLOAD_BLOCK_LAST))
            {
              //Block queued - wait for MS response before clearing FailCnt
            }
            else //putting the command failed
            {
              GSE_DebugStr(NORMAL,FALSE,"UploadMgr UL: Put Last Log Data failed");
              task->FailCnt++;
              if(task->FailCnt  >= UPLOADMGR_LOG_DATA_FAIL_MAX)
              {
                task->State = FILE_UL_ERROR;
              }
            }
          }
          else //micro server response failed
          {
            task->State = FILE_UL_ERROR;
          }
        }
      }
      break;

    case FILE_UL_WAIT_END_UL_RSP:
      if(m_GotResponse)
      {
        if(TPU(ResponseStatusSuccess, eTpUldScr1245))
        {
          task->State = FILE_UL_FINISHED;
        }
        else if(TPU(ResponseStatusBusy, eTpUldScr1245))
        {
          //This resends the END LOG DATA cmd every 5s to poll the
          //microserver for the status of the file processing
          endLogPollDecimator = (endLogPollDecimator + 1) % 500;
          if(0 == endLogPollDecimator)
          {
            GSE_DebugStr(NORMAL,TRUE,
                  "UploadMgr: Waiting for MS to process file...");
            if( !UploadMgr_SendMSCmd(CMD_ID_END_LOG_DATA,5000,
                                    &task->LogFileInfo,
                                     sizeof(task->LogFileInfo)))
            {
              GSE_DebugStr(NORMAL,FALSE,"UploadMgr UL: Put End Log failed");
              task->FailCnt++;
              if(task->FailCnt  >= UPLOADMGR_END_LOG_FAIL_MAX)
              {
                task->State = FILE_UL_ERROR;
              }
            }
          }
        }
        else //failed or timeout, retry command
        {
          if( task->FailCnt++ < UPLOADMGR_END_LOG_FAIL_MAX)
          {
            if( !UploadMgr_SendMSCmd(CMD_ID_END_LOG_DATA,5000,
                                     &task->LogFileInfo,
                                     sizeof(task->LogFileInfo)))
            {
              GSE_DebugStr(NORMAL,FALSE,"UploadMgr UL: Put End Log failed");
              task->FailCnt++;
              if(task->FailCnt  >= UPLOADMGR_END_LOG_FAIL_MAX)
              {
                task->State = FILE_UL_ERROR;
              }
            }
          }
          else
          {
            task->State = FILE_UL_ERROR;
          }
        }
      }
      break;

    case FILE_UL_FINISHED:
      //All done, kill thy self
      GSE_DebugStr(VERBOSE,FALSE,"UploadMgr UL: Upload finished okay");
      TmTaskEnable(UL_Log_File_Upload, FALSE);
      break;

    case FILE_UL_ERROR:
      //Error has occurred. Log it and kill thy self
      LogWriteSystem(APP_UPM_INFO_UPLOAD_FILE_FAIL,LOG_PRIORITY_LOW,NULL,0,NULL);
      GSE_DebugStr(NORMAL,FALSE,"UploadMgr UL: Upload failed");
      TmTaskEnable(UL_Log_File_Upload, FALSE);
      break;

    default:
      FATAL("Unsupported UPLOADMGR_FILE_UL_STATE = %d",task->State);
      break;
  }
}



/******************************************************************************
 * Function:     UploadMgr_UploadFileBlock
 *
 * Description: Upload a chunk of data from the file data queue.  This routine
 *              removes a block of data from the queue and adds an upload
 *              header consisting of an "id" and "sequence" number.  The
 *              id is fixed at 0x1234 and the sequence number starts at 1 and
 *              increments for every subsequent block sent.
 *              The data block and header are sent to the micro-server
 *              interface.
 *
 * Parameters:  [in] Op: Upload operation to perform.
 *                       FIRST: Upload the first block of a file, this
 *                              initializes the upload block header and
 *                              the block sequence number.
 *                       NEXT:  Upload the next data block in the file data
 *                              queue.
 *                       LAST:  Upload the previously uploaded data block,
 *                              using the previous sequence number.  This
 *                              operation is used if the returned status for
 *                              an upload log data command is "fail"
 *
 *
 * Returns:     TRUE:  Log data block transmitted added to the micro-server
 *                     transmission queue, or there was no file data in the
 *                     FileDataFIFO.
 *              FALSE: Attempted to add data to the MSInterface and got an
 *                     error
 *
 * Notes:
 *
 *****************************************************************************/
static
BOOLEAN UploadMgr_UploadFileBlock(UPLOADMGR_UPLOAD_BLOCK_OP Op)
{
  BOOLEAN result = TRUE;
  static union  {
    MSCP_FILE_PACKET_INFO FileInfo;
    INT8 FileDataBuf[MSCP_CMDRSP_MAX_DATA_SIZE];
  } data_buf;
  static UINT32 txSize;

  //For the first data block sent, initialize the static file header information
  switch(Op)
  {
    case UPLOAD_BLOCK_FIRST:
      data_buf.FileInfo.id  = 0x1234;
      data_buf.FileInfo.seq = 0;
      //lint -fallthrough

    case UPLOAD_BLOCK_NEXT:
      //If no data to upload, take no action and wait for the next
      //frame to try again.
      if(FileDataFIFO.Cnt > 0)
      {
        //Seq number starts at 1 and increments by one for every
        //subsequent block.
        data_buf.FileInfo.seq += 1;
        //Compute how much data to send.  Either lesser of the following:
        // 1 .The maximum size that can be transmitted
        //    DataBuf size - FileInfo header
        // 2. The number of bytes remaining to be transmitted in the log data
        //    array
        txSize = MIN((sizeof(data_buf) - sizeof(data_buf.FileInfo)),
                      FileDataFIFO.CmpltCnt);
        FIFO_PopBlock(&FileDataFIFO,
                      &data_buf.FileDataBuf[sizeof(data_buf.FileInfo)],
                      txSize);
        result = UploadMgr_SendMSCmd(CMD_ID_LOG_DATA,5000,&data_buf,
                                     txSize+sizeof(data_buf.FileInfo));
        UploadMgr_PrintUploadStats(txSize);
      }
      break;

    case UPLOAD_BLOCK_LAST:
      //Re-upload the same block.
      result = UploadMgr_SendMSCmd(CMD_ID_LOG_DATA,5000,&data_buf,
                                        txSize+sizeof(data_buf.FileInfo));
      break;

    default:
      FATAL("Unsupported UPLOADMGR_UPLOAD_BLOCK_OP = %d", Op);
      break;
  }

  return result;
}



/******************************************************************************
 * Function:    UploadMgr_PrintUploadStats
 *
 * Description: Print upload statistics every second to the GSE port.  This
 *              routine must be called every time data is sent to the micro-
 *              server interface so that it can accumulate the total data
 *              sent.
 *
 * Parameters:  [in] BytesSent: Number of bytes sent to the dual MSInterface
 *                              Set this value to zero on the first call to
 *                              initialize the local static vars
 * Returns:
 *
 * Notes:
 *
 *****************************************************************************/
static
void UploadMgr_PrintUploadStats(UINT32 BytesSent)
{
  static UINT32 timeLastmS;
  static UINT32 timeElapsedmS;
  static UINT32 nextUpdateTimemS;
  static UINT32 totalBytesSent;
  static UINT32 totalMessagesSent;

  //Initialize local vars if this is running for the first time
  if(0 == BytesSent)
  {
    timeElapsedmS = 0;
    timeLastmS = CM_GetTickCount();
    nextUpdateTimemS = ONE_SEC_IN_MS;  //Update terminal 1s from now
    totalBytesSent = 0;
    totalMessagesSent = 0;
    GSE_StatusStr(NORMAL, "\r\n");
  }
  else
  {
    //Accumulate total time and data
    totalBytesSent += BytesSent;
    totalMessagesSent += 1;
    timeElapsedmS += (CM_GetTickCount() - timeLastmS);
    timeLastmS = CM_GetTickCount();

    //Print out status string every 1s
    if(timeElapsedmS > nextUpdateTimemS)
    {
      GSE_StatusStr( NORMAL,
                     "\rUL Task: Sent %d messages, %dkB @ %dmsg/S %dkB/S",
                     totalMessagesSent,
                     totalBytesSent/ONE_KB,
                     totalMessagesSent/(timeElapsedmS/ONE_SEC_IN_MS),
                     (totalBytesSent/ONE_KB)/(timeElapsedmS/ONE_SEC_IN_MS));
      nextUpdateTimemS += ONE_SEC_IN_MS;
    }
  }
}



/******************************************************************************
 * Function:    UploadMgr_SendMSCmd
 *
 * Description: Puts a micro-server message into the MSInterface queue.
 *              If the command is sent successfully, the global GotResponse
 *              flag is reset, otherwise the GotResponse flag is unchanged on
 *              return.
 *
 * Parameters:  [in] Id: Message ID (see MSCPInterface.h)
 *              [in] TO: Message timeout in milliseconds, this value is
 *                            defined by the application and depends on the
 *                            expected response time from the micro-server
 *              [in] Data:    Pointer to the block containing the message data
 *              [in] Size:    Size of the data, in bytes, pointed to by Data*
 *
 * Returns:     TRUE:  Message sent to the MSInterface queue successfully
 *              FALSE: MSInterface could not put the message in the queue
 *
 * Notes:
 *
 *****************************************************************************/
static
BOOLEAN UploadMgr_SendMSCmd(MSCP_CMD_ID Id, INT32 TO, void* Data, UINT32 Size)
{
  BOOLEAN result = FALSE;
  BOOLEAN gotResponseSave = m_GotResponse;

  //Reset the "GotResponse" Flag.  This must be done before sending the command
  //to prevent a race condition where Upload Task is held off after sending
  //a command and a response is received to the callback
  //before exiting this routine.
  m_GotResponse = FALSE;

  //Send the command
  if(SYS_OK == MSI_PutCommand(Id,Data,Size,TO,UploadMgr_MSRspCallback))
  {
    result = TRUE;
  }
  else
  {
    //Put Command failed, restore Got Response flag to original state.
    m_GotResponse = gotResponseSave;
  }
  return result;
}



/******************************************************************************
 * Function:    UploadMgr_MSCRCCmdHandler
 *
 * Description: This is a callback that is registered with the micro server
 *              interface module. It handles the command id CMD_ID_VALIDATE_CRC
 *              from the MSSIM application on the micro server.
 *              In the command packet is the file name and a CRC value computed
 *              by MSSIM or the ground server.  This function looks up the CRC
 *              associated with the command file name in the File Verification
 *              Table, and compares the CRC value.  If the CRC matches or fails,
 *              the file status is updated in the File Verification Table and
 *              The result of this comparison is returned MSSIM in the response
 *
 *              The function prototype is driven by the MSI command handler
 *              typedef
 * Parameters:  [in] Data: Pointer to the command data block
 *              [in] Size: Size, in bytes, of the data block
 *              [in] Sequence: Command sequence identifier, this is returned
 *                             in the response to uniquely identify this
 *                             command/response transaction.  See the MSI
 *                             design
 *
 * Returns:    TRUE:  Command processing succeeded, response sent
 *             FALSE: Command processing failed (Data Size bad),
 *                    no response sent
 *
 * Notes:
 *
 *****************************************************************************/
static
BOOLEAN UploadMgr_MSCRCCmdHandler(void* Data,UINT16 Size,
                                          UINT32 Sequence)
{
  MSCP_VALIDATE_CRC_CMD*      vfyCmdData = Data;
  UPLOADMGR_FILE_VFY          vfyRow;
  MSCP_VALIDATE_CRC_RSP       crcRsp;
  UPLOAD_CRC_FAIL             crcFailLog;
  UPLOAD_GS_TRANSMIT_SUCCESS  gsCrcPassLog;
  INT32   row;
  BOOLEAN retval;

  if(Size == sizeof(*vfyCmdData))
  {
    //Look up the file
    GSE_DebugStr(NORMAL,TRUE,"UploadMgr: Got VfyCRCCmd for %s",vfyCmdData->FN);
    row = UploadMgr_GetFileVfy(vfyCmdData->FN,&vfyRow);
    if(row >= 0)
    {
      if((vfyCmdData->CRC == vfyRow.CRC) && (VFY_STA_DELETED != vfyRow.State))
      {
        switch(vfyCmdData->CRCSrc)
        {
          //Signal to Log Collection task the last file uploaded passed vfy.  File state
          //is update only after Log Collection task verifies the file was moved to CP_FILES
        case MSCP_CRC_SRC_MS:   //CRC from the MicroServer
          m_MSCrcResult = MS_CRC_RESULT_VFY;
          break;

        case MSCP_CRC_SRC_GS:   //CRC from the ground server
          UploadMgr_UpdateFileVfy(row,VFY_STA_GROUNDVFY,0,0);

          // log system message
          strncpy_safe(gsCrcPassLog.filename, sizeof(gsCrcPassLog.filename),
              vfyCmdData->FN, _TRUNCATE);
          LogWriteSystem(APP_UPM_GS_TRANSMIT_SUCCESS, LOG_PRIORITY_LOW,
              &gsCrcPassLog, sizeof(gsCrcPassLog), NULL);
          GSE_DebugStr(NORMAL,TRUE,
              "UploadMgr: Got Ground CRC for %s",vfyCmdData->FN);
          GSE_DebugStr(NORMAL,TRUE,
              "UploadMgr: CRC OK!, verifying log file was moved...");

          //The Maintain File Table task will eventually delete the entry, but
          //if it not running start the Check_Log task so the entry will be
          //verified and deleted immediatly
          if(!m_MaintainFileTableRunning)
          {
            m_CheckLogFileCnt++;
            TmTaskEnable(UL_Log_Check,TRUE);
          }
          break;

        default:
          FATAL("Unrecognized MSCP_CRC_SRC = %d",vfyCmdData->CRCSrc);
          break;
        }
        crcRsp.CRCRes = MSCP_CRC_RES_PASS;
      }
      else if(VFY_STA_DELETED != vfyRow.State)
      {
        switch(vfyCmdData->CRCSrc)
        {

        case MSCP_CRC_SRC_MS:   //CRC from the MicroServer
          //Signal to Log Collection task the last file uploaded failed vfy
          UploadMgr_UpdateFileVfy(row, VFY_STA_MSFAIL, 0,0);
          m_MSCrcResult = MS_CRC_RESULT_FAIL;

          // log system message
          crcFailLog.fvt_crc = vfyRow.CRC;
          crcFailLog.src_crc = vfyCmdData->CRC;
          strncpy_safe(crcFailLog.filename, sizeof(crcFailLog.filename),
              vfyCmdData->FN, _TRUNCATE);
          LogWriteSystem(APP_UPM_MS_CRC_FAIL, LOG_PRIORITY_LOW,
              &crcFailLog, sizeof(crcFailLog), NULL);
          GSE_DebugStr(NORMAL,TRUE,
              "UploadMgr: Got MS CRC for %s",vfyCmdData->FN);
          GSE_DebugStr(NORMAL, FALSE,
              "UploadMgr: CRC FAILED, from MS CP:%08x MS:%08x",
              vfyRow.CRC, vfyCmdData->CRC);
          break;

        case MSCP_CRC_SRC_GS:   //CRC from the ground server
          // log system message
          crcFailLog.fvt_crc = vfyRow.CRC;
          crcFailLog.src_crc = vfyCmdData->CRC;
          strncpy_safe(crcFailLog.filename, sizeof(crcFailLog.filename),
              vfyCmdData->FN, _TRUNCATE);
          LogWriteSystem(APP_UPM_GS_CRC_FAIL, LOG_PRIORITY_LOW,
              &crcFailLog, sizeof(crcFailLog), NULL);

          GSE_DebugStr(NORMAL, FALSE,
              "UploadMgr: CRC FAILED, from ground CP:%08x Ground:%08x",
              vfyRow.CRC, vfyCmdData->CRC);
          UploadMgr_UpdateFileVfy(row, VFY_STA_DELETED , 0,0);
          break;

        default:
          FATAL("Unrecognized MSCP_CRC_SRC = %d", vfyCmdData->CRCSrc);
          break;
        }
        crcRsp.CRCRes = MSCP_CRC_RES_FAIL;
      }
      else
      {
        //File not found, it was deleted.....
        GSE_DebugStr(NORMAL,FALSE,
            "UploadMgr: CRC FAILED, File not found (DELETED) %s",vfyRow.Name);
        crcRsp.CRCRes = MSCP_CRC_RES_FNF;
      }
    }
    else
    {
      //FN not found, or NV_ read error
      crcRsp.CRCRes = MSCP_CRC_RES_FNF;
      GSE_DebugStr(NORMAL,FALSE,"UploadMgr: CRC FAILED, File not found %s",vfyCmdData->FN);
    }
  }

  MSI_PutResponse(CMD_ID_VALIDATE_CRC,&crcRsp,(Size == sizeof(*vfyCmdData) ?
                  MSCP_RSP_STATUS_SUCCESS : MSCP_RSP_STATUS_FAIL),sizeof(crcRsp),Sequence);
  retval = TRUE;


  return retval;
}


/******************************************************************************
 * Function:    UploadMgr_MSRspCallback
 *
 * Description: Callback from the MSInterface to handle responses from the
 *              micro-server.  The parameter list matches the callback typedef
 *              in MSInterface.h
 *
 * Parameters:  [in]    Id:         ID of the response
 *              [in/out]PacketData  Pointer to response data
 *              [in]    Size        size of the response data
 *              [in]    Status      Response status, (Success,fail,timeout)
 *
 * Returns:
 *
 * Notes:       This routine executes in the context of the MSInterface task
 *              and should be coded to execute quickly.  MSInterfacae task may
 *              also have higher priority, so global data changed in this
 *              routine should be considered volatile.
 *
 *****************************************************************************/
static
void UploadMgr_MSRspCallback(UINT16 Id, void* PacketData, UINT16 Size,
                             MSI_RSP_STATUS Status)
{
  const INT8* statusStrs[MSI_RSP_TIMEOUT+1] =
    {"Success", "Busy", "Failed", "Timeout"};

  ResponseStatusSuccess = MSI_RSP_SUCCESS == Status;
  ResponseStatusBusy    = MSI_RSP_BUSY == Status;
  ResponseStatusTimeout = MSI_RSP_TIMEOUT == Status;


  if(MSI_RSP_SUCCESS != Status)
  {
    GSE_DebugStr(VERBOSE,FALSE,"UploadMgr: MS Response Id %d, Status %s",Id,
               Status <= MSI_RSP_TIMEOUT ? statusStrs[Status]:"Unknown");
  }

  m_GotResponse = TRUE;
}

/******************************************************************************
* Function:    UploadMgr_PrintInstallationInfo
*
* Description: Prints out FAST SerialNumber, installation info and date & time
*              for the upload.
*
* Parameters:  none
*
* Returns:     none
*
* Notes:
*
*
*
*****************************************************************************/
static
void UploadMgr_PrintInstallationInfo()
{
  TIMESTAMP ts;
  TIMESTRUCT time_struct;
  INT8   boxStr[BOX_INFO_STR_LEN];

  GSE_DebugStr( NORMAL, FALSE, "\r\n");

  //***********************
  // FAST Installation Id#
  //***********************
  GSE_DebugStr( NORMAL, TRUE, "UploadMgr: FAST Installation #: %s",
      CfgMgr_RuntimeConfigPtr()->Id);

  //***********************
  // Box Serial Number
  //***********************
  Box_GetSerno(boxStr);
  GSE_DebugStr( NORMAL, TRUE, "UploadMgr: FAST Serial #: %s", boxStr);

  //****************************
  // FAST Configuration Version #
  //****************************
  GSE_DebugStr( NORMAL, TRUE, "UploadMgr: FAST Config Version #: %d",
      CfgMgr_RuntimeConfigPtr()->VerId);

  //****************************
  // Software Version  #
  //****************************
  GSE_DebugStr( NORMAL, TRUE, "UploadMgr: S/W Version: %s", PRODUCT_VERSION );

  //****************************
  // Software CRC Info
  //****************************
  GSE_DebugStr( NORMAL, TRUE, "UploadMgr: S/W CRC: %08X , Cfg CRC: %04X",
                              __CRC_END,
                              NV_GetFileCRC(NV_CFG_MGR) );

  //***********************
  // Date & Time
  //***********************
  CM_GetTimeAsTimestamp( &ts );
  CM_ConvertTimeStamptoTimeStruct( &ts, &time_struct );

  GSE_DebugStr( NORMAL, TRUE,
                "UploadMgr: Upload Start Time: %02d/%02d/%04d %02d:%02d:%02d.%03d",
                time_struct.Month,
                time_struct.Day,
                time_struct.Year,
                time_struct.Hour,
                time_struct.Minute,
                time_struct.Second,
                time_struct.MilliSecond);

  GSE_DebugStr( NORMAL, FALSE, "\r\n");
}


/*************************************************************************
 *  MODIFICATIONS
 *    $History: UploadMgr.c $
 * 
 * *****************  Version 193  *****************
 * User: John Omalley Date: 6/02/16    Time: 3:16p
 * Updated in $/software/control processor/code/application
 * SCR 1334 - Requirements Update
 * 
 * *****************  Version 192  *****************
 * User: John Omalley Date: 6/02/16    Time: 1:32p
 * Updated in $/software/control processor/code/application
 * SCR 1334 - Code Review Update
 * 
 * *****************  Version 190  *****************
 * User: John Omalley Date: 5/31/16    Time: 9:15a
 * Updated in $/software/control processor/code/application
 * SCR 1334 - Modified to hardcoded "-SYS" and "-ETM-"
 * 
 * *****************  Version 189  *****************
 * User: John Omalley Date: 5/24/16    Time: 9:34a
 * Updated in $/software/control processor/code/application
 * SCR 1334 - Added configurable ETM and SYS filenames
 * 
 * *****************  Version 188  *****************
 * User: John Omalley Date: 5/09/16    Time: 2:22p
 * Updated in $/software/control processor/code/application
 * SCR 1329 - Corrected the error path where the Upload Count wasn't being
 * reset.
 * 
 * *****************  Version 187  *****************
 * User: John Omalley Date: 1/21/15    Time: 1:10p
 * Updated in $/software/control processor/code/application
 * SCR 1253 - ModFix because the ETM log check was using the wrong field.
 * 
 * *****************  Version 186  *****************
 * User: John Omalley Date: 1/14/15    Time: 10:31a
 * Updated in $/software/control processor/code/application
 * SCR 1254 - Code Review Updates
 * 
 * *****************  Version 184  *****************
 * User: John Omalley Date: 11/18/14   Time: 2:36p
 * Updated in $/software/control processor/code/application
 * SCR 1254 - Fixed ENUMERATION NAME
 * 
 * *****************  Version 182  *****************
 * User: Contractor2  Date: 11/05/14   Time: 3:15p
 * Updated in $/software/control processor/code/application
 * SCR-1274: Correct bad merge
 *
 * *****************  Version 181  *****************
 * User: Contractor2  Date: 11/05/14   Time: 3:05p
 * Updated in $/software/control processor/code/application
 * SCR-1274: v2.1.0 Instrumented Test Point additions
 * Test for SCR-1245: File Upload command "END_LOG" does not timeout
 * properly
 *
 * *****************  Version 180  *****************
 * User: Contractor V&v Date: 11/04/14   Time: 7:59p
 * Updated in $/software/control processor/code/application
 * SCR #1092 - Forceupload recording-in-progress notification.
 * Modfix-fixed start source
 *
 * *****************  Version 179  *****************
 * User: Contractor V&v Date: 11/03/14   Time: 5:25p
 * Updated in $/software/control processor/code/application
 * SCR #1092 - Forceupload recording-in-progress notification. Modfix-rec
 * flag change
 *
 * *****************  Version 178  *****************
 * User: Contractor V&v Date: 10/06/14   Time: 4:04p
 * Updated in $/software/control processor/code/application
 * SCR #1092 - Forceupload recording-in-progress notification.
 *
 * *****************  Version 177  *****************
 * User: Contractor V&v Date: 10/02/14   Time: 3:59p
 * Updated in $/software/control processor/code/application
 * SCR #1266 - FVT runtime Free count does not count Pri 4 files
 *
 * *****************  Version 176  *****************
 * User: Contractor V&v Date: 9/03/14    Time: 5:11p
 * Updated in $/software/control processor/code/application
 * SCR #1253 - ETM File Header Log Count Wrong Code Review changes
 *
 * *****************  Version 175  *****************
 * User: Contractor V&v Date: 8/26/14    Time: 3:11p
 * Updated in $/software/control processor/code/application
 * SCR #1253 - ETM File Header Log Count Wrong
 *
 * *****************  Version 174  *****************
 * User: Contractor V&v Date: 8/20/14    Time: 11:04a
 * Updated in $/software/control processor/code/application
 * "SCR #1245 - File Upoad command ""END_LOG"" does not timeout properly.
 * SCR #1254
 *  Upload Mgr ""Auto Upload"" does not exit properly when canceled"
 *
 * *****************  Version 173  *****************
 * User: Contractor V&v Date: 8/12/14    Time: 5:01p
 * Updated in $/software/control processor/code/application
 * Fix legacy Battery latch when FSM enabled
 *
 * *****************  Version 172  *****************
 * User: Jim Mood     Date: 2/11/13    Time: 2:43p
 * Updated in $/software/control processor/code/application
 * SCR# 1233 UploadMgr_DeleteFVTEntry() improperly set return value
 *
 * *****************  Version 171  *****************
 * User: Jim Mood     Date: 2/06/13    Time: 3:36p
 * Updated in $/software/control processor/code/application
 * SCR #1231 Modfix to SCR #1098
 *
 * *****************  Version 170  *****************
 * User: Jim Mood     Date: 1/07/13    Time: 11:24a
 * Updated in $/software/control processor/code/application
 * SCR #1198 Assert if binary header is overfilled
 *
 * *****************  Version 169  *****************
 * User: Jim Mood     Date: 12/27/12   Time: 4:50p
 * Updated in $/software/control processor/code/application
 * SCR #1197 Code Review Updates
 *
 * *****************  Version 168  *****************
 * User: Jim Mood     Date: 12/20/12   Time: 6:26p
 * Updated in $/software/control processor/code/application
 * SCR #1197 Code Review Changes
 *
 * *****************  Version 167  *****************
 * User: Jim Mood     Date: 12/14/12   Time: 8:05p
 * Updated in $/software/control processor/code/application
 * SCR #1197
 *
 * *****************  Version 166  *****************
 * User: Jim Mood     Date: 11/29/12   Time: 9:38a
 * Updated in $/software/control processor/code/application
 * SCR #1123 Modfix for debug strings
 *
 * *****************  Version 165  *****************
 * User: Jim Mood     Date: 11/27/12   Time: 8:34p
 * Updated in $/software/control processor/code/application
 * SCR# 1166 Auto Upload Task Overrun (from FASTMgr.c)
 * SCR #1136 Auto Upload on ETM and Data Logs present
 *
 * *****************  Version 164  *****************
 * User: Jim Mood     Date: 11/19/12   Time: 3:56p
 * Updated in $/software/control processor/code/application
 * SCR #1135 Findflight error
 * SCR #1137 Fix skip mark for deletion
 *
 * *****************  Version 163  *****************
 * User: John Omalley Date: 12-11-12   Time: 9:49a
 * Updated in $/software/control processor/code/application
 * SCR 1105, 1107, 1131, 1154 - Code Review Updates
 *
 * *****************  Version 162  *****************
 * User: John Omalley Date: 12-11-09   Time: 10:32a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Code Review Updates
 *
 * *****************  Version 161  *****************
 * User: Melanie Jutras Date: 12-10-31   Time: 12:01p
 * Updated in $/software/control processor/code/application
 * SCR #1172 PCLint 545 Suspicious use of & Error
 *
 * *****************  Version 160  *****************
 * User: John Omalley Date: 12-09-27   Time: 9:13a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Added Engine Identification Fields to software and file
 * header
 *
 * *****************  Version 159  *****************
 * User: John Omalley Date: 12-09-11   Time: 2:03p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Added Logic for the ETM file and ETM Binary Header
 *
 * *****************  Version 158  *****************
 * User: Jeff Vahue   Date: 8/29/12    Time: 1:21p
 * Updated in $/software/control processor/code/application
 * Code Review Tool Findings
 *
 * *****************  Version 157  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 12:43p
 * Updated in $/software/control processor/code/application
 * SCR# 1142
 *
 * *****************  Version 156  *****************
 * User: Jim Mood     Date: 7/26/12    Time: 4:57p
 * Updated in $/software/control processor/code/application
 * SCR 1076: Code review updates
 *
 * *****************  Version 154  *****************
 * User: Jim Mood     Date: 7/19/12    Time: 10:42a
 * Updated in $/software/control processor/code/application
 * SCR 1107: Data Offload changes for 2.0.0
 * SCR 1129: Upload retry on CF CRC failure
 *
 * *****************  Version 153  *****************
 * User: Jim Mood     Date: 2/24/12    Time: 10:32a
 * Updated in $/software/control processor/code/application
 * SCR 1114 - Re labeled after v1.1.1 release
 *
 * *****************  Version 151  *****************
 * User: Contractor V&v Date: 1/11/12    Time: 11:54a
 * Updated in $/software/control processor/code/application
 * SCR #1098 Upload Mgr "CheckFileMoved" function loop limit
 *
 * *****************  Version 150  *****************
 * User: Contractor V&v Date: 12/14/11   Time: 6:49p
 * Updated in $/software/control processor/code/application
 * SCR #1105 End of Flight Log Race Condition
 *
 * *****************  Version 149  *****************
 * User: Jim Mood     Date: 10/20/11   Time: 5:38p
 * Updated in $/software/control processor/code/application
 * SCR 1094: Code Review changes
 *
 * *****************  Version 148  *****************
 * User: Jim Mood     Date: 9/29/11    Time: 1:57p
 * Updated in $/software/control processor/code/application
 * SCR 1080  UploadFileBlock FIFO error.
 *
 * *****************  Version 147  *****************
 * User: Jim Mood     Date: 9/27/11    Time: 3:18p
 * Updated in $/software/control processor/code/application
 * SCR 575 Modfix.  Changed LOG_COLLECTION_INIT_AUTO state to wait if
 * LogFindNextRecord returns busy.  This fixes an issue where AutoUpload
 * did not proceed if called immediatly after writing a log.
 *
 * *****************  Version 146  *****************
 * User: Jim Mood     Date: 9/23/11    Time: 8:02p
 * Updated in $/software/control processor/code/application
 * SCR 575.  Modfix for getting stuck in 'disabled' mode when using the
 * FSM
 *
 * *****************  Version 145  *****************
 * User: John Omalley Date: 9/15/11    Time: 7:30p
 * Updated in $/software/control processor/code/application
 * SCR 1072 - Reset the readOffset in FileCollection Task so next auto
 * upload will start at the begining of memory.
 *
 * *****************  Version 144  *****************
 * User: John Omalley Date: 9/14/11    Time: 1:42p
 * Updated in $/software/control processor/code/application
 * SCR 1073 - Fixed double print of upload started debug message
 *
 * *****************  Version 143  *****************
 * User: John Omalley Date: 9/13/11    Time: 2:28p
 * Updated in $/software/control processor/code/application
 * SCR 1073 - Move Upload Process Start Debug String
 *
 * *****************  Version 142  *****************
 * User: Jim Mood     Date: 8/25/11    Time: 6:12p
 * Updated in $/software/control processor/code/application
 * SCR 575 Modfix
 *
 * *****************  Version 141  *****************
 * User: Jim Mood     Date: 7/20/11    Time: 10:54a
 * Updated in $/software/control processor/code/application
 * SCR 575: GSM Enable when engine status is lost.  (Part of changes for
 * the Fast State Machine)
 *
 * *****************  Version 140  *****************
 * User: Jim Mood     Date: 11/04/10   Time: 7:19p
 * Updated in $/software/control processor/code/application
 * SCR 984. Deleting ground verify FVT entries
 *
 * *****************  Version 139  *****************
 * User: Jim Mood     Date: 11/01/10   Time: 10:26a
 * Updated in $/software/control processor/code/application
 * SCR 972 String format error
 *
 * *****************  Version 138  *****************
 * User: Jim Mood     Date: 10/27/10   Time: 6:33p
 * Updated in $/software/control processor/code/application
 * SCR 969 Code Coverage issues
 *
 * *****************  Version 137  *****************
 * User: Jim Mood     Date: 10/26/10   Time: 9:44a
 * Updated in $/software/control processor/code/application
 * SCR 963 Write CRC to FVT before sending EndLog to MS
 *
 * *****************  Version 136  *****************
 * User: Jim Mood     Date: 10/21/10   Time: 7:51p
 * Updated in $/software/control processor/code/application
 * SCR 959 Maint. Task refactoring
 *
 * *****************  Version 135  *****************
 * User: Jim Mood     Date: 10/21/10   Time: 6:08p
 * Updated in $/software/control processor/code/application
 * SCR 959
 *
 * *****************  Version 133  *****************
 * User: Jim Mood     Date: 10/18/10   Time: 5:35p
 * Updated in $/software/control processor/code/application
 * SCR 944 Bad file CRC on logreadbusy
 * SCR 945 FreeFVTRows not incremented on delete in some paths
 *
 * *****************  Version 132  *****************
 * User: Contractor2  Date: 10/14/10   Time: 3:47p
 * Updated in $/software/control processor/code/application
 * SCR #930 Code Review: UploadMgr
 *
 * *****************  Version 131  *****************
 * User: Contractor2  Date: 10/14/10   Time: 1:55p
 * Updated in $/software/control processor/code/application
 * SCR #930 Code Review: UploadMgr
 *
 * *****************  Version 130  *****************
 * User: Jim Mood     Date: 10/06/10   Time: 7:29p
 * Updated in $/software/control processor/code/application
 * SCR 880: Battery latch while files not round tripped
 * SCR 922: Remaining TODOs and causing bad file checksums because of them
 *
 * *****************  Version 129  *****************
 * User: Contractor2  Date: 10/05/10   Time: 2:57p
 * Updated in $/software/control processor/code/application
 * SCR #916 Code Review Updates
 *
 * *****************  Version 128  *****************
 * User: Jim Mood     Date: 9/30/10    Time: 1:30p
 * Updated in $/software/control processor/code/application
 * SCR 900 Code Coverage updates
 *
 * *****************  Version 127  *****************
 * User: John Omalley Date: 9/30/10    Time: 10:01a
 * Updated in $/software/control processor/code/application
 * SCR 888 - Forgot to remove free file being incremented
 *
 * *****************  Version 126  *****************
 * User: John Omalley Date: 9/24/10    Time: 9:27a
 * Updated in $/software/control processor/code/application
 * SCR 888 - Corrected file deletion when log erase is performed.
 *
 * *****************  Version 125  *****************
 * User: Jeff Vahue   Date: 9/21/10    Time: 5:48p
 * Updated in $/software/control processor/code/application
 * SCR# 880 - Upload Busy Logic
 *
 * *****************  Version 124  *****************
 * User: John Omalley Date: 9/09/10    Time: 9:01a
 * Updated in $/software/control processor/code/application
 * SCR 790 - Changed last offset from 0xfffffffe to use the actual last
 * used address
 *
 * *****************  Version 123  *****************
 * User: Jim Mood     Date: 8/10/10    Time: 5:58p
 * Updated in $/software/control processor/code/application
 * SCR 756.  Maintain a running count of free FVT rows to avoid the
 * computation time of counting the number of rows at the start of a
 * flight.
 *
 * *****************  Version 122  *****************
 * User: Jim Mood     Date: 8/09/10    Time: 3:57p
 * Updated in $/software/control processor/code/application
 * SCR 751 upload loop
 *
 * *****************  Version 121  *****************
 * User: Contractor V&v Date: 7/30/10    Time: 9:21p
 * Updated in $/software/control processor/code/application
 * SCR #282 Misc - Shutdown Processing
 *
 * *****************  Version 120  *****************
 * User: John Omalley Date: 7/30/10    Time: 8:50a
 * Updated in $/software/control processor/code/application
 * SCR 678 - Remove files in the FVT that are marked not deleted
 *
 * *****************  Version 119  *****************
 * User: Jim Mood     Date: 7/30/10    Time: 8:42a
 * Updated in $/software/control processor/code/application
 * SCR 327 Fast.maintainence functionality.  Added a function to get the
 * number of files pending ground verifiication
 *
 * *****************  Version 118  *****************
 * User: Jim Mood     Date: 7/23/10    Time: 7:38p
 * Updated in $/software/control processor/code/application
 * Redo SCR 639, because I checked in over it.
 *
 * *****************  Version 117  *****************
 * User: Jim Mood     Date: 7/23/10    Time: 7:28p
 * Updated in $/software/control processor/code/application
 * SCR 151 Delete FVT entry after good ground crc is received
 * SCR 152 Delete FVT after bad ground crc is received
 * SCR 208 Upload to MSSIM performance with large numbers of small
 * logs(process multiple logs per MIF to improve performance)
 * SCR 296 Fix possible erronous marking of logs as verified
 *
 * *****************  Version 114  *****************
 * User: John Omalley Date: 7/20/10    Time: 9:03a
 * Updated in $/software/control processor/code/application
 * SCR 294 - Add system binary header
 *
 * *****************  Version 113  *****************
 * User: Contractor2  Date: 7/14/10    Time: 1:24p
 * Updated in $/software/control processor/code/application
 * SCR #697 Misc - Two declarations of m_TotalVerifyRows and
 * m_FreeVerifyRows
 * Removed extra declarations
 *
 * *****************  Version 112  *****************
 * User: Jim Mood     Date: 7/02/10    Time: 4:06p
 * Updated in $/software/control processor/code/application
 * SCR 671 Replaced end-log timeout workaround with a MS response
 * indicating the micro-server end log thread is busy
 *
 * *****************  Version 111  *****************
 * User: Jim Mood     Date: 6/29/10    Time: 6:25p
 * Updated in $/software/control processor/code/application
 * SCR 623 Reconfiguration changes.  Log file header now gets the
 * CfgStatus from access function instead of the config manager file
 * because the CfgStatus is no longer part of the configuration file.
 *
 * *****************  Version 110  *****************
 * User: Contractor2  Date: 6/24/10    Time: 11:01a
 * Updated in $/software/control processor/code/application
 * SCR #660 Implementation: SRS-2477 - Delete File when GS CRC does not
 * match FVT CRC
 * Minor coding standard fixes
 *
 * *****************  Version 109  *****************
 * User: Contractor2  Date: 6/18/10    Time: 11:39a
 * Updated in $/software/control processor/code/application
 * SCR #611 Write Logs  on CRC Pass/Fail
 * Added sys log for SRS-2465.
 * Minor coding standard fixes.
 *
 * *****************  Version 108  *****************
 * User: Jeff Vahue   Date: 6/15/10    Time: 1:20p
 * Updated in $/software/control processor/code/application
 * Typo fix in comment
 *
 * *****************  Version 107  *****************
 * User: Jim Mood     Date: 6/11/10    Time: 4:14p
 * Updated in $/software/control processor/code/application
 * SCR 623 Batch configuration implementation updates
 *
 * *****************  Version 106  *****************
 * User: Contractor V&v Date: 6/08/10    Time: 5:51p
 * Updated in $/software/control processor/code/application
 * SCR #614 fast.reset=really should initialize files
 *
 * *****************  Version 105  *****************
 * User: John Omalley Date: 6/08/10    Time: 12:03p
 * Updated in $/software/control processor/code/application
 * SCR 627 - Updated for LJ60
 *
 * *****************  Version 104  *****************
 * User: Jeff Vahue   Date: 6/01/10    Time: 6:40p
 * Updated in $/software/control processor/code/application
 * Add Info to Assert Statement
 *
 * *****************  Version 103  *****************
 * User: Contractor V&v Date: 5/26/10    Time: 6:54p
 * Updated in $/software/control processor/code/application
 * SCR #616 Another Assert
 * SCR # 404 Reverted ASSERT back to if statement on row check
 *
 * *****************  Version 102  *****************
 * User: Contractor V&v Date: 5/26/10    Time: 2:10p
 * Updated in $/software/control processor/code/application
 * SCR #607 The CP shall query Ms CF mounted before File Upload pr
 *
 * *****************  Version 101  *****************
 * User: Contractor2  Date: 5/25/10    Time: 3:04p
 * Updated in $/software/control processor/code/application
 * SCR #611 Write Logs  on CRC Pass/Fail
 * Added sys logs. Minor coding standard fixes.
 *
 * *****************  Version 100  *****************
 * User: Contractor V&v Date: 5/19/10    Time: 6:10p
 * Updated in $/software/control processor/code/application
 * SCR #404 Misc - Preliminary Code Review Issues
 *
 * *****************  Version 99  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:53p
 * Updated in $/software/control processor/code/application
 * SCR #587 Change TmTaskCreate to return void
 *
 * *****************  Version 98  *****************
 * User: Jeff Vahue   Date: 4/28/10    Time: 5:41p
 * Updated in $/software/control processor/code/application
 * SCR #572, #573 - Vcast changes and Startup WD changes
 *
 * *****************  Version 97  *****************
 * User: Jeff Vahue   Date: 4/23/10    Time: 6:33p
 * Updated in $/software/control processor/code/application
 * Remove NL from end of lines on PrintINstallInfo as they are redundant
 * with the use of GSE_DebugStr.
 *
 * *****************  Version 96  *****************
 * User: Jeff Vahue   Date: 4/21/10    Time: 5:03p
 * Updated in $/software/control processor/code/application
 * Add the comment below to the history as it was checked in without it
 * last version:
 *
 * Changed GSE_Putline to GSE_DebugStr as all non-required messages
 * must go out via GSE_DebugStr
 *
 * *****************  Version 95  *****************
 * User: Jeff Vahue   Date: 4/21/10    Time: 4:53p
 * Updated in $/software/control processor/code/application
 * Changed GSE_Putline to GSE_DebugStr as all non-required messages
 * must go out via GSE_DebugStr
 *
 * *****************  Version 94  *****************
 * User: Contractor2  Date: 4/20/10    Time: 11:43a
 * Updated in $/software/control processor/code/application
 * SCR 558: Implementation: SRS-3663 - log file naming convention change
 *
 * *****************  Version 93  *****************
 * User: Contractor V&v Date: 4/12/10    Time: 7:18p
 * Updated in $/software/control processor/code/application
 * SCR #543 Misc code and comment changes
 *
 * *****************  Version 92  *****************
 * User: Jeff Vahue   Date: 4/12/10    Time: 5:43p
 * Updated in $/software/control processor/code/application
 * SCR #541 - all XXX_CreateLogs functions are now dependent on
 * GENERATE_SYS_LOGS
 *
 * *****************  Version 91  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:07p
 * Updated in $/software/control processor/code/application
 * SCR #317 Implement safe strncpy SCR #70 Store/Restore interrupt
 *
 * *****************  Version 90  *****************
 * User: Jeff Vahue   Date: 3/26/10    Time: 5:36p
 * Updated in $/software/control processor/code/application
 * SCR #509 - pulse WD while reseting File Verification Table
 *
 * *****************  Version 89  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:37p
 * Updated in $/software/control processor/code/application
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 *
 * *****************  Version 88  *****************
 * User: Contractor V&v Date: 3/19/10    Time: 4:28p
 * Updated in $/software/control processor/code/application
 * SCR #279 Making NV filenames uppercase
 *
 * *****************  Version 87  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/application
 * SCR# 483 - Function Names
 *
 * *****************  Version 86  *****************
 * User: Jim Mood     Date: 3/12/10    Time: 3:46p
 * Updated in $/software/control processor/code/application
 * SCR #452 ModFix
 *
 * *****************  Version 85  *****************
 * User: Contractor2  Date: 3/02/10    Time: 11:56a
 * Updated in $/software/control processor/code/application
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 84  *****************
 * User: Jeff Vahue   Date: 2/26/10    Time: 11:17a
 * Updated in $/software/control processor/code/application
 * SCR# 243 (b) and SRS-2650 - Wait for MS ready to begin upload
 *
 * *****************  Version 83  *****************
 * User: Jeff Vahue   Date: 2/19/10    Time: 12:56p
 * Updated in $/software/control processor/code/application
 * SCR# 455 remove LINT issues
 *
 * *****************  Version 82  *****************
 * User: Jeff Vahue   Date: 2/17/10    Time: 12:51p
 * Updated in $/software/control processor/code/application
 * SCR# 452 - Code Coverage if/then/else changes
 *
 * *****************  Version 81  *****************
 * User: Jeff Vahue   Date: 2/09/10    Time: 1:35p
 * Updated in $/software/control processor/code/application
 * SCR# 364 - UploadMgr_FileCollectionTask: Default case FindLogResult is
 * not the correct var, use 'Task->State'
 *
 * *****************  Version 80  *****************
 * User: Contractor V&v Date: 2/03/10    Time: 2:56p
 * Updated in $/software/control processor/code/application
 * SCR 279
 *
 * *****************  Version 79  *****************
 * User: Contractor V&v Date: 1/27/10    Time: 5:17p
 * Updated in $/software/control processor/code/application
 * SCR 267 Upload DebugMessage Content
 *
 * *****************  Version 78  *****************
 * User: Jim Mood     Date: 1/27/10    Time: 5:13p
 * Updated in $/software/control processor/code/application
 * SCR #430
 *
 * *****************  Version 77  *****************
 * User: Jim Mood     Date: 1/20/10    Time: 3:46p
 * Updated in $/software/control processor/code/application
 * Added public function to delete an FVT entry by log file name.  Initial
 * user of the function is the MSFileXfr file.
 *
 * *****************  Version 76  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 4:59p
 * Updated in $/software/control processor/code/application
 * SCR# 397
 *
 * *****************  Version 75  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:20p
 * Updated in $/software/control processor/code/application
 * SCR 371
 *
 * *****************  Version 74  *****************
 * User: Jim Mood     Date: 12/23/09   Time: 5:51p
 * Updated in $/software/control processor/code/application
 * SCR 205
 *
 * *****************  Version 73  *****************
 * User: Jim Mood     Date: 12/23/09   Time: 5:35p
 * Updated in $/software/control processor/code/application
 * SCR #205
 *
 * *****************  Version 72  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 2:11p
 * Updated in $/software/control processor/code/application
 * SCR #326
 *
 * *****************  Version 71  *****************
 * User: Jeff Vahue   Date: 12/18/09   Time: 1:33p
 * Updated in $/software/control processor/code/application
 * SCR# 378
 *
 * *****************  Version 70  *****************
 * User: Contractor V&v Date: 12/10/09   Time: 5:39p
 * Updated in $/software/control processor/code/application
 * SCR 106
 *
 * *****************  Version 69  *****************
 * User: Jeff Vahue   Date: 12/04/09   Time: 4:31p
 * Updated in $/software/control processor/code/application
 * SCR# 350
 *
 * *****************  Version 68  *****************
 * User: Contractor V&v Date: 11/18/09   Time: 3:12p
 * Updated in $/software/control processor/code/application
 * Fix for SCR 172.
 * Fix for SCR 138
 *
 * *****************  Version 67  *****************
 * User: Jim Mood     Date: 11/05/09   Time: 3:03p
 * Updated in $/software/control processor/code/application
 * Updated number of file verification entries from 180 to 200 per
 * requirments.
 *
 * *****************  Version 66  *****************
 * User: Jim Mood     Date: 10/19/09   Time: 6:30p
 * Updated in $/software/control processor/code/application
 * Filled in box serial number in the log file header
 *
 * *****************  Version 65  *****************
 * User: Jim Mood     Date: 10/19/09   Time: 4:12p
 * Updated in $/software/control processor/code/application
 * Added power on and power up counts to the log header
 *
 * *****************  Version 64  *****************
 * User: Jim Mood     Date: 10/19/09   Time: 9:16a
 * Updated in $/software/control processor/code/application
 * SCR #304
 *
 * *****************  Version 63  *****************
 * User: Jim Mood     Date: 10/14/09   Time: 6:09p
 * Updated in $/software/control processor/code/application
 * SCR #285 function prototype had the incorrect type for the status
 * parameter.
 *
 * *****************  Version 62  *****************
 * User: Peter Lee    Date: 9/30/09    Time: 2:32p
 * Updated in $/software/control processor/code/application
 * SCR #283 Misc Code Review Updates
 *
 * *****************  Version 61  *****************
 * User: Peter Lee    Date: 9/29/09    Time: 3:52p
 * Updated in $/software/control processor/code/application
 * SCR #283 Misc Code Review Updates.
 *
 * *****************  Version 60  *****************
 * User: Jim Mood     Date: 9/29/09    Time: 11:58a
 * Updated in $/software/control processor/code/application
 * SCR# 178 Updated User Manager Tables
 *
 * *****************  Version 59  *****************
 * User: Jim Mood     Date: 9/16/09    Time: 3:22p
 * Updated in $/software/control processor/code/application
 *
 * *****************  Version 58  *****************
 * User: Jim Mood     Date: 7/10/09    Time: 11:40a
 * Updated in $/software/control processor/code/application
 * SCR #207  Fixes an issue where logs after the last flight were not
 * automatically  uploaded
 *
 * *****************  Version 57  *****************
 * User: Jim Mood     Date: 7/07/09    Time: 8:59a
 * Updated in $/software/control processor/code/application
 * Added user cmds to retrieve the index of a FVT entry by filename
 *
 * *****************  Version 56  *****************
 * User: Peter Lee    Date: 6/29/09    Time: 11:47a
 * Updated in $/software/control processor/code/application
 * SCR #204 Misc Code Review Updates
 *
 * *****************  Version 55  *****************
 * User: Jim Mood     Date: 5/26/09    Time: 1:24p
 * Updated in $/software/control processor/code/application
 *
 * *****************  Version 54  *****************
 * User: Jim Mood     Date: 5/15/09    Time: 5:15p
 * Updated in $/software/control processor/code/application
 * Added user cmds to get verification rows free and total number of rows
 *
 * *****************  Version 53  *****************
 * User: Jim Mood     Date: 4/22/09    Time: 4:21p
 * Updated in $/control processor/code/application
 * Updated comments only
 *
 * *****************  Version 52  *****************
 * User: Jim Mood     Date: 4/16/09    Time: 9:20a
 * Updated in $/control processor/code/application
 * Added logs
 *
 * *****************  Version 51  *****************
 * User: Jim Mood     Date: 4/02/09    Time: 8:57a
 * Updated in $/control processor/code/application
 * Increased Start Log timeout from 5s to 30s.
 *
 * *****************  Version 50  *****************
 * User: Jim Mood     Date: 3/31/09    Time: 4:53p
 * Updated in $/control processor/code/application
 * Added the VerId,  Aircraft.TailNumber,  Aircraft.Operator, and
 * Aircraft.Style configuration data to the log file header.
 *
 * *****************  Version 49  *****************
 * User: Peter Lee    Date: 1/23/09    Time: 10:09a
 * Updated in $/control processor/code/application
 * SCR #132 Support for new G4 file hdr fmt
 *
 * *****************  Version 48  *****************
 * User: Jim Mood     Date: 1/09/09    Time: 2:19p
 * Updated in $/control processor/code/application
 * Added timeout for Check CRC and Remove File commands sent from the File
 * Verification Table Maintenance task.
 *
 * *****************  Version 47  *****************
 * User: Jim Mood     Date: 1/06/09    Time: 4:55p
 * Updated in $/control processor/code/application
 * This version includes full round trip verification functionality.  This
 * impliments a 180 entry file verification table, and validates CRC
 * values from the micro-server.
 *
 * *****************  Version 46  *****************
 * User: Jim Mood     Date: 12/17/08   Time: 9:08a
 * Updated in $/control processor/code/application
 * This is the source used to test with Christos on 12/16/08, the intial
 * test of Validate CRC and Check File commands that worked
 *
 * *****************  Version 45  *****************
 * User: Jim Mood     Date: 12/12/08   Time: 6:53p
 * Updated in $/control processor/code/application
 * Buildable version with some round-trip functionality.  Checked in for
 * backup purposes
 *
 * *****************  Version 43  *****************
 * User: Peter Lee    Date: 10/08/08   Time: 10:16a
 * Updated in $/control processor/code/application
 * Remove unnecessary #includes
 *
 * *****************  Version 41  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 10:30a
 * Updated in $/control processor/code/application
 * SCR #87 Function Prototype
 *
 * *****************  Version 40  *****************
 * User: Jim Mood     Date: 6/20/08    Time: 5:08p
 * Updated in $/control processor/code/application
 *
 * *****************  Version 39  *****************
 * User: Jim Mood     Date: 6/10/08    Time: 2:31p
 * Updated in $/control processor/code/application
 * SCR-0040 - Updated to use new function prototype for LogFindNext
 *
 *
 *
 ***************************************************************************/
