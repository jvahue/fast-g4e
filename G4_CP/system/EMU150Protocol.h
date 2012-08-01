#ifndef EMU150_PROTOCOL_H
#define EMU150_PROTOCOL_H
/******************************************************************************
            Copyright (C) 2008-2011 Pratt & Whitney Engine Services, Inc.
                      Altair Engine Diagnostic Solutions
               All Rights Reserved. Proprietary and Confidential.

    File:        EMU150Protocol.h

    Description: Contains data structures related to the EMU150 Serial Protocol 
                 Handler 
    
    VERSION
      $Revision: 9 $  $Date: 9/16/11 1:15p $     

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    


/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"

#include "TaskManager.h"
#include "ClockMgr.h"

#include "UART.h"
#include "UartMgr.h"


/******************************************************************************
                                 Package Defines
******************************************************************************/
#define EMU150_TIMEOUT                CM_DELAY_IN_MSEC(500)   
#define EMU150_TIMEOUT_RECORDING_HDR  CM_DELAY_IN_MSEC(10500)   
#define EMU150_TIMEOUT_EOM            CM_DELAY_IN_MSEC(0) // Process_ACK
#define EMU150_TIMEOUT_AUTO_DETECT    CM_DELAY_IN_MSEC(500) 
#define EMU150_TIMEOUT_ACK            CM_DELAY_IN_MSEC(2100)

#define EMU150_RETRIES_MAX    3
#define EMU150_RESTARTS_MAX   4
#define EMU150_IDLE_RETRIES_MAX  10


#define EMU150_CFG_DEFAULT    EMU150_TIMEOUT,               /* timeout_ms */\
                              EMU150_TIMEOUT_RECORDING_HDR, /* timeout_rec_hdr */\
                              EMU150_TIMEOUT_EOM,           /* timeout_eom */\
                              EMU150_TIMEOUT_AUTO_DETECT,   /* timeout_auto_detect */\
                              EMU150_TIMEOUT_ACK,           /* timeout_ack */\
                              EMU150_RETRIES_MAX,           /* nRetries */\
                              EMU150_RESTARTS_MAX,          /* nRestarts */\
                              EMU150_IDLE_RETRIES_MAX,      /* nIdleRetries */\
                              TRUE                          /* bMarkDownloaded */


/******************************************************************************
                                 Package Typedefs
******************************************************************************/
typedef enum {
  EMU150_STATE_IDLE = 0,
  EMU150_STATE_AUTODETECT,
  EMU150_STATE_SET_BAUD, 
  EMU150_STATE_GET_INSTALL_CFG,
  EMU150_STATE_GET_RECORDING_HDR, 
  EMU150_STATE_GET_RECORDING,
  EMU150_STATE_RECORDING_CONFIGRMATION,
  EMU150_STATE_WAIT_FOR_STORAGE_CONFIRMATION, 
  EMU150_STATE_COMPLETED, 
  EMU150_STATE_MAX
} EMU150_STATES; 


typedef enum {
  EMU150_CMD_STATE_SEND_HS,
  EMU150_CMD_STATE_HS_ACK,
  EMU150_CMD_STATE_REC_ACK,
  EMU150_CMD_STATE_FINAL_ACK,
  EMU150_CMD_STATE_MAX
} EMU150_CMD_STATES; 

typedef enum {
  EMU150_BLK_RECEIVING,
  EMU150_BLK_EOB,
  EMU150_BLK_EOM
} EMU150_BLK_STATES;

typedef struct {
  EMU150_STATES state;
  
  UINT8  port;          // Physical Uart Port 
   
  EMU150_CMD_STATES cmd_state;
  UINT32 cmd_timer;     // Keeps track of the timeout 
  UINT32 cmd_timeout;   // Timeout value for current action 
  
  UINT32 ack_timer;     // TimeOut for exp ACK response.  
                        //    Required for "babbling" transmitter
  
  EMU150_BLK_STATES blk_state; 
  UINT32 blk_number;    // Current block being received 
  UINT32 blk_exp_number;  // Expect number of blocks for this blk retrieval 
                          //   Rx on first blk of rsp to cmd. 
  UINT32 blk_exp_size; 
  UINT32 blk_hdr_size; 
  UINT16 nRetries;      // Retries of each STATE 
  UINT16 nRestarts;     // Retries of entire Download from the top !
  UINT16 nIdleRetries;  // Idle retries when exceeded, perform "restart"
  
  UINT32 nTotalRetries; // Total retries for this current download. 
  
  UINT32 nCksumErr;     // Number of cksum errors for current retrieval req
  
  UINT32 nRecords;      // Total number of records in EMU150
  UINT32 nRecordsNew;   // Number of records new in EMU150
  UINT32 nRecordsRxBad; // Number of records new not received
  
  UINT32 nCurrRecord;   // Current record being retrieved 
  
  UINT32 nTotalBytes;   // Total bytes received during current retrieval
  
  UINT32 nRecordsRec; // Total number of records recorded for current download
  UINT32 nRecordsRecTotalByte;  // Total number of bytes of all recorded records 

  BOOLEAN bAllRecords;     // TRUE retrieve all records, FALSE only new records
  
  UINT16 IndexBPS;         // Index to BPS for current EMU150 setting
  UINT32 UartBPS;          // Current BPS EMU150 setting
  
  UINT32 UartBPS_Desired;  // Desired BPS EMU150 setting
  
  BOOLEAN bDownloadFailed; // Current download success or failed 
  
  BOOLEAN bDownloadInterrupted;  // Download Interrupt 
  
} EMU150_STATUS, *EMU150_STATUS_PTR; 

typedef struct {
  UINT32 timeout;             // General timeout for generic EMU CMD (def 500 ms)
  UINT32 timeout_rec_hdr;     // Timeout for get rec hdr response (def 10,500 ms)
  UINT32 timeout_eom;         // Additional Delay before processing EOM (def 0 ms)
  UINT32 timeout_auto_detect; // Timeout for AutoDetect (def 200 ms)
  UINT32 timeout_ack;         // Timeout to declare ACK not recieved (def 2000 ms)

  UINT16 nRetries;            // General retries for current generic EMU CMD (def 3)
                              //   If retries exceed, current cmd abort and next cmd 
                              //   tried. 
                              //   NOTE: timeout period == "timeout" period above. 

  UINT16 nRestarts;           // #times to start entire Download Process (from AutoDetect) 
                              //    Required if EMU Resets back to 9600 
  UINT16 nIdleRetries;        // #Cmd Retries in a row with no response before "restarting"
                              //    from the top (AutoDetect).  This value should be > nRetries.
                              //    If set == nRetries then will restart if current cmd fails.
                              //    NOTE: timeout period == "timeout" above. 
  
  BOOLEAN bMarkDownloaded;    // True = Mark Retrieved Rec as downloaded
  
  // UINT32 UartBPS_Desired;     // Desired BPS EMU150 setting
} EMU150_CFG, *EMU150_CFG_PTR; 


#define EMU150_APP_DATA_MAX ((1024 * 6) - 64)
#pragma pack(4)
typedef struct 
{
  UINT16 pad;    // Required for calculating long word align chksum of data[] 
  UINT16 nCnt;   // Total number of bytes or /4 for number of long words 
                 //  Note: EMU150 cfg records are expected to be long word aligned. 
  UINT8 data[EMU150_APP_DATA_MAX]; 
  UINT32 cksum;
} EMU150_APP_DATA; 
#pragma pack()

typedef struct 
{
  BOOLEAN *bStartDownload;
  BOOLEAN *bDownloadCompleted;  // Set UartMgr to FALSE. Clr EMU150 to TRUE on completion 
                                //    of Download Attempt. 
  BOOLEAN *bWriteInProgress;
  BOOLEAN *bWriteOk;
  BOOLEAN *bHalt;
  BOOLEAN *bNewRec;
} EMU150_DOWNLOAD_DATA; 

typedef struct 
{
  BOOLEAN bStartDownload;
  BOOLEAN bDownloadCompleted;
  BOOLEAN bWriteInProgress;
  BOOLEAN bWriteOk;
  BOOLEAN bHalt;
  BOOLEAN bNewRec; 
} EMU150_DOWNLOAD_DATA_LC; 


#pragma pack(1)
// NOTE: EMU150_CreateSummaryLog() should be updated for any changes to this 
//       structure below. 
typedef struct 
{
  UINT32 nRecords;    // Total number of records in EMU150 for current req
  UINT32 nRecordsNew; // Number of records new in EMU150 for current req
  UINT32 nRecordsRxBad;   // Number of records new not received for current req
  
  UINT32 nTotalBytes; // Total bytes received during current retrieval 
  
  UINT32 nTotalRetries; // Total retries for this current download. 
  
  UINT32 nRecordsRec; // Total number of records recorded for current download
  UINT32 nRecordsRecTotalByte;  // Total number of bytes of all rec recorded 
  
  UINT32 UartBPS;     // Current BPS EMU150 setting 
  
  UINT32 nCksumErr;   // Total Checksum errors 
  
  UINT16 nRestarts;        // Retries of entire Download from the top ! 
  BOOLEAN bDownloadInterrupted;   // Current Download was interrupted 
  BOOLEAN bDownloadFailed;  // Current download success or failed 
} EMU150_STATUS_LOG, *EMU150_STATUS_LOG_PTR; 
#pragma pack()


/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( EMU150_PROTOCOL_BODY )
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
EXPORT void EMU150Protocol_Initialize( void ); 

EXPORT BOOLEAN  EMU150Protocol_Handler ( UINT8 *data, UINT16 cnt, UINT16 ch, 
                                         void *runtime_data_ptr, void *info_ptr );

EXPORT EMU150_STATUS_PTR EMU150Protocol_GetStatus (void); 

EXPORT UINT16 EMU150Protocol_ReturnFileHdr ( UINT8 *dest, const UINT16 max_size, 
                                             UINT16 ch ); 

EXPORT void EMU150Protocol_SetBaseUartCfg (UINT16 ch, UART_CONFIG UartCfg); 

EXPORT BOOLEAN EMU150_FileInit(void); 

EXPORT void EMU150Protocol_DownloadHndl ( UINT8 port, 
                                          BOOLEAN *bStartDownload, 
                                          BOOLEAN *bDownloadCompleted,
                                          BOOLEAN *bWriteInProgress,
                                          BOOLEAN *bWriteOk, 
                                          BOOLEAN *bHalt,
                                          BOOLEAN *bNewRec,
                                          UINT32  **pSize,
                                          UINT8   **pData ); 

#endif // EMU150_PROTOCOL_H 


/*****************************************************************************
 *  MODIFICATIONS
 *    $History: EMU150Protocol.h $
 * 
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 9/16/11    Time: 1:15p
 * Updated in $/software/control processor/code/system
 * SCR #1059
 * 
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 9/15/11    Time: 7:59p
 * Updated in $/software/control processor/code/system
 * SCR #1059 EMU150 V&V Issues
 * 
 * *****************  Version 7  *****************
 * User: John Omalley Date: 9/15/11    Time: 10:08a
 * Updated in $/software/control processor/code/system
 * SCR 1059 - Remove Debug commands and variables
 * 
 * *****************  Version 6  *****************
 * User: Jeff Vahue   Date: 9/07/11    Time: 2:52p
 * Updated in $/software/control processor/code/system
 * SCR# 1059 - Updated the timeout(s) minimum and default
 * 
 * *****************  Version 5  *****************
 * User: John Omalley Date: 9/07/11    Time: 1:50p
 * Updated in $/software/control processor/code/system
 * SCR 1059 - Updated the timeout(s) minimum, maximum and default
 * 
 * *****************  Version 4  *****************
 * User: Peter Lee    Date: 7/25/11    Time: 5:39p
 * Updated in $/software/control processor/code/system
 * SCR #1037 Increase NVMgr for NV_UART_EMU150 to 6 KB to accomodate the
 * worst case size. 
 * 
 * *****************  Version 3  *****************
 * User: Peter Lee    Date: 7/25/11    Time: 4:39p
 * Updated in $/software/control processor/code/system
 * SCR #1039 Add option to "Not Mark As Downloaded" and several non-logic
 * updates (names, variable, etc).  
 * 
 * *****************  Version 2  *****************
 * User: Peter Lee    Date: 4/21/11    Time: 3:14p
 * Updated in $/software/control processor/code/system
 * SCR #1029 Fix alignment issue with EEPROM app data.  We want long word
 * alignment for checksum calc. 
 * 
 * *****************  Version 1  *****************
 * User: Peter Lee    Date: 4/11/11    Time: 10:07a
 * Created in $/software/control processor/code/system
 * SCR #1029 EMU150 Download.  Initial check in. 
 * 
 *
 *****************************************************************************/
