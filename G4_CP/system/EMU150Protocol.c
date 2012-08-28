#define EMU150_PROTOCOL_BODY

/******************************************************************************
            Copyright (C) 2008-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:        EMU150Protocol.c

    Description: Contains all functions and data related to the EMU150 Protocol 
                 Handler 
    
    VERSION
      $Revision: 17 $  $Date: 10/06/11 11:39a $     

******************************************************************************/



/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/



/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "EMU150Protocol.h"
#include "ClockMgr.h"
#include "GSE.h"
#include "NVMgr.h"
#include "Assert.h"



/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define EMU150_RAW_RX_BUFF_MAX (128 * 1024)
#define EMU150_RAW_TX_BUFF_MAX (128)
#define EMU150_RECORDS_MAX (1000 * 4)

#define EMU150_CMD_HS  0x12
#define EMU150_CMD_ACK 0x06

#define EMU150_CMD_GET_INSTALL_CFG 0x5C
#define EMU150_CMD_RECORDING_HDR   0x56
#define EMU150_CMD_GET_RECORD      0x57
#define EMU150_CMD_RECORD_CONFIRM  0x58
#define EMU150_CMD_SET_BAUD_RATE   0x1F

#define EMU150_CMD_BAUD_RATE       0x1F
#define EMU150_BAUD_RATE_9600      1
#define EMU150_BAUD_RATE_19200     2
#define EMU150_BAUD_RATE_38400     3
#define EMU150_BAUD_RATE_57600     4
#define EMU150_BAUD_RATE_76800     5
#define EMU150_BAUD_RATE_115200    6


#define HTONL(A)  ((((A) & 0xff000000) >> 24) | \
                   (((A) & 0x00ff0000) >>  8) | \
                   (((A) & 0x0000ff00) <<  8) | \
                   (((A) & 0x000000ff) << 24))


/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
typedef struct {
  UINT32 cnt;
  UINT32 cksum_cnt_ptr; 
  UINT32 cksum; 
  UINT32 cksum_misaligned_flag; 
  UINT8 cksum_misaligned[4]; 
  UINT8 buff[EMU150_RAW_RX_BUFF_MAX]; // This must be long word aligned !
} EMU150_RAW_RX_BUFFER; 

typedef struct {
  UINT32 cnt;
  UINT8 buff[EMU150_RAW_TX_BUFF_MAX];
} EMU150_RAW_TX_BUFFER; 

typedef struct {
  UINT8 cmd;
  UINT32 timeout; 
} EMU150_CMDS_STRUCT;

typedef BOOLEAN (*RSP_HNDL) (EMU150_BLK_STATES blk_state, UINT8 *data, UINT32 cnt, UINT16 ch ); 


#pragma pack(1)
typedef struct {
  UINT16 nRecType; 
  UINT16 nCode;
  UINT32 nRecTime; 
  UINT8  nCycle;
} EMU150_RECCORDING_HDR, *EMU150_RECCORDING_HDR_PTR;
#pragma pack()



/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static EMU150_STATUS m_EMU150_Status; 
static EMU150_RAW_RX_BUFFER m_EMU150_Rx_Buff; 
static EMU150_RAW_TX_BUFFER m_EMU150_Tx_Buff; 
static EMU150_RAW_RX_BUFFER m_EMU150_Rx_Record; 
static EMU150_RECCORDING_HDR m_Record_Hdr[EMU150_RECORDS_MAX]; 
RSP_HNDL EMU150_RSP[EMU150_STATE_MAX]; 

static UART_CONFIG m_UartCfg; 

static char GSE_OutLine[128]; 

static EMU150_APP_DATA m_EMU150_AppData; 

static EMU150_DOWNLOAD_DATA m_EMU150_Download_Ptr;
static EMU150_DOWNLOAD_DATA_LC m_EMU150_Download; 

static EMU150_CFG m_EMU150_Cfg; 

static BOOLEAN bStartup; 


/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
#include "EMU150UserTables.c"

static EMU150_STATES EMU150_State_ProcessCmd( UINT8 *data, UINT16 cnt, UINT16 ch );
static EMU150_STATES EMU150_State_AutoDetect ( UINT8 *data, UINT32 cnt, UINT16 ch);

static BOOLEAN EMU150_Process_InstallCfg (EMU150_BLK_STATES blk_state, UINT8 *data, UINT32 cnt, UINT16 ch);
static BOOLEAN EMU150_Process_Recording_Hdr (EMU150_BLK_STATES blk_state, UINT8 *data, UINT32 cnt, UINT16 ch);
static BOOLEAN EMU150_Process_Record (EMU150_BLK_STATES blk_state, UINT8 *data, UINT32 cnt, UINT16 ch);
static BOOLEAN EMU150_Process_Record_Confirmation (EMU150_BLK_STATES blk_state, UINT8 *data, UINT32 cnt, UINT16 ch);
static BOOLEAN EMU150_Process_Set_Baud (EMU150_BLK_STATES blk_state, UINT8 *data, UINT32 cnt, UINT16 ch);


static void EMU150_CreateTxPacket( UINT8 cnt, UINT8 *data, UINT32 timeout );
static void EMU150_AppendRecordId( void ); 
static void EMU150_AppendBaudRate( void ); 

static BOOLEAN EMU150_Process_Blk (EMU150_BLK_STATES blk_state, UINT8 *data, UINT32 cnt, UINT16 ch); 
static BOOLEAN EMU150_Process_ACK (EMU150_BLK_STATES blk_state, UINT8 *data, UINT32 cnt, UINT16 ch); 
static void EMU150_NextCmdState ( EMU150_CMD_STATES state, BOOLEAN bClrRetries ); 

static void EMU150_NextState( void ); 
static UINT16 EMU150_GetDesiredBaudRateIndex (UINT32 UartBPS_Desired); 

static void EMU150_SetUart ( UINT16 ch, UART_CFG_BPS bps );

static void EMU150_ProcessChksum ( void ); 

static void EMU150_RestoreAppData ( void ); 
static UINT32 EMU150_CalcCksum ( UINT32 *data_ptr,  UINT32 nLongWords); 

static void EMU150_GetCksumStatus ( UINT8 *data_ptr, UINT32 *cksum, UINT32 *status ); 
static void EMU150_CreateSummaryLog ( EMU150_STATUS_PTR pStatus ); 


/*****************************************************************************/
/* Constant Data                                                             */
/*****************************************************************************/
const EMU150_CMDS_STRUCT EMU150_CMDS[EMU150_STATE_MAX] = {
  // EMU150_STATE_IDLE  
  {NULL, EMU150_TIMEOUT},         
  
  // EMU150_STATE_AUTODETECT
  {EMU150_CMD_BAUD_RATE, EMU150_TIMEOUT},  
  
  // EMU150_STATE_SET_BAUD
  {EMU150_CMD_SET_BAUD_RATE, EMU150_TIMEOUT},
  
  // EMU150_STATE_GET_INSTALL_CFG
  {EMU150_CMD_GET_INSTALL_CFG, EMU150_TIMEOUT},  
  
  // EMU150_STATE_GET_RECORDING_HDR
  {EMU150_CMD_RECORDING_HDR, EMU150_TIMEOUT_RECORDING_HDR},  
  
  // EMU150_STATE_GET_RECORDING
  {EMU150_CMD_GET_RECORD, EMU150_TIMEOUT},  
  
  // EMU150_STATE_RECORDING_CONFIGRMATION
  {EMU150_CMD_RECORD_CONFIRM, EMU150_TIMEOUT},  
  
  // EMU150_STATE_WAIT_FOR_STORAGE_CONFIRMATION
  {NULL, EMU150_TIMEOUT},         
  
  // EMU150_STATE_COMPLETED
  {NULL, EMU150_TIMEOUT},         

};

const UINT8 EMU150_CMDS_HS = EMU150_CMD_HS;
const UINT8 EMU150_CMDS_ACK = EMU150_CMD_ACK;

#define EMU150_STATE_NAMES_MAX 32
typedef struct {
  char state[EMU150_STATE_NAMES_MAX]; 
} EMU150_NAMES; 

const EMU150_NAMES EMU150_NAME[EMU150_STATE_MAX] = 
{
  "_IDLE",               // EMU150_STATE_IDLE
  "_AUTODETECT",         // EMU150_STATE_AUTODETECT
  "_SET_BAUD_RATE",      // EMU150_STATE_SET_BAUD
  "_GET_INSTALL_CFG",    // EMU150_STATE_GET_INSTALL_CFG
  "_GET_REC_HDR",        // EMU150_STATE_GET_RECORDING_HDR
  "_GET_REC",            // EMU150_STATE_GET_RECORDING
  "_REC_CONFIRM",        // EMU150_STATE_RECORDING_CONFIGRMATION
  "_REC_WAIT_STORED",    // EMU150_STATE_WAIT_FOR_STORAGE_CONFIRMATION
  "_COMPLETED"           // EMU150_STATE_COMPLETED
};

#define EMU150_BAUD_RATES_MAX 5
const UART_CFG_BPS UART_BPS_SUPPORTED[EMU150_BAUD_RATES_MAX] = 
{
  UART_CFG_BPS_9600, 
  UART_CFG_BPS_19200, 
  UART_CFG_BPS_38400,
  UART_CFG_BPS_57600, 
  UART_CFG_BPS_115200
}; 

const UINT8 EMU150_BAUD_RATES[EMU150_BAUD_RATES_MAX] = 
{
  1,  // 9600  Must match Index to EMU150_BPS_SUPPORTED[] above 
  2,  // 19200
  3,  // 38400
  4,  // 57600
  6   // 115200
};



/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:    EMU150Protocol_Initialize
 *
 * Description: Initializes the EMU150 Protocol functionality 
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
void EMU150Protocol_Initialize ( void ) 
{
   m_EMU150_Status.state = EMU150_STATE_IDLE; 
   m_EMU150_Status.cmd_state = EMU150_CMD_STATE_SEND_HS; 
   m_EMU150_Status.cmd_timer = 0; 
   m_EMU150_Status.cmd_timeout = EMU150_TIMEOUT; 
   m_EMU150_Status.nRetries = 0; 
   m_EMU150_Status.nRestarts = 0;
   m_EMU150_Status.nIdleRetries = 0; 
   m_EMU150_Status.nCksumErr = 0;
   m_EMU150_Status.nTotalRetries = 0;  
   m_EMU150_Status.nRecordsRec = 0; 
   m_EMU150_Status.nRecordsRecTotalByte = 0;
   
   m_EMU150_Status.nRecords = 0; 
   m_EMU150_Status.nRecordsNew = 0; 
   m_EMU150_Status.nRecordsRxBad = 0; 
   m_EMU150_Status.nCurrRecord = 0; 
   
   m_EMU150_Status.ack_timer = 0; 

   m_EMU150_Status.port = UART_NUM_OF_UARTS;  // Init to No Ports allocated !
   
   m_EMU150_Rx_Buff.cnt = 0; 
   m_EMU150_Tx_Buff.cnt = 0; 
   m_EMU150_Rx_Buff.cksum_misaligned_flag = 0; 
     
   EMU150_RSP[EMU150_STATE_IDLE] = EMU150_Process_ACK;        // NOP State
   EMU150_RSP[EMU150_STATE_AUTODETECT] = EMU150_Process_ACK;  // NOP State
   EMU150_RSP[EMU150_STATE_GET_INSTALL_CFG] = EMU150_Process_InstallCfg; 
   EMU150_RSP[EMU150_STATE_GET_RECORDING_HDR] = EMU150_Process_Recording_Hdr; 
   EMU150_RSP[EMU150_STATE_GET_RECORDING] = EMU150_Process_Record; 
   EMU150_RSP[EMU150_STATE_RECORDING_CONFIGRMATION] = EMU150_Process_Record_Confirmation; 
   EMU150_RSP[EMU150_STATE_WAIT_FOR_STORAGE_CONFIRMATION] = EMU150_Process_ACK;  // NOP State
   EMU150_RSP[EMU150_STATE_SET_BAUD] = EMU150_Process_Set_Baud; 
   EMU150_RSP[EMU150_STATE_COMPLETED] = EMU150_Process_ACK; // NOP State
   
   m_EMU150_Status.bAllRecords = FALSE; 
   m_EMU150_Status.bDownloadFailed = FALSE; 
   
   User_AddRootCmd(&EMU150ProtocolRootTblPtr); 
   
   m_EMU150_Status.IndexBPS = 0; 
   m_EMU150_Status.UartBPS = UART_BPS_SUPPORTED[m_EMU150_Status.IndexBPS]; 
   m_EMU150_Status.UartBPS_Desired = 0;  // Set to "uninitialized" as rep by '0'
   
   m_EMU150_Status.nTotalBytes = 0;         
   
   // Restore App Data
   EMU150_RestoreAppData(); 
   
   // Retrieve EMU150 Cfg 
   memcpy(&m_EMU150_Cfg, &(CfgMgr_RuntimeConfigPtr()->EMU150Config), sizeof(EMU150_CFG));

   
   // Initialize pointer / temp vars 
   m_EMU150_Download.bStartDownload = FALSE;
   m_EMU150_Download.bDownloadCompleted = FALSE;
   m_EMU150_Download.bWriteInProgress = TRUE; 
   m_EMU150_Download.bWriteOk = FALSE; 
   m_EMU150_Download.bHalt = FALSE; 
   m_EMU150_Download.bNewRec = FALSE; 
   
   m_EMU150_Download_Ptr.bStartDownload = &m_EMU150_Download.bStartDownload; 
   m_EMU150_Download_Ptr.bDownloadCompleted = &m_EMU150_Download.bDownloadCompleted; 
   m_EMU150_Download_Ptr.bWriteInProgress = &m_EMU150_Download.bWriteInProgress; 
   m_EMU150_Download_Ptr.bWriteOk = &m_EMU150_Download.bWriteOk; 
   m_EMU150_Download_Ptr.bHalt = &m_EMU150_Download.bHalt; 
   m_EMU150_Download_Ptr.bNewRec = &m_EMU150_Download.bNewRec; 
   
   bStartup = TRUE; 
}


/******************************************************************************
 * Function:    EMU150Protocol_Handler
 *
 * Description: Initializes the EMU150 Protocol functionality 
 *
 * Parameters:  data - ptr to received data to be processed 
 *              cnt - count of received data
 *              ch - uart channel to be processed
 *              runtime_data_ptr - not used
 *              info_ptr - not used 
 *
 * Returns:    TRUE always 
 *
 * Notes:       - Only one ch will be support due to limited EEPROM size 
 *                to accomodate EMU150 Cfg Install (~ 5KB / ch )
 *
 *****************************************************************************/
BOOLEAN EMU150Protocol_Handler ( UINT8 *data, UINT16 cnt, UINT16 ch, 
                                 void *runtime_data_ptr, void *info_ptr )
{
  EMU150_STATES prevState; 
  RESULT result; 
  UINT16 sent_cnt; 
  EMU150_CFG_PTR pEMU150_Cfg; 
  BOOLEAN bGenerateLog;  
  

  // If EMU150 Protocol not setup during init, then any runtime request are not allowed.
  // If EMU150 Protocol is setup, only the ch requested during init can be processed.
  ASSERT ((m_EMU150_Status.port != UART_NUM_OF_UARTS) && (m_EMU150_Status.port == ch));
  
  pEMU150_Cfg = &m_EMU150_Cfg; 
  prevState = m_EMU150_Status.state;

  bGenerateLog = FALSE; 
  
  if ( *m_EMU150_Download_Ptr.bHalt == TRUE ) 
  {
    // Did we get interrupted ? 
    if (m_EMU150_Status.state != EMU150_STATE_IDLE) 
    {
      m_EMU150_Status.bDownloadInterrupted = TRUE; 
      bGenerateLog = TRUE; 
    }
    
    // Force back to IDLE state
    m_EMU150_Status.state = EMU150_STATE_IDLE; 
    *m_EMU150_Download_Ptr.bHalt = FALSE; 
    
    // Output msg 
    sprintf (GSE_OutLine,
             "\r\nEMU150 Protocol: Download Halt Encountered ! \r\n \0");
    GSE_DebugStr(NORMAL,TRUE,GSE_OutLine); 
  }

  switch (m_EMU150_Status.state)
  {
    case EMU150_STATE_IDLE:
      // Do nothing.  Wait for transition to EMU Log Retrieval cmd from application
      if ( *m_EMU150_Download_Ptr.bStartDownload == TRUE ) 
      {
        m_EMU150_Status.state = EMU150_STATE_AUTODETECT; 
        m_EMU150_Status.bDownloadFailed = FALSE;
        m_EMU150_Status.nRestarts = 0; 
        m_EMU150_Status.nTotalRetries = 0; 
        m_EMU150_Status.nCksumErr = 0; 
        m_EMU150_Status.bDownloadInterrupted = FALSE; 
        m_EMU150_Status.nRecordsRec = 0; 
        m_EMU150_Status.nRecordsRecTotalByte = 0; 
        EMU150_NextState(); 
        m_EMU150_Status.nTotalBytes = 0;         
        *m_EMU150_Download_Ptr.bStartDownload = FALSE; // Clear the one-shot cmd 
        
        if (bStartup == TRUE) 
        {
          // Reset the Uart to 9600 
          m_EMU150_Status.IndexBPS = 0; 
          EMU150_SetUart ( ch, UART_BPS_SUPPORTED[m_EMU150_Status.IndexBPS] ); 
          bStartup = FALSE; 
        }
      }
      break;
      
    case EMU150_STATE_AUTODETECT:
      if ( prevState != EMU150_State_AutoDetect( data, cnt, ch))
      {
        // Change of State 
        EMU150_NextState(); 
      }
      m_EMU150_Status.nIdleRetries = 0;   // Expect no response during AutoDetect().  Clear this flag.
                                          //   _AutoDetect() has code handle no response. 
      break; 
      
    case EMU150_STATE_GET_INSTALL_CFG:
    case EMU150_STATE_GET_RECORDING_HDR:
    case EMU150_STATE_SET_BAUD: 
      if ( prevState != EMU150_State_ProcessCmd( data, cnt, ch)) 
      {
        // Change of State 
        EMU150_NextState(); 
      }
      break; 
      
    // Loop until all records have been retrieved 
    case EMU150_STATE_GET_RECORDING:
    case EMU150_STATE_RECORDING_CONFIGRMATION:
      if ( prevState != EMU150_State_ProcessCmd( data, cnt, ch)) 
      {
        if ( prevState == EMU150_STATE_RECORDING_CONFIGRMATION )
        {
          // Determine if we have retrieved all the records 
          if ( m_EMU150_Status.nCurrRecord >= m_EMU150_Status.nRecordsNew ) 
          {
            // We are complete
            // Set flag to indicate completed and trans to IDLE
            m_EMU150_Status.state = EMU150_STATE_COMPLETED; 
          }
        }
        // Change of State 
        EMU150_NextState(); 
        
        if ( (prevState == EMU150_STATE_GET_RECORDING) && 
             (m_EMU150_Status.state != EMU150_STATE_COMPLETED) )
        {
          *m_EMU150_Download_Ptr.bNewRec = TRUE; 
        }
      }
      break;
      
    case EMU150_STATE_WAIT_FOR_STORAGE_CONFIRMATION:
      if ( *m_EMU150_Download_Ptr.bWriteInProgress == FALSE ) 
      {
        if ( *m_EMU150_Download_Ptr.bWriteOk == TRUE )
        {
          if (pEMU150_Cfg->bMarkDownloaded == TRUE) 
          {
            m_EMU150_Status.state = EMU150_STATE_RECORDING_CONFIGRMATION; 
          }
        }
        
        // If Record Storage failed OR Don't confirm  
        if ( m_EMU150_Status.state != EMU150_STATE_RECORDING_CONFIGRMATION ) 
        {
          // Move to next record.  Update bad record recording 
          if ( ++m_EMU150_Status.nCurrRecord >= m_EMU150_Status.nRecordsNew ) 
          {
            // We are complete
            // Set flag to indicate completed and trans to IDLE
            m_EMU150_Status.state = EMU150_STATE_COMPLETED; 
          }
          else 
          {
            m_EMU150_Status.state = EMU150_STATE_GET_RECORDING; 
          }
        }
        
        // Change of State 
        EMU150_NextState();
        *m_EMU150_Download_Ptr.bWriteInProgress = TRUE; // Force flag to InProgress
        *m_EMU150_Download_Ptr.bWriteOk = FALSE;        // Force flag to be Write FAILED as default 
      }
      m_EMU150_Status.cmd_timer = CM_GetTickCount();   // Reset timer
      m_EMU150_Status.nIdleRetries = 0; 
      break; 
            
    case EMU150_STATE_COMPLETED:
      // If we have failed and have not exceeded Reboot attempts, try again. 
      if ( (m_EMU150_Status.bDownloadFailed == TRUE) && (m_EMU150_Status.nRestarts < pEMU150_Cfg->nRestarts) )
      {
        // Make another attempt from the start 
        m_EMU150_Status.state = EMU150_STATE_AUTODETECT; 
        m_EMU150_Status.bDownloadFailed = FALSE;
        // Reset the Uart to 9600 
        m_EMU150_Status.IndexBPS = 0; 
        EMU150_SetUart ( ch, UART_BPS_SUPPORTED[m_EMU150_Status.IndexBPS] ); 

        sprintf (GSE_OutLine,
                 "\r\nEMU150 Protocol: Error Encountered.  Re-Attempting Download (n=%d)\r\n \0", 
                 m_EMU150_Status.nRestarts);
        GSE_DebugStr(NORMAL,TRUE,GSE_OutLine); 
        m_EMU150_Status.nRestarts++; 
      }
      else 
      {
        if (m_EMU150_Status.bDownloadFailed == TRUE) 
        {
          // Completed with Errors
          sprintf (GSE_OutLine,
                   "\r\nEMU150 Protocol: Error Encountered.  Download Completed (n=%d)\r\n \0", 
                   m_EMU150_Status.nRestarts);
        }
        else 
        {
          // Completed w/o Errors
          sprintf (GSE_OutLine,
                   "\r\nEMU150 Protocol: Download Completed (n=%d)\r\n \0", 
                   m_EMU150_Status.nRestarts);
        }
        GSE_DebugStr(NORMAL,TRUE,GSE_OutLine); 
        m_EMU150_Status.state = EMU150_STATE_IDLE; 
        bGenerateLog = TRUE; 
      }
      EMU150_NextState(); 
      break; 
      
    default:
      FATAL("Unsupported EMU State = %d", m_EMU150_Status.state);    
      break;      
  }
  
  // If not IDLE state, have overall 2.5 or 10.5 second timeout 
  if ( (m_EMU150_Status.state != EMU150_STATE_IDLE) && (m_EMU150_Status.state != EMU150_STATE_COMPLETED) )
  {
    if (cnt == 0) 
    {
      //if ( (m_EMU150_Status.cmd_timer + m_EMU150_Status.cmd_timeout) < CM_GetTickCount() )
      if ( (CM_GetTickCount() - m_EMU150_Status.cmd_timer) > m_EMU150_Status.cmd_timeout )
      {
      
        m_EMU150_Status.cmd_timer = CM_GetTickCount();   // Reset timer
        m_EMU150_Status.nRetries++;       // Update retries 
        m_EMU150_Status.nTotalRetries++;
        m_EMU150_Status.cmd_state = EMU150_CMD_STATE_SEND_HS;  
      
        if (m_EMU150_Status.nIdleRetries++ >= pEMU150_Cfg->nIdleRetries)
        {
          // No Response from EMU 150 for several seconds, try rebooting ! 
          m_EMU150_Status.bDownloadFailed = TRUE;
          m_EMU150_Status.state = EMU150_STATE_COMPLETED; 
          m_EMU150_Status.nIdleRetries = 0; 
        }
        else 
        {
          if ( m_EMU150_Status.nRetries <= pEMU150_Cfg->nRetries ) 
          {
            sprintf (GSE_OutLine,
                     "\r\nEMU150 Protocol: Cmd Failed/Timeout Retry (%d) State: %s \r\n \0", 
                     m_EMU150_Status.nRetries,
                     EMU150_NAME[m_EMU150_Status.state].state );
            GSE_DebugStr(NORMAL,TRUE,GSE_OutLine);
          }
        }
      }
    }
    else 
    {
      m_EMU150_Status.cmd_timer = CM_GetTickCount();     // Reset timer 
      m_EMU150_Status.nIdleRetries = 0; 
    }
    
    // Transmit any data pending data 
    if (m_EMU150_Tx_Buff.cnt > 0)
    {
      result = UART_Transmit ( m_EMU150_Status.port, (const INT8*) &m_EMU150_Tx_Buff.buff[0], 
                               m_EMU150_Tx_Buff.cnt, &sent_cnt );
      // Note: if result is != OK and nSent != nCnt allow this current action to timeout 
      //   (2.5/10.5 sec) and start over again 
      if (result == DRV_OK)
      {
        m_EMU150_Tx_Buff.cnt = 0; 
      }
    }
    
    // Keeping some statitics 
    m_EMU150_Status.nTotalBytes += cnt; 
  }
  
  // Generate Log if Requested 
  if (bGenerateLog == TRUE) 
  {
    EMU150_CreateSummaryLog ( &m_EMU150_Status );
  }
  
  return (TRUE);
}


/******************************************************************************
 * Function:    EMU150Protocol_GetStatus
 *
 * Description: Utility function to request current EMU150 Status 
 *
 * Parameters:  None
 *
 * Returns:     Ptr to EMU150 Status Data
 *
 * Notes:       None 
 *
 *****************************************************************************/
EMU150_STATUS_PTR EMU150Protocol_GetStatus (void)
{
  return ( (EMU150_STATUS_PTR) &m_EMU150_Status ); 
}


/******************************************************************************
 * Function:    EMU150Protocol_ReturnFileHdr()
 *
 * Description: Function to return the EMU150 file hdr structure
 *
 * Parameters:  dest - ptr to destination bufffer
 *              max_size - max size of dest buffer 
 *              ch - uart channel requested 
 *
 * Returns:     cnt - byte count of data returned 
 *
 * Notes:       - Only one ch will be support due to limited EEPROM size 
 *                to accomodate EMU150 Cfg Install (~ 5KB / ch )
 *
 *****************************************************************************/
UINT16 EMU150Protocol_ReturnFileHdr ( UINT8 *dest, const UINT16 max_size, UINT16 ch )
{
  UINT16 cnt; 


  // If EMU150 Protocol not setup during init, then any runtime request are not allowed.
  // If EMU150 Protocol is setup, only the ch requested during init can be processed.
  ASSERT ((m_EMU150_Status.port != UART_NUM_OF_UARTS) && (m_EMU150_Status.port == ch));
  
  cnt = m_EMU150_AppData.nCnt + sizeof(UINT16); 

#pragma ghs nowarning 1545 //Suppress packed structure alignment warning  
  memcpy (dest, (UINT8 *) &m_EMU150_AppData.nCnt, cnt);
#pragma ghs endnowarning  

  return (cnt); 
}


/******************************************************************************
 * Function:    EMU150Protocol_SetBaseUartCfg()
 *
 * Description: Sets the base Uart Cfg Settings from the calling App
 *
 * Parameters:  ch - uart channel to be processed
 *              UartCfg - Uart Drv port cfg settings
 *
 * Returns:     none
 *
 * Notes:       - Only one ch will be support due to limited EEPROM size 
 *                to accomodate EMU150 Cfg Install (~ 5KB / ch )
 *
 *****************************************************************************/
void EMU150Protocol_SetBaseUartCfg ( UINT16 ch, UART_CONFIG UartCfg )
{
  // If EMU150 Protocol already setup, ASSERT to let user know to FIX HIS/HER CFG 
  // Only one EMU150 Protocol can be setup due to EEPROM memory usage. 
  ASSERT ( m_EMU150_Status.port == UART_NUM_OF_UARTS ); 

  m_UartCfg = UartCfg; 
  m_EMU150_Status.port = ch; 
  m_EMU150_Status.UartBPS_Desired = m_UartCfg.BPS; 
}


/******************************************************************************
 * Function:    EMU150Protocol_DownloadHndl()
 *
 * Description: Initializes / synchronizes local control and flags to the calling 
 *              App
 *
 * Parameters:  bStartDownload - ptr to flag to begin download process
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
 * Notes:       none 
 *
 *****************************************************************************/
void EMU150Protocol_DownloadHndl ( UINT8 port, 
                                   BOOLEAN *bStartDownload, 
                                   BOOLEAN *bDownloadCompleted,
                                   BOOLEAN *bWriteInProgress,
                                   BOOLEAN *bWriteOk, 
                                   BOOLEAN *bHalt,
                                   BOOLEAN *bNewRec,
                                   UINT32  **pSize,
                                   UINT8   **pData )
{                                       
  m_EMU150_Download_Ptr.bStartDownload = bStartDownload; 
  m_EMU150_Download_Ptr.bDownloadCompleted = bDownloadCompleted; 
  m_EMU150_Download_Ptr.bWriteInProgress = bWriteInProgress; 
  m_EMU150_Download_Ptr.bWriteOk = bWriteOk; 
  m_EMU150_Download_Ptr.bHalt = bHalt; 
  m_EMU150_Download_Ptr.bNewRec = bNewRec; 
  
  *pSize = &m_EMU150_Rx_Record.cnt; 
  *pData = &m_EMU150_Rx_Record.buff[0]; 
}


/******************************************************************************
 * Function:    EMU150_State_ProcessCmd
 *
 * Description: Processes any receive data
 *
 * Parameters:  data - ptr to received data
 *              cnt - byte count of received data 
 *              ch - uart channel received data is from  
 *
 * Returns:     EMU150_STATES - next state of the EMU150 
 *
 * Notes:       None
 *
 *****************************************************************************/
static
EMU150_STATES EMU150_State_ProcessCmd ( UINT8 *data, UINT16 cnt, UINT16 ch )
{
  EMU150_STATES state; 
  EMU150_STATES NextState; 
  EMU150_CMD_STATES NextCmdState; 
  EMU150_CFG_PTR pEMU150_Cfg;
  UINT16 timeout; 


  pEMU150_Cfg = &m_EMU150_Cfg; 
  timeout = pEMU150_Cfg->timeout; 
 
  switch ( m_EMU150_Status.cmd_state )
  {
    case EMU150_CMD_STATE_SEND_HS:
      if ( m_EMU150_Status.nRetries <= pEMU150_Cfg->nRetries ) 
      {
        // Generate 'HS' packet
        EMU150_CreateTxPacket( 1, (UINT8 *) &EMU150_CMDS_HS, timeout); 

        // Go to next cmd state 
        EMU150_NextCmdState (EMU150_CMD_STATE_HS_ACK, FALSE);
      }
      else 
      {
        m_EMU150_Status.nRetries = 0; 
        
        if  ( (( m_EMU150_Status.state == EMU150_STATE_GET_RECORDING ) ||
               ( m_EMU150_Status.state == EMU150_STATE_RECORDING_CONFIGRMATION )) )
        {
          // Move to next record but exit if all new rec retrieve have been attempted
          if (++m_EMU150_Status.nCurrRecord >= m_EMU150_Status.nRecordsNew) 
          {
            m_EMU150_Status.state = EMU150_STATE_COMPLETED; 
          }
          else 
          {
            sprintf (GSE_OutLine,
                     "\r\nEMU150 Protocol: Record Retrieve/Confirm Failed. Skipping Rec (n=%d) \r\n \0", 
                     m_EMU150_Status.nCurrRecord);
            GSE_DebugStr(NORMAL,TRUE,GSE_OutLine); 
            if ( m_EMU150_Status.state == EMU150_STATE_RECORDING_CONFIGRMATION ) 
            {
              // Get next record
              m_EMU150_Status.state = EMU150_STATE_GET_RECORDING; 
            }
          }
          m_EMU150_Status.nRecordsRxBad++; 
        }
        else 
        {
          m_EMU150_Status.bDownloadFailed = TRUE;
          m_EMU150_Status.state = EMU150_STATE_COMPLETED; 
        }
        
      }
      break;
    
    case EMU150_CMD_STATE_HS_ACK:
      if (EMU150_Process_ACK( EMU150_BLK_RECEIVING, data, cnt, ch) == TRUE) 
      {
        state = m_EMU150_Status.state; 
        
        // Update counters if get recording summary 
        if ( state == EMU150_STATE_GET_RECORDING_HDR) 
        {
          m_EMU150_Status.nRecordsNew = 0; 
          m_EMU150_Status.nRecords = 0; 
          m_EMU150_Status.nCurrRecord = 0; 
          timeout = pEMU150_Cfg->timeout_rec_hdr; 
        }

        // Generate Cmd Msg based on the current state that we are in 
        EMU150_CreateTxPacket( 1, (UINT8 *) &EMU150_CMDS[state].cmd, timeout ); 

        if ( (state == EMU150_STATE_GET_RECORDING) || (state == EMU150_STATE_RECORDING_CONFIGRMATION) )
        {
          // Have to add Rec Identifier to Cmd Msg for getting records or confirming rec 
          EMU150_AppendRecordId(); 
        }

        // If Set Baud send appropriate command
        if ( state == EMU150_STATE_SET_BAUD )
        {
          // Have to add Baud Rate setting to Cmd Msg 
          EMU150_AppendBaudRate(); 
        }
        
        // Go to next state 
        if ( (state == EMU150_STATE_RECORDING_CONFIGRMATION) || 
             (state == EMU150_STATE_SET_BAUD) )
        {
          NextCmdState = EMU150_CMD_STATE_FINAL_ACK; 
        }
        else 
        {
          NextCmdState = EMU150_CMD_STATE_REC_ACK; 
        }
        
        EMU150_NextCmdState(NextCmdState, TRUE);       
        
      }
      else 
      {
        // .ack_timer used for Babbling Tx, .cmd_timer used for IDLE timer 
        //if ( (m_EMU150_Status.ack_timer + pEMU150_Cfg->timeout_ack) < CM_GetTickCount() )
        if ( (CM_GetTickCount() - m_EMU150_Status.ack_timer) > pEMU150_Cfg->timeout_ack )
        {
        
          m_EMU150_Status.cmd_timer = CM_GetTickCount();   // Reset timer
          m_EMU150_Status.nRetries++;                      // Update retries 
          m_EMU150_Status.nTotalRetries++; 
          EMU150_NextCmdState(EMU150_CMD_STATE_SEND_HS, FALSE); 
          
          sprintf (GSE_OutLine,
                   "\r\nEMU150 Protocol: Process Cmd (HS ACK) Bad Data TimeOut (n=%d)\r\n \0",
                   m_EMU150_Status.nRetries );
          GSE_DebugStr(NORMAL,TRUE,GSE_OutLine); 
        }
      }
      break; 
      
    case EMU150_CMD_STATE_REC_ACK:
      // Process bytes as they are rx from port 
      if (cnt > 0)
      {
        EMU150_Process_Blk( EMU150_BLK_RECEIVING, data, cnt, ch ); 
      }
//      else
//      {
        // Have we received the expected block size. 
        // NOTE: Always have to perform logic. We could have received entire blk in current 
        //       frame, thus can wait until cnt == 0 to process, esp for "babbling EMU Tx". 
        if ((m_EMU150_Rx_Buff.cnt - m_EMU150_Status.blk_hdr_size) == m_EMU150_Status.blk_exp_size) 
        {
        
          EMU150_Process_Blk( EMU150_BLK_EOB, data, cnt, ch ); 
          
          // If this is last block then expect ACK to be returned for our ACK otherwise 
          //    otherwise expect more blocks. 
          if ( m_EMU150_Status.blk_number == m_EMU150_Status.blk_exp_number ) 
          {
            EMU150_NextCmdState (EMU150_CMD_STATE_FINAL_ACK, FALSE); // Send final ACK and exp to receive ACK 
          }
          else 
          {
            EMU150_NextCmdState (EMU150_CMD_STATE_REC_ACK, FALSE);   // Remain in this state for next block 
          }
          
          // Send ACK 
          EMU150_CreateTxPacket( 1, (UINT8 *) &EMU150_CMDS_ACK, timeout); 
        }
        
        // Terminate immediately if the actual blk size is > exp blk size or we have exceeded are local 
        //    buffer size ! 
        if ( ((m_EMU150_Rx_Buff.cnt - m_EMU150_Status.blk_hdr_size) > m_EMU150_Status.blk_exp_size) ||
             (m_EMU150_Rx_Buff.cnt > (256 * 1024)) )
        {
          m_EMU150_Status.cmd_timer = CM_GetTickCount();   // Reset timer
          EMU150_NextCmdState(EMU150_CMD_STATE_SEND_HS, FALSE); 
          
          sprintf (GSE_OutLine,
                   "\r\nEMU150 Protocol: Process Cmd (GET_REC) Bad Data TimeOut (n=%d)\r\n \0",
                   m_EMU150_Status.nRetries );
          GSE_DebugStr(NORMAL,TRUE,GSE_OutLine); 
        }
        
//      }
      break; 
      
    case EMU150_CMD_STATE_FINAL_ACK: 
      if (EMU150_Process_ACK( EMU150_BLK_RECEIVING, data, cnt, ch) == TRUE)
      {
        if (EMU150_Process_Blk( EMU150_BLK_EOM, data, cnt, ch ) == TRUE)
        {  
          // Will change state if successful, otherwise stay in the same state. 
          EMU150_NextCmdState (EMU150_CMD_STATE_SEND_HS, FALSE);
        }
        else 
        { 
          // Clear Raw Rx Buff
          m_EMU150_Rx_Buff.cnt = 0; 
        }
      }
      else 
      {
        // .ack_timer used for Babbling Tx, .cmd_timer used for IDLE timer 
        //if ( (m_EMU150_Status.ack_timer + pEMU150_Cfg->timeout_ack) < CM_GetTickCount() )
        if ( (CM_GetTickCount() - m_EMU150_Status.ack_timer) > pEMU150_Cfg->timeout_ack )
        {
          // Force 
          m_EMU150_Status.cmd_timer = CM_GetTickCount();   // Reset timer
          EMU150_NextCmdState(EMU150_CMD_STATE_SEND_HS, FALSE); 
          
          sprintf (GSE_OutLine,
                   "\r\nEMU150 Protocol: Process Cmd (FINAL ACK) Bad Data TimeOut (n=%d)\r\n \0",
                   m_EMU150_Status.nRetries );
          GSE_DebugStr(NORMAL,TRUE,GSE_OutLine); 
        }
      }
      break; 
      
    default: 
      FATAL("Unsupported EMU Cmd State = %d", m_EMU150_Status.cmd_state);   
      break;   
  }
  
  NextState = m_EMU150_Status.state;  // Init to current state 

  return (NextState);
}


/******************************************************************************
 * Function:    EMU150_State_AutoDetect
 *
 * Description: Performs auto detect of the comm connect to the EMU 150
 *
 * Parameters:  data - ptr to any received data 
 *              cnt - byte count of received data 
 *              ch - uart channel data is received from 
 *
 * Returns:     EMU150_STATES - next state of the EMU150 
 *
 * Notes:       None
 *
 *****************************************************************************/
static 
EMU150_STATES EMU150_State_AutoDetect (UINT8 *data, UINT32 cnt, UINT16 ch)
{
  EMU150_STATES NextState; 
  EMU150_CFG_PTR pEMU150_Cfg; 


  pEMU150_Cfg = &m_EMU150_Cfg; 

  switch ( m_EMU150_Status.cmd_state )
  {
    case EMU150_CMD_STATE_SEND_HS:
      // Try current baud rate at least several times
      if ( m_EMU150_Status.nRetries > pEMU150_Cfg->nRetries ) 
      {
        // Loop thru all known baud rates 
        if ( ++m_EMU150_Status.IndexBPS < EMU150_BAUD_RATES_MAX ) 
        {
          // Set the new baud rate
          EMU150_SetUart ( ch, UART_BPS_SUPPORTED[m_EMU150_Status.IndexBPS]); 
          
          sprintf (GSE_OutLine,
                   "\r\nEMU150 Protocol: AutoDetect Attempting Setting %d BAUD \r\n \0",
                   UART_BPS_SUPPORTED[m_EMU150_Status.IndexBPS] );
          GSE_DebugStr(NORMAL,TRUE,GSE_OutLine); 
        }
        else 
        {
          // Reset the Uart to 9600 
          m_EMU150_Status.IndexBPS = 0; 
          EMU150_SetUart ( ch, UART_BPS_SUPPORTED[m_EMU150_Status.IndexBPS]); 
          
          // Indicate FAILED !!! 
          sprintf (GSE_OutLine,
                   "\r\nEMU150 Protocol: AutoDetect Failed \r\n \0" );
          GSE_DebugStr(NORMAL,TRUE,GSE_OutLine); 
          
          m_EMU150_Status.state = EMU150_STATE_COMPLETED; 
          m_EMU150_Status.bDownloadFailed = TRUE; 
        }
        
        // Call with TRUE just to clr some state vars 
        EMU150_NextCmdState (EMU150_CMD_STATE_FINAL_ACK, TRUE);  
        m_EMU150_Status.nRetries = 0;  // Reset the retries
      }
      // Generate 'HS' packet
      EMU150_CreateTxPacket( 1, (UINT8 *) &EMU150_CMDS_HS, pEMU150_Cfg->timeout_auto_detect); 
      
      EMU150_NextCmdState (EMU150_CMD_STATE_FINAL_ACK, FALSE);
      break; 
      
    case EMU150_CMD_STATE_FINAL_ACK:
      if (EMU150_Process_ACK( EMU150_BLK_RECEIVING, data, cnt, ch) == TRUE) 
      {
        // We got the correct baud rate.  Update UART info. 
        // Transition to Next State
        if ( m_EMU150_Status.UartBPS == m_EMU150_Status.UartBPS_Desired ) 
        {
          m_EMU150_Status.state = EMU150_STATE_GET_INSTALL_CFG;
        }
        else
        {
          m_EMU150_Status.state = EMU150_STATE_SET_BAUD;
        }
        
        sprintf (GSE_OutLine,
                 "\r\nEMU150 Protocol: AutoDetect Found EMU150 @ %d BAUD \r\n \0",
                 UART_BPS_SUPPORTED[m_EMU150_Status.IndexBPS] );
        GSE_DebugStr(NORMAL,TRUE,GSE_OutLine); 
      }
      else 
      {
        // Did not get ACK within a reasonable time 
        // .ack_timer used for Babbling Tx, .cmd_timer used for IDLE timer 
        //if ( (m_EMU150_Status.ack_timer + pEMU150_Cfg->timeout_ack) < CM_GetTickCount() )
        if ( (CM_GetTickCount() - m_EMU150_Status.ack_timer) > pEMU150_Cfg->timeout_ack )
        {
          m_EMU150_Status.cmd_timer = CM_GetTickCount();   // Reset timer
          m_EMU150_Status.nRetries++;                      // Update retries 
          m_EMU150_Status.nTotalRetries++; 
          EMU150_NextCmdState(EMU150_CMD_STATE_SEND_HS, FALSE); 
          
          sprintf (GSE_OutLine,
                   "\r\nEMU150 Protocol: AutoDetect Bad Data TimeOut (n=%d)\r\n \0",
                   m_EMU150_Status.nRetries );
          GSE_DebugStr(NORMAL,TRUE,GSE_OutLine); 
        }
      }
      break; 
      
    default:
      FATAL("Unsupported EMU Auto Detect State = %d", m_EMU150_Status.cmd_state);   
      break; 
  }
  
  NextState = m_EMU150_Status.state;  // Init to current state 
  
  return (NextState); 
  
}                                              


/******************************************************************************
 * Function:    EMU150_NextState
 *
 * Description: Clearing / resetting of some local variables before transition 
 *              to next state 
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static
void EMU150_NextState( void )
{
  m_EMU150_Status.cmd_state = EMU150_CMD_STATE_SEND_HS; 
  m_EMU150_Status.nRetries = 0; 
  m_EMU150_Status.cmd_timer = CM_GetTickCount(); 
  m_EMU150_Status.nIdleRetries = 0; 
  
  sprintf (GSE_OutLine,
           "\r\nEMU150 Protocol: New State Transition to %s \r\n \0", 
           EMU150_NAME[m_EMU150_Status.state].state );
  GSE_DebugStr(NORMAL,TRUE,GSE_OutLine); 
  
  // If new state is IDLE then clear DOWNLOAD STATES 
  if ( m_EMU150_Status.state == EMU150_STATE_IDLE )
  {
    *m_EMU150_Download_Ptr.bDownloadCompleted = TRUE; 
  }
  
}


/******************************************************************************
 * Function:    EMU150_NextCmdState
 *
 * Description: Clearing / resetting of some local variables before transition 
 *              to next cmd state 
 *
 * Parameters:  None 
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static
void EMU150_NextCmdState( EMU150_CMD_STATES state, BOOLEAN bClrRetries )
{

  // For change of cmd state, reset the blk_number(s) else, just update current msg reception
  if ( m_EMU150_Status.cmd_state != state ) 
  {
    m_EMU150_Status.cmd_state = state;
    m_EMU150_Status.blk_number = 0;      // Reset block being received 
    m_EMU150_Status.blk_exp_number = 0;  // Reset expected num blk for this cmd to 1. 
  }
  m_EMU150_Status.blk_exp_size = 0xFFFFFFFFUL;   // Set to large number 
  m_EMU150_Status.blk_hdr_size = 0; 
  
  m_EMU150_Rx_Buff.cnt = 0; 
  m_EMU150_Rx_Buff.cksum_cnt_ptr = 0;   // Reset for each block
  
  if (bClrRetries) 
  {
    //m_EMU150_Status.nRetries = 0;  // Reset the retries 
    m_EMU150_Rx_Record.cnt = 0; 
    m_EMU150_Rx_Buff.cksum = 0;     // Reset for each msg, since the msg can be 
                                    //   sent in mult block, thus we don't want to 
                                    //   reset cksum on blk basis 
                                    //   Currently only when new record is req 
                                    //   does this param get reset.  
  }
  
  m_EMU150_Status.ack_timer = CM_GetTickCount();  // Reset timer 
  
}


/******************************************************************************
 * Function:    EMU150_CreateTxPacket
 *
 * Description: cnt - current count of the number of bytes to be transmitted 
 *              data - ptr to data to be transmitted 
 *              timeout - timeout for this transmission message 
 *
 * Parameters:  None 
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static
void EMU150_CreateTxPacket( UINT8 cnt, UINT8 *data, UINT32 timeout )
{

  // Transmit Packet size and ata
  m_EMU150_Tx_Buff.cnt = cnt;
  memcpy ( &m_EMU150_Tx_Buff.buff[0], data, cnt ); 
  
  // Update Response timeout 
  m_EMU150_Status.cmd_timeout = timeout; 
  m_EMU150_Status.cmd_timer = CM_GetTickCount(); 
  
  // Clear Rx buffer
  m_EMU150_Rx_Buff.cnt = 0; 
  
}


/******************************************************************************
 * Function:    EMU150_AppendRecordId
 *
 * Description: Appends the current rec id to the Uart Tx Buffer
 *
 * Parameters:  None 
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static 
void EMU150_AppendRecordId( void )
{
  UINT32 index;

  
  index = m_EMU150_Status.nCurrRecord;
  
#pragma ghs nowarning 1545 //Suppress packed structure alignment warning  
  // Copy Rec Type with Bit 16 and 15 masked away.  Note byte swapping not necessary
  // Expect Bit 16 and 15 to be Masked away from Rec Type
  memcpy ( &m_EMU150_Tx_Buff.buff[1], &m_Record_Hdr[index].nRecType, 2 );
  
  // Copy Time Stamp with Bit 32 NOT masked away and incl Cycle. Note byte swapping not 
  // necessary
  memcpy ( &m_EMU150_Tx_Buff.buff[3], &m_Record_Hdr[index].nRecTime, 5 );
#pragma ghs endnowarning
  
  m_EMU150_Tx_Buff.cnt += 7; 
}


/******************************************************************************
 * Function:    EMU150_AppendBaudRate
 *
 * Description: Appends the UartBPS_Desired index to the Uart Tx Buffer
 *
 * Parameters:  None 
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static 
void EMU150_AppendBaudRate ( void )
{
  UINT16 index; 
  
  index = EMU150_GetDesiredBaudRateIndex (m_EMU150_Status.UartBPS_Desired); 

  m_EMU150_Tx_Buff.buff[1] = EMU150_BAUD_RATES[index]; 

  m_EMU150_Tx_Buff.cnt += 1; 
}


/******************************************************************************
 * Function:    EMU150_Process_ACK
 *
 * Description: Process EMU150 ACK msg 
 *
 * Parameters:  blk_state - current block receive state (a blk or EOM) 
 *              data - ptr to the received data 
 *              cnt - number of bytes of received data 
 *              ch - uart channel of received data  
 *
 * Returns:     TRUE   Good ACK msg received
 *
 * Notes:       A successful ACK is a resp msg that contains only one ACK char
 *              Timeout processing provide if we get continous Rx char 
 *
 *****************************************************************************/
static
BOOLEAN EMU150_Process_ACK (EMU150_BLK_STATES blk_state, UINT8 *data, UINT32 cnt, UINT16 ch)
{
  BOOLEAN bFound; 
  EMU150_CFG_PTR pEMU150_Cfg;
  

  bFound = FALSE; 
  pEMU150_Cfg = &m_EMU150_Cfg; 

  // Process bytes as they are rx from port 
  if (cnt > 0)
  {
    EMU150_Process_Blk( EMU150_BLK_RECEIVING, data, cnt, ch ); 
  }

  // Wait for IDLE time before processing ACK to ensure all bytes have been received 
  //if ((m_EMU150_Status.cmd_timer + pEMU150_Cfg->timeout_eom) < CM_GetTickCount())
  if ( (CM_GetTickCount() - m_EMU150_Status.cmd_timer) > pEMU150_Cfg->timeout_eom )
  {
    // Determine if we only have ACK and only ACK, else ignore and let timeout 
    if ((m_EMU150_Rx_Buff.cnt == 1) && (m_EMU150_Rx_Buff.buff[0] == EMU150_CMD_ACK))
    {
      bFound = TRUE; 
    }
    // else we either got more bytes than expected or not ACK
    //  expect calling app to incl TIMEOUT (2 sec ?) processing for this case. 
  }
  
  return (bFound);
}


/******************************************************************************
 * Function:    EMU150_Process_Blk
 *
 * Description: Generic process to handle rx block data from EMU150
 *
 * Parameters:  blk_state - current block receive state (a blk or EOM) 
 *              data - ptr to the received data 
 *              cnt - number of bytes of received data 
 *              ch - uart channel of received data 
 *
 * Returns:     TRUE - block successfully processed 
 *
 * Notes:       None
 *
 *****************************************************************************/
static
BOOLEAN EMU150_Process_Blk (EMU150_BLK_STATES blk_state, UINT8 *data, UINT32 cnt, UINT16 ch)
{
  BOOLEAN bOk; 
  UINT8 *data_ptr;
  UINT16 i; 
  UINT16 diff; 
  UINT16 hdr_offset; 

  
  bOk = FALSE; 
  
  switch ( blk_state ) 
  {
    case EMU150_BLK_RECEIVING:
      // Copy rx data to buffer 
      memcpy ( &m_EMU150_Rx_Buff.buff[m_EMU150_Rx_Buff.cnt], data, cnt );
      m_EMU150_Rx_Buff.cnt += cnt; 
      
      if (m_EMU150_Status.blk_exp_size == 0xFFFFFFFFUL) 
      {
        if ( m_EMU150_Status.blk_exp_number == 0 ) 
        {
          if (m_EMU150_Rx_Buff.cnt >= 4) 
          {
            // First block.  Parse Total Number of Blocks AND Parse block size
            m_EMU150_Status.blk_exp_number = (m_EMU150_Rx_Buff.buff[1] << 8) | (m_EMU150_Rx_Buff.buff[0]); 
            m_EMU150_Status.blk_exp_size = (m_EMU150_Rx_Buff.buff[3] << 8) | (m_EMU150_Rx_Buff.buff[2]); 
            m_EMU150_Status.blk_hdr_size = 4; 
            
            // Reset cksum misaligned flag for start of new set of blocks
            m_EMU150_Rx_Buff.cksum_misaligned_flag = 0; 
          }
        }
        else 
        {
          if (m_EMU150_Rx_Buff.cnt >= 2) 
          {
            // Not First block. Parse block size
            m_EMU150_Status.blk_exp_size = (m_EMU150_Rx_Buff.buff[1] << 8) | (m_EMU150_Rx_Buff.buff[0]);
            m_EMU150_Status.blk_hdr_size = 2; 
          }
        }
      }

      // If block is received within current FRAME and has more than Blk Hdr size then 
      //   we want to process checksum 
      if (m_EMU150_Status.blk_exp_size != 0xFFFFFFFFUL) 
      {
        EMU150_ProcessChksum(); 
      }
      break;
      
    case EMU150_BLK_EOB:
      // Save misaligned data from block for checksum calc later 
      hdr_offset = m_EMU150_Status.blk_hdr_size; 
      if ( (m_EMU150_Rx_Buff.cksum_cnt_ptr + hdr_offset) != m_EMU150_Rx_Buff.cnt)
      {
        diff = m_EMU150_Rx_Buff.cnt - (m_EMU150_Rx_Buff.cksum_cnt_ptr + hdr_offset); 
        if ( diff < 4 )
        {
          for (i=0;i<diff;i++)
          {
            m_EMU150_Rx_Buff.cksum_misaligned[i] = 
                  m_EMU150_Rx_Buff.buff[m_EMU150_Rx_Buff.cksum_cnt_ptr + hdr_offset]; 
            m_EMU150_Rx_Buff.cksum_cnt_ptr++;          
          }
          m_EMU150_Rx_Buff.cksum_misaligned_flag = diff; 
          // Note m_EMU150_Rx_Buff.cksum_cnt_ptr will get cleared in app that called this func 
        }
      }
    
      // If this is the first block, then the total number of blocks must also be parsed from data stream
      //  inaddition to block size. Otherwise just parse block size. 
      if (m_EMU150_Status.blk_number == 0) 
      {
        data_ptr = &m_EMU150_Rx_Buff.buff[4]; 
        cnt = m_EMU150_Rx_Buff.cnt - 4; 
      }
      else 
      {
        data_ptr = &m_EMU150_Rx_Buff.buff[2]; 
        cnt = m_EMU150_Rx_Buff.cnt - 2; 
      }
      bOk = EMU150_RSP[m_EMU150_Status.state]( EMU150_BLK_EOB, data_ptr, cnt, ch );
      if (bOk)
      {
        m_EMU150_Status.blk_number++;  
      }
      break;
      
    case EMU150_BLK_EOM:
      bOk = EMU150_RSP[m_EMU150_Status.state]( EMU150_BLK_EOM, data, cnt, ch );
      break; 
      
    default:
      FATAL("Unsupported EMU Block State = %d", blk_state);
      break; 
  }

  return (bOk); 
}


/******************************************************************************
 * Function:    EMU150_Process_InstallCfg
 *
 * Description: Processes receiving / incomming EMU150 Cfg Installation Records 
 *
 * Parameters:  blk_state - current block receive state (a blk or EOM) 
 *              data - ptr to the received data 
 *              cnt - number of bytes of received data 
 *              ch - uart channel of received data 
 *
 * Returns:     TRUE Installation Records processed successfully 
 *
 * Notes:       None
 *
 *****************************************************************************/
static
BOOLEAN EMU150_Process_InstallCfg (EMU150_BLK_STATES blk_state, UINT8 *data, UINT32 cnt, UINT16 ch)
{
  BOOLEAN bOk; 
  UINT32 calc_cksum; 
  UINT32 size; 

  bOk = FALSE; 

  switch (blk_state)
  {
    case EMU150_BLK_EOB: 
      // Process cksum for each block.  It appears that EMU150 sends a cfg rec for each block. 
      //    Therefore we can calc and validate chksum for each block (diff from data log rec as
      //    they have cksum at end of msg, not end of each block !) 
      // It is expected that *data will be pointing to start of cfg rec (not current blk size field)
/*  Differ calc chksum as enhancement       
      if (cnt >= 8) 
      {
        data_ptr = data + (cnt - 8); 
        EMU150_GetCksumStatus ( data_ptr, &exp_cksum, &status);

        calc_cksum = m_EMU150_Rx_Buff.cksum - (exp_cksum + status);
        
        if ( exp_cksum == (~(calc_cksum - 1)) )
        {
          // cksum is good continue,  else have to force failure and try again 
        }
      }
*/      

      // Copy all the blocks to the Record Buffer
      memcpy ( &m_EMU150_Rx_Record.buff[m_EMU150_Rx_Record.cnt], data, cnt ); 
      m_EMU150_Rx_Record.cnt += cnt; 
      bOk = TRUE; 

      GSE_StatusStr( NORMAL, 
                     "\rEMU150 Protocol: Retrieving Install Cfg, Blk=%d of %d, Blk size=%d \0", 
                     m_EMU150_Status.blk_number + 1,
                     m_EMU150_Status.blk_exp_number,
                     m_EMU150_Rx_Buff.cnt - m_EMU150_Status.blk_hdr_size);
      break;
     
    case EMU150_BLK_EOM:
      // Compare and store to EEPROM.  Data already in m_EMU150_Rx_Record.buff[] 
      size = EMU150_APP_DATA_MAX; 
      if (m_EMU150_Rx_Record.cnt < size) 
      {
        size = m_EMU150_Rx_Record.cnt; 
      }

      calc_cksum = EMU150_CalcCksum ( (UINT32 *) &m_EMU150_Rx_Record.buff[0], size / 4); 
      
      if (calc_cksum != m_EMU150_AppData.cksum)
      {
        // Update NV memory
        m_EMU150_AppData.cksum = calc_cksum; 
        m_EMU150_AppData.nCnt = size; 
        memcpy ( &m_EMU150_AppData.data[0], &m_EMU150_Rx_Record.buff[0], m_EMU150_AppData.nCnt ); 
        
        NV_Write( NV_UART_EMU150, 0, (void *) &m_EMU150_AppData,  sizeof(EMU150_APP_DATA)); 
        
        sprintf (GSE_OutLine,
                 "\r\nEMU150 Protocol: New EMU Cfg Detected.  (Bytes=%d, Cksum=0x%08x) \r\n \0",
                 m_EMU150_AppData.nCnt, m_EMU150_AppData.cksum );
        GSE_DebugStr(NORMAL,TRUE,GSE_OutLine); 
      }

      // Go to get recording state       
      m_EMU150_Status.state = EMU150_STATE_GET_RECORDING_HDR; 
      bOk = TRUE; 
      break; 
     
    default:
      FATAL("Unsupported EMU Install Cfg Block State = %d", blk_state);
      break;
  }   
  
  return (bOk); 
}


/******************************************************************************
 * Function:    EMU150_Process_Recording_Hdr
 *
 * Description: Processes receiving / incomming EMU150 Rec Hdr Summary records
 *
 * Parameters:  blk_state - current block receive state (a blk or EOM) 
 *              data - ptr to the received data 
 *              cnt - number of bytes of received data 
 *              ch - uart channel of received data
 *
 * Returns:     TRUE Rec Hdr Summary records processed successfully 
 *
 * Notes:       None
 *
 *****************************************************************************/
static
BOOLEAN EMU150_Process_Recording_Hdr (EMU150_BLK_STATES blk_state, UINT8 *data, UINT32 cnt, 
                                      UINT16 ch)
{
  UINT32 i; 
  BOOLEAN bOk; 

  bOk = FALSE; 

  switch (blk_state) 
  {
    case EMU150_BLK_EOB:
      // Expect each block to contain exactly one record 
      // Determine if the record has been downloaded, if not store in m_Record_Hdr[]
      if (cnt == 9)
      {
        m_EMU150_Status.nRecords++; // Count the number of records returned by EMU150
        if ( m_EMU150_Status.nRecordsNew < EMU150_RECORDS_MAX ) // Retrieve only up to the MAX array 
        {
          memcpy ( &m_Record_Hdr[m_EMU150_Status.nRecordsNew], data, cnt ); 
          if ( ((m_Record_Hdr[m_EMU150_Status.nRecordsNew].nRecType & 0x0080) == 0) ||
               (m_EMU150_Status.bAllRecords == TRUE) )
          {
            // Record has not been downloaded, accept the record
            // Parse all uncessary bits from record.  Note: Little Endian Fmt 
            i = m_EMU150_Status.nRecordsNew; 
            // Mask Bits 16 and 15
            m_Record_Hdr[i].nRecType = (m_Record_Hdr[i].nRecType & 0xFF3F); 
            // Mask Bits 32
            // m_Record_Hdr[i].nRecTime = (m_Record_Hdr[i].nRecTime & 0xFFFFFF7F); 
            m_Record_Hdr[i].nRecTime = m_Record_Hdr[i].nRecTime; 
            ++m_EMU150_Status.nRecordsNew; // Count the number of new records in EMU150
          }
          
          if ( m_EMU150_Status.nRecords == 1) 
          {
            // Output # of Records / Blks
            sprintf (GSE_OutLine,
                     "\r\nEMU150 Protocol: Retrieving Rec Headers, Total Exp Count = %d\r\n \0",
                     m_EMU150_Status.blk_exp_number);
            GSE_DebugStr(NORMAL,TRUE,GSE_OutLine); 
          }
        } // End of if ( m_EMU150_Status.nRecords < EMU150_RECORDS_MAX )
        bOk = TRUE;  // Force to continue even if > EMU150_RECORDS_MAX.  Rec after EMU150_RECORDS_MAX 
                     //  will not be retrieved later.  
      } // End of if (cnt == 9)
      // else 
      // Expect Summary record to be exactly 9 bytes.  Otherwise timeout and try again 
      // Record sys log to indicate Bad Summary Record received, move to next Summary Record ? 
      else 
      {
        // We have a problem.  What do we do ? TBD 
        bOk = TRUE; 
      }
      break; 
    
    case EMU150_BLK_EOM: 
      // If nRecordHdr > 0 then we have records to retrieve 
      if (m_EMU150_Status.nRecordsNew > 0)
      {
        m_EMU150_Status.state = EMU150_STATE_GET_RECORDING; 
        m_EMU150_Status.nCurrRecord = 0;   // Reset record count
        m_EMU150_Status.nRecordsRxBad = 0; 
      }
      else
      {
        m_EMU150_Status.state = EMU150_STATE_COMPLETED; 
      }
      
      sprintf (GSE_OutLine,
               "\r\nEMU150 Protocol: Total Rec = %d, New Rec = %d \r\n \0",
               m_EMU150_Status.nRecords, m_EMU150_Status.nRecordsNew );
      GSE_DebugStr(NORMAL,TRUE,GSE_OutLine); 
      bOk = TRUE; 
      break; 
      
    default:
      FATAL("Unsupported EMU Rec Hdr Block State = %d", blk_state);
      break; 
  }

  return (bOk); 
}


/******************************************************************************
 * Function:    EMU150_Process_Record
 *
 * Description: Processes receiving / incoming EMU150 Record
 *
 * Parameters:  blk_state - current block receive state (a blk or EOM) 
 *              data - ptr to the received data 
 *              cnt - number of bytes of received data 
 *              ch - uart channel of received data
 *
 * Returns:     TRUE Record processed successfully 
 *
 * Notes:       None
 *
 *****************************************************************************/
static
BOOLEAN EMU150_Process_Record (EMU150_BLK_STATES blk_state, UINT8 *data, UINT32 cnt, UINT16 ch)
{
  BOOLEAN bOk; 
  UINT32 exp_cksum; 
  UINT32 calc_cksum; 
  UINT32 status; 
  
  
  bOk = FALSE; 
  
  switch (blk_state)
  {
    case EMU150_BLK_EOB:
      // Copy all the blocks to the Record Buffer
      memcpy ( &m_EMU150_Rx_Record.buff[m_EMU150_Rx_Record.cnt], data, cnt ); 
      m_EMU150_Rx_Record.cnt += cnt; 
      bOk = TRUE; 

      GSE_StatusStr( NORMAL, 
                     "\rEMU150 Protocol: Retrieving Rec=%d of %d, Blk=%d of %d, Blk size=%d \0", 
                     m_EMU150_Status.nCurrRecord + 1,
                     m_EMU150_Status.nRecordsNew,
                     m_EMU150_Status.blk_number + 1,
                     m_EMU150_Status.blk_exp_number,
                     m_EMU150_Rx_Buff.cnt - m_EMU150_Status.blk_hdr_size);
      break; 
      
    case EMU150_BLK_EOM:
      // Peform check sum if OK 1)send to data mgr, 2)wait for confirmation of storage, 
      //    then 3)confirm to EDU. 
      // Last long word is a status and 2nd to last long word is checksum.  
      //    Checksum does not include checksum or status word itself
      EMU150_GetCksumStatus ( &m_EMU150_Rx_Record.buff[m_EMU150_Rx_Record.cnt - 8], 
                              &exp_cksum, &status);
      
      calc_cksum = m_EMU150_Rx_Buff.cksum - (exp_cksum + status);
      if (exp_cksum == (~(calc_cksum - 1)) )
      {
        // If checksum Ok, otherwise stay in same state !
        m_EMU150_Status.state = EMU150_STATE_WAIT_FOR_STORAGE_CONFIRMATION; 
        m_EMU150_Status.nRecordsRec++; 
        m_EMU150_Status.nRecordsRecTotalByte += m_EMU150_Rx_Record.cnt; 
        bOk = TRUE; 
      }
      else 
      {
        sprintf (GSE_OutLine,
                 "\r\nEMU150 Protocol: Checksum Failed Exp=%08x Calc=%08x (Rec=%d of %d)\r\n \0", 
                 exp_cksum, (~(calc_cksum - 1)),
                 m_EMU150_Status.nCurrRecord + 1,  m_EMU150_Status.nRecordsNew);
        GSE_DebugStr(NORMAL,TRUE,GSE_OutLine); 
        m_EMU150_Status.nCksumErr++; 
      }
      break;
      
    default:
      FATAL("Unsupported EMU Record Block State = %d", blk_state);
      break; 
  }
  
  return (bOk); 
  
}


/******************************************************************************
 * Function:    EMU150_Process_Record_Confirmation
 *
 * Description: Processes EMU150 Rec Confirmation ACK
 *
 * Parameters:  blk_state - current block receive state (a blk or EOM) 
 *              data - ptr to the received data 
 *              cnt - number of bytes of received data 
 *              ch - uart channel of received data
 *
 * Returns:     TRUE Record Confirmation cmd ACKed by EMU150 
 *
 * Notes:       None
 *
 *****************************************************************************/
static 
BOOLEAN EMU150_Process_Record_Confirmation (EMU150_BLK_STATES blk_state, 
                                            UINT8 *data, UINT32 cnt, UINT16 ch)
{
  BOOLEAN bOk; 

  bOk = FALSE; 

  if ( blk_state == EMU150_BLK_EOM ) 
  {
    m_EMU150_Status.state = EMU150_STATE_GET_RECORDING;  // Set next state
    
    m_EMU150_Status.nCurrRecord++; 
    
    bOk = TRUE; 
  }
  
  return (bOk);
}                                                         


/******************************************************************************
 * Function:    EMU150_Process_Set_Baud
 *
 * Description: Processes EMU150 ACK to Setting a New Baud Rate
 *
 * Parameters:  blk_state - current block receive state (a blk or EOM) 
 *              data - ptr to the received data 
 *              cnt - number of bytes of received data 
 *              ch - uart channel of received data
 *
 * Returns:     TRUE EMU150 baud rate successfully changed 
 *
 * Notes:       None
 *
 *****************************************************************************/
static 
BOOLEAN EMU150_Process_Set_Baud (EMU150_BLK_STATES blk_state, UINT8 *data, UINT32 cnt, UINT16 ch)
{
  BOOLEAN bOk; 
  

  bOk = FALSE; 

  if ( blk_state == EMU150_BLK_EOM ) 
  {
    EMU150_SetUart ( ch, (UART_CFG_BPS) m_EMU150_Status.UartBPS_Desired ); 

    // Set next state
    m_EMU150_Status.state = EMU150_STATE_GET_INSTALL_CFG;  
    
    // Update Current Status indication 
    m_EMU150_Status.UartBPS = m_EMU150_Status.UartBPS_Desired;
    m_EMU150_Status.IndexBPS = EMU150_GetDesiredBaudRateIndex(m_EMU150_Status.UartBPS_Desired); 
    
    sprintf (GSE_OutLine,
             "\r\nEMU150 Protocol: Setting %d BAUD SUCCESS \r\n \0",
             UART_BPS_SUPPORTED[m_EMU150_Status.IndexBPS] );
    GSE_DebugStr(NORMAL,TRUE,GSE_OutLine); 
    
    bOk = TRUE; 
  }
  
  return (bOk);
}       


/******************************************************************************
 * Function:    EMU150_GetDesiredBaudRateIndex
 *
 * Description: Utility func to return an index number corresponding to a 
 *              Baud Rate Setting.  EMU150_BAUD_RATES[index]
 *
 * Parameters:  UartBPS_Desired - Baud Rate in  UART_CFG_BPS
 *
 * Returns:     index - EMU150_BAUD_RATES[index]
 *
 * Notes:       None
 *
 *****************************************************************************/
static
UINT16 EMU150_GetDesiredBaudRateIndex (UINT32 UartBPS_Desired) 
{
  UINT16 i;
  UINT16 index; 
  
  index = m_EMU150_Status.IndexBPS;  // Init to current index 
  
  for (i=0;i<EMU150_BAUD_RATES_MAX;i++)
  {
    if ( UART_BPS_SUPPORTED[i] == UartBPS_Desired ) 
    {
      index = i;
      break; 
    }
  }
  
  return (index);
}


/******************************************************************************
 * Function:    EMU150_SetUart
 *
 * Description: Sets the Uart Channel to the specified BPS
 *
 * Parameters:  ch - Uart channel to configure
 *              bps - UART_CFG_BPS setting 
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static 
void EMU150_SetUart ( UINT16 ch, UART_CFG_BPS bps ) 
{
  UART_CONFIG UartCfg; 
  
  UartCfg = m_UartCfg; 
  UartCfg.BPS = bps;   // Set to new BPS
  m_EMU150_Status.UartBPS = UartCfg.BPS;  // Update current Uart Status

  UART_ClosePort(UartCfg.Port);
  UART_OpenPort(&UartCfg); 
}


/******************************************************************************
 * Function:    EMU150_ProcessChksum
 *
 * Description: Utility to provide a running checksum count of the Rx Buffer
 *
 * Parameters:  None 
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static 
void EMU150_ProcessChksum (void)
{
  UINT16 hdr_offset; 
  SINT32 diff; 
  UINT16 i; 
  UINT8  *data_ptr; 
  UINT32 curr; 
  UINT8 data[4]; 


  // Jump past the small blk header and the data that has been processed 
  hdr_offset = m_EMU150_Status.blk_hdr_size;
  diff = (m_EMU150_Rx_Buff.cnt - (m_EMU150_Rx_Buff.cksum_cnt_ptr + hdr_offset));
  
  if (diff >= 4)
  {
    // Add in misaligned data from previous block
    if ( (m_EMU150_Rx_Buff.cksum_misaligned_flag != 0) && (m_EMU150_Rx_Buff.cksum_cnt_ptr == 0) )
    {
      // Clear local buffer
      for (i=0;i<4;i++)
      {
        data[i] = 0; 
      }
      // Get misaligned data
      for (i=0;i<m_EMU150_Rx_Buff.cksum_misaligned_flag;i++)
      {
        data[i] = m_EMU150_Rx_Buff.cksum_misaligned[i]; 
      }
      // Get additional bytes from rx buff to make long word 
      for (i=m_EMU150_Rx_Buff.cksum_misaligned_flag;i<4;i++)
      {
        data[i] = m_EMU150_Rx_Buff.buff[m_EMU150_Rx_Buff.cksum_cnt_ptr + hdr_offset];
        m_EMU150_Rx_Buff.cksum_cnt_ptr++; 
      }
      curr = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24); 
      
      // Add to running total
      m_EMU150_Rx_Buff.cksum += curr;
      
      // Clear cksum misaligned flag
      m_EMU150_Rx_Buff.cksum_misaligned_flag = 0; 
    }
  
    for (i=0;i<(diff/4);i++)
    {
      // Get current long word data and 
      data_ptr = &m_EMU150_Rx_Buff.buff[m_EMU150_Rx_Buff.cksum_cnt_ptr + hdr_offset];
      curr = (*data_ptr) | (*(data_ptr + 1) << 8) | (*(data_ptr + 2) << 16) | 
                           (*(data_ptr + 3) << 24); 
      // Add to running total
      m_EMU150_Rx_Buff.cksum += curr; 
      
      // Move chksum index pointer to next long word position 
      m_EMU150_Rx_Buff.cksum_cnt_ptr += 4; 
    }
  } 

}


/******************************************************************************
 * Function:    EMU150_RestoreAppData
 *
 * Description: Retrieves the stored EMU150 Installation Cfg Rec from EEPROM
 *
 * Parameters:  None 
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static
void EMU150_RestoreAppData (void)
{
  RESULT result;  
  UINT32 cksum; 
  
  result = NV_Open(NV_UART_EMU150); 
  if ( result == SYS_OK )
  {
    NV_Read(NV_UART_EMU150, 0, (void *) &m_EMU150_AppData, sizeof(EMU150_APP_DATA));
    
    // Ensure checksum (long word sum and then 2-s complement) is Ok
    cksum = EMU150_CalcCksum ( (UINT32 *) &m_EMU150_AppData.data[0], m_EMU150_AppData.nCnt / 4); 
  }

  // If open failed or checksum bad (if not truncated data), then re-init app data   
  if ( (result != SYS_OK) || (cksum != m_EMU150_AppData.cksum) )
  {
    EMU150_FileInit(); 
  }
}


/******************************************************************************
 * Function:    EMU150_CalcCksum
 *
 * Description: Calculate checksum (long word sum then 2's complement)
 *
 * Parameters:  data - ptr to long word data buffer to appy checksum calc
 *              nLongWords - number of long words (4 bytes) in the buffer 
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static 
UINT32 EMU150_CalcCksum ( UINT32 *data_ptr, UINT32 nLongWords )
{
  UINT16 i;
  UINT32 cksum; 
  
  
  // Ensure long word aligment before processing 
  ASSERT_MESSAGE( (((UINT32) data_ptr & 0x3) == 0), "EMU150_CalcCksum: Long Word Aligned Ptr Required %08x", data_ptr); 
  
  cksum = 0; 
  for (i=0;i<nLongWords;i++)
  {
    cksum += *data_ptr; 
  }
  
  // Two's complement the cksum at the end
  cksum = (~(cksum - 1));
  
  return (cksum); 
}


/******************************************************************************
 * Function:    EMU150_GetCksumStatus
 *
 * Description: Returns the checksum and status words at the end of a EMU 150 
 *              data record
 *
 * Parameters:  data_ptr - ptr to the data buffer contain the record 
 *              cksum - ptr to location to return the checksum value 
 *              status - ptr to location to return the status value 
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static 
void EMU150_GetCksumStatus ( UINT8 *data_ptr, UINT32 *cksum, UINT32 *status )
{
  *cksum = (*data_ptr) | (*(data_ptr + 1) << 8) | (*(data_ptr + 2) << 16) | 
                           (*(data_ptr + 3) << 24); 
  data_ptr += 4; 
  *status = (*data_ptr) | (*(data_ptr + 1) << 8) | (*(data_ptr + 2) << 16) | 
                            (*(data_ptr + 3) << 24); 
}


/******************************************************************************
 * Function:    EMU150_CreateSummaryLog
 *
 * Description: Creates EMU150 download summary complete log 
 *
 * Parameters:  pStatus - ptr to EMU150 Status data 
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static 
void EMU150_CreateSummaryLog ( EMU150_STATUS_PTR pStatus )
{
  EMU150_STATUS_LOG log; 
  
  // Update with current info 
  log.nRecords = pStatus->nRecords; 
  log.nRecordsNew = pStatus->nRecordsNew; 
  log.nRecordsRxBad = pStatus->nRecordsRxBad; 
  log.nTotalBytes = pStatus->nTotalBytes; 
  log.nTotalRetries = pStatus->nTotalRetries; 
  log.nRecordsRec = pStatus->nRecordsRec; 
  log.nRecordsRecTotalByte = pStatus->nRecordsRecTotalByte;
  log.UartBPS = pStatus->UartBPS;
  log.nCksumErr = pStatus->nCksumErr; 
  log.nRestarts = pStatus->nRestarts;
  log.bDownloadInterrupted = pStatus->bDownloadInterrupted; 
  log.bDownloadFailed = pStatus->bDownloadFailed; 
  
  // Write the log
  LogWriteSystem (SYS_ID_UART_EMU150_STATUS, LOG_PRIORITY_LOW, &log, sizeof(EMU150_STATUS_LOG), 
                  NULL);
  
}


/******************************************************************************
 * Function:    EMU150_FileInit
 *
 * Description: Clears the EEPROM storage location 
 *
 * Parameters:  None 
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
BOOLEAN EMU150_FileInit(void)
{
  // Init App data
  // Note: NV_Mgr will record log if data is corrupt
  memset (&m_EMU150_AppData.data[0], 0, EMU150_APP_DATA_MAX); 
  m_EMU150_AppData.cksum = 0; 
  m_EMU150_AppData.nCnt = 0; 

  // Update App data
  NV_Write(NV_UART_EMU150, 0, (void *) &m_EMU150_AppData, sizeof(EMU150_APP_DATA));
  
  return TRUE;
}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/*****************************************************************************
 *  MODIFICATIONS
 *    $History: EMU150Protocol.c $
 * 
 * *****************  Version 17  *****************
 * User: Peter Lee    Date: 10/06/11   Time: 11:39a
 * Updated in $/software/control processor/code/system
 * SCR #1059 Added FATAL to all default breaks
 * 
 * *****************  Version 13  *****************
 * User: Peter Lee    Date: 9/27/11    Time: 11:20a
 * Updated in $/software/control processor/code/system
 * SCR #1059 Changed 4K to 4000 as max records that can be retrieved. 
 * 
 * *****************  Version 12  *****************
 * User: Peter Lee    Date: 9/16/11    Time: 1:15p
 * Updated in $/software/control processor/code/system
 * SCR #1059
 * 
 * *****************  Version 11  *****************
 * User: Peter Lee    Date: 9/15/11    Time: 7:59p
 * Updated in $/software/control processor/code/system
 * SCR #1059 EMU150 V&V Issues
 * 
 * *****************  Version 10  *****************
 * User: John Omalley Date: 9/15/11    Time: 10:08a
 * Updated in $/software/control processor/code/system
 * SCR 1059 - Remove Debug commands and variables
 * 
 * *****************  Version 9  *****************
 * User: John Omalley Date: 9/12/11    Time: 9:17a
 * Updated in $/software/control processor/code/system
 * SCR 1059 - Fixed Clock Issue
 * 
 * *****************  Version 8  *****************
 * User: Jeff Vahue   Date: 9/01/11    Time: 1:01p
 * Updated in $/software/control processor/code/system
 * SCR# 1060 - Autodetect Retries is 2x the specified retry count
 * 
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 8/31/11    Time: 3:54p
 * Updated in $/software/control processor/code/system
 * SCR #1059 nIdleRetries update
 * 
 * *****************  Version 6  *****************
 * User: Peter Lee    Date: 8/31/11    Time: 3:50p
 * Updated in $/software/control processor/code/system
 * SCR #1058 
 *
 * *****************  Version 5  *****************
 * User: John Omalley Date: 8/24/11    Time: 5:27p
 * Updated in $/software/control processor/code/system
 * SCR 1058 - Fixed nRetries count
 * 
 * *****************  Version 4  *****************
 * User: Peter Lee    Date: 7/25/11    Time: 4:39p
 * Updated in $/software/control processor/code/system
 * SCR #1039 Add option to "Not Mark As Downloaded" and several non-logic
 * updates (names, variable, etc).  
 * 
 * *****************  Version 3  *****************
 * User: Peter Lee    Date: 4/21/11    Time: 11:35a
 * Updated in $/software/control processor/code/system
 * SCR #1029 Update 9600 AutoDetect on startup
 * 
 * *****************  Version 2  *****************
 * User: Peter Lee    Date: 4/19/11    Time: 7:50p
 * Updated in $/software/control processor/code/system
 * SCR #1029 Limit New Records to EMU150_RECORDS_MAX
 * 
 * *****************  Version 1  *****************
 * User: Peter Lee    Date: 4/11/11    Time: 10:07a
 * Created in $/software/control processor/code/system
 * SCR #1029 EMU150 Download.  Initial check in. 
 * 
 * 
 *
 *****************************************************************************/
