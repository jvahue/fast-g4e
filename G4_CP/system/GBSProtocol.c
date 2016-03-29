#define GBS_PROTOCOL_BODY
/******************************************************************************
            Copyright (C) 2008-2016 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:        GBSProtocol.c

    Description: Contains all functions and data related to the GBS Protocol
                 Handler

    Export:      ECCN 9D991

    VERSION
      $Revision: 25 $  $Date: 3/26/16 11:40p $

******************************************************************************/

//#define GBS_TIMING_TEST 1
//#define DTU_GBS_SIM 1

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "GBSProtocol.h"
#include "ClockMgr.h"
#include "GSE.h"
#include "NVMgr.h"
#include "Assert.h"
#include "ACS_Interface.h"
#include "ActionManager.h"


/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/

#define GBS_UNIT_APP_DATA  0xFFFF
#define GBS_RAW_BUFF_MAX  (128 * 1024)  // Max Data Record is 64 KB.  Will 2x.
#define GBS_RAW_TX_MAX    (128)         // Max Tx Buffer

#define GBS_BLKREC_CNT_FIELD_POS     0
#define GBS_BLKREC_SIZE_FIELD_POS    4
#define GBS_BLKREC_CHKSUM_FIELD_POS  8
#define GBS_DATAREC_BLK_FIELD_POS    0
#define GBS_DATAREC_SIZE_FIELD_POS   2
#define GBS_DATAREC_CHKSUM_FIELD_POS 8

#define GBS_CMD_RSP_MAX  16 // Max of 16 Cmd Rsp per GBS_STATE
#define GBS_RSP_ACK_SIZE  1
#define GBS_RSP_ACK_DATA  0x06
#define GBS_RSP_NAK_DATA  0x05

#define GBS_HTONL(A)  ((((A) & 0xff000000) >> 24) | \
                       (((A) & 0x00ff0000) >>  8) | \
                       (((A) & 0x0000ff00) <<  8) | \
                       (((A) & 0x000000ff) << 24))
                       
#define GBS_HTONS(A)  ( (((A) & 0xff00) >> 8) | \
                        (((A) & 0x00ff) << 8) )
                       
#define GBS_CKSUM_REC_START      4  // Excludes the U16 blk# and U16 blksize
#define GBS_CKSUM_BLKSIZE_START  0  

#define GBS_CKSUM_REC_TYPE       0
#define GBS_CKSUM_BLKSIZE_TYPE   1


#define GBS_CKSUM_EXCL (2 * sizeof(UINT32))  // GBS cksum excludes the cksum itself and
                                             //   status word (for Rec) or Pad (for BlkSize)

#define GBS_BLK_OH   (1 * sizeof(UINT32)) // OH size of block includes U16 blk# and 
                                          //  U16 blksize blk size if size of payload data !

#define GBS_FLUSH_RXBUFF  \
        m_GBS_RxBuff[ch].cnt = 0;      \
        m_GBS_RxBuff[ch].cksum_pos = 0;\
        m_GBS_RxBuff[ch].cksum = 0


#define GBS_ACTION_SECONDARY_SEL  ACTION_ON
#define GBS_ACTION_PRIMARY_SEL    ACTION_OFF

#define GBS_ACTIONS(a)   ( a & ACTION_ALL )


/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
typedef struct {
  UINT32 cnt;         // Count of bytes in buff[] 
  UINT32 cksum_pos;   // Index into buffer used to support runtime calc of checksum
                      //    Does not incl Hdr Offset. Actual pos is cksum_pos + hdr offset
  UINT32 cksum;       // Current running checksum value 
  UINT8 buff[GBS_RAW_BUFF_MAX];
} GBS_RAW_RX_BUFF, *GBS_RAW_RX_BUFF_PTR;

typedef struct {
  UINT32 cnt;
  UINT8 buff[GBS_RAW_TX_MAX];
} GBS_RAW_TX_BUFF, *GBS_RAW_TX_BUFF_PTR;

typedef struct {
  UINT32 cnt;    
  UINT32 buff_end; // Current end of data 
                       // For efficiency, copy bytes as they are received.  However 
                       //    if cksum fails, then have to move back and 
                       //    start @ buff_end again. 
  UINT32 data_index; // Index into Rx Buff where data is being copied from 
  UINT8 buff[GBS_RAW_BUFF_MAX];
} GBS_RAW_RX_BUFF_MULTI_REC, *GBS_RAW_RX_BUFF_MULTI_REC_PTR;

typedef struct {
  UINT8 cmd;
  UINT8 code;
  UINT8 spare;
  UINT8 dateTime[4]; // Not Used
  UINT8 cycleCnt;    // Not Used
//#ifdef DTU_GBS_SIM  
  // TEST TO WORK WITH DTU SIMULATOR WHERE EXPECT 10 OR 12 BYTES FOR THIS MSG  
  UINT8 pad[2];      
  // TEST TO WORK WITH DTU SIMULATOR WHERE EXPECT 10 OR 12 BYTES FOR THIS MSG 
//#endif 
} GBS_CMD_REQ_BLK, *GBS_CMD_REQ_BLK_PTR;

typedef struct {
  UINT16 numBlks;
  UINT16 pad1;
  UINT32 totalBytes;
  UINT32 chksum;
  UINT32 pad2;
} GBS_BLK_SIZE_REC, *GBS_BLK_SIZE_REC_PTR;


#define GBS_KA_GMT_SIZE  3
#define GBS_KA_DATE_SIZE 3
#define GBS_KA_CODE  0xFE
#define GBS_KA_LRU_SEL 0x9B
#define GBS_KA_DATE_BASE_YR 2000 // Confirmed on 02/12/15 UTFlight 2000EX
#define GBS_KA_CKSUM_EXCL 2

#pragma pack(1)
typedef struct {
  UINT8 selLRU;   // LRU Select is "0x9B"
  UINT8 code;     // Keep alive Code "0xFE"
  UINT8 gmt[GBS_KA_GMT_SIZE];
  UINT8 date[GBS_KA_DATE_SIZE]; 
  UINT8 cksum;  // sum of fields 'code' to end of 'date' inclusive. Excl selLRU
} GBS_KEEPALIVE_MSG, *GBS_KEEPALIVE_MSG_PTR; 
#pragma pack()


typedef struct {
#ifdef DTU_GBS_SIM
  BOOLEAN DTU_GBS_SIM_SimLog;  // TRUE if DTU GBS SIM real log option selected
                                //   The DTU GBS SIM create blk size diff based 
                                //   on option selected.  With Real Log, the
                                //   blk size include CRC (U32) and Status (U32).  
                                //   With Sim Log, blk size excludes CRC and Status.
                                //   Have to acct for difference
#endif        
  UINT16 pad;  
  BOOLEAN dbg_verbosity;        // Enb GBS_Dbg_TxData() and GBS_Dbg_RxData()
} GBS_DEBUG_CTL, *GBS_DEBUG_CTL_PTR ; // Run Time Only

typedef struct {
  UINT8 dnloadCode;             // Download Code for Debugging   
} GBS_DEBUG_DNLOAD, *GBS_DEBUG_DNLOAD_PTR; 


/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static GBS_CFG        m_GBS_Cfg[GBS_MAX_CH];
static GBS_CTL_CFG    m_GBS_Ctl_Cfg;
static GBS_DEBUG_CTL  m_GBS_Debug; 
static GBS_DEBUG_DNLOAD m_GBS_DebugDnload[GBS_MAX_CH]; 

static GBS_STATUS     m_GBS_Status[GBS_MAX_CH];
static GBS_STATUS     m_GBS_Multi_Status;
static GBS_MULTI_CTL  m_GBS_Multi_Ctl;

static GBS_DOWNLOAD_DATA m_GBS_Download_Ptr[GBS_MAX_CH];
static GBS_DOWNLOAD_DATA_LC m_GBS_Download[GBS_MAX_CH];

static GBS_DOWNLOAD_DATA m_GBS_Multi_Download_Ptr;  // Multiplex Chan
static GBS_DOWNLOAD_DATA_LC m_GBS_Multi_Download;   // Multiplex Chan

static GBS_APP_DATA m_GBS_App_Data;

static GBS_RAW_RX_BUFF m_GBS_RxBuff[GBS_MAX_CH]; // Note: Multiplex GBS will
                                                 //       use Physical Ch Port.
                                                 
static GBS_RAW_RX_BUFF_MULTI_REC m_GBS_RxBuffMultiRec[GBS_MAX_CH]; // Use when buff multi Rec
                                                                   //   option set

static GBS_RAW_TX_BUFF m_GBS_TxBuff[GBS_MAX_CH];

static GBS_CMD_RSP cmdRsp_SetEDU[GBS_CMD_RSP_MAX];  // Set EDU Mode will be command
                                                    //   to all chan
static GBS_CMD_RSP cmdRsp_Record[GBS_MAX_CH][GBS_CMD_RSP_MAX];  // Retrieve Rec
static GBS_CMD_REQ_BLK m_CmdReqBlk[GBS_MAX_CH];

static GBS_CMD_RSP cmdRsp_Confirm[GBS_CMD_RSP_MAX];  // Rec Retreived OK.  Confirm EDU

#define GBS_TX_RX_BUFF_MAX 60  // "0x00 ..." or 5 char per bin byte for 10 b max w 10 b OH. 
#define GBS_TX_RX_CHAR_MAX 10  // 10 bytes where each byte -> 6 ASCII char (5 char + 1 space)
static CHAR m_gbs_tx_buff[GBS_TX_RX_BUFF_MAX];
static CHAR m_gbs_rx_buff[GBS_TX_RX_BUFF_MAX];

// Test Timing
#ifdef GBS_TIMING_TEST
  #define GBS_TIMING_BUFF_MAX 200
  UINT32 m_GBSTiming_Buff[GBS_TIMING_BUFF_MAX];
  UINT32 m_GBSTiming_Start;
  UINT32 m_GBSTiming_End;
  UINT16 m_GBSTiming_Index;
#endif
// Test Timing
// m_GBSTiming_Start = TTMR_GetHSTickCount();

//m_GBSTiming_End = TTMR_GetHSTickCount();
//m_GBSTiming_Buff[m_GBSTiming_Index] = (m_GBSTiming_End - m_GBSTiming_Start);
//m_GBSTiming_Index = (++m_GBSTiming_Index) % GBS_TIMING_BUFF_MAX;


/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void GBS_RestoreAppData (void);
static void GBS_CreateSummaryLog ( GBS_STATUS_PTR pStatus, BOOLEAN bSummary );
static GBS_STATUS_PTR GBS_GetStatus (UINT16 index);
static GBS_DEBUG_DNLOAD_PTR GBS_GetDebugDnload (UINT16 i);
static GBS_STATUS_PTR GBS_GetSimStatus (void);
static GBS_MULTI_CTL_PTR GBS_GetCtlStatus (void);

static void GBS_ProcessRxData( GBS_STATUS_PTR pStatus, GBS_DOWNLOAD_DATA_PTR pDownloadData,
                               UINT8 *pData, UINT16 cnt );

static GBS_STATE_ENUM GBS_ProcessStateSetConfirmEDU ( GBS_STATUS_PTR pStatus, UINT8 *pData,
                                                      UINT16 cnt );
static void GBS_ProcessStateSetEDU_Init ( GBS_STATUS_PTR pStatus );
static void GBS_ProcessStateConfirm_Init ( GBS_STATUS_PTR pStatus );
static GBS_STATE_ENUM GBS_ProcessStateRecords ( GBS_STATUS_PTR pStatus, UINT8 *pData,
                                               UINT16 cnt );
static void GBS_ProcessStateRecord_Init ( GBS_STATUS_PTR pStatus, BOOLEAN clearCmdCodeIndex );
static GBS_STATE_ENUM GBS_ProcessStateComplete ( GBS_STATUS_PTR pStatus );

static BOOLEAN GBS_ProcessACK ( UINT8 *pData, UINT16 cnt, UINT32 status_ptr );
static BOOLEAN GBS_ProcessBlkSize ( UINT8 *pData, UINT16 cnt, UINT32 status_ptr  );
static BOOLEAN GBS_ProcessBlkRec ( UINT8 *pData, UINT16 cnt, UINT32 status_ptr);
static BOOLEAN GBS_VerifyChkSum ( UINT16 ch, UINT16 type );

static void GBS_RunningCksum (UINT16 ch, UINT16 type); 
static void GBS_CreateKeepAlive ( GBS_CMD_RSP_PTR cmdRsp_ptr );

static void GBS_ChkMultiInstall ( GBS_STATUS_PTR pStatus ); 

static void GBS_Dbg_TxData (GBS_STATUS_PTR pStatus, UINT16 ch);
static void GBS_Dbg_RxData (GBS_STATUS_PTR pStatus, UINT8 *data, UINT16 cnt);

#include "GBSUserTables.c"

/*****************************************************************************/
/* Constant Data                                                             */
/*****************************************************************************/

const GBS_CMD_RSP CMD_RSP_SET_EDU[] = {
  // Ref CMD_RSP struct
  // cmd_rsp_state, cmd[], cmdSize, respType, nRetries, nRetryTimeOut, nCmdToCmdDelay

  // Wait 2 sec to force EDU back to Normal Mode
  {(UINT32) GBS_SETEDU_IDLE_2SEC, {0}, 0, GBS_RSP_NONE, 0, 2000, 0, NULL },
  // LRU Select Cmd 1, No Rsp, Wait 1000 ms
  {(UINT32) GBS_SETEDU_LRU_SELECT_ACK, {0x9B, 0x01}, 2, GBS_RSP_NONE, 0, 1000, 0,NULL },
  // LRU Select Cmd 2, Ack Rsp, Wait 500 ms, 3 retries
  {(UINT32) GBS_SETEDU_LRU_SELECT_ACK, {0x9B, 0x01}, 2, GBS_RSP_ACK, 3, 500, 0, NULL },
  // HS Cmd 1, ACK Rsp, Wait 500 ms
  // { GBS_SETEDU_HS_ACK, {0x12}, 1, GBS_RSP_ACK, 3, 500, 0 },
  // HS Cmd 2, ACK Rsp, Wait 500 ms
  // { GBS_SETEDU_HS_ACK, {0x12}, 1, GBS_RSP_ACK, 3, 500, 500 },
  // KeepAlive Cmd 1, Wait 1000 ms  NOTE: Optional based on cfg
  {(UINT32) GBS_SETEDU_KEEPALIVE_ACK, {0x9B, 0xFE}, 9, GBS_RSP_NONE, 0, 500, 0, NULL },
  // KeepAlive Cmd 2, Wait 1000 ms  NOTE: Optional based on cfg
  {(UINT32) GBS_SETEDU_KEEPALIVE_ACK, {0x9B, 0xFE}, 9, GBS_RSP_NONE, 0, 500, 0, NULL },
  // KeepAlive Cmd 3, Wait 1000 ms  NOTE: Optional based on cfg
  {(UINT32) GBS_SETEDU_KEEPALIVE_ACK, {0x9B, 0xFE}, 9, GBS_RSP_NONE, 0, 500, 0, NULL },
  // HS Cmd 3, ACK Rsp, Wait 500 ms
  {(UINT32) GBS_SETEDU_HS_ACK, {0x12}, 1, GBS_RSP_ACK, 3, 500, 0, NULL },
  // End of Cmd Delimiter
  {(UINT32) GBS_SETEDU_MAX, {0}, 0, GBS_RSP_NONE, 0, 0, 0, NULL }
};

const GBS_CMD_RSP CMD_RSP_CONFIRM[] = {
  // Ref CMD_RSP struct
  // cmd_rsp_state, cmd[], cmdSize, respType, nRetries, nRetryTimeOut, nCmdToCmdDelay

  // Wait 0.25 sec
  { (UINT32) GBS_STATE_CONFIRM_IDLE, {0}, 0, GBS_RSP_NONE, 0, 100, 0, NULL },
  // ACK 1 to confirm new Rec Downloaded
  { (UINT32) GBS_STATE_CONFIRM_ACK, {0x06}, 1, GBS_RSP_NONE, 0, 250, 0, NULL },
  // ACK 2 to confirm new Rec Downloaded
  { (UINT32) GBS_STATE_CONFIRM_ACK, {0x06}, 1, GBS_RSP_NONE, 0, 250, 0, NULL },
  // End of Cmd Delimiter
  { (UINT32) GBS_STATE_CONFIRM_MAX, {0}, 0, GBS_RSP_NONE, 0, 0, 0, NULL }
};


#define GBS_STATE_RECORD_CODE_INDEX    1 // Must match CMD_RSP_RECORD[] listing
#define GBS_STATE_RECORD_ACK_NAK_INDEX 2 // Must match CMD_RSP_RECORD[] listing

const GBS_CMD_RSP CMD_RSP_RECORD[] = {
  // Ref CMD_RSP struct
  // cmd_rsp_state, cmd[], cmdSize, respType, nRetries, nRetryTimeOut, nCmdToCmdDelay

  // Wait 0.25 sec
  { (UINT32) GBS_STATE_RECORD_IDLE, {0}, 0, GBS_RSP_NONE, 0, 500, 0, NULL },
  // Record Request Code - NOTE: logic overrides the cmd[] buff array later. 
  { (UINT32) GBS_STATE_RECORD_CODE, {0x5E, 0x00}, 8, GBS_RSP_BLK, 20, 1000, 0, NULL },
  // NACK or ACK Record - NOTE: the nRetries and nRetryTimeOut used differently here.
  //   If after this nRetryTimeOut no Rx data is received, then ACK/NAK will be sent
  //   again. Note for long Rx Data Packet, this might take 17 sec (16 bit size @ 38,400,
  //   but to accomodate, retries will not be sent, rather after the 20 retries * 1000 sec
  //   = 20 sec will timeout and indicate cmd failed.
  { (UINT32) GBS_STATE_RECORD_ACK_NACK, {0x00}, 1, GBS_RSP_BLK, 20, 1000, 100, NULL },
  // End of Cmd Delimiter
  { (UINT32) GBS_STATE_RECORD_MAX, {0}, 0, GBS_RSP_NONE, 0, 0, 0, NULL }
};

// Ordering must match GBS_STATE_SET_EDU_ENUM
const CHAR *CMD_RSP_SETEDU_STR[] = {
  "IDLE_2SEC",    // GBS_SETEDU_IDLE_2SEC
  "LRU_SEL",      // GBS_SETEDU_LRU_SELECT_ACK
  "HS",           // GBS_SETEDU_HS_ACK
  "KEEPALIVE",    // GBS_SETEDU_KEEPALIVE_ACK
  NULL            // GBS_SETEDU_MAX
};

// Ordering must match GBS_STATE_RECORD_ENUM
const CHAR *CMD_RSP_RECORD_STR[] = {
  "IDLE_SEC",     // GBS_STATE_RECORD_IDLE
  "GET_BLK_REQ",  // GBS_STATE_RECORD_CODE
  "GET_REC",      // GBS_STATE_RECORD_ACK_NACK
  NULL            // GBS_STATE_RECORD_MAX
};

// Ordering must match GBS_STATE_CONFIRM_ENUM
const CHAR *CMD_RSP_CONFIRM_STR[] = {
  "IDLE_SEC",     // GBS_STATE_CONFIRM_IDLE
  "SEND_ACK",     // GBS_STATE_CONFIRM_ACK
  NULL            // GBS_STATE_CONFIRM_MAX
};


/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:    GBSProtocol_Initialize
 *
 * Description: Initializes the GBS Protocol functionality
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:
 *  - Initialize all local variables
 *  - Initialize GSE User Commands
 *  - Restore NVM data
 *
 *****************************************************************************/
void GBSProtocol_Initialize ( void )
{
  UINT16 i,j;
  GBS_CMD_RSP_PTR cmdRsp_ptr;

  memset (m_GBS_Status, 0, sizeof(m_GBS_Status) );
  memset (m_GBS_Download_Ptr, 0, sizeof(m_GBS_Download_Ptr) );
  memset (m_GBS_Download, 0, sizeof(m_GBS_Download) );
  memset (&m_GBS_Multi_Download_Ptr, 0, sizeof(m_GBS_Multi_Download_Ptr) );
  memset (&m_GBS_Multi_Download, 0, sizeof(m_GBS_Multi_Download) );
  memset (&m_GBS_Multi_Status, 0, sizeof(m_GBS_Multi_Status) );
  memset (&m_GBS_Multi_Ctl, 0, sizeof(m_GBS_Multi_Ctl) );

  User_AddRootCmd(&gbsProtocolRootTblPtr);
  User_AddRootCmd(&gbsCtlProtocolRootTblPtr);
  User_AddRootCmd(&gbsSimProtocolRootTblPtr);

  // Restore App Data
  GBS_RestoreAppData();
  
  // Retrieve GBS Cfg
  memcpy(m_GBS_Cfg, CfgMgr_RuntimeConfigPtr()->GBSConfigs, sizeof(m_GBS_Cfg));
  memcpy(&m_GBS_Ctl_Cfg, &(CfgMgr_RuntimeConfigPtr()->GBSCtlConfig),
         sizeof(m_GBS_Ctl_Cfg));

  for (i=0;i<GBS_MAX_CH;i++)
  {
    m_GBS_Download[i].bWriteInProgress = TRUE;

    m_GBS_Download_Ptr[i].bStartDownload = &m_GBS_Download[i].bStartDownload;
    m_GBS_Download_Ptr[i].bDownloadCompleted = &m_GBS_Download[i].bDownloadCompleted;
    m_GBS_Download_Ptr[i].bWriteInProgress = &m_GBS_Download[i].bWriteInProgress;
    m_GBS_Download_Ptr[i].bWriteOk = &m_GBS_Download[i].bWriteOk;
    m_GBS_Download_Ptr[i].bHalt = &m_GBS_Download[i].bHalt;
    m_GBS_Download_Ptr[i].bNewRec = &m_GBS_Download[i].bNewRec;

    m_GBS_Status[i].ch = i;
    m_GBS_Status[i].pDownloadData = &m_GBS_Download_Ptr[i]; 
    m_GBS_RxBuff[i].cnt = 0;
    m_GBS_TxBuff[i].cnt = 0;
    m_GBS_Status[i].cntBlkSizeNVM = m_GBS_App_Data.pktCnt[i];
    m_GBS_Status[i].relayCksumNVM = m_GBS_App_Data.relayCksum[i]; 
    
    m_GBS_DebugDnload[i].dnloadCode = m_GBS_Cfg[i].dnloadTypes[0];
  }

  m_GBS_Multi_Download.bWriteInProgress = TRUE;

  m_GBS_Multi_Download_Ptr.bStartDownload = &m_GBS_Multi_Download.bStartDownload;
  m_GBS_Multi_Download_Ptr.bDownloadCompleted = &m_GBS_Multi_Download.bDownloadCompleted;
  m_GBS_Multi_Download_Ptr.bWriteInProgress = &m_GBS_Multi_Download.bWriteInProgress;
  m_GBS_Multi_Download_Ptr.bWriteOk = &m_GBS_Multi_Download.bWriteOk;
  m_GBS_Multi_Download_Ptr.bHalt = &m_GBS_Multi_Download.bHalt;
  m_GBS_Multi_Download_Ptr.bNewRec = &m_GBS_Multi_Download.bNewRec;
  
  m_GBS_Multi_Status.ch = (m_GBS_Ctl_Cfg.bMultiplex == TRUE) ? m_GBS_Ctl_Cfg.nPort : 
                                                                 GBS_MAX_CH;
  m_GBS_Multi_Status.pDownloadData = &m_GBS_Multi_Download_Ptr;
  m_GBS_Multi_Status.cntBlkSizeNVM = m_GBS_App_Data.pktCnt_Multi;
  m_GBS_Multi_Status.relayCksumNVM = m_GBS_App_Data.relayCksum_Multi;
  
  // Initialize Ptr for quicker runtime access
  for (i=0;i<GBS_MAX_CH;i++)
  {
    m_GBS_RxBuff[i].cnt = 0;
    m_GBS_TxBuff[i].cnt = 0;
  }

  // Initialize Cmd Rsp Data
  cmdRsp_ptr = (GBS_CMD_RSP_PTR) &CMD_RSP_SET_EDU[0];
  j=0; 
  for (i=0;i<GBS_CMD_RSP_MAX;i++)
  {
    // Don't set KeepAlive msg if not cfg
    if ( (cmdRsp_ptr->cmd_rsp_state != (UINT32) GBS_SETEDU_KEEPALIVE_ACK) ||
         ((cmdRsp_ptr->cmd_rsp_state == (UINT32) GBS_SETEDU_KEEPALIVE_ACK) &&
         (m_GBS_Ctl_Cfg.bKeepAlive == TRUE)) )
    {
      cmdRsp_SetEDU[j] = *cmdRsp_ptr;
      cmdRsp_SetEDU[j].rspHndl = (cmdRsp_SetEDU[j].respType == GBS_RSP_ACK) ?
                                 (GBS_RSP_HNDL) GBS_ProcessACK : (GBS_RSP_HNDL) NULL;
      if (cmdRsp_ptr->cmd_rsp_state == (UINT32) GBS_SETEDU_MAX) {
        break;  // End of cmd rsp list.  Exit.
      }
      j++; 
    }
    cmdRsp_ptr++;
  }
  cmdRsp_ptr = (GBS_CMD_RSP_PTR) &CMD_RSP_CONFIRM[0];
  for (i=0;i<GBS_CMD_RSP_MAX;i++)
  {
    cmdRsp_Confirm[i] = *cmdRsp_ptr;
    cmdRsp_Confirm[i].rspHndl = (GBS_RSP_HNDL) NULL;
    if (cmdRsp_ptr->cmd_rsp_state == (UINT32) GBS_STATE_CONFIRM_MAX) {
      break;  // End of cmd rsp list.  Exit.
    }
    cmdRsp_ptr++;
  }
  // Initialize cmdRsp_Record[][]
  memset ( (UINT8 *) &cmdRsp_Record[0][0], 0, sizeof(cmdRsp_Record) );
  for (j=0;j<GBS_MAX_CH;j++) {
    cmdRsp_ptr = (GBS_CMD_RSP_PTR) &CMD_RSP_RECORD[0];
    for (i=0;i<GBS_CMD_RSP_MAX;i++)
    {
      cmdRsp_Record[j][i] = *cmdRsp_ptr;
      cmdRsp_Record[j][i].rspHndl = (GBS_RSP_HNDL) NULL;
      if (cmdRsp_ptr->cmd_rsp_state == (UINT32) GBS_STATE_RECORD_MAX) {
        break;  // End of cmd rsp list.  Exit.
      }
      cmdRsp_ptr++;
    }
    cmdRsp_Record[j][GBS_STATE_RECORD_CODE_INDEX].rspHndl =
        (GBS_RSP_HNDL) GBS_ProcessBlkSize;
    cmdRsp_Record[j][GBS_STATE_RECORD_ACK_NAK_INDEX].rspHndl =
        (GBS_RSP_HNDL) GBS_ProcessBlkRec;
    cmdRsp_Record[j][GBS_STATE_RECORD_ACK_NAK_INDEX].nCmdToCmdDelay =
        m_GBS_Ctl_Cfg.cmd_delay_ms;  // Get Setting from Cfg 
    m_GBS_Status[j].ptrACK_NAK = (UINT8 *) 
        &cmdRsp_Record[j][GBS_STATE_RECORD_ACK_NAK_INDEX].cmd[0]; 

    cmdRsp_Record[j][GBS_STATE_RECORD_ACK_NAK_INDEX].nRetries =
        m_GBS_Ctl_Cfg.rec_retries_cnt;  // Get Setting from Cfg 
    cmdRsp_Record[j][GBS_STATE_RECORD_ACK_NAK_INDEX].nRetryTimeOut =
        m_GBS_Ctl_Cfg.rec_retries_timeout_ms;  // Get Setting from Cfg 
    cmdRsp_Record[j][GBS_STATE_RECORD_CODE_INDEX].nRetries =
        m_GBS_Ctl_Cfg.blk_req_retries_cnt;  // Get Setting from Cfg 
    cmdRsp_Record[j][GBS_STATE_RECORD_CODE_INDEX].nRetryTimeOut =
      m_GBS_Ctl_Cfg.blk_req_timeout_ms;  // Get Setting from Cfg 
  }

  for (i=0;i<GBS_MAX_CH;i++)  // Initialize each ch cmd-rsp set to _SET_EDU
  {
    m_GBS_Status[i].cmdRsp_ptr = (GBS_CMD_RSP_PTR) &cmdRsp_SetEDU[0];
  }
  m_GBS_Multi_Status.cmdRsp_ptr = (GBS_CMD_RSP_PTR) &cmdRsp_SetEDU[0];
  
  m_GBS_Multi_Status.ptrACK_NAK = (UINT8 *) 
       &cmdRsp_Record[m_GBS_Ctl_Cfg.nPort][GBS_STATE_RECORD_ACK_NAK_INDEX].cmd[0]; 
       
  m_GBS_Multi_Ctl.nReqNum = ACTION_NO_REQ; 
  
  m_GBS_Debug.dbg_verbosity = FALSE; 
#ifdef DTU_GBS_SIM
  /*vcast_dont_instrument_start*/
  m_GBS_Debug.DTU_GBS_SIM_SimLog = FALSE; 
  /*vcast_dont_instrument_end*/
#endif
    
#ifdef GBS_TIMING_TEST
  /*vcast_dont_instrument_start*/
  for (i=0;i<GBS_TIMING_BUFF_MAX;i++)
  {
    m_GBSTiming_Buff[i] = 0;
  }
  m_GBSTiming_Index = 0;
  m_GBSTiming_Start = 0;
  m_GBSTiming_End = 0;
  /*vcast_dont_instrument_end*/
#endif

}


/******************************************************************************
 * Function:    GBSProtocol_Handler
 *
 * Description: Initializes the GBS Protocol functionality
 *
 * Parameters:  data - ptr to received data to be processed
 *              cnt - count of received data
 *              ch - uart channel to be processed
 *              runtime_data_ptr - not used
 *              info_ptr - not used
 *
 * Returns:    TRUE always
 *
 * Notes:
 *  - Protocol Handler called from Uart Mgr Task for each channel
 *  - Manage Multiplex (two physical chan) on single physical channel
 *  - If "Halt" requested and retrieval in progress, terminate
 *  - Call Rx Processing for any received data
 *  - Transmit data to the channel 
 *
 *****************************************************************************/
BOOLEAN GBSProtocol_Handler ( UINT8 *data, UINT16 cnt, UINT16 ch,
                              void *runtime_data_ptr, void *info_ptr )
{
  RESULT result;
  GBS_STATUS_PTR pStatus;
  GBS_DOWNLOAD_DATA_PTR pDownloadData;
  UINT16 sent_cnt;
  UINT32 action; 


  // Check if multiplex is cfg.  Set pointers appropriately
  if ( (m_GBS_Ctl_Cfg.bMultiplex == TRUE) && (ch == m_GBS_Ctl_Cfg.nPort) &&
       (m_GBS_Multi_Ctl.state == GBS_MULTI_SECONDARY) )
  {
    pStatus = &m_GBS_Multi_Status;
    pDownloadData = &m_GBS_Multi_Download_Ptr;
    pStatus->multi_ch = TRUE; 
    action = (UINT32) GBS_ACTION_SECONDARY_SEL;
  }
  else
  {
    pStatus = &m_GBS_Status[ch];
    pDownloadData = &m_GBS_Download_Ptr[ch];
    pStatus->multi_ch = FALSE; 
    action = (UINT32) GBS_ACTION_PRIMARY_SEL; 
  }
  
  if ((m_GBS_Ctl_Cfg.bMultiplex == TRUE) && (ch == m_GBS_Ctl_Cfg.nPort))
  {
    m_GBS_Multi_Ctl.nReqNum = ActionRequest ( m_GBS_Multi_Ctl.nReqNum, 
                                              GBS_ACTIONS(m_GBS_Ctl_Cfg.discAction), 
                                              (ACTION_TYPE) action, FALSE, FALSE ); 
  }

  // If Halt Rx, "reset"
  if ( *pDownloadData->bHalt == TRUE )
  {
    // Did we get interrupted ?
    if ( pStatus->state != GBS_STATE_IDLE ) {
      pStatus->bDownloadInterrupted = TRUE;
      pStatus->state = GBS_STATE_IDLE;
      *pDownloadData->bHalt = FALSE;
      
      // Output msg
      GSE_DebugStr(NORMAL,TRUE,"\r\nGBS Protocol: Download Halt Encountered ! (Ch=%d) \r\n",
                   (pStatus->multi_ch) ? GBS_SIM_PORT_INDEX : pStatus->ch);
      
      GBS_CreateSummaryLog(pStatus, TRUE); // Create statistics summary log
    }
    else { // State is ILDE, but for Multiplex could be Pending. Have to dsb it. 
      if ((m_GBS_Ctl_Cfg.bMultiplex == TRUE) && (pStatus->ch == m_GBS_Ctl_Cfg.nPort))
      { // NOTE: only 1 multiplex channel can be pending
        if ( ((m_GBS_Multi_Ctl.state == GBS_MULTI_PRIMARY) && 
             (m_GBS_Multi_Ctl.bPrimaryReqPending == TRUE)) ||
             ((m_GBS_Multi_Ctl.state == GBS_MULTI_SECONDARY) && 
             (m_GBS_Multi_Ctl.bSecondaryReqPending == TRUE)) )
        {
          pStatus->cntDnloadReq++; 
          pStatus->bDownloadInterrupted = TRUE; 
          pStatus->bCompleted = FALSE; 
          pStatus->bRelayStuck = FALSE; 
          //m_GBS_Multi_Ctl.bSecondaryReqPending = FALSE; 
          GBS_ProcessStateRecord_Init(pStatus, TRUE);
          GSE_DebugStr(NORMAL,TRUE,"GBS Protocol: Download Start Request (Ch=%d)\r\n", 
                     (pStatus->multi_ch) ? GBS_SIM_PORT_INDEX : pStatus->ch);  
          pStatus->state = GBS_STATE_SET_EDU_MODE; // Temp set state to != GBS_STATE_IDLE                     
        }
      }
    }
    
    // If multiplex cfg, and other ch is Pending then switch over to allow it to 'stop'.  
    // Note: Normally both Primary and Secondary will be interrupted
    if ( (m_GBS_Ctl_Cfg.bMultiplex == TRUE) && (pStatus->ch == m_GBS_Ctl_Cfg.nPort) )
    { // If Multiplex is set, check and allow other device to be retrieved 
      // clear Multi Flags
      m_GBS_Multi_Ctl.bPrimaryReqPending = (m_GBS_Multi_Ctl.state == GBS_MULTI_PRIMARY) ?
                                           FALSE : m_GBS_Multi_Ctl.bPrimaryReqPending;
                                           
      m_GBS_Multi_Ctl.bSecondaryReqPending = (m_GBS_Multi_Ctl.state == GBS_MULTI_SECONDARY) ?
                                           FALSE : m_GBS_Multi_Ctl.bSecondaryReqPending;

      if ( m_GBS_Multi_Ctl.state == GBS_MULTI_PRIMARY ) {
        m_GBS_Multi_Ctl.state = (m_GBS_Multi_Ctl.bSecondaryReqPending == TRUE) ?
                                GBS_MULTI_SECONDARY : GBS_MULTI_PRIMARY; 
      }
      else {
        m_GBS_Multi_Ctl.state = (m_GBS_Multi_Ctl.bPrimaryReqPending == TRUE ) ? 
                                GBS_MULTI_PRIMARY : GBS_MULTI_SECONDARY;       
      }
      // Set Multiplex Mode to PRIMARY if no one is pending.  Allow Relay to be set to 
      //   ACTION_OFF (assoc with PRIMARY) so that Relay can be "de-energized". 
      m_GBS_Multi_Ctl.state =  ( (m_GBS_Multi_Ctl.bSecondaryReqPending == FALSE) && 
                                 (m_GBS_Multi_Ctl.bPrimaryReqPending == FALSE) ) ? 
                                 GBS_MULTI_PRIMARY :  m_GBS_Multi_Ctl.state; 
      
    } // end if (m_GBS_Multi_Cfg.bMultiplex == TRUE) && (pStatus->ch == m_GBS_Multi_Cfg.nPort)
  }
  else 
  {
    if (( m_GBS_Debug.dbg_verbosity == TRUE ) && ( pStatus->state != GBS_STATE_IDLE )) {
      GBS_Dbg_RxData( pStatus, data, cnt ); 
    }
    GBS_ProcessRxData( pStatus, pDownloadData, data, cnt);
  }

  // Transmit any queued bytes.  Wait till Cmd to Cmd Delay timer has expired.
  if ( (m_GBS_TxBuff[ch].cnt > 0) && (pStatus->msCmdDelay <= CM_GetTickCount()) )
  {
    result = UART_Transmit ( (UINT8) ch, (const INT8*) &m_GBS_TxBuff[ch].buff[0], 
                             (UINT16) m_GBS_TxBuff[ch].cnt, &sent_cnt);
    
    if ( m_GBS_Debug.dbg_verbosity == TRUE ) {
      GBS_Dbg_TxData(pStatus, ch); 
    }
    
    if (result == DRV_OK)
    {
      m_GBS_TxBuff[ch].cnt = 0;
    }
  }

  return (TRUE);
}


/******************************************************************************
 * Function:    GBSProtocol_ReturnFileHdr()
 *
 * Description: Function to return the GBS file hdr structure
 *
 * Parameters:  dest - ptr to destination bufffer
 *              max_size - max size of dest buffer
 *              ch - uart channel requested
 *
 * Returns:     cnt - byte count of data returned
 *
 * Notes:       None
 *
 *****************************************************************************/
UINT16 GBSProtocol_ReturnFileHdr ( UINT8 *dest, const UINT16 max_size, UINT16 ch )
{
  GBS_FILE_HDR_DATA data; 
  GBS_STATUS_PTR pStatus;
  
  
  pStatus = (ch != GBS_SIM_PORT_INDEX) ?  &m_GBS_Status[ch] : 
                                          &m_GBS_Multi_Status; 
  memset ( (UINT8 *) &data, 0, sizeof(data) );
  
  data.size = (max_size < sizeof(data)) ? max_size : sizeof(data); 
  data.completed = pStatus->bCompleted; 
  
  memcpy (dest, (UINT8 *) &data, data.size); 

  return (data.size);
}


/******************************************************************************
 * Function:    GBSProtocol_DownloadHndl()
 *
 * Description: Initializes / synchronizes local control and flags to the calling
 *              App
 *
 * Parameters:  port - Interface port to download
*               bStartDownload - ptr to flag to begin download process
 *              bDownloadCompleted - ptr to flag to indicate download completed
 *              bWriteInProgress - ptr to flag to indicate FLASH write in progress
 *              bWriteOk - ptr to flag to indicate FLASH write successful
 *              bHalt - ptr to flag to HALT downloading
 *              bNewRec - ptr to flag to indicate new rec received
 *              *pSize - ptr to ptr where size var of the rx data is located
 *              *pData - ptr to ptr where rx data is located
 *
 * Returns:     none
 *
 * Notes:
 *  - Differentiate between physical ch and multiplex channel and set 
 *    appropriate vars
 *  - For multiplex channel, if current request is not the active ch
 *    (PRIMARY/SECONDARY) set it as PENDING
 *  - For GBS data, the blk hdr (blk # and size) are to be stripped to 
 *    create BRC file
 *
 *****************************************************************************/
void GBSProtocol_DownloadHndl ( UINT8 port,
                                BOOLEAN *bStartDownload,
                                BOOLEAN *bDownloadCompleted,
                                BOOLEAN *bWriteInProgress,
                                BOOLEAN *bWriteOk,
                                BOOLEAN *bHalt,
                                BOOLEAN *bNewRec,
                                UINT32  **pSize,
                                UINT8   **pData )
{
  UINT16 nPort;


  // Set pointers for correct port channel and port type
  if (port != GBS_SIM_PORT_INDEX)
  {
    m_GBS_Download_Ptr[port].bStartDownload = bStartDownload;
    m_GBS_Download_Ptr[port].bDownloadCompleted = bDownloadCompleted;
    m_GBS_Download_Ptr[port].bWriteInProgress = bWriteInProgress;
    m_GBS_Download_Ptr[port].bWriteOk = bWriteOk; 
    m_GBS_Download_Ptr[port].bHalt = bHalt;
    m_GBS_Download_Ptr[port].bNewRec = bNewRec;
  }
  else
  {
    m_GBS_Multi_Download_Ptr.bStartDownload = bStartDownload;
    m_GBS_Multi_Download_Ptr.bDownloadCompleted = bDownloadCompleted;
    m_GBS_Multi_Download_Ptr.bWriteInProgress = bWriteInProgress;
    m_GBS_Multi_Download_Ptr.bWriteOk = bWriteOk; 
    m_GBS_Multi_Download_Ptr.bHalt = bHalt;
    m_GBS_Multi_Download_Ptr.bNewRec = bNewRec;
  }

  nPort = (GBS_SIM_PORT_INDEX != port) ? port : m_GBS_Ctl_Cfg.nPort; 

  if ( (m_GBS_Ctl_Cfg.bMultiplex == TRUE) && (m_GBS_Ctl_Cfg.nPort == port) )
  {
    // If this request is for Primary, allow primary to be set if Secondary not
    //  in progress. If Secondary in progress, then set Primary Pending.
    if ( ( *m_GBS_Multi_Download_Ptr.bStartDownload != TRUE ) &&
         ( m_GBS_Multi_Status.state == GBS_STATE_IDLE ) )
    {
      m_GBS_Multi_Ctl.state = GBS_MULTI_PRIMARY; // Set PRIMARY 
    }
    else
    {
      m_GBS_Multi_Ctl.bPrimaryReqPending = TRUE; // Pend PRIMARY
    }
    // Clear the Multi Port Switch Stuck check vars
    memset ( (void *) &m_GBS_Multi_Ctl.chkSwitch[GBS_MULTI_PRIMARY], 0, 
             sizeof(GBS_MULTI_CHK) ); 
    m_GBS_Multi_Ctl.chkSwitch[GBS_MULTI_PRIMARY].nvm_cksum =
             m_GBS_Status[nPort].relayCksumNVM; 
  } // end if ( (m_GBS_Multi_Cfg.bMultiplex == TRUE) && (m_GBS_Multi_Cfg.nPort == port) )
  else if ((m_GBS_Ctl_Cfg.bMultiplex == TRUE) && (GBS_SIM_PORT_INDEX == port))
  {
    // If this request is for Secondary, allow secondary to be set if Primary not
    //  in progress.  If Primary in progress, then set Secondary Pending
    if ( ( *m_GBS_Download_Ptr[nPort].bStartDownload != TRUE ) &&
         ( m_GBS_Status[nPort].state == GBS_STATE_IDLE ) )
    {
      m_GBS_Multi_Ctl.state = GBS_MULTI_SECONDARY; // Set Secondary
    }
    else
    {
      m_GBS_Multi_Ctl.bSecondaryReqPending = TRUE; // Pend Secondary
    }  
    // Clear the Multi Port Switch Stuck check vars
    memset ( (void *) &m_GBS_Multi_Ctl.chkSwitch[GBS_MULTI_SECONDARY], 0, 
             sizeof(GBS_MULTI_CHK) ); 
    m_GBS_Multi_Ctl.chkSwitch[GBS_MULTI_SECONDARY].nvm_cksum =
             m_GBS_Multi_Status.relayCksumNVM; 
  }

  if (m_GBS_Ctl_Cfg.bBuffRecStore == FALSE ) 
  {
    *pSize = &m_GBS_RxBuff[nPort].cnt;
    // Move pass U16 blksize and U16 blk#
    *pData = &m_GBS_RxBuff[nPort].buff[0] + GBS_CKSUM_REC_START;
  }
  else 
  {
    *pSize = &m_GBS_RxBuffMultiRec[nPort].cnt;
    // U16 blksize and U16 blk# removed from pkts when in _RxBuffMultiRec buff
    *pData = &m_GBS_RxBuffMultiRec[nPort].buff[0];
  }
  
}


/******************************************************************************
 * Function:    GBS_FileInit
 *
 * Description: Clears the EEPROM storage location of NVM data 
 *
 * Parameters:  None
 *
 * Returns:     TRUE always
 *
 * Notes:       Standard Initialization format to be compatible with
 *              NVMgr Interface.
 *
 *****************************************************************************/
BOOLEAN GBS_FileInit(void)
{
  // Init App data
  // Note: NV_Mgr will record log if data is corrupt
  memset ( (void *) &m_GBS_App_Data, 0, sizeof(m_GBS_App_Data) ); 

  // Update App data
  NV_Write(NV_GBS_DATA, 0, (void *) &m_GBS_App_Data, sizeof(m_GBS_App_Data));

  return TRUE;
}


/******************************************************************************
 * Function:    GBSProtocol_DownloadClrHndl
 *
 * Description: Clears the NVM Rec Count to force Download of record on next 
 *              Download Request
 *
 * Parameters:  Run - TRUE to clear NVM Rec Counts
 *              param - Not Used. 
 *
 * Returns:     None
 *
 * Notes:       
 *  - All GBS channel NVM Rec Cnt Cleared
 *
 *****************************************************************************/
void GBSProtocol_DownloadClrHndl ( BOOLEAN Run, INT32 param )
{
  UINT16 i; 

  if (Run) {

    GBS_FileInit(); 
    
    for (i=0;i<GBS_MAX_CH;i++) {
      m_GBS_Status[i].cntBlkSizeNVM = 0; 
      m_GBS_Status[i].relayCksumNVM = 0;
    }
    m_GBS_Multi_Status.cntBlkSizeNVM = 0; 
    m_GBS_Multi_Status.relayCksumNVM = 0;
  }
}


/******************************************************************************
 * Function:    GBSProtocol_SIM_Port
 *
 * Description: Returns the Physical Port where GBS Ch is Simulated / Multiplex 
 *
 * Parameters:  None 
 *
 * Returns:     port number of physical ch that is similated / multiplex 
 *
 * Notes:       None
 *
 *****************************************************************************/
UINT16 GBSProtocol_SIM_Port ( void )
{
  return ( m_GBS_Ctl_Cfg.nPort );
}



/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:    GBS_ProcessRxData
 *
 * Description: Process Rx Data Bytes from the channel for GBS Protocol Processing
 *
 * Parameters:  pStatus - ptr to GBS status data for specific cha
 *              *pDownloadData - ptr to GBS_DOWNLOAD_DATA data 
 *              *pData - ptr to UART rx data buff
 *              cnt - bytes received from UART port
 *
 * Returns:     None
 *
 * Notes:
 *  - Process GBS states 
 *  - Manage top level restarts
 *
 *****************************************************************************/
static void GBS_ProcessRxData( GBS_STATUS_PTR pStatus, GBS_DOWNLOAD_DATA_PTR pDownloadData,
                               UINT8 *pData, UINT16 cnt)
{
  UINT32 tick_ms;  
  GBS_STATE_MULTI_ENUM multi; 
  
  tick_ms = CM_GetTickCount(); 
  
  pStatus->lastRxTime_ms = (cnt > 0) ? tick_ms : pStatus->lastRxTime_ms; 

  switch ( pStatus->state )
  {
    case GBS_STATE_IDLE:
      if ( *pDownloadData->bStartDownload == TRUE )
      {
        GBS_ProcessStateSetEDU_Init( pStatus );
        pStatus->nRetriesCurr = m_GBS_Ctl_Cfg.restarts;
        *pDownloadData->bStartDownload = FALSE;
        GSE_DebugStr(NORMAL,TRUE,"GBS Protocol: Download Start Request (Ch=%d)\r\n", 
                     (pStatus->multi_ch) ? GBS_SIM_PORT_INDEX : pStatus->ch);  
        pStatus->cntDnloadReq++;
        pStatus->bRelayStuck = FALSE; 
      }
      break;

    case GBS_STATE_SET_EDU_MODE:
      // Jump to sub state _SET_EDU_MODE,
      pStatus->state = GBS_ProcessStateSetConfirmEDU( pStatus, pData, cnt );
      if (pStatus->state == GBS_STATE_DOWNLOAD){
        GBS_ProcessStateRecord_Init ( pStatus, TRUE );
        GSE_DebugStr(NORMAL,TRUE,"GBS Protocol: EDU in GBS Mode (Ch=%d)\r\n", 
        (pStatus->multi_ch) ? GBS_SIM_PORT_INDEX : pStatus->ch);
      }
      break;

    case GBS_STATE_DOWNLOAD:
      // Jump to sub state _STATE_DOWNLOAD
      // GBS_ProcessStateRecords( pStatus, cnt );
      // If rec retrieved Ok, go to state confirm, else go to
      pStatus->state = GBS_ProcessStateRecords ( pStatus, pData, cnt );
      if (pStatus->state == GBS_STATE_CONFIRM) { // If next state == _COMPLETE don't need init
        GBS_ProcessStateConfirm_Init( pStatus );
        GSE_DebugStr(NORMAL,TRUE,"GBS Protocol: Confirming EDU Recs (Ch=%d)\r\n", 
                     (pStatus->multi_ch) ? GBS_SIM_PORT_INDEX : pStatus->ch);
      }
      break;

    case GBS_STATE_CONFIRM:
      pStatus->state = GBS_ProcessStateSetConfirmEDU( pStatus, pData, cnt );
#ifdef GBS_MULTI_DNLOADS  
      /*vcast_dont_instrument_start*/
      // If _CONFIRM is done, goto _DOWNLOAD again if looping thru Cmd Code is not complete
      //    Else, goto _COMPLETE.  NOTE: If Debug Cmd Code enb, goto _COMPLETE as well (thus
      //    exe the Debug Cmd Code only).  
      if ( (pStatus->state != GBS_STATE_CONFIRM) && 
           (pStatus->cmdRecCodeIndex < GBS_MAX_DNLOAD_TYPES) && 
           (m_GBS_Cfg[pStatus->ch].dnloadTypes[pStatus->cmdRecCodeIndex] != 
            GBS_DNLOAD_CODE_NOTUSED) &&
           (m_GBS_DebugDnload[pStatus->ch].dnloadCode == GBS_DNLOAD_CODE_NOTUSED) )
      { // Check if we have loop thru all cfg dnload code 
        // NOTE:  Need to update this logic as GBS_DNLOAD_CODE_NOTUSED not used anymore
        //        but set == to dnloadTypes[0] and if != use debug for one dnload
        GBS_CreateSummaryLog( pStatus, FALSE ); 
        pStatus->state = GBS_STATE_DOWNLOAD; 
        GBS_ProcessStateRecord_Init ( pStatus, FALSE );
      }
      /*vcast_dont_instrument_end*/
#endif      
      break;

    case GBS_STATE_COMPLETE:
      pStatus->state = GBS_ProcessStateComplete( pStatus );
      GSE_DebugStr(NORMAL,TRUE, 
              "GBS_Protocol: Download Completed (Ch=%d,Ok=%d,Rec=%d,Exp=%d)\r\n",
              (pStatus->multi_ch) ? GBS_SIM_PORT_INDEX : pStatus->ch, 
              pStatus->bCompleted, pStatus->dataBlkState.cntBlkRx, pStatus->cntBlkSizeExp);
      break;

    case GBS_STATE_RESTART_DELAY:
      if ( pStatus->restartDelay < tick_ms ) { // Delay expired
        GBS_ProcessStateSetEDU_Init( pStatus );
        GSE_DebugStr(NORMAL,TRUE, 
                "GBS_Protocol: Restarting (Ch=%d)\r\n",
                (pStatus->multi_ch) ? GBS_SIM_PORT_INDEX : pStatus->ch);
      }
      break;

    case GBS_STATE_MAX:
    default:
      FATAL("Invald GBS Protocol state = %d", pStatus->state); 
      break;
  }

  // If Cmd Seq failed, retry if # retries has not been exceeded
  if (pStatus->bCmdRspFailed == TRUE)
  { // retries exhausted, cmd failed or Relay Check failed 
    if ( (pStatus->nRetriesCurr == 0) || (pStatus->bRelayStuck == TRUE) )
    {
      pStatus->bDownloadFailed = TRUE;
      pStatus->bCmdRspFailed = FALSE;
      pStatus->state = GBS_STATE_COMPLETE;
      GSE_DebugStr(NORMAL,TRUE, 
                    "GBS_Protocol: Comm Loss, Restart Exhausted (Ch=%d)\r\n",
                    (pStatus->multi_ch) ? GBS_SIM_PORT_INDEX : pStatus->ch );       
    }
    else
    {
      // Don't generate log for each retry 
      // GBS_ProcessStateSetEDU_Init( pStatus );
      pStatus->nRetriesCurr--;
      GSE_DebugStr(NORMAL,TRUE, 
                "GBS_Protocol: Comm Loss, Will Restart (Ch=%d,Rtry=%d,Delay=%d)\r\n",
                (pStatus->multi_ch) ? GBS_SIM_PORT_INDEX : pStatus->ch, 
                pStatus->nRetriesCurr, m_GBS_Ctl_Cfg.restart_delay_ms);
      pStatus->state = GBS_STATE_RESTART_DELAY; 
      pStatus->restartDelay = tick_ms + m_GBS_Ctl_Cfg.restart_delay_ms; 
      pStatus->bCmdRspFailed = FALSE; 
      // Clear the Multi Port Switch Stuck check vars
      if ( (m_GBS_Ctl_Cfg.bMultiplex == TRUE) && (pStatus->ch == m_GBS_Ctl_Cfg.nPort) )
      {
        multi = (pStatus->multi_ch == TRUE) ? GBS_MULTI_SECONDARY : GBS_MULTI_PRIMARY; 
        memset ( (void *) &m_GBS_Multi_Ctl.chkSwitch[multi], 0, sizeof(GBS_MULTI_CHK) ); 
        m_GBS_Multi_Ctl.chkSwitch[multi].nvm_cksum = pStatus->relayCksumNVM; 
      }
    }
  } // end if ((pStatus->state != GBS_STATE_IDLE) && (pStatus->bCmdRspFailed == TRUE))
  
}


/******************************************************************************
 * Function:    GBS_ProcessStateSetConfirmEDU
 *
 * Description: Initialize data on transition to GBS_STATE_SET_EDU_MODE or 
 *              GBS_STATE_CONFIRM states
 *
 * Parameters:  pStatus - ptr to GBS_STATUS data for specific chan
 *              *pData - ptr to UART rx data buff
 *              cnt - bytes received from UART port
 *
 * Returns:     GBS_STATE_ENUM next state to transition to
 *
 * Notes:
 *  - Common routine for GBS_STATE_SET_EDU_MODE and GBS_STATE_CONFIRM state
 *  - Execute Cmd and process expected Rsp
 *  - Manage retries and timeout 
 *
 *****************************************************************************/
static GBS_STATE_ENUM GBS_ProcessStateSetConfirmEDU ( GBS_STATUS_PTR pStatus, UINT8 *pData,
                                                      UINT16 cnt)
{
  GBS_CMD_RSP_PTR cmdRsp_ptr;
  BOOLEAN bCmdCompleted;
  GBS_STATE_ENUM nextState, lcNextState;
  UINT32 tick_ms;
  UINT32 maxCmdRspEnum;
  UINT16 ch;
  const CHAR *cmd_rsp_str; 


  ch = pStatus->ch;

  if ( pStatus->state == GBS_STATE_SET_EDU_MODE )
  {
    lcNextState = GBS_STATE_DOWNLOAD;
    maxCmdRspEnum = (UINT32) GBS_SETEDU_MAX;
  }
  else
  {
    lcNextState = GBS_STATE_COMPLETE;
    maxCmdRspEnum = (UINT32) GBS_STATE_CONFIRM_MAX;
    pStatus->bCompleted = TRUE; 
  }

  // If GBS_RSP_NONE, then wait till timeout before moving to next state.
  // Else if GBS_RSP_ACK, process _ACK response.
  //   If _ACK OK then move to next state
  //   ELSE if no _ACK and timeout then retry
  //     If retry exhausted then move back up to try overall retries (start from top)
  cmdRsp_ptr = (GBS_CMD_RSP_PTR) pStatus->cmdRsp_ptr;
  bCmdCompleted = FALSE;
  nextState = pStatus->state;
  tick_ms = CM_GetTickCount();

  if (pStatus->msCmdDelay <= tick_ms)  // Wait Cmd to Cmd delay to expire
  {
    switch ( cmdRsp_ptr->respType )
    {
      case GBS_RSP_NONE:
        // Wait for timeout before transitioning to next cmd-rsp state
        bCmdCompleted = (pStatus->msCmdTimer <= tick_ms) ? TRUE : FALSE;
        break;

      case GBS_RSP_ACK:
        // Process Rsp ACK. If Ok, transition to next state
        //   else if timeout, then retry this cmd if retries have not been exhusted
        bCmdCompleted = cmdRsp_ptr->rspHndl( pData, cnt, (UINT32) pStatus );
        break;

      case GBS_RSP_BLK:
      default:
        FATAL("Invald GBS Protocol respType = %d", cmdRsp_ptr->respType); 
        break;
    }

    // If GBS_SETEDU_MAX reached GBS_STATE_SET_EDU_MODE completed, move to GBS_STATE_DOWNLOAD
    if ( bCmdCompleted == TRUE )
    {
      cmd_rsp_str = (pStatus->state == GBS_STATE_SET_EDU_MODE) ? 
                    CMD_RSP_SETEDU_STR[pStatus->cmdRsp_ptr->cmd_rsp_state] :
                    CMD_RSP_CONFIRM_STR[pStatus->cmdRsp_ptr->cmd_rsp_state]; 
      GSE_DebugStr(NORMAL,TRUE, 
              "GBS_Protocol: SetComfirmEDU Cmd Completed (Ch=%d,Cmd=%s,Rtry=%d)\r\n",
              (pStatus->multi_ch) ? GBS_SIM_PORT_INDEX : pStatus->ch, cmd_rsp_str, 
              pStatus->nCmdRetriesCurr);
    
      cmdRsp_ptr++;  // Move to next Cmd for _SET_EDU_MODE or CONFIRM_MODE
      if ( (UINT32) cmdRsp_ptr->cmd_rsp_state == maxCmdRspEnum) {
        nextState = lcNextState; // _SET_EDU_MODE completed move to _DOWNLOAD state
                                 // _CONFIRM completed move to _COMPLETE state
      }
      else { // Send next CmdRsp for _SET_EDU_MODE
        pStatus->cmdRsp_ptr = cmdRsp_ptr;
        GBS_CreateKeepAlive ( cmdRsp_ptr );  
        memcpy ( (UINT8 *) &m_GBS_TxBuff[ch].buff[0], (UINT8 *) &cmdRsp_ptr->cmd[0],
                 cmdRsp_ptr->cmdSize );
        m_GBS_TxBuff[ch].cnt = cmdRsp_ptr->cmdSize;

        pStatus->msCmdTimer = tick_ms + cmdRsp_ptr->nRetryTimeOut + 
                              cmdRsp_ptr->nCmdToCmdDelay;
        pStatus->nCmdRetriesCurr = cmdRsp_ptr->nRetries;
        pStatus->msCmdDelay = tick_ms + cmdRsp_ptr->nCmdToCmdDelay; // Delay before sending Tx
      }
    } // end if cmd completed Ok
    else // Check for timeout and retries
    { // Time Out expired. For GBS_RSP_ACK, should never reach here
      if ( pStatus->msCmdTimer <= tick_ms )
      {
        if ( pStatus->nCmdRetriesCurr == 0 ) {
          pStatus->bCmdRspFailed = TRUE; // retries exhausted, cmd failed
        }
        else {
          memcpy ( (UINT8 *) m_GBS_TxBuff[ch].buff, (UINT8 *) &cmdRsp_ptr->cmd[0],
                   cmdRsp_ptr->cmdSize );
          m_GBS_TxBuff[ch].cnt = cmdRsp_ptr->cmdSize;
          pStatus->nCmdRetriesCurr--;
          pStatus->msCmdTimer = tick_ms + cmdRsp_ptr->nRetryTimeOut;
          
          cmd_rsp_str = (pStatus->state == GBS_STATE_SET_EDU_MODE) ? 
                        CMD_RSP_SETEDU_STR[pStatus->cmdRsp_ptr->cmd_rsp_state] :
                        CMD_RSP_CONFIRM_STR[pStatus->cmdRsp_ptr->cmd_rsp_state]; 
          GSE_DebugStr(NORMAL,TRUE, 
              "GBS_Protocol: SetComfirmEDU Retry (Ch=%d,Cmd=%s,Rtry=%d)\r\n",
              (pStatus->multi_ch) ? GBS_SIM_PORT_INDEX : pStatus->ch, cmd_rsp_str, 
              pStatus->nCmdRetriesCurr);
          // Don't need Cmd To Cmd Delay timer for retries
        }
      } // end if Time Out expired
    } // end else check for time out and retries
  } // end if (cmdRsp_ptr->nCmdToCmdDelay < tick_ms
  
  return (nextState);
}


/******************************************************************************
 * Function:    GBS_ProcessStateSetEDU_Init
 *
 * Description: Initializes data before entry into SetEDU mode
 *
 * Parameters:  pStatus - ptr to GBS_STATUS data for specific chan
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static void GBS_ProcessStateSetEDU_Init ( GBS_STATUS_PTR pStatus )
{
  pStatus->state = GBS_STATE_SET_EDU_MODE;
  pStatus->bDownloadFailed = FALSE;
  pStatus->bCompleted = FALSE;
  pStatus->bDownloadInterrupted = FALSE;
  // Start from 1st state, init retries / timeouts
  pStatus->cmdRsp_ptr = &cmdRsp_SetEDU[0];
  pStatus->bCmdRspFailed = FALSE;
  pStatus->msCmdTimer = CM_GetTickCount() + cmdRsp_SetEDU[0].nRetryTimeOut;
  pStatus->nCmdRetriesCurr = cmdRsp_SetEDU[0].nRetries;
  pStatus->msCmdDelay = 0; // Clear Cmd to Cmd runtime timer
  // Clear Blk Cnt data
  pStatus->cntBlkSizeBad = 0; 
  pStatus->cntBlkSizeExp = 0;
  pStatus->cntBlkSizeExpDnNew = 0;
  pStatus->cntDnLoadSizeExp = 0; 
  
  memset ( (UINT8 *) &pStatus->dataBlkState, 0, sizeof(GBS_BLK_DATA) ); 
}


/******************************************************************************
 * Function:    GBS_ProcessStateConfirm_Init
 *
 * Description: Initializes data before entry into GBS_STATE_CONFIRM
 *
 * Parameters:  pStatus -  ptr to GBS_STATUS data for specific chan
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static void GBS_ProcessStateConfirm_Init ( GBS_STATUS_PTR pStatus )
{
  // Start from 1st state, init retries / timeouts
  pStatus->cmdRsp_ptr = &cmdRsp_Confirm[0];
  pStatus->bCmdRspFailed = FALSE;
  pStatus->msCmdTimer = CM_GetTickCount() + cmdRsp_Confirm[0].nRetryTimeOut;
  pStatus->nCmdRetriesCurr = cmdRsp_Confirm[0].nRetries;
  pStatus->msCmdDelay = 0; // Clear Cmd to Cmd runtime timer
}


/******************************************************************************
 * Function:    GBS_ProcessStateRecords
 *
 * Description: Processing for GBS Cmd Code / Response
 *
 * Parameters:  pStatus - ptr to GBS_STATUS data for specific chan
 *              *pData - ptr to UART rx data buff
 *              cnt - bytes received from UART port
 *
 * Returns:     GBS_STATE_ENUM next state to transition to
 *
 * Notes:
 *  - Send GBS Cmd Code
 *  - Process GBS Cmd Code Rsp (Blk Info then Blk records)
 *  - Terminate when all Blk records received
 *  - Manage retries and timeout 
 *  - Transition to GBS_STATE_CONFIRM if retrieved success
 *                  GBS_STATE_COMPLETE if retrievd failed
 *  - If store of data rec fails, retrieval considered failed
 *
 *****************************************************************************/
static GBS_STATE_ENUM GBS_ProcessStateRecords ( GBS_STATUS_PTR pStatus, UINT8 *pData,
                                                UINT16 cnt )
{
  GBS_CMD_RSP_PTR cmdRsp_ptr;
  BOOLEAN bCmdCompleted;
  GBS_STATE_ENUM nextState;
  UINT16 ch, index_n;
  UINT32 tick_ms;

  ch = pStatus->ch;

  cmdRsp_ptr = (GBS_CMD_RSP_PTR) pStatus->cmdRsp_ptr;
  bCmdCompleted = FALSE;
  nextState = pStatus->state;
  tick_ms = CM_GetTickCount();

  if (pStatus->msCmdDelay <= tick_ms)  // Wait Cmd to Cmd delay to expire
  {
    switch ( cmdRsp_ptr->respType )
    {
      case GBS_RSP_NONE: // For _RECORD_IDLE
        // Wait for timeout before transitioning to next cmd-rsp state
        bCmdCompleted = (pStatus->msCmdTimer <= tick_ms) ? TRUE : FALSE;
        break;
      case GBS_RSP_BLK:  // For _RECORD_CODE and _RECORD_ACK_NAK
        bCmdCompleted = cmdRsp_ptr->rspHndl( pData, cnt, (UINT32) pStatus );
        break;

      case GBS_RSP_ACK:
      default:
        FATAL("Invald GBS Protocol respType = %d", cmdRsp_ptr->respType); 
        break;
    }

    if ( bCmdCompleted == TRUE )
    {
      GSE_DebugStr(NORMAL,TRUE, 
              "GBS_Protocol: _StateRecord Cmd Completed (Ch=%d,Cmd=%s(%d),Rtry=%d)\r\n",
              (pStatus->multi_ch) ? GBS_SIM_PORT_INDEX : pStatus->ch, 
              CMD_RSP_RECORD_STR[pStatus->cmdRsp_ptr->cmd_rsp_state], 
              pStatus->dataBlkState.cntBlkCurr, pStatus->nCmdRetriesCurr);
    
      if (cmdRsp_ptr->cmd_rsp_state == (UINT32) GBS_STATE_RECORD_CODE)
      { // If # Rec == # Rec last retrieval AND !dnload new code, then exit as NO NEW REC
        index_n = pStatus->cmdRecCodeIndex - 1;
        if ( (pStatus->cntBlkSizeExp == pStatus->cntBlkSizeNVM) && 
             (pStatus->cntBlkSizeNVM != 0) && 
             (m_GBS_Cfg[ch].dnloadTypes[index_n] == GBS_DNLOAD_CODE_NEW) )
        {
          nextState = GBS_STATE_COMPLETE;
          pStatus->bCompleted = TRUE;  // No new records.  Completed OK. 
          GSE_DebugStr(NORMAL,TRUE, 
                        "GBS_Protocol: No New Rec (Ch=%d,NVMRec=%d,Rec=%d)\r\n",
                        (pStatus->multi_ch) ? GBS_SIM_PORT_INDEX : pStatus->ch,
                        pStatus->cntBlkSizeNVM, pStatus->cntBlkSizeExp);          
        }
        pStatus->cntBlkSizeExpDnNew = (m_GBS_Cfg[ch].dnloadTypes[index_n] == 
                                       GBS_DNLOAD_CODE_NEW) ? 
                                       pStatus->cntBlkSizeExp : pStatus->cntBlkSizeExpDnNew; 
      } 
      // Fall thru. Ok if nextState set to GBS_STATE_COMPLETE already. 
      if ( cmdRsp_ptr->cmd_rsp_state != (UINT32) GBS_STATE_RECORD_ACK_NACK ) {
        cmdRsp_ptr++;
      }
      else {
        // If all blk rx, then goto confirm if Ok, else just goto complete 
        if (pStatus->dataBlkState.cntBlkCurr >= pStatus->cntBlkSizeExp) {
          nextState = ((pStatus->dataBlkState.bFailed == TRUE) ||
                       (pStatus->dataBlkState.cntStoreBad > 0) || 
                       (m_GBS_Ctl_Cfg.bConfirm == FALSE)) ?  GBS_STATE_COMPLETE :
                                                             GBS_STATE_CONFIRM;
          pStatus->bCompleted = ((m_GBS_Ctl_Cfg.bConfirm == FALSE) && 
                                 (pStatus->dataBlkState.bFailed == FALSE) &&
                                 (pStatus->dataBlkState.cntStoreBad  == 0)) ? 
                                 TRUE : pStatus->bCompleted;
        }
        GBS_ChkMultiInstall ( pStatus ); 
      }
      
      if (nextState == GBS_STATE_DOWNLOAD) // If still in Rec Dnload mode, sent ACK/NAK
      {
        pStatus->cmdRsp_ptr = cmdRsp_ptr;
        // ACK or NAK set in rspHndl() processing
        memcpy ( (UINT8 *) &m_GBS_TxBuff[ch].buff[0], (UINT8 *) &cmdRsp_ptr->cmd[0],
                 cmdRsp_ptr->cmdSize );
        m_GBS_TxBuff[ch].cnt = cmdRsp_ptr->cmdSize;
  
        pStatus->msCmdTimer = tick_ms + cmdRsp_ptr->nRetryTimeOut + 
                              cmdRsp_ptr->nCmdToCmdDelay;
        pStatus->nCmdRetriesCurr = cmdRsp_ptr->nRetries;
        pStatus->msCmdDelay = tick_ms + cmdRsp_ptr->nCmdToCmdDelay; // Delay before sending Tx
        
        GBS_FLUSH_RXBUFF;
        pStatus->dataBlkState.state = GBS_BLK_STATE_GETSIZE;
        pStatus->dataBlkState.currBlkSize = 0;
      }
    }
    else
    { // Time Out expired. For GBS_RSP_NONE should never reach here.
    
      if ( pStatus->msCmdTimer <= tick_ms )
      { // If retries attempts completed and NOT wait for Rec to be stored, consider this fail
        if ( (pStatus->nCmdRetriesCurr == 0) && 
             (pStatus->dataBlkState.state != GBS_BLK_STATE_SAVING) ) {
          pStatus->bCmdRspFailed = TRUE; // retries exhausted, cmd failed
        }
        else {
          // If storing rec or receiving Data, don't send ACK/NAK, just continue
          //   overall timer count down. Will wait nRetryTimeOut after Rx Idle
          //   before allowing ACK/NAK (needed for retries where prev ACK/NAK not
          //   seen by EDU).
          if ( (pStatus->dataBlkState.state != GBS_BLK_STATE_SAVING) && 
               ((tick_ms - pStatus->lastRxTime_ms) >= cmdRsp_ptr->nRetryTimeOut) && 
               ((tick_ms - pStatus->lastRxTime_ms) > m_GBS_Ctl_Cfg.timeIdleOut) &&
               (pStatus->dataBlkState.bFailed != TRUE) )
          {
            memcpy ( (UINT8 *) m_GBS_TxBuff[ch].buff, (UINT8 *) &cmdRsp_ptr->cmd[0],
                     cmdRsp_ptr->cmdSize );
            m_GBS_TxBuff[ch].cnt = cmdRsp_ptr->cmdSize;
          }
          pStatus->nCmdRetriesCurr--;
          pStatus->msCmdTimer = tick_ms + cmdRsp_ptr->nRetryTimeOut;
          
          GSE_DebugStr(NORMAL,TRUE, 
              "GBS_Protocol: _StateRecord Retry (Ch=%d,Cmd=%s(%d),Rtry=%d)\r\n",
              (pStatus->multi_ch) ? GBS_SIM_PORT_INDEX : pStatus->ch, 
              CMD_RSP_RECORD_STR[pStatus->cmdRsp_ptr->cmd_rsp_state],
              pStatus->dataBlkState.cntBlkCurr, pStatus->nCmdRetriesCurr);   
                        
          // Don't need Cmd To Cmd Delay timer for retries
        } // end else perform retry 
      } // end if Time Out expired
      
    } // end else check for time out and retries
  } // end if (pStatus->msCmdDelay <= tick_ms)

  return (nextState);
}


/******************************************************************************
 * Function:    GBS_ProcessStateRecord_Init
 *
 * Description: Initializes data before entry into GBS_STATE_DOWNLOAD state
 *
 * Parameters:  pStatus - ptr to GBS_STATUS data for specific chan
 *              clearCmdCodeIndex - CmdCode to send for Blk Req
 *
 * Returns:     None
 *
 * Notes:
 *  - Initialize variables and state for Get Rec Blk Size and Rec 
 *  - Send cfg dnload code or debug dnload code (if set)
 *
 *****************************************************************************/
static void GBS_ProcessStateRecord_Init ( GBS_STATUS_PTR pStatus, BOOLEAN clearCmdCodeIndex )
{
  GBS_CMD_RSP_PTR cmdRsp_ptr;
  UINT16 ch;


  ch = pStatus->ch;
  cmdRsp_ptr = &cmdRsp_Record[ch][0];

  // Start from 1st state, init retries / timeouts
  pStatus->cmdRsp_ptr = cmdRsp_ptr;
  pStatus->bCmdRspFailed = FALSE;
  pStatus->msCmdTimer = CM_GetTickCount() + cmdRsp_ptr->nRetryTimeOut;
  pStatus->nCmdRetriesCurr = cmdRsp_ptr->nRetries;
  pStatus->msCmdDelay = 0; // Clear Cmd to Cmd runtime timer
  pStatus->cntBadCRCRow = 0; 

  // Setup GBS_STATE_RECORD_CODE
  // Start with first index into GBS_CFG.dnloadTypes[index] if STARTING DNLOAD
  pStatus->cmdRecCodeIndex = (clearCmdCodeIndex == TRUE) ? 0 : pStatus->cmdRecCodeIndex; 
  
  m_CmdReqBlk[ch].cmd = 0x5E;
  m_CmdReqBlk[ch].code = ((m_GBS_DebugDnload[ch].dnloadCode != 
                            m_GBS_Cfg[ch].dnloadTypes[0]) && 
                           (clearCmdCodeIndex == TRUE)) ? 
                          m_GBS_DebugDnload[ch].dnloadCode :
                          m_GBS_Cfg[ch].dnloadTypes[pStatus->cmdRecCodeIndex];   
  memcpy ( (UINT8 *) &cmdRsp_Record[ch][GBS_STATE_RECORD_CODE_INDEX].cmd[0],
           &m_CmdReqBlk[ch], sizeof(GBS_CMD_REQ_BLK) );
  cmdRsp_Record[ch][GBS_STATE_RECORD_CODE_INDEX].cmdSize = sizeof(GBS_CMD_REQ_BLK);
  ++pStatus->cmdRecCodeIndex; 
  
  GSE_DebugStr(NORMAL,TRUE, 
                "GBS_Protocol: Dnload Code = %d (Ch=%d)\r\n", m_CmdReqBlk[ch].code, 
                (pStatus->multi_ch) ? GBS_SIM_PORT_INDEX : pStatus->ch );
 
  // Initialize Data Vars
  pStatus->cntBlkSizeBad = 0; 
  pStatus->cntBlkSizeExp = 0;
  pStatus->cntBlkSizeExpDnNew = (clearCmdCodeIndex == TRUE) ? 0 : pStatus->cntBlkSizeExpDnNew;
  pStatus->cntDnLoadSizeExp = 0; 
  
  memset ( (UINT8 *) &pStatus->dataBlkState, 0, sizeof(GBS_BLK_DATA) ); 
  
  GBS_FLUSH_RXBUFF;
  
  m_GBS_RxBuffMultiRec[ch].cnt = 0; 
  m_GBS_RxBuffMultiRec[ch].data_index = 0; 
}


/******************************************************************************
 * Function:    GBS_ProcessStateComplete
 *
 * Description: Process download completion 
 *
 * Parameters:  pStatus - ptr to GBS_STATUS data for specific chan
 *
 * Returns:     GBS_STATE_ENUM next state to transition to
 *
 * Notes:
 *  - If chan is multiplex / simulated, set other chan (PRIMARY/SECONDARY)
 *  - Update NVM data if completed successfully 
 *  - Generate summary log
 *
 *****************************************************************************/
static GBS_STATE_ENUM GBS_ProcessStateComplete ( GBS_STATUS_PTR pStatus )
{
  GBS_DOWNLOAD_DATA_PTR pDownloadData; 

  // Set UartMgr flag to indicate download is completed (PASS or FAILED).
  pDownloadData = pStatus->pDownloadData;
  *pDownloadData->bDownloadCompleted = TRUE; 
  
  // Update NVM PktCnt. If retreived successfully and cnt don't match update NVM
  if ( (pStatus->cntBlkSizeExpDnNew != pStatus->cntBlkSizeNVM) &&
       (pStatus->bCompleted == TRUE) )
  {
    pStatus->cntBlkSizeNVM = pStatus->cntBlkSizeExpDnNew; 
    if ( (m_GBS_Multi_Ctl.state == GBS_MULTI_SECONDARY) && 
         (pStatus->ch == m_GBS_Ctl_Cfg.nPort) ) {
      m_GBS_App_Data.pktCnt_Multi = pStatus->cntBlkSizeExpDnNew;
      m_GBS_App_Data.relayCksum_Multi = m_GBS_Multi_Ctl.chkSwitch[GBS_MULTI_SECONDARY].cksum; 
      pStatus->relayCksumNVM = m_GBS_App_Data.relayCksum_Multi; 
    } // end of if MULTI set and SECONDARY active
    else {
      m_GBS_App_Data.pktCnt[pStatus->ch] = pStatus->cntBlkSizeExpDnNew;
      m_GBS_App_Data.relayCksum[pStatus->ch] = (pStatus->ch == m_GBS_Ctl_Cfg.nPort) ?
           m_GBS_Multi_Ctl.chkSwitch[GBS_MULTI_PRIMARY].cksum : 0; 
      pStatus->relayCksumNVM =  m_GBS_App_Data.relayCksum[pStatus->ch]; 
    } // end of else not MULTI and SECONDARY, thus straight channel 
    NV_Write ( NV_GBS_DATA, 0, (void *) &m_GBS_App_Data, sizeof(m_GBS_App_Data) ); 
  } // end if (pStatus->cntBlkSizeExp != pStatus->cntBlkSizeNVM) 
  
  
  // If Multiplex Mode, allow the other channel to retriev 
  if ( (m_GBS_Ctl_Cfg.bMultiplex == TRUE) && (pStatus->ch == m_GBS_Ctl_Cfg.nPort) )
  { // If Multiplex is set, check and allow other device to be retrieved 
    if ( m_GBS_Multi_Ctl.state == GBS_MULTI_PRIMARY ) 
    {
      m_GBS_Multi_Ctl.bPrimaryReqPending = FALSE; 
      m_GBS_Multi_Ctl.state = (m_GBS_Multi_Ctl.bSecondaryReqPending == TRUE) ?
                              GBS_MULTI_SECONDARY : GBS_MULTI_PRIMARY; 
    }
    else 
    {
      m_GBS_Multi_Ctl.bSecondaryReqPending = FALSE; 
      m_GBS_Multi_Ctl.state = (m_GBS_Multi_Ctl.bPrimaryReqPending == TRUE ) ? 
                              GBS_MULTI_PRIMARY : GBS_MULTI_SECONDARY;       
    }
    m_GBS_DebugDnload[pStatus->ch].dnloadCode = 
      ((m_GBS_Multi_Ctl.bPrimaryReqPending == FALSE) && 
       (m_GBS_Multi_Ctl.bSecondaryReqPending == FALSE)) ? 
       m_GBS_Cfg[pStatus->ch].dnloadTypes[0] : m_GBS_DebugDnload[pStatus->ch].dnloadCode;     
    // Set Multiplex Mode to PRIMARY if no one is pending.  Allow Relay to be set to ACTION_OFF 
    //   (assoc with PRIMARY) so that Relay can be "de-energized". 
    m_GBS_Multi_Ctl.state =  ( (m_GBS_Multi_Ctl.bSecondaryReqPending == FALSE) && 
                               (m_GBS_Multi_Ctl.bPrimaryReqPending == FALSE) ) ? 
                               GBS_MULTI_PRIMARY :  m_GBS_Multi_Ctl.state; 
  } // end if (m_GBS_Multi_Cfg.bMultiplex == TRUE) && (pStatus->ch == m_GBS_Multi_Cfg.nPort)
  else 
  {
    // m_GBS_DebugDnload[pStatus->ch].dnloadCode = GBS_DNLOAD_CODE_NOTUSED;
    m_GBS_DebugDnload[pStatus->ch].dnloadCode = m_GBS_Cfg[pStatus->ch].dnloadTypes[0];
  }
 
  GBS_CreateSummaryLog(pStatus, TRUE); // Create statistics summary log
  
  return (GBS_STATE_IDLE); 
}


/******************************************************************************
 * Function:    GBS_ProcessACK
 *
 * Description: Process ACK data from GBS chan 
 *
 * Parameters:  *pData - ptr to UART rx data buff
 *              cnt - bytes received from UART port
 *              status_ptr - not used
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN GBS_ProcessACK ( UINT8 *pData, UINT16 cnt, UINT32 status_ptr )
{
  BOOLEAN bOk;

  bOk = FALSE;

  // Cnt s/b exactly 1 byte for ACK
  if (cnt == GBS_RSP_ACK_SIZE) {
    bOk = (*pData == GBS_RSP_ACK_DATA) ? TRUE : FALSE;
  }

  return (bOk);
}


/******************************************************************************
 * Function:    GBS_ProcessBlkSize
 *
 * Description: Process GBS Blk Size Response
 *
 * Parameters:  *pData - ptr to UART rx data buff
 *              cnt - bytes received from UART port
 *              status_ptr - not used
 *
 * Returns:     TRUE if Blk Response Processed
 *
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN GBS_ProcessBlkSize ( UINT8 *pData, UINT16 cnt, UINT32 status_ptr )
{
  BOOLEAN bOk, bGoodData;
  GBS_STATUS_PTR pStatus;
  UINT8 *pDest;
  UINT32 *pCnt;
  UINT16 ch; 


  bOk = FALSE;
  pStatus = (GBS_STATUS_PTR) status_ptr; 
  ch = pStatus->ch; 

  if (pStatus->cntBlkSizeBad < (m_GBS_Ctl_Cfg.retriesSingle + 1))
  {
    pCnt = (UINT32 *) &m_GBS_RxBuff[ch].cnt;
    pDest = (UINT8 *) &m_GBS_RxBuff[ch].buff[*pCnt];

    if ( (cnt > 0) && (*pCnt < GBS_RAW_BUFF_MAX) )  // copy any new data bytes received
    {
      memcpy ( (UINT8 *) pDest, (UINT8 *) pData, cnt );
      *pCnt = *pCnt + cnt;
      GBS_RunningCksum( ch, GBS_CKSUM_BLKSIZE_TYPE ); 
    }
    bGoodData = (*pCnt > sizeof(GBS_BLK_SIZE_REC)) ? FALSE : TRUE;
    if (*pCnt == sizeof(GBS_BLK_SIZE_REC) )
    { // Verify checksum
      bGoodData = GBS_VerifyChkSum ( ch, GBS_CKSUM_BLKSIZE_TYPE );
      if (bGoodData)
      {
        // Update pStatus var, Trans to GetRecords
        pCnt = (UINT32 *) &m_GBS_RxBuff[ch].buff[0];
        pStatus->cntBlkSizeExp = GBS_HTONL ( *pCnt ) & 0x0000FFFFUL;
        pCnt++; 
        pStatus->cntDnLoadSizeExp  = GBS_HTONL ( *pCnt ); 
        *pStatus->ptrACK_NAK = GBS_RSP_ACK_DATA; 
        bOk = TRUE; 
        GSE_DebugStr(NORMAL,TRUE, 
                      "GBS_Protocol: Block Info Rx (Ch=%d,BlkCnt=%d,TotalSize=%d)\r\n",
                      (pStatus->multi_ch) ? GBS_SIM_PORT_INDEX : pStatus->ch, 
                      pStatus->cntBlkSizeExp, pStatus->cntDnLoadSizeExp);
      }
    }
    // Data Packet too large or chksum bad
    if (bGoodData == FALSE)
    {
      if ( ++pStatus->cntBlkSizeBad <= (m_GBS_Ctl_Cfg.retriesSingle + 1) )
      { // Setup to send NAK instead of BlkReqSize Cmd
        cmdRsp_Record[ch][GBS_STATE_RECORD_CODE_INDEX].cmd[0] = GBS_RSP_NAK_DATA;
        cmdRsp_Record[ch][GBS_STATE_RECORD_CODE_INDEX].cmdSize = sizeof(UINT8); 
        // Reset timeout to give full "20 sec"
        pStatus->msCmdTimer = pStatus->cmdRsp_ptr->nRetryTimeOut + CM_GetTickCount();
        pStatus->nCmdRetriesCurr = pStatus->cmdRsp_ptr->nRetries; 
        GBS_FLUSH_RXBUFF;  // Flush Rx Buffer
      } // end if ( ++pStatus->cntBlkSizeBad <= GBS_BLK_SIZE_BAD_MAX )
    } // end if (bGoodData == FALSE)
  } // end if (pStatus->cntBlkSizeBad <= GBS_BLK_SIZE_BAD_MAX)
  else 
  { // Blk Size retries exhausted.  Send Idle for 20 sec. 
    cmdRsp_Record[ch][GBS_STATE_RECORD_CODE_INDEX].cmdSize = 0;  
  }

  return (bOk);
}


/******************************************************************************
 * Function:    GBS_ProcessBlkRec
 *
 * Description: Process GBS Data Record Response
 *
 * Parameters:  *pData - ptr to UART rx data buff
 *              cnt - bytes received from UART port
 *              status_ptr - ptr to GBS_STATUS data for specific chan
 *
 * Returns:     None
 *
 * Notes:
 *  - Blk Rec
 *      Byte 0:  Blk Number LSB
 *      Byte 1:  Blk Number MSB
 *      Byte 2:  Blk Size LSB (number of total Bytes)
 *      Byte 3:  Blk Size MSB (number of total Bytes) 
 *      Byte 4:  Data Byte 1
 *      Byte 5:  Data Byte 2
 *      Byte m-1: Data Byte m-1
 *      Byte m:   Data Byte m
 *      Byte m+1: Cksum LSB of LSW
 *      Byte m+2: Cksum MSB of LSW
 *      Byte m+3: Cksum LSB of MSW
 *      Byte m+4: Cksum MSB of MSW
 *      Byte m+5: Status LSB of LSW
 *      Byte m+6: Status MSB of LSW
 *      Byte m+7: Status LSB of MSW
 *      Byte m+8: Status MSB of MSW
 *  - If Rec Size larger than expected, consider rec bad
 *  - If Rec Size not reached before IDLE timeout reach, consider rec bad
 *  - If Rec Size reached, verify cksum.  If bad, consider rec bad
 *  - If max retries reach, just timeout to force exit of GBS mode and restart
 *
 *****************************************************************************/
static BOOLEAN GBS_ProcessBlkRec ( UINT8 *pData, UINT16 cnt, UINT32 status_ptr )
{
  BOOLEAN bOk, bGoodData;
  GBS_STATUS_PTR pStatus;
  GBS_BLK_DATA_PTR pBlkData; 
  UINT8 *pDest;
  UINT32 *pCnt, tick_ms, i, j, k;  
  UINT16 *pWord, ch, blkSize; 
  GBS_DOWNLOAD_DATA_PTR pDownloadData;
  GBS_RAW_RX_BUFF_MULTI_REC_PTR pBuffManyRec; 
  

  bOk = FALSE;
  pStatus = (GBS_STATUS_PTR) status_ptr; 
  pDownloadData = pStatus->pDownloadData;
  
  ch = pStatus->ch; 
  pBlkData = &pStatus->dataBlkState; 
  pBuffManyRec = &m_GBS_RxBuffMultiRec[ch];           
  
  tick_ms = CM_GetTickCount();
  
  if ( pBlkData->bFailed == FALSE )
  {
    switch (pBlkData->state) 
    {
      case GBS_BLK_STATE_GETSIZE:
      case GBS_BLK_STATE_DATA: 
        pCnt = (UINT32 *) &m_GBS_RxBuff[ch].cnt;
        pDest = (UINT8 *) &m_GBS_RxBuff[ch].buff[*pCnt];
        if ( (cnt > 0) && (*pCnt < GBS_RAW_BUFF_MAX) )  // copy any new data bytes received
        {
          memcpy ( (UINT8 *) pDest, (UINT8 *) pData, cnt );
          *pCnt = *pCnt + cnt;
          GBS_RunningCksum( ch, GBS_CKSUM_REC_TYPE ); 
          if ( (m_GBS_Ctl_Cfg.bBuffRecStore == TRUE) && (*pCnt > GBS_BLK_OH) )
          { // As data Rx fill the "Combined Rec Buff" 
            // Determine how much to copy 
            i = (*pCnt - GBS_BLK_OH) - pBuffManyRec->data_index; 
            // If beginning of Rec, exclude GBS_BLK_OH
            j = (pBuffManyRec->data_index == 0) ? GBS_BLK_OH : 0; 
            k = pBuffManyRec->cnt + pBuffManyRec->data_index; 
            memcpy ( (UINT8 *) &pBuffManyRec->buff[k], (pDest + j), i ); 
            pBuffManyRec->data_index = pBuffManyRec->data_index + i; 
          }
        }      
      
        if ( (pBlkData->currBlkSize == 0) && (*pCnt > sizeof(UINT32)) )
        { // Get the blk # and size from the beginning of packet
          pWord = (UINT16 *) &m_GBS_RxBuff[ch].buff[0];
          pBlkData->currBlkNum = GBS_HTONS(*pWord);
          pWord++;  
          //pBlkData->currBlkSize = GBS_HTONS (*pWord) + GBS_BLK_OH; 
          pBlkData->currBlkSize = GBS_HTONS (*pWord); 
          pBlkData->state = GBS_BLK_STATE_DATA;
        }
        if ( pBlkData->state == GBS_BLK_STATE_DATA )
        { // If bytes Rx > expected then we have a problem. Expect blk sz not to incl OH
          blkSize = (UINT16) (pBlkData->currBlkSize + GBS_BLK_OH);
#ifdef DTU_GBS_SIM
          /*vcast_dont_instrument_start*/
          if (m_GBS_Debug.DTU_GBS_SIM_SimLog == TRUE) {
            // Temp.  Add '8' to be used with DTU GBS SIM as SIM + '4' for U16 Blk#,Size
            //   blk size does not inlude CRC and STATUS at the end          
            blkSize = pBlkData->currBlkSize + GBS_BLK_OH + GBS_CKSUM_EXCL;  
          }
          /*vcast_dont_instrument_end*/
#endif          
          // blkSize = pBlkData->currBlkSize + GBS_BLK_OH;  
          // blkSize = pBlkData->currBlkSize + GBS_BLK_OH + GBS_CKSUM_EXCL;  
              // Temp.  Add '8' to be used with DTU GBS SIM as SIM + '4' for U16 Blk#,Size
              //   blk size does not inlude CRC and STATUS at the end          
          bGoodData = (*pCnt > blkSize) ? FALSE : TRUE;
          // If bytes Rx < expected after IDLE timeout then we have a problem
            // If last Rx Data time to current exceed cfg IDLE time 
            //   then setup NAK for the retry later.  Note this cfg IDLE timeout must be 
            //   less than retry timeout, so that when retry timeout expires NAK will be sent 
            //   instead of what's in buffer        
            // NOTE: m_GBS_Multi_Cfg.timeIdleOut s/b < cmdRsp_ptr->nRetryTimeOut, else ACK 
            //   might be sent, which is not what we want
          bGoodData = ((*pCnt < blkSize) &&
                       ((tick_ms - pStatus->lastRxTime_ms) > m_GBS_Ctl_Cfg.timeIdleOut)) ?
                       FALSE : bGoodData; 
                       
          if (*pCnt == blkSize)  
          {
            bGoodData = GBS_VerifyChkSum ( ch, GBS_CKSUM_REC_TYPE );
            if (bGoodData)
            { // Store Data and trans to _STATE_SAVING
              if (m_GBS_Ctl_Cfg.bBuffRecStore == TRUE) 
              {
                pBuffManyRec->cnt = pBuffManyRec->cnt + pBuffManyRec->data_index; 
                pBuffManyRec->data_index = 0; 
                pBlkData->cntBlkRx++; 
                pBlkData->cntBlkBuffering++; 
              }
              // Have to remove U16 Blk# and U16 BlkSize from 
              *pCnt = *pCnt - GBS_CKSUM_REC_START; 
              pStatus->cntBadCRCRow = 0; 
              pBlkData->cntBlkBadInRow = 0;  // Got good record
              if ((m_GBS_Ctl_Cfg.bBuffRecStore == FALSE) || 
                  ((m_GBS_Ctl_Cfg.bBuffRecStore == TRUE) && 
                   (pBuffManyRec->cnt >= m_GBS_Ctl_Cfg.cntBuffRecSize)) ||
                  (pStatus->dataBlkState.currBlkNum >= pStatus->cntBlkSizeExp))
              {
                *pDownloadData->bNewRec = TRUE; 
                pBlkData->state = GBS_BLK_STATE_SAVING; 
                // Reset timeout to wait for store complete
                pStatus->msCmdTimer = pStatus->cmdRsp_ptr->nRetryTimeOut + tick_ms;
                pStatus->nCmdRetriesCurr = pStatus->cmdRsp_ptr->nRetries; 
              }
              else // Rec Good, but don't store yet. Buff until cntCombineThresh met
              {
                *pStatus->ptrACK_NAK = GBS_RSP_ACK_DATA;
                if (++pBlkData->cntBlkCurr != pBlkData->currBlkNum) // Statistics
                {
                  pBlkData->cntRecOutSeq++; 
                  pBlkData->cntBlkCurr = (UINT16) pBlkData->currBlkNum; 
                }
                bOk = TRUE;
                GSE_DebugStr(NORMAL,TRUE, 
                  "GBS_Protocol: Record Ok, Buffering (Ch=%d,blk_rx=%d,blk_num=%d,sz=%d)\r\n",
                  (pStatus->multi_ch) ? GBS_SIM_PORT_INDEX : pStatus->ch, 
                  pBlkData->cntBlkRx, pBlkData->currBlkNum, pBlkData->currBlkSize);
              }
            } // end if GBS_VerifyChksum good
            pStatus->cntBadCRCRow = (bGoodData == FALSE) ? pStatus->cntBadCRCRow + 1 : 
                                                           pStatus->cntBadCRCRow;
          } // end if (*pCnt == pBlkData->currBlkSize) 
          
          if (bGoodData == FALSE) // Too Large, Too Small, Cksum Bad
          { // Reset timeout and indicate 1 bad rec in row
            pBlkData->cntBlkBadTotal++; 
            if (++pBlkData->cntBlkBadInRow >= ((m_GBS_Ctl_Cfg.retriesMulti + 1) * 
                                                GBS_TIMEOUT_RETRIES))
            { // Fail this retrieval !!! Just time out after 20 sec and "Restart" again
              pBlkData->bFailed = TRUE; 
              GSE_DebugStr(NORMAL,TRUE, 
                  "GBS_Protocol: _StateRecord Too Many Err. Timeout and restart (Ch=%d)\r\n",
                  (pStatus->multi_ch) ? GBS_SIM_PORT_INDEX : pStatus->ch);
               // If combined is enb, will flush combine buff and stats will be consistent
            }
            else if ((pBlkData->cntBlkBadInRow % (m_GBS_Ctl_Cfg.retriesSingle + 1)) == 0) {
              pBlkData->cntBlkBadRx++; 
              if ( (m_GBS_Ctl_Cfg.bKeepBadRec == FALSE) || 
                   ( (m_GBS_Ctl_Cfg.bKeepBadRec == TRUE) && 
                     (pStatus->cntBadCRCRow < m_GBS_Ctl_Cfg.retriesSingle)) )
              {
                bOk = TRUE;  // Force move onto next Rec
                *pStatus->ptrACK_NAK = GBS_RSP_ACK_DATA;
                GSE_DebugStr(NORMAL,TRUE, 
                  "GBS_Protocol: _StateRecord Bad, GetNext (Ch=%d,blk_rx=%d,blk_num=%d)\r\n",
                  (pStatus->multi_ch) ? GBS_SIM_PORT_INDEX : pStatus->ch, 
                  pBlkData->cntBlkRx, pBlkData->currBlkNum);
                pStatus->cntBadCRCRow = 0; 
                // If combine is enb, flush this rec from Combine Rx Buff
                pBuffManyRec->data_index = 0;  // Clear Mulit Rec Buff
              }
              else // Store bad rec if bKeepBadRec == TRUE
              {
                if (m_GBS_Ctl_Cfg.bBuffRecStore == TRUE) 
                {
                  pBuffManyRec->cnt = pBuffManyRec->cnt + pBuffManyRec->data_index; 
                  pBuffManyRec->data_index = 0; 
                  pBlkData->cntBlkRx++; 
                  pBlkData->cntBlkBuffering++; 
                }
                // Have to remove U16 Blk# and U16 BlkSize from 
                *pCnt = *pCnt - GBS_CKSUM_REC_START; 
                // Don't want to remove OH, because it might not be OH is thisc ase 
                // pBlkData->cntBlkBadInRow = 0;  // Got good record
                pStatus->cntBadCRCRow = 0; 
                *pDownloadData->bNewRec = TRUE; 
                pBlkData->state = GBS_BLK_STATE_SAVING; 
                // Reset timeout to wait for store complete
                pStatus->msCmdTimer = pStatus->cmdRsp_ptr->nRetryTimeOut + tick_ms;
                pStatus->nCmdRetriesCurr = pStatus->cmdRsp_ptr->nRetries; 
                GSE_DebugStr(NORMAL,TRUE, 
                  "GBS_Protocol: _StateRecord Bad, StoreRec (Ch=%d,blk_rx=%d,blk_num=%d)\r\n",
                  (pStatus->multi_ch) ? GBS_SIM_PORT_INDEX : pStatus->ch, 
                  pBlkData->cntBlkRx, pBlkData->currBlkNum);
              }
            }
            else { // Setup and send NAK.  Let outer loop send NAK.  
              bOk = TRUE;  // Force to resend this NAK
              *pStatus->ptrACK_NAK = GBS_RSP_NAK_DATA;
              GSE_DebugStr(NORMAL,TRUE, 
                      "GBS_Protocol: _StateRecord Send NAK (Ch=%d,blk_rx=%d,blk_num=%d)\r\n",
                      (pStatus->multi_ch) ? GBS_SIM_PORT_INDEX : pStatus->ch, 
                      pBlkData->cntBlkRx, pBlkData->currBlkNum);
              // If combine is enb, flush this rec from Combine Rx Buff
              pBuffManyRec->data_index = 0;  // Clear Mulit Rec Buff
            }
          } // end if (bGoodData == FALSE) 
          
        } // end if ( pBlkData->state == GBS_BLK_STATE_DATA )
        break; 
        
      case GBS_BLK_STATE_SAVING:
        if (*pDownloadData->bWriteInProgress == FALSE)
        {
          i = (m_GBS_Ctl_Cfg.bBuffRecStore == TRUE) ? pBlkData->cntBlkBuffering : 1; 
          pBlkData->cntStoreBad = (*pDownloadData->bWriteOk == FALSE) ?  
                                  (i + pBlkData->cntStoreBad) : pBlkData->cntStoreBad;
          pBlkData->cntBlkBuffering = 0; // Clear buffering cnt on store 
          pBuffManyRec->cnt = 0;         // Clear Multi Rec Buff
          pBuffManyRec->data_index = 0;  // Clear Mulit Rec Buff
          pBlkData->cntBlkRx = (m_GBS_Ctl_Cfg.bBuffRecStore == TRUE) ? 
                                pBlkData->cntBlkRx : (pBlkData->cntBlkRx + 1); 
          if (++pBlkData->cntBlkCurr !=  pBlkData->currBlkNum)
          {
            pBlkData->cntRecOutSeq++; 
            pBlkData->cntBlkCurr = (UINT16) pBlkData->currBlkNum; 
          }        
          *pDownloadData->bWriteInProgress = TRUE; // Similar to EMU150
          *pDownloadData->bWriteOk = FALSE; // Similar to EMU150
          // Store Good or Bad we move onto next Rec, but set flag
          bOk = TRUE;
          GSE_DebugStr(NORMAL,TRUE, 
                        "GBS_Protocol: Record Stored (Ch=%d,blk_rx=%d,blk_num=%d,sz=%d)\r\n",
                        (pStatus->multi_ch) ? GBS_SIM_PORT_INDEX : pStatus->ch, 
                        pBlkData->cntBlkRx, pBlkData->currBlkNum, pBlkData->currBlkSize);
          // Check if 20 sec timer has expired ? Not necessary.  
          //   If 20 sec expires and EDU -> Out of GBS mode, then will get failures
          //   which will Restart Dnload conn
          *pStatus->ptrACK_NAK = GBS_RSP_ACK_DATA;
        }
        break; 
        
      default: 
        FATAL("Invald GBS Protocol Rec Proccess state = %d", pBlkData->state); 
        break;
  
    } // end of switch (pBlkData->state) 
  }
  return (bOk);
}


/******************************************************************************
 * Function:    GBS_RestoreAppData
 *
 * Description: Retrieves the stored GBS NVM App Data from EEPROM
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static void GBS_RestoreAppData (void)
{
  RESULT result;


  result = NV_Open(NV_GBS_DATA);
  
  if ( result == SYS_OK ){
    NV_Read(NV_GBS_DATA, 0, (void *) &m_GBS_App_Data, sizeof(m_GBS_App_Data));
  }

  // If open failed or checksum bad (if not truncated data), then re-init app data
  if ( (result != SYS_OK) || (GBS_UNIT_APP_DATA == m_GBS_App_Data.pktCnt_Multi) ){
    GBS_FileInit();
  }
}


/******************************************************************************
 * Function:    GBS_RunningCksum
 *
 * Description: Utility to provide a running checksum count of the Rx Buffer
 *
 * Parameters:  ch - physical UART chan 
 *              type -  GBS_CKSUM_REC_START or GBS_CKSUM_BLKSIZE_START
 *
 * Returns:     None
 *
 * Notes:
 *  - GBS_CKSUM_REC_START cksum does not include OH (blk # and blk size)
 *    32-bit wide sum, then 2's complement
 *  - GBS_CKSUM_BLKSIZE_START 
 *    32-bit wide sum
 *
 *****************************************************************************/
static void GBS_RunningCksum (UINT16 ch, UINT16 type)
{
  UINT32 *pData, pos, bytes, i, *pCksum, words; 
  UINT16 start; 

  start = (type == GBS_CKSUM_REC_TYPE) ? GBS_CKSUM_REC_START : GBS_CKSUM_BLKSIZE_START; 
  
  // U16 Blk# and U16 Blk Size not incl in chksum calc
  pos = m_GBS_RxBuff[ch].cksum_pos + start;  
  pData = (UINT32 *) &m_GBS_RxBuff[ch].buff[pos]; 
  bytes = m_GBS_RxBuff[ch].cnt;
  pCksum = &m_GBS_RxBuff[ch].cksum; 

  if (bytes > (start + GBS_CKSUM_EXCL)) {
    // Determine # long words to process chksum
    words = (((bytes - GBS_CKSUM_EXCL) - pos) / sizeof(UINT32));  
    for (i=0;i<words;i++) {
      *pCksum = GBS_HTONL(*pData) + *pCksum; 
      pData++; 
    }
    m_GBS_RxBuff[ch].cksum_pos = m_GBS_RxBuff[ch].cksum_pos + (i * sizeof(UINT32)); 
  } // end if bytes > (start + GBS_CKSUM_EXCL)

}


/******************************************************************************
 * Function:    GBS_VerifyChkSum
 *
 * Description: Verifies the GBS checksum
 *
 * Parameters:  ch - physicall uart channel of Rx data
 *              type - GBS_CKSUM_REC_TYPE or GBS_CKSUM_BLKSIZE_TYPE
 *
 * Returns:     None
 *
 * Notes:
 *  - Uses m_GBS_RxBuff[] as source of data record to verify checksum
 *  - Verifies m_GBS_RxBuff[].cksum against last cksum (2nd to last long word in buff)
 *  - For GBS_CKSUM_REC_TYPE cksum is 2's complement
 *  - For GBS_CKSUM_BLKSIZE_TYPE cksum is "straight" (no modification)
 *
 *****************************************************************************/
static BOOLEAN GBS_VerifyChkSum ( UINT16 ch, UINT16 type )
{
  BOOLEAN bOk; 
  UINT32 cksumExp, cksumCalc, cnt; 
  UINT32 *pData; 
  
  
  bOk = FALSE; 
  cksumExp = 0; 
  cksumCalc = 0; 
  cnt = m_GBS_RxBuff[ch].cnt; 
  
  // worst case use GBS_CKSUM_REC_START instead of GBS_CKSUM_REC_TYPE
  if (cnt > (GBS_CKSUM_EXCL + GBS_CKSUM_REC_START))
  {
    pData = (UINT32 *) &m_GBS_RxBuff[ch].buff[cnt - GBS_CKSUM_EXCL];
    cksumExp = GBS_HTONL(*pData); 
    cksumCalc = (type == GBS_CKSUM_REC_TYPE) ? 
                ((m_GBS_RxBuff[ch].cksum ^ 0xFFFFFFFFUL) + 1) :
                m_GBS_RxBuff[ch].cksum; 
    
    bOk = (cksumExp == cksumCalc) ? TRUE : FALSE;                
  }

  if (bOk == FALSE) 
  {
    GSE_DebugStr(NORMAL,TRUE, "GBS_Protocol: Chksum Failed (exp=0x%08x, calc=0x%08x)",
                 cksumExp,cksumCalc);
  }

  return (bOk);
}


/******************************************************************************
 * Function:    GBS_CreateSummaryLog
 *
 * Description: Creates GBS download summary complete log
 *
 * Parameters:  pStatus - ptr to GBS_STATUS data for specific chan
 *              bSummary - TRUE to create summary log
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static void GBS_CreateSummaryLog ( GBS_STATUS_PTR pStatus, BOOLEAN bSummary )
{
  GBS_STATUS_LOG log; 
  SYS_APP_ID id; 
  
  memset ( (void *) &log, 0, sizeof(log) ); 
  
  // Blk and Summary Info
  log.nBlkExpected = pStatus->cntBlkSizeExp; 
  log.nBlkSizeTotal = pStatus->cntDnLoadSizeExp; 
  log.nBlkRx = pStatus->dataBlkState.cntBlkRx; 
  log.nBlkRxErr = pStatus->dataBlkState.cntBlkBadRx;
  log.nBlkRxErrTotal = pStatus->dataBlkState.cntBlkBadTotal;
  log.nBadStore = pStatus->dataBlkState.cntStoreBad; 
  if (bSummary == TRUE) {  // Summary log Info
    log.nRestarts = m_GBS_Ctl_Cfg.restarts - pStatus->nRetriesCurr; 
    log.bCompleted = pStatus->bCompleted; 
    log.bDownloadInterrupted = pStatus->bDownloadInterrupted; 
    log.bRelayStuck = pStatus->bRelayStuck; 
  }
  log.ch = (pStatus->multi_ch == TRUE) ? GBS_SIM_PORT_INDEX : pStatus->ch; 
  
#ifdef GBS_MULTI_DNLOADS
  /*vcast_dont_instrument_start*/
  id = bSummary ? SYS_ID_UART_GBS_STATUS : SYS_ID_UART_GBS_BLK_STATUS; 
  /*vcast_dont_instrument_end*/
#else
  id = SYS_ID_UART_GBS_STATUS; 
#endif  
  
  // Write the log
  LogWriteSystem (id, LOG_PRIORITY_LOW, &log, sizeof(log), NULL);
}


/******************************************************************************
 * Function:    GBS_CreateKeepAlive
 *
 * Description: Creates GBS Keep Alive Msg
 *
 * Parameters:  cmdRsp_ptr - ptr to GBS_CMD_RSP_PTR data for specific chan
 *
 * Returns:     None
 *
 * Notes:
 *  1) KeepAlive Msg Format
 *     Byte 0 - LRU Select "0x9B"
 *     Byte 1 - code byte  "0xFE"
 *     Byte 2-4 - GMT (Binary !!!!!!!)
 *                Byte 2: Bits 0-2 Not Used "0" 
 *                        Bits 3-7 seconds (5 bits) (1,2,4,8,16) 
 *                Byte 3: Bits 0   seconds (binary 32) 
 *                        Bits 1-6 minutes (6 bits) (1,2,4,8,16,32)
 *                        Bit  7   hrs (1 bit)  (1)
 *                Byte 4: Bits 0-3 hrs (4 bits) (2,4,8,16)
 *                        Bits 4-7 Not Used "0"
 *     Byte 5-7 - Date  (BCD !!!!!!!) 
 *                Byte 5: Bits 0-1 Not Used "0"
 *                        Bits 2-7 Yr   (1,2,4,8,10,20) 
 *                Byte 6: Bits 0-1 Yr   (40,80)
 *                        Bits 2-6 Mon  (1,2,4,8,10)
 *                        Bit  7   Days (1)
 *                Byte 7: Bits 0-4 Days (2,4,8,10,20)
 *                        Bits 5-7  Not Used "0"
 *      Byte 8 - Cksum (Sum of bytes 1 to 7 inclusive) 
 *
 *  2) If cmd_rsp_state == GBS_SETEDU_KEEPALIVE_ACK, then create Keep Alive msg
 *
 *****************************************************************************/
static void GBS_CreateKeepAlive ( GBS_CMD_RSP_PTR cmdRsp_ptr )
{
  TIMESTRUCT time_s; 
  GBS_KEEPALIVE_MSG msg; 
  UINT32 data; 
  UINT8 *byte_ptr, i; 
  UINT8 bcd_day, bcd_mon, bcd_yr; 
  
  
  if (cmdRsp_ptr->cmd_rsp_state == (UINT32) GBS_SETEDU_KEEPALIVE_ACK)
  {
    msg.selLRU = GBS_KA_LRU_SEL;
    msg.code = GBS_KA_CODE;
  
    CM_GetSystemClock ( (TIMESTRUCT *) &time_s ); 
    
    // Create GMT 
    // 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 
    // x x x s s s s s s m m  m  m  m  m  h  h  h  h  h  x  x  x  x
    data = (time_s.Second & 0x3F) | ((time_s.Minute & 0x3F) << 6) | 
           ((time_s.Hour & 0x1F) << 12 ); 
    data = data << 3; 
    byte_ptr = (UINT8 *) &data; // Big Endian 
    byte_ptr++; // Move past MSB of MSW since GBS_KA_GMT is only 3 bytes
    msg.gmt[2] = *byte_ptr++;  // Reverse the ordering, Big Endian to Little Endian
    msg.gmt[1] = *byte_ptr++;
    msg.gmt[0] = *byte_ptr; 
    
    // Create Date
    // 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23
    // x x y y y y y y y y m  m  m  m  m  d  d  d  d  d  d  x  x  x
    bcd_day = (UINT8) ((time_s.Day / 10) << 4) | (time_s.Day % 10);
    bcd_mon = (UINT8) ((time_s.Month / 10) << 4) | (time_s.Month % 10); 
    data = time_s.Year - GBS_KA_DATE_BASE_YR;
    bcd_yr = (UINT8) ((data / 10) << 4) | (data % 10); 
    data = bcd_yr | (bcd_mon << 8) | (bcd_day << 13); 
    data = data << 2; 
    byte_ptr = (UINT8 *) &data; 
    byte_ptr++; // Move past MSB of MSW since GBS_KA_DATE is only 3 bytes
    msg.date[2] = *byte_ptr++;  // Reverse the ordering, Big Endian to Little Endian
    msg.date[1] = *byte_ptr++; 
    msg.date[0] = *byte_ptr; 
    
    // Update Checksum 
    byte_ptr = (UINT8 *) &msg.code; 
    data = 0; 
    for (i=0;i<sizeof(GBS_KEEPALIVE_MSG) - GBS_KA_CKSUM_EXCL;i++) {
      data = *byte_ptr++ + data; 
    }
    msg.cksum = data & 0xFF;  // cksum is only 8 bits
    
    memcpy ( (UINT8 *) &cmdRsp_ptr->cmd[0], &msg, sizeof(msg) ); 
  }
}  


/******************************************************************************
 * Function:    GBS_ChkMultiInstall
 *
 * Description: When multiplex is enabled, verify that the 4 install records from 
 *              Primary and Second are not the same (should have diff ENG Serial, etc)
 *
 * Parameters:  pStatus -  ptr to GBS_STATUS data for specific chan
 *
 * Returns:     None
 *
 * Notes:       
 *  - Adds the 1st 4 rec (Install Records) if cfg to do Multi Relay Stuck check
 *  - If Install Rec are same for both EDU (based on checksum), then assume 
 *    Relay Stuck and fail the Dnload
 *
 *****************************************************************************/
static void GBS_ChkMultiInstall ( GBS_STATUS_PTR pStatus )
{
  GBS_MULTI_CHK_PTR pChk, pChkOther; 


  if ( (m_GBS_Ctl_Cfg.bChkMultiInstall == TRUE) && (pStatus->dataBlkState.bFailed == FALSE) &&
       (pStatus->dataBlkState.cntBlkRx <= GBS_MULTI_CHK_CNT) &&
       (m_GBS_Ctl_Cfg.bMultiplex == TRUE) && (pStatus->ch == m_GBS_Ctl_Cfg.nPort) )
  {
    pChk = &m_GBS_Multi_Ctl.chkSwitch[m_GBS_Multi_Ctl.state]; 
    if (pStatus->dataBlkState.cntBlkRx < GBS_MULTI_CHK_CNT)
    { // Continue adding the first 4 install Rec checksum
      pChk->cksum += m_GBS_RxBuff[pStatus->ch].cksum;
      pChk->cnt++; 
    }
    else 
    { // 4 Rec Received
      pChk->cksum = (pChk->cksum == 0) ? ~pChk->cksum : pChk->cksum; 
      pChkOther = (m_GBS_Multi_Ctl.state == GBS_MULTI_PRIMARY) ? 
                        &m_GBS_Multi_Ctl.chkSwitch[GBS_MULTI_SECONDARY] :
                        &m_GBS_Multi_Ctl.chkSwitch[GBS_MULTI_PRIMARY] ;
      if ( ((pChkOther->cksum != GBS_MULTI_CHK_INIT) && (pChkOther->cksum == pChk->cksum)) ||
           ((pChkOther->cksum == GBS_MULTI_CHK_INIT) && 
            (pChkOther->nvm_cksum != GBS_MULTI_CHK_INIT) && 
            (pChkOther->nvm_cksum == pChk->cksum)) )
      {
        pStatus->dataBlkState.bFailed = TRUE; 
        pStatus->bRelayStuck = TRUE; 
        GSE_DebugStr(NORMAL,TRUE, 
                "GBS_Protocol: Relay Stuck (Ch=%d)\r\n",
                (pStatus->multi_ch) ? GBS_SIM_PORT_INDEX : pStatus->ch);
      }
    } // end else 4 Rec Received
  } // end if check MultiInstall 
  
} 


/******************************************************************************
 * Function:    GBS_GetStatus
 *
 * Description: Utility function to request current GBS Status
 *
 * Parameters:  i - index into GBS_STATUS data for specific chan 
 *
 * Returns:     GBS_STATUS_PTR Ptr to GBS Status Data
 *
 * Notes:       None
 *
 *****************************************************************************/
static GBS_STATUS_PTR GBS_GetStatus (UINT16 i)
{
  return ( (GBS_STATUS_PTR) &m_GBS_Status[i] );
}


/******************************************************************************
 * Function:    GBS_GetSimStatus
 *
 * Description: Utility function to request current GBS Sim/Multi Ch Status
 *
 * Parameters:  None
 *
 * Returns:     GBS_STATUS_PTR Ptr to GBS Status Data
 *
 * Notes:       None
 *
 *****************************************************************************/
static GBS_STATUS_PTR GBS_GetSimStatus (void)
{
  return ( (GBS_STATUS_PTR) &m_GBS_Multi_Status );
}


/******************************************************************************
 * Function:    GBS_GetDebugDnload
 *
 * Description: Utility function to request current m_GBS_DebugDnload data
 *
 * Parameters:  i - index into m_GBS_DebugDnload data for specific chan
 *
 * Returns:     GBS_DEBUG_DNLOAD_PTR Ptr to m_GBS_DebugDnload Data
 *
 * Notes:       None
 *
 *****************************************************************************/
static GBS_DEBUG_DNLOAD_PTR GBS_GetDebugDnload (UINT16 i)
{
  return ( (GBS_DEBUG_DNLOAD_PTR) &m_GBS_DebugDnload[i] );
}


/******************************************************************************
 * Function:    GBS_GetCtlStatus
 *
 * Description: Utility function to request current GBS Multi Ctl Status
 *
 * Parameters:  None
 *
 * Returns:     GBS_MULTI_CTL_PTR Ptr to GBS Multi Ctl Status
 *
 * Notes:       None
 *
 *****************************************************************************/
static GBS_MULTI_CTL_PTR GBS_GetCtlStatus (void)
{
  return ( (GBS_MULTI_CTL_PTR) &m_GBS_Multi_Ctl );
}


/******************************************************************************
 * Function:    GBS_Dbg_RxData
 *
 * Description: Utility function to output Raw Rx Data to GSE for debugging
 *
 * Parameters:  pStatus - ptr to GBS_STATUS data for specific chan
 *              *data - ptr to Rx Raw Data
 *              cnt - number of bytes in Rx Raw Data
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static void GBS_Dbg_RxData (GBS_STATUS_PTR pStatus, UINT8 *data, UINT16 cnt)
{
  UINT16 i;   
  
  if ((cnt > 0) && (pStatus->dataBlkState.cntBlkCurr == 0))
  {
    for (i=0;(i<cnt) && (i<GBS_TX_RX_CHAR_MAX);i++)
    {
      sprintf( (char *) &m_gbs_rx_buff[i * 3], "%02x ", *(data + i) );
    }
    m_gbs_rx_buff[i * 3] = '\0'; // NULL
    GSE_DebugStr(NORMAL,TRUE,"GBS Protocol: Rx (Ch=%d,Cnt=%d,Data=%s)\r\n", 
                 ((pStatus->multi_ch) ? GBS_SIM_PORT_INDEX : pStatus->ch), cnt,
                 m_gbs_rx_buff);
  }
}


/******************************************************************************
 * Function:    GBS_Dbg_TxData
 *
 * Description: Utility function to output Raw Tx Data to GSE for debugging
 *
 * Parameters:  pStatus - ptr to GBS_STATUS data for specific chan
 *              ch - uart channel 
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static void GBS_Dbg_TxData (GBS_STATUS_PTR pStatus, UINT16 ch)
{
  UINT16 i; 
  
  for (i=0;(i<m_GBS_TxBuff[ch].cnt) && (i<GBS_TX_RX_CHAR_MAX);i++)
  {
    sprintf( (char *) &m_gbs_tx_buff[i * 3], "%02x ", m_GBS_TxBuff[ch].buff[i] );
  }
  m_gbs_tx_buff[i * 3] = '\0'; // NULL
  GSE_DebugStr(NORMAL,TRUE,"GBS Protocol: Tx (Ch=%d,Cnt=%d,Data=%s)\r\n", 
               ((pStatus->multi_ch) ? GBS_SIM_PORT_INDEX : pStatus->ch), m_GBS_TxBuff[ch].cnt,
               m_gbs_tx_buff);
}


/*****************************************************************************
 *  MODIFICATIONS
 *    $History: GBSProtocol.c $
 * 
 * *****************  Version 25  *****************
 * User: Peter Lee    Date: 3/26/16    Time: 11:40p
 * Updated in $/software/control processor/code/system
 * SCR #1324.  Add additional GBS Protocol Configuration Items.  Fix minor
 * bug with ">" vs ">=". 
 * 
 * *****************  Version 24  *****************
 * User: Peter Lee    Date: 15-04-01   Time: 1:44p
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol.  
 * 18) From V&V, for storing 'bad' rec, remove the Blk # and Blk Size to
 * be consistent with storing good rec process.
 * 
 * *****************  Version 23  *****************
 * User: Peter Lee    Date: 15-04-01   Time: 11:26a
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol.  
 * 
 * 17) From V&V statement coverage
 * a) Break w/o FATAL
 * 
 * 
 * *****************  Version 22  *****************
 * User: Peter Lee    Date: 15-03-31   Time: 6:00p
 * Updated in $/software/control processor/code/system
 * SCR #1255  GBS Protocol Implementation. 
 * 
 * 16) From V&V.  Don't send ACK/NAK during IDLE Timeout Period.  
 * 
 * *****************  Version 21  *****************
 * User: Peter Lee    Date: 15-03-30   Time: 10:34a
 * Updated in $/software/control processor/code/system
 * Code Review Updates
 * 
 * *****************  Version 20  *****************
 * User: Peter Lee    Date: 15-03-25   Time: 7:22p
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol Updates
 * 
 * a) SDD Review AI #6. The ->cntStoreBad is incorrectly updated.
 * Currently "=1" but s/b "=+1" for each occurrence. 
 * b) SDD Review AI #13.  PRIMARY s/b ACTION_OFF, and SECONDARY s/b
 * ACTION_ON. 
 * c) SDD Review AI #13, After SECONDARY retrieval switch back to PRIMARY
 * to set ACTION_OFF. 
 * 
 * *****************  Version 19  *****************
 * User: Peter Lee    Date: 15-03-24   Time: 5:29p
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol.  Remove extra comment deliminters from footer
 * 
 * *****************  Version 18  *****************
 * User: Peter Lee    Date: 15-03-24   Time: 5:25p
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol.  Remove extra comment from footer history. 
 * 
 * *****************  Version 17  *****************
 * User: Peter Lee    Date: 15-03-24   Time: 5:04p
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol
 * 
 * 14) V&V findings and GSE/LOG Review AI
 * a) Remove GSE-4067, 4068
 * gbs_ctl.status.multi.primary_pend/secondary_pend from AI #8 of GSE
 * review. 
 * b) Update"cnt" to "size" for LOG-2831
 * c) Add vcast_dont_instrument_start
 * d) Fix 'BAD' record in row before Restart
 * 
 * *****************  Version 16  *****************
 * User: Peter Lee    Date: 15-03-23   Time: 11:34a
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol.  Code review changes. 
 * 
 * *****************  Version 15  *****************
 * User: Peter Lee    Date: 15-03-17   Time: 4:47p
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol.
 * 
 * 13) V&V Finding
 * a) SRS-4883 "bad record stored... in row".  Implemented is "bad record
 * retrieved.. in row", where each bad record stored involves 1 tre and 3
 * retries,  vs just retries in row. 
 * 
 * *****************  Version 14  *****************
 * User: Peter Lee    Date: 15-03-13   Time: 4:17p
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol.  
 * V&V findings: 
 * 
 * c) Call to GBS_FLUSH_RXBUFF needed if Blk Info pkt fails chksum.  Else,
 * cksum buffer 3x in Row (10ms each) and exhausts the retries and don't
 * send NAK. 
 * 
 * 
 * *****************  Version 13  *****************
 * User: Peter Lee    Date: 15-03-09   Time: 9:54p
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol.  Updates, IDLE 20 after BlkReq failed.  Update
 * Retries from 4 to 3. 
 * 
 * *****************  Version 12  *****************
 * User: Peter Lee    Date: 15-03-09   Time: 9:30p
 * Updated in $/software/control processor/code/system
 * Code Review Updates
 * 
 * *****************  Version 11  *****************
 * User: Peter Lee    Date: 15-03-02   Time: 6:36p
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol.  
 * 9) Updates 
 * a) comment out #define DTU_GBS_SIM 1
 * b) keep pad[2] for GBS_CMD_REQ_BLK as tested on 2000EX
 * c) vcast_dont_instrument for converage
 *
 * *****************  Version 10  *****************
 * User: Peter Lee    Date: 15-02-18   Time: 11:42a
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol.  
 * c) Update Summary Log fields (retries, ch)
 * 
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 15-02-17   Time: 5:06p
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol. 
 * b) KeepAlive updates, 
 * - fix checksum at end of data
 * - minutes field incorrect
 * - base yr as confirmed on UTFlight is 2000 not 1900.  
 *
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 15-02-17   Time: 1:06p
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol.  Update to don't send ACK when waiting for 20
 * sec IDLE timeout (to force EDU to exist GBS mode) when download failed.
 * 
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 15-02-06   Time: 7:17p
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol.  Additional Update Item #7. 
 * 
 * *****************  Version 6  *****************
 * User: Peter Lee    Date: 15-02-03   Time: 4:48p
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol, fix dio out for Relay Switch control.
 * 
 * *****************  Version 5  *****************
 * User: Peter Lee    Date: 15-02-03   Time: 4:46p
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol, fix dio out for Relay Switch control. 
 *  
 * *****************  Version 4  *****************
 * User: Peter Lee    Date: 15-02-03   Time: 2:53p
 * Updated in $/software/control processor/code/system
 * SCR #1255, GBS Protocol.   Update "m_GBS_Debug.DTU_GBS_SIM_SimLog" to
 * FALSE rather then TRUE. 
 *  
 * *****************  Version 3  *****************
 * User: Peter Lee    Date: 15-01-19   Time: 6:18p
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol Updates 
 * 1) LSS output control
 * 2) KeepAlive Msg
 * 3) Dnload code loop thru
 * 4) Debug Dnload Code
 * 5) Buffer Multi Records before storing 
 * 
 * *****************  Version 2  *****************
 * User: Peter Lee    Date: 15-01-11   Time: 10:43p
 * Updated in $/software/control processor/code/system
 * SCR #1255 GBS Protocol.  Remove test code. 
 * 
 * *****************  Version 1  *****************
 * User: Peter Lee    Date: 15-01-11   Time: 10:21p
 * Created in $/software/control processor/code/system
 * SCR #1255 GBS Protocol 
 *
 *****************************************************************************/
