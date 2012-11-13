#define UARTMGR_USERTABLES_BODY
/******************************************************************************
            Copyright (C) 2007-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:        UartMgrUserTables.c
    
    Description: Routines to support the user commands for UartMgr

    Note:

    VERSION
    $Revision: 15 $  $Date: 12-11-13 5:46p $
    
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
static UARTMGR_STATUS UartMgrStatusTemp; 
static UARTMGR_CFG UartMgrCfgTemp; 

static UARTMGR_DEBUG UartMgrDebugTemp; 

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
USER_HANDLER_RESULT UartMgrMsg_Status(USER_DATA_TYPE DataType,
                                      USER_MSG_PARAM Param,
                                      UINT32 Index,
                                      const void *SetPtr,
                                      void **GetPtr);

USER_HANDLER_RESULT UartMgrMsg_Cfg(USER_DATA_TYPE DataType,
                                   USER_MSG_PARAM Param,
                                   UINT32 Index,
                                   const void *SetPtr,
                                   void **GetPtr);

#ifdef UARTMGR_DEBUG_TEST                                   
USER_HANDLER_RESULT UartMgrMsg_Debug(USER_DATA_TYPE DataType,
                                     USER_MSG_PARAM Param,
                                     UINT32 Index,
                                     const void *SetPtr,
                                     void **GetPtr);
#endif
USER_HANDLER_RESULT UartMgrMsg_ShowConfig(USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr);

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
USER_ENUM_TBL UartMgrProtocolStrs[] =
{
  {"NONE",        UARTMGR_PROTOCOL_NONE},
  {"F7X_N_PARAM", UARTMGR_PROTOCOL_F7X_N_PARAM},
  {"EMU150",      UARTMGR_PROTOCOL_EMU150}, 
  {NULL,          0}
};

USER_ENUM_TBL UartMgrSysStatusStrs[] = 
{
  {"OK",              UARTMGR_STATUS_OK},
  {"FAULTED_PBIT",    UARTMGR_STATUS_FAULTED_PBIT},
  {"FAULTED_CBIT",    UARTMGR_STATUS_FAULTED_CBIT},
  {"DISABLED_OK",     UARTMGR_STATUS_DISABLED_OK},
  {"DISABLED_FAULTED",UARTMGR_STATUS_DISABLED_FAULTED}, 
  {NULL,0}
};

USER_ENUM_TBL UartMgrBPSStrs[] = 
{  
  {"9600",  UART_CFG_BPS_9600},
  {"19200", UART_CFG_BPS_19200},
  {"38400", UART_CFG_BPS_38400},
  {"57600", UART_CFG_BPS_57600},
  {"115200",UART_CFG_BPS_115200},                              
  {NULL,0} 
};

USER_ENUM_TBL UartMgrsDBStrs[] =
{
   {"7", UART_CFG_DB_7},
   {"8", UART_CFG_DB_8},
   {NULL,0}
};

USER_ENUM_TBL UartMgrSBStrs[] = 
{
   {"1", UART_CFG_SB_1},
   {"2", UART_CFG_SB_2},
   {NULL,0}
};

USER_ENUM_TBL UartMgrsParityStrs[] = 
{
   {"EVEN", UART_CFG_PARITY_EVEN},
   {"ODD",  UART_CFG_PARITY_ODD},
   {"NONE", UART_CFG_PARITY_NONE},                                 
   {NULL,0} 
};

USER_ENUM_TBL UartMgrsDuplexStrs[] = 
{
   {"FULL", UART_CFG_DUPLEX_FULL},
   {"HALF", UART_CFG_DUPLEX_HALF},
   {NULL,0}
};


//--------------------------------------------
// User Table Defintions              
//--------------------------------------------
USER_MSG_TBL UartMgrStatusTbl[] = 
{
  {"CHAN_ACTIVE",NO_NEXT_TABLE,UartMgrMsg_Status, 
          USER_TYPE_BOOLEAN,USER_RO,(void *) &UartMgrStatusTemp.bChanActive, 
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},
          
  {"TIME_CHAN_ACTIVE",NO_NEXT_TABLE,UartMgrMsg_Status, 
          USER_TYPE_UINT32,USER_RO,(void *) &UartMgrStatusTemp.timeChanActive, 
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},
          
  {"SYNC_LOSS_CNT",NO_NEXT_TABLE,UartMgrMsg_Status, 
          USER_TYPE_UINT32,USER_RO,(void *) &UartMgrStatusTemp.dataLossCnt, 
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},

  {"CHANNEL_TIMEOUT",NO_NEXT_TABLE,UartMgrMsg_Status, 
          USER_TYPE_BOOLEAN,USER_RO,(void *) &UartMgrStatusTemp.bChannelTimeOut, 
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},
          
  {"CHANNEL_START_TIMEOUT",NO_NEXT_TABLE,UartMgrMsg_Status, 
          USER_TYPE_BOOLEAN,USER_RO,(void *) &UartMgrStatusTemp.bChannelStartupTimeOut, 
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},
         
  {"PORT_BYTE_CNT",NO_NEXT_TABLE,UartMgrMsg_Status, 
          USER_TYPE_UINT32,USER_RO,(void *) &UartMgrStatusTemp.portByteCnt, 
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},

  {"PORT_FRAMING_ERR_CNT",NO_NEXT_TABLE,UartMgrMsg_Status, 
          USER_TYPE_UINT32,USER_RO,(void *) &UartMgrStatusTemp.portFramingErrCnt, 
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},

  {"PORT_PARITY_ERR_CNT",NO_NEXT_TABLE,UartMgrMsg_Status, 
          USER_TYPE_UINT32,USER_RO,(void *) &UartMgrStatusTemp.portParityErrCnt, 
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},

  {"PORT_RX_OVERFLOW_CNT",NO_NEXT_TABLE,UartMgrMsg_Status, 
          USER_TYPE_UINT32,USER_RO,(void *) &UartMgrStatusTemp.portRxOverFlowErrCnt, 
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},
          
  {"RECORDING",NO_NEXT_TABLE,UartMgrMsg_Status, 
          USER_TYPE_BOOLEAN,USER_RO,(void *) &UartMgrStatusTemp.bRecordingActive, 
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},

  {"PROTOCOL",NO_NEXT_TABLE,UartMgrMsg_Status, 
          USER_TYPE_ENUM,USER_RO,(void *) &UartMgrStatusTemp.protocol,
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,UartMgrProtocolStrs},

  {"STATUS",NO_NEXT_TABLE,UartMgrMsg_Status, 
          USER_TYPE_ENUM,USER_RO,(void *) &UartMgrStatusTemp.status,
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,UartMgrSysStatusStrs},
          
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};


USER_MSG_TBL UartMgrPortCfgTbl[] = 
{
  {"BPS",NO_NEXT_TABLE,UartMgrMsg_Cfg, 
          USER_TYPE_ENUM,USER_RW,(void *) &UartMgrCfgTemp.port.nBPS, 
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,UartMgrBPSStrs},
          
  {"DATABITS",NO_NEXT_TABLE,UartMgrMsg_Cfg, 
          USER_TYPE_ENUM,USER_RW,(void *) &UartMgrCfgTemp.port.dataBits, 
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,UartMgrsDBStrs},
          
  {"STOPBITS",NO_NEXT_TABLE,UartMgrMsg_Cfg, 
          USER_TYPE_ENUM,USER_RW,(void *) &UartMgrCfgTemp.port.stopBits, 
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,UartMgrSBStrs},

  {"PARITY",NO_NEXT_TABLE,UartMgrMsg_Cfg, 
          USER_TYPE_ENUM,USER_RW,(void *) &UartMgrCfgTemp.port.parity, 
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,UartMgrsParityStrs},
          
  {"DUPLEX",NO_NEXT_TABLE,UartMgrMsg_Cfg, 
          USER_TYPE_ENUM,USER_RW,(void *) &UartMgrCfgTemp.port.duplex, 
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,UartMgrsDuplexStrs},

  {NULL,NULL,NULL,NO_HANDLER_DATA}
};



USER_MSG_TBL UartMgrCfgTbl[] = 
{
  {"NAME",NO_NEXT_TABLE,UartMgrMsg_Cfg, 
          USER_TYPE_STR,USER_RW,(void *) &UartMgrCfgTemp.sName, 
          1,UART_NUM_OF_UARTS-1,STR_LIMIT_UART_MGR,NULL},
          
  {"PROTOCOL",NO_NEXT_TABLE,UartMgrMsg_Cfg, 
          USER_TYPE_ENUM,USER_RW,(void *) &UartMgrCfgTemp.protocol, 
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,UartMgrProtocolStrs},

  {"CHANNELSTARTUP_S",NO_NEXT_TABLE,UartMgrMsg_Cfg, 
          USER_TYPE_UINT32,USER_RW,(void *) &UartMgrCfgTemp.channelStartup_s, 
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},

  {"CHANNELTIMEOUT_S",NO_NEXT_TABLE,UartMgrMsg_Cfg, 
          USER_TYPE_UINT32,USER_RW,(void *) &UartMgrCfgTemp.channelTimeOut_s, 
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,NULL},

  {"CHANNELSYSCOND",NO_NEXT_TABLE,UartMgrMsg_Cfg, 
          USER_TYPE_ENUM,USER_RW,(void *) &UartMgrCfgTemp.channelSysCond, 
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,Flt_UserEnumStatus},

  {"PBITSysCond",NO_NEXT_TABLE,UartMgrMsg_Cfg, 
          USER_TYPE_ENUM,USER_RW,(void *) &UartMgrCfgTemp.sysCondPBIT, 
          1,UART_NUM_OF_UARTS-1,NO_LIMIT,Flt_UserEnumStatus},
          
  {"ENABLE",NO_NEXT_TABLE,UartMgrMsg_Cfg, 
          USER_TYPE_BOOLEAN,USER_RW,(void *) &UartMgrCfgTemp.bEnabled, 
          1,UART_NUM_OF_UARTS-1,STR_LIMIT_UART_MGR,NULL},
          
  {"PORT", UartMgrPortCfgTbl, NULL, NO_HANDLER_DATA },

  {NULL,NULL,NULL,NO_HANDLER_DATA}
};


// Testing Only 
#ifdef UARTMGR_DEBUG_TEST
USER_MSG_TBL UartMgrDebugTbl[] = 
{
  {"ENABLE",NO_NEXT_TABLE,UartMgrMsg_Debug, 
          USER_TYPE_BOOLEAN,(USER_RW|USER_GSE),(void *) &UartMgrDebugTemp.bDebug, 
          -1,-1,NO_LIMIT,NULL},

  {"CHANNEL",NO_NEXT_TABLE,UartMgrMsg_Debug, 
          USER_TYPE_UINT16,(USER_RW|USER_GSE),(void *) &UartMgrDebugTemp.ch, 
          -1,-1,1,3,NULL},
          
  {"BYTES",NO_NEXT_TABLE,UartMgrMsg_Debug, 
          USER_TYPE_UINT16,(USER_RW|USER_GSE),(void *) &UartMgrDebugTemp.num_bytes, 
          -1,-1,0,1024,NULL},
          
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};
#endif
// Testing Only 


USER_MSG_TBL UartMgrRoot[] = 
{
  {"STATUS",UartMgrStatusTbl,NULL,NO_HANDLER_DATA},
  {"CFG",UartMgrCfgTbl,NULL,NO_HANDLER_DATA},
#ifdef UARTMGR_DEBUG_TEST
  {"DEBUG",UartMgrDebugTbl,NULL,NO_HANDLER_DATA},
#endif
/*  
#ifdef GENERATE_SYS_LOGS  
  {"CREATELOGS",NO_NEXT_TABLE,Arinc429Msg_CreateLogs,   USER_TYPE_ACTION, USER_RO,               NULL, -1,-1, NULL},
#endif
*/  
  {DISPLAY_CFG, NO_NEXT_TABLE, UartMgrMsg_ShowConfig,  USER_TYPE_ACTION, (USER_RO|USER_NO_LOG|USER_GSE), NULL, -1,-1, NO_LIMIT, NULL},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};


USER_MSG_TBL UartMgrRootTblPtr = {"UART",UartMgrRoot,NULL,NO_HANDLER_DATA};

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
USER_HANDLER_RESULT UartMgrMsg_Status(USER_DATA_TYPE DataType,
                                      USER_MSG_PARAM Param,
                                      UINT32 Index,
                                      const void *SetPtr,
                                      void **GetPtr)
{
  USER_HANDLER_RESULT result ; 

  result = USER_RESULT_OK; 
  
  UartMgrStatusTemp = *UartMgr_GetStatus(Index); 
  
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
USER_HANDLER_RESULT UartMgrMsg_Cfg(USER_DATA_TYPE DataType,
                                   USER_MSG_PARAM Param,
                                   UINT32 Index,
                                   const void *SetPtr,
                                   void **GetPtr)
{
  USER_HANDLER_RESULT result;


  result = USER_RESULT_OK; 

  UartMgrCfgTemp = *UartMgr_GetCfg(Index); 
  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    *UartMgr_GetCfg(Index) = UartMgrCfgTemp; 

    memcpy(&CfgMgr_ConfigPtr()->UartMgrConfig[Index], &UartMgrCfgTemp, 
           sizeof(UARTMGR_CFG));
           
    //Store the modified temporary structure in the EEPROM.       
    CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
      &CfgMgr_ConfigPtr()->UartMgrConfig[Index],
      sizeof(UARTMGR_CFG));
  }
  return result;  

}



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
// Testing Only 
// Specific to "UartMgrCfgTemp.bRecordingActive" only !!! 
#ifdef UARTMGR_DEBUG_TEST
/*vcast_dont_instrument_start*/
USER_HANDLER_RESULT UartMgrMsg_Debug(USER_DATA_TYPE DataType,
                                     USER_MSG_PARAM Param,
                                     UINT32 Index,
                                     const void *SetPtr,
                                     void **GetPtr)
{
  USER_HANDLER_RESULT result; 
  
  result = USER_RESULT_OK; 
  

  UartMgrDebugTemp = *UartMgr_GetDebug(); 
  
  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);
  
  *UartMgr_GetDebug() = UartMgrDebugTemp; 

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
      pCfgTable = UartMgrCfgTbl;  // Re-set the pointer to beginning of CFG table       

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
