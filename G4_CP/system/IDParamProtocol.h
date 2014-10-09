#ifndef ID_PARAM_PROTOCOL_H
#define ID_PARAM_PROTOCOL_H

/******************************************************************************
            Copyright (C) 2008-2014 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:        IDParamProtocol.h

    Description: Contains data structures related to the ID Param Serial Protocol
                 Handler

    VERSION
      $Revision: 1 $  $Date: 14-10-08 7:11p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/


/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"

#include "TaskManager.h"


/******************************************************************************
                                 Package Defines
******************************************************************************/
#define ID_PARAM_NUM_FRAME_TYPES 2 // 0 = FADEC 1 = PEC based on Frame Word X bit #X
#define ID_PARAM_CFG_MAX  256 // Max Params supported for recording.
#define ID_PARAM_FRAME_MAX ID_PARAM_CFG_MAX  // Max Param supported in frame
#define ID_PARAM_FRAME_MIN 5 // Min Params supported in frame

#define ID_PARAM_CH_MAX   3   // Max # Uart Channels allowed for ID Param Protocol Processing
#define ID_PARAM_NOT_USED 0xFFFF // ID to indicate param not used
#define ID_PARAM_INDEX_NOT_USED 0xFFFF   // Index not used.  Initialized state.
#define ID_PARAM_INDEX_SCROLL 0xFFFE     // Value used to indentify Scroll ID Use
#define ID_PARAM_NO_TIMEOUT 0x0


// ID Param Config Defaults
#define ID_PARAM_DATA_DEFAULT  ID_PARAM_NOT_USED, /* id */\
                                               0, /* scale */\
                                               0, /* tol */\
                             ID_PARAM_NO_TIMEOUT  /* timeout_ms */

#define ID_PARAM_DATA_ARRAY_DEFAULT \
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT,\
  ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT, ID_PARAM_DATA_DEFAULT

// ID Param Config Defaults
#define ID_PARAM_SCROLL_DEFAULT  ID_PARAM_NOT_USED, /* id of scroll ID */\
                                 ID_PARAM_NOT_USED  /* id of scroll Value */

#define ID_PARAM_CFG_DEFAULT    \
     ID_PARAM_CFG_MAX,          \
     ID_PARAM_SCROLL_DEFAULT,   \
     ID_PARAM_DATA_ARRAY_DEFAULT


/******************************************************************************
                                 Package Typedefs
******************************************************************************/
typedef struct
{
  UINT16 id;         // FADEC/PEC/etc Param ID (from Engine Spec) to be recorded
  FLOAT32 scale;     // "range scale" of the Param.  Internally lsb will be calculated from
                     //    scale / 32678 (assumes 15 bits always with msb = sign bit)
                     //    scale == 0, will force lsb to be "1" always.
  FLOAT32 tol;       // DR tolerance to be applied to DR algorithm
  UINT32 timeout_ms; // timeout (ms) before declaring param as invalid for DR logic
                     //    note: altough is ms actual resolutionis 40 ms (uart task processing
                     //          resolution).
} ID_PARAM_DATA_CFG, *ID_PARAM_DATA_CFG_PTR;

typedef struct
{
  UINT16 id;         // ID used to identify the Scroll ID
  UINT16 value;      // ID used to identify the Scroll Value ID
} ID_PARAM_SCROLL_CFG;

typedef struct
{
  UINT16 maxWords;   // Max Word expected in FADEC / PEC frame. Rej frames > this value.
  ID_PARAM_SCROLL_CFG scroll;  // Scroll ID Configuration
  ID_PARAM_DATA_CFG data[ID_PARAM_CFG_MAX];
} ID_PARAM_CFG, *ID_PARAM_CFG_PTR;

typedef struct
{
  UINT16 nWords;        // Number of words in frame.
  UINT16 nElements;     // Number of elements (including scrolling) in data stream
  UINT32 cntGoodFrames; // Cnt # of good frames recieved
  UINT32 lastFrameTime_tick; // Tick time of last good frame received
} ID_PARAM_FRAME_TYPE, *ID_PARAM_FRAME_TYPE_PTR;

typedef struct
{
  BOOLEAN sync;         // Run time sync flag.  Current state.
  UINT32 cntBadCRC;     // Cnt # of bad crc frames received
  UINT32 cntBadFrame;   // Cnt # of bad frames (CRC is good) received
  UINT32 cntReSync;     // Cnt # of "resynce" encountered (ref Req on "sync")
  UINT32 cntGoodFrames; // Cnt # total good frames received (both FADEC/PEC)
  UINT32 lastFrameTime_tick; // Tick time of last good frame received
  ID_PARAM_FRAME_TYPE frameType[ID_PARAM_NUM_FRAME_TYPES]; // FADEC or PEC specific
                                                           //    frame data summary
  UINT16 cntRecParam;   // Params sel in cfg to be recorded
  UINT16 cntMonParam;   // Params sel in cfg + sensor request (don't overlap cfg) to be mon
  UINT32 cntBadFrameInRow; // Keeps track of 1 bad Frame in a Row.
  UINT32 cntBadCrcInRow;   // Keeps track of 3 bad CRC in a Row
} ID_PARAM_STATUS, *ID_PARAM_STATUS_PTR;


/**********************************
  ID Param Protocol File Header
**********************************/
#pragma pack(1)
typedef struct
{
  UINT16  id;
  FLOAT32 scale;
  FLOAT32 tol;
  UINT32  timeout;
} ID_PARAM_DATA_HDR, *ID_PARAM_DATA_HDR_PTR;

typedef struct
{
  UINT16 size;       // Protocol Hdr, including "size" field.
  UINT16 numParams;  // Number of parameters
  ID_PARAM_DATA_HDR data[ID_PARAM_CFG_MAX];
} ID_PARAM_FILE_HDR, *ID_PARAM_FILE_HDR_PTR;

typedef struct
{
  UINT32 cntBadCRC;   // Cnt of Bad CRC detected since Power Up
  UINT32 cntBadFrame; // Cnt of Bad Frame detected since Power Up
  UINT32 cntResync;   // Cnt of ReSync since Power Up
  BOOLEAN idleTimeOut;// T=Sync Loss due to idle timeout, F=Sync loss due to CRC/Bad Frame
} ID_PARAM_SYNC_LOSS_LOG, *ID_PARAM_SYNC_LOSS_LOG_PTR;
#pragma pack()


/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( ID_PARAM_PROTOCOL_BODY )
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
EXPORT void IDParamProtocol_Initialize ( void );
EXPORT BOOLEAN  IDParamProtocol_Handler ( UINT8 *data, UINT16 cnt, UINT16 ch,
                                          void *uart_data_ptr, void *info_ptr );

EXPORT ID_PARAM_STATUS_PTR IDParamProtocol_GetStatus (UINT8 index);
EXPORT BOOLEAN IDParamProtocol_SensorSetup ( UINT32 gpA, UINT32 gpB, UINT16 param,
                                             void *info_ptr, void *uart_data_ptr, UINT16 ch );
EXPORT UINT16 IDParamProtocol_ReturnFileHdr ( UINT8 *dest, const UINT16 max_size, UINT16 ch );

EXPORT void IDParamProtocol_InitUartMgrData( UINT16 ch, void *uart_data_ptr );

#endif // ID_PARAM_PROTOCOL_H


/*****************************************************************************
 *  MODIFICATIONS
 *    $History: IDParamProtocol.h $
 * 
 * *****************  Version 1  *****************
 * User: Peter Lee    Date: 14-10-08   Time: 7:11p
 * Created in $/software/control processor/code/system
 * SCR #1263 ID Param Protocol Implementation
 *
 *
 *****************************************************************************/

