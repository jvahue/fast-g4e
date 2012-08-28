#define QAR_BODY
/******************************************************************************
            Copyright (C) 2008 - 2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:         QAR.c    
    
    Description: This packages contains all the driver and system code for the
                 QAR interface.
    
   VERSION
      $Revision: 93 $  $Date: 10/05/11 7:55p $
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    


/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "CfgManager.h"
#include "dio.h"
#include "mcf548x.h"
#include "qar.h"
#include "clockmgr.h"
#include "FaultMgr.h"
#include "gse.h"
#include "systemlog.h"
#include "Assert.h"

#include <string.h>

#include "TestPoints.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
typedef struct {
  UINT16 Word[QAR_SF_MAX_SIZE];
  UINT32 LastSubFrameUpdateTime; 
} QAR_SUBFRAME_DATA, *QAR_SUBFRAME_DATE_PTR; 

typedef struct {
  UINT16 Tstp_val; 
  UINT16 Tstn_val; 
  UINT16 Bipp_exp; 
  UINT16 Biin_exp; 
  QAR_LOOPBACK_TEST TestEnum; 
} QAR_LOOPBACK_TEST_STRUCT; 

typedef struct {
  RESULT resultCode; 
  volatile UINT16 *ptr; 
} QAR_PBIT_INTERNAL_DPRAM; 

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static QAR_REGISTERS       *pQARCtrlStatus;
static QAR_CTRL_REGISTER   *pQARCtrl;
static QAR_STATUS_REGISTER *pQARStatus;
static QAR_DATA            *pQARSubFrame;

static QAR_SUBFRAME_DATA   QARFrame[QAR_SF_MAX];
static QAR_SUBFRAME_DATA   QARFramePrevGood[QAR_SF_MAX]; 

static QAR_STATE           QARState;
static QAR_CONFIGURATION   QARCfg;

static QAR_CBIT_HEALTH_COUNTS QARCBITHealthStatus; 

QAR_WORD_INFO              QARWordInfo[QAR_SF_MAX_SIZE]; 
UINT16                     QARWordSensorCount;  

static char                GSE_OutLine[512];

static BOOLEAN             m_BarkerErr; 

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static QAR_SF QAR_FindCurrentGoodSF (UINT16 nIndex);
static void QAR_RestoreStartupCfg (void);
static void QAR_ReconfigureFPGAShadowRam (void);
static void QAR_ResetState (void);

static RESULT QAR_PBIT_LoopBack_Test(UINT8 *pdest, UINT16 *psize);
static RESULT QAR_PBIT_DPRAM_Test(UINT8 *pdest, UINT16 *psize); 

static void QAR_CreateTimeOutSystemLog( RESULT resultType ); 

// Shutdown handlers
static void QAR_ReadSubFrameTaskShutdown(void);
static void QAR_ReadSubFrameTask (void *pParam);
static void QAR_MonitorTask (void *pParam);


/*****************************************************************************/
/* Local Constants                                                           */
/*****************************************************************************/
static QAR_LOOPBACK_TEST_STRUCT QARLBTest[] = {
  { 1, 0,  1, 0,  QAR_TEST_TSTP_HIGH },
  { 0, 1,  0, 1,  QAR_TEST_TSTN_HIGH }, 
  { 1, 1,  0, 0,  QAR_TEST_TSTPN_HIGH }
};



#ifdef GENERATE_SYS_LOGS
  // QAR System Logs - For Test Purposes
  static SYS_QAR_PBIT_FAIL_LOG_LOOPBACK_STRUCT PbitLogLoopBack[] = {
    {SYS_QAR_PBIT_LOOPBACK, QAR_TEST_TSTPN_HIGH}
  }; 
  
  static SYS_QAR_PBIT_FAIL_LOG_DPRAM_STRUCT PbitLogDpram[] = {
    {SYS_QAR_PBIT_DPRAM, 0x0A5A, 0x05A5} 
  };
  
  static SYS_QAR_PBIT_FAIL_LOG_FPGA_STRUCT PbitLogFpga[] = {
    {SYS_QAR_FPGA_BAD_STATE} 
  };
  
  static QAR_TIMEOUT_LOG CbitLogStartup[] = {
    {SYS_QAR_STARTUP_TIMEOUT, 20010}
  };
  static QAR_TIMEOUT_LOG CbitLogDataLoss[] = {
    {SYS_QAR_DATA_LOSS_TIMEOUT, 10010}
  };
  
  static SYS_QAR_CBIT_FAIL_WORD_TIMEOUT_LOG CbitLogWordTimeout[] = {
    {SYS_QAR_WORD_TIMEOUT, "Xcptn: Word Timeout (S = 15)"}
  };
  
  static QAR_SYS_PBIT_STARTUP_LOG QarSysPbitLog[] = {
     {SYS_QAR_PBIT_FAIL} 
  };
#endif

  // These must match the order in QAR.h FIELD_NAMES
static FIELD_INFO FieldInfo[FN_MAX] = {
    // Control Word
    { 0, 1},  // Resync          C_RESYNC     
    { 1, 3},  // NumWords        C_NUM_WORDS  
    { 4, 1},  // Format          C_FORMAT     
    { 5, 1},  // Enable          C_ENABLE     
    { 6, 1},  // Tstn            C_TSTN       
    { 7, 1},  // Tstp            C_TSTP       
    // Status Word
    { 0, 1},  // FrameValid      S_FRAME_VALID 
    { 1, 1},  // LossOfFrameSync S_LOST_SYNC   
    { 2, 2},  // SubFrame        S_SUBFRAME    
    { 4, 1},  // DataPresent     S_DATA_PRESENT
    { 5, 1},  // BarkerErr       S_BARKER_ERR  
    { 6, 1},  // Biin            S_BIIN        
    { 7, 1},  // Biip            S_BIIP        
};


// Internal blocks commented out because the FPGA v12 has a bug when data is coming in
// during the powerup. (SCR 981)
#define FPGA_DPRAM_MAX_BLK 1 //3
#define FPGA_DPRAM_OP 1

// Internal blocks commented out because the FPGA v12 has a bug when data is coming in
// during the powerup. (SCR 981)
static const QAR_PBIT_INTERNAL_DPRAM FPGA_INTERNAL_DPRAM[FPGA_DPRAM_MAX_BLK] = {
  {SYS_QAR_PBIT_DPRAM, FPGA_QAR_SF_BUFFER1}//, 
  //{SYS_QAR_PBIT_INTERNAL_DPRAM0, FPGA_QAR_INTERNAL_SF_BUFFER1},
  //{SYS_QAR_PBIT_INTERNAL_DPRAM1, FPGA_QAR_INTERNAL_SF_BUFFER3}
}; 


#define QAR_SF1_BARKER 0x247
#define QAR_SF2_BARKER 0x5b8
#define QAR_SF3_BARKER 0xa47
#define QAR_SF4_BARKER 0xdb8


/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:     QAR_Initialize
 *               
 * Description:  QAR_Initialize 
 *                 1) Initializes internal local variables.
 *                 2) Performs PBIT of QAR LoopBack 
 *                 3) Performs Test of DPRAM 
 *                 4) Sets FPGA Shadow RAM to QAR startup default 
 *
 * Parameters:   SysLogId - ptr to the SYS_APP_ID
 *               pdata - ptr to data store to return PBIT fail data 
 *               psize - ptr to size var to return the size of PBIT fail data 
 *                       (*psize == 0 if DRV_OK is returned)
 *
 * Returns:      RESULT [DRV_OK] if PBIT successful 
 *               
 *               SYS_QAR_FPGA_BAD_STATE if FPGA status is bad
 *               QAR_PBIT_LOOPBACK if BIIP and BIIN loopback fails
 *               QAR_PBIT_DPRAM    if QARDATA[] memory test fails                
 *
 * Notes:        
 *  
 *  03/10/2008 PLee Updated to latest FPGA Revision 4
 *
 *****************************************************************************/
//RESULT QAR_Initialize (void)
RESULT QAR_Initialize ( SYS_APP_ID *SysLogId, void *pdata, UINT16 *psize)
{
   RESULT result; 
   QAR_STATE_PTR          pState;
   UINT8 *pdest; 
   
   
   pdest = (UINT8 *) pdata;
   pState = (QAR_STATE_PTR) &QARState;

   result = DRV_OK; 

   // Set the global pointers to the QAR registers
   pQARCtrlStatus      = (QAR_REGISTERS *) (FPGA_QAR_CONTROL);
   pQARSubFrame        = (QAR_DATA *) (FPGA_QAR_SF_BUFFER1);
   pQARCtrl            = &pQARCtrlStatus->Control;
   pQARStatus          = &pQARCtrlStatus->Status;

   // Clear Variables 
   memset ( (void *) &QARFrame, 0, sizeof(QAR_SUBFRAME_DATA) * QAR_SF_MAX );
   memset ( (void *) &QARFramePrevGood, 0, sizeof(QAR_SUBFRAME_DATA) * QAR_SF_MAX );
   memset ( (void *) &QARState, 0, sizeof(QAR_STATE) ); 
   memset ( (void *) &QARCfg, 0, sizeof(QAR_CONFIGURATION) ); 
   memset ( (void *) &QARCBITHealthStatus, 0, sizeof(QAR_CBIT_HEALTH_COUNTS)); 
   memset ( (void *) &QARWordInfo, 0, sizeof(QAR_WORD_INFO) * QAR_SF_MAX_SIZE);
   
   // PBIT FPGA Bad TBD
   if ( FPGA_GetStatus()->InitResult != DRV_OK ) 
   {
     result = SYS_QAR_FPGA_BAD_STATE; 
     *SysLogId = DRV_ID_QAR_PBIT_FPGA_FAIL;
   }
   
   // PBIT Test LOOPBACK 
   if (result == DRV_OK)
   {
     result = QAR_PBIT_LoopBack_Test(pdest, psize); 
     *SysLogId = DRV_ID_QAR_PBIT_LOOPBACK_FAIL;
   }
   
   // PBIT Test QARDATA[]
   if (result == DRV_OK) 
   {
     result = QAR_PBIT_DPRAM_Test(pdest, psize);
     *SysLogId = DRV_ID_QAR_PBIT_DPRAM_FAIL;
   }

   // Force restore of default startup configuration
   QAR_RestoreStartupCfg();  
   
   QAR_ReconfigureFPGAShadowRam(); 
   
   QAR_ResetState(); 

   // Set the overall system status    
   if ( result == DRV_OK ) 
   {
     pState->SystemStatus = QAR_STATUS_OK;       
   }
   else 
   {
     pState->SystemStatus = QAR_STATUS_FAULTED_PBIT; 
   }
   
   
   m_BarkerErr = FALSE; 
   
   return (result); 

}



/******************************************************************************
 * Function:     QAR_Init_Tasks
 *               
 * Description:  QAR System Initialization 
 *                  1) Restores User Configuration
 *                  2) Launches QAR System Tasks if User Cfg QAR to be enabled. 
 *                  3) Re-Configure the FPGA QAR shadow RAM and force these 
 *                     setting into the FPGA
 *
 * Parameters:   None
 *
 * Returns:      None
 *
 * Notes:        
 *  1) Expects QARState.Systemstatus to be initialized 
 *
 *****************************************************************************/
void QAR_Init_Tasks (void)
{
  TCB TaskInfo;
  QAR_SYS_PBIT_STARTUP_LOG StartupLog; 
  

  // Restore User Cfg 
  memcpy(&QARCfg, &(CfgMgr_RuntimeConfigPtr()->QARConfig), sizeof(QARCfg));

  QAR_ReconfigureFPGAShadowRam();  // If QARCfg->Enable == TRUE, will enb QAR and QAR INT !
  QAR_ResetState();   

  if ( ( QARCfg.Enable == TRUE ) && ( QARState.SystemStatus == QAR_STATUS_OK) )
  {
    // Create QAR Read Sub Frame Task
    memset(&TaskInfo, 0, sizeof(TaskInfo));
    strncpy_safe(TaskInfo.Name, sizeof(TaskInfo.Name),"QAR Read Sub Frame",_TRUNCATE);    
    TaskInfo.TaskID         = QAR_Read_Sub_Frame;
    TaskInfo.Function       = QAR_ReadSubFrameTask;
    TaskInfo.Priority       = taskInfo[QAR_Read_Sub_Frame].priority;
    TaskInfo.Type           = taskInfo[QAR_Read_Sub_Frame].taskType;
    TaskInfo.modes          = taskInfo[QAR_Read_Sub_Frame].modes;
    TaskInfo.MIFrames       = taskInfo[QAR_Read_Sub_Frame].MIFframes;    
    TaskInfo.Rmt.InitialMif = taskInfo[QAR_Read_Sub_Frame].InitialMif;  
    TaskInfo.Rmt.MifRate    = taskInfo[QAR_Read_Sub_Frame].MIFrate;     
    TaskInfo.Enabled        = TRUE;
    TaskInfo.Locked         = FALSE;
    TaskInfo.pParamBlock    = &QARState;
    TmTaskCreate (&TaskInfo);
    
    // Create QAR Monitor Task
    memset(&TaskInfo, 0, sizeof(TaskInfo));
    strncpy_safe(TaskInfo.Name, sizeof(TaskInfo.Name),"QAR Monitor",_TRUNCATE);
    TaskInfo.TaskID         = QAR_Monitor;
    TaskInfo.Function       = QAR_MonitorTask;
    TaskInfo.Priority       = taskInfo[QAR_Monitor].priority;
    TaskInfo.Type           = taskInfo[QAR_Monitor].taskType;
    TaskInfo.modes          = taskInfo[QAR_Monitor].modes;
    TaskInfo.MIFrames       = taskInfo[QAR_Monitor].MIFframes;     
    TaskInfo.Rmt.InitialMif = taskInfo[QAR_Monitor].InitialMif;       
    TaskInfo.Rmt.MifRate    = taskInfo[QAR_Monitor].MIFrate;   
    TaskInfo.Enabled        = TRUE;
    TaskInfo.Locked         = FALSE;
    TaskInfo.pParamBlock    = &QARState;
    TmTaskCreate (&TaskInfo);
  }
  
  // Update QAR Disabled status.  Note: DRV QAR_Initialize() must be called before this 
  //   function.
  if ( QARCfg.Enable == FALSE ) 
  {
    if (QARState.SystemStatus != QAR_STATUS_OK) 
    {
      QARState.SystemStatus = QAR_STATUS_DISABLED_FAULTED; 
    }
    else 
    {
      QARState.SystemStatus = QAR_STATUS_DISABLED_OK; 
    }
  }
  
  
  FPGA_Configure();   // Force QAR FPGA Shadow Ram values to loaded into FPGA 
  
  if ( (QARCfg.Enable == TRUE) && (QARState.SystemStatus == QAR_STATUS_FAULTED_PBIT) )
  {
    StartupLog.result = SYS_QAR_PBIT_FAIL; 
    Flt_SetStatus(QARCfg.PBITSysCond, SYS_ID_QAR_PBIT_DRV_FAIL, 
                  &StartupLog, sizeof(QAR_SYS_PBIT_STARTUP_LOG) );
  }
  
}

/******************************************************************************
 * Function:    QAR_Reframe
 *
 * Description: The QAR Reframe function will generate a 100 nanosecond
 *              reframe pulse.
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:       This code contains a 100 nanosecond delay.
 *
 *****************************************************************************/
void QAR_Reframe (void)
{
   // Force a Reframe
   QAR_W( pQARCtrl, C_RESYNC, QAR_FORCE_REFRAME);

   // The Reframe needs a 100 nSecond wait time
   TTMR_Delay(TICKS_PER_100nSec); 

   // Set the QAR back to Normal
   QAR_W( pQARCtrl, C_RESYNC, QAR_NORMAL_OPERATION);
}


/******************************************************************************
 * Function:     QAR_Reconfigure
 * 
 * Description:  The QAR_Reconfigure function will reprogram all the
 *               configuration registers of the FPGA.
 *
 * Parameters:   None
 *
 * Returns:      None
 *
 * Notes:        
 *  1) Reconfigure QAR based on current (or updated) QARCfg
 *
 *****************************************************************************/
void QAR_Reconfigure (void)
{
   QAR_ReconfigureFPGAShadowRam(); 
   QAR_ResetState(); 
   
   FPGA_Configure();   
}




/******************************************************************************
 * Function:     QAR_ProcessISR
 *
 * Description:  This function is called from the FPGA ISR and sets the 
 *               ->bGetNewSubFrame flag
 *
 * Parameters:   None
 *
 * Returns:      None
 *
 * Notes:        This function is called within Interrupt context and processing 
 *               time should be minimized. 
 *
 *****************************************************************************/
void QAR_ProcessISR(void)
{
   QAR_STATE *pState;
   UINT32 TickCount; 
   
   // Set pointer to QAR State Container
   pState = &QARState;

   TickCount = CM_GetTickCount();

   if ( QAR_R(pQARStatus, S_FRAME_VALID) && !QAR_R( pQARStatus, S_LOST_SYNC) )
   {
     if ( TickCount > (pState->LastIntTime + QAR_ALLOWED_INT_FREQ) )
     {
        pState->LastIntTime = TickCount; 
        pState->bGetNewSubFrame = TRUE; 
     }
     else 
     {
        // INT Occurred faster than 1 Hz !!!  
        pState->BadIntFreqCnt++; 
     }
     
     // Update Barker Err Indication
     m_BarkerErr = (QAR_R( pQARStatus, S_BARKER_ERR) == QSR_BARKER_ERROR_VAL); 
     
   }
   else
   {
     if ( QAR_R( pQARStatus, S_LOST_SYNC) ) 
     {
        pState->InterruptCntLOFS++; 
     }
   }
 
   pState->InterruptCnt++; 
}


/******************************************************************************
 * Function:     QAR_ReadSubFrameTask
 *
 * Description:  This function processes the latest received QAR sub-frame. 
 *
 * Parameters:   pParam - Ptr to QAR_STATE_PTR
 *
 * Returns:      None
 *
 * Notes: 
 *       
 *  1) A fault mode is single BIT error -> single BARKER Err -> if another 
 *     BARKER Err leads -> Out of Sync and BARKER Err only cleared thereafter ? 
 *  2) A fault mode where single BIT error -> single BARKER Err -> if 
 *     next BARKER is good, then BARKER Err does not clear until end of FRAME
 *
 *  3) TBD - confirm that we keep SF from all subsequent BARKER error indications
 *     is good. 
 *  
 *****************************************************************************/
static 
void QAR_ReadSubFrameTask( void *pParam )
{
   // Local Data
   UINT16    *pSrc;
   UINT16    i;
   QAR_SF    NewFrame;
   UINT32    CurrentFrame;
   QAR_STATE_PTR pState;
   UINT16        *pQARFrameWord;
   QAR_CONFIGURATION_PTR  pQARCfg;
   

   // If shutdown is signaled,
   // tell Task Mgr to disable my task.
   if (Tm.systemMode == SYS_SHUTDOWN_ID)
   {
     QAR_ReadSubFrameTaskShutdown();
     TmTaskEnable(QAR_Read_Sub_Frame, FALSE);
   }
   else // Normal task execution.
   {
     // Set pointer to QAR State Container
     pState = (QAR_STATE_PTR) pParam;

     // Check that the Frame is Valid before storing
     if ( QAR_R(pQARStatus, S_FRAME_VALID) )
     {
       pState->bChanActive = TRUE;
       pState->FrameSync   = TRUE;
       pState->LossOfFrame = FALSE;

       // Must wait to see if the next frame loses sync on barker 
       // before saving current sub-frame
       if ( ( pState->bGetNewSubFrame == TRUE ) && 
           (( CM_GetTickCount() - pState->LastIntTime ) > LOFS_TIMEOUT_MS))
       {
         // Process the sub-frame and reset the flag
         pState->bGetNewSubFrame = FALSE; 
         
         // Get the Current Frame being captured in the FPGA
         CurrentFrame = QAR_R( pQARStatus, S_SUBFRAME);
         
         // Now determine which frame is ready for processing
         if ( CurrentFrame == QAR_SF1)
         {
           // Set the new frame to Sub-Frame 4
           NewFrame = QAR_SF4;
         }
         else
         {
           // The valid sub-frame is one less than what is
           // in the status register
           NewFrame = (QAR_SF)(CurrentFrame - 1);
         }

         // If the frame is 1 or 3 then copy buffer 1
         if ( (NewFrame == QAR_SF1) ||  (NewFrame == QAR_SF3 ) )
         {
           pSrc = (UINT16*)&pQARSubFrame->Buffer1;
         }
         else // the frame is 2 or 4 copy buffer 2
         {
           pSrc = (UINT16*)&pQARSubFrame->Buffer2;
         }

         pQARFrameWord = (UINT16 *) &QARFrame[NewFrame].Word[0]; 
         
         // Validate that Barker Code is what we want ! 
         // If we have LOFS, then 1 in 4 chance that we will encounter a Barker Code mismatch.
         pQARCfg = (QAR_CONFIGURATION_PTR) &QARCfg;
         if (*pSrc != pQARCfg->BarkerCode[NewFrame] ) 
         {
           GSE_DebugStr(NORMAL,
                  TRUE,
                  "QAR_ReadSubFrame: Barker Code Mismatch (exp=%04X,act=%04X)\r\n", 
                  pQARCfg->BarkerCode[NewFrame], 
                  *pSrc);
         }
         
         // Copy data from QAR Buffer to storage container
         for (i=0; i < pState->TotalWords; i++)
         {
           *(pQARFrameWord + i) = *pSrc++; 
         }

         // Before we continue with the data copied make sure we didn't lose frame
         // sync and the frame is valid because as soon as we lose frame sync the 
         // FPGA changes the SubFrame Number and starts to use the software buffers
         // This check will stop the software from using bad data and recording bad data
         if ( QAR_R(pQARStatus, S_FRAME_VALID) && !QAR_R( pQARStatus, S_LOST_SYNC) )
         {
            // It appears the data is still GOOD, Now Check if we have single Bit Error 
            // NOTE: If we get consecutive BARKER Error in a row then all subsequent 
            //       barker error, the SF data "should" be good.  If SF data was "bad" 
            //       we would have Loss Frame Sync and never get to this point. 
            if ( m_BarkerErr == TRUE )
            {
              if (pState->BarkerError == FALSE ) 
              {
                // Mark Status as "potentially" bad current SF 
                pState->bSubFrameOk[NewFrame] = FALSE; 
                // Note: If we loose sync after the 2nd bad BARKER we should 
                //  go out of sync.  When we get back into sync, this 
                //  SF data should still be marked "suspect" until 
                //  a new "potentially" good SF is received. 
                pState->BarkerError = TRUE; 
                pState->BarkerErrorCount++; 
              }
              else 
              {
                // If we are still in sync on subsequent BARKER ERROR then 
                //  this SF is "potential" good.  Assume so. 
                pState->bSubFrameOk[NewFrame] = TRUE; 
                // Update Time 
                // pState->LastSubFrameUpdateTime[NewFrame] = CM_GetTickCount(); 
                QARFrame[NewFrame].LastSubFrameUpdateTime = CM_GetTickCount(); 
                // Update Last Good Sub Frame Read
                pState->PreviousSubFrameOk = NewFrame; 
              }
            }
            else 
            {
              // Mark Status as "potentially" good current SF 
              pState->bSubFrameOk[NewFrame] = TRUE; 
              // Update Time 
              QARFrame[NewFrame].LastSubFrameUpdateTime = CM_GetTickCount(); 
              // Update Last Good Sub Frame Read
              pState->PreviousSubFrameOk = NewFrame; 
              pState->BarkerError = FALSE; 
            }

            // If ->bSubFrameOk[] == TRUE then copy this data to "shadow" for 
            //   processing in QAR_ReadWord() where last good data is always returned. 
            if ( pState->bSubFrameOk[NewFrame] == TRUE ) 
            {
              memcpy ((void *) &QARFramePrevGood[NewFrame].Word[0], 
                      (void *) &QARFrame[NewFrame].Word[0], pState->TotalWords * sizeof(UINT16));
              QARFramePrevGood[NewFrame].LastSubFrameUpdateTime = CM_GetTickCount(); 
              pState->bSubFrameReceived[NewFrame] = TRUE; 
            }

            // Update the state data, this is what is used by the GetSnap and GetSubFrame
            pState->PreviousFrame = pState->CurrentFrame;
            pState->CurrentFrame  = NewFrame;
            
         } // End of if -> S_FRAME_VALID && !S_LOST_SYNC
       } // End of if ->bGetNewSubFrame == TRUE
     } // End of if ->FrameValid == TRUE
     else 
     {
       pState->FrameSync       = FALSE;
       pState->bGetNewSubFrame = FALSE;
       // FPGA indicates LossOfFrameSync only after SYNC has been established 
       if ( QAR_R( pQARStatus, S_LOST_SYNC) )
       {
         // Only on transition from SYNC OK to SYNC BAD
         if (pState->LossOfFrame == FALSE) 
         {
           // Reset frame received because we don't want stale data for 
           // the start-snap when we resync
           pState->bSubFrameReceived[QAR_SF1] = FALSE; 
           pState->bSubFrameReceived[QAR_SF2] = FALSE;
           pState->bSubFrameReceived[QAR_SF3] = FALSE;
           pState->bSubFrameReceived[QAR_SF4] = FALSE;
 
           pState->LossOfFrame       = TRUE;
           pState->LossOfFrameCount++;  // Cnt instances of loss of frame sync
           pState->CurrentFrame      = QAR_SF4;
           pState->PreviousFrame     = QAR_SF4;
           pState->LastServiced      = QAR_SF4;
           pState->BarkerError       = FALSE;
           m_BarkerErr               = FALSE; 
         } // End of ->LossOfFrame == FALSE 
       } // End of ->LossOfFrameSync indicates TRUE from FPGA
     } // End of else FPGA indicates No Frame Sync
      
   } // Normal task execution.  
   
}

/******************************************************************************
* Function:    QAR_ReadSubFrameTaskShutdown
*
* Description: Shutdown function for QAR_ReadSubFrameTask. This function
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
static 
void QAR_ReadSubFrameTaskShutdown(void)
{
  // Set config to indicate disabled. 
  QARCfg.Enable = FALSE;

  // Update FPGA Shadow RAM
  QAR_ReconfigureFPGAShadowRam();
  
  // Configure the FPGA Shadow RAM
  FPGA_Configure();
}

/******************************************************************************
 * Function:     QAR_GetSubFrame
 *
 * Description:  The QARGetFrame function is used to get the latest QAR
 *               Frame Received
 *
 * Parameters:   pDest - Ptr to destination buffer (MUST BE WORD ALIGNED !)
 *               nop - A "nop" param, used to create same prototype as Arinc stuff
 *               nMaxByteSize - The max byte size of destination buffer
 *
 * Returns:      nCnt - Number of bytes copied to dest buffer 
 *                      Note: QAR words are 16 bits, thus 2 byte / QAR Word
 *
 * Notes:        
 *    1) Returns the most recent SubFrame that has not be "serviced" / "read"
 *    2) Func should be called at the subframe rate
 *
 * Rev: 
 *  07/31/2009 Return byte cnt rather than QAR word cnt 
 *
*****************************************************************************/ 
UINT16 QAR_GetSubFrame ( void *pDest, UINT32 nop, UINT16 nMaxByteSize )
{
   UINT16        *pbuf; 
   UINT16        nMaxWordSize;  
   QAR_STATE_PTR pState; 
   UINT16        nCnt; 
   
   
   // ASSERT() for when pDest != WORD ALIGNED !!!
   ASSERT ( ((UINT32)pDest & 0x1) != 0x01 );
   
   nCnt = 0; 
   pState = (QAR_STATE_PTR) &QARState; 
   
   if ( pState->LastServiced != pState->CurrentFrame ) 
   {
     pbuf = (UINT16 *) pDest; 
     nMaxWordSize = nMaxByteSize / sizeof(UINT16); 

     nCnt = nMaxByteSize;    
     if ( pState->TotalWords <= nMaxWordSize ) 
     {
        nCnt = pState->TotalWords * sizeof(UINT16); 
     }
     
     memcpy ( (void *) pbuf, (void *) &QARFrame[pState->CurrentFrame].Word[0], 
              nCnt ); 
     pState->LastServiced = pState->CurrentFrame; 
   }
   
   // nCnt = nCnt / sizeof(UINT16);   // Change back to word count from byte count ! 
   
   return ( nCnt ); 
   
}


/******************************************************************************
 * Function:     QAR_GetSubFrameDisplay 
 *
 * Description:  The QARGetFrame function is used to get the latest QAR
 *               Frame Recieved for Display Purposes 
 *
 * Parameters:   pDest - Ptr to destination buffer
 *               *lastServiced - Ptr to Last Serviced SF
 *               nMaxByteSize - The max byte size of destination buffer
 *
 * Returns:      nCnt - Number of bytes copied to dest buffer 
 *                      Note: QAR words are 16 bits, thus 2 byte / QAR Word
 *
 * Notes:       
 *   1) Unlike QAR_GetSubFrame() does not modify internal QARState.  Expects
 *      "lastServiced" var to be passed in.  
 *   2) TBD At some point should combine QAR_GetSubFrameDisplay() and 
 *      QAR_GetSubFrame() for now since Data Mgr not aware of this parameter
 *      create this 2nd func
 *
 * Rev: 
 *  07/31/2009 Return byte cnt rather than QAR word cnt 
 *
*****************************************************************************/ 
UINT16 QAR_GetSubFrameDisplay ( void *pDest, QAR_SF *lastServiced, UINT16 nMaxByteSize )
{
   UINT16        *pbuf; 
   UINT16        nMaxWordSize;  
   QAR_STATE_PTR pState; 
   UINT16        nCnt; 
   
   // ASSERT() when pDest != WORD ALIGNED !!!
   ASSERT ( ((UINT32)pDest & 0x1) != 0x01 );
   
   nCnt = 0; 
   pState = (QAR_STATE_PTR) &QARState; 
   
   if ( *lastServiced != pState->CurrentFrame ) 
   {
     pbuf = (UINT16 *) pDest; 
     nMaxWordSize = nMaxByteSize / sizeof(UINT16); 
   
     nCnt = nMaxByteSize; 
     if ( pState->TotalWords <= nMaxWordSize ) 
     {
        nCnt = pState->TotalWords * sizeof(UINT16); 
     }
     memcpy ( (void *) pbuf, (void *) &QARFrame[pState->CurrentFrame].Word[0], 
              nCnt ); 
     *lastServiced = pState->CurrentFrame; 
   }
   
   // nCnt = nCnt / sizeof(UINT16);   // Change back to word count from byte count ! 
   
   return ( nCnt ); 
   
}

/******************************************************************************
 * Function:     QAR_GetFrameSnapShot
 *
 * Description:  Returns current value of the last Four SubFrame
 *
 * Parameters:   pDest - Ptr to destination buffer
 *               nop - dummy var to allow common func() param list 
 *               nMaxByteSize - The max byte size of destination buffer
 *               bStartSnap - TRUE is begin snapshot, FALSE if end snapshot
 *
 * Returns:      nCnt - Number of bytes copied to dest buffer 
 *                      Note: QAR words are 16 bits, thus 2 byte / QAR Word
 *
 * Notes:      
 *   1) Returns snapshot of all 4 subframe.  If no subframes have been received 
 *      "0" (the initialization) value will be returned.  
 *
 * Rev: 
 *  07/31/2009 Return byte cnt rather than QAR word cnt 
 *
*****************************************************************************/ 
UINT16 QAR_GetFrameSnapShot ( void *pDest, UINT32 nop, UINT16 nMaxByteSize, 
                              BOOLEAN bStartSnap )
{
  UINT16        *pbuf; 
  UINT16        nMaxWordSize;  
  QAR_STATE_PTR pState;
  UINT16        nCnt; 
  
  QAR_SF        nSubFrame[QAR_SF_MAX];
  UINT16        i; 
  UINT16        j; 
  SINT16        currentSF; 
  UINT16        nMaxDestBuffWordSize; 


   // ASSERT() when pDest != WORD ALIGNED !!!
  ASSERT ( ((UINT32)pDest & 0x1) != 0x01 );

  pState = (QAR_STATE_PTR) &QARState; 
  pbuf = (UINT16 *) pDest; 
  nMaxWordSize = nMaxByteSize / sizeof(UINT16); 
  
  // From current Frame (4 SubFrames) move backwards to determine the last four 
  //   SubFrames 
  currentSF = (SINT16) pState->CurrentFrame; 
  for (i=0;i<QAR_SF_MAX;i++) 
  {
    nSubFrame[QAR_SF4 - i] = (QAR_SF) currentSF--; 
    // Handle Wrap Case 
    if (currentSF < 0) 
    {
      currentSF = QAR_SF4; 
    }
  }
  
  // Determine Max Word Size to Return 
  nMaxDestBuffWordSize = nMaxWordSize; 
  if ( nMaxWordSize > (QAR_SF_MAX * pState->TotalWords) ) 
  {
    nMaxDestBuffWordSize = (QAR_SF_MAX * pState->TotalWords); 
  }
  
  // Copy all four (or as much as possible) into the Dest Buffer 
  nCnt = 0; 
  i = 0; 
  while ( (i < QAR_SF_MAX) && (nCnt < nMaxDestBuffWordSize) )
  {
    // SCR #29
    // Only if Sub Frame has been received will we return the SF 
    if ( pState->bSubFrameReceived[nSubFrame[i]] == TRUE ) 
    {
      if ( (nCnt + pState->TotalWords) < nMaxDestBuffWordSize ) 
      {
        // Copy entire sub frame 
        j = pState->TotalWords; 
      }
      else 
      {
        // Copy only partial sub frame 
        j = nMaxDestBuffWordSize - nCnt; 
      }
      // Copy last sub frame based on nSubFrame[] calc above to determine the 
      //    last four sub frames. 
      memcpy ( (void *) (pbuf + nCnt), (void *) &QARFrame[nSubFrame[i]].Word[0], 
               j * sizeof(UINT16) ); 
      nCnt = nCnt + j; 
    }
    i++; 
  } // End for Loop thru four SF or DestBuffSize ! 
  
  return ( nCnt * sizeof(UINT16) );   // nCnt == Count byte copied to buffer ! 

} // End of function 

/******************************************************************************
 * Function:    QAR_SyncTime
 *
 * Description: Dummy function used by generic calling App 
 *              To be completed in the future
 *              
 * Parameters:  cha - QAR channel 
 *
 * Returns:     none
 *
 * Notes:       none 
 *
 *****************************************************************************/
void QAR_SyncTime( UINT32 chan )
{
   
}

/******************************************************************************
 * Function:     QAR_GetFileHdr
 *
 * Description:  Dummy file to support generic calling App function 
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
UINT16 QAR_GetFileHdr ( void *pDest, UINT32 chan, UINT16 nMaxByteSize )
{
   // Local Data
   QAR_CHAN_HDR          ChanHdr;
   INT8                  *pBuffer;
   QAR_CONFIGURATION_PTR pCfg;
   
   // Initialize Local Data
   pBuffer   = (INT8 *)(pDest) + sizeof(ChanHdr);  // Move to end of Channel File Hdr
   pCfg    = &QARCfg;
   
   // Update Chan file hdr
   memset ( &ChanHdr, 0, sizeof(ChanHdr) );
  
   ChanHdr.TotalSize = sizeof(ChanHdr); 
   strncpy_safe( ChanHdr.Name, sizeof(ChanHdr.Name), pCfg->Name, _TRUNCATE );
   ChanHdr.Protocol      = QAR_RAW;
   ChanHdr.NumWords      = pCfg->NumWords;
   ChanHdr.BarkerCode[0] = pCfg->BarkerCode[0];
   ChanHdr.BarkerCode[1] = pCfg->BarkerCode[1];
   ChanHdr.BarkerCode[2] = pCfg->BarkerCode[2];
   ChanHdr.BarkerCode[3] = pCfg->BarkerCode[3];
  
   // Copy Chan file hdr to destination 
   pBuffer = pDest; 
   memcpy ( pBuffer, &ChanHdr, sizeof(ChanHdr) ); 
  
   return (ChanHdr.TotalSize);
}

/******************************************************************************
 * Function:     QAR_GetSystemHdr
 *
 * Description:  Retrieves the binary system header for the QAR
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
UINT16 QAR_GetSystemHdr ( void *pDest, UINT16 nMaxByteSize )
{
   // Local Data
   QAR_SYS_HDR QARSysHdr;
   INT8        *pBuffer;
   UINT16      nTotal;
   
   // Initialize Local Data   
   pBuffer    = (INT8 *)pDest;
   nTotal     = 0;
   memset ( &QARSysHdr, 0, sizeof(QARSysHdr) );
   // Check to make sure there is room in the buffer
   if ( nMaxByteSize > sizeof (QARSysHdr) )
   {
      // Copy the QAR name
      strncpy_safe( QARSysHdr.Name, sizeof(QARSysHdr.Name), QARCfg.Name, _TRUNCATE );
      // Increment the size
      nTotal += sizeof (QARSysHdr);
      // Copy the header to the buffer
      memcpy ( pBuffer, &QARSysHdr, nTotal );
   }
   // return the total written
   return ( nTotal );
}

/******************************************************************************
 * Function:     QAR_MonitorTask
 *
 * Description:  QAR Monitor is a CBIT function which checks for
 *               Loss of Frame or Frame Invalid.
 *
 * Parameters:   pParam - ptr to QAR_STATE_PTR
 *
 * Returns:      None.
 * 
 * Notes:
 *  1) Update ->FrameState based on ->FrameSync and ->LossOfFrame which 
 *     are read in QAR_ReadSubFrame() 
 *  2) Monitor for Activity Data Present 
 *  3) NOTE: Barker Error and Loss of Frame logic in QAR_ReadSubFrame() 
 *
 *****************************************************************************/
static 
void QAR_MonitorTask ( void *pParam )
{
   QAR_CONFIGURATION_PTR pQarCfg; 
   QAR_STATE    *pState;
   UINT32       nTimeOut; 
   BOOLEAN      *pTimeOutFlag; 
   QAR_SUBFRAME_DATE_PTR pQARFrame; 
   UINT16       i; 
   UINT32       TempData;
   UINT16       dataPresent; 
   
   BOOLEAN      bStatusOk; 
   
   
   // Set Pointer to State Data
   // pState = &QARState;
   pState = (QAR_STATE_PTR) pParam; 
   
   bStatusOk = TRUE; 
   
   // Read QAR Data activity flag if FPGA Status is OK, otherwise 
   //    flag as no activity.  
   // Should not access FPGA if it is reconfiguring / reconfigure failed, 
   //    otherwise read could give erroneous results. 
   if ( FPGA_GetStatus()->SystemStatus == FPGA_STATUS_OK ) 
   {
     dataPresent = QAR_R( pQARStatus, S_DATA_PRESENT); 
   }
   else 
   {
     dataPresent = QSR_DATA_NOT_PRESENT_VAL; 
   }
   
   // Only if QAR is enabled and not faulted PBIT shall QAR CBIT Monitor Task 
   // process occur 
   if ( ( pState->bQAREnable == TRUE ) && 
        ( pState->SystemStatus != QAR_STATUS_FAULTED_PBIT) )
   {
     // Monitor for Activity Data Present 
     if ( dataPresent == QSR_DATA_PRESENT_VAL ) 
     {
        // Update Internal Data Present Flag 
        if ( pState->DataPresent == FALSE )
        {
           LogWriteSystem (SYS_ID_QAR_DATA_PRESENT, LOG_PRIORITY_LOW, &TempData, 0, NULL);
           pState->DataPresent = TRUE; 
           GSE_DebugStr(NORMAL,TRUE,"QAR System Log: QAR Data Present Detected");
        }
     }
     else 
     {
        // Update Internal Data Present Flag 
        if ( pState->DataPresent == TRUE )
        {
           LogWriteSystem (SYS_ID_QAR_DATA_LOSS, LOG_PRIORITY_LOW, &TempData, 0, NULL);
           pState->DataPresent = FALSE; 
           GSE_DebugStr(NORMAL,TRUE,"QAR System Log: QAR Data Present Lost");
           
           pState->DataPresentLostCount++; 
        }
        
     } // End of else ->DataPresent indicate data not present

     // Update TimeOut Condition 
     pQarCfg = (QAR_CONFIGURATION_PTR) &QARCfg; 
     
     // Channel Data Loss Determination 
     // Note: FPGA when data activity is loss, FPGA might not update FrameSync Flag !
     //       Should qualify Chan Loss as absent of either FrameSync or Data Present ! 
     if ( (pState->FrameSync == FALSE) || 
          (dataPresent != QSR_DATA_PRESENT_VAL ) )
     {

        // Determine StartUp or Regular TimeOut 
        if (pState->LastActivityTime != 0) 
        {
          // Activity received, check for subsequent channel loss
          nTimeOut = pQarCfg->ChannelTimeOut_s; 
          pTimeOutFlag = (BOOLEAN *) &pState->bChannelTimeOut; 
        }
        else 
        {
          // Activity never received, check for startup 
          nTimeOut = pQarCfg->ChannelStartup_s;
          pTimeOutFlag = (BOOLEAN *) &pState->bChannelStartupTimeOut; 
        }
        
        if ( ((CM_GetTickCount() - pState->LastActivityTime) > 
              (nTimeOut * MILLISECONDS_PER_SECOND)) && (nTimeOut != QAR_DSB_TIMEOUT) )
        {
          if (*pTimeOutFlag != TRUE ) 
          {
            // Set Startup TimeOut Flag or Channel Loss TimeOut Flag 
            *pTimeOutFlag = TRUE; 
            // NOTE: Flag set in QAR processing, cleared by read of current state 
            
            // Record timeout system log 
            if (pTimeOutFlag == &pState->bChannelTimeOut)
            {
              QAR_CreateTimeOutSystemLog(SYS_QAR_DATA_LOSS_TIMEOUT); 
              GSE_DebugStr(NORMAL,TRUE,"QAR System Log: QAR Data Loss Timeout");
            }
            else 
            {
              QAR_CreateTimeOutSystemLog(SYS_QAR_STARTUP_TIMEOUT);
              GSE_DebugStr(NORMAL,TRUE,"QAR System Log: QAR Startup Timeout"); 
            }
            pState->bChanActive = FALSE;
            
          } // End if ( *pTimeOutFlag != TRUE
          bStatusOk = FALSE;  // We have CBIT fault at this point ! 
        } // End if ->LastActivityTime > nTimeOut Value occurred 
     
     } // End of Frame Not Sync or Data Not Present 
     else 
     {
        pState->LastActivityTime = CM_GetTickCount();
        
        // Reset Flt_Status() if timeout condition had occurred 
        if ( (pState->bChannelStartupTimeOut == TRUE) || (pState->bChannelTimeOut == TRUE) ) 
        {
          Flt_ClrStatus(pQarCfg->ChannelSysCond); 
        }
        pState->bChannelStartupTimeOut = FALSE; 
        pState->bChannelTimeOut = FALSE; 
     }
          
     
     // Channel Loss of Sync Determination 
     if ( (pState->LossOfFrame == TRUE) &&
          (pState->FrameState  != QAR_LOSF) )
     {
        LogWriteSystem (SYS_ID_QAR_SYNC_LOSS, LOG_PRIORITY_LOW, &TempData, 0, NULL);
        pState->FrameState = QAR_LOSF;
        GSE_DebugStr(NORMAL,TRUE,"QAR System Log: QAR Loss of Frame Sync Detected");
     }
     else if ( (pState->FrameSync == TRUE) && 
               (pState->FrameState != QAR_SYNCD) )
     {
        LogWriteSystem (SYS_ID_QAR_SYNC, LOG_PRIORITY_LOW, &TempData, 0, NULL);
        pState->FrameState = QAR_SYNCD;
        GSE_DebugStr(NORMAL,TRUE,"QAR System Log: QAR Frame Sync Detected");
        SystemLogResetLimitCheck(SYS_ID_QAR_DATA_PRESENT);
        SystemLogResetLimitCheck(SYS_ID_QAR_DATA_LOSS);
        
        // We shall start the Data Loss timeout on QAR SYNC rather than QAR Data Detected 
        if ( pState->bFrameSyncOnce == FALSE ) 
        {
           pState->bFrameSyncOnce = TRUE;  // We have synced at least once ! 
           // Reset TimeOuts
           pQARFrame = (QAR_SUBFRAME_DATE_PTR) &QARFrame[QAR_SF1]; 
           for (i=0;i<QAR_SF_MAX;i++) 
           {
             pQARFrame->LastSubFrameUpdateTime = pState->LastActivityTime; 
             pQARFrame++; 
           }
           pQARFrame = (QAR_SUBFRAME_DATE_PTR) &QARFramePrevGood[QAR_SF1]; 
           for (i=0;i<QAR_SF_MAX;i++) 
           {
             pQARFrame->LastSubFrameUpdateTime = pState->LastActivityTime; 
             pQARFrame++; 
           }
        }
     } // End if SYNC transition was obtained
     
     // Update Real time Status of QAR 
     if (bStatusOk != TRUE) 
     {
       pState->SystemStatus = QAR_STATUS_FAULTED_CBIT; 
     }
     else 
     {
       pState->SystemStatus = QAR_STATUS_OK; 
     }
     
   } // QAR is enabled and QAR has not faulted PBIT   
}


/******************************************************************************
 * Function:     QAR_CBITHealthStatus
 *
 * Description:  Returns the current CBIT Health status of the QAR
 *
 * Parameters:   None.
 *
 * Returns:      Current Data in QARCBITHealthStatus
 * 
 * Notes:        None.
 *
 *****************************************************************************/
QAR_CBIT_HEALTH_COUNTS QAR_GetCBITHealthStatus ( void )
{
   QARCBITHealthStatus.SystemStatus = QARState.SystemStatus; 
   QARCBITHealthStatus.bQAREnable = QARState.bQAREnable; 
   QARCBITHealthStatus.FrameState = QARState.FrameState; 
   QARCBITHealthStatus.DataPresentLostCount = QARState.DataPresentLostCount; 
   QARCBITHealthStatus.LossOfFrameCount = QARState.LossOfFrameCount;
   QARCBITHealthStatus.BarkerErrorCount = QARState.BarkerErrorCount; 
   
   return (QARCBITHealthStatus); 
}


/******************************************************************************
 * Function:     QAR_CalcDiffCBITHealthStatus
 *
 * Description:  Calc the difference in CBIT Health Counts to support PWEH SEU
 *               (Used to support determining SEU cnt during data capture) 
 *
 * Parameters:   PrevCnt - Initial count value
 *
 * Returns:      Diff in CBIT_HEALTH_COUNTS from (Current - PrevCnt)
 * 
 * Notes:        None.
 *
 *****************************************************************************/
QAR_CBIT_HEALTH_COUNTS QAR_CalcDiffCBITHealthStatus ( QAR_CBIT_HEALTH_COUNTS PrevCount )
{
  QAR_CBIT_HEALTH_COUNTS DiffCount; 
  QAR_CBIT_HEALTH_COUNTS *pCurrentCount; 
  
  QAR_GetCBITHealthStatus(); 
  
  pCurrentCount = &QARCBITHealthStatus; 
  
  DiffCount.SystemStatus = pCurrentCount->SystemStatus; 
  DiffCount.bQAREnable = pCurrentCount->bQAREnable;
  DiffCount.FrameState = pCurrentCount->FrameState; 
  DiffCount.DataPresentLostCount = pCurrentCount->DataPresentLostCount 
                                   - PrevCount.DataPresentLostCount; 
  DiffCount.LossOfFrameCount = pCurrentCount->LossOfFrameCount 
                               - PrevCount.LossOfFrameCount; 
  DiffCount.BarkerErrorCount = pCurrentCount->BarkerErrorCount 
                               - PrevCount.BarkerErrorCount; 
  
   return (DiffCount); 
}


/******************************************************************************
 * Function:     QAR_AddPrevCBITHealthStatus
 *
 * Description:  Add CBIT Health Counts to support PWEH SEU
 *               (Used to support determining SEU cnt during data capture) 
 *
 * Parameters:   CurrCnt - Current count value
 *               PrevCnt - Prev count value
 *
 * Returns:      Add CBIT_HEALTH_COUNTS from (CurrCnt + PrevCnt)
 *
 * Notes:        None
 *
 *****************************************************************************/
QAR_CBIT_HEALTH_COUNTS QAR_AddPrevCBITHealthStatus ( QAR_CBIT_HEALTH_COUNTS CurrCnt, 
                                                     QAR_CBIT_HEALTH_COUNTS PrevCnt )
{
  QAR_CBIT_HEALTH_COUNTS AddCount; 
  
  AddCount.SystemStatus = CurrCnt.SystemStatus; 
  AddCount.bQAREnable = CurrCnt.bQAREnable;
  AddCount.FrameState = CurrCnt.FrameState; 
  AddCount.DataPresentLostCount = CurrCnt.DataPresentLostCount 
                                   + PrevCnt.DataPresentLostCount; 
  AddCount.LossOfFrameCount = CurrCnt.LossOfFrameCount 
                               + PrevCnt.LossOfFrameCount; 
  AddCount.BarkerErrorCount = CurrCnt.BarkerErrorCount 
                               + PrevCnt.BarkerErrorCount; 
  
  return (AddCount); 
}


/******************************************************************************
 * Function:     QAR_GetRegisters
 *
 * Description:  Returns the contents of the physical QAR FPGA Control Register
 *
 * Parameters:   Registers - ptr to QAR_REGISTERS
 *
 * Returns:      None.
 * 
 * Notes:        None.
 *
 *****************************************************************************/
void QAR_GetRegisters (QAR_REGISTERS *Registers)
{
   *Registers = *pQARCtrlStatus;
}


/******************************************************************************
 * Function:     QAR_GetState
 *
 * Description:  Returns the address of the QARState variable
 *
 * Parameters:   None.
 *
 * Returns:      Ptr to the QARState variable
 * 
 * Notes:        None. 
 *
 *****************************************************************************/
QAR_STATE_PTR QAR_GetState ( void )
{
   return ( (QAR_STATE_PTR) &QARState ); 
}

/******************************************************************************
 * Function:     QAR_GetCfg
 *
 * Description:  Returns the address of the QARCfg data 
 *
 * Parameters:   None. 
 *
 * Returns:      Ptr to QARCfg
 *
 * Notes:        None. 
 *
 *****************************************************************************/
QAR_CONFIGURATION_PTR QAR_GetCfg ( void )
{
  return ( (QAR_CONFIGURATION_PTR) &QARCfg ); 
}


/******************************************************************************
 * Function:     QAR_SetCfg 
 *
 * Description:  Set QARCfg
 *
 * Parameters:   Cfg - ptr to the QAR_CONFIGURATION
 *
 * Returns:      None.
 * 
 * Notes:        None. 
 *
 *****************************************************************************/
void QAR_SetCfg (QAR_CONFIGURATION *Cfg)
{
   QARCfg = *Cfg;
}


/******************************************************************************
 * Function:     QAR_SensorSetup
 *
 * Description:  Decodes the GPA and GPB setting for QAR word sensor setup
 *
 * Parameters:   GPA - General Purpose A 
 *               GPB - General Purpose B
 *               nSensor - The sensor requesting this word parameter definition
 *
 * Returns:      Index into QARWordInfo[]
 *               (To be used in later QAR Sensor Processing)
 *
 * Notes:
 *  gpA
 *  ===
 *    bits 0 to 3:  Word Size
 *    bits 4 to 7:  MSB Position within word (most will be bit 11 for 12 bit QAR word) 
 *    bits 8 to 11: bit  8 Set = Word available in SF 1 
 *                  bit  9 Set = Word available in SF 2
 *                  bit 10 set = Word available in SF 3
 *                  bit 11 set = Word available in SF 4
 *    bits 12: 1 = Signed Value with MSB == Signed Bit, 0 = Not Signed value, MSB == MSB data
 *    bits 13: for Signed Value, 1 = 2's Complement value, 0 = Positive logic with signed bit
 *    bits 14 to 15: Not Used
 *    bits 16 to 27: Word Position within Sub Frame (max is 1023) (12 bits), 
                     location 0 = offset 0 or Barker Code
 *    bits 28 to 31: Not Used (4 bits) 
 *  gpB
 *  ===
 *    bits 0 to 15: Data Loss TimeOut Value (in msec) 
 *    bits 16 to 31: Not Used
 *
 *****************************************************************************/
UINT16 QAR_SensorSetup (UINT32 gpA, UINT32 gpB, UINT16 nSensor)
{
   QAR_WORD_INFO_PTR pWordInfo;
   QAR_SF sf; 
   UINT16 Cnt; 
   
   Cnt = QARWordSensorCount;
   pWordInfo = (QAR_WORD_INFO_PTR) &QARWordInfo[Cnt]; 
   
   // Parse gpA field 
   pWordInfo->WordSize     = (UINT16)(gpA & 0x0000000FUL);
   pWordInfo->MSBPosition  = (UINT16)(gpA >> 4 ) & 0x0000000FUL; 
   pWordInfo->WordLocation = (UINT16)(gpA >> 16) & 0x00000FFFUL; 
   for (sf=QAR_SF1;sf<QAR_SF_MAX;sf++) 
   {
     if ( ((gpA >>(8+(UINT32)sf)) & (0x00000001UL)) != 0x00 ) 
     {
       pWordInfo->bSubFrameData[sf] = TRUE; 
     }
     else 
     {
       pWordInfo->bSubFrameData[sf] = FALSE; 
     }
   }
   
   if ( ((gpA >> 12) & (0x00000001UL)) != 0x00 ) 
   {
      pWordInfo->SignBit = TRUE; 
   }
   else 
   {
      pWordInfo->SignBit = FALSE; 
   }
   
   if (pWordInfo->SignBit == TRUE) 
   {
     if ( ((gpA >> 13) & (0x00000001UL)) != 0x00 ) 
     {
        pWordInfo->TwoComplement = TRUE; 
     }
     else 
     {
        pWordInfo->TwoComplement = FALSE;
     }
   }
   else 
   {
      pWordInfo->TwoComplement = FALSE; 
   } 
   
   // NOTE:  TBD provide validation where if signed value, assume bit 11 and 
   //        validate that MSB positioin == bit 11 as well, otherwise 
   //        ASSERT ? 
   
   // Parse gpB field 
   // pWordInfo->DataLossTime = (gpB & 0x0000FFFFUL) * 1000;  // In msec !
   pWordInfo->DataLossTime = gpB;  // In msec !
   
   // Update the sensor which requests this word definition 
   pWordInfo->nSensor = nSensor; 
   
   // Update QAR Sensor Count 
   QARWordSensorCount++; 
   
   // Once a QAR Sensor has been requested, set QAR to enable 
   // QAR_Enable(); 
   
   // Return current index count 
   return(Cnt);
   
}


/******************************************************************************
 * Function:     QAR_ReadWord  
 *
 * Description:  Returns the current word data
 *
 * Parameters:   Index into QARWordInfo[] containing parse information 
 *
 * Returns:      FLOAT32 Word Value (partially processed)
 *
 * Notes:
 *   1) Used QARWordInfo[index] to read word from SubFrame
 *   2) If mult SF defined for this word sensor, the latest SF word is returned. 
 *      QARFrame[] or QARFramePrevGood[]
 *   3) Mult word location, only the last word location of a SF is used. All other
 *      will be ignored.  Therefore sensor processing / event max at 1 Hz ! 
 *   4) If ->FrameSync = FALSE or SF is Not OK will use previous GOOD word value
 *   5) TBD - if out of sync then back into sync reset to SF1 and last good was 
 *      ie SF2, then algorithm will look from SF1 and behind, which will use SF3 
 *      and SF4 which will be older than SF2 by about 2 seconds. 
 *
 *****************************************************************************/
FLOAT32 QAR_ReadWord (UINT16 nIndex)
{
   QAR_WORD_INFO_PTR pWordInfo;
   QAR_STATE_PTR     pState; 
   SINT32            value; 
   QAR_SF            currentSF; 
   SINT16            k; 
   UINT16            word; 
   SINT16            signBit; 
   

   pWordInfo = (QAR_WORD_INFO_PTR) &QARWordInfo[nIndex]; 
   pState = (QAR_STATE_PTR) &QARState; 
   
   currentSF = QAR_FindCurrentGoodSF(nIndex); 
   
   // NOTE: if currentSF == QAR_SF_MAX then we should ASSERT ! bad configuration !
   
   // Determine if SF is OK or use previous SF 
   // TBD: If word info identifies other SF, then we might want to try those SF rather 
   //   then use the previous SF.  We could be 4 seconds delayed vs best case 1 sec !
   if ( pState->bSubFrameOk[currentSF] == TRUE ) 
   {
      // word = QARFrame[currentSF][pWordInfo->WordLocation];
      word = QARFrame[currentSF].Word[pWordInfo->WordLocation];
   }
   else 
   {
      // word = QARFramePrevGood[currentSF][pWordInfo->WordLocation]; 
      word = QARFramePrevGood[currentSF].Word[pWordInfo->WordLocation];       
   }

   // Parse the word based on MSB Position and word size 
    // Parse Data Bits before the param (could be param with multiple params embedded). 
    value = ((word << (15 - (pWordInfo->MSBPosition))) & 0x0000FFFFUL);
    // Parse Data Bits after the param (could be param with multiple params embedded). 
    value = (value >> (16 - pWordInfo->WordSize)); 
   
   // If Signed value, assume bit 11. NOTE: MSB Position should be bit 11 as well !
   if ( pWordInfo->SignBit ) 
   {
      if ( pWordInfo->TwoComplement == TRUE ) 
      {
        // Determine if Sign Bit is set 
        signBit = ((word << (15 - pWordInfo->MSBPosition)) >> 15); 
        if (signBit == 0x01 ) 
        {
          // Sign extend the value 
          for (k=0;k<(16 - pWordInfo->WordSize);k++) 
          {
            value = value | (1<<(15-k)); 
          }
          value |= 0xFFFF0000UL;   // Manually sign extend to 32 bits ?? 
                                   // Or will compiler sign extend ? 
        }
      }
      else 
      {
        // MSB is sign value 
        signBit = ((word << (15 - pWordInfo->MSBPosition)) >> 15); 
        if ( signBit == 0x01) 
        {
          // "-1" to move one over from sign bit / filter it away !  AND 
          // Parse Data Bits before the param (could be param with multiple params embedded). 
          value = ((word << (15 - (pWordInfo->MSBPosition - 1))) & 0x0000FFFFUL); 
          // re-justify word, "-1" to account for the filtered out signed bit
          // Parse Data Bits after the param (could be param with multiple params embedded). 
          value = (value >> (16 - (pWordInfo->WordSize - 1))); 
          value = value * ((-1) * signBit); 
        }
      } // Else for Signed Magnitude value 
   }

   return( (FLOAT32) value);
}


/******************************************************************************
 * Function:     QAR_SensorTest
 *
 * Description:  Performs sensor test on QAR sensor.  Basic data loss test. 
 *
 * Parameters:   Index into QARWordInfo[]
 *
 * Returns:      FALSE if Sensor Test Failed 
 *               TRUE if Sensor Test Passes 
 *
 * Notes:
 *  1) Word Timeout comparison only.  
 *  2) Only when channel has become active and QAR data has SYNCed will the 
 *     func calc word timeout.  
 *  3) If Ch startup timeout has occurred, _SensorTest will return FAIL
 *  4) If QAR hw PBIT fails, _SensorTest will return FAIL 
 *
 *****************************************************************************/
BOOLEAN QAR_SensorTest (UINT16 nIndex)
{
   BOOLEAN           bValid; 
   QAR_WORD_INFO_PTR pWordInfo;
   QAR_STATE_PTR     pState; 
   QAR_SF            currentSF; 
   UINT32            nTimeOut; 
   SYS_QAR_CBIT_FAIL_WORD_TIMEOUT_LOG  WordTimeoutLog; 

   bValid = FALSE; 
   pState = (QAR_STATE_PTR) &QARState; 
   
   // Determine if any qar activity has been received and 
   //   we have synced at least once !  
   // Should also perform the test when the channel becomes active but the activity
   // time has not been updated or Frame Sync Once has not been set. This could
   // happen during powerup because the monitor function runs a 1Hz and the 
   // Read sub-Frame runs much faster.
   if ( (( pState->LastActivityTime != 0) && (pState->bFrameSyncOnce == TRUE)) ||
	 ( pState->bChanActive == TRUE) )
   {
      bValid = TRUE;
      pWordInfo = (QAR_WORD_INFO_PTR) &QARWordInfo[nIndex]; 
 
      // Get current good SF
      currentSF = QAR_FindCurrentGoodSF(nIndex); 
      
      // Calculate data loss time from "good" subframe buffer 
      if ( pState->bSubFrameOk[currentSF] == TRUE ) 
      {
        nTimeOut = QARFrame[currentSF].LastSubFrameUpdateTime;
      }
      else 
      { 
        nTimeOut = QARFramePrevGood[currentSF].LastSubFrameUpdateTime;
      } 
      
      // if ( (CM_GetTickCount() - pWordInfo->DataLossTime) > nTimeOut ) 
      if ( ( CM_GetTickCount() - nTimeOut ) > pWordInfo->DataLossTime )
      {
        bValid = FALSE; 
        if (pWordInfo->bFailed != TRUE) 
        {
           pWordInfo->bFailed = TRUE; 
          
           sprintf (GSE_OutLine, "QAR_SensorTest: Word Timeout (S = %d)\r\n", 
                                 pWordInfo->nSensor);
           GSE_DebugStr(NORMAL,TRUE,GSE_OutLine); 
           
           WordTimeoutLog.result = SYS_QAR_WORD_TIMEOUT; 
           sprintf( WordTimeoutLog.FailMsg, "Xcptn: Word Timeout (S = %d)", 
                    pWordInfo->nSensor ); 
           
           // Log CBIT Here only on transition from data rx to no data rx 
           LogWriteSystem( SYS_ID_QAR_CBIT_WORD_TIMEOUT_FAIL, LOG_PRIORITY_LOW, 
                          &WordTimeoutLog,
                          sizeof(SYS_QAR_CBIT_FAIL_WORD_TIMEOUT_LOG), NULL );
        }
      }
      else 
      {
        pWordInfo->bFailed = FALSE; 
      }
   }

   return(bValid);
}


/******************************************************************************
 * Function:     QAR_InterfaceValid
 *
 * Description:  Determines if data has been received for this parameter. 
 *
 * Parameters:   Index into QARWordInfo[],  not used but added for commonality 
 *                 with other (i.e. ARINC) routines.    
 *
 * Returns:      TRUE if interface is _STATUS_OK. 
 *
 * Notes:        none
 *  
 *****************************************************************************/
BOOLEAN QAR_InterfaceValid (UINT16 nIndex)
{
   BOOLEAN           bValid; 
   QAR_STATE_PTR     pState; 
   

   pState = (QAR_STATE_PTR) &QARState; 
   bValid = FALSE;
   
   if ( ( pState->SystemStatus == QAR_STATUS_OK) && (pState->bChanActive == TRUE))
   {
      bValid = TRUE; 
   }
   
   return bValid; 
   
}


/******************************************************************************
 * Function:     QAR_DisableAnyStreamingDebugOuput
 *
 * Description:  Disables current debug output of Raw QAR data to GSE port
 *
 * Parameters:   None.
 *
 * Returns:      None. 
 *
 * Notes:        None. 
 * 
 *****************************************************************************/
/*
void QAR_DisableAnyStreamingDebugOuput ( void )
{
  QAR_CONFIGURATION_PTR pQARCfg; 

  QARCfg.Output = QAR_GSE_NONE; 

  pQARCfg = &CfgMgr_ConfigPtr()->QARConfig; 
  pQARCfg->Output = QAR_GSE_NONE;

  CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
                         &pQARCfg->Output,
                         sizeof(QAR_OUTPUT_TYPE));
    
}
*/

/******************************************************************************
 * Function:     QARwrite
 *               
 * Description:  Write to FPGA QAR Registers 
 *
 * Parameters:   *addr FPGA QAR Register to access
 *               field Register Bit (parameter) to access
 *               data to set 
 *
 * Returns:      none
 *
 * Notes:        none 
 *
 *****************************************************************************/
void QARwrite( volatile UINT16* addr, FIELD_NAMES field, UINT16 data)
{
    UINT16 value = *addr;
    UINT16 mask  = MASK( FieldInfo[field].lsb, FieldInfo[field].size);
    
    value = RESET_BITS( value, mask);
    value |= FIELD( data, FieldInfo[field].lsb, FieldInfo[field].size);

    *addr = value;
}


/******************************************************************************
 * Function:     QARread
 *               
 * Description:  Read of FPGA QAR Registers 
 *
 * Parameters:   *addr FPGA QAR Register to access
 *               field Register Bit (parameter) to access
 *
 * Returns:      This function returns the return value of the EXTRACT macro 
 *               which extracts a bit field from the word and places it in the LSB.
 *
 * Notes:        none 
 *
 *****************************************************************************/
UINT16 QARread( volatile UINT16* addr, FIELD_NAMES field)
{
    return EXTRACT( *addr, FieldInfo[field].lsb, FieldInfo[field].size);
}


/******************************************************************************
 * Function:     QAR_CreateAllInternalLogs
 *
 * Description:  Creates all QAR internal logs for testing / debug of log 
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
void QAR_CreateAllInternalLogs ( void )
{
  // 1 Create SYS_QAR_PBIT_FAIL_LOG:QAR_PBIT_LOOPBACK
  LogWriteSystem( DRV_ID_QAR_PBIT_LOOPBACK_FAIL, LOG_PRIORITY_LOW, &PbitLogLoopBack,
                           sizeof(SYS_QAR_PBIT_FAIL_LOG_LOOPBACK_STRUCT), NULL ); 
  
  // 2 Create SYS_QAR_PBIT_FAIL_LOG:QAR_PBIT_DPRAM
  LogWriteSystem( DRV_ID_QAR_PBIT_DPRAM_FAIL, LOG_PRIORITY_LOW, &PbitLogDpram,
                           sizeof(SYS_QAR_PBIT_FAIL_LOG_DPRAM_STRUCT), NULL ); 

  // 3 Create SYS_QAR_PBIT_FAIL_LOG: FPGA_BAD_STATE
  LogWriteSystem( DRV_ID_QAR_PBIT_FPGA_FAIL, LOG_PRIORITY_LOW, &PbitLogFpga,
                            sizeof(SYS_QAR_PBIT_FAIL_LOG_FPGA_STRUCT), NULL ); 
  
  // 4 Create SYS_QAR_CBIT_FAIL_LOG:STARTUP_TIME
  LogWriteSystem( SYS_ID_QAR_CBIT_STARTUP_FAIL, LOG_PRIORITY_LOW, &CbitLogStartup,
                            sizeof(QAR_TIMEOUT_LOG), NULL ); 
  
  // 5 Create SYS_QAR_CBIT_FAIL_LOG:DATA_LOSS_TIMEOUT
  LogWriteSystem( SYS_ID_QAR_CBIT_DATA_LOSS_FAIL, LOG_PRIORITY_LOW, &CbitLogDataLoss,
                            sizeof(QAR_TIMEOUT_LOG), NULL ); 
  
  // 6 Create SYS_QAR_INFO_DATA_PRESENT_LOG
  LogWriteSystem (SYS_ID_QAR_DATA_PRESENT, LOG_PRIORITY_LOW, NULL, 0, NULL);
  
  // 7 Create SYS_QAR_INFO_DATA_LOST_LOG
  LogWriteSystem (SYS_ID_QAR_DATA_LOSS, LOG_PRIORITY_LOW, NULL, 0, NULL);
  
  // 8 Create SYS_QAR_INFO_SYNC_LOG
  LogWriteSystem (SYS_ID_QAR_SYNC, LOG_PRIORITY_LOW, NULL, 0, NULL);
  
  // 9 Create SYS_QAR_INFO_SYNC_LOST_LOG
  LogWriteSystem (SYS_ID_QAR_SYNC_LOSS, LOG_PRIORITY_LOW, NULL, 0, NULL);
  
  // 10 Create SYS_ID_QAR_CBIT_WORD_TIMEOUT_FAIL
  LogWriteSystem (SYS_ID_QAR_CBIT_WORD_TIMEOUT_FAIL, LOG_PRIORITY_LOW, 
                            &CbitLogWordTimeout, sizeof(SYS_QAR_CBIT_FAIL_WORD_TIMEOUT_LOG),
                            NULL); 
  
  // 11 Create SYS_ID_QAR_PBIT_DRV_FAIL
  LogWriteSystem (SYS_ID_QAR_PBIT_DRV_FAIL, LOG_PRIORITY_LOW, &QarSysPbitLog, 
                            sizeof(QAR_SYS_PBIT_STARTUP_LOG), NULL); 
}
/*vcast_dont_instrument_end*/

#endif


/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:     QAR_ReconfigureFPGAShadowRam
 * 
 * Description:  Updates the FPGA Shadow RAM structure with current QARCfg values
 *
 * Parameters:   None. 
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static 
void QAR_ReconfigureFPGAShadowRam (void)
{
   QAR_CONFIGURATION_PTR   pQARCfg; 
   FPGA_SHADOW_QAR_CFG_PTR pShadowQarCfg;    

   pQARCfg = (QAR_CONFIGURATION_PTR) &QARCfg;

   // 03/10/2008 PLee Update to latest FPGA Rev 4 definition and 
   //            updates to FGPA.h for #defines
   // Enable the QAR FPGA Interrupt
   // FPGA_Interrupt_Enable (QAR_SUB_FRAME, QAR_IRQ_SUB_FRAME_ENABLE, TRUE);


   pShadowQarCfg = (FPGA_SHADOW_QAR_CFG_PTR) &FPGA_GetShadowRam()->Qar; 
   
   // Disable the QAR FPGA Interrupt on startup 
   //   Will be reg Enabled either from ACS[] init or QAR Sensor 
   //if (QARState.bQAREnable == TRUE) {
   if ( pQARCfg->Enable == TRUE ) 
   {
      FPGA_GetShadowRam()->IMR2 = (FPGA_GetShadowRam()->IMR2 & ~IMR2_QAR_SF) |
                               (IMR2_QAR_SF * IMR2_ENB_IRQ_VAL); 
      pShadowQarCfg->QCR = (pShadowQarCfg->QCR & ~QCR_ENABLE) |
                           (QCR_ENABLE * QCR_ENABLED_VAL); 
   }
   else 
   {
      FPGA_GetShadowRam()->IMR2 = (FPGA_GetShadowRam()->IMR2 & ~IMR2_QAR_SF) |
                               (IMR2_QAR_SF * IMR2_DSB_IRQ_VAL); 
      pShadowQarCfg->QCR = (pShadowQarCfg->QCR & ~QCR_ENABLE) |
                           (QCR_ENABLE * QCR_TEST_DISABLED_VAL);       
   }

   // Set QAR format Bipolar or Harvard Biphase 
   pShadowQarCfg->QCR = ( pShadowQarCfg->QCR & ~QCR_FORMAT ) | 
                        ( QCR_FORMAT * pQARCfg->Format );
                        
   // Set QAR words per Sub Frame
   pShadowQarCfg->QCR = ( pShadowQarCfg->QCR & ~QCR_BYTES_SF_FIELD ) | 
                        ( QCR_BYTES_SF * pQARCfg->NumWords );
                        
   // Set Barker Code 1
   pShadowQarCfg->QSFCODE1 = ( pQARCfg->BarkerCode[QAR_SF1] & 0x0FFF ); 
   
   // Set Barker Code 2
   pShadowQarCfg->QSFCODE2 = ( pQARCfg->BarkerCode[QAR_SF2] & 0x0FFF ); 
   
   // Set Barker Code 3
   pShadowQarCfg->QSFCODE3 = ( pQARCfg->BarkerCode[QAR_SF3] & 0x0FFF ); 
   
   // Set Barker Code 4                        
   pShadowQarCfg->QSFCODE4 = ( pQARCfg->BarkerCode[QAR_SF4] & 0x0FFF ); 

}


/******************************************************************************
 * Function:     QAR_ResetState
 * 
 * Description:  Resets the internal QAR state variables 
 *
 * Parameters:   None
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static
void QAR_ResetState (void)
{
   QAR_STATE_PTR           pState;
   QAR_SF                  i;    
   QAR_CONFIGURATION_PTR   pQARCfg; 
   

   // Intialize the Package State Data
   pState = (QAR_STATE_PTR) &QARState;

   // Get current configuration   
   pQARCfg = (QAR_CONFIGURATION_PTR) &QARCfg;
   
   pState->CurrentFrame      = QAR_SF4;
   pState->PreviousFrame     = QAR_SF4;
   pState->LastServiced      = QAR_SF4;
   // pState->TotalWords        = pow(2, (QARCfg.NumWords + 5));
   pState->TotalWords        = 1 << (QARCfg.NumWords + 5); 
   pState->LossOfFrameCount    = 0;
   pState->FrameSync           = FALSE;
   pState->LossOfFrame         = FALSE;
   pState->FrameState          = QAR_NO_STATE;
   pState->BarkerError         = FALSE; 
   pState->BarkerErrorCount    = 0; 
   pState->DataPresent         = FALSE; 
   pState->DataPresentLostCount = 0; 

   
   pState->bGetNewSubFrame    = FALSE;
   pState->LastIntTime        = 0; 
   pState->BadIntFreqCnt      = 0; 
   for (i=QAR_SF1;i<QAR_SF_MAX;i++) 
   {
      pState->bSubFrameOk[i] = TRUE;
      pState->bSubFrameReceived[i] = FALSE; 
      
      // Clear Software SF Buffers
      memset ( (void *) &QARFrame[i].Word[0], 0x00, sizeof(QAR_SUBFRAME_DATA) );
      // NOTE: LastSubFrameUpdateTime cleared !
   }
   pState->PreviousSubFrameOk  = QAR_SF1;    // Start at the beginning ! 
   
   pState->bQAREnable = pQARCfg->Enable; 
   
   pState->LastActivityTime = 0; 
   pState->bChannelStartupTimeOut = FALSE;
   pState->bChannelTimeOut = FALSE; 
   pState->bChanActive = FALSE;
   
   pState->InterruptCnt = 0; 
   pState->InterruptCntLOFS = 0; 
   
}
/******************************************************************************
 * Function:     QAR_PBIT_LoopBack_Test
 *               
 * Description:  FPGA QAR LoopBack Test 
 *
 * Parameters:   pdest - ptr to data store to return PBIT fail data 
 *               psize - ptr to size var to return the size of PBIT fail data 
 *                       (*psize == 0 if DRV_OK is returned)
 *
 * Returns:      DRV_RESULT [DRV_OK] if PBIT successful 
 *               
 *               QAR_PBIT_LOOPBACK if BIIP and BIIN loopback fails
 *
 * Notes:        
 *   1) Func assumes pQARCtrl and pQARStatus to be initialized
 *   2) Will disable QAR and set _TSTP / _TSTN to 0 on func exit 
 *   3) On QAR_PBIT_LOOPBACK data payload = QAR_LOOPBACK_TEST
 *
 *****************************************************************************/
static RESULT QAR_PBIT_LoopBack_Test(UINT8 *pdest, UINT16 *psize)
{
  RESULT result; 
  UINT8  i; 

  
  result = DRV_OK;
  
  // Disable QAR to force test mode
  QAR_W( pQARCtrl, C_ENABLE, QCR_TEST_DISABLED_VAL); 
  
  // Loop thru all test.  Stop on failure 
  for (i=0;i<((UINT8) QAR_TEST_TSTPN_MAX);i++) 
  {
    // Write Values
    QAR_W( pQARCtrl, C_TSTP, QARLBTest[i].Tstp_val);
    QAR_W( pQARCtrl, C_TSTN, QARLBTest[i].Tstn_val); 
    
    // Read and Compare Values
    if ( (QAR_R( pQARStatus, S_BIIP) != STPU( QARLBTest[i].Bipp_exp, eTpQar2778)) ||
         (QAR_R( pQARStatus, S_BIIN) != QARLBTest[i].Biin_exp) )
    {
       // Data Payload for QAR DRV PBIT LoopBack failure
       memcpy ( pdest, &QARLBTest[i].TestEnum, sizeof(QAR_LOOPBACK_TEST) ); 
       *psize = sizeof(QAR_LOOPBACK_TEST); 
       result = SYS_QAR_PBIT_LOOPBACK;
       break; 
    }
  }
  
  // Force Tstp and Tstn OFF
  QAR_W( pQARCtrl, C_TSTP, 0); 
  QAR_W( pQARCtrl, C_TSTN, 0);

  return (result);

}


/******************************************************************************
 * Function:     QAR_PBIT_DPRAM_Test
 *               
 * Description:  FPGA QAR DPRAM Test of QARDATA[]
 *
 * Parameters:   pdest - ptr to data store to return PBIT fail data 
 *               psize - ptr to size var to return the size of PBIT fail data 
 *                       (*psize == 0 if DRV_OK is returned)
 *
 * Returns:      DRV_RESULT [DRV_OK] if PBIT successful 
 *               
 *               QAR_PBIT_DPRAM if memory test of DRPAM of QARDATA[] fails
 *
 * Notes:        
 *   1) Func assumes pQARCtrl and pQARSubFrame to be initialized
 *   2) Will disable QAR  
 *   3) On QAR_PBIT_DPRAM data payload = pattern expected and pattern received
 *
 *****************************************************************************/
static RESULT QAR_PBIT_DPRAM_Test(UINT8 *pdest, UINT16 *psize)
{
  RESULT   result; 
  UINT16   i;
  UINT16   j;
  UINT16   max; 

  volatile UINT16 *pDpram; 
  
  UINT16 val_QAR_TSTN; 
  UINT16 val_QAR_TSTP; 
  
#define QAR_TSTX_TEST 1
  
  
  result = DRV_OK;
  
  // Disable QAR to force test mode
  QAR_W( pQARCtrl, C_ENABLE, QCR_TEST_DISABLED_VAL); 
  
  // Read current setting for QAR_TSTP and QAR_TSTN
  val_QAR_TSTN = QAR_R( pQARCtrl, C_TSTN ); 
  val_QAR_TSTP = QAR_R( pQARCtrl, C_TSTP ); 
  
  // Set Xcvr to be disabled to incoming data 
  QAR_W( pQARCtrl, C_TSTP, QAR_TSTX_TEST ); 
  QAR_W( pQARCtrl, C_TSTN, QAR_TSTX_TEST ); 
  
  // For FPGA Version 12 and higher, loop thru from QARDATA, QARFRAME1, QARFRAME2
  // For ver and below, loop thru QARDATA only 
  max = FPGA_DPRAM_MAX_BLK; 
#ifdef SUPPORT_FPGA_BELOW_V12  
/*vcast_dont_instrument_start*/
  if ( FPGA_GetStatus()->Version < FPGA_VER_12  ) 
  {
    max = FPGA_DPRAM_OP; 
  }
/*vcast_dont_instrument_end*/       
#endif  
  
  for (j=0;j<max;j++) 
  {
    pDpram = FPGA_INTERNAL_DPRAM[j].ptr; 
    
    // Loop thru and write the data
    for (i=0;i<FPGA_QAR_BUFFER_SIZE;i++) 
    {
      *pDpram++ = i; 
    }

    pDpram = FPGA_INTERNAL_DPRAM[j].ptr; 
    for (i=0;i<FPGA_QAR_BUFFER_SIZE;i++)
    {
      if ( *pDpram != STPU( i, eTpQar3774)) 
      {
        // Data Payload for QAR DRV PBIT LoopBack failure
        *(UINT32*) pdest = i; 
        *(UINT32*) (pdest + 4) = *pDpram; 
        *psize = sizeof(UINT32) + sizeof(UINT32); 
        result = FPGA_INTERNAL_DPRAM[j].resultCode;
        j = max;  // Force outer loop to exit 
        break; 
      }
      else 
      {
        pDpram++; 
      }
    } // End for loop i
  } // End for loop j 
  
  // If DPRAM test pass, then clear the memory 
  if (result == DRV_OK) 
  {
    for (j=0;j<max;j++) 
    {
      pDpram = FPGA_INTERNAL_DPRAM[j].ptr; 
      
      // Loop thru and write the data
      for (i=0;i<FPGA_QAR_BUFFER_SIZE;i++) 
      {
        *pDpram++ = 0x00; 
      }
    } // End for loop j 
  }
  
  // Restore Xcvr Ctrl to previous setting 
  QAR_W( pQARCtrl, C_TSTP, val_QAR_TSTP ); 
  QAR_W( pQARCtrl, C_TSTN, val_QAR_TSTN ); 
  
  return (result);

}


/******************************************************************************
 * Function:     QAR_FindCurrentGoodSF
 *
 * Description:  Finds the most recent SF with "good" data 
 *
 * Parameters:   Index into QARWordInfo[]
 *
 * Returns:      Returns SF index
 *
 * Notes:
 *  1) Data Loss comparison only.  Uses SF update time for comparison. 
 *
 *****************************************************************************/
static 
QAR_SF QAR_FindCurrentGoodSF (UINT16 nIndex)
{
   QAR_WORD_INFO_PTR pWordInfo;
   QAR_STATE_PTR     pState; 
   QAR_SF            currentSF; 
   SINT16            k; 
   QAR_SF            i; 
   
   pWordInfo = (QAR_WORD_INFO_PTR) &QARWordInfo[nIndex]; 
   pState = (QAR_STATE_PTR) &QARState; 

   // Find current SF
   if ( pState->FrameSync == FALSE ) 
   {
      // Use Previous Good SF recorded as current SF pointer 
      currentSF = pState->PreviousSubFrameOk; 
   }
   else 
   {
      currentSF = pState->CurrentFrame; 
   }
   
   // Find most recent SF for the wanted word from QAR sensor cfg SF
   k = currentSF;
   for (i=QAR_SF1;i<QAR_SF_MAX;i++) 
   {
      k = k - i; 
      if ( k < 0 ) 
      {
        // Wrap Case
        k = k + QAR_SF_MAX; 
      }
      if (pWordInfo->bSubFrameData[k] == TRUE) 
      {
        // Current SF matches wanted SF !
        break; 
      }
   }
   
   ASSERT ( i != QAR_SF_MAX ); 
   
   currentSF = (QAR_SF) k; 

   return (currentSF); 
}


/******************************************************************************
 * Function:     QAR_RestoreStartupCfg
 *
 * Description:  Restores the Startup QAR Cfg.  To be over written later with 
 *               user cfg setting
 *
 * Parameters:   None. 
 *
 * Returns:      None. 
 *
 * Notes:        None. 
 *
 *****************************************************************************/
static
void QAR_RestoreStartupCfg ( void )
{
   QAR_CONFIGURATION_PTR pQARCfg; 
   
   pQARCfg = (QAR_CONFIGURATION_PTR) &QARCfg; 

   // TBD - This should be coming from the EEPROM
   strncpy_safe(pQARCfg->Name, sizeof(pQARCfg->Name), "QAR",_TRUNCATE); 
   pQARCfg->Format   = QAR_BIPOLAR_RETURN_ZERO;
   pQARCfg->NumWords = QAR_64_WORDS;
   pQARCfg->BarkerCode[QAR_SF1] = QAR_SF1_BARKER;
   pQARCfg->BarkerCode[QAR_SF2] = QAR_SF2_BARKER;
   pQARCfg->BarkerCode[QAR_SF3] = QAR_SF3_BARKER;
   pQARCfg->BarkerCode[QAR_SF4] = QAR_SF4_BARKER;
      
   pQARCfg->ChannelStartup_s = SECONDS_20;  // 20 seconds 
   pQARCfg->ChannelTimeOut_s = SECONDS_10;  // 10 seconds 
   pQARCfg->ChannelSysCond = STA_CAUTION; 
   pQARCfg->PBITSysCond = STA_CAUTION;
   pQARCfg->Enable = FALSE; 
}


/******************************************************************************
 * Function:     QAR_CreateTimeOutSystemLog
 *
 * Description:  Creates the QAR TimeOut System Logs
 *
 * Parameters:   resultType - SYS_QAR_STARTUP_TIMEOUT 
 *                         or SYS_QAR_DATA_LOSS_TIMEOUT
 *
 * Returns:      None. 
 *
 * Notes:        None. 
 *
 *****************************************************************************/
static void QAR_CreateTimeOutSystemLog( RESULT resultType )
{
  QAR_TIMEOUT_LOG timeoutLog; 
  QAR_CONFIGURATION_PTR pQarCfg; 
  SYS_APP_ID sysId; 
   
  pQarCfg = (QAR_CONFIGURATION_PTR) &QARCfg; 

  switch (resultType)
  {
    case SYS_QAR_STARTUP_TIMEOUT: 
      timeoutLog.result = resultType; 
      timeoutLog.cfg_timeout = pQarCfg->ChannelStartup_s;
      sysId = SYS_ID_QAR_CBIT_STARTUP_FAIL; 
      break;
      
    case SYS_QAR_DATA_LOSS_TIMEOUT:
      timeoutLog.result = resultType; 
      timeoutLog.cfg_timeout = pQarCfg->ChannelTimeOut_s; 
      sysId = SYS_ID_QAR_CBIT_DATA_LOSS_FAIL; 
      break; 
      
    default:
      // No such case !
       FATAL("Unrecognized Sys QAR resultType = %d", resultType);
      break;
  } // End switch (resultType)
  
  Flt_SetStatus(pQarCfg->ChannelSysCond, sysId, &timeoutLog, sizeof(QAR_TIMEOUT_LOG)); 
  
}



/*****************************************************************************
 *  MODIFICATIONS
 *    $History: QAR.c $
 * 
 * *****************  Version 93  *****************
 * User: John Omalley Date: 10/05/11   Time: 7:55p
 * Updated in $/software/control processor/code/drivers
 * SCR 1015 - Sensor Test Logic Update
 * 
 * *****************  Version 92  *****************
 * User: John Omalley Date: 10/04/11   Time: 4:56p
 * Updated in $/software/control processor/code/drivers
 * SCR 1015 - Sensor Test Update
 * 
 * *****************  Version 91  *****************
 * User: John Omalley Date: 9/30/11    Time: 11:27a
 * Updated in $/software/control processor/code/drivers
 * SCR 1015 - Interface Valid Logic Update
 * 
 * *****************  Version 90  *****************
 * User: John Omalley Date: 9/29/11    Time: 4:08p
 * Updated in $/software/control processor/code/drivers
 * SCR 1015 - Interface Validity
 * 
 * *****************  Version 89  *****************
 * User: John Omalley Date: 9/12/11    Time: 11:02a
 * Updated in $/software/control processor/code/drivers
 * SCR 1071 - Cleared sub-frame Received on LOFS
 * 
 * *****************  Version 88  *****************
 * User: John Omalley Date: 9/12/11    Time: 9:21a
 * Updated in $/software/control processor/code/drivers
 * SCR 1070 - LOFS Causing bad data retrieval
 * 
 * *****************  Version 87  *****************
 * User: John Omalley Date: 8/22/11    Time: 11:29a
 * Updated in $/software/control processor/code/drivers
 * SCR 1015 - Changed Interface valid to FALSE until data is received
 * 
 * *****************  Version 86  *****************
 * User: John Omalley Date: 6/23/11    Time: 3:08p
 * Updated in $/software/control processor/code/drivers
 * SCR 1032 - ModFix use strncpy_safe for QAR_GetFileHdr
 * 
 * *****************  Version 85  *****************
 * User: Contractor2  Date: 6/20/11    Time: 11:31a
 * Updated in $/software/control processor/code/drivers
 * SCR #113 Enhancement - QAR bad baud rate setting and continuous
 * DATA_PRES trans ind
 * 
 * *****************  Version 84  *****************
 * User: John Omalley Date: 4/21/11    Time: 4:58p
 * Updated in $/software/control processor/code/drivers
 * SCR 1032 - Add Binary Data Header
 * 
 * *****************  Version 83  *****************
 * User: Peter Lee    Date: 11/18/10   Time: 4:44p
 * Updated in $/software/control processor/code/drivers
 * SCR #1004 Error in Barker Error Processing
 * 
 * *****************  Version 82  *****************
 * User: Peter Lee    Date: 11/11/10   Time: 1:12p
 * Updated in $/software/control processor/code/drivers
 * SCR #991 Code Coverage Updates 
 * 
 * *****************  Version 81  *****************
 * User: Peter Lee    Date: 11/10/10   Time: 5:51p
 * Updated in $/software/control processor/code/drivers
 * SCR #890 Marking subsequent SF as Good for Barker Err Detected.
 * 
 * *****************  Version 80  *****************
 * User: Peter Lee    Date: 11/05/10   Time: 12:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #982 Remove dead code from _SensorTest()
 * 
 * *****************  Version 79  *****************
 * User: John Omalley Date: 11/03/10   Time: 4:51p
 * Updated in $/software/control processor/code/drivers
 * SCR 981 - Remove DPRAM tests because of FPGA bug
 * 
 * *****************  Version 78  *****************
 * User: Peter Lee    Date: 10/01/10   Time: 11:36a
 * Updated in $/software/control processor/code/drivers
 * SCR #912 Code Coverage and Code Review Updates
 * 
 * *****************  Version 77  *****************
 * User: Contractor2  Date: 9/24/10    Time: 3:02p
 * Updated in $/software/control processor/code/drivers
 * SCR #882 WD - Old Code for pulsing the WD should be removed
 * 
 * *****************  Version 76  *****************
 * User: Peter Lee    Date: 9/01/10    Time: 2:34p
 * Updated in $/software/control processor/code/drivers
 * SCR #844 Error - When FPGA status -> fail, QAR bypasses its CBIT
 * processing
 * 
 * *****************  Version 75  *****************
 * User: Jeff Vahue   Date: 8/30/10    Time: 2:12p
 * Updated in $/software/control processor/code/drivers
 * SCR #830 - Code Coverage
 * 
 * *****************  Version 74  *****************
 * User: Jeff Vahue   Date: 8/28/10    Time: 12:22p
 * Updated in $/software/control processor/code/drivers
 * SCR# 830 - GENERATE_SYS_LOGS exclude area from coverage results
 * 
 * *****************  Version 73  *****************
 * User: Peter Lee    Date: 8/19/10    Time: 4:49p
 * Updated in $/software/control processor/code/drivers
 * SCR #804 Error reconfiguring QAR with "qar.reconfigure". 
 * 
 * *****************  Version 72  *****************
 * User: Contractor V&v Date: 7/30/10    Time: 9:22p
 * Updated in $/software/control processor/code/drivers
 * SCR #282 Misc - Shutdown Processing
 * 
 * *****************  Version 71  *****************
 * User: John Omalley Date: 7/30/10    Time: 11:57a
 * Updated in $/software/control processor/code/drivers
 * SCR 730 - Changed SYS Logs to DRV Logs
 * 
 * *****************  Version 70  *****************
 * User: Peter Lee    Date: 7/26/10    Time: 6:20p
 * Updated in $/software/control processor/code/drivers
 * SCR #743 Update TimeOut Processing for val == 0
 * 
 * *****************  Version 69  *****************
 * User: Peter Lee    Date: 7/23/10    Time: 1:39p
 * Updated in $/software/control processor/code/drivers
 * SCR #688 Remove FPGA Ver 9 support.  SW will only be compatible with
 * Ver 12.
 * 
 * *****************  Version 68  *****************
 * User: Peter Lee    Date: 7/21/10    Time: 5:23p
 * Updated in $/software/control processor/code/drivers
 * SCR #720 QAR DPRAM PBIT Failure for Ver 12
 * 
 * *****************  Version 67  *****************
 * User: John Omalley Date: 7/20/10    Time: 9:04a
 * Updated in $/software/control processor/code/drivers
 * SCR 294 - Add system binary header
 * 
 * *****************  Version 66  *****************
 * User: Jeff Vahue   Date: 7/19/10    Time: 6:33p
 * Updated in $/software/control processor/code/drivers
 * SCR# 707 - Code Coverage TP
 * 
 * *****************  Version 65  *****************
 * User: Contractor V&v Date: 6/22/10    Time: 6:20p
 * Updated in $/software/control processor/code/drivers
 * SCR #485 Escape Sequence and Box Configuration
 * 
 * *****************  Version 64  *****************
 * User: Jim Mood     Date: 6/16/10    Time: 4:27p
 * Updated in $/software/control processor/code/drivers
 * SCR 623.  Batch Reconfiguration implementation changes
 * 
 * *****************  Version 63  *****************
 * User: John Omalley Date: 6/08/10    Time: 12:04p
 * Updated in $/software/control processor/code/drivers
 * SCR 627 - Updated for LJ60
 * 
 * *****************  Version 62  *****************
 * User: Peter Lee    Date: 5/24/10    Time: 6:37p
 * Updated in $/software/control processor/code/drivers
 * SCR #608 Update QAR Interface Status logic to match Arinc / UART. 
 * 
 * *****************  Version 61  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:54p
 * Updated in $/software/control processor/code/drivers
 * SCR #587 Change TmTaskCreate to return void
 * 
 * *****************  Version 60  *****************
 * User: Contractor2  Date: 5/06/10    Time: 2:09p
 * Updated in $/software/control processor/code/drivers
 * SCR #579 Change LogWriteSys to return void
 * 
 * *****************  Version 59  *****************
 * User: Contractor3  Date: 5/06/10    Time: 11:27a
 * Updated in $/software/control processor/code/drivers
 * Check In SCR #586
 * 
 * *****************  Version 58  *****************
 * User: Jeff Vahue   Date: 4/12/10    Time: 5:43p
 * Updated in $/software/control processor/code/drivers
 * SCR #541 - all XXX_CreateLogs functions are now dependent on
 * GENERATE_SYS_LOGS
 * 
 * *****************  Version 57  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:08p
 * Updated in $/software/control processor/code/drivers
 * SCR #317 Implement safe strncpy
 * 
 * *****************  Version 56  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:36p
 * Updated in $/software/control processor/code/drivers
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 * 
 * *****************  Version 55  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/drivers
 * SCR# 483 - Function Names
 * 
 * *****************  Version 54  *****************
 * User: Contractor V&v Date: 3/10/10    Time: 4:40p
 * Updated in $/software/control processor/code/drivers
 * SCR #437 All passive user commands should be RO
 * 
 * *****************  Version 53  *****************
 * User: Jeff Vahue   Date: 3/10/10    Time: 2:33p
 * Updated in $/software/control processor/code/drivers
 * SCR# 480 - add semi's after ASSERTs
 * 
 * *****************  Version 52  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:22p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 51  *****************
 * User: John Omalley Date: 1/15/10    Time: 5:30p
 * Updated in $/software/control processor/code/drivers
 * SCR 396
 * * Renamed Sensor Test to interface validation.
 * 
 * *****************  Version 50  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 5:01p
 * Updated in $/software/control processor/code/drivers
 * SCR# 397
 * 
 * *****************  Version 49  *****************
 * User: Jeff Vahue   Date: 1/08/10    Time: 11:03a
 * Updated in $/software/control processor/code/drivers
 * SCR# 385
 * 
 * *****************  Version 48  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:23p
 * Updated in $/software/control processor/code/drivers
 * SCR 18, SCR 371
 * 
 * *****************  Version 47  *****************
 * User: Peter Lee    Date: 12/22/09   Time: 5:07p
 * Updated in $/software/control processor/code/drivers
 * Added ASSERT() for param checks
 * 
 * *****************  Version 46  *****************
 * User: Peter Lee    Date: 12/22/09   Time: 3:43p
 * Updated in $/software/control processor/code/drivers
 * SCR #29 Error: QAR Snapshot Data
 * 
 * *****************  Version 45  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 3:23p
 * Updated in $/software/control processor/code/drivers
 * SCR# 326 cleanup
 * 
 * *****************  Version 44  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 2:11p
 * Updated in $/software/control processor/code/drivers
 * SCR# 326
 * 
 * *****************  Version 43  *****************
 * User: Peter Lee    Date: 12/21/09   Time: 6:19p
 * Updated in $/software/control processor/code/drivers
 * SCR #380 Implement Real Time Sys Condition 
 * 
 * *****************  Version 42  *****************
 * User: Jeff Vahue   Date: 12/17/09   Time: 12:04p
 * Updated in $/software/control processor/code/drivers
 * SCR# 364 - remove unused functions
 * 
 * *****************  Version 41  *****************
 * User: Peter Lee    Date: 12/16/09   Time: 2:17p
 * Updated in $/software/control processor/code/drivers
 * QAR Req 3110, 3041, 3114 and 3118
 * 
 * *****************  Version 40  *****************
 * User: Peter Lee    Date: 12/04/09   Time: 5:24p
 * Updated in $/software/control processor/code/drivers
 * SCR #353 FPGA new QARFRAM1 and QARFRAM2 DPRAM FPGA Reg
 * 
 * *****************  Version 38  *****************
 * User: Contractor V&v Date: 11/18/09   Time: 3:38p
 * Updated in $/software/control processor/code/drivers
 * Implement SCR 172 ASSERT processing for all "default" case processing
 * 
 * *****************  Version 37  *****************
 * User: Peter Lee    Date: 11/12/09   Time: 10:13a
 * Updated in $/software/control processor/code/drivers
 * SCR #315 PWEH SEU Processing Support
 * 
 * *****************  Version 36  *****************
 * User: Peter Lee    Date: 10/23/09   Time: 9:58a
 * Updated in $/software/control processor/code/drivers
 * Minor updates to support JV PC version of program
 * 
 * *****************  Version 35  *****************
 * User: Peter Lee    Date: 10/22/09   Time: 10:27a
 * Updated in $/software/control processor/code/drivers
 * SCR #171 Add correct System Status (Normal, Caution, Fault) enum to
 * UserTables
 * 
 * *****************  Version 34  *****************
 * User: Peter Lee    Date: 10/21/09   Time: 4:12p
 * Updated in $/software/control processor/code/drivers
 * SCR #229 Update Ch/Startup TimeOut to 1 sec from 1 msec resolution.
 * Updated GPB channel loss timeout to 1 msec from 1 sec resolution. 
 * 
 * *****************  Version 33  *****************
 * User: Peter Lee    Date: 10/20/09   Time: 6:49p
 * Updated in $/software/control processor/code/drivers
 * Updates for JV to support PC version of program
 * 
 * *****************  Version 32  *****************
 * User: Peter Lee    Date: 10/15/09   Time: 9:59a
 * Updated in $/software/control processor/code/drivers
 * FPGA CBIT Requirements
 * 
 * *****************  Version 31  *****************
 * User: Peter Lee    Date: 10/13/09   Time: 7:53p
 * Updated in $/software/control processor/code/drivers
 * SCR #300 Error - QAR Data Loss Calc in QAR_SensorTest()
 * SCR #301 Enhancement QAR Chan Loss Based on QAR Sync and not QAR Data
 * Present !
 * 
 * *****************  Version 30  *****************
 * User: Peter Lee    Date: 10/05/09   Time: 1:10p
 * Updated in $/software/control processor/code/drivers
 * SCR #287 QAR_SensorTest() records log and debug msg. 
 * 
 * *****************  Version 29  *****************
 * User: Peter Lee    Date: 9/30/09    Time: 4:47p
 * Updated in $/software/control processor/code/drivers
 * SCR #283 Misc Code Review Updates
 * 
 * *****************  Version 28  *****************
 * User: Peter Lee    Date: 9/23/09    Time: 11:53a
 * Updated in $/software/control processor/code/drivers
 * SCR #268 Updates to _SensorTest() to include Ch Loss.  Have _ReadWord()
 * return FLOAT32
 * 
 * *****************  Version 27  *****************
 * User: Peter Lee    Date: 9/15/09    Time: 6:40p
 * Updated in $/software/control processor/code/drivers
 * Remove Junk / Test code in QAR_Initialize() 
 * 
 * *****************  Version 26  *****************
 * User: Peter Lee    Date: 9/15/09    Time: 6:00p
 * Updated in $/software/control processor/code/drivers
 * SCR #94, #95 PBIT returns data structure to Init Mgr. 
 * SCR #257 QAR signed value decoding error
 * Misc Comipiler Warning Updates 
 * 
 * *****************  Version 25  *****************
 * User: Peter Lee    Date: 8/03/09    Time: 10:49a
 * Updated in $/software/control processor/code/drivers
 * SCR #234 Removed pow() call from initializing QAR word size. 
 * 
 * *****************  Version 24  *****************
 * User: Peter Lee    Date: 7/31/09    Time: 3:42p
 * Updated in $/software/control processor/code/drivers
 * SCR #221 Return byte count for reading Raw Data Buffer
 * 
 * *****************  Version 23  *****************
 * User: Peter Lee    Date: 7/31/09    Time: 3:16p
 * Updated in $/software/control processor/code/drivers
 * SCR #221 Return byte cnt and not word cnt when returning Raw Data size.
 * 
 * *****************  Version 22  *****************
 * User: Peter Lee    Date: 1/29/09    Time: 2:12p
 * Updated in $/control processor/code/drivers
 * Updated System Log ID
 * 
 * *****************  Version 21  *****************
 * User: Peter Lee    Date: 1/22/09    Time: 6:13p
 * Updated in $/control processor/code/drivers
 * SCR #136 PACK system log structures 
 * 
 * *****************  Version 20  *****************
 * User: Peter Lee    Date: 1/06/09    Time: 2:48p
 * Updated in $/control processor/code/drivers
 * SCR 129 Misc FPGA Updates. Updated call to FPGA_GetStatus().  
 * 
 * *****************  Version 19  *****************
 * User: Peter Lee    Date: 11/25/08   Time: 4:45p
 * Updated in $/control processor/code/drivers
 * SCR #109 Add _SensorValid() function
 * 
 * *****************  Version 18  *****************
 * User: Peter Lee    Date: 11/25/08   Time: 9:20a
 * Updated in $/control processor/code/drivers
 * Cleanup / Formatting / Remove debug code
 * 
 * *****************  Version 17  *****************
 * User: Peter Lee    Date: 11/24/08   Time: 6:37p
 * Updated in $/control processor/code/drivers
 * SCR #107 qar.createlogs
 * 
 * *****************  Version 16  *****************
 * User: Peter Lee    Date: 11/24/08   Time: 2:10p
 * Updated in $/control processor/code/drivers
 * SCR #97 QAR SW Updates
 * 
 * *****************  Version 15  *****************
 * User: Jim Mood     Date: 10/08/08   Time: 9:56a
 * Updated in $/control processor/code/drivers
 * SCR #87
 * 
 * *****************  Version 14  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 1:46p
 * Updated in $/control processor/code/drivers
 * 1) SCR #87 Function Prototype 
 * 2) Update call LogWriteSystem() to include NULL for ts
 * 
 * *****************  Version 13  *****************
 * User: Jim Mood     Date: 8/25/08    Time: 3:52p
 * Updated in $/control processor/code/drivers
 * SystemLogWrite was moved into LogManager.c and the label was changed to
 * LogWriteSystem.  Updated QAR.c to reflect this change  
 * 
 * *****************  Version 12  *****************
 * User: John Omalley Date: 6/18/08    Time: 2:40p
 * Updated in $/control processor/code/drivers
 * SCR-0044 - Added System Logs
 * 
 *
 *****************************************************************************/
