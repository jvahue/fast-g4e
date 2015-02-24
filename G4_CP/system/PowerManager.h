#ifndef POWERMANAGER_H
#define POWERMANAGER_H

/******************************************************************************
            Copyright (C) 2008-2014 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:         PowerManager.h

    Description:  Monitor the aircraft Battery voltage and process failures of
                  the aircraft Bus or Battery power supplies.

    VERSION
    $Revision: 38 $  $Date: 2/17/15 7:40p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "SystemLog.h"


/******************************************************************************
                                 Package Defines                              *
******************************************************************************/
#define UNLATCH_BATTERY   DIO_SetPin( PowerHold, DIO_SetLow )
#define LATCH_BATTERY     DIO_SetPin( PowerHold, DIO_SetHigh )

#define BUS_VOLTAGE_THRESHOLD 10.0F
#define MIN_BATTERY_VOLTAGE   10.0F

#define PM_INIT_COMPLETE      0x3EE3E33EUL   // Unique identifier to indicate
                                             //   initialization completed.

#define PM_GLITCH_TIMER_EXPIRE       5             // 5 * MIF time        -> 50 msec
#define PM_BATTERY_CHECK_BUS_PERIOD  100           // 100 * MIF time      ->  1 sec
#define PM_BATTERY_2HR_PERIOD        (100 * 7200)  // 10 ms * 100 * 7200  ->  2 Hr
#define PM_BATTERY_1MIN_PERIOD       (100 * 60)    // 10 ms * 100 * 60    ->  1 Min
#define PM_SHUTDOWN_TIMER_EXPIRE     1000          // 1000 * MIF time     -> 10 sec
#define PM_MIF_PER_SEC               100           // 1 MIF (10 ms) * 100 ->  1 sec

#define PM_MAX_SHUTDOWN_APP          10

#define PM_CFG_DEFAULT     FALSE,                        /* Battery */\
                           3600                          /* Battery Latch Time */


/******************************************************************************
                                 Package Typedefs                             *
******************************************************************************/

// PM Log Types
/*
typedef enum
{
    PM_POWER_OFF_FAILURE_LOG   = 0, //
    PM_POWER_RESTORE_LOG,           //
    PM_POWER_OFF_LOG,               //
    PM_FORCED_SHUTDOWN_LOG,         //
    PM_POWER_ON_LOG                 //
} PM_LOG_EVENT;
*/

// PM Battery States
/*
typedef enum
{
    BATTERY_BAD    = 0,         // Battery is not available or is available,
                                // but its voltage is too low.
    BATTERY_GOOD,               // Battery is available and its voltage is
                                // sufficient
    BATTERY_MAX_STATE
} BATTERY_STATE;
*/


// Set of values used by AppShutdown (PmCallAppXXXXXXXX) functions to notify
// the registered callback function as to the reason it is being called.
// This allows a single callback function to be invoked for
// multiple reasons (e.g. ShutdownNormal, ShutdownQuick, BusInterrupt, etc)
// and modify its behavior accordingly
typedef enum
{
  PM_APPSHUTDOWN_NORMAL = 0, // Normal Shutdown
  PM_APPSHUTDOWN_QUICK,      // Quick Shutdown
  PM_APPSHUTDOWN_BUS_INTR,   // Bus Interrupt/Glitch
  PM_MAX_APPSHUTDOWN
}PM_APPSHUTDOWN_REASON;

// PM States
typedef enum
{
    PM_NORMAL    = 0,         // Normal operation, no power failure.
    PM_GLITCH,                // PM checking if bus failure is real ot temp.
    PM_BATTERY,               // System operating from latched battery
    PM_SHUTDOWN,              // System shutting down !
    PM_HALT,                  // System halted due to power failure
    PM_MAX_STATE
} PM_STATE;

typedef BOOLEAN (*APPSHUTDOWN_FUNC)(PM_APPSHUTDOWN_REASON);

// PM DATA Structure
typedef struct
{
    PM_STATE      State;           // Power status of system
    BOOLEAN       Battery;         // TRUE if battery is available
    UINT32        PmStateTimer;    // Timer to track time in PM_BATTERY state
                                   //    (ticks)
    UINT32        PmBattDeglitchTimer;  // Timer used during PM_BATTERY state
                                        //    (tickes)
    FLOAT32       BusVoltage;      // Last aircraft bus voltage reading
    FLOAT32       BatteryVoltage;  // Last aircraft battery voltage reading

    FLOAT32       InternalTemp;    // Internal Board Temperature Reading
    FLOAT32       LithiumBattery;  // Internal Lithium Battery Reading

    UINT32        BusPowerInterruptCnt;  // Count of the number of times
                                         //   Battery Power has been gone below
                                         //   10.8 Volts

    BOOLEAN       AppBusy;            // App Busy flag.
                                      // This flag is set based on testing all relevant
                                      // task busy flags.

    UINT32        nInitCompleted;     // PM Initialization Flag 0xA5A5A5A5
    UINT32        nInitCompletedInv;  // PM Initialization Flag inverted

    UINT16        LowPwrInt;        // Flag set in ISR to indicate Low Power Int Occurred
                                    //    Ack by PmTask() approx 10 msec later.
                                    // Provides indirect debounce of ISR processing.
    UINT16        LowPwrIntAck;     // Low Power Int acknowledged flag

    APPSHUTDOWN_FUNC  AppShutDownNormal[PM_MAX_SHUTDOWN_APP];
    APPSHUTDOWN_FUNC  AppShutDownQuick[PM_MAX_SHUTDOWN_APP];
    APPSHUTDOWN_FUNC  AppBusInterrupt[PM_MAX_SHUTDOWN_APP];
    UINT16            NumAppShutDownNormal;

    UINT16            NumAppBusInterrupt;

    UINT32        battLatchTime;   // Battery latch hold time value copied from pm.cfg
                                   // Resolution is 1 sec.

    TIMESTAMP     PowerOnTime;     // Box Power On Time.  Init after Box RTC is initialized !

    BOOLEAN       bLatchToBattery; // Force System to Latch to Battery Operation

} PM_DATA;


// PM_EEPROM_DATA Structure
typedef struct
{
    PM_STATE      State;                  // Current PM State
    TIMESTAMP     StateTrans_Time;        // Previous Time of PM Transition
    BOOLEAN       bPowerFailure;          // Flag to indicate Power Failure
    BOOLEAN       bBattery;               // System Battery Configuration
    BOOLEAN       bHaltCompleted;         // Flag to indicate HALT processing completed
                                          //   successfully.
    UINT16        appBusyFlags;           // Packed 16 bit word containing m_latchCtrl data
} PM_EEPROM_DATA;

// PM_LOG_TYPES
/*
typedef enum
{
    PM_POWER_ON_LOG = 0,

    PM_POWER_OFF_LOG,
    PM_POWER_OFF_FAILURE_LOG,

    PM_POWER_RESTORE_LOG,
    PM_FORCED_SHUTDOWN_LOG,

    PM_EEPROM_CORRUPT_LOG

} PM_LOG_TYPES;
*/



// Internal PM_INTERNAL_LOG structure use ONLY !!
typedef struct
{
    TIMESTAMP    ts;      // PowerOffRequest_Time value.
                          // Note: This is not the time the log is recorded,
                          //       but the time of the power event, which
                          //       could have been stored from a previous
                          //       power on.

    SYS_APP_ID   Type;    // Power Log Event Type From SystemLog.h

    PM_STATE     State;   // State of the Event Type

    BOOLEAN      bPowerFailure;
    BOOLEAN      bBattery;    // System Battery Configuration
    BOOLEAN      bHaltCompleted; // State of HALT completed processing

} PM_INTERNAL_LOG;
// Internal PM_INTERNAL_LOG structure use ONLY !!



// *****************************************************
// Power Manager Cfg Structure
// *****************************************************
typedef struct
{
  BOOLEAN Battery;           // Battery Latch Enable / Disable.
  UINT32 BatteryLatchTime;   // Battery Latch Hold Time.  Resolution is 1 sec
} POWERMANAGER_CFG;



#pragma pack(1)

// Power Management Data
typedef struct {
  UINT16    Id;          // Echo of SYS_ID_PM_POWER_FAILURE_LOG ID
  TIMESTAMP ts;          // Ts of Power Failure Occurance.
  PM_STATE  State;       // State of PM on power failure.
  BOOLEAN   bHaltCompleted;
} PM_LOG_DATA_POWER_FAILURE, *PM_LOG_DATA_POWER_FAILURE_PTR;

typedef struct {
  UINT32    Reserved;
  BOOLEAN   bHaltCompleted;
} PM_LOG_DATA_POWER_OFF, *PM_LOG_DATA_POWER_OFF_PTR;

typedef struct {
  UINT32    ResultCode;   // Result Code from NV Manager
} PM_LOG_DATA_RESTORE_DATA_FAILED, *PM_LOG_DATA_RESTORE_DATA_FAILED_PTR;

typedef struct {
  UINT32    Reserved;
} PM_LOG_DATA_FORCE_SHUTDOWN, *PM_LOG_DATA_FORCE_SHUTDOWN_PTR;

#pragma pack()

// Battery latching control variables

// Busy Indicators used by: PmRegisterAppBusyFlag
// Each entry represents an offset into the array of pointers
// to busy tasks, activities.
typedef enum
{
  PM_FAST_RECORDING_BUSY,          // FAST is REC
  PM_FAST_REC_TRIG_BUSY,           // FAST REC trigger active
  PM_UPLOAD_LOG_COLLECTION_BUSY,   // UploadMgr is collecting
  PM_WAIT_VPN_CONN,                // Wait for a VPN connection
  PM_MS_FILE_XFR_BUSY,             // MS is transferring to ground
  PM_FSM_BUSY,                     // Fast State Mgr indicates app busy
  PM_NVMGR_BUSY,                   // NVM Mgr is direct-updating P/B to EEPROM
  PM_MAX_BUSY
} PM_BUSY_INDEX;

// This enum control when the Busy flag is
typedef enum
{
  PM_BUSY_NONE   = 0,  // Initial state, unused.
  PM_BUSY_LEGACY = 1,  // The busy flag at PM_BUSY_INDEX is used when in Legacy cfg mode
  PM_BUSY_FSM    = 2,  // The busy flag at PM_BUSY_INDEX is used when in FSM cfg mode
  PM_BUSY_ALL    = 3,  // The busy flag at PM_BUSY_INDEX is used when in legacy or FSM cfg
  PM_MAX_BUSY_USAGE
}PM_BUSY_USAGE;


/******************************************************************************
                                 Package Exports                              *
******************************************************************************/
#undef EXPORT

#if defined ( POWERMANAGER_BODY )
   #define EXPORT
#else
   #define EXPORT extern
#endif

/******************************************************************************
                             Package Exports Variables
******************************************************************************/
// Protect PMP_DATA struct from ram test
#ifndef WIN32
#pragma ghs section bss=".pbss"
#endif
EXPORT PM_DATA Pm;
#ifndef WIN32
#pragma ghs section bss=default
#endif

EXPORT PM_EEPROM_DATA Pm_Eeprom;

/******************************************************************************
                             Package Exports Functions
******************************************************************************/
EXPORT void PmPreInit(void);
EXPORT void PmInitializePowerManager(void);
EXPORT __interrupt void PowerFailIsr (void);
EXPORT void PmInsertAppShutDownNormal( APPSHUTDOWN_FUNC func );
EXPORT void PmInsertAppShutDownQuick( APPSHUTDOWN_FUNC func );
EXPORT void PmInsertAppBusInterrupt( APPSHUTDOWN_FUNC func );
EXPORT void PmSetPowerOnTime( TIMESTAMP *pPtr );
EXPORT BOOLEAN PmFileInit(void);
EXPORT void PmRegisterAppBusyFlag(PM_BUSY_INDEX busyIndex,
                                  BOOLEAN *pAppBusyFlag,
                                  PM_BUSY_USAGE usage );
EXPORT BOOLEAN PmFSMAppBusyGetState(INT32 param);
EXPORT void PmFSMAppBusyRun(BOOLEAN Run, INT32 param);
EXPORT void PmSetFsmActive(void);

#ifdef GENERATE_SYS_LOGS
  EXPORT void PmCreateAllInternalLogs ( void );
#endif



#endif // POWERMANAGER_H
/*************************************************************************
 *  MODIFICATIONS
 *    $History: PowerManager.h $
 * 
 * *****************  Version 38  *****************
 * User: Contractor V&v Date: 2/17/15    Time: 7:40p
 * Updated in $/software/control processor/code/system
 * SCR #1055 - Primary != Back EEPROM added appflag states to PwrMgr file 
 * 
 * *****************  Version 37  *****************
 * User: Contractor V&v Date: 9/03/14    Time: 5:26p
 * Updated in $/software/control processor/code/system
 * SCR #1204 - Legacy App Busy Input still latches battery / CR updates
 *
 * *****************  Version 36  *****************
 * User: Contractor V&v Date: 8/12/14    Time: 5:10p
 * Updated in $/software/control processor/code/system
 * SCR 1055 EEPROM Backup not updated before SHUTDOWN and SCR 1204 Legacy
 * app busy input still latches battery when FSM enabled
 *
 * *****************  Version 35  *****************
 * User: Jim Mood     Date: 12/11/12   Time: 8:31p
 * Updated in $/software/control processor/code/system
 * SCR #1197 Code Review update
 *
 * *****************  Version 34  *****************
 * User: Jim Mood     Date: 12/04/12   Time: 5:59p
 * Updated in $/software/control processor/code/system
 * SCR #1131 Application busy for new 2.0 apps
 *
 * *****************  Version 33  *****************
 * User: John Omalley Date: 12-11-14   Time: 7:20p
 * Updated in $/software/control processor/code/system
 * SCR 1076 - Code Review Updates
 *
 * *****************  Version 32  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 31  *****************
 * User: Jim Mood     Date: 7/20/11    Time: 10:57a
 * Updated in $/software/control processor/code/system
 * SCR 575: GSM Enable when engine status is lost.  (Part of changes for
 * the Fast State Machine)
 *
 * *****************  Version 30  *****************
 * User: Contractor2  Date: 6/15/11    Time: 3:37p
 * Updated in $/software/control processor/code/system
 * SCR #812 Enhancement - Forced Shutdown log, max stack value
 *
 * *****************  Version 29  *****************
 * User: Contractor2  Date: 5/18/11    Time: 11:02a
 * Updated in $/software/control processor/code/system
 * SCR #986 Enhancement: PM - Possiblility of missing a Power interrupt
 *
 * *****************  Version 28  *****************
 * User: Contractor2  Date: 10/14/10   Time: 11:49a
 * Updated in $/software/control processor/code/system
 * SCR #934 Code Review: PowerManager
 *
 * *****************  Version 27  *****************
 * User: Jeff Vahue   Date: 9/21/10    Time: 5:50p
 * Updated in $/software/control processor/code/system
 * SCR #880 - Upload Busy Logic
 *
 * *****************  Version 26  *****************
 * User: John Omalley Date: 9/09/10    Time: 2:15p
 * Updated in $/software/control processor/code/system
 * SCR 855 - Removed unused functions
 *
 * *****************  Version 25  *****************
 * User: Peter Lee    Date: 8/04/10    Time: 11:33a
 * Updated in $/software/control processor/code/system
 * SCR #653 More accurate BATT Volt Reading
 *
 * *****************  Version 24  *****************
 * User: Contractor V&v Date: 7/30/10    Time: 9:38p
 * Updated in $/software/control processor/code/system
 * SCR #282 Misc - Shutdown Processing
 *
 * *****************  Version 23  *****************
 * User: Contractor3  Date: 7/16/10    Time: 8:57a
 * Updated in $/software/control processor/code/system
 * SCR #702 - Changes based on code review.
 *
 * *****************  Version 22  *****************
 * User: Contractor2  Date: 6/14/10    Time: 1:06p
 * Updated in $/software/control processor/code/system
 * SCR #483 Function Names must begin with the CSC it belongs with.
 *
 * *****************  Version 21  *****************
 * User: Contractor V&v Date: 6/09/10    Time: 7:22p
 * Updated in $/software/control processor/code/system
 * SCR #614 fast.reset=really should initialize files
 *
 * *****************  Version 20  *****************
 * User: Contractor2  Date: 5/25/10    Time: 3:03p
 * Updated in $/software/control processor/code/system
 * SCR #563 Implementation: Req SRS-1811,1813,1814,1815,3162 - CBIT RAM
 * Tests
 * Added ram test protected sections. Coding standard fixes.
 *
 * *****************  Version 19  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:55p
 * Updated in $/software/control processor/code/system
 * SCR #587 Change TmTaskCreate to return void
 *
 * *****************  Version 18  *****************
 * User: Contractor2  Date: 5/06/10    Time: 2:10p
 * Updated in $/software/control processor/code/system
 * SCR #579 Change LogWriteSys to return void
 *
 * *****************  Version 17  *****************
 * User: Jeff Vahue   Date: 4/12/10    Time: 5:43p
 * Updated in $/software/control processor/code/system
 * SCR #541 - all XXX_CreateLogs functions are now dependent on
 * GENERATE_SYS_LOGS
 *
 * *****************  Version 16  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 15  *****************
 * User: Jeff Vahue   Date: 2/19/10    Time: 12:58p
 * Updated in $/software/control processor/code/system
 * SCR# 455 - remove LINT issues
 *
 * *****************  Version 14  *****************
 * User: Jeff Vahue   Date: 1/29/10    Time: 3:31p
 * Updated in $/software/control processor/code/system
 * SCR# 412: Assert checking - change all the Shut down registration
 * functions to use ASSERTs vs. If/then/else
 *
 * *****************  Version 13  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 5:11p
 * Updated in $/software/control processor/code/system
 * SCR# 397
 *
 * *****************  Version 12  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:27p
 * Updated in $/software/control processor/code/system
 * SCR 377
 *
 * *****************  Version 11  *****************
 * User: Peter Lee    Date: 11/19/09   Time: 10:45a
 * Updated in $/software/control processor/code/system
 * SCR #315 Fix bug with maintaining SEU count across power cycle
 *
 * *****************  Version 10  *****************
 * User: Peter Lee    Date: 10/05/09   Time: 5:57p
 * Updated in $/software/control processor/code/system
 * SCR #219 PM Battery Latch User Cmd
 *
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 9/30/09    Time: 9:30a
 * Updated in $/software/control processor/code/system
 * SCR #277 Power Management Req Updates.  Fix PM_CFG_DEFAULT.
 *
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 9/28/09    Time: 6:26p
 * Updated in $/software/control processor/code/system
 * scr #277 Power Management Requirements Implemented
 *
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 9/17/09    Time: 11:40a
 * Updated in $/software/control processor/code/system
 * SCR #64 Sequencing of Power On / Off Logs Recording / TS
 *
 * *****************  Version 6  *****************
 * User: Peter Lee    Date: 6/15/09    Time: 9:52a
 * Updated in $/software/control processor/code/system
 * SCR #199 Set Minimum Steady State Voltage to 10.8 as per hw schematic
 *
 * *****************  Version 5  *****************
 * User: Peter Lee    Date: 6/08/09    Time: 11:41a
 * Updated in $/software/control processor/code/system
 * Integrate NvMgr() to store PM shutdown state.  Complete system log
 * generation.
 *
 * *****************  Version 4  *****************
 * User: Peter Lee    Date: 5/21/09    Time: 2:45p
 * Updated in $/software/control processor/code/system
 * Power Management Update - Inprogress
 *
 * *****************  Version 3  *****************
 * User: Peter Lee    Date: 4/29/09    Time: 4:16p
 * Updated in $/software/control processor/code/system
 * SCR #168 Update reference from time.h to alt_time.h
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

