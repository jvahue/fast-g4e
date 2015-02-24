#define POWERMANAGER_BODY
/******************************************************************************
            Copyright (C) 2008-2015 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

   File:         PowerManager.c

   Description:  Monitor the aircraft Battery voltage and process failures of
                 the aircraft Bus or Battery power supplies. Manage an orderly
                 shutdown of applications on powerdown.

   VERSION
   $Revision: 71 $  $Date: 2/17/15 7:40p $


******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include <string.h>


/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "PowerManager.h"

#include "dio.h"

#include "gse.h"
#include "Utility.h"
#include "ClockMgr.h"

#include "adc.h"

#include "user.h"
#include "LogManager.h"

#include "NVMgr.h"

#include "box.h"

#include "CfgManager.h"

#include "Assert.h"
#include "WatchDog.h"


/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/


// PM_LATCH_CTRL Structure
typedef struct
{
  BOOLEAN*       pBusyFlag; // Ptr to app-supplied flag indicating the task is busy
  PM_BUSY_USAGE  usage;     // Indicates under which execution mode pBusyFlag is used.
}PM_LATCH_ENTRY;

typedef struct
{
  BOOLEAN bFsmActive;    // Indicates that FSM is configured.
  PM_LATCH_ENTRY task[PM_MAX_BUSY];
}PM_LATCH_CTRL;

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static PM_INTERNAL_LOG pmInternalLog;
// static BOOLEAN CfgBattery;   // Init Pm.Battery to this variable until callback
                                // called by PmSetCallBack_AppBusy().
//static UINT32 BattLatchTime; // Init Pm.battLatchTime to this variable until call
                             //   back called by PmSetCallBack_BattLatchTime()

// Array of pointers to tasks, activities, etc which constitute
// app-busy status during shutdown
static PM_LATCH_CTRL m_latchCtrl;    // Control structure for task busy flags

static BOOLEAN  m_FSMAppBusy;
POWERMANAGER_CFG m_PowerManagerCfg;


/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void PmTask( void* pPBlock);
static void PmProcessPowerFailure (void);

static BOOLEAN PmCallAppShutDownNormal (void);
static void    PmCallAppBusInterrupt (void);

static BOOLEAN PmUpdateAppBusyStatus(void);

static void PmGlitchProcess ( void );
static void PmBatteryProcess ( void );
// Test Only !!!
/*
static BOOLEAN testAppShutDownQuick_0( void );
static BOOLEAN testAppShutDownQuick_1( void );
static BOOLEAN testAppShutDownQuick_2( void );

static BOOLEAN testAppShutDownNormal_0( void );
static BOOLEAN testAppShutDownNormal_1( void );
static BOOLEAN testAppShutDownNormal_2( void );
*/
// Test Only !!!


#include "PowerManagerUserTables.c" // Include user cmd tables & function .c

/*****************************************************************************/
/* Local Constants                                                           */
/*****************************************************************************/
#ifdef GENERATE_SYS_LOGS
  // PM System Logs - For Test Purposes
  static PM_LOG_DATA_POWER_FAILURE SysPmPowerFailure[] = {
    {SYS_ID_PM_POWER_FAILURE_LOG, {1, 0}, PM_NORMAL, (BOOLEAN) FALSE}
  };

  static PM_LOG_DATA_POWER_OFF SysPmPowerOff[] = {
    {0x66666666, FALSE }
  };

  static PM_LOG_DATA_RESTORE_DATA_FAILED SysPmRestoreDataFailed[] = {
    {0x77777777}
  };

  static PM_LOG_DATA_FORCE_SHUTDOWN SysPmForceShutDown[] = {
    {0x88888888}
  };
#endif


/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:    PmInitializePowerManager
 *
 * Description: Initialize the data and control information necessary for the
 *              power management system to function.
 *
 * Parameters:  void
 *
 * Returns:     void
 *
 * Notes:
 *
 *   Requires SPI to be initialized
 *
 *   Requires Clock Manager to be initialized
 *
 *   Requires DIO to initialize PWR_HOLD output
 *
 *   Requires Log and Flash to be initialized
 *         - Will queue a max of two logs to be recorded !
 *         - The Power Off log first and then the
 *           Power On.

 *   Call back function must be called after Pm has initialized
 *
 *   User Manager must be setup as call to User_AddRootCmd() is called below.
 *
 *   Requires Cfg Mgr must be setup and PM Cfg retrieved
 *
 *****************************************************************************/
void PmInitializePowerManager(void)
{
    TCB        tcbTaskInfo;
    UINT32     i;
    RESULT     result;     // See ResultCodes.h for ADC_() calls
    TIMESTAMP  ts;

    PM_LOG_DATA_POWER_FAILURE       powerFailureData;
    PM_LOG_DATA_POWER_OFF           powerOffData;
    PM_LOG_DATA_RESTORE_DATA_FAILED powerRestoreFailedData;

    // Unlatch the battery.
    // NOTE: Duplicated action. But Ok.
    // NOTE: DIO Driver Initialization normally will initialize all outputs to "0".
    // LATCH_BATTERY;  // Test Purpose Only.  Remove later.

    UNLATCH_BATTERY;

    // Initialize PM Variables and State
    Pm.State = PM_NORMAL;


    // Restore User Default
    memset ( (void *) &m_PowerManagerCfg, 0, sizeof(POWERMANAGER_CFG) );

    memcpy(&m_PowerManagerCfg,
           &(CfgMgr_RuntimeConfigPtr()->PowerManagerCfg),
           sizeof(m_PowerManagerCfg));

    Pm.battLatchTime = m_PowerManagerCfg.BatteryLatchTime * PM_MIF_PER_SEC;
    Pm.Battery = m_PowerManagerCfg.Battery;

    // The m_latchCtrl structure was already init'ed

    //Register the FSM app busy flag that is internal to the PM.
    m_FSMAppBusy = FALSE;
    PmRegisterAppBusyFlag(PM_FSM_BUSY,&m_FSMAppBusy, PM_BUSY_FSM);

// Test Below

    // Note Pm.AppBusy set thru PmSetCallBack_AppBusy() from external app
    //    will init to point to local AppBusy for test.
    Pm.AppBusy              = FALSE;   // Init Local AppBusy to FALSE !
    // AppBusy              = TRUE; // Init Local AppBusy to TRUE ! for Testing !


// Test Above

    // default states
    Pm.PmStateTimer         = 0;
    Pm.PmBattDeglitchTimer  = 0;
    Pm.BusVoltage           = 0;
    Pm.BatteryVoltage       = 0;
    Pm.BusPowerInterruptCnt = 0;
    Pm.bLatchToBattery      = FALSE;

    // Create the Power Manager Task
    memset(&tcbTaskInfo, 0, sizeof(tcbTaskInfo));
    strncpy_safe(tcbTaskInfo.Name, sizeof(tcbTaskInfo.Name),"Pm Task",_TRUNCATE);
    tcbTaskInfo.TaskID         = (TASK_INDEX)Pm_Task;
    tcbTaskInfo.Function       = PmTask;
    tcbTaskInfo.Priority       = taskInfo[Pm_Task].priority;
    tcbTaskInfo.Type           = taskInfo[Pm_Task].taskType;
    tcbTaskInfo.modes          = taskInfo[Pm_Task].modes;
    tcbTaskInfo.MIFrames       = taskInfo[Pm_Task].MIFframes;  //All 32 MIFframes.Every 10 msec
    tcbTaskInfo.Rmt.InitialMif = taskInfo[Pm_Task].InitialMif;
    tcbTaskInfo.Rmt.MifRate    = taskInfo[Pm_Task].MIFrate;
    tcbTaskInfo.Enabled        = TRUE;
    tcbTaskInfo.Locked         = TRUE;
    tcbTaskInfo.pParamBlock    = &Pm;

    TmTaskCreate (&tcbTaskInfo);

    // Retrieve and set PM_EEPROM data
    memset ( (char *) &Pm_Eeprom, 0x00, sizeof(PM_EEPROM_DATA) );

    // Initialize Log
    memset ( (char *) &pmInternalLog, 0x00, sizeof(PM_INTERNAL_LOG) );

    // Set local pointers
    memset ( (char*) &powerFailureData, 0x00, sizeof(PM_LOG_DATA_POWER_FAILURE) );
    memset ( (char*) &powerOffData, 0x00, sizeof(PM_LOG_DATA_POWER_OFF) );
    memset ( (char*) &powerRestoreFailedData,
                0x00, sizeof(PM_LOG_DATA_RESTORE_DATA_FAILED) );


    result = NV_Open(NV_PWR_MGR);
    if ( result == SYS_OK )
    {
      // Read Pm_Eeprom data
      NV_Read( NV_PWR_MGR, 0, (void*) &Pm_Eeprom, sizeof(PM_EEPROM_DATA) );
    }

    if ( result != SYS_OK )  // Open failure - Pm_Eeprom value is Bad !
    {
      // PM_EEPROM_CORRUPT_LOG
      // Create Power Off Log for Corrupt Eeprom Data Case !
      // CM_GetTimeAsTimestamp( &PmInternalLog.ts );

      // ASSERT if PmPowerOnTime->Timestamp != 0 !!!
      // CM_GetTimeAsTimestamp( &Pm_Eeprom.StateTrans_Time );
      // Note: Pm.PowerOnTime set during Drv Initialization().
      pmInternalLog.ts = Pm.PowerOnTime;

      pmInternalLog.Type = SYS_ID_PM_RESTORE_DATA_FAILED;
      pmInternalLog.State = PM_NORMAL;
      pmInternalLog.bPowerFailure = FALSE;
      // Since EEPROM corrupted, can not used saved copy.
      pmInternalLog.bBattery = Pm.Battery;
      // Save state of NVMgr result code
      powerRestoreFailedData.ResultCode = (UINT32) result;
      // Set Halt Processing Completed Flag to FALSE
      pmInternalLog.bHaltCompleted = FALSE;
    }
    else // Pm_Eeprom value Ok !
    {
      pmInternalLog.State = Pm_Eeprom.State;
      pmInternalLog.bBattery = Pm_Eeprom.bBattery;
      pmInternalLog.bHaltCompleted = Pm_Eeprom.bHaltCompleted;

      // Display the contents of the app busy flags from the shutdown
      GSE_DebugStr(NORMAL,TRUE,
                   "PM App Busy Flags at last shutdown: 0x%08X \r\n",
                   Pm_Eeprom.appBusyFlags);

      // Determine last power off condition.
      if ( (Pm_Eeprom.bPowerFailure == TRUE) || ( Pm_Eeprom.State != PM_HALT ) )
      {

// PM_POWER_OFF_FAILURE_LOG
//   Power fail detected on last power on and system attempted to transition
//   to HALT processing.
//   Note: If Pm_Eeprom.State == HALT then system completed HALT procesing
//         If Pm_Eeprom.State != HALT (most likely GLITCH or BATTERY), then system
//            did not complete HALT processing

// PM_POWER_OFF_FAILURE_LOG
//    bPowerFailure was not set and we have not reached HALT, therefore
//    Power had shut down early during PM_GLITCH or PM_BATTERY, PM_SHUTDOWN, PM_NORMAL mode.
//    See Pm_Eeprom.State for state where power failure had occurred.
//    Note: Power Off time is only approximate in this case.

        pmInternalLog.Type = SYS_ID_PM_POWER_FAILURE_LOG;

        // Bus Power Failure !
        pmInternalLog.bPowerFailure = TRUE;

        // PowerOffRequest_Time should be valid as this is set before bPowerFailure
        //   gets set !
        pmInternalLog.ts = Pm_Eeprom.StateTrans_Time;

        // Once bPowerFailure set we normally will transition to PM_SHUTDOWN

      } // End if ( bPowerFailure == TRUE )
      else // Else bPowerFailure == FALSE
      {
        // PM_POWER_OFF_LOG shutdown completes successfully

        // No Bus Power Failure, we have reached PM_HALT thus ShutDown Complete
        pmInternalLog.Type = SYS_ID_PM_POWER_OFF_LOG;

        // PowerOffRequest_Time valid
        // PmInternalLog.ts = Pm_Eeprom.PowerOffRequest_Time;
        pmInternalLog.ts = Pm_Eeprom.StateTrans_Time;

        // Normal Power Off
        pmInternalLog.bPowerFailure = FALSE;

        // Set Power Off Data
        powerOffData.Reserved = Pm_Eeprom.appBusyFlags;

        // Set Halt Completed Processing Flag
        powerOffData.bHaltCompleted = Pm_Eeprom.bHaltCompleted;

      } // End of else ( bPowerFailure == FALSE )

    } // Else Pm_Eeprom data good

    // Call system function to create Power Off Log Here !
    // Note: Changed original switch statement to if/else to remove errors
    //       related to not checking every possible enumerated value!
    if (SYS_ID_PM_POWER_FAILURE_LOG == pmInternalLog.Type)
    {
       // NOTE: There are two Power Failure types that can occur
       //   1) System determines there was a power failure and attempt to HALT
       //      (See if Pm_Eeprom.bPowerFailure == TRUE above)
       //   2) System did not determine there was a power failure and
       //      system abruptly halts (See else case of
       //                             Pm_Eeprom.bPowerFailure == TRUE above)

       // Determine Power Failure with or with out BATTERY
       if ( pmInternalLog.bBattery != TRUE )
       {
          // Power Failure with out BATTERY
          pmInternalLog.Type = SYS_ID_PM_POWER_FAILURE_NO_BATTERY_LOG;
       }
       powerFailureData.Id = (UINT16)pmInternalLog.Type;
       powerFailureData.ts = pmInternalLog.ts;
       powerFailureData.State = pmInternalLog.State;
       powerFailureData.bHaltCompleted = pmInternalLog.bHaltCompleted;
       ts = powerFailureData.ts;
       LogWriteSystem( (SYS_APP_ID) powerFailureData.Id, LOG_PRIORITY_LOW,
                       &powerFailureData, sizeof(PM_LOG_DATA_POWER_FAILURE),
                       &ts );
    }
    else if (SYS_ID_PM_POWER_OFF_LOG == pmInternalLog.Type)
    {
       LogWriteSystem(SYS_ID_PM_POWER_OFF_LOG, LOG_PRIORITY_LOW, &powerOffData,
                      sizeof(PM_LOG_DATA_POWER_OFF), &pmInternalLog.ts);
    }
    else if (SYS_ID_PM_RESTORE_DATA_FAILED == pmInternalLog.Type)
    {
       LogWriteSystem(SYS_ID_PM_RESTORE_DATA_FAILED, LOG_PRIORITY_LOW,
                      &powerRestoreFailedData,  sizeof(PM_LOG_DATA_RESTORE_DATA_FAILED),
                      &pmInternalLog.ts);
    }
    else
    {
       FATAL("PowerManagerInit: Invalid SYS_APP_ID = %d", pmInternalLog.Type);
    }

    // Update new PM_EEPROM_DATA Here
    PmFileInit();

    // Create Power On Log New !
    pmInternalLog.Type = SYS_ID_PM_POWER_ON_LOG;
    // Force Power On Time to current StateTrans_Time
    pmInternalLog.ts = Pm_Eeprom.StateTrans_Time;
    pmInternalLog.bPowerFailure = FALSE;
    pmInternalLog.State = PM_NORMAL;
    pmInternalLog.bBattery = Pm_Eeprom.bBattery;

    // Create Power On Log Old !
    LogWriteSystem(SYS_ID_PM_POWER_ON_LOG, LOG_PRIORITY_LOW, 0, 0, &pmInternalLog.ts);


    // Set the PM Initialize complete flag
    Pm.nInitCompleted       = PM_INIT_COMPLETE;    // Flag to indicate to ISR that
    Pm.nInitCompletedInv    = ~PM_INIT_COMPLETE;   //   initialization has been completed.

    Pm.LowPwrInt = 0;
    Pm.LowPwrIntAck = 0;
    Pm.NumAppShutDownNormal = 0;

    for (i = 0; i < PM_MAX_SHUTDOWN_APP; i++)
    {
      Pm.AppShutDownQuick[i] = (void *) 0;
      Pm.AppShutDownNormal[i] = (void *) 0;
    }

    //
    // Initialize EPORT's IRQ7 port
    // Note: On Startup, EPORT EPIER disabled on reset, thus no IRQ7 from Low Power
    //       Int will reach the ColdFire's internal interrupt controller, until the
    //       EPPORT is configured.
    //
    //
    // Set EPPAR EPORT Pin Assignment Register IRQ7 to falling edge IRQ
    // Note: Edge trigger IRQ are latched (external INT source need not remain
    //       asserted) in the EPFR Edge Port Flag Register and the EPFR reg must
    //       be cleared (acknowledged) before the EPORT will generate another INT.
    //       If not cleared any additional INT from the external source will be
    //       ignored.
    //
    MCF_EPORT_EPPAR |= MCF_EPORT_EPPAR_EPPA7(MCF_EPORT_EPPAR_EPPAx_FALLING);

    // Set EPDDR EPORT Data Direction Register IRQ7 to Input and preserve other
    //   bit settings.
    // Note: On Reset, EPDDR initialized to 0 already.
    MCF_EPORT_EPDDR &= ~MCF_EPORT_EPDDR_EPDD7;

    // Unmask INT Level 7 from IMR.  ("0")
    // Note: The Level 7 of the interrupt controller can not be masked, however,
    //       the IMR might be able to mask level 7 before going into the
    //       interrupt controller, thus the IRQ7 input from the EPORT can be
    //       masked here !
    // Note: On reset, IMR bit 0 is set, thus disabling all INT (forces all other
    //       bits to be interrupted as "1", although on reset all IMR bits 1 to 63
    //       are physically set as well).
    // Note: _MASKALL must be cleared to have individual access to all other
    //       mask bits.
    MCF_INTC_IMRL &= ~(MCF_INTC_IMRL_INT_MASK7|MCF_INTC_IMRL_MASKALL);

    // Clear the Acknowledge for the external interrupt before enabling the EPORT INT.
    // Acknowledge the EPORT Flag Register for the external interrupt.
    MCF_EPORT_EPFR = MCF_EPORT_EPFR_EPF7;

    // Set the EPORT EPIER Interrupt Enable for IRQ7
    MCF_EPORT_EPIER |= MCF_EPORT_EPIER_EPIE7;   //Interrupt enable


    // Test Only !!!!  ***************
    // PmSetCallBack_AppBusy( &AppBusy );
    // PmSetCallBack_CfgBattery ( &CfgBattery );
    /*
    PmInsertAppShutDownQuick( testAppShutDownQuick_0 );
    PmInsertAppShutDownQuick( testAppShutDownQuick_1 );
    PmInsertAppShutDownQuick( testAppShutDownQuick_2 );

    PmInsertAppShutDownNormal( testAppShutDownNormal_0 );
    PmInsertAppShutDownNormal( testAppShutDownNormal_1 );
    PmInsertAppShutDownNormal( testAppShutDownNormal_2 );
    */
    // Test Only !!!   ***************


   //Add an entry in the user message handler table
   User_AddRootCmd(&PowerManagerTblPtr);

   // Initial Read of Bus and Battery Voltages
   result = ADC_GetACBusVoltage (&Pm.BusVoltage);        // Returns 0 if OK
   result = ADC_GetACBattVoltage (&Pm.BatteryVoltage);   // Returns 0 if OK

   result = ADC_GetLiBattVoltage(&Pm.LithiumBattery);
   result = ADC_GetBoardTemp(&Pm.InternalTemp);

}   // End of InitializePowerManager()


/******************************************************************************
 * Function:    PowerFailIsr - IRQ7 ISR
 *
 * Description: Process Power Fail exception via IRQ7 (NMI).
 *
 * Parameters:  void
 *
 * Returns:     void
 *
 * Notes:
 *
 *   1) IRQ7 is both level triggered (Low level) and edge triggered
 *      requiring a falling edge to cause the interrupt. This
 *      prevents multiple interrupts due to the low level /PFI signal
 *      from the MAX800L chip.
 *
 *   2) Low Power Int triggers when 28 V Batt goes below 10.8 V
 *
 *   3) Although level 7 into the Coldfire's interrupt controller is
 *      non-maskable, the IRQ7 pin must pass thru the EPORT and IMR before
 *      reaching the interrupt controller.  On reset both EPORT and IMR
 *      will "disable" the IRQ7 pin into the interrupt controller.
 *      (Pin IRQ7 masked on startup)
 *
 *****************************************************************************/
__interrupt void PowerFailIsr (void)
{
    // Process Low Power Interrupt only if Pm initialization has been completed.
    if ( (Pm.nInitCompleted == (~Pm.nInitCompletedInv)) &&
         (PM_INIT_COMPLETE == Pm.nInitCompleted ) )
    {
      // Note: LowPwrInt cleared in PmTask() at approx 10 msec (dep on Task Mgr)
      if ( Pm.LowPwrInt == Pm.LowPwrIntAck )
      {
        PmProcessPowerFailure();
        ++Pm.LowPwrInt;
      }
    }
    else
    {
      // Interrupt Occurred during startup.  We should just HALT and reset to
      //   allow power to be more stable before operating.
      WatchdogReboot( TRUE);
    }

    // Acknowledge the EPORT Flag Register for the external interrupt.
    // If we do not clear this flag, we get continuous interrupts !!!
    MCF_EPORT_EPFR = MCF_EPORT_EPFR_EPF7;

}   // End PowerFailIsr()


/******************************************************************************
 * Function:     PmInsertAppShutDownNormal
 *
 * Description:  Register an application function to call on normal shutdown
 *
 * Parameters:   APPSHUTDOWN_FUNC func
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
void PmInsertAppShutDownNormal( APPSHUTDOWN_FUNC func )
{
  ASSERT_MESSAGE(Pm.NumAppShutDownNormal < PM_MAX_SHUTDOWN_APP,
                 "NumAppShutDownNormal=%d", Pm.NumAppShutDownNormal);

  Pm.AppShutDownNormal[Pm.NumAppShutDownNormal++] = func;
}


/******************************************************************************
 * Function:     PmInsertAppBusInterrupt
 *
 * Description:  Register an application function to call on Bus Interrupt.
 *
 * Parameters:   APPSHUTDOWN_FUNC func
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
void PmInsertAppBusInterrupt( APPSHUTDOWN_FUNC func )
{
  ASSERT_MESSAGE( Pm.NumAppBusInterrupt < PM_MAX_SHUTDOWN_APP,
                  "NumAppBusInterrupt=%d", Pm.NumAppBusInterrupt);

  Pm.AppBusInterrupt[Pm.NumAppBusInterrupt++] = func;
}

/******************************************************************************
 * Function:     PmRegisterAppBusyFlag
 *
 * Description:  Register an application busy flag for the associated variable
 *               and a indication that the busy flag's use should be locked
 *               even when FSM is active
 *
 * Parameters:   busyIndex - BUSY_INDEX
 *               pAppBusyFlag - BOOLEAN*
 *               usage       - PM_BUSY_USAGE
 *
 * Returns:      none
 *
 * Notes:        none
 *
 *****************************************************************************/
void PmRegisterAppBusyFlag(PM_BUSY_INDEX busyIndex, BOOLEAN *pAppBusyFlag, PM_BUSY_USAGE usage)
{
  ASSERT(busyIndex < PM_MAX_BUSY);
  m_latchCtrl.task[busyIndex].pBusyFlag = pAppBusyFlag;
  m_latchCtrl.task[busyIndex].usage     = usage;
}



/******************************************************************************
 * Function:    PmFSMAppBusyRun   | IMPLEMENTS Run() INTERFACE to
 *                                | FAST STATE MGR
 *
 * Description: Signals the PowerManager to set or clear the App Busy
 *
 * Parameters:  Run =  TRUE  Set the application flag busy
 *                     FALSE Set the application flag not busy
 *              param = ignored, included to match FSM call sig.
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
void PmFSMAppBusyRun(BOOLEAN Run, INT32 param)
{
  m_FSMAppBusy = Run;
}



/******************************************************************************
 * Function:    PmFSMAppBusyGetState   | IMPLEMENTS GetState() INTERFACE to
 *                                     | FAST STATE MGR
 *
 * Description: Returns the state of the pm AppBusy flag set/cleared by
 *
 *
 * Parameters:  param = ignored.  Included to match FSM call sig.
 *
 * Returns:     BOOLEAN - m_FSMAppBusy
 *
 * Notes:
 *
 *****************************************************************************/
BOOLEAN PmFSMAppBusyGetState(INT32 param)
{
  return m_FSMAppBusy;
}



/******************************************************************************
 * Function:     PmSetPowerOnTime
 *
 * Description:  Call back to set the Pm.battLatchTime Ptr.  Points to an
 *               application variable
 *
 * Parameters:   pPtr = Ptr to application variable
 *
 * Returns:      none
 *
 * Notes:        none
 *
 *****************************************************************************/
void PmSetPowerOnTime( TIMESTAMP *pPtr )
{
  Pm.PowerOnTime = *pPtr;
}


/******************************************************************************
 * Function:     PmFileInit
 *
 * Description:  Reset the file to the initial state
 *
 * Parameters:   [in]  none
 *               [out] none
 *
 * Returns:      BOOLEAN TRUE  The file reset was successful.
 *                       FALSE The file reset failed.
 *
 * Notes:        None
 *
 *****************************************************************************/
BOOLEAN PmFileInit(void)
{
  Pm_Eeprom.State = PM_NORMAL;

  // ASSERT if PmPowerOnTime->Timestamp != 0 !!!
  // CM_GetTimeAsTimestamp( &Pm_Eeprom.StateTrans_Time );
  // Note: Pm.PowerOnTime set during Drv Initialization().
  Pm_Eeprom.StateTrans_Time = Pm.PowerOnTime;

  Pm_Eeprom.bPowerFailure = FALSE;
  Pm_Eeprom.bBattery = Pm.Battery;
  Pm_Eeprom.bHaltCompleted = FALSE;
  NV_Write( NV_PWR_MGR, 0, (void *) &Pm_Eeprom, sizeof(PM_EEPROM_DATA) );
  return TRUE;
}


/******************************************************************************
 * Function:     PmCreateAllInternalLogs
 *
 * Description:  Creates all PM internal logs for testing / debug of log
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

void PmCreateAllInternalLogs ( void )
{
  // 1 Create SYS_ID_PM_POWER_FAILURE_LOG
  LogWriteSystem( SYS_ID_PM_POWER_FAILURE_LOG, LOG_PRIORITY_LOW,
                  &SysPmPowerFailure, sizeof(PM_LOG_DATA_POWER_FAILURE),
                  NULL );

  // 2 Create SYS_ID_PM_POWER_FAILURE_NO_BATTERY_LOG
  LogWriteSystem( SYS_ID_PM_POWER_FAILURE_NO_BATTERY_LOG, LOG_PRIORITY_LOW,
                  &SysPmPowerFailure, sizeof(PM_LOG_DATA_POWER_FAILURE),
                  NULL );

  // 3 Create SYS_ID_PM_POWER_OFF_LOG
  LogWriteSystem( SYS_ID_PM_POWER_OFF_LOG, LOG_PRIORITY_LOW, &SysPmPowerOff,
                  sizeof(PM_LOG_DATA_POWER_OFF), NULL );


  // 4 Create SYS_ID_PM_RESTORE_DATA_FAILED
  LogWriteSystem( SYS_ID_PM_RESTORE_DATA_FAILED, LOG_PRIORITY_LOW,
                  &SysPmRestoreDataFailed,  sizeof(PM_LOG_DATA_RESTORE_DATA_FAILED),
                  NULL );

  // 5 Create SYS_ID_PM_FORCED_SHUTDOWN_LOG
  LogWriteSystem( SYS_ID_PM_FORCED_SHUTDOWN_LOG, LOG_PRIORITY_LOW,
                  &SysPmForceShutDown, sizeof(PM_LOG_DATA_FORCE_SHUTDOWN),
                  NULL);

  // 6 Create SYS_ID_PM_POWER_ON_LOG
  LogWriteSystem(SYS_ID_PM_POWER_ON_LOG, LOG_PRIORITY_LOW, 0, 0, NULL);

  // 7 Create SYS_ID_PM_POWER_INTERRUPT_LOG
  LogWriteSystem(SYS_ID_PM_POWER_INTERRUPT_LOG, LOG_PRIORITY_LOW, 0, 0, NULL);

  // 8 Create SYS_ID_PM_POWER_RECONNECT_LOG
  LogWriteSystem(SYS_ID_PM_POWER_RECONNECT_LOG, LOG_PRIORITY_LOW, 0, 0, NULL);
}

/*vcast_dont_instrument_end*/

#endif

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:    PmTask
 *
 * Description: Monitor the aircraft Battery voltage and process failures of
 *              the aircraft Bus or Battery power supplies.
 *
 * Parameters:  void* pPBlock
 *
 * Returns:     void
 *
 * Notes:       Expect PmTask to setup as a DT Task running at 10 msec (MIF rate)
 *
 *****************************************************************************/
static
void PmTask (void* pPBlock)
{
   // Local Data
   BOOLEAN bResult;

   // Refresh the application busy status
    Pm.AppBusy = PmUpdateAppBusyStatus();
    // Update the Halt completed bool here so it's available
    Pm_Eeprom.bHaltCompleted = !Pm.AppBusy;

    switch (Pm.State)
    {
    case PM_NORMAL:
        // Not necessary to monitor bus or battery voltages.
        // Wait until Low Power Interrupt Trips to transition to PM_GLITCH
        break;

    case PM_GLITCH:
        PmGlitchProcess();
        break;

    case PM_BATTERY:
        PmBatteryProcess();
        break;

    case PM_SHUTDOWN:
        // First time here.
        // Signal taskmgr that we need to shutdown
        // Call the shutdown callbacks.
        bResult = TRUE;
        if ( Pm.PmStateTimer == 0 )
        {
          GSE_DebugStr(NORMAL,TRUE,"PM Shutdown\r\n");
          TmSetMode(SYS_SHUTDOWN_ID);
          bResult = PmCallAppShutDownNormal();
        }

        // Check timer and transition to halt if expired.
        if ( (bResult == TRUE) && (Pm.PmStateTimer++ > PM_SHUTDOWN_TIMER_EXPIRE) )
        {
          Pm.State = PM_HALT;
          Pm.PmStateTimer = 0;

          // Update Pm_Eeprom values

          Pm_Eeprom.State = Pm.State;
          CM_GetTimeAsTimestamp( &Pm_Eeprom.StateTrans_Time );
          
          // Don't need to update .bBattery should be set on init !
          // bPowerFailure should be set above
          // bHaltCompleted should be FALSE         

          // Clear State Timer for next state usage.
          Pm.PmStateTimer = 0;

          GSE_DebugStr(NORMAL,TRUE,"PM Halt\r\n");
        }
        break;

    case PM_HALT:
        // One last kick of watchdog.
        KICKDOG;

        // Disable all INT as we are shutting down / halting
        MCF_INTC_IMRL = MCF_INTC_IMRL_MASKALL;

        // HALT processing completed, update state transition information in EEPROM
        CM_GetTimeAsTimestamp( &Pm_Eeprom.StateTrans_Time );
        // .State should be PM_HALT
        // Don't need to update .bBattery should be set on init !
        // bPowerFailure should be set above
        // bHaltCompleted is set on entry to function.
        
        NV_WriteNow( NV_PWR_MGR, 0, (void *) &Pm_Eeprom, sizeof(PM_EEPROM_DATA) );

        // Update Box Power On Usage before shutting Down !
        Box_PowerOn_UpdateElapsedUsage ( TRUE, POWERCOUNT_UPDATE_NOW, NV_PWR_ON_CNTS_RTC );
        Box_PowerOn_UpdateElapsedUsage ( TRUE, POWERCOUNT_UPDATE_NOW, NV_PWR_ON_CNTS_EE );

        UNLATCH_BATTERY;         // Disconnect the battery if we are latched to it
                                 //   We are only latched if *Pm.Battery == TRUE

        GSE_DebugStr(NORMAL,TRUE,"PM Halt\r\n");
        // Wait forever until power to CPU goes away
        WatchdogReboot(TRUE);
#ifdef WIN32
/*vcast_dont_instrument_start*/
        break;
/*vcast_dont_instrument_end*/
#endif
        //lint -fallthrough
    case PM_MAX_STATE:
    default:
        FATAL("Invalid PM State(%d)", Pm.State);
        break;

    }   // End switch (Pm.State)

    return;

}   // End PmTask()

/******************************************************************************
 * Function:    PmGlitchProcess
 *
 * Description: Handles the glitch processing for the PmTask.
 *
 * Parameters:  void
 *
 * Returns:     void
 *
 * Notes:       None
 *
 *****************************************************************************/
static
void PmGlitchProcess ( void )
{
   // Local Data
   RESULT  result;

   // On First entry, print debug message and record the transition state
   if ( Pm.PmStateTimer == 0 )
   {
      // SCR #342
      LATCH_BATTERY;

      GSE_DebugStr(NORMAL,TRUE,"PM Glitch\r\n");

      // Log SYS_ID_PM_POWER_INTERRUPT_LOG
      LogWriteSystem(SYS_ID_PM_POWER_INTERRUPT_LOG, LOG_PRIORITY_LOW, 0, 0, NULL);

      // Need to record the state transition here as the detect of PM_GLITCH is thru ISR
      //        and we don't wan that extra processing in the ISR.  Put processing here.
      Pm_Eeprom.State = Pm.State; // Current State which should be PM_GLITCH
      CM_GetTimeAsTimestamp( &Pm_Eeprom.StateTrans_Time );
      // .bPowerFailure should be FALSE at this point, set on init.
      // .bBattery init to *App.Battery
      // .bHaltCompleted should be FALSE at this point, set on init.
      NV_Write( NV_PWR_MGR, 0, (void *) &Pm_Eeprom, sizeof(PM_EEPROM_DATA) );

// Test
      PmCallAppBusInterrupt();
// Test
   }

   if (Pm.LowPwrInt != Pm.LowPwrIntAck)
   {
      Pm.LowPwrIntAck = Pm.LowPwrInt;    // set interrupt acknowledged flag
   }

   // Read Battery and Bus Voltages
   result = ADC_GetACBusVoltage (&Pm.BusVoltage);        // Returns 0 if OK
   result |= ADC_GetACBattVoltage (&Pm.BatteryVoltage);  // Returns 0 if OK

   // only check voltages after first glitch read to ensure valid values
   if ( ( Pm.PmStateTimer != 0 ) && (result == DRV_OK) &&
         ( (Pm.BusVoltage > BUS_VOLTAGE_THRESHOLD) &&
           (Pm.BatteryVoltage > BUS_VOLTAGE_THRESHOLD ) ) )
   {
      // Voltages have "de-glitched". Resume normal operation.
      // Log SYS_ID_PM_POWER_RECONNECT_LOG
      LogWriteSystem(SYS_ID_PM_POWER_RECONNECT_LOG, LOG_PRIORITY_LOW, 0, 0, NULL);
      GSE_DebugStr(NORMAL,TRUE,"PM Normal\r\n");
      Pm.State = PM_NORMAL;
      UNLATCH_BATTERY;   // Might not be necessary, but added for extra safety.
   }
   else // Power Still below minimums
   {
      // Assume that ADC_GetVoltage() return Busy (result != 0) does not occur often.
      // Will handle this case as though < BUS_VOLTAGE_THRESHOLD, even though
      // it might not be.  On next read, assume that Busy should go away as this
      // condition is not expected to occur often.
      // Averaging of the Voltage not performed.
      if (++Pm.PmStateTimer > PM_GLITCH_TIMER_EXPIRE)
      {
         // "Glitch" period exceeded, bus power has definitely been removed!
         if (Pm.BatteryVoltage < BUS_VOLTAGE_THRESHOLD)
         {
            // Glitch Timer Expires and Batt Volt still bad !
            // Bus Power Failure !
            // Don't need to re-read Batt and Bus Voltages
            Pm_Eeprom.bPowerFailure = TRUE;
            Pm.State = PM_HALT;
         }
         else
         {
            // BatteryVoltage > BUS_VOLTAGE_THRESHOLD, but Bus Voltage still
            // bad which should indicate "master power" switch OFF.
            // Determine if we should continue or shutdown.

            if ( (( Pm.AppBusy == TRUE ) || ( Pm.bLatchToBattery == TRUE )) &&
                  ( Pm.Battery == TRUE ) )
            {
               // Latch to Battery Operation.
               // NOTE: If BatteryVoltage > BUS_VOLTAGE_THRESHOLD, then
               //       power is good and latch-to-battery (PWR_HOLD)
               //       is holding up Battery Voltage.
               LATCH_BATTERY;
               Pm.State = PM_BATTERY;
               Pm.PmBattDeglitchTimer = 0;
            }
            else
            {
               // At this point we can shut down normally
               Pm.State = PM_SHUTDOWN;
            }
         } // End of else BatteryVoltage !< BUS_VOLTAGE_THRESHOLD

      } // End if (PmGlitchTimer > PM_GLITCH_TIMER_EXPIRE) && (Batt < 10.8)
      else
      {
         // Still "De-glitching".  Do Nothing.
      }
   } // End Else Power still De-Glitching !

   // If we have changed PM state then update Pm_Eeprom !
   if (Pm.State != PM_GLITCH)
   {
      Pm_Eeprom.State = Pm.State; // New state we are transition to.
      CM_GetTimeAsTimestamp( &Pm_Eeprom.StateTrans_Time );
      // .bPowerFailure should have been set in the above code
      // .bBattery set to *App.Battery on init
      // .bHaltCompleted should be FALSE
      NV_Write( NV_PWR_MGR, 0, (void *) &Pm_Eeprom, sizeof(PM_EEPROM_DATA) );
      Pm.PmStateTimer = 0;
   } // End if != PM_GLITCH
}

/******************************************************************************
 * Function:    PmBatteryProcess
 *
 * Description: Handles the battery processing for the PmTask.
 *              Continue operation under latched battery power.
 *              Monitor the Bus voltage. If the Bus voltage has been restored,
 *              restore normal operation. Also provides logic for 50 msec
 *              de-glitch while in BATTERY mode
 *
 * Parameters:  void
 *
 * Returns:     void
 *
 * Notes:       None
 *
 *****************************************************************************/
static
void PmBatteryProcess ( void )
{
   // Local Data
   PM_LOG_DATA_FORCE_SHUTDOWN powerForceShutDownData;
   RESULT result;

   // Continue operation under latched battery power.
   // Monitor the Bus voltage. If the Bus voltage has been restored,
   // restore normal operation.
   // Also provide logic for 50 msec de-glitch while in BATTERY mode

   // On First Entry, output debug state transition message
   if ( Pm.PmStateTimer == 0 )
   {
      GSE_DebugStr(NORMAL,TRUE,"PM Battery\r\n");
   }

   // bUpdate_Pm_Eeprom = FALSE;
   // If we experienced another Low Power Interrupt while in BATTERY state
   //  provide de-glitch of 50 msec before indicating Power Failure !
   if (Pm.LowPwrInt != Pm.LowPwrIntAck)
   {
      Pm.LowPwrIntAck = Pm.LowPwrInt;    // set interrupt acknowledged flag

      if (Pm.PmBattDeglitchTimer == 0)
      {
         ++Pm.PmBattDeglitchTimer;

         // Log SYS_ID_PM_POWER_INTERRUPT_LOG while in PM_BATTERY
         LogWriteSystem(SYS_ID_PM_POWER_INTERRUPT_LOG, LOG_PRIORITY_LOW, 0, 0, NULL);

         // Update EEPROM data to update the StateTrans_Time so if the power
         // actually fails, StateTrans_Time will be as close to the power fail time !
         // Note: NvMgr has higher priority then LogMgr thus the EEPROM data should be
         //       updated before the LogWriteSystem()
         CM_GetTimeAsTimestamp( &Pm_Eeprom.StateTrans_Time );

         // .state should still be BATTERY and need not be updated.
         // .bPowerFailure should have been set in the above code
         // Don't need to update .bBattery
         // .bHaltCompleted should be FALSE
         NV_Write(NV_PWR_MGR, 0, (void *) &Pm_Eeprom, sizeof(PM_EEPROM_DATA) );
      }
   }

   if (Pm.PmBattDeglitchTimer != 0)
   {
      result = ADC_GetACBattVoltage(&Pm.BatteryVoltage);
      if ( (Pm.BatteryVoltage > BUS_VOLTAGE_THRESHOLD ) &&
            (result == DRV_OK) )
      {
         // Voltage De-Glitched and OK.  Stay in PM_BATTERY
         Pm.PmBattDeglitchTimer = 0;
         // Do not need log to indicate power (while in BATTERY) is ok again.
         // If not ok then after 50 msec the system will trans to HALT.
      }
      else
      {
         if (++Pm.PmBattDeglitchTimer > PM_GLITCH_TIMER_EXPIRE)
         {
            Pm_Eeprom.bPowerFailure = TRUE;
            Pm.State = PM_HALT;
         }
      }
   } // End if Pm.PmBattDeglitchTimer != 0


   // Has system processing finished ?
   // Test Case
   if ( (Pm.AppBusy == FALSE) && (Pm.bLatchToBattery == FALSE) )
   {

      // Application Not Busy anymore.  Transition to Normal shutdown.
      // If power has been restored since the last time we ran (10ms ago)
      // .. the system will do a power-cycle with via WD reset
      Pm.State = PM_SHUTDOWN;

   } // End if AppBusy == FALSE
   else // AppBusy still TRUE
   {
      // Check if battLatchTime operation has expires,  force shut down
      //  Default is 1 hour of battery latch time.
      if (++Pm.PmStateTimer > Pm.battLatchTime )
      {
         // Log SYS_ID_PM_FORCED_SHUTDOWN_LOG
         powerForceShutDownData.Reserved = Pm_Eeprom.appBusyFlags;
         LogWriteSystem( SYS_ID_PM_FORCED_SHUTDOWN_LOG, LOG_PRIORITY_LOW,
                         &powerForceShutDownData, sizeof(PM_LOG_DATA_FORCE_SHUTDOWN),
                         NULL);
         GSE_DebugStr(NORMAL,TRUE,"PM Force ShutDown !\r\n");
         Pm.State = PM_SHUTDOWN;

      } // End if PM_BATTERY_2HR_PERIOD expires
      else // Continue in PM_BATTERY mode
      {
         // Check Bus Voltage to determine if Bus Reconnected.
         if ( ( Pm.PmStateTimer % PM_BATTERY_CHECK_BUS_PERIOD) == 0 )
         {
            // Check once a second the Bus Volt.
            // Not necessary to check at faster rate !
            result = ADC_GetACBusVoltage (&Pm.BusVoltage);
            if ( (Pm.BusVoltage > BUS_VOLTAGE_THRESHOLD) &&
                  (result == DRV_OK) )
            {
               Pm.State = PM_NORMAL;
            } // End if BUS_VOLTAGE_THRESHOLD exceeded again.  Trans to PM_NORMAL
         } // End if PM_BATTERY_CHECK_BUS_PERIOD expires
      } // End else PM_BATTERY_2HR_PERIOD not exceeded yet !
   } // End else AppBusy still TRUE


   // If we have changed PM state then update Pm_Eeprom
   // if ( ( Pm.State != PM_BATTERY ) || (bUpdate_Pm_Eeprom == TRUE) )
   if ( Pm.State != PM_BATTERY )
   {
      Pm_Eeprom.State = Pm.State;  // New state we are transition to
      CM_GetTimeAsTimestamp( &Pm_Eeprom.StateTrans_Time );
      // Don't need to update .bBattery should be set on init !
      // bPowerFailure should be set above
      // bHaltCompleted should be FALSE
      NV_Write( NV_PWR_MGR, 0, (void *) &Pm_Eeprom, sizeof(PM_EEPROM_DATA) );

      // Clear State Timer for next state usage.
      Pm.PmStateTimer = 0;

      // If transitions back to PM_NORMAL write Re-Connect log and unlatch from battery
      if ( Pm.State == PM_NORMAL )
      {
         // Log SYS_ID_PM_POWER_RECONNECT_LOG
         LogWriteSystem(SYS_ID_PM_POWER_RECONNECT_LOG, LOG_PRIORITY_LOW, 0, 0, NULL);
         GSE_DebugStr(NORMAL,TRUE,"PM Normal\r\n");
         UNLATCH_BATTERY;
      }
   }
}

/******************************************************************************
 * Function:     PmProcessPowerFailure
 *
 * Description:  Process Battery Power Low Interrupt
 *
 * Parameters:   none
 *
 * Returns:      none
 *
 * Notes:        none
 *
 * 1) Must always force latch to battery.  Battery voltage measure
 *      is after the "enable transistor" and not direct measure of battery
 *      voltage. We must "enable transistor" to read battery voltage
 *      to determine BATTERY failure while in PM_GLITCH mode.
 *
 *****************************************************************************/
static
void PmProcessPowerFailure (void)
{
    if ( Pm.State == PM_NORMAL )
    {
      Pm.State = PM_GLITCH;

      // Must always force latch to battery.  Battery voltage measure
      //   is after the "enable transistor" and not direct measure of battery
      //   voltage. We must "enable transistor" to read battery voltage
      //   to determine BATTERY failure while in PM_GLITCH mode.

      // SCR #428 Need to force Power Hold Ctrl here to reliably latch to battery
      //          when the "super caps" are not connected.
      LATCH_BATTERY;
    }
    else if ( (Pm.State == PM_SHUTDOWN) || (Pm.State == PM_HALT) )
    {
      // This could just be a glitch, but we're shutting down anyway,
      // so assume power loss is imminent and immediately halt
      UNLATCH_BATTERY;         // Disconnect the battery
      // Wait forever until power to CPU goes away
      WatchdogReboot(TRUE);
    }

}   // End PmProcessPowerFailure()


/******************************************************************************
* Function:     PmUpdateAppBusyStatus
*
* Description:  Function to check the busy state of the registered tasks
*
* Parameters:   none
*
* Returns:      TRUE if any task/activity is busy, otherwise FALSE
*
* Notes:        none
*
*****************************************************************************/
static
BOOLEAN PmUpdateAppBusyStatus(void)
{
  INT32 busyCount = 0;
  INT32 prevCount = 0;
  INT32 i;
  PM_BUSY_USAGE execMode;
  BOOLEAN bFlagInUse;
  UINT16  appBusyFlags;
  
  // Set the current exec mode ( FSM or Legacy) based on the latch control flag.
  execMode = m_latchCtrl.bFsmActive ? PM_BUSY_FSM : PM_BUSY_LEGACY;

  // Reset the packed busy flags, set bit 15 if FSM is enabled.
  appBusyFlags = (PM_BUSY_FSM == execMode) ? 0x8000 : 0x0000;  

  // check the de-referenced ptr for each registered entry and increment total IFF the system
  // is in the correct usage mode for the associated task and the flag says busy.
  for(i = 0; i < (INT32)PM_MAX_BUSY; ++i)
  {
    // Determine if the busy flag is valid/usable in the current exec mode
    bFlagInUse = ( (m_latchCtrl.task[i].usage == execMode) ||
                   (m_latchCtrl.task[i].usage == PM_BUSY_ALL) )
                   ? TRUE : FALSE;

    // if the busy flag for the task is applicable for current exec mode
    // AND the flag is registered AND contains a true value, increment the busy count. 
    prevCount = busyCount;
    busyCount += ( bFlagInUse                              &&
                   m_latchCtrl.task[i].pBusyFlag  != NULL  &&
                   *m_latchCtrl.task[i].pBusyFlag == TRUE)
                   ? 1:0;

    // if a busyCount was updated, set the corresponding state bit in the busyflag word
    if ( prevCount < busyCount )
    {
      appBusyFlags = appBusyFlags | (0x0001 << (i));
    }
  }

  Pm_Eeprom.appBusyFlags = appBusyFlags;
  // If some task is busy return true.  
  return (busyCount > 0);
}


/******************************************************************************
 * Function:     PmCallAppBusInterrupt
 *
 * Description:  Process applications when bus power interrupt is detected.
 *               This box should reliably detect bus power interrupt. If power
 *               is shutting down, then after the 50 msec, we might not have
 *               enough time to perform all ShutDownNormal or ShutDownQuick()
 *               processing.
 *
 * Parameters:   None
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static
void PmCallAppBusInterrupt( void )
{
  BOOLEAN bResult;
  UINT32  i;
  APPSHUTDOWN_FUNC func;

  bResult = TRUE;
  for (i = 0; i < Pm.NumAppBusInterrupt; i++)
  {
    // Loop thru all Bus Interrupt routines and only when all report TRUE will
    //  AppBusInterrupt() processing considered complete.
    func = Pm.AppBusInterrupt[i];
    bResult &= func(PM_APPSHUTDOWN_BUS_INTR);
  }
}


/******************************************************************************
 * Function:     PmCallAppShutDownNormal
 *
 * Description:  Process all Normal Shut Down Applications registered with the
 *               Pm Process
 *
 * Parameters:   none
 *
 * Returns:      TRUE if all registered applications processing returns TRUE
 *
 * Notes:
 *
 *****************************************************************************/
static
BOOLEAN PmCallAppShutDownNormal( void )
{
  BOOLEAN bResult;
  UINT32  i;
  APPSHUTDOWN_FUNC func;

  bResult = TRUE;
  for ( i = 0; i < Pm.NumAppShutDownNormal; i++ )
  {
    // Loop thru all App Shut Down Routine and only when all report TRUE will
    //  AppShutDownNormal() processing considered complete.
    func = Pm.AppShutDownNormal[i];
    bResult &= func(PM_APPSHUTDOWN_NORMAL);
  }

  return (bResult);
}

/******************************************************************************
 * Function:     PmSetFsmActive
 *
 * Description:  Lock-out all un-authorized tasks from latching battery
 *               when FSM is configured.
 *
 * Parameters:   None
 *
 * Returns:      None
 *
 * Notes:        This function is called once by FASTStateMgr IFF cfg is avail.
 *
 *****************************************************************************/
void PmSetFsmActive(void)
{
  m_latchCtrl.bFsmActive = TRUE;
}

/******************************************************************************
 * Function:     PmPreInit
 *
 * Description:  Pre-initialization function to clear the latch-control structure
 *               This is needed because System -level tasks will be making calls
 *               before this tasks is initialized.
 *
 * Parameters:   None
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
void PmPreInit(void)
{
  UINT32  i;
  // Init the latch controls
  m_latchCtrl.bFsmActive = FALSE;
  for (i = 0; i < (UINT32)PM_MAX_BUSY; ++i)
  {
    m_latchCtrl.task[i].pBusyFlag = NULL;
    m_latchCtrl.task[i].usage     = PM_BUSY_NONE;
  }   
}
/*************************************************************************
 *  MODIFICATIONS
 *    $History: PowerManager.c $
 * 
 * *****************  Version 71  *****************
 * User: Contractor V&v Date: 2/17/15    Time: 7:40p
 * Updated in $/software/control processor/code/system
 * SCR #1055 - Primary != Back EEPROM added appflag states to PwrMgr file 
 * 
 * *****************  Version 70  *****************
 * User: Contractor V&v Date: 2/12/15    Time: 7:22p
 * Updated in $/software/control processor/code/system
 * SCR #1055 - Primary != Back EEPROM remove write to Pwrmgr  file during
 * shutdown
 * 
 * *****************  Version 69  *****************
 * User: Contractor V&v Date: 2/11/15    Time: 7:40p
 * Updated in $/software/control processor/code/system
 * SCR #1055 - Primary != Back EEPROM Data fix to monitor eeprom busy
 * 
 * *****************  Version 68  *****************
 * User: Contractor V&v Date: 9/03/14    Time: 5:24p
 * Updated in $/software/control processor/code/system
 * SCR #1204 - Legacy App Busy Input still latches battery / CR updates
 *
 * *****************  Version 67  *****************
 * User: Contractor V&v Date: 8/14/14    Time: 4:08p
 * Updated in $/software/control processor/code/system
 * SCR #1234 - Configuration update of certain modules overwrites
 *
 * *****************  Version 66  *****************
 * User: Contractor V&v Date: 8/12/14    Time: 5:10p
 * Updated in $/software/control processor/code/system
 * SCR 1055 EEPROM Backup not updated before SHUTDOWN and SCR 1204 Legacy
 * app busy input still latches battery when FSM enabled
 *
 * *****************  Version 65  *****************
 * User: Jim Mood     Date: 12/11/12   Time: 8:31p
 * Updated in $/software/control processor/code/system
 * SCR #1197 Code Review update
 *
 * *****************  Version 64  *****************
 * User: John Omalley Date: 12-11-15   Time: 10:51a
 * Updated in $/software/control processor/code/system
 * SCR 1076 - Code Review Updates
 *
 * *****************  Version 63  *****************
 * User: John Omalley Date: 12-11-14   Time: 7:20p
 * Updated in $/software/control processor/code/system
 * SCR 1076 - Code Review Updates
 *
 * *****************  Version 62  *****************
 * User: Melanie Jutras Date: 12-11-13   Time: 2:31p
 * Updated in $/software/control processor/code/system
 * SCR #1142 File Format Errors
 *
 * *****************  Version 61  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 60  *****************
 * User: Contractor2  Date: 8/31/11    Time: 4:05p
 * Updated in $/software/control processor/code/system
 * SCR #1056 Error - Unexpected degraded mode
 *
 * *****************  Version 59  *****************
 * User: Jim Mood     Date: 7/20/11    Time: 10:57a
 * Updated in $/software/control processor/code/system
 * SCR 575: GSM Enable when engine status is lost.  (Part of changes for
 * the Fast State Machine)
 *
 * *****************  Version 58  *****************
 * User: Contractor2  Date: 6/15/11    Time: 3:37p
 * Updated in $/software/control processor/code/system
 * SCR #812 Enhancement - Forced Shutdown log, max stack value
 *
 * *****************  Version 57  *****************
 * User: Contractor2  Date: 5/18/11    Time: 11:02a
 * Updated in $/software/control processor/code/system
 * SCR #986 Enhancement: PM - Possiblility of missing a Power interrupt
 *
 * *****************  Version 56  *****************
 * User: Contractor2  Date: 4/29/11    Time: 11:26a
 * Updated in $/software/control processor/code/system
 * SCR #966 Error: PM - Glitch to Normal with no power
 * SCR #808 Error: Degraded Mode - still instances where degraded mode
 * prompt appears
 *
 * *****************  Version 55  *****************
 * User: Jeff Vahue   Date: 10/25/10   Time: 5:41p
 * Updated in $/software/control processor/code/system
 * SCR# 962 - default switch and no check for power
 *
 * *****************  Version 54  *****************
 * User: Contractor2  Date: 10/14/10   Time: 3:26p
 * Updated in $/software/control processor/code/system
 * SCR #934 Code Review: PowerManager
 *
 * *****************  Version 53  *****************
 * User: Contractor2  Date: 10/14/10   Time: 11:49a
 * Updated in $/software/control processor/code/system
 * SCR #934 Code Review: PowerManager
 *
 * *****************  Version 52  *****************
 * User: John Omalley Date: 9/09/10    Time: 2:15p
 * Updated in $/software/control processor/code/system
 * SCR 855 - Removed unused functions
 *
 * *****************  Version 51  *****************
 * User: Peter Lee    Date: 8/27/10    Time: 4:53p
 * Updated in $/software/control processor/code/system
 * SCR #814 KICKDOG one last time before disabling INT.
 *
 * *****************  Version 50  *****************
 * User: Peter Lee    Date: 8/27/10    Time: 3:16p
 * Updated in $/software/control processor/code/system
 * SCR #814 Disable INT on entry into HALT processing rather then at the
 * tend of HALT processing.
 *
 * *****************  Version 49  *****************
 * User: Jeff Vahue   Date: 8/22/10    Time: 3:26p
 * Updated in $/software/control processor/code/system
 * SCR #707 - VectorCast coverage change
 *
 * *****************  Version 48  *****************
 * User: Contractor V&v Date: 7/30/10    Time: 9:37p
 * Updated in $/software/control processor/code/system
 * SCR #282 Misc - Shutdown Processing
 *
 * *****************  Version 47  *****************
 * User: Contractor2  Date: 7/30/10    Time: 7:18p
 * Updated in $/software/control processor/code/system
 * SCR #638 Startup requests for Degraded Mode
 *
 * *****************  Version 46  *****************
 * User: Contractor V&v Date: 7/21/10    Time: 7:19p
 * Updated in $/software/control processor/code/system
 * SCR #538 SPIManager startup & NV Mgr Issues
 *
 * *****************  Version 45  *****************
 * User: Contractor3  Date: 7/16/10    Time: 8:57a
 * Updated in $/software/control processor/code/system
 * SCR #702 - Changes based on code review.
 *
 * *****************  Version 44  *****************
 * User: Jeff Vahue   Date: 6/29/10    Time: 5:32p
 * Updated in $/software/control processor/code/system
 * SCR# 667 - add info to asserts
 *
 * *****************  Version 43  *****************
 * User: Jeff Vahue   Date: 6/18/10    Time: 3:12p
 * Updated in $/software/control processor/code/system
 * SCR# 638 - Halt the system right from the PowerDown ISR if we get
 * interrupts before the system has been initialized.
 *
 * *****************  Version 42  *****************
 * User: Contractor2  Date: 6/14/10    Time: 1:06p
 * Updated in $/software/control processor/code/system
 * SCR #483 Function Names must begin with the CSC it belongs with.
 *
 * *****************  Version 41  *****************
 * User: Jim Mood     Date: 6/11/10    Time: 3:30p
 * Updated in $/software/control processor/code/system
 * SCR 623 Batch configuration implementation updates
 *
 * *****************  Version 40  *****************
 * User: Contractor V&v Date: 6/09/10    Time: 7:22p
 * Updated in $/software/control processor/code/system
 * SCR #614 fast.reset=really should initialize files
 *
 * *****************  Version 39  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:55p
 * Updated in $/software/control processor/code/system
 * SCR #587 Change TmTaskCreate to return void
 *
 * *****************  Version 38  *****************
 * User: Contractor2  Date: 5/06/10    Time: 2:10p
 * Updated in $/software/control processor/code/system
 * SCR #579 Change LogWriteSys to return void
 *
 * *****************  Version 37  *****************
 * User: Contractor2  Date: 4/27/10    Time: 1:37p
 * Updated in $/software/control processor/code/system
 * SCR 187: Degraded mode. Moved watchdog reset loop to WatchDog.c
 *
 * *****************  Version 36  *****************
 * User: Jeff Vahue   Date: 4/12/10    Time: 5:43p
 * Updated in $/software/control processor/code/system
 * SCR #541 - all XXX_CreateLogs functions are now dependent on
 * GENERATE_SYS_LOGS
 *
 * *****************  Version 35  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:11p
 * Updated in $/software/control processor/code/system
 * SCR #317 Implement safe strncpy,SCR #70 Store/Restore interrupt
 *
 * *****************  Version 34  *****************
 * User: Jeff Vahue   Date: 4/05/10    Time: 10:01a
 * Updated in $/software/control processor/code/system
 * SCR #481 - add Vectorcast cmds to eliminate code coverage on code to
 * support WIN32 execution.
 *
 * *****************  Version 33  *****************
 * User: Jeff Vahue   Date: 3/26/10    Time: 5:42p
 * Updated in $/software/control processor/code/system
 * Change Debug String to output time during PM state changes
 *
 * *****************  Version 32  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:36p
 * Updated in $/software/control processor/code/system
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 *
 * *****************  Version 31  *****************
 * User: Contractor V&v Date: 3/19/10    Time: 4:31p
 * Updated in $/software/control processor/code/system
 * SCR #279 Making NV filenames uppercase
 *
 * *****************  Version 30  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/system
 * SCR# 483 - Function Names
 *
 * *****************  Version 29  *****************
 * User: Contractor V&v Date: 3/10/10    Time: 4:42p
 * Updated in $/software/control processor/code/system
 * SCR #67 fix spelling error in funct name
 *
 * *****************  Version 28  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 27  *****************
 * User: Jeff Vahue   Date: 2/17/10    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR# 282 - GH compiler warning
 *
 * *****************  Version 26  *****************
 * User: Jeff Vahue   Date: 2/17/10    Time: 1:27p
 * Updated in $/software/control processor/code/system
 * SCR #282 - start shutdown processing implementation
 *
 * *****************  Version 25  *****************
 * User: Contractor V&v Date: 2/03/10    Time: 2:58p
 * Updated in $/software/control processor/code/system
 * SCR 279
 *
 * *****************  Version 24  *****************
 * User: Jeff Vahue   Date: 1/29/10    Time: 3:31p
 * Updated in $/software/control processor/code/system
 * SCR# 412: Assert checking - change all the Shut down registration
 * functions to use ASSERTs vs. If/then/else
 *
 * *****************  Version 23  *****************
 * User: Jeff Vahue   Date: 1/27/10    Time: 6:51p
 * Updated in $/software/control processor/code/system
 * SCR# 222
 *
 * *****************  Version 22  *****************
 * User: Peter Lee    Date: 1/26/10    Time: 6:46p
 * Updated in $/software/control processor/code/system
 * SCR #428 Force Pwr Hold in Low Power Int Detection for reliable battery
 * latch operations when "Super Caps" not installed or fully charged.
 *
 * *****************  Version 21  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 5:10p
 * Updated in $/software/control processor/code/system
 * SCR# 397
 *
 * *****************  Version 20  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:26p
 * Updated in $/software/control processor/code/system
 * SCR 371
 *
 * *****************  Version 19  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 2:11p
 * Updated in $/software/control processor/code/system
 * SCR# 326
 *
 * *****************  Version 18  *****************
 * User: Jeff Vahue   Date: 12/18/09   Time: 1:35p
 * Updated in $/software/control processor/code/system
 * SCR# 378
 *
 * *****************  Version 17  *****************
 * User: Contractor V&v Date: 11/30/09   Time: 5:41p
 * Updated in $/software/control processor/code/system
 * SCR 106  XXXUserTable.c consistency
 *
 * *****************  Version 16  *****************
 * User: Peter Lee    Date: 11/20/09   Time: 10:25a
 * Updated in $/software/control processor/code/system
 * SCR #342 Move PWR_HOLD assert from ISR Level 7 to avoid False Reg Check
 * Fail indication.
 *
 * *****************  Version 15  *****************
 * User: Peter Lee    Date: 11/19/09   Time: 10:43a
 * Updated in $/software/control processor/code/system
 * SCR #315 Fix bug with maintaining SEU count across power cycle
 *
 * *****************  Version 14  *****************
 * User: Contractor V&v Date: 11/18/09   Time: 4:19p
 * Updated in $/software/control processor/code/system
 * Implement SCR 172 ASSERT processing for all "default" case processing
 *
 * *****************  Version 13  *****************
 * User: Peter Lee    Date: 11/02/09   Time: 1:57p
 * Updated in $/software/control processor/code/system
 * Misc non code update to support WIN32 version and spelling.
 *
 * *****************  Version 12  *****************
 * User: Peter Lee    Date: 10/05/09   Time: 5:57p
 * Updated in $/software/control processor/code/system
 * SCR #219 PM Battery Latch User Cmd
 *
 * *****************  Version 11  *****************
 * User: Peter Lee    Date: 9/29/09    Time: 1:25p
 * Updated in $/software/control processor/code/system
 * SCR #280 Fix Compiler Warnings
 *
 * *****************  Version 10  *****************
 * User: Peter Lee    Date: 9/28/09    Time: 6:26p
 * Updated in $/software/control processor/code/system
 * scr #277 Power Management Requirements Implemented
 *
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 9/28/09    Time: 4:17p
 * Updated in $/software/control processor/code/system
 * SCR #275 Box Power On Usage Requirements
 *
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 9/17/09    Time: 11:40a
 * Updated in $/software/control processor/code/system
 * SCR #64 Sequencing of Power On / Off Logs Recording / TS
 *
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 9/16/09    Time: 10:29a
 * Updated in $/software/control processor/code/system
 * SCR #258 Support fix for CM_GetSystemClock() during system
 * initialization.  Call NvWriteNow() during InitializePowerManager().
 *
 * *****************  Version 6  *****************
 * User: Peter Lee    Date: 6/30/09    Time: 3:45p
 * Updated in $/software/control processor/code/system
 * SCR #204 Misc Code Review Updates
 *
 * *****************  Version 5  *****************
 * User: Peter Lee    Date: 6/08/09    Time: 11:41a
 * Updated in $/software/control processor/code/system
 * Integrate NvMgr() to store PM shutdown state.  Complete system log
 * generation.
 *
 * *****************  Version 4  *****************
 * User: Peter Lee    Date: 5/21/09    Time: 4:26p
 * Updated in $/software/control processor/code/system
 * SCR #193 Informal Code Review Findings
 *
 * *****************  Version 3  *****************
 * User: Peter Lee    Date: 5/21/09    Time: 2:45p
 * Updated in $/software/control processor/code/system
 * Power Management Update - Inprogress
 *
 * *****************  Version 2  *****************
 * User: Peter Lee    Date: 4/29/09    Time: 10:22a
 * Updated in $/software/control processor/code/system
 * SCR #168 Updates from preliminary code review (jv).
 * Fix typo for  PM_LOG_DATA_RESOTRE_DATA_FAILED.
 *
 * *****************  Version 1  *****************
 * User: Peter Lee    Date: 10/08/08   Time: 11:45a
 * Created in $/control processor/code/system
 * Initial Check In
 *
 *
 ***************************************************************************/
