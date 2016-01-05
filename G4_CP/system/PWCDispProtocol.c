#define PWCDISP_PROTOCOL_BODY

/******************************************************************************
            Copyright (C) 2008-2015 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    ECCN:        9D991

    File:        PWCDispProtocol.c

    Description: Contains all functions and data related to the PWC Display 
                 Protocol Handler 
    
    VERSION
      $Revision: 4 $  $Date: 1/04/16 6:22p $     

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

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define PWCDISP_MAX_RAW_RX_BUF      512 //MAX raw Buffer size for RX Packets
#define PWCDISP_MAX_RAW_TX_BUF      31  //MAX raw Buffer size for TX Packets
#define MAX_FRAMESEARCH_STATE       6


/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/********************/
/*      Enums       */
/********************/
typedef enum
{
  PWCDISP_PARAM_SEARCH_HEADER,
  PWCDISP_PARAM_SEARCH_EXTRACT_DATA,
  PWCDISP_PARAM_SEARCH_PACKET_ID,
  PWCDISP_PARAM_SEARCH_CHECKSUM,
  PWCDISP_PARAM_SEARCH_UPDATE_BUFFER,
  PWCDISP_PARAM_SEARCH_GOOD_MSG,
  PWCDISP_PARAM_SEARCH_NO_MSG
}PWCDISP_PARAM_SEARCH_ENUM;

typedef enum
{
  PWCDISP_PARAM_ENCODE_HEADER,
  PWCDISP_PARAM_ENCODE_CHARACTERS,
  PWCDISP_PARAM_ENCODE_CHECKSUM
}PWCDISP_PARAM_ENCODE_ENUM;

/********************/
/*      RXData      */
/********************/
typedef struct
{
  UINT8     data[PWCDISP_MAX_RAW_RX_BUF];
  UINT8    *pWr;
  UINT8    *pRd;
  UINT16    cnt;
  UINT8    *pStart;  // For debugging
  UINT8    *pEnd;    // For debugging
}PWCDISP_RAW_RX_BUFFER, *PWCDISP_RAW_RX_BUFFER_PTR;

typedef struct
{
  BOOLEAN             bNewFrame;
  UINT16              sync_word;
  UINT8               size;
  UINT16              chksum;
  UINT8  data[PWCDISP_MAX_RX_LIST_SIZE];
}PWCDISP_RX_DATA_FRAME, *PWCDISP_RX_DATA_FRAME_PTR;

/********************/
/*      TXData      */
/********************/
typedef struct
{
    UINT8     data[PWCDISP_MAX_RAW_TX_BUF];
}PWCDISP_RAW_TX_BUFFER, *PWCDISP_RAW_TX_BUFFER_PTR;

typedef struct
{
  UINT8 charParam[PWCDISP_MAX_CHAR_PARAMS];
  INT8 doubleClickRate;
} PWCDISP_TX_PARAM_LIST, *PWCDISP_TX_PARAM_LIST_PTR;

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static PWCDISP_RX_STATUS                   m_PWCDisp_RXStatus;
static PWCDISP_TX_STATUS                   m_PWCDisp_TXStatus;
                                           
static PWCDISP_RAW_RX_BUFFER               m_PWCDisp_RXBuff;
static PWCDISP_RX_DATA_FRAME               m_PWCDisp_RXDataFrame;
                                           
static PWCDISP_RAW_TX_BUFFER               m_PWCDisp_TXBuff;

static PWCDISP_TX_PARAM_LIST               m_PWCDisp_TXParamList;
                                           
static PWCDISP_CFG                         m_PWCDisp_Cfg;
static PWCDISP_DEBUG                       m_PWCDisp_Debug;
                                           
static PWCDISP_DISPLAY_DEBUG_TASK_PARAMS   PWCDispDisplayDebugBlock;

static PWCDISP_PARAM_ENCODE_ENUM           stateCheck = PWCDISP_PARAM_ENCODE_HEADER;

static UARTMGR_PROTOCOL_STORAGE m_PWCDisp_DataStore[4]; // RX/TX Data Storage.
                                                        // 0 - RX_PROTOCOL
                                                        // 1 - TX_PROTOCOL
                                                        // 2 - Requests From Disp App
                                                        // 3 - Requests From Protocol
// TX Temporary Data Store
static UARTMGR_PROTOCOL_DATA    m_PWCDisp_TXData[PWCDISP_TX_ELEMENT_CNT_COMPLETE];
// RX Temporary Data Store
static UARTMGR_PROTOCOL_DATA    m_PWCDisp_RXData[PWCDISP_RX_ELEMENT_COUNT] = 
{
 //Data Type          Data  Index Range
  {USER_TYPE_UINT8,   NULL, 1, 1},     //RXPACKETID
  {USER_TYPE_BOOLEAN, NULL, 1, 1},     //BUTTONSTATE_RIGHT
  {USER_TYPE_BOOLEAN, NULL, 1, 1},     //BUTTONSTATE_LEFT
  {USER_TYPE_BOOLEAN, NULL, 1, 1},     //BUTTONSTATE_ENTER
  {USER_TYPE_BOOLEAN, NULL, 1, 1},     //BUTTONSTATE_UP
  {USER_TYPE_BOOLEAN, NULL, 1, 1},     //BUTTONSTATE_DOWN
  {USER_TYPE_BOOLEAN, NULL, 1, 1},     //BUTTONSTATE_EVENT
  {USER_TYPE_BOOLEAN, NULL, 1, 1},     //DOUBLECLICK_RIGHT
  {USER_TYPE_BOOLEAN, NULL, 1, 1},     //DOUBLECLICK_LEFT
  {USER_TYPE_BOOLEAN, NULL, 1, 1},     //DOUBLECLICK_ENTER
  {USER_TYPE_BOOLEAN, NULL, 1, 1},     //DOUBLECLICK_UP
  {USER_TYPE_BOOLEAN, NULL, 1, 1},     //DOUBLECLICK_DOWN
  {USER_TYPE_BOOLEAN, NULL, 1, 1},     //DOUBLECLICK_EVENT
  {USER_TYPE_BOOLEAN, NULL, 1, 1},     //DISCRETE_ZERO
  {USER_TYPE_BOOLEAN, NULL, 1, 1},     //DISCRETE_ONE
  {USER_TYPE_BOOLEAN, NULL, 1, 1},     //DISCRETE_TWO
  {USER_TYPE_BOOLEAN, NULL, 1, 1},     //DISCRETE_THREE
  {USER_TYPE_BOOLEAN, NULL, 1, 1},     //DISCRETE_FOUR
  {USER_TYPE_BOOLEAN, NULL, 1, 1},     //DISCRETE_FIVE
  {USER_TYPE_BOOLEAN, NULL, 1, 1},     //DISCRETE_SIX
  {USER_TYPE_BOOLEAN, NULL, 1, 1},     //DISCRETE_SEVEN
  {USER_TYPE_UINT8,   NULL, 1, 1},     //DISPLAY_HEALTH
  {USER_TYPE_UINT8,   NULL, 1, 1},     //PART_NUMBER
  {USER_TYPE_UINT8,   NULL, 1, 1}      //VERSION_NUMBER
};

static UINT8 m_Str[30];
/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/

static void    PWCDispProtocol_UpdateUartMgr(void);
static void    PWCDispProtocol_Resync(void);
static BOOLEAN PWCDispProtocol_FrameSearch(PWCDISP_RAW_RX_BUFFER_PTR rx_buff_ptr);
static void    PWCDispProtocol_ValidRXPacket(BOOLEAN bBadPacket);
static void    PWCDispProtocol_ValidTXPacket(BOOLEAN bBadPacket);
static BOOLEAN PWCDispProtocol_ReadTXData(PWCDISP_TX_PARAM_LIST_PTR tx_param_ptr);
static BOOLEAN PWCDispProtocol_TXCharacterCheck(PWCDISP_TX_PARAM_LIST_PTR tx_param_ptr,
                                                PWCDISP_RAW_TX_BUFFER_PTR tx_buff_ptr);
static void    PWCDispDebug_RX(void *pParam);
static void    PWCDispDebug_TX(void *pParam);
#include "PWCDispUserTables.c"

/*****************************************************************************/
/* Constant Data                                                             */
/*****************************************************************************/
typedef struct
{
  UINT8 name[15];
} PWCDISP_PARAM_NAMES, *PWCDISP_PARAM_NAMES_PTR;

static 
const PWCDISP_PARAM_NAMES PWCDisp_RX_Param_Names[PWCDISP_RX_ELEMENT_COUNT] =
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
const PWCDISP_PARAM_NAMES PWCDisp_TX_Param_Names[PWCDISP_TX_ELEMENT_CNT_COMPLETE] =
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
void PWCDispProtocol_Initialize ( void ) 
{  
  PWCDISP_RX_STATUS_PTR pRXStatus; 
  PWCDISP_TX_STATUS_PTR pTXStatus; 
  PWCDISP_RAW_RX_BUFFER_PTR buff_ptr;

  // Local Data
  TCB TaskInfo;

  // Initialize local variables / structures
  memset (&m_PWCDisp_RXStatus,    0, sizeof(m_PWCDisp_RXStatus   ));
  memset (&m_PWCDisp_TXStatus,    0, sizeof(m_PWCDisp_TXStatus   ));
  memset (&m_PWCDisp_RXBuff,      0, sizeof(m_PWCDisp_RXBuff     ));
  memset (&m_PWCDisp_TXBuff,      0, sizeof(m_PWCDisp_TXBuff     ));
  memset (&m_PWCDisp_RXDataFrame, 0, sizeof(m_PWCDisp_RXDataFrame));
  memset (&m_PWCDisp_TXParamList, 0, sizeof(m_PWCDisp_TXParamList));
  memset (&m_PWCDisp_Cfg,         0, sizeof(m_PWCDisp_Cfg        ));
  memset (&m_PWCDisp_DataStore,   0, sizeof(m_PWCDisp_DataStore  ));

  // Initialize the Write and Read index into the Raw RX Buffer
  PWCDispProtocol_Resync();
  
  // Update m_PWCDisp_RXBuff entries - Debug
  buff_ptr         = (PWCDISP_RAW_RX_BUFFER_PTR) &m_PWCDisp_RXBuff;
  buff_ptr->pStart = (UINT8 *) &buff_ptr->data[0]; 
  buff_ptr->pEnd   = (UINT8 *) &buff_ptr->data[PWCDISP_MAX_RAW_RX_BUF]; 

  // Add an entry in the user message handler table for PWCDisp Cfg Items
  User_AddRootCmd(&PWCDispProtocolRootTblPtr);
  
  // Restore User Cfg
  memcpy(&m_PWCDisp_Cfg, &CfgMgr_RuntimeConfigPtr()->PWCDispConfig, 
         sizeof(m_PWCDisp_Cfg));
  
  // Update Start up Debug
  m_PWCDisp_Debug.bDebug       = FALSE;
  m_PWCDisp_Debug.bProtocol    = RX_PROTOCOL; 
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
  pTXStatus->txTimer = 2; // Abitrary 2 in order to insure that RX and TX take
                           // place on seperate frames.
  
  // Create PWC Display Protocol Display Task
  memset(&TaskInfo, 0, sizeof(TaskInfo));
  strncpy_safe(TaskInfo.Name, sizeof(TaskInfo.Name), 
               "PWCDisp Display Debug",_TRUNCATE);
  TaskInfo.TaskID         = PWCDisp_Debug;
  TaskInfo.Function       = PWCDispDebug_Task;
  TaskInfo.Priority       = taskInfo[PWCDisp_Debug].priority;
  TaskInfo.Type           = taskInfo[PWCDisp_Debug].taskType;
  TaskInfo.modes          = taskInfo[PWCDisp_Debug].modes;
  TaskInfo.MIFrames       = taskInfo[PWCDisp_Debug].MIFframes;
  TaskInfo.Rmt.InitialMif = taskInfo[PWCDisp_Debug].InitialMif;
  TaskInfo.Rmt.MifRate    = taskInfo[PWCDisp_Debug].MIFrate;
  TaskInfo.Enabled        = TRUE;
  TaskInfo.Locked         = FALSE;
  TaskInfo.pParamBlock    = &PWCDispDisplayDebugBlock;
  TmTaskCreate (&TaskInfo);
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
  BOOLEAN   bNewData, bBadTXPacket, bTXFrame;
  UINT16    i;
  UINT16    sent_cnt;
  
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
  if((cnt > 0) || (rx_buff_ptr->cnt > 4))
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
  }
  
  //Begin Processing TX message packets every 10 MIF Frames
  if(tx_status_ptr->txTimer % 10 == 0)
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
    if(bBadTXPacket != TRUE)
    {
      UART_Transmit(UartMgr_GetPort(UARTMGR_PROTOCOL_PWC_DISPLAY), 
                   (const INT8*) &tx_buff_ptr->data[0], 
                   PWCDISP_MAX_RAW_TX_BUF, &sent_cnt);
    }
    //Reset txTimer. If there is a bad packet the timer will reset to 10 in 
    //order to refresh the tx data ASAP
    tx_status_ptr->txTimer = (bBadTXPacket != TRUE) ? 1 : 10; 
  }
  else
  {
    tx_status_ptr->txTimer++;
  }

  // If there is new, recognized data then Update the UART Manager
  if(bNewData == TRUE)
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
PWCDISP_DEBUG_PTR PWCDispProtocol_GetDebug (void)
{
  return((PWCDISP_DEBUG_PTR) &m_PWCDisp_Debug);
}

/******************************************************************************
 * Function:    PWCDispProtocol_GetCfg
 *
 * Description: Utility function to request current PWC Display Protocol config 
 *              state 
 *
 * Parameters:  None
 *
 * Returns:     Ptr to PWCDisp Debug Data
 *
 * Notes:       None 
 *
 *****************************************************************************/
PWCDISP_CFG_PTR PWCDispProtocol_GetCfg(void)
{
  return ((PWCDISP_CFG_PTR) &m_PWCDisp_Cfg);
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
  UARTMGR_PROTOCOL_DATA_PTR pData, pCurrentData;
  UINT16 i, size, cnt;

  frame_ptr = &m_PWCDisp_RXDataFrame;
  pData = &m_PWCDisp_RXData[0];
  cnt = 0;

  // Save RX Packet ID to Table
  pCurrentData = pData + RXPACKETID;
  pCurrentData->data.Int = frame_ptr->data[cnt];
  cnt++;

  pCurrentData = pData + BUTTONSTATE_RIGHT;
  size = (BUTTONSTATE_EVENT - BUTTONSTATE_RIGHT) + 1;

  // Parse Button States and save to table
  for (i = 0; i < size; i++)
  {
    // Account for default 0 bits
    if (i == 5)
    {
      (pCurrentData + i)->data.Int =
        (BOOLEAN)((frame_ptr->data[cnt] >> (i + 2)) & (0x01));
    }
    else
    {
      (pCurrentData + i)->data.Int =
        (BOOLEAN)((frame_ptr->data[cnt] >> i) & (0x01));
    }
  }
  cnt++;

  pCurrentData = pData + DOUBLECLICK_RIGHT;
  size = (DOUBLECLICK_EVENT - DOUBLECLICK_RIGHT) + 1;
  // Parse double click states and save to table
  for (i = 0; i < size; i++)
  {
    // Account for default 0 bits
    if (i == 5)
    {
      (pCurrentData + i)->data.Int =
        (BOOLEAN)((frame_ptr->data[cnt] >> (i + 2)) & (0x01));
    }
    else
    {
      (pCurrentData + i)->data.Int =
        (BOOLEAN)((frame_ptr->data[cnt] >> i) & (0x01));
    }
  }
  cnt++;
  
  // Parse Discrete states and save to table
  pCurrentData = pData + DISCRETE_ZERO;
  size = (DISCRETE_SEVEN - DISCRETE_ZERO) + 1;
  for (i = 0; i < size; i++)
  {
    (pCurrentData + i)->data.Int = 
      (BOOLEAN)((frame_ptr->data[cnt] >> i) & (0x01));
  }
  cnt++;

  // Save Display Health byte to table
  pCurrentData = pData + DISPLAY_HEALTH;
  pCurrentData->data.Int = frame_ptr->data[cnt];
  cnt++;

  // Save Part Number to table
  pCurrentData = pData + PART_NUMBER;
  pCurrentData->data.Int = frame_ptr->data[cnt];
  cnt++;

  // Save Version Number to table
  pCurrentData = pData + VERSION_NUMBER;
  pCurrentData->data.Int = frame_ptr->data[cnt];
  
  // Write the contents of the table to the UART manager
  UartMgr_Write(pData, UARTMGR_PROTOCOL_PWC_DISPLAY, RX_PROTOCOL,
                PWCDISP_RX_ELEMENT_COUNT);
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
  BOOLEAN bNewData, bContinueSearch, bLookingForMsg, bBadPacket;
  UINT8 *src_ptr, *end_ptr, *data, buff[PWCDISP_MAX_RAW_RX_BUF];
  UINT16 i, j, k, checksum;
  PWCDISP_PARAM_SEARCH_ENUM stateSearch;
  PWCDISP_RX_STATUS_PTR pStatus;
  PWCDISP_RX_DATA_FRAME_PTR frame_ptr;
  
  bNewData        = FALSE;
  bContinueSearch = TRUE;
  bLookingForMsg  = TRUE;
  bBadPacket      = FALSE;
  src_ptr         = (UINT8 *) &buff[0];
  end_ptr         = (UINT8 *) &buff[rx_buff_ptr->cnt - 3];
  stateSearch     = PWCDISP_PARAM_SEARCH_HEADER;
  pStatus         = (PWCDISP_RX_STATUS_PTR)&m_PWCDisp_RXStatus;
  frame_ptr       = &m_PWCDisp_RXDataFrame;
  data            = (UINT8 *) &frame_ptr->data[0];
  
  //Copy data from rolling buffer to a flat buffer with recent data first
  i = (UINT16)((UINT32)&rx_buff_ptr->data[PWCDISP_MAX_RAW_RX_BUF] -
      (UINT32)rx_buff_ptr->pRd);
  j = (UINT16)((UINT32)rx_buff_ptr->pRd -
      (UINT32)&rx_buff_ptr->data[0]);
  memcpy((UINT8 *)&buff[0], rx_buff_ptr->pRd, i);
  memcpy((UINT8 *)&buff[i], (UINT8 *)&rx_buff_ptr->data[0], j);
  i = 0;

  while(bContinueSearch == TRUE)
  {
    switch (stateSearch)
    {
      case PWCDISP_PARAM_SEARCH_HEADER:
        // Begin scanning for header while there is space for a message header
        while (src_ptr < end_ptr && bLookingForMsg == TRUE)
        { 
          if (*src_ptr       == PWCDISP_SYNC_BYTE1       &&
              *(src_ptr + 1) == PWCDISP_SYNC_BYTE2       &&
              *(src_ptr + 2) == PWCDISP_MAX_RX_LIST_SIZE &&
              *(src_ptr + 3) == PWCDISP_RX_PACKET_ID)
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
              bBadPacket                  = TRUE;
              m_PWCDisp_Debug.bNewRXFrame = TRUE;
            }
            src_ptr++; i++;
          }
        }
        if(bLookingForMsg == FALSE) // Header found. Begin Data extraction.
        {
          if ((src_ptr + (PWCDISP_RX_MAX_MSG_SIZE -1)) < (end_ptr + 3))
          {
            stateSearch = PWCDISP_PARAM_SEARCH_EXTRACT_DATA;
          }
          else
          {
            stateSearch = PWCDISP_PARAM_SEARCH_UPDATE_BUFFER;
          }
        }
        else // four bytes left in flat buffer
        {
          // Not enough data. Try again later
          stateSearch = PWCDISP_PARAM_SEARCH_UPDATE_BUFFER;
        }
        break;

      case PWCDISP_PARAM_SEARCH_EXTRACT_DATA:
        // Save sync_word, size, and checksum to Frame Buffer
        frame_ptr->sync_word = (*src_ptr << 8) | (*(src_ptr + 1));
        frame_ptr->size      = *(src_ptr + 2);
        frame_ptr->chksum    = (*(src_ptr + (frame_ptr->size + 3)) << 8) |
                               (*(src_ptr + (frame_ptr->size + 4)));
        
        // Adjust buffer position to Packet ID
        src_ptr += 3;
        i += 3;

        // Add data to Frame buffer
        for (k = 0; k < frame_ptr->size; k++)
        {
          *data++ = *src_ptr++;
        }

        // Ready Packet ID verification
        stateSearch = PWCDISP_PARAM_SEARCH_CHECKSUM;
        break;

	  case PWCDISP_PARAM_SEARCH_CHECKSUM:
        // Calculate packet Checksum
        checksum = 0;
        checksum += (UINT8)(frame_ptr->sync_word >> 8);
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
          pStatus->sync = TRUE;
          pStatus->syncCnt++;
          pStatus->lastSyncPeriod = (CM_GetTickCount() -
		                            pStatus->lastSyncTime);
          pStatus->lastSyncTime   = CM_GetTickCount();
          bNewData = TRUE;

          // New RX frame. Update RX Packet Contents.
          memcpy(pStatus->packetContents, src_ptr - (PWCDISP_RX_MAX_MSG_SIZE - 2),
                 PWCDISP_RX_MAX_MSG_SIZE);

          //Update Status and Ready Packet Size Verification
          pStatus->payloadSize = frame_ptr->size;
          pStatus->chksum = frame_ptr->chksum;
	  
          // Message is good, reset bad packet counter
          pStatus->rx_SyncLossCnt     = 0;
          m_PWCDisp_Debug.bNewRXFrame = TRUE;
          i += (PWCDISP_RX_MAX_MSG_SIZE - 3);
          stateSearch = PWCDISP_PARAM_SEARCH_UPDATE_BUFFER;
        }
        else
        {
          // Checksum is incorrect. Invalid/Corrupt message/No message
          // continue scanning for message header.
          stateSearch = PWCDISP_PARAM_SEARCH_HEADER;
        }
        break;
      case PWCDISP_PARAM_SEARCH_UPDATE_BUFFER:
        bContinueSearch = FALSE;
        rx_buff_ptr->cnt -= i;

        // Update the pRd pointer.
        for (k = 0; k < i; k++)
        {
          rx_buff_ptr->pRd++;
          if (rx_buff_ptr->pRd >= &rx_buff_ptr->data[PWCDISP_MAX_RAW_RX_BUF])
          {
              rx_buff_ptr->pRd = &rx_buff_ptr->data[0];
          }
        }
        break;
    }
  }
  // Update bad packet counter status.
  PWCDispProtocol_ValidRXPacket(bBadPacket);

  return (bNewData);
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
  PWCDISP_INVALID_PACKET_LOG invalidPktLog;
  PWCDISP_RX_STATUS_PTR pStatus;
  PWCDISP_CFG_PTR pCfg;
  
  pStatus         = (PWCDISP_RX_STATUS_PTR)&m_PWCDisp_RXStatus;
  pCfg            = &m_PWCDisp_Cfg;
  
  // Message was deemed invalid. Update status and record any necessary logs.
  if(bBadPacket == TRUE)
  {
    pStatus->rx_SyncLossCnt++;
    pStatus->invalidSyncCnt++;
    if (pStatus->rx_SyncLossCnt > pCfg->packetErrorMax)
    {
      invalidPktLog.validSyncCnt  = pStatus->syncCnt;
      invalidPktLog.invalidSyncCnt = pStatus->invalidSyncCnt;
      invalidPktLog.lastFrameTime = pStatus->lastFrameTime;
      LogWriteSystem(SYS_ID_UART_PWCDISP_SYNC_LOSS, LOG_PRIORITY_LOW,
                     &invalidPktLog, sizeof(PWCDISP_INVALID_PACKET_LOG), NULL);
      pStatus->rx_SyncLossCnt = 0;
    }
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
  PWCDISP_DISPLAY_PACKET_ERROR_LOG invalidPktLog;
  PWCDISP_TX_STATUS_PTR pStatus;
  
  pStatus = (PWCDISP_TX_STATUS_PTR)&m_PWCDisp_TXStatus;

  // Display Packet was deemed invalid. Record logs.
  if(bBadPacket == TRUE)
  {
    memcpy(invalidPktLog.packetContents, pStatus->packetContents, 
           PWCDISP_TX_MSG_SIZE);
    invalidPktLog.lastFrameTime = pStatus->lastFrameTime;
    invalidPktLog.validPacketCnt = pStatus->validPacketCnt;
    invalidPktLog.invalidPacketCnt = pStatus->invalidPacketCnt;
    LogWriteSystem(SYS_ID_UART_PWCDISP_TXPACKET_FAIL, LOG_PRIORITY_LOW,
                   &invalidPktLog, sizeof(PWCDISP_DISPLAY_PACKET_ERROR_LOG), 
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
  UARTMGR_PROTOCOL_DATA_PTR pData, pCurrentData;
  BOOLEAN bNewData;
  UINT16 i, size;
  UINT8 *pDest;

  pData = (UARTMGR_PROTOCOL_DATA_PTR)&m_PWCDisp_TXData[0];
  bNewData = UartMgr_Read(pData, UARTMGR_PROTOCOL_PWC_DISPLAY, TX_PROTOCOL,
                          PWCDISP_TX_ELEMENT_CNT_COMPLETE);

  // Aquire new data if available
  if (bNewData == TRUE)
  {
    bNewData     = FALSE;
    pCurrentData = pData + CHARACTER_0;
    size         = PWCDISP_MAX_CHAR_PARAMS;

    pDest   = (UINT8 *) &tx_param_ptr->charParam[0];

    for (i = 0; i < size; i++)
    {
      if(*pDest != (UINT8)(pCurrentData->data.Int))
      {
	    bNewData = TRUE;
      }
      *pDest = (UINT8)pCurrentData->data.Int;
      pDest++; pCurrentData++;
    }
    
    pCurrentData = pData + DC_RATE;
    if(tx_param_ptr->doubleClickRate != (INT8)pCurrentData->data.Int)
    {
      bNewData = TRUE;
    }
    tx_param_ptr->doubleClickRate = (INT8) pCurrentData->data.Int;    
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
  UINT8 *buff, *data;
  UINT16 i, checksum;
  BOOLEAN bBadTXPacket, bContinueCheck;
  PWCDISP_TX_STATUS_PTR pStatus;
  
  buff           = (UINT8 *) &tx_buff_ptr->data;
  data           = (UINT8 *) &tx_param_ptr->charParam;
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
        *buff       = PWCDISP_SYNC_BYTE1;
        *(buff + 1) = PWCDISP_SYNC_BYTE2;
        *(buff + 2) = PWCDISP_MAX_TX_LIST_SIZE;
        *(buff + 3) = PWCDISP_TX_PACKET_ID;
        // Ready Character Check and Encoding
        stateCheck = PWCDISP_PARAM_ENCODE_CHARACTERS;
        break;
      
      case PWCDISP_PARAM_ENCODE_CHARACTERS:
        // set pointer to the first non header byte in the buffer
        buff += 4; 
        for(i = 0; i < PWCDISP_MAX_CHAR_PARAMS; i++)
        {
          // If the character is not approved abort loop and report bad packet
          if(*data > 0x7F && bBadTXPacket != TRUE)
             {
               bBadTXPacket = TRUE;
               pStatus->invalidPacketCnt++;
             }
             // Add the character to the buffer 
             *buff++ = *data++;
        }
        
		*buff = tx_param_ptr->doubleClickRate;
        
		// Ready Checksum Calculation and Encoding
        stateCheck = PWCDISP_PARAM_ENCODE_CHECKSUM;
        break;
      
      case PWCDISP_PARAM_ENCODE_CHECKSUM:
        // Update Status and Reset Buffer pointer
        pStatus->lastPacketPeriod = CM_GetTickCount() - 
                                    pStatus->lastPacketTime;
        pStatus->lastPacketTime   = CM_GetTickCount();
        pStatus->validPacketCnt++;
        buff = (UINT8 *) &tx_buff_ptr->data;
        // Calculate Checksum
        for (i = 0; i < PWCDISP_MAX_RAW_TX_BUF - 2; i++)
        {
          checksum += *buff++;
        }
        // Encode Checksum into Raw Buffer
        *buff       = (UINT8) ((checksum >> 8) & 0xFF);//NOTE: Need to make sure this works
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
  memcpy(pStatus->packetContents, tx_buff_ptr->data, PWCDISP_TX_MSG_SIZE);
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
void PWCDispDebug_Task(void *pParam)
{
  PWCDISP_DEBUG_PTR pDebug;
  
  pDebug = &m_PWCDisp_Debug;
  
  if(pDebug->bDebug == TRUE)
  {
    if(pDebug->bProtocol == RX_PROTOCOL && pDebug->bNewRXFrame == TRUE)
    {
      PWCDispDebug_RX(pParam);
    }
    else if (pDebug->bProtocol == TX_PROTOCOL && pDebug->bNewTXFrame == TRUE)
    {
      PWCDispDebug_TX(pParam);
    }
  }
}


/******************************************************************************
* Function:    TranslateArrows
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
void TranslateArrows(char charString[], UINT16 length)
{
  int i;
  for (i = 0; i < length; i++)
  {
    switch(charString[i])
    {
      case 0x02:
        charString[i] = '>';
        break;
      case 0x04:
        charString[i] = '<';
        break;
      case 0x01:
        charString[i] = '^';
        break;
      case 0x03:
        charString[i] = 'v';
        break;
      case 0x05:
        charString[i] = '@';
        break;
      default:
        break;
    };
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
  UINT8 subString[15];
  
  pDebug    = &m_PWCDisp_Debug;
  pStatus   = (PWCDISP_RX_STATUS_PTR)&m_PWCDisp_RXStatus;
  pName     = (PWCDISP_PARAM_NAMES_PTR)&PWCDisp_RX_Param_Names;

  
  pStatus->chksum = ((UINT16)pStatus->packetContents[10] << 8) |
                    ((UINT16)pStatus->packetContents[11]);
  
  m_Str[0] = NULL;

  // Display Address Checksum
  sprintf((char *) m_Str, "\r\nChksum=0x%04x\r\n", pStatus->chksum);
  GSE_PutLine((const char *)m_Str);
  
  // Display Size
  sprintf((char *)m_Str, "Size=%d\r\n", pStatus->payloadSize);
  GSE_PutLine((const char * )m_Str);

  
  // Display Raw Buffer
  // Display Sync Byte 1
  sprintf((char *)m_Str, "SYNC1          =0x%02x\r\n",
	  pStatus->packetContents[0]);
  GSE_PutLine( (const char *) m_Str );
    
  // Display Sync Byte 2
  sprintf((char *)m_Str, "SYNC2          =0x%02x\r\n",
	  pStatus->packetContents[1]);
  GSE_PutLine( (const char *) m_Str );
  
  // Display Size Byte
  sprintf((char *)m_Str, "SIZE_BYTE      =0x%02x\r\n",
	 pStatus->packetContents[2]);
  GSE_PutLine( (const char *) m_Str );

  m_Str[0] = NULL;
    
  //Display Payload Contents
  for( i = 0; i < PWCDISP_MAX_RX_LIST_SIZE; i++)
  {
    memcpy(subString, (pName+i)->name, 15);
    snprintf((char *)m_Str, 15, "%s", subString);
    snprintf((char *)subString, 9, " =0x%02x\r\n", pStatus->packetContents[i+3]);
    strcat((char *)m_Str, (const char *)subString);
    GSE_PutLine((const char *) m_Str);
    m_Str[0] = NULL;
  }

  // Display RX last packet period
  snprintf((char *)m_Str, 33, "LAST_PACKET_PERIOD =%10d\r\n",
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
  UINT8 subString[15];
  
  pDebug = &m_PWCDisp_Debug;
  pName = (PWCDISP_PARAM_NAMES_PTR)PWCDisp_TX_Param_Names;
  pStatus = (PWCDISP_TX_STATUS_PTR)&m_PWCDisp_TXStatus;


    m_Str[0] = NULL;
  // Display Raw Buffer
  // Display Sync byte 1
  sprintf((char *) m_Str, "\r\nSYNC1          =0x%02x\r\n",
	  pStatus->packetContents[0]);
  GSE_PutLine((const char *)m_Str);
    
  // Display Sync byte 2
  sprintf((char *) m_Str, "SYNC2          =0x%02x\r\n",
	  pStatus->packetContents[1]);
  GSE_PutLine((const char *) m_Str);
    
  // Display Size
  sprintf((char *) m_Str, "SIZE           =0x%02x\r\n",
	  pStatus->packetContents[2]);
  GSE_PutLine((const char *) m_Str);
    
  m_Str[0] = NULL;
    
  // Display Packet ID and Character list
  for(i = 0; i < PWCDISP_TX_ELEMENT_CNT_COMPLETE - 1; i++)
  {
    memcpy(subString, (pName+i)->name, 15);
    snprintf((char *)m_Str, 15, "%s", subString);
    snprintf((char *)subString, 9, " =0x%02x\r\n",
	    pStatus->packetContents[i+4]);
    strcat((char *)m_Str, (const char *)subString);
    GSE_PutLine((const char *) m_Str);
    m_Str[0] = NULL;
  }
  GSE_PutLine("\r\n");
    
  // Display DCRATE
  memcpy(subString, (pName + DC_RATE)->name, 15);
  snprintf((char *)m_Str, 15, "%s", subString);
  snprintf((char *) subString, 11, " =%03d ms\r\n",
           (INT8)(pStatus->packetContents[4+DC_RATE]) * 2);
  strcat((char *)m_Str, (const char*)subString);
  GSE_PutLine((const char*) m_Str);
  GSE_PutLine("\r\n");
  m_Str[0] = NULL;

  // Display LINE 1
  memcpy(m_Str, &(pStatus->packetContents[4]), 12);
  m_Str[12] = '\0';
  TranslateArrows((char *)m_Str, 12);
  GSE_PutLine((const char*) m_Str);
  GSE_PutLine("\r\n");
  m_Str[0] = NULL;

  // Display LINE 2
  memcpy(m_Str, &(pStatus->packetContents[16]), 12);
  m_Str[12] = '\0';
  TranslateArrows((char *)m_Str, 12);
  GSE_PutLine((const char*)m_Str);
  GSE_PutLine("\r\n\n");

  // Display TX last packet period
  snprintf((char *)m_Str, 33, "LAST_PACKET_PERIOD =%10d\r\n",
      pStatus->lastPacketPeriod);
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
 * Function:     PWCDispProtocol_SetRXDebug
 *
 * Description:  Enables the RX Protocol Debug Display
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
void PWCDispProtocol_SetRXDebug(void)
{
  m_PWCDisp_Debug.bProtocol = RX_PROTOCOL;
}

/******************************************************************************
 * Function:     PWCDispProtocol_SetTXDebug
 *
 * Description:  Enables the TX Protocol Debug Display
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
void PWCDispProtocol_SetTXDebug(void)
{
  m_PWCDisp_Debug.bProtocol = TX_PROTOCOL;
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
    BOOLEAN *bNewData, bStatus;
    UINT16 i;
    UARTMGR_PROTOCOL_DATA_PTR pSource, pDestination;

    bNewData = &m_PWCDisp_DataStore[Direction].bNewData;
    pSource = (UARTMGR_PROTOCOL_DATA_PTR)&m_PWCDisp_DataStore[Direction].param;
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
    UARTMGR_PROTOCOL_DATA_PTR pSource, pDestination;

    bNewData = &m_PWCDisp_DataStore[Direction].bNewData;
    pDestination =
	(UARTMGR_PROTOCOL_DATA_PTR)&m_PWCDisp_DataStore[Direction].param;
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
 * Function:    PWCDispProtocol_ReturnFileHdr()
 *
 * Description: Function to return the F7X file hdr structure
 *
 * Parameters:  dest - pointer to destination buffer
 *              max_size - max size of dest buffer 
 *              ch - uart channel requested 
 *
 * Returns:     cnt - byte count of data returned 
 *
 * Notes:       none 
 *
 *****************************************************************************/
UINT16 PWCDispProtocol_ReturnFileHdr(UINT8 *dest, const UINT16 max_size,
                                     UINT16 ch)
{
  PWCDISP_CFG_PTR       pCfg;
  PWCDISP_FILE_HDR      fileHdr;
  
  pCfg = &m_PWCDisp_Cfg;
  
  fileHdr.packetErrorMax = pCfg->packetErrorMax;
  
  fileHdr.size = sizeof(PWCDISP_FILE_HDR);
  
  memcpy(dest, (UINT8 *) &fileHdr, sizeof(fileHdr.size));
  
  return(fileHdr.size);
}

/******************************************************************************
* Function:     PWCDispProtocol_DisableLiveStream
*
* Description:  Disables the outputting the live stream for the PWC Display
*               Protocol.
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
void PWCDispProtocol_DisableLiveStream(void)
{
    m_PWCDisp_Debug.bDebug = FALSE;
}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/*****************************************************************************
 *  MODIFICATIONS
 *    $History: PWCDispProtocol.c $
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
