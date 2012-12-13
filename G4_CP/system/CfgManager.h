#ifndef CMUTIL_H
#define CMUTIL_H
/******************************************************************************
            Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

 File: CfgManager.h

 Description: Definitions for the non-volatile configuration data.

 VERSION
 $Revision: 52 $  $Date: 12/13/12 3:02p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
//Include components that store (a) data structure(s) in the EEPROM

#include "fault.h"
#include "MSStsCtl.h"
#include "MSInterface.h"
#include "Sensor.h"
#include "Trigger.h"
#include "DataManager.h"
#include "FASTMgr.h"
#include "arinc429mgr.h"
#include "qar.h"
#include "AircraftConfigMgr.h"
#include "PowerManager.h"
#include "MSFileXfr.h"
#include "UartMgr.h"
#include "F7XProtocol.h"
#include "EMU150Protocol.h"
#include "FASTStateMgr.h"
#include "Event.h"
#include "ActionManager.h"
#include "TimeHistory.h"
#include "EngineRun.h"
#include "Cycle.h"
#include "Trend.h"
#include "Creep.h"
#ifdef ENV_TEST
#include "Etm.h"
#endif

/******************************************************************************
                                 Package Defines
******************************************************************************/
#define TEXT_BUF_SIZE 64
#define SYS_HDR_VERSION 3
#define ETM_HDR_VERSION 1



#define RESET_CFGMGR_ONLY FALSE
#define RESET_ALL         TRUE

// enumeration of the reasons the system is
// initialized using the default config.
typedef enum
{
  REASON_NORMAL,
  REASON_VERSION_MISMATCH,
  REASON_CFGMGR_OPEN_FAILED,
  REASON_USER_COMMAND,
  /* Add new reset reason codes directly above this line */
  MAX_RESET_REASON
} RESET_REASON;


/******************************************************************************
                                 Package Typedefs
******************************************************************************/


//This structure contains the data that other software components
//use to store their non-volatile data.
typedef struct
{
    //Software build time and version
    // FAST Specific
    INT8                    Id[TEXT_BUF_SIZE];  // FAST installation id
    UINT16                  VerId;              // FAST/DTU version ID
    //Microserver status and control
    MSCP_WLAN_CONFIG        WlanConfig;
    MSSC_GSM_CONFIG         GsmConfig;
    MSSC_CONFIG             MsscConfig;
    //Sensor configuration
    SENSOR_CONFIGS          SensorConfigs;
    //Trigger Configuration
    TRIGGER_CONFIGS         TriggerConfigs;
    // ACS Configuration
    ACS_CONFIGS             ACSConfigs;
    // FAST Mgr configuration
    FASTMGR_CONFIG          FASTCfg;
    // ARINC429 Configuration
    ARINC429_CFG            Arinc429Config;
    // QAR Configuration
    QAR_CONFIGURATION       QARConfig;
    // Live Data Configuration
    SENSOR_LD_CONFIG        LiveDataConfig;
    // Aircraft tail-number based configuration
    AIRCRAFT_CONFIG         Aircraft;
    // PM Configuration
    POWERMANAGER_CFG        PowerManagerCfg;
    // Fault Manager configuration.
    FAULTMGR_CONFIG         FaultMgrCfg;
    // Signal/Trigger configurations.
    FAULT_CONFIGS           FaultConfig;
    //File Transfer configuration
    MSFX_CFG_BLOCK          Msfx;
    // Uart Manager configuration
    UARTMGR_CFGS            UartMgrConfig;
    // F7X Protocol configuration
    F7X_DUMPLIST_CFGS       F7XConfig;
    // F7X Protocol Param configuration
    F7X_PARAM_LIST          F7XParamConfig;
    // EMU150 Protocol configuration
    EMU150_CFG              EMU150Config;
    // FAST State Machine configuration
    FSM_CONFIG              FSMConfig;
    // EVENT Configuration
    EVENT_CONFIGS           EventConfigs;
    // EVENT Table Configuration
    EVENT_TABLE_CONFIGS     EventTableConfigs;
    // Action Configuration
    ACTION_CFG              ActionConfig;
    // TIMEHISTORY Configuration
    TIMEHISTORY_CONFIG      TimeHistoryConfig;
    // ENGINERUN Configuration
    ENGRUN_CFGS             EngineRunConfigs;
    // CYCLE Configuration
    CYCLE_CFGS              CycleConfigs;
    // TREND Configuration
    TREND_CONFIGS           TrendConfigs;
    // CREEP Configuration
    CREEP_CFG               CreepConfig;
#ifdef ENV_TEST
    TestControl             etm;
#endif
} CFGMGR_NVRAM;

//Configuration structure including the checksum to be stored in
//EEPROM.  Compiler padding of the structure can make it difficult to compute
//the checksum of the data so, a separate structure containing the checksum is
//used.
typedef struct
{
  CFGMGR_NVRAM cfg;
  UINT16 cs;
}CFGMGR_NVRAM_CS;



/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( CMUTIL_BODY )
  #define EXPORT
#else
  #define EXPORT extern
#endif

/******************************************************************************
                             Package Exports Variables
******************************************************************************/
////extern CM_ACS_SUMMARY_TBL AcsSummaryTbl;


/******************************************************************************
                             Package Exports Functions
******************************************************************************/
EXPORT void CfgMgr_Init( BOOLEAN degraded );
EXPORT BOOLEAN CfgMgr_FileInit( void );
EXPORT void CfgMgr_ResetConfig( BOOLEAN bResetAll, RESET_REASON reason );
EXPORT CFGMGR_NVRAM* CfgMgr_ConfigPtr(void);
EXPORT CFGMGR_NVRAM* CfgMgr_RuntimeConfigPtr(void);

EXPORT BOOLEAN CfgMgr_RetrieveConfig( CFGMGR_NVRAM_CS* pInfo );
EXPORT BOOLEAN CfgMgr_StoreConfig( CFGMGR_NVRAM_CS* pInfo );
EXPORT BOOLEAN CfgMgr_StoreConfigItem( void *pOffset, void *pSrc, UINT16 nSize );
EXPORT void CfgMgr_GenerateDebugLogs(void);
EXPORT void CfgMgr_StartBatchCfg(void);
EXPORT void CfgMgr_CancelBatchCfg(void);
EXPORT void CfgMgr_CommitBatchCfg(void);
EXPORT BOOLEAN CfgMgr_IsEEWritePending(void);
EXPORT UINT16 CfgMgr_GetSystemBinaryHdr(INT8 *pDest, UINT16 nMaxByteSize );
EXPORT UINT16 CfgMgr_GetETMBinaryHdr(INT8 *pDest, UINT16 nMaxByteSize );
#endif // CMUTIL_H

/*************************************************************************
 *  MODIFICATIONS
 *    $History: CfgManager.h $
 * 
 * *****************  Version 52  *****************
 * User: Jim Mood     Date: 12/13/12   Time: 3:02p
 * Updated in $/software/control processor/code/system
 * SCR #1197 Code Review Updates
 *
 * *****************  Version 51  *****************
 * User: John Omalley Date: 12-11-13   Time: 11:10a
 * Updated in $/software/control processor/code/system
 * SCR 1142 - Code Review Formatting Issues
 *
 * *****************  Version 50  *****************
 * User: John Omalley Date: 12-11-12   Time: 4:46p
 * Updated in $/software/control processor/code/system
 * SCR 1142 - Formatting Error
 *
 * *****************  Version 49  *****************
 * User: Peter Lee    Date: 12-10-27   Time: 4:58p
 * Updated in $/software/control processor/code/system
 * SCR #1190 Creep Requirements
 *
 * *****************  Version 48  *****************
 * User: Contractor V&v Date: 9/14/12    Time: 4:49p
 * Updated in $/software/control processor/code/system
 * FAST 2 Trend config support
 *
 * *****************  Version 47  *****************
 * User: John Omalley Date: 12-09-11   Time: 2:09p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Added Binary ETM Header Logic
 *
 * *****************  Version 46  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 45  *****************
 * User: John Omalley Date: 12-08-09   Time: 8:34a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Updated the System Binary Header to Version 3
 *
 * *****************  Version 44  *****************
 * User: John Omalley Date: 12-07-17   Time: 11:28a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Added new Action Object
 *
 * *****************  Version 43  *****************
 * User: John Omalley Date: 12-07-13   Time: 3:49p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Added Action Configuration
 *
 * *****************  Version 42  *****************
 * User: John Omalley Date: 12-06-07   Time: 11:34a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Added Trend Config
 *
 * *****************  Version 41  *****************
 * User: Contractor V&v Date: 5/10/12    Time: 6:42p
 * Updated in $/software/control processor/code/system
 * Added engine run and cycle to Cfg Manager
 *
 * *****************  Version 40  *****************
 * User: John Omalley Date: 4/27/12    Time: 5:04p
 * Updated in $/software/control processor/code/system
 *
 * *****************  Version 38  *****************
 * User: Jim Mood     Date: 7/20/11    Time: 2:57p
 * Updated in $/software/control processor/code/system
 * SCR 575: GSM Enable when engine status is lost.  (Part of changes for
 *  * the Fast State Machine)
 *
 * *****************  Version 37  *****************
 * User: Peter Lee    Date: 4/11/11    Time: 10:13a
 * Updated in $/software/control processor/code/system
 * SCR #1029 EMU150 Download functionality
 *
 * *****************  Version 36  *****************
 * User: John Omalley Date: 8/05/10    Time: 11:40a
 * Updated in $/software/control processor/code/system
 * SCR 750 - Remove TX Method
 *
 * *****************  Version 35  *****************
 * User: John Omalley Date: 7/30/10    Time: 5:33p
 * Updated in $/software/control processor/code/system
 * SCR 699 - Cfg_MgrInitialization changed return to void
 *
 * *****************  Version 34  *****************
 * User: Contractor V&v Date: 7/27/10    Time: 6:20p
 * Updated in $/software/control processor/code/system
 * SCR #711 Error: Dpram Int Failure incorrectly forces STA_FAULT.
 *
 * *****************  Version 33  *****************
 * User: John Omalley Date: 7/20/10    Time: 9:04a
 * Updated in $/software/control processor/code/system
 * SCR 294 - Add system binary header
 *
 * *****************  Version 32  *****************
 * User: Jim Mood     Date: 6/11/10    Time: 3:31p
 * Updated in $/software/control processor/code/system
 * SCR 623 Batch configuration implementation updates
 *
 * *****************  Version 31  *****************
 * User: Jim Mood     Date: 6/11/10    Time: 10:16a
 * Updated in $/software/control processor/code/system
 * SCR 623
 *
 * *****************  Version 30  *****************
 * User: Contractor V&v Date: 6/08/10    Time: 5:53p
 * Updated in $/software/control processor/code/system
 * SCR #614 fast.reset=really should initialize files
 *
 * *****************  Version 29  *****************
 * User: John Omalley Date: 6/08/10    Time: 12:13p
 * Updated in $/software/control processor/code/system
 * SCR 627 - Updated for LJ60
 *
 * *****************  Version 28  *****************
 * User: Peter Lee    Date: 5/31/10    Time: 6:15p
 * Updated in $/software/control processor/code/system
 * SCR #618 F7X and UartMgr Requirements Implementation
 *
 * *****************  Version 27  *****************
 * User: Contractor V&v Date: 5/12/10    Time: 3:48p
 * Updated in $/software/control processor/code/system
 * SCR #543 Misc - reduce complexity
 *
 * *****************  Version 26  *****************
 * User: Contractor V&v Date: 5/07/10    Time: 4:35p
 * Updated in $/software/control processor/code/system
 * SCR #543 Misc - reduce complexity
 *
 * *****************  Version 25  *****************
 * User: Contractor V&v Date: 4/28/10    Time: 4:53p
 * Updated in $/software/control processor/code/system
 * SCR #571 SysCond should be fault when default config is loaded
 *
 * *****************  Version 24  *****************
 * User: Contractor V&v Date: 4/22/10    Time: 6:38p
 * Updated in $/software/control processor/code/system
 * SCR #187 Run in degraded mode
 *
 * *****************  Version 23  *****************
 * User: Contractor V&v Date: 4/12/10    Time: 7:21p
 * Updated in $/software/control processor/code/system
 * SCR #540 Log reason for cfg reset
 *
 * *****************  Version 22  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:10p
 * Updated in $/software/control processor/code/system
 * SCR #267 Add fields to Upload print
 *
 * *****************  Version 21  *****************
 * User: Contractor V&v Date: 3/29/10    Time: 6:17p
 * Updated in $/software/control processor/code/system
 * SCR #140 Move Log erase status to own file
 *
 * *****************  Version 20  *****************
 * User: Contractor V&v Date: 3/10/10    Time: 4:41p
 * Updated in $/software/control processor/code/system
 * SCR #464 Move Version Info into own file and support it.
 *
 * *****************  Version 19  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:57p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 18  *****************
 * User: Contractor V&v Date: 2/03/10    Time: 2:57p
 * Updated in $/software/control processor/code/system
 * SCR 279
 *
 * *****************  Version 17  *****************
 * User: Jim Mood     Date: 11/09/09   Time: 3:17p
 * Updated in $/software/control processor/code/system
 * Added configuration items for MSFX ( micro-server file transfer )
 * system
 *
 * *****************  Version 16  *****************
 * User: Jim Mood     Date: 10/14/09   Time: 2:33p
 * Updated in $/software/control processor/code/system
 * SCR #101, sw version and build date stored in EE and checked on
 * startup.
 *
 * *****************  Version 15  *****************
 * User: John Omalley Date: 10/01/09   Time: 4:27p
 * Updated in $/software/control processor/code/system
 * Added fault configuration
 *
 * *****************  Version 14  *****************
 * User: Peter Lee    Date: 9/28/09    Time: 6:26p
 * Updated in $/software/control processor/code/system
 * scr #277 Power Management Requirements Implemented
 *
 * *****************  Version 13  *****************
 * User: Jim Mood     Date: 9/11/09    Time: 10:50a
 * Updated in $/software/control processor/code/system
 *
 * *****************  Version 12  *****************
 * User: Peter Lee    Date: 6/29/09    Time: 3:20p
 * Updated in $/software/control processor/code/system
 * SCR #204 Misc Code Review Updates
 *
 ***************************************************************************/


