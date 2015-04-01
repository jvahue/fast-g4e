#define UARTMGR_USERTABLES_BODY
/******************************************************************************
            Copyright (C) 2007-2015 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    ECCN:        9D991

    File:        UartMgrUserTables.c

    Description: Routines to support the user commands for UartMgr

    Note:

    VERSION
    $Revision: 23 $  $Date: 4/01/15 9:31a $

******************************************************************************/
#ifndef UART_MGR_BODY
#error UartMgrUserTables.c should only be included by UartMgr.c
#endif

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include <stdlib.h>
#include <string.h>

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "UartMgr.h"
#include "user.h"
#include "stddefs.h"
#include "CfgManager.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define STR_LIMIT_UART_MGR 0,32

#define UARTMGR_DEBUG_TEST 0

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static UARTMGR_STATUS uartMgrStatusTemp;
static UARTMGR_CFG    uartMgrCfgTemp;

static UARTMGR_DEBUG  uartMgrDebugTemp;

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static USER_HANDLER_RESULT UartMgrMsg_Status       ( USER_DATA_TYPE DataType,
                                                     USER_MSG_PARAM Param,
                                                     UINT32 Index,
                                                     const void *SetPtr,
                                                     void **GetPtr );

static USER_HANDLER_RESULT UartMgrMsg_Cfg         ( USER_DATA_TYPE DataType,
                                                    USER_MSG_PARAM Param,
                                                    UINT32 Index,
                                                    const void *SetPtr,
                                                    void **GetPtr );

#ifdef UARTMGR_DEBUG_TEST
static USER_HANDLER_RESULT UartMgrMsg_Debug       ( USER_DATA_TYPE DataType,
                                                    USER_MSG_PARAM Param,
                                                    UINT32 Index,
                                                    const void *SetPtr,
                                                    void **GetPtr );
#endif
static USER_HANDLER_RESULT UartMgrMsg_ShowConfig ( USER_DATA_TYPE DataType,
                                                   USER_MSG_PARAM Param,
                                                   UINT32 Index,
                                                   const void *SetPtr,
                                                   void **GetPtr );

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static USER_ENUM_TBL uartMgrProtocolStrs[] =
{
  {"NONE",        UARTMGR_PROTOCOL_NONE},
  {"F7X_N_PARAM", UARTMGR_PROTOCOL_F7X_N_PARAM},
  {"ID_PARAM",    UARTMGR_PROTOCOL_ID_PARAM},
  {"EMU150",      UARTMGR_PROTOCOL_EMU150},
  {"GBS",         UARTMGR_PROTOCOL_GBS},
  {NULL,          0}
};

static USER_ENUM_TBL uartMgrSysStatusStrs[] =
{
  {"OK",              UARTMGR_STATUS_OK},
  {"FAULTED_PBIT",    UARTMGR_STATUS_FAULTED_PBIT},
  {"FAULTED_CBIT",    UARTMGR_STATUS_FAULTED_CBIT},
  {"DISABLED_OK",     UARTMGR_STATUS_DISABLED_OK},
  {"DISABLED_FAULTED",UARTMGR_STATUS_DISABLED_FAULTED},
  {NULL,0}
};

static USER_ENUM_TBL uartMgrBPSStrs[] =
{
  {"9600",  UART_CFG_BPS_9600},
  {"19200", UART_CFG_BPS_19200},
  {"38400", UART_CFG_BPS_38400},
  {"57600", UART_CFG_BPS_57600},
  {"115200",UART_CFG_BPS_115200},
  {NULL,0}
};

static USER_ENUM_TBL uartMgrsDBStrs[] =
{
   {"7", UART_CFG_DB_7},
   {"8", UART_CFG_DB_8},
   {NULL,0}
};

static USER_ENUM_TBL uartMgrSBStrs[] =
{
   {"1", UART_CFG_SB_1},
   {"2", UART_CFG_SB_2},
   {NULL,0}
};

static USER_ENUM_TBL uartMgrsParityStrs[] =
{
   {"EVEN", UART_CFG_PARITY_EVEN},
   {"ODD",  UART_CFG_PARITY_ODD},
   {"NONE", UART_CFG_PARITY_NONE},
   {NULL,0}
};

static USER_ENUM_TBL uartMgrsDuplexStrs[] =
{
   {"FULL", UART_CFG_DUPLEX_FULL},
   {"HALF", UART_CFG_DUPLEX_HALF},
   {NULL,0}
};


//--------------------------------------------
// User Table Defintions
//--------------------------------------------
static USER_MSG_TBL uartMgrStatusTbl[] =
{
  {"CHAN_ACTIVE", NO_NEXT_TABLE, UartMgrMsg_Status,
          USER_TYPE_BOOLEAN, USER_RO,(void *) &uartMgrStatusTemp.bChanActive,
          1, UART_NUM_OF_UARTS-1, NO_LIMIT,NULL},

  {"TIME_CHAN_ACTIVE", NO_NEXT_TABLE, UartMgrMsg_Status,
          USER_TYPE_UINT32, USER_RO,(void *) &uartMgrStatusTemp.timeChanActive,
          1, UART_NUM_OF_UARTS-1, NO_LIMIT,NULL},

  {"SYNC_LOSS_CNT", NO_NEXT_TABLE, UartMgrMsg_Status,
          USER_TYPE_UINT32,USER_RO,(void *) &uartMgrStatusTemp.dataLossCnt,
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},

  {"CHANNEL_TIMEOUT",NO_NEXT_TABLE,UartMgrMsg_Status,
          USER_TYPE_BOOLEAN,USER_RO,(void *) &uartMgrStatusTemp.bChannelTimeOut,
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},

  {"CHANNEL_START_TIMEOUT",NO_NEXT_TABLE,UartMgrMsg_Status,
          USER_TYPE_BOOLEAN,USER_RO,(void *) &uartMgrStatusTemp.bChannelStartupTimeOut,
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},

  {"PORT_BYTE_CNT",NO_NEXT_TABLE,UartMgrMsg_Status,
          USER_TYPE_UINT32,USER_RO,(void *) &uartMgrStatusTemp.portByteCnt,
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},

  {"PORT_FRAMING_ERR_CNT",NO_NEXT_TABLE,UartMgrMsg_Status,
          USER_TYPE_UINT32,USER_RO,(void *) &uartMgrStatusTemp.portFramingErrCnt,
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},

  {"PORT_PARITY_ERR_CNT",NO_NEXT_TABLE,UartMgrMsg_Status,
          USER_TYPE_UINT32,USER_RO,(void *) &uartMgrStatusTemp.portParityErrCnt,
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},

  {"PORT_RX_OVERFLOW_CNT",NO_NEXT_TABLE,UartMgrMsg_Status,
          USER_TYPE_UINT32,USER_RO,(void *) &uartMgrStatusTemp.portRxOverFlowErrCnt,
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},

  {"RECORDING",NO_NEXT_TABLE,UartMgrMsg_Status,
          USER_TYPE_BOOLEAN,USER_RO,(void *) &uartMgrStatusTemp.bRecordingActive,
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},

  {"PROTOCOL",NO_NEXT_TABLE,UartMgrMsg_Status,
          USER_TYPE_ENUM,USER_RO,(void *) &uartMgrStatusTemp.protocol,
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,uartMgrProtocolStrs},

  {"STATUS",NO_NEXT_TABLE,UartMgrMsg_Status,
          USER_TYPE_ENUM,USER_RO,(void *) &uartMgrStatusTemp.status,
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,uartMgrSysStatusStrs},

  {NULL,NULL,NULL,NO_HANDLER_DATA}
};


static USER_MSG_TBL uartMgrPortCfgTbl[] =
{
  {"BPS",NO_NEXT_TABLE,UartMgrMsg_Cfg,
          USER_TYPE_ENUM,USER_RW,(void *) &uartMgrCfgTemp.port.nBPS,
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,uartMgrBPSStrs},

  {"DATABITS",NO_NEXT_TABLE,UartMgrMsg_Cfg,
          USER_TYPE_ENUM,USER_RW,(void *) &uartMgrCfgTemp.port.dataBits,
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,uartMgrsDBStrs},

  {"STOPBITS",NO_NEXT_TABLE,UartMgrMsg_Cfg,
          USER_TYPE_ENUM,USER_RW,(void *) &uartMgrCfgTemp.port.stopBits,
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,uartMgrSBStrs},

  {"PARITY",NO_NEXT_TABLE,UartMgrMsg_Cfg,
          USER_TYPE_ENUM,USER_RW,(void *) &uartMgrCfgTemp.port.parity,
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,uartMgrsParityStrs},

  {"DUPLEX",NO_NEXT_TABLE,UartMgrMsg_Cfg,
          USER_TYPE_ENUM,USER_RW,(void *) &uartMgrCfgTemp.port.duplex,
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,uartMgrsDuplexStrs},

  {NULL,NULL,NULL,NO_HANDLER_DATA}
};



static USER_MSG_TBL uartMgrCfgTbl[] =
{
  {"NAME",NO_NEXT_TABLE,UartMgrMsg_Cfg,
          USER_TYPE_STR,USER_RW,(void *)uartMgrCfgTemp.sName,
          1,UART_NUM_OF_UARTS-1,STR_LIMIT_UART_MGR,NULL},

  {"PROTOCOL",NO_NEXT_TABLE,UartMgrMsg_Cfg,
          USER_TYPE_ENUM,USER_RW,(void *) &uartMgrCfgTemp.protocol,
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,uartMgrProtocolStrs},

  {"CHANNELSTARTUP_S",NO_NEXT_TABLE,UartMgrMsg_Cfg,
          USER_TYPE_UINT32,USER_RW,(void *) &uartMgrCfgTemp.channelStartup_s,
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},

  {"CHANNELTIMEOUT_S",NO_NEXT_TABLE,UartMgrMsg_Cfg,
          USER_TYPE_UINT32,USER_RW,(void *) &uartMgrCfgTemp.channelTimeOut_s,
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},

  {"CHANNELSYSCOND",NO_NEXT_TABLE,UartMgrMsg_Cfg,
          USER_TYPE_ENUM,USER_RW,(void *) &uartMgrCfgTemp.channelSysCond,
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,Flt_UserEnumStatus},

  {"PBITSysCond",NO_NEXT_TABLE,UartMgrMsg_Cfg,
          USER_TYPE_ENUM,USER_RW,(void *) &uartMgrCfgTemp.sysCondPBIT,
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,Flt_UserEnumStatus},

  {"ENABLE",NO_NEXT_TABLE,UartMgrMsg_Cfg,
          USER_TYPE_BOOLEAN,USER_RW,(void *) &uartMgrCfgTemp.bEnabled,
          1,UART_NUM_OF_UARTS-1,STR_LIMIT_UART_MGR,NULL},

  {"PORT", uartMgrPortCfgTbl, NULL, NO_HANDLER_DATA },

  {NULL,NULL,NULL,NO_HANDLER_DATA}
};


// Testing Only
#ifdef UARTMGR_DEBUG_TEST
static USER_MSG_TBL uartMgrDebugTbl[] =
{
  {"ENABLE",NO_NEXT_TABLE,UartMgrMsg_Debug,
          USER_TYPE_BOOLEAN,(USER_RW|USER_GSE),(void *) &uartMgrDebugTemp.bDebug,
          -1,-1,NO_LIMIT,NULL},

  {"CHANNEL",NO_NEXT_TABLE,UartMgrMsg_Debug,
          USER_TYPE_UINT16,(USER_RW|USER_GSE),(void *) &uartMgrDebugTemp.ch,
          -1,-1,1,3,NULL},

  {"BYTES",NO_NEXT_TABLE,UartMgrMsg_Debug,
          USER_TYPE_UINT16,(USER_RW|USER_GSE),(void *) &uartMgrDebugTemp.num_bytes,
          -1,-1,0,1024,NULL},

  {NULL,NULL,NULL,NO_HANDLER_DATA}
};
#endif
// Testing Only


static USER_MSG_TBL uartMgrRoot[] =
{
  {"STATUS", uartMgrStatusTbl, NULL, NO_HANDLER_DATA},
  {"CFG"   , uartMgrCfgTbl,    NULL, NO_HANDLER_DATA},
#ifdef UARTMGR_DEBUG_TEST
  {"DEBUG",  uartMgrDebugTbl,  NULL, NO_HANDLER_DATA},
#endif
/*
#ifdef GENERATE_SYS_LOGS
  {"CREATELOGS",NO_NEXT_TABLE,Arinc429Msg_CreateLogs,   USER_TYPE_ACTION, USER_RO,               NULL, -1,-1, NULL},
#endif
*/
  {DISPLAY_CFG, NO_NEXT_TABLE, UartMgrMsg_ShowConfig,  USER_TYPE_ACTION, (USER_RO|USER_NO_LOG|USER_GSE), NULL, -1,-1, NO_LIMIT, NULL},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};


static USER_MSG_TBL uartMgrRootTblPtr = {"UART",uartMgrRoot,NULL,NO_HANDLER_DATA};

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/


/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:    UartMgrMsg_Status
 *
 * Description: Called by the User.c module from the reference to this function
 *              in the user message tables above.
 *              Retrieves the latest UART Mgr Status Data
 *
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                               or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific sensor to change.  Range is validated
 *                               by the user manager
 *               [in] SetPtr:    For write commands, a pointer to the data to
 *                               write to the configuration.
 *               [out] GetPtr:   For read commands, UserCfg function will set
 *                               this to the location of the data requested.
 *
 * Returns:     USER_RESULT_OK:    Processed successfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
static
USER_HANDLER_RESULT UartMgrMsg_Status(USER_DATA_TYPE DataType,
                                      USER_MSG_PARAM Param,
                                      UINT32 Index,
                                      const void *SetPtr,
                                      void **GetPtr)
{
  USER_HANDLER_RESULT result ;

  result = USER_RESULT_OK;

  uartMgrStatusTemp = *UartMgr_GetStatus((UINT8)Index);

  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);

  return result;
}



/******************************************************************************
 * Function:    UartMgrMsg_Cfg
 *
 * Description: Called by the User.c module from the reference to this function
 *              in the user message tables above.
 *              Retrieves the latest UART Mgr Status Data
 *
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                               or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific sensor to change.  Range is validated
 *                               by the user manager
 *               [in] SetPtr:    For write commands, a pointer to the data to
 *                               write to the configuration.
 *               [out] GetPtr:   For read commands, UserCfg function will set
 *                               this to the location of the data requested.
 *
 * Returns:     USER_RESULT_OK:    Processed successfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
static
USER_HANDLER_RESULT UartMgrMsg_Cfg(USER_DATA_TYPE DataType,
                                   USER_MSG_PARAM Param,
                                   UINT32 Index,
                                   const void *SetPtr,
                                   void **GetPtr)
{
  USER_HANDLER_RESULT result;


  result = USER_RESULT_OK;

  //uartMgrCfgTemp = *UartMgr_GetCfg((UINT8)Index);
  memcpy(&uartMgrCfgTemp,
         &CfgMgr_ConfigPtr()->UartMgrConfig[Index],
         sizeof(uartMgrCfgTemp));

  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
  // Copy the updated temp  back to the shadow ram store.
    memcpy(&CfgMgr_ConfigPtr()->UartMgrConfig[Index], &uartMgrCfgTemp,
           sizeof(UARTMGR_CFG));

    //Store the modified temporary structure in the EEPROM.
    CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
                           &CfgMgr_ConfigPtr()->UartMgrConfig[Index],
      sizeof(UARTMGR_CFG));
  }
  return result;

}


// Testing Only
// Specific to "UartMgrCfgTemp.bRecordingActive" only !!!
#ifdef UARTMGR_DEBUG_TEST
/*vcast_dont_instrument_start*/
/******************************************************************************
 * Function:    UartMgrMsg_Debug
 *
 * Description: Called by the User.c module from the reference to this function
 *              in the user message tables above.
 *              Retrieves the latest UART Mgr Status Data
 *
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                               or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific sensor to change.  Range is validated
 *                               by the user manager
 *               [in] SetPtr:    For write commands, a pointer to the data to
 *                               write to the configuration.
 *               [out] GetPtr:   For read commands, UserCfg function will set
 *                               this to the location of the data requested.
 *
 * Returns:     USER_RESULT_OK:    Processed successfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
 *****************************************************************************/
static
USER_HANDLER_RESULT UartMgrMsg_Debug(USER_DATA_TYPE DataType,
                                     USER_MSG_PARAM Param,
                                     UINT32 Index,
                                     const void *SetPtr,
                                     void **GetPtr)
{
  USER_HANDLER_RESULT result;

  result = USER_RESULT_OK;


  uartMgrDebugTemp = *UartMgr_GetDebug();

  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);

  *UartMgr_GetDebug() = uartMgrDebugTemp;

  return result;

}
/*vcast_dont_instrument_end*/
#endif
// Testing Only

/******************************************************************************
* Function:     UartMgrMsg_ShowConfig
*
* Description:  Handles User Manager requests to retrieve the configuration
*               settings.
*
* Parameters:   [in] DataType:  C type of the data to be read or changed, used
*                               for casting the data pointers
*               [in/out] Param: Pointer to the configuration item to be read
*                               or changed
*               [in] Index:     Index parameter is used to reference the
*                               specific sensor to change.  Range is validated
*                               by the user manager
*               [in] SetPtr:    For write commands, a pointer to the data to
*                               write to the configuration.
*               [out] GetPtr:   For read commands, UserCfg function will set
*                               this to the location of the data requested.
*
*
* Returns:     USER_RESULT_OK:    Processed successfully
*              USER_RESULT_ERROR: Error processing command.
*
* Notes:
*****************************************************************************/
static
USER_HANDLER_RESULT UartMgrMsg_ShowConfig(USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr)
{
  USER_HANDLER_RESULT result;
  CHAR label[USER_MAX_MSG_STR_LEN * 3];

  //Top-level name is a single indented space
  CHAR branchName[USER_MAX_MSG_STR_LEN] = " ";

  USER_MSG_TBL*  pCfgTable;
  INT16 idx;

  result = USER_RESULT_OK;

  // Loop for each sensor.
  for (idx = 0; idx < UART_NUM_OF_UARTS && result == USER_RESULT_OK; ++idx)
  {
    // Display sensor id info above each set of data.
    snprintf(label, sizeof(label), "\r\n\r\nUART[%d].CFG", idx);
    result = USER_RESULT_ERROR;
    if (User_OutputMsgString( label, FALSE ) )
    {
      pCfgTable = uartMgrCfgTbl;  // Re-set the pointer to beginning of CFG table

      // User_DisplayConfigTree will invoke itself recursively to display all fields.
      result = User_DisplayConfigTree(branchName, pCfgTable, idx, 0, NULL);
    }
  }
  return result;
}

/*****************************************************************************
 *  MODIFICATIONS
 *    $History: UartMgrUserTables.c $
 * 
 * *****************  Version 23  *****************
 * User: John Omalley Date: 4/01/15    Time: 9:31a
 * Updated in $/software/control processor/code/system
 * SCR 1255 Code Review Updates
 * 
 * *****************  Version 22  *****************
 * User: Peter Lee    Date: 15-01-11   Time: 10:21p
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol 
 * 
 * *****************  Version 21  *****************
 * User: Peter Lee    Date: 14-12-05   Time: 4:39p
 * Updated in $/software/control processor/code/system
 * SCR #1263 ID Param Code Review Mod Fix. 
 *
 * *****************  Version 20  *****************
 * User: Peter Lee    Date: 14-10-08   Time: 6:56p
 * Updated in $/software/control processor/code/system
 * SCR #1263 ID Param Protocol Implementation
 *
 * *****************  Version 19  *****************
 * User: Contractor V&v Date: 9/03/14    Time: 5:27p
 * Updated in $/software/control processor/code/system
 * SCR #1234 - Configuration update of certain modules overwrites default
 * settings/CR updates
 *
 * *****************  Version 18  *****************
 * User: Contractor V&v Date: 8/14/14    Time: 4:14p
 * Updated in $/software/control processor/code/system
 * SCR #1234 - Configuration update of certain modules overwrites
 *
 * *****************  Version 17  *****************
 * User: John Omalley Date: 1/27/14    Time: 1:48p
 * Updated in $/software/control processor/code/system
 * SCR 1244 - UartMgr_Download Record unitialized variable, plus code
 * review updates.
 *
 * *****************  Version 16  *****************
 * User: John Omalley Date: 7/23/13    Time: 4:10p
 * Updated in $/software/control processor/code/system
 * SCR 1244 - UartMgr_Download Record unitialized variable, plus code
 * review updates.
 *
 * *****************  Version 15  *****************
 * User: John Omalley Date: 12-11-13   Time: 5:46p
 * Updated in $/software/control processor/code/system
 * SCR 1197 - Code Review Updates
 *
 * *****************  Version 13  *****************
 * User: John Omalley Date: 12-11-12   Time: 3:04p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review Updates
 *
 * *****************  Version 12  *****************
 * User: John Omalley Date: 12-11-12   Time: 2:58p
 * Updated in $/software/control processor/code/system
 * SCR 1107, 1191 - Code Review Updates
 *
 * *****************  Version 11  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 10  *****************
 * User: Peter Lee    Date: 4/11/11    Time: 10:15a
 * Updated in $/software/control processor/code/system
 * SCR #1029 EMU150 Download
 *
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 9/28/10    Time: 2:04p
 * Updated in $/software/control processor/code/system
 * SCR #892 Code Review Updates
 *
 * *****************  Version 8  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:01a
 * Updated in $/software/control processor/code/system
 * SCR #685 - Add USER_GSE flag
 *
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 6/28/10    Time: 5:17p
 * Updated in $/software/control processor/code/system
 * SCR #635 Force Uart and F7X min array to '1' from '0' since [0]
 * restricted to GSE port.
 *
 * *****************  Version 6  *****************
 * User: Peter Lee    Date: 6/25/10    Time: 6:16p
 * Updated in $/software/control processor/code/system
 * SCR #635 Add UartMgr Debug display raw input data.
 *
 * *****************  Version 5  *****************
 * User: Contractor V&v Date: 6/16/10    Time: 6:11p
 * Updated in $/software/control processor/code/system
 * SCR #643 Implement showcfg for F7X/Uart
 *
 * *****************  Version 4  *****************
 * User: Jeff Vahue   Date: 6/15/10    Time: 1:21p
 * Updated in $/software/control processor/code/system
 * Typo fixes and add vcast instrumentation off commands
 *
 * *****************  Version 3  *****************
 * User: Peter Lee    Date: 6/14/10    Time: 6:32p
 * Updated in $/software/control processor/code/system
 * SCR #635.  Misc updates.  Duplex cfg support.
 *
 * *****************  Version 2  *****************
 * User: Peter Lee    Date: 6/10/10    Time: 2:01p
 * Updated in $/software/control processor/code/system
 * SCR #635 Comment out debug commands
 *
 * *****************  Version 1  *****************
 * User: Peter Lee    Date: 5/31/10    Time: 5:55p
 * Created in $/software/control processor/code/system
 * Initial Check In.  Implementation of the F7X and UartMgr Requirements.
 *
 *
 *
 *****************************************************************************/
