#ifndef CBIT_MANAGER_H
#define CBIT_MANAGER_H

/******************************************************************************
            Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:         CBITManager.h
    
    Description: Continuous Built In Test Definitions.
    
    VERSION
      $Revision: 22 $  $Date: 8/28/12 1:43p $
    
******************************************************************************/


/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    



/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
// For PWEH SEU Healt Count Processing
#include "FpgaManager.h"
#include "QarManager.h"
#include "Arinc429mgr.h"
#include "TaskManager.h"


/******************************************************************************
                                 Package Defines
******************************************************************************/

/******************************************************************************
                                 Package Typedefs
******************************************************************************/

#pragma pack(1)

// CBIT Health Status 
typedef struct 
{
  UINT32  RAMFailCnt;        // Number of RAM location test failures 
  UINT32  ProgramCRCErrCnt;  // Number of times Program CRC check has failed
  UINT32  RegCheckCnt;       // Number of times RegCheckCnt
} CBIT_HEALTH_COUNTS; 

// CBIT Reg Check Failed
typedef struct 
{
  UINT32 RegAddr;    // RegAddr of first failure / bad compare
  UINT32 enumReg;    // Enum of Reg of first failure / bad compare 
  UINT32 nFailures;  // Number of total failures encountered on this iteration 
} CBIT_REG_CHECK_FAILED; 

// CBIT CRC Check Failed
typedef struct 
{
  UINT32 *crcAddr;      // Address of actual CRC value
  UINT32 computedCrc;   // Computed CRC value
  UINT32 actualCrc;     // Actual CRC value
} CBIT_CRC_CHECK_FAILED; 

// CBIT Ram Check Failed
typedef struct 
{
  UINT32 *ramAddr;      // Address of failed ram
  UINT32 writePattern;  // written pattern
  UINT32 readPattern;   // pattern read back
} CBIT_RAM_CHECK_FAILED; 

// PSC Port Interrupt Monitor Failed
typedef struct {
  INT8  port; 
} UART_INTR_FAIL_LOG; 

#pragma pack()


typedef struct 
{
  UINT16  numReg;      // Number of Reg in the Check Table 

  UINT32  RAMFailCnt;         // Number of RAM location test failures 
  UINT32  ProgramCRCErrCnt;   // Number of times Program CRC check has failed
  UINT32  RegCheckCnt;        // Number of times RegCheckCnt
} CBIT_MGR_STATUS, CBIT_MGR_STATUS_PTR; 


// PWEH SEU Counts 
typedef enum 
{
  PWEH_SEU_STARTUP,                  // System Startup 
  PWEH_SEU_RESUME_RECORDING,         // State to determine if Recording interrutped by 
                                     //     reset/power down. 
  PWEH_SEU_WAITING_FOR_RECORDING,    // Waiting for Recording to begin 
  PWEH_SEU_RECORDING,                // Recording in progress 
  PWEH_SEU_STATE_MAX, 
} PWEH_SEU_STATE; 

typedef struct 
{
  PWEH_SEU_STATE state; 
  UINT32 timer;             // GP TICK timer used for determing elapsed time 
  BOOLEAN pmGlitchDetected;  
  RESULT EepromOpenResult;  // Result of call to NV_Open() for stored EEPROM SEU data 
} PWEH_SEU_STATUS; 

// Reset Health Status 
typedef struct 
{
  UINT32  nWatchdogResets;    // Number of Watchdog Resets (Asserts)
  UINT32  nReset;             // Number of Non-Watchdog Reset
} RESET_HEALTH_COUNTS; 

#pragma pack(1)
typedef struct 
{
  BOOLEAN SEUCounts_Interrupted;   // TRUE if SEU Counts interrupted 
                                   //   by shutdown.  Possible loss of
                                   //   count data as End of Recording Not Seen. 
  
  // CBIT_HEALTH_COUNTS
  CBIT_HEALTH_COUNTS cbit; 
  
  // FPGA_CBIT_HEALTH_COUNTS
  FPGA_CBIT_HEALTH_COUNTS  fpga;
  
  // QAR_CBIT_HEALTH_COUNTS
  QAR_CBIT_HEALTH_COUNTS  qar; 
  
  // ARINC429_CBIT_HEALTH_COUNTS
  ARINC429_CBIT_HEALTH_COUNTS arinc429; 
  
  // Task Manager Health Counts
  TM_HEALTH_COUNTS tm; 
  
  // Reset Health Counts
  RESET_HEALTH_COUNTS reset;

  // Log Data Flash Health Counts
  LOG_CBIT_HEALTH_COUNTS  log;

} PWEH_SEU_HEALTH_COUNTS; 
#pragma pack()


typedef struct 
{
  BOOLEAN bRecInProgress;         // TRUE if Record was in progress when box reset
                                  // 
                                  
  BOOLEAN bIntRecInProgress;      // Recording when reset / shutdown occurred.  
                                  //    Recording has been interrupted.                                    
                                  
  BOOLEAN bIntSaveCntCompleted;   // Attempted save of cnts (when rec in progress) 
                                  //     during box reset succeeded 
                                  
  BOOLEAN bEEPromDataCorrupted;   // EEPROM data corrupted at some point.  
                                  //     Only when Count data is logged, will 
                                  //     this flag be cleared.                                  
                                  
  PWEH_SEU_HEALTH_COUNTS Counts;  // Current running health counts during data catpure 
  
} PWEH_SEU_EEPROM_DATA; 




/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( CBIT_MANAGER_BODY )
   #define EXPORT
#else
   #define EXPORT extern
#endif

/******************************************************************************
                             Package Exports Variables
******************************************************************************/
EXPORT CBIT_MGR_STATUS m_CbitMgrStatus; 
EXPORT PWEH_SEU_STATUS m_PWEHSEUStatus; 

/******************************************************************************
                             Package Exports Functions
******************************************************************************/
EXPORT void CBITMgr_Initialize (void); 
EXPORT void CBITMgr_UpdateFlightState(BOOLEAN inFlight);
EXPORT void CBITMgr_PWEHSEU_Initialize (void); 
EXPORT BOOLEAN CBITMgr_PWEHSEU_FileInit ( void );

#ifdef GENERATE_SYS_LOGS
  EXPORT void CBIT_CreateAllInternalLogs ( void );
#endif  

#endif // CBIT_MANAGER_H        
/*****************************************************************************
 *  MODIFICATIONS
 *    $History: CBITManager.h $
 * 
 * *****************  Version 22  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 21  *****************
 * User: John Omalley Date: 10/12/10   Time: 2:42p
 * Updated in $/software/control processor/code/system
 * SCR 826 - Code Review Updates
 * 
 * *****************  Version 20  *****************
 * User: Jeff Vahue   Date: 9/03/10    Time: 8:22p
 * Updated in $/software/control processor/code/system
 * SCR# 853 - UART Int Monitor
 * 
 * *****************  Version 19  *****************
 * User: Contractor2  Date: 6/30/10    Time: 11:04a
 * Updated in $/software/control processor/code/system
 * SCR #150 Implementation: SRS-3662 - Add Data Flash corruption CBIT
 * counts.
 * 
 * *****************  Version 18  *****************
 * User: Contractor2  Date: 6/14/10    Time: 1:05p
 * Updated in $/software/control processor/code/system
 * SCR #483 Function Names must begin with the CSC it belongs with.
 * 
 * *****************  Version 17  *****************
 * User: Contractor V&v Date: 6/08/10    Time: 5:53p
 * Updated in $/software/control processor/code/system
 * SCR #614 fast.reset=really should initialize files
 * 
 * *****************  Version 16  *****************
 * User: John Omalley Date: 6/08/10    Time: 12:13p
 * Updated in $/software/control processor/code/system
 * SCR 627 - Updated for LJ60
 * 
 * *****************  Version 15  *****************
 * User: Contractor2  Date: 6/07/10    Time: 2:16p
 * Updated in $/software/control processor/code/system
 * SCR #628 Count SEU resets during data capture
 * 
 * *****************  Version 14  *****************
 * User: Contractor2  Date: 5/25/10    Time: 3:01p
 * Updated in $/software/control processor/code/system
 * SCR #563 Implementation: Req SRS-1811,1813,1814,1815,3162 - CBIT RAM
 * Tests
 * 
 * *****************  Version 13  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:55p
 * Updated in $/software/control processor/code/system
 * SCR #587 Change TmTaskCreate to return void
 * 
 * *****************  Version 12  *****************
 * User: Contractor2  Date: 5/06/10    Time: 2:10p
 * Updated in $/software/control processor/code/system
 * SCR #579 Change LogWriteSys to return void
 * 
 * *****************  Version 11  *****************
 * User: Contractor V&v Date: 4/30/10    Time: 11:28a
 * Updated in $/software/control processor/code/system
 * SCR #574 System Level objects should not be including Applicati
 * 
 * *****************  Version 10  *****************
 * User: Contractor2  Date: 4/23/10    Time: 2:08p
 * Updated in $/software/control processor/code/system
 * Implementation: Req SRS-1811,1813,1814,1815,3162 - CBIT RAM Tests
 * 
 * *****************  Version 9  *****************
 * User: Jeff Vahue   Date: 4/12/10    Time: 5:43p
 * Updated in $/software/control processor/code/system
 * SCR #541 - all XXX_CreateLogs functions are now dependent on
 * GENERATE_SYS_LOGS
 * 
 * *****************  Version 8  *****************
 * User: Contractor2  Date: 4/06/10    Time: 3:56p
 * Updated in $/software/control processor/code/system
 * SCR 525 - Implement periodic CBIT CRC test.
 * 
 * *****************  Version 7  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:57p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 6  *****************
 * User: Peter Lee    Date: 1/25/10    Time: 2:39p
 * Updated in $/software/control processor/code/system
 * SCR #414 Error - Primary and Backup PWEH SEU Cnt Restore Failure 
 * 
 * *****************  Version 5  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 5:04p
 * Updated in $/software/control processor/code/system
 * SCR# 397
 * 
 * *****************  Version 4  *****************
 * User: Peter Lee    Date: 11/19/09   Time: 3:22p
 * Updated in $/software/control processor/code/system
 * SCR #315 SEU counts for TM Health Counts 
 * 
 * *****************  Version 3  *****************
 * User: Peter Lee    Date: 11/19/09   Time: 10:48a
 * Updated in $/software/control processor/code/system
 * SCR #315 Fix bug with maintaining SEU count across power cycle.
 * 
 * *****************  Version 2  *****************
 * User: Peter Lee    Date: 11/02/09   Time: 11:44a
 * Updated in $/software/control processor/code/system
 * SCR #315 CBIT and SEU Processing.   User Table for CBIT Manager. 
 * 
 * *****************  Version 1  *****************
 * User: Peter Lee    Date: 10/23/09   Time: 9:34a
 * Created in $/software/control processor/code/system
 * Initial Check In.  SCR #315
 * 
 *
 *****************************************************************************/
                                            
