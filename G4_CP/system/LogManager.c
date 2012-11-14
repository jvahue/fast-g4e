#define LOGMNG_BODY
/******************************************************************************
            Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:        LogManager.c

    Description: The log manager is responsible for reading, writing,
                 and erasing logs that are stored in the data flash.

                 TODO: It is not valid to assume that if a blank header
                       is found that the rest of the memory is erased.
                       Should implement an algorithm that verifies the
                       next sector is completely erased before determining
                       the end has been reached.

   VERSION
      $Revision: 106 $  $Date: 12-11-13 5:46p $
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "logmanager.h"
#include "memmanager.h"
#include "gse.h"
#include "WatchDog.h"
#include "NvMgr.h"

#include "TestPoints.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/

#define LOGMGR_PERCENTUSAGE_MAX           85.0f
#define LOG_ERASE_IN_PROGRESS_STARTUP_ID  200000
#define LOG_FIND_NEXT_WRITE_STARTUP_ID    100000

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static UINT32               LogBuffer[LOG_LOCAL_BUFFER_SIZE];
static LOG_MNG_TASK_PARMS   LogMngBlock;
static LOG_STATE_TASK_PARMS LogMarkBlock;
static LOG_CMD_ERASE_PARMS  LogCmdEraseBlock;
static LOG_REQ_STATUS       LogStatusTemp;

static LOG_EEPROM_CONFIG      LogEraseData;
static LOG_CBIT_HEALTH_COUNTS LogHealthCounts;

static BOOLEAN                LogSystemInit;

static LOG_MON_ERASE_PARMS    LogMonEraseBlock;
static LOG_REQ_QUEUE          LogQueue;
static LOG_CONFIG             LogConfig;
static LOG_ERROR_STATUS       LogError;

static LOG_READ_FAIL_LOG      LogReadFailTemp;
static LOG_ERASE_FAIL_LOG     LogEraseFailTemp;

static LOG_PAUSE_WRITE        LogManagerPause;
static LOG_REQUEST            LogEraseRequest;

// this table is sized four bigger than the queue so it can't
// be filled with pending writes
static SYSTEM_LOG systemTable[SYSTEM_TABLE_MAX_SIZE];

// Log Erase Cleanup
DO_CLEANUP DoCleanup;

/*Busy/Not Busy event data*/
static INT32 m_event_tag;
static void (*m_event_func)(INT32 tag,BOOLEAN is_busy) = NULL;

static BOOLEAN is_busy;

#include "LogUserTables.c"

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void LogManageTask          ( void *pParam );
static void LogMngStart            ( LOG_MNG_TASK_PARMS *pTCB );
static void LogMngPending          ( LOG_MNG_TASK_PARMS *pTCB );
static void LogMngStatus           ( LOG_MNG_TASK_PARMS *pTCB );
static void LogMarkTask            ( void *pParam );
static void LogCmdEraseTask        ( void *pParam );
static void LogMonEraseTask        ( void *pParam );
static void LogCheckEraseInProgress( void );
static void LogSaveEraseData       ( UINT32 nOffset,
                                     UINT32 nSize,
                                     BOOLEAN bChipErase,
                                     BOOLEAN bInProgress,
                                     BOOLEAN bWriteNow );
static void LogSystemLogManageTask ( void *pParam );
/*****************************************************************************/
static void           LogCheckResult         ( RESULT Result, CHAR *FuncStr, SYS_APP_ID LogID,
                                               FLT_STATUS FaultStatus, BOOLEAN *pFailFlag,
                                               UINT32 *pCnt, void *LogData,
                                               INT32 LogDataSize );
static BOOLEAN        LogFindNextWriteOffset ( UINT32 *pOffset,
                                               RESULT *Result );
static LOG_HDR_STATUS LogCheckHeader         ( LOG_HEADER Hdr );
static BOOLEAN        LogHdrFilterMatch      ( LOG_HEADER Hdr,
                                               LOG_STATE State,
                                               LOG_TYPE Type,
                                               LOG_SOURCE Source,
                                               LOG_PRIORITY Priority );
static BOOLEAN        LogVerifyAllDeleted    ( UINT32 *pRdOffset,
                                               UINT32 nEndOffset,
                                               UINT32 *pFoundOffset );
static UINT32         LogManageWrite         ( SYS_APP_ID LogID, LOG_PRIORITY Priority,
                                               void *pData, UINT16 nSize,
                                               const TIMESTAMP *pTs, LOG_TYPE LogType );
static void           LogEraseStatusWrite    ( LOG_CMD_ERASE_PARMS *pTCB );
/*****************************************************************************/
static void             LogQueueInit  ( void );
static LOG_QUEUE_STATUS LogQueueGet   ( LOG_REQUEST *Entry );
static void             LogUpdateWritePendingStatuses( void );
/*****************************************************************************/

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:    LogInitialize
 *
 * Description: Initialize the log manager system. Retrieve log manager
 *              configuration data from the EEPROM device. Check the battery
 *              backed RAM and EEPROM for previous incomplete erase. Initialize
 *              the Log Manager Task and setup the Write/Erase Queue.
 *              Initializes the log usage value and number of logs stored in
 *              memory.
 *
 * Parameters:  None
 *
 * Returns:     RESULT
 *
 * Notes:       None.
 *
 *****************************************************************************/
void LogInitialize (void)
{
   // Local Data
   RESULT            sysStatus;
   TCB               tcbTaskInfo;
   UINT32            nOffset;
   BOOLEAN           bValidFound;
   UINT32            i;
   RESULT            resultNV;

   // Initialize Local Data
   sysStatus = SYS_OK;
   nOffset   = 0;
   User_AddRootCmd(&RootLogMsg);

   SystemLogInitLimitCheck();

   // This assert ensures that LOG_INDEX_NOT_SET is a safe value to represent
   // index-not-set.
   ASSERT( SYSTEM_TABLE_MAX_SIZE != LOG_INDEX_NOT_SET );

   // Open the log erase status file
   if( SYS_OK == NV_Open(NV_LOG_ERASE) )
   {
     NV_Read(NV_LOG_ERASE, 0, &LogEraseData, sizeof(LogEraseData));
   }
   else
   {
     // Reset the erase status into to default.
     LogEraseData.inProgress       = FALSE;
     LogEraseData.bChipErase       = FALSE;
     LogEraseData.savedStartOffset = 0;
     LogEraseData.savedSize        = 0;
     resultNV = NV_WriteNow(NV_LOG_ERASE, 0, &LogEraseData, sizeof(LogEraseData));
     GSE_DebugStr(NORMAL,TRUE, "Log Initialize - Reset NV_LOG_ERASE...%s",
                  SYS_OK == resultNV ? "SUCCESS":"FAILED");
   }

   memset(&LogConfig, 0, sizeof(LogConfig));
   LogConfig.bEightyFiveExceeded = FALSE;

   // Open the log error counts file
   if( SYS_OK == NV_Open(NV_LOG_COUNTS) )
   {
      // Open Successfull now read the counts
      NV_Read(NV_LOG_COUNTS, 0, &LogError.counts, sizeof(LogError.counts));
      // Are there any counts from the previous run?
      if ( (LogError.counts.nDiscarded != 0) || (LogError.counts.nErAccess != 0) ||
           (LogError.counts.nRdAccess  != 0) )
      {
         // Record the counts in a system log
         Flt_SetStatus(STA_NORMAL, SYS_LOG_MEM_ERRORS,
                       &LogError.counts, sizeof(LogError.counts));
      }
   }
   // Now Reset the counts back to "0"
   memset(&LogError,  0, sizeof(LogError ));
   if ( SYS_OK != NV_WriteNow(NV_LOG_COUNTS, 0, &LogError.counts, sizeof(LogError.counts)))
   {
      GSE_DebugStr(NORMAL,TRUE, "Log Initialize - Reset NV_LOG_COUNTS...FAILED");
   }
   
   // Set the Fail flags to FALSE
   LogError.bReadAccessFail   = FALSE;
   LogError.bWriteAccessFail  = FALSE;
   LogError.bEraseAccessFail  = FALSE;
   // Clear the CBIT health counts
   memset(&LogHealthCounts, 0, sizeof(LOG_CBIT_HEALTH_COUNTS));
   // Initialize the Log Write Pause storage object
   LogManagerPause.enable       = FALSE;
   LogManagerPause.delay_S      = 0;
   LogManagerPause.startTime_ms = 0;
   // Initialize the erase request
   LogEraseRequest.pStatus = NULL;
   LogEraseRequest.reqType = LOG_REQ_ERASE;

   // Did Memory Manager Initialize correctly?
   if (SYS_OK == MemMgrStatus())
   {
      // Log Check Erase if previous erase was in Progress when power went away
      LogCheckEraseInProgress();

      // Find the log pointers
      // First Log, Last Log, Next Write
      LogConfig.nEndOffset   = MemGetBlockSize(MEM_BLOCK_LOG);

      // Check the Data Flash and find the next offset to write
      bValidFound = STPU( LogFindNextWriteOffset (&nOffset, &sysStatus), eTpLog1871);

      // If a valid address was found then save it and calculate how much was used
      if (TRUE == bValidFound)
      {
         // Store the write offset and percentage used
         LogConfig.nWrOffset     = nOffset;
         LogConfig.fPercentUsage = ((FLOAT32)LogConfig.nWrOffset/
                                    (FLOAT32)LogConfig.nEndOffset)*100.0f;

         // Check if we have exceeded the 85% usage.
         if (LogConfig.fPercentUsage > LOGMGR_PERCENTUSAGE_MAX)
         {
            LogConfig.bEightyFiveExceeded = TRUE;
            GSE_DebugStr(NORMAL,TRUE, "LogInitialize: Log Memory > 85 percent Full");
            // Store the fault log data
            Flt_SetStatus(STA_CAUTION, SYS_LOG_85_PERCENT_FULL, NULL, 0);
         }
      }
      // No Logs were found but the SysStatus is OK...
      else if (SYS_OK == sysStatus)
      {
         // The memory is corrupted. Immediately initiate a chip erase and
         // wait, providing status to the user.
         // Display Debug message in case anybody is watching......
         GSE_DebugStr(NORMAL,TRUE,
                  "LogInitialize: Data Flash Corrupted, Erasing memory!!");
         // Command a Chip Erase... The system cannot be used until the Data Flash is
         // functioning. That is why this blocking and will not return until the
         // erase has completed. If the Chip Erase did not block then the Log
         // Manager Queues would overflow......

         // Write the log erase status and stay here until the log erase
         // status has been persisted before starting the (blocking) chip erase.
         // ( nOffset, nSize, bChipErase, bInProgress, bWriteNow )
         LogSaveEraseData ( 0, 0, TRUE, TRUE, TRUE);

         sysStatus = MemChipErase();

         // Do a sanity check on the erase after it has completed.
         if (SYS_OK != sysStatus)
         {
            // The chip erase has not completed successfully and
            // the FAST box cannot operate without data flash - FATAL
            // The box will reboot and keep attempting to correct the
            // situation
            FATAL( "LogInitialize: Chip Erase Fail FATAL! ResultCode: 0x%08X", sysStatus );
         }
         // Erase is now complete reset the flags
         // ( nOffset, nSize, bChipErase, bInProgress, bWriteNow )
         LogSaveEraseData (0, 0, FALSE, FALSE, TRUE);
         // Log the Erase
         Flt_SetStatus( STA_NORMAL, SYS_LOG_MEM_CORRUPTED_ERASED, NULL, 0);
      }
      else // No Logs found because the System Status returned is bad
      {
         // the FAST box cannot operate without data flash - FATAL
         // The box will reboot and keep attempting to correct the
         // situation
         FATAL( "LogInitialize: FindNextWriteOffset Fail. ResultCode: 0x%08X", sysStatus );
      }
   }
   else // Memory did not initialize and is faulted
   {
      // the FAST box cannot operate without data flash - FATAL
      // The box will reboot and keep attempting to correct the
      // situation
      FATAL ( "LogInitialize: Memory Faulted. ResultCode: 0x%08X", MemMgrStatus() );
   }

   // Log Manager Request Dispatcher
   memset(&tcbTaskInfo, 0, sizeof(tcbTaskInfo));
   strncpy_safe(tcbTaskInfo.Name,sizeof(tcbTaskInfo.Name),"Log Manage Task",_TRUNCATE);
   tcbTaskInfo.TaskID                  = (TASK_INDEX)Log_Manage_Task;
   tcbTaskInfo.Function                = LogManageTask;
   tcbTaskInfo.Priority                = taskInfo[Log_Manage_Task].priority;
   tcbTaskInfo.Type                    = taskInfo[Log_Manage_Task].taskType;
   tcbTaskInfo.modes                   = taskInfo[Log_Manage_Task].modes;
   tcbTaskInfo.MIFrames                = taskInfo[Log_Manage_Task].MIFframes;
   tcbTaskInfo.Rmt.InitialMif          = taskInfo[Log_Manage_Task].InitialMif;
   tcbTaskInfo.Rmt.MifRate             = taskInfo[Log_Manage_Task].MIFrate;
   tcbTaskInfo.Enabled                 = TRUE;
   tcbTaskInfo.Locked                  = FALSE;
   tcbTaskInfo.pParamBlock             = &LogMngBlock;

   LogMngBlock.state                = LOG_MNG_IDLE;
   LogMngBlock.faultCount           = 0;

   // Init all registered 'pending-write' entries to clear/unused.
   for ( i = 0; i < (UINT32)LOG_REGISTER_END; ++i)
   {
     LogMngBlock.logWritePending[i] = LOG_INDEX_NOT_SET;
   }

   LogMngBlock.currentEntry.reqType = LOG_REQ_NONE;
   LogMngBlock.currentEntry.pStatus = &LogStatusTemp;
   memset (&LogMngBlock.currentEntry.request, 0,
           sizeof(LogMngBlock.currentEntry.request));
   TmTaskCreate (&tcbTaskInfo);

   // Log Manager Mark Block Task
   memset(&tcbTaskInfo, 0, sizeof(tcbTaskInfo));
   strncpy_safe(tcbTaskInfo.Name,sizeof(tcbTaskInfo.Name),"Log Mark State Task",_TRUNCATE);
   tcbTaskInfo.TaskID                  = (TASK_INDEX)Log_Mark_State_Task;
   tcbTaskInfo.Function                = LogMarkTask;
   tcbTaskInfo.Priority                = taskInfo[Log_Mark_State_Task].priority;
   tcbTaskInfo.Type                    = taskInfo[Log_Mark_State_Task].taskType;
   tcbTaskInfo.modes                   = taskInfo[Log_Mark_State_Task].modes;
   tcbTaskInfo.MIFrames                = taskInfo[Log_Mark_State_Task].MIFframes;
   tcbTaskInfo.Rmt.InitialMif          = taskInfo[Log_Mark_State_Task].InitialMif;
   tcbTaskInfo.Rmt.MifRate             = taskInfo[Log_Mark_State_Task].MIFrate;
   tcbTaskInfo.Enabled                 = FALSE;
   tcbTaskInfo.Locked                  = FALSE;
   tcbTaskInfo.pParamBlock             = &LogMarkBlock;

   TmTaskCreate (&tcbTaskInfo);

   // Log Manager Cmd Erase Task
   memset(&tcbTaskInfo, 0, sizeof(tcbTaskInfo));
   strncpy_safe(tcbTaskInfo.Name,sizeof(tcbTaskInfo.Name),"Log Command Erase Task",_TRUNCATE);
   tcbTaskInfo.TaskID                  = (TASK_INDEX)Log_Command_Erase_Task;
   tcbTaskInfo.Function                = LogCmdEraseTask;
   tcbTaskInfo.Priority                = taskInfo[Log_Command_Erase_Task].priority;
   tcbTaskInfo.Type                    = taskInfo[Log_Command_Erase_Task].taskType;
   tcbTaskInfo.modes                   = taskInfo[Log_Command_Erase_Task].modes;
   tcbTaskInfo.MIFrames                = taskInfo[Log_Command_Erase_Task].MIFframes;
   tcbTaskInfo.Rmt.InitialMif          = taskInfo[Log_Command_Erase_Task].InitialMif;
   tcbTaskInfo.Rmt.MifRate             = taskInfo[Log_Command_Erase_Task].MIFrate;
   tcbTaskInfo.Enabled                 = FALSE;
   tcbTaskInfo.Locked                  = FALSE;
   tcbTaskInfo.pParamBlock             = &LogCmdEraseBlock;

   LogCmdEraseBlock.nOffset         = 0;
   LogCmdEraseBlock.type            = LOG_ERASE_NONE;
   LogCmdEraseBlock.state           = LOG_ERASE_VERIFY_NONE;
   LogCmdEraseBlock.bVerifyStarted  = FALSE;
   LogCmdEraseBlock.bUpdatingStatusFile = FALSE;
   TmTaskCreate (&tcbTaskInfo);

   // Log Manager Monitor Erase Task
   memset(&tcbTaskInfo, 0, sizeof(tcbTaskInfo));
   strncpy_safe(tcbTaskInfo.Name, sizeof(tcbTaskInfo.Name),// Log_Monitor_Erase_Task
                "Log Monitor Erase Task",_TRUNCATE);
   tcbTaskInfo.TaskID                  = (TASK_INDEX)Log_Monitor_Erase_Task;
   tcbTaskInfo.Function                = LogMonEraseTask;
   tcbTaskInfo.Priority                = taskInfo[Log_Monitor_Erase_Task].priority;
   tcbTaskInfo.Type                    = taskInfo[Log_Monitor_Erase_Task].taskType;
   tcbTaskInfo.modes                   = taskInfo[Log_Monitor_Erase_Task].modes;
   tcbTaskInfo.MIFrames                = taskInfo[Log_Monitor_Erase_Task].MIFframes;
   tcbTaskInfo.Rmt.InitialMif          = taskInfo[Log_Monitor_Erase_Task].InitialMif;
   tcbTaskInfo.Rmt.MifRate             = taskInfo[Log_Monitor_Erase_Task].MIFrate;
   tcbTaskInfo.Enabled                 = FALSE;
   tcbTaskInfo.Locked                  = FALSE;
   tcbTaskInfo.pParamBlock             = &LogMonEraseBlock;

   TmTaskCreate (&tcbTaskInfo);

   // System Log Manage Task (initially enabled to write out any PBIT logs
   // generated during startup)
   memset(&tcbTaskInfo, 0, sizeof(tcbTaskInfo));
   strncpy_safe(tcbTaskInfo.Name, sizeof(tcbTaskInfo.Name),
                "System Log Manage Task",_TRUNCATE);
   tcbTaskInfo.TaskID                  = (TASK_INDEX)System_Log_Manage_Task;
   tcbTaskInfo.Function                = LogSystemLogManageTask;
   tcbTaskInfo.Priority                = taskInfo[System_Log_Manage_Task].priority;
   tcbTaskInfo.Type                    = taskInfo[System_Log_Manage_Task].taskType;
   tcbTaskInfo.modes                   = taskInfo[System_Log_Manage_Task].modes;
   tcbTaskInfo.MIFrames                = taskInfo[System_Log_Manage_Task].MIFframes;
   tcbTaskInfo.Rmt.InitialMif          = taskInfo[System_Log_Manage_Task].InitialMif;
   tcbTaskInfo.Rmt.MifRate             = taskInfo[System_Log_Manage_Task].MIFrate;
   tcbTaskInfo.Enabled                 = TRUE;
   tcbTaskInfo.Locked                  = FALSE;
   tcbTaskInfo.pParamBlock             = NULL;

   TmTaskCreate (&tcbTaskInfo);
   LogSystemInit = TRUE;
   is_busy       = FALSE;

} // LogInitialize


/*****************************************************************************
 * Function:    LogQueuePut
 *
 * Description: Function to add an Entry to the Log Queue. If the
 *              Queue is full then the function returns FULL, else the
 *              the function adds the entry to the Queue, increments the
 *              head and returns OK.
 *
 * Parameters:  LOG_REQUEST Entry - Request to be performed by the Log Mgr.
 *
 * Returns:     LOG_QUEUE_STATUS Status [LOG_QUEUE_OK, LOG_QUEUE_FULL]
 *
 * Notes:       None.
 *
 ****************************************************************************/
LOG_QUEUE_STATUS LogQueuePut(LOG_REQUEST Entry)
{
   // Local Data
   INT32 intLevel;
   LOG_REQ_QUEUE    *pQueue;
   LOG_QUEUE_STATUS status;

   // Initialize Local Data
   pQueue = &LogQueue;

   intLevel = __DIR();

   if ((pQueue->head - pQueue->tail) >= LOG_QUEUE_SIZE)
   {
      status = LOG_QUEUE_FULL;
   }
   else
   {
      pQueue->buffer[pQueue->head & (LOG_QUEUE_SIZE - 1)] = Entry;
      pQueue->head++;
      status = LOG_QUEUE_OK;
   }
   __RIR( intLevel);

   return status;
}


/******************************************************************************
 * Function:    LogPreInit()
 *
 * Description: Log pre-initialization starts some of the log manager function
 *              while minimally using other software modules.  It is intended
 *              to be called during early initialization so that other modules
 *              can write system logs before the entire system is running.
 *              This pre-init function has no dependencies on other modules
 *              being initialized.
 *              Once this init is called, the LogWriteSystem function can
 *              be used.
 *
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:
 *
 *****************************************************************************/
void LogPreInit(void)
{
   INT32 i;
   // Create and initialize the Write/Erase Queue
   LogQueueInit();

   //Setup for log system write queue
   for (i=0; i<SYSTEM_TABLE_MAX_SIZE; i++)
   {
      systemTable[i].logType  = LOG_TYPE_DONT_CARE;
      systemTable[i].source   = SYS_ID_NULL_LOG;
      systemTable[i].priority = LOG_PRIORITY_DONT_CARE;
      systemTable[i].wrStatus = LOG_REQ_NULL;
      systemTable[i].result   = SYS_OK;
      memset(&systemTable[i].payload, 0, sizeof(systemTable[i].payload));
   }

   // indicate the system is in the Pre-Init state
   LogSystemInit = FALSE;
}

/******************************************************************************
 * Function:    LogPauseWrites
 *
 * Description: The LogPauseRequests function will stop the Log Manager from
 *              processing any write requests.
 *
 * Parameters:  BOOLEAN Enable   - Enables and Disables the pause
 *              UINT16  Delay_S  - Number of seconds to pause
 *
 * Returns:     None
 *
 * Notes:
 *
 *****************************************************************************/
void LogPauseWrites ( BOOLEAN Enable, UINT16 Delay_S )
{
   if ( FALSE == LogManagerPause.enable )
   {
      LogManagerPause.startTime_ms = 0;
   }

   LogManagerPause.enable   = Enable;
   LogManagerPause.delay_S  = Delay_S;
}

/******************************************************************************
 * Function:    LogWriteIsPaused
 *
 * Description: Check if the Log Write is Paused.
 *
 * Parameters:  None
 *
 * Returns:     [TRUE  : Writes are paused]
 *              [FALSE : Writes are not paused]
 *
 * Notes:
 *
 *****************************************************************************/
BOOLEAN LogWriteIsPaused ( void )
{
   // Local Data
   BOOLEAN bPaused;

   bPaused = FALSE; // Assumed it isn't commanded to Pause then set true if it is

   // Check if Log Write has been commanded to pause
   if ( TRUE == LogManagerPause.enable )
   {
      // Temporarily set the Pause to TRUE until the timeout is checked
      bPaused = TRUE;

      // Is this the first time checking this?
      if ( 0 == LogManagerPause.startTime_ms )
      {
         // Set the Start Time
         LogManagerPause.startTime_ms = CM_GetTickCount();
      }
      // We were already paused keep checking for the timeout
      else if ( (CM_GetTickCount() - LogManagerPause.startTime_ms) >
                (LogManagerPause.delay_S * MILLISECONDS_PER_SECOND) )
      {
         LogManagerPause.enable       = FALSE;
         bPaused                       = FALSE;
         LogManagerPause.startTime_ms = 0;
      }
   }

   return (bPaused);
}

/******************************************************************************
 * Function:    LogRegisterEraseCleanup
 *
 * Description: Register any functions that need to be run before a Log Erase
 *              is executed.
 *
 * Parameters:  Func - Pointer to function to run before running log.erase
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
void LogRegisterEraseCleanup ( DO_CLEANUP Func )
{
   ASSERT (NULL != Func);

   DoCleanup = Func;
}

/******************************************************************************
 * Function:    LogFindNextRecord
 *
 * Description: The LogFindNext function will search from the current read
 *              offset and locate the next requested log.
 *
 * Parameters:  LOG_STATE State       - State of the log [NEW or ERASED]
 *              LOG_TYPE  Type        - Type of log stored [DATA or SYSTEM]
 *              LOG_SOURCE Source     - Index of the ACS log
 *              LOG_PRIORITY Priority - Priority of the stored log [LOW or HIGH]
 *              UINT32 *pRdOffset     - Offset to start the search from
 *              UINT32 EndOffset      - Offset to search up to, passing in
 *                                      LOG_NEXT will use the end of the memory
 *                                      block.
 *              UINT32 *pFoundOffset  - [in/out] Storage location of the found
 *                                               log.
 *              UINT32 *pSize         - Size of the log that was found
 *
 * Returns:     LOG_FIND_STATUS       - Status of the requested search
 *                                      [LOG_BUSY, LOG_FOUND, LOG_NOT_FOUND]
 *
 * Notes:       The Log Find Next should use the ACS, TYPE, PRIORITY and STATE
 *              to determine which log is being seeked. The Log Find Next will
 *              return the offset to the log when the criteria is met.
 *              Each of the fields should be enumerated to include a Don't
 *              care state. This way the calling function can search on just
 *              the parameter it wants. For example if the calling function is
 *              looking for system type logs the other fields would be set to
 *              don't care and the type would be set to SYS.
 *
 *****************************************************************************/
LOG_FIND_STATUS LogFindNextRecord (LOG_STATE State,      LOG_TYPE Type,
                                   LOG_SOURCE Source,    LOG_PRIORITY Priority,
                                   UINT32 *pRdOffset,    UINT32 EndOffset,
                                   UINT32 *pFoundOffset, UINT32 *pSize)
{
   // Local Data
   BOOLEAN         bDone;
   LOG_HDR_STATUS  hdrStatus;
   LOG_FIND_STATUS findStatus;
   LOG_HEADER      hdr;
   RESULT          result;
   UINT32          previousOffset;

   // Initialize Local Data
   findStatus = LOG_NOT_FOUND;

   // Make sure the memory manager is working before starting the search.
   // If the memory manager is faulted or disabled return LOG_NOT_FOUND.
   if (SYS_OK == MemMgrStatus())
   {
      // If the end offset is LOG_NEXT then search until the end of
      // the memory block is found.
      if (LOG_NEXT == EndOffset)
      {
         EndOffset = LogConfig.nEndOffset;
      }

      // Check the Read Offset has not exceeded end of block OR Last Offset
      while ((*pRdOffset <= EndOffset) &&
             ((*pRdOffset + sizeof(hdr)) < LogConfig.nEndOffset))
      {
         // Read the header at the current Read Offset
         // Note: Using MemRead will advance the Offset to the location
         //       following the size read.
         bDone = MemRead ( MEM_BLOCK_LOG,      pRdOffset, &hdr,
                           sizeof(LOG_HEADER), &result);

         // Was the read successful?
         if (TRUE == bDone)
         {
            // Set a local offset variable for search manipulation
            previousOffset = *pRdOffset - sizeof(LOG_HEADER);

            // Get the status of the header.
            hdrStatus = LogCheckHeader(hdr);

            // Is the log header valid?
            if (LOG_HDR_FOUND == hdrStatus)
            {
               // Increment Read Offset by the header size
               *pRdOffset += MEM_ALIGN_SIZE(hdr.nSize);

               // Does the filter match?
               if (TRUE == LogHdrFilterMatch(hdr, State, Type, Source, Priority))
               {
                  // Log was found, set offset, set size and exit loop
                  findStatus    = LOG_FOUND;
                  *pFoundOffset = previousOffset;
                  *pSize        = hdr.nSize;
                  break;
               }
            }
            // Is the header FREE?
            else if (LOG_HDR_FREE == hdrStatus)
            {
               // Reached the end of logs, Log was not found, exit loop
               findStatus = LOG_NOT_FOUND;
               break;
            }
            else // Header NOT Valid and NOT FREE
            {
               // Handle the corrupted memory case
               if ((previousOffset % MemGetSectorSize()) == 0)
               {
                  // Once the software has reached a sector boundary it
                  // should be able to go sector by sector because records are
                  // not allowed to be written across sectors
                  *pRdOffset = previousOffset + MemGetSectorSize();
               }
               else
               {
                  // Increment the offset by location
                  *pRdOffset = previousOffset + sizeof(UINT32);
               }
            }
         }
         else // Mem Read is Busy
         {
            // The memory read cannot be performed
            findStatus = LOG_BUSY;

            LogReadFailTemp.result = result;
            LogReadFailTemp.offset = *pRdOffset;

            LogCheckResult ( result, "LogFindNextRecord:",
                             SYS_LOG_MEM_READ_FAIL, STA_NORMAL,
                             &LogError.bReadAccessFail, &LogError.counts.nRdAccess,
                             &LogReadFailTemp, sizeof(LogReadFailTemp)
                           );

            // Exit the loop
            break;
         }
      }
   }

   // Return the status of the requested search
   return (findStatus);

} // LogFindNextRecord

/******************************************************************************
 * Function:    LogRead
 *
 * Description: The LogRead function allows the calling application to
 *              read a log from a specific location. The calling application
 *              must know what the offset is for a valid log to be returned.
 *
 * Parameters:  UINT32 nOffset  - Offset of the log to read
 *              UINT32 *pBuf    - Location to store the log read
 *
 * Returns:     LOG_READ_STATUS - Status of the requested log read
 *                                [LOG_READ_SUCCESS, LOG_READ_BUSY, LOG_READ_FAIL]
 *
 * Notes:       The calling function must know the correct offset to
 *              successfully read the log. The offset can be determine
 *              by using the LogFindNextFunction.
 *
 *****************************************************************************/
LOG_READ_STATUS LogRead ( UINT32 nOffset, LOG_BUF *pBuf )
{
   // Local Data
   LOG_HEADER      hdr;
   RESULT          result;
   BOOLEAN         bDone;

   // Default - Error Reporting should be handled by calling function
   // There wasn't a valid record found so return Fail Status
   LOG_READ_STATUS status = LOG_READ_FAIL;

   // Read the log based on the offset
   bDone = MemRead(MEM_BLOCK_LOG, &nOffset, &hdr,
                   sizeof(LOG_HEADER), &result);

   // Check the read could be done.
   if (TRUE == bDone)
   {
      // Read the header of the requested log, verify that it is valid
      if (LOG_HDR_FOUND == LogCheckHeader(hdr))
      {
         // We have a valid header now get the log
         bDone = MemRead(MEM_BLOCK_LOG, &nOffset,
                         pBuf, hdr.nSize, &result);

         // Check that the read completed successfully
         if (TRUE == bDone)
         {
            // Data was successfully copied to the passed in buffer
            status = LOG_READ_SUCCESS;
         }
         else // Can't read the log because the Memory manager is busy
         {
            status = LOG_READ_BUSY;

            LogReadFailTemp.result = result;
            LogReadFailTemp.offset = nOffset - hdr.nSize;

            LogCheckResult ( result, "LogRead:",
                             SYS_LOG_MEM_READ_FAIL, STA_NORMAL,
                             &LogError.bReadAccessFail, &LogError.counts.nRdAccess,
                             &LogReadFailTemp, sizeof(LogReadFailTemp)
                           );

         }
      }
   }
   else // Couldn't read the memory because the memory manager is
        // busy or the passed in data was bad
   {
      status = LOG_READ_BUSY;

      LogReadFailTemp.result = result;
      LogReadFailTemp.offset = nOffset - sizeof(LOG_HEADER);

      LogCheckResult ( result, "LogRead:",
                       SYS_LOG_MEM_READ_FAIL, STA_NORMAL,
                       &LogError.bReadAccessFail, &LogError.counts.nRdAccess,
                       &LogReadFailTemp, sizeof(LogReadFailTemp)
                     );
   }

   return status;
} // LogRead

/******************************************************************************
 * Function:    LogWrite
 *
 * Description: Creates a log based on the data passed from the application
 *              This function will add the flash memory header calculate the
 *              checksum and place request in the Write/Erase Queue.
 *              A location must be passed in so that the calling function can
 *              monitor the status of write.
 *
 * Parameters:  LOG_TYPE  Type            - Type of log [DATA or SYSTEM]
 *              LOG_SOURCE Source         - Index of the ACS log
 *              LOG_PRIORITY Priority     - Priority of the log [LOW or HIGH]
 *              void *pBuf                - Location of the data to write
 *              UINT16 nSize              - Size in bytes of the data to write
 *              UINT32 Checksum           - Checksum of all the data to write
 *              LOG_REQ_STATUS *pWrStatus - Location to store the write status
 *
 * Returns:     None.
 *
 * Notes:       The maximum size of a log is only 64Kbytes. There are two
 *              reasons for limiting the size. The 1st is the amount of
 *              time it will take to write larger logs. 64Kbytes will take
 *              approximately 150milliseconds to write. This is 15% of realtime.
 *              The second reason is that the Log Header has only allocated
 *              16 bits for the size, which limits the log size to 64K anyway.
 *
 *
 *****************************************************************************/
void LogWrite (LOG_TYPE Type, LOG_SOURCE Source, LOG_PRIORITY Priority,
                  void *pBuf, UINT16 nSize, UINT32 Checksum, LOG_REQ_STATUS *pWrStatus )
{
   // Local Data
   LOG_REQUEST new;
   LOG_REQUEST old;
   UINT32      *pData = pBuf;

   ASSERT (NULL != pWrStatus);

   // Build Write Request
   new.reqType                        = LOG_REQ_WRITE;
   new.pStatus                        = pWrStatus;
   new.request.write.hdr.nSize        = nSize;
   new.request.write.hdr.nSizeInverse = ~nSize;
   new.request.write.hdr.nState       = LOG_NEW;
   new.request.write.hdr.nType        = (UINT16)Type;
   new.request.write.hdr.nSource      = Source.nNumber;
   new.request.write.hdr.nPriority    = (UINT16)Priority;
   new.request.write.hdr.nChecksum    = Checksum;
   new.request.write.pData            = pData;
   new.request.write.nSize            = nSize;

   // Place Request in Queue
   while ( LOG_QUEUE_FULL == LogQueuePut(new) )
   {
      // The queue was full.. pop the oldest log
      if ( LOG_QUEUE_EMPTY != LogQueueGet(&old) )
      {
         // Queue Shouldn't be empty because we just detected it was full.

         // Update the status of the LOG to complete even though
         // it wasn't recorded
         *(old.pStatus) = LOG_REQ_COMPLETE;
         // Count the log that was just discarded
         LogError.counts.nDiscarded++;
         // Store count to RTC RAM
         NV_Write(NV_LOG_COUNTS,0,&LogError.counts,sizeof(LogError.counts));
      }
   }

   // Set the write status as pending
   *pWrStatus = LOG_REQ_PENDING;

} // LogWrite

/******************************************************************************
 * Function:    LogErase
 *
 * Description: The log erase function will queue the request for the
 *              log Manager task to service. The function will return
 *              success if the request is queued. The function also requires
 *              a storage location to report back the offset of any log that
 *              has not been marked for erasure, and storage location to
 *              put the status of the erase.
 *
 *
 * Parameters:  UINT32 *pFoundLogOffset - [in/out] Storage location for storing
 *                                        the offset of any record that has
 *                                        not been marked for erasure.
 *              LOG_REQ_STATUS *pErStatus - Storage location to place the
 *                                          updated erase status.
 *
 * Returns:     None.
 *
 * Notes:       None.
 *
 *****************************************************************************/
void LogErase ( UINT32 *pFoundLogOffset, LOG_REQ_STATUS *pErStatus )
{
   // Local Data
   LOG_REQUEST new;

   ASSERT (NULL != pErStatus);

   new.reqType                    = LOG_REQ_ERASE;
   new.pStatus                    = pErStatus;
   new.request.erase.pFoundOffset = pFoundLogOffset;
   new.request.erase.nStartOffset = LogConfig.nStartOffset;
   new.request.erase.type         = LOG_ERASE_NORMAL;

   LogEraseRequest = new;

   // Set the write status as pending
   *pErStatus = LOG_REQ_PENDING;

} // LogErase

/******************************************************************************
 * Function:    LogMarkState
 *
 * Description: The log mark state function will update just the status of the
 *              log header. This function can be used by the application
 *              mark the logs as deleted after a successful transmission.
 *
 * Parameters:  LOG_STATE       State       - State to mark the logs
 *              LOG_TYPE        Type        - Type of log to mark System or Data
 *              LOG_SOURCE      Source      - Index of logs  to mark
 *              LOG_PRIORITY    Priority    - priority of logs to mark
 *              UINT32          StartOffset - where to start marking logs from
 *              UINT32          EndOffset   - where to stop marking logs
 *              LOG_MARK_STATUS *pStatus    - storage location of log mark status
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
void LogMarkState (LOG_STATE State, LOG_TYPE Type,
                   LOG_SOURCE Source, LOG_PRIORITY Priority,
                   UINT32 StartOffset, UINT32 EndOffset,
                   LOG_MARK_STATUS *pStatus)
{
   // Local Data Initialization
   LogMarkBlock.hdrCriteria.nState    = State;
   LogMarkBlock.hdrCriteria.nType     = (UINT16)Type;
   LogMarkBlock.hdrCriteria.nSource   = Source.nNumber;
   LogMarkBlock.hdrCriteria.nPriority = (UINT16)Priority;
   LogMarkBlock.nStartOffset          = StartOffset;
   LogMarkBlock.nEndOffset            = EndOffset;
   LogMarkBlock.nRdOffset             = StartOffset;
   LogMarkBlock.pStatus               = pStatus;

   *(LogMarkBlock.pStatus)            =  LOG_MARK_IN_PROGRESS;

   TmTaskEnable ((TASK_INDEX)Log_Mark_State_Task, TRUE);

}


/******************************************************************************
 * Function:      LogWriteSystem
 *
 * Description:   The LogWriteSystem function is the wrapper writing system logs.
 *
 * Parameters:    SYS_APP_ID    logID     - System Log ID to write
 *                LOG_PRIORITY  priority  - Priority of the Log
 *                void          *pData    - Pointer to the data location
 *                UINT32        nSize     - Size of the data to write
 *                TIMESTAMP     *pTs      - Timestamp to use if passed in
 *
 * Returns:       None
 *
 * Notes:         The nSize value cannot be larger than 1Kbyte.
 *
 *****************************************************************************/
void LogWriteSystem (SYS_APP_ID logID, LOG_PRIORITY priority,
                        void *pData, UINT16 nSize, const TIMESTAMP *pTs)
{
   // Return the SystemTable index in case the caller needs to track the status.
   LogManageWrite ( logID, priority, pData, nSize, pTs, LOG_TYPE_SYSTEM );
}

/******************************************************************************
 * Function:      LogWriteSystemEx
 *
 * Description:   The LogWriteSystem Enhanced function returns an index to
 *                the caller so they can independently track the write status
 *                of the system log.
 *
 * Parameters:    SYS_APP_ID    logID     - System Log ID to write
 *                LOG_PRIORITY  priority  - Priority of the Log
 *                void          *pData    - Pointer to the data location
 *                UINT32        nSize     - Size of the data to write
 *                TIMESTAMP     *pTs      - Timestamp to use if passed in
 *
 * Returns:       UINT32 index value in SystemTable where the log is stored.
 *                This can be used by caller to track the commit-status
 *                of the Write.
 *
 * Notes:         The nSize value cannot be larger than 1Kbyte.
 *
 *****************************************************************************/
UINT32 LogWriteSystemEx ( SYS_APP_ID logID, LOG_PRIORITY priority,
                          void *pData, UINT16 nSize, const TIMESTAMP *pTs )
{
   // Return the SystemTable index in case the caller needs to track the status.
   return ( LogManageWrite ( logID, priority, pData, nSize, pTs, LOG_TYPE_SYSTEM ) );
}

/******************************************************************************
 * Function:      LogWriteETM
 *
 * Description:   The Log Write ETM function is the wrapper for writing ETM Logs.
 *
 * Parameters:    SYS_APP_ID    logID     - System Log ID to write
 *                LOG_PRIORITY  priority  - Priority of the Log
 *                void          *pData    - Pointer to the data location
 *                UINT32        nSize     - Size of the data to write
 *                TIMESTAMP     *pTs      - Timestamp to use if passed in
 *
 * Returns:
 *
 * Notes:         The nSize value cannot be larger than 1Kbyte.
 *
 *****************************************************************************/
void LogWriteETM (SYS_APP_ID logID, LOG_PRIORITY priority,
                    void *pData, UINT16 nSize, const TIMESTAMP *pTs)
{
   LogManageWrite ( logID, priority, pData, nSize, pTs, LOG_TYPE_ETM );
}


/******************************************************************************
* Function:    LogIsLogEmpty
*
* Description: This function returns a boolean identifying whether the log file
*              is empty.
*
* Parameters:  none
*
* Returns:     BOOLEAN  TRUE  - The log file is empty.
*                       FALSE - The log has entries which have not been unloaded
*
* Notes:       None.
*
*****************************************************************************/
BOOLEAN LogIsLogEmpty( void )
{
  return ( LogConfig.nLogCount == 0);
}

/******************************************************************************
* Function:    LogGetLogCount
*
* Description: This function returns the number of log not marked for
*              deletion, stored in the log memory.
*
* Parameters:  none
*
* Returns:     UINT32   Number of logs in memory
*
* Notes:       None.
*
*****************************************************************************/
UINT32 LogGetLogCount( void )
{
  return  LogConfig.nLogCount;
}

/******************************************************************************
* Function:    LogIsEraseInProgress
*
* Description: This function returns a boolean identifying whether a data-flash erase
*              is in progress.
*
* Parameters:  none
*
* Returns:     BOOLEAN  TRUE  - Erase is in progress.
*                       FALSE - Erase not in progress.
*
* Notes:       None.
*
*****************************************************************************/
BOOLEAN LogIsEraseInProgress( void )
{
  return LogEraseData.inProgress;
}
/*****************************************************************************
 * Function:    LogGetConfigPtr
 *
 * Description: Function to return pointer to the LogConfig structure.
 *
 * Parameters:  non
 *
 * Returns:     A const pointer to the LOG_CONFIG structure.
 *
 * Notes:       None.
 *
 ****************************************************************************/
 const LOG_CONFIG* LogGetConfigPtr(void)
 {
   return (const LOG_CONFIG*)&LogConfig;
 }


/******************************************************************************
 * Function:     LogGetCBITHealthStatus
 *
 * Description:  Returns the current CBIT Health status of the Data Flash
 *
 * Parameters:   None
 *
 * Returns:      Current Data in LOG_CBIT_HEALTH_COUNTS
 *
 * Notes:        None
 *
 *****************************************************************************/
LOG_CBIT_HEALTH_COUNTS LogGetCBITHealthStatus ( void )
{
  return (LogHealthCounts);
}


/******************************************************************************
 * Function:    LogCalcDiffCBITHealthStatus
 *
 * Description: Calc the difference in CBIT Health Counts to support PWEH SEU
 *              (Used to support determining SEU cnt during data capture)
 *
 * Parameters:  PrevCount - Initial count value
 *
 * Returns:     diffCount
 *
 * Notes:       Corrupt count only set on restart, so no need for diff calculation
 *              Normally would return Diff in CBIT_HEALTH_COUNTS from (Current - PrevCnt)
 *              Assignment code has been left in to accomodate possible future statuses.
 *
 *****************************************************************************/
LOG_CBIT_HEALTH_COUNTS LogCalcDiffCBITHealthStatus ( LOG_CBIT_HEALTH_COUNTS PrevCount )
{
  LOG_CBIT_HEALTH_COUNTS diffCount;

  diffCount.nCorruptCnt = LogConfig.nCorrupt;

  return (diffCount);
}


/******************************************************************************
 * Function:     LogAddPrevCBITHealthStatus
 *
 * Description:  Add CBIT Health Counts to support PWEH SEU
 *               (Used to support determining SEU cnt during data capture)
 *
 * Parameters:   PrevCnt - Prev count value
 *               CurrCnt - Current count value
 *
 * Returns:      Add CBIT_HEALTH_COUNTS from (CurrCnt + PrevCnt)
 *
 * Notes:        None
 *
 *****************************************************************************/
LOG_CBIT_HEALTH_COUNTS LogAddPrevCBITHealthStatus ( LOG_CBIT_HEALTH_COUNTS CurrCnt,
                                                    LOG_CBIT_HEALTH_COUNTS PrevCnt )
{
  LOG_CBIT_HEALTH_COUNTS addCount;

  addCount.nCorruptCnt = CurrCnt.nCorruptCnt + PrevCnt.nCorruptCnt;

  return (addCount);
}

/*****************************************************************************
 * Function:    LogGetLastAddress
 *
 * Description: Request to determine the last address in the log memory;
 *
 * Parameters:  None
 *
 * Returns:     Last Address offset in the log memory
 *
 * Notes:       None.
 *
 ****************************************************************************/
 UINT32 LogGetLastAddress(void)
 {
   return LogConfig.nEndOffset;
 }

/*****************************************************************************
 * Function:    LogIsWriteComplete
 *
 * Description: Accessor Function allow callers to determine if the write
 *              for the registered type of log has completed.
 *
 * Parameters:  regType is the type of logwrite which was registered for monitoring
 *
 * Returns:     TRUE: (Complete)
 *                    LOG_INDEX_NOT_SET == LogMngBlock.LogWritePending[regType]
 *              FALSE:(In prog)
 *                    0 <= LogMngBlock.LogWritePending[regType] < SYSTEM_TABLE_MAX_SIZE
 *
 * Notes:
 *
 ****************************************************************************/
BOOLEAN LogIsWriteComplete( LOG_REGISTER_TYPE regType )
{
  // Use the value of the index as indicator of write-complete
  return (LogMngBlock.logWritePending[regType] == LOG_INDEX_NOT_SET);
}

/**********************************************************************************************
 * Function:    LogETM_SetRecStateChangeEvt
 *
 * Description: Set the callback fuction to call when the busy/not busy status of the
 *              ETM Log writes.
 *
 * Parameters:  [in] tag:  Integer value to use when calling "func"
 *              [in] func: Function to call when busy/not busy status changes
 *
 * Returns:      none
 *
 * Notes:       Only remembers one event handler.  Subsequent calls overwrite the last
 *              event handler.
 *
 *********************************************************************************************/
void LogETM_SetRecStateChangeEvt(INT32 tag,void (*func)(INT32,BOOLEAN))
{
  m_event_func = func;
  m_event_tag  = tag;
}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:    LogManageTask
 *
 * Description: The Log Manage Task is responsible for reading commands from
 *              Write/Erase Queue and Manages the Process until each request
 *              is completed. The LogManageTask will continue to update the
 *              status so that the calling function has some feedback.
 *
 * Parameters:  void *pParam - pointer to the task control block
 *
 * Returns:     None
 *
 * Notes:
 *
 *****************************************************************************/
static
void LogManageTask( void *pParam )
{
   // Local Data
   LOG_MNG_TASK_PARMS *pTCB;

   // Initialize Local Data
   pTCB = (LOG_MNG_TASK_PARMS*)pParam;

   // Has the previous request complete?
   if (LOG_MNG_IDLE == pTCB->state)
   {
      // Begin processing the next request
      LogMngStart(pTCB);
   }

   // Did the previous request not start?
   if (LOG_MNG_PENDING == pTCB->state)
   {
      // Process the pending request
      LogMngPending(pTCB);
   }

   // Is the previous request being serviced?
   if (LOG_MNG_SERVICING == pTCB->state)
   {
      // Check if the request has completed
      LogMngStatus(pTCB);
   }

   // Check/Update the completion statuses of registered logs writes for
   // any observers that are watching.
   LogUpdateWritePendingStatuses();

} // LogManageTask

/*****************************************************************************
 * Function:     LogMngStart
 *
 * Description:  The Log Manage Start function is responsible for getting
 *               requests from the queue and kicking them off. If a request
 *               cannot be started because of a memory manager contention issue
 *               the request will be marked as pending and the Log Manager
 *               Pending function will continue to try and process the request.
 *
 * Parameters:   LOG_MNG_TASK_PARMS *pTCB - pointer to the task control block
 *
 * Returns:      None.
 *
 * Notes:
 *
 ****************************************************************************/
static
void LogMngStart (LOG_MNG_TASK_PARMS *pTCB)
{
   // Local Data
   RESULT  result;
   BOOLEAN bStarted;
   UINT32  nLogSize;
   UINT8   *pBuf;
   BOOLEAN bProcessRequest;

   // Initialize Local Data
   result         = SYS_OK;
   bStarted       = FALSE;
   pBuf           = (UINT8 *)&LogBuffer[0];
   bProcessRequest = FALSE;

   // Is an Erase Pending
   if ( (NULL != LogEraseRequest.pStatus) && (LOG_REQ_PENDING == *(LogEraseRequest.pStatus)) )
   {
      // Copy the Request
      pTCB->currentEntry = LogEraseRequest;
      bProcessRequest    = TRUE;
   }
   else // Check for a write
   {
      if (FALSE == LogWriteIsPaused())
      {
         bProcessRequest = ( LOG_QUEUE_EMPTY == LogQueueGet(&(pTCB->currentEntry)))
                             ? FALSE : TRUE;
      }
   }

   // Check if there a request to be processed
   if ( TRUE == bProcessRequest )
   {
      // Determine the type of request that needs to be processed
      if ( LOG_REQ_WRITE == pTCB->currentEntry.reqType )
      {
         // The Request is a write log, make sure it fits into the
         // log buffer.
         nLogSize = sizeof(LOG_HEADER) + pTCB->currentEntry.request.write.nSize;

         if (nLogSize < sizeof(LogBuffer))
         {
            // The log fits. Now add the header and data together in the
            // local buffer
            memcpy (pBuf,
                    &(pTCB->currentEntry.request.write.hdr),
                    sizeof(LOG_HEADER));
            memcpy (pBuf + sizeof(LOG_HEADER),
                    pTCB->currentEntry.request.write.pData,
                    pTCB->currentEntry.request.write.nSize);

            // Check if memory full
            if ( ((LogConfig.nWrOffset + nLogSize) < LogConfig.nEndOffset) )
            {
               // Command the Write
               bStarted = MemWrite ( MEM_BLOCK_LOG, &LogConfig.nWrOffset,
                                     pBuf, nLogSize, &result, FALSE);

               // Check if the write started successfully
               if (TRUE == bStarted)
               {
                  // Request was started now monitor the operation
                  // until it is complete.
                  pTCB->state                   = LOG_MNG_SERVICING;
                  *(pTCB->currentEntry.pStatus) = LOG_REQ_INPROG;
               }
               // Couldn't start
               else
               {
                  // Don't care why it didn't start, go to pending and start counting errors
                  pTCB->state = LOG_MNG_PENDING;
               }
            }
            else // Memory is full
            {
                pTCB->state                   = LOG_MNG_IDLE;
                *(pTCB->currentEntry.pStatus) = LOG_REQ_COMPLETE;
            }
         }
         else
         {
            // Data is too large to write...
            // Since the only operations make the write are Data Manager (12K max)
            // and System Log Write (1K max) the only way to get here is through
            // a memory corruption.
            FATAL( "LogMngStart: Log Write To Big. ResultCode: 0x%08X", result );
         }
      }
      else if ( LOG_REQ_ERASE == pTCB->currentEntry.reqType )
      {
         // Populate the TCB for the erase task
         LogCmdEraseBlock.bDone   = FALSE;
         LogCmdEraseBlock.result  = SYS_OK;
         LogCmdEraseBlock.nOffset = pTCB->currentEntry.request.erase.nStartOffset;
         LogCmdEraseBlock.type    = pTCB->currentEntry.request.erase.type;
         LogCmdEraseBlock.nSize   = LogConfig.nWrOffset - LogConfig.nStartOffset;
         // If the erase is a normal system erase verify that all logs
         // are marked for deletion before starting the erase.
         if (LOG_ERASE_NORMAL == pTCB->currentEntry.request.erase.type)
         {
            LogCmdEraseBlock.state  = LOG_VERIFY_DELETED;
            LogCmdEraseBlock.pFound = pTCB->currentEntry.request.erase.pFoundOffset;
            LogCmdEraseBlock.bVerifyStarted = FALSE;
         }
         else // This is a user commanded erase, erase all the sectors that
              // contain data.
         {
            LogCmdEraseBlock.state = LOG_ERASE_STATUS_WRITE;
         }
         // Request was started now monitor the operation
         // until it is complete.
         pTCB->state                   = LOG_MNG_SERVICING;
         *(pTCB->currentEntry.pStatus) = LOG_REQ_INPROG;
         // Enable the erase task
         TmTaskEnable ((TASK_INDEX)Log_Command_Erase_Task, TRUE);
      }
      else
      {
         // Invalid Request - Sad Path
         FATAL( "LogMngStart: Invalid Command. ResultCode: 0x%08X", result );
      }
   }
}

/*****************************************************************************
 * Function:    LogMngPending
 *
 * Description: The LogMngPending functioning is used by the log manager
 *              task to kick-off any pending requested that could not be
 *              started in the Start state. This state will try for TBD
 *              number of times before it reports a failure.
 *
 * Parameters:  LOG_MNG_TASK_PARMS *pTCB
 *
 * Returns:     None.
 *
 * Notes:
 *
 ****************************************************************************/
static
void LogMngPending (LOG_MNG_TASK_PARMS *pTCB)
{
   // Local Data
   BOOLEAN bStarted;
   RESULT  result;
   UINT32  *pBuf;
   UINT32  nSize;

   // Initialize Local Data
   pBuf     = &LogBuffer[0];
   bStarted = FALSE;

   // Determine the type of request that needs to be processed
   if ( LOG_REQ_WRITE == pTCB->currentEntry.reqType )
   {
      // Calculate the size including the header
      nSize = sizeof(LOG_HEADER) + pTCB->currentEntry.request.write.nSize;

      // Check if memory full
      if ( TPU( (LogConfig.nWrOffset + nSize), eTpLogPend) < LogConfig.nEndOffset)
      {
         // Command the Write
         bStarted = MemWrite (MEM_BLOCK_LOG, &LogConfig.nWrOffset,
                              pBuf, nSize, &result, FALSE);

         // Check that the request was successfully started
         if ( TRUE == TPU( bStarted, eTpLogPend) )
         {
            // Request was started now monitor the operation
            // until it is complete.
            pTCB->state                   = LOG_MNG_SERVICING;
            *(pTCB->currentEntry.pStatus) = LOG_REQ_INPROG;
         }
         else // Could Not Start Request
         {
            // Check if the request couldn't start because of an error
            if ( SYS_OK != TPU( result, eTpLogPend) )
            {
               if ( LOG_MAX_FAIL == pTCB->faultCount )
               {
                  // Three Attempts Failed - Now ASSERT
                  FATAL( "LogMngPending: Write Failed. ResultCode: 0x%08X", result );
               }
               else
               {
                  // Increment the fault count and stay in the pending state
                  pTCB->state = LOG_MNG_PENDING;
                  pTCB->faultCount++;
               }
            }
         }
      }
      else // Memory is full or faulted
      {
         pTCB->state                   = LOG_MNG_IDLE;
         *(pTCB->currentEntry.pStatus) = LOG_REQ_COMPLETE;
      }
   }
   else if (LOG_REQ_ERASE == pTCB->currentEntry.reqType)
   {
      // Need to reset Done and Result because this may be a retry.
      LogCmdEraseBlock.bDone   = FALSE;
      LogCmdEraseBlock.result  = SYS_OK;
      // If the erase is a normal system erase verify that all logs
      // are marked for deletion before starting the erase.
      if (LOG_ERASE_NORMAL == pTCB->currentEntry.request.erase.type)
      {
         LogCmdEraseBlock.state          = LOG_VERIFY_DELETED;
         LogCmdEraseBlock.bVerifyStarted = FALSE;
      }
      else // This is a user commanded erase, erase all the sectors that
           // contain data.
      {
         LogCmdEraseBlock.state = LOG_ERASE_STATUS_WRITE;
         LogCmdEraseBlock.nSize = LogConfig.nWrOffset - LogConfig.nStartOffset;
      }
      // Request was started now monitor the operation
      // until it is complete.
      pTCB->state                   = LOG_MNG_SERVICING;
      *(pTCB->currentEntry.pStatus) = LOG_REQ_INPROG;
      // Enable the erase task
      TmTaskEnable ((TASK_INDEX)Log_Command_Erase_Task, TRUE);
   }
}

/*****************************************************************************
 * Function:    LogMngStatus
 *
 * Description: The LogMngStatus function is the state responsible for
 *              monitoring the completion of a requested action.
 *
 * Parameters:  LOG_MNG_TASK_PARMS *pTCB
 *
 * Returns:     None
 *
 * Notes:       None
 *
 ****************************************************************************/
static
void LogMngStatus (LOG_MNG_TASK_PARMS *pTCB)
{
   // Local Data
   BOOLEAN        bDone;
   RESULT         result;
   UINT32         nOffset;

   // Initialize Local Data
   bDone = FALSE;

  // Check the requested action type
  if ( LOG_REQ_WRITE == pTCB->currentEntry.reqType )
  {
     // Log Manage is programming, Check the current status
     bDone = MemGetStatus (MEM_STATUS_PROG, MEM_BLOCK_LOG,
                           &nOffset, &result);

     // Check if the requested write has completed
     if (bDone)
     {
        // Set the WrOffset to the new offset
        LogConfig.nWrOffset     = nOffset;
        LogConfig.nLogCount++;
        LogConfig.fPercentUsage = ((FLOAT32)LogConfig.nWrOffset/
                                   (FLOAT32)LogConfig.nEndOffset) * 100.0f;

        if ( (LogConfig.fPercentUsage > LOGMGR_PERCENTUSAGE_MAX) &&
             (LogConfig.bEightyFiveExceeded == FALSE) )
        {
           LogConfig.bEightyFiveExceeded = TRUE;
           // Create Log for 85% exceedence
           GSE_DebugStr(NORMAL,TRUE, "LogMngStatus: Log Memory > 85 percent Full");
           // Store the fault log data
           Flt_SetStatus(STA_CAUTION, SYS_LOG_85_PERCENT_FULL, NULL, 0);
        }
     }
  }
  else if ( LOG_REQ_ERASE == pTCB->currentEntry.reqType )
  {
     // Get the current erase status
     bDone  = LogCmdEraseBlock.bDone;
     result = LogCmdEraseBlock.result;
  }

  // Check if the request has completed
  if (bDone)
  {
     // Done now check for any faults
     if (SYS_OK != TPU( result, eTpMem3104))
     {
        // Check if fault threshold has been reached
        if (LOG_MAX_FAIL == pTCB->faultCount)
        {
           if ( LOG_REQ_ERASE == pTCB->currentEntry.reqType )
           {
              LogEraseFailTemp.result    = result;
              LogEraseFailTemp.eraseData = pTCB->currentEntry.request.erase;

              LogCheckResult ( result, "LogMngStatus:",
                               SYS_LOG_MEM_ERASE_FAIL, STA_NORMAL,
                               &LogError.bEraseAccessFail, &LogError.counts.nErAccess,
                               &LogEraseFailTemp, sizeof(LogEraseFailTemp) );
           }
           else
           {
              FATAL( "LogMngPending: Write Failed. ResultCode: 0x%08X", result );
           }

           pTCB->state      = LOG_MNG_IDLE;
           pTCB->faultCount = 0;
           *(pTCB->currentEntry.pStatus) = LOG_REQ_FAILED;
        }
        else
        {
           // Increment the fault count and go back to the pending state
           // to retry the request
           pTCB->state = LOG_MNG_PENDING;
           pTCB->faultCount++;
        }
     }
     else
     {
        // request has completed now return to the idle state
        pTCB->state      = LOG_MNG_IDLE;
        pTCB->faultCount = 0;

        if ( (LOG_REQ_ERASE    == pTCB->currentEntry.reqType) &&
             (LOG_ERASE_NORMAL == pTCB->currentEntry.request.erase.type) &&
             (LOG_NEXT != *(pTCB->currentEntry.request.erase.pFoundOffset)) )
        {
           *(pTCB->currentEntry.pStatus) = LOG_REQ_FAILED;
        }
        else
        {
           *(pTCB->currentEntry.pStatus) = LOG_REQ_COMPLETE;
        }
     }
  }
}

/******************************************************************************
 * Function:    LogCheckResult
 *
 * Description: This function will check the result code, count the fails and
 *              log a failure if an error is encountered.
 *
 * Parameters:  RESULT      Result       - Result code to check
 *              CHAR       *FuncStr      - String to display
 *              SYS_APP_ID  LogID        - ID of the system Log
 *              FLT_STATUS  FaultStatus  - System Condition to set the box
 *              BOOLEAN    *pFailFlag    - Location of the Fail flag
 *              UINT32     *pCnt         - Location of the fail counter
 *              void       *LogData      - Log Data location
 *              INT32       LogDataSize  - Log Data Size
 *
 * Returns:     None
 *
 * Notes:
 *
 *****************************************************************************/
static void LogCheckResult ( RESULT Result, CHAR *FuncStr, SYS_APP_ID LogID,
                             FLT_STATUS FaultStatus, BOOLEAN *pFailFlag, UINT32 *pCnt,
                             void *LogData, INT32 LogDataSize )
{
   // Check if there was an error
   if (SYS_OK != Result)
   {
      // There was an error print it out on the screen
      GSE_DebugStr( NORMAL, TRUE, "%s Failed. ResultCode: 0x%08X", FuncStr, Result );
      // Was the fail flag already set?
      if (FALSE == *pFailFlag)
      {
         // Fail flag not set, set it now
         *pFailFlag = TRUE;
         // Record the log and set the system status
         Flt_SetStatus(FaultStatus, LogID, LogData, LogDataSize);
      }
      // Count the failure
      *pCnt += 1;

      NV_Write(NV_LOG_COUNTS,0,&LogError.counts,sizeof(LogError.counts));
   }
}

/******************************************************************************
 * Function:    LogMarkTask
 *
 * Description: The log mark task is responsible for searching the logs for
 *              the specified parameters and marking any of those logs to the
 *              passed in state.
 *
 * Parameters:  void *pParam - Pointer to the TCB
 *
 * Returns:     None.
 *
 * Notes:       This task should be used to write a range of logs to a state.
 *
 *
 *****************************************************************************/
static
void LogMarkTask( void *pParam )
{
   // Local Data
   BOOLEAN              bDone;
   LOG_FIND_STATUS      findStatus;
   UINT32               nFoundOffset;
   UINT32               nSize;
   UINT32               nStartTime;
   UINT32               nStateLocation;
   LOG_STATE_TASK_PARMS *pTCB;
   RESULT               result;
   LOG_SOURCE           source;

   // When TP are active can trigger other TPs the LogMarkTask is active
   TPTR( eTpLogMark);

   // Initialize Local Data
   pTCB           = (LOG_STATE_TASK_PARMS *)pParam;
   source.nNumber = pTCB->hdrCriteria.nSource;

   // Check if writes are paused and the Log Manager is IDLE
   // This will avoid LOG_MARK_FAIL because Writes are being performed
   if (LogWriteIsPaused() && (LOG_MNG_IDLE == LogMngBlock.state))
   {
      nStartTime     = TTMR_GetHSTickCount();

      // While not at the end of the block and 2 milliseconds has not passed
      while ( (pTCB->nRdOffset <= pTCB->nEndOffset) &&
              (TTMR_GetHSTickCount() - nStartTime < (2 * TICKS_PER_mSec)) )
      {
         findStatus = LogFindNextRecord (LOG_NEW,
                                        (LOG_TYPE)pTCB->hdrCriteria.nType,
                                         source,
                                        (LOG_PRIORITY)pTCB->hdrCriteria.nPriority,
                                         &pTCB->nRdOffset,
                                         pTCB->nEndOffset,
                                         &nFoundOffset,
                                         &nSize);

         if (LOG_FOUND == findStatus)
         {
            // Write the header state
            nStateLocation = nFoundOffset + ((UINT32)&pTCB->hdrCriteria.nState -
                                             (UINT32)&(pTCB->hdrCriteria));

            bDone = MemWrite ( MEM_BLOCK_LOG,
                               &nStateLocation,
                               &(pTCB->hdrCriteria.nState),
                               sizeof(pTCB->hdrCriteria.nState),
                               &result,
                               TRUE );

            // Was write successful?
            if (FALSE == TPU( bDone, eTpLogMark))
            {
               *(pTCB->pStatus) = LOG_MARK_FAIL;
               TmTaskEnable ((TASK_INDEX)Log_Mark_State_Task, FALSE);
               break;
            }
            else
            {
               LogConfig.nLogCount--;
               LogConfig.nDeleted++;
            }
         }
         else if (LOG_BUSY == findStatus)
         {
            *(pTCB->pStatus) = LOG_MARK_FAIL;
            TmTaskEnable ((TASK_INDEX)Log_Mark_State_Task, FALSE);
            break;
         }
         else if (LOG_NOT_FOUND == findStatus)
         {
            *(pTCB->pStatus) = LOG_MARK_COMPLETE;
            TmTaskEnable ((TASK_INDEX)Log_Mark_State_Task, FALSE);
            break;
         }

         if (pTCB->nRdOffset > pTCB->nEndOffset)
         {
            *(pTCB->pStatus) = LOG_MARK_COMPLETE;
            TmTaskEnable ((TASK_INDEX)Log_Mark_State_Task, FALSE);
         }
      }
   }
   else // Pause Writes for 1 second
   {
      LogPauseWrites (TRUE, 1);
   }
} // LogMarkTask


/******************************************************************************
 * Function:    LogCmdEraseTask
 *
 * Description: The Log Command Erase task is responsible for verifying the
 *              data flash can be erased, commanding the erase and monitoring
 *              the erase status.
 *
 * Parameters:  void *pParam - Pointer to the TCB
 *
 * Returns:     None.
 *
 * Notes:       None.
 *
 *****************************************************************************/
static
void LogCmdEraseTask ( void *pParam )
{
   // Local Data
   LOG_CMD_ERASE_PARMS *pTCB;
   BOOLEAN             bDone;
   BOOLEAN             bStarted;

   // Initialize Local Data
   pTCB        = (LOG_CMD_ERASE_PARMS*)pParam;
   pTCB->bDone = FALSE;

   // Check the current state of the erase
   switch (pTCB->state)
   {
      case LOG_VERIFY_DELETED:
         // The software must first verify that all the logs in memory
         // have been marked for erasure.
         // Check if the task has already started the deleted verification
         if (FALSE == pTCB->bVerifyStarted)
         {
            // This is the first time in this state so set the search
            // parameters for verifying the logs are marked.
            *(pTCB->pFound)      = LOG_NEXT;
            pTCB->bVerifyStarted = TRUE;
            pTCB->nRdOffset      = pTCB->nOffset;
         }

         // Search through the memory for any logs that are not
         // marked for erasure.
         bDone = LogVerifyAllDeleted ( &pTCB->nRdOffset,
                                       LogConfig.nWrOffset,
                                       pTCB->pFound );

         // Did the Log Verify complete?
         if (TRUE == bDone)
         {
            // Reset the verify started flag
            pTCB->bVerifyStarted = FALSE;

            // check if any log was found
            if (LOG_NEXT == *(pTCB->pFound))
            {
               // No log was found so the memory can now be erased
               pTCB->state      = LOG_ERASE_STATUS_WRITE;
            }
            else // oops there is at least one unmarked log return the
                 // offset to the calling function, Cancel the current erase
                 // request
            {
               pTCB->bDone  = TRUE;
               // Kill this task
               TmTaskEnable ((TASK_INDEX)Log_Command_Erase_Task, FALSE);

            }
         }
         break;

      case LOG_ERASE_STATUS_WRITE:
        // Store Current Erase Data to EEPROM
        LogEraseStatusWrite (pTCB);
        break;

      case LOG_ERASE_MEMORY:

         // Command Erase
         bStarted =  MemErase (MEM_BLOCK_LOG, &pTCB->nOffset,
                               pTCB->nSize, &pTCB->result);

         // Verify the erase does not generate an error and it started
         if ((SYS_OK == pTCB->result) && (TRUE == bStarted))
         {
            // Change the state to get status
            pTCB->state = LOG_ERASE_STATUS;

            // If the erase was user commanded start the monitor task
            if (LOG_ERASE_QUICK == pTCB->type)
            {
               // Perform Any Cleanup - because this was commanded by the USER
               DoCleanup();
               // Start the erase monitor
               GSE_DebugStr(NORMAL,TRUE, "LogCmdEraseTask: Erase..");
               TmTaskEnable ((TASK_INDEX)Log_Monitor_Erase_Task, TRUE);
            }
         }
         else
         {
            pTCB->result    |= SYS_LOG_ERASE_CMD_FAILED;
            pTCB->bDone      = TRUE;

            // Update the EEPROM to reflect finished
            // ( nOffset, nSize, bChipErase, bInProgress, bWriteNow )
            LogSaveEraseData (0, 0, FALSE, FALSE, FALSE);

            // Kill this task
            TmTaskEnable ((TASK_INDEX)Log_Command_Erase_Task, FALSE);

         }
         break;

      case LOG_ERASE_STATUS:
         // Log Manage is erasing the memory, check the current status
         pTCB->bDone = MemGetStatus (MEM_STATUS_ERASE, MEM_BLOCK_LOG,
                                     &(pTCB->nOffset), &(pTCB->result));

         // Check if the Erase request has completed
         if (pTCB->bDone)
         {
            if (SYS_OK == pTCB->result)
            {
               // Reset the read and write offsets
               LogConfig.nWrOffset     = 0;
               LogConfig.nLogCount     = 0;
               LogConfig.nDeleted      = 0;
               LogConfig.nCorrupt      = 0;
               LogConfig.fPercentUsage = 0.0;

               // If 85% Exceeded, must clear current system cond generated by th 85% sys
               //   cond detected.
               if (LogConfig.bEightyFiveExceeded == TRUE)
               {
                  Flt_ClrStatus( STA_CAUTION );
               }
               LogConfig.bEightyFiveExceeded = FALSE;
            }
            else
            {
               pTCB->result |= SYS_LOG_ERASE_CMD_FAILED;
            }

            // Update the EEPROM to reflect finished
            // ( nOffset, nSize, bChipErase, bInProgress, bWriteNow )
            LogSaveEraseData (0, 0, FALSE, FALSE, FALSE);

            // Kill self
            TmTaskEnable ((TASK_INDEX)Log_Command_Erase_Task, FALSE);
         }
         break;
      case LOG_ERASE_VERIFY_NONE:
      default:
         FATAL("LogCmdEraseTask: Invalid LOG_ERASE_STATE = %d", pTCB->state);
         break;
   }
}

/******************************************************************************
 * Function:    LogEraseStatusWrite
 *
 * Description: The LogEraseStatusWrite function manages writing the erase 
 *              status data to non-volatile memory during normal operation.
 *
 * Parameters:  LOG_CMD_ERASE_PARMS *pTCB
 *
 * Returns:     None.
 *
 * Notes:       Function was created to reduce cyclomatic complexity of
 *              LogCmdEraseTask
 *
 *****************************************************************************/
static
void LogEraseStatusWrite (LOG_CMD_ERASE_PARMS *pTCB)
{
   if (FALSE == pTCB->bUpdatingStatusFile)
   {
      // First time here since entering this state? Update Log Erase Status file.
      pTCB->bUpdatingStatusFile = TRUE;
      // ( nOffset, nSize, bChipErase, bInProgress, bWriteNow )
      LogSaveEraseData (pTCB->nOffset, pTCB->nSize, FALSE, TRUE, FALSE);
   }  
   else // Update was issued in previous frame.
   {
      // Checking if write completed. If so, change to the LOG_ERASE_MEMORY state.
      if ( SYS_NV_WRITE_PENDING != NV_WriteState(NV_LOG_ERASE, 0, 0) )
      {
         //GSE_DebugStr(NORMAL,TRUE,"LogErase Status has been written to EEPROM");
         pTCB->bUpdatingStatusFile = FALSE;
         pTCB->state = LOG_ERASE_MEMORY;
      }
   }
}

/******************************************************************************
 * Function:    LogMonEraseTask
 *
 * Description: The Monitor Erase Task is used when a user commanded erase has
 *              been issued and an erase is now in progress.
 *              It will continue to monitor the erase and  provide user feedback
 *              until completion.
 *
 * Parameters:  void *pParam - Pointer to the TCB
 *
 * Returns:     None.
 *
 * Notes:       None.
 *
 *****************************************************************************/
static
void LogMonEraseTask ( void *pParam )
{
   // Local Data
   LOG_MON_ERASE_PARMS *pTCB;

   // Initialize Local Data
   pTCB = (LOG_MON_ERASE_PARMS*)pParam;

   if (LOG_REQ_COMPLETE == pTCB->erStatus)
   {
      // Notify the user the erase has completed
      GSE_DebugStr(NORMAL, TRUE, "LogMonEraseTask: Erase Complete");
      pTCB->erStatus   = LOG_REQ_NULL;
      // Kill Self
      TmTaskEnable ((TASK_INDEX)Log_Monitor_Erase_Task, FALSE);
   }
   else if (LOG_REQ_FAILED  == pTCB->erStatus)
   {
      // Notify the user the erase has failed
      GSE_DebugStr(NORMAL,TRUE, "LogMonEraseTask: Erase Failed");
      pTCB->erStatus   = LOG_REQ_NULL;
      // Kill Self
      TmTaskEnable ((TASK_INDEX)Log_Monitor_Erase_Task, FALSE);
   }
   else // Print an Erase in Progress indication
   {
      GSE_StatusStr(NORMAL,".");
   }
}


/******************************************************************************
 * Function:    LogSystemLogManageTask
 *
 * Description: This task is kicked off and monitors the status of System
 *              Log writes so that the calling function can continue normal
 *              processing.
 *
 * Parameters:  void *pParam
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static
void LogSystemLogManageTask ( void *pParam )
{
   // Local Data
   BOOLEAN        bAllDone;
   UINT32         i;
   BOOLEAN        bETM_InProgress;

   // Initialize Local Data
   bAllDone        = TRUE;
   bETM_InProgress = FALSE;

   // Loop through the entire table
   for ( i = 0; i < SYSTEM_TABLE_MAX_SIZE; i++)
   {
      if (LOG_REQ_COMPLETE == systemTable[i].wrStatus)
      {
         systemTable[i].wrStatus = LOG_REQ_NULL;
      }
      else if ((LOG_REQ_PENDING == systemTable[i].wrStatus) ||
               (LOG_REQ_INPROG  == systemTable[i].wrStatus))
      {
         bAllDone = FALSE;
         // Check if this is an ETM Log in progress
         if ( systemTable[i].logType == LOG_TYPE_ETM )
         {
            bETM_InProgress = TRUE;
         }
      }
      else if (LOG_REQ_FAILED == systemTable[i].wrStatus)
      {
         systemTable[i].wrStatus = LOG_REQ_NULL;
         GSE_DebugStr(NORMAL,TRUE, "SystemLogMngTask: Write Failed");
      }

      //Update Log ETM recording busy/not busy status on change only.
      // Since the busy flag is set in LogManageWrite this logic should
      // only reset the busy flag when there are no pending ETM Writes.
      if(bETM_InProgress != is_busy)
      {
         is_busy = bETM_InProgress;
         if(m_event_func != NULL)
         {
            m_event_func(m_event_tag,is_busy);
         }
      }
   }

   if (TRUE == bAllDone)
   {
      TmTaskEnable ((TASK_INDEX)System_Log_Manage_Task, FALSE);
   }
}

/******************************************************************************
 * Function:    LogSaveEraseData
 *
 * Description: The Log Save Erase Data function is used to store information
 *              to the EEPROM when an erase is to be commanded. This information
 *              can then be checked during each power cycle to determine if
 *              there was an erase in progress during power down.
 *
 * Parameters:  UINT32  nOffset      - Starting location of the erase
 *              UINT32  nSize        - Size of the sections being erased
 *              BOOLEAN bChipErase   - Flag to identify sector vs chip erase
 *              BOOLEAN bInProgress  - Flag to determine if in progress
 *              BOOLEAN bWriteNow    - Write the Erase Data immediately
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static void LogSaveEraseData (UINT32 nOffset, UINT32 nSize,
                       BOOLEAN bChipErase, BOOLEAN bInProgress, BOOLEAN bWriteNow)
{
   // Local Data
   LOG_EEPROM_CONFIG eraseData;
   RESULT resultNV;

   // Copy parameters to local structure
   eraseData.inProgress       = bInProgress;
   eraseData.bChipErase       = bChipErase;
   eraseData.savedStartOffset = nOffset;
   eraseData.savedSize        = nSize;

   // Update the EEPROM
   memcpy(&LogEraseData, &eraseData, sizeof(eraseData));

   if (TRUE == bWriteNow)
   {
      resultNV = NV_WriteNow(NV_LOG_ERASE, 0, &LogEraseData, sizeof(LogEraseData));
      GSE_DebugStr(NORMAL,TRUE, "LogSaveEraseData - NV_LOG_ERASE save...%s",
                   SYS_OK == resultNV ? "SUCCESS":"FAILED");
   }
   else
   {
      NV_Write(NV_LOG_ERASE, 0, &LogEraseData, sizeof(eraseData));
   }
}


/******************************************************************************
 * Function:    LogCheckEraseInProgress
 *
 * Description: The Log Erase in Progress function will check if a previous
 *              erase sequence was started but never completed. This function
 *              should be initiated each time the Log Manager Initialization
 *              is performed.
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:       This is a blocking function and thus should only be called
 *              during initialization. This function also depends on the
 *              MemEraseTask being initialized and available to use.
 *
 *              This function must service the watchdog or the system will
 *              reset.
 *
 *****************************************************************************/
static
void LogCheckEraseInProgress ( void )
{
   // Local Data
   LOG_EEPROM_CONFIG    eraseData;
   BOOLEAN              bDone;
   RESULT               result;
   UINT32               startup_Id;

   startup_Id = LOG_ERASE_IN_PROGRESS_STARTUP_ID;

   // Watchdog - Smack the DOG
   STARTUP_ID(startup_Id++);

   // Retrieve Erase Data from EEPROM and RTC RAM
   // Perform Erase verification logic.
   memcpy( &eraseData, &LogEraseData, sizeof(eraseData) );

   // Check there was a previous erase that did not complete
   if (TRUE == eraseData.inProgress)
   {
      GSE_DebugStr(NORMAL,TRUE,"LogCheckEraseInProgress: Previous Erase Resume");

      if (TRUE == eraseData.bChipErase)
      {
         result = MemChipErase();
      }
      else
      {
         // Initialize the minute counter
         UTIL_MinuteTimerDisplay ( TRUE );

         MemSetupErase (MEM_BLOCK_LOG, eraseData.savedStartOffset, &bDone);
         MemCommandErase (MEM_BLOCK_LOG, eraseData.savedStartOffset,
                          eraseData.savedSize, &bDone);

         bDone = FALSE;
         // Keep looping until memory erase status is no longer in progress
         while (FALSE == bDone)
         {
            // Watchdog - Smack the DOG
            STARTUP_ID(startup_Id++);

            result = MemStatusErase (MEM_BLOCK_LOG, eraseData.savedStartOffset,
                                     eraseData.savedSize, &bDone);

            // Keep checking the minute timer, this will place a
            // status on the screen so that the user knows the flash
            // is being erased.
            UTIL_MinuteTimerDisplay ( FALSE );
         }
      }

      // Check if the erase has completed with an error
      if (SYS_OK != result)
      {
         FATAL ( "LogCheckEraseInProgress: Erase Resume FAIL. ResultCode: 0x%08X", result );
      }
      else // Erase was successful
      {
         // Update the EEPROM
         // ( nOffset, nSize, bChipErase, bInProgress, bWriteNow )
         LogSaveEraseData (0, 0, FALSE, FALSE, TRUE);
         GSE_DebugStr(NORMAL,TRUE, "LogCheckEraseInProgress: Previous Erase Completed");
      }
   }
}


/******************************************************************************
 * Function:    LogFindNextWriteOffset
 *
 * Description: The Log Find Next Write Offset function is called during
 *              initialization and will search the memory for the next
 *              free location in memory
 *
 * Parameters:  UINT32 pOffset  - Offset to start searching from
 *              RESULT *Result  - Storage location to return any errors
 *
 * Returns:     BOOLEAN bValidFound - [TRUE - if at least one log was found,
 *                                     FALSE - if no logs were found ]
 *
 * Notes:       None.
 *
 *****************************************************************************/
static
BOOLEAN LogFindNextWriteOffset (UINT32 *pOffset, RESULT *Result)
{
   // Local Data
   LOG_HEADER     hdr;
   LOG_HDR_STATUS hdrStatus;
   BOOLEAN        bDone;
   BOOLEAN        bCorruptFound;
   BOOLEAN        bTimerStarted;
   BOOLEAN        bValidFound;
   UINT32         nStartTime;
   UINT32         previousOffset;
   LOG_SOURCE     source;
   UINT32         startup_Id = LOG_FIND_NEXT_WRITE_STARTUP_ID;

   // Initialize Local Data
   bCorruptFound = FALSE;
   bTimerStarted = FALSE;
   bValidFound   = FALSE;

   // Check for the end of the block
   while ((*pOffset + sizeof(LOG_HEADER)) < LogConfig.nEndOffset )
   {
      // Watchdog - Smack the DOG
      STARTUP_ID(startup_Id++);

      // read the log at the current read offset
      bDone = MemRead(MEM_BLOCK_LOG, pOffset, (UINT32 *)&hdr,
                      sizeof(LOG_HEADER), Result);

      // Was the read successful?
      if (TRUE == (BOOLEAN)STPU( bDone, eTpLogCheck))
      {
         // Use a temporary location because the read will point
         // to a location after the free location. Using the
         // temp and then returning the nOffset will return
         // the correct location.
         previousOffset = *pOffset - sizeof(LOG_HEADER);

         hdrStatus = STPU( LogCheckHeader(hdr), eTpLogCheck1);

         // Is the header valid?
         if (LOG_HDR_FOUND == hdrStatus)
         {
            // The header is valid so the corrupt flag can be cleared
            bCorruptFound  = FALSE;
            source.nNumber = LOG_SOURCE_DONT_CARE;

            // Now determine the state of the log found and count it
            if ( TRUE == LogHdrFilterMatch(hdr,
                                           LOG_NEW,
                                           LOG_TYPE_DONT_CARE,
                                           source,
                                           LOG_PRIORITY_DONT_CARE) )
            {
               LogConfig.nLogCount++;
            }
            else if ( TRUE == LogHdrFilterMatch(hdr,
                                                LOG_DELETED,
                                                LOG_TYPE_DONT_CARE,
                                                source,
                                                LOG_PRIORITY_DONT_CARE) )
            {
               LogConfig.nDeleted++;
            }

            // Move to the next log
            *pOffset     += MEM_ALIGN_SIZE(hdr.nSize);
            bValidFound   = TRUE;
         }
         // Is the header free?
         else if (LOG_HDR_FREE == hdrStatus)
         {
            // Before the Free Header is found:
            // 1. No Valid Record Found and no Corruption Found then
            //    the first location must be free
            // 2. No Valid Record Found and a corruption found then
            //    the memory is severely corrupted erase the flash
            // 3. A valid record was found and no corruption was found
            //    then the free found is the new record starting point
            // 4. A Valid record was found and a corruption was found
            //    the new record should be at the found free location.
            if (!((FALSE == bValidFound) && (TRUE == bCorruptFound)))
            {
               // Found the next location to write exit loop
               bValidFound = TRUE;
               *pOffset    = previousOffset;
            }
            break;
         }
         else // Corruption
         {
            // Is Corrupt Flag Clear?
            if (FALSE == bCorruptFound)
            {
               // Set the corrupt flag
               bCorruptFound  = TRUE;
               // Increment the corrupt count
               LogConfig.nCorrupt++;
               LogHealthCounts.nCorruptCnt++;

               GSE_DebugStr(NORMAL,TRUE, "LogFindNextWriteOffset: Corruption at 0x%x (%d)",
                        previousOffset, LogConfig.nCorrupt );
            }
            else // Already working on this corrupted area, advance by
                 // location or then by sector when the boundary is
                 // is reached.
            {
               // Check if on a sector boundary
               if ((previousOffset % MemGetSectorSize()) == 0)
               {
                  // Advance to the next sector
                  *pOffset = previousOffset + MemGetSectorSize();
               }
               else // Not on a boundary continue searching sector
               {
                  // Increment the offset by location
                  *pOffset = previousOffset + sizeof(UINT32);
               }
            }
         }
         // The read was successful so reset the timer flag
         bTimerStarted = FALSE;
      }
      else // The memory manager is busy or the read failed
      {
         // Check if a timer has already been started
         if (FALSE == bTimerStarted)
         {
            // No timer started so get the current tick count and
            // set the timer started flag to TRUE
            nStartTime     = TTMR_GetHSTickCount();
            bTimerStarted  = TRUE;
         }

         // Check if the Timer has exceeded the 1 second timeout
         if ((TTMR_GetHSTickCount() - nStartTime) > (1 * TICKS_PER_Sec))
         {
            *Result |= SYS_LOG_FIND_NEXT_WRITE_FAIL;

            GSE_DebugStr(NORMAL,TRUE,
                     "LogFindNextWriteOffset: Timeout. ResultCode: 0x%08X", *Result);
            // Exit the loop the Find Next has failed
            break;
         }
      }
   }

   // return the found status
   return (bValidFound);
}

/******************************************************************************
 * Function:    LogCheckHeader
 *
 * Description: The Log Check Header function will verify if the passed in
 *              header is valid. This check is done by checking the size and
 *              inverse size and the header state.
 *
 * Parameters:  LOG_HEADER Hdr - Header to verify
 *
 * Returns:     LOG_HDR_STATUS - Validity of the header
 *                               [LOG_HDR_FOUND, LOG_HDR_FREE, LOG_HDR_NOT_FOUND]
 *
 * Notes:       None.
 *
 *****************************************************************************/
static
LOG_HDR_STATUS LogCheckHeader (LOG_HEADER Hdr)
{
   // Local Data
   LOG_HDR_STATUS status = LOG_HDR_NOT_FOUND;

   // XOR the size with the Size Inverse and make sure the size is valid
   // And Verify the header contains a valid state
   if ( (LOG_SIZE_CHECK_OK == (Hdr.nSize ^ Hdr.nSizeInverse)) &&
        ((LOG_DELETED == Hdr.nState) || (LOG_NEW == Hdr.nState)) )
   {
     // Move to the next record
     status = LOG_HDR_FOUND;
   }
   // Check if the next header is actually the end of the records
   else if ((LOG_SIZE_BLANK == Hdr.nSize)        &&
            (LOG_SIZE_BLANK == Hdr.nSizeInverse) &&
            (LOG_NONE == Hdr.nState) && (LOG_CHKSUM_BLANK == Hdr.nChecksum))
   {
      // At the end of all stored logs
      status = LOG_HDR_FREE;
   }

   return (LOG_HDR_STATUS)TPU( status, eTpLogCheck);
}

/******************************************************************************
 * Function:    LogHdrFilterMatch
 *
 * Description: The Log Header Filter Match function will compare the
 *              passed in header against the passed in parameters. If the
 *              header matches the function will return TRUE. If the header
 *              does not match the function will return FALSE.
 *
 * Parameters:  LOG_HEADER    Hdr       - Header to check
 *              LOG_STATE     State     - Requested state to find
 *              LOG_TYPE      Type      - Requested type to find
 *              LOG_SOURCE    Source    - Requested ACS to find
 *              LOG_PRIORITY  Priority  - Requested Priority to find
 *
 * Returns:     BOOLEAN       bFound    - [TRUE - The header matches,
 *                                         FALSE - The header does not match]
 *
 * Notes:       None.
 *
 *****************************************************************************/
static
BOOLEAN LogHdrFilterMatch(LOG_HEADER Hdr, LOG_STATE State, LOG_TYPE Type,
                          LOG_SOURCE Source, LOG_PRIORITY Priority)
{
   // Local Data
   BOOLEAN bFound;

   // Initialize Local Data
   bFound = FALSE;

   // Check the State
   if ((Hdr.nState == State) || (LOG_DONT_CARE == State))
   {
      // Check the log priority
      if ( (Hdr.nPriority == (UINT16)Priority) ||
           (LOG_PRIORITY_DONT_CARE == Priority) )
      {
         // Check the Type
         if ((Hdr.nType == (UINT16)Type) || (LOG_TYPE_DONT_CARE == Type))
         {
            // Check the ACS
            if ((Hdr.nSource == Source.nNumber) ||
                (LOG_SOURCE_DONT_CARE == Source.nNumber))
            {
               // One of the requested logs has been found
               // Set the Found Offset and Mark the status as found
               bFound = TRUE;
            }
         }
      }
   }

   // return the found status
   return (bFound);
}

/*****************************************************************************
 * Function:    LogVerifyAllDeleted
 *
 * Description: The Log Verify all deleted will check from the beginning
 *              of the Log block to the end of the block and verify
 *              that all the logs in the flash have been marked for
 *              deletion.
 *
 * Parameters:  UINT32 pRdOffset     - location to start the search
 *              UINT32 nEndOffset    - location to end the search
 *              UINT32 *pFound       - Storage location to store the offset
 *                                     of any log that has not been marked
 *                                     for deletion.
 *
 * Returns:     BOOLEAN bDone        - [TRUE - if the function completed
 *                                             successfully
 *                                      FALSE - if the function returned
 *                                              because the read could not be
 *                                              performed.
 *
 * Notes:       The calling function should pass in the LOG_NEXT for the
 *              found offset and then monitor the Found Offset after the
 *              function returns.
 *
 *              This is a blocking function and could possibly take multiple
 *              frames to complete. Only use within an RMT context.
 *
 ****************************************************************************/
static
BOOLEAN LogVerifyAllDeleted ( UINT32 *pRdOffset, UINT32 nEndOffset, UINT32 *pFound )
{
   // Local Data
   BOOLEAN         bComplete;
   UINT32          nSize;
   LOG_SOURCE      source;
   LOG_FIND_STATUS findStatus;

   // Initialize Local Data
   source.nNumber = LOG_SOURCE_DONT_CARE;
   bComplete      = FALSE;

   // Search the memory for any logs marked as new.
   // Note: this is a blocking call and will not return until either
   //       a new log is found, the end offset is reached or the end of
   //       memory is reached. Make sure this function is only used inside
   //       the context of an RMT.
   findStatus = LogFindNextRecord (LOG_NEW,
                                   LOG_TYPE_DONT_CARE,
                                   source,
                                   LOG_PRIORITY_DONT_CARE,
                                   pRdOffset,
                                   nEndOffset,
                                   pFound,
                                   &nSize);

   // If the find status does not return busy then the search has completed
   if (LOG_BUSY != findStatus)
   {
     bComplete = TRUE;
   }

   return bComplete;
}


/*****************************************************************************
 * Function:    LogQueueInit
 *
 * Description: Initializes the Head and Tail Pointers for the Queue.
 *
 * Parameters:  void
 *
 * Returns:     void
 *
 * Notes:       None.
 *
 ****************************************************************************/
static
void LogQueueInit(void)
{
    ASSERT((LOG_QUEUE_SIZE & (LOG_QUEUE_SIZE - 1)) == 0);

    // Initializes the Log Manager Queue
    LogQueue.head = 0;
    LogQueue.tail = 0;
    memset(LogQueue.buffer, 0, sizeof(LogQueue.buffer));
}

/*****************************************************************************
 * Function:    LogQueueGet
 *
 * Description: Function to get an Entry from the Log Queue. If the
 *              Queue is empty then the function returns EMPTY, else the
 *              the function removes the oldest entry from the Queue and
 *              increments the Tail.
 *
 * Parameters:  LOG_REQUEST Entry  [In:Out]
 *
 * Returns:     LOG_QUEUE_STATUS Status [LOG_QUEUE_OK, LOG_QUEUE_EMPTY]
 *
 * Notes:       None.
 *
 ****************************************************************************/
static
LOG_QUEUE_STATUS LogQueueGet(LOG_REQUEST *Entry)
{
   // Local Data
   INT32 intLevel;
   LOG_REQ_QUEUE    *pQueue;
   LOG_QUEUE_STATUS status;

   // Initialize Local Data
   pQueue = &LogQueue;
   status = LOG_QUEUE_OK;

   intLevel = __DIR();

   if ((pQueue->head - pQueue->tail) == 0)
   {
      status = LOG_QUEUE_EMPTY;
   }
   else
   {
      *Entry = pQueue->buffer[pQueue->tail & (LOG_QUEUE_SIZE - 1)];
      pQueue->tail++;
   }

   __RIR( intLevel);

   return (status);
}

/******************************************************************************
 * Function:      LogManageWrite
 *
 * Description:   The LogManageWrite function is responsible for writing
 *                and monitoring the status of system and ETM logs. This function
 *                allows the calling functions to continue operating instead
 *                of monitoring the status of the log write.
 *
 * Parameters:    SYS_APP_ID    logID     - System Log ID to write
 *                LOG_PRIORITY  priority  - Priority of the Log
 *                void          *pData    - Pointer to the data location
 *                UINT32        nSize     - Size of the data to write
 *                TIMESTAMP     *pTs      - Timestamp to use if passed in
 *                LOG_TYPE      logType   - Type of log to write (ETM or SYSTEM)
 *
 * Returns:       UINT32 index value in SystemTable where the log is stored.
 *                This can be used by caller to track the commit-status
 *                of the Write.
 *
 * Notes:         The nSize value cannot be larger than 1Kbyte.
 *
 *****************************************************************************/
static
UINT32 LogManageWrite ( SYS_APP_ID logID, LOG_PRIORITY priority,
                        void *pData, UINT16 nSize, const TIMESTAMP *pTs, LOG_TYPE logType )
{
   // Local Data
   SYSTEM_LOG sysLog;
   LOG_SOURCE source;
   TIMESTAMP  ts;
   UINT32     i = 0;
   INT32      intLevel;
   BOOLEAN    bSlotFound;

   ASSERT(nSize <= LOG_SYSTEM_ETM_MAX_SIZE);

   if ( SystemLogLimitCheck(logID) )
   {
     // Get current ts or use passed in ts
     if ( pTs == NULL )
     {
        // Get current ts if one is not provided.
        CM_GetTimeAsTimestamp(&ts);
     }
     else
     {
        // Use passed in ts
        ts = *pTs;
     }

     memset(&sysLog,0,sizeof(sysLog));

     sysLog.logType                = logType;
     sysLog.source                 = logID;
     sysLog.priority               = priority;
     sysLog.payload.hdr.ts         = ts;
     sysLog.payload.hdr.nID        = (UINT16)logID;
     sysLog.payload.hdr.nSize      = nSize;

     if (nSize > 0)
     {
        memcpy (&sysLog.payload.data[0], pData, nSize);
     }

     sysLog.payload.hdr.nChecksum  = ChecksumBuffer(&sysLog.payload,
                                            nSize + sizeof(sysLog.payload.hdr),
                                            0xFFFFFFFF);
     bSlotFound = FALSE;

     for (i = 0; (i < SYSTEM_TABLE_MAX_SIZE) && (bSlotFound == FALSE); i++)
     {
        // Protect against trying to write multiple logs to the same entry
        intLevel = __DIR();
        if (LOG_REQ_NULL == systemTable[i].wrStatus)
        {
           systemTable[i] = sysLog;

           source.id = sysLog.source;
           LogWrite(logType,
                    source,
                    systemTable[i].priority,
                    &systemTable[i].payload,
                    systemTable[i].payload.hdr.nSize +
                      sizeof(systemTable[i].payload.hdr),
                    (sysLog.payload.hdr.nChecksum +
                     ChecksumBuffer(&sysLog.payload.hdr.nChecksum,
                     sizeof(UINT32), LOG_CHKSUM_BLANK)),
                    &systemTable[i].wrStatus);

           // Enable the RMT Task
           if ( LogSystemInit )
           {
               TmTaskEnable ((TASK_INDEX)System_Log_Manage_Task, TRUE);
           }
           bSlotFound = TRUE;
        }
        __RIR( intLevel);
        // there is no else statement because the table should
        // never fill up because it is sized 4 bigger than the
        // log queue. Since the log queue pops out the oldest when
        // the queue fills we should always have room.

        //Update Log ETM recording busy status
        // Updates the flag if an ETM Log is requested to be written and the
        // flag is not already set.
        if((LOG_TYPE_ETM == sysLog.logType) && (TRUE == bSlotFound) && (FALSE == is_busy))
        {
           is_busy = bSlotFound;
           if(m_event_func != NULL)
           {
              m_event_func(m_event_tag,is_busy);
           }
        }
     }
   }

   // Return the SystemTable index in case the caller needs to track the status.
   return i;
}

/*****************************************************************************
 * Function:    LogRegisterIndex
 *
 * Description: Function called by a logger to register the passed SystemTable
 *              index containing the location of an end of flight log.
 *              LogMngTask will monitor the status of the write at this index
 *              until it is complete.

 *
 * Parameters:  regType is the type of log being written LOG_REGISTER_TYPE.
 *
 *              SystemTableIndex is the UINT32 index in SystemTable where the
 *              pending log entry is stored
 *
 * Returns:     None
 *
 * Notes:        The idx param to monitor is returned by a previous call to
 *               LogWriteSystem.
 *               After calling this function. Callers can check on the status of the
 *               write by calling LogIsEofWriteComplete()
 *
 ****************************************************************************/
void LogRegisterIndex ( LOG_REGISTER_TYPE regType, UINT32 SystemTableIndex)
{
  ASSERT( SystemTableIndex < SYSTEM_TABLE_MAX_SIZE);

  LogMngBlock.logWritePending[regType] = SystemTableIndex;
  GSE_DebugStr(NORMAL,TRUE,"LogMgr: Log Type: %d registered SystemTable index registered: %u",
               SystemTableIndex);
}


/*****************************************************************************
 * Function:    LogUpdateWritePendingStatuses
 *
 * Description: Function called by the task to update the statuses of any
 *              outstanding registered log writes . This allows the LogManager
 *              to pro-actively manage the state and reset it when it becomes complete
 *              eliminating any chance of the entry in the table being re-used
 *              before an observer checks its status.
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:
 *
****************************************************************************/
static
void LogUpdateWritePendingStatuses( void )
{
  INT32 i;
  // For each type of registered log, see if it is busy
  for ( i = 0; i < (INT32)LOG_REGISTER_END; ++i)
  {
    if (LOG_INDEX_NOT_SET != LogMngBlock.logWritePending[i] )
    {
      if ( LOG_REQ_PENDING == systemTable[ LogMngBlock.logWritePending[i] ].wrStatus ||
       LOG_REQ_INPROG  == systemTable[ LogMngBlock.logWritePending[i] ].wrStatus )
      {
       // Still busy... do nothing.
      }
      else
      {
        // The write has finished. Show this by flagging the index as not used.
        LogMngBlock.logWritePending[i] = LOG_INDEX_NOT_SET;
        GSE_DebugStr(NORMAL,TRUE,"LogMgr: LOG_REGISTER_TYPE: %d - Write completed",i);
      }
    }// i is in-use
  }  // for i
}


/*************************************************************************
 *  MODIFICATIONS
 *    $History: LogManager.c $
 *
 * *****************  Version 106  *****************
 * User: John Omalley Date: 12-11-13   Time: 5:46p
 * Updated in $/software/control processor/code/system
 * SCR 1197 - Code Review Updates
 *
 * *****************  Version 105  *****************
 * User: John Omalley Date: 12-11-12   Time: 1:11p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review Update
 * 
 * *****************  Version 104  *****************
 * User: John Omalley Date: 12-11-12   Time: 9:48a
 * Updated in $/software/control processor/code/system
 * SCR 1105, 1107, 1131, 1154 - Code Review Updates
 *
 * *****************  Version 103  *****************
 * User: John Omalley Date: 12-11-12   Time: 8:36a
 * Updated in $/software/control processor/code/system
 * SCR 1105, 1107, 1131, 1154 - Code Review Updates
 *
 * *****************  Version 102  *****************
 * User: John Omalley Date: 12-11-08   Time: 3:03p
 * Updated in $/software/control processor/code/system
 * SCR 1131 - Added logic for ETM Log Write Busy
 *                    updated Code Review findings
 *
 * *****************  Version 101  *****************
 * User: John Omalley Date: 12-11-08   Time: 8:40a
 * Updated in $/software/control processor/code/system
 * SCR 1154 - miss the interrupt issue by that much...
 *
 * *****************  Version 100  *****************
 * User: John Omalley Date: 12-11-07   Time: 8:29a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review Updates
 * SCR 1154 - Fixed interrupt suseptability issue in LogManageWrite
 *
 * *****************  Version 99  *****************
 * User: John Omalley Date: 12-11-06   Time: 11:19a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review Updates
 *
 * *****************  Version 98  *****************
 * User: John Omalley Date: 12-09-05   Time: 9:42a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Added ASSERT for ETM and System Logs larger than 1K
 *
 * *****************  Version 97  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 96  *****************
 * User: Jim Mood     Date: 7/19/12    Time: 11:08a
 * Updated in $/software/control processor/code/system
 * SCR 1107: Data Offload changes for 2.0.0
 *
 * *****************  Version 95  *****************
 * User: John Omalley Date: 12-05-24   Time: 9:38a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Added the ETM Write function
 *
 * *****************  Version 94  *****************
 * User: John Omalley Date: 4/24/12    Time: 12:01p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Cessna Caravan
 *
 * *****************  Version 93  *****************
 * User: Jim Mood     Date: 2/24/12    Time: 10:39a
 * Updated in $/software/control processor/code/system
 * SCR 1114 - Re labeled after v1.1.1 release
 *
 * *****************  Version 91  *****************
 * User: Contractor V&v Date: 12/14/11   Time: 6:50p
 * Updated in $/software/control processor/code/system
 * SCR #1105 End of Flight Log Race Condition
 *
 * *****************  Version 90  *****************
 * User: Contractor2  Date: 6/01/11    Time: 1:45p
 * Updated in $/software/control processor/code/system
 * SCR #473 Enhancement: Restriction of System Logs
 *
 * *****************  Version 89  *****************
 * User: John Omalley Date: 11/16/10   Time: 3:32p
 * Updated in $/software/control processor/code/system
 * SCR 996 - Log Queue Updates
 *
 * *****************  Version 88  *****************
 * User: Jeff Vahue   Date: 11/11/10   Time: 9:55p
 * Updated in $/software/control processor/code/system
 * SCR# 848 - Code Coverage
 *
 * *****************  Version 87  *****************
 * User: Jeff Vahue   Date: 11/10/10   Time: 11:04p
 * Updated in $/software/control processor/code/system
 * SCR# 848 - Code Coverage
 *
 * *****************  Version 86  *****************
 * User: Jeff Vahue   Date: 11/09/10   Time: 9:08p
 * Updated in $/software/control processor/code/system
 * SCR# 848 - Code Coverage
 *
 * *****************  Version 85  *****************
 * User: John Omalley Date: 11/09/10   Time: 2:25p
 * Updated in $/software/control processor/code/system
 * SCR 990 - Dead Code Removal
 *
 * *****************  Version 84  *****************
 * User: John Omalley Date: 11/04/10   Time: 7:10p
 * Updated in $/software/control processor/code/system
 * SCR 983 - Missed one error path
 *
 * *****************  Version 83  *****************
 * User: John Omalley Date: 11/04/10   Time: 5:11p
 * Updated in $/software/control processor/code/system
 * SCR 983 - Clear the Log Erase Status when the erase fails
 *
 * *****************  Version 82  *****************
 * User: Jeff Vahue   Date: 11/01/10   Time: 9:17p
 * Updated in $/software/control processor/code/system
 * SCR# 848 - Code Coverage
 *
 * *****************  Version 81  *****************
 * User: Jeff Vahue   Date: 11/01/10   Time: 8:38p
 * Updated in $/software/control processor/code/system
 * SCR# 848 - Code Coverage
 *
 * *****************  Version 80  *****************
 * User: Jeff Vahue   Date: 11/01/10   Time: 7:50p
 * Updated in $/software/control processor/code/system
 * SCR# 975 - Code Coverage refactor
 *
 * *****************  Version 79  *****************
 * User: Jeff Vahue   Date: 11/01/10   Time: 4:22p
 * Updated in $/software/control processor/code/system
 * SCR# 974 - Log Full Removal
 * SCR# 848 - Test Point
 *
 * *****************  Version 78  *****************
 * User: John Omalley Date: 10/25/10   Time: 6:10p
 * Updated in $/software/control processor/code/system
 * SCR 964 - Check for Log Write and Pause before marking logs for
 * deletion
 *
 * *****************  Version 77  *****************
 * User: John Omalley Date: 10/25/10   Time: 6:03p
 * Updated in $/software/control processor/code/system
 * SCR 961 - Code Review Updates
 *
 * *****************  Version 76  *****************
 * User: John Omalley Date: 10/21/10   Time: 1:46p
 * Updated in $/software/control processor/code/system
 * SCR 960 - Modified the checksum to load it across frames
 *
 * *****************  Version 75  *****************
 * User: Jim Mood     Date: 10/21/10   Time: 9:22a
 * Updated in $/software/control processor/code/system
 * SCR 959 Upload Manager code coverage changes
 *
 * *****************  Version 74  *****************
 * User: Jeff Vahue   Date: 10/19/10   Time: 7:25p
 * Updated in $/software/control processor/code/system
 * SCR# 848 - Code Coverage
 *
 * *****************  Version 73  *****************
 * User: John Omalley Date: 9/30/10    Time: 1:25p
 * Updated in $/software/control processor/code/system
 * SCR 894 - LogManager Code Coverage and Code Review Updates
 *
 * *****************  Version 72  *****************
 * User: Jeff Vahue   Date: 9/21/10    Time: 5:48p
 * Updated in $/software/control processor/code/system
 * SCR #848 - Code Cov
 *
 * *****************  Version 71  *****************
 * User: John Omalley Date: 9/09/10    Time: 9:00a
 * Updated in $/software/control processor/code/system
 * SCR 790 - Added function to return the last used address
 *
 * *****************  Version 70  *****************
 * User: Jeff Vahue   Date: 9/03/10    Time: 1:48p
 * Updated in $/software/control processor/code/system
 * SCR# 848 - Code Covergae Mod
 *
 * *****************  Version 69  *****************
 * User: John Omalley Date: 8/31/10    Time: 8:47a
 * Updated in $/software/control processor/code/system
 * SCR 826 - Add a write now feature to the LogSaveErase function
 *
 * *****************  Version 68  *****************
 * User: John Omalley Date: 8/12/10    Time: 7:53p
 * Updated in $/software/control processor/code/system
 * SCR 787 - Un-did last change
 *
 * *****************  Version 67  *****************
 * User: John Omalley Date: 8/11/10    Time: 5:21p
 * Updated in $/software/control processor/code/system
 * SCR 787
 *
 * *****************  Version 66  *****************
 * User: John Omalley Date: 8/11/10    Time: 3:08p
 * Updated in $/software/control processor/code/system
 * SCR 787 - Check the size before looking for the next offset.
 *
 * *****************  Version 65  *****************
 * User: John Omalley Date: 7/30/10    Time: 5:03p
 * Updated in $/software/control processor/code/system
 * SCR 699 - Changed the LogInitialize return value to void.
 *
 * *****************  Version 64  *****************
 * User: Peter Lee    Date: 7/30/10    Time: 4:42p
 * Updated in $/software/control processor/code/system
 * SCR #759 Error: How to properly use "*pPtr++".
 *
 * *****************  Version 63  *****************
 * User: John Omalley Date: 7/30/10    Time: 8:51a
 * Updated in $/software/control processor/code/system
 * SCR 678 - Remove files in the FVT that are marked not deleted
 *
 * *****************  Version 62  *****************
 * User: Jeff Vahue   Date: 7/29/10    Time: 4:06p
 * Updated in $/software/control processor/code/system
 * SCR# 698 - cleanup
 *
 * *****************  Version 61  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:10a
 * Updated in $/software/control processor/code/system
 * SCR #698 - Fix code review findings
 *
 * *****************  Version 60  *****************
 * User: John Omalley Date: 7/27/10    Time: 4:42p
 * Updated in $/software/control processor/code/system
 * SCR 730 - Removed Log Write Failed Log
 *
 * *****************  Version 59  *****************
 * User: John Omalley Date: 7/26/10    Time: 10:36a
 * Updated in $/software/control processor/code/system
 * SCR 306
 * - Added the ASSERT for 3 writes
 * - Fixed Uninitialized Erase Request logic
 *
 * *****************  Version 58  *****************
 * User: John Omalley Date: 7/23/10    Time: 10:15a
 * Updated in $/software/control processor/code/system
 * SCR 306 / SCR 639 - Update the error paths, Added pause feature and
 * fixed the Queue ASSERT
 *
 * *****************  Version 57  *****************
 * User: Contractor V&v Date: 7/21/10    Time: 7:17p
 * Updated in $/software/control processor/code/system
 * SCR #260 Implement BOX status command
 *
 * *****************  Version 56  *****************
 * User: Jeff Vahue   Date: 7/19/10    Time: 6:35p
 * Updated in $/software/control processor/code/system
 * SCR# 707 - Code Coverage TP
 *
 * *****************  Version 55  *****************
 * User: Contractor2  Date: 7/13/10    Time: 10:56a
 * Updated in $/software/control processor/code/system
 * SCR #150 Implementation: SRS-3662 - Add Data Flash corruption CBIT
 * counts.
 * Corrected count calculation.
 *
 * *****************  Version 54  *****************
 * User: Contractor V&v Date: 7/07/10    Time: 6:19p
 * Updated in $/software/control processor/code/system
 * "SCR #531 Log Erase and NV_Write Timing
 * comment out debugStr "
 *
 * *****************  Version 53  *****************
 * User: Contractor3  Date: 7/06/10    Time: 10:38a
 * Updated in $/software/control processor/code/system
 * SCR #672 - Changes based on Code Review.
 *
 * *****************  Version 52  *****************
 * User: Contractor V&v Date: 6/30/10    Time: 3:18p
 * Updated in $/software/control processor/code/system
 * SCR #531 Log Erase and NV_Write Timing
 *
 * *****************  Version 51  *****************
 * User: Contractor2  Date: 6/30/10    Time: 11:59a
 * Updated in $/software/control processor/code/system
 * SCR #150 Implementation: SRS-3662 - Add Data Flash corruption CBIT
 * counts.
 * Corrected variable name.
 *
 * *****************  Version 50  *****************
 * User: Contractor2  Date: 6/30/10    Time: 11:04a
 * Updated in $/software/control processor/code/system
 * SCR #150 Implementation: SRS-3662 - Add Data Flash corruption CBIT
 * counts.
 *
 * *****************  Version 49  *****************
 * User: Contractor2  Date: 6/14/10    Time: 1:07p
 * Updated in $/software/control processor/code/system
 * SCR #483 Function Names must begin with the CSC it belongs with.
 *
 * *****************  Version 48  *****************
 * User: Contractor V&v Date: 5/12/10    Time: 3:48p
 * Updated in $/software/control processor/code/system
 * SCR #351 fast.reset=really housekeeping
 *
 * *****************  Version 47  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:55p
 * Updated in $/software/control processor/code/system
 * SCR #587 Change TmTaskCreate to return void
 *
 * *****************  Version 46  *****************
 * User: Contractor2  Date: 5/06/10    Time: 2:08p
 * Updated in $/software/control processor/code/system
 * SCR #579 Change LogWriteSys to return void
 *
 * *****************  Version 45  *****************
 * User: Jeff Vahue   Date: 4/28/10    Time: 5:51p
 * Updated in $/software/control processor/code/system
 * SCR #573 - Startup WD changes
 *
 * *****************  Version 44  *****************
 * User: Jeff Vahue   Date: 4/26/10    Time: 1:27p
 * Updated in $/software/control processor/code/system
 * SCR #566 - disable interrupts in Put/Get operations on Log Q.  Set
 * default fault.cfg.verbosity to OFF
 *
 * *****************  Version 43  *****************
 * User: Contractor V&v Date: 4/22/10    Time: 6:41p
 * Updated in $/software/control processor/code/system
 * SCR #140 Log erase status to own file SCR #306 Log Manager Erro
 *
 * *****************  Version 42  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:10p
 * Updated in $/software/control processor/code/system
 * SCR #317 Implement safe strncpy
 *
 * *****************  Version 41  *****************
 * User: Contractor V&v Date: 3/29/10    Time: 6:18p
 * Updated in $/software/control processor/code/system
 * SCR #140 Move Log erase status to own file
 *
 * *****************  Version 40  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:36p
 * Updated in $/software/control processor/code/system
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 *
 * *****************  Version 39  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/system
 * SCR# 483 - Function Names
 *
 * *****************  Version 38  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 37  *****************
 * User: Jeff Vahue   Date: 2/26/10    Time: 11:24a
 * Updated in $/software/control processor/code/system
 * SCR# 467 - move minute timer display to Utilities and rename TTMR to
 * UTIL in function name
 *
 * *****************  Version 36  *****************
 * User: Jeff Vahue   Date: 2/24/10    Time: 1:05p
 * Updated in $/software/control processor/code/system
 * SCR# 284 - Call to LogWriteSys early in startup sequence
 *
 * *****************  Version 35  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 5:07p
 * Updated in $/software/control processor/code/system
 * SCR# 397
 *
 * *****************  Version 34  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:25p
 * Updated in $/software/control processor/code/system
 * SCR 371
 *
 * *****************  Version 33  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 3:26p
 * Updated in $/software/control processor/code/system
 * SCR# 326 cleanup
 *
 * *****************  Version 32  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 2:11p
 * Updated in $/software/control processor/code/system
 * SCR# 326
 *
 * *****************  Version 31  *****************
 * User: Peter Lee    Date: 12/21/09   Time: 6:19p
 * Updated in $/software/control processor/code/system
 * SCR #380 Implement Real Time Sys Condition Logic
 *
 * *****************  Version 30  *****************
 * User: John Omalley Date: 12/16/09   Time: 3:17p
 * Updated in $/software/control processor/code/system
 * SCR 348
 * Go to FATAL when the System Log Table fills up.
 *
 * *****************  Version 29  *****************
 * User: Jeff Vahue   Date: 12/14/09   Time: 2:41p
 * Updated in $/software/control processor/code/system
 * SCR# 106, SCR# 364
 *
 * *****************  Version 28  *****************
 * User: John Omalley Date: 11/24/09   Time: 2:45p
 * Updated in $/software/control processor/code/system
 * SCR 348
 * * Printed a debug message when the log write system table is full.
 *
 * *****************  Version 27  *****************
 * User: Peter Lee    Date: 11/20/09   Time: 2:35p
 * Updated in $/software/control processor/code/system
 * SCR #345 Implement FPGA Watchdog Requirements
 *
 * *****************  Version 26  *****************
 * User: Contractor V&v Date: 11/18/09   Time: 4:11p
 * Updated in $/software/control processor/code/system
 * Implement SCR 172 ASSERT processing for all "default" case processing
 *
 * *****************  Version 25  *****************
 * User: Peter Lee    Date: 10/21/09   Time: 2:45p
 * Updated in $/software/control processor/code/system
 * SCR #312 Add msec value to logs
 *
 * *****************  Version 24  *****************
 * User: John Omalley Date: 10/19/09   Time: 3:29p
 * Updated in $/software/control processor/code/system
 * SCR 23
 * Added the 85% Full informational log
 *
 * *****************  Version 23  *****************
 * User: John Omalley Date: 8/06/09    Time: 3:55p
 * Updated in $/software/control processor/code/system
 * SCR 23
 *    Added 85% full and full logic.
 * SCR 179
 *    Created a wrapper to protect against memcpy with size 0
 * SCR 231
 *    Added logic to return complete to tasks using the log manager when
 * the memory becomes full.
 *    Fixed memory advancement past end of memory.
 *
 *
 *
 * *****************  Version 22  *****************
 * User: Peter Lee    Date: 7/06/09    Time: 5:51p
 * Updated in $/software/control processor/code/system
 * SCR #204 Misc Code Review Updates
 *
 * *****************  Version 21  *****************
 * User: Jim Mood     Date: 10/23/08   Time: 3:05p
 * Updated in $/control processor/code/system
 * Added a "pre-initialize" function that sets up the log write queue and
 * the system log buffer.  This is used during initialization to allow PBIT
 * log writes
 *
 * *****************  Version 20  *****************
 * User: Jim Mood     Date: 10/08/08   Time: 10:05a
 * Updated in $/control processor/code/system
 * SCR #87
 *
 * *****************  Version 19  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 3:13p
 * Updated in $/control processor/code/system
 * 1) SCR #87 Function Prototype
 * 2) Update call LogWriteSystem() to include NULL for ts
 *
 * *****************  Version 18  *****************
 * User: Jim Mood     Date: 10/07/08   Time: 2:54p
 * Updated in $/control processor/code/system
 * Changed LogWriteSystem prototype 'pData' parameter type from UINT32* to
 * void*
 *
 * *****************  Version 17  *****************
 * User: Jim Mood     Date: 8/26/08    Time: 11:29a
 * Updated in $/control processor/code/system
 * Moved LogWriteSystem function into this module from SystemLog.c
 *
 * *****************  Version 16  *****************
 * User: John Omalley Date: 6/18/08    Time: 2:46p
 * Updated in $/control processor/code/system
 * SCR-0044 - Add System Log Processing
 *
 * *****************  Version 15  *****************
 * User: John Omalley Date: 6/10/08    Time: 11:19a
 * Updated in $/control processor/code/system
 * SCR-0040 - Updated LogFindNext to use a passed in pointer rather than
 * to rely on a static global which can be changed from by any function
 * call.
 *
 *
 *
 ***************************************************************************/
