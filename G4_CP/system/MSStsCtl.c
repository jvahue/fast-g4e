#define MSSC_BODY
/******************************************************************************
            Copyright (C) 2008-2014 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:         MSStsCtl.c

    Description:  MicroServer Status and Control

    VERSION
      $Revision: 66 $  $Date: 12/02/14 2:27p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include <string.h>


/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_basic.h"
#include "MSStsCtl.h"
#include "GSE.h"
#include "Box.h"
#include "User.h"
#include "CfgManager.h"
#include "Assert.h"
#ifdef ENV_TEST
#include "Etm.h"
#endif

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define MSSC_MIN_MSSIM_VER "2.1.0"
/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static MSSC_XFR_STATUS     m_MsXfrSta;
static MSSC_STS_STATUS     m_MsStsSta;
static TIMESTRUCT          m_MsTime;
static BOOLEAN             m_IsOnGround;
static BOOLEAN             m_IsClientConnected;
static BOOLEAN             m_TimeSynced;          // TRUE if PWEH MS app synced MS to remote
                                                  // time server
static BOOLEAN             m_clockDriftChecked;   // Has one-time, System/MS clock drift
                                                  // been checked.
static BOOLEAN             m_CompactFlashMounted; // Is the CF mounted.
static BOOLEAN             m_IsVPNConnected;      // Most recent VPN connect status from MS
static MSSC_MSSIM_STATUS   m_MssimStatus;
static BOOLEAN             m_SendHeartbeat;
static MSSC_GSM_CONFIG     m_GsmCfgTemp;
static MSSC_CONFIG         m_MsCfgTemp;
static MSCP_MS_SHELL_RSP   m_MSShellRsp;

static UINT32             m_LastHeartbeatTime;    // Time last heartbeat received in Seconds
static UINT32             m_HeartbeatTimer;       // Timeout Time in Seconds
static UINT32             m_HeartbeatLogCount;    // Count of log messages
static BOOLEAN            m_HeartbeatTimedOut;    // True if heartbeat timed out
static UINT32             m_MaxHeartbeatLogs;     // Max number of logs to output
static FLT_STATUS         m_CBITSysCond;          // Logged CBIT Sys Condition

static MSCP_GET_MS_INFO_RSP m_GetMSInfoRsp;
static BOOLEAN            m_MsFileXfer;           // File transfer to Ground in-progress;
static BOOLEAN            m_MssimVersionError;    // The MSSIM version is incompatible
                                                  // with this version of CP
/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static  void    MSSC_Task(void* pParam);
static  void    MSSC_SendHeartbeatCmd(void);
static  void    MSSC_MSRspCallback(UINT16 Id, void* PacketData, UINT16 Size,
                               MSI_RSP_STATUS Status);
static  void    MSSC_MSICallback_GSMCmd(UINT16 Id, void* PacketData, UINT16 Size,
                               MSI_RSP_STATUS Status);
static  void    MSSC_MSICallback_ShellCmd(UINT16 Id, void* PacketData, UINT16 Size,
                               MSI_RSP_STATUS Status);
static  BOOLEAN MSSC_MSGetCPInfoCmdHandler(void* Data,UINT16 Size, UINT32 Sequence);
static  void    MSSC_GetMSInfoRspHandler(UINT16 Id, void* PacketData, UINT16 Size,
                               MSI_RSP_STATUS Status);
static  void    MSSC_HBMonitor(void);

#include "MSStsCtlUserTables.c"

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:    MSSC_Init()
 *
 * Description: Initial setup for the MS Status and Control.  Starts the
 *              heartbeat task and initializes module global variables
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
void MSSC_Init(void)
{
  TCB tcbTaskInfo;

  User_AddRootCmd(&msRoot);

  m_SendHeartbeat = TRUE;

  m_MsXfrSta     = MSSC_XFR_NOT_VLD;
  m_MsStsSta     = MSSC_STS_NOT_VLD;
  m_IsOnGround   = FALSE;
  m_MssimStatus  = MSSC_STARTUP;
  m_MssimVersionError = FALSE;

  m_LastHeartbeatTime = CM_GetTickCount();
  m_HeartbeatTimer =
    (UINT32)CfgMgr_ConfigPtr()->MsscConfig.heartBeatStartup_s * MILLISECONDS_PER_SECOND;
  m_HeartbeatTimedOut = FALSE;
  m_HeartbeatLogCount = 0;
  m_MaxHeartbeatLogs = (UINT32)CfgMgr_ConfigPtr()->MsscConfig.heartBeatLogCnt;
  m_CBITSysCond = CfgMgr_ConfigPtr()->MsscConfig.sysCondCBIT;
  m_CompactFlashMounted = FALSE;

  // Register flag indicating file transfering MS -> GRND.
  m_MsFileXfer = FALSE;
  PmRegisterAppBusyFlag(PM_MS_FILE_XFR_BUSY, &m_MsFileXfer, PM_BUSY_LEGACY);

  memset(&m_GetMSInfoRsp,0,sizeof(m_GetMSInfoRsp));

  ASSERT(SYS_OK == MSI_AddCmdHandler((UINT16)CMD_ID_GET_CP_INFO,MSSC_MSGetCPInfoCmdHandler));

  //Wireless Manager Task
  memset(&tcbTaskInfo, 0, sizeof(tcbTaskInfo));
  strncpy_safe(tcbTaskInfo.Name, sizeof(tcbTaskInfo.Name),"MS Status",_TRUNCATE);
  tcbTaskInfo.TaskID         = (TASK_INDEX)MS_Status;
  tcbTaskInfo.Function       = MSSC_Task;
  tcbTaskInfo.Priority       = taskInfo[MS_Status].priority;
  tcbTaskInfo.Type           = taskInfo[MS_Status].taskType;
  tcbTaskInfo.modes          = taskInfo[MS_Status].modes;
  tcbTaskInfo.MIFrames       = taskInfo[MS_Status].MIFframes;
  tcbTaskInfo.Rmt.InitialMif = taskInfo[MS_Status].InitialMif;
  tcbTaskInfo.Rmt.MifRate    = taskInfo[MS_Status].MIFrate;
  tcbTaskInfo.Enabled        = TRUE;
  tcbTaskInfo.Locked         = FALSE;
  tcbTaskInfo.pParamBlock    = NULL;
  TmTaskCreate (&tcbTaskInfo);

}


/******************************************************************************
 * Function:    MSSC_SetIsOnGround()
 *
 * Description: Call to set the system status, on ground or in air.  This value
 *              is transmitted to the MSSIM application on the micro-server as
 *              part of the the heartbeat command.
 *
 * Parameters:  [in] OnGround: Pass in TRUE if weight-on-wheels is present
 *                             Pass in FALSE if weight-on-wheels is absent
 *
 * Returns:     None
 *
 * Notes:       Call this routine when WOW transitions.
 *
 *****************************************************************************/
void MSSC_SetIsOnGround(BOOLEAN OnGround)
{
  m_IsOnGround = OnGround;
}


/******************************************************************************
 * Function:    MSSC_GetIsVPNConnected()
 *
 * Description: Returns the connected/not connected status of the VPN tunnel to
 *              the ground server.  This status is ultimatley provided by the
 *              Pratt EH application running on the micro-server.  This can be
 *              used as an indication that the VPN connection is established
 *              and operations needing connectivity can be used.
 *
 * Parameters:  none
 *
 * Returns:     TRUE:  The PWEH client indicates the VPN is connected
 *              FALSE: The PWEH client indicates the VPN is not connected or
 *                     PWEH client is not connected to the MSSIM server (see
 *                     GetIsClientConnected() )
 *
 * Notes:
 *
 *****************************************************************************/
BOOLEAN MSSC_GetIsVPNConnected(void)
{
  return m_IsVPNConnected;
}



/******************************************************************************
 * Function:    MSSC_FSMGetVPNStatus()   | IMPLEMENTS GetStatus() INTERFACE
 *                                       | for Fast State Machine.
 *
 * Description: Returns the connected/not connected status of the VPN tunnel to
 *              the ground server.  This status is ultimatley provided by the
 *              Pratt EH application running on the micro-server.  This can be
 *              used as an indication that the VPN connection is established
 *              and operations needing connectivity can be used.
 *
 * Parameters:  [in] param: Not Used, just to match FSM call signature
 *
 * Returns:     TRUE:  The PWEH client indicates the VPN is connected
 *              FALSE: The PWEH client indicates the VPN is not connected or
 *                     PWEH client is not connected to the MSSIM server (see
 *                     GetIsClientConnected() )
 *
 * Notes:
 *
 *****************************************************************************/
BOOLEAN MSSC_FSMGetVPNStatus(INT32 param)
{
  return m_IsVPNConnected;
}



/******************************************************************************
 * Function:    MSSC_FSMGetCFStatus()    | IMPLEMENTS GetStatus() INTERFACE
 *                                       | for Fast State Machine.
 *
 * Description: Returns a flag indicating if the Compact Flash has been mounted.
 *
 * Parameters:  [in] param: Not Used, just to match FSM call signature
 *
 * Returns:     TRUE:  The Micro-Server indicates the Compact Flash card is
 *                     mounted.
 *              FALSE: The Micro-Server indicates the Compact Flash card is
 *                     not mounted
 *
 * Notes:
 *
 *****************************************************************************/
BOOLEAN MSSC_FSMGetCFStatus(INT32 param)
{
  return m_CompactFlashMounted;
}




/******************************************************************************
 * Function:    MSSC_GetSTS()
 *
 * Description: Returns the status to be programmed into the STS LED
 *
 * Parameters:  none
 *
 * Returns:     Most recent STS returned in the heartbeat:
 *                MSSC_STS_NOT_VLD:   No valid STS status available
 *                MSSC_STS_OFF:       RF is off
 *                MSSC_STS_ON_W_LOGS  RF is on, with logs to transmit
 *                MSSC_STS_ON_WO_LOGS RF is on, no logs to transmit
 *
 *
 * Notes:
 *
 *****************************************************************************/
MSSC_STS_STATUS MSSC_GetSTS(void)
{
  MSSC_STS_STATUS sts = MSSC_STS_NOT_VLD;

  if(m_IsClientConnected)
  {
    sts = m_MsStsSta;
  }

  return sts;
}


/******************************************************************************
 * Function:    MSSC_GetXFR()
 *
 * Description: Returns the status to be programed into the XFR LED
 *
 * Parameters:  none
 *
 * Returns:     Most recent XFR status returned in the heartbeat:
 *               MSSC_XFR_NOT_VLD:    No valid XFR status available
 *               MSSC_XFR_NONE:       No transfers occuring
 *               MSSC_XFR_DATA_LOG:   Data log transmitting
 *               MSSC_XFR_SPECIAL_TX: Special transmitting
 *               MSSC_XFR_SPECIAL_RX: Special receiving - NOTE: Special RX
 *                                                        no longer specified
 *                                                        by requirments
 *
 * Notes:
 *
 *****************************************************************************/
MSSC_XFR_STATUS MSSC_GetXFR(void)
{
  MSSC_XFR_STATUS sts = MSSC_XFR_NOT_VLD;

  if(m_IsClientConnected)
  {
    sts = m_MsXfrSta;
  }

  return sts;
}


/******************************************************************************
 * Function:    MSSC_GetIsAlive()
 *
 * Description: Returns a flag indicating if the MSSIM application on the
 *              micro server is responding to heartbeat messages.  The
 *              heartbeat timeout and error counts that define the alive/dead
 *              thresholds are definable in MSStsCtl.h
 *
 * Parameters:  none
 *
 * Returns:     TRUE: MSSIM is currently responding to heartbeat messages in a
 *                    timely fashion
 *              FALSE: The heartbeat messages response from MSSIM has timed out
 *
 * Notes:
 *
 *****************************************************************************/
BOOLEAN MSSC_GetIsAlive(void)
{
  BOOLEAN retval = FALSE;
  if(MSSC_RUNNING == m_MssimStatus)
  {
    retval = TRUE;
  }
  return retval;
}

/******************************************************************************
* Function:    MSSC_GetIsCompactFlashMounted()
*
* Description: Returns a flag indicating if the Compact Flash has been mounted.
*
* Parameters:  none
*
* Returns:     TRUE:  CF is mounted.
*              FALSE: CF is not mounted.
*
* Notes:
*
*****************************************************************************/
BOOLEAN MSSC_GetIsCompactFlashMounted(void)
{
  return m_CompactFlashMounted;
}

/******************************************************************************
 * Function:    MSSC_GetLastMSSIMTime()
 *
 * Description: Returns the last time timestamp read from the MSSIM heartbeat
 *              The heartbeat interval is configurable in MSStsCtl.h
 *
 * Parameters:  none
 *
 * Returns:     TIMESTRUCT: Last read time from the heartbeat. This is as
 *                          old as the last received heartbeat response.
 *
 * Notes:
 *
 *****************************************************************************/
TIMESTRUCT MSSC_GetLastMSSIMTime(void)
{
  return m_MsTime;
}


/******************************************************************************
 * Function:    MSSC_GetGSMSCID()
 *
 * Description: Returns the last received SCID (SIM Card ID) returned from
 *              the Get MSSIM Info command.
 *
 * Parameters:  [in] str: Pointer to a location to store up to a 64 byte
 *                        string
 *
 * Returns:     none
 *
 * Notes:
 *
 *****************************************************************************/
void MSSC_GetGSMSCID(CHAR* str)
{
  strncpy_safe(str, MSCP_MAX_STRING_SIZE, m_GetMSInfoRsp.Gsm.Scid, _TRUNCATE);
}



/******************************************************************************
 * Function:    MSSC_GetGSMSignalStrength()
 *
 * Description: Returns the last received GSM signal strength returned from
 *              the Get MSSIM Info command.  Note that this is the signal
 *              strength at connect, and it is not dynamically updated.
 *
 * Parameters:  [in] str: Pointer to a location to store up to a 64 byte
 *                        string
 * Returns:     None
 *
 * Notes:
 *
 *****************************************************************************/
void MSSC_GetGSMSignalStrength(CHAR* str)
{
  strncpy_safe(str, MSSC_CFG_STR_MAX_LEN, m_GetMSInfoRsp.Gsm.Signal, _TRUNCATE);
}


/******************************************************************************
 * Function:    MSSC_GetMsPwVer()
 *
 * Description: Returns the last received Microserver PW Version Number returned
 *              from the Get MSSIM Info command.
 *
 * Parameters:  [in] str: Pointer to a location to store up to a 64 byte
 *                        string
 * Returns:     None
 *
 * Notes:
 *
 *****************************************************************************/
void MSSC_GetMsPwVer(CHAR* str)
{
  strncpy_safe(str, MSSC_CFG_STR_MAX_LEN, m_GetMSInfoRsp.PWEHVer, _TRUNCATE);
}



/******************************************************************************
 * Function:    MSSC_GetMSTimeSynced()
 *
 * Description: Returns the TimeSynced value from the PWEH ms app
 *
 * Parameters:  none
 *
 * Returns:     TRUE: PWEH ms app had synced to a remote time server
 *
 * Notes:       none
 *
 *****************************************************************************/
BOOLEAN MSSC_GetMSTimeSynced( void )
{
  return m_TimeSynced;
}


/******************************************************************************
 * Function:    MSSC_SendGSMCfgCmd()
 *
 * Description: Sends the GSM connection data from the CP EEPROM to the
 *              micro-server where it is written into a file gsm.cfg
 *              If the message fails to send, no re-attempt is made.
 *
 * Parameters:  none
 *
 * Returns:     None
 *
 * Notes:
 *
 *****************************************************************************/
void MSSC_SendGSMCfgCmd(void)
{
 MSCP_GSM_CONFIG_CMD cmd;

  memcpy(&m_GsmCfgTemp, &CfgMgr_RuntimeConfigPtr()->GsmConfig, sizeof(m_GsmCfgTemp));

  strncpy_safe(cmd.Phone, MSCP_MAX_STRING_SIZE,m_GsmCfgTemp.sPhone,_TRUNCATE);
  cmd.Mode = (UINT16)m_GsmCfgTemp.mode;
  strncpy_safe(cmd.APN, MSCP_MAX_STRING_SIZE, m_GsmCfgTemp.sAPN,_TRUNCATE);
  strncpy_safe(cmd.User, MSCP_MAX_STRING_SIZE, m_GsmCfgTemp.sUser,_TRUNCATE);
  strncpy_safe(cmd.Password, MSCP_MAX_STRING_SIZE, m_GsmCfgTemp.sPassword,_TRUNCATE);
  strncpy_safe(cmd.Carrier, MSCP_MAX_STRING_SIZE, m_GsmCfgTemp.sCarrier,_TRUNCATE);
  strncpy_safe(cmd.MCC, MSCP_MAX_STRING_SIZE, m_GsmCfgTemp.sMCC,_TRUNCATE);
  strncpy_safe(cmd.MNC, MSCP_MAX_STRING_SIZE, m_GsmCfgTemp.sMNC,_TRUNCATE);

  MSI_PutCommand(CMD_ID_SETUP_GSM,&cmd,sizeof(cmd),1000,MSSC_MSICallback_GSMCmd);
}



/******************************************************************************
 * Function:    MSSC_DoRefreshMSInfo()
 *
 * Description: Send the "Get MS Info" command to the micro server.  This
 *              command returns information such as software versions and
 *              static GSM connection information.  The refresh is provided
 *              because the GSM information may update when a new connection
 *              is established, however it is mostly static.
 *
 * Parameters:  none
 *
 * Returns:
 *
 * Notes:
 *
 *****************************************************************************/
void MSSC_DoRefreshMSInfo(void)
{
  if(SYS_OK !=
    MSI_PutCommand(CMD_ID_GET_MSSIM_INFO, NULL, 0, 5000, MSSC_GetMSInfoRspHandler))
  {
    GSE_DebugStr(NORMAL,TRUE,"MSSC: Send 'Get MSSIM Info' failed");
  }
}



/******************************************************************************
 * Function:     MSSC_GetMsStatus
 *
 * Description:  Accessor function to return the ms-sim status
 *
 * Parameters:   none
 *
 * Returns:      MSSC_MSSIM_STATUS
 *
 * Notes:
 *
 *****************************************************************************/
MSSC_MSSIM_STATUS MSSC_GetMsStatus(void)
{
  return m_MssimStatus;
}



/******************************************************************************
 * Function:     MSSC_GetConfigSysCond
 *
 * Description:  Accessor function to return the configured system condition
 *
 * Parameters:   MSSC_SYSCOND_TEST_TYPE testType - requested system condition
 *
 * Returns:      FLT_STATUS
 *
 * Notes:
 *
 *****************************************************************************/
FLT_STATUS MSSC_GetConfigSysCond(MSSC_SYSCOND_TEST_TYPE testType)
{
  FLT_STATUS sysCondSetting;

  // Retrieve the configured sys condition for the test type.
  switch(testType)
  {
    case MSSC_PBIT_SYSCOND:
      sysCondSetting = CfgMgr_ConfigPtr()->MsscConfig.sysCondPBIT;
      break;

    case MSSC_CBIT_SYSCOND:
      sysCondSetting = CfgMgr_ConfigPtr()->MsscConfig.sysCondCBIT;
      break;

    default:
      FATAL("Invalid MSSC_SYSCOND_TEST_TYPE received: %d",testType);
  }
  return sysCondSetting;
}




/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:    MSSC_Task()
 *
 * Description: This task times the heartbeat interval.  The SendHeartbeat flag
 *              is set false after sending a heartbeat command, and then reset
 *              when a response or timeout is received.
 *              The frequency this task runs (and how often a heartbeat is
 *              sent) is configured in MSStsCtl.h
 *
 * Parameters:  pParam:  The Task Manager void pointer for this tasks data
 *                       block
 *
 * Returns:     None
 *
 * Notes:
 *
 *****************************************************************************/
static
void MSSC_Task(void* pParam)
{
  // Check MS Heartbeat
  MSSC_HBMonitor();

  if( TRUE == m_SendHeartbeat && !m_MssimVersionError  )
  {
    MSSC_SendHeartbeatCmd();
  }
}


/******************************************************************************
 * Function:    MSSC_SendHeartbeatCmd()
 *
 * Description: Puts a heartbeat command into the MS Interface command queue.
 *              The data includes the current CP time and the In Air/On Ground
 *              status.
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:
 *
 *****************************************************************************/
static
void MSSC_SendHeartbeatCmd(void)
{
#ifdef ENV_TEST
  UINT32 i;
#endif

  MSCP_CP_HEARTBEAT_CMD cmd;
  TIMESTRUCT time;

  CM_GetSystemClock(&time);

  cmd.Time.Year        = time.Year;
  cmd.Time.Month       = time.Month;
  cmd.Time.Day         = time.Day;
  cmd.Time.Hour        = time.Hour;
  cmd.Time.Minute      = time.Minute;
  cmd.Time.Second      = time.Second;
  cmd.Time.MilliSecond = time.MilliSecond;

  cmd.OnGround = m_IsOnGround;

#ifdef ENV_TEST
/*vcast_dont_instrument_start*/
    // fill in an incrementing pattern
    for ( i=0; i < sizeof(cmd.filler); ++i)
    {
        cmd.filler[i] = i;
    }
/*vcast_dont_instrument_end*/
#endif

  //Send the heartbeat message.  If it is sent successfully, switch "send" off
  //until a response is received. Else, switch send on to try again the next
  //time the task runs.
  if(SYS_OK == MSI_PutCommandEx(CMD_ID_HEARTBEAT,
                             &cmd,
                             sizeof(cmd),
                             1000,
                             MSSC_MSRspCallback,TRUE))
  {
    m_SendHeartbeat = FALSE;
  }
  else
  {
    m_SendHeartbeat = TRUE;
#ifdef ENV_TEST
/*vcast_dont_instrument_start*/
    HeartbeatFailSignal();
/*vcast_dont_instrument_end*/
#endif
  }
}


/******************************************************************************
 * Function:     MSSC_HBMonitor
 *
 * Description:  Monitor and Timeout the MS Heartbeat. Set system condition
 *               and log a status on timeout. Log a reconnection. The number
 *               of logs written is limited by a configuration item.
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:
 *
 *****************************************************************************/
static
void MSSC_HBMonitor(void)
{
  UINT32  currentTime;
  currentTime = CM_GetTickCount();

  if ((currentTime - m_LastHeartbeatTime) > m_HeartbeatTimer)
  {
    // we have a timeout - already have it?
    if (!m_HeartbeatTimedOut)
    {
      // new timeout
      m_HeartbeatTimedOut = TRUE;

      // check if timeout should be logged
      if (m_HeartbeatLogCount < m_MaxHeartbeatLogs)
      {
        // output log
        MSSC_HB_FAIL_LOG log;
        log.operational = (MSSC_STARTUP != m_MssimStatus) ? TRUE : FALSE;
        // set system status
        Flt_SetStatus(m_CBITSysCond,
          SYS_ID_MS_HEARTBEAT_FAIL, &log, sizeof(MSSC_HB_FAIL_LOG));
      }
      else
      {
        // set system status - no log
        Flt_SetStatus(m_CBITSysCond, SYS_ID_NULL_LOG, NULL, 0);
      }

      // if timed out during startup, leave status at startup
      if (MSSC_STARTUP != m_MssimStatus)
      {
        m_MssimStatus = MSSC_DEAD;
        m_IsClientConnected = FALSE;
        m_IsVPNConnected = FALSE;
        m_CompactFlashMounted = FALSE;
      }

      //GSE_DebugStr(NORMAL, TRUE, "MSSC: MSSIM not responding...");

#ifdef ENV_TEST
/*vcast_dont_instrument_start*/
      HeartbeatFailSignal();
/*vcast_dont_instrument_end*/
#endif
    }
  }
  // Heartbeat OK - check for reconnection
  // MSSC_MSRspCallback() will set m_MssimStatus to MSSC_RUNNING
  else if (m_HeartbeatTimedOut && (MSSC_RUNNING == m_MssimStatus))
  {
    // clear
    m_HeartbeatTimedOut = FALSE;

    // reset system status
    Flt_ClrStatus(m_CBITSysCond);

    // check if reconnect should be logged
    if (m_HeartbeatLogCount < m_MaxHeartbeatLogs)
    {
      m_HeartbeatLogCount++;
      // output log
      LogWriteSystem(SYS_ID_MS_RECONNECT, LOG_PRIORITY_LOW, NULL, 0, NULL);
    }
  }
}

/******************************************************************************
 * Function:    MSSC_MSGetCPInfoCmdHandler
 *
 * Description: Handles a command from the micro-server to get the CP Box
 *              serial number.
 *
 * Parameters:  void*  Data
 *              UINT16 Size
 *              UINT32 Sequence
 *
 * Returns:     BOOLEAN
 *
 * Notes:       None
 *
 *****************************************************************************/
static
BOOLEAN MSSC_MSGetCPInfoCmdHandler(void* Data, UINT16 Size,
                                          UINT32 Sequence)
{
  MSCP_GET_CP_INFO_RSP msRsp;

  memset(&msRsp,0,sizeof(msRsp));

  Box_GetSerno(msRsp.SN);
  msRsp.Pri4DirMaxSizeMB = CfgMgr_ConfigPtr()->MsscConfig.pri4DirMaxSizeMB;

  //Return value deliberatly ignored.  If the response cannot be sent, the
  //micro-server must timeout and re-request CP info.
  MSI_PutResponse((UINT16)CMD_ID_GET_CP_INFO,
                  msRsp.SN,
                  (UINT16)MSCP_RSP_STATUS_SUCCESS,
                  sizeof(msRsp),
                  Sequence);
  return TRUE;

}

/******************************************************************************
 * Function:    MSSC_MSICallback_GSMCmd
 *
 * Description: Handles response to the configure GSM command sent from the
 *              MSSC module on connect to MSSIM.
 *
 * Parameters:   UINT16          Id
 *               void*           PacketData
 *               UINT16          Size
 *               MSI_RSP_STATUS  Status
 *
 * Returns:     none
 *
 * Notes:
 *
 *****************************************************************************/
static
void MSSC_MSICallback_GSMCmd(UINT16 Id, void* PacketData, UINT16 Size,
                               MSI_RSP_STATUS Status)
{
  if(Status != MSI_RSP_SUCCESS)
  {
    GSE_DebugStr(NORMAL,TRUE,"MSSC: Configure gsm.cfg failed");
  }
}

/******************************************************************************
 * Function:    MSSC_MSICallback_ShellCmd
 *
 * Description: Handles response to the Linux Shell command.  This receives
 *              the output of the shell command sent to MSSIM.
 *
 * Parameters:   UINT16          Id
 *               void*           PacketData
 *               UINT16          Size
 *               MSI_RSP_STATUS  Status
 *
 * Returns:     none
 *
 * Notes:
 *
 *****************************************************************************/
static
void MSSC_MSICallback_ShellCmd(UINT16 Id, void* PacketData, UINT16 Size,
                               MSI_RSP_STATUS Status)
{
  MSCP_MS_SHELL_RSP *rsptemp = PacketData;
  INT8 *str_start;
  INT8 *str_end;

  if(Status == MSI_RSP_SUCCESS)
  {
    //Copy buffers up to the size.
    m_MSShellRsp.Len = MIN(rsptemp->Len,sizeof(m_MSShellRsp.Str));
    rsptemp->Str[m_MSShellRsp.Len] ='\0';

    /* Convert /n to /r/n for serial terminal *
       1. while /n is found, append /r/n in its place
       2. append the final string segment after the last /n
          at the loop exit*/
    m_MSShellRsp.Str[0] = '\0';
    str_start = rsptemp->Str;
    while(NULL != (str_end = strchr(str_start,'\n')))
    {
      *str_end = '\0';   //NULL terminate segment
      SuperStrcat(m_MSShellRsp.Str,str_start,sizeof(m_MSShellRsp.Str));
      SuperStrcat(m_MSShellRsp.Str,"\r\n",sizeof(m_MSShellRsp.Str));
      str_start = str_end + 1;
    }
    SuperStrcat(m_MSShellRsp.Str,str_start,sizeof(m_MSShellRsp.Str));
  }
}

/******************************************************************************
 * Function:     MSSC_MSRspCallback
 *
 * Description:  Handles Microserver Response to Heartbeat Command
 *
 * Parameters:   UINT16          Id
 *               void*           PacketData
 *               UINT16          Size
 *               MSI_RSP_STATUS  Status
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static
void MSSC_MSRspCallback(UINT16 Id, void* PacketData, UINT16 Size,
                               MSI_RSP_STATUS Status)
{
  MSCP_MS_HEARTBEAT_RSP* pRspData;
  INT32 compareResult;
  MSSC_MS_VERSION_ERR_LOG log;

  //If a good heartbeat was received, save the data in the response to local
  //variables.  Make sure the MSSIM status is alive and clear the error count
  if(Status == MSI_RSP_SUCCESS)
  {
    if(m_MssimStatus != MSSC_RUNNING)
    {
      //Check if version number has been received yet...
      if(0 != strlen(m_GetMSInfoRsp.MssimVer))
      {
        compareResult = CompareVersions(m_GetMSInfoRsp.MssimVer, MSSC_MIN_MSSIM_VER);
        //If the MSSIM version is the same ( =0) or greater than ( =1)
        if((compareResult == 0) || (compareResult == 1))
        {
          m_MssimStatus = MSSC_RUNNING;
          m_HeartbeatTimer =
            (UINT32)CfgMgr_ConfigPtr()->MsscConfig.heartBeatLoss_s * MILLISECONDS_PER_SECOND;
        }
        //MSSIM version less than 2.1.0 or decode
        else
        {
          strncpy_safe(log.sMssimVer, sizeof(log.sMssimVer),
                       m_GetMSInfoRsp.MssimVer,_TRUNCATE);
          Flt_SetStatus(m_CBITSysCond,SYS_ID_MS_VERSION_MISMATCH, &log,
                        sizeof(MSSC_MS_VERSION_ERR_LOG));
          m_MssimVersionError = TRUE;
        }
      }
      //Get initial MS Info on connect. Return value deliberatly ignored, if command fails,
      //then the next HB will automatically re-execute this call.
      MSI_PutCommandEx(CMD_ID_GET_MSSIM_INFO, NULL, 0, 5000,
                       MSSC_GetMSInfoRspHandler,TRUE);
    }

    m_LastHeartbeatTime = CM_GetTickCount();

    pRspData = PacketData;

    m_MsTime.Year        = pRspData->Time.Year;
    m_MsTime.Month       = pRspData->Time.Month;
    m_MsTime.Day         = pRspData->Time.Day;
    m_MsTime.Hour        = pRspData->Time.Hour;
    m_MsTime.Minute      = pRspData->Time.Minute;
    m_MsTime.Second      = pRspData->Time.Second;
    m_MsTime.MilliSecond = pRspData->Time.MilliSecond;

    if(!m_MssimVersionError)
    {
      m_TimeSynced = pRspData->TimeSynched;
      m_IsClientConnected  = pRspData->ClientConnected;
      m_IsVPNConnected = pRspData->VPNConnected;
      m_CompactFlashMounted = pRspData->CompactFlashMounted;
    }
    else
    {
      m_TimeSynced =          FALSE;
      m_IsClientConnected =   FALSE;
      m_IsVPNConnected =      FALSE;
      m_CompactFlashMounted = FALSE;
    }

#if 0
    /*vcast_dont_instrument_start*/
    // Set the CF Mounted state
    m_CompactFlashMounted = pRspData->CompactFlashMounted;
    /*vcast_dont_instrument_end*/
#endif

    // Check if this is the first time we received heartbeat since startup.
    // If so, check clock difference between MS and CP
    if(!m_clockDriftChecked)
    {
      CM_CompareSystemToMsClockDrift();
      m_clockDriftChecked = TRUE;
    }

    switch(pRspData->Xfr)
    {
      case MSCP_XFR_NONE:
        m_MsXfrSta = MSSC_XFR_NONE;
        break;
      case MSCP_XFR_DATA_LOG_TX:
        m_MsXfrSta = MSSC_XFR_DATA_LOG;
        break;
      case MSCP_XFR_SPECIAL_TX:
        m_MsXfrSta = MSSC_XFR_SPECIAL_TX;
        break;
      default:
        m_MsXfrSta = MSSC_XFR_NOT_VLD;
        break;
    }

    switch(pRspData->Sta)
    {
      case MSCP_STS_OFF:
        m_MsStsSta   = MSSC_STS_OFF;
        m_MsFileXfer = FALSE;
        break;
      case MSCP_STS_ON_W_LOGS:
        m_MsStsSta   = MSSC_STS_ON_W_LOGS;
        m_MsFileXfer = TRUE;
        break;
      case MSCP_STS_ON_WO_LOGS:
        m_MsStsSta   = MSSC_STS_ON_WO_LOGS;
        m_MsFileXfer = FALSE;
        break;
      default:
        m_MsStsSta = MSSC_STS_NOT_VLD;
        break;
    }
  }

  //Schedule the next heartbeat command (See MSSC_Task)
  m_SendHeartbeat = TRUE;

}


/******************************************************************************
 * Function:     MSSC_GetMSInfoRspHandler
 *
 * Description:  Receive and store the data in the response to Get MSSIM Info.
 *
 * Parameters:   UINT16          Id
 *               void*           PacketData
 *               UINT16          Size
 *               MSI_RSP_STATUS  Status
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static
void MSSC_GetMSInfoRspHandler(UINT16 Id, void* PacketData, UINT16 Size,
                               MSI_RSP_STATUS Status)
{
  if((Status == MSI_RSP_SUCCESS) && (Size == sizeof(m_GetMSInfoRsp)) )
  {
    memcpy(&m_GetMSInfoRsp,PacketData,sizeof(m_GetMSInfoRsp));
  }
}


/*************************************************************************
 *  MODIFICATIONS
 *    $History: MSStsCtl.c $
 * 
 * *****************  Version 66  *****************
 * User: Contractor V&v Date: 12/02/14   Time: 2:27p
 * Updated in $/software/control processor/code/system
 * SCR #1204 - Legacy App Busy Input still latches battery / CR updates
 * 
 * *****************  Version 65  *****************
 * User: Contractor V&v Date: 9/03/14    Time: 5:21p
 * Updated in $/software/control processor/code/system
 * SCR #1204 - Legacy App Busy Input still latches battery / CR updates
 *
 * *****************  Version 64  *****************
 * User: Contractor V&v Date: 8/12/14    Time: 6:09p
 * Updated in $/software/control processor/code/system
 * SCR #1204 - Legacy App Busy Input still latches battery while FSM
 * enabled
 *
 * *****************  Version 63  *****************
 * User: Jim Mood     Date: 12/14/12   Time: 8:05p
 * Updated in $/software/control processor/code/system
 * SCR #1197
 *
 * *****************  Version 62  *****************
 * User: Jim Mood     Date: 12/11/12   Time: 8:32p
 * Updated in $/software/control processor/code/system
 * SCR #1197 Code Review Updates
 *
 * *****************  Version 61  *****************
 * User: Jim Mood     Date: 11/19/12   Time: 8:29p
 * Updated in $/software/control processor/code/system
 * SCR #1173
 *
 * *****************  Version 60  *****************
 * User: John Omalley Date: 12-11-13   Time: 5:46p
 * Updated in $/software/control processor/code/system
 * SCR 1197 - Code Review Updates
 *
 * *****************  Version 59  *****************
 * User: John Omalley Date: 12-11-12   Time: 10:57a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review Updates
 *
 * *****************  Version 58  *****************
 * User: John Omalley Date: 12-11-12   Time: 10:39a
 * Updated in $/software/control processor/code/system
 * SCrR1107 - Code Review Updates
 *
 * *****************  Version 57  *****************
 * User: Melanie Jutras Date: 12-10-10   Time: 1:03p
 * Updated in $/software/control processor/code/system
 * SCR 1172 PCLint 545 Suspicious use of & Error
 *
 * *****************  Version 56  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 55  *****************
 * User: Jim Mood     Date: 7/19/12    Time: 11:07a
 * Updated in $/software/control processor/code/system
 * SCR 1107: Data Offload changes for 2.0.0
 *
 * *****************  Version 54  *****************
 * User: Jim Mood     Date: 7/26/11    Time: 3:06p
 * Updated in $/software/control processor/code/system
 * SCR 575 updates
 *
 * *****************  Version 53  *****************
 * User: Jim Mood     Date: 7/20/11    Time: 10:57a
 * Updated in $/software/control processor/code/system
 * SCR 575: GSM Enable when engine status is lost.  (Part of changes for
 * the Fast State Machine)
 *
 * *****************  Version 52  *****************
 * User: Contractor2  Date: 5/18/11    Time: 2:38p
 * Updated in $/software/control processor/code/system
 * SCR #1031 Enhancement: Add PW_Version to box.status
 *
 * *****************  Version 51  *****************
 * User: Jeff Vahue   Date: 11/06/10   Time: 10:01p
 * Updated in $/software/control processor/code/system
 * SCR# 848 - Code Coverage
 *
 * *****************  Version 50  *****************
 * User: Contractor2  Date: 10/12/10   Time: 2:33p
 * Updated in $/software/control processor/code/system
 * SCR #929 Code Review: MSStsCtl
 *
 * *****************  Version 49  *****************
 * User: Jim Mood     Date: 9/30/10    Time: 5:52p
 * Updated in $/software/control processor/code/system
 * SCR 905 Code Coverage Issues
 *
 * *****************  Version 48  *****************
 * User: John Omalley Date: 9/10/10    Time: 10:01a
 * Updated in $/software/control processor/code/system
 * SCR 858 - Added FATAL to default cases
 *
 * *****************  Version 47  *****************
 * User: John Omalley Date: 9/09/10    Time: 2:14p
 * Updated in $/software/control processor/code/system
 * SCR 855 - Removed unused functions
 *
 * *****************  Version 46  *****************
 * User: Jeff Vahue   Date: 9/07/10    Time: 7:31p
 * Updated in $/software/control processor/code/system
 * SCR# 848 - Code Coverage
 *
 * *****************  Version 45  *****************
 * User: Jim Mood     Date: 8/12/10    Time: 7:07p
 * Updated in $/software/control processor/code/system
 * SCR 788 ms.cmd lockup
 *
 * *****************  Version 44  *****************
 * User: Contractor2  Date: 8/05/10    Time: 11:14a
 * Updated in $/software/control processor/code/system
 * SCR #762 Error: Micro-Server Interface Logic doesn't work
 * Correct heartbeat timeout logging
 *
 * *****************  Version 43  *****************
 * User: Jeff Vahue   Date: 8/02/10    Time: 4:49p
 * Updated in $/software/control processor/code/system
 * Typos in comments
 *
 * *****************  Version 42  *****************
 * User: Contractor2  Date: 8/02/10    Time: 3:42p
 * Updated in $/software/control processor/code/system
 * SCR #762 Error: Micro-Server Interface Logic doesn't work
 *
 * *****************  Version 41  *****************
 * User: Contractor V&v Date: 7/30/10    Time: 9:37p
 * Updated in $/software/control processor/code/system
 * SCR #282 Misc - Shutdown Processing
 *
 * *****************  Version 40  *****************
 * User: Contractor2  Date: 7/30/10    Time: 2:29p
 * Updated in $/software/control processor/code/system
 * SCR #90;243;252 Implementation: CBIT of mssim
 *
 * *****************  Version 39  *****************
 * User: Jim Mood     Date: 7/30/10    Time: 11:28a
 * Updated in $/software/control processor/code/system
 * SCR 327 Fast Transmission Test and
 * SCR 479 ms.status commands
 *
 * *****************  Version 38  *****************
 * User: Jim Mood     Date: 7/30/10    Time: 8:57a
 * Updated in $/software/control processor/code/system
 * SCF 327 Fast transmission test functionality and gsm status information
 *
 * *****************  Version 37  *****************
 * User: Contractor V&v Date: 7/27/10    Time: 6:20p
 * Updated in $/software/control processor/code/system
 * SCR #711 Error: Dpram Int Failure incorrectly forces STA_FAULT.
 *
 * *****************  Version 36  *****************
 * User: Contractor V&v Date: 7/21/10    Time: 7:18p
 * Updated in $/software/control processor/code/system
 * SCR #260 Implement BOX status command
 *
 * *****************  Version 35  *****************
 * User: Contractor3  Date: 7/16/10    Time: 8:57a
 * Updated in $/software/control processor/code/system
 * SCR #702 - Changes based on code review.
 *
 * *****************  Version 34  *****************
 * User: Jim Mood     Date: 7/09/10    Time: 8:28p
 * Updated in $/software/control processor/code/system
 * SCR 607 mod fix (removed cf status stub, repalced with real cf status)
 *
 * *****************  Version 33  *****************
 * User: Contractor V&v Date: 6/23/10    Time: 3:00p
 * Updated in $/software/control processor/code/system
 * SCR #651 Error: MS Status and CF Ready - moved temp assignment of CF
 * mounted to bottom of function after all possible assigments of
 * (coupled) m_MssimStatus have been performed.
 *
 * *****************  Version 32  *****************
 * User: Jim Mood     Date: 6/11/10    Time: 3:30p
 * Updated in $/software/control processor/code/system
 * SCR 623 Batch configuration implementation updates
 *
 * *****************  Version 31  *****************
 * User: Contractor V&v Date: 5/26/10    Time: 2:11p
 * Updated in $/software/control processor/code/system
 * SCR #607 The CP shall query Ms CF mounted before File Upload pr
 *
 * *****************  Version 30  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:55p
 * Updated in $/software/control processor/code/system
 * SCR #587 Change TmTaskCreate to return void
 *
 * *****************  Version 29  *****************
 * User: Jim Mood     Date: 5/10/10    Time: 9:16a
 * Updated in $/software/control processor/code/system
 * SCR 588, VPN Connected
 *
 * *****************  Version 28  *****************
 * User: Contractor V&v Date: 4/28/10    Time: 4:54p
 * Updated in $/software/control processor/code/system
 * SCR #559 Check clock diff CP-MS on startup
 *
 * *****************  Version 27  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:11p
 * Updated in $/software/control processor/code/system
 * SCR #317 Implement safe strncpy
 *
 * *****************  Version 26  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:36p
 * Updated in $/software/control processor/code/system
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 *
 * *****************  Version 25  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/system
 * SCR# 483 - Function Names
 *
 * *****************  Version 24  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 23  *****************
 * User: Jeff Vahue   Date: 2/26/10    Time: 10:18a
 * Updated in $/software/control processor/code/system
 * SCR# 452 - code coverage changes to if/then/else code, and some typos
 *
 * *****************  Version 22  *****************
 * User: Jim Mood     Date: 1/25/10    Time: 1:27p
 * Updated in $/software/control processor/code/system
 * SCR #403
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
 * User: Jeff Vahue   Date: 12/18/09   Time: 3:52p
 * Updated in $/software/control processor/code/system
 * SCR# 378
 *
 * *****************  Version 17  *****************
 * User: Contractor V&v Date: 12/10/09   Time: 5:46p
 * Updated in $/software/control processor/code/system
 * SCR 42 and SCR 106
 *
 * *****************  Version 16  *****************
 * User: Jim Mood     Date: 12/01/09   Time: 2:30p
 * Updated in $/software/control processor/code/system
 * SCR # 352
 *
 * *****************  Version 15  *****************
 * User: Peter Lee    Date: 11/23/09   Time: 5:57p
 * Updated in $/software/control processor/code/system
 * SCR #347 Time Sync Requirements
 *
 * *****************  Version 14  *****************
 * User: Contractor V&v Date: 11/18/09   Time: 4:15p
 * Updated in $/software/control processor/code/system
 * Implement SCR 172 ASSERT processing for all "default" case processing
 *
 * *****************  Version 13  *****************
 * User: Jim Mood     Date: 11/18/09   Time: 1:48p
 * Updated in $/software/control processor/code/system
 * Added GSE commands for micro-server shell commands
 *
 * *****************  Version 12  *****************
 * User: Jim Mood     Date: 9/29/09    Time: 11:58a
 * Updated in $/software/control processor/code/system
 * SCR# 178 Updated User Manager Tables
 *
 * *****************  Version 11  *****************
 * User: Jim Mood     Date: 9/11/09    Time: 10:50a
 * Updated in $/software/control processor/code/system
 *
 * *****************  Version 10  *****************
 * User: Peter Lee    Date: 6/30/09    Time: 3:35p
 * Updated in $/software/control processor/code/system
 * SCR #204 Misc Code Review Updates
 *
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 5/21/09    Time: 4:23p
 * Updated in $/software/control processor/code/system
 * SCR #193 Informal Code Review Findings
 *
 * *****************  Version 8  *****************
 * User: Jim Mood     Date: 5/15/09    Time: 5:15p
 * Updated in $/software/control processor/code/system
 * Update STS lamp definition.  Set STS and XFR status to power-on default
 * when the client disconnects
 *
 * *****************  Version 7  *****************
 * User: Jim Mood     Date: 4/07/09    Time: 10:13a
 * Updated in $/control processor/code/system
 * SCR# 161
 *
 * *****************  Version 6  *****************
 * User: Jim Mood     Date: 4/02/09    Time: 9:04a
 * Updated in $/control processor/code/system
 * Added "startup" state to the MSSIM connected status
 * Added the "Get CP Info" command handler
 *
 * *****************  Version 5  *****************
 * User: Jim Mood     Date: 2/17/09    Time: 11:34a
 * Updated in $/control processor/code/system
 * Added a 3rd value to the micro-server state called "startup" that
 * allows a definite transition to MSSIM Alive or MSSIM Dead after
 * startup.
 *
 * *****************  Version 4  *****************
 * User: Jim Mood     Date: 1/30/09    Time: 8:57a
 * Updated in $/control processor/code/system
 * Added and tested a command handler for the MSSIM command GET_CP_INFO.
 *
 * *****************  Version 3  *****************
 * User: Jim Mood     Date: 1/09/09    Time: 2:31p
 * Updated in $/control processor/code/system
 * Fixed SCR #99.  Changed heartbeat message timeout from 0.1sec to 1.0sec
 *
 * *****************  Version 2  *****************
 * User: Jim Mood     Date: 11/12/08   Time: 1:40p
 * Updated in $/control processor/code/system
 *
 * *****************  Version 1  *****************
 * User: Jim Mood     Date: 10/30/08   Time: 11:30a
 * Created in $/control processor/code/system
 * Replaces WirelessMgr.c
 *
 * *****************  Version 8  *****************
 * User: Jim Mood     Date: 10/08/08   Time: 9:55a
 * Updated in $/control processor/code/system
 *
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 4:30p
 * Updated in $/control processor/code/system
 * SCR #87 Function Prototype
 *
 *
 ***************************************************************************/

