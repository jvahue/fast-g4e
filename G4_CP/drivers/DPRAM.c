#define DRV_DPRAM_BODY

/******************************************************************************
            Copyright (C) 2008 - 2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.
               
  File:        DPRAM.c      
 
  Description: Dual-port RAM driver provides buffering and interrupt
               driven send and receive capiblity for communicating with the
               microserver.

          
 
  Requires:    ResultCodes.h
               Interrupt Vector "DPRAM_EP5_ISR" be placed at the Edge Port 5
               location in the interrupt vector table
               
  VERSION
  $Revision: 40 $  $Date: 8/28/12 1:06p $
               
 
******************************************************************************/
/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include "mcf548x.h"
/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "alt_basic.h"
#include "ResultCodes.h"
#include "DPRAM.h"
#include "FIFO.h"
#include "TTMR.h"
#include "UtilRegSetCheck.h"
#include "Assert.h"
#include "stddefs.h"

#include "TestPoints.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
//This flexbus control reg bit is not defined in the current Freescale
//header files for some reason, so because the DPRAM configuration needs
//it I had to add it here.
#ifndef MCF_FBCS_CSCR_BEM       
#define MCF_FBCS_CSCR_BEM 0x20
#endif //MCF_FBCS_CSCR_BEM

#define DPRAM_ENABLE_INT  MCF_EPORT_EPIER |= MCF_EPORT_EPIER_EPIE5
#define DPRAM_DISABLE_INT MCF_EPORT_EPIER &= ~MCF_EPORT_EPIER_EPIE5

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
//Block headers are used in the FIFO to define the size of a block
//for transmission to the DPRAM send buffer, as well as the size of blocks
//received from the DPRAM receive buffer.
typedef struct {
  UINT32 Size;
}DPRAM_BLOCK_HEADER;


/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
//CP transmit to MS buffer defintions
#define DPRAM_CPtoMS_Buf          ((INT8*)(DPRAM_CP_TO_MS_BUF_ADDR))
#define DPRAM_CPtoMS_Int          (*(volatile UINT16*)DPRAM_CP_TO_MS_INT_FLAG)
#define DPRAM_CPtoMS_Own          (*(volatile UINT16*)DPRAM_CP_TO_MS_OWN_FLAG)
#define DPRAM_CPtoMS_Cnt          (*(volatile UINT16*)DPRAM_CP_TO_MS_COUNT)

//MS transmit to CP buffer definitions
#define DPRAM_MStoCP_Buf          ((INT8*)(DPRAM_MS_TO_CP_BUF_ADDR))
#define DPRAM_MStoCP_Int          (*(volatile UINT16*)DPRAM_MS_TO_CP_INT_FLAG)
#define DPRAM_MStoCP_Own          (*(volatile UINT16*)DPRAM_MS_TO_CP_OWN_FLAG)
#define DPRAM_MStoCP_Cnt          (*(volatile UINT16*)DPRAM_MS_TO_CP_COUNT)

#define DPRAM_INT_PATTERN         0xA5A5

static RESULT  m_InitStatus;      // Result of PBIT tests

//Local data buffers. The interrupt routine copies data to and from
//these local buffers.
static FIFO DPRAM_TxFIFO;
static FIFO DPRAM_RxFIFO;
static INT8 DPRAM_TxBuf[DPRAM_LOCAL_BUF_SIZE];
static INT8 DPRAM_RxBuf[DPRAM_LOCAL_BUF_SIZE];
static BOOLEAN DPRAM_RxOverflow;
static UINT32 TxMessageCnt;
static UINT32 RxMessageCnt;

static INT32  m_TxSem;            //Used for _WriteBlock. The system can
                                  //accommodate multiple preemptive writers.
static INT32  m_RxBlockCnt;       //Number of data blocks in the RX FIFO pending 
                                  //processing.
static INT32  m_TxBlockCnt;       //Number of data blocks in the TX FIFO
                                  //pending transmission

const REG_SETTING DPRAM_Registers[] = 
{
  //Setup chip select 2 for the DPRAM
  //The addressable DPRAM space is only 16kB, but the minimum that the
  //FlexBus can allocate is 64kB.
  // Chip Select 2: 64 KB of SRAM at base address $40100000
  // Port size = 16 bits
  // Assert chip select on first rising clock edge after address is asserted
  // Generate internal transfer acknowledge after 0 wait states
  // Address is held for 1 clock at end of read and write cycles
  
  // SET_AND_CHECK(MCF_FBCS_CSAR2, 0x40100000, bInitOk);
  {(void *) &MCF_FBCS_CSAR2, 0x40100000, sizeof(UINT32), 0x0, REG_SET, TRUE, 
            RFA_FORCE_UPDATE}, 
  
  //SET_AND_CHECK(MCF_FBCS_CSCR2, (MCF_FBCS_CSCR_AA | MCF_FBCS_CSCR_PS(0x2) | 
  //                               MCF_FBCS_CSCR_BEM), bInitOk); 
  {(void *) &MCF_FBCS_CSCR2, (MCF_FBCS_CSCR_AA | MCF_FBCS_CSCR_PS(0x2) | MCF_FBCS_CSCR_BEM), 
            sizeof(UINT32), 0x0, REG_SET, TRUE, RFA_FORCE_UPDATE}, 
  
  //SET_AND_CHECK(MCF_FBCS_CSMR2, MCF_FBCS_CSMR_V, bInitOk); 
  {(void *) &MCF_FBCS_CSMR2, MCF_FBCS_CSMR_V, sizeof(UINT32), 0x0, REG_SET, TRUE, 
            RFA_FORCE_UPDATE}, 
  
  //SET_CHECK_MASK_AND(MCF_EPORT_EPPAR, MCF_EPORT_EPPAR_EPPA5(0x3), bInitOk);
  {(void *) &MCF_EPORT_EPPAR, MCF_EPORT_EPPAR_EPPA5(0x3), sizeof(UINT16), 0x0, 
            REG_SET_MASK_AND, FALSE, RFA_FORCE_NONE}, 
  
  //SET_CHECK_MASK_OR(MCF_EPORT_EPPAR, MCF_EPORT_EPPAR_EPPA5(0), bInitOk); 
  {(void *) &MCF_EPORT_EPPAR, MCF_EPORT_EPPAR_EPPA5(0), sizeof(UINT16), 0x0, REG_SET_MASK_OR, 
            TRUE, RFA_FORCE_UPDATE}, 
  
  //SET_CHECK_MASK_AND(MCF_EPORT_EPDDR, MCF_EPORT_EPDDR_EPDD5, bInitOk); 
  {(void *) &MCF_EPORT_EPDDR, MCF_EPORT_EPDDR_EPDD5, sizeof(UINT8), 0x0, REG_SET_MASK_AND, 
            TRUE, RFA_FORCE_UPDATE}, 
  
  //SET_CHECK_MASK_OR(MCF_EPORT_EPIER, MCF_EPORT_EPIER_EPIE5, bInitOk); 
  {(void *) &MCF_EPORT_EPIER, MCF_EPORT_EPIER_EPIE5, sizeof(UINT8), 0x0, REG_SET_MASK_OR, 
            FALSE, RFA_FORCE_NONE}, 
  
  //SET_CHECK_MASK_AND(MCF_INTC_IMRL, (MCF_INTC_IMRL_INT_MASK5|MCF_INTC_IMRL_MASKALL), 
  //                                   bInitOk); 
  {(void *) &MCF_INTC_IMRL, (MCF_INTC_IMRL_INT_MASK5|MCF_INTC_IMRL_MASKALL), sizeof(UINT32), 
            0x0, REG_SET_MASK_AND, FALSE, RFA_FORCE_NONE}, 
}; 


/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static BOOLEAN DPRAM_TxToDPRAM(void);
static void    DPRAM_RxFromDPRAM(void);


/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/*****************************************************************************
 * Function:    DPRAM_Init
 *
 * Description: Power-0n built in test for the dual-port memory driver
 *              writes a test pattern to the entire accessable
 *              memory, except for the interrupt flag locations.  All
 *              bits in the memory array, except for interrupt flag locations,
 *              are cleared to zero at the exit of this routine.  
 *
 * Parameters:  SysLogId - ptr to the SYS_APP_ID
 *              pdata    - ptr to return DPRAM_PBIT fail data 
 *              psize    - ptr to return the size of DPRAM_PBIT fail data 
 *
 * Returns:     DRV_OK: Initialized sucessfully
 *              !DRV_OK: See ResultCodes.h
 *
 * Notes:       This PBIT erases the entire array, it is assumed the
 *              Micro-Sever is in reset or otherwise not attempting to access
 *              the dual-port memory.
 *              
 ****************************************************************************/
//RESULT DPRAM_Init(void)
RESULT DPRAM_Init (SYS_APP_ID *SysLogId, void *pdata, UINT16 *psize)
{
  UINT32 i;
  UINT8* ptr;
  BOOLEAN bInitOk; 
  DPRAM_RAM_TEST_RESULT *dpram_result; 
  DPRAM_DRV_PBIT_LOG  *pdest; 

  //Initialize local FIFOs for buffering DPRAM data
  FIFO_Init(&DPRAM_TxFIFO, DPRAM_TxBuf, DPRAM_LOCAL_BUF_SIZE);
  FIFO_Init(&DPRAM_RxFIFO, DPRAM_RxBuf, DPRAM_LOCAL_BUF_SIZE);

  DPRAM_RxOverflow = FALSE;
  RxMessageCnt = 0;
  TxMessageCnt = 0;
  m_TxSem      = 0;
  m_TxBlockCnt = 0;
  m_RxBlockCnt = 0;
  
  memset ( (void *) &m_DPRAM_Status, 0x00, sizeof(DPRAM_STATUS));
  m_DPRAM_Status.minIntrTime = (UINT32)-1;    // init minimum interrupt time

  pdest = (DPRAM_DRV_PBIT_LOG *) pdata;
  memset ( pdest, 0, sizeof(DPRAM_DRV_PBIT_LOG) ); 
  pdest->result = DRV_OK;
  m_InitStatus = DRV_OK;
  
  *psize = sizeof(DPRAM_DRV_PBIT_LOG); 
  
  bInitOk = TRUE; 
  for (i = 0; i < (sizeof(DPRAM_Registers)/sizeof(REG_SETTING)); i++)
  {
    bInitOk &= RegSet( (REG_SETTING_PTR) &DPRAM_Registers[i] ); 
  }
    
  // Perform PBIT RAM Test Only if Reg Init is Ok. 
  if ( TRUE == STPU( bInitOk, eTpDpr3776)) 
  {
    dpram_result = (DPRAM_RAM_TEST_RESULT *) &pdest->dpram_result; 
  
    ptr = (UINT8*)DPRAM_BASE_ADDR;
    //Write a test pattern to the entire memory array
    for(i = 0; i < DPRAM_SIZE; i++)
    {
      *ptr++ = (UINT8)(STPU( i, eTpDpr3612) & 0xFF);
    }
  
    //Read back, and verify test pattern
    ptr = (UINT8*)DPRAM_BASE_ADDR;
    for(i = 0; i < DPRAM_SIZE; i++)
    {
      if(*ptr != (i & 0xFF))
      {
        dpram_result->expectData = (UINT8)i; 
        dpram_result->actualData = *ptr; 
        pdest->result = DRV_DPRAM_PBIT_RAM_TEST_FAILED;
        *SysLogId = DRV_ID_DPRAM_PBIT_RAM_TEST_FAIL;
        m_InitStatus = DRV_DPRAM_PBIT_RAM_TEST_FAILED;
      }
      ptr++; 
    }

    //Clear the memory array to zero, ignoring the result of the pattern test
    ptr = (UINT8*)DPRAM_BASE_ADDR;
    for(i = 0; i < DPRAM_SIZE; i++)
    {
      *ptr++ = 0x00;
    }
  }    
  else 
  {
    pdest->result = DRV_DPRAM_PBIT_REG_INIT_FAIL; 
    *SysLogId = DRV_ID_DPRAM_PBIT_REG_INIT_FAIL; 
    m_InitStatus = DRV_DPRAM_PBIT_REG_INIT_FAIL;
  }

  //Return the PBIT result 
  return pdest->result;
}


/*****************************************************************************
 * Function:    DPRAM_InitStatus
 *
 * Description: Returns the result of DPRAM PBIT.
 *
 * Parameters:  none
 *
 * Returns:     RESULT
 *
 * Notes:       
 *              
 ****************************************************************************/
RESULT DPRAM_InitStatus(void)
{
  return m_InitStatus;
}


/*****************************************************************************
 * Function:    DPRAM_WriteBlock
 *
 * Description: Copy a block into a local FIFO buffer.  Initiate transmission
 *              from the FIFO to the dual-port RAM if the RAM port is idle, 
 *              otherwise the block will be transmitted once the RAM is free
 *              and an interrupt is received.
 *
 *              For every block written by calling WriteBlock, a single
 *              discrete write to the dual-port will be made, irregardles
 *              of the block size written.  The block written must be less
 *              than the dual port write block size (see DPRAM_TX_RX_SIZE )
 *
 * Parameters:  [in] Block  A DPRAM block to write to the DPRAM queue
 *                          The data to write to the queue must be copied
 *                          copied in to the block, and the size field of
 *                          DPRAM_WRITE_BLOCK must be properly set.
 * Returns:     DRV_RESULT: DRV_OK: Data copied sucessfully
 *                          DRV_DPRAM_TX_FULL: Not enough free space to buffer 
 *                                             data, or data size is larger
 *                                             than the transmit buffer.
 *                                             Verify data is not bigger than
 *                                             the total FIFO size and try 
 *                                             again later.
 *                          DRV_DPRAM_INTR_ERROR: Interrupt Error
 *                          DRV_DPRAM_PBIT_REG_INIT_FAIL
 *                          DRV_DPRAM_PBIT_RAM_TEST_FAILED
 * Notes:       
 *              
 ****************************************************************************/
RESULT DPRAM_WriteBlock(const DPRAM_WRITE_BLOCK* block)
{
  RESULT result = DRV_DPRAM_TX_FULL;

  // check for DPRAM Failure
  if (m_InitStatus != DRV_OK)
  {
    result = m_InitStatus;          // PBit Failure
  }
  else if (m_DPRAM_Status.intrFail)
  {
    result = DRV_DPRAM_INTR_ERROR;
  }
  else
  {
    //Increment number of writers
    m_TxSem += 1;

    if( (FIFO_FreeBytes(&DPRAM_TxFIFO) >= (block->size+sizeof(block->size)))
        && (block->size <= DPRAM_TX_RX_SIZE) )
    {
      DPRAM_DISABLE_INT;
      //Add a header to the data and push it into the queue
      if(FIFO_PushBlock(&DPRAM_TxFIFO, block, block->size+sizeof(block->size)))
      {
        //Write to q is complete, decrement number of total writers
        //Increment the total number of blocks written to the q
        m_TxSem -= 1;
        m_TxBlockCnt++;
        if(DPRAM_CPtoMS_Own == DPRAM_TX_OWNS)     
        {          
          if(DPRAM_TxToDPRAM())
          {
            DPRAM_CPtoMS_Int = DPRAM_INT_PATTERN;
          }
        }
        //Write complete, decrement number of writers
        result = DRV_OK;
      }    
      DPRAM_ENABLE_INT;
    }
    result != DRV_OK ? m_TxSem -= 1 : 0;
  }

  return result;
}


/*****************************************************************************
 * Function:    DPRAM_WriteFreeCnt
 *
 * Description: Returns the number of bytes in the FIFO buffer available for
 *              writing data to the DPRAM.
 *
 * Parameters:  none
 *
 * Returns:     returns the result of the call to FIFO_FreeBytes()
 *
 * Notes:       
 *              
 ****************************************************************************/
UINT32 DPRAM_WriteFreeCnt(void)
{
  return FIFO_FreeBytes(&DPRAM_TxFIFO);
}


/******************************************************************************
 * Function:    DPRAM_ReadBlock
 *
 * Description: Check for data in the receive FIFO and return a data block
 *              if one is available.  The caller's buffer should be sized
 *              to receive the maximum block size (DPRAM_TX_RX_SIZE .)
 *
 * Parameters:  [out] Data*: Pointer to a block to read data into
 *              [in]  Size:  Size of the Data buffer in bytes
 *              [out] BytesRead*: Number of bytes actually returned in Data
 *                                (may be null)
 * Returns:     DRV_RESULT: DRV_OK: Data copied sucessfully
 *                          DRV_DPRAM_ERR_RX_OVERFLOW: While receiving data the
 *                                                 FIFO buffer overflowed.
 *                                                 The overflow flag is cleared
 *                                                 at the RX FIFO is flushed
 *                                                 by this call. No data is
 *                                                 returned.
 *                          DRV_DPRAM_ERR_BUF_TOO_SMALL: The block received
 *                                                       from the microserver
 *                                                       will not fit in the
 *                                                       caller's buffer.
 *                          DRV_DPRAM_PBIT_REG_INIT_FAIL
 *                          DRV_DPRAM_PBIT_RAM_TEST_FAILED
 * Notes:       
 *              
 *****************************************************************************/
RESULT DPRAM_ReadBlock(INT8* Data, UINT32 Size, UINT32* BytesRead)
{
  UINT32 ReadCount = 0;
  RESULT result;
  DPRAM_BLOCK_HEADER header;

  if (m_InitStatus != DRV_OK)
  {
    result = m_InitStatus;          // PBit Failure
  }
  else if(DPRAM_RxOverflow)
  {
    //Overflow occured, flush RX FIFO, reset OV flag, and return an error.
    FIFO_Flush(&DPRAM_RxFIFO);
    m_RxBlockCnt = 0;
    DPRAM_RxOverflow = FALSE;
    result = DRV_DPRAM_ERR_RX_OVERFLOW;
  }
  else if(m_RxBlockCnt != 0)
  {
    //Read the block header.  
   FIFO_PopBlock(&DPRAM_RxFIFO, (INT8*)&header, sizeof(header));
        
    //Read the number of bytes.  If the number of bytes indicated by the
    //header is not available, something is wrong.
    ReadCount = header.Size;
    FIFO_PopBlock(&DPRAM_RxFIFO, Data, header.Size);
    result = DRV_OK;
    m_RxBlockCnt--;
  }
  else
  {
    // check for DPRAM Failure
    if (m_DPRAM_Status.intrFail)
    {
      result = DRV_DPRAM_INTR_ERROR;
    }
    else
    {
      //No data to read, count is set to zero, just return OK.
      result = DRV_OK;
    }
  }
  
  if(BytesRead != NULL)
  {
    *BytesRead = ReadCount;
  }
  
  return result;
}



/*****************************************************************************
 * Function:    DPRAM_ReceiveCnt
 *
 * Description: Returns the number of bytes in the receive buffer
 *
 * Parameters:  None
 *
 * Returns:     
 *
 * Notes:       CURRENTLY DEAD CODE, UNCOMMENT IF NEEDED
 *              
 ***************************************************************************
UINT32 DPRAM_ReadCnt()
{
  return DPRAM_RxFIFO.Cnt;
}
*/


/*****************************************************************************
 * Function:    DPRAM_GetTxInfo
 *
 * Description: Returns the number of messages transfered to the micro-server
 *              since power on and the semaphore status of the transmit
 *              buffer.
 *
 * Parameters:  [out] TxCnt*: Pointer to a location to store the number of
 *                             transmited messages count.
 *
 *              [out] MsgPending: Pointer to a location to store the semaphore
 *                                status of the DPRAM TX buffer.
 *                                TRUE indicates a message is in the DPRAM TX
 *                                buffer and the micro-server owns the transmit
 *                                buffer
 *                                FALSE indicates no message is in the DPRAM
 *                                TX buffer and the control processor owns the
 *                                TX buffer.
 *                                 
 *
 * Returns:     void
 *
 * Notes:       Function can be used to detect "stuck" DPRAM conditions, where
 *              a message is pending in the DPRAM queue and the TxCnt is not
 *              incrementing in a timely fashion.  This indicates the micro-
 *              server application is not servicing the DPRAM.
 *              
 ****************************************************************************/
void DPRAM_GetTxInfo(UINT32* TxCnt, BOOLEAN* MsgPending)
{
  *TxCnt      = TxMessageCnt;
  *MsgPending = (DPRAM_CPtoMS_Own == DPRAM_RX_OWNS) ? TRUE : FALSE;
}



/*****************************************************************************
 * Function:    DPRAM_GetRxInfo
 *
 * Description: Returns the number of messages received from the micro-server
 *              since power on and the semaphore status of the receive
 *              buffer.
 *
 * Parameters:  [out] RxCnt*:  Pointer to a location to store the number of
 *                             received messages count.
 *
 *              [out] MsgPending: Pointer to a location to store the semaphore
 *                                status of the DPRAM RX buffer.
 *                                TRUE indicates a message is in the DPRAM RX
 *                                buffer and the CP owns the receive
 *                                buffer
 *                                FALSE indicates no message is in the DPRAM
 *                                RX buffer and the micro server owns the
 *                                RX buffer.
 *                                 
 *
 * Returns:     void
 *
 * Notes:       CURRENTLY DEAD CODE, UNCOMMENT IF NEEDED
 *              
 ***************************************************************************
void DPRAM_GetRxInfo(UINT32* RxCnt, BOOLEAN* MsgPending)
{
  *RxCnt      = RxMessageCnt;
  *MsgPending = (DPRAM_MStoCP_Own == DPRAM_RX_OWNS) ? TRUE : FALSE;
}
*/


/*****************************************************************************
 * Function:    DPRAM_KickMS
 *
 * Description: Set the micro-server interrupt flag.  This is used to try
 *              to kick the dual-port RAM out of a deadlock state where the
 *              micro-server missed the previous interrupt.  The deadlock
 *              state can be detected when a message is pending in the
 *              dual-port ram, but the transmitted message count is not
 *              changing.  See GetTxInfo()
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:       
 *              
 ****************************************************************************/
void DPRAM_KickMS(void)
{
  DPRAM_CPtoMS_Int = DPRAM_INT_PATTERN;         //Trigger MS interrupt
}


/*****************************************************************************
 * Function:    DPRAM_EP5_ISR       EDGE PORT INTERRUPT
 *
 * Description: Handle edge port interrupts from the Dual-Port memory.  An
 *              interrupt indicates activity from the Micro-server, either
 *              new data is available in the MStoCP buffer or the microserver
 *              has completed its read of the CP buffer, and the CP buffer is
 *              free
 *
 * Parameters:  none
 *              
 * Returns:     none
 *
 * Notes:       TODO: Copying 8kB of data to/from the DPRAM may be a  
 *              dangerously long operation for interrupt context.  Need to 
 *              evaluate how this impacts the whole system.
 *              
 ****************************************************************************/
__interrupt void DPRAM_EP5_ISR(void)
{
  volatile UINT16 clrIntr;
  UINT32  entryTime;
  UINT32  intrTime;
  UINT32  runTime;
  BOOLEAN NotifyMS = FALSE;

  // Monitor the number of interrupts per specified
  // time period to make sure we don't get swamped
  entryTime = TTMR_GetHSTickCount() / TTMR_HS_TICKS_PER_uS;

  //Determine interrupt cause

  //If we own the MicroServer transmit buffer, it is because the MS
  //is giving us data.
  if(DPRAM_MStoCP_Own == DPRAM_RX_OWNS)
  {
    DPRAM_RxFromDPRAM();
    NotifyMS = TRUE;
  }

  //The MS also interrupts us when it is done reading our transmit buffer
  //Check if the transmit buffer is free. If we have data to send as much
  //as will fit in the buffer OR as much as we have
  if(DPRAM_CPtoMS_Own == DPRAM_TX_OWNS)
  {
    NotifyMS = DPRAM_TxToDPRAM();
  }

  clrIntr = DPRAM_MStoCP_Int;                     //Clear interrupt
  MCF_EPORT_EPFR = MCF_EPORT_EPFR_EPF5;           //Clear edge port interrupt

  if(NotifyMS == TRUE) 
  {
    //Done reading or transmitting.  Notify MS
    DPRAM_CPtoMS_Int = DPRAM_INT_PATTERN;
  }

  // Check if interrupts coming too fast
  intrTime = entryTime - m_DPRAM_Status.lastTime;
  if ( TPU( intrTime, eTpDprIsr3609) < DPRAM_MIN_INTERRUPT_TIME )
  {
    if (m_DPRAM_Status.minIntrTime > intrTime)
    {
      m_DPRAM_Status.minIntrTime = intrTime;    // save minimum interrupt time
    }
    m_DPRAM_Status.intrCnt++;
    // check interrupt count limit exceeded
    if (m_DPRAM_Status.intrCnt > DPRAM_MAX_INTERRUPT_CNT)
    {
      // yes - MSInterface will log error
      m_DPRAM_Status.intrFail = TRUE;
      // disable interrupts
      DPRAM_DISABLE_INT;
    }
  }
  else
  {
    m_DPRAM_Status.intrCnt = 0;    // reset fast interrupt counter
  }
  m_DPRAM_Status.lastTime = entryTime;

  // calc max run time
  runTime = (TTMR_GetHSTickCount() / TTMR_HS_TICKS_PER_uS) - entryTime;
  if ( runTime > m_DPRAM_Status.maxExeTime )
  {
    m_DPRAM_Status.maxExeTime = runTime;
  }
}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/*****************************************************************************
 * Function:    DPRAM_TxToDPRAM
 *
 * Description: Static utility function.
 *              Caller is responsible for checking ownership of the CPtoMS
 *              buffer before calling this routine
 *
 *              This routine copies data from the FIFO buffer into the DPRAM
 *              CPtoMS buffer space.  The number of bytes copied is the lesser
 *              of the number of bytes in the FIFO or the size of the
 *              CPtoMS buffer
 *
 * Parameters:  none
 *              
 * Returns:     TRUE: Data was written to DPRAM
 *              FALSE: No data was written because none was in the queue.
 * Notes:       
 *              
 ****************************************************************************/
/*__inline */
static BOOLEAN DPRAM_TxToDPRAM(void)
{
  INT32 size;
  INT32 intLvl;
  BOOLEAN retval = FALSE;
  static BOOLEAN active = FALSE;

  intLvl = __DIR();
  if ( !active)
  {
    active = TRUE;
    __RIR(intLvl);

    //When data is available in the local FIFO, copy a block of data
    //from the FIFO to the DPRAM.  Check number of writers is <= 1
    //to ensure a FIFO write is not being interrupted.
    if ((m_TxBlockCnt != 0) && (m_TxSem == 0))
    {
      //Get block size field
      FIFO_PopBlock(&DPRAM_TxFIFO, (INT8*)&size, sizeof(size));
      ASSERT( size <= DPRAM_TX_RX_SIZE);
      FIFO_PopBlock(&DPRAM_TxFIFO, DPRAM_CPtoMS_Buf, (UINT32) size);
      //Set count and owner flags
      DPRAM_CPtoMS_Cnt = (UINT16)size;               
      DPRAM_CPtoMS_Own = DPRAM_RX_OWNS;
      //inc total # of blocks txd since pwr on 
      TxMessageCnt++;                        
      m_TxBlockCnt--;
      retval = TRUE;
    }
    else
    {
      //No data to transmit
    }

    active = FALSE;
  }
  else
  {
    __RIR(intLvl);
  }
 
  return retval;
}



/*****************************************************************************
 * Function:    DPRAM_RxFromDPRAM
 *
 * Description: Static utility function.
 *              Caller is responsible for checking ownership of the MStoCP
 *              buffer before calling this routine
 *
 *              This routine copies available data from the MStoCP Dual-Port
 *              RAM to the local FIFO.  If the data cannot fit into the local
 *              FIFO, theRX overflow is flagged.  The flag must be cleared
 *              before additional data can be received.
 *
 * Parameters:  none
 *              
 * Returns:     none
 *
 * Notes:       
 *              
 ****************************************************************************/
/*__inline */
static void DPRAM_RxFromDPRAM(void)
{
  DPRAM_BLOCK_HEADER header;

  //Copy DPRAM data to the local buffer, flag an overflow error if it won't fit
  if(DPRAM_RxOverflow != TRUE)
  {
    //Add the header and received data block to the RX queue
    header.Size = DPRAM_MStoCP_Cnt;
    if(!FIFO_PushBlock(&DPRAM_RxFIFO, (INT8*)&header, sizeof(header)))
    {
      DPRAM_RxOverflow = TRUE;
    }
    if(!FIFO_PushBlock(&DPRAM_RxFIFO, DPRAM_MStoCP_Buf, DPRAM_MStoCP_Cnt))
    {
      DPRAM_RxOverflow = TRUE;
    }
    RxMessageCnt++;         //Number of messages received since power on
    m_RxBlockCnt++;         //Number of messages in the RxFIFO (Dec in _ReadBlock())
  }
  else
  {
    //Buffer has already overflown, discard data
  }

  DPRAM_MStoCP_Own = DPRAM_TX_OWNS;             //Return buffer to the MS
  DPRAM_MStoCP_Cnt = 0;                         
}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: DPRAM.c $
 * 
 * *****************  Version 40  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 39  *****************
 * User: Jim Mood     Date: 11/15/10   Time: 5:21p
 * Updated in $/software/control processor/code/drivers
 * SCR 999 Code coverage
 * 
 * *****************  Version 38  *****************
 * User: Jim Mood     Date: 11/10/10   Time: 1:52p
 * Updated in $/software/control processor/code/drivers
 * SCR 689 Interrupt changes
 * 
 * *****************  Version 37  *****************
 * User: Jim Mood     Date: 9/30/10    Time: 5:42p
 * Updated in $/software/control processor/code/drivers
 * SCR 904 Code Coverage changes
 * 
 * *****************  Version 36  *****************
 * User: Jim Mood     Date: 9/30/10    Time: 4:31p
 * Updated in $/software/control processor/code/drivers
 * SCR 902 Code Coverage Issues
 * 
 * *****************  Version 35  *****************
 * User: Contractor2  Date: 9/27/10    Time: 2:55p
 * Updated in $/software/control processor/code/drivers
 * SCR #795 Code Review Updates
 * 
 * *****************  Version 34  *****************
 * User: Jim Mood     Date: 8/31/10    Time: 10:40a
 * Updated in $/software/control processor/code/drivers
 * SCR 839 Move init of global data before interrupts enabled
 * 
 * *****************  Version 33  *****************
 * User: Jeff Vahue   Date: 8/29/10    Time: 3:02p
 * Updated in $/software/control processor/code/drivers
 * SCR# 830 - Coverage Enhancement
 * 
 * *****************  Version 32  *****************
 * User: Jim Mood     Date: 8/16/10    Time: 6:26p
 * Updated in $/software/control processor/code/drivers
 * SCR 796.  Interrupts enabled before FIFO init
 * 
 * *****************  Version 31  *****************
 * User: Jim Mood     Date: 8/12/10    Time: 11:39a
 * Updated in $/software/control processor/code/drivers
 * SCR 699: Code Review Changes (add asserts around PopBlock in
 * DPRAM_ReadBlock)
 * 
 * *****************  Version 30  *****************
 * User: Jeff Vahue   Date: 8/09/10    Time: 1:42p
 * Updated in $/software/control processor/code/drivers
 * SCR# 707 - V&V TP Update
 * 
 * *****************  Version 29  *****************
 * User: Contractor2  Date: 8/05/10    Time: 11:19a
 * Updated in $/software/control processor/code/drivers
 * SCR #243 Implementation: CBIT of mssim SRS-3625
 * 
 * *****************  Version 28  *****************
 * User: Contractor2  Date: 7/30/10    Time: 6:28p
 * Updated in $/software/control processor/code/drivers
 * SCR #243 Implementation: CBIT of mssim
 * Disable MS Interface if DPRAM PBit Failed
 * 
 * *****************  Version 27  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:10a
 * Updated in $/software/control processor/code/drivers
 * SCR #698 - Fix code review findings
 * 
 * *****************  Version 26  *****************
 * User: Jeff Vahue   Date: 7/21/10    Time: 7:48p
 * Updated in $/software/control processor/code/drivers
 * SCR #689 - account for two interrupts per CP command form the MS
 * 
 * *****************  Version 25  *****************
 * User: Jeff Vahue   Date: 7/19/10    Time: 6:32p
 * Updated in $/software/control processor/code/drivers
 * SCR# 707 - Code Coverage TP
 * 
 * *****************  Version 24  *****************
 * User: Jeff Vahue   Date: 7/17/10    Time: 5:46p
 * Updated in $/software/control processor/code/drivers
 * SCR# 707 - Add TestPoints for code coverage.
 * 
 * *****************  Version 23  *****************
 * User: Jeff Vahue   Date: 7/09/10    Time: 7:11p
 * Updated in $/software/control processor/code/drivers
 * SCR# 688 - DPRAM interupt monitor fix for CP commanded interrupts
 * 
 * *****************  Version 22  *****************
 * User: Contractor2  Date: 7/09/10    Time: 4:31p
 * Updated in $/software/control processor/code/drivers
 * SCR #8 Implementation: External Interrupt Monitors
 * 
 * *****************  Version 21  *****************
 * User: Contractor3  Date: 5/27/10    Time: 10:56a
 * Updated in $/software/control processor/code/drivers
 * SCR #617 - Changes made to meet coding standards as a result of code
 * review.
 * 
 * *****************  Version 20  *****************
 * User: Jim Mood     Date: 5/18/10    Time: 6:24p
 * Updated in $/software/control processor/code/drivers
 * SCR 594.  YMODEM/FIFO preemption issue
 * 
 * *****************  Version 19  *****************
 * User: Jeff Vahue   Date: 3/24/10    Time: 4:09p
 * Updated in $/software/control processor/code/drivers
 * SCR# 504 - fix big file uploads.  Ensure single reader of DPRAM_TxFIFO
 * 
 * *****************  Version 18  *****************
 * User: Jim Mood     Date: 3/12/10    Time: 11:08a
 * Updated in $/software/control processor/code/drivers
 * SCR #465 and #466 modfix for reentrancy of WriteBlock and size of dpram
 * TX buffer
 * 
 * *****************  Version 17  *****************
 * User: Peter Lee    Date: 10/29/09   Time: 2:39p
 * Updated in $/software/control processor/code/drivers
 * SCR #315 CBIT Reg and SEU Processing 
 * 
 * *****************  Version 16  *****************
 * User: Jim Mood     Date: 10/08/09   Time: 6:13p
 * Updated in $/software/control processor/code/drivers
 * SCR #283
 * 
 * *****************  Version 15  *****************
 * User: Peter Lee    Date: 9/29/09    Time: 1:57p
 * Updated in $/software/control processor/code/drivers
 * SCR #280 Fix misc compiler warnings
 * 
 * *****************  Version 14  *****************
 * User: Peter Lee    Date: 9/15/09    Time: 6:04p
 * Updated in $/software/control processor/code/drivers
 * SCR #94, #95 PBIT returns log structure to Init Mgr
 * 
 * *****************  Version 13  *****************
 * User: Peter Lee    Date: 9/14/09    Time: 3:24p
 * Updated in $/software/control processor/code/drivers
 * SCR #94 Updated Init for SET_CHECK() and return ERR CODE Struct
 * 
 * *****************  Version 12  *****************
 * User: Jim Mood     Date: 4/02/09    Time: 9:18a
 * Updated in $/control processor/code/drivers
 * Just noticed while building 2.0.10, a missing return at the end of
 * TxToDPRAM function.
 * 
 * *****************  Version 11  *****************
 * User: Jim Mood     Date: 4/02/09    Time: 8:58a
 * Updated in $/control processor/code/drivers
 * Changed order of interrupt clearing (clear DPRAM register last) 
 * 
 * *****************  Version 10  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 11:44a
 * Updated in $/control processor/code/drivers
 * SCR #87 Function Prototype
 * 
 *
 ***************************************************************************/
