#define DATA_MNG_BODY
/******************************************************************************
            Copyright (C) 2009-2015 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    ECCN:        9E991

    File:        DataManager.c

    Description: The data manager contains the functions used to record
                 data from the various interfaces.

    VERSION
      $Revision: 99 $  $Date: 3/09/15 10:55a $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "DataManager.h"
#include "cfgmanager.h"
#include "arinc429.h"
#include "qar.h"
#include "user.h"
#include "ClockMgr.h"
#include "Utility.h"
#include "gse.h"
#include "Assert.h"
#include "LogManager.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define DM_BUF_EMPTY_STATUS (-2)
#define DM_BUF_BUILD_STATUS (-1)

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static DATA_MNG_INFO       dataMgrInfo[DATA_MGR_MAX_CHANNELS];
static DATA_MNG_TASK_PARMS dmpBlock   [DATA_MGR_MAX_CHANNELS];
static BOOLEAN             dmRecordEnabled; // Local Flag for recording status
static DATA_MNG_DOWNLOAD_RECORD_STATS recordStats[DATA_MGR_MAX_CHANNELS];
// This needs to be declared as UINT32 to ensure proper alignment
// Internally this is handled as a BYTE buffer
// If you change this to a UINT8 like I did, more than once... Then the QAR
// Read Raw will ASSERT.... DON'T CHANGE THIS AGAIN!!!
static UINT32             dmTempBuffer[MAX_BUFF_SIZE/4];

static ACS_CONFIG configTemp;    //ACS configuration block used for modifying
                                 //cfg values from the UM.
static DATA_MNG_TASK_PARMS    tcbTemp;
static DATA_MNG_INFO          dmInfoTemp;
static DM_DOWNLOAD_STATISTICS acs_StatusTemp;
static CHAR tempDebugString[512];
// Temporary location to set storage of status before it is returned from 
// download protocol handler. For channels that record data this needs to 
// initialized even though it isn't needed. 
static DL_WRITE_STATUS tempDLstatus[DATA_MGR_MAX_CHANNELS];

//Used to generate events back to "parent" when the record busy/not busy
//status changes
static void (*m_event_func)(INT32,BOOLEAN);
static INT32 m_event_tag;

//For channels 0-7,
static UINT32 m_channel_busy_flags;
static BOOLEAN m_is_busy_last = FALSE;

static UINT8 noData = 0;

// include user command tables here.(After local var have been declared.)
#include "DataManagerUserTables.c"

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void DataMgrInitChannels( void);

static void DataMgrCreateTask ( UINT32 source, ACS_CONFIG *pACSConfig,
                                GET_DATA GetDataFunc, GET_SNAP GetSnapFunc,
                                DO_SYNC DoSyncFunc, GET_HDR GetDataHdrFunc,
                                DATATASK DataTask, UINT32 MIFframes, BOOLEAN bEnable );

static void DataMgrTask( void *pParam );

static void DataMgrStartRecording( DATA_MNG_TASK_PARMS *pDMParam,
                                   DATA_MNG_INFO       *pDMInfo);

static void DataMgrRecordData( DATA_MNG_TASK_PARMS *pDMParam,
                               DATA_MNG_INFO       *pDMInfo,
                               UINT16              nByteCnt);

static void DataMgrNewBuffer( DATA_MNG_TASK_PARMS *pDMParam,
                              DATA_MNG_INFO       *pDMInfo,
                              UINT16              *nLeftOver);

static void DataMgrGetSnapShot( DATA_MNG_INFO *pDMInfo, GET_SNAP GetSnap,
                                   UINT16 nChannel, DM_PACKET_TYPE eType  );

static void DataMgrBuildPacketHdr( DATA_MNG_BUFFER *pMsgBuf, UINT16 nChannel,
                                   DM_PACKET_TYPE eType);

static void DataMgrFinishPacket( DATA_MNG_INFO *pDMInfo, UINT32 nChannel,
                                 INT32 recordInfo);

static BOOLEAN DataMgrProcBuffers   ( DATA_MNG_INFO *pDMInfo, UINT32 nChannel );

static UINT16 DataMgrWriteBuffer ( DATA_MNG_BUFFER *pMsgBuf, UINT8 *pSrc,
                                   UINT16 nBytes );

static UINT16 DataMgrBuildFailLog( UINT32 Channel, DATA_MNG_INFO *pDMInfo,
                                   DM_FAIL_DATA *pFailLog );

static void DataMgrDownloadTask( void *pParam );

static UINT8* DataMgrRequestRecord ( ACS_PORT_TYPE PortType, ACS_PORT_INDEX PortIndex,
                                     DL_STATUS *pStatus, UINT32 *pSize,
                                     DL_WRITE_STATUS **pDL_WrStatus );
static UINT32 DataMgrSaveDLRecord ( DATA_MNG_INFO *pDMInfo, UINT8 Channel,
                                    DM_PACKET_TYPE PacketType );

static void DataMgrDLSaveAndChangeState ( DATA_MNG_INFO *pDMInfo,
                                          DM_PACKET_TYPE Type, UINT8 nChannel );
static void DataMgrDLUpdateStatistics   ( DATA_MNG_INFO *pDMInfo,
                                          DATA_MNG_DOWNLOAD_RECORD_STATS *pStats );
static BOOLEAN    DataMgrIsPortFFD      ( const ACS_CONFIG *ACS_Config );
static BOOLEAN    DataMgrIsFFDInhibtMode( void );

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/
/*****************************************************************************
 * Function:    DataMgrIntialize
 *
 * Description: Initialize the Data Manager functionality.
 *              1. Loads the Data Manager User commands into the User table
 *              2. Loads the ACS User commands into the User table
 *              3. Based on configuration setup the data channels
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:
 *
 *****************************************************************************/
void DataMgrIntialize(void)
{
   // Initialize Local Data and the user tables
   dmRecordEnabled      = FALSE;
   m_event_func         = NULL;
   m_event_tag          = 0;
   m_channel_busy_flags = 0;
   User_AddRootCmd (&rootACSMsg);
   User_AddRootCmd (&dataMngMsg);

   DataMgrInitChannels();
}

/******************************************************************************
 * Function:    DataMgrGetACSDataSet
 *
 * Description: Return the contents of the ACS Data Configuration.  If Full
 *              Flight Data Offload is in effect, force the priority of Full
 *              Flight Data channels to "4".  Priority 4 logs are stored locally
 *              on the CF card and not offloaded.
 *
 * Parameters:  ACS (i): the requested ACS
 *
 * Returns:     A copy of the data structure.
 *
 * Notes:
 *
 *****************************************************************************/
ACS_CONFIG DataMgrGetACSDataSet(LOG_ACS_FIELD ACS)
{
   // Local Data
   ACS_CONFIG tempConfig;

   memset(&tempConfig,0,sizeof(tempConfig));

   if (ACS_DONT_CARE != ACS)
   {
      tempConfig = dataMgrInfo[ACS].acs_Config;
   }

   return (tempConfig);
}

/*****************************************************************************
 * Function:    DataMgrSetRecStateChangeEvt
 *
 * Description: Set the callback fuction to call when the busy/not busy
 *              status of the Data Manager Recording changes
 *
 *
 *
 * Parameters:  [in] tag:  Integer value to use when calling "func"
 *              [in] func: Function to call when busy/not busy status changes
 *
 * Returns:      none
 *
 * Notes:       Only remembers one event handler.  Subsequent calls overwrite
 *              the last event handler.
 *
 *******************************************************************************/
void DataMgrSetRecStateChangeEvt(INT32 tag,void (*func)(INT32,BOOLEAN))
{
  m_event_func = func;
  m_event_tag  = tag;
}

/******************************************************************************
 * Function:    DataMgrRecord
 *
 * Description: The Data Manager Record function is used to enable or disable
 *              recording of the specified channel.
 *
 * Parameters:  BOOLEAN bEnable - [ TRUE  - Enable Recording
 *                                  FALSE - Disable Recording ]
 *
 * Returns:     None.
 *
 * Notes:
 *****************************************************************************/
void DataMgrRecord (BOOLEAN bEnable )
{
   BOOLEAN is_busy;
   INT32 int_save;

   dmRecordEnabled = bEnable;
   if (bEnable == TRUE)
   {
      // Kill any downloads in process
      DataMgrStopDownload();
   }

   int_save = __DIR();
   //Notify event handler on record status change.
   if(m_event_func != NULL)
   {
      is_busy = (dmRecordEnabled || (m_channel_busy_flags != 0));
      if(is_busy != m_is_busy_last)
      {
         m_is_busy_last = is_busy;
         __RIR(int_save);
         m_event_func(m_event_tag,is_busy);
      }
   }
   __RIR(int_save);

}

/******************************************************************************
 * Function:    DataMgrRecordFinished
 *
 * Description: The Data Manager Record Finished function will search all of the
 *              configured ACS channels and check to see that all data has
 *              been written. This function is used by the FAST Manager to
 *              make sure that every channel is closed out before logging an
 *              end of record.
 *
 * Parameters:  None
 *
 * Returns:     BOOLEAN - TRUE if all channels have completed recording
 *                        FALSE if data is pending
 *
 * Notes:
 *****************************************************************************/
BOOLEAN DataMgrRecordFinished (void)
{
   // Local Data
   BOOLEAN    bFinished;
   UINT32     i;
   UINT32     j;

   // Initialize Local Data
   bFinished = TRUE;

   // Loop and Check all of the possible ACS Channels
   for ( i=0; (i < MAX_ACS_CONFIG) && (bFinished == TRUE); i++ )
   {
      // Check if the channel is configured
      if ( ACS_PORT_NONE != dataMgrInfo[i].acs_Config.portType )
      {
         // The channel is configured so now verify that the
         // recording for this channel has completed
         if ( DM_RECORD_IDLE == dataMgrInfo[i].recordStatus )
         {
            // The recording has stopped now loop through the buffers
            // and verify all buffers have been emptied
            for ( j = 0; (j < MAX_DM_BUFFERS) && (bFinished == TRUE); j++ )
            {
               if (DM_BUF_EMPTY != dataMgrInfo[i].msgBuf[j].status)
               {
                  // The buffer is not empty so the recording is not finished
                  bFinished = FALSE;
               }
            }
         }
         else
         {
            // The current channel is not IDLE still recording
            bFinished = FALSE;
         }
      }
   }

   // Return the finish status
   return bFinished;
}

/******************************************************************************
 * Function:    DataMgrGetHeader
 *
 * Description: Retrieves the file header for a given ACS type
 *
 * Parameters:  INT8   *pDest       - Pointer to storage buffer
 *              LOG_ACS_FIELD ACS   - ACS index
 *              UINT16 nMaxByteSize - Amount of space in buffer
 *
 * Returns:     UINT16 - Total number of bytes written
 *
 * Notes:
 *****************************************************************************/
UINT16 DataMgrGetHeader (INT8 *pDest, LOG_ACS_FIELD ACS, UINT16 nMaxByteSize )
{
   // Local Data
   DATA_MNG_TASK_PARMS *pDMParam;
   DATA_MNG_INFO       *pDMInfo;
   DATA_MNG_HDR        dataHdr;
   CHAR                *pBuffer;
   UINT16              cnt;

   cnt = 0;

   if (ACS_DONT_CARE != ACS)
   {
      // Initialize Local Data
      pDMParam       = &dmpBlock[ACS];
      pDMInfo        = &dataMgrInfo[pDMParam->nChannel];

      // Update protocol file hdr first !
      pBuffer = (CHAR *)(pDest + sizeof(dataHdr));  // Move to end of UartMgr File Hdr

      if ( (ACS_PORT_NONE != pDMInfo->acs_Config.portType) && (pDMParam->pGetDataHdr != NULL) )
      {
         cnt = pDMParam->pGetDataHdr( pBuffer, (UINT32)pDMInfo->acs_Config.nPortIndex, 
                                      nMaxByteSize);
      }

      // Update Data hdr
      dataHdr.version = HDR_VERSION;
      dataHdr.type    = pDMInfo->acs_Config.portType;

      // Copy hdr to destination
      pBuffer = (CHAR *)pDest;
      memcpy ( pBuffer, &dataHdr, sizeof(dataHdr) );

      cnt += sizeof(dataHdr);
   }

  return (cnt);
}



/******************************************************************************
 * Function:    DataMgrDownloadGetState  | IMPLEMENTS GetState() INTERFACE to
 *                                       | FAST STATE MGR
 *
 * Description: Returns the downloading/!downloading state of the data manager
 *              ACSs
 *
 * Parameters:  INT32 param  - Not used, included to match FSM call signature
 *
 * Returns:     BOOLEAN - TRUE: Download task is running
 *                        FALSE: Download task is inactive
 *
 * Notes:
 *****************************************************************************/
BOOLEAN DataMgrDownloadGetState ( INT32 param )
{
  return DataMgrDownloadingACS();
}



/******************************************************************************
 * Function:    DataMgrRecRun       | IMPLEMENTS GetState() INTERFACE to
 *                                  | FAST STATE MGR
 *
 * Description:
 *
 * Parameters:  BOOLEAN Run   - TRUE: Attempt to start recording
 *                              FALSE: Attempt to stop recording
 *              INT32   param - Not used, to match cal signature only
 *
 * Returns:
 *
 * Notes:
 *****************************************************************************/
void DataMgrRecRun ( BOOLEAN Run, INT32 param  )
{
  dmRecordEnabled = Run;
  // Notify the ClockMgr of the new Recording state
  CM_UpdateRecordingState(Run);
}



/******************************************************************************
 * Function:    DataMgrDownloadRun  | IMPLEMENTS DownloadRun() INTERFACE to
 *                                  | FAST STATE MGR
 *
 * Description: Function used to start or end downloads.
 *
 * Parameters:  BOOLEAN Run  - TRUE: Attempt to start downloading
 *                             FALSE: Attempt to stop downloading
 *              INT32   param - Not used, to match cal signature only
 *
 * Returns:     None.
 *
 * Notes:
 *****************************************************************************/
void DataMgrDownloadRun ( BOOLEAN Run, INT32 param  )
{
  if(Run)
  {
    DataMgrStartDownload();
  }
  else
  {
    DataMgrStopDownload();
  }
}

/******************************************************************************
 * Function:    DataMgrStopDownload
 *
 * Description: Data Manager Stop Download is an external command that will
 *              initiate a stop of a download in-progress.
 *
 * Parameters:  None.
 *
 * Returns:     None.
 *
 * Notes:       None.
 *
 *****************************************************************************/
void DataMgrStopDownload ( void )
{
   // Local Data
   DATA_MNG_INFO *pDMInfo;
   UINT8         nChannel;

   // Loop through all the configured channels and look for the
   // channels that are currently downloading
   for ( nChannel = 0; nChannel < MAX_ACS_CONFIG; nChannel++ )
   {
      pDMInfo = &dataMgrInfo[nChannel];

      if ( pDMInfo->dl.bDownloading == TRUE )
      {
         // Check the interface type and command the interface to stop
         switch (pDMInfo->acs_Config.portType)
         {
            case ACS_PORT_UART:
               UartMgr_DownloadStop((UINT8)pDMInfo->acs_Config.nPortIndex);
               break;
            case ACS_PORT_NONE:
            case ACS_PORT_ARINC429:
            case ACS_PORT_QAR:
            case ACS_PORT_MAX:
            default:
              FATAL ("Unexpected ACS_PORT_TYPE = % d", pDMInfo->acs_Config.portType);
            break;
         }
         // Reset the flags for the channel
         pDMInfo->dl.bDownloading  = FALSE;
         pDMInfo->dl.bDownloadStop = TRUE;

         // Decrement for this channel download ending.
         CM_UpdateRecordingState(FALSE);
         //break;  - removed this break as it could cause early-exit from 'for' loop
      }
   }

}


/******************************************************************************
 * Function:    DataMgrDownloadingACS
 *
 * Description: The Data Manager Downloading ACS function polls all of the
 *              data manager tasks to determine if any of them are downloading.
 *
 * Parameters:  None
 *
 * Returns:     BOOLEAN - TRUE  if downloading
 *                        FALSE if not downloading
 *
 * Notes:
 *****************************************************************************/
BOOLEAN DataMgrDownloadingACS (void)
{
   // Local Data
   DATA_MNG_INFO *pDMInfo;
   UINT16        nChannel;
   BOOLEAN       bDownloading;

   // Initialize Local Data
   bDownloading = FALSE;

   // Loop through all possible ACS configuration and determine the
   // download state
   for ( nChannel = 0; nChannel < MAX_ACS_CONFIG; nChannel++ )
   {
      pDMInfo = &dataMgrInfo[nChannel];

      if ( pDMInfo->dl.bDownloading == TRUE )
      {
         bDownloading = TRUE;
         break;
      }
   }

   return bDownloading;
}


/******************************************************************************
 * Function:    DataMgrStartDownload
 *
 * Description: The Data Manager Start Download is responsible for
 *              initiating downloads of Aircraft Subsystems.
 *              This function will enable the task that was intialized on
 *              start up to begin the download process for each channel.
 *
 * Parameters:  None.
 *
 * Returns:     TRUE  - If any task was started for download
 *              FALSE - If there weren't any download tasks
 *
 * Notes:
 *
 *****************************************************************************/
BOOLEAN DataMgrStartDownload( void )
{
   // Local Data
   DATA_MNG_INFO          *pDMInfo;
   ACS_CONFIG             *pACSConfig;
   UINT16                 nChannel;
   BOOLEAN                bStarted;
   TIMESTAMP              currentTime;
   DM_DOWNLOAD_STATISTICS *pStatistics;
   DM_DOWNLOAD_START_LOG  startLog;

   bStarted = FALSE;

   if (!UploadMgr_IsUploadInProgress() && !DataMgrDownloadingACS())
   {
      for (nChannel = 0; nChannel < MAX_ACS_CONFIG; nChannel++)
      {
         pDMInfo    = &dataMgrInfo[nChannel];
         pACSConfig = &pDMInfo->acs_Config;

         if (pACSConfig->mode == ACS_DOWNLOAD)
         {
            pDMInfo->dl.bDownloading                           = TRUE;
            startLog.nChannel  = nChannel;
            strncpy_safe(startLog.model, sizeof(startLog.model),
                         pDMInfo->acs_Config.sModel, _TRUNCATE);
            strncpy_safe(startLog.id,    sizeof(startLog.id),
                         pDMInfo->acs_Config.sID,    _TRUNCATE);
            startLog.portType  = pDMInfo->acs_Config.portType;
            startLog.portIndex = (UINT8)pDMInfo->acs_Config.nPortIndex;

            LogWriteSystem(APP_DM_DOWNLOAD_STARTED, LOG_PRIORITY_LOW,
                           &startLog, sizeof(startLog), NULL);

            pStatistics = &pDMInfo->dl.statistics;

            pDMInfo->dl.bDownloadStop                          = FALSE;
            pStatistics->nRcvd                                 = 0;
            pStatistics->nTotalBytes                           = 0;
            CM_GetTimeAsTimestamp (&currentTime);
            pStatistics->startTime                             = currentTime;
            pStatistics->endTime.Timestamp                     = 0;
            pStatistics->endTime.MSecond                       = 0;
            pStatistics->nStart_ms                             = CM_GetTickCount();
            pStatistics->nDuration_ms                          = 0;

            pStatistics->smallestRecord.nNumber                = 0;
            pStatistics->smallestRecord.nSize                  = UINT32_MAX;
            pStatistics->smallestRecord.nDuration_ms           = 0;
            pStatistics->smallestRecord.nStart_ms              = 0;
            pStatistics->smallestRecord.requestTime.Timestamp  = 0;
            pStatistics->smallestRecord.requestTime.MSecond    = 0;
            pStatistics->smallestRecord.foundTime.Timestamp    = 0;
            pStatistics->smallestRecord.foundTime.MSecond      = 0;

            pStatistics->largestRecord.nNumber                 = 0;
            pStatistics->largestRecord.nSize                   = 0;
            pStatistics->largestRecord.nDuration_ms            = 0;
            pStatistics->largestRecord.nStart_ms               = 0;
            pStatistics->largestRecord.requestTime.Timestamp   = 0;
            pStatistics->largestRecord.requestTime.MSecond     = 0;
            pStatistics->largestRecord.foundTime.Timestamp     = 0;
            pStatistics->largestRecord.foundTime.MSecond       = 0;

            pStatistics->shortestRequest.nNumber               = 0;
            pStatistics->shortestRequest.nSize                 = 0;
            pStatistics->shortestRequest.nDuration_ms          = UINT32_MAX;
            pStatistics->shortestRequest.nStart_ms             = 0;
            pStatistics->shortestRequest.requestTime.Timestamp = 0;
            pStatistics->shortestRequest.requestTime.MSecond   = 0;
            pStatistics->shortestRequest.foundTime.Timestamp   = 0;
            pStatistics->shortestRequest.foundTime.MSecond     = 0;

            pStatistics->longestRequest.nNumber                = 0;
            pStatistics->longestRequest.nSize                  = 0;
            pStatistics->longestRequest.nDuration_ms           = 0;
            pStatistics->longestRequest.nStart_ms              = 0;
            pStatistics->longestRequest.requestTime.Timestamp  = 0;
            pStatistics->longestRequest.requestTime.MSecond    = 0;
            pStatistics->longestRequest.foundTime.Timestamp    = 0;
            pStatistics->longestRequest.foundTime.MSecond      = 0;

            // Enable the tasks to perform the download
            TmTaskEnable ((TASK_INDEX)(nChannel + (UINT32)Data_Mgr0), TRUE);

            // Increment for this channel download starting.
            CM_UpdateRecordingState(TRUE);

            // Indicate that 1..n tasks are active.
            bStarted = TRUE;
         }
      }
   }

   return bStarted;
}


/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/
/******************************************************************************
* Function:    DataMgrCreateTask
*
* Description: The Data Manager Create task is responsible for creating
*              each of the data recording tasks needed for the system. This
*              function should be called by an application level task
*              during initialization.
*
* Parameters:  UINT16 source          - index of the channel being recorded
*              ACS_CONFIG pACSConfig  - Data channel configuration info
*              GET_DATA GetDataFunc   - Function used to
*                                       get the data during recording
*              GET_SNAP GetSnapFunc   - Function that gets a
*                                       snapshot of the channel
*              DO_SYNC  DoSyncFunc    - Function that syncs time delta
*              GET_HDR GetDataHdrFunc - Function to get the Binary Header
*              DATATASK DataTask      - Download or Record Task
*              UINT32 MIFframes       - Minor Frame to run within
*              BOOLEAN bEnable        - Flag to enable or disable the task
*
* Returns:     None
*
* Notes:       This function must be called for each channel that needs
*              to be recorded.
*
*****************************************************************************/
static void DataMgrCreateTask( UINT32 source, ACS_CONFIG *pACSConfig,
                               GET_DATA GetDataFunc, GET_SNAP GetSnapFunc,
                               DO_SYNC DoSyncFunc, GET_HDR GetDataHdrFunc,
                               DATATASK DataTask, UINT32 MIFframes, BOOLEAN bEnable )
{
    // Local Data
    UINT8       i;
    TASK_INDEX  taskId;
    TCB         tcbTaskInfo;

    // Initialize the Data Manager Buffers
    memset( dmTempBuffer, 0, sizeof(dmTempBuffer));
    dataMgrInfo[source].nStartTime_mS         = 0;
    dataMgrInfo[source].nCurrentBuffer       = 0;
    dataMgrInfo[source].acs_Config           = *pACSConfig;
    dataMgrInfo[source].recordStatus         = DM_RECORD_IDLE;
    dataMgrInfo[source].bBufferOverflow      = FALSE;
    dataMgrInfo[source].dl.nBytes            = 0;
    dataMgrInfo[source].dl.state             = DL_RECORD_REQUEST;
    dataMgrInfo[source].dl.bDownloading      = FALSE;
    dataMgrInfo[source].dl.bDownloadStop     = FALSE;
    dataMgrInfo[source].dl.statistics.nRcvd  = 0;

    tempDLstatus[source] = DL_WRITE_SUCCESS;
    
    for (i = 0; i < MAX_DM_BUFFERS; ++i)
    {
        memset( &dataMgrInfo[source].msgBuf[i].packet, 0, sizeof(DM_PACKET));
        dataMgrInfo[source].msgBuf[i].status      = DM_BUF_EMPTY;
        dataMgrInfo[source].msgBuf[i].index       = 0;
        dataMgrInfo[source].msgBuf[i].tryTime_mS  = 0;
        dataMgrInfo[source].msgBuf[i].wrStatus    = LOG_REQ_NULL;
        dataMgrInfo[source].msgBuf[i].pDL_Status  = &tempDLstatus[source];
    }

    memset (&recordStats[source],0,sizeof(DATA_MNG_DOWNLOAD_RECORD_STATS));

    //------------------------------------------------------------------------
    // Create Data Manager - DT
    taskId = (TASK_INDEX)(source + (UINT32)Data_Mgr0);

    memset(&tcbTaskInfo, 0, sizeof(tcbTaskInfo));
    snprintf(tcbTaskInfo.Name, sizeof(tcbTaskInfo.Name), "Data Mgr %d", source);
    tcbTaskInfo.TaskID                     = taskId;
    tcbTaskInfo.Function                   = DataTask;
    tcbTaskInfo.Priority                   = taskInfo[taskId].priority;
    tcbTaskInfo.Type                       = taskInfo[taskId].taskType;
    tcbTaskInfo.modes                      = taskInfo[taskId].modes;
    tcbTaskInfo.Enabled                    = bEnable;
    tcbTaskInfo.Locked                     = FALSE;
    tcbTaskInfo.MIFrames                   = MIFframes;
    tcbTaskInfo.pParamBlock                = &dmpBlock[ source];

    // Initialize the Task Control Block
    dmpBlock[source].nChannel           = (UINT16)source;
    dmpBlock[source].pGetData           = GetDataFunc;
    dmpBlock[source].pGetSnapShot       = GetSnapFunc;
    dmpBlock[source].pDoSync            = DoSyncFunc;
    dmpBlock[source].pGetDataHdr        = GetDataHdrFunc;

    TmTaskCreate (&tcbTaskInfo);
}

/******************************************************************************
 * Function:    DataMgrTask
 *
 * Description: The Data Manager Task is responsible from reading data from
 *              the initialized source and formatting it into packets. Once
 *              the specified packet has been created it is sent to be
 *              in data flash by the Log Manager.
 *
 * Parameters:  void *pParam - Pointer to the Task Control Block for the
 *                             channel being recorded.
 *
 * Returns:     None.
 *
 * Notes:       There can be multiple Data Manger Tasks running, but all are
 *              DT task so no re-entrant issues should occur.
 *
 *****************************************************************************/
static void DataMgrTask( void *pParam )
{
   // Local Data
   UINT16              nSize;
   DATA_MNG_INFO       *pDMInfo;
   DATA_MNG_BUFFER     *pMsgBuf;
   DATA_MNG_TASK_PARMS *pDMParam;
   BOOLEAN              buffers_active;
   // Initialize Local Data
   pDMParam       = (DATA_MNG_TASK_PARMS *)pParam;
   pDMInfo        = &dataMgrInfo[pDMParam->nChannel];
   pMsgBuf        = &pDMInfo->msgBuf[pDMInfo->nCurrentBuffer];

   pDMInfo->nCurrTime_mS = CM_GetTickCount();

   ASSERT( NULL != pDMParam->pGetData);

   // Empty the SW FIFO and discard the data
   nSize = pDMParam->pGetData( dmTempBuffer, (UINT32)pDMInfo->acs_Config.nPortIndex,
                              sizeof(dmTempBuffer));

   // Is the Recording Active?
   if (TRUE == dmRecordEnabled)
   {
      // Was the recording off?
      if (DM_RECORD_IDLE == pDMInfo->recordStatus)
      {
         DataMgrStartRecording( pDMParam, pDMInfo);
      }
      else // The recording is already started IN_PROGRESS
      {
         DataMgrRecordData( pDMParam, pDMInfo, nSize);
      }
   }
   // NO trigger check if this is the end of the recording
   else if (DM_RECORD_IN_PROGRESS == pDMInfo->recordStatus)
   {
      // Check if the buffer is empty
      if (DM_BUF_EMPTY == pMsgBuf->status)
      {
         // Turn recording off
         pDMInfo->recordStatus = DM_RECORD_IDLE;

         // record the time that End Snapshot is collected
         CM_GetTimeAsTimestamp (&(pDMInfo->msgBuf[pDMInfo->nCurrentBuffer].packetTs));

         // Record the current snap shot status of the channel
         DataMgrGetSnapShot( pDMInfo, pDMParam->pGetSnapShot,
                             pDMParam->nChannel, DM_END_SNAP);

         // Close the active buffer and swap
         DataMgrFinishPacket( pDMInfo, pDMParam->nChannel, DM_BUF_EMPTY_STATUS );
      }
      else if (DM_BUF_BUILD == pMsgBuf->status)
      {
         // Close the active buffer and swap
         DataMgrFinishPacket( pDMInfo, pDMParam->nChannel, DM_BUF_BUILD_STATUS );
      }
   }

   // Check buffers and write any pending data packets
   buffers_active = DataMgrProcBuffers( pDMInfo, pDMParam->nChannel );

   //After processing channel buffers, check if all were idle or
   //if one was being processed.
   if(buffers_active || (DM_RECORD_IN_PROGRESS == pDMInfo->recordStatus) )
   {
      m_channel_busy_flags |= 1U << pDMParam->nChannel;
   }
   else
   {
      m_channel_busy_flags &= ~(1U << pDMParam->nChannel);
   }

   //If the busy status (idle or record enabled) changes, then update
   //the event handler.
   if( (m_channel_busy_flags || dmRecordEnabled) != m_is_busy_last )
   {
      m_is_busy_last = (m_channel_busy_flags || dmRecordEnabled);
      if(m_event_func != NULL)
      {
        m_event_func(m_event_tag,m_is_busy_last);
      }
   }
}

/******************************************************************************
* Function:    DataMgrStartRecording
*
* Description: The Data Manager is starting to record data.
*
* Parameters:  pDMParam (i): channelId, GetSnapShot func ptr, etc.
*              pDMInfo  (i): Data Manager info for the current channel.
*                            Allows access to the packet buffer fields.
*
* Returns:     None.
*
* Notes:       None.
*
*****************************************************************************/
static void DataMgrStartRecording( DATA_MNG_TASK_PARMS *pDMParam,
                                   DATA_MNG_INFO       *pDMInfo)
{
    UINT16              nSize;
    TIMESTAMP           timeNow;
    DATA_MNG_BUFFER     *pMsgBuf;

    // get the current buffer
    pMsgBuf = &pDMInfo->msgBuf[pDMInfo->nCurrentBuffer];

    // When a new recording starts, Reset the System Condition if previously set
    //   Note: If a condition is detected during a recording, the sysCond will remain
    //         for the duration of that recording.  Only when new recording starts
    //         will sys condition be cleared and if condition detected again, sysCond
    //         will be set once again.
    //         bBufferOverflow is independent
    //         conditions and can be set individually thus must be reset individually.

    if (pDMInfo->bBufferOverflow != FALSE) {
        Flt_ClrStatus( pDMInfo->acs_Config.systemCondition );
        pDMInfo->bBufferOverflow    = FALSE;
    }

    // Do we have an empty buffer?
    if (DM_BUF_EMPTY == pMsgBuf->status)
    {
        // Get the current Time
        CM_GetTimeAsTimestamp (&timeNow);

        // set the packet timestamp, needed for the header
        pDMInfo->msgBuf[pDMInfo->nCurrentBuffer].packetTs = timeNow;

        // Records the current snapshot to Data Flash and then switched the
        // current buffer flag to the opposite buffer.
        DataMgrGetSnapShot( pDMInfo, pDMParam->pGetSnapShot,
                            pDMParam->nChannel, DM_START_SNAP);

        // Record the time the buffer started recording current channel
        pDMInfo->nStartTime_mS = pDMInfo->nCurrTime_mS;
        pDMInfo->nNextTime_mS = pDMInfo->nCurrTime_mS;
        nSize = 0;

        // a new buffer will be allocated because
        // pDMInfo->NextTime_mS == pDMInfo->CurrTime_mS
        DataMgrNewBuffer( pDMParam, pDMInfo, &nSize);

        // Turn recording on
        pDMInfo->recordStatus = DM_RECORD_IN_PROGRESS;
    }
}

/******************************************************************************
* Function:    DataMgrRecordData
*
* Description: The Data Manager ongoing recording of data.
*
* Parameters:  DATA_MNG_TASK_PARMS *pDMParam - Data Manager Parameters
*              DATA_MNG_INFO *pDMInfo  - Data Manager info for the current
*                                        channel. Allows access to the
*                                        packet buffer fields.
*              UINT16        nByteCnt  - Number of Bytes
*
* Returns:     None.
*
* Notes:       None.
*
*****************************************************************************/
static void DataMgrRecordData( DATA_MNG_TASK_PARMS *pDMParam,
                               DATA_MNG_INFO       *pDMInfo,
                               UINT16              nByteCnt)
{
    UINT8               *pBuf;
    UINT16              nBufIndex;
    UINT16              nRemaining;
    DATA_MNG_BUFFER     *pMsgBuf;

    // Set the buffer index
    nBufIndex  = 0;
    nRemaining = 0;

    pBuf = (UINT8*)dmTempBuffer;

    // Check if any data was received
    while (nByteCnt != 0)
    {
        pMsgBuf = &pDMInfo->msgBuf[pDMInfo->nCurrentBuffer];

        // Data was received is the current buffer empty?
        if (DM_BUF_EMPTY == pMsgBuf->status)
        {
            // This is a new packet build the header
            DataMgrBuildPacketHdr(pMsgBuf, pDMParam->nChannel, DM_STD_PACKET);
            // Now change the buffer state to build
            pMsgBuf->status = DM_BUF_BUILD;
        }

        // Are we already building a packet?
        if (DM_BUF_BUILD == pMsgBuf->status)
        {
            // Write any received data to the buffer
            nRemaining = DataMgrWriteBuffer(pMsgBuf, &pBuf[nBufIndex], nByteCnt);

            // check if we need a new buffer, set nRemaining=0 for Data Loss error
            DataMgrNewBuffer( pDMParam, pDMInfo, &nRemaining);

            // Find the location in the Temp Buffer to continue from
            nBufIndex = nByteCnt - nRemaining;

            // Set the number of bytes to the remaining count
            nByteCnt = nRemaining;
        }
    }
    // Check if we have timed out this buffer
    DataMgrNewBuffer( pDMParam, pDMInfo, &nRemaining);
}

/******************************************************************************
* Function:    DataMgrNewBuffer
*
* Description: The packet size has been met or overflowed, finish this buffer
*              and start the next one, check for and record error conditions
*
* Parameters:
*  DATA_MNG_TASK_PARMS *pDMParam  - channel Id
*  DATA_MNG_INFO       *pDMInfo   - Data Manager info for the current channel.
*                                   Allows access to the packet buffer fields.
*  DATA_MNG_BUFFER     *pMsgBuf   - Pointer to function that will get the snapshot
*                                   data.
*  UINT32              *nLeftOver - Data Bytes not written to the current buffer,
*                                   Can be changed on data loss condition to abort
*                                   callers loop
*
* Returns:     None
*
* Notes:       None.
*
*****************************************************************************/
static void DataMgrNewBuffer( DATA_MNG_TASK_PARMS *pDMParam,
                              DATA_MNG_INFO       *pDMInfo,
                              UINT16              *nLeftOver)
{
    UINT16          nSize;
    DM_FAIL_DATA    failLog;
    DATA_MNG_BUFFER *pMsgBuf;
    TIMESTAMP       tempTime;

    // Check if packet timeout has been reached, or we overflowed the buffer
    if ((pDMInfo->nCurrTime_mS >= pDMInfo->nNextTime_mS) || *nLeftOver > 0)
    {
        // Save the time in case this is an overflow
        tempTime = pDMInfo->msgBuf[pDMInfo->nCurrentBuffer].packetTs;
        
        // Close out packet in buffer and mark for storage, nCurrentBuffer is
        // incremented when the packet is finished!!!
        DataMgrFinishPacket(pDMInfo, pDMParam->nChannel, (INT32)*nLeftOver);

        //------------------------ prep new buffer or error ----------------------------
        pMsgBuf = &pDMInfo->msgBuf[pDMInfo->nCurrentBuffer];
        
        // Check if the new buffer is ready to receive new data
        ASSERT_MESSAGE(DM_BUF_EMPTY == pMsgBuf->status,
                       "Channel %d Buffers Full", pDMParam->nChannel);
        
        // Check if the buffer is full and there is still data to write
        if ( *nLeftOver > 0)
        {
            // Need to keep the same timestamp as the previous buffer because
            // the time deltas will be wrong otherwise
            pDMInfo->msgBuf[pDMInfo->nCurrentBuffer].packetTs = tempTime;
           
            // Check if the overflow was already reported
            if (FALSE == pDMInfo->bBufferOverflow)
            {
                // Set Buffer Overflow TRUE
                pDMInfo->bBufferOverflow = TRUE;

                // Now build the fail log and record the failure
                nSize = DataMgrBuildFailLog(pDMParam->nChannel, pDMInfo, &failLog);

                // Set the system status and record failure
                Flt_SetStatus( pDMInfo->acs_Config.systemCondition,
                               APP_DM_SINGLE_BUFFER_OVERFLOW,
                               &failLog, nSize);

                // Print out a debug message
                GSE_DebugStr(NORMAL, TRUE,
                    "DataManager: BufferOverflow, Channel %d (%d)",
                    pDMParam->nChannel, *nLeftOver);
            }
        }
        else // This is not an overflow continue on normally
        {
           // Increment the next time
           pDMInfo->nNextTime_mS += pDMInfo->acs_Config.nPacketSize_ms;

           // Set the Timestamp for the next packet
           CM_GetTimeAsTimestamp( &(pDMInfo->msgBuf[pDMInfo->nCurrentBuffer].packetTs));
           pDMParam->pDoSync( (UINT32)pDMInfo->acs_Config.nPortIndex );
        }
    }
}

/******************************************************************************
 * Function:    DataMgrGetSnapShot
 *
 * Description: The Data Manager Get Snapshot function is responsible for
 *              building the packet to be used at the beginning and ending
 *              of recording. This data will be the current value of all
 *              data that has been received.
 *
 * Parameters:  DATA_MNG_INFO *pDMInfo  - Data Manager info for the current
 *                                        channel. Allows access to the
 *                                        packet buffer fields.
 *              GET_SNAP      *pGetSnap - Pointer to function that will get
 *                                        the snapshot data.
 *              UINT16        nChannel   - Index of the channel being recorded
 *              DM_PACKET_TYPE  eType   - The type of packet
 *
 * Returns:     None.
 *
 * Notes:       None.
 *
 *****************************************************************************/
static void DataMgrGetSnapShot ( DATA_MNG_INFO *pDMInfo, GET_SNAP GetSnap,
                                 UINT16 nChannel, DM_PACKET_TYPE eType )
{
   // Local Data
   UINT16          nCnt;
   DATA_MNG_BUFFER *pMsgBuf;
   BOOLEAN         bStartSnap;

   ASSERT(NULL != GetSnap);

   // Initialize Local Data
   pMsgBuf   = &pDMInfo->msgBuf[pDMInfo->nCurrentBuffer];

   // Build the packet header
   DataMgrBuildPacketHdr(pMsgBuf, nChannel, eType);

   bStartSnap = (DM_START_SNAP == eType) ? TRUE : FALSE;

   // Get A data snapshot
   nCnt = GetSnap( pMsgBuf->packet.data, (UINT32)pDMInfo->acs_Config.nPortIndex,
       MAX_BUFF_SIZE, bStartSnap);

   pMsgBuf->packet.checksum = ChecksumBuffer(pMsgBuf->packet.data, nCnt, 0xFFFFFFFF);

   // Snapshot data received finish the packet
   pMsgBuf->index += nCnt;
   pMsgBuf->status = DM_BUF_BUILD;

}

/******************************************************************************
 * Function:    DataMgrBuildPacketHdr
 *
 * Description: The Data Manager build header will create the header needed
 *              for each packet.
 *
 * Parameters:  DATA_MSG_BUFFER *pMsgBuf - Buffer to store the header in
 *              UINT16          nChannel - Channel being recorded
 *              DM_PACKET_TYPE  eType    - packet type
 *
 * Returns:     None.
 *
 * Notes:       None.
 *
 *****************************************************************************/
static void DataMgrBuildPacketHdr( DATA_MNG_BUFFER *pMsgBuf,
                                   UINT16 nChannel, DM_PACKET_TYPE eType)
{
   // Record the packet header info, init checksum and size
   pMsgBuf->packet.checksum   = 0;
   pMsgBuf->packet.timeStamp  = pMsgBuf->packetTs;
   pMsgBuf->packet.channel    = nChannel;
   pMsgBuf->packet.packetType = (UINT16)eType;
   pMsgBuf->packet.size       = 0;
   pMsgBuf->index             = 0;
}

/******************************************************************************
 * Function:    DataMgrFinishPacket
 *
 * Description: The Data Manager Finish Packet function is used to close
 *              out a packet. It sets the size in the earlier created header,
 *              calculates the checksum, Updates the buffer status to STORE,
 *              and switches to the other buffer.
 *
 * Parameters:  DATA_MNG_INFO *pDMInfo - Pointer to information about the
 *                                       packet buffers.
 *              UINT32        nChannel - Channel recorded number
 *              INT32         recordInfo - Data Bytes not written to the current buffer
 *
 * Returns:     None.
 *
 * Notes:       None.
 *
 *****************************************************************************/
static void DataMgrFinishPacket( DATA_MNG_INFO *pDMInfo, UINT32 nChannel,
                                 INT32 recordInfo)
{
   // Local Data
   DATA_MNG_BUFFER *pMsgBuf;

   // Initialize Local Data
   pMsgBuf  = &pDMInfo->msgBuf[pDMInfo->nCurrentBuffer];

   if ( DM_BUF_BUILD == pMsgBuf->status )
   {
      // Put size in buffer
      pMsgBuf->packet.size = pMsgBuf->index;

      // Since we leveled the checksum loading by calculating it as the data comes in
      // now we need to add in the header elements
      pMsgBuf->packet.checksum += ChecksumBuffer(
                                    &pMsgBuf->packet.timeStamp,
                                    &pMsgBuf->packet.data[0] -
                                    (UINT8 *)(&pMsgBuf->packet.timeStamp),
                                    0xFFFFFFFF);

      // Mark the buffer as store
      pMsgBuf->status = DM_BUF_STORE;

      // display debug - indicating we just recorded the channel
      GSE_StatusStr( VERBOSE, "DM:%d %5d %3d %2d",
        nChannel, pMsgBuf->index, recordInfo, pDMInfo->nCurrentBuffer);

      // Next buffer
      ++pDMInfo->nCurrentBuffer;
      if (pDMInfo->nCurrentBuffer >= MAX_DM_BUFFERS)
      {
        pDMInfo->nCurrentBuffer = 0;   // reset to beginning
      }
   }
}

/******************************************************************************
 * Function:    DataMgrProcBuffers
 *
 * Description: The Data Manager Process Buffers is responsible for initiating
 *              the Log Writes when a buffer is ready to be stored to flash.
 *              Once the write is initiated this function will monitor the
 *              write for completion. When the write completes the function
 *              mark the buffer written as EMPTY
 *
 * Parameters:  DATA_MNG_INFO *pDMInfo   - Information about the data manager
 *                                         storage buffers
 *              UINT32        nChannel   - Channel being recorded
 *
 * Returns:     Active status TRUE - Buffer(s) pending write to FLASH
 *                            FALSE - All Buffers written to FLASH
 *
 * Notes:       None.
 *
 *****************************************************************************/
static
BOOLEAN DataMgrProcBuffers(DATA_MNG_INFO *pDMInfo, UINT32 nChannel)
{
   // Local Data
   UINT32       i;
   LOG_SOURCE   tempChannel;
   BOOLEAN      active = FALSE;

   tempChannel.nNumber = nChannel;

   // Process packet buffers
   for ( i = 0; i < MAX_DM_BUFFERS; i++)
   {
      // Check that current buffer is ready to transmit
      if (DM_BUF_STORE == pDMInfo->msgBuf[i].status)
      {
         active = TRUE;
         // Has the Current buffer been queued for storage yet?
         if (LOG_REQ_NULL == pDMInfo->msgBuf[i].wrStatus)
         {
            // For channels that download we need to keep track of the status
            *(pDMInfo->msgBuf[i].pDL_Status) = DL_WRITE_IN_PROGRESS;
            // Attempt to store packet in Data Flash
            // Add the checksum's checksum to the checksum for Log Write
            LogWrite ( LOG_TYPE_DATA,
                       tempChannel,
                       pDMInfo->acs_Config.priority,
                       (void*)&pDMInfo->msgBuf[i].packet,
                       DM_PACKET_SIZE(pDMInfo->msgBuf[i].packet.size),
                       ( pDMInfo->msgBuf[i].packet.checksum +
                        ChecksumBuffer(&pDMInfo->msgBuf[i].packet.checksum,
                        sizeof(UINT32), LOG_CHKSUM_BLANK)),
                       &pDMInfo->msgBuf[i].wrStatus );

            // display debug - indicating we just recorded the channel
            GSE_StatusStr ( VERBOSE, " Q%d:%d @ %d"NL, nChannel, i, CM_GetTickCount());
         }
         // Has the buffer completed storage yet?
         else if (LOG_REQ_COMPLETE == pDMInfo->msgBuf[i].wrStatus )
         {
            *(pDMInfo->msgBuf[i].pDL_Status) = DL_WRITE_SUCCESS;
            pDMInfo->msgBuf[i].status        = DM_BUF_EMPTY;
            pDMInfo->msgBuf[i].tryTime_mS    = 0;
            pDMInfo->msgBuf[i].wrStatus      = LOG_REQ_NULL;
         }
         else if ( LOG_REQ_FAILED == pDMInfo->msgBuf[i].wrStatus )
         {
            *(pDMInfo->msgBuf[i].pDL_Status) = DL_WRITE_FAIL;
            pDMInfo->msgBuf[i].status        = DM_BUF_EMPTY;
            pDMInfo->msgBuf[i].tryTime_mS    = 0;
            pDMInfo->msgBuf[i].wrStatus      = LOG_REQ_NULL;
         }
      }
   }

  return active;
}

/******************************************************************************
 * Function:    DataMgrWriteBuffer
 *
 * Description: The Data Manager Write Buffer function is responsible for
 *              managing the writes to the buffers. This function will verify
 *              that the buffer will not overflow before performing any write
 *              action.
 *
 * Parameters:  [out] *pMsgBuf - Data Manager Write Buffer
 *              [in]  *pSrc   - Source of data
 *              [in]  nBytes  - Number of bytes to copy
 *
 * Returns:     Number of bytes left to write
 *
 * Notes:
 *
 *****************************************************************************/
static UINT16 DataMgrWriteBuffer(DATA_MNG_BUFFER *pMsgBuf, UINT8 *pSrc, UINT16 nBytes)
{
   // Local Data
   UINT16 nAvailable;
   UINT16 nRemainingToWrite;

   // Initialize Local Data
   nRemainingToWrite = 0;

   // nAvailable = Size of Buffer - Current Index
   nAvailable =  MAX_BUFF_SIZE - pMsgBuf->index;

   // Check if more space available than needed
   if (nAvailable > nBytes)
   {
      // Copy the data that was received to the buffer
      memcpy (&pMsgBuf->packet.data[pMsgBuf->index], pSrc, nBytes);

      // Checksum the Buffer here to help level out the loading
      pMsgBuf->packet.checksum += ChecksumBuffer(pSrc, nBytes, 0xFFFFFFFF);

      // Update the buffer index
      pMsgBuf->index += nBytes;
   }
   else // Buffer will overflow, because we don't know the format of the data
        // in the temporary buffer, we should just return that we have the bytes
        // remaining to write. This will force the software to write them in the 
        // next packet.
   {
      // Copy as much as possible
      // memcpy (&pMsgBuf->packet.data[pMsgBuf->Index], pSrc, nAvailable);

      // Checksum the buffer here to help level out the loading
      // pMsgBuf->packet.checksum += ChecksumBuffer(pSrc, nAvailable, 0xFFFFFFFF);      

      // Update the buffer index
      // pMsgBuf->Index += nAvailable;
      // Calculate the remaining to be written
      // nRemainingToWrite = nBytes - nAvailable;
      nRemainingToWrite = nBytes;
   }

   // Return the number of bytes left to write
   return nRemainingToWrite;
}

/******************************************************************************
 * Function:    DataMgrInitChannels
 *
 * Description: Initialize Data Manager Channel Process Tasks
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:
 *
 *****************************************************************************/
static
void DataMgrInitChannels(void)
{
  // Local Data
  UINT32 i;
  ACS_CONFIG *pACSConfig;
  DATATASK   fpTask;
  BOOLEAN    bEnable;
  UINT32     nMIFframes;
  TASK_INDEX taskId;

  for (i = 0; i < MAX_ACS_CONFIG; i++)
  {
    pACSConfig = &(CfgMgr_RuntimeConfigPtr()->ACSConfigs[i]);
    taskId     = (TASK_INDEX)(i + (UINT32)Data_Mgr0);

    /*Override ACS priority for Full Flight Data sources if Full Flight Data mode is enabled
    Full Flight Data inhibit.  This modifies the priority in the configuration memory before
    it is copied to the DataManager local configuration.  If the configuration is changed
    at runtime, it will not change DataManager local configuration. */
    if( DataMgrIsPortFFD(pACSConfig) && DataMgrIsFFDInhibtMode( ) )
    {
       pACSConfig->priority = LOG_PRIORITY_4;
    }

    if ((pACSConfig->mode == ACS_DOWNLOAD) && (pACSConfig->portType != ACS_PORT_NONE))
    {
       // Configure the ACS Download Task
       fpTask     = DataMgrDownloadTask;
       bEnable   = FALSE;
       nMIFframes = 0xFFFFFFFF;
    }
    else
    {
       fpTask     = DataMgrTask;
       bEnable   = TRUE;
       nMIFframes = taskInfo[taskId].MIFframes;
    }

    switch (pACSConfig->portType)
    {
       case ACS_PORT_NONE:
          // Do Nothing
          break;

       case ACS_PORT_ARINC429:
          DataMgrCreateTask( i,                                  // ACS Index
                             pACSConfig,
                             Arinc429MgrReadFilteredRaw,         // func to read the data
                             Arinc429MgrReadFilteredRawSnap,     // func to read the snapshot
                             Arinc429MgrSyncTime,                // Time Sync Function
                             Arinc429MgrGetFileHdr,
                             fpTask, nMIFframes,
                             bEnable );                          // Task Enable
          break;

       case ACS_PORT_QAR:
          DataMgrCreateTask( i,                           // ACS index
                             pACSConfig,
                             QAR_GetSubFrame,             // QAR func to get sub-frame
                             QAR_GetFrameSnapShot,        // QAR func to get full frame
                             QAR_SyncTime,                // Time Sync Function
                             QAR_GetFileHdr,
                             fpTask, nMIFframes,
                             bEnable );                      // Task Enable
          break;

       case ACS_PORT_UART:
          DataMgrCreateTask( i,                           // ACS Index
                             pACSConfig,
                             UartMgr_ReadBuffer,          // func to read the data
                             UartMgr_ReadBufferSnapshot,  // func to read the snapshot
                             UartMgr_ResetBufferTimeCnt,  // Time Sync Function
                             UartMgr_GetFileHdr,
                             fpTask, nMIFframes,
                             bEnable );                      // Task Enable
          break;

       case ACS_PORT_MAX:
       default:
          FATAL ("Unexpected ACS_PORT_TYPE = % d", pACSConfig->portType);
          break;
    }
  }
}

/******************************************************************************
 * Function:    DataMgrBuildFailLog
 *
 * Description: Build Data Manager Fail Log
 *
 * Parameters:  [in]  Channel  - Channel Number
 *              [in]  *pDMInfo - Data Manager Info struct
 *              [out] *pFailLog - Fail Log Buffer
 *
 * Returns:     Size of Fail Log Struct
 *
 * Notes:
 *
 *****************************************************************************/
static
UINT16 DataMgrBuildFailLog( UINT32 Channel, DATA_MNG_INFO *pDMInfo,
                            DM_FAIL_DATA *pFailLog )
{
   CHAR tmpBuf[5];

   strncpy_safe(pFailLog->model, sizeof(pFailLog->model),
     pDMInfo->acs_Config.sModel,_TRUNCATE);
   strncpy_safe(pFailLog->id,sizeof(pFailLog->id),pDMInfo->acs_Config.sID,_TRUNCATE);

   pFailLog->nChannel = Channel;

   strncpy_safe(pFailLog->recordStatus,   sizeof(pFailLog->recordStatus),
                recordStatusType[pDMInfo->recordStatus].Str,_TRUNCATE);

   snprintf(tmpBuf, sizeof(tmpBuf), "%02d", pDMInfo->nCurrentBuffer);
   strncpy_safe(pFailLog->currentBuffer,  sizeof(pFailLog->currentBuffer),
                tmpBuf, _TRUNCATE);

   strncpy_safe(pFailLog->bufferStatus,  sizeof(pFailLog->bufferStatus),
                bufferStatusType[pDMInfo->msgBuf[pDMInfo->nCurrentBuffer].status].Str,
                _TRUNCATE);

   strncpy_safe(pFailLog->writeStatus,   sizeof(pFailLog->writeStatus),
                logReqType[pDMInfo->msgBuf[pDMInfo->nCurrentBuffer].wrStatus].Str,_TRUNCATE);

   return (UINT16)(sizeof(DM_FAIL_DATA));
}


/******************************************************************************
 * Function:    DataMgrDownloadTask
 *
 * Description: The Data Manager Download Task is responsible for collecting
 *              the data from interface we are downloading and initiating the
 *              writes to the data flash.
 *              The Task will break-up the passed data into 16KByte Data Logs
 *              and initiate the write. The first chunk of a record will be
 *              saved with a DM_DOWNLOADED_RECORD (3) packet type and
 *              subsequent chunks of the same record will be saved with
 *              DM_DOWNLOADED_RECORD_CONTINUE (4) packet type.
 *
 * Parameters:  void *pParam - Pointer to the Task Control Block for the
 *                             channel being recorded.
 *
 * Returns:     None.
 *
 * Notes: The buffers, chunks and processing were designed taking the following
 *        into consideration;
 *
 *        1. Copying the data to a local buffer is going to take
 *           240 nanoSeconds/byte using memcpy.
 *        2. The checksum is going to take 187 nanoSeconds/byte.
 *        3. LogWrite is going to copy the data to a temp buffer which will
 *           also take 240 nanoSeconds / byte.
 *        4. Mem-Copy and Checksum can do about 4.5KBytes in 2 milliseconds.
 *
 *****************************************************************************/
static void DataMgrDownloadTask( void *pParam )
{
   // Local Data
   DATA_MNG_INFO                  *pDMInfo;
   DATA_MNG_BUFFER                *pMsgBuf;
   DATA_MNG_TASK_PARMS            *pDMParam;
   DL_STATUS                      requestStatus;
   TIMESTAMP                      currentTime;
   UINT32                         tempTime;

   // Initialize Local Data
   pDMParam = (DATA_MNG_TASK_PARMS *)pParam;
   pDMInfo  = &dataMgrInfo[pDMParam->nChannel];
   pMsgBuf  = &pDMInfo->msgBuf[pDMInfo->nCurrentBuffer];
   requestStatus = DL_NO_RECORDS;

   // Get the current time for statistics
   CM_GetTimeAsTimestamp (&currentTime);

   // Determine which state we are in
   switch (pDMInfo->dl.state)
   {
      // Request a new record from the interface
      case DL_RECORD_REQUEST:
         // Make sure we haven't been requested to stop before getting the next record
         if (pDMInfo->dl.bDownloadStop == FALSE)
         {
            // If the statistics start time is "0" then initialized the timers for
            // the current record
            if (recordStats[pDMParam->nChannel].nStart_ms == 0)
            {
               recordStats[pDMParam->nChannel].nStart_ms   = CM_GetTickCount();
               recordStats[pDMParam->nChannel].requestTime = currentTime;
            }

            // Request the next record to download from the interface
            pDMInfo->dl.pDataSrc = DataMgrRequestRecord ( pDMInfo->acs_Config.portType,
                                                          pDMInfo->acs_Config.nPortIndex,
                                                          &requestStatus, &pDMInfo->dl.nBytes,
                                                          &pMsgBuf->pDL_Status );
            // Did the interface report no records or no more records to download?
            if ( requestStatus == DL_NO_RECORDS )
            {
               // Advance to the END State for the download processing
               pDMInfo->dl.state = DL_END;
           // Reset the record statistics so they can be reused
               memset (&recordStats[pDMParam->nChannel],0,
                       sizeof(DATA_MNG_DOWNLOAD_RECORD_STATS));
            }
            // Did the interface find a record?
            else if ( requestStatus == DL_RECORD_FOUND )
            {
               // Save the time the record was found
               recordStats[pDMParam->nChannel].foundTime    = currentTime;
               // Calculate the duration for the request
               tempTime = CM_GetTickCount();
               recordStats[pDMParam->nChannel].nDuration_ms = tempTime -
                                                     recordStats[pDMParam->nChannel].nStart_ms;
               // Increment the number of records found on this interface
               pDMInfo->dl.statistics.nRcvd++;
               // Total the number of bytes received
               pDMInfo->dl.statistics.nTotalBytes     += pDMInfo->dl.nBytes;
               // Save the record number for the current record so statistics can be run
               recordStats[pDMParam->nChannel].nNumber = pDMInfo->dl.statistics.nRcvd;
               // Save the number of bytes for the current record so statistics can be run
               recordStats[pDMParam->nChannel].nSize   = pDMInfo->dl.nBytes;
               // Print a debug message about the record being found
               snprintf (tempDebugString, sizeof(tempDebugString), 
                         "\r\nFOUND: %d Bytes: %d\r\n",
                        pDMInfo->dl.statistics.nRcvd, pDMInfo->dl.nBytes);
               GSE_DebugStr(NORMAL,TRUE,tempDebugString);
               // Initiate the record save and change states
               DataMgrDLSaveAndChangeState ( pDMInfo,
                                             DM_DOWNLOADED_RECORD,
                                             (UINT8)pDMParam->nChannel );
            }
         }
         else // Stop Download was detected advance to the END State
         {
            pDMInfo->dl.state = DL_END;
            // Reset the record statistics so they can be reused
            memset (&recordStats[pDMParam->nChannel],0,sizeof(DATA_MNG_DOWNLOAD_RECORD_STATS));
         }
         break;
      // The current record is larger than 16KBytes so we need to continue
      case DL_RECORD_CONTINUE:
         // Initiate the record save and change states
         DataMgrDLSaveAndChangeState ( pDMInfo,
                                       DM_DOWNLOADED_RECORD_CONTINUED,
                                       (UINT8)pDMParam->nChannel );
         break;
      // A stop download has been requested or we've completed the download
      case DL_END:
        {
          UINT32 i;
          BOOLEAN bufsEmpty = TRUE;

           // Make sure all buffers are empty before killing the task
           // otherwise the last buffers of data will never get written
          for (i = 0; bufsEmpty && (i < MAX_DM_BUFFERS); ++i)
          {
            bufsEmpty = pDMInfo->msgBuf[i].status == DM_BUF_EMPTY;
          }

          if ( bufsEmpty )
          {
            // Debug message that the download has ended
            snprintf (tempDebugString, sizeof(tempDebugString), "\rEND: Total Records %d",
                     pDMInfo->dl.statistics.nRcvd);
            GSE_StatusStr(NORMAL,tempDebugString);
            // There are no more records to download on this channel reset flag
            // Kill Task, save record statistics and wait until next download request
            // from FAST Manager
            TmTaskEnable((TASK_INDEX)(pDMParam->nChannel + (UINT16)Data_Mgr0), FALSE);
            pDMInfo->dl.statistics.nChannel     = pDMParam->nChannel;
            pDMInfo->dl.statistics.endTime      = currentTime;
            pDMInfo->dl.statistics.nDuration_ms = CM_GetTickCount() -
                                                  pDMInfo->dl.statistics.nStart_ms;

            LogWriteSystem(APP_DM_DOWNLOAD_STATISTICS, LOG_PRIORITY_LOW,
                           &pDMInfo->dl.statistics, sizeof(pDMInfo->dl.statistics), NULL);

            pDMInfo->dl.state         = DL_RECORD_REQUEST;
            pDMInfo->dl.bDownloading  = FALSE;
            pDMInfo->dl.bDownloadStop = FALSE;

            // Decrement count for this task's download ending.
            CM_UpdateRecordingState(FALSE);
          }
        }
        break;
      default:
        FATAL("DM_Download Bad State %d", pDMInfo->dl.state);
        break;
   }
   // Check if we have data waiting to be written to the data flash
   DataMgrProcBuffers( pDMInfo, pDMParam->nChannel );
}

/******************************************************************************
 * Function:    DataMgrDLSaveAndChangeState
 *
 * Description: The DataMgrDLSaveAndChangeState function will initiate the
 *              downloaded record save and determine the next state to enter.
 *
 * Parameters:  DATA_MNG_INFO  *pDMInfo  - Information about the channel being processed
 *              DM_PACKET_TYPE  Type     - Type of record to write
 *              UINT8           nChannel - Configured ACS Channel
 *
 * Returns:     None.
 *
 * Notes:
 *
 *****************************************************************************/
static
void DataMgrDLSaveAndChangeState(DATA_MNG_INFO *pDMInfo, DM_PACKET_TYPE Type, UINT8 nChannel)
{
   // This ASSERT should never occur because the Interface is throttling the
   // Data Manager and only allowing one record at a time to be saved to data
   // flash before continuing to the next record
   ASSERT_MESSAGE(DM_BUF_EMPTY == pDMInfo->msgBuf[pDMInfo->nCurrentBuffer].status,
                  "Channel %d Buffers Full", nChannel);

   // Send out a debug message about the save
   snprintf (tempDebugString, sizeof(tempDebugString), "\rCOPY: %d Bytes: %d Buf: %d",
            pDMInfo->dl.statistics.nRcvd, pDMInfo->dl.nBytes, pDMInfo->nCurrentBuffer);
   GSE_StatusStr(NORMAL,tempDebugString);

   // Build the first packet to store to the data flash
   pDMInfo->dl.nBytes = DataMgrSaveDLRecord ( pDMInfo, nChannel, Type );

   // If the entire downloaded record fits in one packet the next state
   // will be to REQUEST another downloaded record
   if ( 0 == pDMInfo->dl.nBytes )
   {
      pDMInfo->dl.state = DL_RECORD_REQUEST;

      // Send out a debug message that the current request completed
      snprintf (tempDebugString, sizeof(tempDebugString), "\rDONE: %d", 
                pDMInfo->dl.statistics.nRcvd);
      GSE_StatusStr(NORMAL,tempDebugString);

      // Update the statistics using the current record
      DataMgrDLUpdateStatistics ( pDMInfo, &recordStats[nChannel] );
      // Reset the record statistics so they can be reused
      memset (&recordStats[nChannel],0,sizeof(DATA_MNG_DOWNLOAD_RECORD_STATS));
   }
   else // The downloaded record is larger than the maximum we can process
        // in one chunk. Next state is CONTINUE with the record
   {
      pDMInfo->dl.state = DL_RECORD_CONTINUE;
      // Print out debug message about continuing with current record
      snprintf (tempDebugString, sizeof(tempDebugString),"\rCONT: %d Bytes: %d Buf: %d",
               pDMInfo->dl.statistics.nRcvd, pDMInfo->dl.nBytes, pDMInfo->nCurrentBuffer);
      GSE_StatusStr(NORMAL,tempDebugString);
   }
}

/******************************************************************************
 * Function:    DataMgrRequestRecord
 *
 * Description: The Data Manager Request Record is reponsible for requesting
 *              each of the configured interfaces to download the attached
 *              device.
 *
 * Parameters:   ACS_PORT_TYPE    PortType      - Configured Port Type
 *               UINT8            PortIndex     - Configured Index
 *               DL_STATUS       *pStatus       - Status from the interface
 *               UINT32          *pSize         - Size of the data in bytes found
 *               DL_WRITE_STATUS **pDL_WrStatus  - Status of the write once data is found
 *
 * Returns:      Location of the data found
 *
 * Notes:        None.
 *
 *****************************************************************************/
static UINT8* DataMgrRequestRecord ( ACS_PORT_TYPE PortType, ACS_PORT_INDEX PortIndex,
                                     DL_STATUS *pStatus, UINT32 *pSize,
                                     DL_WRITE_STATUS **pDL_WrStatus )
{
   // Local Data
   UINT8     *pSrc;

   switch (PortType)
   {
      case ACS_PORT_UART:
         pSrc = UartMgr_DownloadRecord ( (UINT8)PortIndex, pStatus, pSize, pDL_WrStatus );
         break;

      case ACS_PORT_NONE:
      case ACS_PORT_ARINC429:
      case ACS_PORT_QAR:
      case ACS_PORT_MAX:
      default:
         *pStatus = DL_NO_RECORDS;
         pSrc     = &noData;
         *pSize   = 0;
         break;
   }
   return pSrc;
}


/******************************************************************************
 * Function:    DataMgrSaveDLRecord
 *
 * Description: The Data Manager Save Download Record builds the packets in
 *              buffers. It builds the packet header and appends the data.
 *              This function is also responsible for chunking up the records.
 *
 * Parameters:  DATA_MNG_INFO *pDMInfo    - Information about the ACS being downloaded
 *              UINT8          nChannel   - Configured Channel Number
 *              DM_PACKET_TYPE PacketType - DM_DOWNLOADED_RECORD (3)
 *                                          DM_DOWNLOADED_RECORD_CONTINUED (4)
 *
 * Returns:     Remaining bytes to write if the record is larger than 16KBytes
 *
 * Notes:       None.
 *
 *****************************************************************************/
static UINT32 DataMgrSaveDLRecord ( DATA_MNG_INFO *pDMInfo, UINT8 nChannel,
                                    DM_PACKET_TYPE PacketType )
{
   // Local Data
   DATA_MNG_BUFFER  *pMsgBuf;
   UINT32           nRemaining;

   // Initialize Local Data
   pMsgBuf  = &pDMInfo->msgBuf[pDMInfo->nCurrentBuffer];

   // Calling function already checked that the buffer was ready to be used and
   // ASSERTS if the buffer isn't empty. Not possible to get here if we don't
   // already have an empty buffer

   // We have a buffer now mark the status as build
   pMsgBuf->status = DM_BUF_BUILD;

   // Get the time for the current record
   CM_GetTimeAsTimestamp (&(pMsgBuf->packetTs));
   // Create the packet header
   DataMgrBuildPacketHdr( pMsgBuf, nChannel, PacketType);

   // Check if the record is less than the theoretical maximum
   if ( pDMInfo->dl.nBytes < MAX_DOWNLOAD_CHUNK )
   {
      // Write the record to the buffer
      memcpy (&pMsgBuf->packet.data[pMsgBuf->index],
              pDMInfo->dl.pDataSrc, pDMInfo->dl.nBytes);

      // Checksum the Buffer here to help level out the loading
      pMsgBuf->packet.checksum += ChecksumBuffer(pDMInfo->dl.pDataSrc, pDMInfo->dl.nBytes,
                                                 0xFFFFFFFF);

      // Update the buffer index
      pMsgBuf->index += (UINT16)pDMInfo->dl.nBytes;
      // Record fit into on packet non bytes remain
      nRemaining = 0;
   }
   else // Larger than the maximum, write the maximum and then goto the continue state
   {
      // Write the record to the buffer
      memcpy (&pMsgBuf->packet.data[pMsgBuf->index],pDMInfo->dl.pDataSrc,MAX_DOWNLOAD_CHUNK);

      // Checksum the Buffer here to help level out the loading
      pMsgBuf->packet.checksum += ChecksumBuffer(pDMInfo->dl.pDataSrc,
                                                 MAX_DOWNLOAD_CHUNK, 0xFFFFFFFF);

      // Update the buffer index
      pMsgBuf->index += MAX_DOWNLOAD_CHUNK;
      // Move the pointer to the data
      pDMInfo->dl.pDataSrc += MAX_DOWNLOAD_CHUNK;
      // Calculate how many bytes remain
      nRemaining            = pDMInfo->dl.nBytes - MAX_DOWNLOAD_CHUNK;

   }
   // Set the write status to in-progress
   *(pDMInfo->msgBuf[pDMInfo->nCurrentBuffer].pDL_Status) = DL_WRITE_IN_PROGRESS;
   // Finish the Packet by completing the checksum and going to the store state
   DataMgrFinishPacket( pDMInfo, nChannel, DM_BUF_BUILD_STATUS );

   return nRemaining;
}

/******************************************************************************
 * Function:    DataMgrDLUpdateStatistics
 *
 * Description: The Data Manager Download Update Statistics function is run
 *              after each new record is found. This function looks for the
 *              Largest Record, the Smallest Record, the Longest Request and
 *              the shortest request. This data is then saved to a system log
 *              at the completion of the download.
 *
 * Parameters:  DATA_MNG_INFO *pDMInfo  - Information about the ACS being downloaded
 *              DATA_MNG_DOWNLOAD_RECORD_STATS *pStats - Pointer to the stats
 *                                                       for the current record
 * Returns:     None.
 *
 * Notes:       None.
 *
 *****************************************************************************/
static void DataMgrDLUpdateStatistics ( DATA_MNG_INFO *pDMInfo,
                                         DATA_MNG_DOWNLOAD_RECORD_STATS *pStats )
{
   // Check for the largest size
   if ( pStats->nSize > pDMInfo->dl.statistics.largestRecord.nSize )
   {
      pDMInfo->dl.statistics.largestRecord = *pStats;
   }
   // Check for the smallest size
   if ( pStats->nSize < pDMInfo->dl.statistics.smallestRecord.nSize )
   {
      pDMInfo->dl.statistics.smallestRecord = *pStats;
   }
   // Check for longest request
   if( pStats->nDuration_ms > pDMInfo->dl.statistics.longestRequest.nDuration_ms )
   {
      pDMInfo->dl.statistics.longestRequest = *pStats;
   }
   // Check for shortest request
   if ( pStats->nDuration_ms < pDMInfo->dl.statistics.shortestRequest.nDuration_ms )
   {
      pDMInfo->dl.statistics.shortestRequest = *pStats;
   }
}

/******************************************************************************
 * Function:    DataMgrIsPortFFD
 *
 * Description: Examines the ACS port configuration and determines if the port
 *              passed in is considered "Full Flight Data"  The rules for
 *              determining full flight data are:
 *              if ACS_PORT_TYPE is:
 *                   ARINC 429
 *                   ARINC 717(QAR)
 *              if ACS_PORT_TYPE is UART and ACS_MODE is RECORD
 *
 *              429 and 717 are also currently used only in RECORD mode, so
 *              this field could be used to determine FFD instead.  The intent
 *              of this method is to allow more complex rules if they are
 *              necessary in the future.
 *
 * Parameters:  [in] ACS_Config: Structure containg the ACS port configuration
 *                               in question
 *
 * Returns:     BOOLEAN - [ TRUE  - The ACS is a "Full Flight" data source,
 *                          FALSE  - The ACS is not a"Full Flight" data source]
 *
 * Notes:
 *
 *****************************************************************************/
static
BOOLEAN DataMgrIsPortFFD( const ACS_CONFIG *ACS_Config )
{
  BOOLEAN retval = FALSE;


  if( ACS_Config->mode == ACS_RECORD )
  {
    retval = TRUE;
  }

  return retval;
}

/******************************************************************************
 * Function:    DataMgrIsFFDInhibtMode
 *
 * Description: Return the status of the Full Flight Data Offload Inhibit
 *              strap.  When the strap is installed, the box cannot offload
 *              data from sources classified as "Full Flight"
 *
 * Parameters:  None.
 *
 * Returns:     BOOLEAN - [ TRUE  - FFDI Strap is installed,
 *                          FALSE - FFDI Strap is not installed ]
 *
 * Notes:
 *
 *****************************************************************************/
static
BOOLEAN DataMgrIsFFDInhibtMode( void )
{
  return DIO_ReadPin(FFD_Enb) ? FALSE : TRUE;
}


/*************************************************************************
 *  MODIFICATIONS
 *    $History: DataManager.c $
 * 
 * *****************  Version 99  *****************
 * User: John Omalley Date: 3/09/15    Time: 10:55a
 * Updated in $/software/control processor/code/system
 * SCR 1255 - Updated Port Index to an enumeration to handle 0,1,2,3,10
 * 
 * *****************  Version 98  *****************
 * User: John Omalley Date: 11/18/14   Time: 1:45p
 * Updated in $/software/control processor/code/system
 * SCR 1251 - Uninitialized pointer used for DL write status when
 * recording data
 * 
 * *****************  Version 97  *****************
 * User: John Omalley Date: 11/13/14   Time: 10:40a
 * Updated in $/software/control processor/code/system
 * SCR 1251 - Fixed Data Manager DL Write status location
 * 
 * *****************  Version 96  *****************
 * User: Contractor V&v Date: 10/01/14   Time: 3:24p
 * Updated in $/software/control processor/code/system
 * SCR #1164 - CP Time Synce when Not Recording
 * 
 * *****************  Version 95  *****************
 * User: Contractor V&v Date: 9/04/14    Time: 3:11p
 * Updated in $/software/control processor/code/system
 * SCR #1164 - Permit CP Time Syncing only when Not Recording -Forgot to
 * remove call.
 * 
 * *****************  Version 94  *****************
 * User: Contractor V&v Date: 9/04/14    Time: 2:45p
 * Updated in $/software/control processor/code/system
 * SCR #1164 - Permit CP Time Syncing only when Not Recording - Add
 * download constraint.
 * 
 * *****************  Version 93  *****************
 * User: John Omalley Date: 4/01/14    Time: 9:46a
 * Updated in $/software/control processor/code/system
 * SCR 1250 - Reset Statistics when the download has completed or is
 * stopped.
 * 
 * *****************  Version 92  *****************
 * User: John Omalley Date: 12-12-06   Time: 4:29p
 * Updated in $/software/control processor/code/system
 * SCR 1197 - Code Review Update
 * 
 * *****************  Version 91  *****************
 * User: John Omalley Date: 12-12-01   Time: 10:09a
 * Updated in $/software/control processor/code/system
 * SCR 1116
 * 
 * *****************  Version 90  *****************
 * User: John Omalley Date: 12-11-28   Time: 4:50p
 * Updated in $/software/control processor/code/system
 * SCR 1197 - Code Review Updates
 * 
 * *****************  Version 89  *****************
 * User: John Omalley Date: 12-11-28   Time: 2:40p
 * Updated in $/software/control processor/code/system
 * SCR 1116 - Fix Buffer Overflow Issue
 * 
 * *****************  Version 88  *****************
 * User: Jim Mood     Date: 11/27/12   Time: 5:33p
 * Updated in $/software/control processor/code/system
 * SCR# 1131 Modfix for error indication record not active when dm buffers
 * were empty.
 *
 * *****************  Version 87  *****************
 * User: Jim Mood     Date: 11/19/12   Time: 7:55p
 * Updated in $/software/control processor/code/system
 * SCR 1131 Modfix for app busy when no ACSs are configured
 *
 * *****************  Version 86  *****************
 * User: John Omalley Date: 12-11-16   Time: 8:12p
 * Updated in $/software/control processor/code/system
 * SCR 1087 - Code Review Updates
 *
 * *****************  Version 85  *****************
 * User: John Omalley Date: 12-11-13   Time: 11:10a
 * Updated in $/software/control processor/code/system
 * SCR 1142 - Code Review Formatting Issues
 *
 * *****************  Version 84  *****************
 * User: Jim Mood     Date: 11/09/12   Time: 6:50p
 * Updated in $/software/control processor/code/system
 * SCR 1131 Recording busy status
 *
 * *****************  Version 83  *****************
 * User: John Omalley Date: 12-11-09   Time: 4:10p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review Update
 *
 * *****************  Version 82  *****************
 * User: John Omalley Date: 12-11-09   Time: 10:34a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review Update
 *
 * *****************  Version 81  *****************
 * User: Melanie Jutras Date: 12-10-23   Time: 1:08p
 * Updated in $/software/control processor/code/system
 * SCR #1159 Changed FFD_Inh to FFD_Enb in DataMgrIsFFDInhibitMode()
 * function because it had been changed in DIO.h for this SCR.
 *
 * *****************  Version 80  *****************
 * User: Melanie Jutras Date: 12-10-19   Time: 1:28p
 * Updated in $/software/control processor/code/system
 * SCR #1172 PCLint 545 Suspicious use of & Error
 *
 * *****************  Version 79  *****************
 * User: Melanie Jutras Date: 12-10-10   Time: 1:53p
 * Updated in $/software/control processor/code/system
 * SCR 1172 PCLint 545 Suspicious use of & Error
 *
 * *****************  Version 78  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 7:08p
 * Updated in $/software/control processor/code/system
 * SCR# 1142 - Windows Emulation Update and Typos
 *
 * *****************  Version 77  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 76  *****************
 * User: Jim Mood     Date: 7/19/12    Time: 11:07a
 * Updated in $/software/control processor/code/system
 * SCR 1107: Data Offload changes for 2.0.0
 *
 * *****************  Version 75  *****************
 * User: John Omalley Date: 10/27/11   Time: 8:57a
 * Updated in $/software/control processor/code/system
 * SCR 1097 - Added FATAL to case statement default
 *
 * *****************  Version 74  *****************
 * User: John Omalley Date: 10/26/11   Time: 6:44p
 * Updated in $/software/control processor/code/system
 * SCR 1097 - Dead Code Update
 *
 * *****************  Version 73  *****************
 * User: John Omalley Date: 10/11/11   Time: 4:18p
 * Updated in $/software/control processor/code/system
 * SCR 1076 and SCR 1078 - Code Review Updates
 *
 * *****************  Version 72  *****************
 * User: John Omalley Date: 9/15/11    Time: 12:10p
 * Updated in $/software/control processor/code/system
 * SCR 1073 Wrapper on the Start Download Function for already downloading
 * or uploading. Removed wrapper from User Command and used the common
 * Start Download Function.
 *
 * *****************  Version 71  *****************
 * User: John Omalley Date: 9/12/11    Time: 3:12p
 * Updated in $/software/control processor/code/system
 * SCR 1073
 *
 * *****************  Version 70  *****************
 * User: John Omalley Date: 9/12/11    Time: 2:14p
 * Updated in $/software/control processor/code/system
 * SCR 1073 - Made Download Stop Function Global
 *
 * *****************  Version 69  *****************
 * User: Jeff Vahue   Date: 9/01/11    Time: 12:17p
 * Updated in $/software/control processor/code/system
 * SCR# 1021 - Init all DM status buffers not just the first two.
 *
 * *****************  Version 68  *****************
 * User: Contractor2  Date: 8/15/11    Time: 1:47p
 * Updated in $/software/control processor/code/system
 * SCR #1021 Error: Data Manager Buffers Full Fault Detected
 *
 * *****************  Version 67  *****************
 * User: Jim Mood     Date: 8/03/11    Time: 2:08p
 * Updated in $/software/control processor/code/system
 * SCR 1029 Informal Review changes
 *
 * *****************  Version 66  *****************
 * User: Jim Mood     Date: 7/20/11    Time: 10:57a
 * Updated in $/software/control processor/code/system
 * SCR 575: GSM Enable when engine status is lost.  (Part of changes for
 * the Fast State Machine)
 *
 * *****************  Version 65  *****************
 * User: John Omalley Date: 6/21/11    Time: 10:33a
 * Updated in $/software/control processor/code/system
 * SCR 1029 - Added User command to stop download per the Preliminary
 * Software Design Review.
 *
 * *****************  Version 64  *****************
 * User: Contractor2  Date: 5/10/11    Time: 1:26p
 * Updated in $/software/control processor/code/system
 * SCR #980 Enhancement: UART GetHeader Function
 *
 * *****************  Version 63  *****************
 * User: John Omalley Date: 4/20/11    Time: 8:17a
 * Updated in $/software/control processor/code/system
 * SCR 1029
 *
 * *****************  Version 62  *****************
 * User: John Omalley Date: 4/11/11    Time: 10:07a
 * Updated in $/software/control processor/code/system
 * SCR 1029 - Added EMU Download logic to the Data Manager
 *
 * *****************  Version 61  *****************
 * User: John Omalley Date: 11/05/10   Time: 11:16a
 * Updated in $/software/control processor/code/system
 * SCR 985 - Data Manager Code Coverage Updates
 *
 * *****************  Version 60  *****************
 * User: John Omalley Date: 10/21/10   Time: 1:45p
 * Updated in $/software/control processor/code/system
 * SCR 960 - Calculate the checksum as the data comes in.
 *
 * *****************  Version 59  *****************
 * User: Contractor2  Date: 10/14/10   Time: 4:42p
 * Updated in $/software/control processor/code/system
 * SCR #931 Code Review: DataManager
 * Removed TODOs
 *
 * *****************  Version 58  *****************
 * User: Contractor2  Date: 10/14/10   Time: 2:16p
 * Updated in $/software/control processor/code/system
 * SCR #931 Code Review: DataManager
 *
 * *****************  Version 57  *****************
 * User: Contractor2  Date: 10/07/10   Time: 11:48a
 * Updated in $/software/control processor/code/system
 * SCR #916 Code Review Updates
 *
 * *****************  Version 56  *****************
 * User: John Omalley Date: 9/10/10    Time: 10:01a
 * Updated in $/software/control processor/code/system
 * SCR 858 - Added FATAL to default cases
 *
 * *****************  Version 55  *****************
 * User: John Omalley Date: 8/24/10    Time: 5:34p
 * Updated in $/software/control processor/code/system
 * SCR 821 - Updated logic to record snapshot if there is data or not
 *
 * *****************  Version 54  *****************
 * User: John Omalley Date: 8/03/10    Time: 5:03p
 * Updated in $/software/control processor/code/system
 * SCR 769 - Fixed the Temp Buffer.. Changed back to UINT32
 *
 * *****************  Version 53  *****************
 * User: John Omalley Date: 8/03/10    Time: 2:15p
 * Updated in $/software/control processor/code/system
 * SCR 769 - Updated Buffer Sizes and fixed ARINC429 FIFO Index during
 * initialization.
 *
 * *****************  Version 52  *****************
 * User: John Omalley Date: 7/29/10    Time: 7:56p
 * Updated in $/software/control processor/code/system
 * SCR 755 - Add a check for Buffer set to STORE when recording data
 *
 * *****************  Version 51  *****************
 * User: John Omalley Date: 7/02/10    Time: 10:29a
 * Updated in $/software/control processor/code/system
 * SCR 669 - Check for valid port before trying to create binary data
 * header
 *
 * *****************  Version 50  *****************
 * User: Jim Mood     Date: 6/11/10    Time: 3:31p
 * Updated in $/software/control processor/code/system
 * SCR 623 Batch configuration implementation updates
 *
 * *****************  Version 49  *****************
 * User: John Omalley Date: 6/08/10    Time: 12:13p
 * Updated in $/software/control processor/code/system
 * SCR 627 - Updated for LJ60
 *
 * *****************  Version 48  *****************
 * User: Contractor2  Date: 5/25/10    Time: 3:06p
 * Updated in $/software/control processor/code/system
 * SCR #587 Change TmTaskCreate to return void
 * Set function to return void. Coding standard fixes
 *
 * *****************  Version 47  *****************
 * User: Jeff Vahue   Date: 5/12/10    Time: 6:56p
 * Updated in $/software/control processor/code/system
 * SCR# 587 - correct DataManager not being initialized properly
 *
 * *****************  Version 46  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:54p
 * Updated in $/software/control processor/code/system
 * SCR #587 Change TmTaskCreate to return void
 *
 * *****************  Version 45  *****************
 * User: Contractor V&v Date: 5/07/10    Time: 4:36p
 * Updated in $/software/control processor/code/system
 * SCR #306 Log Manager Errors
 *
 * *****************  Version 44  *****************
 * User: Jeff Vahue   Date: 4/28/10    Time: 5:50p
 * Updated in $/software/control processor/code/system
 * SCR #572 - vcast changes
 *
 * *****************  Version 43  *****************
 * User: Contractor V&v Date: 4/22/10    Time: 6:41p
 * Updated in $/software/control processor/code/system
 * SCR #306 Log Manager Errors
 *
 * *****************  Version 42  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:10p
 * Updated in $/software/control processor/code/system
 * SCR #317 Implement safe strncpy
 *
 * *****************  Version 41  *****************
 * User: Jeff Vahue   Date: 4/07/10    Time: 11:47a
 * Updated in $/software/control processor/code/system
 * SCR #535 - reduce packet type size from 4 bytes to two in Data Logs
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
 * User: Jeff Vahue   Date: 3/10/10    Time: 2:34p
 * Updated in $/software/control processor/code/system
 * SCR# 480 - add semi's after ASSERTs
 *
 * *****************  Version 37  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:57p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 36  *****************
 * User: Jeff Vahue   Date: 2/26/10    Time: 10:21a
 * Updated in $/software/control processor/code/system
 * SCR# 454 - Fix recording on buffer overflow, DMTempBuffer is UINT32,
 * but recording offsets into the buffer were handled as byte offsets
 *
 * *****************  Version 35  *****************
 * User: Jeff Vahue   Date: 2/19/10    Time: 12:58p
 * Updated in $/software/control processor/code/system
 * SCR# 455 - remove LINT issues
 *
 * *****************  Version 34  *****************
 * User: Jeff Vahue   Date: 2/19/10    Time: 11:35a
 * Updated in $/software/control processor/code/system
 * Typo
 *
 * *****************  Version 33  *****************
 * User: Jeff Vahue   Date: 2/18/10    Time: 5:18p
 * Updated in $/software/control processor/code/system
 * SCR #454 - correct size error when recording a buffer overflow
 *
 * *****************  Version 32  *****************
 * User: Jeff Vahue   Date: 2/17/10    Time: 1:42p
 * Updated in $/software/control processor/code/system
 * SCR# 454 - GH compiler warnings fixes
 *
 * *****************  Version 31  *****************
 * User: Jeff Vahue   Date: 2/17/10    Time: 1:24p
 * Updated in $/software/control processor/code/system
 * SCR# 454 - DM buffer overflow mods
 *
 * *****************  Version 30  *****************
 * User: Jeff Vahue   Date: 2/09/10    Time: 5:43p
 * Updated in $/software/control processor/code/system
 * Spelling Typo
 *
 * *****************  Version 29  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 5:05p
 * Updated in $/software/control processor/code/system
 * SCR# 397
 *
 * *****************  Version 28  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 3:34p
 * Updated in $/software/control processor/code/system
 * SCR# 326 cleanup
 *
 * *****************  Version 27  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 2:49p
 * Updated in $/software/control processor/code/system
 * SCR# 326
 *
 * *****************  Version 26  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 2:11p
 * Updated in $/software/control processor/code/system
 * SCR# 326
 *
 * *****************  Version 25  *****************
 * User: Peter Lee    Date: 12/21/09   Time: 6:19p
 * Updated in $/software/control processor/code/system
 * SCR #380 Implement Real Time Sys Condition Logic
 *
 * *****************  Version 24  *****************
 * User: Contractor V&v Date: 12/18/09   Time: 6:55p
 * Updated in $/software/control processor/code/system
 * SCR 31 Error: Pointers to NULL functions
 *
 * *****************  Version 23  *****************
 * User: Contractor V&v Date: 12/10/09   Time: 5:45p
 * Updated in $/software/control processor/code/system
 * SCR 106
 *
 * *****************  Version 22  *****************
 * User: Jeff Vahue   Date: 12/03/09   Time: 4:59p
 * Updated in $/software/control processor/code/system
 * SCR# 359
 *
 * *****************  Version 21  *****************
 * User: Peter Lee    Date: 10/21/09   Time: 2:45p
 * Updated in $/software/control processor/code/system
 * SCR #312 Add msec value to logs
 *
 * *****************  Version 20  *****************
 * User: John Omalley Date: 10/19/09   Time: 1:35p
 * Updated in $/software/control processor/code/system
 * SCR 283
 * - Added logic to record a record a fail log if the buffers are full
 * when data recording is started.
 * - Updated the logic to record an error only once when the full
 * condition is detected.
 *
 * *****************  Version 19  *****************
 * User: Jim Mood     Date: 9/29/09    Time: 11:58a
 * Updated in $/software/control processor/code/system
 * SCR# 178 Updated User Manager Tables
 *
 * *****************  Version 18  *****************
 * User: John Omalley Date: 8/06/09    Time: 3:51p
 * Updated in $/software/control processor/code/system
 * SCR 221
 *    Resized buffers so the QAR Snapshot wasn't truncated.
 *    Updated Calls to the interface reads to pass and return bytes rather
 * than words.
 * SCR 153
 *    Added a break out of the Data Manager loop when the buffers overflow
 * because of a flash backup.
 *    Fixed the logic so that a buffer overflow will end a packet and
 * start a new one on an overflow. (Flash is working)
 * SCR 190
 *    Rescheduled the DataManager tasks to eliminate DT Task overruns.
 *    Moved the buffer swap check.
 *
 * *****************  Version 17  *****************
 * User: Peter Lee    Date: 1/14/09    Time: 5:15p
 * Updated in $/control processor/code/system
 * SCR #131 Arinc429 Updates
 * Removed implicit call from DataMgrInitChannel() to request Arinc429
 * Processing.  Updated to have explicit cfg for Arinc429 Enable.
 *
 * *****************  Version 16  *****************
 * User: Peter Lee    Date: 11/24/08   Time: 2:04p
 * Updated in $/control processor/code/system
 * SCR 97 - Remove Implicit QAR_Enable() call
 *
 * *****************  Version 15  *****************
 * User: Jim Mood     Date: 10/07/08   Time: 5:31p
 * Updated in $/control processor/code/system
 *
 * *****************  Version 14  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 2:50p
 * Updated in $/control processor/code/system
 * SCR #87 Function Prototype
 *
 *
 ***************************************************************************/

