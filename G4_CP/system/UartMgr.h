#ifndef UARTMGR_H
#define UARTMGR_H
/******************************************************************************
            Copyright (C) 2008-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:        UartMgr.h

    Description: Contains data structures related to the Uart Mgr CSC

    VERSION
      $Revision: 22 $  $Date: 15-02-06 7:18p $

******************************************************************************/


/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/


/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "Uart.h"
#include "faultmgr.h"
#include "taskmanager.h"
#include "DataReduction.h"

#include "EMU150Protocol.h"
#include "GBSProtocol.h"

#include "alt_Time.h"

#include "ACS_Interface.h"


/******************************************************************************
                                 Package Defines
******************************************************************************/
#define UARTMGR_CH_DEFAULT(ChStr) \
                               ChStr, /* Name */\
               UARTMGR_PROTOCOL_NONE, /* Protocol - _NONE */\
                         SECONDS_120, /* Channel Startup Time */\
                         SECONDS_120, /* Channel Timeout */\
                         STA_CAUTION, /* Channel Sys Cond */\
                           STA_FAULT, /* PBIT Sys Cond */\
                               FALSE, /* Disable Uart Port */\
                {UART_CFG_BPS_115200, /* Port BPS */\
                       UART_CFG_DB_8, /* Port Data */\
                       UART_CFG_SB_1, /* Port Stop */\
                UART_CFG_PARITY_NONE, /* Port None */\
                UART_CFG_DUPLEX_FULL} /* Duplex FULL */


#define UARTMGR_DEFAULT  UARTMGR_CH_DEFAULT("UART 0"),\
                         UARTMGR_CH_DEFAULT("UART 1"),\
                         UARTMGR_CH_DEFAULT("UART 2"),\
                         UARTMGR_CH_DEFAULT("UART 3")


#define UARTMGR_MAX_PARAM_WORD            256
#define UART_WORD_TIMEOUT_FAIL_MSG_SIZE   128   // CBIT Word timeout failure
                                                //     message size
#define UART_CFG_NAME_SIZE                 32
#define UART_DSB_TIMEOUT                    0

/******************************************************************************
                                 Package Typedefs
******************************************************************************/
typedef BOOLEAN (*PROTOCOL_HNDL)          ( UINT8 *data, UINT16 cnt, UINT16 ch,
                                            void *runtime_data_ptr, void *word_info_ptr );
typedef UINT16  (*PROTOCOL_FILE_HDR)      ( UINT8 *dest, const UINT16 max_size, UINT16 ch );
typedef void    (*PROTOCOL_DOWNLOAD_HNDL) ( UINT8 port,
                                            BOOLEAN *bStartDownload,
                                            BOOLEAN *bDownloadCompleted,
                                            BOOLEAN *bWriteInProgress,
                                            BOOLEAN *bWriteOk,
                                            BOOLEAN *bHalt,
                                            BOOLEAN *bNewRec,
                                            UINT32  **pSize,
                                            UINT8   **pData );
typedef BOOLEAN (*PROTOCOL_CH_READY_HNDL)  ( UINT16 ch );
typedef void    (*PROTOCOL_DOWNLOAD_CLR_HNDL) ( BOOLEAN Run, INT32 param ); 


/**********************************
  Uart Mgr Configuration
**********************************/
typedef enum
{
  UARTMGR_PROTOCOL_NONE,
  UARTMGR_PROTOCOL_F7X_N_PARAM,
  UARTMGR_PROTOCOL_EMU150,
  UARTMGR_PROTOCOL_ID_PARAM,
  UARTMGR_PROTOCOL_GBS,
  UARTMGR_PROTOCOL_MAX
} UARTMGR_PROTOCOLS;

typedef struct
{
  UART_CFG_BPS    nBPS;
  UART_CFG_DB     dataBits;
  UART_CFG_SB     stopBits;
  UART_CFG_PARITY parity;
  UART_CFG_DUPLEX duplex;
} UARTMGR_PORT_CFG, *UARTMGR_PORT_CFG_PTR;

typedef struct
{
  CHAR              sName[UART_CFG_NAME_SIZE];
  UARTMGR_PROTOCOLS protocol;
  UINT32            channelStartup_s;
  UINT32            channelTimeOut_s;
  FLT_STATUS        channelSysCond;
  FLT_STATUS        sysCondPBIT;
  BOOLEAN           bEnabled;
  UARTMGR_PORT_CFG  port;
} UARTMGR_CFG, *UARTMGR_CFG_PTR;

//A type for an array of the maximum number of uarts
//Used for storing uart configurations in the configuration manager
typedef UARTMGR_CFG UARTMGR_CFGS[UART_NUM_OF_UARTS];


/**********************************
  Uart Data (Internal)
**********************************/
typedef struct
{
  UINT16    id;
  UINT16    scale;
  FLOAT32   scale_lsb;          // lsb value (from scale) of the integer val for
                                //    conversion to and from eng val.
  UINT16    val_raw;
  UINT32    rxTime;             // Rx time in tick for run time "delta time" recording
  TIMESTAMP rxTS;               // Rx time in TS format for snap shot recording
  BOOLEAN   bNewVal;
  BOOLEAN   bValid;
  BOOLEAN   bReInit;            // On transition from invalid to valid data should
                                //   Reinit data reduction.
  UINT16    dataRed_val;
  UINT16    dataRed_TimeDelta;
  UINT32    cfgTimeOut;
} UARTMGR_RUNTIME_DATA, *UARTMGR_RUNTIME_DATA_PTR;

typedef struct
{
  UARTMGR_RUNTIME_DATA runtime_data;
  REDUCTIONDATASTRUCT  reduction_data;
} UARTMGR_PARAM_DATA, *UARTMGR_PARAM_DATA_PTR;

#define UARTMGR_TIMEDELTA_OUTOFSEQUENCE 0xFF


/**********************************
  Uart Status
**********************************/
typedef enum
{
  UARTMGR_STATUS_OK = 0,          // UartMgr Enabled and OK
  UARTMGR_STATUS_FAULTED_PBIT,    // UartMgr Enabled but currently Faulted PBIT
                                  //    UartMgr Processing disabled until power cycle or
  UARTMGR_STATUS_FAULTED_CBIT,    // UartMgr Enabled but currently Faulted CBIT
                                  //   UartMgr Processing active.
  UARTMGR_STATUS_DISABLED_OK,     // UartMgr Not Enabled but PBIT OK
  UARTMGR_STATUS_DISABLED_FAULTED,// UartMgr Not Enabled but PBIT failed
  UARTMGR_STATUS_MAX
} UARTMGR_SYSTEM_STATUS;


typedef struct
{
  UARTMGR_SYSTEM_STATUS status;
  BOOLEAN               bChanActive;    // Ch is active with valid "packet" data
  UINT32                timeChanActive; // Tick Time when Ch became active
  UINT32                dataLossCnt;    // Count of time Ch Sync (ref protocol)
                                        //    has been lost

  BOOLEAN               bChannelStartupTimeOut;  // Flag to indicate channel startup timeout
  BOOLEAN               bChannelTimeOut;         // Flag to indicate channel loss timeout

  UINT32                portByteCnt;    // Count of bytes Rx from physical port
  UINT32                portFramingErrCnt;
  UINT32                portParityErrCnt;
  UINT32                portTxOverFlowErrCnt;
  UINT32                portRxOverFlowErrCnt;
  UARTMGR_PROTOCOLS     protocol;

  BOOLEAN               bRecordingActive;
  UINT32                timeCntReset;

                        // param ->rxTime is in CPU ticks.  TimeCntReset sub from ->rxTime
                        //    to create the "timedelta" for the data to be stored by the
                        //    DataMgr().

} UARTMGR_STATUS, *UARTMGR_STATUS_PTR;


/**********************************
  Uart Data Store Buffer
**********************************/
#define UARTMGR_RAW_RX_BUFFER_SIZE (3840 * 5)  // This is an arbitary number, but for
                                               //   F7X this represents 1 sec of data

#pragma pack(1)
typedef struct
{
  UINT8   timeDelta;
  UINT8   paramId;
  UINT16  paramVal;
  BOOLEAN bValidity;
} UARTMGR_STORE_DATA;

typedef struct
{
  TIMESTAMP  ts;
  UINT8      paramId;
  UINT16     paramVal;
  BOOLEAN    bValidity;
} UARTMGR_STORE_SNAP_DATA;

typedef struct
{
  UINT8   timeDelta;
  UINT16  paramId;
  UINT16  paramVal;
  BOOLEAN bValidity;
} UARTMGR_STORE_DATA_ID;

typedef struct
{
  TIMESTAMP  ts;
  UINT16     paramId;
  UINT16     paramVal;
  BOOLEAN    bValidity;
} UARTMGR_STORE_SNAP_DATA_ID;
#pragma pack()

typedef struct
{
  UINT8   data[UARTMGR_RAW_RX_BUFFER_SIZE];  // 3840 char * 5 (sizeof _STORE_DATA)
  UINT32  readOffset;
  UINT32  writeOffset;
  UINT32  cnt;
  BOOLEAN bOverFlow;
} UARTMGR_STORE_BUFF, *UARTMGR_STORE_BUFF_PTR;




/**********************************
  Uart Misc
**********************************/
#define UARTMGR_WORD_ID_NOT_USED 0xFFFF
#define UARTMGR_WORD_ID_NOT_INIT UARTMGR_WORD_ID_NOT_USED
#define UARTMGR_WORD_INDEX_NOT_FOUND 0xFFFF

typedef enum
{
  UART_DATA_TYPE_DISC = 0,  // Note: See _SensorSetup() func !
  UART_DATA_TYPE_BNR,       //
  UART_DATA_TYPE_MAX        //
} UART_DATA_TYPE;


typedef struct
{
  UINT16         id;             // Parameter Id (specific to Protocol)
                                 //  0 == Not Used
  UINT16         word_size;      // Size of parameter to decode
  UINT16         msb_position;   // Start position of the MSB of the parameter
  UINT32         dataloss_time;  // Data Loss timeout for param (in msec)
  UINT16         nSensor;        // Sensor index requesting this entry
  UINT16         nIndex;         // Index to the UartMgr Param Data struct
  UART_DATA_TYPE type;           // "BNR" or "DISC"
  BOOLEAN         bFailed;       // This word param has failed its data loss timeout
} UARTMGR_WORD_INFO, *UARTMGR_WORD_INFO_PTR;


typedef struct
{
  TASK_INDEX             taskID;    // Task ID for UartMgr_Task from TM
  UINT16                 nChannel;  // UART channel index
  PROTOCOL_HNDL          exec_protocol;
  PROTOCOL_FILE_HDR      get_protocol_fileHdr;
  PROTOCOL_DOWNLOAD_HNDL download_protocol_hndl;
  PROTOCOL_CH_READY_HNDL get_protocol_ready_hndl;
  PROTOCOL_DOWNLOAD_CLR_HNDL download_protocol_clr_hndl; 
} UARTMGR_TASK_PARAMS, *UARTMGR_TASK_PARAMS_PTR;

typedef struct
{
  TASK_INDEX taskID; // Task ID for UartMgr_BITTask from TM
} UARTMGR_BIT_TASK_PARAMS;



/**********************************
  Uart Logs
**********************************/
#pragma pack(1)
typedef struct
{
  RESULT result;
  UINT32 cfg_timeout;
  UINT8  ch;
} UART_TIMEOUT_LOG;

typedef struct
{
  RESULT result; // 0x002A1000, SYS_UART_DATA_LOSS_TIMEOUT
  CHAR   sFailMsg[UART_WORD_TIMEOUT_FAIL_MSG_SIZE];
} SYS_UART_CBIT_FAIL_WORD_TIMEOUT_LOG;

typedef struct
{
  RESULT result; // 0x002B0000, SYS_UART_PBIT_UART_OPEN_FAIL
  UINT16 ch;
} UART_SYS_PBIT_STARTUP_LOG;
#pragma pack()


/**********************************
  Uart File Header
**********************************/
#pragma pack(1)
typedef struct
{
  UINT16 size;     // Size of UART Hdr + Protocol Hdr, including "size" field.
  UINT8  ch;       // Uart Ch
  CHAR   sName[UART_CFG_NAME_SIZE];  // Uart Ch Name
  UARTMGR_PROTOCOLS protocol;  // Uart Ch Protocol selection
} UARTMGR_FILE_HDR;

typedef struct
{
   CHAR sName[UART_CFG_NAME_SIZE];
} UART_SYS_HDR;
#pragma pack()


/**********************************
  Uart Download
**********************************/
typedef enum
{
  UARTMGR_DOWNLOAD_IDLE,
  UARTMGR_DOWNLOAD_GET_REC,
  UARTMGR_DOWNLOAD_REC_STR_INPROGR,
  UARTMGR_DOWNLOAD_MAX
} UARTMGR_DOWNLOAD_STATE;

typedef struct
{
  UARTMGR_DOWNLOAD_STATE state;

  // Between UartMgr and Protocol
  BOOLEAN bStartDownload;
  BOOLEAN bDownloadCompleted;

  BOOLEAN bWriteInProgress;
  BOOLEAN bWriteOk;

  BOOLEAN bNewRec;

  UINT32  *pSize; // Pointer to data size count
  UINT8   *pData; // Pointer to data buffer

  BOOLEAN bHalt;
  // Between UartMgr and Protocol
  // Between UartMgr and DataMgr
  DL_WRITE_STATUS wrStatus;
  // Between UartMgr and DataMgr

} UARTMGR_DOWNLOAD, *UARTMGR_DOWNLOAD_PTR;


/**********************************
  Uart Debug Support
**********************************/
#define UARTMGR_DEBUG_BUFFER_SIZE 1024
typedef struct
{
  BOOLEAN bDebug;    // Enable debug display
  UINT16  ch;        // Channel to debug - default is firt UART (not GSE)
  UINT16  num_bytes; // Number of bytes to display every iteration.

  UINT8   data[UARTMGR_DEBUG_BUFFER_SIZE];
  UINT32  readOffset;
  UINT32  writeOffset;
  UINT32  cnt;

} UARTMGR_DEBUG, *UARTMGR_DEBUG_PTR;

typedef struct
{
    TASK_INDEX      mgrTaskID;
} UARTMGR_DISP_DEBUG_TASK_PARMS;





/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( UART_MGR_BODY )
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

EXPORT void    UartMgr_Initialize         ( void );
EXPORT BOOLEAN UartMgr_SensorTest         ( UINT16 nIndex );
EXPORT BOOLEAN UartMgr_InterfaceValid     ( UINT16 nIndex );
EXPORT FLOAT32 UartMgr_ReadWord           ( UINT16 nIndex, UINT32 *tickCount );
EXPORT UINT16  UartMgr_SensorSetup        ( UINT32 gpA, UINT32 gpB,
                                            UINT16 param, UINT16 nSensor );
EXPORT UINT16  UartMgr_ReadBuffer         ( void *pDest, UINT32 chan, UINT16 nMaxByteSize );
EXPORT UINT16  UartMgr_ReadBufferSnapshot ( void *pDest, UINT32 chan, UINT16 nMaxByteSize,
                                            BOOLEAN bBeginSnapshot );
EXPORT void    UartMgr_ResetBufferTimeCnt ( UINT32 chan );
EXPORT UINT16  UartMgr_GetFileHdr         ( void *pDest, UINT32 chan, UINT16 nMaxByteSize );
EXPORT UINT16  UartMgr_GetSystemHdr       ( void *pDest, UINT16 nMaxByteSize );
EXPORT void    UartMgr_FlushChannels      ( void );
EXPORT UINT8*  UartMgr_DownloadRecord     ( UINT8 PortIndex, DL_STATUS *pStatus, UINT32 *pSize,
                                            DL_WRITE_STATUS **pDL_WrStatus );
EXPORT void    UartMgr_DownloadStop       ( UINT8 PortIndex );
EXPORT void    UartMgr_DownloadClr        ( BOOLEAN Run, INT32 param ); 

#endif // UARTMGR_H


/*****************************************************************************
 *  MODIFICATIONS
 *    $History: UartMgr.h $
 * 
 * *****************  Version 22  *****************
 * User: Peter Lee    Date: 15-02-06   Time: 7:18p
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol.  Additional Update Item #7. 
 * 
 * *****************  Version 21  *****************
 * User: Peter Lee    Date: 15-01-11   Time: 10:20p
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol 
 * 
 * *****************  Version 20  *****************
 * User: John Omalley Date: 11/13/14   Time: 10:38a
 * Updated in $/software/control processor/code/system
 * SCR 1251 - Fixed Data Manager DL Write status location
 * 
 * *****************  Version 19  *****************
 * User: Peter Lee    Date: 14-10-27   Time: 9:10p
 * Updated in $/software/control processor/code/system
 * SCR #1263 ID Param Protocol, Design Review Updates. 
 *
 * *****************  Version 18  *****************
 * User: Peter Lee    Date: 14-10-08   Time: 6:56p
 * Updated in $/software/control processor/code/system
 * SCR #1263 ID Param Protocol Implementation
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
 * User: John Omalley Date: 12-11-28   Time: 2:09p
 * Updated in $/software/control processor/code/system
 * SCR 1197 - Code Review Updates
 *
 * *****************  Version 14  *****************
 * User: John Omalley Date: 12-11-13   Time: 5:46p
 * Updated in $/software/control processor/code/system
 * SCR 1197 - Code Review Updates
 *
 * *****************  Version 13  *****************
 * User: John Omalley Date: 12-11-12   Time: 2:58p
 * Updated in $/software/control processor/code/system
 * SCR 1107, 1191 - Code Review Updates
 *
 * *****************  Version 12  *****************
 * User: Peter Lee    Date: 12-10-27   Time: 5:05p
 * Updated in $/software/control processor/code/system
 * SCR #1191 Returns update time of param
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
 * User: Peter Lee    Date: 9/28/10    Time: 2:14p
 * Updated in $/software/control processor/code/system
 * SCR #892 Code Review Updates
 *
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 7/29/10    Time: 6:51p
 * Updated in $/software/control processor/code/system
 * SCR #754 Flush Uart Interface before normal op
 *
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 7/26/10    Time: 6:20p
 * Updated in $/software/control processor/code/system
 * SCR #743 Update TimeOut Processing for val == 0
 *
 * *****************  Version 6  *****************
 * User: John Omalley Date: 7/20/10    Time: 9:05a
 * Updated in $/software/control processor/code/system
 * SCR 294 - Add system binary header
 *
 * *****************  Version 5  *****************
 * User: Peter Lee    Date: 6/25/10    Time: 6:15p
 * Updated in $/software/control processor/code/system
 * SCR #635
 * 1) Add UartMgr Debug display of raw data
 * 2) Updated Param not init from "0" to "255"
 *
 * *****************  Version 4  *****************
 * User: Peter Lee    Date: 6/21/10    Time: 3:35p
 * Updated in $/software/control processor/code/system
 * SCR #635 Item 19.  Fix bug where on invalid to valid transition, Data
 * Red Init() is called.
 *
 * *****************  Version 3  *****************
 * User: Peter Lee    Date: 6/14/10    Time: 6:31p
 * Updated in $/software/control processor/code/system
 * SCR #635.  Misc updates.  FAULT for PBIT fail, 120 sec timeout and
 * Duplex cfg.
 *
 * *****************  Version 2  *****************
 * User: Peter Lee    Date: 6/10/10    Time: 2:03p
 * Updated in $/software/control processor/code/system
 * SCR #635 Add UART Startup PBIT failure log
 *
 * *****************  Version 1  *****************
 * User: Peter Lee    Date: 5/31/10    Time: 5:55p
 * Created in $/software/control processor/code/system
 * Initial Check In.  Implementation of the F7X and UartMgr Requirements.
 *
 *
 *
 *****************************************************************************/
