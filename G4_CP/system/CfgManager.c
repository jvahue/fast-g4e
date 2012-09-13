#define CMUTIL_BODY
/******************************************************************************
            Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.


    File:     CfgManager.c


    Description:


    VERSION
    $Revision: 68 $  $Date: 12-09-11 2:26p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_basic.h"
#include "NVMgr.h"
#include "Monitor.h"
#include "utility.h"
#include "Cfgmanager.h"
#include "GSE.h"
#include "LogManager.h"
#include "Version.h"

#include "TestPoints.h"
/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static CFGMGR_NVRAM_CS NVRAMShadow;
static CFGMGR_NVRAM_CS m_BatchModeShadow;
static BOOLEAN         m_IsInBatchMode    =  FALSE;
static const CFGMGR_NVRAM DefaultNVCfg =  {
                                          " ",   //Id
                                          0,     //VerId

                                          //Micro-server default data
                                          {MSI_WLAN_CONFIG_DEFAULT},
                                          {MSI_GSM_CONFIG_DEFAULT},
                                          {MSI_CONFIG_DEFAULT},
                                          //Sensors
                                          {SENSOR_CONFIGS_DEFAULT},
                                          //Triggers
                                          {TRIGGER_CONFIG_DEFAULT},
                                          //ACS
                                          {ACS_CONFIGS_DEFAULT},
                                          //FAST Manager
                                          {FASTMGR_CONFIG_DEFAULT},
                                          // ARINC429
                                          {ARINC429_CFG_DEFAULT},
                                          // QAR
                                          {QAR_CFG_DEFAULT},
                                          // LIVE DATA
                                          {SENSOR_LIVEDATA_DEFAULT},
                                          // Aircraft tail-number based configuration
                                          {AC_CONFIG_DEFAULT },
                                          // Power Manager default config
                                          {PM_CFG_DEFAULT},
                                          // Fault Manager default config
                                          {FAULTMGR_CONFIG_DEFAULT},
                                          // Fault Configuration
                                          {FAULT_CONFIGS_DEFAULT },
                                          // Micro-server default config
                                          {MSFX_CFG_DEFAULT},
                                          // Uart Manager config
                                          {UARTMGR_DEFAULT},
                                          // F7X Protocol config
                                          {F7X_PROTOCOL_DEFAULT},
                                          // F7X Protocol Param config
                                          {F7X_PROTOCOL_PARAM_DEFAULT},
                                          // EMU150 Protocol config
                                          {EMU150_CFG_DEFAULT},
                                          // FAST State Machine config
                                          {FSM_DEFAULT_CFG},
                                          // EVENT Config
                                          {EVENT_CONFIG_DEFAULT},
                                          // EVENT TABLE Config
                                          {EVENT_TABLE_CFG_DEFAULT},
                                          // EVENT ACTION CFG
                                          {ACTION_DEFAULT},
                                          // TIMEHISTORY Table Config
                                          {TIMEHISTORY_DEFAULT},
                                          // ENGINERUN Config
                                          {ENGRUN_CFG_DEFAULT},
                                          // CYCLE Config
                                          {CYCLE_CFG_DEFAULT},
                                          // TREND Config
                                          {TREND_CFG_DEFAULT}
                                          //...more configuration data goes
                                          //   here
#ifdef ENV_TEST
                                          ,{ ETM_DEFAULT }
#endif
                                          };

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:     CfgMgr_Init
 *
 * Description:
 *
 * Parameters:   degraded - TRUE -  degraded mode, use the DefaultNvMgrConfig for
 *                          FALSE - use the normal NVCfgMgr file.
 *
 * Returns:      RESULT
 *
 *****************************************************************************/
void CfgMgr_Init( BOOLEAN degradedMode )
{
  RESET_REASON reason     = REASON_NORMAL;
  BOOLEAN bVersionMatched = TRUE;
  SYS_APP_ID LoggingId    = SYS_ID_NULL_LOG;
  RESULT  NvCfgOpenStatus = NV_Open(NV_CFG_MGR);

  // Operation control flags - init for REASON_NORMAL
  BOOLEAN bReadCfgFile     = FALSE;
  BOOLEAN bResetCfgFile    = FALSE;

  // Ensure default values are initially in shadow memory /
  // This handles degraded mode and cases where NVRAM cannot be read
  memcpy(&NVRAMShadow.Cfg, &DefaultNVCfg, sizeof(NVRAMShadow.Cfg));

  if ( !degradedMode ) // Performing a normal startup
  {
    bReadCfgFile    = TRUE;
    bResetCfgFile   = FALSE;

    bVersionMatched = Ver_CheckVerAndDateMatch();

    if( SYS_OK != NvCfgOpenStatus )
    {
      reason        = REASON_CFGMGR_OPEN_FAILED;
      bReadCfgFile  = FALSE;
      bResetCfgFile = TRUE;
      LoggingId     = SYS_CFG_PBIT_OPEN_CFG_FAIL;
    }
    else if( !bVersionMatched )
    {
      GSE_DebugStr(NORMAL,TRUE, "CfgMgr: Reset cfg because build date or version changed");
      reason        = REASON_VERSION_MISMATCH;
      bReadCfgFile  = FALSE;
      bResetCfgFile = TRUE;
      LoggingId     = SYS_CFG_RESET_TO_DEFAULT;
    }
  }

  // Read the NvCfg file
  if( bReadCfgFile )
  {
    // Read data into cfg shadow and return
    NV_Read( NV_CFG_MGR, 0, &NVRAMShadow, sizeof(NVRAMShadow) );
    GSE_DebugStr(VERBOSE,TRUE,"CfgMgr_Init: NV configuration load success");
  }

  // Perform a reset of the NvCfg file
  if( bResetCfgFile )
  {
    CfgMgr_ResetConfig(RESET_CFGMGR_ONLY, reason);
    // The above call will create a log entry. Reset the LoggingId & reason so we
    // don't issue a dup-log at the end of this function unless a subsequent error is detected.
    LoggingId = SYS_ID_NULL_LOG;
    reason    = REASON_NORMAL;

    //EEPROM write okay, check configuration by performing a read-back retrieve.
    if( TRUE == STPU( CfgMgr_RetrieveConfig( &NVRAMShadow ), eTpCfg3747) )
    {
      GSE_DebugStr(VERBOSE,TRUE,"CfgMgr_Init: Restored NV configuration defaults");
    }
    else
    {
      //EEPROM reset back to default-config failed according to
      // the result of the read-back. Change the logging reason.
      GSE_DebugStr(NORMAL,TRUE, "CfgMgr_Init: Save NV defaults failed");
      LoggingId = SYS_CFG_PBIT_REINIT_CFG_FAIL; //DaveB find better id when CRC mismatch
    }
  }

  // Set status and log it.
  if (SYS_ID_NULL_LOG != LoggingId)
  {
    Flt_SetStatus(STA_FAULT, LoggingId, NULL, 0);
  }

  // if the default config is loaded
  // Set the system-condition status to fault
  // No log message.
  if(CfgMgr_RuntimeConfigPtr()->VerId == 0)
  {
    Flt_SetStatus( STA_FAULT, SYS_ID_NULL_LOG, NULL, NULL );
  }
}

/******************************************************************************
 * Function:     CfgMgr_FileInit
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
BOOLEAN CfgMgr_FileInit(void)
{
  // Copy Default NV Cfg to Shadow RAM
  memcpy(&NVRAMShadow.Cfg, &DefaultNVCfg, sizeof(NVRAMShadow.Cfg));
  return CfgMgr_StoreConfig( &NVRAMShadow );
}


/******************************************************************************
 * Function:     CfgMgr_ResetConfig
 *
 * Description:  Reset Config Manager Configuration to default state
 *
 * Parameters:   [in] bResetAll
 *                    TRUE - Reset all nv files where ResetFlag is true.
 *                    FALSE- Reset NV_CFG_MGR only.
 *               [in] reason the default config has been loaded see RESET_REASON
 *
 * Returns:      SUCCESS or FAIL
 *
 *****************************************************************************/
BOOLEAN CfgMgr_ResetConfig( BOOLEAN bResetAll, RESET_REASON reason )
{
  BOOLEAN status = TRUE;

  if(bResetAll)
  {
    NV_ResetConfig();
  }
  else
  {
    // Erase EEPROM for CfgMgr-only before setting to default
    NV_Erase(NV_CFG_MGR);
    CfgMgr_FileInit();
  }
  // Create a log entry that the Default NVCfg is loaded and the reason.
  Flt_SetStatus(STA_FAULT, SYS_CFG_RESET_TO_DEFAULT, &reason, sizeof(reason));
  return status;
}



/******************************************************************************
 * Function:     CfgMgr_ConfigPtr
 *
 * Description:  Get a pointer to the shadow configuration data.  This should
 *               only be used to read, modify, then set configuration items.
 *               For getting the current configuration during runtime, use
 *               RuntimeCfgPtr().
 *
 * Parameters:   void
 *
 * Returns:      CFGMGR_NVRAM*
 *
 * Notes:
 *               Returning a reference to a local static can be dangerous, but
 *               in this case it was a way to avoid using global variable and
 *               for this instance, all callers use ptr to copy.
 *
 *****************************************************************************/
CFGMGR_NVRAM* CfgMgr_ConfigPtr(void)
{
  CFGMGR_NVRAM *CfgCpy;

  if(m_IsInBatchMode)
  {
    CfgCpy = &m_BatchModeShadow.Cfg;
  }
  else
  {
    CfgCpy = &NVRAMShadow.Cfg;
  }
  return CfgCpy;
}

/******************************************************************************
 * Function:     CfgMgr_RuntimeConfigPtr
 *
 * Description:  Get a pointer to the shadow configuration data.  This pointer
 *               returns the current and comitted configuration copy that
 *               is intended to be read during runtime.  To use the pointer
 *               to modify configuration data, see ConfigPtr();
 *
 *
 *
 * Parameters:
 *
 * Returns:
 *
 *****************************************************************************/
CFGMGR_NVRAM* CfgMgr_RuntimeConfigPtr(void)
{
  return &NVRAMShadow.Cfg;
}

/******************************************************************************
 * Function:     CfgMgr_GetSystemBinaryHdr
 *
 * Description:  Retrieves the Binary header data from each of the interfaces.
 *
 * Parameters:   INT8   *pDest       - Pointer to the destination buffer
 *               UINT16 nMaxByteSize - Total number of bytes available
 *
 * Returns:      UINT16 - Total Number of bytes in binary header
 *
 *****************************************************************************/
UINT16 CfgMgr_GetSystemBinaryHdr(INT8 *pDest, UINT16 nMaxByteSize )
{
   // Local Data
   UINT16 Remaining;
   UINT16 Total;
   INT8   *pBuffer;
   UINT32 Version;

   // Initialize Local Data
   Version   = SYS_HDR_VERSION;
   Remaining = nMaxByteSize;
   pBuffer   = pDest;
   Total     = sizeof(Version);

   // First Copy the version to the header
   memcpy ( pBuffer, &Version, Total );

   // Increment the buffer and decrement the amount of space left
   pBuffer   += Total;
   Remaining -= Total;

   // Get Sensor Hdr
   Total      = SensorGetSystemHdr ( pBuffer, Remaining );
   Remaining -= Total;
   pBuffer   += Total;
   // Get Trigger Hdr
   Total      = TriggerGetSystemHdr ( pBuffer, Remaining );
   Remaining -= Total;
   pBuffer   += Total;
   // Get Arinc429 Hdr
   Total      = Arinc429MgrGetSystemHdr ( pBuffer, Remaining );
   Remaining -= Total;
   pBuffer   += Total;
   // Get QAR Hdr
   Total      = QAR_GetSystemHdr ( pBuffer, Remaining );
   Remaining -= Total;
   pBuffer   += Total;
   // Get UART Hdr
   Total      = UartMgr_GetSystemHdr ( pBuffer, Remaining );
   Remaining -= Total;
   // Calculate and return the total number of bytes
   return (nMaxByteSize - Remaining);
}

/******************************************************************************
 * Function:     CfgMgr_GetETMBinaryHdr
 *
 * Description:  Retrieves the Binary header data from each of the interfaces.
 *
 * Parameters:   INT8   *pDest       - Pointer to the destination buffer
 *               UINT16 nMaxByteSize - Total number of bytes available
 *
 * Returns:      UINT16 - Total Number of bytes in binary header
 *
 *****************************************************************************/
UINT16 CfgMgr_GetETMBinaryHdr(INT8 *pDest, UINT16 nMaxByteSize )
{
   // Local Data
   UINT16 Remaining;
   UINT16 Total;
   INT8   *pBuffer;
   UINT32 Version;

   // Initialize Local Data
   Version   = ETM_HDR_VERSION;
   Remaining = nMaxByteSize;
   pBuffer   = pDest;
   Total     = sizeof(Version);

   // First Copy the version to the header
   memcpy ( pBuffer, &Version, Total );

   // Increment the buffer and decrement the amount of space left
   pBuffer   += Total;
   Remaining -= Total;

   // Get Sensor Hdr
   Total      = SensorGetETMHdr ( pBuffer, Remaining );
   Remaining -= Total;
   pBuffer   += Total;
   // Get Trigger Hdr
   Total      = TriggerGetSystemHdr ( pBuffer, Remaining );
   Remaining -= Total;
   pBuffer   += Total;
   // Get Event Header
   Total      = EventGetBinaryHdr ( pBuffer, Remaining );
   Remaining -= Total;
   pBuffer   += Total;
   // Get Event Table
   Total      = EventTableGetBinaryHdr ( pBuffer, Remaining );
   Remaining -= Total;
   pBuffer   += Total;
   // Get Engine Run
   Total      = EngRunGetBinaryHeader ( pBuffer, Remaining );
   Remaining -= Total;
   pBuffer   += Total;
   // Get Cycle
   Total      = CycleGetBinaryHeader ( pBuffer, Remaining );
   Remaining -= Total;
   pBuffer   += Total;
   // Get Trend
   Total      = TrendGetBinaryHdr ( pBuffer, Remaining );
   Remaining -= Total;
   pBuffer   += Total;
   // Get TimeHistory
   Total      = TH_GetBinaryHeader ( pBuffer, Remaining );
   Remaining -= Total;
   pBuffer   += Total;
   // Calculate and return the total number of bytes
   return (nMaxByteSize - Remaining);
}

/******************************************************************************
 * Function:     CfgMgr_RetrieveConfig
 *
 * Description:  Read a configuration item from shadow RAM
 *               store the structure in the CFGMGR_NVRAM structure passed.
 *
 *
 * Parameters:   CFGMGR_NVRAM_CS* - pInfo buffer to store NVRAM information
 *
 * Returns:      TRUE: Data read from EEPROM and checksum is valid
 *               FALSE: Data read failed or checksum failed
 *
 *****************************************************************************/
BOOLEAN CfgMgr_RetrieveConfig( CFGMGR_NVRAM_CS* pInfo )
{
 UINT16  ReadChecksum;
 UINT16  CalcChecksum;

#ifdef SIMULATE_CM
/*vcast_dont_instrument_start*/
  memcpy( pInfo, m_CmConfig.pDtuNvRam, sizeof(CM_NVRAM_DTU) );
  result = TRUE;
  return;
/*vcast_dont_instrument_end*/
#endif //SIMULATE_CM

  NV_Read( NV_CFG_MGR, 0, pInfo, sizeof(*pInfo) );

  ReadChecksum = pInfo->CS;
  //Get checksum
  CalcChecksum = CRC16(&pInfo->Cfg,sizeof(pInfo->Cfg));

  return ( CalcChecksum == ReadChecksum );
}



/******************************************************************************
 * Function:     CfgMgr_StoreConfig
 *
 * Description:  Store all appropriate information to NVRAM, checksum will be
 *               recalculated
 *
 * Parameters:   CFGMGR_NVRAM_CS* - pInfo information to be stored
 *
 * Returns:      SUCCESS or FAIL
 *
 *****************************************************************************/
BOOLEAN CfgMgr_StoreConfig( CFGMGR_NVRAM_CS* pInfo )
{
  pInfo->CS = CRC16(&pInfo->Cfg,sizeof(pInfo->Cfg));

#ifdef SIMULATE_CM
/*vcast_dont_instrument_start*/
  memcpy( NVRAMShadow, pInfo, sizeof(NVRAMShadow) );
  result = TRUE;
/*vcast_dont_instrument_end*/
#else
  NV_Write(NV_CFG_MGR,0, pInfo, sizeof(*pInfo) );
#endif

  return TRUE;
}



/******************************************************************************
 * Function:     CfgMgr_StartBatchCfg
 *
 * Description:  The "Batch Mode" is used for storing new configuration data
 *               without modifying the existing system configuration
 *               StartBatchCfg enters Batch Mode by:
 *               1)Initalizes a local Batch Mode copy of the configuration with
 *                 factory default configuration data.
 *               2)Routes all calls to modify a configuration
 *               item (_StoreCfgItem) to modfy the Batch Mode copy, not the
 *               EEPROM or runtime NVRAMShadow copy.
 *
 *
 * Parameters:   none
 *
 * Returns:
 *
 *****************************************************************************/
void CfgMgr_StartBatchCfg(void)
{
  //Fill the batch mode shadow with default data
  memcpy(&m_BatchModeShadow.Cfg,&DefaultNVCfg,sizeof(m_BatchModeShadow.Cfg));
  //Set flag to direct _StoreCfgItem calls to the BatchMode copy
  m_IsInBatchMode = TRUE;
}



/******************************************************************************
 * Function:     CfgMgr_CancelBatchCfg
 *
 * Description:  The "Batch Mode" is used for storing new configuration data
 *               without modifying the existing system configuration
 *               CancelBatchCfg exits Batch Mode by
 *               1) Discards any configuration changes to the Batch Mode
 *                  copy.
 *               2) Restores calls to modify the configuration items so they
 *                  will modify the NVRAMShadow and EEPROM copies of the data
 *                  again.
 *
 * Parameters:   none
 *
 * Returns:
 *
 *****************************************************************************/
void CfgMgr_CancelBatchCfg(void)
{
  m_IsInBatchMode = FALSE;
}



/******************************************************************************
 * Function:     CfgMgr_CommitBatchCfg
 *
 * Description:  The "Batch Mode" is used for storing a new configuration
 *               without modifying the existing system configuration
 *
 *               CommitBatchCfg saves the Batch Mode cfg copy to EEPROM and
 *               exits batch mode configuration.
 *
 *               Depending on the configuration size, the save process can take
 *               several seconds to complete.  This call is non-blocking,
 *               use CfgMgr_IsEEWritePending() to poll for completion.
 *
 * Parameters:   none
 *
 * Returns:
 *
 *****************************************************************************/
void CfgMgr_CommitBatchCfg(void)
{
  m_IsInBatchMode = FALSE;
  CfgMgr_StoreConfig(&m_BatchModeShadow);
}



/******************************************************************************
 * Function:     CfgMgr_IsEEWritePending
 *
 * Description:
 *
 * Parameters:   none
 *
 * Returns:      TRUE: Configuration data is pending write to EEPROM
 *               FALSE: There is no configuration data pending write to EEPROM
 *
 *****************************************************************************/
BOOLEAN CfgMgr_IsEEWritePending(void)
{
  return(SYS_NV_WRITE_PENDING == NV_WriteState(NV_CFG_MGR, 0, 0));
}


/******************************************************************************
 * Function:     CfgMgr_StoreConfigItem
 *
 * Description:  Store a single item to NVRAM, checksum will be recalculated
 *
 * Parameters:   void    *pOffset,
 *               void    *pSrc,
 *               UINT16  nSize
 *
 * Returns:      BOOLEAN - SUCCESS or FAIL
 *
 *****************************************************************************/
BOOLEAN CfgMgr_StoreConfigItem( void *pOffset, void *pSrc, UINT16 nSize )
{
    BOOLEAN status;
    UINT32 nOffset;

    UINT16 nChkSumSize;
    void *pDest;
    void *pChkSumSrc;

    //For batch mode, ignore requests to write Store cfg item.  The batch
    //mode copy is not stored to EEPROM until a call to CommitBatchCfg().
    if(m_IsInBatchMode)
    {
      status = TRUE;
    }
    //For standard mode, modify the item, recompute CRC, and write to EE
    else
    {
      status = FAIL;

      nOffset = ( (UINT32)pSrc - (UINT32)pOffset );

      //Write item to NVRAM
#ifdef SIMULATE_CM
  /*vcast_dont_instrument_start*/
      memcpy( ShadowConfig, pSrc, nSize );
      status = SUCCESS;
  /*vcast_dont_instrument_end*/
#else
      NV_Write(NV_CFG_MGR,nOffset,pSrc,nSize);
      status = TRUE;
#endif

      //Update shadow ram and checksum
      nOffset = (UINT32)&(NVRAMShadow) + ( (UINT32)pSrc - (UINT32)pOffset );
      pDest = (void *)nOffset;
      memcpy( pDest, pSrc, nSize );

      // calculate and update checksum on shadow NVRAM
      NVRAMShadow.CS = CRC16(&NVRAMShadow.Cfg,sizeof(NVRAMShadow.Cfg));

      nOffset = ((UINT32)(&NVRAMShadow.CS) - (UINT32)(&NVRAMShadow));

      pChkSumSrc = &(NVRAMShadow.CS);
      nChkSumSize = sizeof(NVRAMShadow.CS);

#ifdef SIMULATE_CM
/*vcast_dont_instrument_start*/
      memcpy( pDest, pChkSumSrc, nChkSumSize );
/*vcast_dont_instrument_end*/
#else
      NV_Write(NV_CFG_MGR,nOffset, pChkSumSrc, nChkSumSize);
#endif

    }
    return status;

}

/*****************************************************************************
 * Function:     CfgMgr_GenerateDebugLogs
 *
 * Description:  Log Write Test will write one copy of every log the Config
 *               Manager generates.  The purpose if to test the log format
 *               for the team writing the log file decoding tools.  This
 *               code will be ifdef'd out of the flag GENERATE_SYS_LOGS is not
 *               defined.
 *
 * Parameters:   none
 *
 * Returns:      none
 *
 * Notes:
 *
 ****************************************************************************/
#ifdef GENERATE_SYS_LOGS
/*vcast_dont_instrument_start*/
void CfgMgr_GenerateDebugLogs(void)
{
  LogWriteSystem(SYS_CFG_RESET_TO_DEFAULT,LOG_PRIORITY_LOW,NULL,0,NULL);
}
/*vcast_dont_instrument_end*/
#endif //GENERATE_SYS_LOGS

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/*************************************************************************
 *  MODIFICATIONS
 *    $History: CfgManager.c $
 *
 * *****************  Version 68  *****************
 * User: John Omalley Date: 12-09-11   Time: 2:26p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Fixed Trend Binary Header Call
 *
 * *****************  Version 67  *****************
 * User: John Omalley Date: 12-09-11   Time: 2:05p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Added Get Binary ETM Header
 * 
 * *****************  Version 66  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 65  *****************
 * User: Jeff Vahue   Date: 8/13/12    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 1145 - remove UINT16 cast
 *
 * *****************  Version 64  *****************
 * User: John Omalley Date: 12-07-17   Time: 11:28a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Added new Action Object
 *
 * *****************  Version 63  *****************
 * User: John Omalley Date: 12-07-13   Time: 3:48p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Added Action Configuration
 *
 * *****************  Version 62  *****************
 * User: Contractor V&v Date: 5/10/12    Time: 6:42p
 * Updated in $/software/control processor/code/system
 * Added engine run and cycle to Cfg Manager
 *
 * *****************  Version 61  *****************
 * User: John Omalley Date: 4/27/12    Time: 5:04p
 * Updated in $/software/control processor/code/system
 *
 * *****************  Version 59  *****************
 * User: Jim Mood     Date: 7/20/11    Time: 2:57p
 * Updated in $/software/control processor/code/system
 * SCR 575: GSM Enable when engine status is lost.  (Part of changes for
 *  * the Fast State Machine)
 *
 * *****************  Version 58  *****************
 * User: Peter Lee    Date: 4/11/11    Time: 10:13a
 * Updated in $/software/control processor/code/system
 * SCR #1029 EMU150 Download functionality
 *
 * *****************  Version 57  *****************
 * User: Jeff Vahue   Date: 9/14/10    Time: 8:21p
 * Updated in $/software/control processor/code/system
 * SCR# 848 - Code Coverage Mods
 *
 * *****************  Version 56  *****************
 * User: Jeff Vahue   Date: 9/13/10    Time: 7:00p
 * Updated in $/software/control processor/code/system
 * SCR# 848 - Coverage Mod
 *
 * *****************  Version 55  *****************
 * User: Jeff Vahue   Date: 9/13/10    Time: 4:03p
 * Updated in $/software/control processor/code/system
 * SCR# 848 - Code Coverage Mod
 *
 * *****************  Version 54  *****************
 * User: John Omalley Date: 8/05/10    Time: 11:40a
 * Updated in $/software/control processor/code/system
 * SCR 750 - Remove TX Method
 *
 * *****************  Version 53  *****************
 * User: John Omalley Date: 7/30/10    Time: 5:33p
 * Updated in $/software/control processor/code/system
 * SCR 699 - Cfg_MgrInitialization changed return to void
 *
 * *****************  Version 52  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:10a
 * Updated in $/software/control processor/code/system
 * SCR #698 - Fix code review findings
 *
 * *****************  Version 51  *****************
 * User: Contractor V&v Date: 7/27/10    Time: 6:20p
 * Updated in $/software/control processor/code/system
 * SCR #711 Error: Dpram Int Failure incorrectly forces STA_FAULT.
 *
 * *****************  Version 50  *****************
 * User: Contractor V&v Date: 7/22/10    Time: 6:49p
 * Updated in $/software/control processor/code/system
 * SCR #674 fast.reset=really needs to wait until EEPROMcommitted to
 * return
 *
 * *****************  Version 49  *****************
 * User: Contractor V&v Date: 7/21/10    Time: 7:17p
 * Updated in $/software/control processor/code/system
 * SCR #694 SRS-3747 cannot occur, should be deleted
 *
 * *****************  Version 48  *****************
 * User: John Omalley Date: 7/20/10    Time: 9:04a
 * Updated in $/software/control processor/code/system
 * SCR 294 - Add system binary header
 *
 * *****************  Version 47  *****************
 * User: Contractor3  Date: 6/25/10    Time: 9:46a
 * Updated in $/software/control processor/code/system
 * SCR #662 - Changes based on code review
 *
 * *****************  Version 46  *****************
 * User: Contractor V&v Date: 6/16/10    Time: 6:40p
 * Updated in $/software/control processor/code/system
 * SCR #623 Implement NV_IsEEPromWriteComplete
 *
 * *****************  Version 45  *****************
 * User: Jim Mood     Date: 6/11/10    Time: 3:31p
 * Updated in $/software/control processor/code/system
 * SCR 623 Batch configuration implementation updates
 *
 * *****************  Version 44  *****************
 * User: Jim Mood     Date: 6/11/10    Time: 10:16a
 * Updated in $/software/control processor/code/system
 * SCR 623
 *
 * *****************  Version 43  *****************
 * User: Contractor V&v Date: 6/08/10    Time: 5:53p
 * Updated in $/software/control processor/code/system
 * SCR #614 fast.reset=really should initialize files
 *
 * *****************  Version 42  *****************
 * User: Peter Lee    Date: 5/31/10    Time: 6:12p
 * Updated in $/software/control processor/code/system
 * SCR #618 F7X and UartMgr Requirements Implementation
 *
 * *****************  Version 41  *****************
 * User: Contractor V&v Date: 5/12/10    Time: 3:48p
 * Updated in $/software/control processor/code/system
 * SCR #543 Misc - reduce complexity
 *
 * *****************  Version 40  *****************
 * User: Contractor V&v Date: 5/07/10    Time: 4:35p
 * Updated in $/software/control processor/code/system
 * SCR #543 Misc - reduce complexity
 *
 * *****************  Version 39  *****************
 * User: Contractor V&v Date: 4/28/10    Time: 6:36p
 * Updated in $/software/control processor/code/system
 * SCR #571 SysCond should be fault when default config is loaded
 *
 * *****************  Version 38  *****************
 * User: Contractor V&v Date: 4/28/10    Time: 4:53p
 * Updated in $/software/control processor/code/system
 * SCR #571 SysCond should be fault when default config is loaded
 *
 * *****************  Version 37  *****************
 * User: Contractor V&v Date: 4/22/10    Time: 6:37p
 * Updated in $/software/control processor/code/system
 * SCR #187 Run in degraded mode
 *
 * *****************  Version 36  *****************
 * User: Contractor V&v Date: 4/12/10    Time: 7:21p
 * Updated in $/software/control processor/code/system
 * SCR #540 Log reason for cfg reset
 *
 * *****************  Version 35  *****************
 * User: Jeff Vahue   Date: 4/12/10    Time: 5:43p
 * Updated in $/software/control processor/code/system
 * SCR #541 - all XXX_CreateLogs functions are now dependent on
 * GENERATE_SYS_LOGS
 *
 * *****************  Version 34  *****************
 * User: Contractor V&v Date: 3/29/10    Time: 6:17p
 * Updated in $/software/control processor/code/system
 * SCR #140 Move Log erase status to own file
 *
 * *****************  Version 33  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:36p
 * Updated in $/software/control processor/code/system
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 *
 * *****************  Version 32  *****************
 * User: Contractor V&v Date: 3/19/10    Time: 4:31p
 * Updated in $/software/control processor/code/system
 * SCR #279 Making NV filenames uppercase
 *
 * *****************  Version 31  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/system
 * SCR# 483 - Function Names
 *
 * *****************  Version 30  *****************
 * User: Contractor V&v Date: 3/10/10    Time: 4:41p
 * Updated in $/software/control processor/code/system
 * SCR #464 Move Version Info into own file and support it.
 *
 * *****************  Version 29  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:57p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 28  *****************
 * User: Contractor V&v Date: 2/03/10    Time: 2:57p
 * Updated in $/software/control processor/code/system
 * SCR 279
 *
 * *****************  Version 27  *****************
 * User: Contractor V&v Date: 1/27/10    Time: 5:23p
 * Updated in $/software/control processor/code/system
 * SCR 372 Updating cfg EEPROM data
 *
 * *****************  Version 26  *****************
 * User: Jeff Vahue   Date: 12/18/09   Time: 1:35p
 * Updated in $/software/control processor/code/system
 * SCR# 378
 *
 * *****************  Version 25  *****************
 * User: Contractor V&v Date: 11/18/09   Time: 3:52p
 * Updated in $/software/control processor/code/system
 * Fixed Typos, No logic .
 *
 * *****************  Version 24  *****************
 * User: Jim Mood     Date: 11/09/09   Time: 3:17p
 * Updated in $/software/control processor/code/system
 * Added configuration items for MSFX ( micro-server file transfer )
 * system
 *
 * *****************  Version 23  *****************
 * User: Jim Mood     Date: 10/14/09   Time: 2:33p
 * Updated in $/software/control processor/code/system
 * SCR #101, sw version and build date stored in EE and checked on
 * startup.
 *
 * *****************  Version 22  *****************
 * User: John Omalley Date: 10/01/09   Time: 4:26p
 * Updated in $/software/control processor/code/system
 * Added fault configuration
 *
 * *****************  Version 21  *****************
 * User: Peter Lee    Date: 9/30/09    Time: 9:28a
 * Updated in $/software/control processor/code/system
 * SCR #277 Power Management Req Updates
 *
 * *****************  Version 20  *****************
 * User: Peter Lee    Date: 6/29/09    Time: 3:20p
 * Updated in $/software/control processor/code/system
 * SCR #204 Misc Code Review Updates
 *
 * *****************  Version 19  *****************
 * User: Jim Mood     Date: 4/22/09    Time: 5:32p
 * Updated in $/control processor/code/system
 *
 * *****************  Version 18  *****************
 * User: Jim Mood     Date: 1/22/09    Time: 1:19p
 * Updated in $/control processor/code/system
 * Added configuration structure for the AircraftConfigMgr module
 *
 * *****************  Version 17  *****************
 * User: Jim Mood     Date: 10/17/08   Time: 11:04a
 * Updated in $/control processor/code/system
 * Added PBITsystem fault log recording
 *
 * *****************  Version 16  *****************
 * User: Jim Mood     Date: 10/08/08   Time: 10:40a
 * Updated in $/control processor/code/system
 *
 * *****************  Version 15  *****************
 * User: Jim Mood     Date: 10/07/08   Time: 5:29p
 * Updated in $/control processor/code/system
 * SCR #87
 *
 * *****************  Version 14  *****************
 * User: Jim Mood     Date: 10/07/08   Time: 5:09p
 * Updated in $/control processor/code/system
 *
 * *****************  Version 13  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 2:39p
 * Updated in $/control processor/code/system
 * SCR #87 Function Prototype
 *
 *
 ***************************************************************************/

