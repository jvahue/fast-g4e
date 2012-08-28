#ifndef ARINC429_H
#define ARINC429_H
/******************************************************************************
            Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:        Arinc429.h      
    
    Description: Contains data structures related to the Arinc429 
    
VERSION
     $Revision: 51 $  $Date: 8/28/12 1:06p $
    
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_basic.h"
#include "fpga.h"
/******************************************************************************
                                 Package Defines
******************************************************************************/
#define ARINC_RX_CHAN_MAX FPGA_MAX_RX_CHAN
#define ARINC_TX_CHAN_MAX FPGA_MAX_TX_CHAN

// Common SSM Bits
#define ARINC_SSM_BITS             0x60     // SSM bit field location

#define ARINC_SSM_FUNCTEST_v       0x40     // SSM Functional Test Data value
#define ARINC_SSM_NOCOMPDATA_v     0x20     // SSM No Computed Data value

#define ARINC_BNR_SSM_SIGN_BIT     0x10     // SSM sign bit
#define ARINC_BNR_SSM_NORM_v       0x60     // SSM BNR Normal Op Data value
#define ARINC_BNR_SSM_FUNCTEST_v   0x40     // SSM BNR Functional Test Data value
#define ARINC_BNR_SSM_NOCOMPDATA_v 0x20     // SSM BNR No Computed Data value
#define ARINC_BNR_SSM_FAILURE_v    0x00     // SSM BNR System Failure value

#define ARINC_BCD_SSM_FUNCTEST_v   0x40     // SSM BCD Functiona Test Data value
#define ARINC_BCD_SSM_NOCOMPDATA_v 0x20     // SSM BCD No Computed Data value
#define ARINC_BCD_SSM_POSTIVE_v    0x00     // SSM BCD Positive value
#define ARINC_BCD_SSM_NEGATIVE_v   0x60     // SSM BCD Negative value

#define ARINC_BCD_MASK             0x1FFFFC00// Mask for all BCD Digits
#define ARINC_BCD_SHIFT            10        // Shift to remove label and SDI bits
#define ARINC_BCD_DIGITS           5

#define ARINC_DISC_SSM_NORM_v        0x00   // SSM DISC Normal Op Data value
#define ARINC_DISC_SSM_NOCOMPDATA_v  0x20   // SSM DISC No Computed Data value
#define ARINC_DISC_SSM_FUNCTEST_v    0x40   // SSM DISC Function Test Data
#define ARINC_DISC_SSM_FAILURE_v     0x60   // SSM DISC System Failure value

#define BNR_VALID_SSM               0x08
#define BCD_VALID_SSM               0x09
#define DISC_VALID_SSM              0x01

#define ARINC_MSG_PARITY_BIT        0x80000000UL
#define ARINC_MSG_SSM_BITS          0x60000000UL
#define ARINC_MSG_SIGN_BIT          0x10000000UL
#define ARINC_MSG_DATA_BITS         0x0FFFFC00UL
#define ARINC_MSG_SDI_BITS          0x00000300UL
#define ARINC_MSG_LABEL_BITS        0x000000FFUL

#define ARINC_MSG_SIGN_DATA_BITS    (ARINC_MSG_SIGN_BIT | ARINC_MSG_DATA_BITS)
#define ARINC_MSG_VALID_BIT         ARINC_MSG_PARITY_BIT

#define ARINC_MSG_SHIFT_SDI          8
#define ARINC_MSG_SHIFT_DATA        10
#define ARINC_MSG_SHIFT_MSB         24       // Shift for SSM masks
#define ARINC_MSG_SHIFT_SIGN        28
#define ARINC_MSG_SHIFT_SSM         29
#define ARINC_MSG_SHIFT_PARITY      31



#define ARINC429_MAX_LABELS         FPGA_429_FILTER_SIZE

/******************************************************************************
                                 Package Typedefs
******************************************************************************/
typedef enum
{
   ARINC429_SPEED_LOW  = CCR_429_LOW_SPEED_VAL,
   ARINC429_SPEED_HIGH = CCR_429_HIGH_SPEED_VAL
} ARINC429_SPEED;

typedef enum
{
   ARINC429_PARITY_EVEN = SR_429_EVEN_PARITY_VAL,
   ARINC429_PARITY_ODD  = SR_429_ODD_PARITY_VAL
} ARINC429_PARITY;

typedef enum
{
   ARINC429_RX_SWAP_LABEL      = RCR_429_BITS_SWAP_VAL,
   ARINC429_RX_DONT_SWAP_LABEL = RCR_429_BITS_NOT_SWAP_VAL
} ARINC429_RX_SWAP;

typedef enum
{
   ARINC429_TX_SWAP_LABEL      = TCR_429_BIT_FLIP_VAL,
   ARINC429_TX_DONT_SWAP_LABEL = TCR_429_NO_BIT_FLIP_VAL
} ARINC429_TX_SWAP;

typedef enum
{
   ARINC429_RCV  = 0,
   ARINC429_XMIT = 1
} ARINC429_CHAN_TYPE;

// ARINC429 Driver Status 
typedef enum 
{
  ARINC429_DRV_STATUS_OK = 0,       // Arinc429 Enabled and OK
  ARINC429_DRV_STATUS_FAULTED_PBIT, // Arinc429 Enabled but currently Faulted PBIT
                                    //    Arinc Processing disabled until power cycle or 
                                    //    "arinc429.reconfigure". 
} ARINC429_DRIVER_STATUS; 

typedef enum 
{
  ARINC_CHAN_0,  // 0 Rx Tx
  ARINC_CHAN_1,  // 1 Rx Tx
  ARINC_CHAN_2,  // 2 Rx
  ARINC_CHAN_3,  // 3 Rx
  ARINC_CHAN_MAX,  
} ARINC429_CHAN_ENUM; 

typedef enum 
{
  A429_RX_FIFO_TEST_EMPTY = 0, 
  A429_RX_FIFO_TEST_HALF_FULL, 
  A429_RX_FIFO_TEST_FULL,
  A429_RX_FIFO_TEST_OVERRUN,
  A429_RX_FIFO_TEST_MAX
} ARINC_RX_FIFO_TEST; 

typedef enum 
{
  A429_TX_FIFO_TEST_EMPTY = 0, 
  A429_TX_FIFO_TEST_HALF_FULL, 
  A429_TX_FIFO_TEST_FULL,
  A429_TX_FIFO_TEST_OVERRUN,
  A429_TX_FIFO_TEST_MAX
} ARINC_TX_FIFO_TEST; 

// ******************************************
// Arinc429 Run Time Structures 
// ****************************************** 

// Run Time Variables with pointers to FPGA registers
//  for easier access. 
typedef struct 
{
    volatile UINT16 *Status; 
    volatile UINT16 *FIFO_Status; 
    volatile UINT16 *lsw;
    volatile UINT16 *msw; 
    volatile UINT16 *Configuration; 
    volatile UINT16 *lsw_test;
    volatile UINT16 *msw_test; 
    
    UINT16 isr_mask;          // Relevant Bits in ISR for this Rx Chan
    UINT16 imr_full_bit;      // Set on init to IMR1_429_RX1_FULL from fpga.h
    UINT16 imr_halffull_bit;  // Set on init to IMR1_429_RX1_HALF_FULL from fpga.h
    UINT16 imr_not_empty_bit; // Set on init to IMR1_429_RX1_NOT_EMPTY from fpga.h
    
    volatile UINT16 *lfr;     // Label Filter Reg Pointer 
    
} FPGA_RX_REG, *FPGA_RX_REG_PTR; 

typedef struct 
{
    volatile UINT16 *Status;
    volatile UINT16 *FIFO_Status;
    volatile UINT16 *Configuration;
    volatile UINT16 *lsw_test;
    volatile UINT16 *msw_test;
    volatile UINT16 *lsw;
    volatile UINT16 *msw; 
    
    UINT16 isr_mask;        // Relevant Bits in ISR for this Tx Chan
    UINT16 imr_full_bit;    // Set on init to IMR1_429_TX1_FULL from fpga.h
    UINT16 imr_halffull_bit;// Set on init to IMR1_429_TX1_HALF_FULL from fpga.h
    UINT16 imr_empty_bit;   // Set on init to IMR1_429_TX1_EMTPY from fpga.h
    
} FPGA_TX_REG, *FPGA_TX_REG_PTR; 


    
// Arinc429 Driver Status 
typedef struct
{
    UINT32 FIFO_HalfFullCnt;     // Count of times HW FIFO indicates half full int 
    UINT32 FIFO_FullCnt;         // Count of times HW FIFO indicates full int 
    UINT32 FIFO_NotEmptyCnt;     // Count of times HW FIFO indicates not empty
    UINT32 FIFO_OverRunCnt;      // Count of times HW FIFO indicates Over Run 

    ARINC429_DRIVER_STATUS  Status;  // Overall Arinc429 Rx[] Driver Status   
    
} ARINC429_DRV_RX_STATUS, *ARINC429_DRV_RX_STATUS_PTR; 

typedef struct 
{
    UINT32 FIFO_FullCnt;          // Count of times HW FIFO indicates FIFO Full
    UINT32 FIFO_HalfFullCnt;      // Count of times HW FIFO indicates FIFO Half Full
    UINT32 FIFO_EmptyCnt;         // Count of times HW FIFO indicates FIFO Empty
    UINT32 FIFO_OverRunCnt;       // Count of times HW FIFO indicates FIFO Over Run 
    
    ARINC429_DRIVER_STATUS  Status;  // Overall Arinc429 Tx[] Driver Status   
    
} ARINC429_DRV_TX_STATUS, *ARINC429_DRV_TX_STATUS_PTR; 

typedef struct 
{
    ARINC429_DRV_RX_STATUS Rx[FPGA_MAX_RX_CHAN]; 
    ARINC429_DRV_TX_STATUS Tx[FPGA_MAX_TX_CHAN]; 
    
    UINT32 InterruptCnt; 
    UINT32 InterruptCntPrev; 
    
} ARINC429_DRV_STATUS, *ARINC429_DRV_STATUS_PTR; 

#pragma pack(1)
// Arinc429 Log Data Payload Structures 
typedef struct
{
  UINT32 ResultCode;  // DRV_A429_FPGA_BAD_STATE
} ARINC_DRV_PBIT_FPGA_FAIL; 


// PL 08/06/2009 The following two structures are used internally only
//     See ARINC_DRV_PBIT_RX_LFR_FIFO_TEST_RESULT for actual log struct 
typedef struct 
{
  RESULT ResultCode;  // DRV_A429_RX_FIFO_TEST_FAIL 
  BOOLEAN bTestPass[A429_RX_FIFO_TEST_MAX][FPGA_MAX_RX_CHAN]; 
} ARINC_DRV_PBIT_RX_FIFO_TEST_RESULT; 

typedef struct 
{
  RESULT ResultCode; // DRV_A429_RX_LFR_TEST_FAIL
  BOOLEAN bTestPass[FPGA_MAX_RX_CHAN]; 
} ARINC_DRV_PBIT_RX_LFR_TEST_RESULT;

typedef struct 
{
  RESULT ResultCode; // DRV_A429_TX_FIFO_TEST_FAIL
  BOOLEAN bTestPass[A429_TX_FIFO_TEST_MAX][FPGA_MAX_TX_CHAN]; 
} ARINC_DRV_PBIT_TX_FIFO_TEST_RESULT;

typedef struct 
{
  RESULT ResultCode; // DRV_A429_LOOPBACK_TEST_FAIL 
  BOOLEAN bTestPassTxLoopBack[FPGA_MAX_TX_CHAN]; 
  BOOLEAN bTestPassRxLoopBack[FPGA_MAX_RX_CHAN]; 
} ARINC_DRV_PBIT_LOOPBACK_TEST_RESULT; 

typedef struct 
{
  // Error Counts
  UINT8   frameErr;         // count of frame errors
  UINT8   parityErr;        // count of parity errors
  UINT8   intErr;           // count of interrupt errors
  UINT8   rxFifoCntErr;     // count of receive fifo count errors
  UINT8   txFifoCntErr;     // count of transmit fifo count errors
  UINT8   rxFifoEmpty;      // receive fifo is empty when it shouldn't be
  UINT8   rxFifoNotEmpty;   // receive fifo is not empty when it should be
  UINT8   txFifoNotEmpty;   // transmit fifo is not empty when it should be
  UINT8   rxFifoNoIntErr;   // receive fifo interrupt didn't occur
  UINT8   txFifoNoIntErr;   // transmit fifo interrupt didn't occur
  // Error Data
  // Note: If multiple errors occur, only the first instance is recorded.
  UINT8   chan;             // failed channel
  UINT16  expLsw;           // expected lsw
  UINT16  expMsw;           // expected msw
  UINT16  rcvdLsw;          // received lsw
  UINT16  rcvdMsw;          // received msw
  UINT16  expLbl;           // expected lfr label
  UINT16  readLbl;          // read lfr label
  // Dump of all ARINC429 FPGA registers
  UINT16  ccr;              // common control reg
  UINT16  lbr;              // loop back reg
  UINT16  rxCR[FPGA_MAX_RX_CHAN]; // receive control registers
  UINT16  rxSR[FPGA_MAX_RX_CHAN]; // receive status registers
  UINT16  rxFF[FPGA_MAX_RX_CHAN]; // receive FIFO registers
  UINT16  txCR[FPGA_MAX_TX_CHAN]; // transmit control registers
  UINT16  txSR[FPGA_MAX_TX_CHAN]; // transmit status registers
  UINT16  txFF[FPGA_MAX_TX_CHAN]; // transmit FIFO registers
  UINT32  ffHalfFullCnt;          // Count of times HW FIFO indicates half full int 
  UINT32  ffFullCnt;              // Count of times HW FIFO indicates full int 
  UINT32  ffNotEmptyCnt;          // Count of times HW FIFO indicates not empty
  UINT32  ffEmptyCnt;             // Count of times HW FIFO indicates empty
} ARINC_DRV_PBIT_FAIL_DATA; 

typedef struct 
{
  UINT32 ResultCode; // DRV_A429_RX_FIFO_TEST_FAIL or 
                     // DRV_A429_RX_LFR_TEST_FAIL
  BOOLEAN bTestPassLFR[FPGA_MAX_RX_CHAN];  // LFR Test PASS / FAIL Result
  
                     // FIFO Test PASS / FAIL Result
  BOOLEAN bTestPassFIFORx[A429_RX_FIFO_TEST_MAX][FPGA_MAX_RX_CHAN];  
  BOOLEAN bTestPassFIFOTx[A429_TX_FIFO_TEST_MAX][FPGA_MAX_TX_CHAN];  
  
                     // Loop Back Test PASS / FAIL Result 
  BOOLEAN bTestPassTxLoopBack[FPGA_MAX_TX_CHAN]; 
  BOOLEAN bTestPassRxLoopBack[FPGA_MAX_RX_CHAN]; 

  ARINC_DRV_PBIT_FAIL_DATA  failData;

} ARINC_DRV_PBIT_TEST_RESULT; 

typedef struct 
{
  RESULT result; // SYS_A429_PBIT_FAIL
  UINT16 ch; 
} ARINC429_SYS_PBIT_STARTUP_LOG; 

#pragma pack()

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( ARINC429_BODY )
   #define EXPORT
#else
   #define EXPORT extern
#endif

/******************************************************************************
                             Package Exports Variables
******************************************************************************/
extern const FPGA_RX_REG fpgaRxReg[FPGA_MAX_RX_CHAN]; 
extern const FPGA_TX_REG fpgaTxReg_v12[FPGA_MAX_TX_CHAN];


/******************************************************************************
                             Package Exports Functions
******************************************************************************/
EXPORT RESULT  Arinc429DrvInitialize      ( SYS_APP_ID *SysLogId, void *pdata, UINT16 *psize );
EXPORT void    Arinc429DrvProcessISR_Rx   ( UINT16 IntStatus ); 
EXPORT void    Arinc429DrvProcessISR_Tx   ( UINT16 IntStatus ); 
EXPORT void    Arinc429DrvFlushHWFIFO     ( void );
EXPORT BOOLEAN Arinc429DrvRead            ( UINT32 *pArinc429Msg, ARINC429_CHAN_ENUM Channel );
EXPORT void    Arinc429DrvReconfigure     ( void );
EXPORT void    Arinc429DrvSetIMR1_2       ( void );
EXPORT void    Arinc429DrvCfgLFR          ( UINT8 SdiMask, UINT8 Channel, UINT8 Label, BOOLEAN bNow );
EXPORT void    Arinc429DrvCfgRxChan       ( ARINC429_SPEED Speed, ARINC429_PARITY Parity,
                                            ARINC429_RX_SWAP SwapBits, BOOLEAN Enable, 
                                            UINT8 Channel );
EXPORT void    Arinc429DrvCfgTxChan       ( ARINC429_SPEED Speed, ARINC429_PARITY Parity,
                                            ARINC429_TX_SWAP SwapBits, BOOLEAN Enable, 
                                            UINT8 Channel );
EXPORT BOOLEAN Arinc429DrvStatus_OK       ( ARINC429_CHAN_TYPE Type, UINT8 Channel );
EXPORT BOOLEAN Arinc429DrvFPGA_OK         ( void );
EXPORT BOOLEAN Arinc429DrvCheckBITStatus  ( UINT32 *pParityErrCnt, UINT32 *pFramingErrCnt, 
                                            UINT32 *pIntCount,     UINT16 *pFPGA_Status,
                                            UINT8  Channel );
EXPORT ARINC429_DRV_RX_STATUS_PTR Arinc429DrvRxGetCounts (UINT8 Channel);
EXPORT ARINC429_DRV_TX_STATUS_PTR Arinc429DrvTxGetCounts (UINT8 Channel);

              
#endif // ARINC429_H               
/*************************************************************************
 *  MODIFICATIONS
 *    $History: arinc429.h $
 * 
 * *****************  Version 51  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 50  *****************
 * User: Contractor2  Date: 9/23/11    Time: 4:17p
 * Updated in $/software/control processor/code/drivers
 * SCR #771 Error: A429 PBIT - occasional A429_RX_FIFO_TEST_FAIL reported
 * Added additional log data and if/else.
 * 
 * *****************  Version 49  *****************
 * User: Contractor2  Date: 9/09/11    Time: 4:33p
 * Updated in $/software/control processor/code/drivers
 * SCR #771 Error: A429 PBIT - occasional A429_RX_FIFO_TEST_FAIL reported
 * 
 * *****************  Version 48  *****************
 * User: Contractor2  Date: 9/09/11    Time: 3:33p
 * Updated in $/software/control processor/code/drivers
 * SCR #771 Error: A429 PBIT - occasional A429_RX_FIFO_TEST_FAIL reported
 * 
 * *****************  Version 47  *****************
 * User: Contractor2  Date: 9/09/11    Time: 2:53p
 * Updated in $/software/control processor/code/drivers
 * SCR #771 Error: A429 PBIT - occasional A429_RX_FIFO_TEST_FAIL reported
 * Added FPGA register snapshot and fail counters to PBIT log.
 * 
 * *****************  Version 46  *****************
 * User: Contractor2  Date: 7/01/11    Time: 1:12p
 * Updated in $/software/control processor/code/drivers
 * SCR #701 Enhancement - ARINC-429 Data Reduction and BCD Parameters
 * 
 * *****************  Version 45  *****************
 * User: Contractor2  Date: 6/20/11    Time: 11:40a
 * Updated in $/software/control processor/code/drivers
 * SCR #988 Enhancement - A429 - Rx/Tx FIFO Overrun Test Status
 * 
 * *****************  Version 44  *****************
 * User: John Omalley Date: 6/08/11    Time: 12:25p
 * Updated in $/software/control processor/code/drivers
 * SCR 1040 - Removed Unused Data Field Size #define
 * 
 * *****************  Version 43  *****************
 * User: Peter Lee    Date: 9/07/10    Time: 7:02p
 * Updated in $/software/control processor/code/drivers
 * SCR #854 Code Review Updates
 * 
 * *****************  Version 42  *****************
 * User: John Omalley Date: 7/29/10    Time: 7:51p
 * Updated in $/software/control processor/code/drivers
 * SCR 754 - Added a FIFO flush after initialization
 * 
 * *****************  Version 41  *****************
 * User: John Omalley Date: 6/08/10    Time: 12:05p
 * Updated in $/software/control processor/code/drivers
 * SCR 627 - Updated for LJ60
 * 
 * *****************  Version 40  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:54p
 * Updated in $/software/control processor/code/drivers
 * SCR #587 Change TmTaskCreate to return void
 * 
 * *****************  Version 39  *****************
 * User: Contractor2  Date: 5/06/10    Time: 2:09p
 * Updated in $/software/control processor/code/drivers
 * SCR #579 Change LogWriteSys to return void
 * 
 * *****************  Version 38  *****************
 * User: Contractor2  Date: 5/03/10    Time: 11:36a
 * Updated in $/software/control processor/code/drivers
 * SCR 577: SRS-1998 - Create fault log for any out of range BCD digit.
 * 
 * *****************  Version 37  *****************
 * User: Jeff Vahue   Date: 4/12/10    Time: 5:43p
 * Updated in $/software/control processor/code/drivers
 * SCR #541 - all XXX_CreateLogs functions are now dependent on
 * GENERATE_SYS_LOGS
 * 
 * *****************  Version 36  *****************
 * User: Peter Lee    Date: 4/07/10    Time: 2:33p
 * Updated in $/software/control processor/code/drivers
 * SCR #495 Additional FPGA PBIT requested by HW
 * 
 * *****************  Version 35  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:07p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 34  *****************
 * User: Peter Lee    Date: 2/01/10    Time: 3:26p
 * Updated in $/software/control processor/code/drivers
 * SCR #432 Add Arinc429 Tx-Rx LoopBack PBIT
 * 
 * *****************  Version 33  *****************
 * User: Jeff Vahue   Date: 2/01/10    Time: 12:44p
 * Updated in $/software/control processor/code/drivers
 * SCR# 196 - remove unused function as a result of this SCR
 * Arinc429_DisableAnyStreamingDebugOuput
 * 
 * *****************  Version 32  *****************
 * User: Jeff Vahue   Date: 1/23/10    Time: 3:11p
 * Updated in $/software/control processor/code/drivers
 * SCR# 404
 * 
 * *****************  Version 31  *****************
 * User: John Omalley Date: 1/15/10    Time: 5:29p
 * Updated in $/software/control processor/code/drivers
 * SCR 396 
 * * Added Interface Validation
 * 
 * *****************  Version 30  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 5:01p
 * Updated in $/software/control processor/code/drivers
 * SCR# 397
 * 
 * *****************  Version 29  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:22p
 * Updated in $/software/control processor/code/drivers
 * SCR 18
 * 
 * *****************  Version 28  *****************
 * User: Peter Lee    Date: 12/15/09   Time: 2:47p
 * Updated in $/software/control processor/code/drivers
 * SCR #318 Arinc429 Tx Implementation
 * 
 * *****************  Version 27  *****************
 * User: Peter Lee    Date: 12/10/09   Time: 9:56a
 * Updated in $/software/control processor/code/drivers
 * SCR #358 Updated Arinc429 Tx Flag processing. 
 * 
 * *****************  Version 26  *****************
 * User: Jeff Vahue   Date: 12/07/09   Time: 4:22p
 * Updated in $/software/control processor/code/drivers
 * SCR# 364 - default Arinc Rx state is disabled
 * 
 * *****************  Version 25  *****************
 * User: Peter Lee    Date: 12/04/09   Time: 5:24p
 * Updated in $/software/control processor/code/drivers
 * SCR #358 New FPGA changes to INT MASK and ISR Status reg
 * 
 * *****************  Version 24  *****************
 * User: Peter Lee    Date: 11/12/09   Time: 10:13a
 * Updated in $/software/control processor/code/drivers
 * SCR #315 PWEH SEU Processing Support
 * 
 * *****************  Version 23  *****************
 * User: Peter Lee    Date: 10/22/09   Time: 10:27a
 * Updated in $/software/control processor/code/drivers
 * SCR #171 Add correct System Status (Normal, Caution, Fault) enum to
 * UserTables
 * 
 * *****************  Version 22  *****************
 * User: Peter Lee    Date: 10/21/09   Time: 4:12p
 * Updated in $/software/control processor/code/drivers
 * SCR #229 Update Ch/Startup TimeOut to 1 sec from 1 msec resolution.
 * Updated GPB channel loss timeout to 1 msec from 1 sec resolution. 
 * 
 * *****************  Version 21  *****************
 * User: Peter Lee    Date: 9/30/09    Time: 9:14a
 * Updated in $/software/control processor/code/drivers
 * SCR #283 Misc Code Review Updates
 * 
 * *****************  Version 20  *****************
 * User: Peter Lee    Date: 9/23/09    Time: 11:53a
 * Updated in $/software/control processor/code/drivers
 * SCR #268 Updates to _SensorTest() to include Ch Loss.  Have _ReadWord()
 * return FLOAT32
 * 
 * *****************  Version 19  *****************
 * User: Peter Lee    Date: 9/17/09    Time: 4:02p
 * Updated in $/software/control processor/code/drivers
 * SCR #262 Add ch to Arinc429 Startup and Data Loss Time Out Logs
 * 
 * *****************  Version 18  *****************
 * User: Peter Lee    Date: 9/15/09    Time: 6:07p
 * Updated in $/software/control processor/code/drivers
 * SCR #94, #95 PBIT returns log structure to Init Mgr
 * 
 * *****************  Version 17  *****************
 * User: Peter Lee    Date: 8/07/09    Time: 3:42p
 * Updated in $/software/control processor/code/drivers
 * SCR #241. Updates to System Logs
 * 
 * *****************  Version 16  *****************
 * User: Peter Lee    Date: 6/29/09    Time: 2:20p
 * Updated in $/software/control processor/code/drivers
 * SCR #204 Misc Code Updates. 
 * 
 * *****************  Version 15  *****************
 * User: Peter Lee    Date: 4/27/09    Time: 3:40p
 * Updated in $/control processor/code/drivers
 * SCR #168 Updates from preliminary code review
 * 
 * *****************  Version 14  *****************
 * User: Peter Lee    Date: 1/30/09    Time: 4:43p
 * Updated in $/control processor/code/drivers
 * SCR #131 Arinc429 Updates 
 * Add SSM Failure Detection to SensorTest()
 * 
 * *****************  Version 13  *****************
 * User: Peter Lee    Date: 1/23/09    Time: 2:54p
 * Updated in $/control processor/code/drivers
 * SCR #131 Arinc429 Updates. Add SensorValid().  
 * 
 * *****************  Version 12  *****************
 * User: Peter Lee    Date: 1/23/09    Time: 11:34a
 * Updated in $/control processor/code/drivers
 * SCR #131 Arinc429 Updates
 * Provide CBIT Health Counts
 * 
 * *****************  Version 11  *****************
 * User: Peter Lee    Date: 1/22/09    Time: 6:13p
 * Updated in $/control processor/code/drivers
 * SCR #136 PACK system log structures 
 * 
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 1/21/09    Time: 11:26a
 * Updated in $/control processor/code/drivers
 * SCR #131 Add PBIT LFR Test 
 * 
 * *****************  Version 8  *****************
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
              
