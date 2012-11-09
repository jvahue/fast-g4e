#define ARINC429_BODY
/******************************************************************************
            Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:         Arinc429.c

    Description:  Contains all functions and data related to the Arinc429.
    
VERSION
     $Revision: 106 $  $Date: 12-11-09 1:20p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "arinc429.h"
#include "assert.h"
#include "mcf548x.h"
#include "ttmr.h"
#include "resultcodes.h"
#include "Dio.h"

#include "gse.h"

#include "TestPoints.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#ifndef WIN32
   #define PAUSE(x) StartTime = TTMR_GetHSTickCount();         \
           while((TTMR_GetHSTickCount() - StartTime) < (x))
#else
   #define PAUSE
#endif

// Uncomment define to enable ARINC429 register output for testing
//#define A429_TEST_OUTPUT

#define TPX( v, tp, tt) ((tt != FIFO_EMPTY_TEST) ? STPU( v, tp) : (v))

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
typedef enum 
{
   FIFO_EMPTY_TEST, 
   FIFO_HALF_FULL_TEST, 
   FIFO_FULL_TEST,
   FIFO_OVERRUN_TEST, 
   FIFO_MAX_TEST
} ARINC429_FIFO_TEST_TYPE; 

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
UINT16 m_FPGA_StatusReg;

ARINC429_DRV_STATUS m_Arinc429HW;

static BOOLEAN fpgaLogged;  // TRUE if fpga registers have been logged on PBIT fail

#ifdef A429_TEST_OUTPUT
static char GSE_OutLine[512];
#endif


/*****************************************************************************/
/* Local Constants                                                           */
/*****************************************************************************/
#ifdef SUPPORT_FPGA_BELOW_V12
const FPGA_TX_REG fpgaTxReg_v9[FPGA_MAX_TX_CHAN] = 
{
   // Tx Ch 0
   { FPGA_429_TX1_STATUS,       // *Status
     FPGA_429_TX1_FIFO_STATUS,  // *FIFO_Status
     FPGA_429_TX1_CONFIG,       // *Configuration
     FPGA_429_TX1_LSW_TEST,     // *lsw_test
     FPGA_429_TX1_MSW_TEST,     // *msw_test
     FPGA_429_TX1_LSW,          // *lsw
     FPGA_429_TX1_MSW,          // *msw
     6,                         // Bit Shift 6 for ISR_429_TX1
     NULL,                      // not used 
     IMR1_429_TX1_HALF_FULL_v9, // imr_halfful_bit
     IMR1_429_TX1_EMTPY_v9      // imr_emtpy_bit
   },
   // Tx Ch 1
   { FPGA_429_TX2_STATUS,       // *Status
     FPGA_429_TX2_FIFO_STATUS,  // *FIFO_Status
     FPGA_429_TX2_CONFIG,       // *Configuration
     FPGA_429_TX2_LSW_TEST,     // *lsw_test
     FPGA_429_TX2_MSW_TEST,     // *msw_test
     FPGA_429_TX2_LSW,          // *lsw
     FPGA_429_TX2_MSW,          // *msw
     4,                         // Bit Shift 4 for ISR_429_TX2
     NULL,                      // not used
     IMR1_429_TX2_HALF_FULL_v9, // imr_halfful_bit
     IMR1_429_TX2_EMTPY_v9      // imr_emtpy_bit
   }
};
#endif

const FPGA_TX_REG fpgaTxReg_v12[FPGA_MAX_TX_CHAN] = 
{
   // Tx Ch 0
   { FPGA_429_TX1_STATUS,       // *Status
     FPGA_429_TX1_FIFO_STATUS,  // *FIFO_Status
     FPGA_429_TX1_CONFIG,       // *Configuration
     FPGA_429_TX1_LSW_TEST,     // *lsw_test
     FPGA_429_TX1_MSW_TEST,     // *msw_test
     FPGA_429_TX1_LSW,          // *lsw
     FPGA_429_TX1_MSW,          // *msw
     6,                         // Bit Shift 6 for ISR_429_TX1
     IMR1_429_TX1_FULL_v12,     // imr_full_bit
     IMR1_429_TX1_HALF_FULL_v12,// imr_halfful_bit
     IMR1_429_TX1_EMPTY_v12     // imr_emtpy_bit
   },
   // Tx Ch 1
   { FPGA_429_TX2_STATUS,       // *Status
     FPGA_429_TX2_FIFO_STATUS,  // *FIFO_Status
     FPGA_429_TX2_CONFIG,       // *Configuration
     FPGA_429_TX2_LSW_TEST,     // *lsw_test
     FPGA_429_TX2_MSW_TEST,     // *msw_test
     FPGA_429_TX2_LSW,          // *lsw
     FPGA_429_TX2_MSW,          // *msw
     4,                         // Bit Shift 4 for ISR_429_TX2
     IMR2_429_TX2_FULL_v12,     // imr_full_bit
     IMR2_429_TX2_HALF_FULL_v12,// imr_halfful_bit
     IMR2_429_TX2_EMPTY_v12     // imr_emtpy_bit
   }
};

const FPGA_RX_REG fpgaRxReg[FPGA_MAX_RX_CHAN] = 
{
   // Rx Ch 0 
   { FPGA_429_RX1_STATUS,       // *Status
     FPGA_429_RX1_FIFO_STATUS,  // *FIFO_Status
     FPGA_429_RX1_LSW,          // *lsw
     FPGA_429_RX1_MSW,          // *msw
     FPGA_429_RX1_CONFIG,       // *Configuration
     FPGA_429_RX1_LSW_TEST,     // *lsw_test
     FPGA_429_RX1_MSW_TEST,     // *msw_test
     14,                        // Bit Shift 14 for ISR_429_RX1
     IMR1_429_RX1_FULL,         // imr_full_bit
     IMR1_429_RX1_HALF_FULL,    // imr_halffull_bit
     IMR1_429_RX1_NOT_EMPTY,    // imr_not_empty_bit
     FPGA_429_RX1_FILTER,       // *lfr
   }, 
   // Rx Ch 1
   { FPGA_429_RX2_STATUS,       // *Status
     FPGA_429_RX2_FIFO_STATUS,  // *FIFO_Status
     FPGA_429_RX2_LSW,          // *lsw
     FPGA_429_RX2_MSW,          // *msw
     FPGA_429_RX2_CONFIG,       // *Configuration 
     FPGA_429_RX2_LSW_TEST,     // *lsw_test
     FPGA_429_RX2_MSW_TEST,     // *msw_test
     12,                        // Bit Shit 12 for ISR_429_RX2
     IMR1_429_RX2_FULL,         // imr_full_bit
     IMR1_429_RX2_HALF_FULL,    // imr_halffull_bit
     IMR1_429_RX2_NOT_EMPTY,    // imr_not_empty_bit
     FPGA_429_RX2_FILTER,       // *lfr
   }, 
   // Rx Ch 2 
   { FPGA_429_RX3_STATUS,       // *Status
     FPGA_429_RX3_FIFO_STATUS,  // *FIFO_Status
     FPGA_429_RX3_LSW,          // *lsw
     FPGA_429_RX3_MSW,          // *msw
     FPGA_429_RX3_CONFIG,       // *Configuration
     FPGA_429_RX3_LSW_TEST,     // *lsw_test
     FPGA_429_RX3_MSW_TEST,     // *msw_test
     10,                        // Bit Shift 10 for ISR_429_RX3
     IMR1_429_RX3_FULL,         // imr_full_bit
     IMR1_429_RX3_HALF_FULL,    // imr_halffull_bit
     IMR1_429_RX3_NOT_EMPTY,    // imr_not_empty_bit
     FPGA_429_RX3_FILTER,       // *lfr
   },
   // Rx Ch 3
   { FPGA_429_RX4_STATUS,       // *Status
     FPGA_429_RX4_FIFO_STATUS,  // *FIFO_Status
     FPGA_429_RX4_LSW,          // *lsw
     FPGA_429_RX4_MSW,          // *msw
     FPGA_429_RX4_CONFIG,       // *Configuration
     FPGA_429_RX4_LSW_TEST,     // *lsw_test
     FPGA_429_RX4_MSW_TEST,     // *msw_test
     8,                         // Bit Shift 8 for ISR_429_RX4
     IMR1_429_RX4_FULL,         // imr_full_bit
     IMR1_429_RX4_HALF_FULL,    // imr_halffull_bit
     IMR1_429_RX4_NOT_EMPTY,    // imr_not_empty_bit
     FPGA_429_RX4_FILTER,       // *lfr
   }
};

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static RESULT Arinc429DrvTest_FIFO     ( UINT8 *pdest, UINT16 *psize );

static RESULT Arinc429DrvTest_Rx_FIFO  ( ARINC429_CHAN_ENUM ch, 
                                         ARINC429_FIFO_TEST_TYPE testType,
                                         ARINC_DRV_PBIT_FAIL_DATA *failData ); 

static RESULT Arinc429DrvTest_Tx_FIFO  ( FPGA_TX_REG_PTR pfpgaTxReg, ARINC429_CHAN_ENUM ch, 
                                         ARINC429_FIFO_TEST_TYPE testType,
                                         ARINC_DRV_PBIT_FAIL_DATA *failData ); 

static RESULT Arinc429DrvTest_LFR      ( UINT8 *pdest, UINT16 *psize ); 

static RESULT Arinc429DrvTest_LoopBack ( UINT8 *pdest, UINT16 *psize ); 

static void   Arinc429DrvRestoreStartupCfg ( void ); 

static void   Arinc429DrvLogFpgaRegs (ARINC_DRV_PBIT_FAIL_DATA *pDest);

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:    Arinc429DrvInitialize
 *
 * Description: 1) Initializes internal local variable
 *              2) Performs PBIT of Arinc429 
 *                     - FPGA PBIT status is OK 
 *                     - Calls Arinc429_Test() for FIFO Test
 *                     - Calls Arinc429_
 *              3) Sets FPGA Shadow RAM to Arinc429 startup default
 *
 * Parameters:  SysLogId - ptr to SYS_APP_ID
 *              pdata    - ptr to data store to return PBIT fail data 
 *              psize    - ptr to size var to return the size of PBIT fail data
 *
 * Returns:     DRV_RESULT (DRV_OK) if PBIT successful
 *              DRV_A429_FPGA_BAD_STATE if FPGA indicates PBIT failure 
 *              DRV_A429_RX_FIFO_TEST_FAIL if Arinc429 PBIT test fails  
 *              DRV_A429_LFR_TEST_FAIL if Arinc429 Label Filter RAM test fails
 *              
 * Notes: 
 *  1) FPGA Driver must be initialized before call to Arinc429_Initialize()
 *  2) Note, no Result Data returned when !DRV_OK, only DRV_RESULT code returned
 *
 *****************************************************************************/
RESULT Arinc429DrvInitialize (SYS_APP_ID *SysLogId, void *pdata, UINT16 *psize)
{
   // Local Data
   RESULT result;
   RESULT result_fifo; 
   RESULT result_loopback; 
   UINT8 *pdest; 

   // Initialize Local Data
   result_loopback = DRV_OK;
   pdest           = (UINT8 *) pdata; 
   result          = DRV_OK;
   *psize          = 0;       // No Result Data returned by Arinc429 PBIT
   fpgaLogged      = FALSE;
  
    memset ( (void *) pdest, 0, sizeof(ARINC_DRV_PBIT_TEST_RESULT) );  // Init clear pdest 
    memset ( &m_Arinc429HW, 0, sizeof(ARINC429_DRV_STATUS) );
  
    // Set Default Arinc 429 Startup Configuration   
    Arinc429DrvRestoreStartupCfg();  

   // PBIT to determine if FPGA PBIT is OK
   if ( FPGA_GetStatus()->InitResult != DRV_OK ) 
   {
      result = DRV_A429_FPGA_BAD_STATE; 
      *SysLogId = DRV_ID_A429_PBIT_FPGA_FAIL; 
  
      // Return Arinc429 Result code DRV_A429_FPGA_BAD_STATE
      memcpy ( pdest, &result, sizeof(RESULT)); 
      *psize = sizeof(RESULT); 
   }
  
   if (result == DRV_OK) 
   {
     // Read Status to clear any residual receive errors 
     UINT32 i;
     for (i = 0; i < FPGA_MAX_RX_CHAN; i++)
     {
       UINT32 parityErrCnt = 0;
       UINT32 framingErrCnt = 0;
       UINT32 intCount = 0;
       UINT16 FPGA_Status = 0;

       // dummy read to clear errors
       Arinc429DrvCheckBITStatus ( &parityErrCnt, &framingErrCnt, &intCount, &FPGA_Status, i );
       FPGA_Status = FPGA_R(fpgaRxReg[i].FIFO_Status);
     }

      // PBIT tests Arinc429_Test_LFR()
      result = Arinc429DrvTest_LFR( pdest, psize ); 
    
      // PBIT tests Arinc429_Test_FIFO()
      result_fifo = Arinc429DrvTest_FIFO( pdest, psize ); 
    
      // PBIT tests Arinc429_Test_LoopBack()
#ifdef SUPPORT_FPGA_BELOW_V12
/*vcast_dont_instrument_start*/
      if ( FPGA_GetStatus()->Version >= FPGA_VER_12 ) 
      {
         result_loopback = Arinc429DrvTest_LoopBack ( pdest, psize ); 
      }
/*vcast_dont_instrument_end*/
#else
      result_loopback = Arinc429DrvTest_LoopBack ( pdest, psize ); 
#endif       
    
      // SCR #383, Only return one DRV Result code.  Priority is _LFR, _FIFO then _LoopBack. 
      if ( result == DRV_OK )
      {
         if ( result_fifo == DRV_OK ) 
         {
            result = result_loopback;  
         }
         else 
         {
            result = result_fifo; 
         }
      }
    
      // Note: If ->result == DRV_OK, then this value is ignored. 
      *SysLogId = DRV_ID_A429_PBIT_TEST_FAIL;  
    
      // NOTE: if _Test_LFR and _Test_FIFO both fails then result = "OR" of the two result 
      //       codes DRV_A429_RX_LFR_FIFO_TEST_FAIL, as of SCR #383 this is not the case 
      //       anymore. 
    
      // NOTE: m_Arinc429MgrBlock.status.Rx[i].Status set to ARINC429_STATUS_DISABLED
      //       on Arinc429_Test() failure for each Rx Ch 
    
      // NOTE: pdest updated with RESULT data on test failure
      //       psize updated with RSULT data size on test failure
    
  }
 
  return (result);
}

/******************************************************************************
 * Function: Arinc429DrvProcessISR_Rx
 *
 * Description: Process Arinc429 Specific Rx Interrupt  
 *
 * Parameters: IntStatus - Current *FPGA_ISR reading 
 *
 * Returns: none 
 *
 * Notes:
 *  - Rx INT provided are FIFO Half Full and FIFO FULL
 *  - NOTE: Called from FPGA ISR.  Processing time must be optimized.
 *          FPGA only ACK int ISR when this func completes and returns
 *          processing to FPGA.  Not, any new FPGA int could be "lost",
 *          esp the longer the ISR processing takes.
 *
 *****************************************************************************/
void Arinc429DrvProcessISR_Rx( UINT16 IntStatus )
{
   // Local Data
   FPGA_RX_REG_PTR         pfpgaRxReg;
   ARINC429_DRV_RX_STATUS *pArincRxStatus;
   UINT16                  IntRxStatus;
   UINT32                  Channel;

   // Process all Rx Ch interrupts
   for ( Channel = 0; Channel < FPGA_MAX_RX_CHAN; Channel++)
   {
      pfpgaRxReg     = (FPGA_RX_REG_PTR) &fpgaRxReg[Channel];
      pArincRxStatus = &m_Arinc429HW.Rx[Channel];
      IntRxStatus    = (IntStatus >> pfpgaRxReg->isr_mask) & ISR_429_RX_TX_INT_BITS;
      
      switch (IntRxStatus)
      {
         case ISR_429_RX_NO_IRQ_VAL:
            // nothing to do here
            break;
         case ISR_429_RX_HALF_FULL_VAL:
            pArincRxStatus->FIFO_HalfFullCnt++;
            break;
         case ISR_429_RX_FULL_VAL:
            pArincRxStatus->FIFO_FullCnt++;
            break;
         case ISR_429_RX_NOT_EMPTY_VAL:
            pArincRxStatus->FIFO_NotEmptyCnt++;
            break;
      
         default:
            // Unrecognized/Unused statuses are FATAL Asserts.
            FATAL("Unrecognized IntRxStatus = %d", IntRxStatus);         
            break;
      } // End of switch (IntRxStatus)
  } // End of for i Loop thru FPGA_MAX_RX_CHAN

  m_Arinc429HW.InterruptCnt++;

}

/******************************************************************************
 * Function: Arinc429DrvProcessISR_Tx
 *
 * Description: Process Arinc429 Specific Tx Interrupt 
 *
 * Parameters: IntStatus - Current *FPGA_ISR reading 
 *
 * Returns: none
 *
 * Notes:
 *  - Tx INT TBD
 *
 ******************************************************************************/
void Arinc429DrvProcessISR_Tx( UINT16 IntStatus )
{
   // Local Data
   FPGA_TX_REG_PTR        pfpgaTxReg;
   ARINC429_DRV_TX_STATUS_PTR pArincTxStatus;
   UINT16                 IntTxStatus;
   UINT32                 Channel;

   // Process all Tx Ch interrupts
   for ( Channel = 0 ; Channel < FPGA_MAX_TX_CHAN; Channel++)
   {
#ifdef SUPPORT_FPGA_BELOW_V12
/*vcast_dont_instrument_start*/
      if ( FPGA_GetStatus()->Version >= FPGA_VER_12 )
      {
         pfpgaTxReg = (FPGA_TX_REG_PTR) &fpgaTxReg_v12[Channel];
      }
      else
      {
         pfpgaTxReg = (FPGA_TX_REG_PTR) &fpgaTxReg_v9[Channel];
      }
/*vcast_dont_instrument_end*/      
#else
      pfpgaTxReg = (FPGA_TX_REG_PTR) &fpgaTxReg_v12[Channel];
#endif      
    
      pArincTxStatus = (ARINC429_DRV_TX_STATUS_PTR) &m_Arinc429HW.Tx[Channel];
      IntTxStatus = (IntStatus >> pfpgaTxReg->isr_mask) & ISR_429_RX_TX_INT_BITS;
      
      switch (IntTxStatus)
      {
         case ISR_429_TX_NO_IRQ_VAL:
            // nothing to do here
            break;
         case ISR_429_TX_HALF_FULL_VAL:
            pArincTxStatus->FIFO_HalfFullCnt++;
            break;
         case ISR_429_TX_FULL_VAL: 
            pArincTxStatus->FIFO_FullCnt++;
            break;
         case ISR_429_TX_EMPTY_VAL: 
            pArincTxStatus->FIFO_EmptyCnt++; 
            break; 
         default:
            FATAL("Unrecognized IntTxStatus = %d", IntTxStatus);         
            break;
      } // End of switch (IntRxStatus)
   } // End of for i Loop thru FPGA_MAX_TX_CHAN
}

/******************************************************************************
 * Function:    Arinc429DrvFlushHWFIFO
 *
 * Description: Flushes all of the Hardware ARINC429 FIFOs.
 *
 * Parameters: None
 *
 * Returns: none 
 *
 * Notes: None
 *
 *
 *****************************************************************************/
void Arinc429DrvFlushHWFIFO (void)
{
   // Local Data
   UINT32 Arinc429Msg;
   UINT32 LoopLimit;
   UINT32 Channel;
   
   // Loop through all the channels 
   for ( Channel = 0; Channel < ARINC_RX_CHAN_MAX; Channel++ )
   {
      // Loop 
      for ( LoopLimit = 0; LoopLimit < FPGA_MAX_RX_FIFO_SIZE; LoopLimit++ )
      {
         if (FALSE == Arinc429DrvRead( &Arinc429Msg, (ARINC429_CHAN_ENUM)Channel ))
         {
            break;
         }
      }
   }
}


/******************************************************************************
 * Function:    Arinc429DrvRead
 *
 * Description: Reads the HW Arinc Rx FIFO for any Rx data.
 *
 * Parameters:  pArinc429Msg - Pointer to buffer to return Raw Arinc429 data
 *              Channel - Arinc429 channnel to read
 *
 * Returns:     TRUE if new data is returned 
 *              FALSE if no new data is returned 
 *
 * Notes: none
 *
 *
 *****************************************************************************/
BOOLEAN Arinc429DrvRead ( UINT32 *pArinc429Msg, ARINC429_CHAN_ENUM Channel )
{
   // Local Data
   BOOLEAN bNewMsg;
   
   bNewMsg = FALSE;
   
   // Check if the FIFO has any messages
   if (!(FPGA_R(fpgaRxReg[Channel].FIFO_Status) & RFSR_429_FIFO_EMPTY))
   {
      // Message was received
      bNewMsg = TRUE;
      
      // Store the new message
      *pArinc429Msg = FPGA_R( fpgaRxReg[Channel].lsw);
      *pArinc429Msg = (FPGA_R(fpgaRxReg[Channel].msw) << 16) | *pArinc429Msg;
   }
   
   return bNewMsg;
}

/******************************************************************************
 * Function:    Arinc429DrvStatus_OK
 *
 * Description: Returns the current Arinc429 channel status
 *
 * Parameters:  Type - Tx or Rx 
 *              Channel - Arinc429 channel to query 
 *
 * Returns:     TRUE if Arinc429 status is Ok
 *              FALSE if Arinc429 status has faulted 
 *
 * Notes: none
 *
 *
 *****************************************************************************/
BOOLEAN Arinc429DrvStatus_OK ( ARINC429_CHAN_TYPE Type, UINT8 Channel)
{
   // Local Data
   BOOLEAN bOK;
     
   if (ARINC429_XMIT == Type)
   {
      bOK = ( (m_Arinc429HW.Tx[Channel].Status == ARINC429_DRV_STATUS_OK) &&
              (FPGA_GetStatus()->SystemStatus  == FPGA_STATUS_OK) );
   }
   else
   {
      bOK = ( (m_Arinc429HW.Rx[Channel].Status == ARINC429_DRV_STATUS_OK) &&
              (FPGA_GetStatus()->SystemStatus  == FPGA_STATUS_OK) );
   }

   return (bOK);
}

/******************************************************************************
 * Function:     Arinc429DrvReconfigure
 *
 * Description:  Calls _ReconfigureFPGAShadowRam to reconfigure the FPGA Shadow
 *               RAM from the m_Arinc429Cfg[].
 *               Calls FPGA_Configure() to update the FPGA HW Registers 
 *
 * Parameters:   none
 *
 * Returns:      none 
 *
 * Notes:        none
 *
 *****************************************************************************/
void Arinc429DrvReconfigure ( void )
{
  FPGA_Configure();
}

/******************************************************************************
 * Function:     Arinc429DrvSetIMR1_2
 *
 * Description:  Initializes the IMR1 and IMR2 register 
 *
 * Parameters:   none
 *
 * Returns:      none
 *
 * Notes:        none
 *
 *****************************************************************************/
void Arinc429DrvSetIMR1_2 ( void )
{
   // Local Data
   UINT16 idata;
 
   // Initialize Arinc 429 part of FPGA thru FPGA Shadow RAM
   // Disable all Rx Interrupts except for FIFO Full for now

   // Initialize Interrupts for RX 1
   idata = (IMR1_429_RX1_FULL      * IMR1_UNMAK_IRQ_VAL)  |
           (IMR1_429_RX1_HALF_FULL * IMR1_UNMAK_IRQ_VAL)  |
           (IMR1_429_RX1_NOT_EMPTY * IMR1_MASK_IRQ_VAL);
   // Initialize Interrupts for RX 2
   idata |= (IMR1_429_RX2_FULL      * IMR1_UNMAK_IRQ_VAL) |
            (IMR1_429_RX2_HALF_FULL * IMR1_UNMAK_IRQ_VAL) |
            (IMR1_429_RX2_NOT_EMPTY * IMR1_MASK_IRQ_VAL);

   // Initialize Interrupts for RX 3
   idata |= (IMR1_429_RX3_FULL      * IMR1_UNMAK_IRQ_VAL) |
            (IMR1_429_RX3_HALF_FULL * IMR1_UNMAK_IRQ_VAL) |
            (IMR1_429_RX3_NOT_EMPTY * IMR1_MASK_IRQ_VAL);

   // Initialize Interrupts for RX 4
   idata |= (IMR1_429_RX4_FULL      * IMR1_UNMAK_IRQ_VAL) |
            (IMR1_429_RX4_HALF_FULL * IMR1_UNMAK_IRQ_VAL) |
            (IMR1_429_RX4_NOT_EMPTY * IMR1_MASK_IRQ_VAL);

#ifdef SUPPORT_FPGA_BELOW_V12
/*vcast_dont_instrument_start*/
   if ( FPGA_GetStatus()->Version >= FPGA_VER_12) 
   {
      // Initialize Interrupts for TX 1
      idata |= (IMR1_429_TX1_FULL_v12      * IMR1_MASK_IRQ_VAL) |
               (IMR1_429_TX1_HALF_FULL_v12 * IMR1_MASK_IRQ_VAL) |
               (IMR1_429_TX1_EMPTY_v12     * IMR1_MASK_IRQ_VAL);
               
      FPGA_GetShadowRam()->IMR1 = idata;
  
      // Initialize Interrupts for TX 2
      idata = FPGA_GetShadowRam()->IMR2 & 
              (~(IMR2_429_TX2_FULL_v12 | IMR2_429_TX2_HALF_FULL_v12 | 
                 IMR2_429_TX2_EMPTY_v12 )); 
      idata  |= (IMR2_429_TX2_FULL_v12      * IMR1_MASK_IRQ_VAL)  |
                (IMR2_429_TX2_HALF_FULL_v12 * IMR1_MASK_IRQ_VAL)  |
                (IMR2_429_TX2_EMPTY_v12     * IMR1_MASK_IRQ_VAL);
               
      FPGA_GetShadowRam()->IMR2 = idata; 
   }
   else 
   {
      // Initialize Interrupts for TX 1
      idata |= (IMR1_429_TX1_HALF_FULL_v9 * IMR1_MASK_IRQ_VAL)  |
               (IMR1_429_TX1_EMTPY_v9 * IMR1_MASK_IRQ_VAL);
  
      // Initialize Interrupts for TX 2
      idata |= (IMR1_429_TX2_HALF_FULL_v9 * IMR1_MASK_IRQ_VAL)  |
               (IMR1_429_TX2_EMTPY_v9 * IMR1_MASK_IRQ_VAL);
               
      FPGA_GetShadowRam()->IMR1 = idata;
   }
/*vcast_dont_instrument_end*/   
#else
    // Initialize Interrupts for TX 1
    idata |= (IMR1_429_TX1_FULL_v12      * IMR1_MASK_IRQ_VAL) |
             (IMR1_429_TX1_HALF_FULL_v12 * IMR1_MASK_IRQ_VAL) |
             (IMR1_429_TX1_EMPTY_v12     * IMR1_MASK_IRQ_VAL);
             
    FPGA_GetShadowRam()->IMR1 = idata;

    // Initialize Interrupts for TX 2
    idata = FPGA_GetShadowRam()->IMR2 & 
            (~(IMR2_429_TX2_FULL_v12 | IMR2_429_TX2_HALF_FULL_v12 | 
               IMR2_429_TX2_EMPTY_v12 )); 
    idata  |= (IMR2_429_TX2_FULL_v12      * IMR1_MASK_IRQ_VAL)  |
              (IMR2_429_TX2_HALF_FULL_v12 * IMR1_MASK_IRQ_VAL)  |
              (IMR2_429_TX2_EMPTY_v12     * IMR1_MASK_IRQ_VAL);
             
    FPGA_GetShadowRam()->IMR2 = idata; 
#endif   

     // Set LBR for all chan 
   // Tx1, Tx2 = Quiet
   // Rx1, Rx2, Rx3, Rx4 = Normal 
   FPGA_GetShadowRam()->LBR = (LBR_429_TX1_DRIVE  * LBR_429_TX_QUIET_VAL)  |
                              (LBR_429_TX2_DRIVE  * LBR_429_TX_QUIET_VAL)  |
                              (LBR_429_RX1_SOURCE * LBR_429_RX_NORMAL_VAL) |
                              (LBR_429_RX2_SOURCE * LBR_429_RX_NORMAL_VAL) |
                              (LBR_429_RX3_SOURCE * LBR_429_RX_NORMAL_VAL) |
                              (LBR_429_RX4_SOURCE * LBR_429_RX_NORMAL_VAL); 
  
   // Single Read of FPGA Status Reg
   m_FPGA_StatusReg = *FPGA_ISR;

   // Initial Read of FPGA Rx and Tx Status Reg to clear any latched bits
   idata = *FPGA_429_RX1_STATUS;
   idata = *FPGA_429_RX2_STATUS;
   idata = *FPGA_429_RX3_STATUS;
   idata = *FPGA_429_RX4_STATUS;
   idata = *FPGA_429_TX1_STATUS;
   idata = *FPGA_429_TX2_STATUS;
}

/******************************************************************************
 * Function:     Arinc429DrvCfgLFR
 *
 * Description:  Configures the HW FPGA LFR registers 
 *
 * Parameters:   SdiMask - SDI mask to set FPGA LFR
 *               Channel - Arinc429 channel to set
 *               Label - Arinc429 label to filter / accept 
 *               bNow - directly update HW FPGA 
 *
 * Returns:      none
 *
 * Notes: // Needs to be called for each channel and all labels
 *
 *****************************************************************************/
void Arinc429DrvCfgLFR ( UINT8 SdiMask, UINT8 Channel, UINT8 Label, BOOLEAN bNow )
{
   // Local Data
   UINT32                          j;
   FPGA_SHADOW_ARINC429_RX_CFG_PTR pShadowRxCfg;
   FPGA_RX_REG_PTR                 pfpgaRxReg;
   UINT16                          *pLFR;
   
   // Update FPGA Shadow RAM
   pShadowRxCfg             = &FPGA_GetShadowRam()->Rx[Channel];
   pShadowRxCfg->LFR[Label] = SdiMask;
      
   if (TRUE == bNow)
   {
      // Setup Pointer to FPGA HW LFR 
      pfpgaRxReg = (FPGA_RX_REG_PTR)&fpgaRxReg[Channel];
      pLFR       = (UINT16 *)pfpgaRxReg->lfr; 
      // Must disable Rx Ch before updating to LFR is possible 
      FPGA_W(pfpgaRxReg->Configuration,(FPGA_R(pfpgaRxReg->Configuration) & ~RCR_429_ENABLE) |
                                                       (CR_429_DISABLE_VAL * RCR_429_ENABLE)); 
      // Update FPGA HW LFR   
      FPGA_W(pLFR + Label, pShadowRxCfg->LFR[Label]); 

      // Restore Rx Ch Enb State from current FPGA Shadow RAM 
      FPGA_W(pfpgaRxReg->Configuration, pShadowRxCfg->CREG);
  
      // Re-enabling Rx Ch, cause the FPGA "Data Present" to be set (sometimes
      //   but not all the time !).  
      //   Call read Rx Status Reg to clear this bit. 
      //   Note: Ensure compiler does not optimize this out ! 
      // Provide some delay to allow the "Data Present" Indication to be propagated
      j = TTMR_GetHSTickCount();
      while ( (TTMR_GetHSTickCount() - j) < TICKS_PER_10msec ); 
      j = FPGA_R(pfpgaRxReg->Status); 
   }
}

/******************************************************************************
 * Function:     Arinc429DrvCfgRxChan
 *
 * Description:  Configures the Arinc429 Rx channel thru the FPGA shadow RAM.
 *               Does not directly write to FPGA Register. 
 *
 * Parameters:   Speed - HIGH or LOW speed ARINC
 *               Parity - EVEN or ODD Parity 
 *               SwapBits - Swap or Not Swap the 8 bit label bits
 *               Enable - Enable or disable the channel for processing 
 *               Channel - Arinc429 channel to cfg
 *
 * Returns:      none
 *
 * Notes: // Needs to be called for each RX channel
 *
 *****************************************************************************/
void Arinc429DrvCfgRxChan ( ARINC429_SPEED Speed, ARINC429_PARITY Parity,
                            ARINC429_RX_SWAP SwapBits, BOOLEAN Enable, UINT8 Channel )
{
   // Local Data
   FPGA_SHADOW_ARINC429_RX_CFG_PTR pShadowRxCfg;
   
   // Initialize Local Data
   pShadowRxCfg = &FPGA_GetShadowRam()->Rx[Channel];
   
   switch (Channel)
   {
      case 0:
         // Rx Chan 0
         // Set Common Ctl Reg
         FPGA_GetShadowRam()->CCR = (FPGA_GetShadowRam()->CCR & ~CCR_429_RX1_RATE)
                                    | (CCR_429_RX1_RATE * Speed);         
         break;
      case 1:
         // Rx Chan 1
         // Set Common Ctl Reg
         FPGA_GetShadowRam()->CCR = (FPGA_GetShadowRam()->CCR & ~CCR_429_RX2_RATE)
                                    | (CCR_429_RX2_RATE * Speed);

         break;
      case 2:
         // Rx Chan 2
         // Set Common Ctl Reg
         FPGA_GetShadowRam()->CCR = (FPGA_GetShadowRam()->CCR & ~CCR_429_RX3_RATE)
                                    | (CCR_429_RX3_RATE * Speed);
         break;
      case 3:
         // Rx Chan 3
         // Set Common Ctl Reg
         FPGA_GetShadowRam()->CCR = (FPGA_GetShadowRam()->CCR & ~CCR_429_RX4_RATE)
                                    | (CCR_429_RX4_RATE * Speed);
         break;
      default:
         FATAL("Invalid ARINC429 RX Channel = %d", Channel );
         break;
   }
   
   // Set Rx Cfg Reg
   pShadowRxCfg->CREG = (pShadowRxCfg->CREG & ~RCR_429_PARITY_SELECT) |
                        (Parity * RCR_429_PARITY_SELECT);

   pShadowRxCfg->CREG = (pShadowRxCfg->CREG & ~RCR_429_LABEL_SWAP) |
                        (SwapBits * RCR_429_LABEL_SWAP);
                        
   
   // TBD: Does this need to be later? Maybe make its own function                     
   if (Enable == TRUE)
   {
      pShadowRxCfg->CREG = (pShadowRxCfg->CREG & ~RCR_429_ENABLE) |
                           (CR_429_ENABLE_VAL * RCR_429_ENABLE);
   }
   else 
   {
      pShadowRxCfg->CREG = (pShadowRxCfg->CREG & ~RCR_429_ENABLE) |
                           (CR_429_DISABLE_VAL * RCR_429_ENABLE);
   }
}

/******************************************************************************
 * Function:     Arinc429DrvCfgTxChan
 *
 * Description:  Configures the Arinc429 Tx channel thru the FPGA shadow RAM.
 *               Does not directly write to FPGA Register. 
 *
 * Parameters:   Speed - HIGH or LOW speed ARINC
 *               Parity - EVEN or ODD Parity 
 *               SwapBits - Swap or Not Swap the 8 bit label bits
 *               Enable - Enable or disable the channel for processing 
 *               Channel - Arinc429 channel to cfg
 *
 * Returns:      none
 *
 * Notes: // Needs to be called for each TX channel
 *
 *****************************************************************************/
void Arinc429DrvCfgTxChan ( ARINC429_SPEED Speed, ARINC429_PARITY Parity,
                            ARINC429_TX_SWAP  SwapBits, BOOLEAN Enable, UINT8 Channel )
{
   // Local Data
   FPGA_SHADOW_ARINC429_TX_CFG_PTR pShadowTxCfg;

   // Set a local transmit mode value based on the enable flag.
   UINT8 Lbr429TxValue = Enable ? LBR_429_TX_NORMAL_VAL : LBR_429_TX_QUIET_VAL;
   
   // Initialize Local Data
   pShadowTxCfg = &FPGA_GetShadowRam()->Tx[Channel];
   
   switch ( Channel )
   {
      case 0:
         FPGA_GetShadowRam()->CCR = (FPGA_GetShadowRam()->CCR & ~CCR_429_TX1_RATE)
                                     | (CCR_429_TX1_RATE * Speed);
         
         FPGA_GetShadowRam()->LBR = (FPGA_GetShadowRam()->LBR & ~LBR_429_TX1_DRIVE)
                                     | (LBR_429_TX1_DRIVE * Lbr429TxValue);
         break;
      case 1:      
         FPGA_GetShadowRam()->CCR = (FPGA_GetShadowRam()->CCR & ~CCR_429_TX2_RATE)
                                     | (CCR_429_TX2_RATE * Speed);
         
         FPGA_GetShadowRam()->LBR = (FPGA_GetShadowRam()->LBR & ~LBR_429_TX2_DRIVE)
                                     | (LBR_429_TX2_DRIVE * Lbr429TxValue);
         break;
      default:
         FATAL("Invalid ARINC429 TX Channel = %d", Channel );
         break;
   }   

   pShadowTxCfg->CREG = (pShadowTxCfg->CREG & ~TCR_429_PARITY_SELECT) |
                        (Parity * TCR_429_PARITY_SELECT);

   pShadowTxCfg->CREG = (pShadowTxCfg->CREG & ~TCR_429_ENABLE) |
                        (Enable * TCR_429_ENABLE);

   pShadowTxCfg->CREG = (pShadowTxCfg->CREG & ~TCR_429_BIT_FLIP) |
                        (SwapBits * TCR_429_BIT_FLIP);

   DIO_SetPin(ARINC429_Pwr, Enable ? DIO_SetHigh : DIO_SetLow );
}

/******************************************************************************
 * Function:     Arinc429DrvFPGA_OK
 *
 * Description:  Returns the current status of the HW FPGA
 *
 * Parameters:   none
 *
 * Returns:      TRUE if FPGA status is OK
 *               FALSE if FPGA status returns not OK 
 *
 * Notes:        none
 *
 *****************************************************************************/
BOOLEAN Arinc429DrvFPGA_OK ( void )
{
   return ( FPGA_GetStatus()->SystemStatus == FPGA_STATUS_OK );  
}

/******************************************************************************
 * Function:     Arinc429DrvCheckBITStatus
 *
 * Description:  Checks and returns Parity and Framing errors detected by FPGA
 *
 * Parameters:   pParityErrCnt - Ptr to return current parity err count 
 *               pFramingErrCnt - Ptr to return current framing err count 
 *               pIntCount - Ptr to return current interrupt count 
 *               pFPGA_Status - Ptr to return current FPGA status 
 *               Channel - Arinc429 channel to return status counts 
 *
 * Returns:      TRUE if Arinc429 status is Ok
 *               FALSE if Arinc429 status is Not Ok
 *
 * Notes:        none
 *
 *****************************************************************************/
BOOLEAN Arinc429DrvCheckBITStatus ( UINT32 *pParityErrCnt, UINT32 *pFramingErrCnt, 
                                    UINT32 *pIntCount,     UINT16 *pFPGA_Status,
                                    UINT8 Channel ) 
{
   // Local Data
   FPGA_RX_REG_PTR pfpgaRxReg;
   BOOLEAN         bDataPresent;
   
   // Initialize Local Data
   pfpgaRxReg = (FPGA_RX_REG_PTR) &fpgaRxReg[Channel];
   
   // Read FPGA Rx n Status Register
   *pFPGA_Status = FPGA_R(pfpgaRxReg->Status);
   *pIntCount    = m_Arinc429HW.InterruptCnt;
  
   // ParityErrCnt
   if ( ( *pFPGA_Status & RSR_429_PARITY_ERROR ) == RSR_429_PARITY_ERROR )
   {
      *pParityErrCnt += 1;
      // Clear the Parity Error
      FPGA_W(pfpgaRxReg->Configuration, (FPGA_R(pfpgaRxReg->Configuration) & 
                                        ~RCR_429_PARITY_ERROR_CLR) |
                                        (RCR_429_SET_CLR_VAL * RCR_429_PARITY_ERROR_CLR));
      FPGA_W(pfpgaRxReg->Configuration, (FPGA_R(pfpgaRxReg->Configuration) & 
                                        ~RCR_429_PARITY_ERROR_CLR) |
                                        (RCR_429_UNSET_CLR_VAL * RCR_429_PARITY_ERROR_CLR));
   }

   // FramingErrCnt
   if ( ( *pFPGA_Status & RSR_429_FRAMING_ERROR ) == RSR_429_FRAMING_ERROR )
   {
      *pFramingErrCnt += 1;
      // Clear the FramingErrCnt
      FPGA_W(pfpgaRxReg->Configuration, (FPGA_R(pfpgaRxReg->Configuration) & 
                                        ~RCR_429_FRAME_ERROR_CLR) |
                                        (RCR_429_SET_CLR_VAL * RCR_429_FRAME_ERROR_CLR));
      FPGA_W(pfpgaRxReg->Configuration, (FPGA_R(pfpgaRxReg->Configuration) & 
                                        ~RCR_429_FRAME_ERROR_CLR) |
                                        (RCR_429_UNSET_CLR_VAL * RCR_429_FRAME_ERROR_CLR));
   }   
   
   bDataPresent = ( Arinc429DrvFPGA_OK() && 
                    ((*pFPGA_Status & RSR_429_DATA_PRESENT) == RSR_429_DATA_PRESENT));
   
   return bDataPresent;
}

/******************************************************************************
 * Function:     Arinc429DrvRxGetCounts
 *
 * Description:  Returns current Rx Channel count status 
 *
 * Parameters:   Channel - current Rx channel 
 *
 * Returns:      Ptr to object m_Arinc429HW.Rx[channel] 
 *
 * Notes:        none 
 *
 *****************************************************************************/
ARINC429_DRV_RX_STATUS_PTR Arinc429DrvRxGetCounts (UINT8 Channel)
{
   return ((ARINC429_DRV_RX_STATUS_PTR) &m_Arinc429HW.Rx[Channel]);
}

/******************************************************************************
 * Function:     Arinc429DrvTxGetCounts
 *
 * Description:  Returns current Tx Channel count status
 *
 * Parameters:   Channel - Tx channel count status
 *
 * Returns:      Ptr to object m_arinc429HW.Tx[Channel]
 *
 * Notes:        none
 *
 *****************************************************************************/
ARINC429_DRV_TX_STATUS_PTR Arinc429DrvTxGetCounts (UINT8 Channel)
{
   return ((ARINC429_DRV_TX_STATUS_PTR) &m_Arinc429HW.Tx[Channel]);
}


/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

#ifdef A429_TEST_OUTPUT
/******************************************************************************
 * Function:    Arinc429DrvDumpRegs
 *
 * Description: Dump contents of FPGA registers.  
 *
 * Parameters:  none
 *
 * Returns:     none 
 *
 * Notes:       none 
 *
 *****************************************************************************/
/*vcast_dont_instrument_start*/
static void Arinc429DrvDumpRegs (void)
{
  sprintf (GSE_OutLine,
      "\r\nArinc429DrvDumpRegs: CCR 0x%04x, LBR 0x%04x",
      FPGA_R( FPGA_429_COMMON_CONTROL ), FPGA_R( FPGA_429_LOOPBACK_CONTROL ) );
  GSE_PutLine(GSE_OutLine); 

  sprintf (GSE_OutLine,
            "\r\n     RX1: CR 0x%04x, SR 0x%04x, FF 0x%04x, LSW 0x%04x, MSW 0x%04x",
            FPGA_R( FPGA_429_RX1_CONFIG ),
            FPGA_R( FPGA_429_RX1_STATUS ), FPGA_R( FPGA_429_RX1_FIFO_STATUS ),
            FPGA_R( FPGA_429_RX1_LSW ),    FPGA_R( FPGA_429_RX1_MSW )       );
  GSE_PutLine(GSE_OutLine); 

  sprintf (GSE_OutLine,
            "\r\n     RX2: CR 0x%04x, SR 0x%04x, FF 0x%04x, LSW 0x%04x, MSW 0x%04x",
            FPGA_R( FPGA_429_RX2_CONFIG ),
            FPGA_R( FPGA_429_RX2_STATUS ), FPGA_R( FPGA_429_RX2_FIFO_STATUS ),
            FPGA_R( FPGA_429_RX2_LSW ),    FPGA_R( FPGA_429_RX2_MSW )       );
  GSE_PutLine(GSE_OutLine); 

  sprintf (GSE_OutLine,
            "\r\n     RX3: CR 0x%04x, SR 0x%04x, FF 0x%04x, LSW 0x%04x, MSW 0x%04x",
            FPGA_R( FPGA_429_RX3_CONFIG ),
            FPGA_R( FPGA_429_RX3_STATUS ), FPGA_R( FPGA_429_RX3_FIFO_STATUS ),
            FPGA_R( FPGA_429_RX3_LSW ),    FPGA_R( FPGA_429_RX3_MSW )       );
  GSE_PutLine(GSE_OutLine); 

  sprintf (GSE_OutLine,
            "\r\n     RX4: CR 0x%04x, SR 0x%04x, FF 0x%04x, LSW 0x%04x, MSW 0x%04x",
            FPGA_R( FPGA_429_RX4_CONFIG ),
            FPGA_R( FPGA_429_RX4_STATUS ), FPGA_R( FPGA_429_RX4_FIFO_STATUS ),
            FPGA_R( FPGA_429_RX4_LSW ),    FPGA_R( FPGA_429_RX4_MSW )       );
  GSE_PutLine(GSE_OutLine); 

  sprintf (GSE_OutLine,
            "\r\n     TX1: CR 0x%04x, SR 0x%04x, FF 0x%04x",
            FPGA_R( FPGA_429_TX1_CONFIG ),
            FPGA_R( FPGA_429_TX1_STATUS ), FPGA_R( FPGA_429_TX1_FIFO_STATUS ) );
  GSE_PutLine(GSE_OutLine); 

  sprintf (GSE_OutLine,
            "\r\n     TX2: CR 0x%04x, SR 0x%04x, FF 0x%04x",
            FPGA_R( FPGA_429_TX2_CONFIG ),
            FPGA_R( FPGA_429_TX2_STATUS ), FPGA_R( FPGA_429_TX2_FIFO_STATUS ) );
  GSE_PutLine(GSE_OutLine); 
}
/*vcast_dont_instrument_end*/
#endif

/******************************************************************************
 * Function:    Arinc429DrvLogFpgaRegs
 *
 * Description: Write contents of ARINC429 FPGA registers to a log.
 *              Data written on test failure. Only first failure
 *              detected is logged.
 *
 * Parameters:  pDest - pointer to log storage area
 *
 * Returns:     none 
 *
 * Notes:       none 
 *
 *****************************************************************************/
static void Arinc429DrvLogFpgaRegs (ARINC_DRV_PBIT_FAIL_DATA *pDest)
{
  if (!fpgaLogged)
  {
    fpgaLogged  = TRUE;
    pDest->ccr  = FPGA_R( FPGA_429_COMMON_CONTROL );
    pDest->lbr  = FPGA_R( FPGA_429_LOOPBACK_CONTROL );
    pDest->rxCR[ARINC_CHAN_0] = FPGA_R( FPGA_429_RX1_CONFIG );
    pDest->rxSR[ARINC_CHAN_0] = FPGA_R( FPGA_429_RX1_STATUS );
    pDest->rxFF[ARINC_CHAN_0] = FPGA_R( FPGA_429_RX1_FIFO_STATUS );
    pDest->rxCR[ARINC_CHAN_1] = FPGA_R( FPGA_429_RX2_CONFIG );
    pDest->rxSR[ARINC_CHAN_1] = FPGA_R( FPGA_429_RX2_STATUS );
    pDest->rxFF[ARINC_CHAN_1] = FPGA_R( FPGA_429_RX2_FIFO_STATUS );
    pDest->rxCR[ARINC_CHAN_2] = FPGA_R( FPGA_429_RX3_CONFIG );
    pDest->rxSR[ARINC_CHAN_2] = FPGA_R( FPGA_429_RX3_STATUS );
    pDest->rxFF[ARINC_CHAN_2] = FPGA_R( FPGA_429_RX3_FIFO_STATUS );
    pDest->rxCR[ARINC_CHAN_3] = FPGA_R( FPGA_429_RX4_CONFIG );
    pDest->rxSR[ARINC_CHAN_3] = FPGA_R( FPGA_429_RX4_STATUS );
    pDest->rxFF[ARINC_CHAN_3] = FPGA_R( FPGA_429_RX4_FIFO_STATUS );
    pDest->txCR[ARINC_CHAN_0] = FPGA_R( FPGA_429_TX1_CONFIG );
    pDest->txSR[ARINC_CHAN_0] = FPGA_R( FPGA_429_TX1_STATUS );
    pDest->txFF[ARINC_CHAN_0] = FPGA_R( FPGA_429_TX1_FIFO_STATUS );
    pDest->txCR[ARINC_CHAN_1] = FPGA_R( FPGA_429_TX2_CONFIG );
    pDest->txSR[ARINC_CHAN_1] = FPGA_R( FPGA_429_TX2_STATUS );
    pDest->txFF[ARINC_CHAN_1] = FPGA_R( FPGA_429_TX2_FIFO_STATUS );
  }
}

/******************************************************************************
 * Function:    Arinc429DrvRestoreStartupCfg
 *
 * Description: Called and used during Arinc429 Startup to initialize the 
 *              Arinc429 Cfg.  This will normally be over written by the 
 *              Usr Cfg during a later part of initialization.  
 *
 * Parameters:  none
 *
 * Returns:     none 
 *
 * Notes:       none 
 *
 *****************************************************************************/
static void Arinc429DrvRestoreStartupCfg (void)
{
   // Local Data
   UINT8  Channel;
   UINT16 Label;
   
   Arinc429DrvSetIMR1_2 ();
   
   // Set Default Arinc 429 Rx Channel
   for ( Channel = 0; Channel < FPGA_MAX_RX_CHAN; Channel++)
   {
      Arinc429DrvCfgRxChan ( ARINC429_SPEED_HIGH, ARINC429_PARITY_ODD,
                             ARINC429_RX_SWAP_LABEL, FALSE, Channel );
      
      for ( Label = 0; Label < ARINC429_MAX_LABELS; Label++ )
      {
         Arinc429DrvCfgLFR ( 0x0F, Channel, (UINT8)Label, FALSE );
      }
   }

   // Set Default Arinc 429 Tx Channel
   for ( Channel = 0; Channel < FPGA_MAX_TX_CHAN; Channel++)
   {
      Arinc429DrvCfgTxChan ( ARINC429_SPEED_HIGH, ARINC429_PARITY_ODD,
                             ARINC429_TX_SWAP_LABEL, FALSE, Channel );
   }
}

/******************************************************************************
 * Function:    Arinc429DrvTest_FIFO
 *
 * Description: Performs the following tests on each Rx Ch
 *                 0) Arinc429_Test_LFR() 
 *                 1) Arinc429_Test_FIFO_Empty()
 *                 2) Arinc429_Test_FIFO_Full()
 *                 3) Arinc429_Test_FIFO_Half_Full()
 *
 * Parameters:  pdest - ptr to data store to return PBIT fail data 
 *                      (see FIFOTest[])
 *              psize - ptr to size var to return the size of PBIT fail data
 *                      (see FIFOTest[]) 
 *
 * Returns:     DRV_OK  
 *              DRV_A429_RX_FIFO_TEST_FAIL  
 *
 * Notes:       
 *  1) All Four Arinc Rx Ch are independently tested 
 *
 *  2) On the first FIFO test failure of a Rx Ch, ->ResultCode is updated 
 *     with DRV_A429_RX_FIFO_TEST_FAIL and the FIFOTest result data is returned
 *     to identify which Ch and Test failed in FIFOTest[] struct  
 *
 *  3) System Status of each Rx Ch is independently determined and set in 
 *     m_Arinc429MgrBlock.status.Rx[i].Status.  
 *     This allows provision to prevent an "unused" Arinc Rx Ch from which 
 *     fails FIFOTest from declaring the entire Arinc Rx func "bad".  TBD
 *     determine if this "provision" is really necessary ? 
 *
 *  4) On the first FIFO failure of each ch, the subsequent FIFO tests are not 
 *     performed for the current ch and thus defaults to test "FAILED" case to 
 *     indicate FIFO test not performed.  
 *     Note: Debug focus should be placed on the first FIFO test failure as
 *           that is the only test case which was actually performed for specified 
 *           ch. 
 *     Note: Test of other Rx Ch will be processed. 
 *
 *  5) Overlays ARINC_DRV_PBIT_RX_LFR_FIFO_TEST_RESULT on to *pdest to return  
 *     both LFR and FIFO test results
 *
 *****************************************************************************/
static RESULT Arinc429DrvTest_FIFO (UINT8 *pdest, UINT16 *psize)
{
  RESULT status_test;           // Status of each FIFO test of each Rx Ch
  RESULT status;                // Overall status of the Arinc429_Test_FIFO() 
  FPGA_TX_REG_PTR pfpgaTxReg;   // Current Tx Ch being tested 
  ARINC_DRV_PBIT_RX_FIFO_TEST_RESULT FIFOTestRx;  // Summary of FIFO test of all Rx Ch.  To 
                                                  //   be returned if at least one FIFO 
                                                  //   test fails
                                                  
  ARINC_DRV_PBIT_TX_FIFO_TEST_RESULT FIFOTestTx;  // Summary of FIFO test of all Tx Ch.  To 
                                                  //   be returned if at least one FIFO test 
                                                  //   fails
                                                  
  ARINC_DRV_PBIT_TEST_RESULT *OverAllTest_Result_ptr; 
  UINT32 i;

  OverAllTest_Result_ptr = (ARINC_DRV_PBIT_TEST_RESULT *) pdest; 
  
  memset ( &FIFOTestRx, 0, sizeof(ARINC_DRV_PBIT_RX_FIFO_TEST_RESULT) ); 
  memset ( &FIFOTestTx, 0, sizeof(ARINC_DRV_PBIT_TX_FIFO_TEST_RESULT) ); 
  FIFOTestRx.ResultCode = DRV_OK;   // Init Overall status of all FIFO test of all Rx Ch
  FIFOTestTx.ResultCode = DRV_OK;   // Init Overall status of all FIFO test of all Tx Ch
  // *psize = 0;   // Init to 0 to indicate no FIFO test failures 
 
  // Loop and test all Arinc Rx Ch and keep track of test result for each ch. 
  //  On single test failure, the overall FIFO test status (.ResultCode) shall be set to 
  //  DRV_A429_RX_FIFO_TEST_FAIL 
  for (i = 0; i < FPGA_MAX_RX_CHAN; i++)
  {    
    // Test FIFO Empty
    status_test = Arinc429DrvTest_Rx_FIFO ( (ARINC429_CHAN_ENUM) i, FIFO_EMPTY_TEST,
                                            &OverAllTest_Result_ptr->failData); 
    
    if (status_test == DRV_OK) 
    {
      // Indicate FIFO Empty Test Pass
      FIFOTestRx.bTestPass[A429_RX_FIFO_TEST_EMPTY][i] = TRUE; 
      // Test FIFO Half Full 
      status_test = Arinc429DrvTest_Rx_FIFO ((ARINC429_CHAN_ENUM) i, FIFO_HALF_FULL_TEST,
                                              &OverAllTest_Result_ptr->failData); 
    }
    
    if (status_test == DRV_OK) 
    {
      // Indicates FIFO Half Full Pass
      FIFOTestRx.bTestPass[A429_RX_FIFO_TEST_HALF_FULL][i] = TRUE;
      // Test FIFO Full
      status_test = Arinc429DrvTest_Rx_FIFO ( (ARINC429_CHAN_ENUM) i, FIFO_FULL_TEST,
                                              &OverAllTest_Result_ptr->failData); 
    }
    
    if (status_test == DRV_OK) 
    {
      // Indicates FIFO Full Pass
      FIFOTestRx.bTestPass[A429_RX_FIFO_TEST_FULL][i] = TRUE;
      // Test Over Run for HW.
      status_test = Arinc429DrvTest_Rx_FIFO ( (ARINC429_CHAN_ENUM) i, FIFO_OVERRUN_TEST,
                                              &OverAllTest_Result_ptr->failData); 
    }
    
    if (status_test == DRV_OK)
    {
      // Indicates FIFO Overrun Pass
      FIFOTestRx.bTestPass[A429_RX_FIFO_TEST_OVERRUN][i] = TRUE;
    }
    
    if (status_test != DRV_OK)
    {
      // Set each Rx Ch Status if fault encountered 
      m_Arinc429HW.Rx[i].Status = ARINC429_DRV_STATUS_FAULTED_PBIT; 
    }
    
    // Update overall FIFO Test status 
    FIFOTestRx.ResultCode |= status_test; 
  }
  
  // Loop and test all Arinc Tx Ch and keep track of test result for each ch.  
  //   And only if status_test was successful. 
  if ( FPGA_GetStatus()->Version >= FPGA_VER_12 ) 
  {
    for (i = 0; i < FPGA_MAX_TX_CHAN; i++) 
    {
      pfpgaTxReg = (FPGA_TX_REG_PTR) &fpgaTxReg_v12[i]; 
      
      status_test = Arinc429DrvTest_Tx_FIFO ( pfpgaTxReg, (ARINC429_CHAN_ENUM) i, 
                                              FIFO_EMPTY_TEST,
                                              &OverAllTest_Result_ptr->failData);  
      
      if (status_test == DRV_OK) 
      {
        // Indicates FIFO Half Full Pass
        FIFOTestTx.bTestPass[A429_TX_FIFO_TEST_EMPTY][i] = TRUE;
        // Test FIFO Half Full
        status_test = Arinc429DrvTest_Tx_FIFO ( pfpgaTxReg, (ARINC429_CHAN_ENUM) i, 
                                                FIFO_HALF_FULL_TEST,
                                                &OverAllTest_Result_ptr->failData);  
      }
      
      if (status_test == DRV_OK) 
      {
        // Indicates FIFO Half Full Pass
        FIFOTestTx.bTestPass[A429_TX_FIFO_TEST_HALF_FULL][i] = TRUE;
        // Test FIFO Full
        status_test = Arinc429DrvTest_Tx_FIFO ( pfpgaTxReg, (ARINC429_CHAN_ENUM) i, 
                                                FIFO_FULL_TEST,
                                                &OverAllTest_Result_ptr->failData);  
      }
      
      if (status_test == DRV_OK)
      {
        // Indicates FIFO Full Pass
        FIFOTestTx.bTestPass[A429_TX_FIFO_TEST_FULL][i] = TRUE;
        // Test Over Run for HW.
        status_test = Arinc429DrvTest_Tx_FIFO ( pfpgaTxReg, (ARINC429_CHAN_ENUM) i,
                                                FIFO_OVERRUN_TEST,
                                                &OverAllTest_Result_ptr->failData);
      }
      
      if (status_test == DRV_OK)
      {
        // Indicates FIFO Overrun Pass
        FIFOTestTx.bTestPass[A429_TX_FIFO_TEST_OVERRUN][i] = TRUE;
      }
      
      if (status_test != DRV_OK)
      {
        // Set each Rx Ch Status if fault encountered 
        m_Arinc429HW.Tx[i].Status = ARINC429_DRV_STATUS_FAULTED_PBIT; 
      }
      
      // Update overall FIFO Test status 
      FIFOTestTx.ResultCode |= status_test; 
    } // End for loop FPGA_MAX_TX_CHAN
  } // End if (FPGA_GetStatus()->Version >= FPGA_VER_12)

  // Always return current results
  memcpy ( &OverAllTest_Result_ptr->bTestPassFIFORx[0][0], &FIFOTestRx.bTestPass[0][0], 
                   sizeof(BOOLEAN) * FPGA_MAX_RX_CHAN * A429_RX_FIFO_TEST_MAX); 
  memcpy ( &OverAllTest_Result_ptr->bTestPassFIFOTx[0][0], &FIFOTestTx.bTestPass[0][0], 
                   sizeof(BOOLEAN) * FPGA_MAX_TX_CHAN * A429_TX_FIFO_TEST_MAX); 
  
  if ( (FIFOTestRx.ResultCode != DRV_OK) || (FIFOTestTx.ResultCode != DRV_OK) )
  {
       
    // Will OR in the ResultCode to Overall Testing 
    OverAllTest_Result_ptr->ResultCode |= (FIFOTestRx.ResultCode | FIFOTestTx.ResultCode); 

    // Set Return Data Size
    *psize = sizeof(ARINC_DRV_PBIT_TEST_RESULT); 
  }
  
  // SCR #383
  // Can only return one result code and not BIT OR of the two tests as 
  //    the startup GSE PBIT display can not handle BIT OR result.  
  //    Rx FIFO Test has higher priority.  
  if ( FIFOTestRx.ResultCode != DRV_OK ) 
  {
    status = FIFOTestRx.ResultCode; 
  }
  else if ( FIFOTestTx.ResultCode != DRV_OK ) 
  {
    status = FIFOTestTx.ResultCode; 
  }
  else 
  {
    status = DRV_OK; 
  }
  
  return ( (RESULT) status );
  
}

/******************************************************************************
 * Function:    Arinc429DrvTest_Rx_FIFO
 *
 * Description: Test EMPTY, HALF_FULL and FULL indications 
 *
 * Parameters:  pfpgaRxReg - Ptr to 1 of 2 Rx Reg in the FPGA to test 
 *              ch - Rx channel to test 
 *              testType - EMPTY, HALF_FULL, FULL 
 *              failData - ARINC429 failure data
 *
 * Returns:     DRV_OK
 *              DRV_A429_RX_FIFO_TEST_FAIL
 *
 * Notes:
 *  1) On entry, assumes all INT are masked and Rx/Tx are disabled.
 *  2) Has initial wait for INT to be "quiet" before start of test 
 *  3) Has max sec timeout (no INT seen) before declaring test failure 
 *
 *****************************************************************************/
static RESULT Arinc429DrvTest_Rx_FIFO ( ARINC429_CHAN_ENUM ch, 
                                       ARINC429_FIFO_TEST_TYPE testType,
                                       ARINC_DRV_PBIT_FAIL_DATA *failData )
{
  // Local Data
  RESULT status;
  UINT16 nFIFOCnt;
  
  UINT16 lsw;
  UINT16 msw;
  
  UINT16 int_mask_val;
  UINT16 int_mask_bits;
  
  ARINC429_DRV_RX_STATUS_PTR pArincRxStatus;
  ARINC429_DRV_STATUS_PTR pArincStatus;
  
  UINT32 StartTime;
  UINT16 nCntTx, nCntRx;
  
  UINT16 fifoFlag; 
  UINT16 XmitSize; 
  UINT32 *flagCnt; 
  
  UINT16 nTestVal; 
  UINT16 nCntDn; 
  UINT16 i; 

  UINT16 loops;  
  
  
  status = DRV_A429_RX_FIFO_TEST_FAIL;
  
  pArincStatus = (ARINC429_DRV_STATUS_PTR) &m_Arinc429HW;
  pArincRxStatus = (ARINC429_DRV_RX_STATUS_PTR) &m_Arinc429HW.Rx[ch];
  
  switch ( testType ) 
  {
    case FIFO_EMPTY_TEST:
      fifoFlag = RFSR_429_FIFO_EMPTY; 
      XmitSize = 1; 
      int_mask_val = ( fpgaRxReg[ch].imr_full_bit *  IMR1_MASK_IRQ_VAL ) |
                     ( fpgaRxReg[ch].imr_halffull_bit * IMR1_MASK_IRQ_VAL ) |
                     ( fpgaRxReg[ch].imr_not_empty_bit * IMR1_UNMAK_IRQ_VAL );
      flagCnt = &pArincRxStatus->FIFO_NotEmptyCnt; 
      break; 
      
    case FIFO_HALF_FULL_TEST:
      fifoFlag = RFSR_429_FIFO_HALF_FULL; 
      XmitSize = (FPGA_MAX_RX_FIFO_SIZE/2); 
      int_mask_val = ( fpgaRxReg[ch].imr_full_bit *  IMR1_MASK_IRQ_VAL ) |
                     ( fpgaRxReg[ch].imr_halffull_bit * IMR1_UNMAK_IRQ_VAL ) |
                     ( fpgaRxReg[ch].imr_not_empty_bit * IMR1_MASK_IRQ_VAL );
      flagCnt = &pArincRxStatus->FIFO_HalfFullCnt; 
      break; 

    case FIFO_FULL_TEST:
      fifoFlag = RFSR_429_FIFO_FULL;
      XmitSize = FPGA_MAX_RX_FIFO_SIZE; 
      int_mask_val = ( fpgaRxReg[ch].imr_full_bit *  IMR1_UNMAK_IRQ_VAL ) |
                     ( fpgaRxReg[ch].imr_halffull_bit * IMR1_MASK_IRQ_VAL ) |
                     ( fpgaRxReg[ch].imr_not_empty_bit * IMR1_MASK_IRQ_VAL );
      flagCnt = &pArincRxStatus->FIFO_FullCnt; 
      break; 
      
    case FIFO_OVERRUN_TEST:
      fifoFlag = RFSR_429_FIFO_OVERRUN;
      XmitSize = FPGA_MAX_RX_FIFO_SIZE + 1; 
      int_mask_val = ( fpgaRxReg[ch].imr_full_bit *  IMR1_MASK_IRQ_VAL ) |
                     ( fpgaRxReg[ch].imr_halffull_bit * IMR1_MASK_IRQ_VAL ) |
                     ( fpgaRxReg[ch].imr_not_empty_bit * IMR1_MASK_IRQ_VAL );
      flagCnt = &pArincRxStatus->FIFO_OverRunCnt; 
      break; 
      
    default: 
      FATAL("Arinc429 Rx Test FIFO Failed ch = %d", ch);           
      break; 
  }

#ifdef SUPPORT_FPGA_BELOW_V12
/*vcast_dont_instrument_start*/
// To be updated when final Version of FPGA is released 
  // loops = 2; 
  loops = ( FPGA_GetStatus()->Version >= (FPGA_VER_12) )  ? 2 : 1; 
// To be updated when final Version of FPGA is released 
/*vcast_dont_instrument_end*/
#else
  loops = 2; 
#endif  
  
  // Disable Rx Enable to force Test Mode
  FPGA_W(fpgaRxReg[ch].Configuration, (FPGA_R(fpgaRxReg[ch].Configuration) & ~TCR_429_ENABLE)
                                      | (TCR_429_ENABLE * CR_429_DISABLE_VAL));


  // Might want to read FIFO until empty !
  nFIFOCnt = 0;
  while ( ( (FPGA_R(fpgaRxReg[ch].FIFO_Status) & RFSR_429_FIFO_EMPTY) != RFSR_429_FIFO_EMPTY )
            && ( nFIFOCnt < FPGA_MAX_RX_FIFO_SIZE ) )
  {
    lsw = FPGA_R( fpgaRxReg[ch].lsw);
    msw = FPGA_R( fpgaRxReg[ch].msw);
    nFIFOCnt++;
  }
  
  // Verify that Rx FIFO Status is empty
  if ( (FPGA_R(fpgaRxReg[ch].FIFO_Status) & RFSR_429_FIFO_EMPTY) == RFSR_429_FIFO_EMPTY )
  {
    // UnMask or set the flag to be tested 
    int_mask_bits =  fpgaRxReg[ch].imr_full_bit | fpgaRxReg[ch].imr_halffull_bit |
                     fpgaRxReg[ch].imr_not_empty_bit;

    FPGA_W(FPGA_IMR1, ((FPGA_R(FPGA_IMR1) & ~int_mask_bits) | int_mask_val));

    //Unmask CPU INT Level 6 for FPGA
    MCF_INTC_IMRL &= ~(MCF_INTC_IMRL_INT_MASK6|MCF_INTC_IMRL_MASKALL);

    // Wait for INT
    PAUSE( TICKS_PER_500nSec);
    
    pArincRxStatus->FIFO_HalfFullCnt = 0;
    pArincRxStatus->FIFO_FullCnt = 0;
    pArincRxStatus->FIFO_NotEmptyCnt = 0;
    pArincRxStatus->FIFO_OverRunCnt = 0; 
    pArincStatus->InterruptCnt = 0;

    // Ensure that no INT occurs
    StartTime = TTMR_GetHSTickCount();
    while ( ( (TTMR_GetHSTickCount() - StartTime) < TICKS_PER_100msec) &&
              (pArincStatus->InterruptCnt == 0) )
    {
    }

    if ( ( pArincStatus->InterruptCnt == 0 ) && 
         ((FPGA_R(fpgaRxReg[ch].FIFO_Status) & RFSR_429_FIFO_EMPTY) == RFSR_429_FIFO_EMPTY) )
    {
      // Loop thru test twice with cnt up and then cnt down value 
      for (i = 0; i < loops; i++)    
      {
        nCntTx = 0;
        nCntDn = i; 
        *flagCnt = 0; 
  
        //Transmit Until FIFO flag condition reached or XmitSize is reached 
        while ( (nCntTx < XmitSize) && 
                (((FPGA_R(fpgaRxReg[ch].FIFO_Status) & fifoFlag) != fifoFlag) ||
                 (testType == FIFO_EMPTY_TEST)) )
        {
          nTestVal = nCntDn ? (0xFFFF - nCntTx) : nCntTx; 
          FPGA_W( fpgaRxReg[ch].lsw_test, nTestVal);
          FPGA_W( fpgaRxReg[ch].msw_test, nTestVal);
          
          //FPGA_W( pfpgaRxReg->lsw_test, nCntTx);
          //FPGA_W( pfpgaRxReg->msw_test, (0xFFFF - nCntTx));
          
          // Wait for status flag change 
          PAUSE( TICKS_PER_500nSec);
          nCntTx++;
        }
        
        // For Over Run condition INT does not occur and _OverRunCnt need to be manually 
        // generated 
        if ( (testType == FIFO_OVERRUN_TEST) && (nCntTx == XmitSize) )
        {
          *flagCnt = 1; 
        }
  
        // Verify cnt value AND INT occurs AND Fifo Flag has transition correctly if not 
        // EMPTY test. 
        if ( (nCntTx == XmitSize) && 
             (*flagCnt != 0)      &&
             ( (testType == FIFO_EMPTY_TEST) || 
               ((FPGA_R(fpgaRxReg[ch].FIFO_Status) & fifoFlag) == fifoFlag) ) 
           )
        {
          // Read FIFO
          nCntRx = 0;  
          while ( ((FPGA_R(fpgaRxReg[ch].FIFO_Status) & RFSR_429_FIFO_EMPTY) !=  
                    RFSR_429_FIFO_EMPTY) && 
                    (nCntRx < FPGA_MAX_RX_FIFO_SIZE) )
          {
            lsw = FPGA_R( fpgaRxReg[ch].lsw);
            msw = FPGA_R( fpgaRxReg[ch].msw);
            
            // Verify cnt value 
            nTestVal = nCntDn ? (0xFFFF - nCntRx) :  nCntRx; 
            nTestVal = TPX( nTestVal, eTp429Fifo1r, testType);

            // if ( (lsw == nCntRx) && (msw == (0xFFFF - (nCntRx))) )
            if ( (lsw == nTestVal) && (msw == nTestVal) )
            {
              nCntRx++;
            }
            else
            {
              // Note Due to 1st Read of MSW which force FIFO pointer to advance,
              // the last read of MSW will echo the last nCnt !  Must take this into 
              // consideration
              //
              // Exit while loop early !
              break;
            }
          } // end while loop thru reading FIFO
          
          // Ensure expected count values are identical 
          //    For over run case, we need to subtract one from nCntTx 
          if ( (((testType != FIFO_OVERRUN_TEST) && (nCntTx == nCntRx)) ||
                ((testType == FIFO_OVERRUN_TEST) && ((nCntTx - 1) == nCntRx))) && 
                ((FPGA_R(fpgaRxReg[ch].FIFO_Status) & RFSR_429_FIFO_EMPTY) 
                  == RFSR_429_FIFO_EMPTY ) )
          {
            // For empty test OR other test ensure flag has reset 
            if ( ( testType == FIFO_EMPTY_TEST ) ||
                 (( testType != FIFO_EMPTY_TEST) && 
                  ((FPGA_R(fpgaRxReg[ch].FIFO_Status) & fifoFlag) != fifoFlag) ) )
            {
              status = DRV_OK; 
            }
            else
            {
              ++failData->rxFifoCntErr;
              Arinc429DrvLogFpgaRegs(failData);
              failData->ffNotEmptyCnt = pArincRxStatus->FIFO_NotEmptyCnt; 
              failData->ffHalfFullCnt = pArincRxStatus->FIFO_HalfFullCnt; 
              failData->ffFullCnt     = pArincRxStatus->FIFO_FullCnt; 

#ifdef A429_TEST_OUTPUT
/*vcast_dont_instrument_start*/
              sprintf (GSE_OutLine,
                  "\r\nArinc429DrvTest_Rx_FIFO: RX FIFO Fail ch %d, type %d, txcnt=%d, rxcnt=%d",
                  i, testType, nCntTx, nCntRx);
              GSE_PutLine(GSE_OutLine); 
              Arinc429DrvDumpRegs();
/*vcast_dont_instrument_end*/
#endif
            }
          }
        } // Initial value matches, nCnt = FIFO size and flag is set 
        else
        {
          // Initial values don't match, nCnt != FIFO size or flag is not set 
          ++failData->rxFifoNoIntErr;
          Arinc429DrvLogFpgaRegs(failData);
          failData->ffNotEmptyCnt = pArincRxStatus->FIFO_NotEmptyCnt; 
          failData->ffHalfFullCnt = pArincRxStatus->FIFO_HalfFullCnt; 
          failData->ffFullCnt     = pArincRxStatus->FIFO_FullCnt; 
#ifdef A429_TEST_OUTPUT
/*vcast_dont_instrument_start*/
          sprintf (GSE_OutLine,
              "\r\nArinc429DrvTest_Rx_FIFO: RX FIFO Fail"
              " ch %d, type %d, txsize=%d, txcnt=%d, rxcnt=%d, flagcnt=%d",
              i, testType, XmitSize, nCntTx, nCntRx, flagCnt);
          GSE_PutLine(GSE_OutLine); 
          Arinc429DrvDumpRegs();
/*vcast_dont_instrument_end*/
#endif
        }
        
        // Ok this is a fudge, should find better solution. 
        // If first time thru this loop and status == DRV_OK then force back to init FAIL 
        // value for second execution, otherwise if status != DRV_OK fail this test and 
        // exit immediately. 
        if ( loops > 1 ) 
        {
          if ( (status == DRV_OK) && (nCntDn == 0) )
          {
            // Run 2nd pass thru this test and re-init status to DRV_A429_RX_FIFO_TEST_FAIL
            status = DRV_A429_RX_FIFO_TEST_FAIL; 
          }
          else if ( (status != DRV_OK) && (nCntDn == 0) )
          {
            // Don't run thru 2nd pass since we failed the first pass. 
            break; 
          }
        }
      } // Loop thru test twice for up cnt then dn cnt

    } // End of if No INT occurs when No INT is expected
    else
    {
      ++failData->intErr;    // Error: INT occured
      Arinc429DrvLogFpgaRegs(failData);
      failData->ffNotEmptyCnt = pArincRxStatus->FIFO_NotEmptyCnt; 
      failData->ffHalfFullCnt = pArincRxStatus->FIFO_HalfFullCnt; 
      failData->ffFullCnt     = pArincRxStatus->FIFO_FullCnt; 
#ifdef A429_TEST_OUTPUT
/*vcast_dont_instrument_start*/
      sprintf (GSE_OutLine,
          "\r\nArinc429DrvTest_Rx_FIFO: RX FIFO type %d ch %d Fail: INT Error", testType, ch);
      GSE_PutLine(GSE_OutLine); 
      Arinc429DrvDumpRegs();
/*vcast_dont_instrument_end*/
#endif
    }

  } // FIFO is not empty.  It should be empty at this point !
  else
  {
    ++failData->rxFifoNotEmpty;    // Error: FIFO is Not Empty
    Arinc429DrvLogFpgaRegs(failData);
    failData->ffNotEmptyCnt = pArincRxStatus->FIFO_NotEmptyCnt; 
    failData->ffHalfFullCnt = pArincRxStatus->FIFO_HalfFullCnt; 
    failData->ffFullCnt     = pArincRxStatus->FIFO_FullCnt; 
#ifdef A429_TEST_OUTPUT
/*vcast_dont_instrument_start*/
    sprintf (GSE_OutLine,
        "\r\nArinc429DrvTest_Rx_FIFO: RX FIFO type %d ch %d Fail: FIFO Not Empty", testType, ch);
    GSE_PutLine(GSE_OutLine); 
    Arinc429DrvDumpRegs();
/*vcast_dont_instrument_end*/
#endif
  }

  // Clear FIFO Cnt Flags
  *flagCnt = 0; 

  // If test failed, empty FIFO if necessary
  if ( status != DRV_OK )
  {
    // Might want to read FIFO until empty !
    nFIFOCnt = 0;
    while ( ( (FPGA_R(fpgaRxReg[ch].FIFO_Status) & RFSR_429_FIFO_EMPTY) 
              != RFSR_429_FIFO_EMPTY ) && ( nFIFOCnt < FPGA_MAX_RX_FIFO_SIZE ) )
    {
      lsw = FPGA_R( fpgaRxReg[ch].lsw);
      msw = FPGA_R( fpgaRxReg[ch].msw);
      nFIFOCnt++;
    }
  }


  // Mask FIFO Not Empty Int
  int_mask_val = ( fpgaRxReg[ch].imr_full_bit * IMR1_MASK_IRQ_VAL ) |
                 ( fpgaRxReg[ch].imr_halffull_bit * IMR1_MASK_IRQ_VAL ) |
                 ( fpgaRxReg[ch].imr_not_empty_bit * IMR1_MASK_IRQ_VAL );

  int_mask_bits =  fpgaRxReg[ch].imr_full_bit | fpgaRxReg[ch].imr_halffull_bit |
                   fpgaRxReg[ch].imr_not_empty_bit;

  FPGA_W(FPGA_IMR1, ((FPGA_R(FPGA_IMR1) & ~int_mask_bits) | int_mask_val));

  // Mask CPU Level 6 INT for the FPGA
  MCF_INTC_IMRL |= MCF_INTC_IMRL_INT_MASK6;

  // Final Read to clear status ! Dummy reads
  nCntRx = FPGA_R(fpgaRxReg[ch].FIFO_Status);
  nCntRx = FPGA_R(fpgaRxReg[ch].Status);
  nCntRx = FPGA_R(FPGA_ISR);

  return (status);

}

/******************************************************************************
 * Function:    Arinc429DrvTest_Tx_FIFO
 *
 * Description: Test EMPTY, HALF_FULL and FULL indications 
 *
 * Parameters:  pfpgaTxReg - Ptr to 1 of 2 Tx Reg in the FPGA to test 
 *              ch - Tx channel to test 
 *              testType - EMPTY, HALF_FULL, FULL 
 *              failData - ARINC429 failure data
 *
 * Returns:     DRV_OK
 *              DRV_A429_TX_FIFO_TEST_FAIL
 *
 * Notes:
 *  1) On entry, assumes all INT are masked and Rx/Tx are disabled.
 *  2) Has initial wait for INT to be "quiet" before start of test 
 *  3) Has max sec timeout (no INT seen) before declaring test failure 
 *
 *****************************************************************************/
static RESULT Arinc429DrvTest_Tx_FIFO (FPGA_TX_REG_PTR pfpgaTxReg, ARINC429_CHAN_ENUM ch, 
                                     ARINC429_FIFO_TEST_TYPE testType,
                                     ARINC_DRV_PBIT_FAIL_DATA *failData )
{
   // Local Data
   RESULT status;
   UINT16 nFIFOCnt;
  
   UINT16 lsw;
   UINT16 msw;
  
   UINT16 int_mask_val;
   UINT16 int_mask_bits;
  
   ARINC429_DRV_TX_STATUS_PTR pArincTxStatus;
   ARINC429_DRV_STATUS_PTR pArincStatus;
  
   UINT32 StartTime;
   UINT16 nCntTx, nCntRx;
   volatile UINT16 *IMR_TX; 
  
   UINT16 fifoFlag; 
   UINT16 XmitSize; 
   UINT32 *flagCnt; 
  
   UINT16 nTestVal; 
   UINT16 nCntDn; 
   UINT16 i; 
   
   status = DRV_A429_TX_FIFO_TEST_FAIL;
  
   if ( ch == ARINC_CHAN_0 ) 
   {
      IMR_TX = FPGA_IMR1;
   }
   else
   {
      IMR_TX = FPGA_IMR2;
   }

   pArincStatus = (ARINC429_DRV_STATUS_PTR) &m_Arinc429HW;
   pArincTxStatus = (ARINC429_DRV_TX_STATUS_PTR) &m_Arinc429HW.Tx[ch];
  
   switch ( testType ) 
   {
      case FIFO_EMPTY_TEST:
         fifoFlag = TSR_429_FIFO_EMPTY; 
         XmitSize = 1; 
         int_mask_val = ( pfpgaTxReg->imr_full_bit *  IMR1_MASK_IRQ_VAL ) |
                        ( pfpgaTxReg->imr_halffull_bit * IMR1_MASK_IRQ_VAL ) |
                        ( pfpgaTxReg->imr_empty_bit * IMR1_UNMAK_IRQ_VAL );
         flagCnt = &pArincTxStatus->FIFO_EmptyCnt; 
         break; 
      case FIFO_HALF_FULL_TEST:
         fifoFlag = TSR_429_FIFO_HALF_FULL; 
         XmitSize = (FPGA_MAX_TX_FIFO_SIZE/2); 
         int_mask_val = ( pfpgaTxReg->imr_full_bit *  IMR1_MASK_IRQ_VAL ) |
                        ( pfpgaTxReg->imr_halffull_bit * IMR1_UNMAK_IRQ_VAL ) |
                        ( pfpgaTxReg->imr_empty_bit * IMR1_MASK_IRQ_VAL );
         flagCnt = &pArincTxStatus->FIFO_HalfFullCnt; 
         break; 
      case FIFO_FULL_TEST:
         fifoFlag = TSR_429_FIFO_FULL;
         XmitSize = FPGA_MAX_TX_FIFO_SIZE; 
         int_mask_val = ( pfpgaTxReg->imr_full_bit *  IMR1_UNMAK_IRQ_VAL ) |
                        ( pfpgaTxReg->imr_halffull_bit * IMR1_MASK_IRQ_VAL ) |
                        ( pfpgaTxReg->imr_empty_bit * IMR1_MASK_IRQ_VAL );
         flagCnt = &pArincTxStatus->FIFO_FullCnt; 
         break; 
      case FIFO_OVERRUN_TEST:
         fifoFlag = TSR_429_FIFO_OVERRUN;
         XmitSize = FPGA_MAX_TX_FIFO_SIZE + 1; 
         int_mask_val = ( pfpgaTxReg->imr_full_bit *  IMR1_MASK_IRQ_VAL ) |
                        ( pfpgaTxReg->imr_halffull_bit * IMR1_MASK_IRQ_VAL ) |
                        ( pfpgaTxReg->imr_empty_bit * IMR1_MASK_IRQ_VAL );
         flagCnt = &pArincTxStatus->FIFO_OverRunCnt; 
         break; 
      default: 
         FATAL("Arinc429 Tx Test FIFO Failed ch = %d, type = %d", ch, testType);           
         break; 
   }
 
   // Disable Tx Enable to force Test Mode
   FPGA_W(pfpgaTxReg->Configuration, (FPGA_R(pfpgaTxReg->Configuration) & ~TCR_429_ENABLE) |
                                     (TCR_429_ENABLE * CR_429_DISABLE_VAL));

   // Might want to read FIFO until empty !
   nFIFOCnt = 0;
   while ( ( (FPGA_R(pfpgaTxReg->FIFO_Status) & TSR_429_FIFO_EMPTY) != TSR_429_FIFO_EMPTY ) 
          && ( nFIFOCnt < FPGA_MAX_TX_FIFO_SIZE ) )
   {
     lsw = FPGA_R( pfpgaTxReg->lsw_test);
     msw = FPGA_R( pfpgaTxReg->msw_test);
     nFIFOCnt++;
   }
  
   // Verify that Tx FIFO Status is empty
   if ( (FPGA_R(pfpgaTxReg->FIFO_Status) & TSR_429_FIFO_EMPTY) == TSR_429_FIFO_EMPTY )
   {
      // UnMask or set the flag to be tested 
      int_mask_bits =  pfpgaTxReg->imr_full_bit | pfpgaTxReg->imr_halffull_bit |
                       pfpgaTxReg->imr_empty_bit;

      FPGA_W(IMR_TX, ((FPGA_R(IMR_TX) & ~int_mask_bits) | int_mask_val));

      // Unmask CPU INT Level 6 for FPGA
      MCF_INTC_IMRL &= ~(MCF_INTC_IMRL_INT_MASK6|MCF_INTC_IMRL_MASKALL);

      // Wait for INT
      PAUSE( TICKS_PER_500nSec);
    
      pArincTxStatus->FIFO_HalfFullCnt = 0;
      pArincTxStatus->FIFO_FullCnt = 0;
      pArincTxStatus->FIFO_EmptyCnt = 0;
      pArincTxStatus->FIFO_OverRunCnt = 0; 
      pArincStatus->InterruptCnt = 0;

      // Ensure that no INT occurs
      StartTime = TTMR_GetHSTickCount();
      while ( ( (TTMR_GetHSTickCount() - StartTime) < TICKS_PER_100msec) &&
                (pArincStatus->InterruptCnt == 0) )
      {
      }

      if ( ( pArincStatus->InterruptCnt == 0 ) && 
           ((FPGA_R(pfpgaTxReg->FIFO_Status) & TSR_429_FIFO_EMPTY) == TSR_429_FIFO_EMPTY) )
      {
         // Loop thru test twice with cnt up and then cnt down value 
         // Comment out as FPGA HW appear to have issue with consecutive reads. 
         for ( i = 0; i < 2; i++ )   
         {
            nCntTx = 0;
            nCntDn = i; 
            *flagCnt = 0; 
  
            //Transmit Until FIFO flag condition reached or expected XmitSize reached 
            while ( (nCntTx < XmitSize) && 
                    (((FPGA_R(pfpgaTxReg->FIFO_Status) & fifoFlag) != fifoFlag)  ||
                    (testType == FIFO_EMPTY_TEST)) )
            {
               nTestVal = nCntDn ? (0x7FFF - nCntTx) : nCntTx; 
               FPGA_W( pfpgaTxReg->lsw, nTestVal);
               FPGA_W( pfpgaTxReg->msw, nTestVal);
        
               // Wait for status flag change 
               PAUSE( TICKS_PER_500nSec);
               nCntTx++;
            }
        
            // For Over Run condition INT does not occur and _OverRunCnt need to be manually
            // generated 
            if ( (testType == FIFO_OVERRUN_TEST) && (nCntTx == XmitSize) )
            {
               *flagCnt = 1; 
            }
        
            // Verify cnt value AND
            // INT occurs and fifoFlag set appropriately if not testing FIFO Empty AND
            if ( (nCntTx == XmitSize) && 
                 ( (testType == FIFO_EMPTY_TEST) || ( (*flagCnt != 0) && 
                 ((FPGA_R(pfpgaTxReg->FIFO_Status) & fifoFlag) == fifoFlag) ) ) )
            {
               // Read FIFO
               nCntRx = 0;  
               while ( ((FPGA_R(pfpgaTxReg->FIFO_Status) & TSR_429_FIFO_EMPTY) 
                       !=  TSR_429_FIFO_EMPTY) && (nCntRx < FPGA_MAX_TX_FIFO_SIZE) )
               {
                  lsw = FPGA_R( pfpgaTxReg->lsw_test);
                  msw = FPGA_R( pfpgaTxReg->msw_test);
            
                  // Verify cnt value 
                  nTestVal = nCntDn ? (0x7FFF - nCntRx) :  nCntRx;
                  nTestVal = TPX( nTestVal, eTp429Fifo1t, testType);
            
                  // Verify cnt value 
                  if ( (lsw == nTestVal) && (msw == nTestVal))
                  {
                     nCntRx++;
                  }
                  else
                  {
                     // Note Due to 1st Read of MSW which force FIFO pointer to advance,
                     // the last read of MSW will echo the last nCnt !  Must take this 
                     // into consideration
                     //
                     // Exit while loop early !
                     break;
                  }
               } // end while loop thru reading FIFO
          
               // Ensure expected count values are identical 
               //   For over run case, we need to subtract one from nCntTx 
               if ( (((testType != FIFO_OVERRUN_TEST) && (nCntTx == nCntRx)) ||
                     ((testType == FIFO_OVERRUN_TEST) && ((nCntTx - 1) == nCntRx)) ) && 
                     ((FPGA_R(pfpgaTxReg->FIFO_Status) & TSR_429_FIFO_EMPTY) 
                       == TSR_429_FIFO_EMPTY ) )
               {
                  // For empty test, ensure empty INT occurred 
                  // For other test, ensure flag has reset 
                  if ( (( testType == FIFO_EMPTY_TEST ) && (*flagCnt != 0)) ||
                       (( testType != FIFO_EMPTY_TEST) && 
                       ((FPGA_R(pfpgaTxReg->FIFO_Status) & fifoFlag) != fifoFlag) ) )
                  {
                     status = DRV_OK; 
                  }
                  else
                  {
                    ++failData->txFifoCntErr;
                    Arinc429DrvLogFpgaRegs(failData);
                    failData->ffEmptyCnt    = pArincTxStatus->FIFO_EmptyCnt; 
                    failData->ffHalfFullCnt = pArincTxStatus->FIFO_HalfFullCnt; 
                    failData->ffFullCnt     = pArincTxStatus->FIFO_FullCnt; 
#ifdef A429_TEST_OUTPUT
/*vcast_dont_instrument_start*/
                    sprintf (GSE_OutLine,
                        "\r\nArinc429DrvTest_Tx_FIFO: TX FIFO Fail ch %d, type %d, txcnt=%d, rxcnt=%d",
                        i, testType, nCntTx, nCntRx);
                    GSE_PutLine(GSE_OutLine); 
                    Arinc429DrvDumpRegs();
/*vcast_dont_instrument_end*/
#endif
                  }
               }
            } // Initial value matches, nCnt == FIFO size and flag is set 
            else
            {
              // Initial values don't match, nCnt != FIFO size or flag is not set 
              ++failData->txFifoNoIntErr;
              Arinc429DrvLogFpgaRegs(failData);
              failData->ffEmptyCnt    = pArincTxStatus->FIFO_EmptyCnt; 
              failData->ffHalfFullCnt = pArincTxStatus->FIFO_HalfFullCnt; 
              failData->ffFullCnt     = pArincTxStatus->FIFO_FullCnt; 
#ifdef A429_TEST_OUTPUT
/*vcast_dont_instrument_start*/
              sprintf (GSE_OutLine,
                  "\r\nArinc429DrvTest_Tx_FIFO: TX FIFO Fail"
                  " ch %d, type %d, txsize=%d, txcnt=%d, rxcnt=%d, flagcnt=%d",
                  i, testType, XmitSize, nCntTx, nCntRx, flagCnt);
              GSE_PutLine(GSE_OutLine); 
              Arinc429DrvDumpRegs();
/*vcast_dont_instrument_end*/
#endif
            }
        
            // Ok this is a fudge, should find better solution. 
            // If first time thru this loop and status == DRV_OK then force back to init 
            //   FAIL value 
            // for second execution, otherwise if status != DRV_OK fail this test and 
            //   exit immediately. 
            if ( (status == DRV_OK) && (nCntDn == 0) )
            {
               // Run 2nd pass thru this test and re-init status to 
               // DRV_A429_RX_FIFO_TEST_FAIL
               status = DRV_A429_TX_FIFO_TEST_FAIL; 
            }
            else if ( (status != DRV_OK) && (nCntDn == 0) )
            {
               // Don't run thru 2nd pass since we failed the first pass. 
               break; 
            } 
         } // Loop thru test twice with cnt up and then cnt down value 
      } // End of if No INT occurs when No INT is expected
      else
      {
        ++failData->intErr;    // Error: INT occured
        Arinc429DrvLogFpgaRegs(failData);
        failData->ffEmptyCnt    = pArincTxStatus->FIFO_EmptyCnt; 
        failData->ffHalfFullCnt = pArincTxStatus->FIFO_HalfFullCnt; 
        failData->ffFullCnt     = pArincTxStatus->FIFO_FullCnt; 
#ifdef A429_TEST_OUTPUT
/*vcast_dont_instrument_start*/
        sprintf (GSE_OutLine,
          "\r\nArinc429DrvTest_Rx_FIFO: RX FIFO type %d ch %d Fail: INT Error", testType, ch);
        GSE_PutLine(GSE_OutLine); 
        Arinc429DrvDumpRegs();
/*vcast_dont_instrument_end*/
#endif
      }
   } // FIFO is not empty.  It should be empty at this point !
   else
   {
     ++failData->txFifoNotEmpty;    // Error: FIFO is Not empty
     Arinc429DrvLogFpgaRegs(failData);
     failData->ffEmptyCnt    = pArincTxStatus->FIFO_EmptyCnt; 
     failData->ffHalfFullCnt = pArincTxStatus->FIFO_HalfFullCnt; 
     failData->ffFullCnt     = pArincTxStatus->FIFO_FullCnt; 
#ifdef A429_TEST_OUTPUT
/*vcast_dont_instrument_start*/
     sprintf (GSE_OutLine,
         "\r\nArinc429DrvTest_Tx_FIFO: TX FIFO type %d ch %d: Fail FIFO Not Empty", testType, ch);
     GSE_PutLine(GSE_OutLine); 
     Arinc429DrvDumpRegs();
/*vcast_dont_instrument_end*/
#endif
   }

   // Clear FIFO Cnt Flags
   *flagCnt = 0; 

   // If test failed, empty FIFO if necessary
   if ( status != DRV_OK )
   {
      // Might want to read FIFO until empty !
      nFIFOCnt = 0;
      while ( ( (FPGA_R(pfpgaTxReg->FIFO_Status) & TSR_429_FIFO_EMPTY) != TSR_429_FIFO_EMPTY ) 
               && ( nFIFOCnt < FPGA_MAX_TX_FIFO_SIZE ) )
      {
         lsw = FPGA_R( pfpgaTxReg->lsw_test);
         msw = FPGA_R( pfpgaTxReg->msw_test);
         nFIFOCnt++;
      }
   }

   // Mask FIFO Not Empty Int
   int_mask_val = ( pfpgaTxReg->imr_full_bit * IMR1_MASK_IRQ_VAL ) |
                  ( pfpgaTxReg->imr_halffull_bit * IMR1_MASK_IRQ_VAL ) |
                  ( pfpgaTxReg->imr_empty_bit * IMR1_MASK_IRQ_VAL );

   int_mask_bits =  pfpgaTxReg->imr_full_bit | pfpgaTxReg->imr_halffull_bit |
                    pfpgaTxReg->imr_empty_bit;

   FPGA_W(IMR_TX, ((FPGA_R(IMR_TX) & ~int_mask_bits) | int_mask_val));

   // Mask CPU Level 6 INT for the FPGA
   MCF_INTC_IMRL |= MCF_INTC_IMRL_INT_MASK6;

   // Final Read to clear status ! Dummy reads
   nCntRx = FPGA_R(pfpgaTxReg->FIFO_Status);
   nCntRx = FPGA_R(pfpgaTxReg->Status);
   nCntRx = FPGA_R(FPGA_ISR);

#ifdef WIN32
   /*vcast_dont_instrument_start*/
   status = DRV_OK; 
   /*vcast_dont_instrument_end*/
#endif

   return (status);
}

/******************************************************************************
 * Function:    Arinc429DrvTest_LFR
 *
 * Description: Performs a data pattern test on each Rx Ch Label Filter RAM 
 *                inside the FPGA. 
 *
 * Parameters:  pdest - ptr to data store to return PBIT fail data 
 *                      (see LFRTest) 
 *              psize - ptr to size var to return the size of PBIT fail data
 *                      (see LFRTest) 
 *
 * Returns:     DRV_OK  
 *              DRV_A429_RX_LFR_FIFO_TEST_FAIL
 *
 * Notes:       
 *  1) All Four Arinc Rx LFR Ch are independently tested 
 *  2) On the first test failure of a Rx Ch, ->ResultCode is updated 
 *     with DRV_A429_RX_LFR_FIFO_TEST_FAIL and the LFRTest result data is returned
 *     to identify which Ch and Test failed in LFRTest[] struct  
 *  3) System Status of each Rx Ch is independently determined and set in 
 *     m_Arinc429MgrBlock.status.Rx[i].Status
 *  4) If test fails on a ch, test of other ch still will be processed. 
 *
 *  5) Forces Arinc429 Rx to be dsb to access LFR and leaves Arinc429 Rx dsb !
 *  6) Forces LFR back to default "F" when completed.  Expects User  Arinc Cfg
 *     to be applied later. 
 *  
 *  7) Overlays ARINC_DRV_PBIT_RX_LFR_FIFO_TEST_RESULT on to *pdest to return  
 *     both LFR and FIFO test results
 *
 *****************************************************************************/
static RESULT Arinc429DrvTest_LFR (UINT8 *pdest, UINT16 *psize)
{
   // Local Data
   FPGA_RX_REG_PTR pfpgaRxReg;   // Current Rx Ch being tested 
   ARINC_DRV_PBIT_RX_LFR_TEST_RESULT LFRTest; // Summary of FIFO test of all Rx Ch.  To 
                                              //   be returned if at least one FIFO test 
                                              //   fails
   ARINC_DRV_PBIT_TEST_RESULT *OverAllTest_Result_ptr;
   ARINC_DRV_PBIT_FAIL_DATA   *pFailData;
   UINT16 i;
   UINT16 k; 
   UINT16 *pLFR; 

   OverAllTest_Result_ptr = (ARINC_DRV_PBIT_TEST_RESULT *) pdest;
   pFailData = &OverAllTest_Result_ptr->failData;
   memset ( &LFRTest, 0, sizeof(ARINC_DRV_PBIT_RX_LFR_TEST_RESULT) ); 
  
   LFRTest.ResultCode = DRV_OK;  // Init Overall status of all LFR test of all Rx Ch
   for ( i = 0; i < FPGA_MAX_RX_CHAN; i++ ) 
   {
      LFRTest.bTestPass[i] = TRUE;   // Init All Test to Pass for all Ch 
   }
  
   for (i = 0; i < FPGA_MAX_RX_CHAN; i++)
   {
      pfpgaRxReg = (FPGA_RX_REG_PTR ) &fpgaRxReg[i];
      // Must disable Rx Ch before updating to LFR is possible 
      FPGA_W(pfpgaRxReg->Configuration, (FPGA_R(pfpgaRxReg->Configuration) & ~RCR_429_ENABLE) 
                                              | (CR_429_DISABLE_VAL * RCR_429_ENABLE)); 

      // Get current Ch LFR Address 
      pLFR = (UINT16 *) pfpgaRxReg->lfr; 

      // Loop thru all of LFR.  Note: LFR Reg only lower 4 nibble accessing (0x0 to 0xf)
      for ( k = 0; k < ARINC429_MAX_LABELS; k++ ) 
      {
         *(pLFR + k) = (k & 0x000F); 
      }
    
      // Loop thru and verify all LFR locations.   
      for ( k = 0; k < ARINC429_MAX_LABELS; k++ ) 
      {
         if ( (*(pLFR + k) != STPU((k & 0x000F), eTp429Lfr1) ) )
         {
            LFRTest.bTestPass[i] = FALSE; 
            LFRTest.ResultCode = DRV_A429_RX_LFR_FIFO_TEST_FAIL; 
            m_Arinc429HW.Rx[i].Status = ARINC429_DRV_STATUS_FAULTED_PBIT;
            if (!fpgaLogged)
            {
              pFailData->chan = i;
              pFailData->expLbl = k;
              pFailData->readLbl = *(pLFR + k);
              Arinc429DrvLogFpgaRegs(pFailData);
            }

#ifdef A429_TEST_OUTPUT
/*vcast_dont_instrument_start*/
            sprintf (GSE_OutLine, "\r\nArinc429DrvTest_LFR: LFR FIFO Fail cnt up lbl %d", k);
            GSE_PutLine(GSE_OutLine); 
            Arinc429DrvDumpRegs();
/*vcast_dont_instrument_end*/
#endif
            break; // Break Out of For Loop for this ch
         }
      }
    
      // Peform a count down - SCR 495 Additional processing request by HW
      // Ok.. This test could be combined with above, later.  
      if ( LFRTest.bTestPass[i] == TRUE ) 
      {
         // Get current Ch LFR Address 
         pLFR = (UINT16 *) pfpgaRxReg->lfr; 
    
         // Loop thru all of LFR.  Note: LFR Reg only lower 4 nibble accessing (0x0 to 0xf)
         for ( k = 0; k < ARINC429_MAX_LABELS; k++ ) 
         {
            // Send Count Down
            *(pLFR + k) = ((~k) & 0x000F); 
         }

         // Loop thru and verify all LFR locations.   
         for ( k = 0; k < ARINC429_MAX_LABELS; k++ ) 
         {
            if ( (*(pLFR + k) != STPU( ((~k) & 0x000F), eTp429Lfr2) ))
            {
               LFRTest.bTestPass[i] = FALSE; 
               LFRTest.ResultCode = DRV_A429_RX_LFR_FIFO_TEST_FAIL; 
               m_Arinc429HW.Rx[i].Status = ARINC429_DRV_STATUS_FAULTED_PBIT;         
               if (!fpgaLogged)
               {
                 pFailData->chan = i;
                 pFailData->expLbl = (~k) & 0x000F;
                 pFailData->readLbl = *(pLFR + k);
                 Arinc429DrvLogFpgaRegs(pFailData);
               }

#ifdef A429_TEST_OUTPUT
/*vcast_dont_instrument_start*/
               sprintf (GSE_OutLine, "\r\nArinc429DrvTest_LFR: LFR FIFO Fail cnt dn lbl %d", k);
               GSE_PutLine(GSE_OutLine); 
               Arinc429DrvDumpRegs();
/*vcast_dont_instrument_end*/
#endif
               break; // Break Out of For Loop for this ch
            }
         }
      }
    
      // Attempt to clear all LFR locations back to "f" the default. 
      for ( k = 0; k < ARINC429_MAX_LABELS; k++ ) 
      {
         *(pLFR + k) = 0xF; 
      }
   } // End of i loop thru all Independent Rx Ch 

   // Always return current results
   memcpy ( &OverAllTest_Result_ptr->bTestPassLFR[0], &LFRTest.bTestPass[0], 
            sizeof(BOOLEAN) * FPGA_MAX_RX_CHAN); 
  
   if (LFRTest.ResultCode != DRV_OK) 
   {
      // Will OR in the ResultCode to Overall Testing 
      OverAllTest_Result_ptr->ResultCode |= LFRTest.ResultCode; 

      // Set Return Data Size
      *psize = sizeof(ARINC_DRV_PBIT_TEST_RESULT); 
   }
  
   return (LFRTest.ResultCode); 
}

/******************************************************************************
 * Function:    Arinc429DrvTest_LoopBack 
 *
 * Description: Loop back test of Arinc429 
 *
 * Parameters:  pdest - ptr to return failure data 
 *              psize - ptr to return size of failure data
 *
 * Returns:     DRV_OK 
 *              DRV_A429_TX_FIFO_TEST_FAIL 
 *
 * Notes: 
 *   1) This test mirrors FPGA hw verification testing 
 *   2) 02/01/2010 There is an inversion in the Rx data from Tx. Test code 
 *      updated to invert data before compare ! 
 *
 *****************************************************************************/
static RESULT Arinc429DrvTest_LoopBack (UINT8 *pdest, UINT16 *psize)
{
   // Local Data
   FPGA_TX_REG_PTR pfpgaTxReg[FPGA_MAX_TX_CHAN]; 
   UINT32 i, j; 
   UINT16 startOrder, endOrder; 
   UINT16 lsw, msw; 
   BOOLEAN bStatusOk; 
  
   UINT16 testOrder[FPGA_MAX_RX_CHAN]; 
  
   ARINC_DRV_PBIT_LOOPBACK_TEST_RESULT LoopBackTest; 
                                              // Summary of FIFO test of all Rx Ch.  To 
                                              //   be returned if at least one FIFO test 
                                              //   fails
  
   ARINC_DRV_PBIT_TEST_RESULT *OverAllTest_Result_ptr; 
   ARINC_DRV_PBIT_FAIL_DATA *pFailData;
   OverAllTest_Result_ptr = (ARINC_DRV_PBIT_TEST_RESULT *) pdest;
   pFailData = &OverAllTest_Result_ptr->failData;
 
#define TX1_LSW 0xFFFF
#define TX1_MSW 0x3333
  
   testOrder[0] = ARINC_CHAN_2; 
   testOrder[1] = ARINC_CHAN_3; 
   testOrder[2] = ARINC_CHAN_0;
   testOrder[3] = ARINC_CHAN_1; 
  
   memset ( &LoopBackTest, 0, sizeof(ARINC_DRV_PBIT_LOOPBACK_TEST_RESULT) ); 
  
   LoopBackTest.ResultCode = DRV_A429_TX_RX_LOOP_FAIL; 
                                     // Init to fail.  See logic below to minimize 
                                     //    code coverage required. 
                                     
   // Set Return Data Size 
   *psize = sizeof(ARINC_DRV_PBIT_TEST_RESULT); 
  
   for ( i = 0; i < FPGA_MAX_TX_CHAN; i++ ) 
   {
      LoopBackTest.bTestPassTxLoopBack[i] = TRUE; 
   }
   for ( i = 0; i < FPGA_MAX_RX_CHAN; i++ )
   {
      LoopBackTest.bTestPassRxLoopBack[i] = TRUE;
   }
  
   // Set speed for all chan 
   // Tx1, Rx3, Rx4 = HIGH 
   // Tx2, Rx1, Rx2 = LOW 
   FPGA_W( FPGA_429_COMMON_CONTROL,  (CCR_429_TX2_RATE * CCR_429_LOW_SPEED_VAL)
                                   | (CCR_429_TX1_RATE * CCR_429_HIGH_SPEED_VAL)
                                   | (CCR_429_RX4_RATE * CCR_429_HIGH_SPEED_VAL)
                                   | (CCR_429_RX3_RATE * CCR_429_HIGH_SPEED_VAL)
                                   | (CCR_429_RX2_RATE * CCR_429_LOW_SPEED_VAL)
                                   | (CCR_429_RX1_RATE * CCR_429_LOW_SPEED_VAL) ); 
   // Set LBR for all chan 
   // Tx1, Tx2 = Quiet
   // Rx1, Rx2 = From Tx2 
   // Rx3, Rx4 = From Tx1 
   // NOTE:  Rx loop back will work for tx Normal or Quiet.  Use Quiet to prevent 
   //        actual output of data 
   FPGA_W( FPGA_429_LOOPBACK_CONTROL,  (LBR_429_TX1_DRIVE * LBR_429_TX_QUIET_VAL) 
                                     | (LBR_429_TX2_DRIVE * LBR_429_TX_QUIET_VAL)
                                     | (LBR_429_RX1_SOURCE * LBR_429_RX_FROM_TX2_VAL)
                                     | (LBR_429_RX2_SOURCE * LBR_429_RX_FROM_TX2_VAL)
                                     | (LBR_429_RX3_SOURCE * LBR_429_RX_FROM_TX1_VAL)
                                     | (LBR_429_RX4_SOURCE * LBR_429_RX_FROM_TX1_VAL) );
                                   
   // Set all Rx Ch Enb and EVEN parity 
   FPGA_W ( FPGA_429_RX1_CONFIG,  ( RCR_429_PARITY_SELECT * CR_429_EVEN_PARITY_VAL )
                                | ( RCR_429_ENABLE * CR_429_ENABLE_VAL ) 
                                | ( RCR_429_LABEL_SWAP * RCR_429_BITS_NOT_SWAP_VAL) ); 
 
   FPGA_W ( FPGA_429_RX2_CONFIG,  ( RCR_429_PARITY_SELECT * CR_429_EVEN_PARITY_VAL )
                                | ( RCR_429_ENABLE * CR_429_ENABLE_VAL ) 
                                | ( RCR_429_LABEL_SWAP * RCR_429_BITS_NOT_SWAP_VAL) ); 
                               
   FPGA_W ( FPGA_429_RX3_CONFIG,  ( RCR_429_PARITY_SELECT * CR_429_EVEN_PARITY_VAL )
                                | ( RCR_429_ENABLE * CR_429_ENABLE_VAL ) 
                                | ( RCR_429_LABEL_SWAP * RCR_429_BITS_NOT_SWAP_VAL) ); 
                               
   FPGA_W ( FPGA_429_RX4_CONFIG,  ( RCR_429_PARITY_SELECT * CR_429_EVEN_PARITY_VAL )
                                | ( RCR_429_ENABLE * CR_429_ENABLE_VAL ) 
                                | ( RCR_429_LABEL_SWAP * RCR_429_BITS_NOT_SWAP_VAL) ); 

   // Set all Tx Ch Enb and EVEN parity and BIT swap to NO 
   FPGA_W ( FPGA_429_TX1_CONFIG,  ( TCR_429_BIT_FLIP * TCR_429_NO_BIT_FLIP_VAL )
                                | ( TCR_429_PARITY_SELECT * CR_429_EVEN_PARITY_VAL ) 
                                | ( TCR_429_ENABLE * CR_429_ENABLE_VAL ) );
                               
   FPGA_W ( FPGA_429_TX2_CONFIG,  ( TCR_429_BIT_FLIP * TCR_429_NO_BIT_FLIP_VAL )
                                | ( TCR_429_PARITY_SELECT * CR_429_EVEN_PARITY_VAL ) 
                                | ( TCR_429_ENABLE * CR_429_ENABLE_VAL ) );
                                
                                
   // Delay at least one 32-bit word time before starting test to avoid "collision" with 
   //   incoming data on the wire.
   
   // Wait for data to be looped back worst case 12.5 kHz one 32 bit word 
   //   at 12.5 kHz 32 bit + 4 bit gap -> 2.8 msec
   TTMR_Delay(TICKS_PER_10msec); 
   
   // Read Status to clear any errors 
   for (i = 0; i < FPGA_MAX_RX_CHAN; i++)
   {
     UINT32 parityErrCnt = 0;
     UINT32 framingErrCnt = 0;
     UINT32 intCount = 0;
     UINT16 FPGA_Status = 0;

     // dummy read to clear
     Arinc429DrvCheckBITStatus ( &parityErrCnt, &framingErrCnt, &intCount, &FPGA_Status, i );
     msw = FPGA_R(fpgaRxReg[i].FIFO_Status);

     // add returned error counts to total
     pFailData->parityErr += parityErrCnt;
     pFailData->frameErr  += framingErrCnt;

#ifdef A429_TEST_OUTPUT
/*vcast_dont_instrument_start*/
     if ( (parityErrCnt != 0) || (framingErrCnt != 0) )
     {
       sprintf (GSE_OutLine,
            "\r\nArinc429DrvTest_LoopBack: Clear Read ch %d: SR=0x%04x, FF=0x%04x, PE=%d, FE=%d",
             i, FPGA_Status, msw, parityErrCnt, framingErrCnt);
       GSE_PutLine(GSE_OutLine);
     }
/*vcast_dont_instrument_end*/
#endif
   }

   // Check Tx FIFOs are empty before starting
   //   v9 and v12 status, fifo_status, lsw and msw are identical 
   pfpgaTxReg[ARINC_CHAN_0] = (FPGA_TX_REG_PTR) &fpgaTxReg_v12[ARINC_CHAN_0];  
   pfpgaTxReg[ARINC_CHAN_1] = (FPGA_TX_REG_PTR) &fpgaTxReg_v12[ARINC_CHAN_1]; 
  
   startOrder = 0; 
   endOrder = 1;
  
   for ( j = ARINC_CHAN_0; j < FPGA_MAX_TX_CHAN; j++ )
   {
      LoopBackTest.bTestPassTxLoopBack[j] = FALSE; 
      if ( (FPGA_R(pfpgaTxReg[j]->FIFO_Status) & TSR_429_FIFO_EMPTY) == TSR_429_FIFO_EMPTY )
      {
         // Perform loop back 
         FPGA_W(pfpgaTxReg[j]->lsw, TX1_LSW); 
         FPGA_W(pfpgaTxReg[j]->msw, TX1_MSW); 
      
         // Wait for data to be looped back worst case 12.5 kHz one 32 bit word 
         TTMR_Delay(TICKS_PER_10msec); 
  
         // Check Ch 2 and 3 
         for ( i = startOrder; i <= endOrder; i++ )
         {
            LoopBackTest.bTestPassRxLoopBack[testOrder[i]] = FALSE; 
            if ( (FPGA_R(fpgaRxReg[testOrder[i]].FIFO_Status) & RFSR_429_FIFO_EMPTY) != 
                  RFSR_429_FIFO_EMPTY )
            {
               lsw = (UINT16) (FPGA_R(fpgaRxReg[testOrder[i]].lsw)); 
               msw = (UINT16) (FPGA_R(fpgaRxReg[testOrder[i]].msw)); 
          
               if ( (lsw == TX1_LSW) && (msw == (UINT16)STPU( TX1_MSW, eTp429Rx1633)) )
               {
                  LoopBackTest.bTestPassRxLoopBack[testOrder[i]] = TRUE; 
               }
               else
               {
                 if (!fpgaLogged)
                 {
                   pFailData->chan = testOrder[i];
                   pFailData->expLsw = TX1_LSW;
                   pFailData->expMsw = TX1_MSW;
                   pFailData->rcvdLsw = lsw;
                   pFailData->rcvdMsw = msw;
                   Arinc429DrvLogFpgaRegs(pFailData);
                 }
#ifdef A429_TEST_OUTPUT
/*vcast_dont_instrument_start*/
                 sprintf (GSE_OutLine,
                    "\r\nArinc429DrvTest_LoopBack: LoopBack RX Fail"
                    " ch %d: wrl=0x%04x, rdl=0x%04x, wrm=0x%04x, rdm=0x%04x",
                        testOrder[i], TX1_LSW, lsw, TX1_MSW, msw);
                 GSE_PutLine(GSE_OutLine); 
                 Arinc429DrvDumpRegs();
/*vcast_dont_instrument_end*/
#endif
               }
            }
            else
            {
              ++pFailData->rxFifoEmpty;     // Error: FIFO is empty
              Arinc429DrvLogFpgaRegs(pFailData);
#ifdef A429_TEST_OUTPUT
/*vcast_dont_instrument_start*/
              sprintf (GSE_OutLine,
                  "\r\nArinc429DrvTest_LoopBack: LoopBack RX Fail ch %d: FIFO is Empty", 
                  testOrder[i]);
              GSE_PutLine(GSE_OutLine); 
              Arinc429DrvDumpRegs();
/*vcast_dont_instrument_end*/
#endif
            }
         }
      
         if ( ( LoopBackTest.bTestPassRxLoopBack[testOrder[startOrder]] == 
                (BOOLEAN)STPU( TRUE, eTp429Tx3106 )) && 
              ( LoopBackTest.bTestPassRxLoopBack[testOrder[endOrder]] == TRUE ) )
         {
            LoopBackTest.bTestPassTxLoopBack[j] = TRUE; 
         }
#ifdef A429_TEST_OUTPUT
/*vcast_dont_instrument_start*/
         else
         {
           sprintf (GSE_OutLine,
               "\r\nArinc429DrvTest_LoopBack: LoopBack TX Fail: RX ch %d: %d, RX ch %d: %d", 
               testOrder[startOrder], LoopBackTest.bTestPassRxLoopBack[testOrder[startOrder]],
               testOrder[endOrder],   LoopBackTest.bTestPassRxLoopBack[testOrder[endOrder]]);
           GSE_PutLine(GSE_OutLine); 
           Arinc429DrvDumpRegs();
         }
/*vcast_dont_instrument_end*/
#endif
      }
      else
      {
        ++pFailData->txFifoNotEmpty;     // Error: FIFO is Not empty
        Arinc429DrvLogFpgaRegs(pFailData);
#ifdef A429_TEST_OUTPUT
/*vcast_dont_instrument_start*/
        sprintf (GSE_OutLine,
            "\r\nArinc429DrvTest_LoopBack: LoopBack ch %d: FAIL NOT TSR_429_FIFO_EMPTY", j);
        GSE_PutLine(GSE_OutLine); 
        Arinc429DrvDumpRegs();
/*vcast_dont_instrument_end*/
#endif
      }

      startOrder = i;
      endOrder = 3;
   }

   // Update return data. 
   memcpy ( &OverAllTest_Result_ptr->bTestPassTxLoopBack[0], 
              &LoopBackTest.bTestPassTxLoopBack[0], sizeof(BOOLEAN) * FPGA_MAX_TX_CHAN); 
   memcpy ( &OverAllTest_Result_ptr->bTestPassRxLoopBack[0], 
              &LoopBackTest.bTestPassRxLoopBack[0], sizeof(BOOLEAN) * FPGA_MAX_RX_CHAN); 

   // Update return value.  Use following logic to minimize code coverage.   
   bStatusOk = TRUE; 
   for ( i = 0; i < FPGA_MAX_RX_CHAN; i++ ) 
   {
      bStatusOk &= LoopBackTest.bTestPassRxLoopBack[i]; 
      if ( LoopBackTest.bTestPassRxLoopBack[i] == FALSE ) 
      {
        m_Arinc429HW.Rx[i].Status = ARINC429_DRV_STATUS_FAULTED_PBIT; 
      }
   }
   for ( i = 0; i < FPGA_MAX_TX_CHAN; i++ )
   {
      bStatusOk &= LoopBackTest.bTestPassTxLoopBack[i]; 
      if ( LoopBackTest.bTestPassTxLoopBack[i] == FALSE ) 
      {
        m_Arinc429HW.Tx[i].Status = ARINC429_DRV_STATUS_FAULTED_PBIT; 
      }
   }

#ifdef WIN32
   /*vcast_dont_instrument_start*/
   bStatusOk = TRUE;
   for ( i = 0; i < FPGA_MAX_RX_CHAN; i++ ) 
   {
       m_Arinc429HW.Rx[i].Status = ARINC429_DRV_STATUS_OK;
   }
   /*vcast_dont_instrument_end*/
#endif

   if (bStatusOk == TRUE ) 
   {
      LoopBackTest.ResultCode = DRV_OK; 
   }
  
   // Update overall result code for Arinc429 PBIT 
   OverAllTest_Result_ptr->ResultCode |= LoopBackTest.ResultCode; 
   
   // Wait for data to be looped back worst case 12.5 kHz one 32 bit word 
   //   at 12.5 kHz 32 bit + 4 bit gap -> 2.8 msec
   TTMR_Delay(TICKS_PER_10msec); 
   
   // Reconfigure ARINC FPGA with startup defaults 
   Arinc429DrvRestoreStartupCfg();
  
   return (LoopBackTest.ResultCode); 
}
 
/*************************************************************************
 *  MODIFICATIONS
 *    $History: arinc429.c $
 * 
 * *****************  Version 106  *****************
 * User: Melanie Jutras Date: 12-11-09   Time: 1:20p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 File Format Errors
 * 
 * *****************  Version 105  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 104  *****************
 * User: Jim Mood     Date: 2/24/12    Time: 10:32a
 * Updated in $/software/control processor/code/drivers
 * SCR 1114 - Re labeled after v1.1.1 release
 * 
 * *****************  Version 102  *****************
 * User: Jeff Vahue   Date: 12/05/11   Time: 7:39p
 * Updated in $/software/control processor/code/drivers
 * SCR# 1106 - Force A429 to valid during PBIT
 * 
 * *****************  Version 101  *****************
 * User: Contractor2  Date: 9/23/11    Time: 4:17p
 * Updated in $/software/control processor/code/drivers
 * SCR #771 Error: A429 PBIT - occasional A429_RX_FIFO_TEST_FAIL reported
 * Added additional log data and if/else.
 * 
 * *****************  Version 100  *****************
 * User: Contractor2  Date: 9/12/11    Time: 11:01a
 * Updated in $/software/control processor/code/drivers
 * SCR #771 Error: A429 PBIT - occasional A429_RX_FIFO_TEST_FAIL reported
 * 
 * *****************  Version 99  *****************
 * User: Contractor2  Date: 9/09/11    Time: 4:33p
 * Updated in $/software/control processor/code/drivers
 * SCR #771 Error: A429 PBIT - occasional A429_RX_FIFO_TEST_FAIL reported
 * 
 * *****************  Version 98  *****************
 * User: Contractor2  Date: 9/09/11    Time: 3:33p
 * Updated in $/software/control processor/code/drivers
 * SCR #771 Error: A429 PBIT - occasional A429_RX_FIFO_TEST_FAIL reported
 * 
 * *****************  Version 97  *****************
 * User: Contractor2  Date: 9/09/11    Time: 2:53p
 * Updated in $/software/control processor/code/drivers
 * SCR #771 Error: A429 PBIT - occasional A429_RX_FIFO_TEST_FAIL reported
 * Added FPGA register snapshot and fail counters to PBIT log.
 * 
 * *****************  Version 96  *****************
 * User: Contractor2  Date: 9/07/11    Time: 1:14p
 * Updated in $/software/control processor/code/drivers
 * SCR #771 Error: A429 PBIT - occasional A429_RX_FIFO_TEST_FAIL reported
 * Added FPGA register and error GSE output statements.
 * 
 * *****************  Version 95  *****************
 * User: Contractor2  Date: 9/06/11    Time: 2:48p
 * Updated in $/software/control processor/code/drivers
 * SCR #1050 Error - Arinc429 PBIT failure after an assert
 * 
 * *****************  Version 94  *****************
 * User: Contractor2  Date: 6/20/11    Time: 11:40a
 * Updated in $/software/control processor/code/drivers
 * SCR #988 Enhancement - A429 - Rx/Tx FIFO Overrun Test Status
 * 
 * *****************  Version 93  *****************
 * User: Contractor V&v Date: 6/03/11    Time: 11:58a
 * Updated in $/software/control processor/code/drivers
 * SCR #1020 Error - Arinc429 Tx Operation
 * 
 * *****************  Version 92  *****************
 * User: Jeff Vahue   Date: 11/10/10   Time: 11:04p
 * Updated in $/software/control processor/code/drivers
 * SCR# 848 - Code Coverage
 * 
 * *****************  Version 91  *****************
 * User: Jeff Vahue   Date: 11/06/10   Time: 10:01p
 * Updated in $/software/control processor/code/drivers
 * SCR# 848 - Code Coverage
 * 
 * *****************  Version 90  *****************
 * User: Jeff Vahue   Date: 11/05/10   Time: 10:04p
 * Updated in $/software/control processor/code/drivers
 * SCR# 848
 * 
 * *****************  Version 89  *****************
 * User: Jeff Vahue   Date: 11/05/10   Time: 10:02p
 * Updated in $/software/control processor/code/drivers
 * SCR# 848 - Code Coverage
 * 
 * *****************  Version 88  *****************
 * User: Jeff Vahue   Date: 11/05/10   Time: 9:48p
 * Updated in $/software/control processor/code/drivers
 * SCR# 848 - Code Coverage
 * 
 * *****************  Version 87  *****************
 * User: John Omalley Date: 10/27/10   Time: 3:51p
 * Updated in $/software/control processor/code/drivers
 * SCR 967 - Fixed TX Status being stored in the receive structure
 * 
 * *****************  Version 86  *****************
 * User: Contractor2  Date: 9/24/10    Time: 3:02p
 * Updated in $/software/control processor/code/drivers
 * SCR #882 WD - Old Code for pulsing the WD should be removed
 * 
 * *****************  Version 85  *****************
 * User: John Omalley Date: 9/10/10    Time: 10:00a
 * Updated in $/software/control processor/code/drivers
 * SCR 858 - Added FATAL to defaults
 * 
 * *****************  Version 84  *****************
 * User: Peter Lee    Date: 8/30/10    Time: 3:21p
 * Updated in $/software/control processor/code/drivers
 * SCR #806 Code Review Updates
 * 
 * *****************  Version 83  *****************
 * User: Peter Lee    Date: 8/03/10    Time: 11:37a
 * Updated in $/software/control processor/code/drivers
 * SCR #759 Proper use of "pPtr++"
 * 
 * *****************  Version 82  *****************
 * User: John Omalley Date: 7/29/10    Time: 7:50p
 * Updated in $/software/control processor/code/drivers
 * SCR 754 - Added a FIFO flush after initialization
 * 
 * *****************  Version 81  *****************
 * User: Peter Lee    Date: 7/23/10    Time: 4:00p
 * Updated in $/software/control processor/code/drivers
 * SCR #732 Updating Arinc429 Chan Status when Tx-Rx LoopBack fails. 
 * 
 * *****************  Version 80  *****************
 * User: Peter Lee    Date: 7/23/10    Time: 1:39p
 * Updated in $/software/control processor/code/drivers
 * SCR #688 Remove FPGA Ver 9 support.  SW will only be compatible with
 * Ver 12.
 * 
 * *****************  Version 79  *****************
 * User: Peter Lee    Date: 7/23/10    Time: 10:30a
 * Updated in $/software/control processor/code/drivers
 * SCR #494 Arinc429 Tx-Rx- Loop Failure and Data Inversion. 
 * 
 * *****************  Version 78  *****************
 * User: Jeff Vahue   Date: 7/17/10    Time: 5:46p
 * Updated in $/software/control processor/code/drivers
 * SCR# 707 - Add TestPoints for code coverage.
 * 
 * *****************  Version 77  *****************
 * User: Jeff Vahue   Date: 6/27/10    Time: 2:18p
 * Updated in $/software/control processor/code/drivers
 * Typos in comments and fix 429 Tx failure in WIN32 emulation
 * 
 * *****************  Version 76  *****************
 * User: John Omalley Date: 6/08/10    Time: 12:22p
 * Updated in $/software/control processor/code/drivers
 * SCR 627 - Updated for LJ60
 * 
 * *****************  Version 75  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:54p
 * Updated in $/software/control processor/code/drivers
 * SCR #587 Change TmTaskCreate to return void
 * 
 * *****************  Version 74  *****************
 * User: Contractor2  Date: 5/06/10    Time: 2:09p
 * Updated in $/software/control processor/code/drivers
 * SCR #579 Change LogWriteSys to return void
 * 
 * *****************  Version 73  *****************
 * User: Contractor2  Date: 5/03/10    Time: 11:36a
 * Updated in $/software/control processor/code/drivers
 * SCR 577: SRS-1998 - Create fault log for any out of range BCD digit.
 * 
 * *****************  Version 72  *****************
 * User: Jeff Vahue   Date: 4/28/10    Time: 5:42p
 * Updated in $/software/control processor/code/drivers
 * SCR #573 - Startup WD changes
 * 
 * *****************  Version 71  *****************
 * User: Jeff Vahue   Date: 4/12/10    Time: 5:43p
 * Updated in $/software/control processor/code/drivers
 * SCR #541 - all XXX_CreateLogs functions are now dependent on
 * GENERATE_SYS_LOGS
 * 
 * *****************  Version 70  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:08p
 * Updated in $/software/control processor/code/drivers
 * SCR #317 Implement safe strncpy
 * 
 * *****************  Version 69  *****************
 * User: Peter Lee    Date: 4/07/10    Time: 2:33p
 * Updated in $/software/control processor/code/drivers
 * SCR #495 Additional FPGA PBIT requested by HW
 * 
 * *****************  Version 68  *****************
 * User: Peter Lee    Date: 3/29/10    Time: 7:14p
 * Updated in $/software/control processor/code/drivers
 * SCR #518 Set No Bit Swap for FPGA_429_TX1_CONFIG register.  
 * 
 * *****************  Version 67  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:36p
 * Updated in $/software/control processor/code/drivers
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 * 
 * *****************  Version 66  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/drivers
 * SCR# 483 - Function Names
 * 
 * *****************  Version 65  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:07p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 64  *****************
 * User: Jeff Vahue   Date: 2/19/10    Time: 12:57p
 * Updated in $/software/control processor/code/drivers
 * SCR# 455 - remove LINT issues
 * 
 * *****************  Version 63  *****************
 * User: Peter Lee    Date: 2/09/10    Time: 4:15p
 * Updated in $/software/control processor/code/drivers
 * SCR #433 init *psize in Arinc429_Test_LoopBack()
 * 
 * *****************  Version 62  *****************
 * User: Jeff Vahue   Date: 2/09/10    Time: 3:05p
 * Updated in $/software/control processor/code/drivers
 * SCR# 433 - init result_loopback
 * 
 * *****************  Version 61  *****************
 * User: Peter Lee    Date: 2/04/10    Time: 3:15p
 * Updated in $/software/control processor/code/drivers
 * SCR #433 Add Tx-Rx Loopback for PBIT.  Add code to not support FPGA
 * vers below 12.  TX_FIFO_EMPTY flag changed to TX_FIFO_NOT_EMPTY in Ver
 * 12.  
 * 
 * *****************  Version 60  *****************
 * User: Peter Lee    Date: 2/02/10    Time: 4:01p
 * Updated in $/software/control processor/code/drivers
 * SCR #433 Init Arinc429 LBR Register 2nd update. 
 * 
 * *****************  Version 59  *****************
 * User: Peter Lee    Date: 2/01/10    Time: 4:26p
 * Updated in $/software/control processor/code/drivers
 * SCR #433 Init Arinc429 LBR Register
 * 
 * *****************  Version 58  *****************
 * User: Peter Lee    Date: 2/01/10    Time: 3:25p
 * Updated in $/software/control processor/code/drivers
 * SCR #432 Add Arinc429 Tx-Rx LoopBack PBIT
 * 
 * *****************  Version 57  *****************
 * User: Jeff Vahue   Date: 2/01/10    Time: 12:44p
 * Updated in $/software/control processor/code/drivers
 * SCR# 196 - remove unused function as a result of this SCR
 * Arinc429_DisableAnyStreamingDebugOuput
 * 
 * *****************  Version 56  *****************
 * User: Jeff Vahue   Date: 1/29/10    Time: 3:29p
 * Updated in $/software/control processor/code/drivers
 * SCR# 364: Preliminary Code Review Findings, correct Channel timeout
 * sysLog size
 * 
 * *****************  Version 55  *****************
 * User: Peter Lee    Date: 1/20/10    Time: 4:48p
 * Updated in $/software/control processor/code/drivers
 * SCR #401 Error - Addition of Arinc Tx PBIT processing causes watchdog
 * 
 * *****************  Version 54  *****************
 * User: John Omalley Date: 1/15/10    Time: 5:29p
 * Updated in $/software/control processor/code/drivers
 * SCR 396 
 * * Added Interface Validation
 * 
 * *****************  Version 53  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 5:01p
 * Updated in $/software/control processor/code/drivers
 * SCR# 397
 * 
 * *****************  Version 52  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:21p
 * Updated in $/software/control processor/code/drivers
 * SCR 18, SCR 371
 * 
 * *****************  Version 51  *****************
 * User: Peter Lee    Date: 12/23/09   Time: 10:17a
 * Updated in $/software/control processor/code/drivers
 * SCR #383 Reliability of reading FPGA ver on first read and returning
 * BIT OR Arinc429 PBIT result to GSE PBIT display. 
 * 
 * *****************  Version 50  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 3:20p
 * Updated in $/software/control processor/code/drivers
 * SCR# 326 Cleanup
 * 
 * *****************  Version 49  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 2:11p
 * Updated in $/software/control processor/code/drivers
 * SCR# 326
 * 
 * *****************  Version 48  *****************
 * User: Peter Lee    Date: 12/21/09   Time: 6:19p
 * Updated in $/software/control processor/code/drivers
 * SCR #380 Implement Real Time Sys Condition 
 * 
 * *****************  Version 47  *****************
 * User: Peter Lee    Date: 12/15/09   Time: 2:47p
 * Updated in $/software/control processor/code/drivers
 * SCR #318 Arinc429 Tx Implementation
 * 
 * *****************  Version 46  *****************
 * User: Peter Lee    Date: 12/14/09   Time: 1:56p
 * Updated in $/software/control processor/code/drivers
 * SCR #361 Proper use of TTMR_GetHSTickCount() for delays. 
 * 
 * *****************  Version 45  *****************
 * User: Peter Lee    Date: 12/14/09   Time: 1:19p
 * Updated in $/software/control processor/code/drivers
 * Add backward supportability for pre-Version 12 FPGA firmware.  
 * 
 * *****************  Version 43  *****************
 * User: Peter Lee    Date: 12/10/09   Time: 9:55a
 * Updated in $/software/control processor/code/drivers
 * SCR #358 Updated Arinc429 Tx Flag Processing 
 * 
 * *****************  Version 42  *****************
 * User: Peter Lee    Date: 12/04/09   Time: 5:24p
 * Updated in $/software/control processor/code/drivers
 * SCR #358 New FPGA changes to INT MASK and ISR Status reg
 * 
 * *****************  Version 41  *****************
 * User: Peter Lee    Date: 12/02/09   Time: 2:18p
 * Updated in $/software/control processor/code/drivers
 * SCR #350 Misc Code Review Items
 * 
 * *****************  Version 40  *****************
 * User: Contractor V&v Date: 11/30/09   Time: 5:34p
 * Updated in $/software/control processor/code/drivers
 * SCR 106	XXXUserTable.c consistency
 * 
 * *****************  Version 39  *****************
 * User: Contractor V&v Date: 11/18/09   Time: 3:19p
 * Updated in $/software/control processor/code/drivers
 * Fix  for SCR 172
 * Implement SCR 196
 * 
 * *****************  Version 38  *****************
 * User: Peter Lee    Date: 11/12/09   Time: 10:13a
 * Updated in $/software/control processor/code/drivers
 * SCR #315 PWEH SEU Processing Support
 * 
 * *****************  Version 37  *****************
 * User: Peter Lee    Date: 11/02/09   Time: 6:09p
 * Updated in $/software/control processor/code/drivers
 * Misc Code update to support WIN32
 * 
 * *****************  Version 36  *****************
 * User: Peter Lee    Date: 10/22/09   Time: 10:27a
 * Updated in $/software/control processor/code/drivers
 * SCR #171 Add correct System Status (Normal, Caution, Fault) enum to
 * UserTables
 * 
 * *****************  Version 35  *****************
 * User: Peter Lee    Date: 10/21/09   Time: 4:12p
 * Updated in $/software/control processor/code/drivers
 * SCR #229 Update Ch/Startup TimeOut to 1 sec from 1 msec resolution.
 * Updated GPB channel loss timeout to 1 msec from 1 sec resolution. 
 * 
 * *****************  Version 34  *****************
 * User: Peter Lee    Date: 10/05/09   Time: 1:09p
 * Updated in $/software/control processor/code/drivers
 * Update Arinc429_SensorTest() logic to use ChanStartupTime rather than
 * Ch DataLossTime.  
 * 
 * *****************  Version 33  *****************
 * User: Peter Lee    Date: 9/30/09    Time: 9:14a
 * Updated in $/software/control processor/code/drivers
 * SCR #283 Misc Code Review Updates
 * 
 * *****************  Version 32  *****************
 * User: Peter Lee    Date: 9/29/09    Time: 4:03p
 * Updated in $/software/control processor/code/drivers
 * SCR #283 Misc Code Review Updates
 * 
 * *****************  Version 31  *****************
 * User: Peter Lee    Date: 9/23/09    Time: 11:53a
 * Updated in $/software/control processor/code/drivers
 * SCR #268 Updates to _SensorTest() to include Ch Loss.  Have _ReadWord()
 * return FLOAT32
 * 
 * *****************  Version 30  *****************
 * User: Peter Lee    Date: 9/17/09    Time: 4:02p
 * Updated in $/software/control processor/code/drivers
 * SCR #262 Add ch to Arinc429 Startup and Data Loss Time Out Logs
 * 
 * *****************  Version 29  *****************
 * User: Peter Lee    Date: 9/15/09    Time: 6:02p
 * Updated in $/software/control processor/code/drivers
 * SCR #94, #95 PBIT returns log structure to Init Mgr
 * SCR #256 Misc Arinc 429 PBIT Log Updates
 * 
 * *****************  Version 28  *****************
 * User: Peter Lee    Date: 8/07/09    Time: 3:42p
 * Updated in $/software/control processor/code/drivers
 * SCR #241. Updates to System Logs
 * 
 * *****************  Version 26  *****************
 * User: Peter Lee    Date: 7/31/09    Time: 3:16p
 * Updated in $/software/control processor/code/drivers
 * SCR #221 Return byte cnt and not word cnt when returning Raw Data size.
 * 
 * *****************  Version 25  *****************
 * User: Peter Lee    Date: 6/29/09    Time: 2:20p
 * Updated in $/software/control processor/code/drivers
 * SCR #204 Misc Code Updates. 
 * 
 * *****************  Version 24  *****************
 * User: Peter Lee    Date: 4/30/09    Time: 9:37a
 * Updated in $/software/control processor/code/drivers
 * SCR #168 Use positive logic for Arinc429_ReadFIFOTask() when checking
 * for STREAMING mode in "if" statement. 
 * 
 * *****************  Version 23  *****************
 * User: Peter Lee    Date: 4/27/09    Time: 3:45p
 * Updated in $/control processor/code/drivers
 * SCR #168 Updates from preliminary code review. 
 * SCR #174 Fix unitialized variable in Arinc429_SensorTest() 
 * 
 * *****************  Version 22  *****************
 * User: Jim Mood     Date: 3/30/09    Time: 12:11p
 * Updated in $/control processor/code/drivers
 * SCR# 160 
 * 
 * *****************  Version 21  *****************
 * User: Peter Lee    Date: 2/02/09    Time: 4:06p
 * Updated in $/control processor/code/drivers
 * SCR #131 Misc Arinc429 Updates
 * Add logic to process _BITTask() and _ReadFIFOTask() only if FPGA Status
 * is OK. 
 * 
 * *****************  Version 20  *****************
 * User: Peter Lee    Date: 1/30/09    Time: 4:43p
 * Updated in $/control processor/code/drivers
 * SCR #131 Arinc429 Updates 
 * Add SSM Failure Detection to SensorTest()
 * 
 * *****************  Version 18  *****************
 * User: Peter Lee    Date: 1/23/09    Time: 2:54p
 * Updated in $/control processor/code/drivers
 * SCR #131 Arinc429 Updates. Add SensorValid().  
 * 
 * *****************  Version 17  *****************
 * User: Peter Lee    Date: 1/23/09    Time: 11:34a
 * Updated in $/control processor/code/drivers
 * SCR #131 Arinc429 Updates
 * Provide CBIT Health Counts
 * 
 * *****************  Version 16  *****************
 * User: Peter Lee    Date: 1/22/09    Time: 1:27p
 * Updated in $/control processor/code/drivers
 * SCR #131 Add arinc429.createlogs() func.  Read Rx Ch Status when Rx Ch
 * Enb to clear "Data Present" flag.  
 * 
 * *****************  Version 15  *****************
 * User: Peter Lee    Date: 1/21/09    Time: 11:26a
 * Updated in $/control processor/code/drivers
 * SCR #131 Add PBIT LFR Test 
 * 
 * *****************  Version 13  *****************
 * User: Peter Lee    Date: 1/20/09    Time: 6:24p
 * Updated in $/control processor/code/drivers
 * SCR #131 
 * Enb FIFO Half Full CBIT
 * Add Startup and Ch Loss TimeOut Logs
 * Add Sensor Filter LFR setup request to FPGA_Configure() 
 * Update Func Comments
 * Add Rev Headers / Footers 
 *
 *
 ***************************************************************************/

