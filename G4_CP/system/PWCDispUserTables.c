#define PWCDISP_USERTABLES_BODY
/******************************************************************************
            Copyright (C) 2007-2015 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    ECCN:        90991

    File:        PWCDispUserTables.c

    Description: Routines to support the user commands for PWC Display Protocol
                 CSC

    VERSION
    $Revision: 2 $  $Date: 11/19/15 4:31p $

******************************************************************************/
#ifndef PWCDISP_PROTOCOL_BODY
#error PWCDispUserTables.c should only be included by PWCDispProtocol.c
#endif

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "CfgManager.h"
#include "PWCDispProtocol.h"


/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/


/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
static USER_HANDLER_RESULT PWCDispRXMsg_Status(USER_DATA_TYPE DataType,
                                               USER_MSG_PARAM Param,
                                               UINT32 Index,
                                               const void *SetPtr,
                                               void **GetPtr);

static USER_HANDLER_RESULT PWCDispTXMsg_Status(USER_DATA_TYPE DataType,
                                              USER_MSG_PARAM Param,
                                              UINT32 Index,
                                              const void *SetPtr,
                                              void **GetPtr);

static USER_HANDLER_RESULT PWCDispMsg_Cfg(USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr);

static USER_HANDLER_RESULT PWCDispmsg_ShowConfig(USER_DATA_TYPE DataType,
                                                 USER_MSG_PARAM Param,
                                                 UINT32 Index,
                                                 const void *SetPtr,
                                                 void **GetPtr);
                                          
static USER_HANDLER_RESULT PWCDispMsg_Debug(USER_DATA_TYPE DataType,
                                            USER_MSG_PARAM Param,
                                            UINT32 Index,
                                            const void *SetPtr,
                                            void **GetPtr);

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/

static PWCDISP_RX_STATUS   PWCDispRXStatusTemp;
static PWCDISP_TX_STATUS   PWCDispTXStatusTemp;
static PWCDISP_CFG         PWCDispCfgTemp;
static PWCDISP_DEBUG       PWCDispDebugTemp;


/*****************************************/
/* User Table Definitions                */
/*****************************************/

static USER_MSG_TBL PWCDispRXParamTbl[] =
{
  /*Str                  Next Tbl Ptr   Handler Func.        Data Type       Access   Parameter                                         Index   DataLimit EnumTbl*/
  {"SYNC_1",             NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_HEX8, USER_RO, (void *) &PWCDispRXStatusTemp.packetContents[0],  -1, -1, NO_LIMIT, NULL},
  {"SYNC_2",             NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_HEX8, USER_RO, (void *) &PWCDispRXStatusTemp.packetContents[1],  -1, -1, NO_LIMIT, NULL},
  {"SIZE",               NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_HEX8, USER_RO, (void *) &PWCDispRXStatusTemp.packetContents[2],  -1, -1, NO_LIMIT, NULL},
  {"PACKET_ID",          NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_HEX8, USER_RO, (void *) &PWCDispRXStatusTemp.packetContents[3],  -1, -1, NO_LIMIT, NULL},
  {"BTN_ST",             NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_HEX8, USER_RO, (void *) &PWCDispRXStatusTemp.packetContents[4],  -1, -1, NO_LIMIT, NULL},
  {"DBL_ST",             NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_HEX8, USER_RO, (void *) &PWCDispRXStatusTemp.packetContents[5],  -1, -1, NO_LIMIT, NULL},
  {"DISC_ST",            NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_HEX8, USER_RO, (void *) &PWCDispRXStatusTemp.packetContents[6],  -1, -1, NO_LIMIT, NULL},
  {"D_HLTH",             NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_HEX8, USER_RO, (void *) &PWCDispRXStatusTemp.packetContents[7],  -1, -1, NO_LIMIT, NULL},
  {"PART_NUMBER",        NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_HEX8, USER_RO, (void *) &PWCDispRXStatusTemp.packetContents[8],  -1, -1, NO_LIMIT, NULL},
  {"VERSION_NUMBER",     NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_HEX8, USER_RO, (void *) &PWCDispRXStatusTemp.packetContents[9],  -1, -1, NO_LIMIT, NULL},
  {"CHECKSUM_H",         NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_HEX8, USER_RO, (void *) &PWCDispRXStatusTemp.packetContents[10], -1, -1, NO_LIMIT, NULL},
  {"CHECKSUM_L",         NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_HEX8, USER_RO, (void *) &PWCDispRXStatusTemp.packetContents[11], -1, -1, NO_LIMIT, NULL},
  {NULL,                 NULL,          NULL,                NO_HANDLER_DATA}
};

static USER_MSG_TBL PWCDispRXStatusTbl[] =
{
  /*Str                Next Tbl Ptr   Handler Func.        Data Type          Access   Parameter                                     IndexRange DataLimit EnumTbl*/
  {"SYNC",             NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_BOOLEAN, USER_RO, (void *) &PWCDispRXStatusTemp.sync,           -1, -1,    NO_LIMIT, NULL},
  {"CHECKSUM",         NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_UINT16,  USER_RO, (void *) &PWCDispRXStatusTemp.chksum,         -1, -1,    NO_LIMIT, NULL},
  {"PAYLOAD_SIZE",     NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_UINT8,   USER_RO, (void *) &PWCDispRXStatusTemp.payloadSize,    -1, -1,    NO_LIMIT, NULL},
  {"LAST_SYNC_PERIOD", NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_UINT32,  USER_RO, (void *) &PWCDispRXStatusTemp.lastSyncPeriod, -1, -1,    NO_LIMIT, NULL},
  {"LAST_SYNC",        NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_UINT32,  USER_RO, (void *) &PWCDispRXStatusTemp.lastSyncTime,   -1, -1,    NO_LIMIT, NULL},
  {"SYNC_CNT",         NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_UINT32,  USER_RO, (void *) &PWCDispRXStatusTemp.syncCnt,        -1, -1,    NO_LIMIT, NULL},
  {"INVALID_D_HLTH",   NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_UINT32,  USER_RO, (void *) &PWCDispRXStatusTemp.dispHealthTimer,-1, -1,    NO_LIMIT, NULL},
  {"INVALID_SYNC_CNT", NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_UINT32,  USER_RO, (void *) &PWCDispRXStatusTemp.invalidSyncCnt, -1, -1,    NO_LIMIT, NULL},
  {"LAST_FRAME_TIME",  NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_UINT32,  USER_RO, (void *) &PWCDispRXStatusTemp.lastFrameTime,  -1, -1,    NO_LIMIT, NULL},
  {"MSG_CONTENTS",     PWCDispRXParamTbl, NULL,            NO_HANDLER_DATA},
  {NULL,               NULL,          NULL,                NO_HANDLER_DATA}
};

static USER_MSG_TBL  PWCDispTXParamTbl[] =
{
  /*Str                  Next Tbl Ptr   Handler Func.        Data Type        Access   Parameter                                         Index   DataLimit EnumTbl*/
  {"SYNC_1",             NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &PWCDispTXStatusTemp.packetContents[0],  -1, -1, NO_LIMIT, NULL},
  {"SYNC_2",             NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &PWCDispTXStatusTemp.packetContents[1],  -1, -1, NO_LIMIT, NULL},
  {"SIZE",               NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &PWCDispTXStatusTemp.packetContents[2],  -1, -1, NO_LIMIT, NULL},
  {"PACKET_ID",          NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &PWCDispTXStatusTemp.packetContents[3],  -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_0",        NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &PWCDispTXStatusTemp.packetContents[4],  -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_1",        NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &PWCDispTXStatusTemp.packetContents[5],  -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_2",        NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &PWCDispTXStatusTemp.packetContents[6],  -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_3",        NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &PWCDispTXStatusTemp.packetContents[7],  -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_4",        NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &PWCDispTXStatusTemp.packetContents[8],  -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_5",        NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &PWCDispTXStatusTemp.packetContents[9],  -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_6",        NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &PWCDispTXStatusTemp.packetContents[10], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_7",        NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &PWCDispTXStatusTemp.packetContents[11], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_8",        NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &PWCDispTXStatusTemp.packetContents[12], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_9",        NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &PWCDispTXStatusTemp.packetContents[13], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_10",       NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &PWCDispTXStatusTemp.packetContents[14], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_11",       NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &PWCDispTXStatusTemp.packetContents[15], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_12",       NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &PWCDispTXStatusTemp.packetContents[16], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_13",       NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &PWCDispTXStatusTemp.packetContents[17], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_14",       NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &PWCDispTXStatusTemp.packetContents[18], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_15",       NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &PWCDispTXStatusTemp.packetContents[19], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_16",       NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &PWCDispTXStatusTemp.packetContents[20], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_17",       NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &PWCDispTXStatusTemp.packetContents[21], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_18",       NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &PWCDispTXStatusTemp.packetContents[22], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_19",       NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &PWCDispTXStatusTemp.packetContents[23], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_20",       NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &PWCDispTXStatusTemp.packetContents[24], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_21",       NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &PWCDispTXStatusTemp.packetContents[25], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_22",       NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &PWCDispTXStatusTemp.packetContents[26], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_23",       NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &PWCDispTXStatusTemp.packetContents[27], -1, -1, NO_LIMIT, NULL},
  {"DCRATE",             NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &PWCDispTXStatusTemp.packetContents[28], -1, -1, NO_LIMIT, NULL},
  {"CHECKSUM_H",         NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &PWCDispTXStatusTemp.packetContents[29], -1, -1, NO_LIMIT, NULL},
  {"CHECKSUM_L",         NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &PWCDispTXStatusTemp.packetContents[30], -1, -1, NO_LIMIT, NULL},
  {NULL,                 NULL,          NULL,                NO_HANDLER_DATA}
};

static USER_MSG_TBL PWCDispTXStatusTbl[] =
{
  /*Str                  Next Tbl Ptr   Handler Func.        Data Type         Access   Parameter                                       IndexRange DataLimit EnumTbl*/
  {"LAST_PACKET_PERIOD", NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_UINT32, USER_RO, (void *) &PWCDispTXStatusTemp.lastPacketPeriod, -1, -1,    NO_LIMIT, NULL},
  {"LAST_PACKET_TIME",   NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_UINT32, USER_RO, (void *) &PWCDispTXStatusTemp.lastPacketTime,   -1, -1,    NO_LIMIT, NULL},
  {"VALID_PACKET_CNT",   NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_UINT32, USER_RO, (void *) &PWCDispTXStatusTemp.validPacketCnt,   -1, -1,    NO_LIMIT, NULL},
  {"INVALID_PACKET_CNT", NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_UINT32, USER_RO, (void *) &PWCDispTXStatusTemp.invalidPacketCnt, -1, -1,    NO_LIMIT, NULL},
  {"LAST_FRAME_TIME",    NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_UINT32, USER_RO, (void *) &PWCDispTXStatusTemp.lastFrameTime,    -1, -1,    NO_LIMIT, NULL},
  {"MSG_CONTENTS",       PWCDispTXParamTbl, NULL,            NO_HANDLER_DATA},
  {NULL,                 NULL,          NULL,                NO_HANDLER_DATA}
};

static USER_MSG_TBL PWCDispCfgTbl[] =
{
  /*Str                    Next Tbl Ptr   Handler Func.   Data Type         Access   Parameter                                IndexRange DataLimit EnumTbl*/
  {"PACKET_ERROR_MAX",     NO_NEXT_TABLE, PWCDispMsg_Cfg, USER_TYPE_UINT32, USER_RW, (void *) &PWCDispCfgTemp.packetErrorMax, -1, -1,    NO_LIMIT, NULL},
  {NULL,                   NULL,          NULL,           NO_HANDLER_DATA}
};

static USER_MSG_TBL PWCDispDebugTbl[] =
{
  /*Str         Next Tbl Ptr   Handler Func.     Data Type          Access              Parameter                              IndexRange DataLimit EnumTbl*/
  {"ENABLE",    NO_NEXT_TABLE, PWCDispMsg_Debug, USER_TYPE_BOOLEAN, (USER_RW|USER_GSE), (void *) &PWCDispDebugTemp.bDebug,     -1, -1,    NO_LIMIT, NULL},
  {"PROTOCOL",  NO_NEXT_TABLE, PWCDispMsg_Debug, USER_TYPE_BOOLEAN, (USER_RW|USER_GSE), (void *) &PWCDispDebugTemp.bProtocol,  -1, -1,    NO_LIMIT, NULL},
  {NULL,        NULL,          NULL,             NO_HANDLER_DATA}
};

static USER_MSG_TBL PWCDispStatusTbl[] =
{
  {"RXSTATUS", PWCDispRXStatusTbl, NULL,         NO_HANDLER_DATA},
  {"TXSTATUS", PWCDispTXStatusTbl, NULL,         NO_HANDLER_DATA},
  {NULL,       NULL,               NULL,         NO_HANDLER_DATA}
};

static USER_MSG_TBL PWCDispProtocolRoot[] =
{
  /*Str        Next Tbl Ptr        Handler Func. Data Type*/
  {"STATUS",   PWCDispStatusTbl,   NULL,         NO_HANDLER_DATA},
  {"DEBUG",    PWCDispDebugTbl,    NULL,         NO_HANDLER_DATA},
  {"CFG",      PWCDispCfgTbl,      NULL,         NO_HANDLER_DATA},
  {DISPLAY_CFG,NO_NEXT_TABLE,      PWCDispmsg_ShowConfig, USER_TYPE_ACTION, (USER_RO|USER_NO_LOG|USER_GSE), NULL,     -1,-1,     NO_LIMIT, NULL},
  {NULL,       NULL,               NULL,         NO_HANDLER_DATA}
};

static USER_MSG_TBL PWCDispProtocolRootTblPtr = 
{"PWCDISP",     PWCDispProtocolRoot, NULL, NO_HANDLER_DATA}; 
/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:    PWCDispRXMsg_Status
 *
 * Description: Called by the User.c module from the reference to this function
 *              in the user message tables above.
 *              Retrieves the latest PWCDisp RX Status Data
 *
 *Parameters:   [in] DataType:  C type of the data to be read or changed, used
 *                              for casting the data pointers
 *              [in/out] Param: Pointer to the configuration item to be read
 *                              or changed
 *              [in] Index:     Index parameter is used to reference the
 *                              specific sensor to change. Range is validated
 *                              by the user manager
 *              [in] SetPtr:    For write commands, a pointer to the data to
 *                              write to the configuration
 *              [out] GetPtr:   For read commands, UserCfg function will set
 *                              this to the location of the data requested
 *
 * Returns:     USER_RESULT_OK:    Processed Successfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
******************************************************************************/
static
USER_HANDLER_RESULT PWCDispRXMsg_Status(USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr)
{
  USER_HANDLER_RESULT result ;

  result = USER_RESULT_OK;
  
  PWCDispRXStatusTemp = *PWCDispProtocol_GetRXStatus();
  
  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
  
  return result;
}

/******************************************************************************
 * Function:    PWCDispTXMsg_Status
 *
 * Description: Called by the User.c module from the reference to this function
 *              in the user message tables above.
 *              Retrieves the latest PWCDisp TX Status Data
 *
 *Parameters:   [in] DataType:  C type of the data to be read or changed, used
 *                              for casting the data pointers
 *              [in/out] Param: Pointer to the configuration item to be read
 *                              or changed
 *              [in] Index:     Index parameter is used to reference the
 *                              specific sensor to change. Range is validated
 *                              by the user manager
 *              [in] SetPtr:    For write commands, a pointer to the data to
 *                              write to the configuration
 *              [out] GetPtr:   For read commands, UserCfg function will set
 *                              this to the location of the data requested
 *
 * Returns:     USER_RESULT_OK:    Processed Successfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
******************************************************************************/
static
USER_HANDLER_RESULT PWCDispTXMsg_Status(USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr)
{
  USER_HANDLER_RESULT result ;

  result = USER_RESULT_OK;
  
  PWCDispTXStatusTemp = *PWCDispProtocol_GetTXStatus();
  
  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
  
  return result;
}

/******************************************************************************
 * Function:    PWCDispMsg_Cfg
 *
 * Description: Called by the User.c module from the reference to this function
 *              in the user message tables above.
 *              Retrieves and set the latest PWC Display Cfg Data
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
 * Notes:       none
 *
*****************************************************************************/
static
USER_HANDLER_RESULT PWCDispMsg_Cfg(USER_DATA_TYPE DataType,
                                   USER_MSG_PARAM Param,
                                   UINT32 Index,
                                   const void *SetPtr,
                                   void **GetPtr)
{
  USER_HANDLER_RESULT result;
  
  result = USER_RESULT_OK;
  
  // Determine which array element
  memcpy(&PWCDispCfgTemp, &CfgMgr_ConfigPtr()->PWCDispConfig, 
         sizeof(PWCDispCfgTemp));
         
  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
  
  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    memcpy(&CfgMgr_ConfigPtr()->PWCDispConfig, &PWCDispCfgTemp,
           sizeof(PWCDISP_CFG));
           
    CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(), 
                           &CfgMgr_ConfigPtr()->PWCDispConfig,
                           sizeof(PWCDISP_CFG));
  }
  return(result);
}

/******************************************************************************
 * Function:    PWCDispMsg_Debug
 *
 * Description: Called by the User.c module from the reference to this function
 *              in the user message tables above.
 *              Retrieves the latest PWC Display Debug Data
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
USER_HANDLER_RESULT PWCDispMsg_Debug(USER_DATA_TYPE DataType,
                                     USER_MSG_PARAM Param,
                                     UINT32 Index,
                                     const void *SetPtr,
                                     void **GetPtr)
{
  USER_HANDLER_RESULT result;
  
  result = USER_RESULT_OK;
  
  PWCDispDebugTemp = *PWCDispProtocol_GetDebug();
  
  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
  
  *PWCDispProtocol_GetDebug() = PWCDispDebugTemp;
  
  return result;
}

/******************************************************************************
* Function:    PWCDispmsg_ShowConfig
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
static USER_HANDLER_RESULT PWCDispmsg_ShowConfig(USER_DATA_TYPE DataType,
                                                 USER_MSG_PARAM Param,
                                                 UINT32 Index,
                                                 const void *SetPtr,
                                                 void **GetPtr)
{
  USER_HANDLER_RESULT result;
  CHAR label[USER_MAX_MSG_STR_LEN * 3];

  // Top level name is a single indented space
  CHAR branchName[USER_MAX_MSG_STR_LEN] = " ";

  USER_MSG_TBL *pCfgTable;

  snprintf(label, sizeof(label), "\r\n\r\nPWC_DISPLAY.CFG");
  
  // Assume the worst possible result
  result = USER_RESULT_ERROR;
  
  if(User_OutputMsgString(label, FALSE))
  {
    pCfgTable = PWCDispCfgTbl;

    result = User_DisplayConfigTree(branchName, pCfgTable, 0, 0, NULL);
  }

  return result;
}

/*****************************************************************************
 *  MODIFICATIONS
 *    $History: PWCDispUserTables.c $
 * 
 * *****************  Version 2  *****************
 * User: John Omalley Date: 11/19/15   Time: 4:31p
 * Updated in $/software/control processor/code/system
 * SCR 1302 - Display Protocol Updates from Design Review
 * 
 * *****************  Version 1  *****************
 * User: Jeremy Hester Date: 10/02/15   Time: 9:56a
 * Created in $/software/control processor/code/system
 * SCR - 1302 Added PWC Display Protocol
 * 
 * *****************  Version 1  *****************
 * User: Jeremy Hester Date: 8/20/15    Time: 10:47a
 * Created in $/software/control processor/code/system
 * Initial Check In. Implementation of the PWCDisp and 
 * UartMgr Requirements
 ****************************************************************************/
