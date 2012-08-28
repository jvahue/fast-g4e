#define AIRCRAFTCONFIGMGR_BODY

/******************************************************************************
          Copyright (C) 2008-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

  File:          AircraftConfigMgr.c
 
  Description:   Aircraft Auto Re-Configuration Routines 
  
  VERSION
      $Revision: 51 $  $Date: 10/26/11 4:04p $ 
 
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    
#include <stdlib.h>

#include "TestPoints.h"

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "AircraftConfigMgr.h"
#include "taskmanager.h"
#include "CfgManager.h"
#include "GSE.h"
#include "Trigger.h"
#include "Sensor.h"
#include "User.h"
#include "ClockMgr.h"
#include "LogManager.h"
#include "Box.h"
#include "WatchDog.h"
#include "UploadMgr.h"
#include "NVMgr.h"


CHAR *ACConfigStatus[] =
{
  "AC_CFG_SUCCESS",             // 0    0 - not used
  "AC_NOT_USED",                // 1    1 - not used 
  "AC_XML_SERVER_NOT_FOUND",    // 2    2 - Server not found OK
  "AC_XML_FILE_NOT_FOUND",      // 3    3 - File not found 
  "AC_XML_FAILED_TO_OPEN",      // 4    4 - File failed to open ???
  "AC_XML_FAST_NOT_FOUND",      // 5    5 - File format error (any type)
  "AC_XML_SN_NOT_FOUND",        // 6    6 - Serial number not in file
  "AC_CFG_SERVER_NOT_FOUND",    // 7    7 - Configuration server not found
  "AC_CFG_FILE_NOT_FOUND",      // 8    8 - Configuration file not found
  "AC_CFG_FILE_NO_VER",         // 9    9 - Configuration version is missing
  "AC_CFG_FILE_FMT_ERR",        // 10   10- Configuration file format error NEW
  "AC_CFG_FAILED_TO_UPDATE",    // 11   11- Reconfiguration of FAST failed
  "AC_ID_MISMATCH",             // 12   12- Aircraft ID mismatch (was 001)     
  "AC_CFG_NO_VALIDATE",         // 13   13- Validate data == no
  "AC_CFG_NOTHING",
  "AC_CFG_STARTED",
  "AC_CFG_CLIENT_ERROR"    
};

CHAR *ACUpdateMethod[] =
  {
    "Nothing",
    "Serial Number",
    "Check Config Version",
    "Check Version & Validate Data",
    "NULL"
  };
  
/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define AC_CFG_STS_LOG_SIZE 8
//Shortened up version of strncpy_safe for use with direct object references.
#define sstrncpy(dest, source) strncpy_safe(dest, sizeof(dest), source,_TRUNCATE);
#define EE_FILE_MAGIC_NUMBER 0x600DF00D
//Number of times to retry getting the configuration status from the 
//Micro-Server.  10 seconds is long enough, that in the event the CP is unable
//to send the status command, and the Micro-Server is still sending .cfg commands,
//that it should be able to finish sending .cfg commands before recfg finally gives
//up
#define AC_CFG_GET_STATUS_RETRY_CNT 10

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
typedef enum {
  TASK_STOPPED,
  TASK_WAIT,
  TASK_ERROR,
  TASK_START_AUTO,
  TASK_START_MANUAL,
  TASK_WAIT_MSSIM_RDY,
  TASK_WAIT_LOG_UPLOAD,
  TASK_WAIT_VPN_CONN,
  TASK_PROCESS_RETRIEVE_CFG,
  TASK_VALIDATE_AC_ID,
  TASK_VERIFY_AIRCRAFT,
  TASK_SEND_GET_CFG_LOAD_STS,
  TASK_PROCESS_GET_CFG_LOAD_STS,
  TASK_WAIT_LOG_UPLOAD_BEFORE_COMMIT,
  TASK_WAIT_FOR_COMMIT,
  TASK_COMMIT_CFG,
  TASK_REBOOTING
} AC_RECFG_TASK_STATE;

//EEPROM storage structure for the sensor values read off the data bus.
typedef struct
{
  UINT32  MagicNumber;
  FLOAT32 Type;
  FLOAT32 Fleet;
  FLOAT32 Number;
}AC_SENSOR_VALUES;

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
//static AC_FAULT_LOG          ACFaultLog;
//static BOOLEAN               m_bACUpdate;
static AC_SENSOR_VALUES      m_ACStoredSensorVals;
static BOOLEAN               m_bAircraftIDMismatch;
static INT32                 m_ReqFilesFailCnt;
static INT32                 m_UploadBeforeCommitRetryCount;
static BOOLEAN               m_bOnGround;
static BOOLEAN               m_bVPNConn;
static BOOLEAN               m_bMsConnected;
static UINT32                m_ACTaskTimeoutTmr;
#define CFG_STATUS_STR_MAX   8
static INT8                  m_CfgStatusStr[CFG_STATUS_STR_MAX];     
                                                    //The PWEH defined error code, 
                                                    //  see AC_SetCfgStatus()

//Task state defines the current activity for the task look in the AC_Task function
static AC_RECFG_TASK_STATE   m_TaskState; 
#define CFG_ERROR_STR_MAX    80              
static INT8                  m_RecfgErrorStr[CFG_ERROR_STR_MAX] = "Err: undefined";
static BOOLEAN               m_bGotCommit;
static BOOLEAN               m_bAutoCfg;
static BOOLEAN               m_ReceivingConfigCmds;
static BOOLEAN               m_CancelWhileRcvingConfigCmds;
static INT32                 m_GetStatusRetryCnt;
static MSCP_MS_REQ_CFG_FILES_RSP m_ReqCfgFilesRsp;
static MSCP_MS_GET_RECFG_STS_RSP m_ReqStsRsp;
                              
/*Task "wait" state control block: includes next state for success or error 
See AC_Wait*/
static struct {
                AC_RECFG_TASK_STATE OnSuccess;
                AC_RECFG_TASK_STATE OnError;
              }m_WaitData;

#include "AircraftConfigMgrUserTables.c"  // Include cmd tables & functions .c



/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/			  
static void    AC_MSReqCfgFilesHandler(UINT16 Id, void* PacketData, 
                               UINT16 Size, MSI_RSP_STATUS Status);
static void    AC_MSReqStartCfgRspHandler(UINT16 Id, void* PacketData, 
                               UINT16 Size, MSI_RSP_STATUS Status);
static void    AC_MSReqCfgStsRspHandler(UINT16 Id, void* PacketData, 
                               UINT16 Size, MSI_RSP_STATUS Status);


static void    AC_MSRspShellCmd(UINT16 Id, void* PacketData, UINT16 Size,
                               MSI_RSP_STATUS Status);
static void    AC_MSRspCancelCfg(UINT16 Id, void* PacketData, UINT16 Size,
                               MSI_RSP_STATUS Status);
static BOOLEAN AC_IsAircraftIDSensorsValid(void);
static void    AC_UpdateEESensorValues(FLOAT32 Type, FLOAT32 Fleet, FLOAT32 Number);
			  
static void AC_WaitMSSIMRdy(void);
static void AC_StartManual(void);
static void AC_StartAuto(void);
//static void AC_WaitLogUpload(void);
static void AC_WaitVPNConnection(void);
static void AC_ProcessRetrieveCfg(void);
static void AC_ValidateAcID(void);
static void AC_SendGetCfgLoadSts(void);
static void AC_ProcessGetCfgLoadSts(void);
static void AC_WaitLogUploadBeforeCommit(void);
static void AC_WaitForCommitCommand(void);
static void AC_CommitCfg(void);
static void AC_Rebooting(void);                                                      

static void AC_Wait(AC_RECFG_TASK_STATE Success,AC_RECFG_TASK_STATE Fail,INT8 *Str);
static void AC_Error(INT8* Str);
static void AC_Signal(BOOLEAN Success);
static void AC_SetCfgStatus(INT8* Str);
static void AC_Task ( void *pParam );

static void AC_CopyACCfgToPackedACCfg( AIRCRAFT_CONFIG_PACKED *ACCfg_Packed );			  
/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:     AircraftConfigInitialization
 *
 * Description:  Set initial state for global vars.  Setup a set of user
 *               user commands and the task control block for reconfiguration
 *               If automatic configuration is enabled, have it run initially
 *               to determine if a configuration update is necessary.
 *
 *
 * Parameters:   none
 *
 * Returns:      none
 *
 * Notes:        2551,(2063,2534 see default cfg),(2531 no),2532,
 *****************************************************************************/
void AircraftConfigInitialization ( void )
{
  // Local Data    
  TCB TaskInfo; 
   
  User_AddRootCmd(&RootMsg);
  sstrncpy(m_CfgStatusStr,AC_CLEAR);
  m_bVPNConn             = FALSE;
  m_bMsConnected         = FALSE;
  //Fake on ground if this is the default configuration.  Default configuration is
  //always in-air, so this is necessary for a box in the default cfg to be able
  //to auto configure.
  m_bOnGround            = TRUE;
  m_bAircraftIDMismatch  = FALSE;
  m_bAutoCfg             = FALSE;
  
  m_ReqFilesFailCnt   = 0;
  m_TaskState         = CfgMgr_ConfigPtr()->Aircraft.bCfgEnabled
                         ? TASK_START_AUTO : TASK_STOPPED;
  
  memset(&TaskInfo, 0, sizeof(TaskInfo));
  strncpy_safe(TaskInfo.Name, sizeof(TaskInfo.Name),"AC Cfg Loader",_TRUNCATE);
  TaskInfo.TaskID         = AC_Task_ID;
  TaskInfo.Function       = AC_Task;
  TaskInfo.Priority       = taskInfo[AC_Task_ID].priority;
  TaskInfo.Type           = taskInfo[AC_Task_ID].taskType;
  TaskInfo.modes          = taskInfo[AC_Task_ID].modes;
  TaskInfo.Enabled        = CfgMgr_ConfigPtr()->Aircraft.bCfgEnabled;
  TaskInfo.Locked         = FALSE;
  TaskInfo.Rmt.InitialMif = taskInfo[AC_Task_ID].InitialMif;
  TaskInfo.Rmt.MifRate    = taskInfo[AC_Task_ID].MIFrate;
  TaskInfo.pParamBlock    = NULL;
  TmTaskCreate(&TaskInfo);

  if(SYS_OK != NV_Open(NV_AC_CFG))
  {
    AC_FileInit();
  }
  else 
  {
    NV_Read(NV_AC_CFG,0,&m_ACStoredSensorVals,sizeof(m_ACStoredSensorVals));
    if(m_ACStoredSensorVals.MagicNumber != STPU( EE_FILE_MAGIC_NUMBER,eTpAcCfg3803))
    {
      GSE_DebugStr(NORMAL,TRUE,"Recfg: Reset stored sensor values in EE");
      AC_FileInit();
    }      
  }
}


/******************************************************************************
 * Function:     AC_FileInit
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
BOOLEAN AC_FileInit(void)
{
  m_ACStoredSensorVals.MagicNumber = EE_FILE_MAGIC_NUMBER;
  m_ACStoredSensorVals.Type        = 0;
  m_ACStoredSensorVals.Fleet       = 0;
  m_ACStoredSensorVals.Number      = 0;
  NV_Write(NV_AC_CFG, 0, &m_ACStoredSensorVals, sizeof(m_ACStoredSensorVals));
  return TRUE;
}



/******************************************************************************
 * Function:     AC_SetIsGroundServerConnected
 *
 * Description:  Call this function to inform AircraftConfig of
 *               changes in the status of the RF connection to the ground
 *               server.  The initial RfEnabled state is FALSE
 *
 * Parameters:   [in] State TRUE: RF is up and a connection to the ground
 *                                server is established
 *                          FALSE: No ground server connection is available
 *               
 *
 * Returns:      
 *
 * Notes:
 *****************************************************************************/
void AC_SetIsGroundServerConnected(BOOLEAN State)
{
  m_bVPNConn = State;
}



/******************************************************************************
 * Function:     AC_SetIsMSConnected
 *
 * Description:  Call this function to inform AircraftConfig of
 *               changes in the status of the MSSIM application on the
 *               micro-server The initial MSSIM Connected state is FALSE
 *
 * Parameters:   [in] State TRUE: MSSIM is running and ready for commands
 *                          FALSE: MSSIM is not running
 * Returns:      
 *
 * Notes:
 *****************************************************************************/
void AC_SetIsMSConnected(BOOLEAN State)
{
  m_bMsConnected = State;
}



/******************************************************************************
 * Function:     AC_SetIsOnGround
 *
 * Description:  Call this function to inform AircraftConfig of
 *               changes in the in air/on ground status of the system
 *               The initial On Ground state is FALSE.
 *               EXCEPTION: If the default configuration is loaded, the AC Mgr
 *               will always assume OnGround to allow auto cfg to run and load
 *               over the default.  This is necessary because the default
 *               configuration always assumes in-air because it is the safe
 *               state that turns off WiFi and GSM.
 *
 * Parameters:   [in] State TRUE: A/C is on the ground
 *                          FALSE: A/C is in the air
 *
 * Returns:      
 *
 * Notes:
 *****************************************************************************/
void AC_SetIsOnGround(BOOLEAN State)
{
  m_bOnGround = (CfgMgr_RuntimeConfigPtr()->VerId == 0) ? TRUE:State;
}



/******************************************************************************
 * Function:     AC_GetConfigStatus
 *
 * Description:  Get the ConfigStatus string that contains the result of
 *               the last reconfiguration.
 *
 * Parameters:   [in/out] str: Pointer to a location to store the 7-byte
 *                             config status string.
 *
 * Returns:      none
 *
 * Notes:
 *****************************************************************************/
void AC_GetConfigStatus(INT8* str)
{
  strncpy_safe(str,(CFG_STATUS_STR_MAX-1),m_CfgStatusStr,_TRUNCATE);
}



/******************************************************************************
 * Function:     AircraftConfig_GetConfigState
 *
 * Description:  Get the configuration status of the aircraft.  The possible
 *               states are no configuration, configured, retrieving
 *               configuration
 *
 * Parameters:   none
 *
 * Returns:      AC_CFG_STATE_NO_CONFIG  - No configuration has been loaded,
 *                                         or the configuration is invalid
 *               AC_CFG_STATE_CONFIG_OK  - The system has a valid configuration
 *               AC_CFG_STATE_RETRIEVING - The A/C config is currently retrieving
 *                                         configuration information from MSSIM
 *                                         or it is actively being configured.
 *
 * Notes:
 *****************************************************************************/
AIRCRAFT_CONFIG_STATE AC_GetConfigState(void)
{
  AIRCRAFT_CONFIG_STATE ACCfgState = AC_CFG_STATE_NO_CONFIG;
  
    // Verify Configuration
  if ( (CfgMgr_RuntimeConfigPtr()->VerId != 0) && !m_bAircraftIDMismatch)
  {
    // Configuration is good
    ACCfgState = AC_CFG_STATE_CONFIG_OK;
  }
  //If waiting for the micro-server to retrieve the configuration files...
  if( m_TaskState == TASK_WAIT &&
      m_WaitData.OnSuccess == TASK_PROCESS_RETRIEVE_CFG )
  {
    ACCfgState = AC_CFG_STATE_RETRIEVING;    
  }

  return ACCfgState;  
}



/******************************************************************************
 * Function:     AircraftConfig_StartAutoRecfg
 *
 * Description:  Start an automatic reconfiguration.  This retrieves the
 *               FAST config files through the Micro-Server and updates the
 *               local configuration if the retrieved version is newer than
 *               the local version.
 *
 * Parameters:   none
 *
 * Returns:      TRUE:  Reconfig started successfully.
 *               FALSE: Not in the correct state to start reconfig
 *
 * Notes:
 *****************************************************************************/
BOOLEAN AC_StartAutoRecfg(void)
{
  BOOLEAN result = FALSE;
  
  if((m_TaskState == TASK_STOPPED) || (m_TaskState == TASK_ERROR))
  {
    TmTaskEnable(AC_Task_ID, TRUE);
    m_TaskState = TASK_START_AUTO;
    result = TRUE;
  }
  return result;
}



/******************************************************************************
 * Function:     AircraftConfig_StartManualRecfg
 *
 * Description:  Start a manual reconfiguration.  It is expected that the
 *               .xml and .cfg files are in the manual reconfiguration folder
 *               on the micro-server.
 *
 * Parameters:
 *
 * Returns:      TRUE: Reconfig started successfully
 *               FALSE: Not in the correct state to start reconfig
 *
 * Notes:
 *****************************************************************************/
BOOLEAN AC_StartManualRecfg(void)
{
  BOOLEAN retval = FALSE;
  
  if((m_TaskState == TASK_STOPPED) || 
     (m_TaskState == TASK_ERROR)   ||
     (m_TaskState == TASK_WAIT_MSSIM_RDY) ||
     (m_TaskState == TASK_WAIT_LOG_UPLOAD)||
     (m_TaskState == TASK_WAIT_VPN_CONN)  
     )
  {
    m_ReqFilesFailCnt = 0;
    m_TaskState = TASK_START_MANUAL;
    TmTaskEnable(AC_Task_ID, TRUE);
    retval = TRUE;
  }
  return retval;
}



/******************************************************************************
 * Function:     AircraftConfig_SendClearCfgDir
 *
 * Description:  Send a shell command to clear the configuration directory
 *               in the tmp configuration folder
 *
 * Parameters:
 *
 * Returns:      TRUE: Command sent successfully, check recfg.status to
 *                     ensure the operation completes successfully.
 *               FALSE: Not in the correct state to send the command
 *
 * Notes:
 *****************************************************************************/
BOOLEAN AC_SendClearCfgDir(void)
{
  MSCP_CP_SHELL_CMD cmd;
  BOOLEAN retval  = FALSE;
  
  if((m_TaskState == TASK_STOPPED) || 
     (m_TaskState == TASK_ERROR)   ||
     (m_TaskState == TASK_WAIT_MSSIM_RDY) ||
     (m_TaskState == TASK_WAIT_LOG_UPLOAD)||
     (m_TaskState == TASK_WAIT_VPN_CONN)  
     )
  {
    strncpy_safe(cmd.Str,sizeof(cmd.Str),"rm [MANUAL_CFG]/*",_TRUNCATE);

    cmd.Len = strlen(cmd.Str) + 1;

    AC_Wait(TASK_STOPPED,TASK_ERROR,
              "MS returned fail clear cfg dir");
  
    if(SYS_OK != MSI_PutCommand(CMD_ID_SHELL_CMD,&cmd,
        sizeof(cmd)-sizeof(cmd.Str)+cmd.Len,5000,AC_MSRspShellCmd))
    {
      AC_Error("Could not send MS command, clear cfg dir");
      m_TaskState = TASK_ERROR;
    }
    retval = TRUE;
  }

  return retval;
}



/******************************************************************************
 * Function:     AircraftConfig_CancelRecfg()
 *
 * Description:  Cancel a reconfiguration in progress.  This will stop the
 *               load of data from the micro server, then restore the
 *               reconfiguration to the state before the reconfiguration
 *               started.  This can only be executed while the task is running
 *               and not in the process of committing the configuration or
 *               rebooting the FAST box.
 *               
 *
 * Parameters:   none
 *
 * Returns:      TRUE: Stop task command sent successfully. Check recfg.status
 *                     to ensure the task stops.
 *               FALSE: Not in the correct state to issue a cancel request
 *
 * Notes:
 *****************************************************************************/
BOOLEAN AC_CancelRecfg(void)
{
  BOOLEAN retval = FALSE;
  AC_RECFG_TASK_STATE save;
  
  if( (m_TaskState != TASK_STOPPED)     ||
      (m_TaskState != TASK_ERROR)       ||
      (m_TaskState != TASK_COMMIT_CFG)  ||
      (m_TaskState != TASK_REBOOTING)  )
  {
    save = m_TaskState;
    AC_Wait(TASK_STOPPED,TASK_ERROR,
              "MS returned fail cancel reconfig");
     
    if( SYS_OK != MSI_PutCommand(CMD_ID_REQ_STOP_RECFG,NULL,0,5000,
        AC_MSRspCancelCfg) )
    {
      //If it is not possible to send the command, setting this flag will
      //cause the process to stop immediatly after all the .cfg commands have
      //come down from the Micro_Server instead.  Restore state to continue
      //polling for completion of the .cfg commands.  If not receving .cfg
      //commands, then this failure is not important, just stop.
      m_TaskState = m_ReceivingConfigCmds ? save : TASK_STOPPED;
      m_CancelWhileRcvingConfigCmds = TRUE;
      
    }
    retval = TRUE;
  }
  return retval;
}



/******************************************************************************
 * Function:     AC_FSMAutoRun()         | IMPLEMENTS GetState() INTERFACE to 
 *                                       | FSM
 *
 * Description:  Run the automatic reconfiguration process
 *
 * Parameters:  [in] Run: TRUE: Run the Auto Cfg task
 *                        FALSE: Stop the Auto Cfg task
 *              [in] param: not used, just to match FSM call signature.
 *
 * Returns:      
 *
 * Notes:
 *****************************************************************************/
void AC_FSMAutoRun(BOOLEAN Run, INT32 param)
{
  if(Run)
  {
    //return ignored, FSM won't run the task if it is not able to run.
    AC_StartAutoRecfg();
  }
  else
  {
    //return ignored, stopped/running is rechecked by FSM (GetState)
    AC_CancelRecfg();
  }
}


/******************************************************************************
 * Function:     AC_FSMGetState()        | IMPLEMENTS GetState() INTERFACE to 
 *                                       | FSM
 *
 * Description:  Return the running/not running state of the auto-configuration
 *               process.  The task is not running if it is in the STOPPED or
 *               or ERROR state, else it is running.
 *
 * Parameters:   [in] param : Not used, for matching FSM call sig.
 *
 * Returns:      
 *
 * Notes:
 *****************************************************************************/
BOOLEAN AC_FSMGetState(INT32 param)
{
  return !((m_TaskState == TASK_STOPPED) || (m_TaskState == TASK_ERROR)) ;
}



/*******************************************************************************
 * Function:     AC_Task
 *
 * Description:  Aircraft reconfiguration process for automatic and
 *               manual reconfiguration.  The task handles initiating log
 *               upload, micro-server command/response, logging and reporting
 *               of the process, and storing configurations to eeprom.
 *               
 *
 * Parameters:   
 *
 * Returns:      
 *
 * Notes:
 ******************************************************************************/
void AC_Task ( void *pParam )
{
  AIRCRAFT_AUTO_ERROR_LOG log;


  switch(m_TaskState)
  {
    case TASK_STOPPED:
      CfgMgr_CancelBatchCfg();
      TmTaskEnable(AC_Task_ID, FALSE);    
    break;
    
    case TASK_ERROR:
      if(m_bAutoCfg)
      {
        memset(log,0,sizeof(log));
        sstrncpy(log,m_RecfgErrorStr);
        LogWriteSystem(APP_AT_INFO_AUTO_ERROR_DESC, LOG_PRIORITY_LOW,
              log, sizeof(log),NULL);
      }
      CfgMgr_CancelBatchCfg();
      TmTaskEnable(AC_Task_ID, FALSE);
    break;

    case TASK_WAIT:
      //Do Nothing
    break;
    
    case TASK_START_AUTO:
      AC_StartAuto();
    break;
    
    case TASK_START_MANUAL:
      AC_StartManual();
    break;
    
    case TASK_WAIT_MSSIM_RDY:
      AC_WaitMSSIMRdy();
    break;
    
    case TASK_WAIT_VPN_CONN:
      AC_WaitVPNConnection();
    break;

    case TASK_PROCESS_RETRIEVE_CFG:
      AC_ProcessRetrieveCfg();
    break;

    case TASK_VALIDATE_AC_ID:
      AC_ValidateAcID();
    break;
        
    case TASK_SEND_GET_CFG_LOAD_STS:
      AC_SendGetCfgLoadSts();
    break;
    
    case TASK_PROCESS_GET_CFG_LOAD_STS:
      AC_ProcessGetCfgLoadSts();
    break;

    case TASK_WAIT_LOG_UPLOAD_BEFORE_COMMIT:
      AC_WaitLogUploadBeforeCommit();
    break;

    case TASK_WAIT_FOR_COMMIT:
      AC_WaitForCommitCommand();
    break;
    
    case TASK_COMMIT_CFG:
      AC_CommitCfg();
    break;
    
    case TASK_REBOOTING:
      AC_Rebooting();
    break;
    
    default:
      FATAL("Unsupported AC_RECFG_TASK_STATE = %d", m_TaskState );
    break;
  }
}



/*******************************************************************************
 * Function:     AC_StartAuto | TASK STATE
 *
 * Description:  Initial state for performing an automatic configuration
 *               check and reconfigure.  Preset the GotCommit flag TRUE
 *               to save the configuration without prompting the GSE port
 *               user.
 *               NEXT STATE:
 *                  WAIT_MSSIM_RDY
 *
 * Parameters:   
 *
 * Returns:      
 *
 * Notes:
 ******************************************************************************/
static void AC_StartAuto(void)
{
  //Preset commit to true because automatic mode does not wait for user
  //confirmation to commit the new config
  m_ReqFilesFailCnt = 0;
  m_bAutoCfg        = TRUE;
  m_GetStatusRetryCnt = AC_CFG_GET_STATUS_RETRY_CNT; 
  m_ReceivingConfigCmds = FALSE;
  m_CancelWhileRcvingConfigCmds = FALSE;
  m_TaskState = TASK_WAIT_MSSIM_RDY;
  GSE_DebugStr(NORMAL,TRUE,"ReCfg: Starting auto configuration check....");
  
}



/*******************************************************************************
 * Function:     AC_StartManual | TASK STATE
 *
 * Description:  Initial state for performing a manual configuration where
 *               the configuration data has already been uploaded through the
 *               GSE port to the Micro-Server.  Start reconfiguration by
 *               commanding the Micro-Server with the "Start Recfg" command.
 *               NEXT STATE:
 *                  WAIT <then> SEND_GET_CFG_LOAD_STS.
 *               
 *               
 *
 * Parameters:   
 *
 * Returns:      
 *
 * Notes:
 ******************************************************************************/
static void AC_StartManual(void)
{
  MSCP_CP_REQ_START_RECFG_CMD cmd;
  
  memset(&cmd,0,sizeof(cmd));
  memset(&m_ReqStsRsp,0,sizeof(m_ReqStsRsp));

  m_bGotCommit = FALSE;
  m_bAutoCfg  = FALSE;
  m_GetStatusRetryCnt = AC_CFG_GET_STATUS_RETRY_CNT;
  m_ReceivingConfigCmds = FALSE;
  m_CancelWhileRcvingConfigCmds = FALSE;  
  CfgMgr_StartBatchCfg();

  //Setup wait state, this stores what state to go to after the micro-server
  //responds
  AC_Wait(TASK_SEND_GET_CFG_LOAD_STS,TASK_ERROR,
         "MS responded error to Start Recfg");

  Box_GetSerno(cmd.CP_SN);
  cmd.Mode    = MSCP_START_RECFG_MANUAL;
  m_ReceivingConfigCmds = TRUE;
  if(SYS_OK != MSI_PutCommand(CMD_ID_REQ_START_RECFG,
     &cmd,sizeof(cmd),5000,&AC_MSReqStartCfgRspHandler) )
  {
    CfgMgr_CancelBatchCfg();
    m_ReceivingConfigCmds = FALSE;
    AC_Error("Could not send MS cmd: Get Start Recfg");
  }
  GSE_DebugStr(NORMAL,TRUE,"ReCfg: Starting manual reconfiguration");  
}



/*******************************************************************************
 * Function:     AC_WaitMSSIMRdy | TASK STATE
 *
 * Description:  Wait for the Micro-Server to be available.  This is used
 *               during automatic reconfiguration, not manual.  After, wait
 *               for VPN connection to be available
 *               NEXT STATE:
 *                 WAIT_VPN_CONN
 * Parameters:   
 *
 * Returns:      
 *
 * Notes:
 ******************************************************************************/
static void AC_WaitMSSIMRdy(void)
{
  if(m_bMsConnected)
  {
    m_TaskState = TASK_WAIT_VPN_CONN;
  }
  //Set server not found error if VPN fails to connect before takeoff
  else if(!m_bOnGround)  
  {
    AC_SetCfgStatus(AC_XML_SERVER_NOT_FOUND_STR);
    AC_Error("In Air/Recording before VPN connected");
  }  
}



/*******************************************************************************
 * Function:     AC_WaitVPNConnection | TASK STATE
 *
 * Description:  Wait for indication that the Micro-Server has established
 *               its connection to the ground server.  After the connection
 *               is ready, request the box's configuration from JMessenger
 *               on the Micro-Server
 *                NEXT STATE:
 *                 WAIT_RETRIEVE_CFG
 *              
 * Parameters:   
 *
 * Returns:      
 *
 * Notes:        2885,2780,2886,2639,2530
 ******************************************************************************/
static void AC_WaitVPNConnection(void)
{
  MSCP_CP_REQ_CFG_FILES_CMD cmd;
  CFGMGR_NVRAM *cfg = CfgMgr_ConfigPtr();
    
  if(m_bVPNConn)
  {
    AC_Wait(TASK_PROCESS_RETRIEVE_CFG,TASK_ERROR,
           "MS responded error to Get Cfg Files");

    Box_GetSerno(cmd.CP_SN);
    sstrncpy(cmd.Type,cfg->Aircraft.Type);
    sstrncpy(cmd.FleetIdent,cfg->Aircraft.FleetIdent);
    sstrncpy(cmd.Number,cfg->Aircraft.Number);
    sstrncpy(cmd.Style,cfg->Aircraft.Style);
    sstrncpy(cmd.Operator,cfg->Aircraft.Operator);
    sstrncpy(cmd.Owner,cfg->Aircraft.Owner);
    sstrncpy(cmd.TailNumber,cfg->Aircraft.TailNumber);
    cmd.ValidateData = cfg->Aircraft.bValidateData;
    
    if(SYS_OK != MSI_PutCommand(CMD_ID_REQ_CFG_FILES,
       &cmd,sizeof(cmd),AC_REQ_FILES_FROM_GROUND_TO,&AC_MSReqCfgFilesHandler) )
    {
      AC_Error("Could not send MS cmd: Get Start Recfg");
    }    
  }
  //Set server not found error if VPN fails to connect before takeoff
  else if(!m_bOnGround)  
  {
    AC_SetCfgStatus(AC_XML_SERVER_NOT_FOUND_STR);
    AC_Error("In Air/Recording before VPN connected");
  }
}



/*******************************************************************************
 * Function:     AC_ProcessRetrieveCfg | TASK STATE
 *
 * Description:  After a the micro-server responds to the Request Cfg Files
 *               command, process the result.
 *               1) Examine the error codes, retry 2x on error
 *               2) Examine the XML data match and Cfg version
 *                  a)If XML data is different, or cfg version is newer, load
 *                    new configuration.
 *               3) If aircraft is not on ground, cancel cfg load
 *
 * Parameters:   
 *
 * Returns:      
 *
 * Notes:        2536,2942,2943,2664,2944,2548,2640,2641,3005,2911,2567,2550,
 *               2530
 ******************************************************************************/
static void AC_ProcessRetrieveCfg(void)
{
  BOOLEAN file_error = FALSE;
  MSCP_CP_REQ_START_RECFG_CMD cmd;
  
  //Check error codes from the configuration file retrieval process
  if(0 != strncmp(m_ReqCfgFilesRsp.CfgStatus,AC_DEFAULT_NO_ERR,
      sizeof(m_ReqCfgFilesRsp.CfgStatus)))
  {
    GSE_DebugStr(NORMAL,TRUE,"Recfg: Cfg file %s",m_ReqCfgFilesRsp.CfgStatus);
    AC_SetCfgStatus(m_ReqCfgFilesRsp.CfgStatus);
    file_error = TRUE;
  }
  if(0 != strncmp(m_ReqCfgFilesRsp.XMLStatus,AC_DEFAULT_NO_ERR,
      sizeof(m_ReqCfgFilesRsp.CfgStatus)))
  {
    GSE_DebugStr(NORMAL,TRUE,"Recfg: XML file %s",m_ReqCfgFilesRsp.XMLStatus);    
    AC_SetCfgStatus(m_ReqCfgFilesRsp.XMLStatus);
    file_error = TRUE;
  }
  
  if(!m_bOnGround)
  {
    GSE_DebugStr(NORMAL,TRUE,"ReCfg: Went in-air while waiting for cfg files");
    AC_Error("In-Air/Recording while retrieving cfg files");
  }
  else if(!file_error)
  { //Check if XML data is different,if the cfg file version is newer,
    //or validation of a/c data on the bus does not match.
    if(!m_ReqCfgFilesRsp.XMLDataMatch ||
        (m_ReqCfgFilesRsp.CfgFileVer > CfgMgr_ConfigPtr()->VerId) )
    {
      GSE_DebugStr(NORMAL,TRUE,"Recfg: %s %s",
          m_ReqCfgFilesRsp.XMLDataMatch ? "" : "XML changed,",
          (m_ReqCfgFilesRsp.CfgFileVer > CfgMgr_ConfigPtr()->VerId) ? 
            "cfg ver higher," : "");

      //Start reconfiguration
      CfgMgr_StartBatchCfg();
      AC_Wait(TASK_SEND_GET_CFG_LOAD_STS,TASK_ERROR,
             "MS responded error to Start Recfg");
      Box_GetSerno(cmd.CP_SN);
      cmd.Mode    = MSCP_START_RECFG_AUTO;
      m_ReceivingConfigCmds = TRUE;
      if(SYS_OK != MSI_PutCommand(CMD_ID_REQ_START_RECFG,
         &cmd,sizeof(cmd),5000,&AC_MSReqStartCfgRspHandler) )
      {
        CfgMgr_CancelBatchCfg();
        m_ReceivingConfigCmds = FALSE;
        AC_Error("Could not send MS cmd: Get Start Recfg");
      }
    }
    else
    {
      GSE_DebugStr(NORMAL,TRUE,"Recfg: Cfg current, validating AC identification...");
      m_TaskState = TASK_VALIDATE_AC_ID;
    }

  }
  else
  {
    //On file retrieval error, go back to previous state to resend the retrieve
    //files command.  If error count exceeded, upload logs, quit.
    if(m_ReqFilesFailCnt++ < 2)
    {
      m_TaskState = TASK_WAIT_VPN_CONN;
    }
    else
    {
      UploadMgr_StartUpload(UPLOAD_START_RECFG);
      AC_Error("Recfg: File retrieval failed");
    }        
  }
}

/*******************************************************************************
 * Function:     AC_ValidateAcID | TASK STATE
 *
 * Description:  Verify the FAST box is on the expected aircraft (if Validate
 *               Data is enabled)  Check the sensor for aircraft type,
 *               aircraft fleet ident, and aircraft number and compare against
 *               the stored data from the XML file.
 *               1) Data matches, stop reconfiguration task
 *               2) Data does not match, set configuration status, write
 *                  mismatch log and upload logs.  Go to error state
 *               3) Data ready (sensor(s)) invalid, wait until sensors
 *                  are valid.
 *
 *
 *               NEXT STATE:
 *                TASK_STOPPED, TASK_ERROR
 *
 *
 * Parameters:   none
 *
 * Returns:      none
 *
 * Notes:        2910,2646,2540,2907,2545,2541,2542,2908,2645,2544,2545
 ******************************************************************************/
static void AC_ValidateAcID(void)
{
  AIRCRAFT_CONFIG   *ACCfg = &CfgMgr_RuntimeConfigPtr()->Aircraft;
  AIRCRAFT_MISMATCH_LOG      ACMismatchLog;
  FLOAT32 TypeSensorVal;
  FLOAT32 FleetSensorVal;
  FLOAT32 ACNumberSensorVal;
  
  if(ACCfg->bValidateData)
  {
    if(AC_IsAircraftIDSensorsValid())
    {
      TypeSensorVal     = SensorGetValue( ACCfg->TypeSensorIndex   );
      FleetSensorVal    = SensorGetValue( ACCfg->FleetSensorIndex  );
      ACNumberSensorVal = SensorGetValue( ACCfg->NumberSensorIndex );

      AC_UpdateEESensorValues(TypeSensorVal,FleetSensorVal,ACNumberSensorVal);    

      if( ( (UINT32)(TypeSensorVal+0.5)     == strtol(ACCfg->Type,NULL,10)       )&& 
          ( (UINT32)(FleetSensorVal+0.5)    == strtol(ACCfg->FleetIdent,NULL,10) )&&
          ( (UINT32)(ACNumberSensorVal+0.5) == strtol(ACCfg->Number,NULL,10)     )  )           
      {
        m_TaskState = TASK_STOPPED;
        m_bAircraftIDMismatch = FALSE;
        GSE_DebugStr(NORMAL,TRUE,"Recfg: AC data validated, recfg complete!");        
      }
      else
      {
        AC_UpdateEESensorValues(TypeSensorVal,FleetSensorVal,ACNumberSensorVal);    
//#pragma ghs nowarning 1545 //Suppress packed structure alignment warning
//      memcpy(&ACMismatchLog.Config,ACCfg,sizeof(*ACCfg));
//#pragma ghs endnowarning 
 
        // Configurations stored in EEPROM are not PACKED, 
        //   however log structures are PACKED.  The following func 
        //   is a "non-elegant" or "cheesy way of dealing with this
        //   mis-alignment.  
        AC_CopyACCfgToPackedACCfg( &ACMismatchLog.Config ); 

        ACMismatchLog.TypeValue   = TypeSensorVal;
        ACMismatchLog.FleetValue  = FleetSensorVal;
        ACMismatchLog.NumberValue = ACNumberSensorVal;
        LogWriteSystem(APP_AT_INFO_AC_ID_MISMATCH, LOG_PRIORITY_LOW,
                      &ACMismatchLog,sizeof(ACMismatchLog),NULL);

        AC_SetCfgStatus(AC_ID_MISMATCH_STR);
        UploadMgr_StartUpload(UPLOAD_START_RECFG);
        //use strtol value b/c this was the value used in the comparison above
        //Bypass the AC_Error() function to avoid writing an two logs  for
        //the same failure
        snprintf(m_RecfgErrorStr,sizeof(m_RecfgErrorStr),
            "Err: Validate (Type, Fleet, Num) exp %d %d %d got %1.0f %1.0f %1.0f",
            strtol(ACCfg->Type,NULL,10),        
            strtol(ACCfg->FleetIdent,NULL,10),
            strtol(ACCfg->Number,NULL,10),TypeSensorVal,
            FleetSensorVal,ACNumberSensorVal);
        CfgMgr_CancelBatchCfg();
        TmTaskEnable(AC_Task_ID, FALSE);
        m_TaskState = TASK_ERROR;
        GSE_DebugStr(NORMAL,TRUE,"Recfg %s",m_RecfgErrorStr);        
        m_bAircraftIDMismatch = TRUE;        
      }
    }
  }
  else
  { //If data validation is disabled, clear Stored sensor values.
    GSE_DebugStr(NORMAL,TRUE,"Recfg: Validate data disabled, recfg complete!");
    AC_UpdateEESensorValues(0,0,0);
    AC_SetCfgStatus(AC_CFG_NO_VALIDATE_STR);
    m_bAircraftIDMismatch = FALSE;
    m_TaskState = TASK_STOPPED;
  }  
}



/*******************************************************************************
 * Function:     AC_SendGetCfgLoadSts | TASK STATE
 *
 * Description:  Send the GET_CFG_LOAD_STS command to the micro-server, then
 *               wait for a response.  The timer is used to resend the command
 *               every 1 second after it is sent.
 *               NEXT STATE:
 *                WAIT <then> PROCESS_GET_CFG_LOAD_STS
 *
 * Parameters:   
 *
 * Returns:      
 *
 * Notes: 
 ******************************************************************************/
static void AC_SendGetCfgLoadSts(void)
{
  //Check if the aircraft went in air, ONLY if automatic reconfigure
  if(!m_bOnGround && m_bAutoCfg && !m_CancelWhileRcvingConfigCmds)
  {
    GSE_DebugStr(NORMAL,TRUE,"ReCfg: Went in-air/recording while loading cfg");
    //Note: will set same error for bad or good response, probably okay
    AC_Wait(TASK_ERROR,TASK_ERROR,
              "In-Air/Recording while loading cfg");
    if( SYS_OK != MSI_PutCommand(CMD_ID_REQ_STOP_RECFG,NULL,0,5000,
        AC_MSRspCancelCfg) )
    {
      //If a command to stop reconfig cannot be set to the MS, then we
      //must wait for the MS to stop sending .cfg commands before 
      //quitting.  Keep polling until MS says it is done.
      m_CancelWhileRcvingConfigCmds = TRUE;
      m_TaskState = TASK_SEND_GET_CFG_LOAD_STS;
    }
  }
  else
  {
    GSE_DebugStr(VERBOSE,TRUE,"ReCfg: Sending Get Load Sts to MS");

    AC_Wait(TASK_PROCESS_GET_CFG_LOAD_STS,TASK_ERROR,
          "MS responded error to Get Recfg Sts");
    if(SYS_OK != MSI_PutCommand(CMD_ID_GET_RECFG_STS,NULL,0,1000,
        AC_MSReqCfgStsRspHandler))
    {
      //Could not send status cmd.  Jump back to ProcessGetCfg, it will
      //see the status from last time, then jump back to this state in
      //1 second.
      m_TaskState = m_GetStatusRetryCnt-- == 0 ? TASK_PROCESS_GET_CFG_LOAD_STS : 
        TASK_ERROR, AC_Error("Could not send MS cmd: Get Recfg Sts");    
    }
      
    //Start timeout to send the get status command again in 1 second
    Timeout(TO_START,TICKS_PER_Sec*1,&m_ACTaskTimeoutTmr);
  }
}



/*******************************************************************************
 * Function:     AC_ProcessGetCfgLoadSts | TASK STATE
 *
 * Description:  Process the response to the micro server command to get
 *               reconfiguration status.
 *               1) If the micro-server indicates the reconfig is working,
 *                  set a timeout to resend the command 1 second from now
 *               2) If the micro-server indicates the reconfig is complete,
 *                  start uploading log data from the CP to the MS
 *               3) If an error occurs, cancel batch reconfig and set the
 *                  error status global.
 *               4) If the aircraft is in air, cancel reconfiguration
 *
 *            
 *
 * Parameters:   
 *
 * Returns:      
 *
 * Notes:        3240,2530
 ******************************************************************************/
static void AC_ProcessGetCfgLoadSts(void)
{
  //MS is still working, continue poll.
  if(m_ReqStsRsp.Status == MSCP_RECFG_STS_WORKING)
  {
    if(Timeout(TO_CHECK,TICKS_PER_Sec*1,&m_ACTaskTimeoutTmr))
    { //1 second up, send another status poll
      m_TaskState = TASK_SEND_GET_CFG_LOAD_STS;
    }
  }
  //MS is done working, upload logs.
  else if(m_ReqStsRsp.Status == MSCP_RECFG_STS_DONE)
  {
    if(m_CancelWhileRcvingConfigCmds)
    {
      AC_Error("In-Air/Recording while loading cfg");
    }
    else
    {
      m_ReceivingConfigCmds = FALSE;
      AC_SetCfgStatus(AC_CLEAR);
      GSE_DebugStr(NORMAL,TRUE,"ReCfg: Load Success, uploading logs to MS...");
      UploadMgr_StartUpload(UPLOAD_START_RECFG);
      m_UploadBeforeCommitRetryCount = 0;
      //If recfg got a cancel request while reconfiguring, then go to stopped
      //and throw out the data just received from the MS.
      m_TaskState = TASK_WAIT_LOG_UPLOAD_BEFORE_COMMIT ;
    }
  }
  //MS error occurred, abort transfer.
  else if(m_ReqStsRsp.Status == MSCP_RECFG_STS_ERROR)
  {
    CfgMgr_CancelBatchCfg();
    AC_SetCfgStatus(m_ReqStsRsp.ErrorStr);
    AC_Error(m_ReqStsRsp.StatusStr);
    GSE_DebugStr(VERBOSE,TRUE,"ReCfg: MS Sts Err: %s %s",
        m_ReqStsRsp.StatusStr,m_ReqStsRsp.ErrorStr);  
  }
}



/*******************************************************************************
 * Function:     AC_WaitLogUploadBeforeCommit | TASK STATE
 *
 * Description:  Wait until the log upload process completes.  If the log 
 *               memory is empty, go wait for the commit new configuration
 *               command.
 *
 * Parameters:   none
 *
 * Returns:      
 *
 * Notes:        3195, 2530
 ******************************************************************************/
static void AC_WaitLogUploadBeforeCommit(void)
{
  //Check if the aircraft went in air, ONLY if automatic reconfigure
  if(!m_bOnGround && m_bAutoCfg)
  {
    AC_Error("In-Air before commit to EEPROM");
  }
  else if(LogIsLogEmpty() && !UploadMgr_IsUploadInProgress())
  {
    m_TaskState = TASK_WAIT_FOR_COMMIT;
  }
  else if(!LogIsLogEmpty() && !UploadMgr_IsUploadInProgress())
  {
    //If upload fails to clear logs, it might be due to another task writing
    //a log in while uploading, try 3x then give up.
    if(m_UploadBeforeCommitRetryCount++ < 3)
    {
      //Pause log writes for one minute to allow upload to clear the log memory      
      LogPauseWrites(TRUE,60);  
      UploadMgr_StartUpload(UPLOAD_START_RECFG);
    }
    else
    {
      AC_SetCfgStatus(AC_CFG_FAILED_TO_UPDATE_STR);
      AC_Error("Log Upload failed, could not erase log memory");
    }
  }
  //else if(LogIsEmpty && UploadInProgress)||(!LogIsEmpty && UploadInProgress)
  //{
    //Do Nothing if upload is still in progress.
  //}

    
  
  //Error
}



/*******************************************************************************
 * Function:     AC_WaitForCommitCommand | TASK STATE
 *
 * Description:  Wait for the GotCommit if in manual mode flag.  Commit
 *               immediately in automatic mode.  GotCommit flag is set from
 *               the recfg.commit command.
 *
 * Parameters:   
 *
 * Returns:      
 *
 * Notes:        2099
 ******************************************************************************/
static void AC_WaitForCommitCommand(void)
{
  //Log upload is complete and log memory is empty, commit configuration
  if(m_bGotCommit || m_bAutoCfg)
  {
    GSE_DebugStr(NORMAL,TRUE,"ReCfg: Committing cfg to EEPROM");
    CfgMgr_CommitBatchCfg();
    m_TaskState = TASK_COMMIT_CFG;
  }
  
}



/*******************************************************************************
 * Function:     AC_CommitCfg | TASK STATE
 *
 * Description:  Wait for the new configuration to finish writing to EEPROM,
 *               then go to reboot mode.
 *
 * Parameters:   
 *
 * Returns:      
 *
 * Notes:        3645
 ******************************************************************************/
static void AC_CommitCfg(void)
{
  if(!CfgMgr_IsEEWritePending())
  {
    GSE_DebugStr(VERBOSE,TRUE,
        "ReCfg: Commit to EEPROM complete, rebooting in 10s.....");    
    m_TaskState = TASK_REBOOTING;
    Timeout(TO_START,TICKS_PER_Sec*10,&m_ACTaskTimeoutTmr);
  }
}



/******************************************************************************
 * Function:     AC_Rebooting | TASK STATE
 *
 * Description:  Wait for 10 seconds, then reboot.
 *               NEXT STATE:
 *                 <NONE>
 *
 * Parameters:   
 *
 * Returns:      
 *
 * Notes:
 *****************************************************************************/
static void AC_Rebooting(void)
{
  if(Timeout(TO_CHECK,TICKS_PER_Sec*10,&m_ACTaskTimeoutTmr))
  {
    GSE_DebugStr(VERBOSE,TRUE,"ReCfg: Waiting for dog to bark");    
    WatchdogReboot(TRUE);
  }
}




/******************************************************************************
 * Function:    AC_Wait
 *
 * Description: Used when the reconfiguration process needs to wait for a 
 *              response from the micro-server.  The command response that 
 *              the task is waiting for calls the AC_Signal() function to go to the
 *              next state when the response is received
 *              
 * Parameters:  [in] Success: State to enter when a successful response is
 *                            received
 *              [in] Fail:    State to enter when a fail response is received.
 *              [in] Str:     Error string for when the Fail state is ERROR
 *                            This allows the caller to preset the error string
 *                            so it won't need to be set when calling
 *                            AC_Signal(FALSE).  Set to NULL if not used
 * Returns:     none
 *
 * Notes:       
 *
 *****************************************************************************/
static void AC_Wait(AC_RECFG_TASK_STATE Success,AC_RECFG_TASK_STATE Fail,INT8 *Str)
{  
  m_WaitData.OnSuccess = Success;
  m_WaitData.OnError   = Fail;
  m_TaskState = TASK_WAIT;
  if(Str != NULL)
  {
    snprintf(m_RecfgErrorStr, CFG_ERROR_STR_MAX,"Err: %s",Str);
  }
  
}

/******************************************************************************
 * Function:    AC_Signal
 *
 * Description: Used to signal the aircraft task that is waiting for that
 *              a response was received from the micro-server, or that
 *              a timeout occurred while waiting for a response. If the task
 *              is not in "WAIT" no action is taken
 *              
 * Parameters:  [in] Success: TRUE:  Operation successful
 *                            FALSE: Operation failed
 *
 * Returns:     none
 *
 * Notes:       
 *
 *****************************************************************************/
static void AC_Signal(BOOLEAN Success)
{
  if(m_TaskState == TASK_WAIT)
  {  
    if(Success)
    {
      m_TaskState = m_WaitData.OnSuccess;
    }
    else //Do not remove else{} Thanks,
    {
      m_TaskState = m_WaitData.OnError;
    }
  }
}



/******************************************************************************
 * Function:    AC_Error
 *
 * Description: Saves a descriptive string describing the condition that caused 
 *              the error.  Set the task state to ERROR.  This will stop
 *              the activity in progress and the GSE status command will
 *              contain the error string passed in Str.
 *
 *              
 * Parameters:  [in]: Str: Description of the error
 *                         must be less than 72 chars.
 *
 * Returns:     none
 *
 * Notes:       
 *
 *****************************************************************************/
static void AC_Error(INT8* Str)
{
  snprintf(m_RecfgErrorStr,sizeof(m_RecfgErrorStr),"Err: %s",Str);  
  m_TaskState = TASK_ERROR;
}



/******************************************************************************
 * Function:    AC_SetCfgStatus
 *
 * Description: Sets the global configuration status string and writes a log.
 *              1) If the global status string is no already set to an
 *                 ERR### code, set Str as the configuration status.
 *              2) Write a system log containing the ERR### string
 *              
 * Parameters:  [in] Str: ERR### string to set in the configuration status
 *
 * Returns:     none
 *
 * Notes:       2561,2909,2552,2562,2565,2595,2644
 *
 *****************************************************************************/
static void AC_SetCfgStatus(INT8* Str)
{
  if(0 != strncmp(m_CfgStatusStr,"ERR",3))
  {
    memset(m_CfgStatusStr,0,sizeof(m_CfgStatusStr));
    strncpy_safe(m_CfgStatusStr,sizeof(m_CfgStatusStr),Str,_TRUNCATE);
  }
  LogWriteSystem(APP_AT_INFO_CFG_STATUS, LOG_PRIORITY_LOW,
        Str, AC_CFG_STS_LOG_SIZE,NULL);
}



/******************************************************************************
 * Function:    AC_MSReqStartCfgRspHandler | MS Response Handler
 *
 * Description: Handle response to the CMD_ID_REQ_START_RECFG command.
 *              There is no data payload expected with this response, just
 *              signal success/fail to AC_Task
 *              
 * Parameters:  See MSInterface.h prototype
 *
 * Returns:     none
 *
 * Notes:       
 *
 *****************************************************************************/
static void AC_MSReqStartCfgRspHandler(UINT16 Id, void* PacketData, 
                                            UINT16 Size, MSI_RSP_STATUS Status)
{
  BOOLEAN success = FALSE;
  
  if(Status == MSI_RSP_SUCCESS)
  {
    success = TRUE;
  }  

  AC_Signal(success);
}



/******************************************************************************
 * Function:    AC_MSReqCfgFilesHandler | MS Response Handler
 *
 * Description: Handle response to the request for configuration files
 *              to be downloaded through the VPN.  Response data is copied
 *              into a shared global, then signal command success/fail to the
 *              AC_Task
 *              
 * Parameters:  See MSInterface.h prototype
 *
 * Returns:     none
 *
 * Notes:       
 *
 *****************************************************************************/
static void AC_MSReqCfgFilesHandler(UINT16 Id, void* PacketData, 
                               UINT16 Size, MSI_RSP_STATUS Status)
{
  BOOLEAN success = FALSE;

  if((Status == MSI_RSP_SUCCESS) && (Size == sizeof(m_ReqCfgFilesRsp)))
  {
    memcpy(&m_ReqCfgFilesRsp,PacketData,Size);
    success = TRUE;
  }
  AC_Signal(success);
}



/******************************************************************************
 * Function:    AC_MSReqCfgFilesHandler | MS Response Handler
 *
 * Description: Handle response to the request for configuration files
 *              to be downloaded through the VPN.  Response data is copied
 *              into a shared global, then signal command success/fail to the
 *              AC_Task
 *              
 * Parameters:  See MSInterface.h prototype
 *
 * Returns:     none
 *
 * Notes:       
 *
 *****************************************************************************/
static void AC_MSReqCfgStsRspHandler(UINT16 Id, void* PacketData, 
                               UINT16 Size, MSI_RSP_STATUS Status)
{
  BOOLEAN success = FALSE;

  if((Status == MSI_RSP_SUCCESS) && (Size == sizeof(m_ReqStsRsp)) )
  {
    memcpy(&m_ReqStsRsp,PacketData,Size);
    success = TRUE;
  }
  AC_Signal(success);
}



/******************************************************************************
 * Function:    AC_MSRspShellCmd | MS Response Handler
 *
 * Description: Receive the response to a micro server shell command.  Signal
 *              success/fail to the AC_Task
 *
 *              
 * Parameters:  See MSInterface.h prototype
 *
 * Returns:     none
 *              
 * Notes:       
 *
 *****************************************************************************/
static void AC_MSRspShellCmd(UINT16 Id, void* PacketData, UINT16 Size,
                               MSI_RSP_STATUS Status)
{
  if(Status == MSI_RSP_SUCCESS)
  {
    if(m_TaskState == TASK_WAIT)
    {
      AC_Signal(TRUE);
    }
    //else, discard unexpected response
  }
  else
  {
    AC_Signal(FALSE);
  }
}



/******************************************************************************
 * Function:    AC_MSRspCancelCfg | MS Response Handler
 *
 * Description: Receive the response to a micro server shell command.  Signal
 *              true or false depending on response success.
 *
 *              
 * Parameters:  See MSInterface.h prototype
 *
 * Returns:     none
 *              
 * Notes:       
 *
 *****************************************************************************/
static void AC_MSRspCancelCfg(UINT16 Id, void* PacketData, UINT16 Size,
                               MSI_RSP_STATUS Status)
{
  if(Status == MSI_RSP_SUCCESS)
  {
    if(m_TaskState == TASK_WAIT)
    {
      AC_Signal(TRUE);
    }
    //else, discard unexpected response
  }
  else
  {
    AC_Signal(FALSE);
  }
}



/******************************************************************************
 * Function:     AC_IsAircraftIDSensorsValid
 *
 * Description:  Check the three sensors used to identify the aircraft the
 *               FAST box is installed on.  Returns true when the Type, Fleet
 *               and Number sensor are valid. 
 *               
 * Parameters:   none
 *
 * Returns:      TRUE:  All three sensors are valid
 *               FALSE: One or more of the three sensors are not valid
 *
 * Notes:        2540
 *****************************************************************************/
static BOOLEAN AC_IsAircraftIDSensorsValid(void)
{
  BOOLEAN          retval = FALSE;
  AIRCRAFT_CONFIG  *ACCfg = &CfgMgr_RuntimeConfigPtr()->Aircraft;

  
  if( SensorIsValid( ACCfg->TypeSensorIndex   ) &&
      SensorIsValid( ACCfg->FleetSensorIndex  ) &&
      SensorIsValid( ACCfg->NumberSensorIndex )    )
  {
    retval = TRUE;
  }
  return retval;
}


/******************************************************************************
 * Function:     AC_UpdateEESensorValues
 *
 * Description:  Store the sensor values into EEPROM.  Update
 *               mACStoreSensorVals at the same time. 
 *
 * Parameters:   none
 *
 * Returns:      none
 *
 * Notes:        2645
 *****************************************************************************/
static void AC_UpdateEESensorValues(FLOAT32 Type, FLOAT32 Fleet, FLOAT32 Number)
{

  NV_Read(NV_AC_CFG,0,&m_ACStoredSensorVals,sizeof(m_ACStoredSensorVals));

  if( (Type != m_ACStoredSensorVals.Type)    ||
      (Fleet != m_ACStoredSensorVals.Fleet)  ||
      (Number != m_ACStoredSensorVals.Number)||
      (EE_FILE_MAGIC_NUMBER != m_ACStoredSensorVals.MagicNumber) )
  {    
    m_ACStoredSensorVals.MagicNumber = EE_FILE_MAGIC_NUMBER;
    m_ACStoredSensorVals.Type        = Type;
    m_ACStoredSensorVals.Fleet       = Fleet;
    m_ACStoredSensorVals.Number      = Number;
    NV_Write(NV_AC_CFG,0,&m_ACStoredSensorVals,sizeof(m_ACStoredSensorVals));
  }

}


/******************************************************************************
 * Function:     AC_CopyACCfgToPackedACCfg 
 *
 * Description:  Configurations stored in EEPROM are not PACKED, 
 *                 however log structures are PACKED.  The following func 
 *                 is a "non-elegant" or "cheesy" way of dealing with this
 *                 mis-alignment.  
 *
 * Parameters:   none
 *
 * Returns:      none
 *
 * Notes:        If new fields are added to AIRCRAFT_CONFIG and are also  
 *               need to be included into the 
 *               AIRCRAFT_MISMATCH_LOG->AIRCRAFT_CONFIG_PACKED, this 
 *               func needs to be manually updated.  Ref SCR #739
 *   
 *****************************************************************************/
static void AC_CopyACCfgToPackedACCfg( AIRCRAFT_CONFIG_PACKED *ACCfg_Packed )
{
  AIRCRAFT_CONFIG *ACCfg = &CfgMgr_RuntimeConfigPtr()->Aircraft;
  
  ACCfg_Packed->bCfgEnabled = ACCfg->bCfgEnabled;
  ACCfg_Packed->bValidateData = ACCfg->bValidateData;
  
  memcpy ( ACCfg_Packed->TailNumber, ACCfg->TailNumber, AIRCRAFT_CONFIG_STR_LEN);
  memcpy ( ACCfg_Packed->Operator, ACCfg->Operator, AIRCRAFT_CONFIG_STR_LEN);
  memcpy ( ACCfg_Packed->Type, ACCfg->Type, AIRCRAFT_CONFIG_STR_LEN);
  
  ACCfg_Packed->TypeSensorIndex = ACCfg->TypeSensorIndex; 
  memcpy ( ACCfg_Packed->FleetIdent, ACCfg->FleetIdent, AIRCRAFT_CONFIG_STR_LEN); 
  
  ACCfg_Packed->FleetSensorIndex = ACCfg->FleetSensorIndex; 
  memcpy ( ACCfg_Packed->Number, ACCfg->Number, AIRCRAFT_CONFIG_STR_LEN); 
  
  ACCfg_Packed->NumberSensorIndex = ACCfg->NumberSensorIndex; 
  memcpy ( ACCfg_Packed->Owner, ACCfg->Owner, AIRCRAFT_CONFIG_STR_LEN);
  memcpy ( ACCfg_Packed->Style, ACCfg->Style, AIRCRAFT_CONFIG_STR_LEN);
  
}
/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/


/*************************************************************************
 *  MODIFICATIONS
 *    $History: AircraftConfigMgr.c $
 * 
 * *****************  Version 51  *****************
 * User: Jim Mood     Date: 10/26/11   Time: 4:04p
 * Updated in $/software/control processor/code/application
 * SCR 1095 modfix to restore error message when cfg stopped b/c in-air
 * 
 * *****************  Version 50  *****************
 * User: Jim Mood     Date: 10/21/11   Time: 4:48p
 * Updated in $/software/control processor/code/application
 * SCR 1076 (Code Review and SCR 1095 (Reconfigure cancel when in-air)
 * 
 * *****************  Version 49  *****************
 * User: Jim Mood     Date: 7/26/11    Time: 6:07p
 * Updated in $/software/control processor/code/application
 * Update to SCR 575
 * 
 * *****************  Version 48  *****************
 * User: Jim Mood     Date: 7/20/11    Time: 10:54a
 * Updated in $/software/control processor/code/application
 * SCR 575: GSM Enable when engine status is lost.  (Part of changes for
 * the Fast State Machine)
 * 
 * *****************  Version 47  *****************
 * User: Jim Mood     Date: 11/11/10   Time: 9:46p
 * Updated in $/software/control processor/code/application
 * SCR 993, checking cfg load while checking if in-air status
 * 
 * *****************  Version 46  *****************
 * User: Jeff Vahue   Date: 10/02/10   Time: 5:08p
 * Updated in $/software/control processor/code/application
 * SCR# 848 - Code Coverage
 * 
 * *****************  Version 45  *****************
 * User: Jim Mood     Date: 9/22/10    Time: 7:03p
 * Updated in $/software/control processor/code/application
 * SCR 884.  Handling unexpected responses to the AC Task from the MS
 * Interface silently instead of going to error.
 * 
 * *****************  Version 44  *****************
 * User: Contractor2  Date: 9/20/10    Time: 4:10p
 * Updated in $/software/control processor/code/application
 * SCR #856 AC Reconfig File should be re-initialized during fast.reset
 * command.
 * 
 * *****************  Version 43  *****************
 * User: John Omalley Date: 9/10/10    Time: 9:59a
 * Updated in $/software/control processor/code/application
 * SCR 858 - Added FATAL to defaults
 * 
 * *****************  Version 42  *****************
 * User: Jeff Vahue   Date: 9/09/10    Time: 11:53a
 * Updated in $/software/control processor/code/application
 * SCR# 833 - Perform only three attempts to get the Cfg files from the
 * MS.  Fix various typos.
 * 
 * *****************  Version 41  *****************
 * User: Peter Lee    Date: 9/03/10    Time: 11:52a
 * Updated in $/software/control processor/code/application
 * SCR #806 Code Review Updates
 * 
 * *****************  Version 40  *****************
 * User: Jim Mood     Date: 8/31/10    Time: 9:21a
 * Updated in $/software/control processor/code/application
 * SCR 829
 * 
 * *****************  Version 39  *****************
 * User: Jim Mood     Date: 8/27/10    Time: 4:30p
 * Updated in $/software/control processor/code/application
 * SCR 828 Does not detect in-air condition while waiting for MSSIM
 * SCR 829 Log upload before pre cfg file retrevial 
 * 
 * *****************  Version 38  *****************
 * User: Jim Mood     Date: 8/26/10    Time: 10:32a
 * Updated in $/software/control processor/code/application
 * SCR 823  Aircraft data mismatch had an extra %s in the DebugStr
 * 
 * *****************  Version 37  *****************
 * User: Jim Mood     Date: 8/12/10    Time: 5:52p
 * Updated in $/software/control processor/code/application
 * SCR 780 modfix
 * 
 * *****************  Version 36  *****************
 * User: Jim Mood     Date: 8/10/10    Time: 11:19a
 * Updated in $/software/control processor/code/application
 * SCR 778 sensor value EE file init fix
 * 
 * *****************  Version 35  *****************
 * User: Peter Lee    Date: 7/30/10    Time: 10:25a
 * Updated in $/software/control processor/code/application
 * SCR #739 AIRCRAFT_CONFIG packed structure
 * 
 * *****************  Version 34  *****************
 * User: Jim Mood     Date: 7/28/10    Time: 8:10p
 * Updated in $/software/control processor/code/application
 * SCR 623: Implementation changes.  Allow manual cfg to start while auto
 * is waiting.  Pause log writes before final upload.
 * 
 * *****************  Version 33  *****************
 * User: Jim Mood     Date: 7/28/10    Time: 1:36p
 * Updated in $/software/control processor/code/application
 * SCR #731 Modfix.  Logic was backwards
 * 
 * *****************  Version 32  *****************
 * User: Jim Mood     Date: 7/23/10    Time: 7:13p
 * Updated in $/software/control processor/code/application
 * SCR #731 Allow auto reconfig when default config is loaded even if
 * in-air
 * 
 * *****************  Version 31  *****************
 * User: Jim Mood     Date: 7/22/10    Time: 11:50a
 * Updated in $/software/control processor/code/application
 * SCR 728 Exit task when Validate Data is no
 * 
 * *****************  Version 30  *****************
 * User: Jim Mood     Date: 6/29/10    Time: 6:29p
 * Updated in $/software/control processor/code/application
 * SCR 623.  Automatic reconfiguration requirments complete
 * 
 * *****************  Version 29  *****************
 * User: Jim Mood     Date: 6/16/10    Time: 6:25p
 * Updated in $/software/control processor/code/application
 * SCR 623 Batch configuration load 
 * 
 * *****************  Version 28  *****************
 * User: Jim Mood     Date: 6/11/10    Time: 10:04a
 * Updated in $/software/control processor/code/application
 * SCR 623 Batch Configuration changes
 * 
 * *****************  Version 27  *****************
 * User: Jeff Vahue   Date: 5/12/10    Time: 6:55p
 * Updated in $/software/control processor/code/application
 * Temp Fix of cut/paste, until Jim finish his rework
 * 
 * *****************  Version 26  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:53p
 * Updated in $/software/control processor/code/application
 * SCR #587 Change TmTaskCreate to return void
 * 
 * *****************  Version 25  *****************
 * User: Contractor2  Date: 4/27/10    Time: 1:38p
 * Updated in $/software/control processor/code/application
 * SCR 187: Degraded mode. Moved watchdog reset loop to WatchDog.c
 * 
 * *****************  Version 24  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:06p
 * Updated in $/software/control processor/code/application
 * SCR #317 Implement safe strncpy.SCR #70 Interrupt level 
 * 
 * *****************  Version 23  *****************
 * User: Contractor V&v Date: 3/29/10    Time: 6:16p
 * Updated in $/software/control processor/code/application
 * SCR #317 Implement safe strncpy
 * 
 * *****************  Version 22  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:37p
 * Updated in $/software/control processor/code/application
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 * 
 * *****************  Version 21  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/application
 * SCR# 483 - Function Names
 * 
 * *****************  Version 20  *****************
 * User: Contractor2  Date: 3/02/10    Time: 11:30a
 * Updated in $/software/control processor/code/application
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 19  *****************
 * User: Contractor V&v Date: 2/03/10    Time: 2:55p
 * Updated in $/software/control processor/code/application
 * SCR 18, SCR 42
 * 
 * *****************  Version 18  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 4:58p
 * Updated in $/software/control processor/code/application
 * SCR# 397
 * 
 * *****************  Version 17  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 2:11p
 * Updated in $/software/control processor/code/application
 * SCR #326
 * 
 * *****************  Version 16  *****************
 * User: Contractor V&v Date: 12/10/09   Time: 5:36p
 * Updated in $/software/control processor/code/application
 * SCR 42 and 106
 * 
 * *****************  Version 15  *****************
 * User: Jeff Vahue   Date: 12/04/09   Time: 4:31p
 * Updated in $/software/control processor/code/application
 * SCR# 350
 * 
 * *****************  Version 14  *****************
 * User: Jeff Vahue   Date: 12/03/09   Time: 4:58p
 * Updated in $/software/control processor/code/application
 * SCR# 359
 * 
 * *****************  Version 13  *****************
 * User: Jim Mood     Date: 10/14/09   Time: 6:09p
 * Updated in $/software/control processor/code/application
 * SCR #285 function prototype had the incorrect type for the status
 * parameter.
 * 
 * *****************  Version 12  *****************
 * User: Jim Mood     Date: 9/29/09    Time: 11:58a
 * Updated in $/software/control processor/code/application
 * SCR# 178 Updated User Manager Tables
 * 
 * *****************  Version 11  *****************
 * User: Jim Mood     Date: 9/16/09    Time: 4:16p
 * Updated in $/software/control processor/code/application
 * SCR# 242, A/C data mismatch log was written with the incorrect data
 * 
 * *****************  Version 10  *****************
 * User: Jim Mood     Date: 7/07/09    Time: 1:37p
 * Updated in $/software/control processor/code/application
 * Added history comment block.  Change fail retry to 2x after the first
 * failure
 * 
 *
 *
 ***************************************************************************/
