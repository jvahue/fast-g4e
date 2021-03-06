#define PWCDISP_USERTABLES_BODY
/******************************************************************************
            Copyright (C) 2007-2016 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    ECCN:        9D991

    File:        PWCDispUserTables.c

    Description: Routines to support the user commands for PWC Display Protocol
                 CSC

    VERSION
    $Revision: 7 $  $Date: 2/25/16 4:56p $

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
                                          
static USER_HANDLER_RESULT PWCDispMsg_Debug(USER_DATA_TYPE DataType,
                                            USER_MSG_PARAM Param,
                                            UINT32 Index,
                                            const void *SetPtr,
                                            void **GetPtr);

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/

static PWCDISP_RX_STATUS   pwcDispRXStatusTemp;
static PWCDISP_TX_STATUS   pwcDispTXStatusTemp;
static PWCDISP_DEBUG       pwcDispDebugTemp;

// Directional index enumeration for modules which ref the PWC Display Protocol
static USER_ENUM_TBL protocolIndexType[] =
{
  { "RX", (UINT32)RX_PROTOCOL },
  { "TX", (UINT32)TX_PROTOCOL },
  { NULL, 0 }
};


/*****************************************/
/* User Table Definitions                */
/*****************************************/

static USER_MSG_TBL pwcDispRXParamTbl[] =
{
  /*Str                  Next Tbl Ptr   Handler Func.        Data Type       Access   Parameter                                         Index   DataLimit EnumTbl*/
  {"SYNC_1",             NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_HEX8, USER_RO, (void *) &pwcDispRXStatusTemp.packetContents[0],  -1, -1, NO_LIMIT, NULL},
  {"SYNC_2",             NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_HEX8, USER_RO, (void *) &pwcDispRXStatusTemp.packetContents[1],  -1, -1, NO_LIMIT, NULL},
  {"SIZE",               NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_HEX8, USER_RO, (void *) &pwcDispRXStatusTemp.packetContents[2],  -1, -1, NO_LIMIT, NULL},
  {"PACKET_ID",          NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_HEX8, USER_RO, (void *) &pwcDispRXStatusTemp.packetContents[3],  -1, -1, NO_LIMIT, NULL},
  {"BTN_ST",             NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_HEX8, USER_RO, (void *) &pwcDispRXStatusTemp.packetContents[4],  -1, -1, NO_LIMIT, NULL},
  {"DBL_ST",             NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_HEX8, USER_RO, (void *) &pwcDispRXStatusTemp.packetContents[5],  -1, -1, NO_LIMIT, NULL},
  {"DISC_ST",            NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_HEX8, USER_RO, (void *) &pwcDispRXStatusTemp.packetContents[6],  -1, -1, NO_LIMIT, NULL},
  {"D_HLTH",             NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_HEX8, USER_RO, (void *) &pwcDispRXStatusTemp.packetContents[7],  -1, -1, NO_LIMIT, NULL},
  {"PART_NUMBER",        NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_HEX8, USER_RO, (void *) &pwcDispRXStatusTemp.packetContents[8],  -1, -1, NO_LIMIT, NULL},
  {"VERSION_NUMBER",     NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_HEX8, USER_RO, (void *) &pwcDispRXStatusTemp.packetContents[9],  -1, -1, NO_LIMIT, NULL},
  {"CHECKSUM_H",         NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_HEX8, USER_RO, (void *) &pwcDispRXStatusTemp.packetContents[10], -1, -1, NO_LIMIT, NULL},
  {"CHECKSUM_L",         NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_HEX8, USER_RO, (void *) &pwcDispRXStatusTemp.packetContents[11], -1, -1, NO_LIMIT, NULL},
  {NULL,                 NULL,          NULL,                NO_HANDLER_DATA}
};

static USER_MSG_TBL pwcDispRXStatusTbl[] =
{
  /*Str                   Next Tbl Ptr   Handler Func.        Data Type          Access   Parameter                                     IndexRange DataLimit EnumTbl*/
  {"SYNC",                NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_BOOLEAN, USER_RO, (void *) &pwcDispRXStatusTemp.sync,           -1, -1,    NO_LIMIT, NULL},
  {"LAST_SYNC_PERIOD_MS", NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_UINT32,  USER_RO, (void *) &pwcDispRXStatusTemp.lastSyncPeriod, -1, -1,    NO_LIMIT, NULL},
  {"LAST_SYNC_MS",        NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_UINT32,  USER_RO, (void *) &pwcDispRXStatusTemp.lastSyncTime,   -1, -1,    NO_LIMIT, NULL},
  {"PACKET_CNT",          NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_UINT32,  USER_RO, (void *) &pwcDispRXStatusTemp.syncCnt,        -1, -1,    NO_LIMIT, NULL},
  {"SYNC_LOSS_CNT",       NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_UINT32,  USER_RO, (void *) &pwcDispRXStatusTemp.invalidSyncCnt, -1, -1,    NO_LIMIT, NULL},
  {"LAST_FRAME_TIME_MS",  NO_NEXT_TABLE, PWCDispRXMsg_Status, USER_TYPE_UINT32,  USER_RO, (void *) &pwcDispRXStatusTemp.lastFrameTime,  -1, -1,    NO_LIMIT, NULL},
  {"MSG_CONTENTS",        pwcDispRXParamTbl, NULL,            NO_HANDLER_DATA},
  {NULL,                  NULL,          NULL,                NO_HANDLER_DATA}
};

static USER_MSG_TBL  pwcDispTXParamTbl[] =
{
  /*Str                  Next Tbl Ptr   Handler Func.        Data Type        Access   Parameter                                         Index   DataLimit EnumTbl*/
  {"SYNC_1",             NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &pwcDispTXStatusTemp.packetContents[0],  -1, -1, NO_LIMIT, NULL},
  {"SYNC_2",             NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &pwcDispTXStatusTemp.packetContents[1],  -1, -1, NO_LIMIT, NULL},
  {"SIZE",               NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &pwcDispTXStatusTemp.packetContents[2],  -1, -1, NO_LIMIT, NULL},
  {"PACKET_ID",          NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &pwcDispTXStatusTemp.packetContents[3],  -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_0",        NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &pwcDispTXStatusTemp.packetContents[4],  -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_1",        NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &pwcDispTXStatusTemp.packetContents[5],  -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_2",        NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &pwcDispTXStatusTemp.packetContents[6],  -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_3",        NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &pwcDispTXStatusTemp.packetContents[7],  -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_4",        NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &pwcDispTXStatusTemp.packetContents[8],  -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_5",        NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &pwcDispTXStatusTemp.packetContents[9],  -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_6",        NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &pwcDispTXStatusTemp.packetContents[10], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_7",        NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &pwcDispTXStatusTemp.packetContents[11], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_8",        NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &pwcDispTXStatusTemp.packetContents[12], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_9",        NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &pwcDispTXStatusTemp.packetContents[13], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_10",       NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &pwcDispTXStatusTemp.packetContents[14], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_11",       NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &pwcDispTXStatusTemp.packetContents[15], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_12",       NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &pwcDispTXStatusTemp.packetContents[16], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_13",       NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &pwcDispTXStatusTemp.packetContents[17], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_14",       NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &pwcDispTXStatusTemp.packetContents[18], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_15",       NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &pwcDispTXStatusTemp.packetContents[19], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_16",       NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &pwcDispTXStatusTemp.packetContents[20], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_17",       NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &pwcDispTXStatusTemp.packetContents[21], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_18",       NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &pwcDispTXStatusTemp.packetContents[22], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_19",       NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &pwcDispTXStatusTemp.packetContents[23], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_20",       NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &pwcDispTXStatusTemp.packetContents[24], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_21",       NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &pwcDispTXStatusTemp.packetContents[25], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_22",       NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &pwcDispTXStatusTemp.packetContents[26], -1, -1, NO_LIMIT, NULL},
  {"CHARACTER_23",       NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &pwcDispTXStatusTemp.packetContents[27], -1, -1, NO_LIMIT, NULL},
  {"DCRATE",             NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &pwcDispTXStatusTemp.packetContents[28], -1, -1, NO_LIMIT, NULL},
  {"CHECKSUM_H",         NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &pwcDispTXStatusTemp.packetContents[29], -1, -1, NO_LIMIT, NULL},
  {"CHECKSUM_L",         NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_HEX8,  USER_RO, (void *) &pwcDispTXStatusTemp.packetContents[30], -1, -1, NO_LIMIT, NULL},
  {NULL,                 NULL,          NULL,                NO_HANDLER_DATA}
};

static USER_MSG_TBL pwcDispTXStatusTbl[] =
{
  /*Str                  Next Tbl Ptr   Handler Func.        Data Type         Access   Parameter                                       IndexRange DataLimit EnumTbl*/
  {"LAST_PACKET_PERIOD_MS", NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_UINT32, USER_RO, (void *) &pwcDispTXStatusTemp.lastPacketPeriod, -1, -1,    NO_LIMIT, NULL},
  {"LAST_PACKET_TIME_MS",   NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_UINT32, USER_RO, (void *) &pwcDispTXStatusTemp.lastPacketTime,   -1, -1,    NO_LIMIT, NULL},
  {"VALID_PACKET_CNT",      NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_UINT32, USER_RO, (void *) &pwcDispTXStatusTemp.validPacketCnt,   -1, -1,    NO_LIMIT, NULL},
  {"INVALID_PACKET_CNT",    NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_UINT32, USER_RO, (void *) &pwcDispTXStatusTemp.invalidPacketCnt, -1, -1,    NO_LIMIT, NULL},
  {"LAST_FRAME_TIME_MS",    NO_NEXT_TABLE, PWCDispTXMsg_Status, USER_TYPE_UINT32, USER_RO, (void *) &pwcDispTXStatusTemp.lastFrameTime,    -1, -1,    NO_LIMIT, NULL},
  {"MSG_CONTENTS",          pwcDispTXParamTbl, NULL,            NO_HANDLER_DATA},
  {NULL,                    NULL,          NULL,                NO_HANDLER_DATA}
};

static USER_MSG_TBL pwcDispDebugTbl[] =
{
  /*Str         Next Tbl Ptr   Handler Func.     Data Type          Access              Parameter                              IndexRange DataLimit EnumTbl*/
  {"ENABLE",    NO_NEXT_TABLE, PWCDispMsg_Debug, USER_TYPE_BOOLEAN, (USER_RW|USER_GSE), (void *) &pwcDispDebugTemp.bDebug,     -1, -1,    NO_LIMIT, NULL},
  {"DIRECTION", NO_NEXT_TABLE, PWCDispMsg_Debug, USER_TYPE_ENUM,    (USER_RW|USER_GSE), (void *) &pwcDispDebugTemp.protocol,   -1, -1,    NO_LIMIT, protocolIndexType},
  {NULL,        NULL,          NULL,             NO_HANDLER_DATA}
};

static USER_MSG_TBL pwcDispStatusTbl[] =
{
  {"RX", pwcDispRXStatusTbl, NULL,         NO_HANDLER_DATA},
  {"TX", pwcDispTXStatusTbl, NULL,         NO_HANDLER_DATA},
  {NULL, NULL,               NULL,         NO_HANDLER_DATA}
};

static USER_MSG_TBL pwcDispProtocolRoot[] =
{
  /*Str        Next Tbl Ptr        Handler Func. Data Type*/
  {"STATUS",   pwcDispStatusTbl,   NULL,         NO_HANDLER_DATA},
  {"DEBUG",    pwcDispDebugTbl,    NULL,         NO_HANDLER_DATA},
  {NULL,       NULL,               NULL,         NO_HANDLER_DATA}
};

static USER_MSG_TBL pwcDispProtocolRootTblPtr = 
{"PWCDISP",     pwcDispProtocolRoot, NULL, NO_HANDLER_DATA}; 
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
  
  pwcDispRXStatusTemp = *PWCDispProtocol_GetRXStatus();
  
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
  
  pwcDispTXStatusTemp = *PWCDispProtocol_GetTXStatus();
  
  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
  
  return result;
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

  pwcDispDebugTemp = *PWCDispProtocol_GetDebug();

  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    *PWCDispProtocol_GetDebug() = pwcDispDebugTemp;
  }
  return result;
}

/*****************************************************************************
 *  MODIFICATIONS
 *    $History: PWCDispUserTables.c $
 * 
 * *****************  Version 7  *****************
 * User: John Omalley Date: 2/25/16    Time: 4:56p
 * Updated in $/software/control processor/code/system
 * SCR 1302 - Update per PWCDispProtocol.c Code Review
 * 
 * *****************  Version 6  *****************
 * User: John Omalley Date: 2/17/16    Time: 10:14a
 * Updated in $/software/control processor/code/system
 * SCR 1302 - Removed configuration items
 * 
 * *****************  Version 5  *****************
 * User: John Omalley Date: 2/10/16    Time: 9:16a
 * Updated in $/software/control processor/code/system
 * SCR 1302 - Code Review Updates
 * 
 * *****************  Version 4  *****************
 * User: John Omalley Date: 1/21/16    Time: 4:33p
 * Updated in $/software/control processor/code/system
 * SCR 1302 - Added discrete processing and code review updates
 * 
 * *****************  Version 3  *****************
 * User: John Omalley Date: 1/04/16    Time: 6:22p
 * Updated in $/software/control processor/code/system
 * SCR 1302 - Performance Software Updates
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
