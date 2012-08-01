#ifndef FPGA_H
#define FPGA_H
/******************************************************************************
            Copyright (C) 2009-2010 Pratt & Whitney Engine Services, Inc. 
                      Altair Engine Diagnostic Solutions
               All Rights Reserved. Proprietary and Confidential.

    File:        FPGA.h     
     
    Description: This file contains all the register definitions for the fpga.
    
VERSION
     $Revision: 29 $  $Date: 7/19/12 10:42a $
    
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "resultcodes.h"
#include "stddefs.h"
#include "SystemLog.h"

/******************************************************************************
                                 Package Defines
******************************************************************************/
#ifndef WIN32
  #define FPGA_BASE_ADDR            0x40000000
  #define FPGA_R(addr)              *(addr)
  #define FPGA_W(addr, val)         *(addr) = (val)
#else
  #include "HwSupport.h"
  #define FPGA_R(addr)              hw_FpgaRead( addr)
  #define FPGA_W(addr, val)         hw_FpgaWrite( addr, val)
#endif

// Uncomment for FPGA < 12 support 
// #define SUPPORT_FPGA_BELOW_V12    1

#define FPGA_ADDR_SIZE            0x00010000

#define FPGA_ISR                  (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0000)
// #define FPGA_IMR               *(volatile UINT16 *)(FPGA_BASE_ADDR + 0x0002)
#define FPGA_IMR1                 (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0002)
#define FPGA_IMR2                 (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0070)
#define FPGA_VER                  (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0004)
#define FPGA_WD_TIMER             (volatile UINT16 *)(FPGA_BASE_ADDR + 0x5A5A)
#define FPGA_WD_TIMER_INV         (volatile UINT16 *)(FPGA_BASE_ADDR + 0xA5A4)

#define FPGA_429_COMMON_CONTROL   (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0006)
#define FPGA_429_LOOPBACK_CONTROL (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0008)

#define FPGA_429_RX1_STATUS       (volatile UINT16 *)(FPGA_BASE_ADDR + 0x000A)
#define FPGA_429_RX1_FIFO_STATUS  (volatile UINT16 *)(FPGA_BASE_ADDR + 0x000C)
#define FPGA_429_RX1_LSW          (volatile UINT16 *)(FPGA_BASE_ADDR + 0x000E)
#define FPGA_429_RX1_MSW          (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0010)
#define FPGA_429_RX1_CONFIG       (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0012)
#define FPGA_429_RX1_LSW_TEST     (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0014)
#define FPGA_429_RX1_MSW_TEST     (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0016)

#define FPGA_429_RX2_STATUS       (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0018)
#define FPGA_429_RX2_FIFO_STATUS  (volatile UINT16 *)(FPGA_BASE_ADDR + 0x001A)
#define FPGA_429_RX2_LSW          (volatile UINT16 *)(FPGA_BASE_ADDR + 0x001C)
#define FPGA_429_RX2_MSW          (volatile UINT16 *)(FPGA_BASE_ADDR + 0x001E)
#define FPGA_429_RX2_CONFIG       (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0020)
#define FPGA_429_RX2_LSW_TEST     (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0022)
#define FPGA_429_RX2_MSW_TEST     (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0024)

#define FPGA_429_RX3_STATUS       (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0026)
#define FPGA_429_RX3_FIFO_STATUS  (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0028)
#define FPGA_429_RX3_LSW          (volatile UINT16 *)(FPGA_BASE_ADDR + 0x002A)
#define FPGA_429_RX3_MSW          (volatile UINT16 *)(FPGA_BASE_ADDR + 0x002C)
#define FPGA_429_RX3_CONFIG       (volatile UINT16 *)(FPGA_BASE_ADDR + 0x002E)
#define FPGA_429_RX3_LSW_TEST     (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0030)
#define FPGA_429_RX3_MSW_TEST     (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0032)

#define FPGA_429_RX4_STATUS       (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0034)
#define FPGA_429_RX4_FIFO_STATUS  (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0036)
#define FPGA_429_RX4_LSW          (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0038)
#define FPGA_429_RX4_MSW          (volatile UINT16 *)(FPGA_BASE_ADDR + 0x003A)
#define FPGA_429_RX4_CONFIG       (volatile UINT16 *)(FPGA_BASE_ADDR + 0x003C)
#define FPGA_429_RX4_LSW_TEST     (volatile UINT16 *)(FPGA_BASE_ADDR + 0x003E)
#define FPGA_429_RX4_MSW_TEST     (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0040)

#define FPGA_429_TX1_STATUS       (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0042)
#define FPGA_429_TX1_FIFO_STATUS  (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0044)
#define FPGA_429_TX1_CONFIG       (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0046)
#define FPGA_429_TX1_LSW_TEST     (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0048)
#define FPGA_429_TX1_MSW_TEST     (volatile UINT16 *)(FPGA_BASE_ADDR + 0x004A)
#define FPGA_429_TX1_LSW          (volatile UINT16 *)(FPGA_BASE_ADDR + 0x004C)
#define FPGA_429_TX1_MSW          (volatile UINT16 *)(FPGA_BASE_ADDR + 0x004E)

#define FPGA_429_TX2_STATUS       (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0050)
#define FPGA_429_TX2_FIFO_STATUS  (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0052)
#define FPGA_429_TX2_CONFIG       (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0054)
#define FPGA_429_TX2_LSW_TEST     (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0056)
#define FPGA_429_TX2_MSW_TEST     (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0058)
#define FPGA_429_TX2_LSW          (volatile UINT16 *)(FPGA_BASE_ADDR + 0x005A)
#define FPGA_429_TX2_MSW          (volatile UINT16 *)(FPGA_BASE_ADDR + 0x005C)

#define FPGA_GPIO_0               (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0072)
#define FPGA_GPIO_1               (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0074)

#define FPGA_429_RX1_FILTER       (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0100)
#define FPGA_429_RX2_FILTER       (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0300)
#define FPGA_429_RX3_FILTER       (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0500)
#define FPGA_429_RX4_FILTER       (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0700)
#define FPGA_429_FILTER_SIZE      256

#define FPGA_QAR_CONTROL          (volatile UINT16 *)(FPGA_BASE_ADDR + 0x005E)
#define FPGA_QAR_STATUS           (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0060)
#define FPGA_QAR_BARKER1          (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0062)
#define FPGA_QAR_BARKER2          (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0064)
#define FPGA_QAR_BARKER3          (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0066)
#define FPGA_QAR_BARKER4          (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0068)

#define FPGA_QAR_SF_BUFFER1       (volatile UINT16 *)(FPGA_BASE_ADDR + 0x0900)
#define FPGA_QAR_SF_BUFFER2       (volatile UINT16 *)(FPGA_BASE_ADDR + 0x1100)

// Ver 12 and Higher 
#define FPGA_QAR_INTERNAL_SF_BUFFER1 (volatile UINT16 *)(FPGA_BASE_ADDR + 0x1900)
#define FPGA_QAR_INTERNAL_SF_BUFFER2 (volatile UINT16 *)(FPGA_BASE_ADDR + 0x2100)
#define FPGA_QAR_INTERNAL_SF_BUFFER3 (volatile UINT16 *)(FPGA_BASE_ADDR + 0x2900)
#define FPGA_QAR_INTERNAL_SF_BUFFER4 (volatile UINT16 *)(FPGA_BASE_ADDR + 0x3100)

#define FPGA_QAR_BUFFER_SIZE      0x800

#define FPGA_ERR_TRUE             TRUE    // or HIGH / ON 
#define FPGA_ERR_FALSE            FALSE   // or LOW / OFF

#define FPGA_CONFIG_DONE          TRUE    // or HIGH / ON 
#define FPGA_CONFIG_NOT_DONE      FALSE   // or LOW / OFF

#define FPGA_MAX_RELOAD_RETRIES   5

#define FPGA_MAJOR_VERSION_COMPATIBILITY  12  // FPGA Major Version number 
                                              // current software is compatible with.
                                               
#define FPGA_VER_12                12      // Last major version with new Registers
                                               
#define FPGA_WD_TIMER_VAL         0xAAAA   // Value req to set FPGA_WD_TIMER reg
#define FPGA_WD_TIMER_INV_VAL     0x5555   // Value req to set FPGA_WD_TIMER_INV reg
                                           //    After _INV reg set, FPGA will reset 
                                           //    both reg to zero. 

#define FPGA_MIN_INTERRUPT_TIME 25  // min accepted time between interrupts (uS)
#define FPGA_MAX_INTERRUPT_CNT  10  // max number of consecutive fast interrupts

// ************************************************************
// Register Definitions
// ************************************************************
// FPGA IMR Register
#define IMR1_429_RX1_FULL         B15
#define IMR1_429_RX1_HALF_FULL    B14
#define IMR1_429_RX1_NOT_EMPTY    B13
#define IMR1_429_RX2_FULL         B12
#define IMR1_429_RX2_HALF_FULL    B11
#define IMR1_429_RX2_NOT_EMPTY    B10
#define IMR1_429_RX3_FULL         B9
#define IMR1_429_RX3_HALF_FULL    B8
#define IMR1_429_RX3_NOT_EMPTY    B7
#define IMR1_429_RX4_FULL         B6
#define IMR1_429_RX4_HALF_FULL    B5
#define IMR1_429_RX4_NOT_EMPTY    B4

/*
#define IMR1_429_TX1_HALF_FULL    B3
#define IMR1_429_TX1_EMTPY        B2
#define IMR1_429_TX2_HALF_FULL    B1
#define IMR1_429_TX2_EMTPY        B0
*/

#ifdef SUPPORT_FPGA_BELOW_V12
#define IMR1_429_TX1_HALF_FULL_v9 B3
#define IMR1_429_TX1_EMTPY_v9     B2
#define IMR1_429_TX2_HALF_FULL_v9 B1
#define IMR1_429_TX2_EMTPY_v9     B0
#endif

#define IMR1_429_TX1_EMPTY_v12     B1  // New Reg Bit definition for v12
#define IMR1_429_TX1_HALF_FULL_v12 B2  // New Reg Bit definition for v12
#define IMR1_429_TX1_FULL_v12      B3  // New Reg Bit definition for v12

#define IMR2_429_TX2_EMPTY_v12     B1  // New Reg Bit definition for v12
#define IMR2_429_TX2_HALF_FULL_v12 B2  // New Reg Bit definition for v12
#define IMR2_429_TX2_FULL_v12      B3  // New Reg Bit definition for v12


#define IMR2_INT_TEST             B15
#ifdef SUPPORT_FPGA_BELOW_V12
#define IMR2_NOT_USED_v9          B1    // Bits 14..01
#endif
#define IMR2_NOT_USED_v12         B4    // Bits 14.04
#define IMR2_QAR_SF               B0

#define IMR1_MASK_IRQ_VAL         0x01  // NOTE: IMR1 is a "mask" interrupt reg
#define IMR1_UNMAK_IRQ_VAL        0x00

#define IMR2_ENB_IRQ_VAL          0x01  // NOTE: IMR2 is an interrupt enb reg
#define IMR2_DSB_IRQ_VAL          0x00

/*
#define ARINC429_RX1_FIFO         B14
#define ARINC429_RX2_FIFO         B12
#define ARINC429_RX3_FIFO         B10
#define ARINC429_RX4_FIFO         B8
#define ARINC429_TX1_FIFO         B6
#define ARINC429_TX2_FIFO         B4
#define QAR_SUB_FRAME             B3
*/
/*
#define ARINC429_IRQ_FIFO_EMPTY       0x01
#define ARINC429_IRQ_FIFO_HALF        0x02
#define ARINC429_IRQ_FIFO_FULL        0x03
#define QAR_IRQ_SUB_FRAME_ENABLE      0x01
*/


// FPGA ISR Register
#define ISR_429_RX1                B14   // Bits 15..14
#define ISR_429_RX2                B12   // Bits 13..12
#define ISR_429_RX3                B10   // Bits 11..10
#define ISR_429_RX4                B8    // Bits 09..08
#define ISR_429_TX1                B6    // Bits 07..06
#define ISR_429_TX2                B4    // Bits 05..04
#define ISR_QAR_SF                 B3    // Bits 03..03
#define ISR_INT_TEST               B2
#define ISR_NOT_USED               B0    // Bits 01..00

#define ISR_429_RX_INT             0xFF00 // Any Arinc 429 Rx Ch Interrupt
#define ISR_429_TX_INT             0x00F0 // Any Arinc 429 Tx Ch Interrupt

#define ISR_429_RX_TX_INT_BITS     0x0003 // Two bits allocated for Rx/Tx INT

#define ISR_429_RX_NO_IRQ_VAL      0x00
#define ISR_429_RX_NOT_EMPTY_VAL   0x01
#define ISR_429_RX_HALF_FULL_VAL   0x02
#define ISR_429_RX_FULL_VAL        0x03

#define ISR_429_TX_NO_IRQ_VAL      0x00
#define ISR_429_TX_EMPTY_VAL       0x01
#define ISR_429_TX_HALF_FULL_VAL   0x02
#define ISR_429_TX_FULL_VAL        0x03

#define ISR_QAR_SF_RX              0x01
#define ISR_QAR_SF_NOT_RX          0x00

// ARINC429 Control Register
#define CCR_NOT_USED               B6        // Bits 15..06
#define CCR_429_TX2_RATE           B5
#define CCR_429_TX1_RATE           B4
#define CCR_429_RX4_RATE           B3 
#define CCR_429_RX3_RATE           B2 
#define CCR_429_RX2_RATE           B1
#define CCR_429_RX1_RATE           B0

#define CCR_429_HIGH_SPEED_VAL     0x01
#define CCR_429_LOW_SPEED_VAL      0x00


// ARINC429 Loopback Register
#define LBR_429_NOT_USED           B10       // Bits 15..10      
#define LBR_429_TX1_DRIVE          B9
#define LBR_429_TX2_DRIVE          B8
#define LBR_429_RX1_SOURCE         B6        // Bits 7..6
#define LBR_429_RX2_SOURCE         B4        // Bits 5..4
#define LBR_429_RX3_SOURCE         B2        // Bits 3..2
#define LBR_429_RX4_SOURCE         B0        // Bits 1..0 

#define LBR_429_TX_NORMAL_VAL      0x00
#define LBR_429_TX_QUIET_VAL       0x01

#define LBR_429_RX_NORMAL_VAL      0x00
#define LBR_429_RX_FROM_TX1_VAL    0x02
#define LBR_429_RX_FROM_TX2_VAL    0x03


// ARINC429 RX Status Register
#define RSR_429_NOT_USED           B5        // Bits 15..05 
#define RSR_429_DATA_PRESENT       B4
#define RSR_429_PARITY_ERROR       B3
#define RSR_429_FRAMING_ERROR      B2        
#define RSR_429_CHAN_ENABLE        B1
#define RSR_429_PARITY_SELECT      B0

#define SR_429_ENABLED_VAL         0x01     // Common to Rx and Tx
#define SR_429_DISABLED_VAL        0x00     // Common to Rx and Tx
#define SR_429_EVEN_PARITY_VAL     0x00     // Common to Rx and Tx
#define SR_429_ODD_PARITY_VAL      0x01     // Common to Rx and Tx


// ARINC429 RX FIFO Status
#define RFSR_429_NOT_USED          B4        // Bits 15..04
#define RFSR_429_FIFO_HALF_FULL    B3        //   1 = Half Full
#define RFSR_429_FIFO_OVERRUN      B2        //   1 = Over Run
#define RFSR_429_FIFO_FULL         B1        //   1 = Full 
// Test before FPGA fixed
// #define RFSR_429_FIFO_OVERRUN      B1        //   1 = Over Run
// #define RFSR_429_FIFO_FULL         B2        //   1 = Full 
// Test before FPGA fixed
#define RFSR_429_FIFO_EMPTY        B0        //   1 = Empty 


// ARINC429 TX Status Register
// #define TSR_429_NOT_USED           B2        // Bits 15..02
#define TSR_429_CHAN_ENABLE        B1
#define TSR_429_PARITY_SELECT      B0


// ARINC429 TX FIFO Status 
#define TSR_429_NOT_USED           B4        // Bits 15..04
#define TSR_429_FIFO_HALF_FULL     B3        //   1 = Half Full
#define TSR_429_FIFO_OVERRUN       B2        //   1 = Over Run
#define TSR_429_FIFO_FULL          B1        //   1 = Full 
#define TSR_429_FIFO_EMPTY         B0        //   1 = Empty 


// ARINC429 RX Configuration
#define RCR_429_NOT_USED           B5        // Bits 15..5
//#define RCR_429_PARITY_ERROR_CLR   B4
//#define RCR_429_LABEL_SWAP         B3
#define RCR_429_LABEL_SWAP         B4
#define RCR_429_PARITY_ERROR_CLR   B3
#define RCR_429_FRAME_ERROR_CLR    B2 
#define RCR_429_ENABLE             B1
#define RCR_429_PARITY_SELECT      B0

#define RCR_429_SET_CLR_VAL        0x01      // Parity and Framing 
#define RCR_429_UNSET_CLR_VAL      0x00      // Parity and Framing, "No Change"
#define RCR_429_BITS_NOT_SWAP_VAL  0x01
#define RCR_429_BITS_SWAP_VAL      0x00
#define CR_429_ENABLE_VAL          0x01      // Common to Rx and Tx
#define CR_429_DISABLE_VAL         0x00      // Common to Rx and Tx
#define CR_429_ODD_PARITY_VAL      0x01      // Common to Rx and Tx
#define CR_429_EVEN_PARITY_VAL     0x00      // Common to Rx and Tx


// ARINC429 TX Configuration
#define TCR_429_NOT_USED           B3        // Bits 15..03
#define TCR_429_BIT_FLIP           B2        // 1 = label filter bits flipped
#define TCR_429_ENABLE             B1        // 1 = Tx enable
#define TCR_429_PARITY_SELECT      B0        // 1 = Odd Parity 

#define TCR_429_BIT_FLIP_VAL       0x01       //1 = label filter bits flipped
#define TCR_429_NO_BIT_FLIP_VAL    0x00       //0 = label fliter bits not flipped 

// GPIO 0 Bit definitions
#define FPGA_GPIO_0_232_485        B0
#define FPGA_GPIO_0_CAN0_WAKE      B1
#define FPGA_GPIO_0_CAN1_WAKE      B2

#define FPGA_GPIO_0_232_VAL        0x1
#define FPGA_GPIO_O_485_VAL        0x0
#define FPGA_GPIO_0_CAN_OFF_VAL    0x0

// GPIO 1 Bit definitions
#define FPGA_GPIO_1_FFDI           0x4000     // Full Flight Data Inhibit offload.  
                                              // 0 = Inhibit 
                                              // 1 = No Inhibit

// ARINC429 RX LABEL WRAP / FILTER REGISTER
#define LFR_429_NOT_USED           B4        // Bits 15..04
#define LFR_429_SDI_MASK3          B3
#define LFR_429_SDI_MASK2          B2
#define LFR_429_SDI_MASK1          B1
#define LFR_429_SDI_MASK0          B0

#define LFR_429_REJECT_SDI_VAL     0x00
#define LFR_429_ACCEPT_SDI_VAL     0x01

// ARINC429 Misc Defines 
#define FPGA_MAX_RX_CHAN           4
#define FPGA_MAX_TX_CHAN           2

#define FPGA_MAX_RX_FIFO_SIZE      256
#define FPGA_MAX_TX_FIFO_SIZE      256

#define ARINC429_MAX_SD            4


/******************************************************************************
                             QAR Register Definitions
******************************************************************************/
/*
// QAR Control Register
#define QAR_FORMAT                B4
#define QAR_BYTES_PER_SUBFRAME    B1
#define QAR_RESYNC                B0

// QAR Status Register
#define QAR_LOSS_OF_FRAME_SYNC    B1
#define QAR_SYNC                  B0
*/


// QAR Control Register
#define QCR_NOT_USED               B8
#define QCR_TSTP                   B7
#define QCR_TSTN                   B6
#define QCR_ENABLE                 B5
#define QCR_FORMAT                 B4 
#define QCR_BYTES_SF               B1        // Bits 03..01
#define QCR_RESYNC                 B0

#define QCR_BYTES_SF_FIELD         0x000E    // Bits 03..01

#define QCR_TEST_DISABLED_VAL      0x00
#define QCR_ENABLED_VAL            0x01
#define QCR_FORMAT_BIP_VAL         0x00
#define QCR_FORMAT_HBP_VAL         0x01
#define QCR_BYTES_SF_DISABLED_VAL  0x00
#define QCR_BYTES_SF_64WORDS_VAL   0x01
#define QCR_BYTES_SF_128WORDS_VAL  0x02
#define QCR_BYTES_SF_256WORDS_VAL  0x03
#define QCR_BYTES_SF_512WORDS_VAL  0x04
#define QCR_BYTES_SF_1024WORDS_VAL 0x05
#define QCR_RESYNC_NORMAL_VAL      0x00
#define QCR_RESYNC_REFRAME_VAL     0x01


// QAR Sub-Frame Code Word Register 
#define QSF_NOT_USED               B12       // Bits 15..12
#define QSF_CODE                   B0        // Bits 11..00

#define QSF_CODE_1_VAL             0x0E24    
#define QSF_CODE_2_VAL             0x01D8    
#define QSF_CODE_3_VAL             0x0E25    
#define QSF_CODE_4_VAL             0x01D9    


// QAR Status Register 
#define QSR_BIPP                   B7
#define QSR_BIIN                   B6
#define QSR_BARKER_ERROR           B5
#define QSR_DATA_PRESENT           B4
#define QSR_FRAME                  B2        // Bits 03..02 
                                             //     00 = SF 1
                                             //     01 = SF 2
                                             //     02 = SF 3
                                             //     03 = SF 4
#define QSR_LOSS_SYNC              B1        
#define QSR_SYNC                   B0        

#define QSR_BARKER_ERROR_VAL       0x01
#define QSR_BARKER_NO_ERROR_VAL    0x00
//#define QSR_DATA_PRESENT_VAL       0x01
//#define QSR_DATA_NOT_PRESENT_VAL   0x00
#define QSR_DATA_PRESENT_VAL       0x00
#define QSR_DATA_NOT_PRESENT_VAL   0x01
#define QSR_LOSS_SYNC_YES_VAL      0x01
#define QSR_LOSS_SYNC_NO_VAL       0x00
#define QSR_SYNC_YES_VAL           0x01
#define QSR_SYNC_NO_VAL            0x00

/******************************************************************************
                                 Package Typedefs
******************************************************************************/
typedef struct 
{
  UINT16 CREG; 
  UINT16 LFR[FPGA_429_FILTER_SIZE]; 
} FPGA_SHADOW_ARINC429_RX_CFG, *FPGA_SHADOW_ARINC429_RX_CFG_PTR; 

typedef struct 
{
  UINT16 CREG; 
} FPGA_SHADOW_ARINC429_TX_CFG, *FPGA_SHADOW_ARINC429_TX_CFG_PTR; 

typedef struct 
{
  UINT16 QCR; 
  UINT16 QSFCODE1;  // Barker code 1
  UINT16 QSFCODE2;  // Barker code 2
  UINT16 QSFCODE3;  // Barker code 3
  UINT16 QSFCODE4;  // Barker code 4
} FPGA_SHADOW_QAR_CFG, *FPGA_SHADOW_QAR_CFG_PTR; 

typedef struct {
  UINT16  IMR1;
  UINT16  IMR2; 
  UINT16  CCR;   // Arinc429 Specific
  UINT16  LBR;   // Arinc429 Specific
  UINT16  GPIO;  // FPGA GPIO General Purpose Output 
  FPGA_SHADOW_ARINC429_RX_CFG Rx[FPGA_MAX_RX_CHAN]; 
  FPGA_SHADOW_ARINC429_TX_CFG Tx[FPGA_MAX_TX_CHAN]; 
  FPGA_SHADOW_QAR_CFG Qar; 
} FPGA_SHADOW_RAM, *FPGA_SHADOW_RAM_PTR; 

typedef enum {
  FPGA_TEST_NONE,
  FPGA_TEST_INT_SET,
  FPGA_TEST_INT_ACK,
  FPGA_TEST_MAX,
} FPGA_TEST_ENUM;

// QAR System Status 
typedef enum 
{
  FPGA_STATUS_OK = 0,
  FPGA_STATUS_FAULTED, 
  FPGA_STATUS_FORCERELOAD, 
  FPGA_STATUS_MAX
} FPGA_SYSTEM_STATUS; 

typedef struct {
  FPGA_TEST_ENUM nIntTest; 
  UINT16 nIntTestOkCnt; 
  UINT16 nIntTestFailCnt;
  
  RESULT InitResult;         // Initialization result code. 
                             //  Init success == DRV_OK 
                             
  UINT32 CRCErrCnt;          // Cnt of number of CRC Error 
  UINT32 RegCheckFailCnt;    // Cnt of number of CBIT Reg Check Fail Compare
  
  FPGA_SYSTEM_STATUS SystemStatus;  // Current FPGA System Status
  
  UINT32 MaxReloadTime;      // Max time to wait before declaring Force Reload of
                             //    FPGA a failure. According to FPGA HW Eng, 
                             //    Reload should take 50msec to 2.2 sec
  UINT32 ReloadTries;        // Current number of Force Reload retries.  
  
  UINT32  LastTime;         // Last Interrupt Time (uS)
  UINT32  MinIntrTime;      // Minimum time between interrupts (uS)
  UINT32  IntrCnt;          // Consecutive Fast Interrupt Counter
  BOOLEAN IntrFail;         // Excessive Interrupt Failure Flag

  UINT16 Version;     // FPGA Version 
  
} FPGA_STATUS, *FPGA_STATUS_PTR;


typedef enum {
  FPGA_ENUM_IMR1 = 1, 
  FPGA_ENUM_IMR2,
  FPGA_ENUM_CCR, 
  FPGA_ENUM_LBR,
  FPGA_ENUM_RX1_CFG,
  FPGA_ENUM_RX2_CFG,
  FPGA_ENUM_RX3_CFG,
  FPGA_ENUM_RX4_CFG,
  FPGA_ENUM_QAR_CCR,
  FPGA_ENUM_QAR_BARKER1,
  FPGA_ENUM_QAR_BARKER2,
  FPGA_ENUM_QAR_BARKER3,
  FPGA_ENUM_QAR_BARKER4
} FPGA_ENUM_REG; 

typedef struct {
  volatile UINT16* ptrReg;   // Pointer to FPGA HW Register 
  UINT16 *shadow1;           // Pointer to FPGA Shadow RAM 1
  UINT16 *shadow2;           // Pointer to FPGA Shadow RAM 2
  const UINT16 mask;         // FPGA HW Register Mask - Unused / Reserved Bits
  FPGA_ENUM_REG  enumReg;    // Identifies the Reg that for the Reg CBIT check 
} FPGA_REG_CHECK, *FPGA_REG_CHECK_PTR; 


/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( FPGA_BODY )
   #define EXPORT
#else
   #define EXPORT extern
#endif

/******************************************************************************
                             Package Exports Variables
******************************************************************************/
EXPORT FPGA_STATUS m_FPGA_Status;


/******************************************************************************
                             Package Exports Functions
******************************************************************************/
EXPORT RESULT FPGA_Initialize       ( SYS_APP_ID *SysLogId, void *pdata, UINT16 *psize );
/*
EXPORT RESULT FPGA_Interrupt_Enable (UINT16  IntBit,
                                     UINT16  IntMask,
                                     BOOLEAN Enable);
*/                                     
EXPORT RESULT FPGA_Set_Interrupt    (UINT16 IntBit, BOOLEAN Enable); 
EXPORT BOOLEAN FPGA_Monitor_Ok      ( void ); 
EXPORT void FPGA_ForceReload        ( void );
EXPORT BOOLEAN FPGA_ForceReload_Status ( void );
EXPORT RESULT FPGA_Configure        ( void ); 
EXPORT FPGA_SHADOW_RAM_PTR FPGA_GetShadowRam ( void ); 
EXPORT FPGA_STATUS_PTR FPGA_GetStatus ( void ); 
EXPORT FPGA_REG_CHECK_PTR FPGA_GetRegCheckTbl ( UINT16 *nCnt ); 


#endif // FPGA_H                             

/*************************************************************************
 *  MODIFICATIONS
 *    $History: FPGA.h $
 * 
 * *****************  Version 29  *****************
 * User: Jim Mood     Date: 7/19/12    Time: 10:42a
 * Updated in $/software/control processor/code/drivers
 * SCR 1107: Data Offload changes for 2.0.0
 * 
 * *****************  Version 28  *****************
 * User: Contractor2  Date: 9/24/10    Time: 3:02p
 * Updated in $/software/control processor/code/drivers
 * SCR #882 WD - Old Code for pulsing the WD should be removed
 * 
 * *****************  Version 27  *****************
 * User: Peter Lee    Date: 8/19/10    Time: 4:48p
 * Updated in $/software/control processor/code/drivers
 * SCR #804 Shutdown DT Task over run for Arinc429 and QAR. 
 * 
 * *****************  Version 26  *****************
 * User: Peter Lee    Date: 7/23/10    Time: 1:39p
 * Updated in $/software/control processor/code/drivers
 * SCR #688 Remove FPGA Ver 9 support.  SW will only be compatible with
 * Ver 12.
 * 
 * *****************  Version 25  *****************
 * User: Contractor2  Date: 7/09/10    Time: 4:31p
 * Updated in $/software/control processor/code/drivers
 * SCR #8 Implementation: External Interrupt Monitors
 * 
 * *****************  Version 24  *****************
 * User: Peter Lee    Date: 5/31/10    Time: 5:52p
 * Updated in $/software/control processor/code/drivers
 * SCR #601 FPGA DIO Logic Updates/Bug fixes
 * 
 * *****************  Version 23  *****************
 * User: Contractor3  Date: 5/06/10    Time: 11:27a
 * Updated in $/software/control processor/code/drivers
 * Check In SCR #586
 * 
 * *****************  Version 22  *****************
 * User: Jeff Vahue   Date: 4/12/10    Time: 5:43p
 * Updated in $/software/control processor/code/drivers
 * SCR #541 - all XXX_CreateLogs functions are now dependent on
 * GENERATE_SYS_LOGS
 * 
 * *****************  Version 21  *****************
 * User: Peter Lee    Date: 12/15/09   Time: 2:47p
 * Updated in $/software/control processor/code/drivers
 * SCR #318 Arinc429 Tx Implementation
 * 
 * *****************  Version 20  *****************
 * User: Peter Lee    Date: 12/10/09   Time: 9:55a
 * Updated in $/software/control processor/code/drivers
 * SCR #358 Updated Arinc429 Tx Flag Processing 
 * 
 * *****************  Version 19  *****************
 * User: Peter Lee    Date: 12/04/09   Time: 5:24p
 * Updated in $/software/control processor/code/drivers
 * SCR #358 New FPGA changes to INT MASK and ISR Status reg
 * 
 * *****************  Version 18  *****************
 * User: Peter Lee    Date: 12/04/09   Time: 2:15p
 * Updated in $/software/control processor/code/drivers
 * SCR #361 Error - Delays using TTMR_GetHSTickCount and rollover. 
 * 
 * *****************  Version 17  *****************
 * User: Peter Lee    Date: 11/20/09   Time: 2:35p
 * Updated in $/software/control processor/code/drivers
 * SCR #345 Implement FPGA Watchdog Requirements
 * 
 * *****************  Version 16  *****************
 * User: Peter Lee    Date: 11/02/09   Time: 1:51p
 * Updated in $/software/control processor/code/drivers
 * Misc Code Update to support WIN32 and spelling corrections
 * 
 * *****************  Version 15  *****************
 * User: Peter Lee    Date: 10/15/09   Time: 9:59a
 * Updated in $/software/control processor/code/drivers
 * SCR 273 and FPGA CBIT Requirements
 * 
 * *****************  Version 14  *****************
 * User: Peter Lee    Date: 9/15/09    Time: 6:09p
 * Updated in $/software/control processor/code/drivers
 * SCR #94, #95 PBIT returns log structure to Init Mgr
 * 
 * *****************  Version 13  *****************
 * User: Peter Lee    Date: 6/29/09    Time: 2:20p
 * Updated in $/software/control processor/code/drivers
 * SCR #204 Misc Code Updates. 
 * 
 * *****************  Version 12  *****************
 * User: Jim Mood     Date: 2/05/09    Time: 11:21a
 * Updated in $/control processor/code/drivers
 * 
 * *****************  Version 11  *****************
 * User: Jim Mood     Date: 2/05/09    Time: 11:16a
 * Updated in $/control processor/code/drivers
 * Added the FPGA_GPIO_0 register for FPGA verison 9
 * 
 * *****************  Version 10  *****************
 * User: Peter Lee    Date: 1/20/09    Time: 6:25p
 * Updated in $/control processor/code/drivers
 * SCR #131 
 * Updated FPGA Reg defines for LFR. 
 * 
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 1/12/09    Time: 11:23a
 * Updated in $/control processor/code/drivers
 * Updated FPGA Register Defines (in FPGA.h) to be constants rather than
 * "Get Content of Address" (prefix by "*") to simply code references in
 * Arinc429, QAR and FPGA. 
 * 
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 1/06/09    Time: 4:45p
 * Updated in $/control processor/code/drivers
 * SCR 129 Fix FPGA_CheckVersion() to handle toggle of hw FPGA Version
 * identifier. 
 * 
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 1/06/09    Time: 2:42p
 * Updated in $/control processor/code/drivers
 * SCR 129 Misc FPGA Updates
 * 
 * *****************  Version 6  *****************
 * User: Peter Lee    Date: 11/24/08   Time: 2:08p
 * Updated in $/control processor/code/drivers
 * SCR #97 Add new FPGA Register bits
 * 
 * *****************  Version 5  *****************
 * User: John Omalley Date: 6/11/08    Time: 9:29a
 * Updated in $/control processor/code/drivers
 * Added QAR Enable bit
 * 
 *
 *
 ***************************************************************************/
                          
