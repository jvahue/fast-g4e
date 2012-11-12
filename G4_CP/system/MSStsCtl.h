#ifndef MSSC_H
#define MSSC_H
/******************************************************************************
            Copyright (C) 2007-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:         MSStsCtl.h

    Description: MicroServer Status and Control.

    VERSION
      $Revision: 26 $  $Date: 12-11-12 10:57a $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "MSCPInterface.h"
#include "FaultMgr.h"

/******************************************************************************
                                 Package Defines                              *
******************************************************************************/
#define MSI_WLAN_CONFIG_DEFAULT     MSCP_WLAN_MODE_MANAGED,           /*Mode*/\
                                    "ANY",                           /*EssId*/\
                                    "05C6-97E3-5B22-55A5-16FF-D0BF-9F",/*Wep*/\
                                    " ",                                /*Ip*/\
                                    "10.17.1.129",                 /*GateWay*/\
                                    " "                            /*NetMask*/

#define MSI_GSM_CONFIG_DEFAULT      " ",                             /*Phone*/\
                                    MSCP_GSM_MODE_GPRS,               /*Mode*/\
                                    " ",                               /*APN*/\
                                    " ",                              /*User*/\
                                    " ",                          /*Password*/\
                                    " ",                           /*Carrier*/\
                                    " ",                               /*MCC*/\
                                    " "                                /*MNC*/

#define MSI_CONFIG_DEFAULT          STA_CAUTION,               /*PBITSysCond*/\
                                    STA_CAUTION,               /*CBITSysCond*/\
                                    120,                /*HeartBeatStartup_s*/\
                                    3,                     /*HeartBeatLoss_s*/\
                                    5,                     /*HeartBeatLogCnt*/\
                                    100                   /*Pri4DirMaxSizeMB*/

#define MSSC_CFG_STR_MAX_LEN        64
/******************************************************************************
                                 Package Typedefs                             *
******************************************************************************/
typedef enum {
  MSSC_STS_NOT_VLD,
  MSSC_STS_OFF,
  MSSC_STS_ON_W_LOGS,
  MSSC_STS_ON_WO_LOGS
} MSSC_STS_STATUS;

typedef enum {
  MSSC_XFR_NOT_VLD,
  MSSC_XFR_NONE,
  MSSC_XFR_DATA_LOG,
  MSSC_XFR_SPECIAL_TX
} MSSC_XFR_STATUS;

typedef enum {
  MSSC_STARTUP,
  MSSC_RUNNING,
  MSSC_DEAD
}MSSC_MSSIM_STATUS;

//Configuration data for the GSM modem.  This data is sent to the micro-server
//to make a connected with a defined SIM card.
#pragma pack(1)
typedef struct {
  INT8          sPhone[MSSC_CFG_STR_MAX_LEN];
  MSCP_GSM_MODE mode;
  INT8          sAPN[MSSC_CFG_STR_MAX_LEN];
  INT8          sUser[MSSC_CFG_STR_MAX_LEN];
  INT8          sPassword[MSSC_CFG_STR_MAX_LEN];
  INT8          sCarrier[MSSC_CFG_STR_MAX_LEN];
  INT8          sMCC[MSSC_CFG_STR_MAX_LEN];
  INT8          sMNC[MSSC_CFG_STR_MAX_LEN];
} MSSC_GSM_CONFIG;
#pragma pack()

#pragma pack(1)

//Configuration data for the micro-server
typedef struct {
  FLT_STATUS sysCondPBIT;         // PBIT Status
  FLT_STATUS sysCondCBIT;         // CBIT Status
  UINT16     heartBeatStartup_s;  // Heartbeat startup-timeout value in secs.
  UINT16     heartBeatLoss_s;     // Heartbeat normal-timeout  value in secs.
  UINT8      heartBeatLogCnt;     // Max number of timout logs to write
  UINT16     pri4DirMaxSizeMB;    // Size limit for the Priority 4 logs dir on the CF
} MSSC_CONFIG;

// Fail Log for Heartbeat Timeout
typedef struct {
  BOOLEAN  operational;         // True if system in Normal mode when timeout
                                // False if system in Startup mode when timeout
} MSSC_HB_FAIL_LOG;

// Fail Log Micro-Server version mismatch
// 9-chars sufficient to store nn.nn.nnl,
typedef struct {
  CHAR sMssimVer[9];
} MSSC_MS_VERSION_ERR_LOG;

#pragma pack()

// enum used as param in MSSC_GetConfigSysCond
// to return system condition
typedef enum {
  MSSC_PBIT_SYSCOND,
  MSSC_CBIT_SYSCOND
}MSSC_SYSCOND_TEST_TYPE;



/******************************************************************************
                                 Package Exports                              *
******************************************************************************/
#undef EXPORT

#if defined ( MSSC_BODY )
   #define EXPORT
#else
   #define EXPORT extern
#endif

/******************************************************************************
                             Package Exports Variables
******************************************************************************/

/******************************************************************************
                             Package Exports Functions
******************************************************************************/
EXPORT void MSSC_Init(void);
EXPORT void MSSC_SetIsOnGround(BOOLEAN OnGround);
EXPORT void MSSC_SendGSMCfgCmd(void);

EXPORT BOOLEAN MSSC_GetIsVPNConnected(void);
EXPORT BOOLEAN MSSC_GetIsCompactFlashMounted(void);
EXPORT BOOLEAN MSSC_GetIsAlive(void);
EXPORT BOOLEAN MSSC_GetMSTimeSynced(void);

EXPORT MSSC_STS_STATUS MSSC_GetSTS(void);
EXPORT MSSC_XFR_STATUS MSSC_GetXFR(void);

EXPORT TIMESTRUCT         MSSC_GetLastMSSIMTime(void);
EXPORT MSSC_MSSIM_STATUS  MSSC_GetMsStatus(void);
EXPORT FLT_STATUS         MSSC_GetConfigSysCond(MSSC_SYSCOND_TEST_TYPE testType);
EXPORT void               MSSC_DoRefreshMSInfo(void);
EXPORT void               MSSC_GetGSMSCID(CHAR* str);
EXPORT void               MSSC_GetGSMSignalStrength(CHAR* str);
EXPORT BOOLEAN            MSSC_FSMGetVPNStatus(INT32 param);
EXPORT BOOLEAN            MSSC_FSMGetCFStatus(INT32 param);
EXPORT void               MSSC_GetMsPwVer(CHAR* str);


#endif // MSSC_H
/*************************************************************************
 *  MODIFICATIONS
 *    $History: MSStsCtl.h $
 *
 * *****************  Version 26  *****************
 * User: John Omalley Date: 12-11-12   Time: 10:57a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review Updates
 *
 * *****************  Version 25  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 24  *****************
 * User: Jim Mood     Date: 7/19/12    Time: 11:07a
 * Updated in $/software/control processor/code/system
 * SCR 1107: Data Offload changes for 2.0.0
 *
 * *****************  Version 23  *****************
 * User: Jim Mood     Date: 7/26/11    Time: 3:06p
 * Updated in $/software/control processor/code/system
 * SCR 575 updates
 *
 * *****************  Version 22  *****************
 * User: Jim Mood     Date: 7/20/11    Time: 10:57a
 * Updated in $/software/control processor/code/system
 * SCR 575: GSM Enable when engine status is lost.  (Part of changes for
 * the Fast State Machine)
 *
 * *****************  Version 21  *****************
 * User: Contractor2  Date: 5/18/11    Time: 2:38p
 * Updated in $/software/control processor/code/system
 * SCR #1031 Enhancement: Add PW_Version to box.status
 *
 * *****************  Version 20  *****************
 * User: John Omalley Date: 9/10/10    Time: 10:01a
 * Updated in $/software/control processor/code/system
 * SCR 858 - Added FATAL to default cases
 *
 * *****************  Version 19  *****************
 * User: John Omalley Date: 9/09/10    Time: 2:14p
 * Updated in $/software/control processor/code/system
 * SCR 855 - Removed unused functions
 *
 * *****************  Version 18  *****************
 * User: John Omalley Date: 8/05/10    Time: 11:40a
 * Updated in $/software/control processor/code/system
 * SCR 750 - Remove TX Method
 *
 * *****************  Version 17  *****************
 * User: Contractor2  Date: 8/02/10    Time: 3:42p
 * Updated in $/software/control processor/code/system
 * SCR #762 Error: Micro-Server Interface Logic doesn't work
 *
 * *****************  Version 16  *****************
 * User: Contractor2  Date: 7/30/10    Time: 2:29p
 * Updated in $/software/control processor/code/system
 * SCR #90;243;252 Implementation: CBIT of mssim
 *
 * *****************  Version 15  *****************
 * User: Jim Mood     Date: 7/30/10    Time: 11:28a
 * Updated in $/software/control processor/code/system
 * SCR 327 Fast Transmission Test and
 * SCR 479 ms.status commands
 *
 * *****************  Version 14  *****************
 * User: Jim Mood     Date: 7/30/10    Time: 8:57a
 * Updated in $/software/control processor/code/system
 * SCF 327 Fast transmission test functionality and gsm status information
 *
 * *****************  Version 13  *****************
 * User: Contractor V&v Date: 7/27/10    Time: 6:20p
 * Updated in $/software/control processor/code/system
 * SCR #711 Error: Dpram Int Failure incorrectly forces STA_FAULT.
 *
 * *****************  Version 12  *****************
 * User: Peter Lee    Date: 7/26/10    Time: 6:01p
 * Updated in $/software/control processor/code/system
 * SCR #714 Update default gsm cfg fields to " ".
 *
 * *****************  Version 11  *****************
 * User: Contractor V&v Date: 7/21/10    Time: 7:18p
 * Updated in $/software/control processor/code/system
 * SCR #260 Implement BOX status command
 *
 * *****************  Version 10  *****************
 * User: Contractor V&v Date: 5/26/10    Time: 2:11p
 * Updated in $/software/control processor/code/system
 * SCR #607 The CP shall query Ms CF mounted before File Upload pr
 *
 * *****************  Version 9  *****************
 * User: Jim Mood     Date: 5/10/10    Time: 9:16a
 * Updated in $/software/control processor/code/system
 * SCR 588, VPN Connected
 *
 * *****************  Version 8  *****************
 * User: Contractor V&v Date: 4/22/10    Time: 6:41p
 * Updated in $/software/control processor/code/system
 * SCR #453 Misc fix
 *
 * *****************  Version 7  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 *
 *
 ***************************************************************************/
