#ifndef PWCDISP_PROTOCOL_H
#define PWCDISP_PROTOCOL_H

/******************************************************************************
            Copyright (C) 2008-2016 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    ECCN:        9D991

    File:        PWCDispProtocol.h

    Description: Contains data structures related to the PWC Display Serial 
                 Protocol Handler 
    
    VERSION
      $Revision: 8 $  $Date: 2/17/16 10:14a $     

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    


/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/

#include "alt_stdtypes.h"
#include "TaskManager.h"
#include "UartMgr.h"

/******************************************************************************
                                 Package Defines
******************************************************************************/
#define PWCDISP_MAX_PARAMS         256 // The max number of parameters
                                       // able to be stored between
                                       // the protocol and the display
                                       // app
#define PWCDISP_MAX_CHAR_PARAMS     24 // Max number of disp characters
#define PWCDISP_MAX_RX_LIST_SIZE     7 // RX Payload Packet size
#define PWCDISP_MAX_TX_LIST_SIZE    26 // TX Payload Packet size with ID
#define PWCDISP_RX_MAX_MSG_SIZE     12 // Full RX Packet size
#define PWCDISP_TX_MSG_SIZE         31 // Full TX Packet size
#define MAX_DEBUG_STRING_SIZE       40 // The maximum debug string size
#define MAX_STORAGE_ITEMS            4 // The max amount of storage items
                                       // between the display and the app
#define TX_TIMER_OFFSET              2 // Arbitrary 2 in order to ensure 
                                       // that rx and tx handling are on
                                       // different frames.
#define PWCDISP_SYNC_BYTE1        0x55
#define PWCDISP_SYNC_BYTE2        0xAA
#define PWCDISP_RX_PACKET_ID      0x11
#define PWCDISP_TX_PACKET_ID      0x01
#define PWCDISP_MAX_RAW_RX_BUF     512 //MAX raw Buffer size for RX Packets
#define PWCDISP_MAX_RAW_TX_BUF      31 //MAX raw Buffer size for TX Packets
#define PWCDISP_TX_FREQUENCY        10 //MAX time between TX messages
#define PWCDISP_HEADER_SIZE          4 //Header size of tx/rx messages
#define PWCDISP_UNUSED_BIT1          5 //Unused bit 1 from RX message.
#define PWCDISP_EVENT_OFFSET         2 //Event button offset from unused bits 
#define PWCDISP_MAX_LINE_LENGTH     12 //Max display line length.
#define PWCDISP_PARAM_NAMES_LENGTH  15 //Max char length for 
//PWCDISP_PARAM_NAMES
#define UP_ARROW                  0x01 //Designated Hex value for up arrow
#define RIGHT_ARROW               0x02 //Designated Hex value for right arrow
#define DOWN_ARROW                0x03 //Designated Hex value for down arrow
#define LEFT_ARROW                0x04 //Designated Hex value for left arrow
#define ENTER_DOT                 0x05 //Designated Hex value for enter dot

#define DISP_PRINT_PARAM(a) sizeof(a), (const char*)a
#define DISP_PRINT_PARAMF(a,b) sizeof(a), (const char*)a, b
/******************************************************************************
                                 Package Typedefs
******************************************************************************/
/********************/
/*   Enumerators    */
/********************/
typedef enum
{
  RX_PROTOCOL,
  TX_PROTOCOL
}PWCDISP_DIRECTION_ENUM;

typedef enum
{
  RXPACKETID,
  BUTTONSTATE_RIGHT,
  BUTTONSTATE_LEFT,
  BUTTONSTATE_ENTER,
  BUTTONSTATE_UP,
  BUTTONSTATE_DOWN,
  BUTTONSTATE_EVENT,
  DOUBLECLICK_RIGHT,
  DOUBLECLICK_LEFT,
  DOUBLECLICK_ENTER,
  DOUBLECLICK_UP,
  DOUBLECLICK_DOWN,
  DOUBLECLICK_EVENT,
  DISCRETE_ZERO,
  DISCRETE_ONE,
  DISCRETE_TWO,
  DISCRETE_THREE,
  DISCRETE_FOUR,
  DISCRETE_FIVE,
  DISCRETE_SIX,
  DISCRETE_SEVEN,
  DISPLAY_HEALTH,
  PART_NUMBER,
  VERSION_NUMBER,
  PWCDISP_RX_ELEMENT_COUNT
}UARTMGR_RX_PWCDISP_ELEMENT;

typedef enum
{
  CHARACTER_0,
  CHARACTER_1,
  CHARACTER_2,
  CHARACTER_3,
  CHARACTER_4,
  CHARACTER_5,
  CHARACTER_6,
  CHARACTER_7,
  CHARACTER_8,
  CHARACTER_9,
  CHARACTER_10,
  CHARACTER_11,
  CHARACTER_12,
  CHARACTER_13,
  CHARACTER_14,
  CHARACTER_15,
  CHARACTER_16,
  CHARACTER_17,
  CHARACTER_18,
  CHARACTER_19,
  CHARACTER_20,
  CHARACTER_21,
  CHARACTER_22,
  CHARACTER_23,
  DC_RATE,
  PWCDISP_TX_ELEMENT_CNT_COMPLETE
}UARTMGR_TX_PWCDISP_ELEMENT_COMPLETE;

/********************/
/*   Data Storage   */
/********************/

typedef union
{
  UINT32 value;
  UINT8 *pPointer;
}PACKET_DATA;

typedef struct
{
  PACKET_DATA      data;      //The actual data 
  INT32            indexMin;  //Minimum array index
  INT32            indexMax;  //Maximum array index
}UARTMGR_PROTOCOL_DATA, *UARTMGR_PROTOCOL_DATA_PTR;

typedef struct
{
  UARTMGR_PROTOCOL_DATA   param[PWCDISP_MAX_PARAMS];
  BOOLEAN                 bNewData;
}UARTMGR_PROTOCOL_STORAGE, *UARTMGR_PROTOCOL_STORAGE_PTR;

/*********************
*       Status       *
*********************/
typedef struct
{
  BOOLEAN     sync;
  UINT16      chksum;
  UINT8       payloadSize;
  UINT8       packetContents[PWCDISP_RX_MAX_MSG_SIZE];
  UINT32      lastSyncPeriod;
  UINT32      lastSyncTime;
  UINT32      syncCnt;
  UINT32      invalidSyncCnt;
  UINT32      lastFrameTime; 
  TIMESTAMP   lastFrameTS; //RX time in TS format for snap shot recording
  }PWCDISP_RX_STATUS, *PWCDISP_RX_STATUS_PTR;

typedef struct
{
  UINT8       packetContents[PWCDISP_TX_MSG_SIZE];
  UINT32      lastPacketPeriod;
  UINT32      lastPacketTime;
  UINT32      validPacketCnt;
  UINT32      invalidPacketCnt;
  UINT32      lastFrameTime; 
  TIMESTAMP   lastFrameTS; //TX time in TS format for snap shot recording
  UINT8       txTimer;
}PWCDISP_TX_STATUS, *PWCDISP_TX_STATUS_PTR;

/*********************
*       Debug        *
*********************/
typedef struct
{
  BOOLEAN                bDebug;      //Enable debug frame display
  PWCDISP_DIRECTION_ENUM protocol;    //Protocol to debug. (TX/RX)
  BOOLEAN                bNewRXFrame; //New frame = RX sync has occurred 
  BOOLEAN                bNewTXFrame; //New frame = TX data checked 
}PWCDISP_DEBUG, *PWCDISP_DEBUG_PTR;

/**********************************
  PWC Display Protocol File Header
**********************************/
#pragma pack(1)
typedef struct
{
  UINT16      size;
  UINT32      packetErrorMax;
  UINT32      noDataTO_ms;
}PWCDISP_FILE_HDR, *PWCDISP_FILE_HDR_PTR;
#pragma pack()
/*********************
*       Logs         *
*********************/
#pragma pack(1)
typedef struct
{
  UINT32 lastFrameTime;
  UINT32 validSyncCnt;
  UINT32 invalidSyncCnt;
}PWCDISP_INVALID_PACKET_LOG;
#pragma pack()

#pragma pack(1)
typedef struct
{
  UINT32 lastFrameTime;
  UINT8  packetContents[PWCDISP_TX_MSG_SIZE];
  UINT32 validPacketCnt;
  UINT32 invalidPacketCnt;
}PWCDISP_DISPLAY_PACKET_ERROR_LOG;
#pragma pack()
/*********************
*   Miscellaneous    *
*********************/

typedef struct
{
  TASK_INDEX    MgrTaskID;
}PWCDISP_DISPLAY_DEBUG_TASK_PARAMS;

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( PWCDISP_PROTOCOL_BODY )
   #define EXPORT
#else
   #define EXPORT extern
#endif

#if defined (PWCDISP_PROTOCOL_BODY)
// Directional index enumeration for modules which ref the PWC Display Protocol
USER_ENUM_TBL protocolIndexType[] =
{ 
  { "RX", RX_PROTOCOL },
  { "TX", TX_PROTOCOL },
  { NULL, 0 }
};
#else
// Export the protocolIndexType enum table
EXPORT USER_ENUM_TBL protocolIndexType[];
#endif
/******************************************************************************
                             Package Exports Variables
******************************************************************************/


/******************************************************************************
                             Package Exports Functions
******************************************************************************/
EXPORT BOOLEAN PWCDispProtocol_Handler(UINT8 *data, UINT16 cnt, UINT16 ch, 
                                       void *runtime_data_ptr, void *info_ptr);
EXPORT void    PWCDispProtocol_Initialize(void);

EXPORT PWCDISP_RX_STATUS_PTR PWCDispProtocol_GetRXStatus (void);
EXPORT PWCDISP_TX_STATUS_PTR PWCDispProtocol_GetTXStatus (void);

EXPORT PWCDISP_DEBUG_PTR     PWCDispProtocol_GetDebug (void);

EXPORT void    PWCDispDebug_Task(void *pParam);
EXPORT void    PWCDispProtocol_DisableLiveStrm(void);
EXPORT void    PWCDispProtocol_TranslateArrows(CHAR charString[], 
                                               UINT16 length);
EXPORT BOOLEAN PWCDispProtocol_Read_Handler(void *pDest, UINT32 chan, 
                                            UINT16 Direction, 
                                            UINT16 nMaxByteSize);
EXPORT void    PWCDispProtocol_Write_Handler(void *pDest, UINT32 chan, 
                                             UINT16 Direction, 
                                             UINT16 nMaxByteSize);
EXPORT UINT8*  PWCDispProtocol_DataLossFlag(void);
EXPORT UINT16 PWCDispProtocol_ReturnFileHdr(UINT8 *dest, const UINT16 max_size,
                                     UINT16 ch);
#endif // PWCDISP_PROTOCOL_H

/******************************************************************************
 *  MODIFICATIONS
 *    $History: PWCDispProtocol.h $
 * 
 * *****************  Version 8  *****************
 * User: John Omalley Date: 2/17/16    Time: 10:14a
 * Updated in $/software/control processor/code/system
 * SCR 1302 - Removed configuration items
 * 
 * *****************  Version 7  *****************
 * User: John Omalley Date: 2/10/16    Time: 9:15a
 * Updated in $/software/control processor/code/system
 * SCR 1302 - Code Review Updates
 * 
 * *****************  Version 6  *****************
 * User: John Omalley Date: 1/29/16    Time: 8:57a
 * Updated in $/software/control processor/code/system
 * SCR 1302 - Display Protocol Updates
 * 
 * *****************  Version 5  *****************
 * User: John Omalley Date: 1/21/16    Time: 4:33p
 * Updated in $/software/control processor/code/system
 * SCR 1302 - Added discrete processing and code review updates
 * 
 * *****************  Version 4  *****************
 * User: John Omalley Date: 1/04/16    Time: 6:22p
 * Updated in $/software/control processor/code/system
 * SCR 1302 - Performance Software Updates
 * 
 * *****************  Version 3  *****************
 * User: John Omalley Date: 12/18/15   Time: 11:10a
 * Updated in $/software/control processor/code/system
 * SCR 1302 - Updates from PSW Contractor
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
 * User: Jeremy Hester    Date: 8/20/2015    Time: 8:11a
 * Created in $/software/control processor/code/system
 * Initial Check In.  Implementation of the PWC Display Protocol and UartMgr 
 *                    Requirements
 * 
 *
 *****************************************************************************/

