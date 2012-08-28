#define CBIT_MANAGER_BODY
/******************************************************************************
            Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:       CBITManager.c

    Description: File containing all functions related to Continuous Built In Test.
    
    VERSION
      $Revision: 54 $  $Date: 4/11/11 5:24p $     

******************************************************************************/


/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "CBITManager.h"
#include "UtilRegSetCheck.h" 
#include "GSE.h"
#include "LogManager.h"
#include "NVMgr.h"
#include "ClockMgr.h"
#include "PowerManager.h"
#include "Assert.h"
#include "Utility.h"
#include "WatchDog.h"

#include "CBITManagerUserTable.c" // Include cmd tables & functions .c

#include "TestPoints.h"

#if defined(WIN32) 
    UINT32  __CRC_START = 0x00000000;
    UINT32  __CRC_END = 0xffffffff;
    UINT32  __ghs_ramstart[1024];
    UINT32  __ghs_allocatedramend;
    UINT32  __SP_INIT;
    UINT32  __SP_END[1024];
#else
    // addresses provided by linker
    extern UINT32  __CRC_START;
    extern UINT32  __CRC_END;
    extern UINT32  __ghs_allocatedramend;
    extern UINT32  __SP_INIT;
    extern UINT32  __SP_END;
    #if defined(STE_TP) 
        // For STE_TP we do not want to do the entire RAM as we are 
        // executing coverage instructions while we run the RAM test
        // and that screws up the coverage pointer contents
        // so just set aside a little memory to run the test on
        UINT32  __ghs_ramstart[1024];
    #else
        extern UINT32  __ghs_ramstart;
    #endif
#endif

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define RAM_TEST_SECTIONS 2

// Will assume that for in air reset, that recording will restart again 
//   < 10 seconds, otherwise this #define should be updated to reflect 
//   worse case startup time. 
#define PWEH_INTERRUPTED_STARTUP_DELAY_S  10 

#define MAX_REG_CHECK_LOGS                10 
#define MAX_REG_ENTRIES                   10

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/

static CBIT_HEALTH_COUNTS CBITHealthCounts; 

static RESET_HEALTH_COUNTS ResetHealthCounts;

static PWEH_SEU_EEPROM_DATA PwehSeuEepromData;

// Snapshot of current count value just as data recording begins.  Used to 
//   subtract any count values accumulated before start of recording.  
static PWEH_SEU_HEALTH_COUNTS PwehSeuCountsAtStartOfRec; 
static PWEH_SEU_HEALTH_COUNTS PwehSeuCountsPrevStored;

// Boolean flag maintained by FAST of the current in-flight state
static BOOLEAN InFlight;

// has the port logged a failed Interrupt monitor
static BOOLEAN IntFailLogged[UART_NUM_OF_UARTS];  

/*****************************************************************************/
/* Local Constants                                                           */
/*****************************************************************************/
#ifdef GENERATE_SYS_LOGS
  // CBIT REG Check Failure log 
  static const CBIT_REG_CHECK_FAILED CbitRegCheckFailLog[] = 
  {
    { 0x100666, 0x66, 77 }  // RegAddr, enumReg, nFailures 
  };
  
  static const PWEH_SEU_HEALTH_COUNTS CbitPwehSeuCountLog[] = 
  {
    { TRUE,
      {1, 2, 3},
      {FPGA_STATUS_FAULTED, 4, 5},
      {QAR_STATUS_FAULTED, TRUE, QAR_LOSF, 6, 7, 8},
      { 
        10, 11, 12, 13, 14, 15, ARINC429_STATUS_FAULTED_PBIT,
        20, 21, 22, 23, 24, 25, ARINC429_STATUS_FAULTED_CBIT,
        30, 31, 32, 33, 34, 35, ARINC429_STATUS_DISABLED_OK,
        40, 41, 42, 43, 44, 45, ARINC429_STATUS_DISABLED_FAULTED,
      }
    }
  }; 
#endif

static const UINT16  RamTestPattern = 0xa5a5;  // Initial Ram Test Pattern

/***************************************************************
* The following is used to ensure the PBIT CRC test will pass
* during development testing when the program is loaded via the
* GHS tools.  The code is setup to accept the computed CRC or
* 0xFFFFFFFF.  The expectation was that the FLASH would be 
* erased leaving any unused memory at 0xFF, this is not the 
* case when using GHS tools.
* This value will be over written by the actual CRC when a full
* release build is made.
****************************************************************/
#ifndef WIN32
#pragma ghs section text=".crcDefault"
#ifdef RELEASE
const UINT32  CRC_DEFAULT = 0x00000000;     // Release build default CRC
#else
const UINT32  CRC_DEFAULT = 0xffffffff;     // Debug build default CRC
#endif
#pragma ghs section text=default
#endif

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static PWEH_SEU_STATE CBITMgr_PWEHSEU_Startup( void ); 
static PWEH_SEU_STATE CBITMgr_PWEHSEU_WaitForRec ( void ); 
static PWEH_SEU_STATE CBITMgr_PWEHSEU_ResumeRec ( void ); 
static PWEH_SEU_STATE CBITMgr_PWEHSEU_Rec ( void ); 

static PWEH_SEU_HEALTH_COUNTS CBITMgr_PWEHSEU_GetAllCurrentCountValues ( void ); 
static PWEH_SEU_HEALTH_COUNTS CBITMgr_PWEHSEU_GetDiffCountValues (
                                                PWEH_SEU_HEALTH_COUNTS PrevCount); 
static PWEH_SEU_HEALTH_COUNTS CBITMgr_PWEHSEU_AddPrevCountValues (
                                                PWEH_SEU_HEALTH_COUNTS CurrCnt, 
                                                PWEH_SEU_HEALTH_COUNTS PrevCnt ); 

static void CBITMgr_PWEHSEU_UpdateSeuEepromData ( void ); 

static CBIT_HEALTH_COUNTS CBITMgr_AddPrevCBITHealthStatus ( CBIT_HEALTH_COUNTS CurrCnt,
                                                            CBIT_HEALTH_COUNTS PrevCnt );
static CBIT_HEALTH_COUNTS CBITMgr_CalcDiffCBITHealthStatus ( CBIT_HEALTH_COUNTS PrevCnt );  
static CBIT_HEALTH_COUNTS CBITMgr_GetCBITHealthStatus ( void ); 
static void CBITMgr_Task( void *pParam ); 
static void CBITMgr_CRC_Task(void* pParam);
static void CBITMgr_Ram_Task(void* pParam);
static void CBITMgr_PWEHSEU_Task ( void *pParam ); 
static BOOLEAN CBITMgr_PWEHSEU_ShutDown ( void );


/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:     CBITMgr_Initialize
 *
 * Description:  Initializes CBIT Manager Processing 
 *
 * Parameters:   none
 *
 * Returns:      none
 *
 * Notes:        none 
 *
 *****************************************************************************/
void CBITMgr_Initialize (void)
{
  TCB TaskInfo;

  memset ( (void *) &m_CbitMgrStatus, 0x00, sizeof(CBIT_MGR_STATUS)); 
  memset ( (void *) &IntFailLogged,   0x00, sizeof(IntFailLogged));

  //Add an entry in the user message handler table
  User_AddRootCmd(&CbitMgrRootTblPtr);
  
  // Creates the CBITMgr Task
  memset(&TaskInfo, 0, sizeof(TaskInfo));
  strncpy_safe(TaskInfo.Name, sizeof(TaskInfo.Name),"CBIT Manager",_TRUNCATE);
  TaskInfo.TaskID         = CBIT_Manager;
  TaskInfo.Function       = CBITMgr_Task;
  TaskInfo.Priority       = taskInfo[CBIT_Manager].priority;
  TaskInfo.Type           = taskInfo[CBIT_Manager].taskType;
  TaskInfo.modes          = taskInfo[CBIT_Manager].modes;
  TaskInfo.MIFrames       = taskInfo[CBIT_Manager].MIFframes;  // 25 Hz Update
  TaskInfo.Rmt.InitialMif = taskInfo[CBIT_Manager].InitialMif;
  TaskInfo.Rmt.MifRate    = taskInfo[CBIT_Manager].MIFrate;
  TaskInfo.Enabled        = TRUE;
  TaskInfo.Locked         = FALSE;
  TaskInfo.pParamBlock    = NULL;
  TmTaskCreate (&TaskInfo);
  
  memset(&TaskInfo, 0, sizeof(TaskInfo));
  strncpy_safe(TaskInfo.Name, sizeof(TaskInfo.Name),"CBIT CRC",_TRUNCATE);
  TaskInfo.TaskID         = CRC_CBIT_Task;
  TaskInfo.Function       = CBITMgr_CRC_Task;
  TaskInfo.Priority       = taskInfo[CRC_CBIT_Task].priority;
  TaskInfo.Type           = taskInfo[CRC_CBIT_Task].taskType;
  TaskInfo.modes          = taskInfo[CRC_CBIT_Task].modes;
  TaskInfo.MIFrames       = taskInfo[CRC_CBIT_Task].MIFframes;
  TaskInfo.Rmt.InitialMif = taskInfo[CRC_CBIT_Task].InitialMif;
  TaskInfo.Rmt.MifRate    = taskInfo[CRC_CBIT_Task].MIFrate;
  TaskInfo.Enabled        = TRUE;
  TaskInfo.Locked         = FALSE;
  TaskInfo.pParamBlock    = NULL;
  TmTaskCreate (&TaskInfo);
  
  memset(&TaskInfo, 0, sizeof(TaskInfo));
  strncpy_safe(TaskInfo.Name, sizeof(TaskInfo.Name),"CBIT RAM",_TRUNCATE);
  TaskInfo.TaskID         = Ram_CBIT_Task;
  TaskInfo.Function       = CBITMgr_Ram_Task;
  TaskInfo.Priority       = taskInfo[Ram_CBIT_Task].priority;
  TaskInfo.Type           = taskInfo[Ram_CBIT_Task].taskType;
  TaskInfo.modes          = taskInfo[Ram_CBIT_Task].modes;
  TaskInfo.MIFrames       = taskInfo[Ram_CBIT_Task].MIFframes;
  TaskInfo.Rmt.InitialMif = taskInfo[Ram_CBIT_Task].InitialMif;
  TaskInfo.Rmt.MifRate    = taskInfo[Ram_CBIT_Task].MIFrate;
  TaskInfo.Enabled        = TRUE;
  TaskInfo.Locked         = FALSE;
  TaskInfo.pParamBlock    = NULL;
  TmTaskCreate (&TaskInfo);
}


/******************************************************************************
 * Function:     CBITMgr_PWEHSEU_Initialize
 *
 * Description:  Initializes the PWEH SEU Processing Task 
 *
 * Parameters:   none
 *
 * Returns:      none
 *
 * Notes:        none 
 *
 *****************************************************************************/
void CBITMgr_PWEHSEU_Initialize (void)
{
  TCB TaskInfo;

  memset ( (void *) &m_PWEHSEUStatus, 0x00, sizeof(PWEH_SEU_STATUS)); 
  
  m_PWEHSEUStatus.state = PWEH_SEU_STARTUP; 
  
  // Creates the PWEHSEU Task
  memset(&TaskInfo, 0, sizeof(TaskInfo));
  strncpy_safe(TaskInfo.Name, sizeof(TaskInfo.Name),"PWEHSEU Manager",_TRUNCATE);
  TaskInfo.TaskID         = PWEHSEU_Manager;
  TaskInfo.Function       = CBITMgr_PWEHSEU_Task;
  TaskInfo.Priority       = taskInfo[PWEHSEU_Manager].priority;
  TaskInfo.Type           = taskInfo[PWEHSEU_Manager].taskType;
  TaskInfo.modes          = taskInfo[PWEHSEU_Manager].modes;
  TaskInfo.MIFrames       = taskInfo[PWEHSEU_Manager].MIFframes;
  TaskInfo.Rmt.InitialMif = taskInfo[PWEHSEU_Manager].InitialMif;
  TaskInfo.Rmt.MifRate    = taskInfo[PWEHSEU_Manager].MIFrate;
  TaskInfo.Enabled        = TRUE;
  TaskInfo.Locked         = FALSE;
  TaskInfo.pParamBlock    = NULL;
  TmTaskCreate (&TaskInfo);
  
  // Register PWEHSEU shutdown task and bus interrupt task 
  PmInsertAppBusInterrupt( CBITMgr_PWEHSEU_ShutDown ); 
  PmInsertAppShutDownNormal( CBITMgr_PWEHSEU_ShutDown ); 
  
  // Retrieve From EPROM 
  m_PWEHSEUStatus.EepromOpenResult = NV_Open( NV_PWEH_CNTS_SEU ); 
}


/******************************************************************************
* Function:     CBITMgr_PWEHSEU_FileInit
*
* Description:  Reset the file to the initial state
*
* Parameters:   [in]  none
*               [out] none
*
* Returns:      BOOLEAN TRUE  The file reset was successful.
*                       FALSE The file reset failed.
*
*****************************************************************************/
BOOLEAN CBITMgr_PWEHSEU_FileInit(void)
{
  
  // Reset count 
  memset ( (void *) &PwehSeuEepromData, 0x00, sizeof(PWEH_SEU_EEPROM_DATA) );
  
  // Set flag to indicate Lost Data.  Flag will be maintained until log is recorded.  
  PwehSeuEepromData.bEEPromDataCorrupted = TRUE;

  // Make attempt to update EEPROM 
  NV_Write( NV_PWEH_CNTS_SEU, 0, &PwehSeuEepromData, sizeof(PWEH_SEU_EEPROM_DATA));

  return TRUE;
}


/******************************************************************************
 * Function:     CBITMgr_UpdateFlightState 
 *
 * Description:  Updates state of the inflight bool
 *
 * Parameters:   inflight TRUE  - in flight
 *                        FALSE - on ground
 * Returns:      none
 *
 * Notes:        none
 * 
 *****************************************************************************/
void CBITMgr_UpdateFlightState(BOOLEAN inFlight)
{
  InFlight = inFlight;
}

/******************************************************************************
 * Function:     CBIT_CreateAllInternalLogs
 *
 * Description:  Creates all CBIT internal logs for testing / debug of log 
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
void CBIT_CreateAllInternalLogs ( void )
{
  // 1 Create SYS_CBIT_REG_CHECK_FAIL 
  LogWriteSystem( SYS_CBIT_REG_CHECK_FAIL, LOG_PRIORITY_LOW, 
                  (void *) &CbitRegCheckFailLog, sizeof(CBIT_REG_CHECK_FAILED), 
                  NULL ); 
                           
  // 2 Create SYS_CBIT_PWEH_SEU_COUNT_LOG
  LogWriteSystem( SYS_CBIT_PWEH_SEU_COUNT_LOG, LOG_PRIORITY_LOW, 
                  (void *) &CbitPwehSeuCountLog, sizeof(PWEH_SEU_HEALTH_COUNTS), 
                  NULL ); 
}
/*vcast_dont_instrument_end*/
#endif


/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/


/******************************************************************************
 * Function:     CBITMgr_Task
 *
 * Description:  CBIT Mgr Processing 
 *
 * Parameters:   void *pParam - not used
 *
 * Returns:      None
 *
 * Notes:        
 *  Performs CBIT that is not owned by any other module.   
 *  1) CBIT RAM Test 
 *  2) Program CRC check 
 *
 *****************************************************************************/
static void CBITMgr_Task( void *pParam )
{
  UINT8  i;
  UINT32 RegAddrFail; 
  UINT32 enumRegFail; 
  CBIT_REG_CHECK_FAILED RegCheckFailedLog; 
  
  // Get current number of Registers in Check list as this can change dynamically based on 
  //    different App Settings. 
  m_CbitMgrStatus.numReg = GetRegCheckNumReg(); 
  
  // Spread out RegCheck from 10 reg / _Task update time of 25 Hz (or 10 reg every 40 msec) 
  //    For 80 reg, this will be about 3.125 Hz, satisfying 2 Hz Req SRS-1817
  //    Currently it takes about 20 usec to reg check a single Reg with Branch Cache 
  //    enabled. 
  RegCheckFailedLog.nFailures = RegCheck( MAX_REG_ENTRIES, (UINT32*)&RegAddrFail, (UINT32*)&enumRegFail );
  
#ifdef WIN32
/*vcast_dont_instrument_start*/
  RegCheckFailedLog.nFailures = 0;
/*vcast_dont_instrument_end*/
#endif

  if ( ( RegCheckFailedLog.nFailures > 0 ) && (m_CbitMgrStatus.RegCheckCnt < MAX_REG_CHECK_LOGS) )
  {
    // Print Debug as indication 
    GSE_DebugStr( NORMAL, TRUE, "CBITMgr_Task: Reg Check Failure (cnt=%d)\r\n", 
                  RegCheckFailedLog.nFailures);
    
    RegCheckFailedLog.RegAddr = RegAddrFail; 
    RegCheckFailedLog.enumReg = enumRegFail; 
    
    // Create Failure Log
    Flt_SetStatus(STA_CAUTION, SYS_CBIT_REG_CHECK_FAIL, &RegCheckFailedLog, 
                  sizeof(CBIT_REG_CHECK_FAILED) );
                  
    // Update cumulative fail cnt
    m_CbitMgrStatus.RegCheckCnt += RegCheckFailedLog.nFailures; 
  }

  // check for UART Interrupt Monitor Failures
  for ( i = 0; i < UART_NUM_OF_UARTS; i++)
  {
    if ( UART_IntrMonitor(i) && !IntFailLogged[i])
    {
      // log error and set system condition if Interrupt Failure
      UART_INTR_FAIL_LOG log;
      IntFailLogged[i] = TRUE;

      log.port = i;
      Flt_SetStatus( STA_NORMAL, DRV_ID_PSC_INTERRUPT_FAIL,
                     &log, sizeof(UART_INTR_FAIL_LOG));
      GSE_DebugStr(NORMAL, TRUE, "Uart Interrupt Fail. Port %d Tx Interrupt Disabled\r\n", i); 
    }
  }
} 

/*****************************************************************************
 * Function:      CBITMgr_CRC_Task | TASK
 *
 * Description:   Continuous CRC verification of program flash
 *                
 *
 * Parameters:    pParam - not used.  Generic header for system tasks
 *              
 * Returns:       none
 *
 * Notes:  The CRC test shall verify the program flash every 10 minutes.
 *         The CRC test shall set the system status to FAULT if
 *         a fault is detected.
 *         The CRC failure log shall contain the calculated and expected
 *         CRC when the error is recorded.
 *              
 ****************************************************************************/
static void CBITMgr_CRC_Task(void* pParam)
{
  CBIT_CRC_CHECK_FAILED CrcCheckFailedLog; 
  UINT32 computedCrc;
  UINT32* pFlashCrc = (UINT32*)TPU( (UINT32)&__CRC_END, eTpCrc1806);
  UINT32* pFlashBeg = &__CRC_START;
  static BOOLEAN recorded = FALSE;

  UINT32 programSize = (UINT8*)pFlashCrc - (UINT8*)pFlashBeg;

/*vcast_dont_instrument_start*/
#if 0
  UINT32 startTime = TTMR_GetHSTickCount();
  UINT32 totalTime;
#endif
#ifdef WIN32
  programSize = 1024;
#endif
/*vcast_dont_instrument_end*/

  CRC32(pFlashBeg, programSize, &computedCrc, CRC_FUNC_SINGLE);

  if ( (*pFlashCrc != computedCrc) && (*pFlashCrc != 0xffffffff) )
  {
    GSE_DebugStr( NORMAL, TRUE, 
                 "CRC_CBit: Program CRC Mismatch: Computed=0x%08x, Actual=0x%08x\r\n", 
                 computedCrc, *pFlashCrc);

    ++m_CbitMgrStatus.ProgramCRCErrCnt;

    if (!recorded)
    {
      // Create Failure Log
      CrcCheckFailedLog.crcAddr = pFlashCrc;
      CrcCheckFailedLog.actualCrc = *pFlashCrc;
      CrcCheckFailedLog.computedCrc = computedCrc;

      Flt_SetStatus(STA_FAULT, SYS_CBIT_CRC_CHECK_FAIL, &CrcCheckFailedLog, 
                    sizeof(CBIT_CRC_CHECK_FAILED) );
      recorded = TRUE;
    }
  }
#if 0
/*vcast_dont_instrument_start*/
  totalTime = TTMR_GetHSTickCount() - startTime;
  GSE_DebugStr(NORMAL,FALSE, "CBIT CRC32 CalcTime: %d\r\n", totalTime);
/*vcast_dont_instrument_end*/
#endif
}

/*****************************************************************************
 * Function:      CBITMgr_Ram_Task | TASK
 *
 * Description:   Continuous Ram verification test
  *
 * Parameters:    pParam - not used.  Generic header for system tasks
 *              
 * Returns:       none
 *
 * Notes:  The Ram test shall pattern test the used RAM every 10 minutes.
 *         The Ram test shall set the system status to FAULT if
 *         a fault is detected.
 *         The Ram failure log shall contain the failed address and the 
 *         written and read back data.
 *              
 ****************************************************************************/
static void CBITMgr_Ram_Task(void* pParam)
{
  CBIT_RAM_CHECK_FAILED RamCheckFailedLog;

  // Following are 32 bits for walking 1 test. Otherwise, pattern test is 16 bits.
  static   UINT32  readPattern;     // static forces ram usage
  register UINT32  writePattern;    // write pattern must be in a register

  register BOOLEAN testFailed = FALSE;

#if 0
  /*vcast_dont_instrument_start*/
  UINT32 startTime = TTMR_GetHSTickCount();
  UINT32 totalTime;
  /*vcast_dont_instrument_end*/
#endif

  // walking 1 test can be done anywhere in ram,
  // so use the memory allocated by readPattern
  for (writePattern = 1; (writePattern != 0) && !testFailed;)
  {
    readPattern = writePattern;     // write pattern

    if (readPattern != TPU( writePattern, eTpRam1811))
    {
      testFailed = TRUE;
    }
    else
    {
      writePattern <<= 1;           // next pattern
    }
  }

  if (testFailed)
  {
    GSE_DebugStr( NORMAL, TRUE, 
                 "Ram_CBit: Walking 1 test failed: Written=0x%08x, Read=0x%08x\r\n", 
                 writePattern, readPattern);

    ++m_CbitMgrStatus.RAMFailCnt;

    // Create Failure Log
    RamCheckFailedLog.ramAddr = (UINT32*)&readPattern;
    RamCheckFailedLog.writePattern = writePattern;
    RamCheckFailedLog.readPattern = readPattern;

    Flt_SetStatus(STA_FAULT, SYS_CBIT_RAM_CHECK_FAIL, &RamCheckFailedLog, 
                  sizeof(CBIT_RAM_CHECK_FAILED) );
  }
  else
  {
    // walking 1 test passed - now run pattern tests
    UINT16* pStart;
    UINT16* pEnd;
    UINT32  i;
    for (i = 0; i < RAM_TEST_SECTIONS; ++i)
    {
      // We have to skip over the power ISR storage.
      // The power ISR is non-maskable and will cause the ram test to fail
      // and possibly impact power down processing should it happen to occur
      // when that section of memory is being tested.
      if (i == 0)
      {
        // start of mem to end of allocated
        pStart = (UINT16*)&__ghs_ramstart;
        pEnd   = (UINT16*)&__ghs_allocatedramend;
      }
      else
      {
        // test stack memory
        pStart = (UINT16*)&__SP_END;
        pEnd   = (UINT16*)&__SP_INIT;
      }

#if defined(WIN32) || defined(STE_TP)
      /*vcast_dont_instrument_start*/
      // if Windows, only use local array for test
      pStart = (UINT16*)&__ghs_ramstart;
      pEnd   = (UINT16*)&__ghs_ramstart[1024];
      /*vcast_dont_instrument_end*/
#endif

      readPattern = 0;
      while ( (pStart < pEnd) && !testFailed)
      {
         UINT32 intrLevel;
         register UINT16 dataSave;    // save original data - must be in register

         intrLevel = __DIR();         // disable interrupts

         dataSave = *pStart;          // save original location data
         writePattern = RamTestPattern;
         *pStart = (UINT16)writePattern;
         readPattern = *pStart;
         if ((UINT16)readPattern != (UINT16)TPU( writePattern, eTpRam3162a))
         {
           testFailed = TRUE;
         }
         else
         {
           // first test passed - test with inverse pattern
           writePattern = ~RamTestPattern;
           *pStart = (UINT16)writePattern;
           readPattern = *pStart;
           if ((UINT16)readPattern != (UINT16)TPU( writePattern,eTpRam3162b))
           {
             testFailed = TRUE;
           }
         }

         *pStart = dataSave;        // restore original location data
    
         __RIR(intrLevel);          // enable interrupts

         if (!testFailed)
         {
           // memory location OK - check next word
           ++pStart;
         }
         else
         {
           // memory test failed - report
           GSE_DebugStr( NORMAL, TRUE, 
              "Ram_CBit: Pattern test failed at addr 0x%08x  Written=0x%04x, Read=0x%04x\r\n", 
                         (UINT32)pStart, (UINT16)writePattern, (UINT16)readPattern);

           ++m_CbitMgrStatus.RAMFailCnt;

           // Create Failure Log
           RamCheckFailedLog.ramAddr = (UINT32*)pStart;
           RamCheckFailedLog.writePattern = writePattern;
           RamCheckFailedLog.readPattern = readPattern;

           Flt_SetStatus(STA_FAULT, SYS_CBIT_RAM_CHECK_FAIL, &RamCheckFailedLog, 
                          sizeof(CBIT_RAM_CHECK_FAILED) );
         }
      }   // while ( (pStart < pEnd) && !testFailed)
    }   // for (i = 0; i < RAM_TEST_SECTIONS; ++i)
  }

#if 0
  /*vcast_dont_instrument_start*/
  totalTime = TTMR_GetHSTickCount() - startTime;
  GSE_DebugStr(NORMAL,FALSE, "CBIT Ram Test CalcTime: %d\r\n", totalTime);
  /*vcast_dont_instrument_end*/
#endif
}

/******************************************************************************
 * Function:     CBITMgr_PWEHSEU_Task
 *
 * Description:  PWEH SEU Processing during data capture 
 *
 * Parameters:   void *pParam - not used
 *
 * Returns:      None
 *
 * Notes:        
 *
 * Logic 
 * On startup, if Recording was in progress, wait 10 sec. 
 *    If recording did not startup yet, then terminate current count 
 *    and record (to close out pending counting).  Otherwise continue 
 *    with current count !
 * On startup, If interrupted flag was not set, but record in progress set, 
 *    then log this condition.  Will continue counting 
 *    from this point.  (Reset counts would be valid, but others most likely will
 *    be zero). 
 * Record start time.  Record End Time.  Interrupted Flag.   
 * We only record on power down to EEPROM, with flag to indicate 
 *    powering down ? Another flag Interrupted ! 
 * 
 * Close log when Data Recording stops.  
 *
 *****************************************************************************/
static void CBITMgr_PWEHSEU_Task( void *pParam )
{

  switch ( m_PWEHSEUStatus.state ) 
  {
    case PWEH_SEU_STARTUP:
      // Retrieve from EEPROM 
      // Is previous recording in progress ? 
      // If Yes, startup counter to wait 10 seconds to to _INTERRUPT_RECORDING
      // If No, go to _WAITING_FOR_RECORDING  
      m_PWEHSEUStatus.state = CBITMgr_PWEHSEU_Startup(); 
      break; 
      
    case PWEH_SEU_RESUME_RECORDING:
      // Wait 10 sec, if recording does not cont then close Health Count Logs
      //   transition to _WAITING_FOR_RECORDING 
      m_PWEHSEUStatus.state = CBITMgr_PWEHSEU_ResumeRec(); 
      break; 
      
    case PWEH_SEU_WAITING_FOR_RECORDING:
      // Pass in Recording flag and if recording on ->
      // Clear counts and then Update EEPROM data
      //   transition to _RECORDING 
      m_PWEHSEUStatus.state = CBITMgr_PWEHSEU_WaitForRec(); 
      break; 
      
    case PWEH_SEU_RECORDING:
      // If Recording completed, then determine when recording is completed. 
      m_PWEHSEUStatus.state = CBITMgr_PWEHSEU_Rec(); 
      break; 
    
    default:
        FATAL("Unrecognized PWEH_SEU_STATE: %d", m_PWEHSEUStatus.state); 
      break; 
  }


  // Resets
  
  // Watchdog Resets
  
  // Exceptions Resets (Bus Fault, etc)
  
  // Internal Resets ( ASSERTS, FATAL )
  
  // Task Over Runs

} 


/******************************************************************************
 * Function:     CBITMgr_PWEHSEU_ShutDown
 *
 * Description:  Store current Pweh Seu Data due to power interruption if 
 *               recording in progress.  
 *
 * Parameters:   none
 *
 * Returns:      TRUE if update of Pweh Seu Count Data OK.  
 *
 * Notes:        
 * 1) .bIntSaveCntCompleted is cleared after 1/2 sec in the PWEH Rec Process. 
 *    If CBITMgr_PWEHSEU_ShutDown() is requested again before this time expiration, 
 *    the counts, if updated will not be saved.  (Prevents re-entrant call from 
 *    PM -> Glitch and Shutdown)
 *
 *****************************************************************************/
static BOOLEAN CBITMgr_PWEHSEU_ShutDown( void ) 
{
  // If recording, we should attempt to save current SEU count values. 
  //  Also check if bIntSaveCntCompleted == FALSE.  This flag will be reset
  //  1 sec after a Pm Glitch is detected.  If this 1 sec has not expired 
  //  do not attempt to update EEPROM data.  If power shuts down, we could 
  //  miss count values over this 1 sec, but should not be significant. 
  if ( (m_PWEHSEUStatus.state == PWEH_SEU_RECORDING) && 
       (PwehSeuEepromData.bIntSaveCntCompleted == FALSE) )
  {
    PwehSeuEepromData.bIntSaveCntCompleted = TRUE; 

    CBITMgr_PWEHSEU_UpdateSeuEepromData(); 
    
    // Update EEPROM with current run time data 
    NV_Write( NV_PWEH_CNTS_SEU, 0, &PwehSeuEepromData, sizeof(PWEH_SEU_EEPROM_DATA));
  }  
  return TRUE;
}


/******************************************************************************
 * Function:     CBITMgr_PWEHSEU_Startup
 *
 * Description:  Initializes the PWEH SEU count processing 
 *
 * Parameters:   none
 *
 * Returns:      PWEH_SEU_STATE
 *
 * Notes:        none
 * 
 *****************************************************************************/
static PWEH_SEU_STATE CBITMgr_PWEHSEU_Startup( void ) 
{
  PWEH_SEU_STATE nextState; 
    
  // Initialize Data Store 
  memset ( (void *) &PwehSeuEepromData, 0x00, sizeof(PWEH_SEU_EEPROM_DATA) ); 
  memset ( (void *) &PwehSeuCountsAtStartOfRec, 0x00, sizeof(PWEH_SEU_HEALTH_COUNTS) ); 
  memset ( (void *) &PwehSeuCountsPrevStored, 0x00, sizeof(PWEH_SEU_HEALTH_COUNTS) ); 


  // Read from EE version.
  NV_Read(NV_PWEH_CNTS_SEU, 0, &PwehSeuEepromData, sizeof(PWEH_SEU_EEPROM_DATA));
  
  // Open of FILE failed both backup and primary  
  if ( m_PWEHSEUStatus.EepromOpenResult != SYS_OK )
  {
    CBITMgr_PWEHSEU_FileInit();
    
    // Reset counts
    //memset ( (void *) &PwehSeuEepromData, 0x00, sizeof(PWEH_SEU_EEPROM_DATA) ); 
    memset ( (void *) &ResetHealthCounts, 0x00, sizeof(ResetHealthCounts)); 

    // Send debug msg. 
    GSE_DebugStr(NORMAL,TRUE,"PWEHSEU Task: Data Restore Failed");
    
    // Make attempt to update EEPROM 
    NV_Write( NV_PWEH_CNTS_SEU, 0, &PwehSeuEepromData, sizeof(PWEH_SEU_EEPROM_DATA));
  }
  else
  {
    // open OK - Get reset counts
    ResetHealthCounts = PwehSeuEepromData.Counts.reset; 
  }
  
  // Is previous recording in progress  ?
  if ( PwehSeuEepromData.bRecInProgress == TRUE ) 
  {
    PwehSeuEepromData.bIntRecInProgress = TRUE; 
    
    // check type of restart and inc appropriate health count
    if (isWatchdogRestart)
    {
      ResetHealthCounts.nWatchdogResets++;
    }
    else
    {
      ResetHealthCounts.nReset++;
    }
    // copy back to eeprom since this won't change til next interruption
    PwehSeuEepromData.Counts.reset = ResetHealthCounts;
    
    if ( PwehSeuEepromData.bIntSaveCntCompleted == FALSE ) 
    {
      // Counts not recorded on last shutdown ! 
      PwehSeuEepromData.Counts.SEUCounts_Interrupted = TRUE; 

      // Send debug msg. 
      GSE_DebugStr(NORMAL,TRUE,
        "PWEHSEU Task: Previous Rec In Progress Detected - SEU Data Not Saved");
    }
    else 
    {
      PwehSeuEepromData.bIntSaveCntCompleted = FALSE;   // Reset this flag
      
      // Send debug msg. 
      GSE_DebugStr(NORMAL,TRUE,
        "PWEHSEU Task: Previous Rec In Progress Detected - SEU Data Saved");
    }
    
    // Make attempt to update EEPROM
    NV_Write( NV_PWEH_CNTS_SEU, 0, &PwehSeuEepromData, sizeof(PWEH_SEU_EEPROM_DATA));
    
    nextState = PWEH_SEU_RESUME_RECORDING; 
    m_PWEHSEUStatus.timer =
      CM_GetTickCount() + CM_DELAY_IN_SECS(PWEH_INTERRUPTED_STARTUP_DELAY_S); 
  }  
  else 
  {
    nextState = PWEH_SEU_WAITING_FOR_RECORDING; 
  }
  
  PwehSeuCountsPrevStored = PwehSeuEepromData.Counts; 
  
  // Clear flag to indicate EEPROM data has been restored 
  m_PWEHSEUStatus.EepromOpenResult = SYS_OK;

  return ( nextState ); 
  
}


/******************************************************************************
 * Function:     CBITMgr_AddPrevCBITHealthStatus
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
static
 CBIT_HEALTH_COUNTS CBITMgr_AddPrevCBITHealthStatus ( CBIT_HEALTH_COUNTS CurrCnt,
                                                     CBIT_HEALTH_COUNTS PrevCnt )
{
  CBIT_HEALTH_COUNTS AddCount;
  
  AddCount.RAMFailCnt = CurrCnt.RAMFailCnt + PrevCnt.RAMFailCnt; 
  AddCount.ProgramCRCErrCnt = CurrCnt.ProgramCRCErrCnt + PrevCnt.ProgramCRCErrCnt; 
  AddCount.RegCheckCnt = CurrCnt.RegCheckCnt + PrevCnt.RegCheckCnt; 

  return ( AddCount ); 
}

/******************************************************************************
 * Function:     CBITMgr_CalcDiffCBITHealthStatus
 *
 * Description:  Calc the difference in CBIT Health Counts to support PWEH SEU
 *               (Used to support determining SEU cnt during data capture) 
 *
 * Parameters:   PrevCnt - Initial count value
 *
 * Returns:      Diff in CBIT_HEALTH_COUNTS from (Current - PrevCnt)
 *
 * Notes:        None
 *
 *****************************************************************************/
static CBIT_HEALTH_COUNTS CBITMgr_CalcDiffCBITHealthStatus ( CBIT_HEALTH_COUNTS PrevCnt )
{
  CBIT_HEALTH_COUNTS DiffCount;
  CBIT_HEALTH_COUNTS *pCurrent; 
  
  pCurrent = &CBITHealthCounts; 
  
  DiffCount.RAMFailCnt = pCurrent->RAMFailCnt - PrevCnt.RAMFailCnt; 
  DiffCount.ProgramCRCErrCnt = pCurrent->ProgramCRCErrCnt - PrevCnt.ProgramCRCErrCnt; 
  DiffCount.RegCheckCnt = pCurrent->RegCheckCnt - PrevCnt.RegCheckCnt; 

  return ( DiffCount ); 
}


/******************************************************************************
 * Function:     CBITMgr_GetCBITHealthStatus
 *
 * Description:  Returns the current CBIT Health status of the CBIT Mgr 
 *
 * Parameters:   None
 *
 * Returns:      Current Data in CBIT_HEALTH_COUNTS
 *
 * Notes:        None
 *
 *****************************************************************************/
static CBIT_HEALTH_COUNTS CBITMgr_GetCBITHealthStatus ( void )
{
  return ( CBITHealthCounts ); 
}


/******************************************************************************
 * Function:     CBITMgr_PWEHSEU_ResumeRec
 *
 * Description:  Data recording was interrupted by reset/shutdown, 
 *               wait 10 sec for start of data recording.  If time out, 
 *               and no data recording seen, mark as interrupted SEU 
 *               count, record and then transition to wait for new recording. 
 *
 * Parameters:   none
 *
 * Returns:      PWEH_SEU_STATE
 *
 * Notes:        none
 * 
 *****************************************************************************/
static 
PWEH_SEU_STATE CBITMgr_PWEHSEU_ResumeRec ( void ) 
{
  PWEH_SEU_STATE nextState; 
  
  
  nextState = m_PWEHSEUStatus.state;

  if ( InFlight ) 
  {
    // Transition to PWEH_SEU_RECORDING  
    nextState = PWEH_SEU_RECORDING; 
  }
  else 
  {
    if (CM_GetTickCount() > m_PWEHSEUStatus.timer)
    {
      // No Recording encountered on startup, save current EEPROM data to 
      //  a log, even though on shut down rec was in progress, we must 
      //  not have seen the record ending. 
      
      // Note: PWEH_SEU_EEPROM_DATA -> Counts should contain count value  
      //  as of last recording before reset / shutdown.  
      
      // Note: If this log is recorded here, there is a high possibility that 
      //  we did not properly see end of recording, and thus these count 
      //  numbers are not 100% accurate.  ->SEUCounts_Interrupted will 
      //  be set to TRUE as an indication of this condition, if not 
      //  already set in CBITMgr_PWEHSEU_Startup()
      PwehSeuEepromData.Counts.SEUCounts_Interrupted = TRUE; 
      
      // Log PWEH SEU here ! 
      LogWriteSystem( SYS_CBIT_PWEH_SEU_COUNT_LOG, LOG_PRIORITY_LOW,
                      &PwehSeuEepromData.Counts, sizeof(PWEH_SEU_HEALTH_COUNTS), 
                      NULL ); 
      
      PwehSeuEepromData.bRecInProgress = FALSE;       // Reset RecInProgress flag
      PwehSeuEepromData.bIntSaveCntCompleted = FALSE; // Reset IntSaveCntCompleted flag
      PwehSeuEepromData.bEEPromDataCorrupted = FALSE; // Reset EEPromDataCorrupted flag
      PwehSeuEepromData.bIntRecInProgress = FALSE;    // Reset IntRecInProgress flag 
      
      // Clear all my count values.  Values should have been clr, but 
      //    will double clear again... 
      // Clear EEPROM data and save to EEPROM
      memset ( &PwehSeuEepromData.Counts, 0x00, sizeof(PWEH_SEU_HEALTH_COUNTS) );
      NV_Write( NV_PWEH_CNTS_SEU, 0, &PwehSeuEepromData, sizeof(PWEH_SEU_EEPROM_DATA));
      
      nextState = PWEH_SEU_WAITING_FOR_RECORDING; 
      
      // Send debug msg. 
      GSE_DebugStr(NORMAL,TRUE,
        "PWEHSEU Task: Startup Data recording not detected, closing pending SEU Cnt log");
      
    } 
    // else we do nothing and continue counting down ! 
  } 
  
  return nextState; 
}


/******************************************************************************
 * Function:     CBITMgr_PWEHSEU_WaitForRec
 *
 * Description:  Wait for Data Recording trigger and start / reset SEU counts 
 *
 * Parameters:   none
 *
 * Returns:      PWEH_SEU_STATE
 *
 * Notes:        none
 * 
 *****************************************************************************/
static 
PWEH_SEU_STATE CBITMgr_PWEHSEU_WaitForRec ( void )
{
  PWEH_SEU_STATE nextState; 
  
  
  nextState = m_PWEHSEUStatus.state;

  if ( InFlight )
  {
    // Clear Counts.  This process gets current snapshot of all counts 
    //   and will subtract it from the end count to determine 
    // Update current count values PwehSeuCountsAtStartOfRec
    PwehSeuCountsAtStartOfRec = CBITMgr_PWEHSEU_GetAllCurrentCountValues (); 
    
    // Update EEPROM data 
    PwehSeuEepromData.bRecInProgress = TRUE;
    PwehSeuEepromData.bIntSaveCntCompleted = FALSE; 
    PwehSeuEepromData.bEEPromDataCorrupted = FALSE; 
    PwehSeuEepromData.bIntRecInProgress = FALSE; 
    
    // Clear all my count values.  Values should have been clr, but 
    //    will double clear again... 
    memset ( &PwehSeuEepromData.Counts, 0x00, sizeof(PWEH_SEU_HEALTH_COUNTS) );
    
    // Update EEPROM 
    NV_Write( NV_PWEH_CNTS_SEU, 0, &PwehSeuEepromData, sizeof(PWEH_SEU_EEPROM_DATA));
    
    // Set next state as Recording 
    nextState = PWEH_SEU_RECORDING; 
    
  }
  // else we do nothing and continue waiting 
  
  return ( nextState ); 
  
}


/******************************************************************************
 * Function:     CBITMgr_PWEHSEU_Rec
 *
 * Description:  When data recording transitions to OFF, calc SEU counts 
 *               accumulated during data recording and store. 
 *
 * Parameters:   none
 *
 * Returns:      PWEH_SEU_STATE
 *
 * Notes:        none
 * 
 *****************************************************************************/
static 
PWEH_SEU_STATE CBITMgr_PWEHSEU_Rec ( void ) 
{
  PWEH_SEU_STATE nextState;
  // PWEH_SEU_HEALTH_COUNTS CurrentCount; 


  nextState = m_PWEHSEUStatus.state;

  // Have we transitioned to stop recording 
  // if (bSysRecording == FALSE) 
  if ( !InFlight )
  {
    CBITMgr_PWEHSEU_UpdateSeuEepromData();      
     
    // Log PWEH Count Values 
    LogWriteSystem( SYS_CBIT_PWEH_SEU_COUNT_LOG, LOG_PRIORITY_LOW,
                    &PwehSeuEepromData.Counts, sizeof(PWEH_SEU_HEALTH_COUNTS), 
                    NULL ); 

    // Update EEprom data 
    memset ( (void *) &PwehSeuEepromData, 0x00, sizeof(PWEH_SEU_EEPROM_DATA) ); 
    NV_Write( NV_PWEH_CNTS_SEU, 0, &PwehSeuEepromData, sizeof(PWEH_SEU_EEPROM_DATA));
    
    // Send debug msg. 
    GSE_DebugStr(NORMAL,TRUE,"PWEHSEU Task: End flight detected.  Closing SEU count log");
                       
    nextState = PWEH_SEU_WAITING_FOR_RECORDING; 
  }
  else 
  {
    // Did we save current SEU counts due to Power Glitch or Battery Latch ?
    if ( PwehSeuEepromData.bIntSaveCntCompleted == TRUE ) 
    {
      // Count down 500 msec before clearing the bIntSaveCntCompleted 
      //   If power shutdown before this timer expires then worst case we would have missed 
      //   < 0.5 sec of potential SEU count data.  The count values will be 99.9% up to date.
      if ( m_PWEHSEUStatus.pmGlitchDetected == FALSE ) 
      {
        m_PWEHSEUStatus.timer = CM_GetTickCount() + CM_DELAY_IN_MSEC(500); 
        m_PWEHSEUStatus.pmGlitchDetected = TRUE; 
      }
      else 
      {
        if ( m_PWEHSEUStatus.timer < CM_GetTickCount() ) 
        {
          // clear bIntSaveCntCompleted 
          m_PWEHSEUStatus.pmGlitchDetected = FALSE; 
          
          PwehSeuEepromData.bIntSaveCntCompleted = FALSE; 
          
          // Update EEprom data 
          NV_Write( NV_PWEH_CNTS_SEU, 0, &PwehSeuEepromData, sizeof(PWEH_SEU_EEPROM_DATA));
        }
      }
    }
  }

  return (nextState);   
}


/******************************************************************************
 * Function:     CBITMgr_PWEHSEU_GetAllCurrentCountValues
 *
 * Description:  Get current SEU count value from all modules 
 *
 * Parameters:   none
 *
 * Returns:      PWEH_SEU_HEALTH_COUNTS
 *
 * Notes:        none
 * 
 *****************************************************************************/
static 
PWEH_SEU_HEALTH_COUNTS CBITMgr_PWEHSEU_GetAllCurrentCountValues ( void )
{
  PWEH_SEU_HEALTH_COUNTS CountCurrent;

  CountCurrent.SEUCounts_Interrupted = FALSE;       // default value

  // CBIT_HEALTH_COUNTS
  CountCurrent.cbit = CBITMgr_GetCBITHealthStatus(); 

  // FPGA_CBIT_HEALTH_COUNTS
  CountCurrent.fpga = FPGAMgr_GetCBITHealthStatus(); 
  
  // QAR_CBIT_HEALTH_COUNTS
  CountCurrent.qar = QAR_GetCBITHealthStatus(); 
  
  // ARINC429_CBIT_HEALTH_COUNTS
  CountCurrent.arinc429 = Arinc429MgrGetCBITHealthStatus(); 
  
  // Task Manager Health Counts
  CountCurrent.tm = Tm_GetTMHealthStatus(); 
  
  // Reset Health Counts
  CountCurrent.reset = ResetHealthCounts;
  
  // Log Data Flash Health Counts
  CountCurrent.log = LogGetCBITHealthStatus();

  return ( CountCurrent );
  
}


/******************************************************************************
 * Function:     CBITMgr_PWEHSEU_GetDiffCountValues
 *
 * Description:  Get the difference from current SEU count value from a previous 
 *               count point 
 *
 * Parameters:   PWEH_SEU_HEALTH_COUNTS PrevCount
 *
 * Returns:      PWEH_SEU_HEALTH_COUNTS
 *
 * Notes:        none
 * 
 *****************************************************************************/
static 
PWEH_SEU_HEALTH_COUNTS CBITMgr_PWEHSEU_GetDiffCountValues ( PWEH_SEU_HEALTH_COUNTS PrevCount )
{
  PWEH_SEU_HEALTH_COUNTS DiffCount; 

  DiffCount.SEUCounts_Interrupted = PrevCount.SEUCounts_Interrupted; 

  // CBIT_HEALTH_COUNTS
  DiffCount.cbit = CBITMgr_CalcDiffCBITHealthStatus( PrevCount.cbit ); 

  // FPGA_CBIT_HEALTH_COUNTS
  DiffCount.fpga = FPGAMgr_CalcDiffCBITHealthStatus( PrevCount.fpga ); 
  
  // QAR_CBIT_HEALTH_COUNTS
  DiffCount.qar = QAR_CalcDiffCBITHealthStatus( PrevCount.qar ); 
  
  // ARINC429_CBIT_HEALTH_COUNTS
  DiffCount.arinc429 = Arinc429MgrCalcDiffCBITHealthStatus( PrevCount.arinc429 ); 
  
  // Task Manager Health Counts
  DiffCount.tm = Tm_CalcDiffTMHealthStatus ( PrevCount.tm ); 
  
  // Reset Health Counts 
  // only updated on restart
  DiffCount.reset.nWatchdogResets = 0;
  DiffCount.reset.nReset = 0;
  
  // Log Data Flash Health Counts
  DiffCount.log = LogCalcDiffCBITHealthStatus( PrevCount.log );

  return ( DiffCount );
  
}


/******************************************************************************
 * Function:     CBITMgr_PWEHSEU_AddPrevCountValues
 *
 * Description:  Add two different count points
  *
 * Parameters:   PWEH_SEU_HEALTH_COUNTS CurrCnt,
 *               PWEH_SEU_HEALTH_COUNTS PrevCnt
 *
 * Returns:      PWEH_SEU_HEALTH_COUNTS
 *
 * Notes:        none
 * 
 *****************************************************************************/
static 
PWEH_SEU_HEALTH_COUNTS CBITMgr_PWEHSEU_AddPrevCountValues ( PWEH_SEU_HEALTH_COUNTS CurrCnt, 
                                                            PWEH_SEU_HEALTH_COUNTS PrevCnt )
{
  PWEH_SEU_HEALTH_COUNTS AddCount; 

  AddCount.SEUCounts_Interrupted = CurrCnt.SEUCounts_Interrupted; 

  // CBIT_HEALTH_COUNTS
  AddCount.cbit = CBITMgr_AddPrevCBITHealthStatus( CurrCnt.cbit, PrevCnt.cbit ); 

  // FPGA_CBIT_HEALTH_COUNTS
  AddCount.fpga = FPGAMgr_AddPrevCBITHealthStatus( CurrCnt.fpga, PrevCnt.fpga ); 
  
  // QAR_CBIT_HEALTH_COUNTS
  AddCount.qar = QAR_AddPrevCBITHealthStatus( CurrCnt.qar, PrevCnt.qar ); 
  
  // ARINC429_CBIT_HEALTH_COUNTS
  AddCount.arinc429 = Arinc429MgrAddPrevCBITHealthStatus( CurrCnt.arinc429, PrevCnt.arinc429 ); 
  
  // Task Manager Health Counts
  AddCount.tm = Tm_AddPrevTMHealthStatus( CurrCnt.tm, PrevCnt.tm ); 
  
  // Reset Health Counts
  AddCount.reset.nWatchdogResets =
    CurrCnt.reset.nWatchdogResets + PrevCnt.reset.nWatchdogResets;
  AddCount.reset.nReset = CurrCnt.reset.nReset + PrevCnt.reset.nReset;

  // Log Data Flash Health Counts
  AddCount.log = LogAddPrevCBITHealthStatus( CurrCnt.log, PrevCnt.log );

  return ( AddCount );
  
}


/******************************************************************************
 * Function:     CBITMgr_PWEHSEU_UpdateSeuEepromData 
 *
 * Description:  Updates PwehSeuEepromData with current Run Time Count 
  *
 * Parameters:   none
 *
 * Returns:      none
 *
 * Notes:        none
 * 
 *****************************************************************************/
static 
void CBITMgr_PWEHSEU_UpdateSeuEepromData ( void )
{
  PWEH_SEU_HEALTH_COUNTS CurrentCount; 

  // Get current Health Counts (PwehSeuCounts) and subtract from PwehSeuCountsPrevious
  CurrentCount = CBITMgr_PWEHSEU_GetDiffCountValues(PwehSeuCountsAtStartOfRec); 
  
  // If we were reset/shutdown, add previous (power on) count values to current running total
  if (PwehSeuEepromData.bIntRecInProgress) 
  {
    PwehSeuEepromData.Counts = CBITMgr_PWEHSEU_AddPrevCountValues( CurrentCount, 
                                                                   PwehSeuCountsPrevStored );
  }
  else 
  {
    PwehSeuEepromData.Counts = CurrentCount; 
  }
}



/*****************************************************************************
 *  MODIFICATIONS
 *    $History: CBITManager.c $
 * 
 * *****************  Version 54  *****************
 * User: Contractor2  Date: 4/11/11    Time: 5:24p
 * Updated in $/software/control processor/code/system
 * SCR 1012: Unitialized variable "SEUCounts_Interrupted"
 * 
 * *****************  Version 53  *****************
 * User: John Omalley Date: 10/12/10   Time: 2:45p
 * Updated in $/software/control processor/code/system
 * SCR 826 - Code Review - Spelling problem
 * 
 * *****************  Version 52  *****************
 * User: John Omalley Date: 10/12/10   Time: 2:42p
 * Updated in $/software/control processor/code/system
 * SCR 826 - Code Review Updates
 * 
 * *****************  Version 51  *****************
 * User: Contractor2  Date: 10/07/10   Time: 5:11p
 * Updated in $/software/control processor/code/system
 * SCR #921 Watchdog reset counts not showing in
 * SYS_CBIT_PWEH_SEU_COUNT_LOG
 * 
 * *****************  Version 50  *****************
 * User: Jeff Vahue   Date: 9/07/10    Time: 11:57a
 * Updated in $/software/control processor/code/system
 * SCR# 853 - Item 6 Set the sysCond to NORMAL on fault recording
 * 
 * *****************  Version 49  *****************
 * User: Jeff Vahue   Date: 9/03/10    Time: 8:22p
 * Updated in $/software/control processor/code/system
 * SCR# 853 - UART Int Monitor
 * 
 * *****************  Version 48  *****************
 * User: Jeff Vahue   Date: 9/01/10    Time: 2:39p
 * Updated in $/software/control processor/code/system
 * SCR# 830 - Code Coverage Changes
 * 
 * *****************  Version 47  *****************
 * User: Jeff Vahue   Date: 8/17/10    Time: 9:43p
 * Updated in $/software/control processor/code/system
 * SCR# 707 - Disable CBIT RAM Test when doing coverage runs
 * 
 * *****************  Version 46  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:10a
 * Updated in $/software/control processor/code/system
 * SCR #698 - Fix code review findings
 * 
 * *****************  Version 45  *****************
 * User: Jeff Vahue   Date: 7/23/10    Time: 8:12p
 * Updated in $/software/control processor/code/system
 * SCR# 707, 733 - More TP INstrumentation, and enable CBIT RAM test
 * 
 * *****************  Version 44  *****************
 * User: Contractor V&v Date: 7/21/10    Time: 7:16p
 * Updated in $/software/control processor/code/system
 * SCR #538 SPIManager startup & NV Mgr Issues
 * 
 * *****************  Version 43  *****************
 * User: Jeff Vahue   Date: 7/17/10    Time: 5:46p
 * Updated in $/software/control processor/code/system
 * SCR# 707 - Add TestPoints for code coverage.
 * 
 * *****************  Version 42  *****************
 * User: Contractor2  Date: 7/13/10    Time: 11:19a
 * Updated in $/software/control processor/code/system
 * SCR #563 Modfix: CBIT Ram Test.
 * 
 * *****************  Version 41  *****************
 * User: Peter Lee    Date: 7/08/10    Time: 3:05p
 * Updated in $/software/control processor/code/system
 * SCR #683 Fix constant recording of SYS_CBIT_REG_CHECK_FAIL log. 
 * 
 * *****************  Version 40  *****************
 * User: Contractor2  Date: 6/30/10    Time: 11:04a
 * Updated in $/software/control processor/code/system
 * SCR #150 Implementation: SRS-3662 - Add Data Flash corruption CBIT
 * counts.
 * 
 * *****************  Version 39  *****************
 * User: Peter Lee    Date: 6/17/10    Time: 3:22p
 * Updated in $/software/control processor/code/system
 * SCR #634 Distribute reg check loading. 
 * 
 * *****************  Version 38  *****************
 * User: Contractor2  Date: 6/14/10    Time: 1:05p
 * Updated in $/software/control processor/code/system
 * SCR #483 Function Names must begin with the CSC it belongs with.
 * 
 * *****************  Version 37  *****************
 * User: Contractor3  Date: 6/10/10    Time: 10:44a
 * Updated in $/software/control processor/code/system
 * SCR #642 - Changes based on Code Review
 * 
 * *****************  Version 36  *****************
 * User: Contractor V&v Date: 6/08/10    Time: 5:53p
 * Updated in $/software/control processor/code/system
 * SCR #614 fast.reset=really should initialize files
 * 
 * *****************  Version 35  *****************
 * User: John Omalley Date: 6/08/10    Time: 12:13p
 * Updated in $/software/control processor/code/system
 * SCR 627 - Updated for LJ60
 * 
 * *****************  Version 34  *****************
 * User: Contractor2  Date: 6/07/10    Time: 2:16p
 * Updated in $/software/control processor/code/system
 * SCR #628 Count SEU resets during data capture
 * 
 * *****************  Version 33  *****************
 * User: Contractor2  Date: 5/25/10    Time: 3:00p
 * Updated in $/software/control processor/code/system
 * SCR #612 Initial CRC value set for release build
 * SCR #563 Implementation: Req SRS-1811,1813,1814,1815,3162 - CBIT RAM
 * Tests
 * 
 * *****************  Version 32  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:55p
 * Updated in $/software/control processor/code/system
 * SCR #587 Change TmTaskCreate to return void
 * 
 * *****************  Version 31  *****************
 * User: Contractor2  Date: 5/06/10    Time: 2:10p
 * Updated in $/software/control processor/code/system
 * SCR #579 Change LogWriteSys to return void
 * 
 * *****************  Version 30  *****************
 * User: Contractor V&v Date: 4/30/10    Time: 11:28a
 * Updated in $/software/control processor/code/system
 * SCR #574 System Level objects should not be including Applicati
 * 
 * *****************  Version 29  *****************
 * User: Jeff Vahue   Date: 4/28/10    Time: 5:48p
 * Updated in $/software/control processor/code/system
 * SCR # 572 - vcast changes
 * 
 * *****************  Version 28  *****************
 * User: Contractor2  Date: 4/23/10    Time: 2:08p
 * Updated in $/software/control processor/code/system
 * Implementation: Req SRS-1811,1813,1814,1815,3162 - CBIT RAM Tests
 * 
 * *****************  Version 27  *****************
 * User: Jeff Vahue   Date: 4/12/10    Time: 5:43p
 * Updated in $/software/control processor/code/system
 * SCR #541 - all XXX_CreateLogs functions are now dependent on
 * GENERATE_SYS_LOGS
 * 
 * *****************  Version 26  *****************
 * User: Jeff Vahue   Date: 4/07/10    Time: 8:23p
 * Updated in $/software/control processor/code/system
 * SCR #534 - no message when treating default CRC as valid
 * 
 * *****************  Version 25  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:09p
 * Updated in $/software/control processor/code/system
 * SCR #317 Implement safe strncpy
 * 
 * *****************  Version 24  *****************
 * User: Jeff Vahue   Date: 4/07/10    Time: 1:39p
 * Updated in $/software/control processor/code/system
 * SCR #534 - Record the CBIT Program CRC only once/powerup
 * 
 * *****************  Version 23  *****************
 * User: Jeff Vahue   Date: 4/07/10    Time: 11:45a
 * Updated in $/software/control processor/code/system
 * SCR #534 - set the default program CRC to allow for development and
 * reformat so messages related tot he CRC.
 * 
 * *****************  Version 22  *****************
 * User: Contractor2  Date: 4/06/10    Time: 3:56p
 * Updated in $/software/control processor/code/system
 * SCR 525 - Implement periodic CBIT CRC test.
 * 
 * *****************  Version 21  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:36p
 * Updated in $/software/control processor/code/system
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 * 
 * *****************  Version 20  *****************
 * User: Contractor V&v Date: 3/19/10    Time: 4:30p
 * Updated in $/software/control processor/code/system
 * SCR #279 Making NV filenames uppercase
 * 
 * *****************  Version 19  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/system
 * SCR# 483 - Function Names
 * 
 * *****************  Version 18  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:57p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 17  *****************
 * User: Contractor V&v Date: 2/03/10    Time: 2:57p
 * Updated in $/software/control processor/code/system
 * SCR 279
 * 
 * *****************  Version 16  *****************
 * User: Peter Lee    Date: 1/25/10    Time: 2:39p
 * Updated in $/software/control processor/code/system
 * SCR #414 Error - Primary and Backup PWEH SEU Cnt Restore Failure 
 * 
 * *****************  Version 15  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 5:04p
 * Updated in $/software/control processor/code/system
 * SCR# 397
 * 
 * *****************  Version 14  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:24p
 * Updated in $/software/control processor/code/system
 * SCR 371
 * 
 * *****************  Version 13  *****************
 * User: Peter Lee    Date: 12/22/09   Time: 2:39p
 * Updated in $/software/control processor/code/system
 * SCR #237 Replace LogWriteSystem() with Flt_SetStatus() 
 * 
 * *****************  Version 12  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 2:11p
 * Updated in $/software/control processor/code/system
 * SCR# 326
 * 
 * *****************  Version 11  *****************
 * User: Peter Lee    Date: 12/22/09   Time: 11:35a
 * Updated in $/software/control processor/code/system
 * SCR #381
 * 
 * *****************  Version 10  *****************
 * User: Peter Lee    Date: 12/18/09   Time: 4:55p
 * Updated in $/software/control processor/code/system
 * Distribute reg check across several MIF. 
 * 
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 12/18/09   Time: 4:49p
 * Updated in $/software/control processor/code/system
 * 
 * *****************  Version 8  *****************
 * User: Jeff Vahue   Date: 12/18/09   Time: 1:35p
 * Updated in $/software/control processor/code/system
 * SCR# 378
 * 
 * *****************  Version 7  *****************
 * User: Contractor V&v Date: 11/30/09   Time: 5:36p
 * Updated in $/software/control processor/code/system
 * SCR 106 XXXUserTable.c consistency
 * 
 * *****************  Version 6  *****************
 * User: Peter Lee    Date: 11/19/09   Time: 3:22p
 * Updated in $/software/control processor/code/system
 * SCR #315 SEU counts for TM Health Counts 
 * 
 * *****************  Version 5  *****************
 * User: Peter Lee    Date: 11/19/09   Time: 10:53a
 * Updated in $/software/control processor/code/system
 * SCR #315 Fix bug with maintaining SEU counts across power cycle
 * 
 * *****************  Version 4  *****************
 * User: Contractor V&v Date: 11/18/09   Time: 4:45p
 * Updated in $/software/control processor/code/system
 * Added macro condition for PC testing. 
 *  
 * *****************  Version 3  *****************
 * User: Peter Lee    Date: 11/05/09   Time: 5:30p
 * Updated in $/software/control processor/code/system
 * SCR #315 DIO CBIT for SEU Processing
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
 *****************************************************************************/

