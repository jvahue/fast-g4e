#define FAULTMGR_BODY

/******************************************************************************
            Copyright (C) 2008-2010 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:        FaultMgr.c

    Description:
   VERSION
      $Revision: 55 $  $Date: 12-11-09 4:41p $
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include <string.h>
#include <stdio.h>

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "alt_basic.h"
#include "gse.h"
#include "FaultMgr.h"
#include "CfgManager.h"
#include "NVMgr.h"
#include "SystemLog.h"
#include "LogManager.h"
#include "User.h"
#include "ClockMgr.h"
#include "Assert.h"
#include "actionmanager.h"
/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
// NOTE: The current Fault History entries are about 14 bytes, so our file
// size is 14*10 = 140 bytes.  File size is set to 256, keep this in mind
// if you change the fault history buffer size or FAULT_HISTORY struct.
#define FLT_LOG_BUF_ENTRIES 10

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
// This is a self-indexed list.  The count indicates the newest entry, that
// is the highest count is the newest entry.  This approach is used to require
// one NV_Write (and CRC calculation during an update) when the fault occurs.
typedef struct {
  SYS_APP_ID Id;
  TIMESTAMP  Ts;
  UINT32     count;
} FAULT_HISTORY;


/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
// working set of faults and fault data
static FAULT_HISTORY faultHistory[FLT_LOG_BUF_ENTRIES];
static INT8 faultIndex;
static UINT32 faultCount;

static FLT_STATUS  FaultSystemStatus;
static BOOLEAN     SetFaultInitialized;

static UINT32      FaultSystemStatusCnt[STA_MAX];

static FAULTMGR_ACTION m_Previous;
static FAULTMGR_ACTION m_Current;

static FLT_DBG_LEVEL  DebugLevel;   // local runtime copy of config data

// Include cmd tables and functions here after local dependencies are declared.
#include "FaultMgrUserTables.c"

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void Flt_AddToFaultBuf(SYS_APP_ID LogID, TIMESTAMP ts);
static void Flt_UpdateSystemStatus( void );
static void Flt_LogSysStatus(SYS_APP_ID LogID, FLT_STATUS SetStatus, FLT_STATUS prevStatus);

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
* Function:    Flt_PreInitFaultMgr
*
* Description: Initialize the GSE debug string output functionality.  This
*              initialization allows calls to GSE_DebugStr and GSE_StatusStr
*              and is dependent on the GSE driver being initialized already.
*              No other Flt functionality can be used until it is also
*              initialized.
*
* Parameters:  none
*
* Returns:     none
*
* Notes:
*
*****************************************************************************/
void Flt_PreInitFaultMgr(void)
{
  FaultSystemStatus = STA_NORMAL;
  SetFaultInitialized = FALSE;
  Flt_SetDebugVerbosity( NORMAL); // SCR# 640
  // Fault Action Flags
  m_Previous.nID     = ACTION_NO_REQ;
  m_Previous.state   = OFF;
  m_Previous.sysCond = FaultSystemStatus;

  m_Current.nID     = ACTION_NO_REQ;
  m_Current.state   = OFF;
  m_Current.sysCond = FaultSystemStatus;
}

/******************************************************************************
 * Function:    Flt_Init
 *
 * Description: Setup the initial state for the Fault Manager.
 *              -Add the Fault Manager commands to the User Manager
 *              -Initialize the System Status to NORMAL
 *              -Initialize the EEPROM fault buffer
 *
 *              Initialize the SetStatus/GetStatus functions, the NV Fault
 *              buffer and the log writing functionality.  This functionality
 *              is dependent on the NV_Open, NV_Write, and NV_Read being
 *              ready, as well as LogWriteSystem and CM_GetTimeAsTimestamp
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:
 *
 *****************************************************************************/
void Flt_Init(void)
{
  INT8 i;

  //Add the Fault Manager set of commands to User Manager
  User_AddRootCmd(&FaultMgr_RootMsg);

  //Open EE fault buffer
  if ( SYS_OK != NV_Open(NV_FAULT_LOG))
  {
    // clear and reset the fault buffer
    Flt_InitFltBuf();
  }
  else
  {
    // read the data into our local working buffer
    NV_Read( NV_FAULT_LOG, 0, faultHistory, sizeof( faultHistory));

    // find the highest count and move to the next one
    faultIndex = -1;
    faultCount = 0;
    for (i=0; i < FLT_LOG_BUF_ENTRIES; ++i)
    {
      // make sure there is really something there vs. just the EEPROM be cleared
      if (faultHistory[i].count >= faultCount && faultHistory[i].count != 0xFFFFFFFF)
      {
        faultIndex = i;
        faultCount = faultHistory[i].count;
      }
    }

    // make sure there is some thing in the buffer
    if ( faultIndex == -1)
    {
        // clear and reset the fault buffer
        Flt_InitFltBuf();
    }
    else
    {
        // move to the next one
        faultIndex = (faultIndex + 1) % FLT_LOG_BUF_ENTRIES;
        ++faultCount;
    }
  }

  // Clear fault counter used for keeping real time system status condition
  memset ( (void *) FaultSystemStatusCnt, 0x00, sizeof(FaultSystemStatusCnt));

  SetFaultInitialized = TRUE;
}

/******************************************************************************
* Function:    Flt_InitFltBuf
*
* Description: Re-initialize the fault buffer in EEPROM.  This sets each
*              fault log to a null log, at time 0.
*
* Parameters: None
*
* Returns:    BOOLEAN status
*
* Notes:
*
*****************************************************************************/
BOOLEAN Flt_InitFltBuf(void)
{
  // reset the local working buffer and save it to EEPROM
  memset( faultHistory, 0, sizeof( faultHistory));

  NV_Write(NV_FAULT_LOG, 0, faultHistory, sizeof(faultHistory));

  // start count at one to indicate something is written here
  faultCount = 1;
  faultIndex = 0;

  return TRUE;
}

/******************************************************************************
 * Function:    Flt_SetStatus
 *
 * Description: Set the System Status variable to a higher priority fault level
 *              and write a fault log (with data field up to FLT_LOG_MAX_SIZE),
 *              indicating what error occurred to SystemLog and also to a
 *              rolling buffer of the last 10 faults stored in EEPROM.
 *              The System Status will only be changed if the passed Status
 *              parameter is a higher priority than the current System Status.
 *              If the Log ID is passed as "NULL_LOG", no log will be written.
 *              This feature is used for setting the system status on startup,
 *              where the fault log has already been written on a previous
 *              power cycle.
 *
 * Parameters:  [out] Status:  System Status to set
 *                    Returns: FLT_STATUS the previous (i.e. current)
 *                    fault system status at time call was issued.
 *
 *              [in] LogID:   ID of the log to write that is associated with
 *                            this status change.
 *              [in] LogData: Pointer to a variable size block of data to
 *                            write with the log ID.  Max size is 128 bytes
 *              [in] LogSize: Size of the LogData block, Max is 128 bytes.
 *                            Larger size values will be truncated.
 *
 *  Returns:    void
 *
  * Notes:
 *
 *****************************************************************************/
void Flt_SetStatus( FLT_STATUS Status, SYS_APP_ID LogID, void *LogData,
                          INT32 LogDataSize)
{
  FLT_STATUS prevSystemStatus;
  BOOLEAN    bLogSysStatus = FALSE;

  // If Flt_Init() has not been called, ASSERT !
  ASSERT ( SetFaultInitialized != FALSE );

  prevSystemStatus = Flt_GetSystemStatus();

  // Inc request for this system condition
  if ( Status != STA_NORMAL )
  {
    // Save the current value of System Status and
    // set flag to log a Sys-Status-change after the sys log.
    bLogSysStatus = TRUE;

    // Increment Sys Condition request
    FaultSystemStatusCnt[Status] += 1;

    // The FaultSystemStatus always reflect the highest request sys condition
    Flt_UpdateSystemStatus();
  }

  //If requested, write the log to System Log
  //TBD who will be requesting Flt_SetStatus w/o assigning a valid SYS_ID ?
  //    make this an ASSERT ?
  if(LogID != SYS_ID_NULL_LOG)
  {
    TIMESTAMP ts;
    CM_GetTimeAsTimestamp(&ts);
    LogWriteSystem(LogID, LOG_PRIORITY_LOW, LogData, (UINT16)LogDataSize, &ts);
    Flt_AddToFaultBuf( LogID, ts);
  }

  if ( bLogSysStatus )
  {
    Flt_LogSysStatus(LogID, Status, prevSystemStatus );
  }
}


/******************************************************************************
 * Function:    Flt_ClrStatus
 *
 * Description: Clears the requested status condition
 *
 * Parameters:  [in] Status:  System Status to set
 *
 * Returns:     none
 *
 * Notes:       none
 *
 *****************************************************************************/
void Flt_ClrStatus(FLT_STATUS Status)
{

  // Dec request for this system condition
  if ( Status != STA_NORMAL )
  {
    FLT_STATUS prevSystemStatus;
    // If Count == 0, then we have more decrement requests then increment requests,
    //   some how we go out of sync !
    ASSERT ( FaultSystemStatusCnt[Status] != 0 );

    prevSystemStatus = Flt_GetSystemStatus();

    // Dec request for this system condition
    FaultSystemStatusCnt[Status] -= 1;

    // Update the current system status
    Flt_UpdateSystemStatus();

    // If system status has returned to normal, create log.
    if (STA_NORMAL == Flt_GetSystemStatus() )
    {
      Flt_LogSysStatus(SYS_ID_NULL_LOG, Status, prevSystemStatus );
    }
  }
}

/******************************************************************************
 * Function:    Flt_SetDebugVerbosity
 *
 * Description: Set the debug output level to NORMAL, VERBOSE or OFF
 *
 * Parameters:  [in] NewLevel: New debug level to set
 *
 * Returns:     Previous (i.e. current) debug level at the time of the
 *              call.
 * Notes:
 *
 *****************************************************************************/
void Flt_SetDebugVerbosity(FLT_DBG_LEVEL NewLevel)
{
  // set local working copy
  DebugLevel = NewLevel;
}

/******************************************************************************
* Function:    Flt_GetDebugVerbosity
*
* Description: Get the current debug output level (NORMAL, VERBOSE or OFF)
*
* Parameters:  None
*
* Returns:     Current debug level at the time of the call.
*
* Notes:
*
*****************************************************************************/
FLT_DBG_LEVEL Flt_GetDebugVerbosity( void)
{
  return DebugLevel;  //CfgMgr_ConfigPtr()->FaultMgrCfg.DebugLevel;
}

/******************************************************************************
* Function:    Flt_InitDebugVerbosity
*
* Description: Init the current debug output level (NORMAL, VERBOSE or OFF)
*              to the value set in non-vol configuration.
*              This is done here because the fault manager is initialized
*              before the configuration manager.
*
* Parameters:  None
*
* Returns:     None
*
* Notes:
*
*****************************************************************************/
void Flt_InitDebugVerbosity( void)
{
  DebugLevel = CfgMgr_RuntimeConfigPtr()->FaultMgrCfg.DebugLevel;
}

/******************************************************************************
* Function:    Flt_GetSystemStatus
*
* Description: Returns the value of the local copy of the system status
*
* Parameters:  none
*
* Returns:     FLT_STATUS: The current system status
*
* Notes:
*
*****************************************************************************/
FLT_STATUS Flt_GetSystemStatus(void)
{
  return FaultSystemStatus;
}

/******************************************************************************
* Function:    Flt_GetSysCondOutputPin
*
* Description: Returns the current value of DIO Output Pin used to display Sys Condition
*
* Parameters:  none
*
* Returns:     DIO_OUTPUT
*
* Notes:       none
*
*****************************************************************************/
DIO_OUTPUT Flt_GetSysCondOutputPin(void)
{
  return CfgMgr_RuntimeConfigPtr()->FaultMgrCfg.SysCondDioOutPin;
}

/******************************************************************************
* Function:    Flt_GetSysAnunciationMode
*
* Description: Returns the configured mode to annunciate the system condition.
*
* Parameters:  none
*
* Returns:     DIO_OUTPUT
*
* Notes:       none
*
*****************************************************************************/
FLT_ANUNC_MODE Flt_GetSysAnunciationMode( void )
{
  return CfgMgr_RuntimeConfigPtr()->FaultMgrCfg.Mode;
}

/******************************************************************************
 * Function:    Flt_UpdateAction
 *
 * Description: Monitors the state of the system condition and then requests
 *              then configure action.
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:       none
 *
 *****************************************************************************/
void Flt_UpdateAction ( FLT_STATUS sysCond )
{
   m_Current.sysCond = sysCond;
   m_Current.nAction = CfgMgr_RuntimeConfigPtr()->FaultMgrCfg.action[sysCond];

   // Perform the configured fault Action
  if ( m_Current.sysCond != m_Previous.sysCond )
  {
     if ( ON == m_Previous.state )
     {
        m_Previous.nID = ActionRequest( m_Previous.nID, m_Previous.nAction,
                                        ACTION_OFF, FALSE, FALSE );
     }

     m_Current.state = ON;
     m_Current.nID   = ActionRequest( m_Current.nID, m_Current.nAction,
                                         ACTION_ON, FALSE, FALSE );
     m_Previous = m_Current;

  }
}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
* Function:    Flt_AddToFaultBuf
*
* Description: Add a fault log to the rolling fault buffer stored in EEPROM
*              This routine overwrites the oldest fault, replaces it with
*              the new fault and updates EEPROM.
*
* Parameters:  LogId - the new log ID
*              ts - the timestamp of the log
*
* Returns:     None
*
* Notes:       There is a chance (that after years? of use) that the faultCount
*              could wraparound (0xFFFFFFFF -> 0) however long before that
*              the EEPROM should have stopped working so we ignore the
*              possibility here
*
*****************************************************************************/
void Flt_AddToFaultBuf(SYS_APP_ID LogID, TIMESTAMP ts)
{
  // insert the data into the buffer
  faultHistory[faultIndex].Id = LogID;
  faultHistory[faultIndex].Ts = ts;
  faultHistory[faultIndex].count = faultCount;

  //Update EERPROM Log ID
  NV_Write(NV_FAULT_LOG,
           sizeof(FAULT_HISTORY)*faultIndex, &faultHistory[faultIndex],
           sizeof(FAULT_HISTORY));

  // move to the next slot & check for wrap around
  // faultIndex = (faultIndex + 1) % FLT_LOG_BUF_ENTRIES; (did not use % for speed)
  ++faultIndex;
  if (faultIndex >= FLT_LOG_BUF_ENTRIES)
  {
    faultIndex = 0;
  }

  // next fault count (see Note in header)
  ++faultCount;
}

/******************************************************************************
 * Function:    Flt_UpdateSystemStatus
 *
 * Description: Updates the Current System Status
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:       none
 *
 *****************************************************************************/
static void Flt_UpdateSystemStatus( void )
{
  UINT32 intrLevel;

  intrLevel = __DIR();
  // The FaultSystemStatus always reflect the highest request sys condition
  if ( FaultSystemStatusCnt[STA_FAULT] > 0 )
  {
    FaultSystemStatus = STA_FAULT;
  }
  else if ( FaultSystemStatusCnt[STA_CAUTION] > 0 )
  {
    FaultSystemStatus = STA_CAUTION;
  }
  else
  {
    // Clear back to STA_NORMAL;
    FaultSystemStatus = STA_NORMAL;
  }
  __RIR(intrLevel);
}

/******************************************************************************
 * Function:    Flt_LogSysStatus
 *
 * Description: Create a Sys Log entry with the current Sys Status
 *
 * Parameters:  LogID      - The ID of the CSC from SystemLog.h
 *              Status     - The status requested by the caller at LogID
 *              prevStatus - The previous system status.
 *
 * Returns:     none
 *
 * Notes:       none
 *
 *****************************************************************************/
static void Flt_LogSysStatus(SYS_APP_ID LogID, FLT_STATUS Status, FLT_STATUS prevStatus)
{
  INFO_SYS_STATUS_UPDATE_LOG sysStatusLog;
  TIMESTAMP ts;

  sysStatusLog.logID      = LogID;
  sysStatusLog.statusReq  = Status;
  sysStatusLog.sysStatus  = Flt_GetSystemStatus();
  sysStatusLog.prevStatus = prevStatus;

  // Copy the fault status counts into the log structure.
  // Could've just used the array of counts but wanted to
  // de-couple the log structure from the status counting implementation.
  sysStatusLog.statusNormalCnt  = FaultSystemStatusCnt[STA_NORMAL];
  sysStatusLog.statusCautionCnt = FaultSystemStatusCnt[STA_CAUTION];
  sysStatusLog.statusFaultCnt   = FaultSystemStatusCnt[STA_FAULT];

  CM_GetTimeAsTimestamp(&ts);
  LogWriteSystem( SYS_ID_BOX_INFO_SYS_STATUS_UPDATE, LOG_PRIORITY_LOW,
                  &sysStatusLog, sizeof(INFO_SYS_STATUS_UPDATE_LOG), &ts );

/*
  GSE_DebugStr(NORMAL,TRUE, "Sys Status Logged: S:%d P:%d R:%d Cnt: N:%d C:%d F:%d",
                            SysStatusLog.SysStatus,
                            SysStatusLog.PrevStatus,
                            SysStatusLog.StatusReq,
                            SysStatusLog.FaultSystemStatusCnt[0],
                            SysStatusLog.FaultSystemStatusCnt[1],
                            SysStatusLog.FaultSystemStatusCnt[2]);
*/
}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: FaultMgr.c $
 *
 * *****************  Version 55  *****************
 * User: John Omalley Date: 12-11-09   Time: 4:41p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review Updates
 * 
 * *****************  Version 54  *****************
 * User: Melanie Jutras Date: 12-10-10   Time: 12:43p
 * Updated in $/software/control processor/code/system
 * SCR 1172 PCLint 545 Suspicious use of & Error
 *
 * *****************  Version 53  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 52  *****************
 * User: John Omalley Date: 12-08-28   Time: 8:34a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Added ETM Fault Logic
 *
 * *****************  Version 51  *****************
 * User: John Omalley Date: 12-08-24   Time: 9:30a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - ETM Fault Action Logic
 *
 * *****************  Version 50  *****************
 * User: John Omalley Date: 12-08-16   Time: 4:15p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Fault Action Processing
 *
 * *****************  Version 49  *****************
 * User: Jim Mood     Date: 2/24/12    Time: 10:39a
 * Updated in $/software/control processor/code/system
 * SCR 1114 - Re labeled after v1.1.1 release
 *
 * *****************  Version 47  *****************
 * User: Contractor V&v Date: 12/14/11   Time: 6:50p
 * Updated in $/software/control processor/code/system
 * SCR #307 Add System Status to Fault / Event / etc logs
 *
 * *****************  Version 46  *****************
 * User: John Omalley Date: 9/09/10    Time: 2:12p
 * Updated in $/software/control processor/code/system
 * SCR 855 - Removed unused functions
 *
 * *****************  Version 45  *****************
 * User: Peter Lee    Date: 8/30/10    Time: 7:15p
 * Updated in $/software/control processor/code/system
 * SCR #838 Error - Flt_SetStatus() and Flt_ClrStatus() and Interrupt
 * Suceptibility
 *
 * *****************  Version 44  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:01a
 * Updated in $/software/control processor/code/system
 * SCR #685 - Add USER_GSE flag
 *
 * *****************  Version 43  *****************
 * User: Contractor V&v Date: 7/21/10    Time: 7:17p
 * Updated in $/software/control processor/code/system
 * SCR #538 SPIManager startup & NV Mgr Issues
 *
 * *****************  Version 42  *****************
 * User: Jeff Vahue   Date: 6/29/10    Time: 5:27p
 * Updated in $/software/control processor/code/system
 * SCR# 640 - Allow Normal verbosity debug messages during startup
 *
 * *****************  Version 41  *****************
 * User: Contractor3  Date: 6/25/10    Time: 9:46a
 * Updated in $/software/control processor/code/system
 * SCR #662 - Changes based on code review
 *
 * *****************  Version 40  *****************
 * User: Jim Mood     Date: 6/11/10    Time: 3:30p
 * Updated in $/software/control processor/code/system
 * SCR 623 Batch configuration implementation updates
 *
 * *****************  Version 39  *****************
 * User: Contractor V&v Date: 6/08/10    Time: 5:54p
 * Updated in $/software/control processor/code/system
 * SCR #614 fast.reset=really should initialize files
 *
 * *****************  Version 38  *****************
 * User: Contractor2  Date: 6/07/10    Time: 1:28p
 * Updated in $/software/control processor/code/system
 * SCR #485 Escape Sequence & Box Config
 *
 * *****************  Version 37  *****************
 * User: Contractor V&v Date: 3/29/10    Time: 6:17p
 * Updated in $/software/control processor/code/system
 * SCR #513 Configure fault status  LED
 *
 * *****************  Version 36  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:36p
 * Updated in $/software/control processor/code/system
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 *
 * *****************  Version 35  *****************
 * User: Contractor V&v Date: 3/19/10    Time: 4:31p
 * Updated in $/software/control processor/code/system
 * SCR #279 Making NV filenames uppercase SCR #248 Parameter Log C
 *
 * *****************  Version 34  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/system
 * SCR# 483 - Function Names
 *
 * *****************  Version 33  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:57p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 32  *****************
 * User: Jeff Vahue   Date: 3/02/10    Time: 10:49a
 * Updated in $/software/control processor/code/system
 * Fix references to SCR 329 to SCR 362, version 29 and 31 comments
 *
 * *****************  Version 31  *****************
 * User: Jeff Vahue   Date: 3/02/10    Time: 10:44a
 * Updated in $/software/control processor/code/system
 * SCR# 362 - more init clean up of the recent fault buffer
 *
 * *****************  Version 30  *****************
 * User: Jeff Vahue   Date: 2/25/10    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * Output CR/LF before each Debug message
 *
 * *****************  Version 29  *****************
 * User: Jeff Vahue   Date: 2/19/10    Time: 12:56p
 * Updated in $/software/control processor/code/system
 * SCR# 455 - LINT Issue removal
 * DebugStr - remove error detection
 *
 * *****************  Version 28  *****************
 * User: Jeff Vahue   Date: 2/17/10    Time: 1:25p
 * Updated in $/software/control processor/code/system
 * SCR# 362 - correct initialization of recent fault buffer when the
 * EEPROM is reset via a fast.reset command
 *
 * *****************  Version 27  *****************
 * User: Contractor V&v Date: 2/03/10    Time: 2:57p
 * Updated in $/software/control processor/code/system
 * SCR 279
 *
 * *****************  Version 26  *****************
 * User: Jeff Vahue   Date: 1/13/10    Time: 11:21a
 * Updated in $/software/control processor/code/system
 * SCR# 330
 *
 * *****************  Version 25  *****************
 * User: Jeff Vahue   Date: 1/11/10    Time: 12:22p
 * Updated in $/software/control processor/code/system
 * SCR# 388
 *
 * *****************  Version 24  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:25p
 * Updated in $/software/control processor/code/system
 * SCR 371, SCR 386
 *
 * *****************  Version 23  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 3:42p
 * Updated in $/software/control processor/code/system
 * SCR# 364
 *
 * *****************  Version 22  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 3:11p
 * Updated in $/software/control processor/code/system
 * SCR# 326 cleanup and remerge
 *
 * *****************  Version 21  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 2:11p
 * Updated in $/software/control processor/code/system
 * SCR# 326
 *
 * *****************  Version 20  *****************
 * User: Peter Lee    Date: 12/21/09   Time: 6:19p
 * Updated in $/software/control processor/code/system
 * SCR #380 Implement Real Time Sys Condition Logic
 *
 * *****************  Version 19  *****************
 * User: Peter Lee    Date: 12/18/09   Time: 4:34p
 * Updated in $/software/control processor/code/system
 * SCR #368 Calling Flt_SetStatus() before Flt_Init().  Added additional
 * debug statements.
 *
 * *****************  Version 18  *****************
 * User: Jeff Vahue   Date: 12/17/09   Time: 12:12p
 * Updated in $/software/control processor/code/system
 * SCR# 364 - INT8 -> CHAR type
 *
 * *****************  Version 17  *****************
 * User: Contractor V&v Date: 12/10/09   Time: 5:45p
 * Updated in $/software/control processor/code/system
 * SCR 106
 *
 * *****************  Version 16  *****************
 * User: Peter Lee    Date: 12/02/09   Time: 2:17p
 * Updated in $/software/control processor/code/system
 * SCR #350 Misc Code Review Items
 *
 * *****************  Version 15  *****************
 * User: Peter Lee    Date: 11/23/09   Time: 2:53p
 * Updated in $/software/control processor/code/system
 *
 * *****************  Version 14  *****************
 * User: Contractor V&v Date: 11/18/09   Time: 3:56p
 * Updated in $/software/control processor/code/system
 * Implement SCR 172 ASSERT processing for all "default" case processing
 * Implement SCR 138 Consistency with naming index of [0]
 *
 * *****************  Version 13  *****************
 * User: John Omalley Date: 10/01/09   Time: 4:27p
 * Updated in $/software/control processor/code/system
 * Updated the System Status for sensors
 *
 * *****************  Version 12  *****************
 * User: Jim Mood     Date: 9/29/09    Time: 4:31p
 * Updated in $/software/control processor/code/system
 * SCR #178 User Manager updates to user manager table
 *
 * *****************  Version 11  *****************
 * User: Peter Lee    Date: 9/29/09    Time: 1:25p
 * Updated in $/software/control processor/code/system
 * SCR #280 Fix Compiler Warnings
 *
 * *****************  Version 10  *****************
 * User: Peter Lee    Date: 6/30/09    Time: 2:41p
 * Updated in $/software/control processor/code/system
 * SCR #204 Misc Code Review Updates
 *
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 5/21/09    Time: 4:51p
 * Updated in $/software/control processor/code/system
 * SCR #193 Informal Code Review Update
 *
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 4/27/09    Time: 3:48p
 * Updated in $/software/control processor/code/system
 * SCR #168 Updates from preliminary code review.
 *
 * *****************  Version 7  *****************
 * User: Jim Mood     Date: 4/07/09    Time: 10:13a
 * Updated in $/control processor/code/system
 * SCR# 161
 *
 * *****************  Version 6  *****************
 * User: Jim Mood     Date: 1/26/09    Time: 4:39p
 * Updated in $/control processor/code/system
 * Changed StatusStr prototype to accept const class strings
 *
 * *****************  Version 5  *****************
 * User: Jim Mood     Date: 11/12/08   Time: 1:40p
 * Updated in $/control processor/code/system
 *
 * *****************  Version 4  *****************
 * User: Jim Mood     Date: 10/29/08   Time: 3:26p
 * Updated in $/control processor/code/system
 *
 * *****************  Version 3  *****************
 * User: Jim Mood     Date: 10/24/08   Time: 10:30a
 * Updated in $/control processor/code/system
 *
 * *****************  Version 2  *****************
 * User: Jim Mood     Date: 10/07/08   Time: 5:56p
 * Updated in $/control processor/code/system
 *
 * *****************  Version 1  *****************
 * User: Jim Mood     Date: 9/16/08    Time: 10:26a
 * Created in $/control processor/code/system
 *
 ***************************************************************************/
