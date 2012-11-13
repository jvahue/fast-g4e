#define FPGA_MANAGER_BODY
/******************************************************************************
            Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:       FPGAManager.c

    Description: System level functions to provide BIT support for the FPGA 
                 interface 
    
    VERSION
      $Revision: 37 $  $Date: 12-11-13 1:56p $     

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include <math.h>

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "fpgamanager.h"
#include "stddefs.h"
#include "gse.h"

#include "clockmgr.h"

#include "FaultMgr.h"
#include "systemlog.h"

#include "FPGAUserTables.c" // Include user cmd tables  & functions

#include "TestPoints.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static FPGA_MGR_TASK_PARMS FpgaMgrBlock; 
static FPGA_CBIT_HEALTH_COUNTS FPGACBITHealthStatus; 
static BOOLEAN FpgaFailLogged;


/*****************************************************************************/
/* Local Constants                                                           */
/*****************************************************************************/
#ifdef GENERATE_SYS_LOGS
  // FPGA DRV System Logs - For Test Purposes
  static DRV_FPGA_BAD_VERSION_LOG_STRUCT DrvFpgaBadVersion[] = {
    {0x000C, 0xFFFC}
  }; 
  
  static DRV_FPGA_PBIT_GENERIC_LOG_STRUCT DrvFpgaNotConfigured[] = {
    {DRV_FPGA_NOT_CONFIGURED}
  }; 
  static DRV_FPGA_PBIT_GENERIC_LOG_STRUCT DrvFpgaConfiguredFail[] = {
    {DRV_FPGA_CONFIGURED_FAIL}
  }; 
  static DRV_FPGA_PBIT_GENERIC_LOG_STRUCT DrvFpgaIntFail[] = {
    {DRV_FPGA_INT_FAIL} 
  }; 
  static DRV_FPGA_PBIT_GENERIC_LOG_STRUCT DrvMCFSetupFail[] = {
    {DRV_FPGA_MCF_SETUP_FAIL} 
  }; 
  static DRV_FPGA_PBIT_GENERIC_LOG_STRUCT SysFpgaCrcFail[] = {
    {SYS_FPGA_CRC_FAIL} 
  }; 
  static DRV_FPGA_PBIT_GENERIC_LOG_STRUCT SysFpgaForceReloadFail[] = {
    {SYS_FPGA_FORCE_RELOAD_FAIL} 
  }; 
  static DRV_FPGA_PBIT_GENERIC_LOG_STRUCT SysFpgaForceReloadSuccess[] = {
    {SYS_FPGA_FORCE_RELOAD_SUCCESS} 
  }; 
  static FPGA_CBIT_REG_CHECK_LOG SysFpgaRegCheckFail[] = {
    {SYS_FPGA_CBIT_REG_FAIL_HW, FPGA_ENUM_CCR, 100, 0x66, 0x88}
  };
#endif

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void FPGAMgr_Monitor_HW_CRC( void ); 
static void FPGAMgr_CBIT_VerifyShadowReg( void ); 


/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:     FPGAMgr_Initialize
 *
 * Description:  Initializes FPGA Manager Processing 
 *
 * Parameters:   None
 *
 * Returns:      None
 *
 * Notes:        
 *  1) Sets up the FPGAMgr_CBIT_Task to run at 10 Hz (100 msec) 
 *  2) Expects FPGA driver initialization to be competed and 
 *     PBIT completed 
 *
 *****************************************************************************/
void FPGAMgr_Initialize (void)
{
  TCB TaskInfo;
  
  // Initializes Global Variables
  memset ( &FpgaMgrBlock, 0, sizeof(FpgaMgrBlock) ); 
  memset ( &FPGACBITHealthStatus, 0, sizeof(FPGA_CBIT_HEALTH_COUNTS));
  FpgaFailLogged = FALSE;

  if ( FPGA_GetStatus()->InitResult == DRV_OK ) 
  {
    // Creates the FPGACBIT Task
    memset(&TaskInfo, 0, sizeof(TaskInfo));
    strncpy_safe(TaskInfo.Name,sizeof(TaskInfo.Name),"FPGA Manager",_TRUNCATE);
    TaskInfo.TaskID         = FPGA_Manager;
    TaskInfo.Function       = FPGAMgr_CBIT_Task;
    TaskInfo.Priority       = taskInfo[FPGA_Manager].priority;
    TaskInfo.Type           = taskInfo[FPGA_Manager].taskType;
    TaskInfo.modes          = taskInfo[FPGA_Manager].modes;
    TaskInfo.MIFrames       = taskInfo[FPGA_Manager].MIFframes;
    TaskInfo.Rmt.InitialMif = taskInfo[FPGA_Manager].InitialMif;
    TaskInfo.Rmt.MifRate    = taskInfo[FPGA_Manager].MIFrate;
    TaskInfo.Enabled        = TRUE;
    TaskInfo.Locked         = FALSE;
    TaskInfo.pParamBlock    = &FpgaMgrBlock;
    TmTaskCreate (&TaskInfo);
  }
  
  // Add an entry in the user message handler table for FPGA user commands
  User_AddRootCmd(&FpgaRootTblPtr);
}


/******************************************************************************
 * Function:     FPGAMgr_CBIT_Task
 *
 * Description:  FPGA CBIT Processing 
 *
 * Parameters:   void *pParam
 *
 * Returns:      None
 *
 * Notes:        
 *  1) Monitors FPGA CRC Error Line.  If Asserted attempts to force re-configure
 *     of HW FPGA.  Waits ->MaxReloadTime or FPGA CRC Error Line asserted again 
 *     before declaring attempt failed.  Tries FPGA_MAX_RELOAD_RETRIES attempts 
 *     before declaring FPGA Failed and no more HW reconfigue is attempted.  
 *
 *****************************************************************************/
void FPGAMgr_CBIT_Task( void *pParam )
{
  // check for excessive FPGA interrupt fail
  if ( FPGA_GetStatus()->IntrFail && !FpgaFailLogged )
  {
    FPGA_INTR_FAIL_LOG intrLog;

    // Set SystemStatus to FAULTED
    FPGA_GetStatus()->SystemStatus = FPGA_STATUS_FAULTED; 

    // log error and set system condition
    intrLog.minIntrTime = FPGA_GetStatus()->MinIntrTime;
    Flt_SetStatus(STA_NORMAL, DRV_ID_FPGA_INTERRUPT_FAIL, &intrLog,
      sizeof(FPGA_INTR_FAIL_LOG));
    FpgaFailLogged = TRUE;
    GSE_DebugStr(NORMAL, TRUE,
      "\r\nFPGAMgr_CBIT_Task: Interrupt Failure (%d uSec). Interrupts Disabled\r\n",
      intrLog.minIntrTime);
  }
  
  FPGAMgr_Monitor_HW_CRC(); 
  
  if ( CM_GetTickCount() > FpgaMgrBlock.CbitRegCheckTimer ) 
  {
    FPGAMgr_CBIT_VerifyShadowReg(); 
    // Reset for the next 2 second 
    FpgaMgrBlock.CbitRegCheckTimer = CM_GetTickCount() + CM_DELAY_IN_SECS(2); 
  }
} 


/******************************************************************************
 * Function:     FPGAMgr_GetCBITHealthStatus
 *
 * Description:  Returns the current CBIT Health status of the FPGA
 *
 * Parameters:   None
 *
 * Returns:      Current Data in FPGA_CBIT_HEALTH_COUNTS
 *
 * Notes:        None
 *
 *****************************************************************************/
FPGA_CBIT_HEALTH_COUNTS FPGAMgr_GetCBITHealthStatus ( void )
{
  FPGACBITHealthStatus.SystemStatus = FPGA_GetStatus()->SystemStatus;
  FPGACBITHealthStatus.CRCErrCnt = FPGA_GetStatus()->CRCErrCnt; 
  FPGACBITHealthStatus.RegCheckFailCnt = FPGA_GetStatus()->RegCheckFailCnt; 
  
  return (FPGACBITHealthStatus); 
}


/******************************************************************************
 * Function:     FPGAMgr_CalcDiffCBITHealthStatus
 *
 * Description:  Calc the difference in CBIT Health Counts to support PWEH SEU
 *               (Used to support determining SEU cnt during data capture) 
 *
 * Parameters:   PrevCount - Initial count value
 *
 * Returns:      Diff in CBIT_HEALTH_COUNTS from (Current - PrevCnt)
 *
 * Notes:        None
 *
 *****************************************************************************/
FPGA_CBIT_HEALTH_COUNTS FPGAMgr_CalcDiffCBITHealthStatus ( FPGA_CBIT_HEALTH_COUNTS PrevCount )
{
  FPGA_CBIT_HEALTH_COUNTS DiffCount; 
  FPGA_CBIT_HEALTH_COUNTS *pCurrentCount; 

  FPGAMgr_GetCBITHealthStatus();
  
  pCurrentCount =  &FPGACBITHealthStatus; 
  
  DiffCount.SystemStatus = pCurrentCount->SystemStatus; 
  DiffCount.CRCErrCnt = pCurrentCount->CRCErrCnt - PrevCount.CRCErrCnt; 
  DiffCount.RegCheckFailCnt = pCurrentCount->RegCheckFailCnt - PrevCount.RegCheckFailCnt; 
  
  return (DiffCount); 
}


/******************************************************************************
 * Function:     FPGAMgr_AddPrevCBITHealthStatus
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
FPGA_CBIT_HEALTH_COUNTS FPGAMgr_AddPrevCBITHealthStatus ( FPGA_CBIT_HEALTH_COUNTS CurrCnt,
                                                          FPGA_CBIT_HEALTH_COUNTS PrevCnt )
{
  FPGA_CBIT_HEALTH_COUNTS AddCount; 
  
  AddCount.SystemStatus = CurrCnt.SystemStatus; 
  AddCount.CRCErrCnt = CurrCnt.CRCErrCnt + PrevCnt.CRCErrCnt; 
  AddCount.RegCheckFailCnt = CurrCnt.RegCheckFailCnt + PrevCnt.RegCheckFailCnt; 
  
  return (AddCount); 
}


/******************************************************************************
 * Function:     FPGA_CreateAllInternalLogs
 *
 * Description:  Creates all FPGA internal logs for testing / debug of log 
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
void FPGA_CreateAllInternalLogs ( void )
{
  // 1 Create DRV_ID_FPGA_PBIT_BAD_FPGA_VER:DRV_FPGA_BAD_VERSION 
  LogWriteSystem( DRV_ID_FPGA_PBIT_BAD_FPGA_VER, LOG_PRIORITY_LOW, &DrvFpgaBadVersion, 
                  sizeof(DRV_FPGA_BAD_VERSION_LOG_STRUCT), NULL );
                            
  // 2 Create DRV_ID_FPGA_PBIT_NOT_CONFIGURED:DRV_FPGA_NOT_CONFIGURED 
  LogWriteSystem( DRV_ID_FPGA_PBIT_NOT_CONFIGURED, LOG_PRIORITY_LOW, &DrvFpgaNotConfigured, 
                  sizeof(DRV_FPGA_PBIT_GENERIC_LOG_STRUCT), NULL );
                            
  // 3 Create DRV_ID_FPGA_PBIT_CONFIGURED_FAIL:DRV_FPGA_CONFIGURED_FAIL 
  LogWriteSystem( DRV_ID_FPGA_PBIT_CONFIGURED_FAIL, LOG_PRIORITY_LOW, &DrvFpgaConfiguredFail, 
                  sizeof(DRV_FPGA_PBIT_GENERIC_LOG_STRUCT), NULL );
                            
  // 4 Create DRV_ID_FPGA_PBIT_INT_FAIL:DRV_FPGA_INT_FAIL 
  LogWriteSystem( DRV_ID_FPGA_PBIT_INT_FAIL, LOG_PRIORITY_LOW, &DrvFpgaIntFail, 
                  sizeof(DRV_FPGA_PBIT_GENERIC_LOG_STRUCT), NULL );
                            
  // 5 Create DRV_ID_FPGA_PBIT_MCF_SETUP_FAIL: DRV_FPGA_MCF_SETUP_FAIL
  LogWriteSystem( DRV_ID_FPGA_MCF_SETUP_FAIL, LOG_PRIORITY_LOW, &DrvMCFSetupFail, 
                  sizeof(DRV_FPGA_PBIT_GENERIC_LOG_STRUCT), NULL ); 
  
  // 6 Create SYS_ID_FPGA_CBIT_CRC_FAIL:SYS_FPGA_CRC_FAIL 
  LogWriteSystem( SYS_ID_FPGA_CBIT_CRC_FAIL, LOG_PRIORITY_LOW, &SysFpgaCrcFail, 
                  sizeof(DRV_FPGA_PBIT_GENERIC_LOG_STRUCT), NULL );

  // 7 Create SYS_ID_FPGA_CBIT_FORCE_RELOAD_RESULT:SYS_FPGA_CRC_FAIL 
  LogWriteSystem( SYS_ID_FPGA_CBIT_FORCE_RELOAD_RESULT, LOG_PRIORITY_LOW, 
                  &SysFpgaForceReloadFail, 
                  sizeof(DRV_FPGA_PBIT_GENERIC_LOG_STRUCT), NULL );

  // 8 Create SYS_ID_FPGA_CBIT_FORCE_RELOAD_RESULT:SYS_FPGA_CRC_SUCCESS
  LogWriteSystem( SYS_ID_FPGA_CBIT_FORCE_RELOAD_RESULT, LOG_PRIORITY_LOW, 
                  &SysFpgaForceReloadSuccess, 
                  sizeof(DRV_FPGA_PBIT_GENERIC_LOG_STRUCT), NULL );

  // 9 Create SYS_ID_FPGA_CBIT_FORCE_RELOAD_RESULT:SYS_FPGA_CRC_SUCCESS
  LogWriteSystem( SYS_ID_FPGA_CBIT_REG_CHECK_FAIL, LOG_PRIORITY_LOW, &SysFpgaRegCheckFail, 
                  sizeof(FPGA_CBIT_REG_CHECK_LOG), NULL );
}
/*vcast_dont_instrument_end*/

#endif


/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:     FPGAMgr_Monitor_HW_CRC
 *
 * Description:  Monitor FPGA HW CRC Signal 
 *
 * Parameters:   None
 *
 * Returns:      None
 *
 * Notes:        
 *
 *  1) Monitors FPGA CRC Error Line.  If Asserted attempts to force re-configure
 *     of HW FPGA.  Waits ->MaxReloadTime or FPGA CRC Error Line asserted again 
 *     before declaring attempt failed.  Tries FPGA_MAX_RELOAD_RETRIES attempts 
 *     before declaring FPGA Failed and no more HW reconfigue is attempted.  
 *  2) Records SYS_ID_FPGA_CBIT_CRC_FAIL on detection of CRC 
 *     Records SYS_ID_FPGA_RECONFIGURE_RESULT on success or failure of reconfigure
 *
 *****************************************************************************/
static
void FPGAMgr_Monitor_HW_CRC( void  )
{
  BOOLEAN bReloadFPGA; 
  RESULT TempData; 
  
  bReloadFPGA = FALSE; 

  // Determine current FPGA System Status 
  if ( FPGA_GetStatus()->SystemStatus == FPGA_STATUS_OK ) 
  {
    // Check FPGA CRC Error Line 
    if ( TPU( FPGA_Monitor_Ok(), eTpFpga2997) != TRUE )
    {
      // Log this Condition 
      TempData = SYS_FPGA_CRC_FAIL; 
      Flt_SetStatus( STA_CAUTION, SYS_ID_FPGA_CBIT_CRC_FAIL, &TempData, 
                     sizeof(TempData) ); 
                            
      GSE_DebugStr(NORMAL,TRUE,"\r\nFPGA System Log: FPGA HW CRC Failed Asserted\r\n");

      // Force HW ReLoad of the FPGA 
      bReloadFPGA = TRUE; 
      
      // Inc cnt of this condition 
      FPGA_GetStatus()->CRCErrCnt += 1; 
    }
  }
  // FPGA System Status != FPGA_STATUS_OK 
  else 
  {
    if ( FPGA_GetStatus()->SystemStatus == FPGA_STATUS_FORCERELOAD ) 
    { 
      // Check to see if Force Reload Done 
      if ( FPGA_ForceReload_Status() == TRUE) 
      {
      
        // Must wait at least 5 msec per FPGA Data Sheet 
        //  before CRC Error de-asserts.  After Re-Configure is completed, 
        //  FPGA will perform one CRC check and if good de-asserting CRC Error. 
        TTMR_Delay(TICKS_PER_10msec);
      
        // Check if CRC Error During Cfg Reload 
        if ( TPU( FPGA_Monitor_Ok(), eTpFpga2997) != TRUE ) 
        {
          // Force HW ReLoad of the FPGA again !
          bReloadFPGA = TRUE; 
        }
        // FPGA Reload Success 
        else 
        {
          // ReLoad is OK ! Also log this condition 
          FPGA_GetStatus()->SystemStatus = FPGA_STATUS_OK; 
          FPGA_GetStatus()->ReloadTries = 0; 
          
          TempData = SYS_FPGA_FORCE_RELOAD_SUCCESS; 
          Flt_SetStatus( STA_NORMAL, SYS_ID_FPGA_CBIT_FORCE_RELOAD_RESULT, &TempData, 
                         sizeof(TempData) ); 
          GSE_DebugStr(NORMAL,TRUE,"\r\nFPGA CBIT Task: FPGA HW Force Reload OK\r\n");
          Flt_ClrStatus(STA_CAUTION); 
        }
      } // End of if ( FPGA_Monitor_Ok() != TRUE )
      else  // ForceReload Not Done 
      {
        // Check for Reload Time Out 
        // Force HW Reload of the FPGA again ! 
        bReloadFPGA = FPGA_GetStatus()->MaxReloadTime < CM_GetTickCount(); 

      } // End else (ForceReload != Done) 
    } // End of if ( SystemStatus == FPGA_STATUS_FORCERELOAD ) 
    // else 
    // {
    //   SystemStatus is FAULTED do nothing at this point ! 
    // }
    
  } // End of else ( != FPGA_STATUS_OK )
  
  if ( bReloadFPGA == TRUE )
  {
    if ( FPGA_GetStatus()->ReloadTries > FPGA_MAX_RELOAD_RETRIES ) 
    {
      // Log this Condition 
      TempData = SYS_FPGA_FORCE_RELOAD_FAIL; 
      Flt_SetStatus( STA_FAULT, SYS_ID_FPGA_CBIT_FORCE_RELOAD_RESULT, &TempData, 
                     sizeof(TempData) ); 
      GSE_DebugStr(NORMAL,TRUE,"\r\nFPGA System Log: FPGA HW Force Reconfigure Failed\r\n");
      
      // Set SystemStatus to FAULTED
      FPGA_GetStatus()->SystemStatus = FPGA_STATUS_FAULTED; 
    }
    else 
    {
      FPGA_ForceReload(); 
      
      // Set the Reload Timer and ReloadTries
      FPGA_GetStatus()->MaxReloadTime = CM_GetTickCount() + (MILLISECONDS_PER_SECOND * 50);
      FPGA_GetStatus()->ReloadTries += 1; 
    }
  } // End of if (bReloadFPGA == TRUE)
}


/******************************************************************************
 * Function:     FPGAMgr_CBIT_VerifyShadowReg
 *
 * Description:  Verify RAM Shadow Copy of FPGA reg setting with physical
 *               FPGA reg settings
 *
 * Parameters:   None
 *
 * Returns:      None
 *
 * Notes:        
 *
 *  1) Performs a 2 out of 3 comparison (2 shadow copy and HW reg) to determine 
 *     if reg setting is corrupted due to SEU
 *  2) Returns the data on the first failure 
 *
 *****************************************************************************/
static
void FPGAMgr_CBIT_VerifyShadowReg( void  )
{
  FPGA_REG_CHECK_PTR fpgaRegCheck; 
  UINT16 hw_reg, shadow1_reg, shadow2_reg; 
  FPGA_CBIT_REG_CHECK_LOG log; 
  UINT16 nCnt, i;
  UINT16 actual, expected; 

  // Compare the two RAM copies first, if different than record mismatch 

  // If both RAM OK, compare to HW, if different than record mismatch 
  
  // If two RAM copies are different, update RAM copy that differs from 
  //   HW (2 out of 3 vote) 
  
  fpgaRegCheck = FPGA_GetRegCheckTbl ( &nCnt ); 
  log.ResultCode = SYS_OK;   // Will get set to first failure only 
  log.nFailures = 0;         // Counts the number of register check failures on this pass
  log.Expected = 0;
  log.Actual = 0; 
  
  // Compare Shadow RAM 1 and 2 First, if same then compare to FPGA HW Reg
  for (i = 0; i < nCnt; i++)
  {
    hw_reg = *fpgaRegCheck->ptrReg & fpgaRegCheck->mask; 
    shadow1_reg = *fpgaRegCheck->shadow1 & fpgaRegCheck->mask;  
    shadow2_reg = *fpgaRegCheck->shadow2 & fpgaRegCheck->mask;  
    
    // Compare Shadow Copies 1 and 2 and HW Reg
    if ( (hw_reg != shadow1_reg) || ( hw_reg != shadow2_reg) )
    {
      // Update nFailures Count 
      log.nFailures++; 
    
      // We have a problem.  Determine if Shadow RAM is bad 
      if ( shadow1_reg != shadow2_reg ) 
      {
        // Determine which Shadow RAM is 'bad' 
        if ( shadow1_reg != hw_reg ) 
        {
          actual = shadow1_reg; 
          expected = hw_reg; 
          // Update shadow1_reg setting
          *fpgaRegCheck->shadow1 = hw_reg; 
        }
        else 
        {
          actual = shadow2_reg; 
          expected = hw_reg; 
          // Update shadow2_reg setting
          *fpgaRegCheck->shadow2 = hw_reg;  
        }
        
        // Set Result Code to Shadow Ram Failure 
        if ( log.ResultCode == SYS_OK ) 
        {
          log.ResultCode = SYS_FPGA_CBIT_REG_FAIL_SW;
          log.Actual = actual; 
          log.Expected = expected; 
          log.enumReg = fpgaRegCheck->enumReg; 
        }
      }
      else 
      {
        // Set Result Code to HW Failure 
        if ( log.ResultCode == SYS_OK ) 
        {
          log.ResultCode = SYS_FPGA_CBIT_REG_FAIL_HW; 
          log.Actual = hw_reg; 
          log.Expected = shadow1_reg; 
          log.enumReg = fpgaRegCheck->enumReg; 
        }
      
        // Hw Reg Failure. Force update of Reg (per PWEH req !) 
        *fpgaRegCheck->ptrReg = shadow1_reg; 
      } 
    } 
    
    // Move to next register to check 
    fpgaRegCheck++; 
    
  } // End for loop 
  
  if (log.nFailures > 0)
  {
    // Log System Log
    Flt_SetStatus( STA_CAUTION, SYS_ID_FPGA_CBIT_REG_CHECK_FAIL, &log, 
                   sizeof(FPGA_CBIT_REG_CHECK_LOG) ); 
    // Cummulative Reg Check Failures 
    FPGA_GetStatus()->RegCheckFailCnt += log.nFailures; 
  }
  
}

/*****************************************************************************
 *  MODIFICATIONS
 *    $History: FPGAManager.c $
 * 
 * *****************  Version 37  *****************
 * User: Melanie Jutras Date: 12-11-13   Time: 1:56p
 * Updated in $/software/control processor/code/system
 * SCR #1142 File Format Error
 * 
 * *****************  Version 36  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 35  *****************
 * User: Jeff Vahue   Date: 11/01/10   Time: 7:50p
 * Updated in $/software/control processor/code/system
 * SCR# 975 - Code Coverage refactor
 * 
 * *****************  Version 34  *****************
 * User: John Omalley Date: 10/14/10   Time: 1:52p
 * Updated in $/software/control processor/code/system
 * SCR 879 - Added delays for FPGA CRC error processing
 * 
 * *****************  Version 33  *****************
 * User: Contractor2  Date: 9/24/10    Time: 3:16p
 * Updated in $/software/control processor/code/system
 * SCR #840 Code Review Updates
 * 
 * *****************  Version 32  *****************
 * User: Peter Lee    Date: 9/03/10    Time: 6:07p
 * Updated in $/software/control processor/code/system
 * SCR #837 Misc - FPGA Reconfigure Command must be assert for min of 500
 * nsec
 * 
 * *****************  Version 31  *****************
 * User: Peter Lee    Date: 8/31/10    Time: 11:59a
 * Updated in $/software/control processor/code/system
 * SCR #840 Code Review Updates
 * 
 * *****************  Version 30  *****************
 * User: Jeff Vahue   Date: 8/28/10    Time: 12:22p
 * Updated in $/software/control processor/code/system
 * SCR# 830 - GENERATE_SYS_LOGS exclude area from coverage results
 * 
 * *****************  Version 29  *****************
 * User: Peter Lee    Date: 8/18/10    Time: 4:16p
 * Updated in $/software/control processor/code/system
 * SCR #793 Set Status to Normal for FPGA INT Failure 
 * 
 * *****************  Version 28  *****************
 * User: Jeff Vahue   Date: 8/02/10    Time: 4:49p
 * Updated in $/software/control processor/code/system
 * SCR# 707 - Add V&V Test Points
 * 
 * *****************  Version 27  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:10a
 * Updated in $/software/control processor/code/system
 * SCR #698 - Fix code review findings
 * 
 * *****************  Version 26  *****************
 * User: Contractor2  Date: 7/09/10    Time: 4:32p
 * Updated in $/software/control processor/code/system
 * SCR #8 Implementation: External Interrupt Monitors
 * 
 * *****************  Version 25  *****************
 * User: Contractor3  Date: 6/25/10    Time: 9:46a
 * Updated in $/software/control processor/code/system
 * SCR #662 - Changes based on code review
 * 
 * *****************  Version 24  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:54p
 * Updated in $/software/control processor/code/system
 * SCR #587 Change TmTaskCreate to return void
 * 
 * *****************  Version 23  *****************
 * User: Contractor2  Date: 5/06/10    Time: 2:10p
 * Updated in $/software/control processor/code/system
 * SCR #579 Change LogWriteSys to return void
 * 
 * *****************  Version 22  *****************
 * User: Jeff Vahue   Date: 4/12/10    Time: 5:43p
 * Updated in $/software/control processor/code/system
 * SCR #541 - all XXX_CreateLogs functions are now dependent on
 * GENERATE_SYS_LOGS
 * 
 * *****************  Version 21  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:10p
 * Updated in $/software/control processor/code/system
 * SCR #317 Implement safe strncpy
 * 
 * *****************  Version 20  *****************
 * User: Peter Lee    Date: 3/29/10    Time: 6:41p
 * Updated in $/software/control processor/code/system
 * SCR #517 Error in Bad FPGA Version Failed Log
 * 
 * *****************  Version 19  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:36p
 * Updated in $/software/control processor/code/system
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 * 
 * *****************  Version 18  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/system
 * SCR# 483 - Function Names
 * 
 * *****************  Version 17  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 16  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 5:06p
 * Updated in $/software/control processor/code/system
 * SCR# 397
 * 
 * *****************  Version 15  *****************
 * User: Peter Lee    Date: 12/22/09   Time: 2:39p
 * Updated in $/software/control processor/code/system
 * SCR #237 Replace LogWriteSystem() with Flt_SetStatus()
 * 
 * *****************  Version 14  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 2:11p
 * Updated in $/software/control processor/code/system
 * SCR# 326
 * 
 * *****************  Version 13  *****************
 * User: Contractor V&v Date: 12/10/09   Time: 5:45p
 * Updated in $/software/control processor/code/system
 * SCR 106
 * 
 * *****************  Version 12  *****************
 * User: Contractor V&v Date: 11/30/09   Time: 5:37p
 * Updated in $/software/control processor/code/system
 * SCR 106 XXXUserTable.c consistency
 * 
 * *****************  Version 11  *****************
 * User: Peter Lee    Date: 11/20/09   Time: 11:48a
 * Updated in $/software/control processor/code/system
 * Update FPGA Reg Check to be 2 Hz.
 * 
 * *****************  Version 10  *****************
 * User: Peter Lee    Date: 11/12/09   Time: 10:12a
 * Updated in $/software/control processor/code/system
 * SCR #315 PWEH SEU Count Processing Support
 * 
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 10/15/09   Time: 9:58a
 * Updated in $/software/control processor/code/system
 * SCR 115, 116 and FPGA CBIT requirements
 * 
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 9/15/09    Time: 6:17p
 * Updated in $/software/control processor/code/system
 * Add additional FPGA Result Code for _MCF_SET_FAIL. 
 * 
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 5/21/09    Time: 4:39p
 * Updated in $/software/control processor/code/system
 * SCR #193 Informal Code Review Updates
 * 
 * *****************  Version 6  *****************
 * User: Peter Lee    Date: 2/02/09    Time: 11:14a
 * Updated in $/control processor/code/system
 * Uncomment monitoring of FPGA Err Input
 * 
 * *****************  Version 5  *****************
 * User: Peter Lee    Date: 1/29/09    Time: 2:12p
 * Updated in $/control processor/code/system
 * Updated System Log ID
 * 
 * *****************  Version 4  *****************
 * User: Peter Lee    Date: 1/23/09    Time: 10:55a
 * Updated in $/control processor/code/system
 * Initialize FPGACBITHealthStatus on startup
 * 
 * *****************  Version 3  *****************
 * User: Peter Lee    Date: 1/22/09    Time: 6:13p
 * Updated in $/control processor/code/system
 * SCR #136 Pack system log structures
 * 
 * *****************  Version 2  *****************
 * User: Peter Lee    Date: 1/21/09    Time: 11:33a
 * Updated in $/control processor/code/system
 * Misc Update Debug CreateSystemLogs to use correct SysFpgaCrcFail for
 * CBIT FPGA CRC Fail indicated. 
 * 
 * *****************  Version 1  *****************
 * User: Peter Lee    Date: 1/06/09    Time: 2:43p
 * Created in $/control processor/code/system
 * SCR 129 Misc FPGA Updates
 * 
 *
 *****************************************************************************/

