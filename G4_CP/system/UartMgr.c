#define UART_MGR_BODY
/******************************************************************************
            Copyright (C) 2008-2016 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    ECCN:        9D991

    File:        UartMgr.c

    Description: Contains all functions and data related to the UART Mgr CSC

    VERSION
      $Revision: 76 $  $Date: 6/06/16 1:54p $

******************************************************************************/

//#define UARTMGR_TIMING_TEST 1

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/



/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "UartMgr.h"
#include "CfgManager.h"
#include "GSE.h"



/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define UARTMGR_RAW_BUF_SIZE 256 // Worst case 115.2 kbps. At 10 msec 115 bytes worst case.

#define UART_SENSOR_CHAN_SHIFT         8
#define UART_SENSOR_CHAN_MASK          0x7
#define UART_SENSOR_INDEX_MASK         UINT8MAX
#define UART_SENSOR_DEFAULT_MSB        15
#define UART_SENSOR_DEFAULT_WORD_SIZE  16

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/



/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static UARTMGR_CFG             m_UartMgrCfg[UART_NUM_OF_UARTS];//Note Index 0 allocated to GSE
static UARTMGR_STATUS          m_UartMgrStatus[UART_NUM_OF_UARTS];
static UARTMGR_WORD_INFO       m_UartMgr_WordInfo[UART_NUM_OF_UARTS][UARTMGR_MAX_PARAM_WORD];
static UINT16                  m_UartMgr_WordSensorCount[UART_NUM_OF_UARTS];
static UARTMGR_PARAM_DATA      m_UartMgr_Data[UART_NUM_OF_UARTS][UARTMGR_MAX_PARAM_WORD];
static UARTMGR_TASK_PARAMS     uartMgrBlock[UART_NUM_OF_UARTS];
static UARTMGR_BIT_TASK_PARAMS uartMgrBITBlock;
static UINT8                   m_UartMgr_RawBuffer[UART_NUM_OF_UARTS][UARTMGR_RAW_BUF_SIZE];
static UARTMGR_STORE_BUFF      m_UartMgr_StoreBuffer[UART_NUM_OF_UARTS];

static UARTMGR_DEBUG           m_UartMgr_Debug;

static UARTMGR_DISP_DEBUG_TASK_PARMS uartMgrDispDebugBlock;
static UARTMGR_DOWNLOAD              m_UartMgr_Download[UART_NUM_OF_UARTS];
static UARTMGR_DOWNLOAD              m_UartMgr_Download_GBS_SIM; 

static INT8 str[UARTMGR_DEBUG_BUFFER_SIZE * 6]; // convert byte to "0xXX" with CR/LF

// Test Timing
#ifdef UARTMGR_TIMING_TEST
  #define UARTMGR_TIMING_BUFF_MAX 200
  UINT32 m_UARTMGRTiming_Buff[UARTMGR_TIMING_BUFF_MAX];
  UINT32 m_UARTMGRTiming_Start;
  UINT32 m_UARTMGRTiming_End;
  UINT16 m_UARTMGRTiming_Index;
#endif
// Test Timing


/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void              UartMgr_InitData               ( void );
static void              UartMgr_Task                   ( void *pParam );
static void              UartMgr_BITTask                ( void *pParam );
static BOOLEAN           UartMgr_DetermineChanLoss      ( UINT16 ch );
static void              UartMgr_CreateTimeOutSystemLog ( RESULT resultType, UINT16 ch );
static void              UartMgr_DetermineParamDataLoss ( UINT16 ch );
static UINT16            UartMgr_CopyRollBuff           ( UINT8  *dest_ptr, UINT8 *src_ptr,
                                                          UINT16 size,      UINT32 *wr_cnt,
                                                          UINT32 *rd_cnt,   UINT16 wrap_size );
static FLOAT32           UartMgr_ConvertToEngUnits (UARTMGR_RUNTIME_DATA_PTR runtime_data_ptr);
static void              UartMgrDispDebug_Task          ( void *pParam );
static void              UartMgr_Download_NoneHndl      ( UINT8 port,
                                                          BOOLEAN *bStartDownload,
                                                          BOOLEAN *bDownloadCompleted,
                                                          BOOLEAN *bWriteInProgress,
                                                          BOOLEAN *bWriteOk,
                                                          BOOLEAN *bHalt,
                                                          BOOLEAN *bNewRec,
                                                          UINT32  **pSize,
                                                          UINT8   **pData );
static void               UartMgr_ClearDownloadState    ( UINT8 PortIndex );
static UARTMGR_DEBUG_PTR  UartMgr_GetDebug              ( void );
static UARTMGR_STATUS_PTR UartMgr_GetStatus             ( UINT8 portIndex );

static void UartMgr_UpdateDataReduction                 ( UINT16 ch,
                                                          UARTMGR_PROTOCOLS protocols );

static  UINT16  UartMgr_ReadBuffSnapshotProtocol     ( void *pDest, UINT32 chan,
                                                       UINT16 nMaxByteSize,
                                                       BOOLEAN bBeginSnapshot,
                                                       UARTMGR_PROTOCOLS protocols );
static BOOLEAN UartMgr_Protocol_ReadyOk_Hndl         ( UINT16 ch );
static void UartMgr_Download_Clr_NoneHndl            ( BOOLEAN Run, INT32 param ); 
static BOOLEAN UartMgr_Get_ESN_NoneHndl              ( UINT16 ch, CHAR *esn_ptr, UINT16 cnt );

#include "UartMgrUserTables.c"   // Include the cmd table & functions

/*****************************************************************************/
/* Constant Data                                                             */
/*****************************************************************************/
static UART_CONFIG uartDrvDefaultCfg =
{
  FALSE, 0, UART_CFG_DUPLEX_FULL, UART_CFG_BPS_115200, UART_CFG_DB_8,
  UART_CFG_SB_1, UART_CFG_PARITY_NONE, FALSE, NULL, NULL
};


/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:    UartMgr_Initialize
 *
 * Description: Initializes the UartMgr functionality
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:
 *  - The UartMgr Initialization will initialize all local module variables
 *    and data structures.
 *  - The UartMgr Initialization will initialize and open each UART port based
 *    on the stored port configuration.
 *  - If a Uart port initialization fails, the UartMgr Initialization will:
 *    1) set the box system condition to the Uart PBIT system condition
 *       configuration.
 *    2) the interface will be declared "invalid".
 *  - If a Uart port initialization succeeds, the UartMgr Initialization will:
 *    1) initialize the UartMgr Processing Task (for the specified port).
 *    2) call the serial protocol handler function initialization (i.e.
 *       F7X N-Param, CSDB, ASCB, etc).
 *  - A UartMgr Processing task will be created / enabled for each configured
         UART Port.
 *****************************************************************************/
void UartMgr_Initialize (void)
{
  TCB    tcbTaskInfo;
  UINT16  i;
  RESULT result;
  UARTMGR_CFG_PTR pUartMgrCfg;
  UARTMGR_PORT_CFG_PTR pUartMgrPortCfg;
  UARTMGR_STATUS_PTR pUartMgrStatus;
  UART_CONFIG uartCfg;
  UART_SYS_PBIT_STARTUP_LOG startupLog;
  BOOLEAN bAtLeastOneEnabled;

  // Initialize all local variables
  memset (m_UartMgrStatus, 0, sizeof(UARTMGR_STATUS) * (UINT8)UART_NUM_OF_UARTS);
  memset (m_UartMgr_WordInfo, 0, sizeof(m_UartMgr_WordInfo) );
  memset (m_UartMgr_Data, 0, sizeof(m_UartMgr_Data) );
  memset (m_UartMgr_RawBuffer, 0, UARTMGR_RAW_BUF_SIZE * (UINT8)UART_NUM_OF_UARTS );
  memset (m_UartMgr_StoreBuffer, 0, sizeof(UARTMGR_STORE_BUFF) * (UINT8)UART_NUM_OF_UARTS);
  memset (m_UartMgr_Download, 0, sizeof(UARTMGR_DOWNLOAD) * (UINT8)UART_NUM_OF_UARTS);
  memset ( (void *) &m_UartMgr_Download_GBS_SIM, 0, sizeof(m_UartMgr_Download_GBS_SIM) );
  // Initialize Data and WordInfo
  UartMgr_InitData();
  //Add an entry in the user message handler table
  User_AddRootCmd(&uartMgrRootTblPtr);
  // Restore User Default
  memcpy(m_UartMgrCfg, CfgMgr_RuntimeConfigPtr()->UartMgrConfig, sizeof(m_UartMgrCfg));
  // Open and initialize each UART Port.  Exclude UART[0] GSE port !
  result = SYS_OK;
  bAtLeastOneEnabled = FALSE;
  for (i = 1; i < (UINT8)UART_NUM_OF_UARTS; i++)
  {
    pUartMgrStatus = (UARTMGR_STATUS_PTR) &m_UartMgrStatus[i];
    pUartMgrCfg = (UARTMGR_CFG_PTR) &m_UartMgrCfg[i];
    // Port Cfg for use ?
    if ( pUartMgrCfg->bEnabled == TRUE )
    {
      // Update flag to indicate at least one Uart Ch is enabled
      bAtLeastOneEnabled = TRUE;
      // Update the protocol for current status
      pUartMgrStatus->protocol = pUartMgrCfg->protocol;
      // Cfg Port
      pUartMgrPortCfg = (UARTMGR_PORT_CFG_PTR) &pUartMgrCfg->port;
      uartCfg = uartDrvDefaultCfg;    // Set the defaults;
      uartCfg.Port = i;
      uartCfg.BPS = pUartMgrPortCfg->nBPS;
      uartCfg.DataBits = pUartMgrPortCfg->dataBits;
      uartCfg.StopBits = pUartMgrPortCfg->stopBits;
      uartCfg.Parity = pUartMgrPortCfg->parity;
      uartCfg.Duplex = pUartMgrPortCfg->duplex;
      uartCfg.RxFIFO = NULL;
      uartCfg.TxFIFO = NULL;
      // Init UartMgrBlock[] struct
      uartMgrBlock[i].nChannel = i;
      uartMgrBlock[i].download_protocol_clr_hndl = UartMgr_Download_Clr_NoneHndl;
      uartMgrBlock[i].get_protocol_esn = UartMgr_Get_ESN_NoneHndl;
      uartMgrBlock[i].get_protocol_ready_hndl = UartMgr_Protocol_ReadyOk_Hndl;
      uartMgrBlock[i].download_protocol_hndl = UartMgr_Download_NoneHndl;
      switch ( pUartMgrCfg->protocol )
      {
        case UARTMGR_PROTOCOL_F7X_N_PARAM:
          uartMgrBlock[i].exec_protocol = F7XProtocol_Handler;
          uartMgrBlock[i].get_protocol_fileHdr = F7XProtocol_ReturnFileHdr;
          uartMgrBlock[i].get_protocol_esn = F7XProtocol_GetESN;
          break;
        case UARTMGR_PROTOCOL_EMU150:
          uartMgrBlock[i].exec_protocol = EMU150Protocol_Handler;
          uartMgrBlock[i].get_protocol_fileHdr = EMU150Protocol_ReturnFileHdr;
          uartMgrBlock[i].download_protocol_hndl = EMU150Protocol_DownloadHndl;
          EMU150Protocol_SetBaseUartCfg(i, uartCfg);
          break;
        case UARTMGR_PROTOCOL_ID_PARAM:
          uartMgrBlock[i].exec_protocol = IDParamProtocol_Handler;
          uartMgrBlock[i].get_protocol_fileHdr = IDParamProtocol_ReturnFileHdr;
          uartMgrBlock[i].get_protocol_ready_hndl = IDParamProtocol_Ready;
          IDParamProtocol_InitUartMgrData ( i, (void *) m_UartMgr_Data[i] );
          break;
        case UARTMGR_PROTOCOL_GBS:
          uartMgrBlock[i].exec_protocol = GBSProtocol_Handler;
          uartMgrBlock[i].get_protocol_fileHdr = GBSProtocol_ReturnFileHdr;
          uartMgrBlock[i].download_protocol_hndl = GBSProtocol_DownloadHndl;          
          uartMgrBlock[i].download_protocol_clr_hndl = GBSProtocol_DownloadClrHndl;
          break;          
        case UARTMGR_PROTOCOL_PWC_DISPLAY:
          uartMgrBlock[i].exec_protocol           = PWCDispProtocol_Handler;
          uartMgrBlock[i].read_protocol           = PWCDispProtocol_Read_Handler;
          uartMgrBlock[i].write_protocol          = PWCDispProtocol_Write_Handler;
          uartMgrBlock[i].get_protocol_fileHdr    = PWCDispProtocol_ReturnFileHdr;
          uartMgrBlock[i].protocol_ID             = UARTMGR_PROTOCOL_PWC_DISPLAY;
          PWCDispProtocol_SetBaseUARTCh(i);
          break;
        case UARTMGR_PROTOCOL_NONE:
        case UARTMGR_PROTOCOL_MAX:
          // Nothing to do here - Fall Through
        default:
          FATAL ("Unexpected UARTMGR_PROTOCOL = %d, UART Channel = %d",
                    pUartMgrCfg->protocol, i);
          break;
      }
      // Open Uart Port
      result = UART_OpenPort( &uartCfg );

      if ( result == SYS_OK )
      {
        // If cfg and port ok, Initialize Uart Task
        memset(&tcbTaskInfo, 0, sizeof(tcbTaskInfo));
        snprintf(tcbTaskInfo.Name, sizeof(tcbTaskInfo.Name),"Uart Mgr %d", i);
        tcbTaskInfo.TaskID      = (TASK_INDEX)((UINT8)UART_Mgr0 + i);
        tcbTaskInfo.Function    = UartMgr_Task;
        tcbTaskInfo.Priority    = taskInfo[(UINT8)UART_Mgr0 + i].priority;
        tcbTaskInfo.Type        = taskInfo[(UINT8)UART_Mgr0 + i].taskType;
        tcbTaskInfo.modes       = taskInfo[(UINT8)UART_Mgr0 + i].modes;
        tcbTaskInfo.Enabled     = TRUE;
        tcbTaskInfo.Locked      = FALSE;
        tcbTaskInfo.MIFrames    = taskInfo[(UINT8)UART_Mgr0 + i].MIFframes;
        // Store my TaskID in the param block
        // so the shared UartMgr_Task function can disambiguate when needed.
        uartMgrBlock[i].taskID = tcbTaskInfo.TaskID;
        tcbTaskInfo.pParamBlock = (void *) &uartMgrBlock[i];
        TmTaskCreate ( &tcbTaskInfo );
        pUartMgrStatus->status = UARTMGR_STATUS_OK;
      }
      else
      {
        // Set PBIT sys condition and exit
        pUartMgrStatus->status = UARTMGR_STATUS_FAULTED_PBIT;
      }
    } // End if ( pUartCfg->bEnabled == TRUE )
    else
    {
      pUartMgrStatus->status = UARTMGR_STATUS_DISABLED_OK;
    }
    // Create log and update sys status if failure detected
    if ( result != SYS_OK )
    {
      // Update log data
      startupLog.result = SYS_UART_PBIT_UART_OPEN_FAIL;
      startupLog.ch = i;
      // Set Fault Status
      Flt_SetStatus(pUartMgrCfg->sysCondPBIT, SYS_ID_UART_PBIT_FAIL,
                    &startupLog, sizeof(UART_SYS_PBIT_STARTUP_LOG));
    }
    // Update internal variables
    UartMgr_ClearDownloadState(i);
  } // End for loop
  
  // Update internal variables  
  UartMgr_ClearDownloadState(GBS_SIM_PORT_INDEX); 
  // Enable UartMgr BIT Task if any Uart is enabled
  if ( bAtLeastOneEnabled == TRUE )
  {
    memset(&tcbTaskInfo, 0, sizeof(tcbTaskInfo));
    strncpy_safe(tcbTaskInfo.Name, sizeof(tcbTaskInfo.Name),"Uart Mgr BIT",_TRUNCATE);
    tcbTaskInfo.TaskID      = (TASK_INDEX)Uart_BIT;
    tcbTaskInfo.Function    = UartMgr_BITTask;
    tcbTaskInfo.Priority    = taskInfo[Uart_BIT].priority;
    tcbTaskInfo.Type        = taskInfo[Uart_BIT].taskType;
    tcbTaskInfo.modes       = taskInfo[Uart_BIT].modes;
    tcbTaskInfo.MIFrames    = taskInfo[Uart_BIT].MIFframes;
    tcbTaskInfo.Rmt.InitialMif = taskInfo[Uart_BIT].InitialMif;
    tcbTaskInfo.Rmt.MifRate    = taskInfo[Uart_BIT].MIFrate;
    tcbTaskInfo.Enabled     = TRUE;
    tcbTaskInfo.Locked      = FALSE;
    tcbTaskInfo.pParamBlock = (void *) &uartMgrBITBlock;
    TmTaskCreate ( &tcbTaskInfo );
    // Enable UartMgr Display Task
    memset(&tcbTaskInfo, 0, sizeof(tcbTaskInfo));
    strncpy_safe(tcbTaskInfo.Name, sizeof(tcbTaskInfo.Name),"Uart Mgr Debug Disp",_TRUNCATE);
    tcbTaskInfo.TaskID      = (TASK_INDEX)Uart_Disp_Debug;
    tcbTaskInfo.Function    = UartMgrDispDebug_Task;
    tcbTaskInfo.Priority    = taskInfo[Uart_Disp_Debug].priority;
    tcbTaskInfo.Type        = taskInfo[Uart_Disp_Debug].taskType;
    tcbTaskInfo.modes       = taskInfo[Uart_Disp_Debug].modes;
    tcbTaskInfo.MIFrames    = taskInfo[Uart_Disp_Debug].MIFframes;
    tcbTaskInfo.Rmt.InitialMif = taskInfo[Uart_Disp_Debug].InitialMif;
    tcbTaskInfo.Rmt.MifRate    = taskInfo[Uart_Disp_Debug].MIFrate;
    tcbTaskInfo.Enabled     = TRUE;
    tcbTaskInfo.Locked      = FALSE;
    tcbTaskInfo.pParamBlock = (void *) &uartMgrDispDebugBlock;
    TmTaskCreate ( &tcbTaskInfo );
  }
  // For UART[0] force to GSE default conditions
  // Initialize Uart Mgr Debug
  memset ( &m_UartMgr_Debug, 0x00, sizeof(UARTMGR_DEBUG) );
  m_UartMgr_Debug.bDebug = FALSE;
  m_UartMgr_Debug.ch = 1;
  m_UartMgr_Debug.num_bytes = 20;

#ifdef UARTMGR_TIMING_TEST
  /*vcast_dont_instrument_start*/
  for (i=0;i<UARTMGR_TIMING_BUFF_MAX;i++)
  {
    m_UARTMGRTiming_Buff[i] = 0;
  }
  m_UARTMGRTiming_Index = 0;
  m_UARTMGRTiming_Start = 0;
  m_UARTMGRTiming_End = 0;
  /*vcast_dont_instrument_end*/
#endif
}

/******************************************************************************
 * Function:    UartMgr_Task
 *
 * Description: Main UartMgr Processing Task, instantiated for each UART
 *
 * Parameters:  pParam - ptr to UartMgrBlock[]
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static void UartMgr_Task (void *pParam)
{
  UINT16 nChannel;
  UARTMGR_TASK_PARAMS_PTR pUartMgrBlock;
  UARTMGR_STATUS_PTR      pUartMgrStatus;
  UINT16 cnt;
  RESULT result;
  BOOLEAN bNewChanData;
  UARTMGR_CFG_PTR    pUartCfg;

  pUartMgrBlock = (UARTMGR_TASK_PARAMS_PTR) pParam;

  nChannel = pUartMgrBlock->nChannel;
  pUartMgrStatus = (UARTMGR_STATUS_PTR) &m_UartMgrStatus[nChannel];

  // If shutdown is signaled. Close my port
  // and tell Task Mgr to stop running me.
  if ( Tm.systemMode == SYS_SHUTDOWN_ID )
  {
    UART_ClosePort( (UINT8)pUartMgrBlock->nChannel);
    TmTaskEnable  ( pUartMgrBlock->taskID, FALSE);
  }
  else  // Normal task execution.
  {
    // Process data from Uart Rx Port if Any
    result = UART_Receive ( (UINT8)nChannel, (INT8 *) m_UartMgr_RawBuffer[nChannel],
                            UARTMGR_RAW_BUF_SIZE, &cnt );

    bNewChanData = FALSE;

    if (result != DRV_OK)
    {
      // Flush Uart is required !
      // On startup, the UART will overflow once
      UART_Purge ( (UINT8)nChannel );
    }

    // Update ByteCnt Status
    pUartMgrStatus->portByteCnt += cnt;

    ASSERT( NULL != pUartMgrBlock->exec_protocol);
    // Call Serial Protocol Handler
    bNewChanData = pUartMgrBlock->exec_protocol( (UINT8 *) m_UartMgr_RawBuffer[nChannel],
      cnt,
      nChannel,
      m_UartMgr_Data[nChannel],
      m_UartMgr_WordInfo[nChannel] );


    // If we received new data then update some status data.
    //    Note: Ch Data Loss determination performed in UartMgr_BITTask()
    if ( bNewChanData == TRUE )
    {
      // On transition to new "valid" data received, output a debug msg, similar
      //    to other I/F
      if ( pUartMgrStatus->bChanActive == FALSE )
      {
        GSE_DebugStr(NORMAL,TRUE,
                     "\r\nUartMgr Task: Valid Uart Data Present Detected (Ch=%d)\r\n",
                     nChannel);

        // On transition from timeout to NORMAL clear fault condition
        if ( (pUartMgrStatus->bChannelStartupTimeOut == TRUE) ||
             (pUartMgrStatus->bChannelTimeOut == TRUE) )
        {
          pUartCfg = (UARTMGR_CFG_PTR) &m_UartMgrCfg[nChannel];
          Flt_ClrStatus ( pUartCfg->channelSysCond );
        }

        pUartMgrStatus->bChanActive = TRUE;
        pUartMgrStatus->bChannelStartupTimeOut = FALSE;
        pUartMgrStatus->bChannelTimeOut = FALSE;

      }
      pUartMgrStatus->timeChanActive = CM_GetTickCount();
    }

    // Perform Param Data Loss calculation, here.  Might have to perform
    //   this logic on every frame and not just when new data is rx.
    //   For F7X, performing calc when no new data is OK for F7X  as it is
    //   frame based (when new frame is received, then all param are udpated).
    //   For param / word based protocols, have to monitor each invidual
    //   param continuously.
    UartMgr_DetermineParamDataLoss(nChannel);

    // Call data reduction for every new param or if data loss determination
    //    send last known param with flag set to invalid to data buffer.
    if ( pUartMgrStatus->bRecordingActive == TRUE )
    {
      UartMgr_UpdateDataReduction(nChannel, m_UartMgrCfg[nChannel].protocol);
    }

    // For debug display of raw input data
    if ( m_UartMgr_Debug.bDebug == TRUE )
    {
      if ( ( m_UartMgr_Debug.ch == nChannel ) && (cnt > 0) )
      {
        m_UartMgr_Debug.cnt += UartMgr_CopyRollBuff (
                        &m_UartMgr_Debug.data[0],
                        (UINT8 *) m_UartMgr_RawBuffer[nChannel],
                        cnt,
                        &m_UartMgr_Debug.writeOffset,
                        &m_UartMgr_Debug.readOffset,
                         m_UartMgr_Debug.num_bytes );
      }
    }
  }

}



/******************************************************************************
 * Function:     UartMgr_BITTask
 *
 * Description:  Performs the background CBIT Testing of the UART interfaces
 *
 * Parameters:   pParam
 *
 * Returns:      none
 *
 * Notes:
 *   1) Performs chan data loss processing
 *   2) Reads and collects all the UART Drv error statistics - TBD
 *
 *****************************************************************************/
static
void UartMgr_BITTask ( void *pParam )
{
  UARTMGR_STATUS_PTR pUartMgrStatus;
  UARTMGR_CFG_PTR    pUartCfg;
  UINT16  i;
  BOOLEAN bOk;
  UART_STATUS uartBIT_status;


  // Perform BIT only on channels that are enabled
  for (i = 0; i < (UINT16) UART_NUM_OF_UARTS; i++)
  {
    pUartCfg = (UARTMGR_CFG_PTR) &m_UartMgrCfg[i];
    pUartMgrStatus = (UARTMGR_STATUS_PTR) &m_UartMgrStatus[i];

    if ( (pUartCfg->bEnabled == TRUE) &&
         (pUartMgrStatus->status != UARTMGR_STATUS_FAULTED_PBIT) )
    {
      // Determine Ch Data Loss TimeOut
      bOk = UartMgr_DetermineChanLoss( i );

      // Read UART DRV error counts
      uartBIT_status = UART_BITStatus( (UINT8)i );
      pUartMgrStatus->portFramingErrCnt = uartBIT_status.FramingErrCnt;
      pUartMgrStatus->portParityErrCnt = uartBIT_status.ParityErrCnt;
      pUartMgrStatus->portTxOverFlowErrCnt = uartBIT_status.TxOverflowErrCnt;
      pUartMgrStatus->portRxOverFlowErrCnt = uartBIT_status.RxOverflowErrCnt;

      // In the future could add logic that if too many error counts, could
      //    consider this ch as "bad".

      // Do we have a CBIT failure ?
      if (bOk == FALSE)
      {
        pUartMgrStatus->status = UARTMGR_STATUS_FAULTED_CBIT;
      }
      else
      {
        pUartMgrStatus->status = UARTMGR_STATUS_OK;
      }

    } // End of if ->bEnabled == TRUE

  } // End of looping thru all the enabled Uart Channels
}

/******************************************************************************
 * Function:    UartMgr_SensorSetup
 *
 * Description: Sets the m_UartMgr_WordInfo[] table with parse info for a
 *              specific Uart Data Parameter
 *
 * Parameters:  gpA - General Purpose A (See below for definition)
 *              gpB - General Purpose B (See below for definition)
 *              label -   (See below for definition)
 *              nSensor - Sensor Index requesting Uart parameter
 *
 * Returns:     Index into m_UartMgr_WordInfo[] element
 *              (To be used in later Uart Sensor Processing)
 *              NOTE: Index bits 10 to 8 embeds the Uart ch
 *                          bits  7 to 0 embeds the index into word_info
 *
 *              NOTE: Index = UARTMGR_WORD_INDEX_NOT_FOUND returned
 *                            when requested param is not decodable
 * Notes:
 *
 *  gpA
 *  ===
 *     bits  7 to  0  Protocol Selection (see UARTMGR_PROTOCOLS)
 *                     0 - Not Used
 *                     1 - F7X-N-Parameter
 *                     2 - EMU 150
 *                     3 - ID Parameter
 *                     4 - GBS
 *                     5 - PWC Display
 *                     255 to 1 - Not Used
 *     bits 10 to  8: Selects the uart channel
 *                    000b - ch 0 (Allocated to GSE, not valid !)
 *                    001b - ch 1
 *                    010b - ch 2
 *                    011b - ch 3
 *                    111b to 100b not used
 *     bits 15 to 11: Word Size
 *     bits 20 to 16: MSB Position
 *     bits 22 to 21: 00 = "DISC" where all bits are used
 *                  : 01 = "BNR" where MSB = bit 15 and always == sign bit
 *                  : 10, 11 = Not Used
 *     bits 31 to 23: Not Used
 *
 *  gpB
 *  ===
 *     bits 31 to  0: timeout (msec)
 *
 *****************************************************************************/
UINT16 UartMgr_SensorSetup (UINT32 gpA, UINT32 gpB, UINT16 param, UINT16 nSensor)
{
  UARTMGR_PROTOCOLS protocol;
  UINT16 ch;

  UARTMGR_WORD_INFO_PTR word_info_ptr;
  BOOLEAN bOk;
  UINT16 wordIndex;
  UARTMGR_PARAM_DATA_PTR uart_data_ptr;


  protocol = (UARTMGR_PROTOCOLS) (gpA & 0xFF);
  ch = (UINT16)(gpA >> 8) & 0x7;

  wordIndex = UARTMGR_WORD_INDEX_NOT_FOUND;
  bOk = FALSE;

  word_info_ptr = (UARTMGR_WORD_INFO_PTR)
                  &m_UartMgr_WordInfo[ch][m_UartMgr_WordSensorCount[ch]];

  uart_data_ptr = (UARTMGR_PARAM_DATA_PTR)
                  m_UartMgr_Data[ch];

  // Determine Protocol
  switch ( protocol )
  {
    case UARTMGR_PROTOCOL_F7X_N_PARAM:
      bOk = F7XProtocol_SensorSetup ( gpA, gpB, (UINT8) param, word_info_ptr );
      break;
    case UARTMGR_PROTOCOL_ID_PARAM:
      bOk = IDParamProtocol_SensorSetup ( gpA, gpB, param, word_info_ptr, uart_data_ptr, ch );
      break;
    case UARTMGR_PROTOCOL_NONE:
    case UARTMGR_PROTOCOL_EMU150:
    case UARTMGR_PROTOCOL_GBS:
    case UARTMGR_PROTOCOL_PWC_DISPLAY:
    case UARTMGR_PROTOCOL_MAX:
    default:
      // This is a configuration error
      bOk = FALSE;
      break;
  }

  if (bOk == TRUE)
  {
    // Update nSensor here
    word_info_ptr->nSensor = nSensor;

    // Embed and update to next entry
    wordIndex = (ch << 8) | m_UartMgr_WordSensorCount[ch]++;
  }

  return (wordIndex);

}



/******************************************************************************
 * Function:     UartMgr_ReadWord
 *
 * Description:  Returns a current Uart Mgr Data Word Value from the
 *               m_UartMgr_Data[] data store
 *
 * Parameters:   nIndex - index into the m_UartMgr_WordInfo[] containing the
 *                        the specific Uart Mgr Word Data to read / parse
 *               *tickCount - ptr to return last rx update time of parameter
 *
 * Returns:      FLOAT32 the requested Uart Mgr Data Word Value
 *
 * Notes:        none
 *
 *****************************************************************************/
FLOAT32 UartMgr_ReadWord (UINT16 nIndex, UINT32 *tickCount)
{
  UARTMGR_WORD_INFO_PTR word_info_ptr;
  UARTMGR_PARAM_DATA_PTR data_ptr;
  UARTMGR_RUNTIME_DATA_PTR runtime_data_ptr;

  UINT16 ch;
  UINT16 wIndex;
  FLOAT32 fval;

  UINT16 val;
  SINT16 sval;

  fval = 0.0f;

  // Get channel for this sensor and Index into Word Info Data
  ch     = (nIndex >> UART_SENSOR_CHAN_SHIFT) & UART_SENSOR_CHAN_MASK;
  wIndex = (nIndex & UART_SENSOR_INDEX_MASK);

  word_info_ptr = (UARTMGR_WORD_INFO_PTR) &m_UartMgr_WordInfo[ch][wIndex];
  *tickCount = 0;

  if ( word_info_ptr->nIndex != UARTMGR_WORD_INDEX_NOT_FOUND )
  {
    data_ptr = (UARTMGR_PARAM_DATA_PTR) &m_UartMgr_Data[ch][word_info_ptr->nIndex];
    runtime_data_ptr = (UARTMGR_RUNTIME_DATA_PTR) &data_ptr->runtime_data;
    *tickCount = runtime_data_ptr->rxTime;

    // Protect against negative value when shifting val position
    ASSERT_MESSAGE( (word_info_ptr->msb_position <= UART_SENSOR_DEFAULT_MSB),
                    "Sensor GPA msb_position %d greater than %d",
                    word_info_ptr->msb_position, UART_SENSOR_DEFAULT_MSB );

    switch ( word_info_ptr->type  )
    {
      case UART_DATA_TYPE_BNR:
        // Interpret RAW Data as 2's complement 16 bit data
        // Parse the data based on user sensor setting
        // Normally msb_pos = 15 and word_size = 16 to return entire 16 bit
        val  = ((runtime_data_ptr->val_raw <<
                (UART_SENSOR_DEFAULT_MSB - word_info_ptr->msb_position))
                & 0x0000FFFFUL);
        sval = (SINT16) (val >> (UART_SENSOR_DEFAULT_WORD_SIZE - word_info_ptr->word_size));
        fval = (FLOAT32) sval;
        break;

      case UART_DATA_TYPE_DISC:
        // Parse Data Bits before the param (could be param with multiple params embedded).
        val  = ((runtime_data_ptr->val_raw <<
                (UART_SENSOR_DEFAULT_MSB - word_info_ptr->msb_position))
                & 0x0000FFFFUL);
        // Parse Data Bits after the param (could be param with multiple params embedded).
        val  = (val >> (UART_SENSOR_DEFAULT_WORD_SIZE - word_info_ptr->word_size));
        // return float32
        fval = (FLOAT32) val;
        break;

    case UART_DATA_TYPE_MAX:
    default:
        FATAL ("Unexpected UART_DATA_TYPE = %d", word_info_ptr->type);
        break;
    }
  }

  // Convert to Eng Units
  return ( fval );

}



/******************************************************************************
 * Function:     UartMgr_SensorTest
 *
 * Description:  Determines if the Data Loss Time Out for the sensor word
 *               has been exceeded.
 *
 * Parameters:   nIndex - index into the m_UartMgr_WordInfo[] containing the
 *                        the specific Uart Mgr Word Data to read / parse
 *
 * Returns:      FALSE - If word data loss timeout exceeded
 *               TRUE  - If new word data received
 *
 * Notes:        none
 *
 *****************************************************************************/
BOOLEAN UartMgr_SensorTest (UINT16 nIndex)
{
  BOOLEAN   bValid;
  UINT16    ch;
  UINT16    wordinfo_index;
  UINT16    uartdata_index;
  UARTMGR_WORD_INFO_PTR pWordInfo;
  UARTMGR_STATUS_PTR  pUartMgrStatus;

  UARTMGR_PARAM_DATA_PTR   pUartData;
  UARTMGR_RUNTIME_DATA_PTR pRunTimeData;

  SYS_UART_CBIT_FAIL_WORD_TIMEOUT_LOG  wordTimeoutLog;

  UARTMGR_TASK_PARAMS_PTR pUartMgrBlock;


  bValid = FALSE;

  ch = (nIndex >> 8) & 0x7;
  wordinfo_index = (nIndex & 0xFF);

  pUartMgrStatus = (UARTMGR_STATUS_PTR) &m_UartMgrStatus[ch];

  pWordInfo = (UARTMGR_WORD_INFO_PTR) &m_UartMgr_WordInfo[ch][wordinfo_index];
  uartdata_index = pWordInfo->nIndex;

  pUartMgrBlock = (UARTMGR_TASK_PARAMS_PTR) &uartMgrBlock[ch];


  // Note: Not a fan of the last condition for checking != UARTMGR_WORD_INDEX_NOT_FOUND
  //       Believe it should be ASSERT during cfg setup, but we are fairly inconsistent
  //       everywhere, add here just as "fall back".  Not level A/B, all conditions need
  //       not to be checked !!!
  if ( ((pUartMgrStatus->timeChanActive != 0) || (pUartMgrStatus->bChanActive == TRUE)) &&
        (pWordInfo->id != UARTMGR_WORD_ID_NOT_USED) )
  {
    pUartData = (UARTMGR_PARAM_DATA_PTR) &m_UartMgr_Data[ch][uartdata_index];
    pRunTimeData = (UARTMGR_RUNTIME_DATA_PTR) &pUartData->runtime_data;

    ASSERT( NULL != pUartMgrBlock->get_protocol_ready_hndl );

    if (((CM_GetTickCount() - pRunTimeData->rxTime) > pWordInfo->dataloss_time) &&
        (pUartMgrBlock->get_protocol_ready_hndl(ch) == TRUE))
    {
      // Update Log once per transition occurance !
      if (pWordInfo->bFailed == FALSE)
      {
        pWordInfo->bFailed = TRUE;

        GSE_DebugStr(NORMAL,TRUE, "Uart_SensorTest: Word Timeout (ch = %d, S = %d)\r\n",
                                   ch, pWordInfo->nSensor);

        wordTimeoutLog.result = SYS_UART_DATA_LOSS_TIMEOUT;
        snprintf( wordTimeoutLog.sFailMsg, sizeof(wordTimeoutLog.sFailMsg),
          "Xcptn: Word Timeout (ch = %d, S = %d)",
                 ch, pWordInfo->nSensor );

        // Log CBIT Here only on transition from data rx to no data rx
        LogWriteSystem( SYS_ID_UART_CBIT_WORD_TIMEOUT_FAIL, LOG_PRIORITY_LOW,
                        &wordTimeoutLog,
                       sizeof(SYS_UART_CBIT_FAIL_WORD_TIMEOUT_LOG), NULL );

      } // End of if ->bFailed != TRUE
    } // End of TimeOut occurred
    else
    {
      pWordInfo->bFailed = FALSE;
      bValid = TRUE;
    }
  } // End of Ch data has been received

  return (bValid);

}


/******************************************************************************
 * Function:     UartMgr_InterfaceValid
 *
 * Description:  Determines if any Uart Mgr Data has been received since
 *               startup.
 *
 * Parameters:   nIndex - Uart Channel
 *
 * Returns:      TRUE - Uart Mgr Data has been received since startup
 *               FALSE - Uart Mgr Data has NOT been received since startup
 *
 * Notes:        none
 *
 *****************************************************************************/
BOOLEAN UartMgr_InterfaceValid (UINT16 nIndex)
{
  BOOLEAN   bValid;
  UINT16    ch;
  UARTMGR_STATUS_PTR  pUartMgrStatus;


  ch = (nIndex >> 8) & 0x7;

  pUartMgrStatus = (UARTMGR_STATUS_PTR) &m_UartMgrStatus[ch];

  bValid = FALSE;

  if ( (pUartMgrStatus->status == UARTMGR_STATUS_OK) &&
     (pUartMgrStatus->bChanActive == TRUE) )
  {
    bValid = TRUE;
  }

  return (bValid);
}



/******************************************************************************
 * Function:     UartMgr_ReadBuffer
 *
 * Description:  Returns the Rx Processed Uart data from the specified
 *               ch to the calling application
 *
 * Parameters:   pDest - ptr to App Buffer to copy data
 *               chan - Rx Chan to process raw data
 *               nMaxByteSize - max size of App Buffer
 *
 * Returns:      Number of bytes returned
 *
 * Notes:        none
 *
 *****************************************************************************/
UINT16 UartMgr_ReadBuffer ( void *pDest, UINT32 chan, UINT16 nMaxByteSize)
{
  UINT16 cnt;
  UINT16 bytesToEnd;
  UINT16 bytesAtStart;

  UARTMGR_STORE_BUFF_PTR pUartStoreData;
  UINT8 *pBytes;


  pBytes = (UINT8 *) pDest;

  pUartStoreData = (UARTMGR_STORE_BUFF_PTR) &m_UartMgr_StoreBuffer[chan];

  cnt = (pUartStoreData->cnt > nMaxByteSize) ? nMaxByteSize : (UINT16)pUartStoreData->cnt;

  if (cnt > 0)
  {
    bytesToEnd = (UINT16)(UARTMGR_RAW_RX_BUFFER_SIZE - pUartStoreData->readOffset);
    if (cnt > bytesToEnd)
    {
      // Split into two memcopies
      memcpy( (UINT8 *)pBytes,
        (UINT8 *)&pUartStoreData->data[pUartStoreData->readOffset], bytesToEnd );
      bytesAtStart = cnt - bytesToEnd;
      pUartStoreData->readOffset = 0; // Wrap Case

      memcpy( (UINT8 *)(pBytes + bytesToEnd),
        (UINT8 *)&pUartStoreData->data[pUartStoreData->readOffset], bytesAtStart );
      pUartStoreData->readOffset += bytesAtStart; // Advance pointer to next byte to read
    }
    else
    {
      // Single mem copy
      memcpy ( (UINT8 *) pBytes,
        (UINT8 *) &pUartStoreData->data[pUartStoreData->readOffset], cnt);

      pUartStoreData->readOffset += cnt;

      pUartStoreData->readOffset =(pUartStoreData->readOffset>=UARTMGR_RAW_RX_BUFFER_SIZE) ?
          0 : pUartStoreData->readOffset;
    }

    // Note: cnt will always be <= pUartStoreData->Cnt
    pUartStoreData->cnt -= cnt;

  } // if cnt > 0

  return (cnt);

}


/******************************************************************************
 * Function:     UartMgr_ReadBufferSnapshot
 *
 * Description:  Returns the current snapshot view of all the word parameters.
 *               Only if label word data has been received is data returned.
 *
 * Parameters:   pDest - ptr to App Buffer to copy data
 *               chan - Rx Chan to process raw data
 *               nMaxByteSize - max size of App Buffer
 *               bBeginSnapshot - TRUE for begin snapshot
 *
 * Returns:      Number of bytes returned
 *
 * Notes:        none
 *
 *****************************************************************************/
UINT16 UartMgr_ReadBufferSnapshot ( void *pDest, UINT32 chan, UINT16 nMaxByteSize,
                                    BOOLEAN bBeginSnapshot )
{
  UINT16 cnt;

  cnt = UartMgr_ReadBuffSnapshotProtocol ( pDest, chan, nMaxByteSize, bBeginSnapshot,
                                           m_UartMgrCfg[chan].protocol );

  return (cnt);
}


/******************************************************************************
 * Function:     UartMgr_ResetBufferTimeCnt
 *
 * Description:  Resets the ->TimeCntReset used for determining
 *               UARTMGR_STORE_DATA->TimeDelta param
 *
 * Parameters:   chan - channel to update
 *
 * Returns:      none
 *
 * Notes:        none
 *
 *****************************************************************************/
void UartMgr_ResetBufferTimeCnt ( UINT32 chan )
{
  UARTMGR_STATUS_PTR pUartMgrStatus;

  pUartMgrStatus = (UARTMGR_STATUS_PTR) &m_UartMgrStatus[chan];
  pUartMgrStatus->timeCntReset = CM_GetTickCount() / MIF_PERIOD_mS;
}



/******************************************************************************
 * Function:     UartMgr_GetFileHdr
 *
 * Description:  Returns the current UART file header with current UART
 *               cfg required for decoding UART data log files
 *
 * Parameters:   pDest - pointer to destination buffer
 *               chan  - channel to request UART file header
 *               nMaxByteSize - maximum destination buffer size
 *
 * Returns:      cnt - actual number of bytes returned
 *
 * Notes:        none
 *
 *****************************************************************************/
UINT16 UartMgr_GetFileHdr ( void *pDest, UINT32 chan, UINT16 nMaxByteSize )
{
  UINT8 *pdest;
  UARTMGR_FILE_HDR        fileHdr;
  UARTMGR_TASK_PARAMS_PTR pUartMgrBlock;
  UARTMGR_CFG_PTR         pUartCfg;
  UINT16 nPort; 

  UINT16 cnt;
  
  nPort = (chan != GBS_SIM_PORT_INDEX) ? (UINT16) chan : GBSProtocol_SIM_Port();

  pUartMgrBlock = (UARTMGR_TASK_PARAMS_PTR) &uartMgrBlock[nPort];  

  // Update protocol file hdr first !
  pdest = (UINT8 *) pDest;
  pdest = (pdest + sizeof(UARTMGR_FILE_HDR));  // Move to end of UartMgr File Hdr
  
  ASSERT( NULL != pUartMgrBlock->get_protocol_fileHdr);
  cnt = pUartMgrBlock->get_protocol_fileHdr( pdest, nMaxByteSize - sizeof(UARTMGR_FILE_HDR),
                                             (UINT16)chan );

  // Update Uart file hdr
  memset ( (UINT8 *) &fileHdr, 0, sizeof(UARTMGR_FILE_HDR) );

  pUartCfg = (UARTMGR_CFG_PTR) &m_UartMgrCfg[nPort];
  fileHdr.size = cnt + sizeof(UARTMGR_FILE_HDR);
  fileHdr.ch = (UINT8)chan;
  strncpy ( (char *) fileHdr.sName, pUartCfg->sName, UART_CFG_NAME_SIZE );
  fileHdr.protocol = pUartCfg->protocol;

  // Copy Uart file hdr to destination
  memcpy ( pDest, (UINT8 *) &fileHdr, sizeof(UARTMGR_FILE_HDR) );

  return (fileHdr.size);

}

/******************************************************************************
 * Function:     UartMgr_GetSystemHdr
 *
 * Description:  Retrieves the binary system header for the UART
 *               configuration.
 *
 * Parameters:   void *pDest         - Pointer to storage buffer
 *               UINT16 nMaxByteSize - Amount of space in buffer
 *
 * Returns:      UINT16 - Total number of bytes written
 *
 * Notes:        None
 *
 *****************************************************************************/
UINT16 UartMgr_GetSystemHdr ( void *pDest, UINT16 nMaxByteSize )
{
   // Local Data
   UART_SYS_HDR uartSysHdr[UART_NUM_OF_UARTS];
   UINT16       nChannelIndex;
   INT8         *pBuffer;
   UINT16       nRemaining;
   UINT16       nTotal;

   // Initialize Local Data
   pBuffer    = (INT8 *)pDest;
   nRemaining = nMaxByteSize;
   nTotal     = 0;
   memset ( uartSysHdr, 0, sizeof(uartSysHdr) );
   // Loop through all the channels
   for ( nChannelIndex = 1;
         ((nChannelIndex < (UINT16)UART_NUM_OF_UARTS) &&
          (nRemaining > sizeof (UART_SYS_HDR)));
         nChannelIndex++ )
   {
      // Copy the UART Name to the header
      strncpy_safe( uartSysHdr[nChannelIndex-1].sName,
                    sizeof(uartSysHdr[nChannelIndex-1].sName),
                    m_UartMgrCfg[nChannelIndex].sName,
                    _TRUNCATE);
      // Increment the total used and decrement the remaining
      nTotal     += sizeof (UART_SYS_HDR);
      nRemaining -= sizeof (UART_SYS_HDR);
   }
   // Make sure there is something to write and then copy to buffer
   if ( 0 != nTotal )
   {
      memcpy ( pBuffer, uartSysHdr, nTotal );
   }
   // return the total bytes written to the buffer
   return ( nTotal );
}

/******************************************************************************
 * Function:    UartMgr_FlushChannels
 *
 * Description: Flushes all used / enable UART channels
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:
 *  - Should be called after UartMgr_Initialize() is called, otherwise just
 *    returns
 *
 *****************************************************************************/
void UartMgr_FlushChannels (void)
{
  UART_ID ch;

  for (ch=UART_1;ch<UART_NUM_OF_UARTS;ch++)
  {
    // Only if ch is enabled will we purge.  Expect m_UartMgrCfg[] to be cleared
    //   on startup.
    if ( ( m_UartMgrCfg[ch].bEnabled == TRUE ) &&
         ( m_UartMgrStatus[ch].status == UARTMGR_STATUS_OK ) )
    {
      UART_Purge ( (UINT8)ch );
    }
  }

}


/******************************************************************************
 * Function:    UartMgr_DownloadRecord
 *
 * Description: The UART Manager Download record manages the interface between
 *              the data manager and the UART protocol. It requests records
 *              from the attached device using the configured protocol.
 *
 * Parameters:  UINT8 PortIndex     - Index of the Channel to communicate on
 *              DL_STATUS *pStatus  - Storage location for the download status
 *              UINT32 *pSize,      - Storage location for the size of the record
                DL_WRITE_STATUS *pDL_WrStatus - Write Status storage location
                                                from the Data Manager
 *
 * Returns:     UINT8 Pointer to buffer containing record
 *
 * Notes:       None
 *
 *****************************************************************************/
UINT8* UartMgr_DownloadRecord ( UINT8 PortIndex, DL_STATUS *pStatus, UINT32 *pSize,
                                DL_WRITE_STATUS **pDL_WrStatus )
{
  UARTMGR_TASK_PARAMS_PTR pUartMgrBlock;
  UARTMGR_DOWNLOAD_PTR pUartMgrDownload;
  UINT8 nPort; 
  
  if (PortIndex == GBS_SIM_PORT_INDEX)
  {
    nPort = (UINT8) GBSProtocol_SIM_Port();
    pUartMgrBlock = (UARTMGR_TASK_PARAMS_PTR) &uartMgrBlock[nPort];
    pUartMgrDownload = (UARTMGR_DOWNLOAD_PTR) &m_UartMgr_Download_GBS_SIM;  
  }
  else 
  {
    pUartMgrBlock = (UARTMGR_TASK_PARAMS_PTR) &uartMgrBlock[PortIndex];
    pUartMgrDownload = (UARTMGR_DOWNLOAD_PTR) &m_UartMgr_Download[PortIndex];  
  }
  *pDL_WrStatus = &pUartMgrDownload->wrStatus;

  switch ( pUartMgrDownload->state)
  {
    case UARTMGR_DOWNLOAD_IDLE:
      // Tell Data Manager DL IN Process and clear size
      *pStatus = DL_IN_PROGRESS;
      *pSize = 0;

      // Kick start protocol download process and then transition to next record
      pUartMgrDownload->bStartDownload = TRUE;
      pUartMgrDownload->bDownloadCompleted = FALSE;

      pUartMgrDownload->bWriteInProgress = TRUE;  // Worse case set to WriteInProgress
      pUartMgrDownload->bWriteOk = FALSE;         // Worse case set to FALSE

      pUartMgrDownload->bNewRec = FALSE;

      // Go to next state
      pUartMgrDownload->state = UARTMGR_DOWNLOAD_GET_REC;

      ASSERT( NULL != pUartMgrBlock->download_protocol_hndl );
      pUartMgrBlock->download_protocol_hndl ( PortIndex,
                                              &pUartMgrDownload->bStartDownload,
                                              &pUartMgrDownload->bDownloadCompleted,
                                              &pUartMgrDownload->bWriteInProgress,
                                              &pUartMgrDownload->bWriteOk,
                                              &pUartMgrDownload->bHalt,
                                              &pUartMgrDownload->bNewRec,
                                              &pUartMgrDownload->pSize,
                                              &pUartMgrDownload->pData );

      *pSize = *pUartMgrDownload->pSize;
      break;

    case UARTMGR_DOWNLOAD_GET_REC:
      *pStatus = DL_IN_PROGRESS;
      // Ptr to bDownloadCompleted passed to Protocol to be updated !
      if ( pUartMgrDownload->bDownloadCompleted == TRUE )
      {
        *pStatus = DL_NO_RECORDS;
        *pSize = 0;
        UartMgr_ClearDownloadState(PortIndex);
      }
      else
      {
        // Ptr to UartMgr size param to be updated by Protocol when record received
        if ( pUartMgrDownload->bNewRec == TRUE )
        {
          *pStatus = DL_RECORD_FOUND;
          *pSize = *pUartMgrDownload->pSize;  // Return size of packet to DM
          // pUartMgrDownload->pData;         // Return ptr to buffer to packet to DM

          pUartMgrDownload->state = UARTMGR_DOWNLOAD_REC_STR_INPROGR;
          pUartMgrDownload->bNewRec = FALSE;
        }
      }
      break;

    case UARTMGR_DOWNLOAD_REC_STR_INPROGR:
      *pStatus = DL_IN_PROGRESS; // Immediately Say InProgress
      if ( pUartMgrDownload->wrStatus != DL_WRITE_IN_PROGRESS )
      {
        if ( pUartMgrDownload->wrStatus == DL_WRITE_SUCCESS )
        {
          pUartMgrDownload->bWriteOk = TRUE;
        }
        else
        {
          pUartMgrDownload->bWriteOk = FALSE;
        }
        pUartMgrDownload->bWriteInProgress = FALSE;
        pUartMgrDownload->bDownloadCompleted = FALSE;
        pUartMgrDownload->state = UARTMGR_DOWNLOAD_GET_REC;
        *pSize = 0;
      }
      break;

    case UARTMGR_DOWNLOAD_MAX:
    default:
       FATAL("Unsupported UART Download State = %d", pUartMgrDownload->state);
      break;
  }

  return (pUartMgrDownload->pData);

}


/******************************************************************************
 * Function:    UartMgr_DownloadStop
 *
 * Description: For protocols that have download options, this func provides
 *              the mechansim to HALT an inprogress download
 *
 * Parameters:  PortIndex - uart channel
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
void UartMgr_DownloadStop ( UINT8 PortIndex )
{
  UARTMGR_DOWNLOAD_PTR pUartMgrDownload;

  pUartMgrDownload = (PortIndex != GBS_SIM_PORT_INDEX) ? 
                     (UARTMGR_DOWNLOAD_PTR) &m_UartMgr_Download[PortIndex] :
                     (UARTMGR_DOWNLOAD_PTR) &m_UartMgr_Download_GBS_SIM; 

  pUartMgrDownload->bHalt = TRUE;

  pUartMgrDownload->state = UARTMGR_DOWNLOAD_IDLE;
  pUartMgrDownload->bStartDownload = FALSE;

}


/******************************************************************************
 * Function:    UartMgr_DownloadClr
 *
 * Description: For Dnload protocols that have "house cleaning" to perform 
 *              that can be invoke via State Machine Transition
 *
 * Parameters:  Run - Not used, but passed to Protocol for specific operation
 *              param - Not used, but passed to Protocol for specific operation
 *
 * Returns:     None
 *
 * Notes:      
 *  - Start @ UART ch 1 as ch 0 is allocated to GSE
 *  - On first occurence of GBS Protocol, call _DonwloadClrHndl once, then exit.
 *    Clears all channels set for GBS Protocol
 *
 *****************************************************************************/
void UartMgr_DownloadClr ( BOOLEAN Run, INT32 param )
{
  UINT16 i; 
  
  for (i=1;i< (UINT16) UART_NUM_OF_UARTS;i++) {
    if ( uartMgrBlock[i].download_protocol_clr_hndl == GBSProtocol_DownloadClrHndl ) {
      uartMgrBlock[i].download_protocol_clr_hndl( Run, param ); 
      break; // Exit for loop on first occurance
    }
  }
}

/******************************************************************************
 * Function:     UartMgr_ReadESN
 *
 * Description:  Returns the ESN decoded by the protocol (if available)
 *
 * Parameters:   ch - Uart Ch Protocol to request ESN from 
 *               *esn_ptr - Ptr where ESN will be returned to App
 *               cnt - Max char to return to the *esn_ptr
 *
 * Returns:      TRUE - if ESN has been decoded "once" from bus on this power up
 *
 * Notes:        none
 *
 *****************************************************************************/
BOOLEAN UartMgr_ReadESN ( UINT16 ch, CHAR *esn_ptr, UINT16 cnt )
{
  return ( uartMgrBlock[ch].get_protocol_esn( ch, esn_ptr, cnt ) );
}

/******************************************************************************
 * Function:    UartMgr_ClearDownloadState
 *
 * Description: Resets the download state back to IDLE
 *
 * Parameters:  PortIndex - uart channel
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static
void UartMgr_ClearDownloadState ( UINT8 PortIndex )
{
  UARTMGR_DOWNLOAD_PTR pUartMgrDownload;

  pUartMgrDownload = (PortIndex != GBS_SIM_PORT_INDEX) ? 
                     (UARTMGR_DOWNLOAD_PTR) &m_UartMgr_Download[PortIndex] :
                     (UARTMGR_DOWNLOAD_PTR) &m_UartMgr_Download_GBS_SIM; 
  
  pUartMgrDownload->state = UARTMGR_DOWNLOAD_IDLE;

  pUartMgrDownload->bStartDownload = FALSE;
  pUartMgrDownload->bDownloadCompleted = FALSE;

  pUartMgrDownload->bWriteInProgress = TRUE;
  pUartMgrDownload->bWriteOk = FALSE;

  pUartMgrDownload->pSize = NULL;
  pUartMgrDownload->pData = NULL;

  pUartMgrDownload->bHalt = FALSE;

  pUartMgrDownload->bNewRec = FALSE;
}

/******************************************************************************
 * Function:    UartMgr_GetChannel
 *
 * Description: Utility function that returns a protocol UART port.
 *
 * Parameters:  port         - The UART port
 *
 * Returns:     BYTE channel - The UART channel
 *
 * Notes:       None
 *
 *****************************************************************************/

BYTE UartMgr_GetChannel(UINT32 port)
{
  BYTE i;
  BYTE chan = 0;
  for (i = 1; i < (UINT8)UART_NUM_OF_UARTS; i++)
  {
    if (uartMgrBlock[i].protocol_ID == (UARTMGR_PROTOCOLS)port)
    {
      chan = i;
      break;
    }
  }
  return(chan);
}

/******************************************************************************
 * Function:    UartMgr_Read
 *
 * Description: Utility function that allows a protocol to communicate in
 *              simple fixed message formats with its application.
 *
 * Parameters:  *pDest       - The destination to Read the Data store into
 *              port         - The UART port
 *              Direction    - FALSE -> RX or TRUE -> TX
 *              nMaxByteSize - The total size in bytes being Read
 *
 * Returns:     BOOLEAN bStatus - Verifying New Data Available
 *
 * Notes:       None
 *
 *****************************************************************************/
BOOLEAN UartMgr_Read(void *pDest, UINT32 port, UINT16 Direction,
                     UINT16 nMaxByteSize)
{
  BYTE chan = UartMgr_GetChannel(port);
  BOOLEAN bStatus;

  bStatus = uartMgrBlock[chan].read_protocol(pDest, chan, Direction,
                                             nMaxByteSize);
  return (bStatus);
}

/******************************************************************************
 * Function:    UartMgr_Write
 *
 * Description: Utility function that allows a protocol to communicate in
 *              simple fixed message formats with its application.
 *
 * Parameters:  *pDest       - The destination to Read the Data store into
 *              port         - The UART port
 *              Direction    - FALSE -> RX or TRUE -> TX
 *              nMaxByteSize - The total size in bytes being Read
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
void UartMgr_Write(void *pDest, UINT32 port, UINT16 Direction, 
                   UINT16 nMaxByteSize)
{
  BYTE chan = UartMgr_GetChannel(port);
  uartMgrBlock[chan].write_protocol(pDest, chan, Direction, nMaxByteSize);
}


/******************************************************************************
 * Function:    UartMgrDispDebug_Task
 *
 * Description: Utility function to display raw Uart Incoming Data
 *
 * Parameters:  pParam - not used.
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static
void UartMgrDispDebug_Task ( void *pParam )
{
  UINT16 i;


  if ( (m_UartMgr_Debug.bDebug == TRUE ) && (m_UartMgr_Debug.cnt != 0) )
  {
    snprintf( (char *) str, sizeof(str), "Last %d bytes: \r\n", m_UartMgr_Debug.num_bytes );
    GSE_PutLine( (const char *) str );

    // Output up to .num_bytes or .cnt -> 0 in if below.
    for ( i = 0; i < m_UartMgr_Debug.num_bytes; i++ )
    {
      // Output one char per line
      snprintf( (char *) str, sizeof(str),
        "%02x ", m_UartMgr_Debug.data[m_UartMgr_Debug.readOffset++] );
      GSE_PutLine( (const char *) str );

      // Check for Wrap
      if (m_UartMgr_Debug.readOffset >= m_UartMgr_Debug.num_bytes)
      {
        m_UartMgr_Debug.readOffset = 0;
      }

      // Dec count
      m_UartMgr_Debug.cnt--;
      if ( m_UartMgr_Debug.cnt == 0)
      {
        break; // Exit
      }
    } // End for loop thru .num_bytes

    GSE_PutLine( NEW_LINE );
    GSE_PutLine( NEW_LINE );

  } // End of .bDebug is Enabled
}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/
/******************************************************************************
 * Function:    UartMgr_InitData
 *
 * Description: Initializes the UartMgr Data, WordInfo Data and Protocol 
 *              handlers.
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:
 *  
 *****************************************************************************/
static void UartMgr_InitData(void)
{
  UINT16 i;
  UINT16 k;
  UARTMGR_WORD_INFO_PTR    pWordInfo;
  UARTMGR_PARAM_DATA_PTR   pUartData;
  UARTMGR_RUNTIME_DATA_PTR runtime_data_ptr;  
  
  // UartMgr_Data.runtime_data need to be inititalized to UARTMGR_WORD_ID_NOT_INIT
  for (k = 0; k < (UINT8)UART_NUM_OF_UARTS; k++)
  {
    for (i = 0; i < UARTMGR_MAX_PARAM_WORD; i++)
    {
      pUartData = (UARTMGR_PARAM_DATA_PTR) &m_UartMgr_Data[k][i];
      runtime_data_ptr = (UARTMGR_RUNTIME_DATA_PTR) &pUartData->runtime_data;
      runtime_data_ptr->id = UARTMGR_WORD_ID_NOT_INIT;
      // Also set flag to ReInit to TRUE to call DataReductionInit() later on
      runtime_data_ptr->bReInit = TRUE;
    }
  }
  // UartMgr_WordInfo.nIndex to UARTMGR_WORD_INDEX_NOT_FOUND
  for (k = 0; k < (UINT8)UART_NUM_OF_UARTS; k++)
  {
    pWordInfo = (UARTMGR_WORD_INFO_PTR) &m_UartMgr_WordInfo[k][0];
    for (i = 0; i < UARTMGR_MAX_PARAM_WORD; i++)
    {
      pWordInfo->nIndex = UARTMGR_WORD_INDEX_NOT_FOUND;
      pWordInfo->id = UARTMGR_WORD_ID_NOT_USED;  // Initialize to Not Used.  Init when
      //   _SensorSetup() called.
      pWordInfo++;
    }
    m_UartMgr_WordSensorCount[k] = 0;
  }	
  
  // Initialize all protocol handlers after UargMgrConfig restored
  F7XProtocol_Initialize();
  EMU150Protocol_Initialize();
  IDParamProtocol_Initialize();
  GBSProtocol_Initialize(); 
  PWCDispProtocol_Initialize();
}

/******************************************************************************
 * Function:    UartMgr_GetStatus
 *
 * Description: Utility function to request current UartMgr[index] status
 *
 * Parameters:  portIndex - Uart Port Index
 *
 * Returns:     UARTMGR_STATUS_PTR to m_UartMgrStatus[index]
 *
 * Notes:       None
 *
 *****************************************************************************/
static UARTMGR_STATUS_PTR UartMgr_GetStatus (UINT8 portIndex)
{
  return ( (UARTMGR_STATUS_PTR) &m_UartMgrStatus[portIndex] );
}

/******************************************************************************
 * Function:    UartMgr_GetDebug
 *
 * Description: Utility function to request current UartMgr debug state
 *
 * Parameters:  None
 *
 * Returns:     Ptr to UartMgr Debug Data
 *
 * Notes:       None
 *
 *****************************************************************************/
static UARTMGR_DEBUG_PTR UartMgr_GetDebug (void)
{
  return ( (UARTMGR_DEBUG_PTR) &m_UartMgr_Debug );
}


/******************************************************************************
 * Function:     UartMgr_DetermineChanLoss
 *
 * Description:  Determines startup and data loss timeout processing for the
 *                Uart Channel
 *
 * Parameters:   ch - Uart channel to perform data loss processing
 *
 * Returns:      none
 *
 * Notes:        none
 *
 *****************************************************************************/
static
BOOLEAN UartMgr_DetermineChanLoss (UINT16 ch)
{
  UARTMGR_CFG_PTR     pUartMgrCfg;
  UARTMGR_STATUS_PTR  pUartMgrStatus;

  BOOLEAN  *pTimeOutFlag;
  UINT32   nTimeOut;

  BOOLEAN  bOk;


  bOk = TRUE;

  pUartMgrCfg = (UARTMGR_CFG_PTR) &m_UartMgrCfg[ch];
  pUartMgrStatus = (UARTMGR_STATUS_PTR) &m_UartMgrStatus[ch];

  // Determine if performing startup or data lost timeout calculation
  if ( pUartMgrStatus->timeChanActive != 0 )
  {
    nTimeOut = pUartMgrCfg->channelTimeOut_s;
    pTimeOutFlag = (BOOLEAN *) &pUartMgrStatus->bChannelTimeOut;
  }
  else
  {
    nTimeOut = pUartMgrCfg->channelStartup_s;
    pTimeOutFlag = (BOOLEAN *) &pUartMgrStatus->bChannelStartupTimeOut;
  }

  if ( ((CM_GetTickCount() - pUartMgrStatus->timeChanActive) >
        (nTimeOut * MILLISECONDS_PER_SECOND)) && (nTimeOut != UART_DSB_TIMEOUT) )
  {
    // We have a timeout !
    bOk = FALSE;

    // Only record and update status on transition
    if (*pTimeOutFlag != TRUE)
    {
      *pTimeOutFlag = TRUE;
      if (pTimeOutFlag == &pUartMgrStatus->bChannelTimeOut)
      {
        UartMgr_CreateTimeOutSystemLog(SYS_UART_DATA_LOSS_TIMEOUT, ch);
        GSE_DebugStr(NORMAL,TRUE, "\r\nUartMgr: Uart Data Loss Timeout (Ch=%d) !\r\n", ch);
      }
      else
      {
        UartMgr_CreateTimeOutSystemLog(SYS_UART_STARTUP_TIMEOUT, ch);
        GSE_DebugStr(NORMAL,TRUE,"\r\nUartMgr: Uart StartUp Timeout (Ch=%d) !\r\n", ch);
      }

      // Increment data loss counter !
      pUartMgrStatus->dataLossCnt++;

      // Ch is now inactive !
      pUartMgrStatus->bChanActive = FALSE;

    } // End of if (*pTimeOutFlag != TRUE)
  } // End of if time exceeded the timeout value

  return (bOk);

}


/******************************************************************************
 * Function:     UartMgr_DetermineParamDataLoss
 *
 * Description:  Determines individual data loss timeout processing for the
 *                 Uart Channel.
 *
 * Parameters:   ch - Uart channel to perform param data loss processing
 *
 * Returns:      none
 *
 * Notes:        none
 *
 *****************************************************************************/
static
void UartMgr_DetermineParamDataLoss ( UINT16 ch )
{
  UARTMGR_PARAM_DATA_PTR pUartMgrData;
  UARTMGR_RUNTIME_DATA_PTR runtime_data_ptr;

  UINT32 tick;
  UINT32 i;


  pUartMgrData = (UARTMGR_PARAM_DATA_PTR) &m_UartMgr_Data[ch][0];

  tick = CM_GetTickCount();

  for (i = 0; i < UARTMGR_MAX_PARAM_WORD; i++)
  {

    runtime_data_ptr = (UARTMGR_RUNTIME_DATA_PTR) &pUartMgrData->runtime_data;

    // If run time param data has been initialized and param is currently not
    //   invalid, perform param data loss calc.
    if ( (runtime_data_ptr->id != UARTMGR_WORD_ID_NOT_INIT) &&
         (runtime_data_ptr->bValid != FALSE ) )
    {
      // Determine if timeout exceeded
      if ( (runtime_data_ptr->cfgTimeOut != F7X_PARAM_NO_TIMEOUT) &&
           ((tick - runtime_data_ptr->rxTime) > runtime_data_ptr->cfgTimeOut) )
      {
        // Update new val and bva  lid
        runtime_data_ptr->bNewVal = TRUE;
        runtime_data_ptr->bValid = FALSE;
        runtime_data_ptr->bReInit = TRUE;
      }
    }
    else
    {
      // The runtime_data should be initialized in sequential order.
      // First id encountered == UARTMGR_WORD_ID_NOT_INIT, identifies EOF
      break;
    }

    pUartMgrData++;

  } // End for looping thru all param entries

}



/******************************************************************************
 * Function:     UartMgr_CreateTimeOutSystemLog
 *
 * Description:  Creates the Uart TimeOut System Logs
 *
 * Parameters:   resultType = SYS_UART_STARTUP_TIMEOUT or
 *                            SYS_UART_DATA_LOSS_TIMEOUT
 *               ch - Uart Channel
 *
 * Returns:      none
 *
 * Notes:        none
 *
 *****************************************************************************/
static
void UartMgr_CreateTimeOutSystemLog (RESULT resultType, UINT16 ch)
{
  UART_TIMEOUT_LOG timeoutLog;
  UARTMGR_CFG_PTR pUartCfg;
  SYS_APP_ID sysId = SYS_ID_NULL_LOG;

  pUartCfg = (UARTMGR_CFG_PTR) &m_UartMgrCfg[ch];

  // fill in the structure
  timeoutLog.result = resultType;
  timeoutLog.ch = (UINT8)ch;

  if (SYS_UART_STARTUP_TIMEOUT == resultType)
  {
     timeoutLog.cfg_timeout = pUartCfg->channelStartup_s;
     sysId = SYS_ID_UART_CBIT_STARTUP_FAIL;
  }
  else if (SYS_UART_DATA_LOSS_TIMEOUT == resultType)
  {
     timeoutLog.cfg_timeout = pUartCfg->channelTimeOut_s;
     sysId = SYS_ID_UART_CBIT_DATA_LOSS_FAIL;
  }
  else
  {
     // No such case !
     FATAL("Unrecognized Sys Uart resultType = %d", resultType);
  }

  Flt_SetStatus(pUartCfg->channelSysCond, sysId, &timeoutLog, sizeof(timeoutLog));

}


/******************************************************************************
 * Function:     UartMgr_UpdateDataReduction
 *
 * Description:  Update data reduction on the specified parameter if necessary.
 *
 * Parameters:   ch - Uart channel
 *               protocols - UARTMGR_PROTOCOLS
 *
 * Returns:      none
 *
 * Notes:        none
 *
 *****************************************************************************/
 static
 void UartMgr_UpdateDataReduction ( UINT16 ch, UARTMGR_PROTOCOLS protocols )
 {
   UARTMGR_PARAM_DATA_PTR   pUartData;
   UARTMGR_RUNTIME_DATA_PTR runtime_data_ptr;
   REDUCTIONDATASTRUCT_PTR  data_red_ptr;
   UARTMGR_STATUS_PTR       pUartMgrStatus;

   BOOLEAN bRecord;
   UARTMGR_STORE_BUFF_PTR pUartStoreData;
   UARTMGR_STORE_DATA     storeData;
   UARTMGR_STORE_DATA_ID  storeDataID;

   UINT8 *timeDelta_ptr;
   UINT8 *paramId_U8_ptr;
   UINT16 *paramId_U16_ptr;
   UINT16 *paramVal_ptr;
   BOOLEAN *bValidity_ptr;
   void *store_ptr;
   UINT16 storeSize;

   UINT32 i;
   UINT16 cnt;
   UINT32 rxTime;
   SINT16 sval;

   #pragma ghs nowarning 1545 //Suppress packed structure alignment warning
   if (protocols == UARTMGR_PROTOCOL_ID_PARAM)
   {
     timeDelta_ptr = &storeDataID.timeDelta;
     paramId_U16_ptr = &storeDataID.paramId;
     paramVal_ptr = &storeDataID.paramVal;
     bValidity_ptr = &storeDataID.bValidity;
     store_ptr = &storeDataID;
     storeSize = sizeof(storeDataID);
   }
   else //all other protocols
   {
     timeDelta_ptr = &storeData.timeDelta;
     paramId_U8_ptr = &storeData.paramId;
     paramVal_ptr = &storeData.paramVal;
     bValidity_ptr = &storeData.bValidity;
     store_ptr = &storeData;
     storeSize = sizeof(storeData);
   }
   #pragma ghs endnowarning

   pUartData = (UARTMGR_PARAM_DATA_PTR) &m_UartMgr_Data[ch][0];
   pUartMgrStatus = (UARTMGR_STATUS_PTR) &m_UartMgrStatus[ch];

   // Loop thru all parameters and determine if new value needs to be saved
   for (i = 0; i < UARTMGR_MAX_PARAM_WORD; i++)
   {
     bRecord = FALSE;
     runtime_data_ptr = (UARTMGR_RUNTIME_DATA_PTR) &pUartData->runtime_data;
     data_red_ptr = (REDUCTIONDATASTRUCT_PTR) &pUartData->reduction_data;

     if ( runtime_data_ptr->id == UARTMGR_WORD_ID_NOT_INIT )
     {
       break; // Exit if reach end of defined (and initailzed by Protocol Rx) list.
     }

     if ( runtime_data_ptr->bNewVal == TRUE )
     {
       // Convert to EngVal
       data_red_ptr->EngVal = UartMgr_ConvertToEngUnits ( runtime_data_ptr );

       // Update the current time
       data_red_ptr->Time = runtime_data_ptr->rxTime;

       if ( runtime_data_ptr->bValid == TRUE )
       {
         if ( runtime_data_ptr->bReInit == FALSE )
         {
           // Call data reduction here
           bRecord = (RS_KEEP == DataReduction ( data_red_ptr )) ? TRUE : FALSE;
           // bRecord = TRUE;  // TEST !!!
         }
         else  // Transitioned from Invalid to Valid data
         {
           bRecord = TRUE;
           DataReductionInit ( data_red_ptr );
           runtime_data_ptr->bReInit = FALSE;   // Clear Re-Init flag !
         }
         rxTime = runtime_data_ptr->rxTime;
       } // End of if == TRUE
       else
       {
         // Have transition to invalid, record
         DataReductionGetCurrentValue ( data_red_ptr );
         rxTime = CM_GetTickCount();
         bRecord = TRUE;
       } // End of else ->bValid == FALSE

       // Record param
       if ( bRecord == TRUE )
       {

         // If rxTime < TimeCntReset then TimeCntReset must have resetted before
         //   param was selected for storage.  This case should normally not
         //   occur as UartTask and DataTask are DT tasks.  Only one task
         //   can be running at a time, thus param should have been marked
         //   for storage before TimeCntReset can be "cleared".
         // For ->bValid == FALSE case, the rxTime will be forced to the current
         //   time, thus avoiding rxTime < TimeCntReset.
         // Note: For new update rxTime should (not guarantee if
         //           UartMgr_Task sample time changes) be tick from current frame,
         //       For invalid, we want current frame time !
         rxTime = rxTime / MIF_PERIOD_mS;
         *timeDelta_ptr = (rxTime > pUartMgrStatus->timeCntReset) ?
                          (UINT8)(rxTime - pUartMgrStatus->timeCntReset) :
                          (UINT8)(UARTMGR_TIMEDELTA_OUTOFSEQUENCE);
         if ( protocols == UARTMGR_PROTOCOL_ID_PARAM ) {
           *paramId_U16_ptr = (UINT16)runtime_data_ptr->id;
         }
         else {
           *paramId_U8_ptr = (UINT8)runtime_data_ptr->id;
         }

         sval = (SINT16) (data_red_ptr->EngVal / runtime_data_ptr->scale_lsb);
         *paramVal_ptr = *(UINT16 *) &sval;

         *bValidity_ptr = runtime_data_ptr->bValid;

         pUartStoreData = (UARTMGR_STORE_BUFF_PTR) &m_UartMgr_StoreBuffer[ch];

         cnt = UartMgr_CopyRollBuff ( (UINT8 *) &pUartStoreData->data[0],
                            (UINT8 *) store_ptr, storeSize,
                            (UINT32 *) &pUartStoreData->writeOffset,
                            (UINT32 *) &pUartStoreData->readOffset,
                            UARTMGR_RAW_RX_BUFFER_SIZE );

          pUartStoreData->cnt += cnt;

       } // End of record param

       runtime_data_ptr->bNewVal = FALSE;

     } // End of if (runtime_data_ptr->bNewVal == TRUE)

     pUartData++; // Next Entry

   } // End of for loop thru all param entries

 }


/******************************************************************************
 * Function:     UartMgr_ReadBuffSnapshotProtocol
 *
 * Description:  Returns the current snapshot view of all the word parameters.
 *               Only if label word data has been received is data returned.
 *
 * Parameters:   pDest - ptr to App Buffer to copy data
 *               chan - Rx Chan to process raw data
 *               nMaxByteSize - max size of App Buffer
 *               bBeginSnapshot - TRUE for begin snapshot
 *               protocols - UARTMGR_PROTOCOLS
 *
 * Returns:      Number of bytes returned
 *
 * Notes:        none
 *
 *****************************************************************************/
static UINT16 UartMgr_ReadBuffSnapshotProtocol ( void *pDest, UINT32 chan,
                 UINT16 nMaxByteSize, BOOLEAN bBeginSnapshot, UARTMGR_PROTOCOLS protocols )
{
  UINT16 cnt;

  UARTMGR_PARAM_DATA_PTR pUartData;
  UARTMGR_RUNTIME_DATA_PTR runtime_data_ptr;
  REDUCTIONDATASTRUCT_PTR reduction_data_ptr;

  UINT8 *dest_ptr;
  UINT32 i;
  SINT16 sval;

  UARTMGR_STORE_SNAP_DATA storeData;
  UARTMGR_STORE_SNAP_DATA_ID  storeDataID;

  TIMESTAMP *ts_ptr;
  UINT8 *paramId_U8_ptr;
  UINT16 *paramId_U16_ptr;
  UINT16 *paramVal_ptr;
  BOOLEAN *bValidity_ptr;
  void *store_ptr;
  UINT16 storeSize;


  UARTMGR_STATUS_PTR pUartMgrStatus;

  BOOLEAN bDestBuffNotFull;


#ifdef UARTMGR_TIMING_TEST
  /*vcast_dont_instrument_start*/
   m_UARTMGRTiming_Start = TTMR_GetHSTickCount();
   /*vcast_dont_instrument_end*/
#endif

  #pragma ghs nowarning 1545 //Suppress packed structure alignment warning
  if (protocols == UARTMGR_PROTOCOL_ID_PARAM)
  {
    ts_ptr = &storeDataID.ts;
    paramId_U16_ptr = &storeDataID.paramId;
    paramVal_ptr = &storeDataID.paramVal;
    bValidity_ptr = &storeDataID.bValidity;
    store_ptr = &storeDataID;
    storeSize = sizeof(storeDataID);
  }
  else //all other protocols
  {
    ts_ptr = &storeData.ts;
    paramId_U8_ptr = &storeData.paramId;
    paramVal_ptr = &storeData.paramVal;
    bValidity_ptr = &storeData.bValidity;
    store_ptr = &storeData;
    storeSize = sizeof(storeData);
  }
  #pragma ghs endnowarning

  pUartData = (UARTMGR_PARAM_DATA_PTR) &m_UartMgr_Data[chan][0];
  cnt = 0;
  dest_ptr = (UINT8 *) pDest;
  pUartMgrStatus = (UARTMGR_STATUS_PTR) &m_UartMgrStatus[chan];

  bDestBuffNotFull = TRUE;

  // Set Recording Status
  if ( bBeginSnapshot == TRUE )
  {
    pUartMgrStatus->bRecordingActive = TRUE;
  }
  else
  {
    pUartMgrStatus->bRecordingActive = FALSE;
  }

  // Loop thru all parameters and calc the current data reduction value
  for (i = 0; i < UARTMGR_MAX_PARAM_WORD; i++)
  {
    runtime_data_ptr = (UARTMGR_RUNTIME_DATA_PTR) &pUartData->runtime_data;
    reduction_data_ptr = (REDUCTIONDATASTRUCT_PTR) &pUartData->reduction_data;

    if ( ( runtime_data_ptr->id != UARTMGR_WORD_ID_NOT_INIT ) &&
         ( bDestBuffNotFull == TRUE ) )
    {
      // Return current data reduced value
      *ts_ptr = runtime_data_ptr->rxTS;
      if ( protocols == UARTMGR_PROTOCOL_ID_PARAM ) {
        *paramId_U16_ptr = (UINT16)runtime_data_ptr->id;
      }
      else {
        *paramId_U8_ptr = (UINT8)runtime_data_ptr->id;
      }
      *bValidity_ptr = runtime_data_ptr->bValid;

      reduction_data_ptr->EngVal = UartMgr_ConvertToEngUnits ( runtime_data_ptr );

      reduction_data_ptr->Time = runtime_data_ptr->rxTime;

      if ( bBeginSnapshot == TRUE )
      {
        // Call Init Data Reduction for Begin Snapshot, for any data that has been received !
        DataReductionInit ( reduction_data_ptr );
        // Conversion at this point not needed, first point !
        *paramVal_ptr = runtime_data_ptr->val_raw;

        if (runtime_data_ptr->bValid == TRUE)
        {
          // Clear Re-Init flag as we have just called DataReductionInit()
          runtime_data_ptr->bReInit = FALSE;
        }
        // Note: If bValid == FALSE, indicates Uart Data was received then was lost again.
        //       In this case we can return a snapshot with the last known value
        //       and when data is received again, DataReductionInit() will be recalled to
        //       restart !
      }
      else
      {
        // Call Get current value for End Snapshot.
        DataReductionGetCurrentValue ( reduction_data_ptr );

        sval = (SINT16) ( reduction_data_ptr->EngVal / runtime_data_ptr->scale_lsb );
        *paramVal_ptr = *(UINT16 *) &sval;
      }

      bDestBuffNotFull = FALSE; // Constant update here prevents use of "else" case below.

      // Copy data to dest buffer, iff entire packet fits !
      if ( (cnt + storeSize ) < nMaxByteSize )
      {
        memcpy ( (void *) (dest_ptr + cnt), (UINT8 *) store_ptr, storeSize );
        cnt += storeSize;
        bDestBuffNotFull = TRUE;
      }

      // Clear param new flag ! Note: bReInit should remain in current state
      //    so that is data received again, will have to call ReInit of data reduction
      //    point !
      runtime_data_ptr->bNewVal = FALSE;

    }

    if ( ( runtime_data_ptr->id == UARTMGR_WORD_ID_NOT_INIT ) ||
         ( bDestBuffNotFull == FALSE ) )
    {
      // Single exit point for two conditions. Easy coverage. JV should appreciate this.
      break;
    }
    else
    {
      pUartData++;
    }

  } // End for loop thru parameters

#ifdef UARTMGR_TIMING_TEST
  /*vcast_dont_instrument_start*/
   m_UARTMGRTiming_End = TTMR_GetHSTickCount();
   m_UARTMGRTiming_Buff[m_UARTMGRTiming_Index] =
                    (m_UARTMGRTiming_End - m_UARTMGRTiming_Start);
   m_UARTMGRTiming_Index = (++m_UARTMGRTiming_Index) % UARTMGR_TIMING_BUFF_MAX;
  /*vcast_dont_instrument_end*/
#endif

  return (cnt);

}


/******************************************************************************
 * Function:     UartMgr_CopyRollBuff
 *
 * Description:  Copy src data to specified m_UartStoreData[] structure.
 *
 * Parameters:   start_dest_ptr - destination buffer
 *               src_ptr  - source pointer
 *               size     - number of bytes to move
 *               *wr_cnt  - ptr to write counter to update
 *               *rd_cnt  - ptr to read counter to update (if NULL, no updates)
 *               wrap_size - size of rolling buffer to perform wrap !
 *
 * Returns:      cnt - number of bytes moved.  If rolling buff wraps,
 *                     the wrap counts are not include.
 *
 * Notes:        none
 *
 *****************************************************************************/
static
UINT16 UartMgr_CopyRollBuff ( UINT8 *start_dest_ptr, UINT8 *src_ptr, UINT16 size,
                              UINT32 *wr_cnt, UINT32 *rd_cnt, UINT16 wrap_size )
{
  UINT16 i;
  UINT16 cnt;
  UINT8 *dest_ptr;

  dest_ptr = start_dest_ptr + (*wr_cnt);
  cnt = 0;

  for (i = 0; i < size; i++)
  {
    *dest_ptr = *src_ptr;
    *wr_cnt += 1;
    cnt++;

    // Test wrap case
    if ( *wr_cnt >= wrap_size )
    {
      // Reset to beginning
      *wr_cnt = 0;
      dest_ptr = start_dest_ptr;
    }
    else
    {
      dest_ptr++;
    }

    // Test overflow case, if selected
    if (*wr_cnt == *rd_cnt)
    {
      *rd_cnt += 1; // Move the read pointer to flush "rolling" buffer
      if ( *rd_cnt >= wrap_size )
      {
        *rd_cnt = 0;
      }
      cnt--;  // Purge the oldest value, don't update count
    }

    src_ptr++;

  } // End for loop

  // If cnt = 0 then, buffer is full and oldest value has been purged.
  //   In this case, cnt will be 0.
  return (cnt);

}



/******************************************************************************
 * Function:     UartMgr_ConvertToEngUnits
 *
 * Description:  Returns the Engine Unit from scaled int value
 *
 * Parameters:   runtime_data_ptr - int val and scale
 *
 * Returns:      FLOAT32 EngUnit
 *
 * Notes:        none
 *
 *****************************************************************************/
static
FLOAT32 UartMgr_ConvertToEngUnits ( UARTMGR_RUNTIME_DATA_PTR runtime_data_ptr )
{
  FLOAT32 fval;
  SINT16 *sval_ptr;

  // Determine if "BNR" or DISC param value
  if ( runtime_data_ptr->scale == (FLOAT32) DR_NO_RANGE_SCALE )
  {
    // DISC Param, just convert from raw to eng unit
    fval = (FLOAT32) runtime_data_ptr->val_raw;
  }
  else
  {
    // "BNR" signed value, convert to SINT16 before converting to FLOAT32 !
    sval_ptr = (SINT16 *) &runtime_data_ptr->val_raw;
    fval = (FLOAT32) (*sval_ptr * runtime_data_ptr->scale_lsb);
  }

  return (fval);
}


/******************************************************************************
 * Function:     UartMgr_Download_NoneHndl
 *
 * Description:  Function to handle UART protocols that don't download.
 *
 * Parameters:   Not Used function FATALS
 *               UINT8 port,
 *               BOOLEAN *bStartDownload,
 *               BOOLEAN *bDownloadCompleted,
 *               BOOLEAN *bWriteInProgress,
 *               BOOLEAN *bWriteOk,
 *               BOOLEAN *bHalt,
 *               BOOLEAN *bNewRec,
 *               UINT32  **pSize,
 *               UINT8   **pData
 *
 * Returns:      none
 *
 * Notes:        none
 *
 *****************************************************************************/
static
void UartMgr_Download_NoneHndl ( UINT8 port,
                                 BOOLEAN *bStartDownload,
                                 BOOLEAN *bDownloadCompleted,
                                 BOOLEAN *bWriteInProgress,
                                 BOOLEAN *bWriteOk,
                                 BOOLEAN *bHalt,
                                 BOOLEAN *bNewRec,
                                 UINT32  **pSize,
                                 UINT8   **pData )
{
   FATAL("UartMgr: Protocol Does Not Have Download Function",NULL);
}


/******************************************************************************
 * Function:     UartMgr_Protocol_ReadyOk_Hndl
 *
 * Description:  Function to handle UART protocols that don't have special channel
 *               ready status.
 *               Ex:  ID Param Protocol, when 1st packet is received, the ch is consider
 *               ACTIVE even though all params in frame are unknown until at least
 *               several sec later.  We don't want to hold off Uart Mgr processing
 *               of received known params by holding off channel ACTIVE until all params
 *               are known.  A 3rd state Channel Ready will be added.  Not all protocols
 *               will require this 3rd state.  For F7X, on first frame, all params are known.
 *
 * Parameters:   UINT16 ch - serial channel cfg for this protocol
 *
 * Returns:      TRUE - always returned true, to indicate Protocol Ch Ready
 *
 * Notes:        none
 *
 *****************************************************************************/
static
BOOLEAN UartMgr_Protocol_ReadyOk_Hndl ( UINT16 ch )
{
   return (TRUE);
}

/******************************************************************************
 * Function:     UartMgr_Download_Clr_NoneHndl
 *
 * Description:  Function to handle UART protocols that don't have "house cleaning"
 *               routine(s) that is exe via state machine transition. 
 *
 * Parameters:   Run - not used
 *               param - not used 
 *
 * Returns:      none
 *
 * Notes:        none
 *
 *****************************************************************************/
static
void UartMgr_Download_Clr_NoneHndl ( BOOLEAN Run, INT32 param )
{
   FATAL("UartMgr: Protocol Does Not Have Download Clr Function",NULL);   
}

/******************************************************************************
 * Function:     UartMgr_Get_ESN_NoneHndl
 *
 * Description:  Function to handle UART protocols that don't have get ESN 
 *               routine(s). 
 *
 * Parameters:   ch - not used
 *               *esn_ptr - not used
 *               cnt - not used
 *
 * Returns:      BOOLEAN not used
 *
 * Notes:        none
 *
 *****************************************************************************/
static
BOOLEAN UartMgr_Get_ESN_NoneHndl ( UINT16 ch, CHAR *esn_ptr, UINT16 cnt )
{
   FATAL("UartMgr: Protocol Does Not Have Get ESN Function",NULL);
   return FALSE;
}

/*****************************************************************************
 *  MODIFICATIONS
 *    $History: UartMgr.c $
 * 
 * *****************  Version 76  *****************
 * User: Peter Lee    Date: 6/06/16    Time: 1:54p
 * Updated in $/software/control processor/code/system
 * SCR #1297 - UARTMGR_RUNTIME_DATA.scale s/b FLOAT32 
 * 
 * *****************  Version 75  *****************
 * User: John Omalley Date: 3/16/16    Time: 11:19a
 * Updated in $/software/control processor/code/system
 * SCR 1302 - Code Review Updates
 * 
 * *****************  Version 74  *****************
 * User: Peter Lee    Date: 2/29/16    Time: 9:34a
 * Updated in $/software/control processor/code/system
 * SCR #1317 Item #5 Add back in uartMgrBlock[i].download_protocol_hndl =
 * GBSProtocol_DownloadHndl;
 * 
 * *****************  Version 73  *****************
 * User: Peter Lee    Date: 2/25/16    Time: 8:38p
 * Updated in $/software/control processor/code/system
 * SCR #1317 Item #5 ESN decode pointing to wrong port causes crash
 * 
 * *****************  Version 72  *****************
 * User: John Omalley Date: 2/25/16    Time: 5:01p
 * Updated in $/software/control processor/code/system
 * SCR 1302 - Design Review Update
 * 
 * *****************  Version 71  *****************
 * User: John Omalley Date: 2/10/16    Time: 9:18a
 * Updated in $/software/control processor/code/system
 * SCR 1302 - Code Review Updates
 * 
 * *****************  Version 70  *****************
 * User: Contractor V&v Date: 2/01/16    Time: 5:24p
 * Updated in $/software/control processor/code/system
 * SCR #1192 - Perf Enhancment improvements sort log list for bin srch
 * 
 * *****************  Version 69  *****************
 * User: John Omalley Date: 1/29/16    Time: 8:54a
 * Updated in $/software/control processor/code/system
 * SCR 1302 - Display Protocol Updates
 * 
 * *****************  Version 68  *****************
 * User: John Omalley Date: 1/21/16    Time: 4:34p
 * Updated in $/software/control processor/code/system
 * SCR 1302 - Code review updates
 * 
 * *****************  Version 67  *****************
 * User: John Omalley Date: 11/19/15   Time: 4:28p
 * Updated in $/software/control processor/code/system
 * SCR 1303 - Updates for the Display Processing App
 * 
 * *****************  Version 66  *****************
 * User: Peter Lee    Date: 15-10-19   Time: 10:29p
 * Updated in $/software/control processor/code/system
 * SCR #1304 ESN support for APAC Processing
 * 
 * *****************  Version 65  *****************
 * User: Jeremy Hester Date: 10/02/15   Time: 9:55a
 * Updated in $/software/control processor/code/system
 * SCR - 1302 Added PWC Display Protocol
 * 
 * *****************  Version 64  *****************
 * User: Peter Lee    Date: 15-04-14   Time: 7:15p
 * Updated in $/software/control processor/code/system
 * SCR #1289 - UartMgr_Protocol_ReadyOk_Hndl() not executed.  Update code
 * in UartMgr_SensorTest() to exe _Hndl() function.
 * 
 * *****************  Version 63  *****************
 * User: John Omalley Date: 4/01/15    Time: 10:34a
 * Updated in $/software/control processor/code/system
 * SCR 1287 - Added asserts for NULL function pointers.
 * 
 * *****************  Version 62  *****************
 * User: John Omalley Date: 4/01/15    Time: 8:51a
 * Updated in $/software/control processor/code/system
 * SCR 1255 Code Review Updates
 * 
 * *****************  Version 61  *****************
 * User: Peter Lee    Date: 15-03-30   Time: 10:23a
 * Updated in $/software/control processor/code/system
 * Code Review Updates
 * 
 * *****************  Version 60  *****************
 * User: Peter Lee    Date: 15-03-11   Time: 12:30p
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol.  Clear NVM cnt thru FSM issues.  
 * a) Uart[0] reserved for GSE
 * b) Can't call NV_Write() multiple times in MIF with same data.  Crashes
 * for some reason.  
 * 
 * *****************  Version 59  *****************
 * User: Peter Lee    Date: 15-03-10   Time: 6:33p
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol.  Fix FSM UartMgr_DownloadClr() to only exe
 * protocol if Download Hndl exists. 
 * 
 * *****************  Version 58  *****************
 * User: Peter Lee    Date: 15-02-06   Time: 7:18p
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol.  Additional Update Item #7. 
 * 
 * *****************  Version 57  *****************
 * User: Peter Lee    Date: 15-01-11   Time: 10:20p
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol 
 * 
 * *****************  Version 56  *****************
 * User: Peter Lee    Date: 14-12-05   Time: 4:39p
 * Updated in $/software/control processor/code/system
 * SCR #1263 ID Param Code Review Mod Fix. 
 *
 * *****************  Version 55  *****************
 * User: John Omalley Date: 11/13/14   Time: 10:38a
 * Updated in $/software/control processor/code/system
 * SCR 1251 - Fixed Data Manager DL Write status location
 *
 * *****************  Version 54  *****************
 * User: Peter Lee    Date: 14-10-27   Time: 9:10p
 * Updated in $/software/control processor/code/system
 * SCR #1263 ID Param Protocol, Design Review Updates.
 *
 * *****************  Version 53  *****************
 * User: Peter Lee    Date: 14-10-08   Time: 6:56p
 * Updated in $/software/control processor/code/system
 * SCR #1263 ID Param Protocol Implementation
 *
 * *****************  Version 52  *****************
 * User: Contractor V&v Date: 9/03/14    Time: 5:27p
 * Updated in $/software/control processor/code/system
 * SCR #1234 - Configuration update of certain modules overwrites default
 * settings/CR updates
 *
 * *****************  Version 51  *****************
 * User: Contractor V&v Date: 8/14/14    Time: 4:14p
 * Updated in $/software/control processor/code/system
 * SCR #1234 - Removed Get/Set Cfg function
 *
 * *****************  Version 50  *****************
 * User: John Omalley Date: 1/27/14    Time: 1:48p
 * Updated in $/software/control processor/code/system
 * SCR 1244 - UartMgr_Download Record unitialized variable, plus code
 * review updates.
 *
 * *****************  Version 49  *****************
 * User: John Omalley Date: 7/23/13    Time: 4:10p
 * Updated in $/software/control processor/code/system
 * SCR 1244 - UartMgr_Download Record unitialized variable, plus code
 * review updates.
 *
 * *****************  Version 48  *****************
 * User: John Omalley Date: 12-12-13   Time: 4:35p
 * Updated in $/software/control processor/code/system
 * SCR 1197 - Code Review Update
 *
 * *****************  Version 47  *****************
 * User: John Omalley Date: 12-11-28   Time: 2:08p
 * Updated in $/software/control processor/code/system
 * SCR 1197- Code Review Updates
 *
 * *****************  Version 46  *****************
 * User: John Omalley Date: 12-11-14   Time: 7:20p
 * Updated in $/software/control processor/code/system
 * SCR 1076 - Code Review Updates
 *
 * *****************  Version 45  *****************
 * User: John Omalley Date: 12-11-13   Time: 5:46p
 * Updated in $/software/control processor/code/system
 * SCR 1197 - Code Review Updates
 *
 * *****************  Version 44  *****************
 * User: John Omalley Date: 12-11-12   Time: 2:58p
 * Updated in $/software/control processor/code/system
 * SCR 1107, 1191 - Code Review Updates
 *
 * *****************  Version 43  *****************
 * User: Peter Lee    Date: 12-10-27   Time: 5:05p
 * Updated in $/software/control processor/code/system
 * SCR #1191 Returns update time of param
 *
 * *****************  Version 42  *****************
 * User: Melanie Jutras Date: 12-10-19   Time: 1:48p
 * Updated in $/software/control processor/code/system
 * SCR #1172 PCLint 545 Suspicious use of & Error
 *
 * *****************  Version 41  *****************
 * User: Melanie Jutras Date: 12-10-10   Time: 1:29p
 * Updated in $/software/control processor/code/system
 * SCR 1172 PCLint 545 Suspicious use of & Error
 *
 * *****************  Version 40  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 39  *****************
 * User: John Omalley Date: 10/10/11   Time: 5:35p
 * Updated in $/software/control processor/code/system
 * SCR 1075 and SCR 1078 - Added missing function descriptions and FATALS
 * per code reviews
 *
 * *****************  Version 38  *****************
 * User: John Omalley Date: 10/05/11   Time: 7:55p
 * Updated in $/software/control processor/code/system
 * SCR 1015 - Sensor Test Logic Update
 *
 * *****************  Version 37  *****************
 * User: John Omalley Date: 9/29/11    Time: 4:07p
 * Updated in $/software/control processor/code/system
 * SCR 1015 - Interface Validity fix
 *
 * *****************  Version 36  *****************
 * User: John Omalley Date: 8/22/11    Time: 11:29a
 * Updated in $/software/control processor/code/system
 * SCR 1015 - Changed Interface valid to FALSE until data is received
 *
 * *****************  Version 35  *****************
 * User: Contractor2  Date: 7/14/11    Time: 11:26a
 * Updated in $/software/control processor/code/system
 * SCR #864 Error: UART Enable without protocol causes an ASSERT
 *
 * *****************  Version 34  *****************
 * User: Contractor2  Date: 6/15/11    Time: 3:10p
 * Updated in $/software/control processor/code/system
 * SCR #864 Error: UART Enable without protocol causes an ASSERT
 *
 * *****************  Version 33  *****************
 * User: Contractor2  Date: 6/14/11    Time: 11:43a
 * Updated in $/software/control processor/code/system
 * SCR #864 Error: UART Enable without protocol causes an ASSERT
 *
 * *****************  Version 32  *****************
 * User: Contractor V&v Date: 5/31/11    Time: 1:52p
 * Updated in $/software/control processor/code/system
 * SCR #1018 Error - Uart Startup TimeOut Value. Code now uses
 * ChannelStartup_s for startup instead of ChannelTimeout_s.
 *
 * *****************  Version 31  *****************
 * User: Peter Lee    Date: 4/11/11    Time: 10:15a
 * Updated in $/software/control processor/code/system
 * SCR #1029 EMU150 Download
 *
 * *****************  Version 30  *****************
 * User: Peter Lee    Date: 11/05/10   Time: 12:06p
 * Updated in $/software/control processor/code/system
 * SCR #982 Remove dead code from _SensorTest()
 *
 * *****************  Version 29  *****************
 * User: Jeff Vahue   Date: 11/01/10   Time: 8:05p
 * Updated in $/software/control processor/code/system
 * SCR# 975 - Code Coverage refactor
 *
 * *****************  Version 28  *****************
 * User: Jeff Vahue   Date: 11/01/10   Time: 7:50p
 * Updated in $/software/control processor/code/system
 * SCR# 975 - Code Coverage refactor
 *
 * *****************  Version 27  *****************
 * User: Peter Lee    Date: 9/28/10    Time: 3:45p
 * Updated in $/software/control processor/code/system
 * SCR #892 Code Review Updates
 *
 * *****************  Version 26  *****************
 * User: John Omalley Date: 9/10/10    Time: 10:01a
 * Updated in $/software/control processor/code/system
 * SCR 858 - Added FATAL to default cases
 *
 * *****************  Version 25  *****************
 * User: Jeff Vahue   Date: 9/03/10    Time: 8:22p
 * Updated in $/software/control processor/code/system
 * SCR# 853 - UART Int Monitor
 *
 * *****************  Version 24  *****************
 * User: Peter Lee    Date: 8/30/10    Time: 11:00a
 * Updated in $/software/control processor/code/system
 * SCR #834 UartMgr - UartMgr_FlushChannels does not check PBIT results
 *
 * *****************  Version 23  *****************
 * User: Peter Lee    Date: 8/26/10    Time: 6:13p
 * Updated in $/software/control processor/code/system
 * SCR #821 Update UartMgr to allow GetSnapShot to be called before Uart
 * Data has been received.
 *
 * *****************  Version 22  *****************
 * User: Peter Lee    Date: 8/26/10    Time: 5:13p
 * Updated in $/software/control processor/code/system
 * SCR #827 UartMgr_ReadWord() incorrect interpret value as signed
 * magnitude.  s/b 16 bit 2-s complement.
 *
 * *****************  Version 21  *****************
 * User: Peter Lee    Date: 8/18/10    Time: 4:18p
 * Updated in $/software/control processor/code/system
 * SCR #792 Set Sys Status to Normal for UART INT Failure.
 *
 * *****************  Version 20  *****************
 * User: John Omalley Date: 8/10/10    Time: 5:26p
 * Updated in $/software/control processor/code/system
 * SCR 783 - Fixed the Binary System Header for UARTs
 *
 * *****************  Version 19  *****************
 * User: Contractor V&v Date: 7/30/10    Time: 9:39p
 * Updated in $/software/control processor/code/system
 * SCR #282 Misc - Shutdown Processing
 *
 * *****************  Version 18  *****************
 * User: Peter Lee    Date: 7/29/10    Time: 6:51p
 * Updated in $/software/control processor/code/system
 * SCR #754 Flush Uart Interface before normal op
 *
 * *****************  Version 17  *****************
 * User: Peter Lee    Date: 7/28/10    Time: 9:28a
 * Updated in $/software/control processor/code/system
 * SCR #729 Another path, Uart Data Rec time delta / count s/b in 10 msec
 * Tick time and not msec !
 *
 * *****************  Version 16  *****************
 * User: Peter Lee    Date: 7/26/10    Time: 6:20p
 * Updated in $/software/control processor/code/system
 * SCR #743 Update TimeOut Processing for val == 0
 *
 * *****************  Version 15  *****************
 * User: Peter Lee    Date: 7/22/10    Time: 6:15p
 * Updated in $/software/control processor/code/system
 * SCR #729 Clear Sys Cond after CBIT goes away.  Update Delta time in
 * Data Pkt to be tick time (10 msec) from msec.
 *
 * *****************  Version 14  *****************
 * User: Peter Lee    Date: 7/21/10    Time: 6:02p
 * Updated in $/software/control processor/code/system
 * SCR #722 Path where UartMgrBlock[i].get_protocol-fileHdr is
 * un-initialized.
 *
 * *****************  Version 13  *****************
 * User: Peter Lee    Date: 7/20/10    Time: 7:24p
 * Updated in $/software/control processor/code/system
 * SCR #719 Update code to copy Uart Drv BIT cnt to Uart Mgr Status
 *
 * *****************  Version 12  *****************
 * User: John Omalley Date: 7/20/10    Time: 9:05a
 * Updated in $/software/control processor/code/system
 * SCR 294 - Add system binary header
 *
 * *****************  Version 11  *****************
 * User: Contractor2  Date: 7/09/10    Time: 4:32p
 * Updated in $/software/control processor/code/system
 * SCR #8 Implementation: External Interrupt Monitors
 *
 * *****************  Version 10  *****************
 * User: Jeff Vahue   Date: 6/27/10    Time: 5:30p
 * Updated in $/software/control processor/code/system
 * SCR# 662 - bad sizeof specified, use variable vs. type
 *
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 6/25/10    Time: 6:14p
 * Updated in $/software/control processor/code/system
 * SCR #635
 * 1) Add UartMgr Debug Display Raw input
 * 2) Change Param not init from "0" to "255"
 * 3) Updated UartMgr name to 1,2,3
 * 4) Removed (cnt >0) to always call protocol processing (fixes
 * compatibility with WIN32)
 * 5) Fix UartMgr_CoopyRollBuff() bug.
 *
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 6/21/10    Time: 3:35p
 * Updated in $/software/control processor/code/system
 * SCR #635 Item 19.  Fix bug where on invalid to valid transition, Data
 * Red Init() is called.
 *
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 6/14/10    Time: 6:29p
 * Updated in $/software/control processor/code/system
 * SCR #635: MIsc Uart Updates. Uart Port duplex update.
 *
 * *****************  Version 6  *****************
 * User: Jim Mood     Date: 6/11/10    Time: 3:30p
 * Updated in $/software/control processor/code/system
 * SCR 623 Batch configuration implementation updates
 *
 * *****************  Version 5  *****************
 * User: Peter Lee    Date: 6/10/10    Time: 2:13p
 * Updated in $/software/control processor/code/system
 * SCR #365 Fix Uart_GetFileHdr() return (cnt) and use of pDest
 *
 * *****************  Version 4  *****************
 * User: Peter Lee    Date: 6/10/10    Time: 2:05p
 * Updated in $/software/control processor/code/system
 * SCR #635 Add UART Startup PBIT log
 *
 * *****************  Version 3  *****************
 * User: Jeff Vahue   Date: 6/07/10    Time: 4:08p
 * Updated in $/software/control processor/code/system
 * SCR# 632 - fill in the chan Id for UART timeout logs
 *
 * *****************  Version 2  *****************
 * User: Peter Lee    Date: 5/31/10    Time: 8:12p
 * Updated in $/software/control processor/code/system
 * SCR #618 SW-SW Integration problems. Task Mgr updated to allow only one
 * Task Id creation.
 *
 * *****************  Version 1  *****************
 * User: Peter Lee    Date: 5/31/10    Time: 5:55p
 * Created in $/software/control processor/code/system
 * Initial Check In.  Implementation of the F7X and UartMgr Requirements.
 *
 *
 *
 *****************************************************************************/
