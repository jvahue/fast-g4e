#define ID_PARAM_PROTOCOL_BODY

/******************************************************************************
            Copyright (C) 2008-2014 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:        IDParamProtocol.c

    Description: Contains all functions and data related to the ID Param Protocol
                 Handler

    VERSION
      $Revision: 1 $  $Date: 14-10-08 7:11p $

******************************************************************************/

//#define ID_TIMING_TEST 1


/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/



/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "IDParamProtocol.h"
#include "UartMgr.h"
#include "ClockMgr.h"
#include "GSE.h"
#include "NVMgr.h"


/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define ID_PARAM_MAX_RAW_BUF  1024 // (FADEC is ~85 words -> 170 bytes)
                                   // (PEC is ~48 words -> 96 bytes)
#define ID_PARAM_MAX_FRAME    522  // (256 * 2) + 10 (OH) -> 522 bytes

#define ID_PARAM_FM_SYNC_AND_WORD_SIZE 4

#define ID_PARAM_SYNC_WORD 0x8000

#define ID_PARAM_WRAP_RAW_BUFF(ptr,end_ptr,start_ptr) \
        if ( ptr >= end_ptr ) {        \
          ptr = start_ptr;}            \

#define ID_PARAM_FRAME_WORD_BIT0_SET(nWord) ((nWord & 0x0001) == TRUE)

#define ID_PARAM_FRAME_SYNC_WORD_OFFSET 0
#define ID_PARAM_FRAME_ELEM_CNT_OFFSET 1
#define ID_PARAM_FRAME_CKSUM_END_OFFSET 1

#define ID_PARAM_FRAME_TYPES 2
#define ID_PARAM_FRAME_TYPE_FADEC 0
#define ID_PARAM_FRAME_TYPE_PEC 1

#define ID_PARAM_FRAME_OH 5 // ID Param Frame Overhead = sync + elem cnt + elem pos + elem id
                             //                               + chksum (all word size)

#define ID_PARAM_FRAME_IDLE_TIMEOUT 1000
#define ID_PARAM_FRAME_CONSEC_BAD_CNT 1
#define ID_PARAM_FRAME_CONSEC_CRC_CNT 3


/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
typedef enum
{
  ID_PARAM_SEARCH_SYNC_WORD,
  ID_PARAM_SEARCH_PACKET_SIZE,
  ID_PARAM_SEARCH_MAX
} ID_PARAM_SEARCH_ENUM;

typedef struct
{
  UINT8 data[ID_PARAM_MAX_RAW_BUF];
  UINT8 *pWr;
  UINT8 *pRd;
  UINT8 *pStart;  // Ptr to SYNC Word or start of frame

  UINT8 *pEnd;    // Ptr to end of raw buff
  UINT8 *pBegin;  // Ptr to beginning of raw buff

  UINT16 cnt;
  ID_PARAM_SEARCH_ENUM stateSearch;
  UINT16 frameSync;
  UINT16 frameSize;
  UINT32 lastActivity_tick; // Tick time of last byte Rx.

} ID_PARAM_RAW_BUFFER, *ID_PARAM_RAW_BUFFER_PTR;

typedef struct
{
  UINT16 syncWord;      // Sync word Bit 15 = 1 and Bit 14-0 = 0
  UINT16 totalElements; // # element in block = # word in frame - 5 words (OverHead)
  UINT16 elementPos;    // Element Position in frame.  Bit 15 = LRU (1=PEC,0=FADEC), Bit 0 = 1
  UINT16 elementID;     // Element Id related to above element position. Bit 0 = 1
  UINT8  data;
} ID_PARAM_FRAME_HDR, *ID_PARAM_FRAME_HDR_PTR;

typedef struct
{
  UINT8 data[ID_PARAM_MAX_FRAME];
  ID_PARAM_FRAME_HDR_PTR pFrameHdr;  // Ptr to overlay onto ->data[] above
  UINT16 size;
} ID_PARAM_FRAME_BUFFER, *ID_PARAM_FRAME_BUFFER_PTR;


typedef struct
{
  UINT16 elementID;     // Element ID #
  UINT16 index;         // Index into UART_PARAM_DATA to transfer data
  UINT16 frameType;     // Used for SCROLLING ID only. FrameType.
} ID_PARAM_FRAME_DATA, *ID_PARAM_FRAME_DATA_PTR;

typedef struct
{
  ID_PARAM_FRAME_DATA data[ID_PARAM_CFG_MAX];
  UINT16 size;
} ID_PARAM_FRAME, *ID_PARAM_FRAME_PTR;

#define ID_PARAM_SCROLL_ID_MAX 4

typedef struct
{
  UINT16 id_elemId; // Param Id which contains the Element Id identifier
  UINT16 id_value;     // Id which contains the Element value
  UINT16 pos_id;       // Position in frame of the scolling id word
  UINT16 pos_value;    // Position in frame of the parameter value word
  UINT16 frameType;    // 0 = FADEC, 1 = PEC frame
} ID_PARAM_SCROLL_INFO, *ID_PARAM_SCROLL_INFO_PTR;

typedef struct
{
  ID_PARAM_SCROLL_INFO info[ID_PARAM_SCROLL_ID_MAX];
  UINT16 numCfgScrollID;
} ID_PARAM_SCROLL_DATA, *ID_PARAM_SCROLL_DATA_PTR;


/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
ID_PARAM_CFG m_IDParam_Cfg;
ID_PARAM_STATUS m_IDParam_Status[UART_NUM_OF_UARTS];

ID_PARAM_RAW_BUFFER m_IDParam_RawBuff[UART_NUM_OF_UARTS];
ID_PARAM_FRAME_BUFFER m_IDParam_FrameBuff[UART_NUM_OF_UARTS];

ID_PARAM_FRAME m_IDParam_Frame[UART_NUM_OF_UARTS][ID_PARAM_FRAME_TYPES];

ID_PARAM_SCROLL_DATA m_IDParam_Scroll[UART_NUM_OF_UARTS];
ID_PARAM_FRAME m_IDParam_ScrollList[UART_NUM_OF_UARTS];


// Test Timing
#ifdef ID_TIMING_TEST
  #define ID_TIMING_BUFF_MAX 200
  UINT32 m_IDTiming_Buff[ID_TIMING_BUFF_MAX];
  UINT32 m_IDTiming_Start;
  UINT32 m_IDTiming_End;
  UINT16 m_IDTiming_Index;
#endif
// Test Timing


/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static BOOLEAN IDParamProtocol_FrameSearch( ID_PARAM_RAW_BUFFER_PTR buff_ptr,
                                            UARTMGR_PARAM_DATA_PTR data_ptr,
                                            UINT16 ch );
static UINT16 IDParamProtocol_CalcFrameChksum( UINT16 *pData, UINT16 word_cnt  );
static BOOLEAN IDParamProtocol_ConfirmFrameFmt( ID_PARAM_FRAME_HDR_PTR pFrameHdr,
                                                UINT16 cksum );
static void IDParamProtocol_ProcElemID( ID_PARAM_FRAME_HDR_PTR pFrameHdr,
                                        UARTMGR_PARAM_DATA_PTR data_ptr, UINT16 ch );
static void IDParamProtocol_ProcFrame( ID_PARAM_FRAME_BUFFER_PTR frame_ptr,
                                       UARTMGR_PARAM_DATA_PTR data_ptr, UINT16 ch );
static void IDParamProtocol_CheckSyncLoss( BOOLEAN bNewData, UINT16 ch,
                                           ID_PARAM_STATUS_PTR pStatus,
                                           ID_PARAM_RAW_BUFFER_PTR buff_ptr,
                                           UINT32 tick );
static void IDParamProtocol_CreateSyncLog( BOOLEAN bIdleTimeOut,
                                           ID_PARAM_STATUS_PTR pStatus );
static BOOLEAN IDParamProtocol_CheckScrollID( UINT16 elemID, UINT16 ch, UINT16 pos,
                                              UINT16 frameType );
static void IDParamProtocol_ProcScollID( ID_PARAM_FRAME_BUFFER_PTR frame_ptr,
                                         UARTMGR_PARAM_DATA_PTR data_ptr, UINT16 ch,
                                         UINT16 frameType, UINT32 frameTime,
                                         TIMESTAMP frameTS );
#include "IDParamUserTables.c"

/*****************************************************************************/
/* Constant Data                                                             */
/*****************************************************************************/


/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:    IDParamProtocol_Initialize
 *
 * Description: Initializes the ID Param Protocol functionality
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
void IDParamProtocol_Initialize ( void )
{
  UINT16 i,j;

  memset ( (void *) &m_IDParam_Status, 0, sizeof(m_IDParam_Status) );
  memset ( (void *) &m_IDParam_Cfg, 0, sizeof(m_IDParam_Cfg) );

  // Add an entry in the user message handler table for F7X Cfg Items
  User_AddRootCmd(&IDParamProtocolRootTblPtr);

  // Copy data from CfgMgr, to be used as the run time
  memcpy( (void *) &m_IDParam_Cfg, (const void *) &CfgMgr_RuntimeConfigPtr()->IDParamConfig,
          sizeof(m_IDParam_Cfg));

  memset ( (void *) &m_IDParam_Frame, 0, sizeof(m_IDParam_Frame) );

  memset ( (void *) &m_IDParam_RawBuff, 0, sizeof(m_IDParam_RawBuff) );

  memset ( (void *) &m_IDParam_FrameBuff, 0, sizeof(m_IDParam_FrameBuff) );

  for (i=0;i<UART_NUM_OF_UARTS;i++)
  {
    m_IDParam_RawBuff[i].pRd = (UINT8 *) &m_IDParam_RawBuff[i].data[0];
    m_IDParam_RawBuff[i].pWr = m_IDParam_RawBuff[i].pRd;
    m_IDParam_RawBuff[i].pStart = m_IDParam_RawBuff[i].pWr;
    m_IDParam_RawBuff[i].stateSearch = ID_PARAM_SEARCH_SYNC_WORD;
    m_IDParam_RawBuff[i].pEnd = (UINT8 *) &m_IDParam_RawBuff[i].data[ID_PARAM_MAX_RAW_BUF];
    m_IDParam_RawBuff[i].pBegin = (UINT8 *) &m_IDParam_RawBuff[i].data[0];

    m_IDParam_FrameBuff[i].pFrameHdr = (ID_PARAM_FRAME_HDR_PTR)
                                       &m_IDParam_FrameBuff[i].data[0];

    for (j=0;j<ID_PARAM_CFG_MAX;j++)
    {
      m_IDParam_Frame[i][ID_PARAM_FRAME_TYPE_FADEC].data[j].elementID = ID_PARAM_NOT_USED;
      m_IDParam_Frame[i][ID_PARAM_FRAME_TYPE_FADEC].data[j].index = ID_PARAM_INDEX_NOT_USED;

      m_IDParam_Frame[i][ID_PARAM_FRAME_TYPE_PEC].data[j].elementID = ID_PARAM_NOT_USED;
      m_IDParam_Frame[i][ID_PARAM_FRAME_TYPE_PEC].data[j].index = ID_PARAM_INDEX_NOT_USED;
    }
  }

  for (i=0;i<UART_NUM_OF_UARTS;i++) {
    for (j=0;j<ID_PARAM_SCROLL_ID_MAX;j++) {
      m_IDParam_Scroll[i].info[j].id_elemId = ID_PARAM_NOT_USED;
      m_IDParam_Scroll[i].info[j].id_value = ID_PARAM_NOT_USED;
      m_IDParam_Scroll[i].info[j].pos_id = ID_PARAM_INDEX_NOT_USED;
      m_IDParam_Scroll[i].info[j].pos_value = ID_PARAM_INDEX_NOT_USED;
    }
    m_IDParam_Scroll[i].numCfgScrollID = 0;

    for (j=0;j<ID_PARAM_CFG_MAX;j++) {
     m_IDParam_ScrollList[i].data[j].index = ID_PARAM_INDEX_NOT_USED;
    }

    // Setup Scroll Configuration
    if ((m_IDParam_Cfg.scroll.id != ID_PARAM_NOT_USED) &&
        (m_IDParam_Cfg.scroll.value != ID_PARAM_NOT_USED))
    {
      m_IDParam_Scroll[i].info[0].id_elemId = m_IDParam_Cfg.scroll.id;
      m_IDParam_Scroll[i].info[0].id_value = m_IDParam_Cfg.scroll.value;
      m_IDParam_Scroll[i].numCfgScrollID++;
    }
  }

#ifdef ID_TIMING_TEST
  /*vcast_dont_instrument_start*/
  for (i=0;i<ID_TIMING_TEST;i++)
  {
    m_IDTiming_Buff[i] = 0;
  }
  m_IDTiming_Index = 0;
  m_IDTiming_Start = 0;
  m_IDTiming_End = 0;
  /*vcast_dont_instrument_end*/
#endif

}



/******************************************************************************
 * Function:    IDParamProtocol_Handler
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
BOOLEAN  IDParamProtocol_Handler ( UINT8 *data, UINT16 cnt, UINT16 ch,
                                   void* uart_data_ptr, void* info_ptr )
{
  UARTMGR_PARAM_DATA_PTR   data_ptr;

  ID_PARAM_RAW_BUFFER_PTR  buff_ptr;
  UINT8 *dest_ptr, *end_ptr, *begin_ptr;

  BOOLEAN bNewData;
  UINT16 i;
  UINT32 tick;


#ifdef ID_TIMING_TEST
  /*vcast_dont_instrument_start*/
// m_IDTiming_Start = TTMR_GetHSTickCount();
   /*vcast_dont_instrument_end*/
#endif

  bNewData = FALSE;

  data_ptr = (UARTMGR_PARAM_DATA_PTR) uart_data_ptr;

  buff_ptr = (ID_PARAM_RAW_BUFFER_PTR) &m_IDParam_RawBuff[ch];

  dest_ptr = buff_ptr->pWr;
  end_ptr = buff_ptr->pEnd;
  begin_ptr = buff_ptr->pBegin;

  tick = CM_GetTickCount();

  // If new bytes Rx, process
  if (cnt > 0)
  {
    // Copy any data into buffer.  NOTE: Don't expect buff over write as buff
    //  >> frame size, thus no additional code to protect.
    for (i=0;i<cnt;i++)
    {
      *dest_ptr++ = *data++;
      ID_PARAM_WRAP_RAW_BUFF (dest_ptr, end_ptr, begin_ptr);
    }
    buff_ptr->pWr = dest_ptr;  // Update index for next write
    buff_ptr->cnt += cnt;      // Update buff cnt
    buff_ptr->lastActivity_tick = tick;
  }

  bNewData = IDParamProtocol_FrameSearch ( buff_ptr, data_ptr, ch );

#ifdef ID_TIMING_TEST
  /*vcast_dont_instrument_start*/
/*
   m_IDTiming_End = TTMR_GetHSTickCount();
   m_IDTiming_Buff[m_IDTiming_Index] = (m_IDTiming_End - m_IDTiming_Start);
   m_IDTiming_Index = (++m_IDTiming_Index) % ID_TIMING_BUFF_MAX;

if (m_IDTiming_Index == 0)
   m_IDTiming_Index = 1;  // Set BP here
*/
   /*vcast_dont_instrument_end*/
#endif

  return (bNewData);
}



/******************************************************************************
 * Function:    IDParamProtocol_GetStatus
 *
 * Description: Utility function to request current ID Param Protocol[index] status
 *
 * Parameters:  index - Uart Port Index
 *
 * Returns:     Ptr to ID Param Status data
 *
 * Notes:       None
 *
 *****************************************************************************/
ID_PARAM_STATUS_PTR IDParamProtocol_GetStatus (UINT8 index)
{
  return ( (ID_PARAM_STATUS_PTR) &m_IDParam_Status[index] );
}


/******************************************************************************
 * Function:    IDParamProtocol_SensorSetup()
 *
 * Description: Sets the m_UartMgr_WordInfo[] table with parse info for a
 *              specific Id Param Protocol Data Word
 *
 * Parameters:  gpA - General Purpose A (See below for definition)
 *              gpB - General Purpose B (See below for definition)
 *              param - ID Parameter to monitor as sensor
 *              info_ptr- call be ptr to update Uart Mgr Word Info data
 *              uart_data_ptr - Ptr to UARTMGR_PARAM_DATA data
 *              ch - Uart Port Index
 *
 * Returns:     TRUE decode Ok
 *
 * Notes:
 *  gpA
 *  ===
 *     bits 10 to  0: Not Used
 *     bits 15 to 11: Word Size
 *     bits 20 to 16: MSB Position
 *     bits 22 to 21: 00 = "DISC" where all bits are used
 *                  : 01 = "BNR" where MSB = bit 15 and always = sign bit
 *                  : 10, 11 = Not Used
 *     bits 31 to 23: Not Used
 *
 *  gpB
 *  ===
 *     bits 31 to  0: timeout (msec)
 *
 *****************************************************************************/
BOOLEAN IDParamProtocol_SensorSetup ( UINT32 gpA, UINT32 gpB, UINT16 param,
                                   void *info_ptr, void *uart_data_ptr, UINT16 ch )
{
  BOOLEAN bOk;
  UARTMGR_WORD_INFO_PTR word_info_ptr;
  UARTMGR_PARAM_DATA_PTR data_ptr;
  UARTMGR_RUNTIME_DATA_PTR runtime_data_ptr;
  UINT16 i;


  bOk = FALSE;
  word_info_ptr = (UARTMGR_WORD_INFO_PTR) info_ptr;

  // Parse Word Size
  word_info_ptr->word_size = (gpA >> 11) & 0x1F;

  // Parse Word MSB Positioin
  word_info_ptr->msb_position = (gpA >> 16) & 0x1F;

  // Parse Data Loss TimeOut
  word_info_ptr->dataloss_time = gpB;

  // Update id
  word_info_ptr->id = param;

  // Update data type
  word_info_ptr->type = (UART_DATA_TYPE) ((gpA >> 21) & 0x0F);

  // Find and return index into UARTMGR_PARAM_DATA[] array where data value can be
  //   read.
  data_ptr = (UARTMGR_PARAM_DATA_PTR) uart_data_ptr;
  for (i=0;i<UARTMGR_MAX_PARAM_WORD;i++)
  {
    runtime_data_ptr = (UARTMGR_RUNTIME_DATA_PTR) &data_ptr->runtime_data;
    if ( runtime_data_ptr->id == UARTMGR_WORD_ID_NOT_INIT )
    { // end of UARTMGR_DATA, param has not be setup for Recording from cfg
      break;
    }
    if ( runtime_data_ptr->id == param )
    { // param has been setup. return this entry to sensor
      word_info_ptr->nIndex = i;
      bOk = TRUE;
      break;
    }
    data_ptr++;
  }

  // If UARTMGR_PARAM_DATA[] entry not found AND < UARTMGR_MAX_PARAM_WORD - 1, then this
  //   param must not have been selected for record, but now is req for sensor.  Add entry.
  if ( (word_info_ptr->nIndex == UARTMGR_WORD_ID_NOT_USED) &&
       (i < (UARTMGR_MAX_PARAM_WORD - 1)) )
  {
    // Loop thru to find end of UARTMGR_PARAM_DATA[] array
    i++; // Skip the "delimiter" between "Record" and "Monitored"
         //    UARTMGR_PARAM_DATA[]
    for ( ;i<UARTMGR_MAX_PARAM_WORD;i++)
    {
      data_ptr++;  // From code above, data_ptr ptr to UARTMGR_WORD_ID_NOT_INIT
                   //  "delimiter".  Move pass it and find open entry.
      runtime_data_ptr = (UARTMGR_RUNTIME_DATA_PTR) &data_ptr->runtime_data;
      if ( runtime_data_ptr->id == UARTMGR_WORD_ID_NOT_INIT )
      { // Empty entry found, use it
        runtime_data_ptr->id = param;
        // ->scale, ->scale_lsb, ->cfgTimeOut DR->Tol not needed as this uart
        //   param will only be monitored
        word_info_ptr->nIndex = i;
        m_IDParam_Status[ch].cntMonParam++;
        bOk = TRUE;
        break;
      } // end if ( runtime_data_ptr->id == UARTMGR_WORD_ID_NOT_INIT )
    } // end for loop ( ;i<UARTMGR_MAX_PARAM_WORD;i++)
  } // end if nIndex not set and < (UARTMGR_MAX_PARAM_WORD - 1)

  return (bOk);
}



/******************************************************************************
 * Function:    IDParamProtocol_ReturnFileHdr()
 *
 * Description: Function to return the ID Param file hdr structure
 *
 * Parameters:  dest - pointer to destination bufffer
 *              max_size - max size of dest buffer
 *              ch - uart channel requested
 *
 * Returns:     cnt - byte count of data returned
 *
 * Notes:       none
 *
 *****************************************************************************/
UINT16 IDParamProtocol_ReturnFileHdr ( UINT8 *dest, const UINT16 max_size, UINT16 ch )
{
  ID_PARAM_FILE_HDR_PTR pfileHdr;
  ID_PARAM_DATA_HDR_PTR pIDParam;
  ID_PARAM_DATA_CFG_PTR pDataCfg;
  UINT16 i, cnt, size;


  pfileHdr = (ID_PARAM_FILE_HDR_PTR) dest;
  pIDParam = (ID_PARAM_DATA_HDR_PTR) &pfileHdr->data[0];

  pDataCfg = (ID_PARAM_DATA_CFG_PTR) &m_IDParam_Cfg.data[0];

  // loop thru and find cfg params
  cnt = 0;
  for (i=0;i<ID_PARAM_CFG_MAX;i++)
  {
    if (pDataCfg->id != ID_PARAM_NOT_USED)
    {
      pIDParam->id = pDataCfg->id;
      pIDParam->scale = pDataCfg->scale;
      pIDParam->tol = pDataCfg->tol;
      pIDParam->timeout = pDataCfg->timeout_ms;
      pIDParam++;
      cnt++;
    }
    pDataCfg++;
  }

  size = sizeof(ID_PARAM_FILE_HDR) - ( (ID_PARAM_CFG_MAX - cnt) *
                                        sizeof(ID_PARAM_DATA_HDR) );
  pfileHdr->size = size;
  pfileHdr->numParams = cnt;

  return (size);
}


/******************************************************************************
 * Function:    IDParamProtocol_InitUartMgrData
 *
 * Description: Utility function to initialize Uart Mgr Data from cfg data.
 *
 * Parameters:  ch - Uart Port Index
 *              uart_data_ptr - Ptr to UART Mgr Runtime Data
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
void IDParamProtocol_InitUartMgrData( UINT16 ch, void *uart_data_ptr )
{
  ID_PARAM_DATA_CFG_PTR pDataCfg;
  ID_PARAM_STATUS_PTR pStatus;
  UARTMGR_PARAM_DATA_PTR   data_ptr;
  UINT16 i;

  UARTMGR_RUNTIME_DATA_PTR runtime_data_ptr;
  REDUCTIONDATASTRUCT_PTR data_red_ptr;


  pDataCfg = (ID_PARAM_DATA_CFG_PTR) &m_IDParam_Cfg.data[0];
  pStatus = (ID_PARAM_STATUS_PTR) &m_IDParam_Status[ch];
  data_ptr = (UARTMGR_PARAM_DATA_PTR) uart_data_ptr;

  // Loop thru cfg and pull all Param ID to be monitored / recorded.
  for (i=0;i<ID_PARAM_CFG_MAX;i++)
  {
    if (pDataCfg->id != ID_PARAM_NOT_USED)
    {
      runtime_data_ptr = (UARTMGR_RUNTIME_DATA_PTR) &data_ptr->runtime_data;
      data_red_ptr = (REDUCTIONDATASTRUCT_PTR) &data_ptr->reduction_data;

      runtime_data_ptr->id = pDataCfg->id;
      runtime_data_ptr->scale = pDataCfg->scale;
      if ( pDataCfg->scale != DR_NO_RANGE_SCALE )
      {
        // Binary Data "BNR" type
        runtime_data_ptr->scale_lsb = (FLOAT32) pDataCfg->scale / 32768;
      }
      else
      {
        // Discrete Data type
        runtime_data_ptr->scale_lsb = 1.0f;
      }
      runtime_data_ptr->cfgTimeOut = pDataCfg->timeout_ms;

      data_red_ptr->Tol = pDataCfg->tol; // Update tol

      data_ptr++; // Move to the next entry
      pStatus->cntRecParam++;
    } // end if (pDataCfg->id != ID_PARAM_NOT_USED)
    pDataCfg++;
  } // end for loop (i=0;i<ID_PARAM_CFG_MAX;i++)

  pStatus->cntMonParam = pStatus->cntRecParam;

}


/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:    IDParamProtocol_FrameSearch
 *
 * Description: Function to search thru raw data buff to search for ID Param Frame
 *
 * Parameters:  buff_ptr - Ptr to Raw Data Buffer
 *              data_ptr - Ptr to UART Mgr Runtime Data
 *              ch - UART channel
 *
 * Returns:     TRUE if New Packet Found
 *
 * Notes:       none
 *
 *****************************************************************************/
static BOOLEAN IDParamProtocol_FrameSearch( ID_PARAM_RAW_BUFFER_PTR buff_ptr,
                                            UARTMGR_PARAM_DATA_PTR data_ptr,
                                            UINT16 ch )
{
  BOOLEAN bNewData, bContinueSearch, bOk;
  UINT8 *pData;
  UINT16 itemp,jtemp;
  ID_PARAM_FRAME_BUFFER_PTR frame_ptr;
  ID_PARAM_STATUS_PTR pStatus;
  UINT32 tick;

  bNewData = FALSE;
  bContinueSearch = TRUE;
  frame_ptr = (ID_PARAM_FRAME_BUFFER_PTR) &m_IDParam_FrameBuff[ch];
  pStatus = (ID_PARAM_STATUS_PTR) &m_IDParam_Status[ch];
  tick = CM_GetTickCount();


  while (bContinueSearch == TRUE)
  {
    switch ( buff_ptr->stateSearch )
    {
      case ID_PARAM_SEARCH_SYNC_WORD:
        // Loop until SYNC WORD found OR buff is empty.  NOTE: Uart HW FIFO is 512 bytes, thus
        //   worst case have 512 bytes MAX to search.  Normally @ 38400 and 10 msec -> 38.4
        //   bytes to search threw
        // Note: If get buff where we have 0x8000 0x0005 repeating then we can be in this loop
        //   100 times for worst case 512 buff size and normally 10 times..
        while ( ( buff_ptr->cnt > ID_PARAM_FM_SYNC_AND_WORD_SIZE ) &&
                ( buff_ptr->stateSearch == ID_PARAM_SEARCH_SYNC_WORD ) )
        {
          pData = buff_ptr->pRd;
          itemp = (*pData++) << 8;  // High Byte
          ID_PARAM_WRAP_RAW_BUFF ( pData, buff_ptr->pEnd, buff_ptr->pBegin);
          itemp = itemp | (*pData++); // Low Byte
          ID_PARAM_WRAP_RAW_BUFF ( pData, buff_ptr->pEnd, buff_ptr->pBegin);

          if ( itemp == ID_PARAM_SYNC_WORD )
          {
            // Get next two bytes for # elements cnt
            itemp = (*pData++) << 8;  // High Byte
            ID_PARAM_WRAP_RAW_BUFF ( pData, buff_ptr->pEnd, buff_ptr->pBegin);
            itemp = itemp | (*pData++); // Low Byte
            ID_PARAM_WRAP_RAW_BUFF ( pData, buff_ptr->pEnd, buff_ptr->pBegin);
            // Bit #0 is s/b '1' and not part of #total element and total element is shifted
            //     1 to right
            itemp = itemp >> 1;
            // If #total element is within expected range then accept the size value
            if ( ((itemp >= ID_PARAM_FRAME_MIN) && (itemp <= m_IDParam_Cfg.maxWords)) )
            {
              // Have Sync and #total element, now trans to wait for frame rx to complete
              // Overhead 10 = (sync + total e + e pos + e id + cksum) * 2
              frame_ptr->size = (itemp * sizeof(UINT16)) + (ID_PARAM_FRAME_OH * 2);
              buff_ptr->pStart = (UINT8 *) buff_ptr->pRd;           // Update pStart
              buff_ptr->stateSearch = ID_PARAM_SEARCH_PACKET_SIZE;  // Move to next state
              break;  // exit while loop
            }
          } // end if ( itemp == ID_PARAM_SYNC_WORD )
          // Sync Word or total element not found, move to next byte and re-start
          //   sync word search. NOTE: move 1 byte at time as data might not be "word aligned"
          buff_ptr->pRd++;
          ID_PARAM_WRAP_RAW_BUFF (buff_ptr->pRd, buff_ptr->pEnd, buff_ptr->pBegin);
          buff_ptr->cnt = (buff_ptr->cnt > 0) ? (buff_ptr->cnt - 1) : 0; // Flush this byte
          // buff_ptr->cnt--;
        } // end while loop

        // Loop thru Raw Buff and start of sync frame encountered
        if ( (buff_ptr->cnt <= ID_PARAM_FM_SYNC_AND_WORD_SIZE) &&
             (buff_ptr->stateSearch == ID_PARAM_SEARCH_SYNC_WORD) )
        {
          bContinueSearch = FALSE;
        }
        break;

      case ID_PARAM_SEARCH_PACKET_SIZE:
        // If cnt of data in Raw Buff is >= expected size, then process the frame
        if ( buff_ptr->cnt >= frame_ptr->size )
        { // Copy from Raw Buff to the Frame Buff
          if ( ( buff_ptr->pRd + frame_ptr->size ) >= (buff_ptr->pEnd) )
          { // Frame is WRAPPED in Raw Buffer. Need to split
            itemp = (UINT16) ( (UINT32) buff_ptr->pEnd - (UINT32) buff_ptr->pRd );
            memcpy ( (void *) &frame_ptr->data[0], buff_ptr->pRd, itemp);
            jtemp = frame_ptr->size - itemp;
            memcpy ( (void *) &frame_ptr->data[itemp], buff_ptr->pBegin, jtemp);
          }
          else
          {
            // Frame is not WRAPPED in Raw Buffer.  Just copy.
            memcpy ( (void *) &frame_ptr->data[0], buff_ptr->pRd, frame_ptr->size );
          }

          bOk = FALSE;
          // Verify checksum.  Cksum is entire packet except Sync Word and Checksum itself.
          jtemp = (frame_ptr->size/2) - (ID_PARAM_FRAME_CKSUM_END_OFFSET
                                      + ID_PARAM_FRAME_ELEM_CNT_OFFSET);
          itemp = IDParamProtocol_CalcFrameChksum (
                    (UINT16 *) &frame_ptr->data[ID_PARAM_FRAME_ELEM_CNT_OFFSET * 2],
                    jtemp );  // mult '2' since ->data is byte wide and _OFFSET in words.
          if ( itemp == *(UINT16 *) &frame_ptr->data[frame_ptr->size -
                                        (ID_PARAM_FRAME_CKSUM_END_OFFSET * 2)] )
          { // Confirm packet format
            bOk = IDParamProtocol_ConfirmFrameFmt( frame_ptr->pFrameHdr, itemp );
            if (bOk == FALSE)
            {
              pStatus->cntBadFrame++;
              pStatus->cntBadFrameInRow =
                  (pStatus->cntBadFrameInRow < ID_PARAM_FRAME_CONSEC_BAD_CNT) ?
                  (pStatus->cntBadFrameInRow + 1) : pStatus->cntBadFrameInRow;
            }
          }
          else { // checksum does not match !
            pStatus->cntBadCRC++;
            pStatus->cntBadCrcInRow =
                (pStatus->cntBadCrcInRow < ID_PARAM_FRAME_CONSEC_CRC_CNT) ?
                (pStatus->cntBadCrcInRow + 1) : pStatus->cntBadCrcInRow;
          }

          if (bOk == TRUE)
          { // Process Element ID / Position
            IDParamProtocol_ProcElemID( frame_ptr->pFrameHdr, data_ptr, ch );
            // Process Frame
            IDParamProtocol_ProcFrame(frame_ptr, data_ptr, ch);
            // Update Raw Buffer Ptrs
            buff_ptr->pRd = buff_ptr->pRd + frame_ptr->size;
            if ( buff_ptr->pRd >= (buff_ptr->pEnd) )  // Buff Wrapped
            {
              itemp = (UINT16) ( buff_ptr->pRd - buff_ptr->pEnd );
              buff_ptr->pRd = buff_ptr->pBegin + itemp;
            }
            buff_ptr->cnt = (buff_ptr->cnt < frame_ptr->size) ? 0 :
                            (buff_ptr->cnt - frame_ptr->size);

            bNewData = TRUE;
          }
          else
          {
            // Frame checksum was bad or we did not have a proper frame
            //  go back to buffer and re-search
            buff_ptr->pRd++;
            ID_PARAM_WRAP_RAW_BUFF (buff_ptr->pRd, buff_ptr->pEnd, buff_ptr->pBegin);
            buff_ptr->cnt = (buff_ptr->cnt > 0) ? (buff_ptr->cnt - 1) : 0; // Flush this byte
          }
          buff_ptr->stateSearch = ID_PARAM_SEARCH_SYNC_WORD;
          // bContinueSearch = TRUE @ this pt.  If Raw Buff have more packets, will process
        }
        else // else wait until complete packet frame has been received
        {
          bContinueSearch = FALSE;  // Exit overall search, complete frame not Rx yet.
        }
        break;

      default:
        // ASSERT if we get here
        break;

    } // End switch ( buff_ptr->stateSearch )
  } // End while (bContinue == TRUE )

  IDParamProtocol_CheckSyncLoss ( bNewData, ch, pStatus, buff_ptr, tick );

  return (bNewData);
}


/******************************************************************************
 * Function:    IDParamProtocol_CalcFrameChksum
 *
 * Description: Function to calculate the ID Param Checksum (word wide)
 *
 * Parameters:  pData - Ptr to Frame Buffer (excludes Sync Word)
 *              word_cnt - Word count to calculate checksum over
 *
 * Returns:     UINT16 calculated checksum
 *
 * Notes:       none
 *
 *****************************************************************************/
static UINT16 IDParamProtocol_CalcFrameChksum( UINT16 *pData, UINT16 word_cnt  )
{
  UINT16 i;
  UINT16 cksum;

  cksum = 0;
  for (i=0;i<word_cnt;i++)
  {
    cksum += *pData++;
  }
  cksum = cksum | 0x0001;// Per Definition of ID Param Checksum, bit #0 forced to '1' always.

  return (cksum);
}


/******************************************************************************
 * Function:    IDParamProtocol_ConfirmFrameFmt
 *
 * Description: Function to verify the ID Param Frame
 *
 * Parameters:  pFrameHdr - Ptr to Frame Hdr
 *              cksum - Raw frame checksum value
 *
 * Returns:     TRUE if frame format is confirmed
 *
 * Notes:
 *  1) Checks Bit #0 of Word 1 "Total Element" is "1"
 *  2) Checks Bit #0 of Word 2 "Word Position" is "1"
 *  3) Checks Bit #0 of Word 3 "Element ID" is "1"
  *
 *****************************************************************************/
static BOOLEAN IDParamProtocol_ConfirmFrameFmt ( ID_PARAM_FRAME_HDR_PTR pFrameHdr,
                                                 UINT16 cksum )
{
  BOOLEAN bOk;

  bOk = FALSE;

  if ( (ID_PARAM_FRAME_WORD_BIT0_SET(pFrameHdr->totalElements)) ||
       (ID_PARAM_FRAME_WORD_BIT0_SET(pFrameHdr->elementPos)) ||
       (ID_PARAM_FRAME_WORD_BIT0_SET(pFrameHdr->elementID)) ||
       (ID_PARAM_FRAME_WORD_BIT0_SET(cksum)) )
  {
    bOk = TRUE;
  }

  return (bOk);
}


/******************************************************************************
 * Function:    IDParamProtocol_ProcElemID
 *
 * Description: Process the Element ID / Pos Info in the Frame Hdr
 *
 * Parameters:  pFrameHdr - Ptr to Frame Hdr
 *              data_ptr - Ptr to UARTMGR_PARAM_DATA_PTR
 *              ch - Uart Channel
 *
 * Returns:     none
 *
 * Notes:       none
 *
 *****************************************************************************/
static void IDParamProtocol_ProcElemID( ID_PARAM_FRAME_HDR_PTR pFrameHdr,
                                        UARTMGR_PARAM_DATA_PTR data_ptr,
                                        UINT16 ch)
{
  UINT16 index, elemID, frameType, i;
  ID_PARAM_FRAME_PTR frameData_ptr;
  UARTMGR_RUNTIME_DATA_PTR runtime_data_ptr;

  // Get Element ID / Pos Words from the frame header
  // Element Position in frame.  Bit 15 = LRU flag (1=PEC,0=FADEC), Bit 0 = 1
  index = (pFrameHdr->elementPos & 0x7FFE) >> 1;
  // Element pos starts @ '1' while we start at '0'
  index = (index > 0) ? (index - 1) : 0;
  elemID = pFrameHdr->elementID >> 1;
  frameType = (pFrameHdr->elementPos & 0x8000) >> 15;
  // Ptr to FADEC=0 or PEC=1 Data Frame
  frameData_ptr = (ID_PARAM_FRAME_PTR) &m_IDParam_Frame[ch][frameType];
  // Update # element count, if not updated.
  frameData_ptr->size = (pFrameHdr->totalElements >> 1);
    // Don't expect frame count to change. Do we add code to check ?

  if ( frameData_ptr->data[index].elementID == ID_PARAM_NOT_USED )
  { // Assign slot to this elem ID
    frameData_ptr->data[index].elementID = elemID;
    if ( IDParamProtocol_CheckScrollID(elemID, ch, index, frameType) == FALSE )
    {
      for (i=0;i<ID_PARAM_CFG_MAX;i++)
      {
        // Ptr to UART_PARAM_DATA entry
        runtime_data_ptr = (UARTMGR_RUNTIME_DATA_PTR) &data_ptr->runtime_data;
        if (runtime_data_ptr->id != ID_PARAM_NOT_USED)
        {
          if (runtime_data_ptr->id == elemID)
          {
            frameData_ptr->data[index].index = i;
            break; // Corresponding entry found.  Exit
          } // end of if (runtime_data_ptr->id == elemID)
        } // end of if (runtime_data_ptr->id != ID_PARAM_NOT_USED)
        else
        {
          if ( (i <(ID_PARAM_CFG_MAX - 1)) &&
               ((runtime_data_ptr + 1)->id == ID_PARAM_NOT_USED) )
          { // Two IDs with ID_PARAM_NOT_USED signifies end of list.
            break; // End of list found. Exit for loop early here.
          }
        } // end of else (runtime_data_ptr->id == ID_PARAM_NOT_USED)
        data_ptr++;
      } // end of for (i=0;i<ID_PARAM_CFG_MAX;i++)
    } // end of if not Scrolling ID
    else // elemID is Scroll ID
    {
      frameData_ptr->data[index].index = ID_PARAM_INDEX_SCROLL;
    }
  } // end of if elementID == ID_PARAM_NOT_USED
  else
  { // Check that this element ID at this pos has not changed !
    if ( frameData_ptr->data[index].elementID != elemID )
    {
      // TBD increment some counter here / sys log ?
    }
  } // end else elementID != ID_PARAM_NOT_USED
}


/******************************************************************************
 * Function:    IDParamProtocol_ProcFrame
 *
 * Description: Process the data from the ID Param Frame
 *
 * Parameters:  frame_ptr - Ptr to Frame Data
 *              data_ptr - Ptr to UART_PARAM_DATA
 *              ch - UART channel
 *
 * Returns:     none
 *
 * Notes:       none
 *
 *****************************************************************************/
static void IDParamProtocol_ProcFrame( ID_PARAM_FRAME_BUFFER_PTR frame_ptr,
                                       UARTMGR_PARAM_DATA_PTR data_ptr, UINT16 ch )
{
  UINT16 frameType, numElements, i;
  UINT16 *pData;
  ID_PARAM_FRAME_HDR_PTR pFrameHdr;

  ID_PARAM_FRAME_PTR frameData_ptr;
  ID_PARAM_FRAME_DATA_PTR frameElem_ptr;
  UARTMGR_RUNTIME_DATA_PTR runtime_data_ptr;
  ID_PARAM_FRAME_TYPE_PTR pFrameTypeStats;

  UINT32 frameTime;
  TIMESTAMP frameTS;


  pFrameHdr = (ID_PARAM_FRAME_HDR_PTR) frame_ptr->pFrameHdr;

  frameType = (pFrameHdr->elementPos & 0x8000) >> 15;
  // Ptr to FADEC=0 or PEC=1 Data Frame Info
  frameData_ptr = (ID_PARAM_FRAME_PTR) &m_IDParam_Frame[ch][frameType];
  // Note: numElement value should remain constant for FADEC or PEC frame, and is check
  //       to ensure < 256 entries.
  numElements = (frame_ptr->size / sizeof(UINT16)) - ID_PARAM_FRAME_OH;
  // Ptr to Frame Data
  pData = (UINT16 *) &pFrameHdr->data;
  // Ptr to Frame Data Info
  frameElem_ptr = (ID_PARAM_FRAME_DATA_PTR) &frameData_ptr->data[0];
  frameTime = CM_GetTickCount();
  CM_GetTimeAsTimestamp( (TIMESTAMP*) &frameTS);

  // Update Frame Type Statistics
  pFrameTypeStats = (ID_PARAM_FRAME_TYPE_PTR) &m_IDParam_Status[ch].frameType[frameType];
  pFrameTypeStats->nWords = frame_ptr->size;
  pFrameTypeStats->nElements = pFrameHdr->totalElements >> 1; // param shifted << 1 in raw
  pFrameTypeStats->cntGoodFrames++;
  pFrameTypeStats->lastFrameTime_tick = frameTime;

  // Loop thru m_IDParam_FrameBuff frame data, with m_IDParam_Frame as corresponding list
  //  based on the number of element indicated in Frame
  for (i=0;i<numElements;i++)
  {
    // if (frameElem_ptr->index != ID_PARAM_INDEX_NOT_USED)
    if (frameElem_ptr->index < ID_PARAM_INDEX_SCROLL)
    { // this parameter has been selected.  copy current value to UART_PARAM_DATA
      runtime_data_ptr = (UARTMGR_RUNTIME_DATA_PTR)
                         &(data_ptr + frameElem_ptr->index)->runtime_data;
      runtime_data_ptr->val_raw = *pData;
      runtime_data_ptr->rxTime = frameTime;
      runtime_data_ptr->rxTS = frameTS;
      runtime_data_ptr->bNewVal = TRUE;
      runtime_data_ptr->bValid = TRUE;
    }
    pData++;  // Move to next entry in m_IDParam_FrameBuff
    frameElem_ptr++;  // Move to next entry in m_IDParam_Frame
  } // end for loop (i=0;i<numElements;i++)

  // Process any cfg scroll ID
  if (m_IDParam_Scroll[ch].numCfgScrollID > 0)
  {
    IDParamProtocol_ProcScollID ( frame_ptr, data_ptr, ch, frameType, frameTime, frameTS );
  }

}


/******************************************************************************
 * Function:    IDParamProtocol_CheckSyncLoss
 *
 * Description: Checks if Sync Loss has occurred
 *
 * Parameters:  bNewData - New Data Frame Received
 *              ch - UART channel
 *              pStatus - Ptr to ID_PARAM_STATUS of ch
 *              buff_ptr - Ptr to ID_PARAM_RAW_BUFFER for ch
 *              tick - current tick time
 *
 * Returns:     none
 *
 * Notes:       none
 *
 *****************************************************************************/
static void IDParamProtocol_CheckSyncLoss( BOOLEAN bNewData, UINT16 ch,
                                           ID_PARAM_STATUS_PTR pStatus,
                                           ID_PARAM_RAW_BUFFER_PTR buff_ptr,
                                           UINT32 tick )
{
  BOOLEAN bSync, bIdleTimeOut;

  // Check Sync frame timeout or bad frame for loss of sync
  if  (bNewData == FALSE)
  { // If several bad frames set sync to FALSE
    bSync = TRUE;
    if ( (pStatus->cntBadFrameInRow >= ID_PARAM_FRAME_CONSEC_BAD_CNT) ||
         (pStatus->cntBadCRC >= ID_PARAM_FRAME_CONSEC_CRC_CNT) )
    {
      bSync = FALSE;
    }
    // NOTE: this check s/b performed periodically for new ID_PARAM_FRAME_IDLE_TIMEOUT
    bIdleTimeOut = FALSE;
    if ((buff_ptr->lastActivity_tick + ID_PARAM_FRAME_IDLE_TIMEOUT) < tick)
    { // Idle time exceed, flush buff and reset
      buff_ptr->pRd = buff_ptr->pBegin;
      buff_ptr->pWr = buff_ptr->pBegin;
      buff_ptr->pStart = buff_ptr->pBegin;
      buff_ptr->cnt = 0;
      buff_ptr->stateSearch = ID_PARAM_SEARCH_SYNC_WORD;
      buff_ptr->lastActivity_tick = tick;  // Next Idle period
      bSync = FALSE;
      bIdleTimeOut = TRUE;
    }
    if ((bSync == FALSE) && (pStatus->sync == TRUE)) //If prev sync is TRUE, then sync loss
    {
      pStatus->sync = FALSE; // Reset to FALSE.  Loss Sync
      GSE_DebugStr(NORMAL,TRUE,"IDParam: Sync Loss (Ch = %d)\r\n", ch);
      IDParamProtocol_CreateSyncLog ( bIdleTimeOut, pStatus );
    }
  }
  else
  {
    pStatus->cntGoodFrames++;
    pStatus->cntBadFrameInRow = 0;
    pStatus->cntBadCrcInRow = 0;
    pStatus->lastFrameTime_tick = tick;
    if (pStatus->sync == FALSE)
    {
      pStatus->sync = TRUE;  // Set to SYNC true
      pStatus->cntReSync = (pStatus->cntGoodFrames != 1) ? ( pStatus->cntReSync + 1)
                           : pStatus->cntReSync;
      GSE_DebugStr(NORMAL,TRUE,"IDParam: Sync Frame (Ch = %d)\r\n", ch);
    }
  }
}


/******************************************************************************
 * Function:    IDParamProtocol_CreateSyncLog
 *
 * Description: Generate and Record ID Param Sync Loss Log
 *
 * Parameters:  bIdleTimeOut - Sync Loss due to IDLE timeout exceeded
 *              pStatus - Ptr to ID_PARAM_STATUS of ch
 *
 * Returns:     none
 *
 * Notes:       none
 *
 *****************************************************************************/
static void IDParamProtocol_CreateSyncLog( BOOLEAN bIdleTimeOut,
                                           ID_PARAM_STATUS_PTR pStatus )
{
  ID_PARAM_SYNC_LOSS_LOG log;

  log.cntBadCRC = pStatus->cntBadCRC;
  log.cntBadFrame = pStatus->cntBadFrame;
  log.cntResync = pStatus->cntReSync;
  log.idleTimeOut = bIdleTimeOut;
  LogWriteSystem (SYS_ID_UART_ID_PARAM_SYNC_LOSS, LOG_PRIORITY_LOW, &log, sizeof(log), NULL);
}


/******************************************************************************
 * Function:    IDParamProtocol_CheckScrollID
 *
 * Description: Check and process Scroll ID parameter
 *
 * Parameters:  elemID - Element ID from Frame Header
 *              ch - Uart Channel
 *              pos - Word Position in Frame
 *              frameType - FADEC (0) or PEC (1) frame
 *
 * Returns:     TRUE ID is scrolling ID (Param ID or Value ID)
 *
 * Notes:       none
 *
 *****************************************************************************/
static BOOLEAN IDParamProtocol_CheckScrollID( UINT16 elemID, UINT16 ch, UINT16 pos,
                                              UINT16 frameType )
{
  ID_PARAM_SCROLL_INFO_PTR scrollInfo_ptr;
  UINT16 i,j;
  BOOLEAN bScrollID;

  scrollInfo_ptr = (ID_PARAM_SCROLL_INFO_PTR) &m_IDParam_Scroll[ch].info[0];
  bScrollID = FALSE;
  j = m_IDParam_Scroll[ch].numCfgScrollID;

  for (i=0;i<j;i++)
  {
    if ( (scrollInfo_ptr->id_elemId == elemID) || (scrollInfo_ptr->id_value == elemID) )
    {
      scrollInfo_ptr->pos_id = (scrollInfo_ptr->id_elemId == elemID) ? pos :
                                scrollInfo_ptr->pos_id;
      scrollInfo_ptr->pos_value =  (scrollInfo_ptr->id_value == elemID) ? pos :
                                scrollInfo_ptr->pos_value;
      scrollInfo_ptr->frameType = frameType;
      bScrollID = TRUE;
      break; // Exit, elemID is a scroll ID
    }
    scrollInfo_ptr++;  // Move to next entry in Scroll Info Table
  } // end for loop i

  return (bScrollID);
}


/******************************************************************************
 * Function:    IDParamProtocol_ProcScollID
 *
 * Description: Process Scroll ID parameters in the Frame
 *
 * Parameters:  frame_ptr - Ptr to Frame Data
 *              data_ptr - Ptr to UART_PARAM_DATA
 *              ch - UART channel
 *              frameType - FADEC = 0, PEC = 1
 *              frameTime - Current tick time
 *              frameTS - Current time in TIMESTAMP
 *
 * Returns:     none
 *
 * Notes:       none
 *
 *****************************************************************************/
static void IDParamProtocol_ProcScollID( ID_PARAM_FRAME_BUFFER_PTR frame_ptr,
                                         UARTMGR_PARAM_DATA_PTR data_ptr, UINT16 ch,
                                         UINT16 frameType, UINT32 frameTime,
                                         TIMESTAMP frameTS )
{
  ID_PARAM_SCROLL_INFO_PTR scrollInfo_ptr;
  UINT16 i,j,k;
  UINT16 elemID;
  UINT16 *pData;
  UINT16 imax, jmax;
  ID_PARAM_FRAME_DATA_PTR pScrollData;
  UARTMGR_RUNTIME_DATA_PTR runtime_data_ptr;
  ID_PARAM_FRAME_HDR_PTR pFrameHdr;


  scrollInfo_ptr = (ID_PARAM_SCROLL_INFO_PTR) &m_IDParam_Scroll[ch].info[0];
  imax = m_IDParam_Scroll[ch].numCfgScrollID;
  pFrameHdr = (ID_PARAM_FRAME_HDR_PTR) frame_ptr->pFrameHdr;

  for (i=0;i<imax;i++) // Loop thru Scroll Info Table
  {
    if ( (scrollInfo_ptr->frameType == frameType) &&
         (scrollInfo_ptr->pos_id != ID_PARAM_INDEX_NOT_USED) &&
         (scrollInfo_ptr->pos_value != ID_PARAM_INDEX_NOT_USED) )
    {
      pData = (UINT16 *) &pFrameHdr->data;    // Ptr to Raw Data Frame
      pData = pData + scrollInfo_ptr->pos_id; // Jump to Word Pos that contains Elem ID
      elemID = *pData; // Get Elem ID
      // Ptr to Scroll List of Elem ID
      pScrollData = (ID_PARAM_FRAME_DATA_PTR) &m_IDParam_ScrollList[ch].data[0];
      jmax = m_IDParam_ScrollList[ch].size;
      // Loop thru Scroll List to find Elem ID entry, if exists.
      for (j=0;j<jmax;j++)
      {
        if (pScrollData->elementID == elemID)
        {
          if (pScrollData->index != ID_PARAM_INDEX_NOT_USED)
          {
            runtime_data_ptr = (UARTMGR_RUNTIME_DATA_PTR)
                               &(data_ptr + pScrollData->index)->runtime_data;
            pData = (UINT16 *) &pFrameHdr->data;
            pData = pData + scrollInfo_ptr->pos_value;
            runtime_data_ptr->val_raw = *pData;
            runtime_data_ptr->rxTime = frameTime;
            runtime_data_ptr->rxTS = frameTS;
            runtime_data_ptr->bNewVal = TRUE;
            runtime_data_ptr->bValid = TRUE;
          } // end if elem ID is requested
          // else elem ID not requested
          break; // elemID found and updated. Exit.
        } // end if elem ID found in list
        // else // else elem ID not found in list, move to next entry
        pScrollData++;  // Move to next entry to check
      } // end for j loop thru scoll ID list

      if (j >= jmax ) // If elemID not in Scroll List, add it.
      {
        ++m_IDParam_ScrollList[ch].size; // Add one to size
        pScrollData->elementID = elemID; // pScrollData should be pointing to last entry
        pScrollData->frameType = frameType; // Store frameType with this elem ID
        for (k=0;k<ID_PARAM_CFG_MAX;k++) // Loop and find UART_PARAM_DATA
        {
          // Ptr to UART_PARAM_DATA entry
          runtime_data_ptr = (UARTMGR_RUNTIME_DATA_PTR) &data_ptr->runtime_data;
          if (runtime_data_ptr->id != ID_PARAM_NOT_USED)
          {
            if (runtime_data_ptr->id == elemID)
            {
              pScrollData->index = k;
              break; // Corresponding entry found.  Exit
            } // end of if (runtime_data_ptr->id == elemID)
          } // end of if (runtime_data_ptr->id != ID_PARAM_NOT_USED)
          else
          {
            if ( (k <(ID_PARAM_CFG_MAX - 1)) &&
                 ((runtime_data_ptr + 1)->id == ID_PARAM_NOT_USED) )
            { // Two IDs with ID_PARAM_NOT_USED signifies end of list.
              break; // End of list found. Exit for loop early here.
            }
          } // end of else (runtime_data_ptr->id == ID_PARAM_NOT_USED)
          data_ptr++;
        } // end of for (i=0;i<ID_PARAM_CFG_MAX;i++)
      } // end if elem ID not in scroll List
    } // end if scroll ID found
    // else scroll ID entry not found from frame yet, or diff frame type
    scrollInfo_ptr++; // Move to next entry
  } // end for i loop

}


/*****************************************************************************
 *  MODIFICATIONS
 *    $History: IDParamProtocol.c $
 * 
 * *****************  Version 1  *****************
 * User: Peter Lee    Date: 14-10-08   Time: 7:11p
 * Created in $/software/control processor/code/system
 * SCR #1263 ID Param Protocol Implementation
 *
 *
 *****************************************************************************/
