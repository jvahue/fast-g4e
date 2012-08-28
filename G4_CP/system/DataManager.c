#define DATA_MNG_BODY
/******************************************************************************
            Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:        DataManager.c
    
    Description: The data manager contains the functions used to record
                 data from the various interfaces.

    VERSION
      $Revision: 76 $  $Date: 7/19/12 11:07a $ 
    
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
static DATA_MNG_INFO       DataMgrInfo[DATA_MGR_MAX_CHANNELS];
static DATA_MNG_TASK_PARMS DMPBlock   [DATA_MGR_MAX_CHANNELS];
static BOOLEAN             DMRecordEnabled; // Local Flag for recording status
static DL_WRITE_STATUS     DataMgrDLWriteStatus[MAX_DM_BUFFERS][DATA_MGR_MAX_CHANNELS];
static DATA_MNG_DOWNLOAD_RECORD_STATS RecordStats[DATA_MGR_MAX_CHANNELS];
// This needs to be declared as UINT32 to ensure proper alignment
// Internally this is handled as a BYTE buffer
// If you change this to a UINT8 like I did, more than once... Then the QAR
// Read Raw will ASSERT.... DON'T CHANGE THIS AGAIN!!!
static UINT32             DMTempBuffer[MAX_BUFF_SIZE/4];

static ACS_CONFIG ConfigTemp;    //ACS configuration block used for modifying
                                 //cfg values from the UM.
static DATA_MNG_TASK_PARMS    TCBTemp;
static DATA_MNG_INFO          DMInfoTemp;
static DM_DOWNLOAD_STATISTICS ACS_StatusTemp;
static CHAR TempDebugString[512];

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

static void DataMgrProcBuffers   ( DATA_MNG_INFO *pDMInfo, UINT32 nChannel );

static UINT16 DataMgrWriteBuffer ( DATA_MNG_BUFFER *pMsgBuf, UINT8 *pSrc,
                                   UINT16 nBytes );

static UINT16 DataMgrBuildFailLog( UINT32 Channel, DATA_MNG_INFO *pDMInfo, 
                                   DM_FAIL_DATA *pFailLog );
                                   
static void DataMgrDownloadTask( void *pParam ); 

static UINT8* DataMgrRequestRecord ( ACS_PORT_TYPE PortType, UINT8 PortIndex, 
                                     DL_STATUS *pStatus, UINT32 *pSize, 
                                     DL_WRITE_STATUS *pDL_WrStatus );
static UINT32 DataMgrSaveDLRecord ( DATA_MNG_INFO *pDMInfo, UINT8 Channel, 
                                    DM_PACKET_TYPE PacketType );

static void DataMgrDLSaveAndChangeState ( DATA_MNG_INFO *pDMInfo, 
                                          DM_PACKET_TYPE Type, UINT8 nChannel );
static void DataMgrDLUpdateStatistics   ( DATA_MNG_INFO *pDMInfo, 
                                          DATA_MNG_DOWNLOAD_RECORD_STATS *pStats );
                                    

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/
/*****************************************************************************
 * Function:    DataMgrInitialize
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
  
   DMRecordEnabled = FALSE;
   User_AddRootCmd (&RootACSMsg); 
   User_AddRootCmd (&DataMngMsg);
   
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
   ACS_CONFIG TempConfig;
   
   memset(&TempConfig,0,sizeof(TempConfig));
   
   if (ACS_DONT_CARE != ACS)
   {
      TempConfig = DataMgrInfo[ACS].ACS_Config;
   }
   
   return (TempConfig);
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
BOOLEAN DataMgrIsFFDInhibtMode( void )
{ 
  return DIO_ReadPin(FFD_Inh) ? FALSE : TRUE;
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
BOOLEAN DataMgrIsPortFFD( const ACS_CONFIG *ACS_Config )
{
  BOOLEAN retval = FALSE;


  if( (ACS_Config->PortType == ACS_PORT_ARINC429) ||
      (ACS_Config->PortType == ACS_PORT_QAR)      ||
      ((ACS_Config->PortType == ACS_PORT_UART)    &&
       (ACS_Config->Mode == ACS_RECORD       ))      )
  {
    retval = TRUE;
  }
  
  return retval;
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
   DMRecordEnabled = bEnable;
   
   
   if (bEnable == TRUE)
   {
      // Kill any downloads in process
      DataMgrStopDownload();
   }
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
      if ( ACS_PORT_NONE != DataMgrInfo[i].ACS_Config.PortType )
      {
         // The channel is configured so now verify that the 
         // recording for this channel has completed
         if ( DM_RECORD_IDLE == DataMgrInfo[i].RecordStatus )
         {
            // The recording has stopped now loop through the buffers
            // and verify all buffers have been emptied
            for ( j = 0; (j < MAX_DM_BUFFERS) && (bFinished == TRUE); j++ )
            {
               if (DM_BUF_EMPTY != DataMgrInfo[i].MsgBuf[j].Status)
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
   DATA_MNG_HDR        DataHdr;
   CHAR                *pBuffer;
   UINT16              cnt;
   
   cnt = 0;
   
   if (ACS_DONT_CARE != ACS)
   {
      // Initialize Local Data
      pDMParam       = &DMPBlock[ACS];
      pDMInfo        = &DataMgrInfo[pDMParam->nChannel];
   
      // Update protocol file hdr first ! 
      pBuffer = (CHAR *)(pDest + sizeof(DataHdr));  // Move to end of UartMgr File Hdr
   
      if ( (ACS_PORT_NONE != pDMInfo->ACS_Config.PortType) && (pDMParam->GetDataHdr != NULL) )
      {
         cnt = pDMParam->GetDataHdr( pBuffer, pDMInfo->ACS_Config.PortIndex, nMaxByteSize);
      }
   
      // Update Data hdr
      DataHdr.Version = HDR_VERSION; 
      DataHdr.Type    = pDMInfo->ACS_Config.PortType;
  
      // Copy hdr to destination 
      pBuffer = (CHAR *)pDest; 
      memcpy ( pBuffer, &DataHdr, sizeof(DataHdr) ); 
   
      cnt += sizeof(DataHdr);
   }
  
  return (cnt); 
}



/******************************************************************************
 * Function:    DataMgrRecGetState  | IMPLEMENTS GetState() INTERFACE to
 *                                  | FAST STATE MGR
 *  
 * Description: Returns the recording/!recording state of the data manager ACSs
 *
 * Parameters:  INT32 param  - Not used, included to match FSM call signature
 *
 * Returns:     BOOLEAN - TRUE: Record task is running
 *                        FALSE: Record task is inactive
 *
 * Notes:       
 *****************************************************************************/
BOOLEAN DataMgrRecGetState ( INT32 param )
{
  return  !DataMgrRecordFinished();
}



/******************************************************************************
 * Function:    DataMgrRecGetState  | IMPLEMENTS GetState() INTERFACE to
 *                                  | FAST STATE MGR
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
  DMRecordEnabled = Run;
}



/******************************************************************************
 * Function:    DataMgrRecGetState  | IMPLEMENTS GetState() INTERFACE to
 *                                  | FAST STATE MGR
 *  
 * Description: 
 *
 * Parameters:  BOOLEAN Run  - TRUE: Attempt to start downloading
 *                             FALSE: Attempt to stop downloading
 *              INT32   param - Not used, to match cal signature only
 *
 * Returns:     
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
    //Return value ignored, the FSM does not handle this error.
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
      pDMInfo = &DataMgrInfo[nChannel];      
      
      if ( pDMInfo->DL.bDownloading == TRUE )
      {
         // Check the interface type and command the interface to stop
         switch (pDMInfo->ACS_Config.PortType)
         {
            case ACS_PORT_UART:
               UartMgr_DownloadStop(pDMInfo->ACS_Config.PortIndex);
               break;
            case ACS_PORT_NONE:
            case ACS_PORT_ARINC429:
            case ACS_PORT_QAR:
            case ACS_PORT_MAX:
            default:
              FATAL ("Unexpected ACS_PORT_TYPE = % d", pDMInfo->ACS_Config.PortType);    
            break;
         }
         // Reset the flags for the channel
         pDMInfo->DL.bDownloading  = FALSE;
         pDMInfo->DL.bDownloadStop = TRUE;
         break;
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
      pDMInfo = &DataMgrInfo[nChannel];      
      
      if ( pDMInfo->DL.bDownloading == TRUE )
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
   TIMESTAMP              CurrentTime;
   DM_DOWNLOAD_STATISTICS *pStatistics;
   DM_DOWNLOAD_START_LOG  StartLog;

   bStarted = FALSE;
   
   if (!UploadMgr_IsUploadInProgress() && !DataMgrDownloadingACS())
   {
      for (nChannel = 0; nChannel < MAX_ACS_CONFIG; nChannel++)
      {
         pDMInfo    = &DataMgrInfo[nChannel]; 
         pACSConfig = &pDMInfo->ACS_Config;
      
         if (pACSConfig->Mode == ACS_DOWNLOAD)
         {
            pDMInfo->DL.bDownloading                           = TRUE;
            StartLog.nChannel  = nChannel;
            strncpy_safe(StartLog.Model, sizeof(StartLog.Model), 
                         pDMInfo->ACS_Config.Model, _TRUNCATE);
            strncpy_safe(StartLog.ID,    sizeof(StartLog.ID),    
                         pDMInfo->ACS_Config.ID,    _TRUNCATE);
            StartLog.PortType  = pDMInfo->ACS_Config.PortType;
            StartLog.PortIndex = pDMInfo->ACS_Config.PortIndex;
         
            LogWriteSystem(APP_DM_DOWNLOAD_STARTED, LOG_PRIORITY_LOW, 
                           &StartLog, sizeof(StartLog), NULL);
         
            pStatistics = &pDMInfo->DL.Statistics;
         
            pDMInfo->DL.bDownloadStop                          = FALSE;
            pStatistics->nRcvd                                 = 0;
            pStatistics->nTotalBytes                           = 0;
            CM_GetTimeAsTimestamp (&CurrentTime);
            pStatistics->StartTime                             = CurrentTime;
            pStatistics->EndTime.Timestamp                     = 0;
            pStatistics->EndTime.MSecond                       = 0;
            pStatistics->nStart_ms                             = CM_GetTickCount();
            pStatistics->nDuration_ms                          = 0;
         
            pStatistics->SmallestRecord.nNumber                = 0;
            pStatistics->SmallestRecord.nSize                  = UINT32_MAX;
            pStatistics->SmallestRecord.nDuration_ms           = 0;
            pStatistics->SmallestRecord.nStart_ms              = 0;
            pStatistics->SmallestRecord.RequestTime.Timestamp  = 0;
            pStatistics->SmallestRecord.RequestTime.MSecond    = 0;
            pStatistics->SmallestRecord.FoundTime.Timestamp    = 0;
            pStatistics->SmallestRecord.FoundTime.MSecond      = 0;
         
            pStatistics->LargestRecord.nNumber                 = 0;
            pStatistics->LargestRecord.nSize                   = 0;
            pStatistics->LargestRecord.nDuration_ms            = 0;
            pStatistics->LargestRecord.nStart_ms               = 0;
            pStatistics->LargestRecord.RequestTime.Timestamp   = 0;
            pStatistics->LargestRecord.RequestTime.MSecond     = 0;
            pStatistics->LargestRecord.FoundTime.Timestamp     = 0;
            pStatistics->LargestRecord.FoundTime.MSecond       = 0;
         
            pStatistics->ShortestRequest.nNumber               = 0;
            pStatistics->ShortestRequest.nSize                 = 0;
            pStatistics->ShortestRequest.nDuration_ms          = UINT32_MAX;
            pStatistics->ShortestRequest.nStart_ms             = 0;
            pStatistics->ShortestRequest.RequestTime.Timestamp = 0;
            pStatistics->ShortestRequest.RequestTime.MSecond   = 0;
            pStatistics->ShortestRequest.FoundTime.Timestamp   = 0;
            pStatistics->ShortestRequest.FoundTime.MSecond     = 0;
         
            pStatistics->LongestRequest.nNumber                = 0;
            pStatistics->LongestRequest.nSize                  = 0;
            pStatistics->LongestRequest.nDuration_ms           = 0;
            pStatistics->LongestRequest.nStart_ms              = 0;
            pStatistics->LongestRequest.RequestTime.Timestamp  = 0;
            pStatistics->LongestRequest.RequestTime.MSecond    = 0;
            pStatistics->LongestRequest.FoundTime.Timestamp    = 0;
            pStatistics->LongestRequest.FoundTime.MSecond      = 0;
                  
            // Enable the tasks to perform the download
            TmTaskEnable ((TASK_INDEX)(nChannel + (UINT32)Data_Mgr0), TRUE);
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
*              UINT16 DataWidthBytes  - Width of the data interface being
*                                       recorded
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
    TCB         TaskInfo;

    // Initialize the Data Manager Buffers
    memset( &DMTempBuffer, 0, sizeof(DMTempBuffer));
    DataMgrInfo[source].StartTime_mS         = 0;
    DataMgrInfo[source].CurrentBuffer        = 0;
    DataMgrInfo[source].ACS_Config           = *pACSConfig;
    DataMgrInfo[source].RecordStatus         = DM_RECORD_IDLE;
    DataMgrInfo[source].bBufferOverflow      = FALSE;
    DataMgrInfo[source].DL.State             = DL_RECORD_REQUEST;
    DataMgrInfo[source].DL.bDownloading      = FALSE;
    DataMgrInfo[source].DL.bDownloadStop     = FALSE;
    DataMgrInfo[source].DL.Statistics.nRcvd  = 0;

    for (i = 0; i < MAX_DM_BUFFERS; ++i)
    {
        memset( &DataMgrInfo[source].MsgBuf[i].packet, 0, sizeof(DM_PACKET));
        DataMgrInfo[source].MsgBuf[i].Status      = DM_BUF_EMPTY;
        DataMgrInfo[source].MsgBuf[i].Index       = 0;
        DataMgrInfo[source].MsgBuf[i].TryTime_mS  = 0;
        DataMgrInfo[source].MsgBuf[i].WrStatus    = LOG_REQ_NULL;
        DataMgrDLWriteStatus[i][source]           = DL_WRITE_SUCCESS;
        DataMgrInfo[source].MsgBuf[i].pDL_Status  = &DataMgrDLWriteStatus[i][source];
    }

    memset (&RecordStats[source],0,sizeof(RecordStats[source]));
    
    //------------------------------------------------------------------------
    // Create Data Manager - DT
    taskId = (TASK_INDEX)(source + (UINT32)Data_Mgr0);

    memset(&TaskInfo, 0, sizeof(TaskInfo));
    sprintf(TaskInfo.Name,"Data Mgr %d", source);
    TaskInfo.TaskID                     = taskId;
    TaskInfo.Function                   = DataTask;
    TaskInfo.Priority                   = taskInfo[taskId].priority;
    TaskInfo.Type                       = taskInfo[taskId].taskType;
    TaskInfo.modes                      = taskInfo[taskId].modes;
    TaskInfo.Enabled                    = bEnable;
    TaskInfo.Locked                     = FALSE;
    TaskInfo.MIFrames                   = MIFframes;
    TaskInfo.pParamBlock                = &DMPBlock[ source];

    // Initialize the Task Control Block
    DMPBlock[source].nChannel           = (UINT16)source;
    DMPBlock[source].GetData            = GetDataFunc;
    DMPBlock[source].GetSnapShot        = GetSnapFunc;
    DMPBlock[source].DoSync             = DoSyncFunc;
    DMPBlock[source].GetDataHdr         = GetDataHdrFunc;

    TmTaskCreate (&TaskInfo);
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
     
   // Initialize Local Data
   pDMParam       = (DATA_MNG_TASK_PARMS *)pParam;
   pDMInfo        = &DataMgrInfo[pDMParam->nChannel];
   pMsgBuf        = &pDMInfo->MsgBuf[pDMInfo->CurrentBuffer]; 

   pDMInfo->CurrTime_mS = CM_GetTickCount();
   
   ASSERT( NULL != pDMParam->GetData);
   
   // Empty the SW FIFO and discard the data
   nSize = pDMParam->GetData( DMTempBuffer, pDMInfo->ACS_Config.PortIndex, 
                              sizeof(DMTempBuffer)); 

   // Is the Recording Active?
   if (TRUE == DMRecordEnabled)
   {
      // Was the recording off?
      if (DM_RECORD_IDLE == pDMInfo->RecordStatus)
      {
         DataMgrStartRecording( pDMParam, pDMInfo);
      } 
      else // The recording is already started IN_PROGRESS
      {
         DataMgrRecordData( pDMParam, pDMInfo, nSize); 
      }
   }
   // NO trigger check if this is the end of the recording
   else if (DM_RECORD_IN_PROGRESS == pDMInfo->RecordStatus)
   {
      // Check if the buffer is empty
      if (DM_BUF_EMPTY == pMsgBuf->Status)
      {
         // Turn recording off
         pDMInfo->RecordStatus = DM_RECORD_IDLE;

         // record the time that End Snapshot is collected
         CM_GetTimeAsTimestamp (&(pDMInfo->MsgBuf[pDMInfo->CurrentBuffer].PacketTs));

         // Record the current snap shot status of the channel
         DataMgrGetSnapShot( pDMInfo, pDMParam->GetSnapShot,
                             pDMParam->nChannel, DM_END_SNAP);

         // Close the active buffer and swap
         DataMgrFinishPacket( pDMInfo, pDMParam->nChannel, DM_BUF_EMPTY_STATUS );
      }
      else if (DM_BUF_BUILD == pMsgBuf->Status)
      {
         // Close the active buffer and swap
         DataMgrFinishPacket( pDMInfo, pDMParam->nChannel, DM_BUF_BUILD_STATUS );
      }
   }

   // Check buffers and write any pending data packets
   DataMgrProcBuffers( pDMInfo, pDMParam->nChannel );
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
    TIMESTAMP           TimeNow;
    DATA_MNG_BUFFER     *pMsgBuf;

    // get the current buffer
    pMsgBuf = &pDMInfo->MsgBuf[pDMInfo->CurrentBuffer]; 

    // When a new recording starts, Reset the System Condition if previously set   
    //   Note: If a condition is detected during a recording, the sysCond will remain 
    //         for the duration of that recording.  Only when new recording starts 
    //         will sys condition be cleared and if condition detected again, sysCond 
    //         will be set once again.  
    //         bBufferOverflow is independent  
    //         conditions and can be set individually thus must be reset individually. 

    if (pDMInfo->bBufferOverflow != FALSE) {
        Flt_ClrStatus( pDMInfo->ACS_Config.SystemCondition ); 
        pDMInfo->bBufferOverflow    = FALSE;
    }

    // Do we have an empty buffer?
    if (DM_BUF_EMPTY == pMsgBuf->Status)
    {
        // Get the current Time
        CM_GetTimeAsTimestamp (&TimeNow);

        // set the packet timestamp, needed for the header
        pDMInfo->MsgBuf[pDMInfo->CurrentBuffer].PacketTs = TimeNow;

        // Records the current snapshot to Data Flash and then switched the 
        // current buffer flag to the opposite buffer. 
        DataMgrGetSnapShot( pDMInfo, pDMParam->GetSnapShot, 
                            pDMParam->nChannel, DM_START_SNAP);

        // Record the time the buffer started recording current channel
        pDMInfo->StartTime_mS = pDMInfo->CurrTime_mS;
        pDMInfo->NextTime_mS = pDMInfo->CurrTime_mS;
        nSize = 0;

        // a new buffer will be allocated because 
        // pDMInfo->NextTime_mS == pDMInfo->CurrTime_mS
        DataMgrNewBuffer( pDMParam, pDMInfo, &nSize);

        // Turn recording on
        pDMInfo->RecordStatus = DM_RECORD_IN_PROGRESS;
    }
}

/******************************************************************************
* Function:    DataMgrRecordData
*
* Description: The Data Manager ongoing recording of data.
*
* Parameters:  DATA_MNG_INFO *pDMInfo  - Data Manager info for the current 
*                                        channel. Allows access to the 
*                                        packet buffer fields.
*              GET_SNAP      *pGetSnap - Pointer to function that will get
*                                        the snapshot data. 
*              UINT16        Channel   - Index of the channel being recorded
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

    pBuf = (UINT8*)DMTempBuffer;

    // Check if any data was received
    while (nByteCnt != 0)
    {
        pMsgBuf = &pDMInfo->MsgBuf[pDMInfo->CurrentBuffer];

        // Data was received is the current buffer empty?
        if (DM_BUF_EMPTY == pMsgBuf->Status)
        {
            // This is a new packet build the header
            DataMgrBuildPacketHdr(pMsgBuf, pDMParam->nChannel, DM_STD_PACKET);
            // Now change the buffer state to build
            pMsgBuf->Status = DM_BUF_BUILD;
        }

        // Are we already building a packet?
        if (DM_BUF_BUILD == pMsgBuf->Status)
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
    UINT32          nSize;
    DM_FAIL_DATA    FailLog;
    DATA_MNG_BUFFER *pMsgBuf;

    // Check if packet timeout has been reached, or we overflowed the buffer
    if ((pDMInfo->CurrTime_mS >= pDMInfo->NextTime_mS) || *nLeftOver > 0)
    {
        // Close out packet in buffer and mark for storage
        DataMgrFinishPacket(pDMInfo, pDMParam->nChannel, (INT32)*nLeftOver);

        // Increment the next time
        pDMInfo->NextTime_mS += pDMInfo->ACS_Config.PacketSize_ms;

        // Check if the buffer is full and there is still data to write
        if ( *nLeftOver > 0)
        {
            if (FALSE == pDMInfo->bBufferOverflow)
            {
                // Set Buffer Overflow TRUE
                pDMInfo->bBufferOverflow = TRUE;

                // Now build the fail log and record the failure
                nSize = DataMgrBuildFailLog(pDMParam->nChannel, pDMInfo, &FailLog);

                // Set the system status and record failure          
                Flt_SetStatus( pDMInfo->ACS_Config.SystemCondition, 
                               APP_DM_SINGLE_BUFFER_OVERFLOW,
                               &FailLog, nSize);

                // Print out a debug message
                GSE_DebugStr(NORMAL, TRUE, 
                    "DataManager: BufferOverflow, Channel %d (%d)",
                    pDMParam->nChannel, *nLeftOver);
            }
        }

        //------------------------ prep new buffer or error ----------------------------
        pMsgBuf = &pDMInfo->MsgBuf[pDMInfo->CurrentBuffer];

        // Check if the new buffer is ready to receive new data
        ASSERT_MESSAGE(DM_BUF_EMPTY == pMsgBuf->Status,
                        "Channel %d Buffers Full", pDMParam->nChannel);
 
        // Set the Timestamp for the next packet
        CM_GetTimeAsTimestamp( &(pDMInfo->MsgBuf[pDMInfo->CurrentBuffer].PacketTs));
        pDMParam->DoSync( pDMInfo->ACS_Config.PortIndex );
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
 *              UINT16        Channel   - Index of the channel being recorded
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
   pMsgBuf   = &pDMInfo->MsgBuf[pDMInfo->CurrentBuffer];
 
   // Build the packet header
   DataMgrBuildPacketHdr(pMsgBuf, nChannel, eType);
           
   bStartSnap = (DM_START_SNAP == eType) ? TRUE : FALSE;
   
   // Get A data snapshot
   nCnt = GetSnap( &pMsgBuf->packet.data, pDMInfo->ACS_Config.PortIndex,
       MAX_BUFF_SIZE, bStartSnap);

   pMsgBuf->packet.checksum = ChecksumBuffer(&pMsgBuf->packet.data, nCnt, 0xFFFFFFFF);
       
   // Snapshot data received finish the packet
   pMsgBuf->Index += nCnt;
   pMsgBuf->Status = DM_BUF_BUILD;

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
   pMsgBuf->packet.timeStamp  = pMsgBuf->PacketTs;
   pMsgBuf->packet.channel    = nChannel;
   pMsgBuf->packet.packetType = (UINT16)eType;
   pMsgBuf->packet.size       = 0;
   pMsgBuf->Index             = 0;   
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
   pMsgBuf  = &pDMInfo->MsgBuf[pDMInfo->CurrentBuffer];
      
   if ( DM_BUF_BUILD == pMsgBuf->Status )  
   {
      // Put size in buffer
      pMsgBuf->packet.size = pMsgBuf->Index;
  
      // Since we leveled the checksum loading by calculating it as the data comes in
      // now we need to add in the header elements
      pMsgBuf->packet.checksum += ChecksumBuffer(
                                    &pMsgBuf->packet.timeStamp,
                                    &pMsgBuf->packet.data[0] -
                                    (UINT8 *)(&pMsgBuf->packet.timeStamp),
                                    0xFFFFFFFF);

      // Mark the buffer as store
      pMsgBuf->Status = DM_BUF_STORE;

      // display debug - indicating we just recorded the channel
      GSE_StatusStr( VERBOSE, "DM:%d %5d %3d %2d",
        nChannel, pMsgBuf->Index, recordInfo, pDMInfo->CurrentBuffer);

      // Next buffer
      ++pDMInfo->CurrentBuffer;
      if (pDMInfo->CurrentBuffer >= MAX_DM_BUFFERS)
      {
        pDMInfo->CurrentBuffer = 0;   // reset to beginning
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
 *              UINT32        Timeout_nS - Number of nanoseconds to wait 
 *                                         before declaring a timeout.
 *
 * Returns:     None.
 *
 * Notes:       None.
 *
 *****************************************************************************/
static
void DataMgrProcBuffers(DATA_MNG_INFO *pDMInfo, UINT32 nChannel )
{
   // Local Data
   UINT32       i;
   LOG_SOURCE   TempChannel;
 
   TempChannel.nNumber = nChannel;
      
   // Process packet buffers
   for ( i = 0; i < MAX_DM_BUFFERS; i++)
   {
      // Check that current buffer is ready to transmit
      if (DM_BUF_STORE == pDMInfo->MsgBuf[i].Status)
      {
         // Has the Current buffer been queued for storage yet?
         if (LOG_REQ_NULL == pDMInfo->MsgBuf[i].WrStatus) 
         {
            // For channels that download we need to keep track of the status
            *(pDMInfo->MsgBuf[i].pDL_Status) = DL_WRITE_IN_PROGRESS;
            // Attempt to store packet in Data Flash
            // Add the checksum's checksum to the checksum for Log Write
            LogWrite ( LOG_TYPE_DATA, 
                       TempChannel,
                       pDMInfo->ACS_Config.Priority, 
                       (void*)&pDMInfo->MsgBuf[i].packet,
                       DM_PACKET_SIZE(pDMInfo->MsgBuf[i].packet.size),
                       ( pDMInfo->MsgBuf[i].packet.checksum + 
                        ChecksumBuffer(&pDMInfo->MsgBuf[i].packet.checksum,
                        sizeof(UINT32), LOG_CHKSUM_BLANK)),
                       &pDMInfo->MsgBuf[i].WrStatus );

            // display debug - indicating we just recorded the channel
            GSE_StatusStr ( VERBOSE, " Q%d:%d @ %d"NL, nChannel, i, CM_GetTickCount());
         }
         // Has the buffer completed storage yet?
         else if (LOG_REQ_COMPLETE == pDMInfo->MsgBuf[i].WrStatus )
         {
            *(pDMInfo->MsgBuf[i].pDL_Status) = DL_WRITE_SUCCESS;
            pDMInfo->MsgBuf[i].Status        = DM_BUF_EMPTY;
            pDMInfo->MsgBuf[i].TryTime_mS    = 0;
            pDMInfo->MsgBuf[i].WrStatus      = LOG_REQ_NULL;
         }
         else if ( LOG_REQ_FAILED == pDMInfo->MsgBuf[i].WrStatus )
         {
	         *(pDMInfo->MsgBuf[i].pDL_Status) = DL_WRITE_FAIL;
            pDMInfo->MsgBuf[i].Status        = DM_BUF_EMPTY;
            pDMInfo->MsgBuf[i].TryTime_mS    = 0;
            pDMInfo->MsgBuf[i].WrStatus      = LOG_REQ_NULL;
         }
      }
   }
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
   nAvailable =  MAX_BUFF_SIZE - pMsgBuf->Index;
     
   // Check if more space available than needed
   if (nAvailable > nBytes)
   {
      // Copy the data that was received to the buffer
      memcpy (&pMsgBuf->packet.data[pMsgBuf->Index], pSrc, nBytes);
      
      // Checksum the Buffer here to help level out the loading
      pMsgBuf->packet.checksum += ChecksumBuffer(pSrc, nBytes, 0xFFFFFFFF);
            
      // Update the buffer index
      pMsgBuf->Index += nBytes;
   }
   else // Buffer will overflow just fill to the end less the checksum
   {
      // Copy as much as possible
      memcpy (&pMsgBuf->packet.data[pMsgBuf->Index], pSrc, nAvailable);
      
      // Checksum the buffer here to help level out the loading
      pMsgBuf->packet.checksum += ChecksumBuffer(pSrc, nAvailable, 0xFFFFFFFF);      
      
      // Update the buffer index
      pMsgBuf->Index += nAvailable;
      // Calculate the remaining to be written
      nRemainingToWrite = nBytes - nAvailable;
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
  DATATASK   Task;
  BOOLEAN    bEnable;
  UINT32     MIFframes;
  TASK_INDEX taskId;
    
  for (i = 0; i < MAX_ACS_CONFIG; i++)
  {
    pACSConfig = &(CfgMgr_RuntimeConfigPtr()->ACSConfigs[i]);
    taskId     = (TASK_INDEX)(i + (UINT32)Data_Mgr0);
    
	/*Override ACS priority for Full Flight Data sources if Full Flight Data mode is enabled
	  Full Flight Data inhibit.  This modifies the prioriy in the configuration memory before
    it is copied to the DataManager local configuration.  If the configuration is changed
    at runtime, it will not change DataManager local configuration. */
    if( DataMgrIsPortFFD(pACSConfig) && DataMgrIsFFDInhibtMode( ) )
    {
	pACSConfig->Priority = LOG_PRIORITY_4;
    }

	
    if ((pACSConfig->Mode == ACS_DOWNLOAD) && (pACSConfig->PortType != ACS_PORT_NONE))
    {
       // Configure the ACS Download Task
       Task      = DataMgrDownloadTask;        
       bEnable   = FALSE;
       MIFframes = 0xFFFFFFFF;
    }
    else
    {
       Task      = DataMgrTask;
       bEnable   = TRUE;
       MIFframes = taskInfo[taskId].MIFframes;
    }

    switch (pACSConfig->PortType)
    {
       case ACS_PORT_NONE:
          // Do Nothing
          break;

       case ACS_PORT_ARINC429:
          DataMgrCreateTask( i,                                  // ACS Index
                             pACSConfig,
                             Arinc429MgrReadFilteredRaw,         // func to read the data
                             Arinc429MgrReadFilteredRawSnapshot, // func to read the snapshot
                             Arinc429MgrSyncTime,                // Time Sync Function
                             Arinc429MgrGetFileHdr,
                             Task, MIFframes,
                             bEnable );                          // Task Enable
          break;

       case ACS_PORT_QAR:
          DataMgrCreateTask( i,                           // ACS index
                             pACSConfig,
                             QAR_GetSubFrame,             // QAR func to get sub-frame
                             QAR_GetFrameSnapShot,        // QAR func to get full frame
                             QAR_SyncTime,                // Time Sync Function
                             QAR_GetFileHdr,
                             Task, MIFframes,
                             bEnable );                      // Task Enable
          break;

       case ACS_PORT_UART:
          DataMgrCreateTask( i,                           // ACS Index
                             pACSConfig,
                             UartMgr_ReadBuffer,          // func to read the data
                             UartMgr_ReadBufferSnapshot,  // func to read the snapshot
                             UartMgr_ResetBufferTimeCnt,  // Time Sync Function
                             UartMgr_GetFileHdr,
                             Task, MIFframes,
                             bEnable );                      // Task Enable
          break;

       default:
          FATAL ("Unexpected ACS_PORT_TYPE = % d", pACSConfig->PortType);
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

   strncpy_safe(pFailLog->Model, sizeof(pFailLog->Model),
     pDMInfo->ACS_Config.Model,_TRUNCATE);
   strncpy_safe(pFailLog->ID,sizeof(pFailLog->ID),pDMInfo->ACS_Config.ID,_TRUNCATE);
   
   pFailLog->nChannel = Channel;
   
   strncpy_safe(pFailLog->RecordStatus,   sizeof(pFailLog->RecordStatus),
                RecordStatusType[pDMInfo->RecordStatus].Str,_TRUNCATE);
   
   sprintf(tmpBuf, "%02d", pDMInfo->CurrentBuffer);
   strncpy_safe(pFailLog->CurrentBuffer,  sizeof(pFailLog->CurrentBuffer),
                tmpBuf, _TRUNCATE);
   
   strncpy_safe(pFailLog->BufferStatus,  sizeof(pFailLog->BufferStatus),
                BufferStatusType[pDMInfo->MsgBuf[pDMInfo->CurrentBuffer].Status].Str,
                _TRUNCATE);
   
   strncpy_safe(pFailLog->WriteStatus,   sizeof(pFailLog->WriteStatus),
                LogReqType[pDMInfo->MsgBuf[pDMInfo->CurrentBuffer].WrStatus].Str,_TRUNCATE);
   
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
   DL_STATUS                      RequestStatus;
   TIMESTAMP                      CurrentTime;
   UINT32                         TempTime;
     
   // Initialize Local Data
   pDMParam = (DATA_MNG_TASK_PARMS *)pParam;
   pDMInfo  = &DataMgrInfo[pDMParam->nChannel];
   pMsgBuf  = &pDMInfo->MsgBuf[pDMInfo->CurrentBuffer]; 
  
   // Get the current time for statistics
   CM_GetTimeAsTimestamp (&CurrentTime);
   
   // Determine which state we are in
   switch (pDMInfo->DL.State)
   {
      // Request a new record from the interface
      case DL_RECORD_REQUEST:
         // Make sure we haven't been requested to stop before getting the next record
         if (pDMInfo->DL.bDownloadStop == FALSE)
         {
            // If the statistics start time is "0" then initialized the timers for 
            // the current record
            if (RecordStats[pDMParam->nChannel].nStart_ms == 0)
            {
               RecordStats[pDMParam->nChannel].nStart_ms   = CM_GetTickCount();
               RecordStats[pDMParam->nChannel].RequestTime = CurrentTime;
            }
            
            // Request the next record to download from the interface
            pDMInfo->DL.pDataSrc = DataMgrRequestRecord ( pDMInfo->ACS_Config.PortType, 
                                                          pDMInfo->ACS_Config.PortIndex, 
                                                          &RequestStatus, &pDMInfo->DL.nBytes, 
                                                          pMsgBuf->pDL_Status );
            // Did the interface report no records or no more records to download?
            if ( RequestStatus == DL_NO_RECORDS )
            {
               // Advance to the END State for the download processing
               pDMInfo->DL.State = DL_END;
            }
            // Did the interface find a record?
            else if ( RequestStatus == DL_RECORD_FOUND )
            {
               // Save the time the record was found
               RecordStats[pDMParam->nChannel].FoundTime    = CurrentTime;
               // Calculate the duration for the request
               TempTime = CM_GetTickCount();
               RecordStats[pDMParam->nChannel].nDuration_ms = TempTime - 
                                                     RecordStats[pDMParam->nChannel].nStart_ms;
               // Increment the number of records found on this interface
               pDMInfo->DL.Statistics.nRcvd++;
               // Total the number of bytes received
               pDMInfo->DL.Statistics.nTotalBytes     += pDMInfo->DL.nBytes;
               // Save the record number for the current record so statistics can be run
               RecordStats[pDMParam->nChannel].nNumber = pDMInfo->DL.Statistics.nRcvd;
               // Save the number of bytes for the current record so statistics can be run
               RecordStats[pDMParam->nChannel].nSize   = pDMInfo->DL.nBytes;
               // Print a debug message about the record being found              
               sprintf (TempDebugString, "\r\nFOUND: %d Bytes: %d\r\n", 
                        pDMInfo->DL.Statistics.nRcvd, pDMInfo->DL.nBytes);
               GSE_DebugStr(NORMAL,TRUE,TempDebugString); 
               // Initiate the record save and change states
               DataMgrDLSaveAndChangeState ( pDMInfo, 
                                             DM_DOWNLOADED_RECORD, 
                                             (UINT8)pDMParam->nChannel );
            }
         }
         else // Stop Download was detected advance to the END State
         {
            pDMInfo->DL.State = DL_END;
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
            bufsEmpty = pDMInfo->MsgBuf[i].Status == DM_BUF_EMPTY;
          }

          if ( bufsEmpty )
          {
            // Debug message that the download has ended
            sprintf (TempDebugString, "\rEND: Total Records %d \0", 
                     pDMInfo->DL.Statistics.nRcvd);
            GSE_StatusStr(NORMAL,TempDebugString); 
            // There are no more records to download on this channel reset flag
            // Kill Task, save record statistics and wait until next download request 
            // from FAST Manager
            TmTaskEnable((pDMParam->nChannel + Data_Mgr0), FALSE);
            pDMInfo->DL.Statistics.nChannel     = pDMParam->nChannel;
            pDMInfo->DL.Statistics.EndTime      = CurrentTime;
            pDMInfo->DL.Statistics.nDuration_ms = CM_GetTickCount() - 
                                                  pDMInfo->DL.Statistics.nStart_ms;
            
            LogWriteSystem(APP_DM_DOWNLOAD_STATISTICS, LOG_PRIORITY_LOW, 
                           &pDMInfo->DL.Statistics, sizeof(pDMInfo->DL.Statistics), NULL);
            
            pDMInfo->DL.State         = DL_RECORD_REQUEST;
            pDMInfo->DL.bDownloading  = FALSE;
            pDMInfo->DL.bDownloadStop = FALSE;
          }
        }
        break;
      default:
        FATAL("DM_Download Bad State %d", pDMInfo->DL.State);
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
void DataMgrDLSaveAndChangeState ( DATA_MNG_INFO *pDMInfo, DM_PACKET_TYPE Type, UINT8 nChannel )
{
   // This ASSERT should never occur because the Interface is throttling the 
   // Data Manager and only allowing one record at a time to be saved to data 
   // flash before continuing to the next record
   ASSERT_MESSAGE(DM_BUF_EMPTY == pDMInfo->MsgBuf[pDMInfo->CurrentBuffer].Status,
                  "Channel %d Buffers Full", nChannel);
   
   // Send out a debug message about the save
   sprintf (TempDebugString, "\rCOPY: %d Bytes: %d Buf: %d\0", 
            pDMInfo->DL.Statistics.nRcvd, pDMInfo->DL.nBytes, pDMInfo->CurrentBuffer);
   GSE_StatusStr(NORMAL,TempDebugString); 
               
   // Build the first packet to store to the data flash 
   pDMInfo->DL.nBytes = DataMgrSaveDLRecord ( pDMInfo, nChannel, Type );

   // If the entire downloaded record fits in one packet the next state
   // will be to REQUEST another downloaded record
   if ( 0 == pDMInfo->DL.nBytes )
   {
      pDMInfo->DL.State = DL_RECORD_REQUEST;

      // Send out a debug message that the current request completed
      sprintf (TempDebugString, "\rDONE: %d\0", pDMInfo->DL.Statistics.nRcvd);
      GSE_StatusStr(NORMAL,TempDebugString); 
         
      // Update the statistics using the current record
      DataMgrDLUpdateStatistics ( pDMInfo, &RecordStats[nChannel] );
      // Reset the record statistics so they can be reused 
      memset (&RecordStats[nChannel],0,sizeof(RecordStats[nChannel]));
   }
   else // The downloaded record is larger than the maximum we can process
        // in one chunk. Next state is CONTINUE with the record
   {
      pDMInfo->DL.State = DL_RECORD_CONTINUE;
      // Print out debug message about continuing with current record
      sprintf (TempDebugString, "\rCONT: %d Bytes: %d Buf: %d\0", 
               pDMInfo->DL.Statistics.nRcvd, pDMInfo->DL.nBytes, pDMInfo->CurrentBuffer);
      GSE_StatusStr(NORMAL,TempDebugString); 
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
 *               DL_WRITE_STATUS *pDL_WrStatus  - Status of the write once data is found
 * 
 * Returns:      Location of the data found
 *
 * Notes:        None.
 *
 *****************************************************************************/
static UINT8* DataMgrRequestRecord ( ACS_PORT_TYPE PortType, UINT8 PortIndex, 
                                     DL_STATUS *pStatus, UINT32 *pSize, 
                                     DL_WRITE_STATUS *pDL_WrStatus )
{
   // Local Data
   UINT8     *pSrc;
   
   switch (PortType)
   {
      case ACS_PORT_UART:
         pSrc = UartMgr_DownloadRecord ( PortIndex, pStatus, pSize, pDL_WrStatus );
         break;

      case ACS_PORT_NONE:
      case ACS_PORT_ARINC429:
      case ACS_PORT_QAR:
      case ACS_PORT_MAX:
      default:
         *pStatus = DL_NO_RECORDS;
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
   pMsgBuf  = &pDMInfo->MsgBuf[pDMInfo->CurrentBuffer]; 

   // Calling function already checked that the buffer was ready to be used and 
   // ASSERTS if the buffer isn't empty. Not possible to get here if we don't 
   // already have an empty buffer
   
   // We have a buffer now mark the status as build
   pMsgBuf->Status = DM_BUF_BUILD;

   // Get the time for the current record
   CM_GetTimeAsTimestamp (&(pMsgBuf->PacketTs));
   // Create the packet header         
   DataMgrBuildPacketHdr( pMsgBuf, nChannel, PacketType);
      
   // Check if the record is less than the theoretical maximum
   if ( pDMInfo->DL.nBytes < MAX_DOWNLOAD_CHUNK )
   {        
      // Write the record to the buffer
      memcpy (&pMsgBuf->packet.data[pMsgBuf->Index], 
              pDMInfo->DL.pDataSrc, pDMInfo->DL.nBytes);
      
      // Checksum the Buffer here to help level out the loading
      pMsgBuf->packet.checksum += ChecksumBuffer(pDMInfo->DL.pDataSrc, pDMInfo->DL.nBytes, 
                                                 0xFFFFFFFF);
            
      // Update the buffer index
      pMsgBuf->Index += pDMInfo->DL.nBytes;
      // Record fit into on packet non bytes remain
      nRemaining = 0;
   }
   else // Larger than the maximum, write the maximum and then goto the continue state
   {
      // Write the record to the buffer
      memcpy (&pMsgBuf->packet.data[pMsgBuf->Index],pDMInfo->DL.pDataSrc,MAX_DOWNLOAD_CHUNK);
      
      // Checksum the Buffer here to help level out the loading
      pMsgBuf->packet.checksum += ChecksumBuffer(pDMInfo->DL.pDataSrc, 
                                                 MAX_DOWNLOAD_CHUNK, 0xFFFFFFFF);
            
      // Update the buffer index
      pMsgBuf->Index += MAX_DOWNLOAD_CHUNK;
      // Move the pointer to the data
      pDMInfo->DL.pDataSrc += MAX_DOWNLOAD_CHUNK; 
      // Calculate how many bytes remain
      nRemaining            = pDMInfo->DL.nBytes - MAX_DOWNLOAD_CHUNK;

   }
   // Set the write status to in-progress
   *(pDMInfo->MsgBuf[pDMInfo->CurrentBuffer].pDL_Status) = DL_WRITE_IN_PROGRESS;
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
   if ( pStats->nSize > pDMInfo->DL.Statistics.LargestRecord.nSize )
   {
      pDMInfo->DL.Statistics.LargestRecord = *pStats;
   }
   // Check for the smallest size
   if ( pStats->nSize < pDMInfo->DL.Statistics.SmallestRecord.nSize )
   {
      pDMInfo->DL.Statistics.SmallestRecord = *pStats;
   }   
   // Check for longest request
   if( pStats->nDuration_ms > pDMInfo->DL.Statistics.LongestRequest.nDuration_ms )
   {
      pDMInfo->DL.Statistics.LongestRequest = *pStats;
   }
   // Check for shortest request
   if ( pStats->nDuration_ms < pDMInfo->DL.Statistics.ShortestRequest.nDuration_ms )
   {
      pDMInfo->DL.Statistics.ShortestRequest = *pStats;
   }
}
                                         
/*************************************************************************
 *  MODIFICATIONS
 *    $History: DataManager.c $
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
 
