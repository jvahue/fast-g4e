#ifndef GBS_PROTOCOL_H
#define GBS_PROTOCOL_H

/******************************************************************************
            Copyright (C) 2008-2015 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:        GBSProtocol.h

    Description: Contains data structures related to the GBS Serial Protocol
                 Handler

    Export:      ECCN 9D991

    VERSION
      $Revision: 3 $  $Date: 15-02-04 1:17p $

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
#define GBS_MAX_CH 4  // One for each physical UART channel

#define GBS_MAX_DNLOAD_TYPES  5
#define GBS_DNLOAD_CODE_NOTUSED 255
#define GBS_DNLOAD_CODE_NEW 5
//#define GBS_DNLOAD_CODE_NEW_INDEX 1


#define GBS_IDLE_TIMEOUT      CM_DELAY_IN_MSEC(500)
#define GBS_TIMEOUT_RETRIES   4

#define GBS_RESTART_SETEDU_MODE  3
#define GBS_MULTIPLE_RETRIES 12 // Max of 12
#define GBS_RESTART_DELAY_MS  CM_DELAY_IN_MSEC(10000)
#define GBS_CMD_DELAY_MS      CM_DELAY_IN_MSEC(100)

                            /*0,  1,  2,  3,  4*/ /* dnloadTypes */
#define GBS_CFG_DEFAULT     0x5,GBS_DNLOAD_CODE_NOTUSED,GBS_DNLOAD_CODE_NOTUSED,\
                                GBS_DNLOAD_CODE_NOTUSED,GBS_DNLOAD_CODE_NOTUSED

#define GBS_MULTI_CFG_DEFAULT  FALSE,                /* bMultiplex */\
                               1,                    /* nPort */\
                               GBS_LSS0,             /* discAddr */\
                               FALSE,                /* bKeepAlive */\
                               GBS_IDLE_TIMEOUT,     /* timeOut */\
                               GBS_TIMEOUT_RETRIES,  /* retriesSingle */\
                               GBS_MULTIPLE_RETRIES, /* retriesMulti */\
                               GBS_RESTART_SETEDU_MODE,  /* restarts */\
                               GBS_RESTART_DELAY_MS,     /* restart_delay_ms */\
                               GBS_CMD_DELAY_MS,         /* cmd_delay_ms */\
                               TRUE,                     /* bKeepBadRec */\
                               FALSE,                    /* bBuffRecStore */\
                               1000                      /* cntBuffRecSize */


#define GBS_CFGS_DEFAULT    GBS_CFG_DEFAULT, \
                            GBS_CFG_DEFAULT, \
                            GBS_CFG_DEFAULT, \
                            GBS_CFG_DEFAULT

#define GBS_SIM_PORT_INDEX  10

/******************************************************************************
                                 Package Typedefs
******************************************************************************/

typedef enum {  // Note: if DIO_OUTPUT_LIST changes
  GBS_LSS0, 
  GBS_LSS1,
  GBS_LSS2, 
  GBS_LSS3, 
  GBS_MAX_ENUM
} GBS_LSS_ENUM; 

typedef struct {
  BOOLEAN bMultiplex;   // TRUE = Two EDU multiplex onto same UART channel
  UINT8 nPort;          // Physical UART Port for Multiplex EDU data
  GBS_LSS_ENUM discPort;// Port / Identifier for discrete to switch when
                        //   comm with two EDU(s)
  BOOLEAN bKeepAlive;   // Send Keep Alive Cmd with GMT Time

  UINT32 timeIdleOut;   // TBD add more timeouts
  UINT32 retriesSingle; // Max retries before moving onto next cmd
  UINT32 retriesMulti;  // Max retries before restarting dnload
  UINT32 restarts;      // Max restart before declaring dnload failed
  UINT32 restart_delay_ms;  // Delay between restarts
  UINT32 cmd_delay_ms;  // Cmd to Cmd Delay 
  BOOLEAN bKeepBadRec;  // TRUE keep bad CRC record
  BOOLEAN bBuffRecStore; // Enb the buffering of small records for storage
  UINT32 cntBuffRecSize; // When buffering of rec enb, the size to flush
} GBS_CTL_CFG, *GBS_CTL_CFG_PTR;

typedef struct {
  UINT8 dnloadTypes[GBS_MAX_DNLOAD_TYPES];  // default is Dnload Type = 4
} GBS_CFG, *GBS_CFG_PTR;

typedef GBS_CFG GBS_CFGS[GBS_MAX_CH];

typedef struct {
  UINT16 pktCnt[GBS_MAX_CH];
  UINT16 pktCnt_Multi;
} GBS_APP_DATA;


typedef BOOLEAN  (*GBS_RSP_HNDL) ( UINT8 *pData, UINT16 cnt, UINT32 pStatus );

typedef enum {
  GBS_SETEDU_IDLE_2SEC,
  GBS_SETEDU_LRU_SELECT_ACK,
  GBS_SETEDU_HS_ACK,
  GBS_SETEDU_KEEPALIVE_ACK,
  GBS_SETEDU_MAX
} GBS_STATE_SET_EDU_ENUM;

typedef enum {
  GBS_STATE_CONFIRM_IDLE,
  GBS_STATE_CONFIRM_ACK,
  GBS_STATE_CONFIRM_MAX
} GBS_STATE_CONFIRM_ENUM;

typedef enum {
  GBS_STATE_RECORD_IDLE,
  GBS_STATE_RECORD_CODE,
  GBS_STATE_RECORD_ACK_NACK,
  GBS_STATE_RECORD_MAX
} GBS_STATE_RECORD_ENUM;

/*
CMD, RESP, CFG(Send?), TIMEOUT, RETRY
*/

#define GBS_CMD_SIZE 32
typedef enum {
  GBS_RSP_NONE, // No response expected
  GBS_RSP_ACK,  // Resp is ACK (i.e. cmd(HS) <- rsp(ACK)
  GBS_RSP_BLK,  // Block response to a command
} GBS_RSP_TYPE;

typedef struct {
  // static items. Must match CMD_RSP_SET_EDU[] and
  UINT32 cmd_rsp_state;
  UINT8 cmd[GBS_CMD_SIZE];
  UINT16 cmdSize;
  GBS_RSP_TYPE respType;
  UINT16 nRetries;       // 0 == no retries necessary
  UINT32 nRetryTimeOut;  // timeout in msec
  UINT32 nCmdToCmdDelay; // Cmd Msg to Cmd Msg delay
  // dynamic items
  GBS_RSP_HNDL rspHndl;  // For respType != GBS_RSP_NONE, rsp hndl code
} GBS_CMD_RSP, *GBS_CMD_RSP_PTR;


typedef enum {
  GBS_STATE_IDLE,
  GBS_STATE_SET_EDU_MODE,
  GBS_STATE_DOWNLOAD,
  GBS_STATE_CONFIRM,
  GBS_STATE_COMPLETE,
  GBS_STATE_RESTART_DELAY,
  GBS_STATE_MAX
} GBS_STATE_ENUM;

typedef enum {
  GBS_MULTI_PRIMARY,
  GBS_MULTI_SECONDARY,
  GBS_MULTI_MAX
} GBS_STATE_MULTI_ENUM;

typedef enum {
  GBS_BLK_STATE_GETSIZE,
  GBS_BLK_STATE_DATA,
  GBS_BLK_STATE_SAVING,
  GBS_BLK_STATE_MAX
} GBS_BLK_RX_STATE_ENUM; 

typedef struct {
  UINT16 cntBlkCurr;     // Current Rec #. Note: Rec # in data is master
  UINT16 cntBlkBadInRow; // Current # of bad rec in row
  UINT32 cntBlkBadTotal; // Total Bad Rec encountered for this retrieval 
  UINT32 cntBlkRx;       // # Rec Received
  
  UINT32 currBlkNum;     // Current working blk #
  UINT32 currBlkSize;    // Current working blk size 
  
  UINT32 cntRecOutSeq; 
  UINT32 cntStoreBad; 
  
  GBS_BLK_RX_STATE_ENUM state;
  
  BOOLEAN bFailed;       // Rec Retrieval Failed.  Too many bad Rec in row. 
  
  UINT32 cntBlkBadRx;    // Cnt number of Bad Rx Block (i.e Bad Checksum) not stored
  UINT32 cntBlkBuffering; // Number of Blks buffer for single Store. 
  
} GBS_BLK_DATA, *GBS_BLK_DATA_PTR; 

typedef struct
{
  BOOLEAN *bStartDownload;
  BOOLEAN *bDownloadCompleted;  // Set UartMgr to FALSE. Clr EMU150 to TRUE on completion
                                //    of Download Attempt.
  BOOLEAN *bWriteInProgress;
  BOOLEAN *bWriteOk;
  BOOLEAN *bHalt;
  BOOLEAN *bNewRec;
} GBS_DOWNLOAD_DATA, *GBS_DOWNLOAD_DATA_PTR;

typedef struct
{
  BOOLEAN bStartDownload;
  BOOLEAN bDownloadCompleted;
  BOOLEAN bWriteInProgress;
  BOOLEAN bWriteOk;
  BOOLEAN bHalt;
  BOOLEAN bNewRec;
} GBS_DOWNLOAD_DATA_LC, *GBS_DOWNLOAD_DATA_LC_PTR;

typedef struct {
  UINT16 ch;             // Physical Uart Channel
  
  BOOLEAN multi_ch;      // TRUE if this is the Multiplex Ch
  BOOLEAN spare; 

  GBS_STATE_ENUM state;  // GBS Xfer/Comm State

  GBS_CMD_RSP_PTR cmdRsp_ptr;
  BOOLEAN bCmdRspFailed; // Retries of Current Cmd failed

  BOOLEAN bCompleted;    // Download Request Completed
  BOOLEAN bDownloadInterrupted;  // Download Interrupt
  BOOLEAN bDownloadFailed;       // Download failed

  UINT32 nRetriesCurr; // Runtime Retries

  UINT16 nCmdRetriesCurr;  // Cmd Rsp # of retries runtime
  UINT32 msCmdTimer;       // Cmd Rsp timeout runtime timer (ms)
  UINT32 msCmdDelay;       // Cmd To Cmd delay runtime timer (ms)

  UINT16 cmdRecCodeIndex;  // Index into GBS_CFG.dnloadTypes[index]

  UINT16 cntBlkSizeBad;    // Num of Bad BlkSize Req Record returned from EDU
  UINT16 cntBlkSizeExp;    // On Good BlkSize Req Rec, this is # Blk to expected
  UINT32 cntDnLoadSizeExp; // On Good BlkSize Req Rec, this is Total Byte size expected
  
  UINT16 cntBlkSizeExpDnNew;  // this is # Blk expected for Dnload New Cmd 
  
  UINT16 cntBlkSizeNVM;    // NVM copy of previously retrieved download size 
  
  UINT8 *ptrACK_NAK;       // Ptr to data buff for ACK / NAK response 
  
  GBS_BLK_DATA dataBlkState;  
  
  GBS_DOWNLOAD_DATA_PTR pDownloadData;   
  
  UINT32 lastRxTime_ms;   // Receive time of last Rx Data Byte
 
  UINT16 cntDnloadReq;    // Count # of download req since power up. Multi Req before 
                          //    req initiated is not incl.  
  UINT32 restartDelay;    // Restart Delay Timer 
  
  UINT32 cntBadCRCRow;    // Consecutive Cnt of Bad CRC In Row 
                            
} GBS_STATUS, *GBS_STATUS_PTR;


typedef struct {
  GBS_STATE_MULTI_ENUM state;   // _PRIMARY or _SECONDARY
  BOOLEAN bPrimaryReqPending;   // Data Mgr Primary Ch Req Dnload
  BOOLEAN bSecondaryReqPending; // Data Mgr Secondary Ch Req Dnload

} GBS_MULTI_CTL, *GBS_MULTI_CTL_PTR ;


#pragma pack(1)
// NOTE: GBS_CreateSummaryLog() should be updated for any changes to this
//       structure below.
typedef struct
{
  UINT16 ch;  
  BOOLEAN bCompleted;  // TRUE completed successfully 
  BOOLEAN bDownloadInterrupted;  // Download was interrupted 

  UINT32 nBlkExpected;  // # Blk indicate by EDU (0=INIT)
  UINT32 nBlkSizeTotal; // Total Size of all Blk indicated by EDU (0=INIT)
  
  UINT32 nBlkRx;        // # Blk received by FAST (s/b == nBlkExpected)
  UINT32 nBlkRxErr;     // # Blk not rx from EDU due to problems
  UINT32 nBlkRxErrTotal; // # Blk Rx experiencing problem.  Retry might have solved. 
  UINT32 nBadStore; 
  
  UINT32 nRestarts;    // # times retrieval had to restart from the top
} GBS_STATUS_LOG, *GBS_STATUS_LOG_PTR;
#pragma pack()

#pragma pack(1)
typedef struct 
{
  UINT16  cnt;       // Size of GBS Protocol file header
  BOOLEAN completed; // 1=Retrieval Completed Successfully 
                     // 0=Retrieval Not completed successfully 
  UINT8 reserved[13];// Reserved for future use
} GBS_FILE_HDR_DATA, *GBS_FILE_HDR_DATA_PTR; 
#pragma pack()

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( GBS_PROTOCOL_BODY )
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
EXPORT void GBSProtocol_Initialize( void );

EXPORT BOOLEAN  GBSProtocol_Handler ( UINT8 *data, UINT16 cnt, UINT16 ch,
                                         void *runtime_data_ptr, void *info_ptr );

EXPORT UINT16 GBSProtocol_ReturnFileHdr ( UINT8 *dest, const UINT16 max_size,
                                             UINT16 ch );

EXPORT BOOLEAN GBS_FileInit(void);

EXPORT void GBSProtocol_DownloadHndl ( UINT8 port,
                                       BOOLEAN *bStartDownload,
                                       BOOLEAN *bDownloadCompleted,
                                       BOOLEAN *bWriteInProgress,
                                       BOOLEAN *bWriteOk,
                                       BOOLEAN *bHalt,
                                       BOOLEAN *bNewRec,
                                       UINT32  **pSize,
                                       UINT8   **pData );
                                       
EXPORT UINT16 GBSProtocol_SIM_Port ( void ); 


#endif // GBS_PROTOCOL_H
/*****************************************************************************
 *  MODIFICATIONS
 *    $History: GBSProtocol.h $
 * 
 * *****************  Version 3  *****************
 * User: Peter Lee    Date: 15-02-04   Time: 1:17p
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol.  UINT16 -> BOOLEAN for "completed" field of
 * UART GBS Protocol File Header field. 
 * 
 * *****************  Version 2  *****************
 * User: Peter Lee    Date: 15-01-19   Time: 6:19p
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol Updates 
 * 1) LSS output control
 * 2) KeepAlive Msg
 * 3) Dnload code loop thru
 * 4) Debug Dnload Code
 * 5) Buffer Multi Records before storing 
 * 
 * *****************  Version 1  *****************
 * User: Peter Lee    Date: 15-01-11   Time: 10:21p
 * Created in $/software/control processor/code/system
 * SCR #1255 GBS Protocol 
 *
 *****************************************************************************/

