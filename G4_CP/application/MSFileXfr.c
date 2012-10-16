#define MSFX_BODY
/******************************************************************************
            Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:        MSFileXfr.c
    
    Description: MSFX Micro Server File Transfer
                 msfx.get_file
                 FAST sends files to the GSE port from the Micro-Server by
                 YMODEM, not supporting batch transfer, streaming mode, or
                 128 byte packets.

                 msfx.put_file
                 FAST receives files from the GSE port to the Micro-Server 
                 by YMODEM, not supporting streaming mode, but supporting 
                 128 and 1024 byte packets and batch transfer for compatibility
                 with most YMODEM senders.

                 FAST specific functionality allows a list of transmitted and
                 non-transmitted log files to be retrieved with one command.
                 Also this module allows log files to be marked as 
                 "transmitted" with one command.                 

    VERSION
    $Revision: 28 $  $Date: 12-10-10 12:03p $   
    
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    
#include <stdlib.h>
/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "GSE.h"
#include "Monitor.h"
#include "MSFileXfr.h"
#include "UploadMgr.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define DATA_128               128
#define DATA_1024              1024
#define DATA_16384             16384
#define ERROR_LENGTH           80
#define MSI_REQ_DATA_TIMEOUT   5000
#define MSI_MARK_TIMEOUT       30000
#define MSI_SEND_TIMEOUT       10000
#define MSI_PAUSE_TIMEOUT      2000
#define BASE_8                 8
#define BASE_10                10

#define SOH    0x01
#define STX    0x02
#define EOT    0x04
#define ACK    0x06
#define NAK    0x15
#define CAN    0x18
#define CPMEOF 0x1A
#define YMDM_START_CHAR_TO (30U*TICKS_PER_Sec)
#define YMDM_ACK_CHAR_TO   (30U*TICKS_PER_Sec)
#define YMDM_MS_DATA_TO    (5U*TICKS_PER_Sec)
#define YMDM_TX_TO         (2U*TICKS_PER_Sec) //Allow a couple seconds in case
                                              //of PIT timer or other interrupt

#define YMDM_RCVR_CHAR_TO  (2U*TICKS_PER_Sec)
#define YMDM_RCVR_RETRY    15                 /*15 retries * 2s each is 30 seconds*/

//YMODEM packet size with overhead
#define YMODEM_OVERHEAD_SIZE   5         //Start+Block+InvBlock+CRC16
#define YMODEM_128_SIZE       (DATA_128+YMODEM_OVERHEAD_SIZE)
#define YMODEM_1024_SIZE      (DATA_1024+YMODEM_OVERHEAD_SIZE)
/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

#pragma pack(1)
typedef struct         {UINT8 Start;               /*Start char for packet   */
                        UINT8 Blk;                 /*Rolling block number    */
                        UINT8 InvBlk;              /*Inverse of block number */
                        INT8 Data[DATA_1024];      /*Data portion            */
                        UINT16 CRC;                /*CRC of Data portion only*/
                       }MSFX_YMODEM_1024_BLK;

typedef struct         {UINT8 Start;               /*Start char for packet   */
                        UINT8 Blk;                 /*Rolling block number    */
                        UINT8 InvBlk;              /*Inverse of block number */
                        INT8 Data[DATA_128];       /*Data portion            */
                        UINT16 CRC;                /*CRC of Data portion only*/
                       }MSFX_YMODEM_128_BLK;

//Overlay both blocks.  All that really changes is the CRC position.
typedef union          {MSFX_YMODEM_1024_BLK k1;  /*1k ymodem block*/
                        MSFX_YMODEM_128_BLK  B128;/*128B ymodem block*/
                       }MSFX_YMODEM_BLK;
#pragma pack()

typedef enum{
  TASK_STOPPED,
  TASK_STOP_SEND,
  TASK_ERROR,
  TASK_WAIT,
  TASK_REQUEST_NON_TXD_FILE_LIST,
  TASK_REQUEST_TXD_FILE_LIST,
  TASK_REQPRI4_NON_TXD_FILE_LIST,
  TASK_REQPRI4_TXD_FILE_LIST,
  TASK_REQUEST_START_FILE,
  TASK_MARK_FILE_TXD,
  TASK_YMDM_SNDR_WAIT_FOR_START,
  TASK_YMDM_SNDR_WAIT_FOR_ACKNAK,
  TASK_YMDM_SNDR_SEND,
  TASK_YMDM_SNDR_GET_NEXT,
  TASK_YMDM_SNDR_SEND_EOT,
  TASK_YMDM_SNDR_WAIT_EOT_ACKNAK,
  TASK_YMDM_RCVR_SEND_START_CHAR,
  TASK_YMDM_RCVR_SEND_ACK,
  TASK_YMDM_RCVR_SEND_NAK,
  TASK_YMDM_RCVR_WAIT_FOR_SNDR,
}MSFX_TASK_STATE;
/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/


static INT8             m_FileDataQBuf[DATA_16384]; /*Buffer for file data Q*/
static MSFX_TASK_STATE  m_TaskState;                /*MSFX task state (see enum)*/


/*control and data block for file transfer from the MS to the CP and CP to MS*/
static struct {
               INT8    FN[MSFX_MAX_FN_SIZE];/*File name to request from MS*/
               INT8    Err[ERROR_LENGTH];   /*Description of error (valid on error)*/
               UINT32  TO;          /*Working location for Timeout() function*/
               UINT32  TOCnt;       /*Timeout count for states that retry*/
               UINT32  Seq;         /*Seq. # for FILE DATA rsp to MS*/
               UINT8   ExpBlock;    /*Next expected FILE DATA block #*/
               FIFO    Q;           /*Q to hold data from FILE DATA*/
               UINT32  SizeRcvd;    /*Number of bytes received for YMODEM receive mode*/
               BOOLEAN yMdmB0;      /*Indicates YMODEM file header block being sent or rcvd*/
               BOOLEAN RspPending;  /*MS is waiting for FILE DATA rsp*/
               BOOLEAN FileDone;    /*MS indicates the current file is complete*/
               BOOLEAN TransferDone;/*MS indicates file transfer complete*/
              }m_FileXfrData;

/*Task "wait" state control block: includes next state for success or error*/
static struct {
                MSFX_TASK_STATE OnSuccess;
                MSFX_TASK_STATE OnError;
              }m_WaitData;

/*control and data block for file transfer from the CP to the GSE port*/
static MSFX_YMODEM_BLK  m_YMdm;

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void    MSFX_Task(void* pParam);
static BOOLEAN MSFX_RequestFileList(MSFX_TASK_STATE ListType);
static BOOLEAN MSFX_RequestGetFile(const INT8* FN);
static BOOLEAN MSFX_RequestPutFile(const INT8* Path);
static BOOLEAN MSFX_MarkFileAsTransmitted(const INT8* FN);
static BOOLEAN MSFX_MSCmdFileData(void* Data,UINT16 Size,UINT32 Sequence);
static void    MSFX_MSRspShellCmd(UINT16 Id, void* PacketData, UINT16 Size,
                                  MSI_RSP_STATUS Status);
static void    MSFX_MSRspStartFile(UINT16 Id, void* PacketData, UINT16 Size,
                                  MSI_RSP_STATUS Status);
static void    MSFX_MSRspFileXfrd(UINT16 Id, void* PacketData, UINT16 Size,
                                  MSI_RSP_STATUS Status);
static void    MSFX_MSRspGenericPassFail(UINT16 Id, void* PacketData, UINT16 Size,
                                  MSI_RSP_STATUS Status);
static void    MSFX_AddYMdm1kCRC(MSFX_YMODEM_BLK *MdmBlk);
static void    MSFX_SendFileDataResponse(INT8 RspType);
static void    MSFX_Error(INT8* Str);
static void    MSFX_PauseJMessenger(BOOLEAN Pause);
static void    MSFX_ReceiveDataPacket(MSFX_YMODEM_BLK* YMdm);
static void    MSFX_SendStartPutFileToMS(MSFX_YMODEM_BLK* YMdm);
static void    MSFX_SendPutFileDataToMS(MSFX_YMODEM_BLK* YMdm);
static void    MSFX_SendEndPutFileToMS(MSFX_TASK_STATE NextState);

/*Wait and Signal are used to manipulate the WaitData control block*/
static void    MSFX_Wait(MSFX_TASK_STATE Success,MSFX_TASK_STATE Fail, INT8* Str);
static void    MSFX_Signal(BOOLEAN Success);

static void    MSFX_RequestNonTxdFileListState(void);
static void    MSFX_RequestTxdFileListState(void);
static void    MSFX_ReqPri4TxdFileListState(void);
static void    MSFX_ReqPri4NonTxdFileListState(void);
static void    MSFX_RequestStartFileSndrState(void);
static void    MSFX_MarkFileTransmittedState(void);
static void    MSFX_YMdmSndrWaitForStart(void);
static void    MSFX_YMdmSndrWaitForACKNAK(void);
static void    MSFX_YMdmSndrSend(void);
static void    MSFX_YMdmSndrGetNext(void);
static void    MSFX_YMdmSndrSendEOT(void);
static void    MSFX_YMdmSndrWaitForEOT_ACKNAK(void);

static void    MSFX_YMdmRcvrSendStartChar(void);
static void    MSFX_YMdmRcvrSendACK(void);
static void    MSFX_YMdmRcvrSendNAK(void);
static void    MSFX_YMdmRcvrWaitForSndr(void);

#include "MSFileXfrUserTables.c"  // Include cmd tables & functions .c
/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:    MSFX_Init()
 *
 * Description: Setup the user manager commands, globals, and initiates a task
 *              responsible for handling micro-server communication and
 *              file transfer.
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:
 *
 *****************************************************************************/
void MSFX_Init(void)
{   
  TCB TaskInfo;

  m_TaskState = TASK_STOPPED;

  User_AddRootCmd(&RootUserTbl);

  /*Setup a data queue for file blocks received from the microserver*/
  FIFO_Init(&m_FileXfrData.Q,m_FileDataQBuf,sizeof(m_FileDataQBuf));

  /*Setup a command handler for file blocks received from the microserver*/
  ASSERT(SYS_OK == MSI_AddCmdHandler(CMD_ID_GET_FILE_DATA,MSFX_MSCmdFileData));

  /*Setup the task that manages micro-server communication and file transfer*/
  memset(&TaskInfo, 0, sizeof(TaskInfo));
  strncpy_safe(TaskInfo.Name,sizeof(TaskInfo.Name),"MS<->GSE File XFR",_TRUNCATE);
  TaskInfo.TaskID         = MS_GSE_File_XFR;
  TaskInfo.Function       = MSFX_Task;
  TaskInfo.Priority       = taskInfo[MS_GSE_File_XFR].priority;
  TaskInfo.modes          = taskInfo[MS_GSE_File_XFR].modes;
  TaskInfo.Type           = taskInfo[MS_GSE_File_XFR].taskType;
  TaskInfo.MIFrames       = taskInfo[MS_GSE_File_XFR].MIFframes;
  TaskInfo.Rmt.InitialMif = taskInfo[MS_GSE_File_XFR].InitialMif;
  TaskInfo.Rmt.MifRate    = taskInfo[MS_GSE_File_XFR].MIFrate;  // Run once every frame
  // (max throughput for YMODEM)
  TaskInfo.Enabled        = FALSE;
  TaskInfo.Locked         = FALSE;
  TaskInfo.pParamBlock    = NULL;
  TmTaskCreate (&TaskInfo);
  
}


/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:    MSFX_Task
 * Description: Task combines the micro-server communication and YMODEM file
 *              transfer management functions of the micro-server to GSE port
 *              log file transfer process.
 *              When the task is in the stopped state, another module can
 *              enable the task by setting m_TaskState to...
 *
 *              RequestStartFile:      Start transferring a file from the
 *                                     Micro-Server.
 *              RequestPutFile:        Start transferring a file to the
 *                                     Micro-Server.
 *              RequestTXDFileList:    to create and download a file containing
 *                                     a list of transmitted log files
 *              RequestNonTXDFileList: to create and download a file containing
 *                                     a list of log files that have not been
 *                                     transmitted
 *              ReqPri4TXDFileList:
 *              ReqPri4NonTXDFileList:
 *
 *              ..and then enabling the task.  
 *
 * Parameters:  [in/out] pParam: Task parameter, not used
 * Returns:     none
 * Notes:
 *
 *****************************************************************************/
static void MSFX_Task(void* pParam)
{
  switch(m_TaskState)
  {
    case TASK_STOPPED:
      //Do nothing, task should be disabled.
      break;
    
    case TASK_STOP_SEND:
      MSFX_SendFileDataResponse(CAN);
      MSFX_Wait(TASK_STOPPED,TASK_STOPPED,"MS returned fail for Messenger Ctl");
      MSFX_PauseJMessenger(FALSE);
      TmTaskEnable(MonitorGetTaskId(),TRUE);
      TmTaskEnable(MS_GSE_File_XFR, FALSE);
      break;

    case TASK_ERROR:
      MSFX_SendFileDataResponse(CAN);
      TmTaskEnable(MonitorGetTaskId(),TRUE);
      TmTaskEnable(MS_GSE_File_XFR, FALSE);
      break;

    case TASK_WAIT:
      //waiting for MS
      break;

    case TASK_REQUEST_NON_TXD_FILE_LIST:
      MSFX_RequestNonTxdFileListState();
      break;
    
    case TASK_REQUEST_TXD_FILE_LIST:
      MSFX_RequestTxdFileListState();
      break;
      
    case TASK_REQPRI4_NON_TXD_FILE_LIST:
      MSFX_ReqPri4NonTxdFileListState();
      break;
      
    case TASK_REQPRI4_TXD_FILE_LIST:
      MSFX_ReqPri4TxdFileListState();
      break;
      
    case TASK_REQUEST_START_FILE:
      MSFX_RequestStartFileSndrState();
      break;
    
    case TASK_MARK_FILE_TXD:
      MSFX_MarkFileTransmittedState();
      break;

    case TASK_YMDM_SNDR_WAIT_FOR_START:
      MSFX_YMdmSndrWaitForStart();
      break;
    
    case TASK_YMDM_SNDR_WAIT_FOR_ACKNAK:
      MSFX_YMdmSndrWaitForACKNAK();
      break;
    
    case TASK_YMDM_SNDR_SEND:
      MSFX_YMdmSndrSend();
      break;

    case TASK_YMDM_SNDR_GET_NEXT:
      MSFX_YMdmSndrGetNext();
      break;
    
    case TASK_YMDM_SNDR_SEND_EOT:
      MSFX_YMdmSndrSendEOT();
      break;

    case TASK_YMDM_SNDR_WAIT_EOT_ACKNAK:
      MSFX_YMdmSndrWaitForEOT_ACKNAK();
      break;

    case TASK_YMDM_RCVR_SEND_START_CHAR:
      MSFX_YMdmRcvrSendStartChar();
      break;
    
    case TASK_YMDM_RCVR_SEND_ACK:
      MSFX_YMdmRcvrSendACK();
      break;
    
    case TASK_YMDM_RCVR_SEND_NAK:
      MSFX_YMdmRcvrSendNAK();
      break;
    
    case TASK_YMDM_RCVR_WAIT_FOR_SNDR:
      MSFX_YMdmRcvrWaitForSndr();
      break;
    
    default:
      FATAL("MSFX TASK: BAD STATE %d",m_TaskState);
      break;
  }
}



/******************************************************************************
 * Function:    MSFX_RequestFileList()
 *
 * Description: Start the MSFX_Task to request a list of transmitted or
 *              non-transmitted log files, then transfer the list as a list of
 *              files.
 *              
 *
 * Parameters:  [in] ListType: TASK_REQUEST_TXD_FILE_LIST:
 *                             Request the list of transmitted log files
 *                             TASK_REQUEST_NON_TXD_FILE_LIST:
 *                             Request the list of non-transmitted log files
 *			       TASK_REQPRI4_NON_TXD_FILE_LIST:
 *		               Request the list of non-transmitted log files
 *                             in the priority 4 dir
 *                             TASK_REQPRI4_TXD_FILE_LIST:
 *                             Request the list of transmitted log files
 *                             in the priority 4 dir
 *
 * Returns:     TRUE: Task started successfully
 *              FALSE: Task did not start.
 *
 * Notes:       ASSERT check that the parameter is one of the two 
 *              aforementioned states.
 *
 *****************************************************************************/
static BOOLEAN MSFX_RequestFileList(MSFX_TASK_STATE ListType)
{
  BOOLEAN retval = FALSE;
 
  ASSERT( (ListType == TASK_REQUEST_TXD_FILE_LIST)    ||
          (ListType == TASK_REQUEST_NON_TXD_FILE_LIST)||
          (ListType == TASK_REQPRI4_NON_TXD_FILE_LIST)||
          (ListType == TASK_REQPRI4_TXD_FILE_LIST)      );
  
  if((m_TaskState == TASK_STOPPED ) || (m_TaskState == TASK_ERROR))
  {
    m_TaskState = ListType;
    TmTaskEnable(MonitorGetTaskId(),FALSE);    
    TmTaskEnable(MS_GSE_File_XFR, TRUE);

    //Wait for 'C' char for 30s after GSE requests to transfer a file
    Timeout(TO_START,YMDM_START_CHAR_TO,&m_FileXfrData.TO);
    
    retval = TRUE;
  }

  return retval;
}



/******************************************************************************
 * Function:    MSFX_RequestGetFile()
 *
 * Description: Start the MSFX_Task to transfer a file from the Micro-Server
 *              to the GSE port.  
 *
 * Parameters:  [in] FN: Full file name and path of the file to request from
 *                       the micro-server.
 *
 * Returns:     TRUE: Task started successfully.
 *              FALSE: Task did not start.
 *
 * Notes:
 *
 *****************************************************************************/
static BOOLEAN MSFX_RequestGetFile(const INT8* FN)
{
  BOOLEAN retval = FALSE;
 
  if((m_TaskState == TASK_STOPPED ) || (m_TaskState == TASK_ERROR))
  {
    m_FileXfrData.Seq        = 0; 
    m_FileXfrData.ExpBlock   = 0;
    m_FileXfrData.RspPending = 0; 
    strncpy_safe(m_FileXfrData.FN,sizeof(m_FileXfrData.FN), FN, _TRUNCATE);
    MSFX_Wait(TASK_REQUEST_START_FILE,TASK_REQUEST_START_FILE,
        "MS returned fail for Messenger Ctl");
    MSFX_PauseJMessenger(TRUE);

    TmTaskEnable(MonitorGetTaskId(),FALSE);    
    TmTaskEnable(MS_GSE_File_XFR, TRUE);

    //Wait for 'C' char for 30s after GSE requests to transfer a file
    Timeout(TO_START,YMDM_START_CHAR_TO,&m_FileXfrData.TO);
    retval = TRUE;
  }

  return retval;
}





/******************************************************************************
 * Function:    MSFX_RequestPutFile()
 *
 * Description: Start the MSFX_Task to transfer a file from the GSE port to
 *              the GSE port.  
 *
 * Parameters:  [in] Path: Full directory path to store the file to.
 *
 * Returns:     TRUE: Task started successfully.
 *              FALSE: Task did not start.
 *
 * Notes:
 *
 *****************************************************************************/
static BOOLEAN MSFX_RequestPutFile(const INT8* Path)
{
  BOOLEAN retval = FALSE;
 
  if((m_TaskState == TASK_STOPPED ) || (m_TaskState == TASK_ERROR))
  {
    m_FileXfrData.Seq        = 0; 
    m_FileXfrData.ExpBlock   = 0; 
    m_FileXfrData.TOCnt      = 0;
    m_FileXfrData.yMdmB0     = TRUE;
    strncpy_safe(m_FileXfrData.FN,sizeof(m_FileXfrData.FN), Path, _TRUNCATE);      
    TmTaskEnable(MonitorGetTaskId(),FALSE);    
    TmTaskEnable(MS_GSE_File_XFR, TRUE);
    m_TaskState = TASK_YMDM_RCVR_SEND_START_CHAR;
    retval = TRUE;
  }

  return retval;
}



/******************************************************************************
 * Function:    MSFX_MarkFileAsTransmitted()
 *
 * Description: Start the MSFX_Task to mark a file as transmitted.
 *              The following is performed in this order:
 *              1) Command the JMessenger to suspend file processing
 *              2) Command MSSIM to make a file as transmitted:
 *                 a) create a file in the crc_verify folder
 *                    <logname>.gse_offload
 *                 b) Move the file <logname> from CP_FILES to CP_SAVED
 *              3) Remove the FVT entry from Upload Manager. (see UploadMgr.c)
 *              4) Restart JMessenger processing
 *
 * Parameters:  [in] FN: Full file name and path of the file to request from
 *                       the micro-server.
 *
 * Returns:     TRUE: Task started successfully
 *              FALSE: Task did not start.
 *
 * Notes:
 *
 *****************************************************************************/
static BOOLEAN MSFX_MarkFileAsTransmitted(const INT8* FN)
{
  BOOLEAN retval = FALSE;
 
  if((m_TaskState == TASK_STOPPED ) || (m_TaskState == TASK_ERROR))
  {
    strncpy_safe(m_FileXfrData.FN, sizeof(m_FileXfrData.FN),FN,_TRUNCATE);
    MSFX_Wait(TASK_MARK_FILE_TXD, TASK_MARK_FILE_TXD,
        "MS returned fail for Messenger Ctl");
    MSFX_PauseJMessenger(TRUE);
    TmTaskEnable(MonitorGetTaskId(),FALSE);    
    TmTaskEnable(MS_GSE_File_XFR, TRUE);
    retval = TRUE;
  }

  return retval;
}


/******************************************************************************
 * Function:    MSFX_RequestNonTxdFileListState
 *
 * Description: Request the list of non-transmitted files from the micro-server
 *              Previous state: STOPPED
 *              Next state:     WAIT
 *                              ERROR
 *              
 * Parameters:  none
 *
 * Returns:     none 
 *
 * Notes:       
 *
 *****************************************************************************/
static void MSFX_RequestNonTxdFileListState(void)
{
  MSCP_CP_SHELL_CMD cmd;
 
 
  //Set file name and path
  sprintf(m_FileXfrData.FN,"/tmp/filelist.txt");

  //construct a command to create a formatted listing of the CP_FILES
  //folder and pipe it to /tmp
  sprintf(cmd.Str,"ls -lre [CP_FILES] > %s",
      m_FileXfrData.FN);
  
  cmd.Len = strlen(cmd.Str) + 1;

  MSFX_Wait(TASK_REQUEST_START_FILE,TASK_ERROR,
      "MS returned fail for get file list");
  if(SYS_OK != MSI_PutCommand(CMD_ID_SHELL_CMD,&cmd,
        sizeof(cmd)-sizeof(cmd.Str)+cmd.Len,MSI_REQ_DATA_TIMEOUT,MSFX_MSRspShellCmd))
  {
    MSFX_Error("Could not send MS command, file list");
    m_TaskState = TASK_ERROR;
  }
}



/******************************************************************************
 * Function:    MSFX_RequestTxdFileListState
 *
 * Description: Request the list of transmitted files from the micro-server
 *              Previous state: STOPPED
 *              Next state:     WAIT_FOR_REQUEST_FILE_LIST_RSP
 *                              ERROR
 *              
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:       
 *
 *****************************************************************************/
static void MSFX_RequestTxdFileListState(void)
{
  MSCP_CP_SHELL_CMD cmd;
 
 
  //Set file name and path
  sprintf(m_FileXfrData.FN,"/tmp/filelist.txt");

  //construct a command to create a formatted listing of the CP_SAVED
  //folder and pipe it to /tmp
  sprintf(cmd.Str,"ls -lre [CP_SAVED] > %s",m_FileXfrData.FN);

  cmd.Len = strlen(cmd.Str) + 1;


  MSFX_Wait(TASK_REQUEST_START_FILE,TASK_ERROR,
            "MS returned fail for get file list");
  
  if(SYS_OK != MSI_PutCommand(CMD_ID_SHELL_CMD,&cmd,
        sizeof(cmd)-sizeof(cmd.Str)+cmd.Len,MSI_REQ_DATA_TIMEOUT,MSFX_MSRspShellCmd))
  {
    MSFX_Error("Could not send MS command, file list");
    m_TaskState = TASK_ERROR;
  }
}



/******************************************************************************
 * Function:    MSFX_ReqPri4TxdFileListState
 *
 * Description: Request the list of transmitted priority 4 files from the
 *		micro-server
 *              Previous state: STOPPED
 *              Next state:     WAIT_FOR_REQUEST_FILE_LIST_RSP
 *                              ERROR
 *              
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:       
 *
 *****************************************************************************/
static void MSFX_ReqPri4TxdFileListState(void)
{
  MSCP_CP_SHELL_CMD cmd;
 
 
  //Set file name and path
  sprintf(m_FileXfrData.FN,"/tmp/filelist.txt");

  //construct a command to create a formatted listing of the CP_SAVED
  //folder and pipe it to /tmp
  sprintf(cmd.Str,"ls -lre [CP_PRI4] | grep [.]qc > %s",m_FileXfrData.FN);

  cmd.Len = strlen(cmd.Str) + 1;


  MSFX_Wait(TASK_REQUEST_START_FILE,TASK_ERROR,
            "MS returned fail for get file list");
  
  if(SYS_OK != MSI_PutCommand(CMD_ID_SHELL_CMD,&cmd,
        sizeof(cmd)-sizeof(cmd.Str)+cmd.Len,MSI_REQ_DATA_TIMEOUT,MSFX_MSRspShellCmd))
  {
    MSFX_Error("Could not send MS command, file list");
    m_TaskState = TASK_ERROR;
  }
}



/******************************************************************************
 * Function:    MSFX_ReqPri4NonTxdFileListState
 *
 * Description: Request the list of non-transmitted priority 4 files from the
 *		micro-server
 *              Previous state: STOPPED
 *              Next state:     WAIT
 *                              ERROR
 *              
 * Parameters:  none
 *
 * Returns:     none 
 *
 * Notes:       
 *
 *****************************************************************************/
static void MSFX_ReqPri4NonTxdFileListState(void)
{
  MSCP_CP_SHELL_CMD cmd;
 
 
  //Set file name and path
  sprintf(m_FileXfrData.FN,"/tmp/filelist.txt");

  //construct a command to create a formatted listing of the CP_FILES
  //folder and pipe it to /tmp
  sprintf(cmd.Str,"ls -lre [CP_PRI4] | grep -v [.]qc > %s",
      m_FileXfrData.FN);
  
  cmd.Len = strlen(cmd.Str) + 1;

  MSFX_Wait(TASK_REQUEST_START_FILE,TASK_ERROR,
      "MS returned fail for get file list");
  if(SYS_OK != MSI_PutCommand(CMD_ID_SHELL_CMD,&cmd,
        sizeof(cmd)-sizeof(cmd.Str)+cmd.Len,MSI_REQ_DATA_TIMEOUT,MSFX_MSRspShellCmd))
  {
    MSFX_Error("Could not send MS command, file list");
    m_TaskState = TASK_ERROR;
  }
}



/******************************************************************************
 * Function:    MSFX_RequestStartFileSndrState
 *
 * Description: Request a file from the micro-server to be send out the GSE
 *              port
 *              Previous state: STOPPED 
 *              Next state:     WAIT
 *                              ERROR
 *              
 *              
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:       
 *
 *****************************************************************************/
static void MSFX_RequestStartFileSndrState(void)
{
  MSCP_CP_GET_FILE_START_CMD cmd;

  //Initialize the control block for the FILE_DATA command/response
  m_FileXfrData.Seq          = 0;
  m_FileXfrData.ExpBlock     = 0;
  m_FileXfrData.RspPending   = 0;
  m_FileXfrData.FileDone     = FALSE;
  m_FileXfrData.TransferDone = FALSE;
  m_FileXfrData.yMdmB0       = TRUE;
  FIFO_Flush(&m_FileXfrData.Q);
  
  strncpy_safe(cmd.Str, sizeof(cmd.Str), m_FileXfrData.FN, _TRUNCATE);

  MSFX_Wait(TASK_YMDM_SNDR_WAIT_FOR_START,TASK_ERROR,NULL);
  
  if(  SYS_OK != MSI_PutCommand(CMD_ID_GET_FILE_START,&cmd,
        sizeof(cmd),MSI_REQ_DATA_TIMEOUT,MSFX_MSRspStartFile)        )
  {
    MSFX_Error("Could not send MS command, start file");
    m_TaskState = TASK_ERROR;
  }
}



/******************************************************************************
 * Function:    MSFX_MarkFileTransmittedState
 *
 * Description: Sends the command to the Micro-Server to mark a log file as
 *              transmitted out the GSE port.  
 *              
 * Parameters:  none
 *
 * Returns:     
 *
 * Notes:       
 *
 *****************************************************************************/
static void MSFX_MarkFileTransmittedState(void)
{
  MSCP_CP_FILE_XFRD_CMD cmd;

  strncpy_safe(cmd.FN, sizeof(cmd.FN),m_FileXfrData.FN,_TRUNCATE);

  MSFX_Wait(TASK_STOP_SEND,TASK_ERROR,NULL);
  if(  SYS_OK != MSI_PutCommand(CMD_ID_FILE_XFRD,&cmd,
        sizeof(cmd),MSI_MARK_TIMEOUT,MSFX_MSRspFileXfrd)        )
  {
    MSFX_Error("Could not send MS command, mark transfered");
    m_TaskState = TASK_ERROR;
  }
}



/******************************************************************************
 * Function:    MSFX_YMdmSndrWaitForStart
 * 
 * Description: Wait for the prompt for the GSE connected receiver that
 *              indicates it is ready to receive data.  Currently supports
 *              the 'C' character to indicate YMODEM with acknowledges.
 
 *              The routine also waits for the file name block from the Micro-
 *              Server before switching to send the first YMODEM block
 
 *              After receiving the start char from the PC and the file
 *              name block from the Micro-Server, the block is sent to
 *              the receiver.
 *
 *              Previous state: WAIT_FOR_START_FILE_RSP
 *              Next State:     YMODEM_SEND
 *                              TASK_YMDM_SNDR_WAIT_FOR_START
 *                              ERROR
 *              
 * Parameters:
 *
 * Returns:
 *
 * Notes:       
 *
 *****************************************************************************/
static void MSFX_YMdmSndrWaitForStart(void)
{
  INT16 start_char;
  INT8* err_str;
  
  start_char = GSE_getc();

  if(start_char == 'C')
  {
    //ExpBlock is zero until the file name block has been received, then
    //it increments to expect block 1.  Wait in this state until file
    //name block is received.
    m_TaskState = m_FileXfrData.ExpBlock == 0 ? 
                          TASK_YMDM_SNDR_WAIT_FOR_START : TASK_YMDM_SNDR_SEND;
  }
  else if(start_char == CAN)
  {
    m_TaskState = TASK_STOP_SEND;
    MSFX_SendFileDataResponse(CAN);
  }
  else if(start_char == -1)
  {
    if(Timeout(TO_CHECK,YMDM_START_CHAR_TO,&m_FileXfrData.TO))
    {
      err_str = m_FileXfrData.ExpBlock == 0 ? 
             "Timeout while waiting for data from Micro-Server" :
             "Timeout while waiting for 'C' from GSE";
             
      MSFX_Wait(TASK_ERROR,TASK_ERROR,NULL);
      MSFX_PauseJMessenger(FALSE);

      m_TaskState = TASK_ERROR;
      MSFX_Error(err_str);
    }
  }
  else
  {
    //RX framing error, wait for next character.
  }
}



/******************************************************************************
 * Function:    MSFX_YMdmSndrWaitForACKNAK
 *
 * Description: Wait for the receiver to respond after sending the file data
 *              block.
 *              If ACK, send the next block
 *              If ACK & File Done flag set & File data q empty, send EOT.
 *              If ACK & File name sent blank (last file) stop, all done
 *              
 *              If NAK, resend YMdm block
 *
 *              If CAN, inform microserver and stop.
 *
 *              Next state:     YMODEM_SEND
 *                              YMODEM_GET_NEXT
 *                              YMODEM_WAIT_FOR_ACKNAK
 *                              YMODEM_SEND_FINAL_BLOCK
 *                              TASK_STOP_SEND
 *                              ERROR
 *                       
 * Parameters:
 *
 * Returns:
 *
 * Notes:       
 *
 *****************************************************************************/
static void MSFX_YMdmSndrWaitForACKNAK(void)
{
  INT16 rsp_char;
 
  rsp_char = GSE_getc();

  if(ACK == rsp_char)
  {
    if(m_FileXfrData.FileDone &&(m_FileXfrData.Q.Cnt == 0)&&
        (!m_FileXfrData.yMdmB0))
    {
      m_TaskState = TASK_YMDM_SNDR_SEND_EOT;
    }
    /*Note: Block == 0 needs to evaluate first
      to ensure the y_Mdm.Data is the FN string*/
    else if((m_FileXfrData.yMdmB0) && (strlen(m_YMdm.k1.Data) ==  0))
    {
      m_TaskState = TASK_STOP_SEND;
    }
    else
    {
      m_TaskState = TASK_YMDM_SNDR_GET_NEXT;
      //Wait 5s for new data from Micro-Server
      Timeout(TO_START,YMDM_MS_DATA_TO,&m_FileXfrData.TO);
    }
  }
  else if(NAK == rsp_char)
  {
    m_TaskState = TASK_YMDM_SNDR_SEND;
  }
  else if(CAN == rsp_char)
  {
    m_TaskState = TASK_STOP_SEND;
    MSFX_SendFileDataResponse(CAN);
  }
  else
  {
    //Waiting for a good char
    //Wait for ACK/NAK char after sending data block
    if(Timeout(TO_CHECK,YMDM_ACK_CHAR_TO,&m_FileXfrData.TO))
    {
      MSFX_Wait(TASK_ERROR,TASK_ERROR,NULL);
      MSFX_PauseJMessenger(FALSE);
      m_TaskState = TASK_ERROR;
      MSFX_Error("Timeout while waiting for ACK/NAK from GSE");
      MSFX_SendFileDataResponse(CAN);
    }
  }
}



/******************************************************************************
 * Function:    MSFX_YMdmSndrSend
 *
 * Description: Send the YModem block currently in the buffer to the GSE port.
 *              Routine needs to wait until enough room is available in the
 *              GSE buffer for 'G' mode compatibility.  Otherwise, could poss.
 *              be rolled into the ACK/NAK and WAIT_FOR_START.
 *              Next State: YMODEM_WAIT_FOR_ACKNAK
 *                          YMODEM_SEND_BLOCK
 *              
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:       
 *
 *****************************************************************************/
static void MSFX_YMdmSndrSend(void)
{
  UINT32 to;
  if(GSE_write(&m_YMdm,sizeof(m_YMdm.k1)) )
  {
    m_TaskState = TASK_YMDM_SNDR_WAIT_FOR_ACKNAK;
    Timeout(TO_START,YMDM_TX_TO,&to);
    //While GSE tx is busy){} //While GSE tx is busy
    while( UART0_TRANSMITTING & UART_CheckForTXDone() & 
           !Timeout(TO_CHECK,YMDM_TX_TO,&to))
    {
    }
    //Wait for ACK/NAK char after sending data block
    Timeout(TO_START,YMDM_ACK_CHAR_TO,&m_FileXfrData.TO);
  }
  else
  {
    //For C mode s/b impossible to overrun GSE.  In g mode it is certian.
    //nd to test how much space is available in the GSE fifo before sending
    //data
    MSFX_Wait(TASK_ERROR,TASK_ERROR,NULL);
    MSFX_PauseJMessenger(FALSE);
    MSFX_Error("TX overrun while sending packet");
    m_TaskState = TASK_ERROR;
  }
}



/******************************************************************************
 * Function:    MSFX_YMdmSndrGetNext
 *
 * Description: Get the next block of data from the Micro-Server, through
 *              the file data Q.  If the q is less than 50% full, request more
 *              data from the MS.
 *              Do not allow a partial YMODEM block until the last data
 *              packet.
 *            
 *              Next state: YMODEM_WAIT_FOR_START (if Block 0)
 *                          YMODEM_SEND_FINAL_BLOCK
 *                          YMODEM_SEND
 *                          YMODEM_GET_NEXT
 *
 *              
 * Parameters:  
 *
 * Returns:     
 *
 * Notes:       
 *
 *****************************************************************************/
static void MSFX_YMdmSndrGetNext(void)
{
 
  //Request more data from the micro-server if the data Q is less than 50 % 
  //full and the micro-server is waiting to send more data (RspPending)
  if((FIFO_FreeBytes(&m_FileXfrData.Q) > m_FileXfrData.Q.Size/2)
      && (m_FileXfrData.RspPending == TRUE) && !m_FileXfrData.FileDone)
  {
    MSFX_SendFileDataResponse(ACK);
  }

  /*Load data from the Q into the YModem block IFF it can fill the entire data
    field.  Data block is fixed size and must be filled.  When the FileDone
    is set, the last block is padded out with ASCII EOF chars to make a full
    1024 size*/
  if((m_FileXfrData.Q.CmpltCnt >= sizeof(m_YMdm.k1.Data)) ||
      m_FileXfrData.FileDone)
  {
    memset(m_YMdm.k1.Data,CPMEOF,sizeof(m_YMdm.k1.Data));
    FIFO_PopBlock(&m_FileXfrData.Q,m_YMdm.k1.Data,
        MIN(sizeof(m_YMdm.k1.Data),m_FileXfrData.Q.Cnt));
    MSFX_AddYMdm1kCRC(&m_YMdm);
    if(m_FileXfrData.yMdmB0)
    {
      m_FileXfrData.yMdmB0 = FALSE;
      m_TaskState = TASK_YMDM_SNDR_WAIT_FOR_START;
    }
    else
    {
      m_TaskState = TASK_YMDM_SNDR_SEND;
    }

    m_YMdm.k1.Blk++;
    m_YMdm.k1.InvBlk  = ~m_YMdm.k1.Blk;
  }
  else
  {
    if(Timeout(TO_CHECK,YMDM_MS_DATA_TO,&m_FileXfrData.TO))
    {
      MSFX_Wait(TASK_ERROR,TASK_ERROR,NULL);
      MSFX_PauseJMessenger(FALSE);
      m_TaskState = TASK_ERROR;
      MSFX_Error("Timeout while waiting for data from Micro-Server");
      MSFX_SendFileDataResponse(CAN);      
    }
  }
}



/******************************************************************************
 * Function:    MSFX_YMdmSndrSendEOT
 *
 * Description: Send the terminating EOT
 *            
 * Parameters:  
 *
 * Returns:     
 *
 * Notes:       
 *
 *****************************************************************************/
static void MSFX_YMdmSndrSendEOT(void)
{
  INT8 eot = EOT;
  UINT32 to;
  if(GSE_write(&eot,sizeof(eot)) )
  {
    m_TaskState = TASK_YMDM_SNDR_WAIT_EOT_ACKNAK;
    Timeout(TO_START,YMDM_TX_TO,&to);
    //While GSE tx is busy){} //While GSE tx is busy
    while( UART0_TRANSMITTING & UART_CheckForTXDone() & 
           !Timeout(TO_CHECK,YMDM_TX_TO,&to))
    {
    }
    Timeout(TO_START, YMDM_ACK_CHAR_TO, &m_FileXfrData.TO);
  }
  else
  {
    //For C mode s/b impossible to overrun GSE.  In g mode it is certain.
    //nd to test how much space is available in the GSE fifo before sending
    //data
    MSFX_Wait(TASK_ERROR,TASK_ERROR,NULL);
    MSFX_PauseJMessenger(FALSE);
    MSFX_Error("TX overrun while sending EOT");
    m_TaskState = TASK_ERROR;
  }
}



/******************************************************************************
 * Function:    MSFX_YMdmSndrWaitForEOT_ACKNAK
 *
 * Description: Wait for the ACK after the EOT.  This is valid in g mode, so
 *              is is separate from the normal ACK/NAK.
 *              EOT is resent on NAK, else, final data block sent.
 *            
 * Parameters:  
 *
 * Returns:     
 *
 * Notes:       
 *
 *****************************************************************************/
static void MSFX_YMdmSndrWaitForEOT_ACKNAK(void)
{
  INT16 rsp_char;

  rsp_char = GSE_getc();

  if(ACK == rsp_char)
  {
    //After one file is transfered, send block #00 with nulls to signal the
    //end of transfer.  If batch mode was supported, the next file would
    //start here, but batch is not currently supported.
    memset(&m_YMdm,0,sizeof(m_YMdm));
    m_YMdm.k1.Start  = STX;
    m_YMdm.k1.Blk    = 0;
    m_YMdm.k1.InvBlk = UINT8MAX;
    MSFX_AddYMdm1kCRC(&m_YMdm);
    m_FileXfrData.yMdmB0 = TRUE;
    m_TaskState = TASK_YMDM_SNDR_SEND;
  }
  else if(NAK == rsp_char)
  {
    m_TaskState = TASK_YMDM_SNDR_SEND_EOT;
  }
  else if(CAN == rsp_char)
  {
    m_TaskState = TASK_STOP_SEND;
    MSFX_SendFileDataResponse(CAN);
  }
  else
  {
    if(Timeout(TO_CHECK, YMDM_ACK_CHAR_TO, &m_FileXfrData.TO))
    {
      MSFX_Wait(TASK_ERROR,TASK_ERROR,NULL);
      MSFX_PauseJMessenger(FALSE);
      MSFX_Error("Timeout while waiting for ACK/NAK from GSE");
      m_TaskState = TASK_ERROR;
    }
  }
}



/******************************************************************************
 * Function:    MSFX_YMdmRcvrSendStartChar
 *
 * Description: YMODEM receiver mode state.
 *              Send the start character to the sender.  This tells the sender
 *              to transmit the first block of the file containing the
 *              file name, size, and modification date.
 *              Next State:
 *                YMDM_RCVR_WAIT_FOR_SNDR
 *              
 *            
 * Parameters:  
 *
 * Returns:     
 *
 * Notes:       
 *
 *****************************************************************************/
static void MSFX_YMdmRcvrSendStartChar(void)
{
  INT8 buf = 'C';
  UINT32 to;
  
  if(GSE_write(&buf,1))
  {    
    m_FileXfrData.SizeRcvd = 0;
    m_TaskState = TASK_YMDM_RCVR_WAIT_FOR_SNDR;
    Timeout(TO_START,YMDM_TX_TO,&to);
    //While GSE tx is busy){} //While GSE tx is busy
    while( UART0_TRANSMITTING & UART_CheckForTXDone() & 
           !Timeout(TO_CHECK,YMDM_TX_TO,&to))
    {
    }

    Timeout(TO_START,YMDM_RCVR_CHAR_TO,&m_FileXfrData.TO);
  }
  else
  {
    MSFX_Error("TX error while sending start char");
    m_TaskState = TASK_ERROR;
  }
}



/******************************************************************************
 * Function:    MSFX_YMdmRcvrSendACK
 *
 * Description: YMODEM receiver mode state.
 *              Send the ACK character to the sender.  This tells the sender
 *              the last packet sent was processed successfully.
 *              Next State:
 *                YMDM_RCVR_WAIT_FOR_SNDR
 *                YMDM_RCVR_SEND_START_CHAR (after block 0 only)
 *            
 * Parameters:  
 *
 * Returns:     
 *
 * Notes:       
 *
 *****************************************************************************/
static void MSFX_YMdmRcvrSendACK(void)
{
  INT8 buf = ACK;
  UINT32 to;
  if(GSE_write(&buf,1))
  {
    Timeout(TO_START,YMDM_TX_TO,&to);
    //While GSE tx is busy){} //While GSE tx is busy
    while( UART0_TRANSMITTING & UART_CheckForTXDone() & 
           !Timeout(TO_CHECK,YMDM_TX_TO,&to))
    {
    }

    Timeout(TO_START,YMDM_RCVR_CHAR_TO,&m_FileXfrData.TO);

    m_FileXfrData.SizeRcvd = 0;

    if(m_FileXfrData.yMdmB0)
    {
      //Exp block 1 only after acking the file name block 0.  This stops
      //the B0 flag from being cleared after ACKING the EOT from the sender
      if(m_FileXfrData.ExpBlock == 1)
      {
        m_FileXfrData.yMdmB0 = FALSE;
      }
      m_TaskState = TASK_YMDM_RCVR_SEND_START_CHAR;
    }
    else
    {
      m_TaskState = TASK_YMDM_RCVR_WAIT_FOR_SNDR;
    }
  }
  else
  {
    MSFX_Error("TX error while sending ACK");
    m_TaskState = TASK_ERROR;
  }  
}



/******************************************************************************
 * Function:    MSFX_YMdmRcvrSendNAK
 *
 * Description: YMODEM receiver mode state.
 *              Send the ACK character to the sender.  This tells the sender
 *              the last packet sent was processed successfully.
 *              Next State:
 *                YMDM_RCVR_WAIT_FOR_SNDR
 *            
 * Parameters:  
 *
 * Returns:     
 *
 * Notes:       
 *
 *****************************************************************************/
static void MSFX_YMdmRcvrSendNAK(void)
{
  INT8 buf = NAK;
  UINT32 to;
  
  if(GSE_write(&buf,1))
  {
    Timeout(TO_START,YMDM_TX_TO,&to);
    //While GSE tx is busy){} //While GSE tx is busy
    while( UART0_TRANSMITTING & UART_CheckForTXDone() & 
           !Timeout(TO_CHECK,YMDM_TX_TO,&to))
    {
    }

    Timeout(TO_START,YMDM_RCVR_CHAR_TO,&m_FileXfrData.TO);
    m_FileXfrData.SizeRcvd = 0;
    m_TaskState = TASK_YMDM_RCVR_WAIT_FOR_SNDR;
  }
  else
  {
    MSFX_Error("TX error while sending NAK");
    m_TaskState = TASK_ERROR;
  }    
}



/******************************************************************************
 * Function:    MSFX_YMdmRcvrWaitForSndr
 *
 * Description: YMODEM receiver state
 *              Listen for a packet from the sender on the GSE port.
 *              The state then exits to one of the several other states
 *              depending on what is received from the sender
 *              Next State:
 *                WAIT  (after sending data to the MS)
 *                NAK   (after packet not received successfully
 *                ERROR (after bad block number of timeout count exceeded)
 *                SEND_START_CHAR (after timeout waiting for block 0)
 *                STOPPED (after getting a NUL block 0)
 *            
 * Parameters:  
 *
 * Returns:     
 *
 * Notes:       
 *
 *****************************************************************************/
static void MSFX_YMdmRcvrWaitForSndr(void)
{
  UINT8 buf;

  if(Timeout(TO_CHECK,YMDM_RCVR_CHAR_TO,&m_FileXfrData.TO))
  {
    if(m_FileXfrData.TOCnt++ < YMDM_RCVR_RETRY)
    {
      //If waiting for block 0, resend the 'start' character, else send
      //a NAK for timeout.
      m_TaskState = m_FileXfrData.yMdmB0 ? TASK_YMDM_RCVR_SEND_START_CHAR :
                  TASK_YMDM_RCVR_SEND_NAK;
    }
    else
    {
      MSFX_Error("Timeout count exceeded waiting for sender");
      m_TaskState = TASK_ERROR;
    }
  }
  //If data received is zero, the task is waiting for the control character
  else if(m_FileXfrData.SizeRcvd == 0)
  {
    if(1 == GSE_read(&buf,1))
    {
      m_FileXfrData.SizeRcvd = 1;
      if(buf == SOH)
      {
        m_YMdm.k1.Start = buf;
        m_FileXfrData.TOCnt = 0;
      }
      else if(buf == STX)
      {
        m_YMdm.k1.Start = buf;
        m_FileXfrData.TOCnt = 0;
      }
      else if(buf == EOT)
      {
        MSFX_SendEndPutFileToMS(TASK_YMDM_RCVR_SEND_ACK);
        m_FileXfrData.yMdmB0 = TRUE;
        m_FileXfrData.ExpBlock = 0;
      }
      //CAN is not described in the C Forsberg document, but Hyperterminal 
      //seems to use it to cancel a transfer in progress.
      else if(buf == CAN)
      {
        MSFX_SendEndPutFileToMS(TASK_STOPPED);
        TmTaskEnable(MonitorGetTaskId(),TRUE);
        TmTaskEnable(MS_GSE_File_XFR, FALSE);        
      }
      else
      {
        //TODO: Lets try ignoring it m_TaskState = TASK_YMDM_RCVR_SEND_NAK;
      }
    }
  }
  else //Receiving data packet    
  {
    MSFX_ReceiveDataPacket(&m_YMdm);
  }
}



/******************************************************************************
 * Function:    MSFX_ReceiveDataPacket
 *
 * Description: Receive a data packet from the serial port.  The Start
 *              character has already been received and indicates if this
 *              is a 128 or 1024 size YMODEM packet.  The function is polled
 *              repeatedly until the entire packet is received, then it is
 *              checked for validity (block number and CRC.)  Good blocks are
 *              transmitted to the Micro-Server.
 *
 *              The next file transfer state is returned.
 *            
 * Parameters:  [in/out]: YMdm pointer to a YMODEM block to receive the data
 *
 * Returns:
 *
 * Notes:       
 *
 *****************************************************************************/
static void MSFX_ReceiveDataPacket(MSFX_YMODEM_BLK* YMdm)
{
  INT8* ptr   = (INT8*)YMdm;
  //Packet size depends on start character: SOH 128B, STX 1024B
  UINT32 size = YMdm->k1.Start == SOH ? YMODEM_128_SIZE : YMODEM_1024_SIZE;
  INT32 read;
  UINT16 RcvdCRC;
  UINT16 CalcCRC;
  
  read = GSE_read(&ptr[m_FileXfrData.SizeRcvd],size-m_FileXfrData.SizeRcvd);

  if(read >= 0)
  {
    m_FileXfrData.SizeRcvd += read;
      
    //Check if entire packet was received
    if(m_FileXfrData.SizeRcvd == size)
    {
      //Get received and computed CRC.  CRC location differs based on YMODEM
      //packet size.
      CalcCRC = CRC_CCITT(YMdm->k1.Data,size-YMODEM_OVERHEAD_SIZE);
      RcvdCRC = YMdm->k1.Start == SOH ? YMdm->B128.CRC : YMdm->k1.CRC;
      
      if( m_FileXfrData.yMdmB0                               &&
         (YMdm->k1.Blk            == m_FileXfrData.ExpBlock) &&
         ((UINT8)~YMdm->k1.InvBlk == m_FileXfrData.ExpBlock) &&
         (CalcCRC                 == RcvdCRC ) )
      {
        //Got a complete first block, open the file on the micro-server
        MSFX_SendStartPutFileToMS(YMdm);
        m_FileXfrData.ExpBlock += 1;
      }
      else if((YMdm->k1.Blk            == m_FileXfrData.ExpBlock) &&
              ((UINT8)~YMdm->k1.InvBlk == m_FileXfrData.ExpBlock) &&
              (CalcCRC                 == RcvdCRC))
      {
        //Got a complete block
        MSFX_SendPutFileDataToMS(YMdm);
        m_FileXfrData.ExpBlock += 1; 
      }
      else //Block has an error, return NAK;
      {
        // May want to just allow to ensure we don't hammer the sender if it is still sending
        m_TaskState = TASK_YMDM_RCVR_SEND_NAK;
      }
    }
  }
  else //GSE error, return NAK 
  {
    // May want to just allow to ensure we don't hammer the sender if it is still sending
    m_TaskState = TASK_YMDM_RCVR_SEND_NAK;
  }


}



/******************************************************************************
 * Function:    MSFX_SendStartPutFileToMS
 *
 * Description: Sends the start (open) file command to initiate file transfer
 *              to the Micro-Server.  The data needed by the MS command is
 *              extracted from the "Block 0" YMODEM data block.  The
 *              file name and path is mandatory, followed by mandatory file
 *              size and optional file modification date.  Any other YMODEM
 *              defined fields are ignored.
 *         
 * Parameters:  [in] YMdm: YMODEM block containing the "Block 0" file data
 *              
 * Returns:     none.  Global file task state is modified depending on the
 *              outcome of sending the MS command
 *
 * Notes:
 *
 *****************************************************************************/
static void MSFX_SendStartPutFileToMS(MSFX_YMODEM_BLK* YMdm)
{
  MSCP_CP_PUT_FILE_START_CMD  cmd;
  INT8                        *ptr;
  INT8 buf = ACK;
  UINT32                      to;
  
  /*Pull apart the file information contained in the Data portion of the
    YMODEM block.  Get the file name, file size and timestamp.  Format is:

    [Filename string][NULL byte][Filesize string (decimal)]
    [SPACE][Time string (Unix Time in Octal)]
  */
  //If the file name is blank.  If string length of the data section longer
  //than the size of the section something is wrong, stop the transfer.
  if( (strlen(YMdm->k1.Data) != 0) &&
      (strlen(YMdm->k1.Data) <= sizeof(YMdm->k1.Data) ) )
  {
    //Extract file name and append to the directory path that has been set
    //from the user command "put_file."  Add a trailing '/' if necessary to
    //the path name before appending the file name.  If Strcat returns fail,
    //just ignore it.
    strncpy_safe(cmd.FN,sizeof(cmd.FN),m_FileXfrData.FN,_TRUNCATE);
    if('/' != cmd.FN[strlen(cmd.FN)])
    {
      SuperStrcat(cmd.FN,"/",sizeof(cmd.FN));
    }
    SuperStrcat(cmd.FN,YMdm->k1.Data,sizeof(cmd.FN));

    //Convert size and timestamp to binary values.
    cmd.Size     = strtoul(&YMdm->k1.Data[strlen(YMdm->k1.Data)+1],&ptr,BASE_10);
    cmd.UnixTime = strtoul(ptr+1,&ptr,BASE_8);
    
    MSFX_Wait(TASK_YMDM_RCVR_SEND_ACK,TASK_ERROR,
          "MS returned fail: start put file");
    if(SYS_OK != MSI_PutCommand( CMD_ID_PUT_FILE_START,&cmd,sizeof(cmd),MSI_SEND_TIMEOUT,
                               MSFX_MSRspGenericPassFail))
    {
      m_TaskState = TASK_ERROR;
      MSFX_Error("Could not send MS command: start put file");
    }
  }
  //Filename string is blank, no more files to start.
  else
  {
    if(GSE_write(&buf,1))
    {
      Timeout(TO_START,YMDM_TX_TO,&to);
      //While GSE tx is busy){} //While GSE tx is busy
      while( UART0_TRANSMITTING & UART_CheckForTXDone() & 
             !Timeout(TO_CHECK,YMDM_TX_TO,&to))
      {
      }
      m_TaskState = TASK_STOPPED;
      TmTaskEnable(MonitorGetTaskId(),TRUE);
      TmTaskEnable(MS_GSE_File_XFR, FALSE);
    }
    else
    {
      MSFX_Error("GSE overrun while sending final ACK");
    }
  }
}



/******************************************************************************
 * Function:    MSFX_SendPutFileDataToMS
 *
 * Description: Sends the file data command as part of the file transfer to
 *              the Micro-Server.  The file data is extracted from the
 *              YMODEM data block.  
 *              
 * Parameters:  [in] YMdm: YMODEM block containing file data.
 *              
 * Returns:     none. Global file task state is modified depending on the
 *              outcome of sending the MS command
 * Notes:       
 *
 *****************************************************************************/
static void MSFX_SendPutFileDataToMS(MSFX_YMODEM_BLK* YMdm)
{
  MSCP_CP_PUT_FILE_DATA_CMD  cmd;
  
  cmd.Block = YMdm->k1.Blk;
  cmd.Len   = (YMdm->k1.Start == SOH ? YMODEM_128_SIZE : YMODEM_1024_SIZE) - 
              YMODEM_OVERHEAD_SIZE;

  memcpy(cmd.Data,YMdm->k1.Data,cmd.Len);

  MSFX_Wait(TASK_YMDM_RCVR_SEND_ACK,TASK_ERROR,
      "MS command failed: put file data");
  if(SYS_OK != MSI_PutCommand( CMD_ID_PUT_FILE_DATA,&cmd,
                               sizeof(cmd)-sizeof(cmd.Data)+cmd.Len,MSI_SEND_TIMEOUT,
                               MSFX_MSRspGenericPassFail))
  {
    m_TaskState = TASK_ERROR;
    MSFX_Error("Could not send MS command: put file data");
  }

}



/******************************************************************************
 * Function:    MSFX_SendEndPutFileToMS
 *
 * Description: Sends the file data command with zero data to indicated
 *              the end of a file transfer.  This signals the MS to close
 *              the file currently being transfered.
 *              
 * Parameters:  [in] NextState: State to enter after MS response is received.
 *              
 * Returns:     none. Global files task state is modified depending on the
 *              outcome of sending the MS command.
 *
 * Notes:       
 *
 *****************************************************************************/
static void MSFX_SendEndPutFileToMS(MSFX_TASK_STATE NextState)
{
  MSCP_CP_PUT_FILE_DATA_CMD  cmd;
  
  cmd.Len   = 0;
  MSFX_Wait(NextState,TASK_ERROR,
      "MS command failed: put file data");
  if(SYS_OK != MSI_PutCommand( CMD_ID_PUT_FILE_DATA,&cmd,
                               sizeof(cmd)-sizeof(cmd.Data)+cmd.Len,MSI_SEND_TIMEOUT,
                               MSFX_MSRspGenericPassFail))
  {
    m_TaskState = TASK_ERROR;
    MSFX_Error("Could not send MS command: put file data");
  }
}



/******************************************************************************
 * Function:    MSFX_PauseJMessenger
 *
 * Description: Send the command to MSSIM that will pause JMessenger (an
 *              application by PWEH) while this task is tranferring
 *              log files or marking log files as trasmitted
 *
 *              
 * Parameters:  [in] Pause: TRUE: Pause JMessenger process
 *                          FALSE: Run JMessenger process
 *              
 * Returns:     
 *
 * Notes:       Task state set to error on failure to send message
 *
 *****************************************************************************/
static void MSFX_PauseJMessenger(BOOLEAN Pause)
{
  MSCP_CP_MSNGR_CTL_CMD cmd;
 
  cmd.Pause = Pause;
  
  if(  SYS_OK != MSI_PutCommand(CMD_ID_MSNGR_CTL,&cmd,
        sizeof(cmd),MSI_PAUSE_TIMEOUT,MSFX_MSRspGenericPassFail)        )
  {
    MSFX_Error("Could not send MS command, Pause JMessenger");
    m_TaskState = TASK_ERROR;
  }
}



/******************************************************************************
 * Function:    MSFX_Wait
 *
 * Description: Used when the file transfer task needs to wait for response
 *              from the micro-server.  The response or command that the task
 *              is waiting for calls the MSFX_Signal() function to go to the
 *              next state when the response is received
 *              
 * Parameters:  [in] Success: State to enter when a successful response is
 *                            received
 *              [in] Fail:    State to enter when a fail response is received.
 *              [in] Str:     Error string for when the Fail state is ERROR
 *                            This allows the caller to preset the error string
 *                            so it won't need to be set when calling
 *                            MSFX_Signal(FALSE);
 * Returns:     none
 *
 * Notes:       
 *
 *****************************************************************************/
static void MSFX_Wait(MSFX_TASK_STATE Success,MSFX_TASK_STATE Fail,INT8 *Str)
{  
  m_WaitData.OnSuccess = Success;
  m_WaitData.OnError   = Fail;
  if(Str != NULL)
  {
    sprintf(m_FileXfrData.Err,"Err: %s",Str);
  }
  m_TaskState = TASK_WAIT;
}


/******************************************************************************
 * Function:    MSFX_Signal
 *
 * Description: Used when 
 *              
 * Parameters:  [in] Success: TRUE:  Operation successful
 *                            FALSE: Operation failed
 *
 * Returns:     none
 *
 * Notes:       
 *
 *****************************************************************************/
static void MSFX_Signal(BOOLEAN Success)
{
  if(Success)
  {
    m_TaskState = m_WaitData.OnSuccess;
  }
  else
  {
    m_TaskState = m_WaitData.OnError;
  }
    
}



/******************************************************************************
 * Function:    MSFX_AddYMdm1kCRC
 *
 * Description: Compute the CRC for the Data portion of a 1k YMODEM block and
 *              store the result in the CRC portion of the block.  
 *              
 * Parameters:  [in/out]: MdmBlock .Data portion read, result written to .CRC
 *
 * Returns:     none
 *
 * Notes:       
 *
 *****************************************************************************/
static void MSFX_AddYMdm1kCRC(MSFX_YMODEM_BLK *MdmBlk)
{
  
  MdmBlk->k1.CRC = CRC_CCITT(MdmBlk->k1.Data,sizeof(MdmBlk->k1.Data));
}



/******************************************************************************
 * Function:    MSFX_Error
 *
 * Description: Saves a descriptive string describing the condition that caused 
 *              the error.  The errors cannot be fed out real-time by GSE_DebugStr
 *              because it will likely be OFF while doing binary transfers.
 *              
 * Parameters:  [in]: Str: Description of the error
 *                         must be less than 74 chars.
 *
 * Returns:     none
 *
 * Notes:       
 *
 *****************************************************************************/
static void MSFX_Error(INT8* Str)
{
  sprintf(m_FileXfrData.Err,"Err: %s",Str);
}


/******************************************************************************
 * Function:    MSFX_SendFileDataResponse
 *
 * Description: If a response to the File Data command from the Micro-Server
 *              is pending, send the response.  Errors handled silently.
 *
 * Parameters:  [in] RspType: ACK Acknowledge receipt of previous data block.
 *                            CAN Cancel data transfer.
 *
 * Returns:     none
 *
 * Notes:       
 *
 *****************************************************************************/
static void MSFX_SendFileDataResponse(INT8 RspType)
{
  MSCP_CP_GET_FILE_DATA_RSP rsp;
  
  if(RspType == CAN)
  {
    rsp.Status = MSCP_FILE_DATA_CAN;
  }
  else
  {
    rsp.Status = MSCP_FILE_DATA_OK;
  }
  
  if(m_FileXfrData.RspPending == TRUE)
  {
    if(SYS_OK == MSI_PutResponse(CMD_ID_GET_FILE_DATA,&rsp,MSCP_RSP_STATUS_SUCCESS,
        sizeof(rsp),m_FileXfrData.Seq))
    {
      m_FileXfrData.RspPending = FALSE;
    }
  }
}



/******************************************************************************
 * Function:    MSFX_MSRspShellCmd | MS Response Handler
 *
 * Description: Response handler for ls shell commands sent from MSFX task to 
 *              the Micro-Server.  The routine notifies the task of the
 *              response by changing the task state from WAIT_FOR_FILE_LIST to
 *              REQUEST_START_FILE if list creation was successful or ERROR if
 *              a timeout or error occurred.
 *              
 * Parameters:  Id:         Message Id of the response (MSCP_CMD_ID)
 *              PacketData: Pointer to the data returned from the microserver.
 *              Size:       Length (in bytes) of the data returned from the
 *                          microserver
 *              Status:     Response status from the micro server
 *
 * Returns:     none
 *              
 * Notes:       
 *
 *****************************************************************************/
static void MSFX_MSRspShellCmd(UINT16 Id, void* PacketData, UINT16 Size,
                               MSI_RSP_STATUS Status)
{
  if(Status == MSI_RSP_SUCCESS)
  {
    if(TASK_WAIT == m_TaskState)
    {
      //ls command complete.  If an error occurred a blank file will be
      //transfered, or a file not found error will occur
      MSFX_Signal(TRUE);
    }
  }
  else
  {
    MSFX_Signal(FALSE);
  }
}



/******************************************************************************
 * Function:    MSFX_MSRspStartFile | MS Response Handler
 *
 * Description: Response handler for the start file command sent to the
 *              Micro-Server.
 *              
 * Parameters:  see MSInterface.h for prototype
 *
 * Returns:     none
 *              
 * Notes:       
 *
 *****************************************************************************/
static void MSFX_MSRspStartFile(UINT16 Id, void* PacketData, UINT16 Size,
                               MSI_RSP_STATUS Status)
{ 
  MSCP_MS_GET_FILE_START_RSP *rsp = PacketData;
 
  if(Status == MSI_RSP_SUCCESS)
  {
    if(rsp->Status == MSCP_FILE_START_OK)
    {
      MSFX_Signal(TRUE);
    }
    else //File Not Found
    {
      MSFX_Error("MS responded file not found");
      MSFX_Signal(FALSE);
    }      
  }
  else
  {
    MSFX_Error("MS returned fail for start file");
    MSFX_Signal(FALSE);
  }
}



/******************************************************************************
 * Function:    MSFX_MSRspFileXfrd | MS Response Handler
 *
 * Description: Response handler for the Mark File as Transfered command.
 *              
 *              
 * Parameters:  Id:         Message Id of the response (MSCP_CMD_ID)
 *              PacketData: Pointer to the data returned from the microserver.
 *              Size:       Length (in bytes) of the data returned from the
 *                          microserver
 *              Status:     Response status from the micro server
 *
 * Returns:     none
 *              
 * Notes:       
 *
 *****************************************************************************/
static void MSFX_MSRspFileXfrd(UINT16 Id, void* PacketData, UINT16 Size,
                               MSI_RSP_STATUS Status)
{ 
  MSCP_MS_FILE_XFRD_RSP *rsp = PacketData;
  INT8 *ptr;
  if(Status == MSI_RSP_SUCCESS)
  {
    if((rsp->Status == MSCP_FILE_XFRD_OK) ||
       (rsp->Status == MSCP_FILE_XFRD_CRC_FILE_EXISTS) ||
       (rsp->Status == MSCP_FILE_XFRD_LOG_ALREADY_MOVED) )
    {
      //Strip Micro-Server added file extensions
      ptr = strstr(m_FileXfrData.FN,".bz2.bfe");
      if(ptr != NULL)
      {
        *ptr = '\0';
      }
      
      if(UploadMgr_DeleteFVTEntry(m_FileXfrData.FN))
      {
        MSFX_Signal(TRUE);
      }
      else
      {
        MSFX_Error("MS Mark Log cmd, remove FVT entry failed");
        MSFX_Signal(FALSE);
      }
    }
    //Priority 4 logs, there is no FVT entry to delete.
    else if((rsp->Status == MSCP_PRI4_FILE_XFRD_OK) ||
            (rsp->Status == MSCP_PRI4_FILE_XFRD_LOG_ALREADY_MOVED) )
    {
      MSFX_Signal(TRUE);
    }
    else if(rsp->Status == MSCP_FILE_XFRD_FNF)//File Not Found
    {
      MSFX_Error("MS Mark Log cmd, log not found");
      MSFX_Signal(FALSE);
    }
    else
    {
      MSFX_Error("MS returned fail for Mark Log cmd");
      MSFX_Signal(FALSE);
    }
  }
  else
  {
    MSFX_Error("MS returned fail for Mark Log cmd");
    MSFX_Signal(FALSE);
  }
}



/******************************************************************************
 * Function:    MSFX_MSCmdFileData | MS Command Handler
 *
 * Description: Receives data blocks from the Micro-Server while transferring
 *              files.  The response to the data block is normally sent from
 *              The MSFX_Task when the data Q has room for the next block.
 *              This allows the Task to throttle the data stream from the
 *              microserver, matching the throughput of the GSE port.
 *              If an unexpected error occurs, or the MSFX task is stopped,
 *              a "cancel" response is sent immediately.
 *
 *              The first block send contains file information needed by
 *              YMODEM, including the file name, size, and date.
 *
 *              A data block length of zero indicates the current file transfer
 *              is completed.
 *
 *              A file information block with a blank FN field indicates the
 *              entire transfer is complete.
 *              
 * Parameters:  PacketData: Pointer to the data returned from the microserver.
 *              Size:       Length (in bytes) of the data returned from the
 *                          microserver
 *              Sequence:   Command sequence number assigned by the microserver.
 *                          This value must be returned in the response, it is
 *                          the command handlers duty to copy the sequence number
 *                          to the response.
 *
 * Returns:     TRUE: Command accepted (always)
 *              FALSE: Command failed (not used)
 * Notes:       
 *
 *****************************************************************************/
static BOOLEAN MSFX_MSCmdFileData(void* Data,UINT16 Size,UINT32 Sequence)
{
  MSCP_MS_GET_FILE_DATA_CMD* cmd = Data;


  m_FileXfrData.Seq = Sequence;

  /*Block number is zero: Start a new file*/
  if(cmd->BlockNum == 0)
  {    /*Must be expecting to start a new file*/
    if(TASK_YMDM_SNDR_WAIT_FOR_START == m_TaskState &&
        strlen(cmd->Data.FileInfo.FN) < sizeof(cmd->Data.FileInfo.FN))
    {
       /* Build ymodem block 00 with file data from MS*/
      memset(&m_YMdm,0,sizeof(m_YMdm));
      m_YMdm.k1.Start  = STX;
      m_YMdm.k1.Blk    = 0;
      m_YMdm.k1.InvBlk = UINT8MAX;
      sprintf(m_YMdm.k1.Data,"%s %d %o",cmd->Data.FileInfo.FN,
          cmd->Data.FileInfo.Size,
          cmd->Data.FileInfo.UnixTime);

      //Replace space after FN with a NULL (per YMODEM format)
      m_YMdm.k1.Data[strlen(cmd->Data.FileInfo.FN)] = '\0';
      MSFX_AddYMdm1kCRC(&m_YMdm);
      m_FileXfrData.RspPending     = TRUE;
      m_FileXfrData.ExpBlock       = 1;
      m_FileXfrData.yMdmB0         = TRUE;
    }
  }
  else /*Else, push file data into data q*/
  {  
    if( (m_TaskState != TASK_STOPPED)                 &&
        (m_TaskState != TASK_ERROR)                   &&
        (FIFO_FreeBytes(&m_FileXfrData.Q) > cmd->Len) &&
        (cmd->Len <= sizeof(cmd->Data))               &&
        (m_FileXfrData.ExpBlock == cmd->BlockNum)           )
    {
      FIFO_PushBlock(&m_FileXfrData.Q,&cmd->Data,cmd->Len);
      if(cmd->Len == 0) /*Len == 0 means file complete, signal file done*/
      {
        m_FileXfrData.FileDone = TRUE;
      }
      m_FileXfrData.ExpBlock =
                m_FileXfrData.ExpBlock == UINT8MAX ? 1 : m_FileXfrData.ExpBlock + 1;
      m_FileXfrData.RspPending = TRUE;
    }
  }
  return TRUE;
}



/******************************************************************************
 * Function:    MSFX_MSRspGenericPassFail | MS Command Handler
 *
 * Description: 
 *              
 * Parameters:  Id:         Message Id of the response (MSCP_CMD_ID)
 *              PacketData: Pointer to the data returned from the microserver.
 *              Size:       Length (in bytes) of the data returned from the
 *                          microserver
 *              Status:     Response status from the micro server
 *
 * Returns:
 *
 * Notes:       
 *
 *****************************************************************************/
static void  MSFX_MSRspGenericPassFail(UINT16 Id, void* PacketData, UINT16 Size,
                               MSI_RSP_STATUS Status)
{
  BOOLEAN success;

  success = FALSE;
  if(Status == MSI_RSP_SUCCESS)
  {
    success = TRUE; 
  }

  MSFX_Signal(success);
}

    
/******************************************************************************
 *  MODIFICATIONS
 *    $History: MSFileXfr.c $
 * 
 * *****************  Version 28  *****************
 * User: Melanie Jutras Date: 12-10-10   Time: 12:03p
 * Updated in $/software/control processor/code/application
 * SCR 1172
 * 
 * *****************  Version 27  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 12:43p
 * Updated in $/software/control processor/code/application
 * SCR# 1142
 * 
 * *****************  Version 26  *****************
 * User: Jim Mood     Date: 7/19/12    Time: 10:41a
 * Updated in $/software/control processor/code/application
 * SCR 1107: Data Offload changes for 2.0.0
 * 
 * *****************  Version 25  *****************
 * User: Jim Mood     Date: 11/15/10   Time: 10:34a
 * Updated in $/software/control processor/code/application
 * SCR 998 YMODEM not clearing FIFO after canceled get_file
 * 
 * *****************  Version 24  *****************
 * User: Jim Mood     Date: 10/15/10   Time: 2:05p
 * Updated in $/software/control processor/code/application
 * SCR 935 again.  Wait() before JMessenger
 * 
 * *****************  Version 23  *****************
 * User: Jim Mood     Date: 10/14/10   Time: 6:45p
 * Updated in $/software/control processor/code/application
 * SCR 924 (Added while loop timeouts)
 * SCR 935 GetFile race condition
 * 
 * *****************  Version 22  *****************
 * User: John Omalley Date: 10/08/10   Time: 10:46a
 * Updated in $/software/control processor/code/application
 * SCR 924 - Code Review Updates
 * 
 * *****************  Version 21  *****************
 * User: Jim Mood     Date: 10/01/10   Time: 3:14p
 * Updated in $/software/control processor/code/application
 * SCR 909 Code Coverage Issues
 * 
 * *****************  Version 20  *****************
 * User: Jim Mood     Date: 9/30/10    Time: 6:45p
 * Updated in $/software/control processor/code/application
 * SCR 909 Code Coverage Changes
 * 
 * *****************  Version 19  *****************
 * User: Jim Mood     Date: 7/30/10    Time: 5:08p
 * Updated in $/software/control processor/code/application
 * SCR 760 YMODEM half duplex timing
 * 
 * *****************  Version 18  *****************
 * User: Jim Mood     Date: 7/19/10    Time: 8:40p
 * Updated in $/software/control processor/code/application
 * SCR 693 YMODEM receiver timeout should have been 30s
 * SCR 696 Unpause JMessenger on error exit points for YMODEM Sender
 * 
 * *****************  Version 17  *****************
 * User: Peter Lee    Date: 6/15/10    Time: 5:25p
 * Updated in $/software/control processor/code/application
 * SCR #599 Removed '10 sec" comment. 
 * 
 * *****************  Version 16  *****************
 * User: Jim Mood     Date: 6/11/10    Time: 10:06a
 * Updated in $/software/control processor/code/application
 * SCR 623 Batch mode.  Implements YMODEM  receiver
 * 
 * *****************  Version 15  *****************
 * User: Jim Mood     Date: 5/18/10    Time: 6:50p
 * Updated in $/software/control processor/code/application
 * SCR 594 YMODEM/Premption of FIFO issue
 * SCR 599 Extend YMODEM sender ACK timeout
 * 
 * *****************  Version 14  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:53p
 * Updated in $/software/control processor/code/application
 * SCR #587 Change TmTaskCreate to return void
 * 
 * *****************  Version 13  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:07p
 * Updated in $/software/control processor/code/application
 * SCR #317 Implement safe strncpy
 * 
 * *****************  Version 12  *****************
 * User: Jeff Vahue   Date: 4/05/10    Time: 9:57a
 * Updated in $/software/control processor/code/application
 * SCR# 452 - Init some uninitialized locals, rearrange code to eliminate
 * 'else' paths.
 * 
 * *****************  Version 11  *****************
 * User: Contractor V&v Date: 3/29/10    Time: 6:15p
 * Updated in $/software/control processor/code/application
 * SCR #317 Implement safe strncpy
 * 
 * *****************  Version 10  *****************
 * User: Jim Mood     Date: 3/26/10    Time: 5:38p
 * Updated in $/software/control processor/code/application
 * Basic working YMODEM receiver
 * 
 * *****************  Version 9  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:37p
 * Updated in $/software/control processor/code/application
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 * 
 * *****************  Version 8  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/application
 * SCR# 483 - Function Names
 * 
 * *****************  Version 7  *****************
 * User: Contractor2  Date: 3/02/10    Time: 3:40p
 * Updated in $/software/control processor/code/application
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 6  *****************
 * User: Jim Mood     Date: 2/24/10    Time: 10:49a
 * Updated in $/software/control processor/code/application
 * 
 * *****************  Version 5  *****************
 * User: Jeff Vahue   Date: 1/23/10    Time: 3:05p
 * Updated in $/software/control processor/code/application
 * SCR# 397
 * 
 * *****************  Version 4  *****************
 * User: Jim Mood     Date: 1/20/10    Time: 3:45p
 * Updated in $/software/control processor/code/application
 * Initial working build and completion of outstanding TODO:s
 * 
 * *****************  Version 3  *****************
 * User: Contractor V&v Date: 12/10/09   Time: 5:39p
 * Updated in $/software/control processor/code/application
 * SCR 106 
 * 
 * *****************  Version 2  *****************
 * User: Jim Mood     Date: 12/08/09   Time: 5:15p
 * Updated in $/software/control processor/code/application
 * 
 * *****************  Version 1  *****************
 * User: Jim Mood     Date: 11/17/09   Time: 6:26p
 * Created in $/software/control processor/code/application
 * YModem transfer works, still need to fill in todo's
 * 
 *
 *****************************************************************************/
 
