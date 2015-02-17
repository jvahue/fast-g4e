#define ARINC429MGR_BODY
/******************************************************************************
            Copyright (C) 2009-2015 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    ECCN:         9D991

    File:         Arinc429Mgr.c

    Description:  Contains all functions and data related to Managing the
                  data received on ARINC429.

VERSION
     $Revision: 64 $  $Date: 2/17/15 3:55p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include <stdio.h>
/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "arinc429mgr.h"
#include "CfgManager.h"
#include "gse.h"
#include "user.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define ARINC429_MAX_DISPLAY_LINES    8
/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static ARINC429_CFG            m_Arinc429Cfg;
static ARINC429_CHAN_DATA      m_Arinc429RxChannel    [FPGA_MAX_RX_CHAN];

static ARINC429_SENSOR_LABEL_FILTER
                               m_ArincSensorFilterData[FPGA_MAX_RX_CHAN][ARINC429_MAX_LABELS];
static ARINC429_MGR_TASK_PARAMS m_Arinc429MgrBlock;
static ARINC429_RAW_RX_BUFFER  m_Arinc429RxBuffer     [FPGA_MAX_RX_CHAN];
static ARINC429_RECORD         m_Arinc429RxStorage[FPGA_MAX_RX_CHAN][ARINC429_MAX_RECORDS];
static ARINC429_RAW_RX_BUFFER  m_Arinc429DebugBuffer  [FPGA_MAX_RX_CHAN];
static UINT32                  m_Arinc429DebugStorage[FPGA_MAX_RX_CHAN][ARINC429_MAX_RECORDS];
static ARINC429_SENSOR_INFO    m_Arinc429SensorInfo   [ARINC429_MAX_WORDS];
static UINT16                  m_Arinc429WordSensorCount;
static ARINC429_DEBUG          m_Arinc429_Debug;
static ARINC429_CBIT_HEALTH_COUNTS  m_Arinc429CBITHealthStatus;
static UINT32                  RawDataArinc[FPGA_MAX_RX_CHAN][ARINC429_MAX_RECORDS];
static ARINC429_RECORD         RawRecords  [FPGA_MAX_RX_CHAN][ARINC429_MAX_RECORDS];
static UINT32                  RawDataCnt  [FPGA_MAX_RX_CHAN];
// Invalid BCD digit log flags. One flag for each ARINC429 label.
// Set TRUE and logged only once per label per power cycle.
static BOOLEAN                 m_Arinc429BCDInvDigit [ARINC429_MAX_LABELS][ARINC_CHAN_MAX];

// Note: Failure Condition Array ordering is directly related to the
//         bit ordering in Arinc429Filter->ValidSSM. Which is
//         directly defined in ARINC_FAIL_COUNT enum.
static const UINT8 FailureCondition[END_OF_FAIL_COUNT] =
{
  ARINC_BNR_SSM_FAILURE_v,   // 0x00 MSB - SSM BNR System Failure value
  ARINC_SSM_NOCOMPDATA_v,    // 0x20 MSB - SSM BCD No Computed Data value
  ARINC_SSM_FUNCTEST_v,      // 0x40 MSB - SSM Functional Test Data value
  ARINC_BNR_SSM_NORM_v       // 0x60 MSB - SSM BNR Normal Op Data value
};

static CHAR GSE_OutLine[512];

/*****************************************************************************/
/* Include the cmd table & functions                                         */
/*****************************************************************************/
#include "Arinc429UserTables.c"


/******************************************/
/* Arinc429 Constants                     */
/******************************************/

// Note: Failure Condition Array ordering is directly related to the
//         bit ordering in Arinc429Filter->ValidSSM. Which is
//         directly defined in ARINC_FAIL_COUNT enum.
const CHAR *Arinc429_ErrMsg[END_OF_FAIL_COUNT+1] =
{
  // [0] SSM_FAIL
  "Xcptn: SSM Fail 00",
  // [1] SSM_NO_COMPUTE
  "Xcptn: SSM No Comp",
  // [2] SSM_TEST_DATA
  "Xcptn: SSM Test",
  // [3] SSM_NORMAL
  "Xcptn: SSM Fail 11",
  // [4] DATA LOSS  *This must be the last log*
  "Xcptn: Data Loss"
};

#ifdef GENERATE_SYS_LOGS
  // Arinc429 DRV PBIT FPGA "BAD STATE" Log
  static ARINC_DRV_PBIT_FPGA_FAIL Arinc429DrvPbitLogFpga[] = {
    {DRV_A429_FPGA_BAD_STATE}
  };

  // Arinc429 DRV PBIT LFR Failed Log
  static ARINC_DRV_PBIT_TEST_RESULT Arinc429DrvPbitLogLFR[] = {
    { DRV_A429_RX_LFR_TEST_FAIL,
      {FALSE, TRUE, FALSE, TRUE},
      {TRUE, TRUE, TRUE, TRUE,
       TRUE, TRUE, TRUE, TRUE,
       TRUE, TRUE, TRUE, TRUE},
      {TRUE, TRUE, TRUE,
       TRUE, TRUE, TRUE},
      {TRUE, TRUE},
      {TRUE, TRUE, TRUE, TRUE}}
  };

  // Arinc429 DRV PBIT RX FIFO Failed Log
  static ARINC_DRV_PBIT_TEST_RESULT Arinc429DrvPbitLogRxFIFO[] = {
    { DRV_A429_RX_FIFO_TEST_FAIL,
      {TRUE, TRUE, TRUE, TRUE},
      {TRUE, FALSE, TRUE, FALSE,
       FALSE, TRUE, FALSE, TRUE,
       TRUE, TRUE, FALSE, FALSE},
      {TRUE, TRUE, TRUE,
       TRUE, TRUE, TRUE},
      {TRUE, TRUE},
      {TRUE, TRUE, TRUE, TRUE}}
  };

  // Arinc429 DRV PBIT TX FIFO Failed Log
  static ARINC_DRV_PBIT_TEST_RESULT Arinc429DrvPbitLogTxFIFO[] = {
    { DRV_A429_TX_FIFO_TEST_FAIL,
      {TRUE, TRUE, TRUE, TRUE},
      {TRUE, TRUE, TRUE, TRUE,
       TRUE, TRUE, TRUE, TRUE,
       TRUE, TRUE, TRUE, TRUE},
      {FALSE, TRUE, FALSE,
       FALSE, FALSE, TRUE},
      {TRUE, TRUE},
      {TRUE, TRUE, TRUE, TRUE}}
  };

  // Arinc429 DRV PBIT TX-RX LoopBack Failed Log
  static ARINC_DRV_PBIT_TEST_RESULT Arinc429DrvPbitLogTxRxLoop[] = {
    { DRV_A429_TX_FIFO_TEST_FAIL,
      {TRUE, TRUE, TRUE, TRUE},
      {TRUE, TRUE, TRUE, TRUE,
       TRUE, TRUE, TRUE, TRUE,
       TRUE, TRUE, TRUE, TRUE},
      {TRUE, TRUE, TRUE,
       TRUE, TRUE, TRUE},
      {TRUE, FALSE},
      {TRUE, TRUE, FALSE, FALSE}}
  };

  // Arinc429 SYS PBIT Failed Log (Arinc429 DRV had failed to load successfully)
  static ARINC429_SYS_PBIT_STARTUP_LOG Arinc429SysPbitLog[] = {
    {SYS_A429_PBIT_FAIL, 2}
  };

  // Arinc429 SYS CBIT Failed Startup Log
  static ARINC429_SYS_TIMEOUT_LOG Arinc429SysCbitStartupTimeoutLog[] = {
    { SYS_A429_STARTUP_TIMEOUT, 0x1234, 2 }
  };

  // Arinc429 SYS CBIT Failed Data Loss Log
  static ARINC429_SYS_TIMEOUT_LOG Arinc429SysCbitDataTimeoutLog[] = {
    { SYS_A429_DATA_LOSS_TIMEOUT, 0x5678, 3 }
  };

  // Arinc429 SYS CBIT SSM Failed Log
  static ARINC429_SYS_CBIT_SSM_FAIL_LOG Arinc429SysCbitSSMFailLog[] = {
    { SYS_A429_SSM_FAIL, "Xcptn: SSM Fail XX" }
  };

#endif

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
// Initialization
static void   Arinc429MgrReconfigureHW                ( void );

static void   Arinc429MgrCfgReduction                 ( ARINC429_RX_CFG_PTR pRxChanCfg,
                                                        ARINC429_CHAN_DATA_PTR pChanData );
// Common
static UINT16 Arinc429MgrReadBuffer                   ( void *pDest, UINT16 nMaxByteSize,
                                                        ARINC429_RAW_RX_BUFFER_PTR pBuf );

static void   Arinc429MgrParseGPA_GPB                 ( UINT32 GPA, UINT32 GPB,
                                                        ARINC429_WORD_INFO_PTR pWordInfo );

static SINT32 Arinc429MgrParseMsg                     ( UINT32 Arinc429Msg,
                                                        ARINC_FORM Format,
                                                        UINT8  Position,
                                                        UINT8  Size,
                                                        UINT32 Chan,
                                                        BOOLEAN *pWordValid );

static SINT32 Arinc429MgrParseBCD                     ( UINT32 raw,  UINT32  Chan,
                                                        UINT8  Size, BOOLEAN *pWordValid );
// Debug Display
void Arinc429MgrDispSingleArincChan                   ( void );

void Arinc429MgrDisplayMultiArincChan                 ( void );

void Arinc429MgrDisplayFmtedLine                      ( BOOLEAN isFormatted, UINT32 ArincMsg );

// Message Processing
static BOOLEAN Arinc429MgrApplyProtocol               ( ARINC429_RX_CFG_PTR pCfg,
                                                        ARINC429_CHAN_DATA_PTR pChanData,
                                                        ARINC429_LABEL_DATA_PTR pLabelData,
                                                        ARINC_SDI sdi,
                                                        ARINC429_RECORD_PTR pDataRecord );

static BOOLEAN Arinc429MgrDataReduction               ( ARINC429_RX_CFG_PTR     pCfg,
                                                        ARINC429_CHAN_DATA_PTR  pChanData,
                                                        ARINC429_LABEL_DATA_PTR pSdiData,
                                                        ARINC_SDI               sdi,
                                                        ARINC429_RECORD_PTR     pDataRecord );

static BOOLEAN Arinc429MgrProcPW305ABEngineMFD       ( MFD_DATA_TABLE_PTR   pMFDTable,
                                                       UINT32               *pArincMsg,
                                                       UINT8                Label );

static void    Arinc429MgrChkPW305AB_MFDTimeout        ( ARINC429_CHAN_DATA_PTR pChanData,
                                                        ARINC429_RAW_RX_BUFFER_PTR pRxBuffer );

static ARINC429_LABEL_DATA_PTR Arinc429MgrStoreLabelData ( UINT32 Arinc429Msg,
                                                           ARINC429_CHAN_DATA_PTR  pChanData,
                                                           ARINC_SDI *psdi,
                                                           UINT32 TickCount,
                                                           TIMESTAMP TimeStamp );

static void   Arinc429MgrWriteBuffer                   ( ARINC429_RAW_RX_BUFFER_PTR pBuffer,
                                                         const void *Data, UINT16 Size );

static BOOLEAN Arinc429MgrParameterTest                ( ARINC429_SDI_DATA_PTR pSdiData,
                                                         ARINC429_WORD_INFO_PTR pWordInfo );

static BOOLEAN Arinc429MgrInvalidateParameter          ( ARINC429_SDI_DATA_PTR pSdiData,
                                                         ARINC429_WORD_INFO_PTR pWordInfo,
                                                         ARINC429_RECORD_PTR pDataRecord );

static void    Arinc429MgrChkParamLostTimeout      ( ARINC429_RX_CFG_PTR pCfg,
                                                     ARINC429_CHAN_DATA_PTR pChanData,
                                                     ARINC429_RAW_RX_BUFFER_PTR pRxBuffer );
static UINT32  Arinc429MgrConvertEngToMsg          ( FLOAT32 EngVal, FLOAT32 Scale,
                                                     UINT8 WordSize, UINT8 Position,
                                                     UINT32 Format,  UINT32 Arinc429Msg,
                                                     BOOLEAN bValid );
// Sensor
static void   Arinc429MgrUpdateLabelFilter         ( ARINC429_SENSOR_INFO_PTR pSenorInfo );

// Built-In-Test
static void   Arinc429MgrCreateTimeOutSysLog           ( RESULT resultType, UINT8 ch );

static void   Arinc429MgrDetermineSSMFailure           ( UINT32 *pFailCount, UINT8 TotalTests,
                                                         char *pMsg );
// Data Manager
static UINT16 Arinc429MgrReadReduceSnapshot            ( ARINC429_SNAPSHOT_RECORD_PTR pBuff,
                                                         UINT16 nMaxRecords,
                                                         ARINC429_CHAN_DATA_PTR pChanData,
                                                         ARINC429_RX_CFG_PTR pCfg,
                                                         BOOLEAN bStartSnap );
static UINT16 Arinc429MgrGetReduceHdr                  ( INT8 *pDest,
                                                         UINT32 chan,
                                                         UINT16 nMaxByteSize,
                                                         UINT16 *pNumParams );

// Shutdown
void Arinc429MgrProcMsgsTaskShutdown(void);


/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* INITIALIZATION FUNCTIONS                                                  */
/*****************************************************************************/
/******************************************************************************
 * Function:    Arinc429MgrInitTasks
 *
 * Description: Arinc429 Mgr Initialization Tasks
 *               - Launch Arinc429 System Tasks
 *               - Restore User Arinc429 Cfg
 *               - For Arinc429 Mgr PBIT, check Arinc429 Drv PBIT status
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:
 *  1) Must be called after User_Init() (user manager initialization)
 *     since Arinc429 user commands are "registered" in this routine
 *  2) Expects Arinc429_Initialize() to be called prior to exe of this func
 *     expects Arinc429 PBIT to have executed.
 *  3) If Arinc429 Drv PBIT fails, Arinc429 Mgr PBIT shall be set to fail and
 *     Arinc429 processing disabled.
 *
 *****************************************************************************/
void Arinc429MgrInitTasks (void)
{
   // Local Data
   UINT16                        nChannel;
   ARINC429_SYS_PBIT_STARTUP_LOG m_StartupLog;
   TCB                           tcbTaskInfo;


   Arinc429MgrInitialize ( );

   //Add an entry in the user message handler table
   User_AddRootCmd(&arinc429RootTblPtr);

   // Restore User Default
   // memcpy(&m_Arinc429Cfg, &( CfgMgr_ConfigPtr()->Arinc429Config ), sizeof( m_Arinc429Cfg ));
   //   moved to Arinc429MgrInitialize()
   memset(m_Arinc429BCDInvDigit, FALSE, sizeof(m_Arinc429BCDInvDigit));

   // Create Arinc429 Read FIFO Task
   memset(&tcbTaskInfo, 0, sizeof(tcbTaskInfo));
   strncpy_safe(tcbTaskInfo.Name,sizeof(tcbTaskInfo.Name),"Arinc429 Process Msgs",_TRUNCATE);
   tcbTaskInfo.TaskID         = (TASK_INDEX)Arinc429_Process_Msgs;
   tcbTaskInfo.Function       = Arinc429MgrProcessMsgsTask;
   tcbTaskInfo.Priority       = taskInfo[Arinc429_Process_Msgs].priority;
   tcbTaskInfo.Type           = taskInfo[Arinc429_Process_Msgs].taskType;
   tcbTaskInfo.modes          = taskInfo[Arinc429_Process_Msgs].modes;
   tcbTaskInfo.MIFrames       = taskInfo[Arinc429_Process_Msgs].MIFframes;
   tcbTaskInfo.Rmt.InitialMif = taskInfo[Arinc429_Process_Msgs].InitialMif;
   tcbTaskInfo.Rmt.MifRate    = taskInfo[Arinc429_Process_Msgs].MIFrate;
   tcbTaskInfo.Enabled        = TRUE;
   tcbTaskInfo.Locked         = FALSE;
   tcbTaskInfo.pParamBlock    = &m_Arinc429MgrBlock;
   TmTaskCreate (&tcbTaskInfo);

   // Create Arinc429 BIT Task
   memset(&tcbTaskInfo, 0, sizeof(tcbTaskInfo));
   strncpy_safe(tcbTaskInfo.Name,sizeof(tcbTaskInfo.Name),"Arinc429 BIT",_TRUNCATE);
   tcbTaskInfo.TaskID         = (TASK_INDEX)Arinc429_BIT;
   tcbTaskInfo.Function       = Arinc429MgrBITTask;
   tcbTaskInfo.Priority       = taskInfo[Arinc429_BIT].priority;
   tcbTaskInfo.Type           = taskInfo[Arinc429_BIT].taskType;
   tcbTaskInfo.modes          = taskInfo[Arinc429_BIT].modes;
   tcbTaskInfo.MIFrames       = taskInfo[Arinc429_BIT].MIFframes;
   tcbTaskInfo.Rmt.InitialMif = taskInfo[Arinc429_BIT].InitialMif;
   tcbTaskInfo.Rmt.MifRate    = taskInfo[Arinc429_BIT].MIFrate;
   tcbTaskInfo.Enabled        = TRUE;
   tcbTaskInfo.Locked         = FALSE;
   tcbTaskInfo.pParamBlock    = &m_Arinc429MgrBlock;
   TmTaskCreate (&tcbTaskInfo);

   // Create Arinc429 Display SW Buffer Task
   memset(&tcbTaskInfo, 0, sizeof(tcbTaskInfo));
   strncpy_safe(tcbTaskInfo.Name,sizeof(tcbTaskInfo.Name),"Arinc429 Disp SW Buff",_TRUNCATE);
   tcbTaskInfo.Function       = Arinc429MgrDisplaySWBufferTask;
   tcbTaskInfo.TaskID         = (TASK_INDEX)Arinc429_Disp_SW_Buff;
   tcbTaskInfo.Priority       = taskInfo[Arinc429_Disp_SW_Buff].priority;
   tcbTaskInfo.Type           = taskInfo[Arinc429_Disp_SW_Buff].taskType;
   tcbTaskInfo.modes          = taskInfo[Arinc429_Disp_SW_Buff].modes;
   tcbTaskInfo.MIFrames       = taskInfo[Arinc429_Disp_SW_Buff].MIFframes;
   tcbTaskInfo.Rmt.InitialMif = taskInfo[Arinc429_Disp_SW_Buff].InitialMif;
   tcbTaskInfo.Rmt.MifRate    = taskInfo[Arinc429_Disp_SW_Buff].MIFrate;
   tcbTaskInfo.Enabled        = TRUE;
   tcbTaskInfo.Locked         = FALSE;
   tcbTaskInfo.pParamBlock    = &m_Arinc429MgrBlock;
   TmTaskCreate (&tcbTaskInfo);

   // Always reconfigure the ARINC429 Hardware with current User Arinc Settings
   Arinc429MgrReconfigureHW();
   Arinc429DrvReconfigure();

   // Update the status of each Rx Ch at this point
   // Loop thru and set the proper DISABLE state of each Rx Ch where Enable == FALSE
   for ( nChannel = 0; nChannel < FPGA_MAX_RX_CHAN; nChannel++)
   {
      // Update DISABLE state for Rx Ch where Enable == FALSE
      if (m_Arinc429Cfg.RxChan[nChannel].Enable == FALSE )
      {
         // Note: ->Status updated to ARINC429_STATUS_OK or 
         //       ARINC429_STATUS_FAULTED_PBIT by Arinc429MgrInitialize() PBIT
         if (m_Arinc429MgrBlock.Rx[nChannel].Status != ARINC429_STATUS_OK)
         {
             m_Arinc429MgrBlock.Rx[nChannel].Status = ARINC429_STATUS_DISABLED_FAULTED;
         }
         else
         {
            m_Arinc429MgrBlock.Rx[nChannel].Status = ARINC429_STATUS_DISABLED_OK;
         }
      } // End of if Rx Ch is disabled.
      // Rx Ch request to be enabled, determine Status after PBIT
      //   If PBIT failed, the Rx Ch shall be disabled until Power Cycle or
      //   arinc429.reconfigure() called.
      else
      {
         // Note: ->Status updated to ARINC429_STATUS_OK or 
         //       ARINC429_STATUS_FAULTED_PBIT by Arinc429MgrInitialize() PBIT
         if (m_Arinc429MgrBlock.Rx[nChannel].Status != ARINC429_STATUS_OK)
         {
            // Arinc429 DRV PBIT failed.  Disable this Ch.
            m_Arinc429MgrBlock.Rx[nChannel].Enabled = FALSE;

            // Create PBIT Arinc429 Mgr Startup PBIT Log
            m_StartupLog.result = SYS_A429_PBIT_FAIL;
            m_StartupLog.ch = nChannel;
            Flt_SetStatus(m_Arinc429Cfg.RxChan[nChannel].PBITSysCond, 
                          SYS_ID_A429_PBIT_DRV_FAIL,
                          &m_StartupLog, sizeof(ARINC429_SYS_PBIT_STARTUP_LOG));
         }
      }
   } // End of for Channel loop

   // Update the status of each Tx Ch at this point
   // Loop thru and set the proper DISABLE state of each Rx Ch where Enable == FALSE
   for ( nChannel = 0; nChannel < FPGA_MAX_TX_CHAN; nChannel++)
   {
      // Update DISABLE state for Tx Ch where Enable == FALSE
      if (m_Arinc429Cfg.TxChan[nChannel].Enable == FALSE )
      {
         // Note: ->Status updated to ARINC429_STATUS_OK or 
         //       ARINC429_STATUS_FAULTED_PBIT by Arinc429MgrInitialize() PBIT
         if (m_Arinc429MgrBlock.Tx[nChannel].Status != ARINC429_STATUS_OK)
         {
            m_Arinc429MgrBlock.Tx[nChannel].Status = ARINC429_STATUS_DISABLED_FAULTED;
         }
         else
         {
            m_Arinc429MgrBlock.Tx[nChannel].Status = ARINC429_STATUS_DISABLED_OK;
         }
      } // End of if Tx Ch is disabled.
      // Tx Ch request to be enabled, determine Status after PBIT
      //   If PBIT failed, the Tx Ch shall be disabled until Power Cycle or
      //   arinc429.reconfigure() called.
      else
      {
         // Note: ->Status updated to ARINC429_STATUS_OK or 
         //       ARINC429_STATUS_FAULTED_PBIT by Arinc429MgrInitialize() PBIT
         if (m_Arinc429MgrBlock.Tx[nChannel].Status != ARINC429_STATUS_OK)
         {
            // Create PBIT Arinc429 Mgr Startup PBIT Log
            m_StartupLog.result = SYS_A429_PBIT_FAIL;
            m_StartupLog.ch = nChannel;
            LogWriteSystem(SYS_ID_A429_PBIT_DRV_FAIL, LOG_PRIORITY_LOW, &m_StartupLog,
                           sizeof(ARINC429_SYS_PBIT_STARTUP_LOG), NULL);
         }
      }
   } // End of for (i) loop
}

/******************************************************************************
 * Function:    Arinc429MgrInitialize
 *
 * Description: 1) Initializes internal local variable
 *              2) Checks the Driver Status
 *              3) Initializes the receive Buffers
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:
 *  1) FPGA Driver must be initialized before call to Arinc429_Initialize()
 *  2) Note, no Result Data returned when !DRV_OK, only DRV_RESULT code returned
 *
 *****************************************************************************/
void Arinc429MgrInitialize ( void )
{
   // Local Data
   UINT8                       nChannel;
   ARINC429_RAW_RX_BUFFER_PTR  pRxBuffer;
   ARINC429_RAW_RX_BUFFER_PTR  pDebugBuffer;

   // Initialize Local Data
   // Clear all Storage Containers
   memset ( (void *) m_Arinc429RxChannel,         0, sizeof( m_Arinc429RxChannel        ) );
   memset ( (void *) m_ArincSensorFilterData,     0, sizeof( m_ArincSensorFilterData    ) );
   memset ( (void *) &m_Arinc429Cfg,              0, sizeof( m_Arinc429Cfg              ) );
   memset ( (void *) &m_Arinc429MgrBlock,         0, sizeof( m_Arinc429MgrBlock         ) );
   memset ( (void *) m_Arinc429RxBuffer,          0, sizeof( m_Arinc429RxBuffer         ) );
   memset ( (void *) m_Arinc429DebugBuffer,       0, sizeof( m_Arinc429DebugBuffer      ) );
   memset ( (void *) m_Arinc429SensorInfo,        0, sizeof( m_Arinc429SensorInfo       ) );
   memset ( (void *) &m_Arinc429_Debug,           0, sizeof( m_Arinc429_Debug           ) );
   memset ( (void *) &m_Arinc429CBITHealthStatus, 0, sizeof( m_Arinc429CBITHealthStatus ) );

   m_Arinc429_Debug.bOutputRawFilteredBuff       = FALSE;
   m_Arinc429_Debug.bOutputRawFilteredFormatted  = TRUE;
   m_Arinc429_Debug.bOutputMultipleArincChan     = TRUE;
   m_Arinc429_Debug.OutputType                   = DEBUG_OUT_RAW;

   m_Arinc429WordSensorCount = 0;

   // Restore User Default
   memcpy(&m_Arinc429Cfg, &( CfgMgr_ConfigPtr()->Arinc429Config ), sizeof( m_Arinc429Cfg ));

   // Loop through all the Receive Channels and set the Channel's Status to the
   // driver status for that channel
   for (nChannel = 0; nChannel < FPGA_MAX_RX_CHAN; nChannel++)
   {
      pRxBuffer    = &m_Arinc429RxBuffer[nChannel];
      pDebugBuffer = &m_Arinc429DebugBuffer[nChannel];

      m_Arinc429RxBuffer[nChannel].RecordSize    = sizeof(ARINC429_RECORD);
      m_Arinc429DebugBuffer[nChannel].RecordSize = sizeof(UINT32);

      FIFO_Init( &pRxBuffer->RecordFIFO,    (INT8*)m_Arinc429RxStorage[nChannel],
                 sizeof(m_Arinc429RxStorage[0]) );
      FIFO_Init( &pDebugBuffer->RecordFIFO, (INT8*)m_Arinc429DebugStorage[nChannel],
                 sizeof(m_Arinc429DebugStorage[0]) );

      if ( ( TRUE == Arinc429DrvStatus_OK ( ARINC429_RCV, nChannel ) ) ||
           ( m_Arinc429Cfg.bIgnoreDrvPBIT == TRUE ) )
      {
         m_Arinc429MgrBlock.Rx[nChannel].Status = ARINC429_STATUS_OK;
      }
      else
      {
         m_Arinc429MgrBlock.Rx[nChannel].Status = ARINC429_STATUS_FAULTED_PBIT;
      }
   }

   // Loop through all the Transmit Channels and set the Channel's Status to the
   // driver status for that channel
   for (nChannel = 0; nChannel < FPGA_MAX_TX_CHAN; nChannel++)
   {
      if ( ( TRUE == Arinc429DrvStatus_OK ( ARINC429_XMIT, nChannel ) ) ||
           ( m_Arinc429Cfg.bIgnoreDrvPBIT == TRUE ) )
      {
         m_Arinc429MgrBlock.Tx[nChannel].Status = ARINC429_STATUS_OK;
      }
      else
      {
         m_Arinc429MgrBlock.Tx[nChannel].Status = ARINC429_STATUS_FAULTED_PBIT;
      }
   }
}

/*****************************************************************************/
/* ARINC429 TASKS                                                            */
/*****************************************************************************/

/******************************************************************************
 * Function:    Arinc429MgrProcessMsgsTask
 *
 * Description: Reads the HW Arinc Rx FIFO for any Rx data and stores it into
 *              LabelData table (based on individual labels). Writes all
 *              received labels to the debug buffer, applies the configured
 *              protocol and writes records to the Receive buffer.
 *              This is a Polled mechanism and not INT driven. This task must
 *              execute at a rate fast enough to avoid HW Rx FIFO Overflow.
 *
 * Parameters:  pParam - ptr to m_Arinc429MgrBlock
 *
 * Returns:     none
 *
 * Notes:
 *   03/12/2008 PLee - Determine worst case processing of all 4 ch with
 *                     FIFO half or full ! Should we do only 1 rx per MIF ?
 *   1) Read the HW FIFO is a polled task (normally at 10 msec), rather than
 *      INT driven.  FPGA HW provides the following INT: Not Empty, Half Full
 *      and Full.  Setting Not Empty could generate too many INT (esp if
 *      set for 4 ch of HiSpeed).  Half and Full INT would still require a
 *      polled routine to "flush" the Rx Data out of the FIFO.
 *
 *****************************************************************************/
void Arinc429MgrProcessMsgsTask(void *pParam)
{
   // Local Data
   UINT32                     nArinc429Msg;
   ARINC429_RECORD            m_ARINC429Record;
   UINT32                     nChannel;
   UINT8                      nLoopLimit;
   ARINC429_RX_CFG_PTR        pRxCfgChan;
   ARINC429_CHAN_DATA_PTR     pRxChan;
   ARINC429_SYS_RX_STATUS_PTR pRxStatusChan;
   ARINC429_LABEL_DATA_PTR    pLabelData;
   ARINC_SDI                  sdi;
   UINT32                     nTickCount;
   TIMESTAMP                  timeStamp;

   // If shutdown is signaled,
   // perform shutdown activities, then
   // tell Task Mgr to disable my task.
   if (Tm.systemMode == SYS_SHUTDOWN_ID)
   {
     Arinc429MgrProcMsgsTaskShutdown();
     TmTaskEnable((TASK_INDEX)Arinc429_Process_Msgs, FALSE);
   }
   else // Normal task execution.
   {

     // Current MIF Frame tick count. Don't require finer resolution.
     nTickCount = CM_GetTickCount();
     CM_GetTimeAsTimestamp (&timeStamp);

     // Loop thru all ARINC429 Receive Channels
     for ( nChannel = 0; nChannel < (UINT32)ARINC_CHAN_MAX; nChannel++)
     {
       pRxChan = &m_Arinc429RxChannel[nChannel];

       // Check that the Channel is enabled and the Driver is OK
       if ( ( TRUE == m_Arinc429MgrBlock.Rx[nChannel].Enabled ) &&
              ( Arinc429DrvStatus_OK(ARINC429_RCV, nChannel) ||
                (m_Arinc429Cfg.bIgnoreDrvPBIT == TRUE) ) )
       {
         // Record the time sync delta for this channel
         pRxChan->TimeSyncDelta = (UINT8)((nTickCount - pRxChan->SyncStart)/10);
         pRxCfgChan             = &m_Arinc429Cfg.RxChan[nChannel];

         // Loop until the ARINC429 RX FIFO is empty or the limit is reached
         for ( nLoopLimit = 0;
               Arinc429DrvRead( &nArinc429Msg, (ARINC429_CHAN_ENUM)nChannel ) &&
                                (nLoopLimit < ARINC429_MAX_READS);
               nLoopLimit++ )
         {
           pRxStatusChan  = &m_Arinc429MgrBlock.Rx[nChannel];
           // Code optimization: Save 5 microseconds per read by not storing
           if ( ( TRUE          == m_Arinc429_Debug.bOutputRawFilteredBuff) &&
                ( DEBUG_OUT_RAW == m_Arinc429_Debug.OutputType ) )
           {
             // Write all messages received to the Debug Buffer so they can be displayed
             Arinc429MgrWriteBuffer ( &m_Arinc429DebugBuffer[nChannel], &nArinc429Msg,
                                      sizeof(nArinc429Msg) );
           }
           // Update over all Rx Chan Count
           pRxStatusChan->RxCnt++;
           // Store the received message to the Label Data Storage Table
           pLabelData = Arinc429MgrStoreLabelData ( nArinc429Msg, pRxChan,
                                                    &sdi, nTickCount, timeStamp );
           // Check if recording is active for this channel -
           // Is activated through a start snapshot request
           if ( TRUE == pRxChan->bRecordingActive )
           {
             // Apply the configured protocol
             if ( Arinc429MgrApplyProtocol( pRxCfgChan, pRxChan,
               pLabelData, sdi, &m_ARINC429Record ) )
             {
               // Protocol has decided to keep the message received now
               // check if Label is to be or not to be filtered OUT.
               if ( TRUE == pLabelData->sd[sdi].Accept )
               {
                 // Store the current TimeSyncDelta and write the record to the Buffer
                 m_ARINC429Record.DeltaTime = pRxChan->TimeSyncDelta;
                 Arinc429MgrWriteBuffer ( &m_Arinc429RxBuffer[nChannel],
                                          &m_ARINC429Record,
                                          sizeof(m_ARINC429Record) );
               }
             } // end protocol applied
           } // end recording active
         } // end for all messages read or loop limit reached

         if ( ( ARINC429_RX_PROTOCOL_REDUCE == pRxCfgChan->Protocol) &&
           ( TRUE == pRxChan->bRecordingActive ) )
         {
           // Now check all configured parameters for reduction protocol
           Arinc429MgrChkParamLostTimeout ( pRxCfgChan, pRxChan,
                                            &m_Arinc429RxBuffer[nChannel] );
         }
       }
     }
   } // Normal task execution.

}

/******************************************************************************
 * Function:    Arinc429MgrProcMsgsTaskShutdown
 *
 * Description: Shutdown function for Arinc429MgrProcessMsgsTask. This function
 *              is called when system mode enters SHUTDOWN.
 *              This provides an opportunity to disable interfaces prior to terminating
 *              the task.
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:
 *
 *
 *****************************************************************************/
void Arinc429MgrProcMsgsTaskShutdown(void)
{
  ARINC429_RX_CFG_PTR  pRxCfgChan;
  ARINC429_TX_CFG_PTR  pTxCfgChan;
  UINT8                Channel;

  // Re-config the RX channels settings as disabled (enable <- "FALSE")
  for (Channel = 0; Channel < FPGA_MAX_RX_CHAN; Channel++)
  {
    pRxCfgChan = &m_Arinc429Cfg.RxChan[Channel];
    Arinc429DrvCfgRxChan( pRxCfgChan->Speed, pRxCfgChan->Parity,
                          pRxCfgChan->LabelSwapBits, FALSE, Channel );
  }
  // Re-config the TX channels settings as disabled (enable <- "FALSE")
  for (Channel = 0; Channel < FPGA_MAX_TX_CHAN; Channel++)
  {
    pTxCfgChan = &m_Arinc429Cfg.TxChan[Channel];
    Arinc429DrvCfgTxChan ( pTxCfgChan->Speed, pTxCfgChan->Parity,
                           pTxCfgChan->LabelSwapBits, FALSE, Channel );
  }
   // Reconfigure the FPGA with the new config settings.
  Arinc429DrvReconfigure();
}


/******************************************************************************
 * Function:    Arinc429MgrBITTask
 *
 * Description: Provides Arinc429 CBIT Test of all four Rx Ch
 *                - Counts Framing and Parity Errors Detected
 *                - Startup and Data Loss Timeout Processing
 *                - m_Arinc429RxBuffer[] buffer overflow
 *
 * Parameters:  pParam -  ptr to m_Arinc429MgrBlock
 *
 * Returns:     none
 *
 * Notes:
 *  1) FIFO Half Full and FIFO Full count logic are provided in the _ProcessISR_RX
 *     routine and are INT driven.  Note: Current
 *  2) Logic to determine if too many Parity, Framing, FIFO Half Full, or
 *     FIFO Full cnt not provided.
 *  3) ARINC429_SYSTEM_STATUS updated for CBIT data loss only.
 *     If too many Parity or Framing Errors are detected, ARINC429_SYSTEM_STATUS
 *     will be un-effected.  Logic must be changed to incorporate these
 *     parameters.
 *
 *****************************************************************************/
void Arinc429MgrBITTask ( void *pParam )
{
   // Local Data
   ARINC429_MGR_TASK_PARAMS_PTR  pArinc429MgrBlock;
   ARINC429_SYS_RX_STATUS_PTR    pArincRxStatus;
   ARINC429_RAW_RX_BUFFER_PTR    pArincRxBuffer;
   UINT8                         Channel;
   ARINC429_RX_CFG_PTR           pArincRxCfg;
   UINT32                        nTimeOut;
   BOOLEAN                       *pTimeOutFlag;
   BOOLEAN                       bStatusOk;
   BOOLEAN                       bDataPresent;
   ARINC429_RX_CFG_PTR           pArinc429Cfg;

   pArinc429MgrBlock = pParam;
   bDataPresent      = FALSE;

   // Monitors Bus Activity from FPGA (FPGA_429_RXn_STATUS) if Rx Chan enabled
   //    based on (m_Arinc429MgrBlock->RxChanEnabled[])
   for ( Channel = 0; Channel < FPGA_MAX_RX_CHAN; Channel++ )
   {
      pArincRxStatus = &pArinc429MgrBlock->Rx[Channel];
      bStatusOk = TRUE;  // Assume all CBIT for this ch is ok.

      // Process CBIT for Rx Chan that is Enb and PBIT (Drv and Sys) Passes
      if ( ( pArinc429MgrBlock->Rx[Channel].Enabled == TRUE ) &&
           ( pArincRxStatus->Status != ARINC429_STATUS_FAULTED_PBIT ) )
      {
         // If FPGA is currently OK, perform the following CBIT processing
         if ( TRUE == Arinc429DrvFPGA_OK() )
         {
            bDataPresent = Arinc429DrvCheckBITStatus ( &pArincRxStatus->ParityErrCnt,
                                                       &pArincRxStatus->FramingErrCnt,
                                                       &pArinc429MgrBlock->InterruptCnt,
                                                       &pArincRxStatus->FPGA_Status,
                                                       Channel );
         }

         // Check Channel Activity
         if ( TRUE == bDataPresent )
         {
            if ( pArincRxStatus->bChanActive == FALSE )
            {
               // Set to TRUE
               pArincRxStatus->bChanActive = TRUE;
               // Output Debug Message on transition
               snprintf (GSE_OutLine, sizeof(GSE_OutLine),
                        "Arinc429_Monitor: Arinc429 Data Present Detected (Ch=%d)",
                        Channel );
               GSE_DebugStr(NORMAL,TRUE,GSE_OutLine);

               // Clear SetStatus, if set Chan or Startup Time Occurred
               if ( (pArincRxStatus->bChanTimeOut == TRUE) ||
                    (pArincRxStatus->bChanStartUpTimeOut == TRUE) )
               {
                  pArinc429Cfg = ( ARINC429_RX_CFG_PTR ) &m_Arinc429Cfg.RxChan[Channel];
                  Flt_ClrStatus ( pArinc429Cfg->ChannelSysCond );
               }
            }
            pArincRxStatus->LastActivityTime    = CM_GetTickCount();
            pArincRxStatus->bChanTimeOut        = FALSE;
            pArincRxStatus->bChanStartUpTimeOut = FALSE;
         } // End if Arinc Data present !
         else
         {
            pArincRxCfg = ( ARINC429_RX_CFG_PTR ) &m_Arinc429Cfg.RxChan[Channel];

            // Determine StartUp or Regular TimeOut
            if ( pArincRxStatus->LastActivityTime != 0 )
            {
               // Activity received, check for subsequent timeout
               nTimeOut = pArincRxCfg->ChannelTimeOut_s;
               pTimeOutFlag = (BOOLEAN *) &pArincRxStatus->bChanTimeOut;
            }
            else
            {
               // Activity never received, check for startup timeout
               nTimeOut = pArincRxCfg->ChannelStartup_s;
               pTimeOutFlag = (BOOLEAN *) &pArincRxStatus->bChanStartUpTimeOut;
            }

            if ( ((CM_GetTickCount() - pArincRxStatus->LastActivityTime ) >
                  (nTimeOut * MILLISECONDS_PER_SECOND)) && (nTimeOut != ARINC429_DSB_TIMEOUT) )
            {
               if ( *pTimeOutFlag != TRUE )
               {
                  // Output Debug Msg only on transition !
                  if ( pArincRxStatus->LastActivityTime != 0 )
                  {
                     // Output Debug Message on transition
                     snprintf (GSE_OutLine, sizeof(GSE_OutLine),
                              "Arinc429_Monitor: Arinc429 Data Present Lost (Ch=%d)!",
                              Channel);
                     GSE_DebugStr(NORMAL,TRUE,GSE_OutLine);
                     Arinc429MgrCreateTimeOutSysLog(SYS_A429_DATA_LOSS_TIMEOUT, Channel);
                  }
                  else
                  {
                     // Output Debug Message on transition
                     snprintf (GSE_OutLine, sizeof(GSE_OutLine),
                              "Arinc429_Monitor: Arinc429 Startup Time Out (Ch=%d)!",
                              Channel);
                     GSE_DebugStr(NORMAL,TRUE,GSE_OutLine);
                     Arinc429MgrCreateTimeOutSysLog(SYS_A429_STARTUP_TIMEOUT, Channel);
                  }
                  pArincRxStatus->DataLossCnt++;
               }
               *pTimeOutFlag               = TRUE;
               pArincRxStatus->bChanActive = FALSE;
               bStatusOk                   = FALSE;
            } // End if TimeOut exceeded
         } // End else if data not present

         //    Always Enb INT for Rx Ch Half Full and Full for all ch.  Only if
         //    rx ch is Enb will actual INT for that ch actually be generate.
         //    NOTE: As of revision 8 of the FPGA HW the Rx FIFO are 256 deep.
         // Check FIFO_HalfFullCnt (set in Arinc429_ISR) and FIFO_HalfFullCntPrev

         // Check RawSwBuffOverFlowCnt
         pArincRxBuffer = (ARINC429_RAW_RX_BUFFER_PTR) &m_Arinc429RxBuffer[Channel];

         if ( pArincRxBuffer->bOverFlow == TRUE )
         {
            pArincRxStatus->SwBuffOverFlowCnt++;
            pArincRxBuffer->bOverFlow = FALSE;
         }

         // Update Overall Rx Chan Status for CBIT processing.
         // Note: If ARINC429_STATUS_FAULTED_PBIT, ARINC429_STATUS_DISABLED_OK,
         //       or ARINC429_STATUS_DISABLED_FAULTED logic for CBIT will not
         //       execute.  For CBIT execution, status will toggle between
         //       ARINC429_STATUS_OK or ARINC429_STATUS_FAULTED_CBIT.
         if (bStatusOk != TRUE)
         {
            pArincRxStatus->Status = ARINC429_STATUS_FAULTED_CBIT;
         }
         else
         {
            pArincRxStatus->Status = ARINC429_STATUS_OK;
         }
      } // End of if ( pArinc429MgrBlock->RxChanEnabled[i] == TRUE ) &&
        //           ( pArincRxStatus->Status != ARINC429_STATUS_FAULTED_PBIT )
   } // End for loop thru FPGA_MAX_RX_CHAN
    // TBD Loop thru Tx Stuff here
}

/******************************************************************************
 * Function:    Arinc429MgrDisplaySWBufferTask
 *
 * Description: Debug routine to print raw Arinc Rx data to the GSE port
 *
 * Parameters:  pParam -  ptr to m_Arinc429MgrBlock
 *
 * Returns:     none
 *
 * Notes:
 *  1) arinc429.debug.rawfilter is enabled, then output debug to GSE port
 *  2) Two screen format are provided, a col mode where all 4 rx ch are
 *     displayed and a single col mode where first in first out is displayed.
 *  3) A formatted mode where Octal label and parity bits are seperated.
 *  4) Note:  For fully loaded hi speed ch, data is trunc to the GSE port
 *     to prevent GSE from overflowing and crashing.
 *
 *****************************************************************************/
void Arinc429MgrDisplaySWBufferTask ( void *pParam )
{
  // Local Data
   static UINT32 nMIFCnt = 0;  // Since task manager can only be set to 320 msec slowest,
                               //  will set this task as 250 msec RMT and exe every even
                               //  nMIFCnt for 2 Hz !

  if (m_Arinc429_Debug.bOutputRawFilteredBuff == TRUE)
  {
    if ( (nMIFCnt++ % 2) == 0 )
    {
      if ( m_Arinc429_Debug.bOutputMultipleArincChan == FALSE )
      {
        Arinc429MgrDispSingleArincChan();
      }
      else
      {
        Arinc429MgrDisplayMultiArincChan();
      }
    }
  }
}


/*****************************************************************************/
/* DATA MANAGER INTERFACE FUNCTIONS                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:    Arinc429MgrReadFilteredRaw
 *
 * Description: Returns the Rx Raw Arinc 429 data (stored in sw m_Arinc429RxBuffer[])
 *              from a specified ch to the calling application
 *
 *
 * Parameters:  pDest - ptr to App Buffer to copy data (MUST BE LONG WORD ALIGNED !)
 *              chan - Rx Chan to process raw data
 *              nMaxByteSize - max size of App Buffer
 *
 * Returns:     UINT16 cnt - Number of bytes copied into pDest Buffer.
 *
 * Notes:
 *  1) When "bOutputRawFilteredBuff==TRUE and the Type is not DEBUG_OUT_RAW the Data
 *     from the Receive Buffer will be pumped to the display and will not be available
 *     to applications
 *  2) m_Arinc429RxBuffer[] is a Rolling buffer.
 *  3) Max records storable in Raw Arinc Buff is ARINC429_MAX_RECORDS or 3200
 *     thus no more than 3200 records or 16000 bytes can be returned.
 *
 *****************************************************************************/
UINT16 Arinc429MgrReadFilteredRaw( void *pDest, UINT32 chan, UINT16 nMaxByteSize)
{
  // Local Data
  UINT16 Cnt;
  UINT32 Records;

  Cnt = 0;

  if ( ( FALSE == m_Arinc429_Debug.bOutputRawFilteredBuff ) ||
       (( TRUE == m_Arinc429_Debug.bOutputRawFilteredBuff ) &&
        ( m_Arinc429_Debug.OutputType == DEBUG_OUT_RAW)) )
  {
     Records = Arinc429MgrReadBuffer ( pDest, nMaxByteSize, &m_Arinc429RxBuffer[chan] );
     Cnt     = Records * sizeof(ARINC429_RECORD);
  }

  return ( Cnt );
}

/******************************************************************************
 * Function:    Arinc429MgrReadFilteredRawSnap
 *
 * Description: Returns the current snapshot view of all label word data from the
 *              m_ArincLabelData[] data store. Only if label word data has been
 *              received (see logic below) is data returned. Function calls
 *              Special processing for the reduction protocol.
 *
 * Parameters:  pDest - ptr to App Buffer to copy data
 *              chan - Rx Chan to process raw data
 *              nMaxByteSize - max size of App Buffer
 *              bStartSnap - BOOLEAN indicating TRUE if start of a snapshot
 *
 * Returns:     UINT16 cnt - Number of bytes copied into pDest Buffer.
 *
 * Notes:
 *  - Based on ARINC429_LABEL_INFO_FILTER ->Accept, return current snapshot
 *    of label word(s) that are != 0.
 *
 *
 *****************************************************************************/
UINT16 Arinc429MgrReadFilteredRawSnap( void *pDest, UINT32 chan, UINT16 nMaxByteSize,
                                       BOOLEAN bStartSnap )
{
   // Local Data
   UINT16                       Cnt;
   UINT16                       label;
   UINT16                       nMaxRecords;
   UINT8                        sdi;
   ARINC429_SNAPSHOT_RECORD_PTR pbuff;
   ARINC429_RX_CFG_PTR          pCfg;
   ARINC429_CHAN_DATA_PTR       pChanData;
   ARINC429_LABEL_DATA_PTR      pLabelData;
   ARINC429_SDI_DATA_PTR        pSdiData;
   ARINC429_SYS_RX_STATUS_PTR   pRxStatusChan;

   // Initialize Local Data
   pChanData     = &m_Arinc429RxChannel[chan];
   pCfg          = &m_Arinc429Cfg.RxChan[chan];
   pRxStatusChan = &m_Arinc429MgrBlock.Rx[chan];
   pbuff         = (ARINC429_SNAPSHOT_RECORD_PTR)pDest;
   nMaxRecords   = nMaxByteSize / sizeof(ARINC429_SNAPSHOT_RECORD);
   Cnt           = 0;

   // Set the recording flag on a start snapshot and clear it on an end snapshot
   pChanData->bRecordingActive = (bStartSnap == TRUE) ? TRUE : FALSE;

   // Check if any data received on the channel before trying to build the
   // snapshot
   if ( pRxStatusChan->RxCnt > 0 )
   {
      // If the storage area is larger than the total possible Messages
      // Then set the Max to the possible max
      if ( nMaxRecords > (ARINC429_MAX_LABELS * ARINC429_MAX_SD) )
      {
         nMaxRecords = ARINC429_MAX_LABELS * ARINC429_MAX_SD;
      }

      // Check if channel is configured for Reduction Protocol
      if ( ARINC429_RX_PROTOCOL_REDUCE == pCfg->Protocol )
      {
         // Request the Reduction Protocol Snapshot
         Cnt = Arinc429MgrReadReduceSnapshot(pbuff, nMaxRecords, pChanData, pCfg, bStartSnap);
      }
      else // No Special Protocol
      {
         // Loop through all the labels
         for (label = 0; (( label < ARINC429_MAX_LABELS ) && ( Cnt < nMaxRecords )); label++)
         {
            pLabelData = &pChanData->LabelData[label];

            // Loop through all the sdi combinations for each label
            for ( sdi = 0; sdi < ARINC429_MAX_SD; sdi++ )
            {
               pSdiData = &pLabelData->sd[sdi];

               // Check if the label should be accepted
               if ( TRUE == pSdiData->Accept )
               {
                  // Determine if at least one message has been received
                  if ( 0 != pSdiData->CurrentMsg )
                  {
                     // Store the Timestamp and Message
                     pbuff->TimeStamp   = pSdiData->RxTimeStamp;
                     pbuff->Arinc429Msg = pSdiData->CurrentMsg;

                     pbuff++;
                     Cnt++;
                  }
               }
            }
         }
      }
   }

   // Return the total size of the data stored to the buffer
   return( Cnt * sizeof(ARINC429_SNAPSHOT_RECORD) );
}

/******************************************************************************
 * Function:    Arinc429MgrSyncTime
 *
 * Description: Called by the application using the Data Interface to sync
 *              the time. This function zero's the count and then records
 *              the start time so the delta can be calculated when the
 *              ARINC429 messages are received.
 *
 * Parameters:  UINT32 chan - Channel to sync time on.
 *
 * Returns:     None.
 *
 * Notes:       Storage container for TimeSyncDelta is UINT8 so time sync delta
 *              should be in a 10ms Resolution. This will allow for a 2.55 Second
 *              delta max.
 *
 *****************************************************************************/
void Arinc429MgrSyncTime( UINT32 chan )
{
   m_Arinc429RxChannel[chan].TimeSyncDelta = 0;
   m_Arinc429RxChannel[chan].SyncStart     = CM_GetTickCount();
}

/******************************************************************************
 * Function:     Arinc429MgrGetFileHdr
 *
 * Description:  Called by the Data Manager Application to retrieve the
 *               specific configuration for the interface.
 *
 * Parameters:   pDest - pointer to destination buffer
 *               chan  - channel to request file header
 *               nMaxByteSize - maximum destination buffer size
 *
 * Returns:      cnt - actual number of bytes returned
 *
 * Notes:        none
 *
 *****************************************************************************/
UINT16 Arinc429MgrGetFileHdr ( void *pDest, UINT32 chan, UINT16 nMaxByteSize )
{
   // Local Data
   ARINC429_CHAN_HDR   ChanHdr;
   UINT16              cnt;
   INT8                *pBuffer;
   ARINC429_RX_CFG_PTR pCfg;
   UINT16              NumParams;

   // Initialize Local Data
   pBuffer   = (INT8 *)(pDest) + sizeof(ChanHdr);  // Move to end of Channel File Hdr
   pCfg    = &m_Arinc429Cfg.RxChan[chan];
   cnt     = 0;
   NumParams = 0;

   // Check if the protocol is reduction
   if ( ARINC429_RX_PROTOCOL_REDUCE == pCfg->Protocol )
   {
      // Retrieve the Reduction Prtotocol Configuration
      cnt = Arinc429MgrGetReduceHdr (pBuffer, chan, (nMaxByteSize - sizeof(ChanHdr)),
                                                                                  &NumParams);
   }

   // Update Chan file hdr
   memset ( &ChanHdr, 0, sizeof(ChanHdr) );

   ChanHdr.TotalSize = cnt + sizeof(ChanHdr);
   ChanHdr.Channel   = chan;
   strncpy ( ChanHdr.Name, pCfg->Name, sizeof(pCfg->Name) );
   ChanHdr.Protocol  = pCfg->Protocol;
   ChanHdr.NumParams = NumParams;

   // Copy Chan file hdr to destination
   pBuffer = pDest;
   memcpy ( pBuffer, &ChanHdr, sizeof(ChanHdr) );

   return (ChanHdr.TotalSize);
}

/******************************************************************************
 * Function:     Arinc429MgrGetSystemHdr
 *
 * Description:  Retrieves the binary system header for the ARINC429
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
UINT16 Arinc429MgrGetSystemHdr ( void *pDest, UINT16 nMaxByteSize )
{
   // Local Data
   ARINC429_SYS_HDR m_Arinc429SysHdr[FPGA_MAX_RX_CHAN];
   UINT32           nChannelIndex;
   INT8             *pBuffer;
   UINT16           nRemaining;
   UINT16           nTotal;

   // Initialize Local Data
   pBuffer    = (INT8 *)pDest;
   nRemaining = nMaxByteSize;
   nTotal     = 0;
   // Clear the header
   memset ( m_Arinc429SysHdr, 0, sizeof(m_Arinc429SysHdr) );
   // Loop through all the ARINC429 channels
   for ( nChannelIndex = 0;
         ((nChannelIndex < FPGA_MAX_RX_CHAN) &&
          (nRemaining > sizeof (ARINC429_SYS_HDR)));
         nChannelIndex++ )
   {
      // Copy the name to the header
      strncpy_safe( m_Arinc429SysHdr[nChannelIndex].Name,
                    sizeof(m_Arinc429SysHdr[nChannelIndex].Name),
                    m_Arinc429Cfg.RxChan[nChannelIndex].Name,
                    _TRUNCATE);
      // Increment the total used and decrement the total remaining
      nTotal     += sizeof (ARINC429_SYS_HDR);
      nRemaining -= sizeof (ARINC429_SYS_HDR);
   }
   // Make sure the is something to copy and then copy it to the buffer
   if ( 0 != nTotal )
   {
      memcpy ( pBuffer, m_Arinc429SysHdr, nTotal );
   }
   // Return the total number of bytes written to the buffer
   return ( nTotal );
}

/*****************************************************************************/
/* SENSOR INTERFACE FUNCTIONS                                                */
/*****************************************************************************/

/******************************************************************************
 * Function:     Arinc429MgrSensorSetup
 *
 * Description:  Sets the m_Arinc429WordInfo[] table with parse info for a
 *               specific Arinc429 Data Word.
 *
 * Parameters:   gpA - General Purpose A (See below for definition)
 *               gpB - General Purpose B (See below for definition)
 *               label - Label to be stored in word Info
 *               nSensor - Sensor associated with this word info
 *
 * Returns:      Index into Arinc429WordInfo[]
 *               (To be used in later Arinc429 Sensor Processing)
 *
 * Notes:
 *
 *  1) After calling this func, the calling app should issue Arinc429_Reconfigure()
 *       to clr and reset the FPGA Shadow RAM (in particular the LFR regs !)
 *
 *****************************************************************************/
UINT16 Arinc429MgrSensorSetup (UINT32 gpA, UINT32 gpB, UINT8 label, UINT16 nSensor)
{
  // Local Data
  ARINC429_WORD_INFO_PTR   pWordInfo;
  ARINC429_SENSOR_INFO_PTR pSensorInfo;
  UINT16                   Cnt;

  // Initialize Local Data
  Cnt         = m_Arinc429WordSensorCount;
  pSensorInfo = &m_Arinc429SensorInfo[Cnt];
  pWordInfo   = &pSensorInfo->WordInfo;

  // Parse the General Purpose Configuration Fields
  Arinc429MgrParseGPA_GPB (gpA, gpB, pWordInfo);
  // Store the Label
  pWordInfo->Label        = label;
  pSensorInfo->TotalTests = (UINT8) END_OF_FAIL_COUNT;
  pSensorInfo->nSensor    = nSensor;

  // Update Arinc429 Sensor Count
  m_Arinc429WordSensorCount++;

  // Once an Arinc429 Sensor has been requested, update the label Filtering on
  //  FPGA (thru FPGA Shadow RAM and internal Arinc 429 struct)
  Arinc429MgrUpdateLabelFilter ( pSensorInfo );

  // Return current index count back to calling app, so this object can
  //  be referenced later.
  return(Cnt);

}

/******************************************************************************
 * Function:     Arinc429MgrReadWord
 *
 * Description:  Returns a current Arinc429 Data Word Value from the
 *               m_ArincLabelData[] data store
 *
 * Parameters:   nIndex - index into the m_Arinc429SensorInfo[] containing the
 *                        the specific Arinc Sensor Data to read / parse
 *               *tickCount - ptr to return last rx update time of parameter
 *
 * Returns:      FLOAT32 the requested Arinc429 Data Word Value
 *
 * Notes:
 *  1) If the current Data Word fails SSM filtering, the prev Data Word
 *     passing SSM filtering is returned.
 *  2) The BCD, BNR or DISC word data is converted and return as a SINT32 value
 *  3) For SDI ignore, the most recent received Data Word (independent of SDI)
 *     is returned.
 *
 *****************************************************************************/
FLOAT32 Arinc429MgrReadWord (UINT16 nIndex, UINT32 *tickCount)
{
   // Local Data
   ARINC429_SENSOR_INFO_PTR pSensorInfo;
   ARINC429_WORD_INFO_PTR   pWordInfo;
   ARINC429_LABEL_DATA_PTR  pLabelData;
   SINT32                   svalue;
   UINT32                   raw;
   UINT32                   rxTime;
   UINT8                    msb;
   BOOLEAN                  bWordValid;
   UINT16                   i;

   // Initialize Local Data
   pSensorInfo = &m_Arinc429SensorInfo[nIndex];
   pWordInfo   = &pSensorInfo->WordInfo;
   pLabelData  = &m_Arinc429RxChannel[pWordInfo->RxChan].LabelData[pWordInfo->Label];

   // Initially set to previous good value
   svalue = pSensorInfo->value_prev;

   // Based on Octal and SD retrieve Arinc Word from m_ArincLabelData[][]

   // If SDI is Ignore or All Call get value from ->SD_Ignore field
   if ( ( pWordInfo->IgnoreSDI == TRUE ) || ( pWordInfo->SDIAllCall  == TRUE ) )
   {
      raw    = pLabelData->sd[SDI_IGNORE].CurrentMsg;    // Get raw value from sd_ignore field
      rxTime = pLabelData->sd[SDI_IGNORE].RxTime;
   }
   else
   {
      raw    = pLabelData->sd[pWordInfo->SDBits].CurrentMsg;   // Get raw value from sd[] field
      rxTime = pLabelData->sd[pWordInfo->SDBits].RxTime;
   }

   // If have updated value then process raw value, otherwise return previous data
   if ( pSensorInfo->TimeSinceLastUpdate != rxTime )
   {
      // We have updated value
      pSensorInfo->TimeSinceLastUpdate = rxTime;

      // If SSM processing indicates "BAD" or filtered out/in value, return "value_prev" or
      //   current word value in m_ArincLabelData[][]
      msb        = (UINT8) (raw >> ARINC_MSG_SHIFT_MSB);
      bWordValid = TRUE;

      // Loop thru all SSM comparison checks
      // Warning: Be aware that the order of FailureCondition is directly
      //          related to the ValidSSM field of the ArincFilter. Any
      //          changes should be made with care.
      //          ValidSSM bit 0 = SSM 00 = FailureCondition[0]
      //          ValidSSM bit 1 = SSM 01 = FailureCondition[1]
      //          ValidSSM bit 2 = SSM 10 = FailureCondition[2]
      //          ValidSSM bit 3 = SSM 11 = FailureCondition[3]
      // NOTE: For normal SSM processing, in Arinc429_SensorSetup() ->ValidSSM "faked" to
      //   "mask" or not perform a specific failure test.  If defined for normal SSM
      //   processing the FailureCondition[SSM_NORMAL] == ARINC_BNR_SSM_NORM_v test would
      //   be "bypassed" !
      for (i = 0; ((i < pSensorInfo->TotalTests) && (bWordValid == TRUE)); i++)
      {
         if ( ((pWordInfo->ValidSSM & (1U << i)) == 0) &&
              ((msb & ARINC_SSM_BITS) == FailureCondition[i]) )
         {
            bWordValid = FALSE;
            pSensorInfo->FailCount[i]++; // FailCount only gets update if we have updated value
         }
      } // End for i loop thru FailureCondition[] comparison tests

      if ( bWordValid == TRUE )
      {
         svalue = Arinc429MgrParseMsg ( raw, pWordInfo->Format, pWordInfo->WordPos,
                                        pWordInfo->WordSize, (UINT32)pWordInfo->RxChan,
                                        &bWordValid );
         if ( TRUE == bWordValid )
         {
            // We have good value, update TimeSinceLastGoodValue param
            pSensorInfo->TimeSinceLastGoodValue = rxTime;
         }
         else if ( ( FALSE == bWordValid ) && ( BCD == pWordInfo->Format ) )
         {
            // ->value_prev will be returned
            pSensorInfo->FailCountBCD++;
            svalue = pSensorInfo->value_prev;
         }
      }
   } // End if (pWordInfo->TimeSinceLastUpdate != rxTime) we have updated value

   // Update value previous
   // NOTE: On word decode error or no updated value, svalue initially set to previous value
   pSensorInfo->value_prev = svalue;

   *tickCount = pSensorInfo->TimeSinceLastUpdate;

   return( (FLOAT32) svalue );
}

/******************************************************************************
 * Function:     Arinc429MgrSensorTest
 *
 * Description:  Determines if the Data Loss Time Out for the sensor word
 *               has been exceeded.
 *
 * Parameters:   nIndex - index into the m_Arinc429SensorInfo[] containing the
 *                        the specific Arinc Word Data to read / parse
 *
 * Returns:      FALSE - If word data loss timeout exceeded
 *               TRUE  - If new word data received
 *
 * Notes:
 *   1) Arinc Sensor Test philosophy is based on "Data Loss Timeout".
 *      The cause of the data loss could be the result of a bad SSM or
 *      no Word Label Data not in the Arinc data stream, or Arinc Ch
 *      has faulted.  When data loss timeout is exceeded, the sensor
 *      data is considered FAULTED.
 *   2) TBD cause of timeout determined from SSM failure or No Data on
 *      ch.  An Xcptn log is generated.
 *
 *****************************************************************************/
BOOLEAN Arinc429MgrSensorTest (UINT16 nIndex)
{
   // Local Data
   BOOLEAN                        bValid;
   ARINC429_SENSOR_INFO_PTR       pSensorInfo;
   ARINC429_WORD_INFO_PTR         pWordInfo;
   ARINC429_SYS_RX_STATUS_PTR     pArincRxStatus;
   ARINC429_SYS_CBIT_SSM_FAIL_LOG SSMFailLog;
   CHAR                           TempStr[32];

   // Initialize Local Data
   bValid         = FALSE;
   pSensorInfo    = &m_Arinc429SensorInfo[nIndex];
   pWordInfo      = &pSensorInfo->WordInfo;
   pArincRxStatus = &m_Arinc429MgrBlock.Rx[pSensorInfo->WordInfo.RxChan];

   // We have data activity on the Arinc 429 Bus  !!!
   //if ( pArincRxStatus->LastActivityTime != 0 )
   if (TRUE == pArincRxStatus->bChanActive)
   {
      bValid = TRUE;

      // Initial Start. Set TimeSinceLastGoodValue
      if ( pSensorInfo->TimeSinceLastGoodValue == 0 )
      {
         pSensorInfo->TimeSinceLastGoodValue = CM_GetTickCount();
      }

      // Determine if we exceeded the Data Loss Time Out value
      if ((CM_GetTickCount() - pSensorInfo->TimeSinceLastGoodValue) > pWordInfo->DataLossTime)
      {
         // Sensor Data Loss Time Out Exceeded
         bValid = FALSE;

         // Determine SSM or Data Loss Failure if it has not been reported.
         if ( pSensorInfo->bFailed == FALSE )
         {
            SSMFailLog.result = SYS_A429_SSM_FAIL;
            Arinc429MgrDetermineSSMFailure( pSensorInfo->FailCount,
                                            pSensorInfo->TotalTests,
                                            SSMFailLog.FailMsg );
            snprintf(TempStr, sizeof(TempStr)," S = %d", pSensorInfo->nSensor);
            strcat(SSMFailLog.FailMsg, TempStr);

#ifdef ARINC429_DEBUG_COMPILE
            /*vcast_dont_instrument_start*/
            // Debug testing
            snprintf (GSE_OutLine, sizeof(GSE_OutLine), 
                      "Arinc429_SensorTest: %s", SSMFailLog.FailMsg);
            GSE_DebugStr(NORMAL,TRUE,GSE_OutLine);
            /*vcast_dont_instrument_end*/
#endif
            // Log CBIT Here only on transition from data rx to no data rx
            LogWriteSystem( SYS_ID_A429_CBIT_SSM_FAIL, LOG_PRIORITY_LOW,
                           &SSMFailLog,
                           sizeof(ARINC429_SYS_CBIT_SSM_FAIL_LOG), NULL );

            // Count every Word Loss Transition
            pSensorInfo->WordLossCnt++;
            pSensorInfo->bFailed = TRUE;

         } // End of if ->bFailed != TRUE
      } // End of if Data Loss Time Out Exceeded
      else // Else Data Loss Time Out Not Exceeded
      {
         // Transition from no data rx to data rx, clear counters
         if ( pSensorInfo->bFailed == TRUE )
         {
            pSensorInfo->bFailed = FALSE;
            // Clear current counts on transition from no data to data rx
            memset ((void *) pSensorInfo->FailCount, 0, sizeof(pSensorInfo->FailCount));
         } // End of for (i) loop
      } // End of else for Data has not met Data Loss Timeout Condition
   } // End if of ->LastActivityTime != 0

   return (bValid);
}

/******************************************************************************
 * Function:     Arinc429MgrInterfaceValid
 *
 * Description:  Determines if any Arinc 429 Data has been received since
 *               startup.
 *
 * Parameters:   nIndex - index used to initialize sensorInfo
 *
 * Returns:      TRUE - Arinc429 Data has been received since startup
 *               FALSE - Arinc429 Data has NOT been received since startup
 *
 * Notes:
 *  1) On startup ->TimeSinceLastGoodValue int to 0.  Becomes !0 on any Rx Data
 *
 *****************************************************************************/
BOOLEAN Arinc429MgrInterfaceValid (UINT16 nIndex)
{
   // Local Data
   BOOLEAN                   bValid;
   ARINC429_SENSOR_INFO_PTR  pSensorInfo;

   // Intialize Local Data
   pSensorInfo = &m_Arinc429SensorInfo[nIndex];
   bValid      = FALSE;

   if ( ( ARINC429_STATUS_OK == m_Arinc429MgrBlock.Rx[pSensorInfo->WordInfo.RxChan].Status ) &&
      ( TRUE == m_Arinc429MgrBlock.Rx[pSensorInfo->WordInfo.RxChan].bChanActive ) )
   {
      bValid = TRUE;
   }
   else
   {
      // Keep updating the time so we don't fail if the channel goes away and returns
      pSensorInfo->TimeSinceLastGoodValue = 0;
   }

   return (bValid);
}

/*****************************************************************************/
/* BUILT-IN-TEST INTERFACE                                                   */
/*****************************************************************************/

/******************************************************************************
 * Function:     Arinc429MgrGetCBITHealthStatus
 *
 * Description:  Returns the current CBIT Health status count of all Arinc
 *               429 Rx Ch
 *
 * Parameters:   none
 *
 * Returns:      Current data in ARINC429_CBIT_HEALTH_COUNTS
 *
 * Notes:        none
 *
 *****************************************************************************/
ARINC429_CBIT_HEALTH_COUNTS Arinc429MgrGetCBITHealthStatus ( void )
{
   // Local Data
   ARINC429_SYS_RX_STATUS_PTR pArincRxStatusChan;
   ARINC429_CBIT_HEALTH_COUNTS_CH_PTR pArincCBITCountsCh;
   UINT8 i;

   for ( i = 0; i < FPGA_MAX_RX_CHAN; i++ )
   {
      pArincRxStatusChan = &m_Arinc429MgrBlock.Rx[i];
      pArincCBITCountsCh = &m_Arinc429CBITHealthStatus.ch[i];

      pArincCBITCountsCh->SwBuffOverFlowCnt = pArincRxStatusChan->SwBuffOverFlowCnt;
      pArincCBITCountsCh->ParityErrCnt      = pArincRxStatusChan->ParityErrCnt;
      pArincCBITCountsCh->FramingErrCnt     = pArincRxStatusChan->FramingErrCnt;
      pArincCBITCountsCh->FIFO_HalfFullCnt  = (Arinc429DrvRxGetCounts(i)->FIFO_HalfFullCnt);
      pArincCBITCountsCh->FIFO_FullCnt      = (Arinc429DrvRxGetCounts(i)->FIFO_FullCnt);
      pArincCBITCountsCh->DataLossCnt       = pArincRxStatusChan->DataLossCnt;
      pArincCBITCountsCh->Status            = pArincRxStatusChan->Status;
   }
   return (m_Arinc429CBITHealthStatus);
}

/******************************************************************************
 * Function:     Arinc429MgrCalcDiffCBITHealthSts
 *
 * Description:  Calc the difference in CBIT Health Counts to support PWEH SEU
 *               (Used to support determining SEU cnt during data capture)
 *
 * Parameters:   PrevCount - Initial count value
 *
 * Returns:      Diff in CBIT_HEALTH_COUNTS from (Current - PrevCnt)
 *
 * Notes:        none
 *
 *****************************************************************************/
ARINC429_CBIT_HEALTH_COUNTS Arinc429MgrCalcDiffCBITHealthSts ( ARINC429_CBIT_HEALTH_COUNTS
                                                                  PrevCount )
{
   // Local Data
   UINT16 i;
   ARINC429_CBIT_HEALTH_COUNTS     DiffCount;
   ARINC429_CBIT_HEALTH_COUNTS_CH *pDiffCountCh;
   ARINC429_CBIT_HEALTH_COUNTS_CH *pCurrentCountCh;
   ARINC429_CBIT_HEALTH_COUNTS_CH *pPrevCountCh;
   ARINC429_CBIT_HEALTH_COUNTS    Arinc429CBITHealthState;

   Arinc429CBITHealthState = Arinc429MgrGetCBITHealthStatus();

   // Loop thru each ch data
   for (i=0;i<FPGA_MAX_RX_CHAN;i++)
   {
      pDiffCountCh    = &DiffCount.ch[i];
      pCurrentCountCh = &Arinc429CBITHealthState.ch[i];
      pPrevCountCh    = &PrevCount.ch[i];

      pDiffCountCh->SwBuffOverFlowCnt = pCurrentCountCh->SwBuffOverFlowCnt -
                                        pPrevCountCh->SwBuffOverFlowCnt;
      pDiffCountCh->ParityErrCnt      = pCurrentCountCh->ParityErrCnt -
                                        pPrevCountCh->ParityErrCnt;
      pDiffCountCh->FramingErrCnt     = pCurrentCountCh->FramingErrCnt -
                                        pPrevCountCh->FramingErrCnt;
      pDiffCountCh->FIFO_HalfFullCnt  = pCurrentCountCh->FIFO_HalfFullCnt -
                                        pPrevCountCh->FIFO_HalfFullCnt;
      pDiffCountCh->FIFO_FullCnt      = pCurrentCountCh->FIFO_FullCnt -
                                        pPrevCountCh->FIFO_FullCnt;
      pDiffCountCh->DataLossCnt       = pCurrentCountCh->DataLossCnt -
                                        pPrevCountCh->DataLossCnt;
      pDiffCountCh->Status            = pCurrentCountCh->Status;
   }
   return (DiffCount);
}

/******************************************************************************
 * Function:     Arinc429MgrAddPrevCBITHealthSts
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
ARINC429_CBIT_HEALTH_COUNTS Arinc429MgrAddPrevCBITHealthSts (
                                      ARINC429_CBIT_HEALTH_COUNTS CurrCnt,
                                      ARINC429_CBIT_HEALTH_COUNTS PrevCnt )
{
  // Local Data
  UINT16 i;
  ARINC429_CBIT_HEALTH_COUNTS AddCount;
  ARINC429_CBIT_HEALTH_COUNTS_CH *pAddCountCh;
  ARINC429_CBIT_HEALTH_COUNTS_CH *pCurrCntCh;
  ARINC429_CBIT_HEALTH_COUNTS_CH *pPrevCntCh;

  // Loop thru each ch data
  for (i = 0; i < FPGA_MAX_RX_CHAN; i++)
  {
    pAddCountCh = &AddCount.ch[i];
    pCurrCntCh  = &CurrCnt.ch[i];
    pPrevCntCh  = &PrevCnt.ch[i];

    pAddCountCh->SwBuffOverFlowCnt = pCurrCntCh->SwBuffOverFlowCnt +
                                     pPrevCntCh->SwBuffOverFlowCnt;
    pAddCountCh->ParityErrCnt      = pCurrCntCh->ParityErrCnt +
                                     pPrevCntCh->ParityErrCnt;
    pAddCountCh->FramingErrCnt     = pCurrCntCh->FramingErrCnt +
                                     pPrevCntCh->FramingErrCnt;
    pAddCountCh->FIFO_HalfFullCnt  = pCurrCntCh->FIFO_HalfFullCnt +
                                     pPrevCntCh->FIFO_HalfFullCnt;
    pAddCountCh->FIFO_FullCnt      = pCurrCntCh->FIFO_FullCnt +
                                     pPrevCntCh->FIFO_FullCnt;
    pAddCountCh->DataLossCnt       = pCurrCntCh->DataLossCnt +
                                     pPrevCntCh->DataLossCnt;
    pAddCountCh->Status            = pCurrCntCh->Status;
  } // Loop for i thru all Arinc Rx Ch

  return (AddCount);
}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/****************************************************************************/
/* INITIALIZATION                                                           */
/****************************************************************************/

/******************************************************************************
 * Function:     Arinc429MgrReconfigureHW
 *
 * Description:  Updates the ARINC429 Driver Hardware
 *
 * Parameters:   none
 *
 * Returns:      none
 *
 * Notes:
 *  1) Only Rx Cfg items are set.
 *  2) Label Filter set by Arinc429_SensorSetup() and stored in Label Data
 *     will be reloaded.
 *  3) Always Enb INT for Rx Ch Half Full and Full for all ch.  Only if
 *     rx ch is Enb will actual INT for that ch actually be generate.
 *     NOTE: As of revision 8 of the FPGA HW the Rx FIFO are 256 deep.
 *
 *****************************************************************************/
static void Arinc429MgrReconfigureHW ( void )
{
   // Local Data
   UINT8                            bit;
   UINT8                            Channel;
   UINT8                            Filter;
   UINT16                           Label;
   ARINC429_CHAN_DATA_PTR           pChanData;
   ARINC429_LABEL_DATA_PTR          pLabelData;
   ARINC429_RX_CFG_PTR              pRxChanCfg;
   ARINC429_TX_CFG_PTR              pTxChanCfg;
   UINT8                            sdi;
   UINT8                            SdiMask;

   // Reset the Label Info struct (and current LFR) setting.  The current values
   //   in the Arinc429 Cfg will be restored first.  Then the filter request from the
   //   sensors (stored in m_ArincSensorLabelFilter[]) will be restored.
   //   Note: Dynamic updates to a sensor cfg will not force an update to the
   //         current m_ArincSensorLabelFilter[].  A reset is required to call
   //         SensorsConfigure() which will setup m_ArincSensorLabelFilter[].
   memset ( (void *) m_Arinc429RxChannel, 0, sizeof(m_Arinc429RxChannel) );

   // Parse arinc_rx[].FilterArray[] to run time variable
   for (Channel = 0; Channel < FPGA_MAX_RX_CHAN; Channel++)
   {
      pChanData  = &m_Arinc429RxChannel[Channel];
      pLabelData = &pChanData->LabelData[0];
      pRxChanCfg = &m_Arinc429Cfg.RxChan[Channel];

      // Loop thru Arinc429 Filter Array
      for ( Filter = 0; Filter < 8; Filter++ )
      {
         // Loop thru all bits
         for ( bit = 0; bit < 32; bit++ )
         {
            if ( ((pRxChanCfg->FilterArray[Filter] >> bit) & 0x00000001UL) != 0x00)
            {
               pLabelData->sd[SDI_00].Accept = TRUE;
               pLabelData->sd[SDI_01].Accept = TRUE;
               pLabelData->sd[SDI_10].Accept = TRUE;
               pLabelData->sd[SDI_11].Accept = TRUE;
            }
            else
            {
               pLabelData->sd[SDI_00].Accept = FALSE;
               pLabelData->sd[SDI_01].Accept = FALSE;
               pLabelData->sd[SDI_10].Accept = FALSE;
               pLabelData->sd[SDI_11].Accept = FALSE;
            }

            pLabelData++; // Move pointer to next label
         } // End for loop bits for each FilterArray[]
      } // End for loop Filter

      // Check if channel is configured for Reduction Protocol
      if ( ARINC429_RX_PROTOCOL_REDUCE == pRxChanCfg->Protocol )
      {
         // Configure the reduction protocol
         Arinc429MgrCfgReduction ( pRxChanCfg, pChanData );
      }

   } // End Channel Loop

   // Intialize the Arinc429 Interrupts
   Arinc429DrvSetIMR1_2();

   // Loop through all the receive channels and configure
   for (Channel = 0; Channel < FPGA_MAX_RX_CHAN; Channel++)
   {
      pRxChanCfg = &m_Arinc429Cfg.RxChan[Channel];

      // Configure the ARINC429 Channel Hardware
      Arinc429DrvCfgRxChan ( pRxChanCfg->Speed, pRxChanCfg->Parity,
                             pRxChanCfg->LabelSwapBits, pRxChanCfg->Enable,
                             Channel );

      // Loop through all possible labels and enable the hardware if the
      // SDI for that label is accept
      for ( Label = 0; Label < ARINC429_MAX_LABELS; Label++ )
      {
         pLabelData = &m_Arinc429RxChannel[Channel].LabelData[Label];
         SdiMask    = 0;

         for ( sdi = 0; sdi < ARINC429_MAX_SD; sdi++ )
         {
            SdiMask |= pLabelData->sd[sdi].Accept ? 0x1 << sdi : 0;
         }

         // Initialize the hardware to accept the configured label sdi combinations
         Arinc429DrvCfgLFR ( SdiMask, Channel, (UINT8)Label, FALSE );
      }

      // Set the channel status to the configured enable state
      m_Arinc429MgrBlock.Rx[Channel].Enabled          = pRxChanCfg->Enable;
      m_Arinc429MgrBlock.Rx[Channel].bChanActive      = FALSE;
      m_Arinc429MgrBlock.Rx[Channel].LastActivityTime = 0;
   }

   // Loop through each of the Transmit channels and initialize the hardware
   // based on the configuration
   for ( Channel = 0; Channel < FPGA_MAX_TX_CHAN; Channel++ )
   {
      pTxChanCfg = &m_Arinc429Cfg.TxChan[Channel];
      Arinc429DrvCfgTxChan ( pTxChanCfg->Speed, pTxChanCfg->Parity,
                             pTxChanCfg->LabelSwapBits, pTxChanCfg->Enable,
                             Channel );
   }

   // Reset the interrupt count for this channel
   m_Arinc429MgrBlock.InterruptCnt = 0;
}

/******************************************************************************
 * Function:     Arinc429MgrCfgReduction
 *
 * Description:  Sets the protocol conversion and reduction configuration
 *               items needed for runtime execution.
 *
 * Parameters:   pRxChanCfg - pointer to the Channel Configuration
 *               pChanData  - pointer to the Channel Data Storage
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static void Arinc429MgrCfgReduction ( ARINC429_RX_CFG_PTR pRxChanCfg,
                                      ARINC429_CHAN_DATA_PTR pChanData )
{
   // Local Data
   ARINC429_WORD_INFO_PTR  pWordInfo;
   ARINC429_LABEL_DATA_PTR pLabelData;
   ARINC429_SDI_DATA_PTR   pSdi;
   UINT16                  Index;
   UINT8                   sdi;
   UINT8                   mfd_label;

   // Loop through all possible configured parameters
   for ( Index = 0; Index < ARINC429_MAX_PARAMS; Index++ )
   {
      // Check if parameter configuration is being used
      if ( ( 0 != pRxChanCfg->Parameter[Index].Label ) &&
           ( 0 != pRxChanCfg->Parameter[Index].GPA) )
      {
         // Initialize all relevant pointers
         pWordInfo         = &pChanData->ReduceWordInfo[pChanData->ReduceParamCount];
         pLabelData        = &pChanData->LabelData[pRxChanCfg->Parameter[Index].Label];
         pLabelData->label = pRxChanCfg->Parameter[Index].Label;
         pWordInfo->Label  = pRxChanCfg->Parameter[Index].Label;

         // Parse the General Purpose A and B fields and populate the WordInfo
         Arinc429MgrParseGPA_GPB ( pRxChanCfg->Parameter[Index].GPA,
                                   pRxChanCfg->Parameter[Index].GPB,
                                   pWordInfo );

         // Store the Word Info to the Reduction Word Info Table
         pChanData->ReduceWordInfo[pChanData->ReduceParamCount] = *pWordInfo;

         // Check if configured for SDI Ignore
         if ( TRUE == pWordInfo->IgnoreSDI )
         {
            // Parameter is SDI ignore configure all parameters to be accepted
            for ( sdi = 0; sdi < SDI_MAX; sdi++)
            {
               pSdi          = &pLabelData->sd[sdi];
               pSdi->Accept  = TRUE;
               // Store the parameter configuration for the label SDI combination
               pSdi->ProtocolStorage.ConversionData.WordInfoIndex =
                                                     pChanData->ReduceParamCount;
               pSdi->ProtocolStorage.ConversionData.Scale = pRxChanCfg->Parameter[Index].Scale;                                              

               if (BNR == pWordInfo->Format)
               {
                  pSdi->ProtocolStorage.ConversionData.Scale /= 
                                                       (FLOAT32)(1 << pWordInfo->WordSize);
               }
               else if ( DISCRETE == pWordInfo->Format)
               {
                  pSdi->ProtocolStorage.ConversionData.Scale = 1.0f;
               }
               pSdi->ProtocolStorage.ReduceData.Tol  = pRxChanCfg->Parameter[Index].Tolerance;
               pSdi->ProtocolStorage.bIgnoreSDI      = TRUE;
               pSdi->ProtocolStorage.bReduce         = TRUE;
               pSdi->ProtocolStorage.bFailedOnce     = FALSE;
               pSdi->ProtocolStorage.TotalTests      = END_OF_FAIL_COUNT;
            }
         }
         else // Allow specified "SDI" to be accepted
         {
            pSdi          = &pLabelData->sd[pWordInfo->SDBits];
            pSdi->Accept  = TRUE;
            // Store the parameter configuration for the label SDI combination
            pSdi->ProtocolStorage.ConversionData.WordInfoIndex = pChanData->ReduceParamCount;
            pSdi->ProtocolStorage.ConversionData.Scale = pRxChanCfg->Parameter[Index].Scale;                                                 
            if (BNR == pWordInfo->Format)
            {
               pSdi->ProtocolStorage.ConversionData.Scale /= 
                                                     (FLOAT32)(1 << pWordInfo->WordSize);
            }
            else if ( DISCRETE == pWordInfo->Format)
            {
               pSdi->ProtocolStorage.ConversionData.Scale = 1.0f;
            }
            pSdi->ProtocolStorage.ReduceData.Tol   = pRxChanCfg->Parameter[Index].Tolerance;
            pSdi->ProtocolStorage.bReduce          = TRUE;
            pSdi->ProtocolStorage.bFailedOnce      = FALSE;
            pSdi->ProtocolStorage.TotalTests       = END_OF_FAIL_COUNT;
         }
         // Count the number of reduction parameters found
         pChanData->ReduceParamCount++;
      }
   }

   // Check for MFD sub Protocol - This Protocol is specific to PW305 A and B EECs
   if ( ARINC429_RX_SUB_PW305AB_MFD_REDUCE == pRxChanCfg->SubProtocol )
   {
      // Loop through all the possible MFD Labels
      for ( mfd_label = MFD_300; mfd_label <= MFD_306; mfd_label++ )
      {
         pLabelData        = &pChanData->LabelData[mfd_label];
         pLabelData->label = mfd_label;

         // Set all SDI Combinations as accept - the processing will only use IGNORE
         for ( sdi = 0; sdi < SDI_MAX; sdi++)
         {
            pSdi                             = &pLabelData->sd[sdi];
            pSdi->Accept                     = TRUE;
            pSdi->ProtocolStorage.bIgnoreSDI = TRUE;
         }
      }
   }
}

/*****************************************************************************/
/* COMMON FUNCTIONS                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:     Arinc429MgrParseGPA_GPB
 *
 * Description:  This function is a utility to parse the GPA and GPB fields
 *               used for both sensor and parameter configurations.
 *
 * Parameters:   GPA - General Purpose A (See below for definition)
 *               GPB - General Purpose B (See below for definition)
 *               pWordInfo - Location to store the parsed GPA and GPB.
 *
 * Returns:      None.
 *
 * Notes:
 *
 *  gpA
 *  ===
 *    bits  0 to  1:  Rx Channel
 *    bits  2 to  3:  Word Format  (BNR, BCD, DISC, Other)
 *    bits  4      :  Not Used
 *    bits  5 to  9:  Word Size
 *    bits 10 to 11:  SDI definition
 *    bits 12      :  Ignore SDI
 *    bits 13 to 17:  Word Position (within 32 bit word, Bits 0 to 31)
 *                    NOTE: Not including sign bit, which is always Bit 28 (cnt from bit 0).
 *    bits 18 to 19:  Packed DISC type
 *    bits 20 to 23:  SSM Filter ( 0 = Normal SSM Processing, based on Word Format )
 *    bits 24      :  SDI All Call Mode
 *
 *  gpB
 *  ===
 *    bits 0 to 15: Data Loss TimeOut Value (in msec)
 *    bits 16 to 31: Not Used
 *****************************************************************************/
static void Arinc429MgrParseGPA_GPB ( UINT32 GPA, UINT32 GPB,
                                      ARINC429_WORD_INFO_PTR pWordInfo )
{
   // Parse General Purpose A parameter
   pWordInfo->RxChan     = (ARINC429_CHAN_ENUM)GPA_RX_CHAN(GPA);
   pWordInfo->Format     = (ARINC_FORM)        GPA_WORD_FORMAT(GPA);
   pWordInfo->WordSize   = GPA_WORD_SIZE(GPA);
   pWordInfo->SDBits     = GPA_SDI_DEFINITION(GPA);
   pWordInfo->IgnoreSDI  = (GPA_IGNORE_SDI(GPA) == TRUE) ? TRUE : FALSE;
   pWordInfo->WordPos    = GPA_WORD_POSITION(GPA);
   pWordInfo->DiscType   = (DISC_TYPE)GPA_PACKED_DISCRETE(GPA);
   pWordInfo->ValidSSM   = GPA_SSM_FILTER(GPA);
   pWordInfo->SDIAllCall = (GPA_ALL_CALL(GPA) == TRUE) ? TRUE : FALSE;
   // Parse General Purpose B parameter from 1 to 2^32 where lsb = 1 sec
   pWordInfo->DataLossTime = GPB;

   if (pWordInfo->ValidSSM == 0)
   {
      // Store the Default Valid SSM
      switch (pWordInfo->Format)
      {
         default:
         case BNR:
            pWordInfo->ValidSSM = BNR_VALID_SSM;
            break;
         case DISCRETE:
            switch (pWordInfo->DiscType)
            {
               case DISC_BNR:
                  pWordInfo->ValidSSM = BNR_VALID_SSM;
                  break;
               case DISC_BCD:
                  pWordInfo->ValidSSM = BCD_VALID_SSM;
                  break;
              case DISC_STANDARD:
              default:
                 pWordInfo->ValidSSM = DISC_VALID_SSM;
                 break;
            }
            break;
         case BCD:
            pWordInfo->ValidSSM = BCD_VALID_SSM;
            break;
      }
   }
}

/*****************************************************************************
 * Function:     Arinc429MgrParseMsg
 *
 * Description:  Parses ARINC429 BNR, BCD and Discrete Messages.
 *
 * Parameters:   Arinc429Msg - The message to parse
 *               Format      - Format of the message (BNR, BCD, Discrete)
 *               Position    - Bit position where the data starts (Starting from 0)
 *               Size        - Number of bits in the data field
 *               Chan        - Arinc Channel Number
 *               pWordValid  - [in/out] conversion was valid flag
 *
 * Returns:      SINT32 Value - Value of the Decoded message
 *
 * Notes:
 *               1. Sign Bit is Bit 28 (cnt from Bit 0)
 *               2. For standard BNR, MSD always starts at Bit 27.
 *               3. For disc embedded within BNR, the MSD of BNR word always start at Bit 27
 *
 ****************************************************************************/
static SINT32 Arinc429MgrParseMsg (UINT32 Arinc429Msg, ARINC_FORM Format,
                                   UINT8  Position,    UINT8 Size,
                                   UINT32 Chan,        BOOLEAN *pWordValid )
{
   // Local Data
   SINT32 svalue = 0;
   UINT32 nvalue;
   UINT32 raw;
   UINT32 mask;

   raw = Arinc429Msg & (~ARINC_MSG_LABEL_BITS);  // Remove octal label from word !

   // Parse raw value based on cfg arinc format before returning
   switch ( Format )
   {
      case BNR:
         // ARINC429 Standard defines an ARINC message as bits 32 - 1
         // BNR Sign bit is 29
         // BNR Position always starts at 28
         // BNR Data is 2's complement
         // BNR Size is from position 28 to LSBit of data
         // The +1 is added to include the sign bit so the sign can be extended
         // Note: Word Position is configured to start at 0
         mask   = (1U     << Size);                  // This algorithm would be Size - 1
                                                     // but we need to included the sign bit
         nvalue = (raw    << (31 - (Position + 1))); // Lob off unwanted MSB's
         nvalue = (nvalue >> (32 - (Size     + 1))); // Shift away unwanted LSB's
         nvalue = nvalue & ((1U << (Size + 1)) - 1);
         svalue = (SINT32)((nvalue ^ mask) - mask);

         *pWordValid = TRUE;
         break;

      case BCD:
         // If decode fails, bWordValid set to FALSE and return value is ->value_prev
         svalue = Arinc429MgrParseBCD( Arinc429Msg, Chan, Size, pWordValid );
         break;

      case DISCRETE:
         // Grab Word Pos - Shift and left justify data
         nvalue = (raw << (31 - Position));
         // Grab Word Size - Shift and right justify data and parse proper word size
         // NOTE: Verify that "sign" of SINT32 does not get signed extended !! In G3 it does.
         svalue = (SINT32) (nvalue >> (32 - Size));
         *pWordValid = TRUE;
         break;

         // All other types of ARINC_FORM are invalid.
      default:
         // OTHER
         FATAL("Arinc429 Read Failed Unsupported Format = %d", Format);
         break;
   } // End of switch ()

   return svalue;
}

/*****************************************************************************
 * Function: Arinc429MgrParseBCD
 *
 * Description: Parses ARINC word based on Filter and converts the proper
 *              number of BCD digits. Will report errors on any BCD digit > 9
 *              and Word size more than 5 digits.
 *
 * Parameters:  raw         - Full Arinc Data word
 *              Chan        - Arinc Channel Number
 *              Size        - Number of bits in the data field
 *              *pWordValid - Ptr to Word Decode Result Flag
 *
 * Returns:     Parsed Signed BCD Value
 *
 * Notes:
 *
 ****************************************************************************/
static
SINT32 Arinc429MgrParseBCD ( UINT32 raw, UINT32 Chan, UINT8 Size, BOOLEAN *pWordValid )
{
   // Local Data
   SINT32  sValue = 0;
   UINT8   msb;
   UINT32  bcdData;
   UINT8   digit;
   UINT8   i;
   ARINC429_SYS_BCD_FAIL_LOG sysBcdFailLog;

   *pWordValid = TRUE;

   // Verify no more than 5 digits assigned
   if ( Size <= ARINC_BCD_DIGITS )
   {
      // Store Arinc Word right justified
      bcdData = (raw & ARINC_BCD_MASK) >> ARINC_BCD_SHIFT;

      // Loop through all defined digits
      for ( i = 0; i < Size; i++ )
      {
         // Extract digits from MSB to LSB
         digit  = (UINT8)(bcdData >> (16 - (i*4))) & 0xF;

         // If digit received is greater than 9 then it is not a valid BCD digit
         if ( digit > 9 )
         {
           // Only log invalid BCD digit once per Aring429 label
           UINT32 label = raw & ARINC_MSG_LABEL_BITS;     // get ARINC429 label
           ASSERT(Chan < ARINC_CHAN_MAX);
           if (!m_Arinc429BCDInvDigit[label][Chan])
           {
              // Log Out-Of-Range Data
              sysBcdFailLog.a429word = raw;
              sysBcdFailLog.bcdData  = bcdData;
              sysBcdFailLog.digit    = digit;

              LogWriteSystem( SYS_ID_A429_BCD_DIGITS, LOG_PRIORITY_LOW,
                              &sysBcdFailLog,
                              sizeof(ARINC429_SYS_BCD_FAIL_LOG), NULL );
              m_Arinc429BCDInvDigit[label][Chan] = TRUE;
           }
#ifdef ARINC429_DEBUG_COMPILE
            /*vcast_dont_instrument_start*/
            // Debug testing
            snprintf ( GSE_OutLine, sizeof(GSE_OutLine),
            "Arinc429_ParseBCD: Word-0x%08x Data-0x%05x Digit-%d",
                      raw,
            bcdData,
            digit);
            GSE_DebugStr(NORMAL,TRUE,GSE_OutLine);
            /*vcast_dont_instrument_end*/
#endif
           // Return previous good value
           *pWordValid = FALSE;
           break;
         }
         else
         {
            // Calculate word total
            sValue = (sValue * 10) + digit;
         }
      } // End for loop thru Word chars
   } // End of if Word chars defined <= ARINC_BCD_DIGITS
   else // Too many BCD digits
   {
      *pWordValid = FALSE;
   }

   if ( TRUE == *pWordValid )
   {
      // Check if value is negative
      msb = (UINT8)(raw >> ARINC_MSG_SHIFT_MSB);
      if ( (msb & ARINC_SSM_BITS) ==  ARINC_BCD_SSM_NEGATIVE_v)
      {
         // Negative, Minus, South, West, Left, From, Below Number
         sValue = sValue * -1;
      }
   }
  return (sValue);
} // end of ParseArinc429_BCD

/*****************************************************************************
 * Function:     Arinc429MgrReadBuffer
 *
 * Description:  Reads data from passed in buffer.
 *
 * Parameters:   pDest        - Location to store the data
 *               nMaxByteSize - Total bytes that can be stored
 *               pBuf         - Buffer to retrieve the data from
 * Returns:      UINT16       - Total Records received
 *
 * Notes:        None
 *
 ****************************************************************************/
static
UINT16 Arinc429MgrReadBuffer ( void *pDest, UINT16 nMaxByteSize,
                               ARINC429_RAW_RX_BUFFER_PTR pBuf )
{
   // Local Data
   UINT16  Cnt;
   UINT16  Records;

   Cnt = 0;

   // Check if all the records store will fit in the destination
   if ( ( pBuf->RecordCnt * pBuf->RecordSize ) < nMaxByteSize )
   {
      // Pop all the records to the destination
      FIFO_PopBlock(&pBuf->RecordFIFO, pDest, ( pBuf->RecordCnt * pBuf->RecordSize ));
      Records         = pBuf->RecordCnt;
      pBuf->RecordCnt = 0;
   }
   else // All the records won't fit - Pop what will fit
   {
      Records = nMaxByteSize / pBuf->RecordSize;
      Cnt     = Records * pBuf->RecordSize;

      FIFO_PopBlock(&pBuf->RecordFIFO, (UINT8*)pDest, Cnt);
      pBuf->RecordCnt = pBuf->RecordCnt - Records;
   }

   // Return the number of records popped
   return ( Records );
}

/*****************************************************************************/
/* MESSAGE PROCESSING                                                        */
/*****************************************************************************/
/******************************************************************************
 * Function:     Arinc429MgrStoreLabelData
 *
 * Description:  Takes the raw Arinc429 messages received and stores them
 *               to the Label Data Table.
 *
 * Parameters:   Arinc429Msg - Raw Message
 *               pChanData   - Location of storage
 *               psdi        - [in/out] sdi value
 *               TickCount   - Time in milliseconds from powerup when rcvd
 *               TimeStamp   - Time in Date and Time when rcvd
 *
 * Returns:      ARINC429_LABEL_DATA_PTR - Pointer to the label data
 *
 * Notes:        None
 *
 *****************************************************************************/
static ARINC429_LABEL_DATA_PTR Arinc429MgrStoreLabelData ( UINT32 Arinc429Msg,
                                                           ARINC429_CHAN_DATA_PTR  pChanData,
                                                           ARINC_SDI *psdi,
                                                           UINT32 TickCount,
                                                           TIMESTAMP TimeStamp )
{
   // Local Data
   UINT8                   label;
   ARINC429_SDI_DATA_PTR   pSdiData;
   ARINC429_LABEL_DATA_PTR pLabelData;

   // NOTE: Arinc Octal Label either swapped or not swapped by FPGA.
   //       Normally on Bus LSB = bit 8 and MSB = bit 0, by setting
   //       arinc_rx[].swaplabelbits to TRUE will force FPGA
   //       to swap to MSB = 8 and LSB = 0 before FPGA processes the label.
   //       Once swapped the FPGA will save the "swapped" value to Rx FIFO.
   label     = (UINT8)    ( ARINC_MSG_LABEL_BITS & Arinc429Msg );
   *psdi     = (ARINC_SDI)( ( ARINC_MSG_SDI_BITS & Arinc429Msg ) >> ARINC_MSG_SHIFT_SDI);

   pLabelData = &pChanData->LabelData[label];
   pSdiData   = &pLabelData->sd[*psdi];

   // Copy Current Data to previous before over writing
   pSdiData->PreviousMsg = pSdiData->CurrentMsg;
   pSdiData->CurrentMsg  = Arinc429Msg;
   pSdiData->RxTime      = TickCount;
   // TODO: From the GHS Trace analysis this seems to be a slow point
   //       Determine if a memcpy would be faster
   pSdiData->RxTimeStamp = TimeStamp;
   pSdiData->RxCnt++;
   // Save / echo this new data to "general" sd location
   pLabelData->sd[SDI_IGNORE].PreviousMsg    = pLabelData->sd[SDI_IGNORE].CurrentMsg;
   pLabelData->sd[SDI_IGNORE].CurrentMsg     = Arinc429Msg;
   pLabelData->sd[SDI_IGNORE].RxTime         = TickCount;
   // TODO: Determine if a memcpy would be faster
   pLabelData->sd[SDI_IGNORE].RxTimeStamp    = TimeStamp;
   pLabelData->sd[SDI_IGNORE].RxCnt++;

   return pLabelData;
}

/******************************************************************************
 * Function:     Arinc429MgrApplyProtocol
 *
 * Description:  Applies the protocol configured for the channel.
 *
 * Parameters:   pCfg        - Channel configuration pointer
 *               pChanData   - Channel Data Storage location
 *               pLabelData  - Label Data location
 *               sdi         - source destination index
 *               pDataRecord - [in/out] Record to save if any
 *
 * Returns:      BOOLEAN     - TRUE  - Save Record
 *                             FALSE - Don't Save Record
 *
 * Notes:        None.
 *
 *****************************************************************************/
static BOOLEAN Arinc429MgrApplyProtocol( ARINC429_RX_CFG_PTR pCfg,
                                         ARINC429_CHAN_DATA_PTR pChanData,
                                         ARINC429_LABEL_DATA_PTR pLabelData, ARINC_SDI sdi,
                                         ARINC429_RECORD_PTR pDataRecord )
{
   // Local Data
   BOOLEAN               bSaveRecord;
   ARINC429_SDI_DATA_PTR pSdiData;

   bSaveRecord = FALSE;
   pSdiData    = &pLabelData->sd[sdi];

   switch (pCfg->Protocol)
   {
      case ARINC429_RX_PROTOCOL_STREAM:
         pDataRecord->ARINC429Msg   = pSdiData->CurrentMsg;
         bSaveRecord                = TRUE;
         break;
      case ARINC429_RX_PROTOCOL_CHANGE_ONLY:
         if (pSdiData->CurrentMsg != pSdiData->PreviousMsg)
         {
            pDataRecord->ARINC429Msg = pSdiData->CurrentMsg;
            bSaveRecord              = TRUE;
         }
         break;
      case ARINC429_RX_PROTOCOL_REDUCE:
         bSaveRecord = Arinc429MgrDataReduction(pCfg, pChanData, pLabelData, sdi, pDataRecord);
         break;
      default:
         FATAL ("Unexpected ARINC429_RX_PROTOCOL = % d", pCfg->Protocol);
         break;
   }

   return bSaveRecord;
}

/******************************************************************************
 * Function:     Arinc429MgrDataReduction
 *
 * Description:  This function applies the data reduction logic. It manages
 *               converting the Arinc429 messages to and from engineering
 *               values and calls the data reduction functions. It monitors
 *               the validity of the messages and initiated the MFD processing
 *               when necessary.
 *
 * Parameters:   pCfg        - Channel configuration pointer
 *               pChanData   - Channel Data Storage location
 *               pLabelData  - Label Data location
 *               sdi         - source destination index
 *               pDataRecord - [in/out] Record to save if any
 *
 * Returns:      BOOLEAN     - TRUE  - Save Record
 *                             FALSE - Don't Save Record
 *
 * Notes:        None.
 *
 *****************************************************************************/
static BOOLEAN Arinc429MgrDataReduction( ARINC429_RX_CFG_PTR pCfg,
                                         ARINC429_CHAN_DATA_PTR pChanData,
                                         ARINC429_LABEL_DATA_PTR pLabelData, ARINC_SDI sdi,
                                         ARINC429_RECORD_PTR pDataRecord )
{
   // Local Data
   BOOLEAN                        bSave;
   BOOLEAN                        bWordValid;
   SINT32                         sValue;
   ARINC429_REDUCE_PROTOCOL_PTR   pProtocol;
   ARINC429_CONV_DATA_PTR         pConvData;
   ARINC429_SDI_DATA_PTR          pSdiData;
   ARINC429_WORD_INFO_PTR         pWordInfo;
   REDUCE_STATUS                  status;

   bSave     = FALSE;

   // Determine if the data to processes is dependent on an SDI or
   // if it's SDI ignore.
   pSdiData  = ( pLabelData->sd[SDI_IGNORE].ProtocolStorage.bIgnoreSDI == TRUE ) ?
                                           &pLabelData->sd[SDI_IGNORE] : &pLabelData->sd[sdi];
   pProtocol = &pSdiData->ProtocolStorage;
   pConvData = &pProtocol->ConversionData;
   pWordInfo = &pChanData->ReduceWordInfo[pConvData->WordInfoIndex];

   // Check if special processing
   if ( (pCfg->SubProtocol == ARINC429_RX_SUB_PW305AB_MFD_REDUCE) &&
        ((pLabelData->label >= (UINT8)MFD_300) && (pLabelData->label <= (UINT8)MFD_306)) )
   {
      if ( Arinc429MgrProcPW305ABEngineMFD ( &pChanData->MFDTable,
                                             &pSdiData->CurrentMsg,
                                             pLabelData->label ) )
      {
         bSave = TRUE;
         pDataRecord->ARINC429Msg = pLabelData->sd[SDI_IGNORE].CurrentMsg;
      }
   }
   else // Normal Arinc429 reduction processing
   {
      if ( TRUE == pProtocol->bReduce )
      {
         if ( Arinc429MgrParameterTest (pSdiData, pWordInfo))
         {
            pProtocol->bValid = TRUE;

            // Convert Message to EngValue
            sValue = Arinc429MgrParseMsg ( pSdiData->CurrentMsg, pWordInfo->Format,
                                           pWordInfo->WordPos,   pWordInfo->WordSize,
                                           (UINT32)pWordInfo->RxChan,    &bWordValid );

            pProtocol->ReduceData.Time   = pSdiData->RxTime;

            if ( ( FALSE == bWordValid ) && ( BCD == pWordInfo->Format ) )
            {
              pProtocol->bValid = FALSE;
              // Get the current value of the parameter
              DataReductionGetCurrentValue(&pProtocol->ReduceData);
              status = RS_KEEP;
            }
            else
            {
              pProtocol->ReduceData.EngVal = sValue * pConvData->Scale;

              // Check if the Data is Initialized
              if ( FALSE == pProtocol->bInitialized )
              {
                 status = RS_KEEP;
                 DataReductionInit(&pProtocol->ReduceData);
                 pProtocol->bInitialized = TRUE;
              }
              else
              {
                 status = DataReduction (&pProtocol->ReduceData);
              }
            }

            // Check if the value should be kept
            if (status == RS_KEEP)
            {
               // Convert the value back to an ARINC429 Message
               bSave   = TRUE;

               pProtocol->Reduced429Msg =
                 Arinc429MgrConvertEngToMsg ( pProtocol->ReduceData.EngVal,
                                              pConvData->Scale,
                                              pWordInfo->WordSize,
                                              pWordInfo->WordPos,
                                              (UINT32)pWordInfo->Format,
                                              pSdiData->CurrentMsg,
                                              pProtocol->bValid );
               // Store it in the Data Record
               pDataRecord->ARINC429Msg = pProtocol->Reduced429Msg;
            }
         }
         else // Not Valid -- Check the Timeout
         {
            // Determine if we exceeded the Data Loss Time Out value for an SSM Fail
            if ( (CM_GetTickCount() - pProtocol->TimeSinceLastGoodValue) >
                  pWordInfo->DataLossTime )
            {
               // Data Loss Time Out Exceeded
               bSave = Arinc429MgrInvalidateParameter ( pSdiData, pWordInfo, pDataRecord );

               if ( FALSE == pProtocol->bFailedOnce )
               {
#ifdef ARINC429_DEBUG_COMPILE
                 /*vcast_dont_instrument_start*/
                  ARINC429_SYS_CBIT_SSM_FAIL_LOG SSMFailLog;
                  CHAR                           TempStr[32];

                  Arinc429MgrDetermineSSMFailure( pProtocol->FailCount,
                                                  pProtocol->TotalTests,
                                                  SSMFailLog.FailMsg );

                  snprintf(TempStr, sizeof(TempStr),
                           " Ch = %d L = %o"NL, pWordInfo->RxChan, pWordInfo->Label);
                  strcat(SSMFailLog.FailMsg, TempStr);

                  // Debug testing
                  snprintf (GSE_OutLine, sizeof(GSE_OutLine),
                            "Arinc429_ParameterTest: %s",
                           SSMFailLog.FailMsg);
                  GSE_DebugStr(NORMAL,TRUE,GSE_OutLine);
                  /*vcast_dont_instrument_end*/
#endif
                  pProtocol->bFailedOnce = TRUE;
               }
            }
         }
      }
   }

   return bSave;
}

/******************************************************************************
 * Function:    Arinc429MgrConvertEngToMsg
 *
 * Description: This function converts an Engineering Value into an ARINC429
 *              Message.
 *
 * Parameters:  EngVal      - Engineer Value to be converted into the Data field
 *              Scale       - Scale of the Engineering Value
 *              WordSize    - How many bits the Data field consumes
 *              Position    - Position for building new message
 *              Format      - Format of message (BNR/BCD)
 *              Arinc429Msg - Message to store the value in
 *              bValid      - Validity of the message
 *
 * Returns:     UINT32      - Converted ARINC429 Message
 *
 * Notes:       none
 *
 *****************************************************************************/
static UINT32 Arinc429MgrConvertEngToMsg ( FLOAT32 EngVal, FLOAT32 Scale, UINT8 WordSize,
                                           UINT8 Position, UINT32 Format,
                                           UINT32 Arinc429Msg, BOOLEAN bValid )
{
    //Local Data
    FLOAT32 CalculatedValue;
    SINT32  ConvertedValue;
    UINT32  NewMessage;

    // Keep SDI & ARINC Label bits, clear all others
    NewMessage  = Arinc429Msg & (ARINC_MSG_SDI_BITS | ARINC_MSG_LABEL_BITS);

    // Convert the engineering value back to bit weighted value
    CalculatedValue = EngVal / Scale;
    // Round to the next value if necessary
    if (CalculatedValue < 0)
    {
      CalculatedValue -= 0.5f;
    }
    else
    {
      CalculatedValue += 0.5f;
    }
    ConvertedValue = (SINT32)CalculatedValue;           // convert float to signed int

    if (BCD == Format)
    {
      UINT32  i;
      UINT32  offset = 0;
      UINT32  bcdValue = 0;
      UINT32  numDigits;
      BOOLEAN bNegValue = FALSE;

      if (ConvertedValue < 0)
      {
        bNegValue = TRUE;
        ConvertedValue = -ConvertedValue;   // make value positive
      }

      numDigits  = (WordSize < ARINC_BCD_DIGITS) ? WordSize : ARINC_BCD_DIGITS;
      for (i = 0; i < numDigits; ++i)
      {
        UINT32 digit = (UINT32)ConvertedValue % 10;   // get digit
        bcdValue |= digit << offset;                  // add to word
        offset += 4;
        ConvertedValue /= 10;
      }
      NewMessage |= (bcdValue << ARINC_MSG_SHIFT_DATA) & ARINC_BCD_MASK;
      NewMessage |= bNegValue ? (ARINC_BCD_SSM_NEGATIVE_v << ARINC_MSG_SHIFT_MSB) :
                                (ARINC_BCD_SSM_POSTIVE_v  << ARINC_MSG_SHIFT_MSB);
    }
    else
    {
      // Store the Calculated value back into the Arinc429 Message
      // +1 is added to make up for the fact that the configuration is based on bits 31-0
      NewMessage |= ( ARINC_MSG_SIGN_DATA_BITS &
                      ((UINT32)CalculatedValue << ((Position - WordSize)+1)));
      // Add back SSM bits
      NewMessage |= Arinc429Msg & ARINC_MSG_SSM_BITS;

    }

    // Set the validity flag if necessary
    NewMessage |= (TRUE == bValid) ? ARINC_MSG_VALID_BIT : 0;

    return (NewMessage);
}

/******************************************************************************
 * Function:    Arinc429MgrChkParamLostTimeout
 *
 * Description: The Check Parameter Lost Timeout Function monitors all the
 *              parameters that are configured for data reduction and checks
 *              for timeouts. When a parameter times out the function will
 *              store the last known good point.
 *
 * Parameters:  pCfg      - Configuration for this channel
 *              pChanData - Storage location for all channel data
 *              pRxBuffer - Location to store any records
 *
 * Returns:     none
 *
 * Notes:       none
 *
 *****************************************************************************/
static void Arinc429MgrChkParamLostTimeout ( ARINC429_RX_CFG_PTR pCfg,
                                             ARINC429_CHAN_DATA_PTR pChanData,
                                             ARINC429_RAW_RX_BUFFER_PTR pRxBuffer )
{
   // Local Data
   ARINC429_RECORD                ARINC429Record;
   UINT16                         Index;
   ARINC429_REDUCE_PROTOCOL_PTR   pProtocol;
   ARINC429_LABEL_DATA_PTR        pLabelData;
   ARINC429_SDI_DATA_PTR          pSdiData;
   ARINC429_WORD_INFO_PTR         pWordInfo;

   // Check if channel is configured for reduction
   if ( ARINC429_RX_PROTOCOL_REDUCE == pCfg->Protocol )
   {
      // Loop through all entries in the Reduce Word Info table
      for ( Index = 0; Index < pChanData->ReduceParamCount; Index++ )
      {
         pWordInfo  = &pChanData->ReduceWordInfo[Index];
         pLabelData = &pChanData->LabelData[pWordInfo->Label];
         pSdiData   = ( pLabelData->sd[SDI_IGNORE].ProtocolStorage.bIgnoreSDI == TRUE ) ?
                        &pLabelData->sd[SDI_IGNORE] : &pLabelData->sd[pWordInfo->SDBits];
         pProtocol  = &pSdiData->ProtocolStorage;

         // Check Configured Parameter for timeout
         if ((CM_GetTickCount() - pProtocol->TimeSinceLastGoodValue) > pWordInfo->DataLossTime)
         {
            // Call Invalidate Parameter and Store returned Record
            if ( Arinc429MgrInvalidateParameter ( pSdiData, pWordInfo, &ARINC429Record) )
            {
               // Check if Label is to be or not to be filtered OUT.
               if ( TRUE == pSdiData->Accept )
               {
                  ARINC429Record.DeltaTime = pChanData->TimeSyncDelta;
                  Arinc429MgrWriteBuffer ( pRxBuffer,
                       &ARINC429Record,
                       sizeof(ARINC429Record) );
               }
            }

            // If this failure hasn't been reported yet - do it now
            if ( FALSE == pProtocol->bFailedOnce )
            {
#ifdef ARINC429_DEBUG_COMPILE
              /*vcast_dont_instrument_start*/
               ARINC429_SYS_CBIT_SSM_FAIL_LOG SSMFailLog;
               CHAR                           TempStr[32];

               Arinc429MgrDetermineSSMFailure( pProtocol->FailCount,
                                               pProtocol->TotalTests,
                                               SSMFailLog.FailMsg );

               snprintf(TempStr, sizeof(TempStr),
                        " Ch = %d L = %o"NL, pWordInfo->RxChan, pWordInfo->Label);
               strcat(SSMFailLog.FailMsg, TempStr);

               // Debug testing
               snprintf (GSE_OutLine, sizeof(GSE_OutLine),
                         "Arinc429_ParamTest: %s", SSMFailLog.FailMsg);
               GSE_DebugStr(NORMAL,TRUE,GSE_OutLine);
               /*vcast_dont_instrument_end*/
#endif
               pProtocol->bFailedOnce = TRUE;
            }
         }
      }
   }

   if ( ARINC429_RX_SUB_PW305AB_MFD_REDUCE == pCfg->SubProtocol)
   {
      Arinc429MgrChkPW305AB_MFDTimeout(pChanData, pRxBuffer);
   }
}

/******************************************************************************
 * Function:     Arinc429MgrChkPW305AB_MFDTimeout
 *
 * Description:  Checks the absence or timeout condition of MFD word
 *
 * Parameters:   pChanData - channel to process
 *               pRxBuffer - data buffer to return timeout flag value
 *
 * Returns:      none
 *
 * Notes:        none
 *
 *****************************************************************************/
static void Arinc429MgrChkPW305AB_MFDTimeout ( ARINC429_CHAN_DATA_PTR pChanData,
                                               ARINC429_RAW_RX_BUFFER_PTR pRxBuffer )
{
   // Local Data
   MFD_MSG             *pMFD_Msg;
   UINT16              nLine;
   UINT16              nMsg;
   UINT32              nArincMsg;
   ARINC429_RECORD     m_ARINC429Record;
   MFD_DATA_TABLE_PTR  pMFDTable;
   UINT32              nTickCount;

   pMFDTable = &pChanData->MFDTable;
   nTickCount = CM_GetTickCount();

   for ( nLine = 0; nLine < MFD_MAX_LINE; nLine++ )
   {
      if ( TRUE == pMFDTable->Line[nLine].bRcvdOnce )
      {
         for ( nMsg = 0; nMsg <= (UINT16)(MFD_306 - MFD_300); nMsg++ )
         {
            pMFD_Msg = &pMFDTable->Line[nLine].Msg[nMsg];

            if ( ( TRUE == pMFD_Msg->bRcvd ) &&
                 ((nTickCount - pMFD_Msg->RcvTime) > MFD_LABEL_TIMEOUT) )
            {
               pMFD_Msg->bRcvd = FALSE;

               // Return the last good message with Validity set to FALSE
               nArincMsg  = pMFD_Msg->Data;
               nArincMsg &= ~(0x1FC00000);
               nArincMsg |= (nLine << 22);
               nArincMsg  = ~(ARINC_MSG_VALID_BIT) & nArincMsg;

               m_ARINC429Record.ARINC429Msg = nArincMsg;

               m_ARINC429Record.DeltaTime = pChanData->TimeSyncDelta;
               Arinc429MgrWriteBuffer (pRxBuffer, &m_ARINC429Record, sizeof(m_ARINC429Record));
            }
         }
      }
   }
}

/******************************************************************************
 * Function:     Arinc429MgrInvalidateParameter
 *
 * Description:  When a parameter is determined to be invalid this function
 *               is called retrieve the current value and convert it to an
 *               ARINC-429 Message.
 *
 * Parameters:   pSdiData - ptr to sdi/label word to process
 *               pWordInfo - ptr to sdi/label word decode info
 *               pDataRecord - ptr to record to be returned
 *
 * Returns:     TRUE - Param invalid flag to be stored
 *
 * Notes:       none
 *
 *****************************************************************************/
static BOOLEAN Arinc429MgrInvalidateParameter ( ARINC429_SDI_DATA_PTR pSdiData,
                                                ARINC429_WORD_INFO_PTR pWordInfo,
                                                ARINC429_RECORD_PTR pDataRecord )
{
   // Local Data
   BOOLEAN                      bSave;
   ARINC429_REDUCE_PROTOCOL_PTR pProtocol;
   ARINC429_CONV_DATA_PTR       pConvData;

   // Initialize Local Data
   bSave     = FALSE;
   pProtocol = &pSdiData->ProtocolStorage;
   pConvData = &pProtocol->ConversionData;

   // If the parameter is currently valid
   if ( TRUE == pProtocol->bValid )
   {
      // Save the current value - Mark it not valid and not initialized
      bSave                   = TRUE;
      pProtocol->bInitialized = FALSE;
      pProtocol->bValid       = FALSE;

      // Call data reduction to get the latest valid engineering value
      DataReductionGetCurrentValue(&pProtocol->ReduceData);

      pProtocol->Reduced429Msg = Arinc429MgrConvertEngToMsg ( pProtocol->ReduceData.EngVal,
                                                              pConvData->Scale,
                                                              pWordInfo->WordSize,
                                                              pWordInfo->WordPos,
                                                              pWordInfo->Format,
                                                              pSdiData->CurrentMsg,
                                                              FALSE );
      // Store it in the Data Record
      pDataRecord->ARINC429Msg = pProtocol->Reduced429Msg;
   }
   return bSave;
}

/******************************************************************************
 * Function:     Arinc429MgrProcPW305ABEngineMFD
 *
 * Description:  The Process PW305 A B Engine MFD function is a special
 *               protocol processing to reduce the data being received for the
 *               MFD. This logic emulates the MFD in a table and only returns
 *               ARINC-429 message to keep when the Data on the specific line
 *               changes.
 *
 * Parameters:   pMFDTable - MFD Storage Table
 *               pArincMsg - [in/out] Message passed and converted if to be stored
 *               Label     - Label of the message
 *
 * Returns:      BOOLEAN   - TRUE  - Save message
 *                           FALSE - Don't Save message
 *
 * Notes:        The logic embeds the line number received in message 300 into
 *               messages 301 to 306.
 *
 *****************************************************************************/
static BOOLEAN Arinc429MgrProcPW305ABEngineMFD ( MFD_DATA_TABLE_PTR pMFDTable,
                                                 UINT32 *pArincMsg, UINT8 Label )
{
   // Local Data
   UINT32    TickCount;
   MFD_STATE NextState;
   BOOLEAN   bSave;
   MFD_MSG   *pMFD_Msg;

   // Initialize Local Data
   TickCount = CM_GetTickCount();
   NextState = MFD_START;
   bSave     = FALSE;

   // Check the state machine state
   if (MFD_START == pMFDTable->State)
   {
      // While in the start state wait for Label 300
      if ( MFD_300 == Label )
      {
         // Goto the next state, store the label, line and start time
         NextState              = MFD_PROCESS;
         pMFDTable->CurrentLine = (*pArincMsg & 0x003F8000) >> 15;
         pMFDTable->PrevLabel   = (MFD_LABEL)Label;
         pMFDTable->StartTime   = TickCount;
         pMFDTable->Line[pMFDTable->CurrentLine].Color = (MFD_COLOR)
                                                         ((*pArincMsg & 0x00000700) >> 8);
         pMFDTable->Line[pMFDTable->CurrentLine].bRcvdOnce = TRUE;
      }
   }
   else // MFD_PROCESS
   {
      // While in the process state verify that the line
      // labels are increasing by one each time.
      if ( Label == (pMFDTable->PrevLabel + 1) )
      {
         // Save the label
         pMFDTable->PrevLabel = (MFD_LABEL)Label;
         pMFD_Msg             = &pMFDTable->Line[pMFDTable->CurrentLine].Msg[Label%MFD_300];

         if ( (SSM_01 == ARINC_MSG_GET_SSM(*pArincMsg)) ||
              (SSM_11 == ARINC_MSG_GET_SSM(*pArincMsg)) )
         {
            // Save the time the label was receive for this line
            pMFD_Msg->RcvTime = TickCount;
            pMFD_Msg->bRcvd   = TRUE;

            // Check if the message for this line has changed
            if ( pMFD_Msg->Data != *pArincMsg )
            {
               // New message detected, store it in the table and embed the line number
               pMFD_Msg->Data = *pArincMsg;
               *pArincMsg &= ~(0x1FC00000);
               *pArincMsg |= (pMFDTable->CurrentLine << 22);
               *pArincMsg |= ARINC_MSG_VALID_BIT;
               bSave = TRUE;
            }
         }
         // else not needed because Bad SSM will be caught by
         // Arinc429MgrCheckPW305AB_MFD_Timeout

         // Labels incremented OK so stay in this state
         NextState = MFD_PROCESS;
      }

      // Check if the messages have been received in the proper time
      // OR if this is the last label OR if the label is not equal to previous
      if ( ((TickCount - pMFDTable->StartTime) > MFD_CYCLE_TIMEOUT ) ||
            ( MFD_306 == Label ) || (Label != pMFDTable->PrevLabel) )
      {
         // Return to the start state
         NextState = MFD_START;
      }
   }

   // Save the next state
   pMFDTable->State = NextState;

   return bSave;
}

/******************************************************************************
 * Function:     Arinc429MgrWriteBuffer
 *
 * Description:  Manages writing messages to the receive buffers.
 *
 * Parameters:   pBuffer - Buffer to store the data
 *               Data    - Location of the data
 *               Size    - number of bytes to copy from location
 *
 * Returns:      none
 *
 * Notes:        Used to write both the debug and receive buffers
 *
 *****************************************************************************/
static void Arinc429MgrWriteBuffer ( ARINC429_RAW_RX_BUFFER_PTR pBuffer,
                                     const void* Data, UINT16 Size )
{
   // Local Data
   INT8 OldestRecord[16];

   while ( FALSE == FIFO_PushBlock(&pBuffer->RecordFIFO, Data, Size))
   {
      pBuffer->bOverFlow = TRUE;
      FIFO_PopBlock (&pBuffer->RecordFIFO, OldestRecord, Size);
      pBuffer->RecordCnt--;
   }

   pBuffer->RecordCnt++;
}

/******************************************************************************
 * Function:     Arinc429MgrParameterTest
 *
 * Description:  Checks that the SSM value is correct for the parameter and
 *               determines if the Data Loss Time Out for the parameter
 *               has been exceeded.
 *
 * Parameters:   pSdiData  - Storage location of the Arinc Data
 *               pWordInfo - Decode Data for the Arinc Data
 *
 * Returns:      BOOLEAN   - TRUE if Test is OK
 *                           FALSE if Test has Failed
 *
 * Notes:        None.
 *
 *****************************************************************************/
static BOOLEAN Arinc429MgrParameterTest (ARINC429_SDI_DATA_PTR pSdiData,
                                         ARINC429_WORD_INFO_PTR pWordInfo )
{
   // Local Data
   UINT8   SSM;
   UINT8   SSM_bit;
   BOOLEAN bWordValid;
   ARINC429_REDUCE_PROTOCOL_PTR pProtocol;

   // Initialize Local Data
   bWordValid = TRUE;
   pProtocol  = &pSdiData->ProtocolStorage;

   // Grab the SSM bits
   SSM = (UINT8)((pSdiData->CurrentMsg & ARINC_MSG_SSM_BITS) >> ARINC_MSG_SHIFT_MSB);

   // Loop thru all SSM comparison checks
   // Warning: Be aware that the order of FailureCondition is directly
   //          related to the ValidSSM field of the ArincFilter. Any
   //          changes should be made with care.
   //          ValidSSM bit 0 = SSM 00 = FailureCondition[0]
   //          ValidSSM bit 1 = SSM 01 = FailureCondition[1]
   //          ValidSSM bit 2 = SSM 10 = FailureCondition[2]
   //          ValidSSM bit 3 = SSM 11 = FailureCondition[3]
   // NOTE: For normal SSM processing, in Arinc429_SensorSetup() ->ValidSSM "faked" to
   //   "mask" or not perform a specific failure test.  If defined for normal SSM
   //   processing the FailureCondition[SSM_NORMAL] == ARINC_BNR_SSM_NORM_v test would
   //   be "bypassed" !
   for (SSM_bit = 0; ((SSM_bit < pProtocol->TotalTests) && (bWordValid == TRUE)); SSM_bit++)
   {
      if ( ((pWordInfo->ValidSSM & (1 << SSM_bit)) == 0) &&
           ((SSM & ARINC_SSM_BITS) == FailureCondition[SSM_bit]) )
      {
         bWordValid = FALSE;
         pProtocol->FailCount[SSM_bit]++; // FailCount only gets update if we have
                                          //    updated value
      }
      else
      {
         pProtocol->FailCount[SSM_bit] = 0;
      }
   }

   if ( TRUE == bWordValid )
   {
      // We have good value, update TimeSinceLastGoodValue param
      pProtocol->TimeSinceLastGoodValue = pSdiData->RxTime;
      pProtocol->LastGoodMsg            = pSdiData->CurrentMsg;
   }

   return (bWordValid);
}

/*****************************************************************************/
/* SENSOR PROCESSING                                                         */
/*****************************************************************************/

/******************************************************************************
 * Function:     Arinc429MgrUpdateLabelFilter
 *
 * Description:  Updates m_ArincLabelInfo and FPGA Shadow RAM LFR from Arinc429
 *               Sensor Word definition
 *
 * Parameters:   pSensorInfo - Storage location of sensor data
 *
 * Returns:      none
 *
 * Notes:
 *  1) Updates FPGA_GetShadowRam LFR directly and m_ArincLabelInfo[].
 *  2) If Arinc429_Reconfigure() is called, the FPGA Shadow LBR register and
 *     m_ArincLabelInfo[] is cleared.  The m_ArincLabelInfo[] will initially
 *     be configured with the Arinc429Cfg settings.
 *     The updates to m_ArincLabelInfo[] from this func will be lost until
 *     this func is called again.
 *     Mitigation: Store Sensor Filter settings into m_ArincSensorLabelFilter[]
 *                 and when Arinc429_Reconfigure() called, update m_ArincLabelInfo[]
 *                 with info from m_ArincSensorLabelFilter[] as well.
 *  3) Directly updates the FPGA LBR.  This avoid having to call FPGA_Configure()
 *     to copy updates in the FPGA Shadow RAM into the FPGA Regs.
 *
 *****************************************************************************/
static void Arinc429MgrUpdateLabelFilter ( ARINC429_SENSOR_INFO_PTR pSensorInfo )
{
   // Local Data
   ARINC429_LABEL_DATA_PTR          pLabelData;
   ARINC429_WORD_INFO_PTR           pWordInfo;
   ARINC429_SENSOR_LABEL_FILTER_PTR pSensorData;
   UINT8                            sdi;
   UINT16                           SdiMask;

   pWordInfo  = &pSensorInfo->WordInfo;
   pLabelData = &m_Arinc429RxChannel[pWordInfo->RxChan].LabelData[pWordInfo->Label];

   // Update the Sensor Copy of the Label Filter Request.
   // Note: When Arinc429_ReconfigureFPGAShadowRam() is called, the LBR ShadowRAM
   //       is cleared and then updated with data in Arinc429.cfg.LBR and
   //       from this m_ArincSensorLabelFilter[][].
   pSensorData = &m_ArincSensorFilterData[pWordInfo->RxChan][pWordInfo->Label];

   if ( TRUE == pWordInfo->IgnoreSDI )
   {
      for ( sdi = 0; sdi < ARINC429_MAX_SD; sdi++)
      {
         // Allow all labels to be "accepted"
         pLabelData->sd[sdi].Accept  = TRUE;
         // Also update the Sensor Label Accept request for later processing
         pSensorData->sd[sdi].Accept = TRUE;
      }
   }
   else
   {
      if ( pWordInfo->SDIAllCall == TRUE )
      {
         // For "All Call Mode" allow "SD=0" to be accepted
         pLabelData->sd[0].Accept  = TRUE;
         // Also update the Sensor Label Accept request for later processing
         pSensorData->sd[0].Accept = TRUE;
      }
      // Allow specified "SD" to be accepted
      pLabelData->sd[pWordInfo->SDBits].Accept = TRUE;
      // NOTE: Do not set the un-referenced ->sd[] to "Not Accept", since
      //   thru Arinc cfg ->FilterArray[] the entire label word could be
      //   be selected (incl all SD versions) to be accepted. We do not
      //   want to "undo" this selection
      // NOTE: Potential conflict if label is set for "accept" all, and
      //  Arinc429 sensor defines AllCall (where Arinc429_ReadWord() would
      //  return value from the SD_Ignore field in the m_ArincLabelData[][] table
      //  for SD==0 and the specified SD) reading SD_Ignore field would
      //  return all the SD values received, not just the specified All Call Mode.
      // TBD - This only works if Arinc429 sensor defines only specific SD !!!

      // Also update the Sensor Label Accept request for later processing (specifically
      //  when Arinc429_ReconfigureFPGAShadowRam() is called
      pSensorData->sd[pWordInfo->SDBits].Accept = TRUE;
   }

   SdiMask = 0;

   for ( sdi = 0; sdi < ARINC429_MAX_SD; sdi++ )
   {
      SdiMask |= pLabelData->sd[sdi].Accept ? 0x1 << sdi : 0;
   }

   // Initialize the hardware to accept the configured label sdi combinations
   Arinc429DrvCfgLFR ( (UINT8)SdiMask, pWordInfo->RxChan, pWordInfo->Label, TRUE );
}

/*****************************************************************************/
/* BUILT IN TEST INTERFACE                                                   */
/*****************************************************************************/

/******************************************************************************
 * Function:     Arinc429MgrCreateTimeOutSysLog
 *
 * Description:  Creates the Arinc429 TimeOut System Logs
 *
 * Parameters:   resultType - SYS_A429_STARTUP_TIMEOUT
 *                         or SYS_A429_DATA_LOSS_TIMEOUT
 *               ch - Arinc429 Rx Ch
 *
 * Returns:      none
 *
 * Notes:        none
 *
 *****************************************************************************/
static
void Arinc429MgrCreateTimeOutSysLog( RESULT resultType, UINT8 ch )
{
   // Local Data
   ARINC429_SYS_TIMEOUT_LOG timeoutLog;
   ARINC429_RX_CFG_PTR pArinc429Cfg;
   SYS_APP_ID sysId;

   pArinc429Cfg = &m_Arinc429Cfg.RxChan[ch];

   if ( SYS_A429_STARTUP_TIMEOUT == resultType )
   {
      timeoutLog.result      = resultType;
      timeoutLog.cfg_timeout = pArinc429Cfg->ChannelStartup_s;
      timeoutLog.ch          = ch;
      sysId                  = SYS_ID_A429_CBIT_STARTUP_FAIL;
   }
   else if ( SYS_A429_DATA_LOSS_TIMEOUT == resultType )
   {
      timeoutLog.result      = resultType;
      timeoutLog.cfg_timeout = pArinc429Cfg->ChannelTimeOut_s;
      timeoutLog.ch          = ch;
      sysId                  = SYS_ID_A429_CBIT_DATA_LOSS_FAIL;
   }
   else
   {
      // No such case!
      FATAL("Unrecognized resultType = %d", resultType);
   } // end if (resultType)
   
   Flt_SetStatus(pArinc429Cfg->ChannelSysCond, sysId, &timeoutLog,
                 sizeof(timeoutLog));
}

/******************************************************************************
 * Function:     Arinc429MgrDetermineSSMFailure
 *
 * Description:  Determines SSM Failure or Data Loss Timeout
 *
 * Parameters:   pFailCount - ptr to Fail Count Array
 *               TotalTests - Total Tests to check
 *               pMsg       - ptr to Msg Str Buffer
 *
 * Returns:      none
 *
 * Notes:        none
 *
 *****************************************************************************/
static
void Arinc429MgrDetermineSSMFailure ( UINT32 *pFailCount, UINT8 TotalTests, char *pMsg )
{
   // Local Data
   UINT16 i;
   UINT16 FailIndex;
   UINT16 MaxCount;

   FailIndex = END_OF_FAIL_COUNT;   // Init to Data Loss Condition.
                                    //  See Arinc429_ErrMsg[END_OF_FAIL_COUNT]
   MaxCount = 0;

   for ( i = 0; i < TotalTests; i++ )
   {
      // Find the largest fail count
      if ( pFailCount[i] > MaxCount )
      {
         // Update MaxCount
         MaxCount  = pFailCount[i];
         FailIndex = i;
      }
   } // End of for i loop

   // If the MaxCount is still zero then the label is not being
   // transmitted.  FailIndex == END_OF_FAIL_COUNT.
   snprintf ( pMsg, ARINC429_SSM_FAIL_MSG_SIZE, "%s", Arinc429_ErrMsg[FailIndex] );
}

/*****************************************************************************/
/* DATA MANAGER INTERFACE                                                    */
/*****************************************************************************/

/******************************************************************************
 * Function:     Arinc429MgrReadReduceSnapshot
 *
 * Description:  The Arinc429 Read Reduce snapshot reads all the received
 *               parameters that have been configured for reduction. These
 *               parameters are returned to the data manager for storage as
 *               a snapshot.
 *
 * Parameters:   pBuff        - Location to store the snapshot data
 *               nMaxRecords  - Total number of records that can be stored
 *               pChanData    - Channel Data Storage location
 *               pCfg         - ARINC429 RX Config Pointer
 *               bStartSnap   - Flag for Start Snapshot
 *
 *
 * Returns:      UINT16       - Total records saved
 *
 * Notes:        Reduction logic depends on the Start Snapshot to intialize
 *               all configured parameters.
 *
 *****************************************************************************/
static UINT16 Arinc429MgrReadReduceSnapshot( ARINC429_SNAPSHOT_RECORD_PTR pBuff,
                                             UINT16 nMaxRecords, 
                                             ARINC429_CHAN_DATA_PTR pChanData,
                                             ARINC429_RX_CFG_PTR pCfg, BOOLEAN bStartSnap )
{
   // Local Data
   BOOLEAN                      bWordValid;
   UINT8                        sdi;
   UINT16                       Cnt;
   UINT16                       Index;
   SINT32                       sValue;
   ARINC429_LABEL_DATA_PTR      pLabelData;
   ARINC429_SDI_DATA_PTR        pSdiData;
   ARINC429_REDUCE_PROTOCOL_PTR pProtocol;
   ARINC429_CONV_DATA_PTR       pConvData;
   ARINC429_WORD_INFO_PTR       pWordInfo;

   Cnt = 0;

   // Loop through all possible configured reduction parameters
   // This should be faster than searching the entire ARINC-429 storage table
   for ( Index = 0; Index < ARINC429_MAX_PARAMS; Index++ )
   {
      // Check if parameter configuration is being used
      if ( ( 0 != pCfg->Parameter[Index].Label ) && (0 != pCfg->Parameter[Index].GPA) )
      {
         pLabelData = &pChanData->LabelData[pCfg->Parameter[Index].Label];

         // Set the pointers to be used - If configured for ignore just do the ignore
         sdi = ( pLabelData->sd[SDI_IGNORE].ProtocolStorage.bIgnoreSDI == TRUE ) ?
                SDI_IGNORE : GPA_SDI_DEFINITION(pCfg->Parameter[Index].GPA);

         pSdiData  = &pLabelData->sd[sdi];
         pProtocol = &pSdiData->ProtocolStorage;
         pConvData = &pProtocol->ConversionData;
         pWordInfo = &pChanData->ReduceWordInfo[pConvData->WordInfoIndex];

         // Check that the reduction processing is configured and
         // this data is to be accepted
         if ( ( TRUE == pSdiData->Accept ) && ( 0 != pSdiData->CurrentMsg ) )
         {
           // If at least one label has been received,
           //    should be != 0 since bit 7..0 is the
           //    non-zero label !

           // Is this a start or end snapshot?
           if ( TRUE == bStartSnap )
           {
              // Check that the message is OK to store
              if ( Arinc429MgrParameterTest ( pSdiData, pWordInfo ) )
              {
                 // Message is OK tag it as Valid
                 pProtocol->bValid = TRUE;
                 // Convert Message to EngValue
                 sValue = Arinc429MgrParseMsg ( pSdiData->CurrentMsg, pWordInfo->Format,
                                                pWordInfo->WordPos,   pWordInfo->WordSize,
                                                pWordInfo->RxChan,    &bWordValid );

                 if ( ( FALSE == bWordValid ) && ( BCD == pWordInfo->Format ) )
                 {
                    pProtocol->bValid = FALSE;
                    // Get the current value of the parameter
                    DataReductionGetCurrentValue(&pProtocol->ReduceData);
                 }
                 else
                 {
                    // Convert the Engineering Units
                    pProtocol->ReduceData.EngVal = sValue * pConvData->Scale;
                 }

                 pProtocol->ReduceData.Time   = pSdiData->RxTime;
                 // Perform the Data Reduction Initialization
                 DataReductionInit(&pProtocol->ReduceData);
                 pProtocol->bInitialized = TRUE;

                 pProtocol->Reduced429Msg = Arinc429MgrConvertEngToMsg
                                                       ( pProtocol->ReduceData.EngVal,
                                                         pConvData->Scale,
                                                         pWordInfo->WordSize,
                                                         pWordInfo->WordPos,
                                                         pWordInfo->Format,
                                                         pSdiData->CurrentMsg,
                                                         pProtocol->bValid );
                 // Store it in the Buffer
                 pBuff->TimeStamp         = pSdiData->RxTimeStamp;
                 pBuff->Arinc429Msg       = pProtocol->Reduced429Msg;
                 // Increment the location in the buffer and count the record
                 pBuff++;
                 Cnt++;
              }
           }
           else // End Snapshot
           {
              // We are returning the last value until next recording so set
              // the init flag false also. The next start snap will set it back.
              pProtocol->bInitialized     = FALSE;

              // Get the current value of the parameter
              DataReductionGetCurrentValue(&pProtocol->ReduceData);

              pProtocol->Reduced429Msg = Arinc429MgrConvertEngToMsg
                                                  ( pProtocol->ReduceData.EngVal,
                                                    pConvData->Scale,
                                                    pWordInfo->WordSize,
                                                    pWordInfo->WordPos,
                                                    pWordInfo->Format,
                                                    pSdiData->CurrentMsg,
                                                    pProtocol->bValid );
              // Store it in the Buffer
              pBuff->TimeStamp         = pSdiData->RxTimeStamp;
              pBuff->Arinc429Msg       = pProtocol->Reduced429Msg;
              // Increment the location in the buffer and count the record
              pBuff++;
              Cnt++;
           } // End of else - request end snapshot
         }  // End of ->Accept == TRUE and ( 0 != pSdiData->CurrentMsg )
      } // End of if param being used
   }  // End for loop

   return( Cnt );
}

/******************************************************************************
 * Function:     Arinc429MgrGetReduceHdr
 *
 * Description:  Retrieve the Reduction Binary Data Header.
 *
 * Parameters:   pDest        - pointer to destination buffer
 *               chan         - channel to request file header
 *               nMaxByteSize - maximum destination buffer size
 *               pNumParams   - Storage location for counting parameters
 *
 * Returns:      UINT16       - actual number of bytes returned
 *
 * Notes:        none
 *
 *****************************************************************************/
static UINT16 Arinc429MgrGetReduceHdr ( INT8 *pDest, UINT32 chan, UINT16 nMaxByteSize,
                                                                  UINT16 *pNumParams )
{
   // Local Data
   UINT16                 Index;
   ARINC429_RX_CFG_PTR    pCfg;
   ARINC429_RX_REDUCE_HDR ReduceHdr;
   UINT16                 Size;
   UINT16                 Remaining;

   // Initialize Local Data
   pCfg      = &m_Arinc429Cfg.RxChan[chan];
   Remaining = nMaxByteSize;
   Size      = 0;

   // Clear the Header
   memset(&ReduceHdr, 0, sizeof(ReduceHdr));

   // Loop through all the possible configured parameters for the channel
   for ( Index = 0; Index < ARINC429_MAX_PARAMS; Index++)
   {
      // If there is a label and a GPA then the parameter must be configured
      if ( ( 0 != pCfg->Parameter[Index].Label ) && (0 != pCfg->Parameter[Index].GPA) )
      {
         // Store the parameter configuration, increment the number of params,
         // and adjust the size of the header for the configuration found
         ReduceHdr.ParamCfg[*pNumParams].Label     = pCfg->Parameter[Index].Label;
         ReduceHdr.ParamCfg[*pNumParams].GPA       = pCfg->Parameter[Index].GPA;
         ReduceHdr.ParamCfg[*pNumParams].GPB       = pCfg->Parameter[Index].GPB;
         ReduceHdr.ParamCfg[*pNumParams].Scale     = pCfg->Parameter[Index].Scale;
         ReduceHdr.ParamCfg[*pNumParams].Tolerance = pCfg->Parameter[Index].Tolerance;
         *pNumParams += 1;
         Size        += sizeof(ReduceHdr.ParamCfg[*pNumParams]);
         Remaining   -= sizeof(ReduceHdr.ParamCfg[*pNumParams]);
      }
   }

   // Copy the reduce header to the passed in buffer
   memcpy(pDest, &ReduceHdr, Size);

   // Return the number of bytes stored
   return (Size);
}

/*****************************************************************************/
/* DEBUG INTERFACE                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:     Arinc429MgrCreateAllInternalLogs
 *
 * Description:  Creates all Arinc429 internal logs for testing / debug of log
 *               decode and structure.
 *
 * Parameters:   None.
 *
 * Returns:      None.
 *
 * Notes:
 *  1) LogWriteSystem() queues up to a max of 16 logs.  Limit to 10 logs here
 *
 *****************************************************************************/
#ifdef GENERATE_SYS_LOGS
/*vcast_dont_instrument_start*/

void Arinc429MgrCreateAllInternalLogs ( void )
{
  ARINC429_SYS_BCD_FAIL_LOG sysBcdFailLog;

  // 1 Create DRV_ID_FPGA_PBIT_FAIL_LOG:DRV_A429_FPGA_BAD_STATE
  LogWriteSystem( DRV_ID_A429_PBIT_FPGA_FAIL, LOG_PRIORITY_LOW,
                  &Arinc429DrvPbitLogFpga,
                  sizeof(ARINC_DRV_PBIT_FPGA_FAIL), NULL );

  // 2 Create DRV_ID_A429_PBIT_RX_LFR_FIFO_FAIL:DRV_A429_RX_LFR_FIFO_TEST_FAIL
  //   -> LFR Test Fails
  LogWriteSystem( DRV_ID_A429_PBIT_TEST_FAIL, LOG_PRIORITY_LOW,
                  &Arinc429DrvPbitLogLFR,
                  sizeof(ARINC_DRV_PBIT_TEST_RESULT), NULL );

  // 3 Create DRV_ID_A429_PBIT_RX_LFR_FIFO_FAIL:DRV_A429_RX_LFR_FIFO_TEST_FAIL
  //   -> Rx FIFO Test Fails
  LogWriteSystem( DRV_ID_A429_PBIT_TEST_FAIL, LOG_PRIORITY_LOW,
                  &Arinc429DrvPbitLogRxFIFO,
                  sizeof(ARINC_DRV_PBIT_TEST_RESULT), NULL );

  // 4 Create DRV_ID_A429_PBIT_RX_LFR_FIFO_FAIL:DRV_A429_RX_LFR_FIFO_TEST_FAIL
  //  -> Tx FIFO Test Fails
  LogWriteSystem( DRV_ID_A429_PBIT_TEST_FAIL, LOG_PRIORITY_LOW,
                  &Arinc429DrvPbitLogTxFIFO,
                  sizeof(ARINC_DRV_PBIT_TEST_RESULT), NULL );

  // 5 Create DRV_ID_A429_PBIT_RX_LFR_FIFO_FAIL:DRV_A429_TX_RX_LOOP_FAIL
  //  -> Tx-Rx Loop Test Fails
  LogWriteSystem( DRV_ID_A429_PBIT_TEST_FAIL, LOG_PRIORITY_LOW,
                  &Arinc429DrvPbitLogTxRxLoop,
                  sizeof(ARINC_DRV_PBIT_TEST_RESULT), NULL );

  // 6 Create SYS_ID_A429_PBIT_FAIL_LOG:SYS_A429_PBIT_FAIL
  LogWriteSystem( SYS_ID_A429_PBIT_DRV_FAIL, LOG_PRIORITY_LOW,
                  &Arinc429SysPbitLog,
                  sizeof(ARINC429_SYS_PBIT_STARTUP_LOG), NULL );

  // 7 Create SYS_ID_A429_CBIT_FAIL_LOG:SYS_A429_STARTUP_TIMEOUT
  LogWriteSystem( SYS_ID_A429_CBIT_STARTUP_FAIL, LOG_PRIORITY_LOW,
                  &Arinc429SysCbitStartupTimeoutLog,
                  sizeof(ARINC429_SYS_TIMEOUT_LOG), NULL );

  // 8 Create SYS_ID_A429_CBIT_FAIL_LOG:SYS_A429_DATA_LOSS_TIMEOUT
  LogWriteSystem( SYS_ID_A429_CBIT_DATA_LOSS_FAIL, LOG_PRIORITY_LOW,
                  &Arinc429SysCbitDataTimeoutLog,
                  sizeof(ARINC429_SYS_TIMEOUT_LOG), NULL );

  // 9 Create SYS_ID_A429_CBIT_SSM_FAIL:SYS_A429_SSM_FAIL
  LogWriteSystem( SYS_ID_A429_CBIT_SSM_FAIL, LOG_PRIORITY_LOW,
                  &Arinc429SysCbitSSMFailLog,
                  sizeof(ARINC429_SYS_CBIT_SSM_FAIL_LOG), NULL );

  // 10 Create SYS_ID_ARINC429_BCD_DIGITS
  sysBcdFailLog.a429word = 0x12345678;
  sysBcdFailLog.bcdData = 0x12345678;
  sysBcdFailLog.digit = 0x1234;
  LogWriteSystem( SYS_ID_A429_BCD_DIGITS, LOG_PRIORITY_LOW,
                  &sysBcdFailLog,
                  sizeof(ARINC429_SYS_BCD_FAIL_LOG), NULL );
}
/*vcast_dont_instrument_end*/

#endif
/******************************************************************************
 * Function:     Arinc429MgrDisableLiveStream
 *
 * Description:  Disables the output of live data streams.
 *
 * Parameters:   None.
 *
 * Returns:      None.
 * 
 * Notes:        None.
 *
 ****************************************************************************/
void Arinc429MgrDisableLiveStream(void)
{
  m_Arinc429_Debug.bOutputRawFilteredBuff = FALSE;
}

/******************************************************************************
 * Function:    Arinc429MgrDispSingleArincChan
 *
 * Description: Debug routine to print single raw Arinc Rx data to the GSE port
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:
 *  1) This function is used by the Arinc429MgrDisplaySWBufferTask to
 *     display the single col mode where first-in-first-out is displayed
 *
 *
 *****************************************************************************/
void Arinc429MgrDispSingleArincChan ( void  )
{
  UINT16 i;
  UINT16 j;
  UINT16 numActiveChannels;
  UINT16 numRows = 0;
  UINT16 cnt;
  UINT32 nStartTime;
  UINT32 nArincMsg;

  // Destination buffers for RAW
  UINT32 rawData[ARINC429_MAX_RECORDS];
  ARINC429_RECORD rawDataRecords[ARINC429_MAX_RECORDS];

  // Single ch display factor 21 = 115.2 KBps / 100 msec / 4 chan max @ 75% loading
  #define ARINC429_DEBUG_DISP_SINGLE_CH_MAX  21

  // Determine number of active channels
  numActiveChannels = 0;
  for (j = 0; j < ARINC_RX_CHAN_MAX; j++ )
  {
    if ( m_Arinc429Cfg.RxChan[j].Enable == TRUE )
    {
      numActiveChannels++;
    }
  }

  // Determine number of rows that can be displayed
  if (numActiveChannels > 0)
  {
    numRows = ARINC429_DEBUG_DISP_SINGLE_CH_MAX / numActiveChannels;
  }

  for ( j = 0; j < ARINC_RX_CHAN_MAX; j++ )
  {
    // Read from Arinc429 Software Raw Filtered Buffer
    if ( m_Arinc429_Debug.OutputType == DEBUG_OUT_RAW )
    {
      cnt = Arinc429MgrReadBuffer ( rawData, sizeof(rawData), &m_Arinc429DebugBuffer[j] );
    }
    else
    {
      cnt = Arinc429MgrReadBuffer ( rawDataRecords, sizeof(rawDataRecords),
                                    &m_Arinc429RxBuffer[j]);
    }

    // If Debug Arinc Raw Display Enabled and Cnt != 0 output to GSE
    // label then raw data
    i = 0;
    if ( ( i != cnt ) && ( m_Arinc429_Debug.bOutputRawFilteredBuff == TRUE) )
    {
      nStartTime = CM_GetTickCount();
      snprintf ( GSE_OutLine, sizeof(GSE_OutLine),
                 "(Ch=%d) Tick: %08d ================= \r\n", (j), nStartTime );
      GSE_PutLine( GSE_OutLine );
    }

    while ((i != cnt) && ( m_Arinc429_Debug.bOutputRawFilteredBuff == TRUE) && ( i < numRows))
    {
      nArincMsg = ( m_Arinc429_Debug.OutputType == DEBUG_OUT_RAW ) ?
                    rawData[i] : rawDataRecords[i].ARINC429Msg;

      Arinc429MgrDisplayFmtedLine ( m_Arinc429_Debug.bOutputRawFilteredFormatted, nArincMsg );
      i++;
    } // End while i loop
  } // End for j Loop
}

/******************************************************************************
 * Function:    Arinc429MgrDisplayMultiArincChan
 *
 * Description: Debug routine to print single raw Arinc Rx data to the GSE port
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:
 *  1) This function is used by the Arinc429MgrDisplaySWBufferTask to
 *     display a col mode where all 4 rx ch are displayed
 *
 *
 *****************************************************************************/
void Arinc429MgrDisplayMultiArincChan ( void )
{
  UINT16 i;
  UINT16 k;
  UINT32  nStartTime;
  BOOLEAN bHaveRawData;
  UINT32  nMaxCnt = 0;
  UINT32  nArincMsg;

  for (i = 0; i < ARINC_RX_CHAN_MAX; i++)
  {
    RawDataCnt[i] = 0; // Clear the Rx Raw Data Cnt
  }

  for ( i = 0; i < ARINC_RX_CHAN_MAX; i++ )
  {
    // Read from Arinc429 Software Raw Filtered Buffer
    if ( m_Arinc429_Debug.OutputType == DEBUG_OUT_RAW )
    {
      RawDataCnt[i] = Arinc429MgrReadBuffer ( RawDataArinc[i],
                                              sizeof(RawDataArinc[0]),
                                              &m_Arinc429DebugBuffer[i] );
    }
    else
    {
      RawDataCnt[i] = Arinc429MgrReadBuffer ( RawRecords[i],
                                             sizeof(RawRecords[0]),
                                             &m_Arinc429RxBuffer[i] );
    }
  }

  bHaveRawData = FALSE;
  for ( i = 0; i < ARINC_RX_CHAN_MAX; i++ )
  {
    if (RawDataCnt[i] != 0)
    {
      bHaveRawData = TRUE;
      // Find max cnt
      if (nMaxCnt < RawDataCnt[i])
      {
       nMaxCnt = RawDataCnt[i];
      }
    }
  }

  if ((bHaveRawData == TRUE) && ( m_Arinc429_Debug.bOutputRawFilteredBuff == TRUE))
  {
    nStartTime = CM_GetTickCount();
    snprintf( GSE_OutLine, sizeof(GSE_OutLine), "\r\nt=%07d ",nStartTime);
    GSE_PutLine( GSE_OutLine );

    i = 0;
    while ((i != nMaxCnt) && (i < ARINC429_MAX_DISPLAY_LINES))
     // Max 12 lines (80 bytes each) at 100 msec
    //   @ 115200 bps GSE
    {
      for ( k = 0; k < ARINC_RX_CHAN_MAX; k++ )
      {
        if ( i < RawDataCnt[k] )
        {
          nArincMsg = ( m_Arinc429_Debug.OutputType == DEBUG_OUT_RAW ) ?
                                        RawDataArinc[k][i] : RawRecords[k][i].ARINC429Msg;

          Arinc429MgrDisplayFmtedLine ( m_Arinc429_Debug.bOutputRawFilteredFormatted,
                                       nArincMsg );
        }
        else
        {
          GSE_PutLine( "               " );   // Format "filler"
        }
      } // End for k loop thru each Rx Chan

      i++;
      if ( ( i != nMaxCnt ) && ( i < ARINC429_MAX_DISPLAY_LINES ) )
      {
        GSE_PutLine( "\r\n          ");
      }
      else
      {
        // GSE_PutLine ( " \r\n");
        if ( ( i >= ARINC429_MAX_DISPLAY_LINES ) && ( i < nMaxCnt) )
        {
          GSE_PutLine(
        "\r\n                 more           more           more           more");
        }
      }
    }  // End while i loop thru all Max Rx Cnt
  } // End if bHaveRawData == TRUE
}


/******************************************************************************
 * Function:    Arinc429MgrDisplayFmtedLine
 *
 * Description: Utility routine to format an Arinc DEBUG_OUT_PROTOCOL display msg.
 *
 * Parameters: isFormatted - flag signaling if the output should be displayed
 *                           as formatted or raw.
 *             ArincMsg - The Arinc word to be processed.
 *
 * Returns:     none
 *
 * Notes:
 *  1) This function is used by Arinc429MgrDisplaySingleArincChan and
 *     Arinc429MgrDisplayMultiArincChan to format PROTOCOL display msg in support of
 *      Arinc429MgrDisplaySWBufferTask.
 *
 *
 *****************************************************************************/
void Arinc429MgrDisplayFmtedLine ( BOOLEAN isFormatted, UINT32 ArincMsg )
{
  UINT8  label;
  UINT8  parity;
  UINT32 word;
  UINT8  baseIndex;

  // Offset enums into fmtSpecifiers array below
  enum
  {
    OFST_SINGLE_CHAN  = 0,
    OFST_MULTI_CHAN   = 2
  };

  // Fmt-spec table
  CHAR* fmtSpecifiers[4] =
       {
         "                        %03o %d %06x\r\n",  // SingleChannel, Formatted
         "                        %08x\r\n"        ,  // SingleChannel, Raw
         "   %03o %d %06x"                         ,  // MultiChannel,  Formatted
         "      %08x "                                // MultiChannel,  Raw
       };

  // Determine the offset into the fmt-specifier array based on Multi/Single channel display.
  baseIndex = m_Arinc429_Debug.bOutputMultipleArincChan ? OFST_MULTI_CHAN : OFST_SINGLE_CHAN;

  // Create the display line based on formatted or raw.
  if (isFormatted)
  {
    label  = (UINT8)(  ArincMsg & ARINC_MSG_LABEL_BITS);
    word   =        (( ArincMsg & 0x7FFFFFFFUL) >> 8);
    parity =        (  ArincMsg >> ARINC_MSG_SHIFT_PARITY);
    // Create the output string using the corresponding "Formatted" entry in table
    snprintf(GSE_OutLine, sizeof(GSE_OutLine), fmtSpecifiers[baseIndex], label, parity, word );
  }
  else
  {
    // Create the output string using the corresponding "raw" entry in table
    // Offset into fmt-spec array is the base, plus one for "raw"
    snprintf ( GSE_OutLine, sizeof(GSE_OutLine), fmtSpecifiers[baseIndex + 1], ArincMsg );
  }

  GSE_PutLine( GSE_OutLine );
}

 /*************************************************************************
 *  MODIFICATIONS
 *    $History: ARINC429Mgr.c $
 * 
 * *****************  Version 64  *****************
 * User: John Omalley Date: 2/17/15    Time: 3:55p
 * Updated in $/software/control processor/code/system
 * SCR 1261 - ModFix for Code Review Update injected error
 * 
 * *****************  Version 63  *****************
 * User: John Omalley Date: 1/29/15    Time: 4:51p
 * Updated in $/software/control processor/code/system
 * SCR 1261 - Code Review Update
 * 
 * *****************  Version 62  *****************
 * User: John Omalley Date: 1/19/15    Time: 11:02a
 * Updated in $/software/control processor/code/system
 * SCR 1261 - Code Review Updates
 * 
 * *****************  Version 61  *****************
 * User: John Omalley Date: 4/17/14    Time: 2:04p
 * Updated in $/software/control processor/code/system
 * SCR 1261 - Fixed Data Reduction for BCD parameters
 * SCR 1168 - Fixed Debug messages with timestamp on same line
 * SCR 1146 - Changed Arinc429 data reduction scale parameter to FLOAT
 * 
 * *****************  Version 60  *****************
 * User: John Omalley Date: 12-12-02   Time: 9:59a
 * Updated in $/software/control processor/code/system
 * SCR 1197 - Code Review Updates
 * 
 * *****************  Version 59  *****************
 * User: John Omalley Date: 12-11-29   Time: 7:19p
 * Updated in $/software/control processor/code/system
 * SCR 1197 - Code Review Update
 * 
 * *****************  Version 58  *****************
 * User: John Omalley Date: 12-11-29   Time: 4:13p
 * Updated in $/software/control processor/code/system
 * SCR 1197 - Code Review Fixes
 * 
 * *****************  Version 57  *****************
 * User: John Omalley Date: 12-11-16   Time: 8:12p
 * Updated in $/software/control processor/code/system
 * SCR 1087 - Code Review Updates
 * 
 * *****************  Version 56  *****************
 * User: John Omalley Date: 12-11-15   Time: 7:04p
 * Updated in $/software/control processor/code/system
 * SCR 1197 - Code Review Updates
 * 
 * *****************  Version 55  *****************
 * User: John Omalley Date: 12-11-14   Time: 7:20p
 * Updated in $/software/control processor/code/system
 * SCR 1076 - Code Review Updates
 * 
 * *****************  Version 54  *****************
 * User: John Omalley Date: 12-11-12   Time: 7:19p
 * Updated in $/software/control processor/code/system
 * SCR 1102, 1191, 1194 Code Review Updates
 *
 * *****************  Version 53  *****************
 * User: John Omalley Date: 12-11-09   Time: 2:00p
 * Updated in $/software/control processor/code/system
 * SCR 1102, 1191, 1194 Code Review Updates
 *
 * *****************  Version 52  *****************
 * User: Peter Lee    Date: 12-11-01   Time: 6:44p
 * Updated in $/software/control processor/code/system
 * SCR #1194 Add option to ignore A429 Drv PBIT failures when setting up
 * A429 I/F for operation.
 *
 * *****************  Version 51  *****************
 * User: Melanie Jutras Date: 12-10-31   Time: 1:46p
 * Updated in $/software/control processor/code/system
 * SCR #1142 File format errors
 *
 * *****************  Version 50  *****************
 * User: Peter Lee    Date: 12-10-27   Time: 5:06p
 * Updated in $/software/control processor/code/system
 * SCR #1191 Returns update time of param
 *
 * *****************  Version 49  *****************
 * User: Melanie Jutras Date: 12-10-12   Time: 1:51p
 * Updated in $/software/control processor/code/system
 * SCR 1142 Formatting Changes for Code Review Tool findings
 *
 * *****************  Version 48  *****************
 * User: Melanie Jutras Date: 12-10-10   Time: 12:35p
 * Updated in $/software/control processor/code/system
 * SCR 1172 PCLint 545 Suspicious use of & Error
 *
 * *****************  Version 47  *****************
 * User: Melanie Jutras Date: 12-10-04   Time: 3:50p
 * Updated in $/software/control processor/code/system
 * SCR 1172 PCLint 545 Suspicious use of & Error
 *
 * *****************  Version 45  *****************
 * User: Melanie Jutras Date: 12-10-04   Time: 1:45p
 * Updated in $/software/control processor/code/system
 * SCR 1172 PCLint 545 Error Suspicious use of &
 *
 * *****************  Version 44  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 43  *****************
 * User: Jim Mood     Date: 2/24/12    Time: 10:38a
 * Updated in $/software/control processor/code/system
 * SCR 1114 - Re labeled after v1.1.1 release
 *
 * *****************  Version 41  *****************
 * User: Contractor V&v Date: 12/14/11   Time: 6:50p
 * Updated in $/software/control processor/code/system
 * SCR #1102 ARINC429 PW305AB MFD Timeout Stores wrong Line Number
 *
 * *****************  Version 40  *****************
 * User: John Omalley Date: 10/10/11   Time: 6:05p
 * Updated in $/software/control processor/code/system
 * SCR 701 - BCD Invalid fix
 *
 * *****************  Version 39  *****************
 * User: John Omalley Date: 9/30/11    Time: 11:27a
 * Updated in $/software/control processor/code/system
 * SCR 1015 - Interface Valid Logic Update
 *
 * *****************  Version 38  *****************
 * User: John Omalley Date: 9/29/11    Time: 4:08p
 * Updated in $/software/control processor/code/system
 * SCR 1015 - Interface Validity
 *
 * *****************  Version 37  *****************
 * User: John Omalley Date: 8/22/11    Time: 11:29a
 * Updated in $/software/control processor/code/system
 * SCR 1015 - Changed Interface valid to FALSE until data is received
 *
 * *****************  Version 36  *****************
 * User: Contractor2  Date: 7/05/11    Time: 10:30a
 * Updated in $/software/control processor/code/system
 * SCR #701 Enhancement - ARINC-429 Data Reduction and BCD Parameters
 * Corrected BCD conversion.
 *
 * *****************  Version 35  *****************
 * User: Contractor2  Date: 7/01/11    Time: 2:04p
 * Updated in $/software/control processor/code/system
 * SCR #701 Enhancement - ARINC-429 Data Reduction and BCD Parameters
 * SCR #597 Enhancement - BCD bad value log
 *
 * *****************  Version 34  *****************
 * User: John Omalley Date: 6/08/11    Time: 12:27p
 * Updated in $/software/control processor/code/system
 * SCR 1025 - Fixed Truncation of Most Significant Discrete Bit
 * SCR 1040 - Fixed the left justification of discrete data and the Sign
 * Magnitude BNR conversion which should have been 2's complement.
 *
 * *****************  Version 33  *****************
 * User: Contractor2  Date: 6/01/11    Time: 11:42a
 * Updated in $/software/control processor/code/system
 * SCR #597 Enhancement - BCD bad value log
 * Correct label mask error. Changed arinc masks to predefined symbol
 * names.
 *
 * *****************  Version 32  *****************
 * User: Contractor2  Date: 5/10/11    Time: 2:09p
 * Updated in $/software/control processor/code/system
 * SCR #597 Enhancement - BCD bad value log
 *
 * *****************  Version 31  *****************
 * User: John Omalley Date: 11/16/10   Time: 11:56a
 * Updated in $/software/control processor/code/system
 * SCR 997 - Fixed the size in the call to FIFO_Init.
 *
 * *****************  Version 30  *****************
 * User: John Omalley Date: 11/09/10   Time: 2:28p
 * Updated in $/software/control processor/code/system
 * SCR 970 - Unused variable
 *
 * *****************  Version 29  *****************
 * User: John Omalley Date: 11/09/10   Time: 1:33p
 * Updated in $/software/control processor/code/system
 * SCR 970- Code Coverage Dead Code Removal
 *
 * *****************  Version 28  *****************
 * User: Peter Lee    Date: 11/05/10   Time: 12:06p
 * Updated in $/software/control processor/code/system
 * SCR #982 Remove dead code from _SensorTest()
 *
 * *****************  Version 27  *****************
 * User: John Omalley Date: 10/28/10   Time: 7:19p
 * Updated in $/software/control processor/code/system
 * SCR 970 - Code Coverage Update
 *
 * *****************  Version 26  *****************
 * User: John Omalley Date: 10/27/10   Time: 7:59p
 * Updated in $/software/control processor/code/system
 * SCR 970 - Fixed MFD Processing
 *
 * *****************  Version 25  *****************
 * User: John Omalley Date: 10/04/10   Time: 3:10p
 * Updated in $/software/control processor/code/system
 * SCR 896 - Code Coverage Updates
 *
 * *****************  Version 24  *****************
 * User: John Omalley Date: 9/10/10    Time: 10:25a
 * Updated in $/software/control processor/code/system
 * SCR 865 - Changed Condition Compile Flag names
 *
 * *****************  Version 23  *****************
 * User: John Omalley Date: 9/10/10    Time: 10:01a
 * Updated in $/software/control processor/code/system
 * SCR 858 - Added FATAL to default cases
 *
 * *****************  Version 22  *****************
 * User: Jeff Vahue   Date: 9/07/10    Time: 7:30p
 * Updated in $/software/control processor/code/system
 * SCR# 848 - Code Coverage
 *
 * *****************  Version 21  *****************
 * User: Peter Lee    Date: 9/07/10    Time: 12:03p
 * Updated in $/software/control processor/code/system
 * SCR #854 Code Review Updates
 *
 * *****************  Version 20  *****************
 * User: John Omalley Date: 8/24/10    Time: 5:33p
 * Updated in $/software/control processor/code/system
 * SCR 821 - Optimized the Snapshot Processing
 *
 * *****************  Version 19  *****************
 * User: John Omalley Date: 8/18/10    Time: 11:34a
 * Updated in $/software/control processor/code/system
 * SCR 801 - Remove "NL" from log
 *
 * *****************  Version 18  *****************
 * User: Peter Lee    Date: 8/10/10    Time: 4:37p
 * Updated in $/software/control processor/code/system
 * SCR #782 A429 debug display disable
 *
 * *****************  Version 17  *****************
 * User: John Omalley Date: 8/05/10    Time: 6:48p
 * Updated in $/software/control processor/code/system
 * SCR 748 - Removed unnecessary DebugStr prints
 *
 * *****************  Version 16  *****************
 * User: John Omalley Date: 8/03/10    Time: 2:15p
 * Updated in $/software/control processor/code/system
 * SCR 769 - Updated Buffer Sizes and fixed ARINC429 FIFO Index during
 * initialization.
 *
 * *****************  Version 15  *****************
 * User: Contractor V&v Date: 7/30/10    Time: 9:37p
 * Updated in $/software/control processor/code/system
 * SCR #282 Misc - Shutdown Processing
 *
 * *****************  Version 14  *****************
 * User: Peter Lee    Date: 7/30/10    Time: 3:45p
 * Updated in $/software/control processor/code/system
 * SCR #747 Arinc429 Debug Display
 *
 * *****************  Version 13  *****************
 * User: John Omalley Date: 7/29/10    Time: 7:51p
 * Updated in $/software/control processor/code/system
 * SCR 754 - Added a FIFO flush after initialization
 *
 * *****************  Version 12  *****************
 * User: John Omalley Date: 7/28/10    Time: 8:58p
 * Updated in $/software/control processor/code/system
 * SCR 752 - Moved recording flag to snapshot
 *
 * *****************  Version 11  *****************
 * User: Jeff Vahue   Date: 7/27/10    Time: 3:40p
 * Updated in $/software/control processor/code/system
 * SCR# 745 - missing index to limit the size of a structure.
 *
 * *****************  Version 10  *****************
 * User: Peter Lee    Date: 7/26/10    Time: 6:20p
 * Updated in $/software/control processor/code/system
 * SCR #743 Update TimeOut Processing for val == 0
 *
 * *****************  Version 9  *****************
 * User: John Omalley Date: 7/20/10    Time: 7:34p
 * Updated in $/software/control processor/code/system
 * SCR 720 - Fixed Debug Message display problem
 *
 * *****************  Version 8  *****************
 * User: John Omalley Date: 7/20/10    Time: 9:04a
 * Updated in $/software/control processor/code/system
 * SCR 294 - Add system binary header
 *
 * *****************  Version 7  *****************
 * User: John Omalley Date: 7/15/10    Time: 4:12p
 * Updated in $/software/control processor/code/system
 * SCR 650
 *
 * *****************  Version 6  *****************
 * User: John Omalley Date: 7/08/10    Time: 2:51p
 * Updated in $/software/control processor/code/system
 * SCR 650
 *
 * *****************  Version 5  *****************
 * User: Contractor V&v Date: 6/22/10    Time: 6:24p
 * Updated in $/software/control processor/code/system
 * SCR #485 Escape Sequence and Box Configuration
 *
 * *****************  Version 4  *****************
 * User: Jeff Vahue   Date: 6/15/10    Time: 1:11p
 * Updated in $/software/control processor/code/system
 * Init Var for Windows Emulation
 *
 * *****************  Version 3  *****************
 * User: John Omalley Date: 6/11/10    Time: 5:57p
 * Updated in $/software/control processor/code/system
 * SCR 627 - Fixed Debug Messages
 *
 * *****************  Version 2  *****************
 * User: Contractor V&v Date: 6/08/10    Time: 5:53p
 * Updated in $/software/control processor/code/system
 * SCR #615 Showcfg/Long msg enhancement
 *
 * *****************  Version 1  *****************
 * User: John Omalley Date: 6/08/10    Time: 12:19p
 * Created in $/software/control processor/code/system
 * SCR 627 - Updated for LJ60
 *
 *
 *
 ***************************************************************************/

