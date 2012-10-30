#define INITIALIZATION_MANAGER_BODY
/******************************************************************************
            Copyright (C) 2007-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

  File:        InitializationManager.c


  Description: Perform all software and peripheral initialization for the
               System and Application.

 VERSION
     $Revision: 114 $  $Date: 12-10-27 4:59p $

******************************************************************************/
/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "adc.h"
#include "Cfgmanager.h"
#include "diomgr.h"
#include "gse.h"
#include "rtc.h"
#include "version.h"
#include "UploadMgr.h"
#include "Monitor.h"
#include "MemManager.h"
#include "box.h"
#include "UtilRegSetCheck.h"
#include "CBITManager.h"
#include "WatchDog.h"
#include "BusTO.h"
#include "EngineRun.h"
#include "Cycle.h"
#include "event.h"
#include "ActionManager.h"
#include "Creep.h"

#ifdef STE_TP
  #include "TestPoints.h"
#endif

#ifdef ENV_TEST  
  #include "Etm.h"
#endif

#ifndef WIN32
extern UINT32   __ghs_pramstart;
extern UINT32   __ghs_pramend;
#endif

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
// Num seconds to wait for user to press any key to signal startup in
// degraded mode.
#define PROCEED_AS_DEGRADED_PROMPT_INTERVAL 5
#undef DEBUG_STARTUP_TIME
#define INIT_MGR_DESC_SIZE         32
#define INIT_MGR_DRV_INIT_BUF_SIZE 80
#define INIT_MGR_PROMPT_BUF_SIZE   128

#define INIT_MGR_SYS_STARTUP_ID             99
#define INIT_MGR_APP_STARTUP_ID            200
#define INIT_MGR_PROMPTCONTINUE_STARTUP_ID 300

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
typedef RESULT (*DRIVER_INIT_FUNC)(SYS_APP_ID *, void *, UINT16 *);

typedef struct {
  DRIVER_INIT_FUNC InitFunction;
  RESULT InitResult;
  INT8 Description[INIT_MGR_DESC_SIZE];
} DRV_INIT_FUNC;

#define INIT_MGR_PBIT_LOG_MAX_SIZE 128
#define INIT_MGR_MAX_DRV  14

typedef struct {
  RESULT InitResult; 
  SYS_APP_ID SysId; 
  LOG_PRIORITY Priority; // Init to "0" which is HIGH_PRIORITY 
  UINT16 nSize; 
  UINT8 data[INIT_MGR_PBIT_LOG_MAX_SIZE];       // Maximum PBIT log must be < 128 bytes 
  TIMESTAMP ts; 
} INIT_MGR_PBIT_LOGS, *INT_MGR_PBIT_LOG_PTR;

#pragma pack(1)

// Watchdog Reset Log
typedef struct {
  UINT32  assertCnt;
  UINT32  unknownCnt;
  UINT32  pbitCnt;
  UINT8   cause;
} INIT_MGR_RESET_LOG;

#pragma pack()

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
INIT_MGR_PBIT_LOGS InitMgrPbitLogs[INIT_MGR_MAX_DRV]; 

static const DRV_INIT_FUNC DriverInitList[INIT_MGR_MAX_DRV] =
                                    {{DIO_Init,              DRV_OK,"Discrete I/0  "},  //1
                                     {TTMR_Init,             DRV_OK,"System Timer  "},  //2
                                     {SPI_Init,              DRV_OK,"SPI Bus       "},  //3
                                     {RTC_Init,              DRV_OK,"RT Clock      "},  //4
                                     {ADC_Init,              DRV_OK,"A/D Converter "},  //5
                                     {UART_Init,             DRV_OK,"UART          "},  //6
                                     {GSE_Init,              DRV_OK,"GSE           "},  //7
                                     {DPRAM_Init,            DRV_OK,"Dual-Port RAM "},  //8
                                     {EEPROM_Init,           DRV_OK,"EEPROM        "},  //9
                                     {FPGA_Initialize,       DRV_OK,"FPGA          "},  //10
                                     {QAR_Initialize,        DRV_OK,"QAR           "},  //11
                                     {Arinc429DrvInitialize, DRV_OK,"ARINC 429     "},  //12
                                     {FlashInitialize,       DRV_OK,"Data Flash    "},  //13
                                     {BTO_Init,              DRV_OK,"XL BUS T/O    "}   //14
                                     // {Arinc429_Test,  DRV_OK, "ARINC 429 TEST"}
                                    };

extern UINT32 __CRC_END; // address provided by linker

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void Im_Driver_Initialize(void);
static void Im_System_Initialize(BOOLEAN degradedMode);
static void Im_Application_Initialize(void);
static void Im_RecordDrvStartup_Logs(void);
static BOOLEAN Im_PromptContinueAsDegraded(void);
//static void Test_Initialize(void);

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/*****************************************************************************
 * Function:    Im_InitializeControlProcessor
 *
 * Description: Initialize the hardware, System firmware and Application
 *              software systems.
 *
 * Parameters:  void
 *
 * Returns:     void
 *
 * Notes:
 *
 ****************************************************************************/
void Im_InitializeControlProcessor(void)
{
   INT8 AssertLogBuf[ASSERT_MAX_LOG_SIZE];
   INIT_MGR_RESET_LOG resetLog;
   size_t size;

   // detect system restart for degraded mode
   BOOLEAN degradedStartup = FALSE;
   UINT32  watchdogFlag = WATCHDOG_RESTART_FLAG;
   BOOLEAN assertDetected = ( (watchdogFlag != 0) &&
       (watchdogFlag != INIT_RESTART_SET) ) ? TRUE : FALSE;
   // was this an assert or an unexpected restart?
   isWatchdogRestart = (assertDetected &&
     (watchdogFlag == ASSERT_RESTART_SET)) ? TRUE : FALSE;
   // reset flag for next restart
   WATCHDOG_RESTART_FLAG = 0;

   // Set direct-to-device write-mode until
   // SPIManager task has been initialized/started
   SPIMgr_SetModeDirectToDevice();

#ifndef WIN32
   // initialize CBIT Ram Test protected memory
   memset(&__ghs_pramstart, 0, ((UINT8*)&__ghs_pramend - (UINT8*)&__ghs_pramstart));
#endif

   CM_PreInit();
   RegSetCheck_Init(); 

    //Init drivers
   Im_Driver_Initialize();

   // turn on the four system LEDs
   DIO_SetPin( LED_CFG, DIO_SetHigh);
   DIO_SetPin( LED_XFR, DIO_SetHigh);
   DIO_SetPin( LED_STS, DIO_SetHigh);
   DIO_SetPin( LED_FLT, DIO_SetHigh);

#ifdef DEGRADE_MODE_DEBUG
   /*vcast_dont_instrument_start*/
   sprintf( AssertLogBuf,
       "\r\n\r\nWd1: 0x%08x Wd2: 0x%08x WdFlag: 0x%08x WdCnt: 0x%08x UnkCnt: 0x%08x PBitCnt: 0x%08x\r\n",
       SRAM_INIT_SAVE, SRAM_INV_SAVE, watchdogFlag,
       ASSERT_COUNT, UNKNOWN_RESTART_COUNT, INIT_RESTART_COUNT);     
   GSE_PutLine( AssertLogBuf);
   /*vcast_dont_instrument_end*/
#endif

   if(assertDetected)
   {
     degradedStartup = Im_PromptContinueAsDegraded();
   }

   //Init system
   Im_System_Initialize(degradedStartup);

   //Init Application
   Im_Application_Initialize();

   // Send an identification record via the GSE serial port
   GSE_PutLine("\r\n");
   GSE_PutLine("\r\n");
   GSE_PutLine(PRODUCT_COPYRIGHT);
   MonitorPrintVersion();
   
   //Check assert log and copy to flash
   size = Assert_GetLog(AssertLogBuf);
   if(size != 0)
   {
     LogWriteSystem( SYS_CBIT_ASSERT_LOG, LOG_PRIORITY_LOW, AssertLogBuf,
                        (UINT16)size, NULL);
     //Mark assert as written to flash
     Assert_MarkLogRead();

     // display the assert msg
     GSE_PutLine("\r\nASSERT MESSAGE:\r\n");
     GSE_PutLine( AssertLogBuf);
     GSE_PutLine("\r\n");
   }

   // If a watchdog/unknown restart was detected, write a log
   if (assertDetected)
   {
     resetLog.assertCnt = ASSERT_COUNT;
     resetLog.unknownCnt = UNKNOWN_RESTART_COUNT;
     resetLog.pbitCnt = INIT_RESTART_COUNT;
     resetLog.cause = (UINT8)watchdogFlag;
     LogWriteSystem( SYS_CBIT_RESET_LOG, LOG_PRIORITY_LOW, &resetLog,
                        sizeof(resetLog), NULL);
   }
   
   CM_Init();                //Clock manager goes last, RTC time is loaded into
                             //the system (interrupt driven) clock here
                             
   UartMgr_FlushChannels();  // Flush Uart Port just before normal operation

   Arinc429DrvFlushHWFIFO();  // Flush the ARINC429 Ports before normal operation
                             
                             
   // Start the Task Manager
   TmInitializeTaskDispatcher();
   TmStartTaskManager();

   __EI();
   
   Box_PowerOn_StartTimeCounting(); 

   // SCR# 640 inti real cfg verbosity after startup is complete
   Flt_InitDebugVerbosity();   // now that config data is available - init debug level
   
}   // End of Im_InitializeControlProcessor()

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:     Im_Driver_Initialize
 *
 * Description:  This function manages the call to all driver specific
 *               initialization that needs to be performed.
 *
 * Parameters:   None.
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static void Im_Driver_Initialize(void)
{
  UINT32 i;
  CHAR ResultStr[RESULTCODES_MAX_STR_LEN];
  INT8 StrBuf[INIT_MGR_DRV_INIT_BUF_SIZE];
  INT8 TimeStr[INIT_MGR_DRV_INIT_BUF_SIZE]; 
  DRV_INIT_FUNC DriverInits[sizeof(DriverInitList)/sizeof(DRV_INIT_FUNC)];

  INT_MGR_PBIT_LOG_PTR pLog; 
  TIMESTAMP ts; 
  TIMESTRUCT time_struct; 

  //Create a local, writable copy of the driver init list
  memcpy(DriverInits, DriverInitList, sizeof(DriverInits));

  //Clear InitMgrPbitLog Struct 
  memset ( InitMgrPbitLogs, 0x00, sizeof(INIT_MGR_PBIT_LOGS) * INIT_MGR_MAX_DRV ); 

  pLog = &InitMgrPbitLogs[0]; 
  memset ( pLog, 0x00, sizeof(INIT_MGR_PBIT_LOGS) * INIT_MGR_MAX_DRV ); 
  
  //Init all drivers in the driver init list defined at the top of this file
  //  InitMgrPbitLogs[].SysId != 0x00 or SYS_ID_NULL_LOG if PBIT fails and require PBIT log!!
  __EI();
  for(i = 0; i < INIT_MGR_MAX_DRV; i++) 
  {
    DriverInits[i].InitResult = DriverInits[i].InitFunction(&pLog->SysId,
                                            (void *) pLog->data,  &pLog->nSize);
    // Record Result to be used to in Im_RecordDrvStartup_Logs() to 
    pLog->InitResult = DriverInits[i].InitResult; 
    pLog++;                                                             
    STARTUP_ID((i+1));
  }

  // Allow the timer interrupt to occur so we can keep the system alive during init
  __RIR(TTMR_GPT0_INT_LEVEL);
  
  //Init ts to current time.  Note, SPI and RTC Drivers have been initialized at this point ! 
  CM_GetTimeAsTimestamp( &ts ); 
  pLog = &InitMgrPbitLogs[0]; 
  for (i = 0; i < INIT_MGR_MAX_DRV; i++)
  {
    pLog->ts = ts; 
    pLog++; 
  }
  
  // Set the Power On Time Write After Drv Init has completed.  
  //   Note: Expect Drv Initialization and Process.s routines to be < 2 seconds 
  //         thus the actual Power On Time shall be approx 2 seconds late.  
  PmSet_PowerOnTime( &ts ); 
  
  // Use for Box Power On Time calculations ! 
  Box_PowerOn_SetStartupTime( ts ); 

  //Output the init result to the GSE port
  GSE_PutLine("\r\n\r\nStart up: Driver initialization\r\n");
  //for(i = 0; i < NumDrivers; i++)
  for(i = 0; i < INIT_MGR_MAX_DRV; i++)
  {
    sprintf(StrBuf,"  %s = %s\r\n",DriverInits[i].Description,
                              RcGetResultCodeString(DriverInits[i].InitResult,ResultStr));
    GSE_PutLine(StrBuf);
  }
  
  //Output current Box System Time SRS-2979
  CM_ConvertTimeStamptoTimeStruct( &ts, &time_struct ); 
  snprintf(TimeStr,INIT_MGR_DRV_INIT_BUF_SIZE,"%02d/%02d/%04d %02d:%02d:%02d.%03d",
      time_struct.Month,
      time_struct.Day,
      time_struct.Year,
      time_struct.Hour,
      time_struct.Minute,
      time_struct.Second,
      time_struct.MilliSecond);
  sprintf(StrBuf,"  %s = %s", "Current Time  ", TimeStr);
  GSE_PutLine(StrBuf); 

}

/******************************************************************************
 * Function:      Im_System_Initialize
 *
 * Description:   Performs system pre-initialization and initialization tasks.
 *
 * Parameters:    BOOLEAN degradedMode
 *
 * Returns:       None
 *
 * Notes:         None
 *
 *****************************************************************************/
static void Im_System_Initialize(BOOLEAN degradedMode)
{
  UINT32 sysStartupId = INIT_MGR_SYS_STARTUP_ID;
  /*
  The function calls below "pre" initialize the system modules.  The system
  calls that are available at each step are listed after each init call.
  Once complete, init functions will be able to write time-stamped PBIT logs
  Note that the time stamp resolution is limited to 1s because the PIT driven
  wall clock is not available until the system is running.
  */
  STARTUP_ID(sysStartupId++);
  Flt_PreInitFaultMgr(); 

  //   DebugStr
  STARTUP_ID(sysStartupId++);
  TmInitializeTaskManager();

  //   DebugStr, TmTaskCreate, CM_GetRTCClock, CM_GetTimeAsTimestamp
  STARTUP_ID(sysStartupId++);
  LogPreInit();

  //   DebugStr, TmTaskCreate, CM_GetRTCClock, CM_GetTimeAsTimestamp, LogWrite
  //   LogWriteSystem.
  STARTUP_ID(sysStartupId++);
  NV_Init();
  //   DebugStr, TmTaskCreate, CM_GetRTCClock, CM_GetTimeAsTimestamp, LogWrite
  //   LogWriteSystem, NV_Open, NV_Read, NV_Write
  //   DebugStr, TmTaskCreate, CM_GetRTCClock, CM_GetTimeAsTimestamp, LogWrite
  //   LogWriteSystem, NV_Open, NV_Read, NV_Write, Flt_SetStatus, Flt_GetStatus


  /*
  Pre-init is complete, now system PBIT logs can be written to the log queue.
  The will be committed to flash when the system is done initialization and tasks
  are running.
  */
  STARTUP_ID(sysStartupId++);
  Assert_Init();

  STARTUP_ID(sysStartupId++);  
  MonitorInitialize();

  STARTUP_ID(sysStartupId++);
  MemInitialize ();

  STARTUP_ID(sysStartupId++);
  MSI_Init();               //Micro-server interface init depends on User

  STARTUP_ID(sysStartupId++);
  User_Init();              //User init is here, despite being an application
                            //because some system modules register a user
                            //message table with User.
#ifdef STE_TP
   STARTUP_ID(sysStartupId++); 
  TestPointInit();
#endif
  STARTUP_ID(sysStartupId++);  
  Flt_Init();

  STARTUP_ID(sysStartupId++);
  LogInitialize ();

  STARTUP_ID(sysStartupId++);
  SPIMgr_Initialize();

  STARTUP_ID(sysStartupId++);
  CfgMgr_Init(degradedMode);

  STARTUP_ID(sysStartupId++);
  ActionsInitialize();

  STARTUP_ID(sysStartupId++);
  Box_PowerOn_InitializeUsage (); 

  STARTUP_ID(sysStartupId++);
  PmInitializePowerManager();   // Power On Log Recorded Here !

  STARTUP_ID(sysStartupId++);
  Im_RecordDrvStartup_Logs();   // Driver Initialization PBIT Logs Recorded Here !
  STARTUP_ID(sysStartupId++);           
  QARMgr_Initialize();

  STARTUP_ID(sysStartupId++);
  FPGAMgr_Initialize();

  STARTUP_ID(sysStartupId++); 
  Arinc429MgrInitTasks();    // User Cfg Loaded and FPGA Reconfigured

  STARTUP_ID(sysStartupId++);
  QAR_Init_Tasks();         // User Cfg Loaded and FPGA Reconfigured

  UartMgr_Initialize();

  STARTUP_ID(sysStartupId++);
  SensorsInitialize();

  STARTUP_ID(sysStartupId++);
  TriggerInitialize();

  STARTUP_ID(sysStartupId++);
  FaultInitialize();

  STARTUP_ID(sysStartupId++);
  DataMgrIntialize();

  STARTUP_ID(sysStartupId++);
  UploadMgr_Init();

  STARTUP_ID(sysStartupId++);
  DIOMgr_Init();

  STARTUP_ID(sysStartupId++);
  MSSC_Init();

  STARTUP_ID(sysStartupId++);
  Box_Init();

  STARTUP_ID(sysStartupId++);
  CBITMgr_Initialize();

  STARTUP_ID(sysStartupId++);  
  CBITMgr_PWEHSEU_Initialize(); 

  STARTUP_ID(sysStartupId++);
  MSFX_Init();

  STARTUP_ID(sysStartupId++);
  CycleInitialize();

  STARTUP_ID(sysStartupId++);
  Creep_Initialize(degradedMode);   
  

#ifdef ENV_TEST
/*vcast_dont_instrument_start*/
  InitEtm();
/*vcast_dont_instrument_end*/
#endif
}

/******************************************************************************
 * Function:      Im_Application_Initialize
 *
 * Description:   Performs application initialization tasks, including FAST_Init
 *                and AircraftConfigInitialization.
 *
 * Parameters:    None
 *
 * Returns:       None
 *
 * Notes:         None
 *
 *****************************************************************************/
static void Im_Application_Initialize(void)
{
  UINT32 appStartupId = INIT_MGR_APP_STARTUP_ID;

  //MemTest_Init();
  STARTUP_ID(appStartupId++);
  FAST_Init();

  STARTUP_ID(appStartupId++);
  AircraftConfigInitialization();

  STARTUP_ID(appStartupId++);
  FSM_Init();

  STARTUP_ID(appStartupId++);
  EngRunInitialize();
  
  STARTUP_ID(appStartupId++);
  EventsInitialize();

  STARTUP_ID(appStartupId++);
  TrendInitialize();
  
  STARTUP_ID(appStartupId++);
  TH_Init();

  STARTUP_ID(appStartupId);
}


/******************************************************************************
 * Function:     Im_RecordDrvStartup_Logs
 * Description:  Loop through each Pbit Log for startup preparation.
 * Parameters:   None
 * Returns:      None
 * Notes:
 *   - Im_RecordDrvStartup_Logs should be called immediately after LogInitialization
 *     has completed. Although there are some system initialization that occurs 
 *     before Log Init is completed, all system logs generated by those process
 *     shall call the regular SystemWriteLog() and will pend log writing 
 *     until box initialization completes.
 * 
 *****************************************************************************/
static void Im_RecordDrvStartup_Logs(void)
{
  UINT32 i; 
  INT_MGR_PBIT_LOG_PTR pLog; 

  // clear this in preparation of PBIT results
  DIO_SetPin( LED_FLT, DIO_SetLow);

  pLog = &InitMgrPbitLogs[0]; 
  for(i = 0; i < INIT_MGR_MAX_DRV; i++) 
  {
    if (pLog->InitResult != DRV_OK) 
    {
      LogWriteSystem( pLog->SysId, pLog->Priority, (void *) pLog->data, pLog->nSize, 
                      &pLog->ts ); 
      
      // PBIT error turn on LED FLT
      DIO_SetPin( LED_FLT, DIO_SetHigh);
    }
    pLog++;   // Move to Next Array Element 
  }
}


/******************************************************************************
* Function:    Im_PromptContinueAsDegraded  
* Description: Prompt user to press any key to enter degraded mode.
*              This function loops performing a countdown display which is
*              updated every 1 second. Input is tested to check if the user
*              has signaled to proceed in degraded mode. If no input, this
*              function returns false.
* Parameters:  none
* Returns:     Bool TRUE if system should start up in degraded mode,otherwise
*              false.
* Notes:
*
*****************************************************************************/
BOOLEAN Im_PromptContinueAsDegraded(void)
{
  BOOLEAN proceedAsDegraded = FALSE;
  const CHAR* PromptString = "\rNormal startup in %d seconds.";
  CHAR       promptBuffer[INIT_MGR_PROMPT_BUF_SIZE];
  UINT32     lastTime;
  UINT32     currentTime;
  UINT32     startup_Id = INIT_MGR_PROMPTCONTINUE_STARTUP_ID;

  INT8       timeRemainingSecs = PROCEED_AS_DEGRADED_PROMPT_INTERVAL;
  /*
  Activate this code to to flash the fault light during countdown.
  UINT32     flashTime;
  BOOLEAN    fltLedState;
  DIO_OUT_OP ledState = DIO_SetLow;
  // Save the current FLT DIO state
  DIO_GetOutputPin(LED_FLT, &fltLedState);  
  DIO_SetPin( LED_FLT, ledState);
  */  
  
  // skip down a couple of line to separate the countdown prompt
  GSE_PutLine("\r\n\r\nProblems detected\r\nPress 'D' to start in degraded mode\r\n");
  
  snprintf(promptBuffer, sizeof(promptBuffer), PromptString, timeRemainingSecs);
  GSE_PutLine(promptBuffer);
  
  // Set up a countdown timer to await some input from the user
  lastTime = TTMR_GetHSTickCount();

  //flashTime = lastTime;

  // Wait for countdown expiration or user input.
  do 
  {
    currentTime = TTMR_GetHSTickCount();
    /*
    // Flash/toggle the FLT LED every 500msec until we leave this function.     
    if ( (currentTime - flashTime) >= (TICKS_PER_Sec / 2) )
    {
      ledState =  (DIO_OUT_OP)((UINT16)ledState ^ 0x0001);
      DIO_SetPin( LED_FLT, ledState);
      flashTime = currentTime;
    }
    */

    // Every 1 sec, update the prompt to show how much time the user has before
    // the system will proceed to attempt a normal mode startup.
    // NOTE: The PromptString will display the prompt to the same screen line is updated.
    if ( currentTime - lastTime > TICKS_PER_Sec )
    {
      --timeRemainingSecs;
      lastTime = currentTime;
      snprintf(promptBuffer, sizeof(promptBuffer), PromptString, timeRemainingSecs);
      GSE_PutLine(promptBuffer);
    }

    UART_CheckForTXDone();

    // Check to see if the user has pressed a key
    if ( GSE_getc() == 'D' ) 
    {
      proceedAsDegraded = TRUE;
      GSE_PutLine("\r\n\r\nSystem will startup in degraded mode using default configuration");
    }

    // We will be in this loop awhile,
    // so pet the watchdog.
    STARTUP_ID(startup_Id++);
  } while( !proceedAsDegraded && (timeRemainingSecs > 0) );

  GSE_PutLine("\r\n\r\n");
  /*
  // Restore the current FLT DIO state
  DIO_SetPin(LED_FLT, (DIO_OUT_OP)fltLedState);
  */
 
  // Return results to caller.
  return proceedAsDegraded;
}


/******************************************************************************
 * Function:    Im_StartupTickHandler  
 * Description: Interrupt handler used during system startup. Used to check for stalled tasks
 *              during startup.
 * Parameters:  none
 * Returns:     none             
 * Notes:
 *
 *****************************************************************************/
void Im_StartupTickHandler(void)
{
   static UINT32 lastId = 0xFFFFFFFF;
   static UINT32 sameIdCount = 0;
   if ( startupId == lastId)
   {
     ++sameIdCount;
   }
   else
   {
#ifdef DEBUG_STARTUP_TIME
     /*vcast_dont_instrument_start*/
     if ( lastId > 50 && lastId != 0xFFFFFFFF)  // no msgs during Driver init ID 0-13
     {
       GSE_DebugStr( DBGOFF, FALSE, "Startup ID: %d took %d ms", lastId, sameIdCount*10);
     }
     /*vcast_dont_instrument_end*/
#endif
     sameIdCount = 0;
     lastId = startupId;
   }

   // pulse the WD if it's ok
   if ( sameIdCount < MAX_STARTUP_DURATION)
   {
     KICKDOG;
   }

#ifdef DEBUG_STARTUP_TIME
   /*vcast_dont_instrument_start*/
   else if (sameIdCount > (MAX_STARTUP_DURATION+100))
   {
     GSE_DebugStr( DBGOFF, FALSE, "Startup ID: %d took %d ms", startupId, sameIdCount*10);
   }
   /*vcast_dont_instrument_end*/
#endif


}

/******************************************************************************
 * Function:    Test_Initialize  
 * Description: Initialize EMI/Environmental test code specific modules
 * Parameters:  void
 * Returns:     void
 * Notes:
 *
 *****************************************************************************/
/*static void Test_Initialize(void)
 - {
 -  INT8* ExampleTests[] = {"SDRAM","SPI Bus","SPI RTC","SPI EEPROM",
 -                               "SPI ADC","QAR","ARINC 429","DUAL-PORT","DATA FLASH"};
 -  UINT32 i;
 -   TestMgr_Init();
 -
 -   for(i = 0; i < 9; i++)
 -   {
 -     TestMgr_AddTest(ExampleTests[i], 0, TRUE);
 -   }
 -   TestMgr_Start();
 - }*/

// End of File


/*************************************************************************
 *  MODIFICATIONS
 *    $History: InitializationManager.c $
 * 
 * *****************  Version 114  *****************
 * User: Peter Lee    Date: 12-10-27   Time: 4:59p
 * Updated in $/software/control processor/code/system
 * SCR #1190 Creep Requirements
 * 
 * *****************  Version 113  *****************
 * User: Melanie Jutras Date: 12-10-10   Time: 12:54p
 * Updated in $/software/control processor/code/system
 * SCR 1172 PCLint 545 Suspicious use of & Error
 * 
 * *****************  Version 112  *****************
 * User: Jim Mood     Date: 9/21/12    Time: 5:28p
 * Updated in $/software/control processor/code/system
 * SCR 1107: 2.0.0 Requirements - Time History
 * 
 * *****************  Version 111  *****************
 * User: Contractor V&v Date: 9/14/12    Time: 5:15p
 * Updated in $/software/control processor/code/system
 * FAST 2 Activate Trend
 * 
 * *****************  Version 110  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 109  *****************
 * User: John Omalley Date: 12-08-16   Time: 4:15p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Fault Action Processing
 * 
 * *****************  Version 108  *****************
 * User: John Omalley Date: 12-08-14   Time: 2:20p
 * Updated in $/software/control processor/code/system
 * SCR 1076 - Code Review Updates
 * 
 * *****************  Version 107  *****************
 * User: Contractor V&v Date: 7/18/12    Time: 6:27p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 Added Trend task
 * 
 * *****************  Version 106  *****************
 * User: John Omalley Date: 12-07-17   Time: 11:29a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Added new Action Object
 * 
 * *****************  Version 105  *****************
 * User: John Omalley Date: 12-05-22   Time: 2:18p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Check in for Dave
 * 
 * *****************  Version 104  *****************
 * User: John Omalley Date: 4/27/12    Time: 5:04p
 * Updated in $/software/control processor/code/system
 * 
 * *****************  Version 103  *****************
 * User: Contractor V&v Date: 4/23/12    Time: 8:01p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2  Engine Run
 * 
 * *****************  Version 102  *****************
 * User: Jim Mood     Date: 9/26/11    Time: 6:15p
 * Updated in $/software/control processor/code/system
 * SCR 575 Modfix.  Needed to change the place where FSM was initialized
 * 
 * *****************  Version 101  *****************
 * User: Contractor V&v Date: 7/26/11    Time: 7:29p
 * Updated in $/software/control processor/code/system
 * SCR 1035 fix EEPROM causing startup errors
 * 
 * *****************  Version 100  *****************
 * User: Jim Mood     Date: 7/20/11    Time: 10:57a
 * Updated in $/software/control processor/code/system
 * SCR 575: GSM Enable when engine status is lost.  (Part of changes for
 * the Fast State Machine)
 * 
 * *****************  Version 99  *****************
 * User: Contractor2  Date: 6/27/11    Time: 1:53p
 * Updated in $/software/control processor/code/system
 * SCR #515 Enhancement - sys.get.mem.l halts processor when invalid
 * memory is referenced
 * 
 * *****************  Version 98  *****************
 * User: Jim Mood     Date: 10/26/10   Time: 5:12p
 * Updated in $/software/control processor/code/system
 * SCR 958 ResultCodes reentrancy
 * 
 * *****************  Version 97  *****************
 * User: John Omalley Date: 10/18/10   Time: 11:20a
 * Updated in $/software/control processor/code/system
 * SCR 947 - Remove duplicate CM_PreInit
 * 
 * *****************  Version 96  *****************
 * User: Peter Lee    Date: 8/31/10    Time: 2:57p
 * Updated in $/software/control processor/code/system
 * SCR #840 Code Review Updates
 * 
 * *****************  Version 95  *****************
 * User: Peter Lee    Date: 8/11/10    Time: 5:32p
 * Updated in $/software/control processor/code/system
 * SCR #777 Consolidate duplicate Program CRC. 
 * 
 * *****************  Version 94  *****************
 * User: Jeff Vahue   Date: 8/03/10    Time: 12:37p
 * Updated in $/software/control processor/code/system
 * Remove (ifdef out) Degraded Mode debug message
 * 
 * *****************  Version 93  *****************
 * User: Contractor2  Date: 7/30/10    Time: 7:18p
 * Updated in $/software/control processor/code/system
 * SCR #638 Startup requests for Degraded Mode
 * 
 * *****************  Version 92  *****************
 * User: John Omalley Date: 7/29/10    Time: 7:51p
 * Updated in $/software/control processor/code/system
 * SCR 754 - Added a FIFO flush after initialization
 * 
 * *****************  Version 91  *****************
 * User: Peter Lee    Date: 7/29/10    Time: 6:51p
 * Updated in $/software/control processor/code/system
 * SCR #754 Flush Uart Interface before normal op
 * 
 * *****************  Version 90  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:10a
 * Updated in $/software/control processor/code/system
 * SCR #698 - Fix code review findings
 * 
 * *****************  Version 89  *****************
 * User: Contractor V&v Date: 7/27/10    Time: 2:26p
 * Updated in $/software/control processor/code/system
 * SCR #573 WD overrun at startup SCR #282 Shutdown processing
 * 
 * *****************  Version 88  *****************
 * User: Contractor2  Date: 7/13/10    Time: 3:24p
 * Updated in $/software/control processor/code/system
 * SCR #687 Watchdog Reset Log
 * Fix log file data.
 * 
 * *****************  Version 87  *****************
 * User: Contractor2  Date: 7/13/10    Time: 2:35p
 * Updated in $/software/control processor/code/system
 * SCR #638 Startup requests for Degraded Mode
 * Fixed display output
 * 
 * *****************  Version 86  *****************
 * User: Contractor2  Date: 7/09/10    Time: 1:44p
 * Updated in $/software/control processor/code/system
 * SCR #687 Implementation: Watchdog Reset Log
 * 
 * *****************  Version 85  *****************
 * User: Jeff Vahue   Date: 7/06/10    Time: 1:47p
 * Updated in $/software/control processor/code/system
 * SCR# 675 - enter degreaded mode only when a 'D' is entered
 * 
 * *****************  Version 84  *****************
 * User: Jeff Vahue   Date: 6/29/10    Time: 5:27p
 * Updated in $/software/control processor/code/system
 * SCR# 640 - Allow Normal verbosity debug messages during startup
 * 
 * *****************  Version 83  *****************
 * User: Contractor3  Date: 6/25/10    Time: 9:46a
 * Updated in $/software/control processor/code/system
 * SCR #662 - Changes based on code review
 * 
 * *****************  Version 82  *****************
 * User: Contractor2  Date: 6/18/10    Time: 1:48p
 * Updated in $/software/control processor/code/system
 * SCR #638 Misc - Startup requests for Degraded Mode
 * Fix debug error display
 * 
 * *****************  Version 81  *****************
 * User: Contractor2  Date: 6/14/10    Time: 1:11p
 * Updated in $/software/control processor/code/system
 * SCR #483 Function Names must begin with the CSC it belongs with.
 * 
 * *****************  Version 80  *****************
 * User: Jeff Vahue   Date: 6/09/10    Time: 2:58p
 * Updated in $/software/control processor/code/system
 * SCR #638 - modification  made to investigate erroneous degraded mode,
 * seem to have fixed the issue?  Test will detect any occurance in the
 * future.
 * 
 * *****************  Version 79  *****************
 * User: John Omalley Date: 6/08/10    Time: 12:14p
 * Updated in $/software/control processor/code/system
 * SCR 627 - Updated for LJ60
 * 
 * *****************  Version 78  *****************
 * User: Contractor2  Date: 6/07/10    Time: 1:28p
 * Updated in $/software/control processor/code/system
 * SCR #485 Escape Sequence & Box Config
 * 
 * *****************  Version 77  *****************
 * User: Contractor2  Date: 6/07/10    Time: 11:51a
 * Updated in $/software/control processor/code/system
 * SCR #628 Count SEU resets during data capture
 * 
 * *****************  Version 76  *****************
 * User: Peter Lee    Date: 5/31/10    Time: 6:35p
 * Updated in $/software/control processor/code/system
 * SCR #618 F7X and UartMgr Requirements Implementation 
 * 
 * *****************  Version 75  *****************
 * User: Contractor2  Date: 5/25/10    Time: 3:03p
 * Updated in $/software/control processor/code/system
 * SCR #563 Implementation: Req SRS-1811,1813,1814,1815,3162 - CBIT RAM
 * Tests
 * Fix for ram test protected sections. Minor coding standard fixes
 * 
 * *****************  Version 74  *****************
 * User: Contractor V&v Date: 5/19/10    Time: 6:11p
 * Updated in $/software/control processor/code/system
 * SCR #404 Misc - Preliminary Code Review Issues
 * 
 * *****************  Version 73  *****************
 * User: Contractor2  Date: 5/06/10    Time: 2:08p
 * Updated in $/software/control processor/code/system
 * SCR #579 Change LogWriteSys to return void
 * 
 * *****************  Version 72  *****************
 * User: Jeff Vahue   Date: 4/28/10    Time: 5:50p
 * Updated in $/software/control processor/code/system
 * SCR #573 - startup WD changes
 * 
 * *****************  Version 71  *****************
 * User: Contractor2  Date: 4/27/10    Time: 1:42p
 * Updated in $/software/control processor/code/system
 * SCR 187: Degraded mode. Modified to correctly test watchdog reset
 * flags.
 * 
 * *****************  Version 70  *****************
 * User: Contractor V&v Date: 4/22/10    Time: 6:41p
 * Updated in $/software/control processor/code/system
 * SCR #187 Run in degraded mode
 * 
 * *****************  Version 69  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:10p
 * Updated in $/software/control processor/code/system
 * SCR #317 Implement safe strncpy
 * 
 * *****************  Version 68  *****************
 * User: Jeff Vahue   Date: 4/07/10    Time: 12:12p
 * Updated in $/software/control processor/code/system
 * SCR #534 - remove unused variable
 * 
 * *****************  Version 67  *****************
 * User: Jeff Vahue   Date: 4/07/10    Time: 12:09p
 * Updated in $/software/control processor/code/system
 * SCR #534 - Cleanup Program CRC messages
 * 
 * *****************  Version 66  *****************
 * User: Jeff Vahue   Date: 4/07/10    Time: 11:45a
 * Updated in $/software/control processor/code/system
 * SCR #534 - set the default program CRC to allow for development and
 * reformat so messages related tot he CRC.
 * 
 * *****************  Version 65  *****************
 * User: Contractor2  Date: 4/06/10    Time: 4:08p
 * Updated in $/software/control processor/code/system
 * SCR 528: Display Software CRC in power-up message
 * 
 * *****************  Version 64  *****************
 * User: Jeff Vahue   Date: 3/29/10    Time: 2:31p
 * Updated in $/software/control processor/code/system
 * SCR# 514 - display assert message retrieved from EEPROM at startup
 * 
 * *****************  Version 63  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/system
 * SCR# 483 - Function Names
 * 
 * *****************  Version 62  *****************
 * User: Contractor V&v Date: 3/10/10    Time: 4:42p
 * Updated in $/software/control processor/code/system
 * SCR #464 Move Version Info into own file and support it.
 * 
 * *****************  Version 61  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 4:01p
 * Updated in $/software/control processor/code/system
 * SCR 67 Interrupted SPI Access (Multiple / Nested SPI Access)
 * 
 * *****************  Version 60  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 59  *****************
 * User: Jeff Vahue   Date: 2/19/10    Time: 12:58p
 * Updated in $/software/control processor/code/system
 * SCR# 455 - remove LINT issues
 * 
 * *****************  Version 58  *****************
 * User: Jeff Vahue   Date: 2/17/10    Time: 1:26p
 * Updated in $/software/control processor/code/system
 * SCR# 453 - LED processing at startup
 * SCR# 369 - remove test code
 * 
 * *****************  Version 57  *****************
 * User: Jeff Vahue   Date: 1/21/10    Time: 12:59p
 * Updated in $/software/control processor/code/system
 * SCR# 369
 * 
 * *****************  Version 56  *****************
 * User: Jim Mood     Date: 1/20/10    Time: 3:44p
 * Updated in $/software/control processor/code/system
 * Added msfx_init() to system initialization
 * 
 * *****************  Version 55  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 5:22p
 * Updated in $/software/control processor/code/system
 * SCR# 378
 * 
 * *****************  Version 54  *****************
 * User: Peter Lee    Date: 12/18/09   Time: 4:36p
 * Updated in $/software/control processor/code/system
 * SCR #368 Use of Flt_SetStatus() before Flt_Init(). 
 * 
 * *****************  Version 53  *****************
 * User: Jeff Vahue   Date: 12/18/09   Time: 3:52p
 * Updated in $/software/control processor/code/system
 * SCR# 378
 * 
 * *****************  Version 52  *****************
 * User: Peter Lee    Date: 11/20/09   Time: 2:35p
 * Updated in $/software/control processor/code/system
 * SCR #345 Implement FPGA Watchdog Requirements
 * 
 * *****************  Version 51  *****************
 * User: Peter Lee    Date: 11/19/09   Time: 10:55a
 * Updated in $/software/control processor/code/system
 * SCR #315 Fix bug with maintaining SEU counts across power cycle
 * 
 * *****************  Version 50  *****************
 * User: Peter Lee    Date: 10/23/09   Time: 9:36a
 * Updated in $/software/control processor/code/system
 * SCR #315 CBIT / SEU Reg Processing 
 * 
 * *****************  Version 49  *****************
 * User: John Omalley Date: 10/14/09   Time: 5:09p
 * Updated in $/software/control processor/code/system
 * SCR 292 - Reversed the Order of the Sensor and Trigger initialization.
 * 
 * *****************  Version 48  *****************
 * User: Peter Lee    Date: 10/07/09   Time: 11:34a
 * Updated in $/software/control processor/code/system
 * SCR #292 Issues found with v2.0.11 Test10052009
 * 
 * *****************  Version 47  *****************
 * User: John Omalley Date: 9/30/09    Time: 9:38a
 * Updated in $/software/control processor/code/system
 * Add Fault Initialization for sensor processing.
 * 
 * *****************  Version 46  *****************
 * User: Peter Lee    Date: 9/29/09    Time: 1:25p
 * Updated in $/software/control processor/code/system
 * SCR #280 Fix Compiler Warnings
 * 
 * *****************  Version 45  *****************
 * User: Peter Lee    Date: 9/28/09    Time: 4:16p
 * Updated in $/software/control processor/code/system
 * SCR 275 Box Power On Usage Requirements
 * 
 * *****************  Version 44  *****************
 * User: Peter Lee    Date: 9/18/09    Time: 1:38p
 * Updated in $/software/control processor/code/system
 * SRS-2979 
 * 
 * *****************  Version 43  *****************
 * User: Peter Lee    Date: 9/17/09    Time: 11:48a
 * Updated in $/software/control processor/code/system
 * SCR #94, #96 Support PBIT and PBIT structure recordings. 
 * SCR #65 Sequencing of recording intialization logs.  
 * 
 * *****************  Version 42  *****************
 * User: Peter Lee    Date: 9/15/09    Time: 6:22p
 * Updated in $/software/control processor/code/system
 * SCR #94, #95 return PBIT Log Structure to Init Mgr
 * 
 * *****************  Version 39  *****************
 * User: John Omalley Date: 8/06/09    Time: 3:52p
 * Updated in $/software/control processor/code/system
 * SCR 179  - Added Flash initialization to the Init Manager. This change
 * was needed to support the logic check of memory problems. 
 *
 * *****************  Version 38  *****************
 * User: Peter Lee    Date: 6/30/09    Time: 2:44p
 * Updated in $/software/control processor/code/system
 * SCR #204 Misc Code Review Updates
 * 
 * *****************  Version 37  *****************
 * User: Peter Lee    Date: 4/29/09    Time: 10:27a
 * Updated in $/software/control processor/code/system
 * SCR #168 Updates per preliminary code review (jv). Remove extra \n from
 * Driver_Initialize()
 * 
 * *****************  Version 36  *****************
 * User: Jim Mood     Date: 4/22/09    Time: 4:03p
 * Updated in $/control processor/code/system
 * Function prototype
 * 
 * *****************  Version 35  *****************
 * User: Jim Mood     Date: 4/09/09    Time: 1:56p
 * Updated in $/control processor/code/system
 * Changed initilization order so the Clock Manager init is dead-last.
 * This ensures the interrupt driven "system clock" is started right after
 * its power on value is loaded from the battery backed RTC.
 * 
 * *****************  Version 34  *****************
 * User: Jim Mood     Date: 4/02/09    Time: 9:02a
 * Updated in $/control processor/code/system
 * Moved User init after MSI init b/c User needs to register a command
 * handler with MSI
 * Added init calls to Box and AircraftConfig
 * 
 * *****************  Version 33  *****************
 * User: Jim Mood     Date: 2/17/09    Time: 5:09p
 * Updated in $/control processor/code/system
 * 
 * *****************  Version 32  *****************
 * User: Jim Mood     Date: 2/06/09    Time: 8:58a
 * Updated in $/control processor/code/system
 * 
 * *****************  Version 31  *****************
 * User: Jim Mood     Date: 2/05/09    Time: 11:33a
 * Updated in $/control processor/code/system
 * 
 * *****************  Version 30  *****************
 * User: Peter Lee    Date: 1/19/09    Time: 5:27p
 * Updated in $/control processor/code/system
 * Removed Arinc429_Test() from Drv Init.  Func now part of
 * Arinc429_Init()
 * 
 * *****************  Version 29  *****************
 * User: Peter Lee    Date: 1/06/09    Time: 2:59p
 * Updated in $/control processor/code/system
 * SCR 129 Misc FPGA Updates. Add call to FPGAMgr_Initialize(). 
 * 
 * *****************  Version 28  *****************
 * User: Jim Mood     Date: 11/12/08   Time: 1:40p
 * Updated in $/control processor/code/system
 * 
 * *****************  Version 27  *****************
 * User: Jim Mood     Date: 10/23/08   Time: 3:06p
 * Updated in $/control processor/code/system
 * Broke up system initilization into two stages.  A pre-init that
 * performs minimal initilization, allowing PBIT log writes, and a normal
 * startup to finish initializing the system
 * 
 * *****************  Version 26  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 3:05p
 * Updated in $/control processor/code/system
 * 1) SCR #87 Function Prototype
 * 2) PM Code
 * 
 *
 ***************************************************************************/
