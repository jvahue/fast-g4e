#define FASTMGR_BODY
/******************************************************************************
            Copyright (C) 2007-2014 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:         FASTMgr.c

    Description:  Monitors triggers and other inputs to determing when the
                  in-flight, recording, battery latch, and weight on wheel
                  events occur, and notifies other tasks that handle these
                  events.

   VERSION
   $Revision: 128 $  $Date: 11/03/14 5:24p $


******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include <stdlib.h>

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "UploadMgr.h"
#include "CfgManager.h"
#include "user.h"
#include "FastMgr.h"
#include "DIO.h"
#include "LogManager.h"
#include "GSE.h"
#include "Assert.h"
#include "Version.h"
#include "CBitManager.h"
#include "EngineRun.h"
#include "DataManager.h"

#ifndef WIN32
#include "ProcessorInit.h"
#endif

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
// Define 10 minutes worth of MIFs 100 MIF/s * 60s/min * 10 min
#define _10_MINUTES ( (10*60*1000) / MIF_PERIOD_mS)
#define TICKS_PER_SEC       1000
#define TXTEST_DATA_STR_LEN 80
#define BLINK_LO_CNT        100
#define BLINK_HI_CNT        50
#define NO_VPN_TO           600
#define PERCENT_CONST       99

//Tags for setting up "record" active events in FAST_Init.  Tags are OR'd
//together in FAST_OnRecordChange, each should have its own bitfield.
#define FAST_REC_EVENTS_TIME_HIST   0x1
#define FAST_REC_EVENTS_EVENTS      0x2
#define FAST_REC_EVENTS_ENG_RUN     0x4
#define FAST_REC_EVENTS_DATA_MGR    0x8
#define FAST_REC_EVENTS_ETM_LOG     0x10


/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
typedef enum {
  FAST_TXTEST_STATE_STOPPED,
  FAST_TXTEST_STATE_SYSCON,
  FAST_TXTEST_STATE_WOWDISC,
  FAST_TXTEST_STATE_ONGND,
  FAST_TXTEST_STATE_REC,
  FAST_TXTEST_STATE_MSRDY,
  FAST_TXTEST_STATE_SIMRDY,
  FAST_TXTEST_STATE_GSM,
  FAST_TXTEST_STATE_VPN,
  FAST_TXTEST_STATE_UL,
  FAST_TXTEST_STATE_PASS,
  FAST_TXTEST_STATE_FAIL
}FAST_TXTEST_TASK_STATE;

typedef enum{
  FAST_TXTEST_INIT,
  FAST_TXTEST_PASS,
  FAST_TXTEST_FAIL,
  FAST_TXTEST_INPROGRESS,
  FAST_TXTEST_GSMPASS,
  FAST_TXTEST_MOVELOGSTOMS,
  FAST_TXTEST_MOVELOGSTOGROUND
} FAST_TXTEST_TEST_STATUS;

typedef struct {
  FAST_TXTEST_TASK_STATE state;
  FAST_TXTEST_TEST_STATUS sysCon;
  FAST_TXTEST_TEST_STATUS wowDisc;
  FAST_TXTEST_TEST_STATUS onGround;
  FAST_TXTEST_TEST_STATUS record;
  FAST_TXTEST_TEST_STATUS msReady;
  FAST_TXTEST_TEST_STATUS simReady;
  FAST_TXTEST_TEST_STATUS gsmSignal;
  FAST_TXTEST_TEST_STATUS vpnStatus;
  FAST_TXTEST_TEST_STATUS ulStatus;
  CHAR inProgressStr[TXTEST_DATA_STR_LEN];
  CHAR gsmPassStr[TXTEST_DATA_STR_LEN];
  CHAR failReason[TXTEST_DATA_STR_LEN];
  CHAR moveLogsStr[TXTEST_DATA_STR_LEN];
}FAST_TXTEST_DATA;

typedef struct {
  BOOLEAN inFlight;
  BOOLEAN recording;
  BOOLEAN onGround;
  BOOLEAN commandedRfEnb;         //Desired state of the RF before factoring in override,
                                  //WOW, and system condition
  BOOLEAN mssimRunning;
  BOOLEAN isVPNConnected;
  BOOLEAN rfGsmEnable;            //Actual state of the RF output
  BOOLEAN wlanPowerEnable;
  BOOLEAN logTransferToGSActive;  //Transfering logs to the ground server
  UINT32  logTransferToGSNoVPNTO; //Timeout if no VPN connection after upload
  BOOLEAN downloadStarted;
  BOOLEAN autoDownload;           // Flag that Auto download performed once
} FASTMGR_STATUS;

typedef struct{
  UINT32 timeElapsedS;
}FASTMGR_AUTO_UL_TASK_DATA;

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static FASTMGR_AUTO_UL_TASK_DATA autoUploadTaskData;
static FAST_TXTEST_DATA m_FastTxTest;
static FASTMGR_STATUS fastStatus;

static BOOLEAN wlanOverride;
static BOOLEAN gsmOverride;
static CHAR    swVersion[SW_VERSION_LEN];
static BOOLEAN m_FASTControlEnb;
static INT32   m_RecordBusyFlags;
static BOOLEAN m_IsRecordBusy;


#include "FastMgrUserTables.c"

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void FAST_Task(void* pParam);
static void FAST_UploadCheckTask(void* pParam);
static void FAST_AtStartOfFlight(void);
static void FAST_AtEndOfFlight(void);
static void FAST_AtStartOfRecord(void);
static void FAST_AtEndOfRecord(void);
static void FAST_AtEndOfDownload(void);
static void FAST_AtStartOnGround(void);
static void FAST_AtEndOnGround(void);
static void FAST_AtMSSIMRunning(void);
static void FAST_AtMSSIMLost(void);
static void FAST_AtVPNConnected(void);
static void FAST_AtVPNNotConnected(void);
static void FAST_WatchdogTask1(void* pParam);
static void FAST_WatchdogTask2(void* pParam);
static void FAST_DioControl(void);
static void FAST_RfGsmEnable(void);
static void FAST_WlanPowerEnable(void);
static void FAST_TxTestTask(void* pParam);
static void FAST_DoTxTestTask(BOOLEAN Condition, UINT32 timeout, UINT32 StartTime_s,
      FAST_TXTEST_TEST_STATUS* TestStatus, FAST_TXTEST_TASK_STATE NextTest,
      CHAR* FailStr );
static void FAST_OnRecordingChange(INT32 tag, BOOLEAN is_busy);
static void FAST_TxTestStateUL (INT32 *NumFilesPendingRoundTrip);
static void FAST_UpdateSysAnnuciation ( DIO_OUT_OP Blink );


/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:    FAST_Init
 *
 * Description: Initial setup for the FAST Manager.  Starts the FAST manager
 *              task and initializes module global variables
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:
 *
 *****************************************************************************/
void FAST_Init(void)
{
  TCB task_info;

  User_AddRootCmd(&root_msg);

  //State flags for FAST system status
  fastStatus.inFlight               = FALSE;
  fastStatus.recording              = FALSE;
  fastStatus.mssimRunning           = FALSE;
  fastStatus.isVPNConnected         = FALSE;
  fastStatus.logTransferToGSActive  = FALSE;
  fastStatus.logTransferToGSNoVPNTO = 0;
  fastStatus.downloadStarted        = FALSE;
  fastStatus.autoDownload           = FALSE;
  m_RecordBusyFlags                 = 0;
  m_IsRecordBusy                    = FALSE;

  memset(&m_FastTxTest,0,sizeof(m_FastTxTest));

  CBITMgr_UpdateFlightState(FALSE);

  wlanOverride     = FALSE;
  gsmOverride      = FALSE;
  m_FASTControlEnb = TRUE;

  strncpy_safe(swVersion,  sizeof(swVersion), PRODUCT_VERSION, _TRUNCATE);

  //FAST Manager Task

  // Register the application busy flag with the Power Manager.
  PmRegisterAppBusyFlag(PM_FAST_REC_TRIG_BUSY, &fastStatus.recording, PM_BUSY_LEGACY);
  PmRegisterAppBusyFlag(PM_FAST_RECORDING_BUSY,&m_IsRecordBusy, PM_BUSY_LEGACY);
  PmRegisterAppBusyFlag(PM_WAIT_VPN_CONN, &fastStatus.logTransferToGSActive, PM_BUSY_LEGACY);

  memset(&task_info, 0, sizeof(task_info));
  strncpy_safe(task_info.Name, sizeof(task_info.Name),"FAST Manager",_TRUNCATE);
  task_info.TaskID         = FAST_Manager;
  task_info.Function       = FAST_Task;
  task_info.Priority       = taskInfo[FAST_Manager].priority;
  task_info.Type           = taskInfo[FAST_Manager].taskType;
  task_info.modes          = taskInfo[FAST_Manager].modes;
  task_info.MIFrames       = taskInfo[FAST_Manager].MIFframes;
  task_info.Rmt.InitialMif = taskInfo[FAST_Manager].InitialMif;
  task_info.Rmt.MifRate    = taskInfo[FAST_Manager].MIFrate;
  task_info.Enabled        = TRUE;
  task_info.Locked         = FALSE;
  task_info.pParamBlock    = NULL;
  TmTaskCreate (&task_info);

  //Watchdog Tasks
  memset(&task_info, 0, sizeof(task_info));
  strncpy_safe(task_info.Name, sizeof(task_info.Name),"FAST WDOG1",_TRUNCATE);
  task_info.TaskID         = FAST_WDOG1;
  task_info.Function       = FAST_WatchdogTask1;
  task_info.Priority       = taskInfo[FAST_WDOG1].priority;
  task_info.Type           = taskInfo[FAST_WDOG1].taskType;
  task_info.modes          = taskInfo[FAST_WDOG1].modes;
  task_info.MIFrames       = taskInfo[FAST_WDOG1].MIFframes;
  task_info.Rmt.InitialMif = taskInfo[FAST_WDOG1].InitialMif;
  task_info.Rmt.MifRate    = taskInfo[FAST_WDOG1].MIFrate;
  task_info.Enabled        = TRUE;
  task_info.Locked         = FALSE;
  task_info.pParamBlock    = NULL;
  TmTaskCreate (&task_info);

  //Watchdog Tasks
  memset(&task_info, 0, sizeof(task_info));
  strncpy_safe(task_info.Name, sizeof(task_info.Name),"FAST WDOG2",_TRUNCATE);
  task_info.TaskID         = FAST_WDOG2;
  task_info.Function       = FAST_WatchdogTask2;
  task_info.Priority       = taskInfo[FAST_WDOG2].priority;
  task_info.Type           = taskInfo[FAST_WDOG2].taskType;
  task_info.modes          = taskInfo[FAST_WDOG2].modes;
  task_info.MIFrames       = taskInfo[FAST_WDOG2].MIFframes;
  task_info.Rmt.InitialMif = taskInfo[FAST_WDOG2].InitialMif;
  task_info.Rmt.MifRate    = taskInfo[FAST_WDOG2].MIFrate;
  task_info.Enabled        = TRUE;
  task_info.Locked         = FALSE;
  task_info.pParamBlock    = NULL;
  TmTaskCreate (&task_info);

  //Auto-upload task
  // Performs timing based on frames, should be highish priority
  memset(&task_info, 0, sizeof(task_info));
  memset(&autoUploadTaskData,0,sizeof(autoUploadTaskData));
  strncpy_safe(task_info.Name, sizeof(task_info.Name),"FAST U/L Chk",_TRUNCATE);
  task_info.TaskID         = FAST_UL_Chk;
  task_info.Function       = FAST_UploadCheckTask;
  task_info.Priority       = taskInfo[FAST_UL_Chk].priority;
  task_info.Type           = taskInfo[FAST_UL_Chk].taskType;
  task_info.modes          = taskInfo[FAST_UL_Chk].modes;
  task_info.MIFrames       = taskInfo[FAST_UL_Chk].MIFframes;
  task_info.Rmt.InitialMif = taskInfo[FAST_UL_Chk].InitialMif; //run every second
  task_info.Rmt.MifRate    = taskInfo[FAST_UL_Chk].MIFrate;
  task_info.Enabled        = TRUE;
  task_info.Locked         = FALSE;
  task_info.pParamBlock    = &autoUploadTaskData;
  TmTaskCreate (&task_info);

  //FAST TxTest Task
  // Performs timing based on frames, should be highish priority
  memset(&task_info, 0, sizeof(task_info));
  memset(&autoUploadTaskData,0,sizeof(autoUploadTaskData));
  strncpy_safe(task_info.Name, sizeof(task_info.Name),"FAST TxTest",_TRUNCATE);
  task_info.TaskID         = FAST_TxTestID;
  task_info.Function       = FAST_TxTestTask;
  task_info.Priority       = taskInfo[FAST_TxTestID].priority;
  task_info.Type           = taskInfo[FAST_TxTestID].taskType;
  task_info.modes          = taskInfo[FAST_TxTestID].modes;
  task_info.MIFrames       = taskInfo[FAST_TxTestID].MIFframes;
  task_info.Rmt.InitialMif = taskInfo[FAST_TxTestID].InitialMif; //run every second
  task_info.Rmt.MifRate    = taskInfo[FAST_TxTestID].MIFrate;
  task_info.Enabled        = FALSE;
  task_info.Locked         = FALSE;
  task_info.pParamBlock    = NULL;
  TmTaskCreate (&task_info);

  //Setup events for recording/not recording status change.
  TH_SetRecStateChangeEvt(FAST_REC_EVENTS_TIME_HIST,FAST_OnRecordingChange);
  DataMgrSetRecStateChangeEvt(FAST_REC_EVENTS_DATA_MGR,FAST_OnRecordingChange);
  EngRunSetRecStateChangeEvt(FAST_REC_EVENTS_ENG_RUN,FAST_OnRecordingChange);
  EventSetRecStateChangeEvt(FAST_REC_EVENTS_EVENTS,FAST_OnRecordingChange);
  LogETM_SetRecStateChangeEvt(FAST_REC_EVENTS_ETM_LOG, FAST_OnRecordingChange);
}


/******************************************************************************
 * Function:    FAST_TimeSourceCfg
 *
 * Description: Returns the Time Source Cfg
 *
 * Parameters:  none
 *
 * Returns:     TIME_SOURCE_ENUM
 *
 * Notes:       none
 *
 *****************************************************************************/
TIME_SOURCE_ENUM FAST_TimeSourceCfg( void )
{
  return ( CfgMgr_RuntimeConfigPtr()->FASTCfg.time_source );
}

/******************************************************************************
 * Function:    FAST_GetSoftwareVersion
 *
 * Description: Returns the Software Version String
 *
 * Parameters:  [in/out] SwVerStr - Software Version Str
 *
 * Returns:     None
 *
 * Notes:       none
 *
 *****************************************************************************/
void FAST_GetSoftwareVersion(INT8* SwVerStr)
{
  strncpy_safe(SwVerStr,sizeof(swVersion),swVersion,_TRUNCATE);
}

/******************************************************************************
 * Function:    FAST_SignalUploadComplete
 *
 * Description: When the upload completes if we do not have a VPN connection
 *              then we must wait for up to 10 minutes waiting for one.  This
 *              wait will keep the system "busy" or powered up if battery
 *              latching is enabled.
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:       none
 *
 *****************************************************************************/
void FAST_SignalUploadComplete( void)
{

  //Do not start LogTransfer timer if the
  if(m_FASTControlEnb )
  {
    fastStatus.logTransferToGSActive = TRUE;
    fastStatus.logTransferToGSNoVPNTO = CM_GetTickCount()+CM_DELAY_IN_SECS(NO_VPN_TO);
  }
}



/******************************************************************************
 * Function:    FAST_TurnOffFASTControl
 *
 * Description: Selectivly disables certian parts of FAST manager relating
 *              to the "OnGround" and "Recording" configurable controls
 *
 *              Turning off FAST control will remove control of:
 *              1. AtStart/AtEnd On Ground condition
 *              2. AtStart/AtEnd Record condition
 *              3. Battery Latch while for the LogTransferToGSActive flag
 *              4. Disable upload check task.
 *
 *              Parts relating to the VPN and Micro-Server events still run.
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:       none
 *
 *****************************************************************************/
void FAST_TurnOffFASTControl( void)
{
  //Clear on ground and record trigger masks (can't do AtStart/AtEnd without
  //them
  m_FASTControlEnb = FALSE;
 // CfgMgr_RuntimeConfigPtr()->FASTCfg.OnGroundTriggers = 0;
  memset( CfgMgr_RuntimeConfigPtr()->FASTCfg.on_ground_triggers, 0, sizeof(BITARRAY128 ));
 // CfgMgr_RuntimeConfigPtr()->FASTCfg.RecordTriggers = 0;
  memset( CfgMgr_RuntimeConfigPtr()->FASTCfg.record_triggers, 0, sizeof(BITARRAY128 ));
  TmTaskEnable(FAST_UL_Chk, FALSE);
}



/******************************************************************************
 * Function:    FAST_FSMRfRun    | IMPLEMENTS Run() INTERFACE to
 *                               | FAST STATE MGR
 *
 * Description: Signals the FAST Manager to turn on or turn off RF.  The
 *              legacy AtStartOnGround and AtEndOnGround are called to
 *              accomplish this.  They also call the Micro-Server to signal
 *              the "On Ground" (which now isn't really on-ground but engine-
 *              off) so it can run the scripts for Rf turning on and off.
 *
 * Parameters:  [in] Run: TRUE: Run the Rf Power
 *                        FALSE: Turn Rf Power off
 *              [in] param: not used, just to match FSM call signature.
 * Returns:     none
 *
 * Notes:
 *
 *****************************************************************************/
void FAST_FSMRfRun(BOOLEAN Run, INT32 param)
{
  if(Run)
  {
    FAST_AtStartOnGround();
  }
  else
  {
    FAST_AtEndOnGround();
  }
}



/******************************************************************************
 * Function:    FAST_FSMRfGetState   | IMPLEMENTS GetState() INTERFACE to
 *                                   | FAST STATE MGR
 *
 * Description: Returns the actual state of the GSM RF power.  This is not
 *              the exact same as the Run parameter, it also factors in
 *              the hardware WOW signal and system condition.
 *
 *              The reason for returning the actual state is that the "task"
 *              of controlling the commanded state of the RF does not involve
 *              any processing, so if it was turned on it is on, and turned off
 *              is off.  Returning the commanded state adds no value, so the
 *              actual state may be of more use.
 *
 *
 * Parameters:  [in] param: not used, just to match FSM call sig.
 *
 * Returns:     BOOLEAN: TRUE: RF Power is on
 *                       FALSE: RF Power is off
 *
 * Notes:
 *
 *****************************************************************************/
BOOLEAN FAST_FSMRfGetState(INT32 param)
{
  return fastStatus.rfGsmEnable;
}



/******************************************************************************
 * Function:    FAST_FSMRecordGetState | IMPLEMENTS GetState() INTERFACE to
 *                                     | FAST STATE MGR
 *
 * Description:  Return the recording/not recording state of the following
 *
 *
 * Parameters:  [in] param: not used, just to match FSM call sig.
 *
 * Returns:     BOOLEAN: TRUE:
 *                       FALSE:
 *
 * Notes:
 *
 *****************************************************************************/
BOOLEAN FAST_FSMRecordGetState(INT32 param)
{
  return m_IsRecordBusy;
}



/******************************************************************************
 * Function:    FAST_FSMEndOfFlightRun    | IMPLEMENTS Run() INTERFACE to
 *                                        | FAST STATE MGR
 *
 * Description: The End of Flight log is used by the Upload Manager to
 *              delimit flights.
 *
 * Parameters:  [in] Run: TRUE:  Write the End Of Flight log
 *                        FALSE: No action
 *              [in] param: not used, just to match FSM call sig.
 *
 * Returns:     none
 *
 * Notes:
 *
 *****************************************************************************/
void FAST_FSMEndOfFlightRun(BOOLEAN Run, INT32 Param)
{
  if(Run)
  {
    // Write EOF Log and register the returned index for completion monitoring.
    LogRegisterIndex( LOG_REGISTER_END_FLIGHT,
                      LogWriteSystemEx(APP_ID_END_OF_FLIGHT, LOG_PRIORITY_LOW, 0, 0, NULL) );
  }
}


/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/
/******************************************************************************
 * Function:    FAST_OnRecordingChange
 *
 * Description: "Event" handler to be called by modules feeding the recording
 *              or not recording status back to FAST Mgr.  Currently, these
 *              are: Time History, Events, Engine Run, and
 *              Data Manager "recording"
 *
 * Parameters:  INT32 tag
 *              BOOLEAN is_busy
 *
 * Returns:     None
 *
 * Notes:
 *
 *****************************************************************************/
static void FAST_OnRecordingChange(INT32 tag, BOOLEAN is_busy)
{
  if(is_busy)
  {
    m_RecordBusyFlags |= tag;
  }
  else
  {
    m_RecordBusyFlags &= ~tag;
  }
  m_IsRecordBusy = m_RecordBusyFlags != 0;
}



/******************************************************************************
 * Function:    FAST_Task()
 *
 * Description: Monitors triggers and digital inputs for transitions of the
 *              Flight, recording, battery latch, and weight on wheels states
 *              Logs are generated on each transition and calls are made to
 *              other modules that need to be notified of these state
 *              transitions.
 *
 * Parameters:  pParam (i) - dummy to match function signature
 *
 * Returns:     None
 *
 * Notes:
 *
 *****************************************************************************/
static
void FAST_Task(void* pParam)
{
  BOOLEAN prevFlightState;

  /*ON GROUND
    Transition into the on ground state*/
  if( TriggerIsActive( CfgMgr_RuntimeConfigPtr()->FASTCfg.on_ground_triggers) &&
      !fastStatus.onGround)
  {
    LogWriteSystem(APP_ID_FAST_WOWSTART, LOG_PRIORITY_LOW, 0, 0, NULL);
    GSE_DebugStr(VERBOSE,TRUE,"FAST: On ground");
    FAST_AtStartOnGround();
    fastStatus.onGround = TRUE;
  }
  /*Transition out of the on ground state
    Could be in air or sensor went invalid, so don't assume "in air"*/
  else if(!TriggerIsActive(CfgMgr_RuntimeConfigPtr()->FASTCfg.on_ground_triggers)
           && fastStatus.onGround)
  {
    LogWriteSystem(APP_ID_FAST_WOWEND, LOG_PRIORITY_LOW, 0, 0, NULL);
    GSE_DebugStr(VERBOSE,TRUE,"FAST: Not on ground");
    FAST_AtEndOnGround();
    fastStatus.onGround = FALSE;
  }

  /*RECORDING
    Transition into the recording state*/
  if(TriggerIsActive( CfgMgr_RuntimeConfigPtr()->FASTCfg.record_triggers) &&
     !fastStatus.recording)
  {
    LogWriteSystem(APP_ID_START_OF_RECORDING, LOG_PRIORITY_LOW, 0, 0, NULL);
    fastStatus.recording = TRUE;
    GSE_DebugStr(VERBOSE,TRUE,"FAST: Start Record");
    FAST_AtStartOfRecord();
  }
  //Transition out of the recording state
  else if(!TriggerIsActive(CfgMgr_RuntimeConfigPtr()->FASTCfg.record_triggers) &&
          fastStatus.recording)
  {
    DataMgrRecord(FALSE);                      //Signal Data Mgr and wait for
    if(!m_IsRecordBusy)                        //acknowledge
    {
       LogWriteSystem(APP_ID_END_OF_RECORDING, LOG_PRIORITY_LOW, 0, 0, NULL);
       GSE_DebugStr(VERBOSE,TRUE,"FAST: End Record");
       FAST_AtEndOfRecord();
       fastStatus.recording = FALSE;
    }
  }

 /*IN-FLIGHT
    Transition into the start engine run or flight state*/

  // Save current flight state to test-for-change later.
  prevFlightState = fastStatus.inFlight;

  if(fastStatus.recording  && !fastStatus.inFlight)
  {
    GSE_DebugStr(VERBOSE,TRUE,"FAST: Start of Flight/Engine Run");
    FAST_AtStartOfFlight();
    fastStatus.inFlight = TRUE;
  }
  //Transition out of the flight state
  else if(fastStatus.onGround && !fastStatus.recording && fastStatus.inFlight )
  {
     // Write EOF Log and tell LogMgr to register the returned index so we can perform
     // completion-monitoring.
     LogRegisterIndex( LOG_REGISTER_END_FLIGHT,
                       LogWriteSystemEx(APP_ID_END_OF_FLIGHT, LOG_PRIORITY_LOW, 0, 0, NULL)
                     );

     GSE_DebugStr(NORMAL,TRUE,"FAST: End of Flight/Engine Run");
     fastStatus.inFlight = FALSE;

     fastStatus.downloadStarted = DataMgrStartDownload();
     if (fastStatus.downloadStarted == TRUE)
     {
        LogWriteSystem(APP_ID_START_OF_DOWNLOAD, LOG_PRIORITY_LOW, 0, 0, NULL);
        GSE_DebugStr(NORMAL,TRUE,"FAST: Start ACS Download");
     }
     else
     {
        FAST_AtEndOfFlight();
     }
  }

  /*DOWNLOAD ACS(s)
    Transition into the download state */
  if (!TriggerIsActive( CfgMgr_RuntimeConfigPtr()->FASTCfg.record_triggers) &&
      !fastStatus.recording && fastStatus.downloadStarted && fastStatus.onGround &&
      (FALSE == DataMgrDownloadingACS()))
  {
     LogWriteSystem(APP_ID_END_OF_DOWNLOAD, LOG_PRIORITY_LOW, 0, 0, NULL);
     GSE_DebugStr(NORMAL,TRUE,"FAST: End ACS Download");
     fastStatus.downloadStarted = FALSE;
     // Since a flight was detected and a normal download was performed
     // reset the flag so the auto-upload will initiate this at least once
     fastStatus.autoDownload    = FALSE;
     FAST_AtEndOfDownload();
  }


  // On change of flight state, notify CBIT Mgr.
  if (prevFlightState != fastStatus.inFlight)
  {
    CBITMgr_UpdateFlightState(fastStatus.inFlight);
  }

  /*MSSIM running
    Transition into the MSSIM running state*/
  if(MSSC_GetIsAlive() && !fastStatus.mssimRunning)
  {
    GSE_DebugStr(NORMAL, TRUE, "FAST: MSSIM is alive");
    FAST_AtMSSIMRunning();
    fastStatus.mssimRunning = TRUE;
  }
  //Transition out of the MSSIM running state
  else if(!MSSC_GetIsAlive() && fastStatus.mssimRunning)
  {
    GSE_DebugStr(NORMAL, TRUE, "FAST: MSSIM not detected");
    FAST_AtMSSIMLost();
    fastStatus.mssimRunning = FALSE;
  }

  /*VPN Connected
    Transition into the VPN*/
  if(MSSC_GetIsVPNConnected() && !fastStatus.isVPNConnected)
  {
    GSE_DebugStr(NORMAL,TRUE,"FAST: VPN connected");
    FAST_AtVPNConnected();
    fastStatus.isVPNConnected = TRUE;
  }
  //Transition out of the VPN connected
  else if(!MSSC_GetIsVPNConnected() && fastStatus.isVPNConnected)
  {
    GSE_DebugStr(NORMAL,TRUE,"FAST: VPN disconnected");
    FAST_AtVPNNotConnected();
    fastStatus.isVPNConnected = FALSE;
  }

  /*Declare log file transfer active and hold the battery latch if:
    1: It is within the 10 minutes timeout to establish a VPN connection after
       log upload OR
    2: There are files not verified on the Ground Server, and the VPN
       connection has been established.
  */
  fastStatus.logTransferToGSActive =
    ( (!fastStatus.isVPNConnected &&
       (CM_GetTickCount() < fastStatus.logTransferToGSNoVPNTO) ) ||
      (fastStatus.isVPNConnected && (UploadMgr_GetFilesPendingRT() != 0) ) );

    FAST_DioControl();
}

/******************************************************************************
* Function:    FAST_WatchdogTask1() | TASK
*
* Description: Feed the Watchdog (Step 1) at 1 Hz, 500 ms before Step 2.
*              See task scheduling in Taskmanager.h
*              Timeout for the ST Microelectronics STM70x on the FAST board is
*              approximately 1.6s
*
* Parameters:  pParam (i): dummy to satisfy call signature
*
* Returns:     None
*
* Notes:       This task conditionally performs all steps required for the
*              old (non-FPGA) version of WD pulsing.
*              Define SUPPORT_FPGA_BELOW_V12 to enable old watchdog pulse.
*
*****************************************************************************/
static
void FAST_WatchdogTask1(void* pParam)
{
  // OLDDOG conditionally supports old style WD - Automatically removed in compile
  OLDDOG( WDog, DIO_Toggle);
  FPGA_W( FPGA_WD_TIMER, FPGA_WD_TIMER_VAL);
}

/******************************************************************************
* Function:    FAST_WatchdogTask2() | TASK
*
* Description: Feed the Watchdog (Step 2) at 1 Hz, 500 ms after Step 1.
*              See task scheduling in Taskmanager.h
*              Timeout for the ST Microelectronics STM70x on the FAST board is
*              approximately 1.6s
*
* Parameters:  pParam (i): dummy to satisfy call signature
*
* Returns:     None
*
* Notes:
*
*****************************************************************************/
static
void FAST_WatchdogTask2(void* pParam)
{
    FPGA_W( FPGA_WD_TIMER_INV, FPGA_WD_TIMER_INV_VAL);

#ifndef WIN32
    // flush processor caches @ 1Hz rate
    PI_FlushCache();
#endif
}
/******************************************************************************
 * Function:    FAST_UploadCheckTask() | TASK
 *
 * Description: Checks for engine runs in log memory. If a aircraft data is
 *              found and the FAST is not currently recording, the upload
 *              process is started.  The task only looks for data logs, system
 *              logs are ignored.
 *
 * Parameters:  pParam (i): holds how long since last upload check
 *
 * Returns:     None
 *
 * Notes:       Interrupts turned off briefly to prevent a race condition
 *              between this task and the FAST Mgr task.
 *
 *****************************************************************************/
static
void FAST_UploadCheckTask(void* pParam)
{
  FASTMGR_AUTO_UL_TASK_DATA* task = pParam;
  static BOOLEAN upload_immediate_after_download = FALSE;
  INT32 int_level;

  task->timeElapsedS += 1;

  //If auto upload is enabled
  if(0 != CfgMgr_RuntimeConfigPtr()->FASTCfg.auto_ul_per_s)
  {
    if((CfgMgr_RuntimeConfigPtr()->FASTCfg.auto_ul_per_s <= task->timeElapsedS) ||
        upload_immediate_after_download )
    {
      // Start an auto-download only if the system is not already downloading,
      // not recording, not already uploading and has not tried to auto-download before
      if (!fastStatus.autoDownload && !DataMgrDownloadingACS() &&
          !m_IsRecordBusy    && !UploadMgr_IsUploadInProgress())
      {
        upload_immediate_after_download = DataMgrStartDownload();
        int_level = __DIR();
        fastStatus.autoDownload = upload_immediate_after_download;
        __RIR(int_level);
      }

      task->timeElapsedS = 0;

      int_level = __DIR();
      if(!fastStatus.recording && !UploadMgr_IsUploadInProgress() &&
         !DataMgrDownloadingACS() && MSSC_GetIsAlive())
      {
        UploadMgr_StartUpload(UPLOAD_START_AUTO);
        upload_immediate_after_download = FALSE;
      }
      __RIR(int_level);
    }
  }
  else
  {
    //Clear time elapsed while configuration has the
    //auto-upload disabled.
    task->timeElapsedS = 0;
  }
}

/******************************************************************************
 * Function:    FAST_DioControl()
 *
 * Description: Monitors the subsystems for configuration status, log transfer
 *              status and RF status.  Toggles the XFR, STA, and CFG LED
 *              indicators based on those statuses
 *
 *              For FAST: Status LED 1 is STS
 *                        Status LED 2 is XFR
 *                        Status LED 3 is CFG
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:       LED blink timing requires that this function MUST BE CALLED AT
 *              100Hz
 *
 *****************************************************************************/
static
void FAST_DioControl(void)
{
  static INT32 blink_count = 0;
  DIO_OUT_OP   blink = DIO_SetHigh;

  // compute RF GSM and WLAN Power control
  FAST_RfGsmEnable();
  FAST_WlanPowerEnable();

  if(blink_count++ < BLINK_HI_CNT)
  {
    blink = DIO_SetHigh;
  }
  else if((blink_count > BLINK_HI_CNT) && (blink_count < BLINK_LO_CNT))
  {
    blink = DIO_SetLow;
  }
  else
  {
    blink_count = 0;
  }

  //XFR
  switch(MSSC_GetXFR())
  {
    //LED off states
    case MSSC_XFR_NOT_VLD:
    case MSSC_XFR_NONE:
      DIO_SetPin(LED_XFR,DIO_SetLow);
      break;

    //LED on states
    case MSSC_XFR_DATA_LOG:
      DIO_SetPin(LED_XFR,DIO_SetHigh);
      break;

    case MSSC_XFR_SPECIAL_TX:
      DIO_SetPin(LED_XFR,blink);
      break;

    default:
      FATAL("Unsupported XFR_STATUS = %d", MSSC_GetXFR());
      break;
  }
  //STA
  switch(MSSC_GetSTS())
  {
    //LED off states
    case MSSC_STS_NOT_VLD:    //OFF
      // Use local RF Power State SRS-3186
      DIO_SetPin( LED_STS, fastStatus.rfGsmEnable ? DIO_SetHigh : DIO_SetLow);
      break;

    case MSSC_STS_OFF:        //OFF
      DIO_SetPin( LED_STS,DIO_SetLow);
      break;

    //LED on states
    case MSSC_STS_ON_WO_LOGS: //ON
      DIO_SetPin( LED_STS, DIO_SetHigh);
      break;

    //LED blinking states
    case MSSC_STS_ON_W_LOGS:  //FLASHING
      DIO_SetPin( LED_STS, blink);
      break;

    default:
     FATAL("Unsupported STS_STATUS = %d", MSSC_GetSTS());
     break;
  }

  //CFG
  switch(AC_GetConfigState())
  {
    case AC_CFG_STATE_NO_CONFIG:
      DIO_SetPin(LED_CFG,DIO_SetLow);
      break;

    case AC_CFG_STATE_CONFIG_OK:
      DIO_SetPin(LED_CFG,DIO_SetHigh);
      break;

    case AC_CFG_STATE_RETRIEVING:
      DIO_SetPin(LED_CFG,blink);
      break;

    default:
       FATAL("Unsupported AIRCRAFT_CONFIG_STATE = %d", AC_GetConfigState());
       break;
  }

  FAST_UpdateSysAnnuciation(blink);
}


/******************************************************************************
 * Function:    FAST_UpdateSysAnnuciation()
 *
 * Description: Performs annuciation of the system condition for both direct
 *              LSS output and Action modes.
 *
 * Parameters:  DIO_OUT_OP Blink
 *
 * Returns:     none
 *
 * Notes:
 *
 *****************************************************************************/
static
void FAST_UpdateSysAnnuciation ( DIO_OUT_OP Blink )
{
   // Local Data
   FLT_STATUS   sys_cond;
   FLT_ANUNC_MODE anunc_mode;
   DIO_OUTPUT   sys_cond_out_pin;

   sys_cond   = Flt_GetSystemStatus();
   anunc_mode = Flt_GetSysAnunciationMode();

  if (FLT_ANUNC_DIRECT == anunc_mode )
  {
    // SYS CONDITION - output to LSSx if configured
    sys_cond_out_pin = Flt_GetSysCondOutputPin();

    // If an output Pin has been enabled/configured, set the output Pin state
    // based on system condition.
    if(sys_cond_out_pin != SYS_COND_OUTPUT_DISABLED)
    {

      switch( sys_cond)
      {
        case STA_NORMAL:
          DIO_SetPin( sys_cond_out_pin, DIO_SetLow);
          break;

        case STA_CAUTION:
          DIO_SetPin( sys_cond_out_pin, Blink);
          break;

        case STA_FAULT:
          DIO_SetPin( sys_cond_out_pin, DIO_SetHigh);
          break;

        case STA_MAX:
        default:
          FATAL( "Unsupported SYSTEM CONDITION STATE = %d", sys_cond );
          break;
      }
    }
  }
  else if (FLT_ANUNC_ACTION == anunc_mode)
  {
     Flt_UpdateAction(sys_cond);
  }
}



/******************************************************************************
 * Function:    FAST_AtStartOfFlight(void)
 *
 * Description: Actions to take when the Flight state becomes active
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:
 *
 *****************************************************************************/
static
void FAST_AtStartOfFlight(void)
{
  //Record how many upload file verification rows are available in the table
  UploadMgr_WriteVfyTblRowsLog();
  AC_SetIsOnGround(FALSE);
  TmTaskEnable(FAST_UL_Chk, FALSE);       //Disable upload manager so it won't
                                          //try to read/erase logs while
                                          //record is writing them

  // Cancel any downloads in progress
  DataMgrStopDownload();
}

/******************************************************************************
 * Function:    FAST_AtEndOfFlight(void)
 *
 * Description: Actions to take when the Flight state becomes inactive
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:
 *
 *****************************************************************************/
static
void FAST_AtEndOfFlight(void)
{
  UploadMgr_SetUploadEnable(TRUE);
  autoUploadTaskData.timeElapsedS = 0;  //Reset seconds count for auto-ul
  TmTaskEnable(FAST_UL_Chk, TRUE);      //re-enable auto upload task
  UploadMgr_StartUpload(UPLOAD_START_POST_FLIGHT);
}

/******************************************************************************
 * Function:    FAST_AtStartOfRecord(void)
 *
 * Description: Actions to take when the Recording state becomes active
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:
 *
 *****************************************************************************/
static
void FAST_AtStartOfRecord(void)
{
  UploadMgr_SetUploadEnable(FALSE);
  DataMgrRecord(TRUE);                     //Signal Data Mgr
  CM_UpdateRecordingState(TRUE);           // Signal the Clock Mgr

}

/******************************************************************************
 * Function:    FAST_AtEndOfRecord(void)
 *
 * Description: Actions to take when the Recording state becomes inactive.
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:
 *
 *****************************************************************************/
static
void FAST_AtEndOfRecord(void)
{
  CM_UpdateRecordingState(FALSE);   // Signal the Clock Mgr
}

/******************************************************************************
 * Function:    FAST_AtEndOfDownload(void)
 *
 * Description: Actions to take when the Downloading state becomes inactive.
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:
 *
 *****************************************************************************/
static
void FAST_AtEndOfDownload(void)
{
  UploadMgr_SetUploadEnable(TRUE);
  UploadMgr_StartUpload(UPLOAD_START_POST_FLIGHT);
}

/******************************************************************************
 * Function:    FAST_AtStartOnGround(void)
 *
 * Description: Actions to take when the OnGround state becomes
 *              active
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:
 *
 *****************************************************************************/
static
void FAST_AtStartOnGround(void)
{
  //Tell wireless mgr FAST is on the ground
  MSSC_SetIsOnGround(TRUE);
  fastStatus.commandedRfEnb = TRUE;
}

/******************************************************************************
 * Function:    FAST_AtEndOnGround(void)
 *
 * Description: Actions to take when the OnGround state becomes
 *              inactive
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:
 *
 *****************************************************************************/
static
void FAST_AtEndOnGround(void)
{
  //Tell wireless mgr FAST is not on the ground
  MSSC_SetIsOnGround(FALSE);
  fastStatus.commandedRfEnb = FALSE;
  // Cancel any downloads in progress
  DataMgrStopDownload();
}

/******************************************************************************
 * Function:    FAST_AtMSSIMRunning
 *
 * Description: Actions to take when it is detected that MSSIM is running.
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:
 *
 *****************************************************************************/
static
void FAST_AtMSSIMRunning(void)
{
  //Aircraft Config may begin
  AC_SetIsMSConnected(TRUE);

  //Send the GSM configuration data one MSSIM is running.
  MSSC_SendGSMCfgCmd();

}

/******************************************************************************
 * Function:    FAST_AtMSSIMLost
 *
 * Description: Actions to take when it is detected that MSSIM is not running
 *              anymore.
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:
 *
 *****************************************************************************/
static
void FAST_AtMSSIMLost(void)
{
  AC_SetIsMSConnected(FALSE);
}

/******************************************************************************
 * Function:    FAST_AtVPNNotConnected
 *
 * Description: Actions to take when it is detected that the VPN is not
 *              connected to the ground server.
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:
 *
 *****************************************************************************/
static
void FAST_AtVPNNotConnected(void)
{
  AC_SetIsGroundServerConnected(FALSE);
}

/******************************************************************************
 * Function:    FAST_AtVPNConnected
 *
 * Description: Actions to take when the VPN connection to the ground server
 *              is established.
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:
 *
 *****************************************************************************/
static
void FAST_AtVPNConnected(void)
{
  AC_SetIsGroundServerConnected(TRUE);
}

/******************************************************************************
* Function:    FAST_RfGsmEnable
*
* Description: Compute the logic specified in SRS-2136. Set RfGsmEnable
*              in FAST_DioControl, and set the DOUT pin.
*
* Parameters:  none
*
* Returns:     none
*
* Notes:
*
*****************************************************************************/
static
void FAST_RfGsmEnable(void)
{
  BOOLEAN sysCondOk = (BOOLEAN)Flt_GetSystemStatus() != STA_FAULT;
  BOOLEAN wow = DIO_ReadPin( WOW);

  fastStatus.rfGsmEnable = (gsmOverride || sysCondOk) &&
                           fastStatus.commandedRfEnb &&
                           wow;

  // Output the computed value to the Dout Pin
  DIO_SetPin(GSMEnb, fastStatus.rfGsmEnable ? DIO_SetHigh : DIO_SetLow);
}

/******************************************************************************
* Function:    FAST_WlanPowerEnable
*
* Description: Compute the logic specified in SRS-2252.
*
* Parameters:  none
*
* Returns:     none
*
* Notes:
*
*****************************************************************************/
static
void FAST_WlanPowerEnable(void)
{
  BOOLEAN sysCondOk = (BOOLEAN)Flt_GetSystemStatus() != STA_FAULT;
  BOOLEAN wow = DIO_ReadPin( WOW);
  BOOLEAN wowOverride = DIO_ReadPin( WLAN_WOW_Enb);

  fastStatus.wlanPowerEnable = ((wlanOverride || sysCondOk) &&
                                fastStatus.commandedRfEnb &&
                                wow) ||
                                wowOverride;

  // Output the computed value to the Dout Pin
  DIO_SetPin(WLANEnb, fastStatus.wlanPowerEnable ? DIO_SetHigh : DIO_SetLow);
}


/******************************************************************************
* Function:    FAST_TxTestTask
*
* Description: State machine to test the pre-conditions necessary to
*              enable GSM and transmit files to the ground server. Performs
*              the following tests:
*               1) System Condition != FAULT
*               2) Wow Discrete == On ground
*               3) On Ground trigger == active
*               4) Recording trigger == inactive
*               5) MicroServer ready within configured timeout
*               6) SIM card ID available within configured timeout
*               7) GSM signal available within configured timeout
*               8) VPN connection to ground server ready within configured
*                  timeout
*               9) Files uploaded to the micro-server are round-trip
*                  verified to the ground server.
*
*
* Parameters:  pParam (i) - dummy to match function signature
*
* Returns:     none
*
* Notes:
*
*****************************************************************************/
static
void FAST_TxTestTask(void* pParam)
{
  static UINT32 testStart;
  static INT32 numFilesPendingRoundTrip;
  CHAR  gsm_str[MSCP_MAX_STRING_SIZE];

  switch(m_FastTxTest.state)
  {
    case FAST_TXTEST_STATE_STOPPED:
      break;

    case FAST_TXTEST_STATE_SYSCON:
      testStart = CM_GetTickCount()/TICKS_PER_SEC;
      FAST_DoTxTestTask((Flt_GetSystemStatus() != STA_FAULT),//Test Condition
                        0,                                  //Test Timeout
                        0,                                  //Test Start time
                        &m_FastTxTest.sysCon,               //Test Result loc.
                        FAST_TXTEST_STATE_WOWDISC,          //Next test after pass
                        FAST_TXTEST_SYSCON_FAIL_STR );      //Fail desc if test fails
      break;

    case FAST_TXTEST_STATE_WOWDISC:
      FAST_DoTxTestTask((DIO_ReadPin( WOW) == TRUE),        //Test Condition
                        0,                                  //Test Timeout
                        0,                                  //Test Start time
                        &m_FastTxTest.wowDisc,              //Test Result loc.
                        FAST_TXTEST_STATE_ONGND,            //Next test after pass
                        FAST_TXTEST_WOWDIS_FAIL_STR );      //Fail desc if test fails
      break;

    case FAST_TXTEST_STATE_ONGND:
      FAST_DoTxTestTask((fastStatus.commandedRfEnb == TRUE),//Test Condition
                        0,                                  //Test Timeout
                        0,                                  //Test Start time
                        &m_FastTxTest.onGround,             //Test Result loc.
                        FAST_TXTEST_STATE_REC,              //Next test after pass
                        FAST_TXTEST_ONGRND_FAIL_STR );      //Fail desc if test fails
      break;

    case FAST_TXTEST_STATE_REC:
      FAST_DoTxTestTask((DataMgrRecordFinished() == TRUE),  //Test Condition
                        0,                                  //Test Timeout
                        0,                                  //Test Start time
                        &m_FastTxTest.record,               //Test Result loc.
                        FAST_TXTEST_STATE_MSRDY,            //Next test after pass
                        FAST_TXTEST_RECORD_FAIL_STR );      //Fail desc if test fails
      break;

    case FAST_TXTEST_STATE_MSRDY:
      FAST_DoTxTestTask((MSSC_GetIsAlive() == TRUE),        //Test Condition
                        CfgMgr_RuntimeConfigPtr()->FASTCfg.tx_test_msrdy_to, //Test Timeout
                        testStart,                          //Test Start time
                        &m_FastTxTest.msReady,              //Test Result loc.
                        FAST_TXTEST_STATE_SIMRDY,           //Next test after pass
                        FAST_TXTEST_MSREDY_FAIL_STR );      //Fail desc if test fails
      break;

    case FAST_TXTEST_STATE_SIMRDY:
      MSSC_DoRefreshMSInfo();
      MSSC_GetGSMSCID(gsm_str);
      //Verify SIM ID is a big number (usually 18 or 19 numbers, starting with "89")
      FAST_DoTxTestTask((UINT32_MAX == strtoul(gsm_str,NULL,10)) ,//Test Condition
                        CfgMgr_RuntimeConfigPtr()->FASTCfg.tx_test_SIM_rdy_to, //Test Timeout
                        testStart,                        //Test Start time
                        &m_FastTxTest.simReady,           //Test Result loc.
                        FAST_TXTEST_STATE_GSM,            //Next test after pass
                        FAST_TXTEST_SIMCRD_FAIL_STR );    //Fail desc if test fails
      break;

    case FAST_TXTEST_STATE_GSM:
      MSSC_DoRefreshMSInfo();
      MSSC_GetGSMSignalStrength(gsm_str);
      //Verify the gsm string has a decoded signal strength, formatted as
      //(-xxdB) by MSSIM.  If no signal is available, the (-xxdB) format won't
      //be in the string
      FAST_DoTxTestTask((NULL != strstr(gsm_str,"dB)")),  //Test Condition
                        CfgMgr_RuntimeConfigPtr()->FASTCfg.tx_test_GSM_rdy_to, //Test Timeout
                        testStart,                        //Test Start time
                        &m_FastTxTest.gsmSignal,          //Test Result loc.
                        FAST_TXTEST_STATE_VPN,            //Next test after pass
                        FAST_TXTEST_GSMSIG_FAIL_STR );    //Fail desc if test fails
      //After test passes, change test status to show special GSM pass message
      if(m_FastTxTest.state == FAST_TXTEST_STATE_VPN)
      {
        snprintf(m_FastTxTest.gsmPassStr,sizeof(m_FastTxTest.gsmPassStr)
            ,"PASS %s",strchr(gsm_str,'('));
        m_FastTxTest.gsmSignal = FAST_TXTEST_GSMPASS;
      }
      break;

    case FAST_TXTEST_STATE_VPN:
      FAST_DoTxTestTask((MSSC_GetIsVPNConnected()),       //Test Condition
                        CfgMgr_RuntimeConfigPtr()->FASTCfg.tx_test_VPN_rdy_to, //Test Timeout
                        testStart,                        //Test Start time
                        &m_FastTxTest.vpnStatus,          //Test Result loc.
                        FAST_TXTEST_STATE_UL,             //Next test after pass
                        FAST_TXTEST_VPNSTA_FAIL_STR );    //Fail desc if test fails

      //After test passes, start file upload.
      if(m_FastTxTest.state == FAST_TXTEST_STATE_UL)
      {
        numFilesPendingRoundTrip = -1;
        m_FastTxTest.ulStatus = FAST_TXTEST_MOVELOGSTOMS;
        UploadMgr_StartUpload(UPLOAD_START_TXTEST);
      }
      break;

    case FAST_TXTEST_STATE_UL:
      FAST_TxTestStateUL(&numFilesPendingRoundTrip);
      break;

    case FAST_TXTEST_STATE_PASS:
      TmTaskEnable(FAST_TxTestID,TRUE);
      break;

    case FAST_TXTEST_STATE_FAIL:
      TmTaskEnable(FAST_TxTestID,TRUE);
      break;

    default:
      FATAL("Unknown TxTaskState %d",m_FastTxTest.state);
      break;
  }
}


/******************************************************************************
* Function:    FAST_TxTestStateUL
*
* Description: Function to support Transmit Test that updates the number
*              of files pending round trip and computes the % remaining for
*              file upload.
*
* Parameters:  [in/out] num_files_pending_round_trip: Static, set to -1 on
*                                                 first call to set initial
*                                                 number of files to RT.
*                                                 Updated to total number of
*                                                 files to round trip when
*                                                 upload completes.
*
* Returns:     none
*
* Notes:
*
*****************************************************************************/
static
void FAST_TxTestStateUL (INT32 *num_files_pending_round_trip)
{
   // Local Data
   INT32 numFilesStillPendingRoundTrip;

   if(*num_files_pending_round_trip == -1)
   {
      if(!UploadMgr_IsUploadInProgress())
      {
         //After upload, record the number of files waiting for round trip
         snprintf(m_FastTxTest.moveLogsStr,sizeof(m_FastTxTest.moveLogsStr),
                  "MoveLogsToGround 00%%");
         *num_files_pending_round_trip = UploadMgr_GetNumFilesPendingRT();
         //Preventing div/0
         *num_files_pending_round_trip = *num_files_pending_round_trip == 0 ?
                                    1 : *num_files_pending_round_trip;
         m_FastTxTest.ulStatus = FAST_TXTEST_MOVELOGSTOGROUND;
      }
   }
   else
   {
      numFilesStillPendingRoundTrip = UploadMgr_GetNumFilesPendingRT();
      snprintf(m_FastTxTest.moveLogsStr,sizeof(m_FastTxTest.moveLogsStr),
               "MoveLogsToGround %02d%%",
               PERCENT_CONST - (numFilesStillPendingRoundTrip * PERCENT_CONST)/
               *num_files_pending_round_trip);

      if(numFilesStillPendingRoundTrip == 0)
      {
         m_FastTxTest.ulStatus = FAST_TXTEST_PASS;
         m_FastTxTest.state = FAST_TXTEST_STATE_PASS;
      }
   }
}

/******************************************************************************
* Function:    FAST_DoTxTestTask
*
* Description: Performs the basic logic for each transmission test.  If the
*              test pass condition is not met, the test continues "InProgress"
*              until the test passes or times out/fails
*
*
* Parameters:  BOOLEAN  Condition
*              UINT32   timeout
*              INT32    StartTime_s
*              FAST_TXTEST_TEST_STATUS* TestStatus
*              FAST_TXTEST_TASK_STATE   NextTest
*              CHAR*    FailStr
*
* Returns:     none
*
* Notes:
*
*****************************************************************************/
static
void FAST_DoTxTestTask(BOOLEAN Condition, UINT32 timeout, UINT32 StartTime_s,
      FAST_TXTEST_TEST_STATUS* TestStatus, FAST_TXTEST_TASK_STATE NextTest,
      CHAR* FailStr )
{
  UINT32 elapsed_s;
  //If the test condition is not true, check timeout
  if(!Condition)
  {  //Timeout not zero, then check timeout
    if(timeout != 0)
    {
      elapsed_s = (CM_GetTickCount()/TICKS_PER_SEC)-StartTime_s;
      if(elapsed_s > timeout)
      {
        //Timeout elapsed, fail.
        strncpy_safe(m_FastTxTest.failReason,
          sizeof(m_FastTxTest.failReason),FailStr,_TRUNCATE);
        *TestStatus = FAST_TXTEST_FAIL;
        m_FastTxTest.state = FAST_TXTEST_STATE_FAIL;
      }
      else
      {  //Compute percentage of timeout elapsed and set this test to InProgress
        *TestStatus = FAST_TXTEST_INPROGRESS;
        snprintf(m_FastTxTest.inProgressStr,sizeof(m_FastTxTest.inProgressStr),
                 "InProgress %02d%%",(elapsed_s * PERCENT_CONST)/timeout);
      }
    }
    else
    {  //No timeout specified, and condition is not true so this test fails
      strncpy_safe(m_FastTxTest.failReason,
        sizeof(m_FastTxTest.failReason),FailStr,_TRUNCATE);
      *TestStatus = FAST_TXTEST_FAIL;
      m_FastTxTest.state = FAST_TXTEST_STATE_FAIL;
    }
  }
  else
  { //Condition true, test passes
    *TestStatus = FAST_TXTEST_PASS;
    m_FastTxTest.state = NextTest;
  }

}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: FASTMgr.c $
 * 
 * *****************  Version 128  *****************
 * User: Contractor V&v Date: 11/03/14   Time: 5:24p
 * Updated in $/software/control processor/code/application
 * SCR #1092 - Forceupload recording-in-progress notification. Modfix-rec
 * flag change
 *
 * *****************  Version 127  *****************
 * User: Contractor V&v Date: 10/06/14   Time: 4:04p
 * Updated in $/software/control processor/code/application
 * SCR #1092 - Forceupload recording-in-progress notification.
 *
 * *****************  Version 126  *****************
 * User: Contractor V&v Date: 9/03/14    Time: 5:09p
 * Updated in $/software/control processor/code/application
 * SCR #1164 - Permit CP Time Syncing only when Not Recording - CR fixes
 *
 * *****************  Version 125  *****************
 * User: Contractor V&v Date: 8/26/14    Time: 4:20p
 * Updated in $/software/control processor/code/application
 * SCR #1164 - Permit CP Time Syncing only when Not Recording
 *
 * *****************  Version 124  *****************
 * User: Contractor V&v Date: 8/12/14    Time: 4:58p
 * Updated in $/software/control processor/code/application
 * Fix legacy Battery latch when FSM enabled
 *
 * *****************  Version 123  *****************
 * User: John Omalley Date: 12-12-20   Time: 5:00p
 * Updated in $/software/control processor/code/application
 * SCR 1197 - Code Review Update
 *
 * *****************  Version 122  *****************
 * User: John Omalley Date: 12-12-20   Time: 4:12p
 * Updated in $/software/control processor/code/application
 * SCR 1197 - Code Review Updates
 *
 * *****************  Version 121  *****************
 * User: Jim Mood     Date: 12/13/12   Time: 2:56p
 * Updated in $/software/control processor/code/application
 * SCR #1197
 *
 * *****************  Version 120  *****************
 * User: Jim Mood     Date: 12/13/12   Time: 1:48p
 * Updated in $/software/control processor/code/application
 * [MOD] SCR #1197
 *
 * *****************  Version 119  *****************
 * User: Jim Mood     Date: 12/11/12   Time: 8:33p
 * Updated in $/software/control processor/code/application
 * SCR #1197 Code Review Updates
 *
 * *****************  Version 118  *****************
 * User: Jim Mood     Date: 12/04/12   Time: 6:00p
 * Updated in $/software/control processor/code/application
 * SCR #1131 Application busy
 *
 * *****************  Version 117  *****************
 * User: Contractor V&v Date: 12/03/12   Time: 5:29p
 * Updated in $/software/control processor/code/application
 * SCR #1107 Code Review
 *
 * *****************  Version 116  *****************
 * User: Jim Mood     Date: 11/27/12   Time: 8:34p
 * Updated in $/software/control processor/code/application
 * SCR# 1166 Auto Upload Task Overrun (from FASTMgr.c)
 *
 * *****************  Version 115  *****************
 * User: John Omalley Date: 12-11-14   Time: 7:09p
 * Updated in $/software/control processor/code/application
 * SCR 1076 - Code Review Updates
 *
 * *****************  Version 114  *****************
 * User: Jim Mood     Date: 11/09/12   Time: 6:16p
 * Updated in $/software/control processor/code/application
 * SCR 1131 Record busy status
 *
 * *****************  Version 113  *****************
 * User: John Omalley Date: 12-11-08   Time: 3:03p
 * Updated in $/software/control processor/code/application
 * SCR 1131 - Busy Recording Logic
 *
 * *****************  Version 112  *****************
 * User: John Omalley Date: 12-11-07   Time: 8:35a
 * Updated in $/software/control processor/code/application
 * SCR 1105 - Added new LogWriteSystem Function that returns a value
 *
 * *****************  Version 111  *****************
 * User: Jim Mood     Date: 11/06/12   Time: 11:54a
 * Updated in $/software/control processor/code/application
 * SCR# 1107
 *
 * *****************  Version 110  *****************
 * User: Melanie Jutras Date: 12-10-19   Time: 2:09p
 * Updated in $/software/control processor/code/application
 * SCR #1172 PCLint 545 Suspicious use of & Error
 *
 * *****************  Version 109  *****************
 * User: Jeff Vahue   Date: 8/29/12    Time: 12:40p
 * Updated in $/software/control processor/code/application
 * Code Review Tool Findings
 *
 * *****************  Version 108  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 12:43p
 * Updated in $/software/control processor/code/application
 * SCR# 1142
 *
 * *****************  Version 107  *****************
 * User: John Omalley Date: 12-08-28   Time: 8:32a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - ETM Fault Logic Updates
 *
 * *****************  Version 106  *****************
 * User: John Omalley Date: 12-08-24   Time: 9:30a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Added ETM Fault Action Logic
 *
 * *****************  Version 105  *****************
 * User: Jim Mood     Date: 7/26/12    Time: 2:08p
 * Updated in $/software/control processor/code/application
 * SCR# 1076 Code Review Updates
 *
 * *****************  Version 104  *****************
 * User: Contractor V&v Date: 3/21/12    Time: 6:46p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 Renamed TRIGGER_FLAGS
 *
 * *****************  Version 103  *****************
 * User: Contractor V&v Date: 3/14/12    Time: 4:50p
 * Updated in $/software/control processor/code/application
 * SCR #1107 Trigger processing
 *
 * *****************  Version 102  *****************
 * User: Jim Mood     Date: 2/24/12    Time: 10:25a
 * Updated in $/software/control processor/code/application
 * SCR 1114 - Re labeled after v1.1.1 release
 *
 * *****************  Version 100  *****************
 * User: Contractor V&v Date: 12/14/11   Time: 6:49p
 * Updated in $/software/control processor/code/application
 * SCR #1105 End of Flight Log Race Condition
 *
 * *****************  Version 99  *****************
 * User: John Omalley Date: 10/11/11   Time: 4:59p
 * Updated in $/software/control processor/code/application
 * SCR 1078  and SCR 1076 Code Review Updates
 *
 * *****************  Version 98  *****************
 * User: Jim Mood     Date: 10/04/11   Time: 3:15p
 * Updated in $/software/control processor/code/application
 * SCR 1083 Move Auto Upload enable from end of record to end of flight
 * per requirement SRS2521
 *
 * *****************  Version 97  *****************
 * User: Jim Mood     Date: 9/27/11    Time: 6:19p
 * Updated in $/software/control processor/code/application
 * SCR 1072 Modfix.  Fixes upload attempts every 1 second by the auto
 * upload task after downloading ACS
 *
 * *****************  Version 95  *****************
 * User: John Omalley Date: 9/12/11    Time: 2:20p
 * Updated in $/software/control processor/code/application
 * SCR 1073 - Cancelled download on In-Flight and when on-ground no longer
 * detected.
 *
 * *****************  Version 94  *****************
 * User: Jim Mood     Date: 7/26/11    Time: 3:06p
 * Updated in $/software/control processor/code/application
 * SCR 575 updates
 *
 * *****************  Version 93  *****************
 * User: Jim Mood     Date: 7/20/11    Time: 10:54a
 * Updated in $/software/control processor/code/application
 * SCR 575: GSM Enable when engine status is lost.  (Part of changes for
 * the Fast State Machine)
 *
 * *****************  Version 92  *****************
 * User: John Omalley Date: 4/21/11    Time: 4:55p
 * Updated in $/software/control processor/code/application
 * SCR 1029
 *
 * *****************  Version 91  *****************
 * User: John Omalley Date: 4/20/11    Time: 8:17a
 * Updated in $/software/control processor/code/application
 * SCR 1029
 *
 * *****************  Version 90  *****************
 * User: John Omalley Date: 4/14/11    Time: 11:38a
 * Updated in $/software/control processor/code/application
 * SCR 1029 - Added Upload Enable if no ACS Downloaded needed
 *
 * *****************  Version 89  *****************
 * User: John Omalley Date: 4/11/11    Time: 10:08a
 * Updated in $/software/control processor/code/application
 * SCR 1029 - ACS Download logic added
 *
 * *****************  Version 88  *****************
 * User: Jeff Vahue   Date: 10/16/10   Time: 1:33p
 * Updated in $/software/control processor/code/application
 * SCR# 939 - Logic error for upload busy
 *
 * *****************  Version 87  *****************
 * User: Contractor2  Date: 10/12/10   Time: 3:09p
 * Updated in $/software/control processor/code/application
 * SCR #916 Code Review
 *
 * *****************  Version 86  *****************
 * User: Jim Mood     Date: 10/06/10   Time: 7:29p
 * Updated in $/software/control processor/code/application
 * SCR 880: Battery latch while files not round tripped
 *
 * *****************  Version 85  *****************
 * User: Contractor2  Date: 10/05/10   Time: 1:54p
 * Updated in $/software/control processor/code/application
 * SCR #916 Code Review Updates
 *
 * *****************  Version 84  *****************
 * User: Contractor2  Date: 9/24/10    Time: 3:03p
 * Updated in $/software/control processor/code/application
 * SCR #882 WD - Old Code for pulsing the WD should be removed
 *
 * *****************  Version 83  *****************
 * User: Jeff Vahue   Date: 9/21/10    Time: 5:39p
 * Updated in $/software/control processor/code/application
 * SCR# 880 - Busy Logic
 *
 * *****************  Version 82  *****************
 * User: Jeff Vahue   Date: 9/09/10    Time: 11:53a
 * Updated in $/software/control processor/code/application
 * SCR# 833 - Perform only three attempts to get the Cfg files from the
 * MS.  Fix various typos.
 *
 * *****************  Version 81  *****************
 * User: Jim Mood     Date: 8/27/10    Time: 4:30p
 * Updated in $/software/control processor/code/application
 * SCR 828 Aircraft config in-air detection
 *
 * *****************  Version 80  *****************
 * User: Contractor V&v Date: 7/30/10    Time: 9:21p
 * Updated in $/software/control processor/code/application
 * SCR #282 Misc - Shutdown Processing
 *
 * *****************  Version 79  *****************
 * User: Jim Mood     Date: 7/30/10    Time: 6:57p
 * Updated in $/software/control processor/code/application
 * SCR 327 Fast transmission test implementation
 *
 * *****************  Version 78  *****************
 * User: Jim Mood     Date: 7/30/10    Time: 11:23a
 * Updated in $/software/control processor/code/application
 * SCR 327 Fast transmission test
 *
 * *****************  Version 77  *****************
 * User: Jim Mood     Date: 7/30/10    Time: 8:56a
 * Updated in $/software/control processor/code/application
 * SCR 327 Fast transmssion test functionality
 *
 * *****************  Version 76  *****************
 * User: Contractor V&v Date: 7/27/10    Time: 2:25p
 * Updated in $/software/control processor/code/application
 * SCR #282 Misc - Shutdown Processing
 *
 * *****************  Version 75  *****************
 * User: Jeff Vahue   Date: 7/19/10    Time: 6:29p
 * Updated in $/software/control processor/code/application
 * SCR# 716 - fix INT32 access and unindent code in FastMgr.c
 *
 * *****************  Version 74  *****************
 * User: Jim Mood     Date: 6/29/10    Time: 6:16p
 * Updated in $/software/control processor/code/application
 * SCR 623 Reconfiguration function updates
 *
 * *****************  Version 73  *****************
 * User: Contractor2  Date: 6/14/10    Time: 4:12p
 * Updated in $/software/control processor/code/application
 * SCR #645 Periodically flush instruction/branch caches
 *
 * *****************  Version 72  *****************
 * User: Jim Mood     Date: 6/11/10    Time: 4:14p
 * Updated in $/software/control processor/code/application
 * SCR 623 Batch Configuration implementation updates
 *
 * *****************  Version 71  *****************
 * User: John Omalley Date: 6/08/10    Time: 12:03p
 * Updated in $/software/control processor/code/application
 * SCR 627 - Updated for LJ60
 *
 * *****************  Version 70  *****************
 * User: Contractor2  Date: 5/28/10    Time: 1:40p
 * Updated in $/software/control processor/code/application
 * SCR #595 CP SW Version accessable through USER.c command
 * Added SW Version number to fast.status command.
 * Also some minor coding standard fixes.
 *
 * *****************  Version 69  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:53p
 * Updated in $/software/control processor/code/application
 * SCR #587 Change TmTaskCreate to return void
 *
 * *****************  Version 68  *****************
 * User: Jim Mood     Date: 5/07/10    Time: 6:24p
 * Updated in $/software/control processor/code/application
 * Added VPN Conneced status
 *
 * *****************  Version 67  *****************
 * User: Jeff Vahue   Date: 5/03/10    Time: 3:09p
 * Updated in $/software/control processor/code/application
 * SCR #580 - code review changes
 *
 * *****************  Version 66  *****************
 * User: Contractor V&v Date: 4/30/10    Time: 11:28a
 * Updated in $/software/control processor/code/application
 * SCR #574 System Level objects should not be including Applicati
 *
 * *****************  Version 65  *****************
 * User: Contractor V&v Date: 4/28/10    Time: 4:49p
 * Updated in $/software/control processor/code/application
 * SCR #559 Check clock diff CP-MS on startup
 *
 * *****************  Version 64  *****************
 * User: Contractor V&v Date: 4/22/10    Time: 6:37p
 * Updated in $/software/control processor/code/application
 * SCR #559 Check clock diff CP-MS on startup
 *
 * *****************  Version 63  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:07p
 * Updated in $/software/control processor/code/application
 * SCR #317 Implement safe strncpy SCR #70 Store/Restore interrupt
 *
 * *****************  Version 62  *****************
 * User: Jeff Vahue   Date: 3/30/10    Time: 4:59p
 * Updated in $/software/control processor/code/application
 * SCR# 463 - correct WD headers and other functio headers, other minor
 * changes see Investigator note.
 *
 * *****************  Version 61  *****************
 * User: Contractor V&v Date: 3/29/10    Time: 6:14p
 * Updated in $/software/control processor/code/application
 * SCR #512 Configure fault status  LED
 *
 * *****************  Version 60  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:37p
 * Updated in $/software/control processor/code/application
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 *
 * *****************  Version 59  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/application
 * SCR# 483 - Function Names
 *
 * *****************  Version 58  *****************
 * User: Contractor2  Date: 3/02/10    Time: 11:37a
 * Updated in $/software/control processor/code/application
 * SCR# 472 - Fix file/Function headers, footer
 *
 * *****************  Version 57  *****************
 * User: Jeff Vahue   Date: 3/02/10    Time: 10:19a
 * Updated in $/software/control processor/code/application
 * SCR# 469 - init override signals
 *
 * *****************  Version 56  *****************
 * User: Jeff Vahue   Date: 3/02/10    Time: 10:16a
 * Updated in $/software/control processor/code/application
 * SCR# 469 - provide unique override commands for WLAN and GSM
 *
 * *****************  Version 55  *****************
 * User: Jeff Vahue   Date: 2/26/10    Time: 2:59p
 * Updated in $/software/control processor/code/application
 * SCR# 469 - RF GSM/WLAN POWER logic implementation
 *
 * *****************  Version 54  *****************
 * User: Jeff Vahue   Date: 2/26/10    Time: 11:17a
 * Updated in $/software/control processor/code/application
 * SCR# 243 (b) and SRS-2650 - Wait for MS ready to begin upload
 *
 * *****************  Version 53  *****************
 * User: Jeff Vahue   Date: 2/24/10    Time: 12:40p
 * Updated in $/software/control processor/code/application
 * SCR# 463 - 2 WD Tasks to pulse 500 ms apart
 *
 * *****************  Version 52  *****************
 * User: Jeff Vahue   Date: 2/17/10    Time: 12:51p
 * Updated in $/software/control processor/code/application
 * SCR# 452 - Code Coverage if/then/else changes
 *
 * *****************  Version 51  *****************
 * User: Contractor V&v Date: 2/03/10    Time: 2:56p
 * Updated in $/software/control processor/code/application
 * SCR 18
 *
 * *****************  Version 50  *****************
 * User: John Omalley Date: 1/29/10    Time: 9:33a
 * Updated in $/software/control processor/code/application
 * SCR 395
 * - Updated the LED names to correspond with externally labeled LEDs
 *
 * *****************  Version 49  *****************
 * User: Jeff Vahue   Date: 1/27/10    Time: 11:01a
 * Updated in $/software/control processor/code/application
 * SCR# 429
 *
 * *****************  Version 48  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 4:59p
 * Updated in $/software/control processor/code/application
 * SCR# 397
 *
 * *****************  Version 47  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:19p
 * Updated in $/software/control processor/code/application
 * SCR 371
 *
 * *****************  Version 46  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 2:11p
 * Updated in $/software/control processor/code/application
 * SCR #326
 *
 * *****************  Version 45  *****************
 * User: Contractor V&v Date: 12/10/09   Time: 5:38p
 * Updated in $/software/control processor/code/application
 * SCR 42
 *
 * *****************  Version 44  *****************
 * User: Peter Lee    Date: 11/23/09   Time: 5:57p
 * Updated in $/software/control processor/code/application
 * SCR #347 Time Sync Requirements
 *
 * *****************  Version 43  *****************
 * User: Peter Lee    Date: 11/20/09   Time: 2:36p
 * Updated in $/software/control processor/code/application
 * SCR #345 Implement FPGA Watchdog Requirements
 *
 * *****************  Version 42  *****************
 * User: Contractor V&v Date: 11/18/09   Time: 3:09p
 * Updated in $/software/control processor/code/application
 * Fix for SCR 172
 *
 * *****************  Version 41  *****************
 * User: Peter Lee    Date: 11/12/09   Time: 11:57a
 * Updated in $/software/control processor/code/application
 * SCR #315  Support PWEH SEU Processing.  Add func to return current
 * InFlight status.
 *
 * *****************  Version 40  *****************
 * User: Jim Mood     Date: 10/26/09   Time: 11:03a
 * Updated in $/software/control processor/code/application
 * Updated the user tables (moved .ver and .auto_ul under .cfg) per the
 * User IRS document
 *
 * *****************  Version 39  *****************
 * User: Jim Mood     Date: 10/20/09   Time: 5:32p
 * Updated in $/software/control processor/code/application
 * more SCR 288 additions
 *
 * *****************  Version 38  *****************
 * User: Jim Mood     Date: 10/16/09   Time: 10:08a
 * Updated in $/software/control processor/code/application
 * SCR #288.  Commented out "old flight test" user commands that were not
 * specified by the requriments and are now handled elsewhere.
 *
 * *****************  Version 37  *****************
 * User: Jim Mood     Date: 9/29/09    Time: 11:58a
 * Updated in $/software/control processor/code/application
 * SCR# 178 Updated User Manager Tables
 *
 * *****************  Version 36  *****************
 * User: Jim Mood     Date: 9/23/09    Time: 10:50a
 * Updated in $/software/control processor/code/application
 * Send gsm.cfg file on MSSIM connected
 *
 * *****************  Version 35  *****************
 * User: Jim Mood     Date: 7/06/09    Time: 3:44p
 * Updated in $/software/control processor/code/application
 * SCR# 206
 *
 * *****************  Version 34  *****************
 * User: Peter Lee    Date: 6/29/09    Time: 11:33a
 * Updated in $/software/control processor/code/application
 * SCR #204 Misc Code Review Updates
 *
 * *****************  Version 33  *****************
 * User: Jim Mood     Date: 5/15/09    Time: 5:16p
 * Updated in $/software/control processor/code/application
 * Update STS lamp definiton and control (NO Special RX state anymore)
 *
 * *****************  Version 32  *****************
 * User: Jim Mood     Date: 4/22/09    Time: 3:28p
 * Updated in $/control processor/code/application
 *
 * *****************  Version 31  *****************
 * User: Jim Mood     Date: 4/02/09    Time: 8:56a
 * Updated in $/control processor/code/application
 * Added an event for MSSIM client connect/disconnect
 * Added LED control of STS, XFR, and CFG based on MSSIM status and
 * Configuration status
 * Added a FAST.VER (same as G3/DTU's DTU.VER) command to identify the
 * configuration version
 *
 * *****************  Version 30  *****************
 * User: Jim Mood     Date: 3/05/09    Time: 2:28p
 * Updated in $/control processor/code/application
 * Added AircraftCfgManager control
 *
 * *****************  Version 29  *****************
 * User: Jim Mood     Date: 11/12/08   Time: 1:41p
 * Updated in $/control processor/code/application
 *
 * *****************  Version 28  *****************
 * User: Jim Mood     Date: 10/08/08   Time: 10:06a
 * Updated in $/control processor/code/application
 * SCR #87
 *
 * *****************  Version 27  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 11:26a
 * Updated in $/control processor/code/application
 * 1) SCR #87 Function Prototype
 * 2) Update call LogWriteSystem() to include NULL for ts
 * 3) Moved call to create POWER_ON log to PM
 *
 * *****************  Version 26  *****************
 * User: Jim Mood     Date: 10/07/08   Time: 10:55a
 * Updated in $/control processor/code/application
 *
 * *****************  Version 25  *****************
 * User: Jim Mood     Date: 6/20/08    Time: 5:08p
 * Updated in $/control processor/code/application
 *
 * *****************  Version 24  *****************
 * User: John Omalley Date: 6/18/08    Time: 2:57p
 * Updated in $/control processor/code/application
 * SCR-0044 Update System Logs
 *
 * *****************  Version 23  *****************
 * User: Jim Mood     Date: 6/11/08    Time: 9:03a
 * Updated in $/control processor/code/application
 *
 * *****************  Version 22  *****************
 * User: Jim Mood     Date: 6/10/08    Time: 2:30p
 * Updated in $/control processor/code/application
 * SCR-0040 - Updated to use the new function prototype for LogFindNext
 *
 *
 *
 ***************************************************************************/
