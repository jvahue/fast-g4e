#ifndef DRV_UART_H
#define DRV_UART_H
/******************************************************************************
            Copyright (C) 2008-2010 Pratt & Whitney Engine Services, Inc. 
                      Altair Engine Diagnostic Solutions
               All Rights Reserved. Proprietary and Confidential.
 
  File:        UART.h
 
  Description: UART driver for the MCF5485 PSC (Programmable Serial Controller)              
               This file contains an API and serial interrupt handlers
 
   VERSION
   $Revision: 15 $  $Date: 9/03/10 8:22p $

******************************************************************************/
/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "FIFO.h"
#include "ResultCodes.h"
#include "SystemLog.h"

/******************************************************************************
                                 Package Defines
******************************************************************************/
//Values used for software FIFO buffer definition
#define UART_MCF_PSC_FIFO_SIZE 512

#define UART0_TRANSMITTING 0x01
#define UART1_TRANSMITTING 0x02
#define UART2_TRANSMITTING 0x04
#define UART3_TRANSMITTING 0x08

#define UART_SYSTEM_CLOCK_MHZ 100 //System clock speed

#define UART_MIN_INTERRUPT_TIME 35  // min accepted time between interrupts (uS)
#define UART_MAX_INTERRUPT_CNT  10  // max number of consecutive fast interrupts

/******************************************************************************
                                 Package Typedefs
******************************************************************************/
typedef enum {
    UART_0 = 0,
    UART_1 = 1,
    UART_2 = 2,
    UART_3 = 3,
    UART_NUM_OF_UARTS
} UART_ID;

//UART initial configuration information , used as a parameter to UART_Init

// UART BAUD Rate
typedef enum {
    UART_CFG_BPS_9600   = 9600,
    UART_CFG_BPS_19200  = 19200,
    UART_CFG_BPS_38400  = 38400,
    UART_CFG_BPS_57600  = 57600,
    UART_CFG_BPS_115200 = 115200
} UART_CFG_BPS;

// UART Duplex Mode
typedef enum {
    UART_CFG_DUPLEX_FULL,
    UART_CFG_DUPLEX_HALF
} UART_CFG_DUPLEX;

// UART Data Bits
typedef enum {
    UART_CFG_DB_7 = 0x2,
    UART_CFG_DB_8 = 0x3
} UART_CFG_DB;

// UART Stop Bits
typedef enum {
    UART_CFG_SB_1 = 0x7,
    UART_CFG_SB_2 = 0xF
} UART_CFG_SB;

// UART Parity
typedef enum {
    UART_CFG_PARITY_EVEN = 0,
    UART_CFG_PARITY_ODD  = 1,
    UART_CFG_PARITY_NONE = 4
} UART_CFG_PARITY;

// UART Configuration Data
typedef struct {
  BOOLEAN         Open;           // Flag indicates if the port ready for RX/TX
  BYTE            Port;           // Port 0,1,2 or 3
  UART_CFG_DUPLEX Duplex;
  UART_CFG_BPS    BPS;
  UART_CFG_DB     DataBits;
  UART_CFG_SB     StopBits;
  UART_CFG_PARITY Parity;
  BOOLEAN         LocalLoopback;  // For debugging, internally wires TX to RX 
  FIFO*           RxFIFO;         // Optional SW extension to the 512 byte
                                  //  HW FIFO for Rx data
  FIFO*           TxFIFO;         // Optional SW extension to the 512 byte
                                  //  HW FIFO for Tx data
} UART_CONFIG;

// UART Error Status
typedef struct {
  UINT32  FramingErrCnt;
  UINT32  ParityErrCnt;
  UINT32  TxOverflowErrCnt;
  UINT32  RxOverflowErrCnt;
  UINT32  TxLastTime;     // Last Transmit Interrupt Time uS
  UINT32  TxIntrCnt;      // Consecutive Fast Interrupt Counter
  BOOLEAN IntFail;        // Excessive Interrupt Failure Flag
} UART_STATUS;

//UART Error types
typedef enum {
    TX_OVERFLOW = 0, 
    RX_OVERFLOW = 1, 
    FRAMING_ERR = 2,
    PARITY_ERR = 3,
    UART_ERROR_MAX
} UART_ERROR;
             

#pragma pack(1)
typedef struct {
  RESULT  result; 
} UART_DRV_PBIT_LOG; 
#pragma pack()
             

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined( DRV_UART_BODY )
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
EXPORT RESULT UART_Init       (SYS_APP_ID *SysLogId, void *pdata, UINT16 *psize); 
EXPORT RESULT UART_OpenPort   (UART_CONFIG* Config);
EXPORT void   UART_ClosePort  (UINT8 Port);
EXPORT void   UART_Purge      (UINT8 Port);
EXPORT RESULT UART_Transmit   (UINT8 Port, const INT8* Data,
                               UINT16 Size, UINT16* Sent);

EXPORT UINT16 UART_Write      (UINT8 Port, const INT8* Data, 
                               UINT16 Size );

EXPORT RESULT UART_Receive    (UINT8 Port,  INT8* Data,
                               UINT16 Size, UINT16* BytesReceived);
// EXPORT RESULT UART_CBIT(void);
EXPORT UART_STATUS UART_BITStatus ( UINT8 Port ); 

EXPORT UINT8   UART_CheckForTXDone(void);
EXPORT BOOLEAN UART_IntrMonitor( UINT8 port);

EXPORT __interrupt void UART_PSC0_ISR(void);
EXPORT __interrupt void UART_PSC1_ISR(void);
EXPORT __interrupt void UART_PSC2_ISR(void);
EXPORT __interrupt void UART_PSC3_ISR(void);
EXPORT void UART_PSCX_ISR( INT8 port);

#endif        // End DRV_UART_H

/*************************************************************************
 *  MODIFICATIONS
 *    $History: UART.h $
 * 
 * *****************  Version 15  *****************
 * User: Jeff Vahue   Date: 9/03/10    Time: 8:22p
 * Updated in $/software/control processor/code/drivers
 * SCR# 853 - UART Int Monitor
 * 
 * *****************  Version 14  *****************
 * User: Jim Mood     Date: 7/30/10    Time: 5:11p
 * Updated in $/software/control processor/code/drivers
 * SCR 760 YMODEM half duplex timing issues
 * 
 * *****************  Version 13  *****************
 * User: Peter Lee    Date: 7/20/10    Time: 7:24p
 * Updated in $/software/control processor/code/drivers
 * SCR #719 Update code to copy Uart Drv BIT cnt to Uart Mgr Status
 * 
 * *****************  Version 12  *****************
 * User: Contractor2  Date: 7/09/10    Time: 4:31p
 * Updated in $/software/control processor/code/drivers
 * SCR #8 Implementation: External Interrupt Monitors
 * 
 * *****************  Version 11  *****************
 * User: Contractor2  Date: 5/28/10    Time: 1:35p
 * Updated in $/software/control processor/code/drivers
 * SCR #592 UART Control Signal State on Port Close
 * Reset port registers to initial values on port close.
 * Code standard fixes including line length, spacing and elimination of
 * magic numbers.
 * 
 * *****************  Version 10  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:22p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 10/20/09   Time: 6:49p
 * Updated in $/software/control processor/code/drivers
 * Updates for JV to support PC version of program
 * 
 * *****************  Version 8  *****************
 * User: Jim Mood     Date: 10/08/09   Time: 6:13p
 * Updated in $/software/control processor/code/drivers
 * SCR #283
 * 
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 9/15/09    Time: 6:11p
 * Updated in $/software/control processor/code/drivers
 * SCR #94, #95 PBIT returns log structure to Init Mgr
 * 
 * *****************  Version 6  *****************
 * User: Peter Lee    Date: 9/14/09    Time: 3:59p
 * Updated in $/software/control processor/code/drivers
 * SCR #94 Updated Init for SET_CHECK() and return ERR CODE Struct
 * 
 * *****************  Version 5  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 2:16p
 * Updated in $/control processor/code/drivers
 * SCR #87 Function Prototype
 * 
 *
 ***************************************************************************/
 


