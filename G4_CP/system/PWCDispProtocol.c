#define PWCDISP_PROTOCOL_BODY

/******************************************************************************
            Copyright (C) 2008-2016 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    ECCN:        9D991

    File:        PWCDispProtocol.c

    Description: Contains all functions and data related to the PWC Display 
                 Protocol Handler 
    
    VERSION
      $Revision: 16 $  $Date: 5/11/16 7:02p $     

******************************************************************************/



/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/


/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "PWCDispProtocol.h"
#include "UartMgr.h"
#include "ClockMgr.h"
#include "GSE.h"
#include "NVMgr.h"

#include "TestPoints.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/********************/
/*      Enums       */
/********************/
typedef enum
{
  PWCDISP_PARAMSRCH_HEADER,
  PWCDISP_PARAMSRCH_EXTRACT_DATA,
  PWCDISP_PARAMSRCH_CHECKSUM,
  PWCDISP_PARAMSRCH_UPDATE_BUFFER
}PWCDISP_PARAM_SEARCH_ENUM;

typedef enum
{
  PWCDISP_PARAM_ENCODE_HEADER,
  PWCDISP_PARAM_ENCODE_CHARACTERS,
  PWCDISP_PARAM_ENCODE_CHECKSUM
}PWCDISP_PARAM_ENCODE_ENUM;

typedef enum
{
  PWCDISP_RX_PARAM_SYNC1,
  PWCDISP_RX_PARAM_SYNC2,
  PWCDISP_RX_PARAM_SIZE,
  PWCDISP_RX_PARAM_PACKETID,
  PWCDISP_RX_PARAM_BUTTON_STATE,
  PWCDISP_RX_PARAM_DBL_CLK_STATE,
  PWCDISP_RX_PARAM_DISCRETE_STATE,
  PWCDISP_RX_PARAM_D_HLTH_STATE,
  PWCDISP_RX_PARAM_PART_NUMBER,
  PWCDISP_RX_PARAM_VERSION_NUMBER,
  PWCDISP_RX_PARAM_CHKSUM_HIGH,
  PWCDISP_RX_PARAM_CHKSUM_LOW,
  PWCDISP_RX_PARAM_COUNT
}PWCDISP_RX_PARAM;

typedef enum
{
  PWCDISP_TX_PARAM_SYNC1,
  PWCDISP_TX_PARAM_SYNC2,
  PWCDISP_TX_PARAM_SIZE,
  PWCDISP_TX_PARAM_PACKETID,
  PWCDISP_TX_PARAM_CHARDATA,
  PWCDISP_TX_PARAM_DCRATE = PWCDISP_MAX_CHAR_PARAMS+PWCDISP_TX_PARAM_CHARDATA,
  PWCDISP_TX_PARAM_CHKSUM_HIGH,
  PWCDISP_TX_PARAM_CHKSUM_LOW,
  PWCDISP_TX_PARAM_COUNT
}PWCDISP_TX_PARAM;

/********************/
/*      RXData      */
/********************/
typedef struct
{
  UINT8   data[PWCDISP_MAX_RAW_RX_BUF];
  UINT8  *pWr;
  UINT8  *pRd;
  UINT16  cnt;
  UINT8  *pStart;  // For debugging
  UINT8  *pEnd;    // For debugging
}PWCDISP_RAW_RX_BUFFER, *PWCDISP_RAW_RX_BUFFER_PTR;

typedef struct
{
  BOOLEAN bNewFrame;
  UINT16  sync_word;
  UINT8   size;
  UINT16  chksum;
  UINT8   data[PWCDISP_MAX_RX_LIST_SIZE];
}PWCDISP_RX_DATA_FRAME, *PWCDISP_RX_DATA_FRAME_PTR;

/********************/
/*      TXData      */
/********************/
typedef struct
{
    UINT8     data[PWCDISP_TX_MSG_SIZE];
}PWCDISP_RAW_TX_BUFFER, *PWCDISP_RAW_TX_BUFFER_PTR;

typedef struct
{
  UINT8 charParam[PWCDISP_MAX_CHAR_PARAMS];
  INT8 doubleClickRate;
} PWCDISP_TX_PARAM_LIST, *PWCDISP_TX_PARAM_LIST_PTR;

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static PWCDISP_RX_STATUS                 m_PWCDisp_RXStatus;
static PWCDISP_TX_STATUS                 m_PWCDisp_TXStatus;
                                         
static PWCDISP_RAW_RX_BUFFER             m_PWCDisp_RXBuff;
static PWCDISP_RX_DATA_FRAME             m_PWCDisp_RXDataFrame;
                                         
static PWCDISP_RAW_TX_BUFFER             m_PWCDisp_TXBuff;

static PWCDISP_TX_PARAM_LIST             m_PWCDisp_TXParamList;
                                         
static PWCDISP_DEBUG                     m_PWCDisp_Debug;
                                         
static PWCDISP_DISPLAY_DEBUG_TASK_PARAMS pwcDisplayDebugBlock;

static PWCDISP_PARAM_ENCODE_ENUM         stateCheck = PWCDISP_PARAM_ENCODE_HEADER;

static UARTMGR_PROTOCOL_STORAGE m_PWCDisp_DataStore[MAX_STORAGE_ITEMS];
                                                 // RX/TX Data Storage.
                                                 // 0 - RX_PROTOCOL
                                                 // 1 - TX_PROTOCOL
                                                 // 2 - Requests From Disp App
                                                 // 3 - Requests From Protocol
// TX Temporary Data Store
static UARTMGR_PROTOCOL_DATA    m_PWCDisp_TXData[PWCDISP_TX_ELEMENT_CNT_COMPLETE];
// RX Temporary Data Store
static UARTMGR_PROTOCOL_DATA    m_PWCDisp_RXData[PWCDISP_RX_ELEMENT_COUNT] = 
{
 //Data 
  {NULL}, //RXPACKETID
  {NULL}, //BUTTONSTATE_RIGHT
  {NULL}, //BUTTONSTATE_LEFT
  {NULL}, //BUTTONSTATE_ENTER
  {NULL}, //BUTTONSTATE_UP
  {NULL}, //BUTTONSTATE_DOWN
  {NULL}, //UNUSED1_BUTTON
  {NULL}, //UNUSED2_BUTTON
  {NULL}, //BUTTONSTATE_EVENT
  {NULL}, //DOUBLECLICK_RIGHT
  {NULL}, //DOUBLECLICK_LEFT
  {NULL}, //DOUBLECLICK_ENTER
  {NULL}, //DOUBLECLICK_UP
  {NULL}, //DOUBLECLICK_DOWN
  {NULL}, //UNUSED1_DBLCLK
  {NULL}, //UNUSED2_DBLCLK
  {NULL}, //DOUBLECLICK_EVENT
  {NULL}, //DISCRETE_ZERO
  {NULL}, //DISCRETE_ONE
  {NULL}, //DISCRETE_TWO
  {NULL}, //DISCRETE_THREE
  {NULL}, //DISCRETE_FOUR
  {NULL}, //DISCRETE_FIVE
  {NULL}, //DISCRETE_SIX
  {NULL}, //DISCRETE_SEVEN
  {NULL}, //DISPLAY_HEALTH
  {NULL}, //PART_NUMBER
  {NULL}  //VERSION_NUMBER
};

static UINT8  m_Str[MAX_DEBUG_STRING_SIZE];
static UINT8  dataLossFlag = (UINT8)DIO_DISPLAY_DATA_LOSS_FAIL;
static UINT32 dataLossCounter = 0;
/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/

static PWCDISP_RX_STATUS_PTR PWCDispProtocol_GetRXStatus(void);
static PWCDISP_TX_STATUS_PTR PWCDispProtocol_GetTXStatus(void); 
static PWCDISP_DEBUG_PTR PWCDispProtocol_GetDebug(void);
static void    PWCDispProtocol_UpdateUartMgr(void);
static void    PWCDispProtocol_Resync(void);
static BOOLEAN PWCDispProtocol_FrameSearch(PWCDISP_RAW_RX_BUFFER_PTR rx_buff_ptr);
static void    PWCDispProtocol_UpdateRdPtr(PWCDISP_RAW_RX_BUFFER_PTR rx_buff_ptr,
                                           UINT16 bytesScanned);
static void    PWCDispProtocol_ValidRXPacket(BOOLEAN bBadPacket);
static void    PWCDispProtocol_ValidTXPacket(BOOLEAN bBadPacket);
static BOOLEAN PWCDispProtocol_ReadTXData(PWCDISP_TX_PARAM_LIST_PTR tx_param_ptr);
static BOOLEAN PWCDispProtocol_TXCharacterCheck(PWCDISP_TX_PARAM_LIST_PTR tx_param_ptr,
                                                PWCDISP_RAW_TX_BUFFER_PTR tx_buff_ptr);
static void    PWCDispDebug_Task(void *pParam);
static void    PWCDispDebug_RX(void *pParam);
static void    PWCDispDebug_TX(void *pParam);
#include "PWCDispUserTables.c"

/*****************************************************************************/
/* Constant Data                                                             */
/*****************************************************************************/
typedef struct
{
  UINT8 name[PWCDISP_PARAM_NAMES_LENGTH];
} PWCDISP_PARAM_NAMES, *PWCDISP_PARAM_NAMES_PTR;

static 
const PWCDISP_PARAM_NAMES pwcDisp_RX_Param_Names[PWCDISP_MAX_RX_LIST_SIZE] =
{
  "RX_PACKETID    ",
  "BUTTONSTATE    ",
  "DOUBLECLICK    ",
  "DISCRETE       ",
  "DISPLAY_HEALTH ",
  "PART_NUMBER    ",
  "VERSION_NUMBER "
};

static
const PWCDISP_PARAM_NAMES pwcDisp_TX_Param_Names[PWCDISP_TX_ELEMENT_CNT_COMPLETE] =
{
  "CHARACTER_0   ",
  "CHARACTER_1   ",
  "CHARACTER_2   ",
  "CHARACTER_3   ",
  "CHARACTER_4   ",
  "CHARACTER_5   ",
  "CHARACTER_6   ",
  "CHARACTER_7   ",
  "CHARACTER_8   ",
  "CHARACTER_9   ",
  "CHARACTER_10  ",
  "CHARACTER_11  ",
  "CHARACTER_12  ",
  "CHARACTER_13  ",
  "CHARACTER_14  ",
  "CHARACTER_15  ",
  "CHARACTER_16  ",
  "CHARACTER_17  ",
  "CHARACTER_18  ",
  "CHARACTER_19  ",
  "CHARACTER_20  ",
  "CHARACTER_21  ",
  "CHARACTER_22  ",
  "CHARACTER_23  ",
  "DC_RATE       ",
};
/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:    PWCDispProtocol_Initialize
 *
 * Description: Initializes the PWC Display Protocol functionality 
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
void PWCDispProtocol_Initialize(void)
{  
  PWCDISP_RX_STATUS_PTR pRXStatus; 
  PWCDISP_TX_STATUS_PTR pTXStatus; 
  PWCDISP_RAW_RX_BUFFER_PTR buff_ptr;

  // Local Data
  TCB pwcTaskInfo;

  // Initialize local variables / structures
  memset (&m_PWCDisp_RXStatus,    0, sizeof(m_PWCDisp_RXStatus   ));
  memset (&m_PWCDisp_TXStatus,    0, sizeof(m_PWCDisp_TXStatus   ));
  memset (&m_PWCDisp_RXBuff,      0, sizeof(m_PWCDisp_RXBuff     ));
  memset (&m_PWCDisp_TXBuff,      0, sizeof(m_PWCDisp_TXBuff     ));
  memset (&m_PWCDisp_RXDataFrame, 0, sizeof(m_PWCDisp_RXDataFrame));
  memset (&m_PWCDisp_TXParamList, 0, sizeof(m_PWCDisp_TXParamList));
  memset (m_PWCDisp_DataStore,    0, sizeof(m_PWCDisp_DataStore  ));
  m_PWCDisp_RXStatus.port = (UINT8)UART_NUM_OF_UARTS;

  // Initialize the Write and Read index into the Raw RX Buffer
  PWCDispProtocol_Resync();
  
  // Update m_PWCDisp_RXBuff entries - Debug
  buff_ptr         = (PWCDISP_RAW_RX_BUFFER_PTR) &m_PWCDisp_RXBuff;
  buff_ptr->pStart = (UINT8 *) &buff_ptr->data[0]; 
  buff_ptr->pEnd   = (UINT8 *) &buff_ptr->data[PWCDISP_MAX_RAW_RX_BUF]; 

  // Add an entry in the user message handler table for PWCDisp Cfg Items
  User_AddRootCmd(&pwcDispProtocolRootTblPtr);

  // Update Start up Debug
  m_PWCDisp_Debug.bDebug       = FALSE;
  m_PWCDisp_Debug.protocol     = RX_PROTOCOL;
  m_PWCDisp_Debug.bNewRXFrame  = FALSE;
  m_PWCDisp_Debug.bNewTXFrame  = FALSE;
  
  // Update RX Status
  pRXStatus = (PWCDISP_RX_STATUS_PTR) &m_PWCDisp_RXStatus;
  CM_GetTimeAsTimestamp((TIMESTAMP *) &pRXStatus->lastFrameTS);
  pRXStatus->lastFrameTime = CM_GetTickCount();
  
  
  // Update TX Status
  pTXStatus = (PWCDISP_TX_STATUS_PTR) &m_PWCDisp_TXStatus;
  CM_GetTimeAsTimestamp((TIMESTAMP *) &pTXStatus->lastFrameTS);
  pTXStatus->lastFrameTime = CM_GetTickCount();
  pTXStatus->txTimer = TX_TIMER_OFFSET; 
  
  // Create PWC Display Protocol Display Task
  memset(&pwcTaskInfo, 0, sizeof(pwcTaskInfo));
  strncpy_safe(pwcTaskInfo.Name, sizeof(pwcTaskInfo.Name),
               "PWCDisp Display Debug",_TRUNCATE);
  pwcTaskInfo.TaskID         = PWCDisp_Debug;
  pwcTaskInfo.Function       = PWCDispDebug_Task;
  pwcTaskInfo.Priority       = taskInfo[PWCDisp_Debug].priority;
  pwcTaskInfo.Type           = taskInfo[PWCDisp_Debug].taskType;
  pwcTaskInfo.modes          = taskInfo[PWCDisp_Debug].modes;
  pwcTaskInfo.MIFrames       = taskInfo[PWCDisp_Debug].MIFframes;
  pwcTaskInfo.Rmt.InitialMif = taskInfo[PWCDisp_Debug].InitialMif;
  pwcTaskInfo.Rmt.MifRate    = taskInfo[PWCDisp_Debug].MIFrate;
  pwcTaskInfo.Enabled        = TRUE;
  pwcTaskInfo.Locked         = FALSE;
  pwcTaskInfo.pParamBlock    = &pwcDisplayDebugBlock;
  TmTaskCreate(&pwcTaskInfo);
}

/******************************************************************************
 * Function:    PWCDispProtocol_Handler
 *
 * Description: Processes raw serial data  
 *
 * Parameters:  [in] data: Pointer to a location containing a block of data
 *                         read from the UART
 *              [in] cnt:  Size of "data" in bytes
 *              [in] ch:   Channel (UART 1,2,or 3) the data was read from
 *              [in] runtime_data_ptr: Location that has scaling data for 
 *                                     the parameters stored in the 
 *                                     buffer
 *              [in] info_ptr:         Location that has dumplist definition
 *                                     data
 *
 * Returns:     TRUE - Protocol Sync has been obtained 
 *              
 * Notes:       None
 *
 *****************************************************************************/
BOOLEAN  PWCDispProtocol_Handler ( UINT8 *data, UINT16 cnt, UINT16 ch, 
                                   void* runtime_data_ptr, void* info_ptr ) 
{
  PWCDISP_TX_STATUS_PTR       tx_status_ptr;
  PWCDISP_RX_STATUS_PTR       rx_status_ptr;
  PWCDISP_RAW_RX_BUFFER_PTR   rx_buff_ptr;
  PWCDISP_RAW_TX_BUFFER_PTR   tx_buff_ptr;
  PWCDISP_TX_PARAM_LIST_PTR   tx_param_ptr;
  UINT8     *dest_ptr;
  UINT8     *end_ptr;
  BOOLEAN   bNewData;
  BOOLEAN   bBadTXPacket;
  BOOLEAN   bTXFrame;
  UINT16    i, sent_cnt;
  
  bNewData        = FALSE;
  bBadTXPacket    = FALSE;
  bTXFrame        = FALSE;
  tx_status_ptr   = (PWCDISP_TX_STATUS_PTR) &m_PWCDisp_TXStatus;
  rx_status_ptr   = (PWCDISP_RX_STATUS_PTR) &m_PWCDisp_RXStatus;
  tx_buff_ptr     = (PWCDISP_RAW_TX_BUFFER_PTR) &m_PWCDisp_TXBuff;
  rx_buff_ptr     = (PWCDISP_RAW_RX_BUFFER_PTR) &m_PWCDisp_RXBuff;
  tx_param_ptr    = (PWCDISP_TX_PARAM_LIST_PTR) &m_PWCDisp_TXParamList;
  dest_ptr        = rx_buff_ptr-> pWr;
  end_ptr         = (UINT8 *) &rx_buff_ptr->data[PWCDISP_MAX_RAW_RX_BUF]; 
  
  // If there is new data or still data left in the buffer to check.
  if((cnt > 0) || (rx_buff_ptr->cnt > PWCDISP_HEADER_SIZE))
  {
    // Copy any data into buffer
    for (i=0; i<cnt; i++)
    {
      *dest_ptr++ = *data++; //NOTE: I really like this little technique here
      // Wrap Raw Buffer (a.k.a. roll-over)
      if(dest_ptr >= end_ptr)
      {
        dest_ptr = (UINT8 *)rx_buff_ptr->data;
      }
    }
    
    // Update index for next write
    rx_buff_ptr->pWr = dest_ptr;
    
    // Update buff size
    rx_buff_ptr->cnt += cnt;
    
    if(rx_buff_ptr->cnt >= PWCDISP_MAX_RAW_RX_BUF)
    {
      // Purge old data and force to max buffer size
      rx_buff_ptr->cnt = PWCDISP_MAX_RAW_RX_BUF;
      rx_buff_ptr->pRd = rx_buff_ptr->pWr;
    }
    
    // Attempt to synchronize and decode RX raw buffer
    bNewData = PWCDispProtocol_FrameSearch(rx_buff_ptr);
    dataLossCounter = 0;
  }
  else if (++dataLossCounter == PWCDISP_DATALOSSWAIT)
  {
    dataLossFlag = (UINT8)DIO_DISPLAY_DATA_LOSS_FAIL;
  }
  
  //Begin Processing TX message packets every 10 MIF Frames
  if(tx_status_ptr->txTimer % PWCDISP_TX_FREQUENCY == 0)
  {    
    //Request new data from Display Processing Application
    bTXFrame = PWCDispProtocol_ReadTXData(tx_param_ptr);
    //Verify that all characters are approved and encode packet contents
    bBadTXPacket =
    PWCDispProtocol_TXCharacterCheck(tx_param_ptr, tx_buff_ptr);
    //Update TX Debug Frame if there is new data.
    if (m_PWCDisp_Debug.bNewTXFrame != TRUE)
    {
      m_PWCDisp_Debug.bNewTXFrame = bTXFrame;
    }
    if(!bBadTXPacket)
    {
      UART_Transmit(UartMgr_GetChannel((UINT32)UARTMGR_PROTOCOL_PWC_DISPLAY), 
                   (const INT8*) &tx_buff_ptr->data[0], 
                   PWCDISP_TX_MSG_SIZE, &sent_cnt);
    }
    //Reset txTimer. If there is a bad packet the timer will reset to 10 in 
    //order to refresh the tx data ASAP
    tx_status_ptr->txTimer = (!bBadTXPacket) ? 1 : PWCDISP_TX_FREQUENCY;
  }
  else
  {
    tx_status_ptr->txTimer++;
  }

  // If there is new, recognized data then Update the UART Manager
  if(bNewData)
  {
    PWCDispProtocol_UpdateUartMgr();
  }
  
  // Update current frame time
  CM_GetTimeAsTimestamp((TIMESTAMP *) &rx_status_ptr->lastFrameTS);
  CM_GetTimeAsTimestamp((TIMESTAMP *) &tx_status_ptr->lastFrameTS);
  rx_status_ptr->lastFrameTime = CM_GetTickCount();
  tx_status_ptr->lastFrameTime = CM_GetTickCount();

  return(bNewData);
}


/******************************************************************************
 * Function:    PWCDispProtocol_GetRXStatus
 *
 * Description: Utility function to request current PWCDispProtocol 
 *              status
 *
 * Parameters:  None 
 *
 * Returns:     Ptr to PWCDisp Status data 
 *
 * Notes:       None 
 *
 *****************************************************************************/
static
PWCDISP_RX_STATUS_PTR PWCDispProtocol_GetRXStatus (void)
{
  return ((PWCDISP_RX_STATUS_PTR) &m_PWCDisp_RXStatus);
}

/******************************************************************************
 * Function:    PWCDispProtocol_GetTXStatus
 *
 * Description: Utility function to request current PWCDispProtocol 
 *              status
 *
 * Parameters:  None 
 *
 * Returns:     Ptr to PWCDisp Status data 
 *
 * Notes:       None 
 *
 *****************************************************************************/
static
PWCDISP_TX_STATUS_PTR PWCDispProtocol_GetTXStatus (void)
{
  return ((PWCDISP_TX_STATUS_PTR) &m_PWCDisp_TXStatus);
}

/******************************************************************************
 * Function:    PWCDispProtocol_GetDebug
 *
 * Description: Utility function to request current PWC Display Protocol debug 
 *              state 
 *
 * Parameters:  None
 *
 * Returns:     Ptr to PWCDisp Debug Data
 *
 * Notes:       None 
 *
 *****************************************************************************/
static
PWCDISP_DEBUG_PTR PWCDispProtocol_GetDebug (void)
{
  return((PWCDISP_DEBUG_PTR) &m_PWCDisp_Debug);
}

/******************************************************************************
 * Function:    PWCDispProtocol_UpdateUartMgr
 *
 * Description: Utility function to update Uart Mgr Data on new frame 
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:       None 
 *
 *****************************************************************************/
static 
void PWCDispProtocol_UpdateUartMgr(void)
{
  PWCDISP_RX_DATA_FRAME_PTR frame_ptr;
  UARTMGR_PROTOCOL_DATA_PTR pData;
  UARTMGR_PROTOCOL_DATA_PTR pCurrentData;
  UINT16 i;
  UINT16 size;
  UINT16 cnt;

  frame_ptr = &m_PWCDisp_RXDataFrame;
  pData = &m_PWCDisp_RXData[0];
  cnt = 0;

  // Save RX Packet ID to Table
  pCurrentData = pData + (UINT16)RXPACKETID;
  pCurrentData->data.value = frame_ptr->data[cnt];
  cnt++;

  pCurrentData = pData + (UINT16)BUTTONSTATE_RIGHT;
  size = (UINT16)(BUTTONSTATE_EVENT - BUTTONSTATE_RIGHT) + 1;

  // Parse Button States and save to table
  for (i = 0; i < size; i++)
  {
    (pCurrentData + i)->data.value =
      (BOOLEAN)((frame_ptr->data[cnt] >> i) & (0x01));
  }
  cnt++;

  pCurrentData = pData + (UINT16)DOUBLECLICK_RIGHT;
  size = (UINT16)(DOUBLECLICK_EVENT - DOUBLECLICK_RIGHT) + 1;
  // Parse double click states and save to table
  for (i = 0; i < size; i++)
  {
    (pCurrentData + i)->data.value =
      (BOOLEAN)((frame_ptr->data[cnt] >> i) & (0x01));
  }
  cnt++;
  
  // Parse Discrete states and save to table
  pCurrentData = pData + (UINT16)DISCRETE_ZERO;
  size = (UINT16)(DISCRETE_SEVEN - DISCRETE_ZERO) + 1;
  for (i = 0; i < size; i++)
  {
    (pCurrentData + i)->data.value = 
      (BOOLEAN)((frame_ptr->data[cnt] >> i) & (0x01));
  }
  cnt++;

  // Save Display Health byte to table
  pCurrentData = pData + (UINT16)DISPLAY_HEALTH;
  pCurrentData->data.value = frame_ptr->data[cnt];
  cnt++;

  // Save Part Number to table
  pCurrentData = pData + (UINT16)PART_NUMBER;
  pCurrentData->data.value = frame_ptr->data[cnt];
  cnt++;

  // Save Version Number to table
  pCurrentData = pData + (UINT16)VERSION_NUMBER;
  pCurrentData->data.value = frame_ptr->data[cnt];
  
  // Write the contents of the table to the UART manager
  UartMgr_Write(pData, (UINT32)UARTMGR_PROTOCOL_PWC_DISPLAY, 
               (UINT16)RX_PROTOCOL, (UINT16)PWCDISP_RX_ELEMENT_COUNT);
}
/******************************************************************************
 * Function:    PWCDispProtocol_Resync()
 *
 * Description: Utility function to force re-sync of incoming data 
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:       None 
 *
 *****************************************************************************/
static
void PWCDispProtocol_Resync(void)
{
  PWCDISP_RAW_RX_BUFFER_PTR buff_ptr;

  buff_ptr = (PWCDISP_RAW_RX_BUFFER_PTR) &m_PWCDisp_RXBuff;
  
  buff_ptr->cnt = 0;
  buff_ptr->pRd = (UINT8 *) &buff_ptr->data[0];
  buff_ptr->pWr = buff_ptr->pRd;
}

/******************************************************************************
 * Function:    PWCDispProtocol_FrameSearch
 *
 * Description: Function to search through raw data buffer in order to locate
 *              the PWC Display Parameter Frame 
 *
 * Parameters:  rx_buff_ptr - pointer to the RX Raw Data Buffer
 *
 * Returns:     TRUE if a New Packet is Found
 *
 * Notes:       
 *   1) Searches for Sync Word first
 *   2) Use the Sync Word to move the data into a flat buffer
 *   3) Verify Packet size
 *   4) Verify Packet ID
 *   5) Verify Packet Checksum
 *   6) If Packet is okay, then call process packet function 
 *
 *****************************************************************************/ 
static
BOOLEAN PWCDispProtocol_FrameSearch(PWCDISP_RAW_RX_BUFFER_PTR rx_buff_ptr)
{
  BOOLEAN bNewData;
  BOOLEAN bContinueSearch;
  BOOLEAN bLookingForMsg;
  BOOLEAN bBadPacket;
  UINT8 *src_ptr;
  UINT8 *end_ptr;
  UINT8 *data;
  UINT8 buff[PWCDISP_MAX_RAW_RX_BUF];
  UINT16 i;
  UINT16 j;
  UINT16 k;
  UINT16 checksum;
  PWCDISP_PARAM_SEARCH_ENUM stateSearch;
  PWCDISP_RX_STATUS_PTR pStatus;
  PWCDISP_RX_DATA_FRAME_PTR frame_ptr;
  
  bNewData        = FALSE;
  bContinueSearch = TRUE;
  bLookingForMsg  = TRUE;
  bBadPacket      = FALSE;
  src_ptr         = (UINT8 *) &buff[0];
  end_ptr         = (UINT8 *) &buff[rx_buff_ptr->cnt-(PWCDISP_HEADER_SIZE-1)];
  stateSearch     = PWCDISP_PARAMSRCH_HEADER;
  pStatus         = (PWCDISP_RX_STATUS_PTR)&m_PWCDisp_RXStatus;
  frame_ptr       = &m_PWCDisp_RXDataFrame;
  data            = (UINT8 *) &frame_ptr->data[0];
  
  //Copy data from rolling buffer to a flat buffer with recent data first
  i = (UINT16)((UINT32)&rx_buff_ptr->data[PWCDISP_MAX_RAW_RX_BUF] -
      (UINT32)rx_buff_ptr->pRd);
  j = (UINT16)((UINT32)rx_buff_ptr->pRd - (UINT32)&rx_buff_ptr->data[0]);
  memcpy((UINT8 *)&buff[0], rx_buff_ptr->pRd, i);
  memcpy((UINT8 *)&buff[i], (UINT8 *)&rx_buff_ptr->data[0], j);
  i = 0;

  while(bContinueSearch == TRUE)
  {
    switch (stateSearch)
    {
      case PWCDISP_PARAMSRCH_HEADER:
        // Begin scanning for header while there is space for a message header
        while (src_ptr < end_ptr && bLookingForMsg == TRUE)
        { 
          if (*src_ptr == PWCDISP_SYNC_BYTE1             &&
              *(src_ptr + (UINT16)PWCDISP_RX_PARAM_SYNC2) 
                 == PWCDISP_SYNC_BYTE2                   &&
              *(src_ptr + (UINT16)PWCDISP_RX_PARAM_SIZE)  
                 == PWCDISP_MAX_RX_LIST_SIZE             &&
              *(src_ptr + (UINT16)PWCDISP_RX_PARAM_PACKETID) 
                 == PWCDISP_RX_PACKET_ID)
          {
            // Discontinue searching for message header
            bLookingForMsg = FALSE;
          }
          else
          {
            if(pStatus->sync == TRUE)
            {
              // Lost sync. Increment invalid packet
              pStatus->sync               = FALSE;
              pStatus->lastSyncLossReason = SYNC_LOSS_RSN_HEADER_INVALID;
              bBadPacket                  = TRUE;
              m_PWCDisp_Debug.bNewRXFrame = TRUE;
              dataLossFlag = (UINT8)DIO_DISPLAY_SYNC_LOSS_FAIL;
            }
            src_ptr++; i++;
          }
        }
        // Header found. Begin Data extraction.
        // Is there enough data for a full message??
        if(bLookingForMsg == FALSE &&
          ((src_ptr + ((UINT16)PWCDISP_RX_PARAM_COUNT - 1)) < 
           (end_ptr + (PWCDISP_HEADER_SIZE-1)))) 
        {
          stateSearch = PWCDISP_PARAMSRCH_EXTRACT_DATA;
        }
        else // at least four bytes left in flat buffer
        {
          // Not enough data. Try again when there is more data
          stateSearch = PWCDISP_PARAMSRCH_UPDATE_BUFFER;
        }
        break;

      case PWCDISP_PARAMSRCH_EXTRACT_DATA:
        // Save sync_word, size, and checksum to Frame Buffer
        frame_ptr->sync_word = (*src_ptr << BYTE_LENGTH) | 
                               (*(src_ptr + (UINT16)PWCDISP_RX_PARAM_SYNC2));
        frame_ptr->size      = *(src_ptr + (UINT16)PWCDISP_RX_PARAM_SIZE);
        frame_ptr->chksum    = 
          (*(src_ptr + (UINT16)PWCDISP_RX_PARAM_CHKSUM_HIGH) << BYTE_LENGTH) |
          (*(src_ptr + (UINT16)PWCDISP_RX_PARAM_CHKSUM_LOW));
        
        // Adjust buffer position to Packet ID
        src_ptr += (UINT16)PWCDISP_RX_PARAM_PACKETID;
        i += (UINT16)PWCDISP_RX_PARAM_PACKETID;

        // Add data to Frame buffer
        for (k = 0; k < frame_ptr->size; k++)
        {
          *data++ = *src_ptr++;
        }

        // Ready Packet ID verification
        stateSearch = PWCDISP_PARAMSRCH_CHECKSUM;
        break;

      case PWCDISP_PARAMSRCH_CHECKSUM:
        // Calculate packet Checksum
        checksum = 0;
        checksum += (UINT8)(frame_ptr->sync_word >> BYTE_LENGTH);
        checksum += (UINT8)(frame_ptr->sync_word);
        checksum += (UINT16) frame_ptr->size;
        for(k = 0; k < frame_ptr->size; k++)
        {
          checksum += (UINT16) frame_ptr->data[k];
        }

        // Verify packet Checksum and Update Status
        if (frame_ptr->chksum == checksum)
        { 
          // Set bNewData to TRUE to indicate a new recognized message
          // and update status
          bNewData = TRUE;
          if (!(pStatus->sync)){
            CM_GetTimeAsTimestamp((TIMESTAMP *)&pStatus->lastSyncTS);
            pStatus->recentSyncCnt = 1;
          }
          else{
            pStatus->recentSyncCnt++;
          }
          pStatus->sync = TRUE;
          pStatus->syncCnt++;
          pStatus->lastSyncPeriod = (CM_GetTickCount() -
                                    pStatus->lastSyncTime);
          pStatus->lastSyncTime   = CM_GetTickCount();

          // New Msg. Update Packet Contents minus the two checkum bytes.
          memcpy(pStatus->packetContents, src_ptr-(PWCDISP_RX_MAX_MSG_SIZE-2),
                 PWCDISP_RX_MAX_MSG_SIZE);

          //Update Status and Ready Packet Size Verification
          pStatus->payloadSize = frame_ptr->size;
          pStatus->chksum = frame_ptr->chksum;

          // Message is good
          m_PWCDisp_Debug.bNewRXFrame = TRUE;
          // Update the new read position to the end of the message
          // (excluding the three bytes that have been scanned thus far 
          //  in the header)
          i += (PWCDISP_RX_MAX_MSG_SIZE - (PWCDISP_HEADER_SIZE - 1));
          stateSearch = PWCDISP_PARAMSRCH_UPDATE_BUFFER;
        }
        else
        {
          // Checksum is incorrect. Invalid message/No message
          // continue scanning for message header.
          stateSearch = PWCDISP_PARAMSRCH_HEADER;
          if(pStatus->sync == TRUE)
          {
            // Lost sync. Increment invalid packet
            pStatus->sync               = FALSE;
            bBadPacket                  = TRUE;
            pStatus->lastSyncLossReason = SYNC_LOSS_CHECKSUM_INVALID;
            m_PWCDisp_Debug.bNewRXFrame = TRUE;
          }
          dataLossFlag = (UINT8)DIO_DISPLAY_SYNC_LOSS_FAIL;
        }
        break;
      case PWCDISP_PARAMSRCH_UPDATE_BUFFER:
        bContinueSearch = FALSE;
        PWCDispProtocol_UpdateRdPtr(rx_buff_ptr, i);
        break;
    }
  }
  // Update bad packet counter status.
  PWCDispProtocol_ValidRXPacket(bBadPacket);

  return (bNewData);
}

/******************************************************************************
* Function:    PWCDispProtocol_UpdateRdPtr
*
* Description: Utility Function to update a given rx read buffer based on the
*              amount of bytes scanned.
*
* Parameters:  rx_buff_ptr - The Raw RX buffer structure pointer
*              bytesScanned - The total number of bytes scanned.
*
* Returns:     None
*
* Notes:       None
*
*****************************************************************************/
static
void PWCDispProtocol_UpdateRdPtr(PWCDISP_RAW_RX_BUFFER_PTR rx_buff_ptr, 
                                 UINT16 bytesScanned)
{
  UINT16 k;
  rx_buff_ptr->cnt -= bytesScanned;

  // Update the pRd pointer.
  for (k = 0; k < bytesScanned; k++)
  {
    rx_buff_ptr->pRd++;
    if (rx_buff_ptr->pRd >= &rx_buff_ptr->data[PWCDISP_MAX_RAW_RX_BUF])
    {
        rx_buff_ptr->pRd = &rx_buff_ptr->data[0];
    }
  }
}

/******************************************************************************
 * Function:    PWCDispProtocol_ValidRXPacket
 *
 * Description: Function to update statuses and logs of invalid or corrupt
 *              RX packets. 
 *
 * Parameters:  bBadPacket - flag to indicate the validity of an RX packet
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/ 
static 
void PWCDispProtocol_ValidRXPacket(BOOLEAN bBadPacket)
{
  PWCDISP_SYNC_LOSS_LOG invalidPktLog;
  PWCDISP_RX_STATUS_PTR pStatus;
  
  pStatus         = (PWCDISP_RX_STATUS_PTR)&m_PWCDisp_RXStatus;
  
  // Message was deemed invalid. Update status and record any necessary logs.
  if(bBadPacket == TRUE)
  {
    pStatus->invalidSyncCnt++;
    invalidPktLog.reason           = pStatus->lastSyncLossReason;
    invalidPktLog.validSyncCnt     = pStatus->recentSyncCnt;
    invalidPktLog.achievedSyncTime = pStatus->lastSyncTS;
    LogWriteSystem(SYS_ID_UART_DISP_SYNC_LOSS, LOG_PRIORITY_LOW,
                   &invalidPktLog, sizeof(PWCDISP_SYNC_LOSS_LOG), NULL);
  }
}

/******************************************************************************
* Function:    PWCDispProtocol_ValidTXPacket
*
* Description: Function to update statuses and logs of invalid or corrupt
*              TX packets from the Display.
*
* Parameters:  bBadPacket - flag to indicate the validity of an RX packet
*
* Returns:     None
*
* Notes:       None
*
*****************************************************************************/
static
void PWCDispProtocol_ValidTXPacket(BOOLEAN bBadPacket)
{
  PWCDISP_TXPACKET_FAIL_LOG invalidPktLog;
  PWCDISP_TX_STATUS_PTR pStatus;
  UINT8 charString[PWCDISP_MAX_CHAR_PARAMS];
  UINT16 i;
  
  pStatus = (PWCDISP_TX_STATUS_PTR)&m_PWCDisp_TXStatus;

  // Display Packet was deemed invalid. Record logs.
  if(bBadPacket == TRUE)
  {
    for (i = 0; i < PWCDISP_MAX_CHAR_PARAMS; i++)
    {
      charString[i] = pStatus->packetContents[i + (PWCDISP_HEADER_SIZE)];
    }
    snprintf((char *)invalidPktLog.packetContents, PWCDISP_MAX_CHAR_PARAMS + 1,
             "%s", (const char *)charString);
    invalidPktLog.validPacketCnt = pStatus->validPacketCnt;
    LogWriteSystem(SYS_ID_UART_DISP_TXPACKET_FAIL, LOG_PRIORITY_LOW,
                   &invalidPktLog, sizeof(PWCDISP_TXPACKET_FAIL_LOG), 
                   NULL);    
  }
}

/******************************************************************************
 * Function:    PWCDispProtocol_ReadTXData
 *
 * Description: Function to Read TX Packet data from the Display Processing
 *              Application. 
 *
 * Parameters:  tx_param_ptr - pointer to the unencoded TX parameter data.
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/ 
static 
BOOLEAN PWCDispProtocol_ReadTXData(PWCDISP_TX_PARAM_LIST_PTR tx_param_ptr)
{
  // Acquire character Parameters from the Display Processing Application
  UARTMGR_PROTOCOL_DATA_PTR pData;
  UARTMGR_PROTOCOL_DATA_PTR pCurrentData;
  BOOLEAN bNewData;
  UINT16 i;
  UINT16 size;
  UINT8 *pDest;

  pData = (UARTMGR_PROTOCOL_DATA_PTR)&m_PWCDisp_TXData[0];
  bNewData = UartMgr_Read(pData, (UINT32)UARTMGR_PROTOCOL_PWC_DISPLAY, 
                         (UINT16)TX_PROTOCOL,
                         (UINT16)PWCDISP_TX_ELEMENT_CNT_COMPLETE);

  // Aquire new data if available
  if (bNewData == TRUE)
  {
    bNewData     = FALSE;
    pCurrentData = pData + (UINT16)CHARACTER_0;
    size         = PWCDISP_MAX_CHAR_PARAMS;

    pDest   = (UINT8 *) &tx_param_ptr->charParam[0];

    for (i = 0; i < size; i++)
    {
      if(*pDest != (UINT8)(pCurrentData->data.value))
      {
        bNewData = TRUE;
      }
      *pDest = (UINT8)pCurrentData->data.value;
      pDest++; pCurrentData++;
    }
    
    pCurrentData = pData + (UINT16)DC_RATE;
    if(tx_param_ptr->doubleClickRate != (INT8)pCurrentData->data.value)
    {
      bNewData = TRUE;
    }
    tx_param_ptr->doubleClickRate = (INT8) pCurrentData->data.value;    
  }
  return(bNewData);
}

/******************************************************************************
 * Function:    PWCDispProtocol_TXCharacterCheck
 *
 * Description: Function to search through unencoded TX parameter data and 
 *              verify that all characters are on the list of approved 
 *              characters in the ER8890 document. When this is completed
 *              the function will encode the data into a Raw Serial buffer.
 *
 * Parameters:  tx_param_ptr - pointer to the unencoded TX parameter data.
 *              tx_buff_ptr - pointer to encoded TX parameter data.
 *
 * Returns:     TRUE if a character in the parameter data fails its check.
 *              FALSE if all characters in the parameter data pass their 
 *              checks.
 *
 * Notes:       
 *   1) Adds Sync bytes, Payload size and Packet ID to the raw buffer first
 *   2) Verifies that each character byte is between 0x00 and 0x7F and adds
 *      them to the raw buffer.
 *   3) If a character fails the check the function will cease the character
 *      check and return a value of TRUE.
 *   4) When all characters pass the check the DCRATE byte will be added
 *   5) The checksum will be calculated and added to the raw buffer
 *   6) The function will return a value of FALSE
 *
 *****************************************************************************/ 
static 
BOOLEAN PWCDispProtocol_TXCharacterCheck(PWCDISP_TX_PARAM_LIST_PTR tx_param_ptr,
                                         PWCDISP_RAW_TX_BUFFER_PTR tx_buff_ptr)
{
  UINT8 *buff;
  UINT8 *data;
  UINT16 i;
  UINT16 checksum;
  BOOLEAN bBadTXPacket;
  BOOLEAN bContinueCheck;
  PWCDISP_TX_STATUS_PTR pStatus;
  
  buff           = (UINT8 *) &tx_buff_ptr->data[0];
  data           = (UINT8 *) &tx_param_ptr->charParam[0];
  checksum       = 0;
  bBadTXPacket   = FALSE;
  bContinueCheck = TRUE;
  pStatus        = (PWCDISP_TX_STATUS_PTR)&m_PWCDisp_TXStatus;
  
  while(bContinueCheck == TRUE && bBadTXPacket == FALSE)
  {
    switch(stateCheck)
    {
      case PWCDISP_PARAM_ENCODE_HEADER:
        // Set the Header of the message buffer on the first TX packet check
        *(buff + (UINT16)PWCDISP_TX_PARAM_SYNC1)    = PWCDISP_SYNC_BYTE1;
        *(buff + (UINT16)PWCDISP_TX_PARAM_SYNC2)    = PWCDISP_SYNC_BYTE2;
        *(buff + (UINT16)PWCDISP_TX_PARAM_SIZE)     = PWCDISP_MAX_TX_LIST_SIZE;
        *(buff + (UINT16)PWCDISP_TX_PARAM_PACKETID) = PWCDISP_TX_PACKET_ID;
        // Ready Character Check and Encoding
        stateCheck = PWCDISP_PARAM_ENCODE_CHARACTERS;
        break;
      
      case PWCDISP_PARAM_ENCODE_CHARACTERS:
        // set pointer to the first non header byte in the buffer
        buff += (UINT16)PWCDISP_TX_PARAM_CHARDATA; 
        for(i = 0; i < PWCDISP_MAX_CHAR_PARAMS; i++)
        {
          // If the character is not approved abort loop and report bad packet
          if(*data > TPU(0x7F, eTpPwcDispChr) && bBadTXPacket != TRUE)
          {
            bBadTXPacket = TRUE;
            pStatus->invalidPacketCnt++;
          }
          // Add the character to the buffer 
          *buff++ = *data++;
        }
        
        *buff = (UINT8)tx_param_ptr->doubleClickRate;
        
        // Ready Checksum Calculation and Encoding
        stateCheck = PWCDISP_PARAM_ENCODE_CHECKSUM;
        break;
      
      case PWCDISP_PARAM_ENCODE_CHECKSUM:
        // Update Status and Reset Buffer pointer
        pStatus->lastPacketPeriod = CM_GetTickCount() - 
                                    pStatus->lastPacketTime;
        pStatus->lastPacketTime   = CM_GetTickCount();
        pStatus->validPacketCnt++;
        buff = (UINT8 *) &tx_buff_ptr->data[0];
        // Calculate Checksum
        for (i = 0; i < PWCDISP_TX_MSG_SIZE - 2; i++)
        {
          checksum += *buff++;
        }
        // Encode Checksum into Raw Buffer
        *buff       = (UINT8) ((checksum >> 8) & 0xFF);
        *(buff + 1) = (UINT8) (checksum & 0xFF);
        
        // End Loop
        bContinueCheck = FALSE;
        break;
      
      default:
        FATAL("Unsupported PWC TX Param State = %d", stateCheck);
        break;
    }
  }
  // Update status, reset stateCheck switch, and return bBadTXPacket
  for (i = 0; i < PWCDISP_TX_MSG_SIZE; i++){
    pStatus->packetContents[i] = tx_buff_ptr->data[i];
  }
  stateCheck = PWCDISP_PARAM_ENCODE_CHARACTERS;
  // Record a System Log if the Packet is bad.
  PWCDispProtocol_ValidTXPacket(bBadTXPacket);
  return(bBadTXPacket);
}

/******************************************************************************
* Function:    PWCDispDebug_Task
*
* Description: Utility function to display PWC Display frame data for a 
*              specific Protocol Direction
*
* Parameters:  pParam: Not used, only to match Task Mgr call signature
*
* Returns:     None
*
* Notes:       None
*
*****************************************************************************/
static 
void PWCDispDebug_Task(void *pParam)
{
  PWCDISP_DEBUG_PTR pDebug;
  
  pDebug = &m_PWCDisp_Debug;
  
  if(pDebug->bDebug == TRUE)
  {
    if(pDebug->protocol == RX_PROTOCOL && pDebug->bNewRXFrame == TRUE)
    {
      PWCDispDebug_RX(pParam);
    }
    else if (pDebug->protocol == TX_PROTOCOL && pDebug->bNewTXFrame == TRUE)
    {
      PWCDispDebug_TX(pParam);
    }
  }
}


/******************************************************************************
* Function:    PWCDispProtocol_TranslateArrows
*
* Description: Utility Function to replace Hex directional characters with 
*              "v,^,<,>,@"
*
* Parameters:  char   charString[] - The string to be edited.
*              UINT16 length       - The length of the string to be edited.
*
* Returns:     none
*
* Notes:       none
*
*****************************************************************************/
void PWCDispProtocol_TranslateArrows(CHAR charString[], UINT16 length)
{
  UINT16 i;
  for (i = 0; i < length; i++)
  {
    switch(charString[i])
    {
      case (UINT16) RIGHT_ARROW:
        charString[i] = '>';
        break;
      case (UINT16) LEFT_ARROW:
        charString[i] = '<';
        break;
      case (UINT16) UP_ARROW:
        charString[i] = '^';
        break;
      case (UINT16) DOWN_ARROW:
        charString[i] = 'v';
        break;
      case (UINT16) ENTER_DOT:
        charString[i] = '@';
        break;
      default:
        break;
    }
  }
}

/******************************************************************************
* Function:    PWCDispDebug_RX
*
* Description: Utility function to display PWC Display frame data for the 
*              RX Protocol Direction
*
* Parameters:  pParam: Not used, only to match Task Mgr call signature
*
* Returns:     None
*
* Notes:       None
*
*****************************************************************************/
static void PWCDispDebug_RX(void *pParam)
{
  PWCDISP_DEBUG_PTR pDebug;
  PWCDISP_RX_STATUS_PTR pStatus;
  PWCDISP_PARAM_NAMES_PTR pName;
  UINT16 i;
  UINT8 subString[PWCDISP_SUBSTRING_MAX];
  
  pDebug    = &m_PWCDisp_Debug;
  pStatus   = (PWCDISP_RX_STATUS_PTR)&m_PWCDisp_RXStatus;
  pName     = (PWCDISP_PARAM_NAMES_PTR)&pwcDisp_RX_Param_Names[0];

  
  pStatus->chksum = ((UINT16)pStatus->packetContents
                      [(UINT16)PWCDISP_RX_PARAM_CHKSUM_HIGH] << 8) |
                    ((UINT16)pStatus->packetContents
                      [(UINT16)PWCDISP_RX_PARAM_CHKSUM_LOW]);

  // Display Address Checksum
  m_Str[0] = NULL;
  snprintf((char *) m_Str, 
    DISP_PRINT_PARAMF("\r\nChksum         =0x%04x\r\n", pStatus->chksum));
  GSE_PutLine((const char *)m_Str);
  
  // Display Size
  snprintf((char *)m_Str, 
    DISP_PRINT_PARAMF("Size           =%d\r\n", pStatus->payloadSize));
  GSE_PutLine((const char * )m_Str);

  
  // Display Raw Buffer
  // Display Sync Byte 1
  snprintf((char *)m_Str, DISP_PRINT_PARAMF("SYNC1          =0x%02x\r\n",
    pStatus->packetContents[(UINT16)PWCDISP_RX_PARAM_SYNC1]));
  GSE_PutLine( (const char *) m_Str );
    
  // Display Sync Byte 2
  snprintf((char *)m_Str, DISP_PRINT_PARAMF("SYNC2          =0x%02x\r\n",
    pStatus->packetContents[(UINT16)PWCDISP_RX_PARAM_SYNC2]));
  GSE_PutLine( (const char *) m_Str );
  
  // Display Size Byte
  snprintf((char *)m_Str, DISP_PRINT_PARAMF("SIZE_BYTE      =0x%02x\r\n",
    pStatus->packetContents[(UINT16)PWCDISP_RX_PARAM_SIZE]));
  GSE_PutLine( (const char *) m_Str );

  m_Str[0] = NULL;
    
  //Display Payload Contents
  for( i = 0; i < PWCDISP_MAX_RX_LIST_SIZE; i++)
  {
    snprintf((char *)m_Str, DISP_PRINT_PARAM((pName + i)->name));
    snprintf((char*)subString, DISP_PRINT_PARAMF(" =0x%02x\r\n",
      pStatus->packetContents[i + (UINT16)PWCDISP_RX_PARAM_PACKETID]));
    strcat((char *)m_Str, (const char *)subString);
    GSE_PutLine((const char *) m_Str);
    m_Str[0] = NULL;
  }

  // Display RX last packet period
  snprintf((char *)m_Str, MAX_DEBUG_STRING_SIZE,"LAST_PACKET_PERIOD =%10d\r\n",
           pStatus->lastSyncPeriod);
  GSE_PutLine( (const char *) m_Str );
  GSE_PutLine("\r\n");
    
  pDebug->bNewRXFrame = FALSE;
}

/******************************************************************************
* Function:    PWCDispDebug_TX
*
* Description: Utility function to display PWC Display frame data for the 
*              TX Protocol Direction
*
* Parameters:  pParam: Not used, only to match Task Mgr call signature
*
* Returns:     None
*
* Notes:       None
*
*****************************************************************************/
static void PWCDispDebug_TX(void *pParam)
{
  PWCDISP_DEBUG_PTR pDebug;
  PWCDISP_PARAM_NAMES_PTR pName;
  PWCDISP_TX_STATUS_PTR pStatus;
  UINT16 i;
  UINT8 subString[PWCDISP_SUBSTRING_MAX];
  
  pDebug = &m_PWCDisp_Debug;
  pName = (PWCDISP_PARAM_NAMES_PTR)&pwcDisp_TX_Param_Names[0];
  pStatus = (PWCDISP_TX_STATUS_PTR)&m_PWCDisp_TXStatus;


    m_Str[0] = NULL;
  // Display Raw Buffer
  // Display Sync byte 1
  snprintf((char *) m_Str, DISP_PRINT_PARAMF("\r\nSYNC1          =0x%02x\r\n",
      pStatus->packetContents[PWCDISP_TX_PARAM_SYNC1]));
  GSE_PutLine((const char *)m_Str);
    
  // Display Sync byte 2
  snprintf((char *) m_Str, DISP_PRINT_PARAMF("SYNC2          =0x%02x\r\n",
      pStatus->packetContents[PWCDISP_TX_PARAM_SYNC2]));
  GSE_PutLine((const char *) m_Str);
    
  // Display Size
  snprintf((char *) m_Str, DISP_PRINT_PARAMF("SIZE           =0x%02x\r\n",
      pStatus->packetContents[PWCDISP_TX_PARAM_SIZE]));
  GSE_PutLine((const char *) m_Str);
    
  m_Str[0] = NULL;
    
  // Display Packet ID and Character list
  for(i = 0; i < (UINT16)PWCDISP_TX_ELEMENT_CNT_COMPLETE - 1; i++)
  {
    snprintf((char *)m_Str, DISP_PRINT_PARAM((pName + i)->name));
    snprintf((char *)subString, DISP_PRINT_PARAMF(" =0x%02x\r\n",
        pStatus->packetContents[i+(UINT16)PWCDISP_TX_PARAM_CHARDATA]));
    strcat((char *)m_Str, (const char *)subString);
    GSE_PutLine((const char *) m_Str);
    m_Str[0] = NULL;
  }
  GSE_PutLine("\r\n");
    
  snprintf((char *)m_Str, DISP_PRINT_PARAM((pName + (UINT16)DC_RATE)->name));
  snprintf((char *) subString, DISP_PRINT_PARAMF(" =%03d ms\r\n",
           (INT8)(pStatus->packetContents
           [(UINT16)PWCDISP_TX_PARAM_CHARDATA + (UINT16)DC_RATE]) * 
           DCRATE_INT8_TO_MS_CONVERSION));
  strcat((char *)m_Str, (const char*)subString);
  GSE_PutLine((const char*) m_Str);
  GSE_PutLine("\r\n");
  m_Str[0] = NULL;

  // Display LINE 1
  snprintf((char *)m_Str, PWCDISP_MAX_LINE_LENGTH + 1, "%s", 
           (const char *)&(pStatus->packetContents
           [(UINT16)PWCDISP_TX_PARAM_CHARDATA]));
  PWCDispProtocol_TranslateArrows((CHAR *)m_Str, PWCDISP_MAX_LINE_LENGTH);
  GSE_PutLine((const char*) m_Str);
  GSE_PutLine("\r\n");
  m_Str[0] = NULL;

  // Display LINE 2
  snprintf((char *)m_Str, PWCDISP_MAX_LINE_LENGTH + 1, "%s",
           (const char *)&(pStatus->packetContents
           [(UINT16)PWCDISP_TX_PARAM_CHARDATA + PWCDISP_MAX_LINE_LENGTH]));
  PWCDispProtocol_TranslateArrows((CHAR *)m_Str, PWCDISP_MAX_LINE_LENGTH);
  GSE_PutLine((const char*)m_Str);
  GSE_PutLine("\r\n\n");

  // Display TX last packet period
  snprintf((char *)m_Str, MAX_DEBUG_STRING_SIZE, 
           "LAST_PACKET_PERIOD =%10d\r\n", pStatus->lastPacketPeriod);
  GSE_PutLine((const char *)m_Str);
  GSE_PutLine("\r\n\n");
  
  pDebug->bNewTXFrame = FALSE;
}

/******************************************************************************
 * Function:     PWCDispProtocol_DisableLiveStrm
 *
 * Description:  Disables the outputting the live stream for PWC Display 
 *
 * Parameters:   None
 *
 * Returns:      None. 
 *
 * Notes:        
 *  1) Used for debugging only 
 *
 *
 *****************************************************************************/
void PWCDispProtocol_DisableLiveStrm(void)
{
  m_PWCDisp_Debug.bDebug = FALSE;
}

/******************************************************************************
* Function:    PWCDispProtocol_Read_Handler
*
* Description: Utility function that allows a protocol to communicate in
*              simple fixed message formats with its application. In this
*              instance message data is being read from the Protocol Data
*              Store and marked as 'No Longer New Data'
*
* Parameters:  *pDest       - The destination to Read the Data store into
*              chan         - The UART channel
*              Direction    - FALSE -> RX or TRUE -> TX
*              nMaxByteSize - The total size in bytes being Read
*
* Returns:     BOOLEAN bStatus verifying New Data available
*
* Notes:       None
*
******************************************************************************/
BOOLEAN PWCDispProtocol_Read_Handler(void *pDest, UINT32 chan, 
                                     UINT16 Direction, UINT16 nMaxByteSize)
{
    //Already in code. Needs to be adapted to this framework.
    BOOLEAN *bNewData;
    BOOLEAN bStatus;
    UINT16 i;
    UARTMGR_PROTOCOL_DATA_PTR pSource;
    UARTMGR_PROTOCOL_DATA_PTR pDestination;

    bNewData = &m_PWCDisp_DataStore[Direction].bNewData;
    pSource = 
      (UARTMGR_PROTOCOL_DATA_PTR)&m_PWCDisp_DataStore[Direction].param[0];
    pDestination = (UARTMGR_PROTOCOL_DATA_PTR)pDest;

    bStatus = (*bNewData != NULL) ? *bNewData : FALSE;
    if (*bNewData == TRUE)
    {
        for (i = 0; i < nMaxByteSize; i++)
        {
            *pDestination++ = *pSource++;
        }
    }

    *bNewData = FALSE;
    return (bStatus);
}

/******************************************************************************
* Function:    PWCDispProtocol_Write_Handler
*
* Description: Utility function that allows a protocol to communicate in
*              simple fixed message formats with its application. In this
*              instance message data is being temporarily written to the
*              Protocol Data Store.
*
* Parameters:  *pDest       - The data to be written to the Data Store
*              chan         - The UART channel
*              Direction    - FALSE -> RX or TRUE -> TX
*              nMaxByteSize - The total size in bytes being written
*
* Returns:     None
*
* Notes:       None
*
******************************************************************************/
void PWCDispProtocol_Write_Handler(void *pDest, UINT32 chan, UINT16 Direction, 
                                   UINT16 nMaxByteSize)
{
    //Already in code. Needs to be adapted to this framework.
    BOOLEAN *bNewData;
    UINT16 i;
    UARTMGR_PROTOCOL_DATA_PTR pSource;
    UARTMGR_PROTOCOL_DATA_PTR pDestination;

    bNewData = &m_PWCDisp_DataStore[Direction].bNewData;
    pDestination =
    (UARTMGR_PROTOCOL_DATA_PTR)&m_PWCDisp_DataStore[Direction].param[0];
    pSource = (UARTMGR_PROTOCOL_DATA_PTR)pDest;

    // Data is saved to temporary storage in the UartMgr
    for (i = 0; i < nMaxByteSize; i++)
    {
        *pDestination++ = *pSource++;
    }
    // Confirm new data in temporary storage.
    *bNewData = TRUE;
}

/******************************************************************************
* Function:    PWCDispProtocol_DataLossFlag()
*
* Description: Function to return the Data Loss Status(i.e. synced/no data)
*
* Parameters:  None
*
* Returns:     UINT8* &dataLossFlag - Address of the data loss flag
*
* Notes:       none
*
*****************************************************************************/
UINT8* PWCDispProtocol_DataLossFlag(void)
{
    return(&dataLossFlag);
}

/******************************************************************************
* Function:    PWCDispProtocol_SetBaseUARTCh()
*
* Description: An Assert function to ensure that the PWC Display Protocol is 
*              only assigned to one UART channel
*
* Parameters:  UINT16 ch -- The UART Channel being Configured
*
* Returns:     None
*
* Notes:       none
*
*****************************************************************************/
void PWCDispProtocol_SetBaseUARTCh(UINT16 ch)
{
  // Only one instance of PWC Display Protocol can be initialized
  ASSERT(m_PWCDisp_RXStatus.port == (UINT8)UART_NUM_OF_UARTS );

  m_PWCDisp_RXStatus.port = (UINT8)ch;
}

/******************************************************************************
* Function:    PWCDispProtocol_ReturnFileHdr()
*
* Description: PWC Display has no cfg items so returns 0.
*
* Parameters:  dest - pointer to destination buffer
*              max_size - max size of dest buffer
*              ch - uart channel requested
*
* Returns:     0
*
* Notes:       none
*
*****************************************************************************/
UINT16 PWCDispProtocol_ReturnFileHdr(UINT8 *dest, const UINT16 max_size,
    UINT16 ch)
{
  return(0);
}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/*****************************************************************************
 *  MODIFICATIONS
 *    $History: PWCDispProtocol.c $
 * 
 * *****************  Version 16  *****************
 * User: Jeff Vahue   Date: 5/11/16    Time: 7:02p
 * Updated in $/software/control processor/code/system
 * SCR-1331 Add Test Point for H5308, 5310
 * 
 * *****************  Version 15  *****************
 * User: John Omalley Date: 3/31/16    Time: 1:32p
 * Updated in $/software/control processor/code/system
 * SCR 1302 - Code Review Updates
 * 
 * *****************  Version 14  *****************
 * User: John Omalley Date: 3/24/16    Time: 12:57p
 * Updated in $/software/control processor/code/system
 * SCR 1302 - V&V Findings Updates
 * 
 * *****************  Version 13  *****************
 * User: John Omalley Date: 3/17/16    Time: 1:30p
 * Updated in $/software/control processor/code/system
 * SCR 1302 - Code eview Updates
 * 
 * *****************  Version 12  *****************
 * User: John Omalley Date: 3/15/16    Time: 10:56a
 * Updated in $/software/control processor/code/system
 * SCR 1302 - Code Review Update
 * 
 * *****************  Version 11  *****************
 * User: John Omalley Date: 3/08/16    Time: 8:23a
 * Updated in $/software/control processor/code/system
 * SCR 1302 - Logic Updates
 * 
 * *****************  Version 10  *****************
 * User: John Omalley Date: 2/29/16    Time: 5:33p
 * Updated in $/software/control processor/code/system
 * SCR 1302 - Logic Updates from V&V findings
 * 
 * *****************  Version 9  *****************
 * User: John Omalley Date: 2/25/16    Time: 4:50p
 * Updated in $/software/control processor/code/system
 * SCR 1302 - Code Review Updates
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
 * User: John Omalley Date: 1/29/16    Time: 8:56a
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
 * User: Jeremy Hester Date: 8/20/15    Time: 12:08p
 * Created in $/software/control processor/code/system
 * Initial Check In. Implementation of the PWCDisp and UartMgr Requirements.
 *
 *
 *
 *****************************************************************************/
