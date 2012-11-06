#define UART_BODY

/******************************************************************************
            Copyright (C) 2008 - 2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.


 File:        UART.c

 Description: UART driver for the MCF5485 PSC (Programmable Serial Controller)
              This file contains methods to interface (read and write) and
              manage (open, close, interrupts and buffering) the PSC
              peripherals in UART mode. The caller can:
              1. Open a port and verify is opened successfully
              2. Optionally define SW FIFO (see FIFO.c) extensions to handle
                 messages over 512 bytes
              3. Use the read, write the UART port or purge the buffer
              4. Close a port when it no longer needs the resource

              The caller shall also be responsible for using the
              UART_CheckForTXDone periodically when any of the 4 PSC ports are
              enabled in half-duplex mode.  This function will check the
              transmitter status for the empty flag, and set the port to RX
              mode if the transmitter is empty.

              The only serial mode supported for this driver is UART.

 Requires:    DIO.c (Discrete I/O driver)
              FIFO.c (First-in-First-out driver)
              ResultCodes.c (Driver/system call result codes)

  VERSION
  $Revision: 62 $  $Date: 12-11-06 3:17p $

******************************************************************************/


/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include "mcf548x.h"
#include <string.h>

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "uart.h"
#include "FIFO.h"
#include "ResultCodes.h"
#include "DIO.h"
#include "alt_basic.h"

#include "UtilRegSetCheck.h"
#include "Assert.h"
#include "Utility.h"

#include "TestPoints.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/

//For half duplex mode, these macros are used to set the data direction
#define UART0_SET_TO_TX     DIO_SetPin(GSE_TxEnb,  DIO_SetHigh)
#define UART0_SET_TO_RX     DIO_SetPin(GSE_TxEnb,  DIO_SetLow)
#define UART1_SET_TO_TX     DIO_SetPin(ACS1_TxEnb, DIO_SetHigh)
#define UART1_SET_TO_RX     DIO_SetPin(ACS1_TxEnb, DIO_SetLow)
#define UART2_SET_TO_TX     DIO_SetPin(ACS2_TxEnb, DIO_SetHigh)
#define UART2_SET_TO_RX     DIO_SetPin(ACS2_TxEnb, DIO_SetLow)
#define UART3_SET_TO_TX     DIO_SetPin(ACS3_TxEnb, DIO_SetHigh)
#define UART3_SET_TO_RX     DIO_SetPin(ACS3_TxEnb, DIO_SetLow)

//For UART channels 1 to 3, these macros set the duplex mode (full or half)
#define UART1_SET_TO_FDX    DIO_SetPin(ACS1_DX, DIO_SetLow)
#define UART1_SET_TO_HDX    DIO_SetPin(ACS1_DX, DIO_SetHigh)
#define UART2_SET_TO_FDX    DIO_SetPin(ACS2_DX, DIO_SetLow)
#define UART2_SET_TO_HDX    DIO_SetPin(ACS2_DX, DIO_SetHigh)
#define UART3_SET_TO_FDX    DIO_SetPin(ACS3_DX, DIO_SetLow)
#define UART3_SET_TO_HDX    DIO_SetPin(ACS3_DX, DIO_SetHigh)

#define UART_SET_IMR_B(Port,Mask) UART_IMR[Port] |= Mask; MCF_PSC_IMR(Port) = UART_IMR[Port]
#define UART_CLR_IMR_B(Port,Mask) UART_IMR[Port] &= ~Mask; MCF_PSC_IMR(Port) = UART_IMR[Port]

#define UART_CLR_MASK 65536

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
//local storage for the configuration of open UARTS.
static UART_CONFIG UART_Config[UART_NUM_OF_UARTS];
static UART_STATUS UART_Status[UART_NUM_OF_UARTS];
static BOOLEAN     UART_Failed[UART_NUM_OF_UARTS];

//Shadow register for the write-only IMR reg.
static UINT16 UART_IMR[UART_NUM_OF_UARTS];

const REG_SETTING UART_Registers[] =
{
  // Pin assignments for port PSC3PSC2
  //     Pin /PSC3CTS : GPIO output
  //     Pin /PSC3RTS : GPIO output
  //     Pin PSC3RXD  : PSC3 receive-data
  //     Pin PSC3TXD  : PSC3 transmit-data
  //     Pin /PSC2CTS : GPIO output
  //     Pin /PSC0RTS : GPIO output
  //     Pin PSC2RXD  : PSC2 receive-data
  //     Pin PSC2TXD  : PSC2 transmit-data

  // SET_AND_CHECK(MCF_GPIO_PDDR_PSC3PSC2, (MCF_GPIO_PDDR_PSC3PSC2_PDDR_PSC3PSC27 |
  //                                       MCF_GPIO_PDDR_PSC3PSC2_PDDR_PSC3PSC26 |
  //                                       MCF_GPIO_PDDR_PSC3PSC2_PDDR_PSC3PSC23 |
  //                                       MCF_GPIO_PDDR_PSC3PSC2_PDDR_PSC3PSC22),
  //              bInitOk);
  {(void *) &MCF_GPIO_PDDR_PSC3PSC2,
            (MCF_GPIO_PDDR_PSC3PSC2_PDDR_PSC3PSC27 | MCF_GPIO_PDDR_PSC3PSC2_PDDR_PSC3PSC26 |
             MCF_GPIO_PDDR_PSC3PSC2_PDDR_PSC3PSC23 | MCF_GPIO_PDDR_PSC3PSC2_PDDR_PSC3PSC22),
             sizeof(UINT8), 0x0, REG_SET, TRUE, RFA_FORCE_UPDATE},

  // SET_AND_CHECK(MCF_GPIO_PAR_PSC3, (MCF_GPIO_PAR_PSC3_PAR_RXD3 |
  //                                  MCF_GPIO_PAR_PSC3_PAR_TXD3), bInitOk);
  {(void *) &MCF_GPIO_PAR_PSC3, (MCF_GPIO_PAR_PSC3_PAR_RXD3 | MCF_GPIO_PAR_PSC3_PAR_TXD3),
             sizeof(UINT8), 0x0, REG_SET, TRUE, RFA_FORCE_UPDATE},

  // SET_AND_CHECK(MCF_GPIO_PAR_PSC2, (MCF_GPIO_PAR_PSC2_PAR_RXD2 |
  //                                   MCF_GPIO_PAR_PSC2_PAR_TXD2), bInitOk);
  {(void *) &MCF_GPIO_PAR_PSC2, (MCF_GPIO_PAR_PSC2_PAR_RXD2 | MCF_GPIO_PAR_PSC2_PAR_TXD2),
             sizeof(UINT8), 0x0, REG_SET, TRUE, RFA_FORCE_UPDATE},

  // Pin assignments for port PSC1PSC0
  //     Pin /PSC1CTS : GPIO output
  //     Pin /PSC0RTS : GPIO output
  //     Pin PSC1RXD  : PSC1 receive-data
  //     Pin PSC1TXD  : PSC1 transmit-data
  //     Pin /PSC0CTS : GPIO output
  //     Pin /PSC0RTS : GPIO output
  //     Pin PSC0RXD  : PSC0 receive-data
  //     Pin PSC0TXD  : PSC0 transmit-data
  // SET_AND_CHECK(MCF_GPIO_PDDR_PSC1PSC0,
  //                (MCF_GPIO_PDDR_PSC1PSC0_PDDR_PSC1PSC07 |
  //                 MCF_GPIO_PDDR_PSC1PSC0_PDDR_PSC1PSC06 |
  //                 MCF_GPIO_PDDR_PSC1PSC0_PDDR_PSC1PSC03 |
  //                 MCF_GPIO_PDDR_PSC1PSC0_PDDR_PSC1PSC02), bInitOk);
  {(void *) &MCF_GPIO_PDDR_PSC1PSC0,
            (MCF_GPIO_PDDR_PSC1PSC0_PDDR_PSC1PSC07 | MCF_GPIO_PDDR_PSC1PSC0_PDDR_PSC1PSC06 |
             MCF_GPIO_PDDR_PSC1PSC0_PDDR_PSC1PSC03 | MCF_GPIO_PDDR_PSC1PSC0_PDDR_PSC1PSC02),
             sizeof(UINT8), 0x0, REG_SET, TRUE, RFA_FORCE_UPDATE},

  // SET_AND_CHECK(MCF_GPIO_PAR_PSC1, (MCF_GPIO_PAR_PSC1_PAR_RXD1 |
  //                                   MCF_GPIO_PAR_PSC1_PAR_TXD1 ), bInitOk);
  {(void *) &MCF_GPIO_PAR_PSC1,
            (MCF_GPIO_PAR_PSC1_PAR_RXD1 | MCF_GPIO_PAR_PSC1_PAR_TXD1 ),
             sizeof(UINT8), 0x0, REG_SET, TRUE, RFA_FORCE_UPDATE},

  // SET_AND_CHECK(MCF_GPIO_PAR_PSC0, (MCF_GPIO_PAR_PSC0_PAR_RXD0 |
  //                                   MCF_GPIO_PAR_PSC0_PAR_TXD0 ), bInitOk);
  {(void *) &MCF_GPIO_PAR_PSC0,
            (MCF_GPIO_PAR_PSC0_PAR_RXD0 | MCF_GPIO_PAR_PSC0_PAR_TXD0 ),
             sizeof(UINT8), 0x0, REG_SET, TRUE, RFA_FORCE_UPDATE},

  //Interrupt level changed to 6, allowing preemption of the DPRAM interrupt
  //This is necessary to prevent deadlock in the MSFX/YMODEM routine when the
  //task waits for the UART transmission to complete.

  //Setup interrupt controller for PSC0,PSC1,PSC2,PSC3
  //PSC module RX and TX interrupts are not unmasked until a port is opened
  // SET_AND_CHECK(MCF_INTC_ICR32, (MCF_INTC_ICRn_IL(0x6) |
  //                                MCF_INTC_ICRn_IP(0x4)), bInitOk);
  {(void *) &MCF_INTC_ICR32, (MCF_INTC_ICRn_IL(0x6) | MCF_INTC_ICRn_IP(0x4)),
             sizeof(UINT8), 0x0, REG_SET, TRUE, RFA_FORCE_UPDATE},

  // SET_AND_CHECK(MCF_INTC_ICR33, (MCF_INTC_ICRn_IL(0x6) |
  //                                MCF_INTC_ICRn_IP(0x2)), bInitOk);
  {(void *) &MCF_INTC_ICR33, (MCF_INTC_ICRn_IL(0x6) | MCF_INTC_ICRn_IP(0x2)),
             sizeof(UINT8), 0x0, REG_SET, TRUE, RFA_FORCE_UPDATE},

  // SET_AND_CHECK(MCF_INTC_ICR34, (MCF_INTC_ICRn_IL(0x6) |
  //                                MCF_INTC_ICRn_IP(0x1)), bInitOk);
  {(void *) &MCF_INTC_ICR34, (MCF_INTC_ICRn_IL(0x6) | MCF_INTC_ICRn_IP(0x1)),
             sizeof(UINT8), 0x0, REG_SET, TRUE, RFA_FORCE_UPDATE},

  // SET_AND_CHECK(MCF_INTC_ICR35, MCF_INTC_ICRn_IL(0x3), bInitOk);
  {(void *) &MCF_INTC_ICR35, MCF_INTC_ICRn_IL(0x6), sizeof(UINT8), 0x0, REG_SET,
            TRUE, RFA_FORCE_UPDATE},

  // SET_CHECK_MASK_AND(MCF_INTC_IMRH, (MCF_INTC_IMRH_INT_MASK32 | MCF_INTC_IMRH_INT_MASK33 |
  //                                    MCF_INTC_IMRH_INT_MASK34 | MCF_INTC_IMRH_INT_MASK35),
  //                                    bInitOk);
  {(void *) &MCF_INTC_IMRH, (MCF_INTC_IMRH_INT_MASK32 | MCF_INTC_IMRH_INT_MASK33 |
                             MCF_INTC_IMRH_INT_MASK34 | MCF_INTC_IMRH_INT_MASK35),
                             sizeof(UINT32), 0x0, REG_SET_MASK_AND, TRUE, RFA_FORCE_UPDATE},
};


/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
RESULT UART_PBIT(void);

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/*****************************************************************************
 * Function:    UART_Init
 *
 * Description: Setup the UART module to its initial state
 *              Initializes PSC I/O ports, pin assignments and direction
 *
 * Parameters:  [out] SysLogId - ptr to return SYS_APP_ID
 *              [out] pdata    - ptr to buffer to return result data
 *              [out] psize    - ptr to size var to return size of result data
 *
 * Returns:     DRV_RESULT: DRV_OK=Init successful
 *                          !DRV_OK= See ResultCodes.h
 *
 * Notes:       None
 *
 ****************************************************************************/
RESULT UART_Init (SYS_APP_ID *SysLogId, void *pdata, UINT16 *psize)
{
  UINT32 i;
  BOOLEAN bInitOk;
  UART_DRV_PBIT_LOG  *pdest;

  pdest = (UART_DRV_PBIT_LOG *) pdata;
  memset ( pdest, 0, sizeof(UART_DRV_PBIT_LOG) );
  pdest->result = DRV_OK;

  *psize = sizeof(UART_DRV_PBIT_LOG);
  bInitOk = TRUE;

  for(i = 0; i < (UINT32) UART_NUM_OF_UARTS; i++)
  {
    memset(&UART_Config[i], 0, sizeof(UART_Config[0]));
    memset(&UART_Status[i], 0, sizeof(UART_Status[0]));
    UART_Failed[i] = FALSE;
    UART_IMR[i] = 0;
  }

  bInitOk = TRUE;
  for (i = 0; i < (sizeof(UART_Registers)/sizeof(REG_SETTING)); i++)
  {
    bInitOk &= RegSet( (REG_SETTING_PTR) &UART_Registers[i] );
  }

  // default to error condition SUCCESS
  pdest->result = DRV_UART_PBIT_REG_INIT_FAIL;
  *SysLogId = DRV_ID_PSC_PBIT_REG_INIT_FAIL;

  // Perform PBIT only if Reg Init SUCCESS
  if ( TRUE == STPU( bInitOk, eTpPsc1579))
  {
    /*
    SCR 1110 It is necessary to toggle the serial isolator control lines
    to make sure the output state matches the input state.  Toggle each line
    high then low.  See HCPL-092J Datasheet.    */

    /*Set RX/TX enable high*/
    UART0_SET_TO_TX;
    UART1_SET_TO_TX;
    UART2_SET_TO_TX;
    UART3_SET_TO_TX;
    /*Set RX/TX enable low*/
    UART0_SET_TO_RX;
    UART1_SET_TO_RX;
    UART2_SET_TO_RX;
    UART3_SET_TO_RX;

    /*Set RX/TX enable high*/
    UART1_SET_TO_HDX;
    UART2_SET_TO_HDX;
    UART3_SET_TO_HDX;
    /*Set RX/TX enable low*/
    UART1_SET_TO_FDX;
    UART2_SET_TO_FDX;
    UART3_SET_TO_FDX;

    pdest->result = UART_PBIT();
    *SysLogId = DRV_ID_PSC_PBIT_FAIL;  // Note: If ->result == DRV_OK this value is ignored.
  }
  else
  {
    // All four ports will be disabled if set of ColdFire Processor fails
    for(i = 0; i < (UINT32) UART_NUM_OF_UARTS; i++)
    {
      UART_Failed[i] = TRUE;
    }

#ifdef STE_TP
    // When doing coverage testing ensure the GSE port is valid
    UART_Failed[0] = FALSE;
#endif

  }

  return pdest->result;

}


/*****************************************************************************
 * Function:    UART_OpenPort
 *
 * Description: Initialize and open the specified port for RX/TX of data
 *
 * Parameters:  [in]  UART_Config structure, that contains the following:
 *              Channel
 *              Mode/Duplex
 *              baud rate
 *              start bits
 *              data bits
 *              stop bits
 *              parity
 *              Interrupt Settings
 *              RxFIFO  *Optional Rx and Tx buffer extensions.
 *              TxFIFO
 *
 *              Optional buffer extensions may be used, however the hardware provides
 *              512 send and receive buffers by default
 *              A copy of the UART_Config structure is made locally, so the
 *              structure passed to OpenPort need not have static class.
 *
 * Returns:     RESULT
 *                DRV_OK: Operation succeeded
 *                DRV_UART_ERR_PORT_NOT_OPEN: Operation failed, port already open
 *
 * Notes:       None
 *
 ****************************************************************************/
RESULT UART_OpenPort(UART_CONFIG* Config)
{
  register UINT16 divider;
  BOOLEAN bInitOk;
  RESULT result;
  UINT32 port;

  bInitOk = FALSE;
  result = DRV_OK;
  port = (UINT32)Config->Port;

  ASSERT_MESSAGE( port < (UINT32)UART_NUM_OF_UARTS, "Invalid UART Port (%d)", port);
  ASSERT_MESSAGE( !UART_Config[port].Open, "Port is open(%d)", port);

  // If UART has previously failed - don't open
  if ( !UART_Failed[port])
  {
    bInitOk = TRUE;

    //Copy configuration over to local storage
    memcpy(&UART_Config[port], Config, sizeof(UART_CONFIG));
    // clear error counters
    memset(&UART_Status[port], 0, sizeof(UART_STATUS));

    //Set GPIO port register to enable PSC(port) signals
    switch (port)
    {
      case UART_0:
        bInitOk &= RegSetOrUpdate((void *) &MCF_GPIO_PAR_PSC0,
                                  (MCF_GPIO_PAR_PSC0_PAR_TXD0 | MCF_GPIO_PAR_PSC0_PAR_RXD0),
                                  sizeof(UINT8), 0x00);
        break;

      case UART_1:
        bInitOk &= RegSetOrUpdate((void *) &MCF_GPIO_PAR_PSC1,
                                  (MCF_GPIO_PAR_PSC1_PAR_TXD1 | MCF_GPIO_PAR_PSC1_PAR_RXD1),
                                  sizeof(UINT8), 0x00);
        break;

    case UART_2:
        bInitOk &= RegSetOrUpdate((void *) &MCF_GPIO_PAR_PSC2,
                                  (MCF_GPIO_PAR_PSC2_PAR_TXD2 | MCF_GPIO_PAR_PSC2_PAR_RXD2),
                                  sizeof(UINT8), 0x00);
        break;

    case UART_3:
        bInitOk &= RegSetOrUpdate((void *) &MCF_GPIO_PAR_PSC3,
                                  (MCF_GPIO_PAR_PSC3_PAR_TXD3 |MCF_GPIO_PAR_PSC3_PAR_RXD3),
                                  sizeof(UINT8), 0x00);
        break;

    default:
       FATAL("Unrecognized UART Port = %d", port);
       break;
    }

    //Put PSC in UART mode
    bInitOk &= RegSetOrUpdate((void *) &MCF_PSC_SICR(port), MCF_PSC_SICR_SIM_UART,
                              sizeof(UINT8), 0x00);

    //Generate the Rx and Tx baud rate from the System Clock - Write Only Register
    MCF_PSC_CSR(port) = (0
        | MCF_PSC_CSR_RCSEL_SYS_CLK
        | MCF_PSC_CSR_TCSEL_SYS_CLK);

    //Calculate baud settings - Counter Can not verify / check.
    divider = (UINT16)((UART_SYSTEM_CLOCK_MHZ*1000000)/((UINT16)Config->BPS * 32));
    MCF_PSC_CTUR(port) =  (UINT8) ((divider >> 8) & 0xFF);
    MCF_PSC_CTLR(port) =  (UINT8) (divider & 0xFF);

    //Reset transmitter, receiver, mode register, and error conditions - Write Only Register
    MCF_PSC_CR(port) = MCF_PSC_CR_RESET_RX;
    MCF_PSC_CR(port) = MCF_PSC_CR_RESET_TX;
    MCF_PSC_CR(port) = MCF_PSC_CR_RESET_ERROR;
    MCF_PSC_CR(port) = MCF_PSC_CR_BKCHGINT;
    MCF_PSC_CR(port) = MCF_PSC_CR_RESET_MR;

    //Configure parity, word size, and set RX interrupt to FIFO full
    //Register access require double access as this register is PSCMR1/PSCMR2
    //                                                 (Ref 27-5 MCF Reference)
    //First Access to MCF_PSC_MR is PSCMR1n
    MCF_PSC_MR(port) = ((( (UINT8)Config->Parity )&0x07)<<2) | //This sets PM and PT at same time
                          MCF_PSC_MR_BC( (UINT8)Config->DataBits ) |
                          MCF_PSC_MR_RXIRQ ;

    //Configure loopback and stop bits
    // Second Access to MCF_PSC_MR is PSCMR2n
    MCF_PSC_MR(port) = ((Config->LocalLoopback ? MCF_PSC_MR_CM_LOCAL_LOOP : 0) |
                         MCF_PSC_MR_SB( (UINT8)Config->StopBits ));

    // Check PSCMR1n and then PSCMR2n settings of MCF_PSC_MR. Need to reset pointer to PSCMR1n
    MCF_PSC_CR(port) = MCF_PSC_CR_RESET_MR;

    // Need to mask away MCF_PSC_MR_ERR as this is read only and
    // in current version always set to "1" !
    CHECK_SETTING ( (MCF_PSC_MR(port) & (~MCF_PSC_MR_ERR)),
                    ( ((( (UINT8)Config->Parity )&0x07)<<2) |
                       MCF_PSC_MR_BC( (UINT8)Config->DataBits ) |
                       MCF_PSC_MR_RXIRQ),
                    bInitOk);

    CHECK_SETTING ( MCF_PSC_MR(port),
                    ( ( (Config->LocalLoopback ? MCF_PSC_MR_CM_LOCAL_LOOP : 0)
                        | MCF_PSC_MR_SB( (UINT8)Config->StopBits ) ) ),
                    bInitOk );


    //If a software FIFO is defined, enable RX FIFO Full interrupt
    ASSERT ( Config->RxFIFO == NULL )
/*  Uncomment if SW Rx FIFO Buffer is used in the future
    if(Config->RxFIFO != NULL)
    {
      UART_SET_IMR_B(port,MCF_PSC_IMR_RXRDY_FU);
    }
*/

    //Interrupt when RX is half full
     bInitOk &= RegSetOrUpdate((void *) &MCF_PSC_RFAR(port),
                                MCF_PSC_RFAR_ALARM(256),
                                sizeof(UINT16),
                                0x00);

    //Interrupt when TX has fewer than 10 chars to TX
     bInitOk &= RegSetOrUpdate((void *) &MCF_PSC_TFAR(port),
                                MCF_PSC_TFAR_ALARM(10),
                                sizeof(UINT16),
                                0x00);

    // Write Only Register
    MCF_PSC_CR(port) =(0
        | MCF_PSC_CR_RX_ENABLED
        | MCF_PSC_CR_TX_ENABLED);

    //Set GPIO port register to enable PSC(port) signals
    switch (port)
    {
    case UART_0:
       // Not possible to set port as full duplex
       ASSERT (Config->Duplex != UART_CFG_DUPLEX_FULL);

       break;

    case UART_1:
      if(Config->Duplex == UART_CFG_DUPLEX_FULL)
      {
        UART1_SET_TO_FDX;
      }
      else
      {
        UART1_SET_TO_HDX;
      }
      break;

    case UART_2:
      if(Config->Duplex == UART_CFG_DUPLEX_FULL)
      {
        UART2_SET_TO_FDX;
      }
      else
      {
        UART2_SET_TO_HDX;
      }
      break;

    case UART_3:
      if(Config->Duplex == UART_CFG_DUPLEX_FULL)
      {
        UART3_SET_TO_FDX;
      }
      else
      {
        UART3_SET_TO_HDX;
      }
      break;

    default:
      FATAL("Unrecognized UART Port = %d", port);
      break;
    }

#ifdef WIN32
/*vcast_dont_instrument_start*/
    bInitOk = TRUE;
/*vcast_dont_instrument_end*/
#endif
  }

  if ( bInitOk  )
  {
    //Ensure the port is opened in case this was not set TRUE in the Config parameter
    UART_Config[port].Open = TRUE;
  }
  else
  {
    result = DRV_UART_ERR_PORT_NOT_OPEN;
    UART_Failed[port] = TRUE;
  }

  return result;
}

/*****************************************************************************
 * Function:    UART_ClosePort
 *
 * Description: Close and disable the specified UART port
 *
 * Parameters:  [in] Port:  Port number to be closed 0,1,2 or 3
 *
 * Returns:     None
 *
 * Notes:       ASSERTS when Operation would fail due to invalid port
 ****************************************************************************/
void UART_ClosePort(UINT8 Port)
{
  UINT32 intrLevel;

  ASSERT_MESSAGE( Port < (UINT8) UART_NUM_OF_UARTS, "Invalid UART Port (%d)", Port);
  intrLevel = __DIR();
  UART_CLR_IMR_B(Port,UART_CLR_MASK); //Disable port interrupts
  __RIR(intrLevel);
  UART_Config[Port].Open  = FALSE;    //Close port

  //Reset port to full duplex & port disabled
  switch (Port)
  {
  case UART_0:
    // Note: Port 0 is half duplex only
    UART0_SET_TO_RX;
    break;

  case UART_1:
    UART1_SET_TO_RX;
    UART1_SET_TO_FDX;
    break;

  case UART_2:
    UART2_SET_TO_RX;
    UART2_SET_TO_FDX;
    break;

  case UART_3:
    UART3_SET_TO_RX;
    UART3_SET_TO_FDX;
    break;

  default:
    FATAL("Unrecognized UART Port = %d", Port);
    break;
  }

  // Write Only Register
  MCF_PSC_CR(Port) =(0
      | MCF_PSC_CR_RX_DISABLED
      | MCF_PSC_CR_TX_DISABLED);
}

/*****************************************************************************
 * Function:    UART_Purge
 *
 * Description: Clear the software and hardware TX and RX FIFO buffers of
 *              the specified port
 *
 * Parameters:  [in] Port:  The port buffers to be purged
 *
 * Returns:     None
 *
 * Notes:
 ****************************************************************************/
void UART_Purge(UINT8 Port)
{
  UINT32 intrLevel;
  ASSERT_MESSAGE( Port < (UINT8) UART_NUM_OF_UARTS, "Invalid UART Port (%d)", Port);
  ASSERT_MESSAGE( UART_Config[Port].Open, "Port not open(%d)", Port);

  //Kill TX/RX interrupts
  intrLevel = __DIR();
  UART_CLR_IMR_B(Port,UART_CLR_MASK);
  __RIR(intrLevel);

  //Flush SW FIFOs (if present)
  ASSERT ( UART_Config[Port].RxFIFO == NULL );
/* Uncomment if SW_Rx_FIFO are used
  if( UART_Config[Port].RxFIFO != NULL )
  {
    FIFO_Flush(UART_Config[Port].RxFIFO);
  }
*/

  ASSERT ( UART_Config[Port].TxFIFO == NULL );
/* Although SW TX fifo used in GSE, the UART_Purge is never called from
   GSE.  If used by GSE, uncomment code below.
  if( UART_Config[Port].TxFIFO != NULL )
  {
    FIFO_Flush(UART_Config[Port].TxFIFO);
  }
*/

  //Flush the hardware
  MCF_PSC_CR(Port) = MCF_PSC_CR_RESET_RX;
  MCF_PSC_CR(Port) = MCF_PSC_CR_RESET_TX;
  MCF_PSC_CR(Port) =(0
      | MCF_PSC_CR_RX_ENABLED
      | MCF_PSC_CR_TX_ENABLED);

  //If a software FIFO is defined, enable RX FIFO Full interrupt

  ASSERT ( UART_Config[Port].RxFIFO == NULL );
/* Uncomment if SW_Rx FIFO are used
  if(UART_Config[Port].RxFIFO != NULL)
  {
    intrLevel = __DIR();
    UART_SET_IMR_B(Port,MCF_PSC_IMR_RXRDY_FU);
    __RIR(intrLevel);
  }
*/

}

/*****************************************************************************
 * Function:    UART_Transmit
 *
 * Description: Copies data (Data) of (Size) to the Tx FIFO for transmission
 *              via the selected UART (Port).  If the UART port is not open,
 *              or the data does not fit in the Tx FIFO buffer, FALSE is returned.
 *              Additionally, if the data does not fit, the TxOverrun counter is
 *              incremented
 *
 * Parameters:  [in] Port: The port to transmit data to 0,1,2, or 3
 *              [in] Data: Pointer to the data to transmit
 *              [in] Size: Size of the data in the Data buffer
 *              [out] BytesSent: Optional parameter to notify the caller how
 *                    many bytes were actually copied to the FIFO.  Set to
 *                    NULL if not needed
 *
 * Returns:     DRV_RESULT: DRV_OK="Size" number of bytes copied to FIFO.
 *                          DRV_UART_TX_OVERFLOW= < Size bytes were copied
 *                          Other = See ResultCodes.h
 *
 * Notes:
 ****************************************************************************/
RESULT UART_Transmit(UINT8 Port, const INT8* Data, UINT16 Size, UINT16* Sent)
{
  UINT32 intrLevel;

  UINT16 BytesSent = 0;
  RESULT result = DRV_OK;

  ASSERT_MESSAGE( Port < (UINT8) UART_NUM_OF_UARTS, "Invalid UART Port (%d)", Port);
  ASSERT_MESSAGE( UART_Config[Port].Open, "Port not open(%d)", Port);

  // Enable Tx
  switch (Port)
  {
  case UART_0:
      UART0_SET_TO_TX;
      break;
  case UART_1:
      UART1_SET_TO_TX;
      break;
  case UART_2:
      UART2_SET_TO_TX;
      break;
  case UART_3:
      UART3_SET_TO_TX;
      break;
  default:
      FATAL("Unrecognized UART Port = %d", Port);
      break;
  }

  //NO SOFTWARE FIFO - Use hardware FIFO only
  if(UART_Config[Port].TxFIFO == NULL)
  {
    BytesSent = UART_Write( Port, Data, Size );
  }
  else
  {
#ifndef WIN32
    if(Size > BytesSent)
    {
      // local var assigned so function is not called twice if it is the min value see MIN
      UINT32 fifoFree = FIFO_FreeBytes(UART_Config[Port].TxFIFO);

      // Which one is less
      BytesSent = MIN( fifoFree, Size);
      FIFO_PushBlock(UART_Config[Port].TxFIFO, Data, BytesSent);

      intrLevel = __DIR();
      UART_SET_IMR_B(Port,MCF_PSC_IMR_TXRDY);
      __RIR(intrLevel);
    }
#else
    /*vcast_dont_instrument_start*/
    BytesSent = UART_Write(Port, Data, Size);
    /*vcast_dont_instrument_end*/
#endif
  }

  //Verify that between the HW and SW FIFOs, all the bytes in Data have been queued up
  //to TX
  if(Size != BytesSent)
  {
    UART_Status[Port].TxOverflowErrCnt++;
    result = DRV_UART_TX_OVERFLOW;
  }

  if(Sent != NULL)
  {
    *Sent = BytesSent;
  }

  return result;
}

/*****************************************************************************
* Function:    UART_Write
*
* Description: Actual Copy of data (Data) of (Size) to the Tx FIFO for
*              transmission via the selected UART (Port).  Copy until the Tx
*              FIFO is full or all data is written.
*
* Parameters:  [in] Port: The port to transmit data to 0,1,2, or 3
*              [in] Data: Pointer to the data to transmit
*              [in] Size: Size of the data in the Data buffer
*
* Returns:     The number of bytes written to the FIFO
*
* Notes:
****************************************************************************/
UINT16 UART_Write(  UINT8 Port, const INT8* Data, UINT16 Size)
{
    UINT16 BytesSent = 0;
#ifndef WIN32
    while( (MCF_PSC_TFCNT(Port) < UART_MCF_PSC_FIFO_SIZE) && Size > BytesSent)
    {
        //Must cast TX buffer as Byte* to generate MOVE.B
        *((UINT8 *) &MCF_PSC_TB(Port)) = *(Data + BytesSent);
        BytesSent++;
    }
#else
/*vcast_dont_instrument_start*/
    if (UART_Config[Port].TxFIFO != NULL)
    {
        // We are in Normal mode
        UART_Config[Port].TxFIFO->Cnt = 0;
        BytesSent = hw_UART_Transmit( Port, Data, Size, 0);
    }
    else
    {
        // test mode wraparound
        BytesSent = hw_UART_Transmit( Port, Data, Size, 1);

    }
/*vcast_dont_instrument_end*/
#endif

    return BytesSent;
}

/*****************************************************************************
 * Function:    UART_Receive
 *
 * Description: Reads (Size) number of characters from the RX buffer.  The bytes
 *              may be located in both the hardware 512 byte FIFO, as well as
 *              the optional software FIFO extension.  Both are read and
 *              concatenated into the (Data) buffer, up to (Size) bytes.
 *
 * Parameters:  [in]  Port: UART port to read from
 *              [in]  Data: A buffer to receive the data from the RX buffer
 *              [in]  Size: Size of Data buffer
 *              [out] BytesReceived: Number of chars read from the port FIFO
 *                                   This is optional and can be set to NULL
 *
 * Returns:     DRV_OK = Read okay
 *              Other = See ResultCodes.h
 * Notes:
 ****************************************************************************/
RESULT UART_Receive( UINT8 Port, INT8* Data, UINT16 Size, UINT16* BytesReceived)
{
  UINT16 ReadCount = 0;
  RESULT result = DRV_OK;
#ifndef WIN32
  UINT32 intrLevel;
#endif

  ASSERT_MESSAGE( Port < (UINT8) UART_NUM_OF_UARTS, "Invalid UART Port (%d)", Port);
  ASSERT_MESSAGE( UART_Config[Port].Open, "Port not open(%d)", Port);

  if ( result == DRV_OK )
  {
    //Check for hardware or RX overflow errors
    if(MCF_PSC_SR(Port) & MCF_PSC_SR_OE)
    {
      UART_Status[Port].RxOverflowErrCnt++;
      result = DRV_UART_RX_OVERFLOW;
      //Reset RX and RX FIFO, then reset error flags and re-enable RX.
      MCF_PSC_CR(Port) = MCF_PSC_CR_RESET_RX;
      MCF_PSC_CR(Port) = MCF_PSC_CR_RESET_ERROR;
      MCF_PSC_CR(Port) = MCF_PSC_CR_RX_ENABLED;
      ASSERT ( UART_Config[Port].RxFIFO == NULL );
/*    Uncomment if Rx SW FIFO are defined
      if(UART_Config[Port].RxFIFO != NULL)
      {
        FIFO_Flush(UART_Config[Port].RxFIFO);
      }
*/
    }
    if(MCF_PSC_SR(Port) & MCF_PSC_SR_FE_PHYERR)
    {
      result = DRV_UART_RX_HW_ERROR;
      UART_Status[Port].FramingErrCnt++;
      //Reset RX and RX FIFO, then reset error flags and re-enable RX.
      MCF_PSC_CR(Port) = MCF_PSC_CR_RESET_RX;
      MCF_PSC_CR(Port) = MCF_PSC_CR_RESET_ERROR;
      MCF_PSC_CR(Port) = MCF_PSC_CR_RX_ENABLED;
      ASSERT ( UART_Config[Port].RxFIFO == NULL );
/*    Uncomment if Rx SW FIFO are defined
      if(UART_Config[Port].RxFIFO != NULL)
      {
        FIFO_Flush(UART_Config[Port].RxFIFO);
      }
*/
    }
    if(MCF_PSC_SR(Port) & MCF_PSC_SR_PE_CRCERR)
    {
      result = DRV_UART_RX_HW_ERROR;
      UART_Status[Port].ParityErrCnt++;
      //Reset RX and RX FIFO, then reset error flags and re-enable RX.
      MCF_PSC_CR(Port) = MCF_PSC_CR_RESET_RX;
      MCF_PSC_CR(Port) = MCF_PSC_CR_RESET_ERROR;
      MCF_PSC_CR(Port) = MCF_PSC_CR_RX_ENABLED;
      ASSERT ( UART_Config[Port].RxFIFO == NULL );
/*    Uncomment if Rx SW FIFO are defined
      if(UART_Config[Port].RxFIFO != NULL)
      {
        FIFO_Flush(UART_Config[Port].RxFIFO);
      }
*/
    }

    if(DRV_OK == result)
    {
      //Check for data in the Rx SW FIFO if SW FIFO enabled,
      //and empty RxFIFO into the Data buffer
      ASSERT(UART_Config[Port].RxFIFO == NULL);
      /* Receive SW FIFO NOT USED IN FAST, DEAD CODE
      if(UART_Config[Port].RxFIFO != NULL)
      {
        while(FIFO_Pop( UART_Config[Port].RxFIFO,Data ) && ReadCount < Size)
        {
          Data++;
          ReadCount++;
          }
      }
      */
      //Read data (if available) from the RX HW FIFO
      //Disable interrupts until one byte is read, to prevent a race condition
      //interrupting between testing the HW FIFO count and reading the first byte
  #ifndef WIN32
      intrLevel = __DIR();
      while(MCF_PSC_RFCNT(Port) > 0 && ReadCount < Size)
      {
        ReadCount++;
        *Data++ = *((UINT8 *) &MCF_PSC_RB(Port)); // Cast RX buffer as Byte* to generate MOVE.B
      }
      __RIR(intrLevel); // move to after While loop in case there is nothing in the HW FIFO
  #else
  /*vcast_dont_instrument_start*/
      ReadCount = hw_UART_Receive( Port, Data, Size);
  /*vcast_dont_instrument_end*/
  #endif

    }

    if( BytesReceived != NULL )
    {
      *BytesReceived = ReadCount;
    }
  }

  return result;
}

/*****************************************************************************
 * Function:    UART_IntrMonitor
 *
 * Description: Test if the Uart Interrupt Monitor has failed.
 *
 * Parameters:  port (i) - which port to check
 *
 * Returns:     TRUE when the Interrupt monitor has failed,
 *              FALSE otherwise
 *
 * Notes:       None
 *
 ****************************************************************************/
BOOLEAN UART_IntrMonitor( UINT8 port)
{
    return (UART_Config[port].Open && UART_Status[port].IntFail);
}

/*****************************************************************************
 * Function:    UART_CheckForTXDone
 *
 * Description: Test the TXEMP_URERR bit of all open UART ports.
 *              If a transmitter is empty, open, and half duplex, this routine
 *              will negate the GPIO pin controlling the transmit enable
 *              and enter receive mode.
 *
 *              This function should be called frequently enough to ensure the
 *              port will be reset to receive before the connected device
 *              enters transmit mode.
 *
 * Parameters:  None
 *
 * Returns:     UINT8: Bit fields [0:3] set bit indicates UART port 0:3 is
 *                     currently transmitting respectively.
 *
 * Notes:       This function has been made interrupt safe, so it may be
 *              called in the idle loop.
 ****************************************************************************/
UINT8 UART_CheckForTXDone(void)
{
 UINT8 i;
 UINT32 intrLevel;
 UINT8 TxFlags = 0;
 BOOLEAN fifoEmpty = TRUE;

  for(i = 0; i < (UINT8) UART_NUM_OF_UARTS; i++)          //Check all ports, if open
  {
    intrLevel = __DIR();
    if (UART_Config[i].Open)
    {                                             //Initial assumption,
                                                  //set UART_TRANSMITTING flag
      if ( UART_Config[i].TxFIFO != NULL && UART_Config[i].TxFIFO->Cnt != 0)
      {
         fifoEmpty = FALSE;
      }

      TxFlags |= 1<<i;
      if ( fifoEmpty && (MCF_PSC_SR(i) & MCF_PSC_SR_TXEMP_URERR))
      {                                           //If TX shifter is empty, set to RX mode
        switch(i)                                 //and clear the UART_TRANSMITTING flag
        {
          case UART_0:
            UART0_SET_TO_RX;
            TxFlags &= ~UART0_TRANSMITTING;
            break;

          case UART_1:
            UART1_SET_TO_RX;
            TxFlags &= ~UART1_TRANSMITTING;
            break;

          case UART_2:
            UART2_SET_TO_RX;
            TxFlags &= ~UART2_TRANSMITTING;
            break;

          case UART_3:
            UART3_SET_TO_RX;
            TxFlags &= ~UART3_TRANSMITTING;
            break;

          default:
            FATAL("Unrecognized UART Port = %d", i);
            break;
        }
      }
    }
    __RIR(intrLevel);
  }
  return TxFlags;
}


/*****************************************************************************
 * Function:    UART_BITStatus
 *
 * Description: Returns the current BIT counters for the UART Interface
 *
 * Parameters:  Port - UART channel
 *
 * Returns:     UART_STATUS - structure for uart channel
 *
 * Notes:       none
 *
 ****************************************************************************/
UART_STATUS UART_BITStatus (UINT8 Port)
{
  return ( UART_Status[Port] );
}


/*****************************************************************************
 * Function:    UART_PSC0_ISR
 * Description: INTERRUPT SERVICE ROUTINE for PSC0
 *              This routine handles RX FIFO, TX FIFO, and Bus Error interrupt
 *              causes
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:
 ****************************************************************************/
__interrupt void UART_PSC0_ISR(void)
{
  UART_PSCX_ISR(0);
}

/*****************************************************************************
 * Function:    UART_PSC1_ISR
 * Description: INTERRUPT SERVICE ROUTINE for PSC1
 *              This routine handles RX FIFO, TX FIFO, and Bus Error interrupt
 *              causes
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:
 ****************************************************************************/
__interrupt void UART_PSC1_ISR(void)
{
  UART_PSCX_ISR(1);
}

/*****************************************************************************
 * Function:    UART_PSC2_ISR
 * Description: INTERRUPT SERVICE ROUTINE for PSC2
 *              This routine handles RX FIFO, TX FIFO, and Bus Error interrupt
 *              causes
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:
 ****************************************************************************/
__interrupt void UART_PSC2_ISR(void)
{
  UART_PSCX_ISR(2);
}

/*****************************************************************************
 * Function:    UART_PSC3_ISR
 * Description: INTERRUPT SERVICE ROUTINE for PSC3
 *              This routine handles RX FIFO, TX FIFO, and Bus Error interrupt
 *              causes
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:
 ****************************************************************************/
__interrupt void UART_PSC3_ISR(void)
{
  UART_PSCX_ISR(3);
}

/*****************************************************************************
* Function:    UART_PSCX_ISR
* Description: INTERRUPT SERVICE ROUTINE for PSCx
*              This routine handles RX FIFO, TX FIFO, and Bus Error interrupt
*              causes
*
* Parameters:  [in] Port (i): the port number
*
* Returns:     None
*
* Notes:
****************************************************************************/
void UART_PSCX_ISR( INT8 port)
{
  static volatile UINT16* isr[UART_NUM_OF_UARTS] = {
      &MCF_PSC_ISR0,
      &MCF_PSC_ISR1,
      &MCF_PSC_ISR2,
      &MCF_PSC_ISR3,
  };

  UART_CONFIG*  pUartCfg;
  UART_STATUS*  pUartSts;
  UINT32  entryTime;
  UINT8   buf[512];
  INT32   i,count;

  entryTime = TTMR_GetHSTickCount() / TTMR_HS_TICKS_PER_uS;
  pUartCfg = &UART_Config[port];
  pUartSts = &UART_Status[port];

  // GSE is the only half duplex
  if (port == (INT8) UART_0)
  {
    UART0_SET_TO_TX;
  }

  //TX Interrupt, fill the HW FIFO from the SW FIFO
  if ( (*isr[port] & MCF_PSC_ISR_TXRDY) && (pUartCfg->TxFIFO != NULL))
  {
    count = MIN(pUartCfg->TxFIFO->CmpltCnt,UART_MCF_PSC_FIFO_SIZE-MCF_PSC_TFCNT(port));
    FIFO_PopBlock(pUartCfg->TxFIFO,buf,count);
    for(i = 0;i<count;i++)
    {
      *((UINT8 *) &MCF_PSC_TB(port)) = buf[i];
    }
    if(pUartCfg->TxFIFO->CmpltCnt == 0)  //No more bytes to TX, shut off TX empty int.
    {
      UART_CLR_IMR_B(port, MCF_PSC_IMR_TXRDY);
    }
    // Check if interrupts coming too fast
    if ( TPU( (entryTime - pUartSts->TxLastTime), eTpPscIsr3711) < UART_MIN_INTERRUPT_TIME )
    {
      pUartSts->TxIntrCnt++;
    }
    else
    {
      pUartSts->TxIntrCnt = 0;    // reset fast interrupt counter
    }
    pUartSts->TxLastTime = entryTime;
  }
  else
  {
    FATAL( "Spurious UART Rx Interrupt Port(%d) ISR(0x%x) Mask(0x%x)",
      port, *isr[port], MCF_PSC_IMR(port));
  }

  // check interrupt count limit exceeded
  if ( pUartSts->TxIntrCnt > UART_MAX_INTERRUPT_CNT)
  {
    pUartSts->IntFail = TRUE;

    // because we failed remove the FIFO so we can limp along with short I/O responses
    // Y-Modem will be broken, but the system is broken.
    pUartCfg->TxFIFO = NULL;

    // Turn Tx interrupts off - leave the UART functional ... but crippled
    UART_CLR_IMR_B(port, MCF_PSC_IMR_TXRDY);

    // Turn off the UART functionality
    //MCF_PSC_CR(port) =(MCF_PSC_CR_RX_DISABLED | MCF_PSC_CR_TX_DISABLED);
  }
}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/
/*****************************************************************************
 * Function:    UART_PBIT
 *
 * Description: Power on built in test.  Opens all 4 UART ports in
 *              local-loopback mode, prints a short string out each port
 *              and verifies that it is correctly received.
 *              If the string received does not match the string sent, or
 *              a hardware error occurs, the channel that first fails the test
 *              is returned in the result code
 *
 * Parameters:  void
 *
 * Returns:     RESULT: DRV_OK: PBIT passed
 *                      DRV_UART_n_PBIT_FAILED: channel "n", that failed the
 *                                              PBIT
 *
 * Notes:
 ****************************************************************************/
RESULT UART_PBIT(void)
{
#ifndef WIN32
#define FUDGY_DELAY 200000
#else
#define FUDGY_DELAY 1
#endif

#define UART_PBIT_MSG_SIZE 16

  UART_CONFIG config;
  INT8 RxBuf[UART_PBIT_MSG_SIZE];
  UINT32 i;
  UINT16 BytesReceived;
  RESULT result;
  RESULT UARTresult;
  RESULT PBITResultCodes[UART_NUM_OF_UARTS];
  INT8 TestStr[UART_PBIT_MSG_SIZE];
  UINT32 startTime;

  memset(&config,0,sizeof(config));
  result = DRV_OK;
  PBITResultCodes[UART_0] = DRV_UART_0_PBIT_FAILED;
  PBITResultCodes[UART_1] = DRV_UART_1_PBIT_FAILED;
  PBITResultCodes[UART_2] = DRV_UART_2_PBIT_FAILED;
  PBITResultCodes[UART_3] = DRV_UART_3_PBIT_FAILED;
  strncpy_safe( TestStr, UART_PBIT_MSG_SIZE, "Test Message", _TRUNCATE );

  config.Port             = (BYTE) UART_0;
  config.Duplex           = UART_CFG_DUPLEX_HALF;   // UART 0 only half duplex
  config.BPS              = UART_CFG_BPS_115200;
  config.DataBits         = UART_CFG_DB_8;
  config.StopBits         = UART_CFG_SB_1;
  config.Parity           = UART_CFG_PARITY_EVEN;
  config.LocalLoopback    = TRUE;
  config.TxFIFO           = NULL;
  config.RxFIFO           = NULL;

  //Open each port and force each channel to transmit the
  //data by interrupt to verify the interrupts are working
  // NOTE: Expect UART_OpenPort() to return only DRV_UART_ERR_PORT_NOT_OPEN otherwise the
  //   "result != " logic need to be updated.
  result |= UART_OpenPort(&config);

  config.Duplex = UART_CFG_DUPLEX_FULL;     // all other ports full duplex
  config.Port = (BYTE) UART_1;
  result |= UART_OpenPort(&config);

  config.Port = (BYTE) UART_2;
  result |= UART_OpenPort(&config);

  config.Port = (BYTE) UART_3;
  result |= UART_OpenPort(&config);

  // Perform test only if _OpenPort() is successful

  //Transmit a short string out each port, and receive/verify it
  for(i = 0; i < (UINT32) UART_NUM_OF_UARTS; i++)
  {
    if (!UART_Failed[i])
    {
      UART_Transmit((UINT8)i, TestStr, sizeof(TestStr), NULL);
    }
  }

  // delay 2ms for Tx to occur.
  startTime = TTMR_GetHSTickCount();
  while ( (TTMR_GetHSTickCount() - startTime) < (2*TICKS_PER_mSec));

  //Verify test string is received correctly
  for(i = 0; i < (UINT32) UART_NUM_OF_UARTS; i++)
  {
    if (!UART_Failed[i])
    {
      UARTresult = UART_Receive((UINT8)i, RxBuf, sizeof(TestStr), &BytesReceived);

      if( (STPU( UARTresult, eTpPsc1580) != DRV_OK) ||
          (BytesReceived != sizeof(TestStr)) ||
          (strncmp(TestStr, RxBuf, sizeof(TestStr)) != 0) )
      {
        result = PBITResultCodes[i];
        UART_Failed[i] = TRUE;
      }
    }
  }

#ifdef STE_TP
  // When doing coverage testing ensure the GSE port is valid
  UART_Failed[0] = FALSE;
#endif

  //Close all ports
  for(i = 0; i < (UINT32) UART_NUM_OF_UARTS; i++)
  {
    UART_ClosePort((UINT8)i);
  }

  return result;
}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: UART.c $
 * 
 * *****************  Version 62  *****************
 * User: Melanie Jutras Date: 12-11-06   Time: 3:17p
 * Updated in $/software/control processor/code/drivers
 * SCR #1196 PCLint 641 Added casts for enums where necessary and added
 * warning comment to enum definitions to avoid making enum too large.
 * 
 * *****************  Version 61  *****************
 * User: Melanie Jutras Date: 12-11-05   Time: 1:02p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Removed dead code that was also causing formatting errors to
 * be reported by the code review tool.
 * 
 * *****************  Version 60  *****************
 * User: Melanie Jutras Date: 12-11-02   Time: 1:26p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 File Format Error
 *
 * *****************  Version 59  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 58  *****************
 * User: Jim Mood     Date: 2/09/12    Time: 11:20a
 * Updated in $/software/control processor/code/drivers
 * SCR:1110 Update UART Driver to toggle each duplex and direction control
 * line at startup init. Update SPI ADC clock to 200kHz
 *
 * *****************  Version 57  *****************
 * User: Contractor V&v Date: 5/31/11    Time: 1:50p
 * Updated in $/software/control processor/code/drivers
 * SCR #1038 Misc - Update EMU150 User Table to include "show cfg" Added
 * fix for bug affecting G4E.
 *
 * *****************  Version 56  *****************
 * User: Jeff Vahue   Date: 10/26/10   Time: 9:55p
 * Updated in $/software/control processor/code/drivers
 * SCR #965 - Only do the Tx Control for the GSE as that is the only half
 * duplex port
 *
 * *****************  Version 55  *****************
 * User: Jeff Vahue   Date: 10/26/10   Time: 8:59p
 * Updated in $/software/control processor/code/drivers
 * SCR# 965 - problems with the original mod dumping coverage data.  Based
 * on Jim's note left the Tx enable in Uart Tx in case there is no FIFO,
 * but we have FIFOs.  PBIT test still worked, assume because this is an
 * "internal" wraparound.
 *
 * *****************  Version 54  *****************
 * User: Jim Mood     Date: 10/26/10   Time: 12:29p
 * Updated in $/software/control processor/code/drivers
 * SCR 965 UART Direction Control
 *
 * *****************  Version 53  *****************
 * User: Jeff Vahue   Date: 10/17/10   Time: 2:17p
 * Updated in $/software/control processor/code/drivers
 * SCR# 848 - Code Coverage/WIN32 execution
 *
 * *****************  Version 52  *****************
 * User: Jim Mood     Date: 10/12/10   Time: 2:12p
 * Updated in $/software/control processor/code/drivers
 * SCR 927 Modfix of uart race condition
 *
 * *****************  Version 51  *****************
 * User: Jim Mood     Date: 10/11/10   Time: 5:04p
 * Updated in $/software/control processor/code/drivers
 * SCR 927 UART Half duplex direction control race condition
 *
 * *****************  Version 50  *****************
 * User: Peter Lee    Date: 10/01/10   Time: 5:34p
 * Updated in $/software/control processor/code/drivers
 * SCR #917 Code coverage and code review updates
 *
 * *****************  Version 49  *****************
 * User: Jim Mood     Date: 9/30/10    Time: 5:42p
 * Updated in $/software/control processor/code/drivers
 * SCR 904 Code Coverage changes
 *
 * *****************  Version 48  *****************
 * User: Jeff Vahue   Date: 9/10/10    Time: 4:46p
 * Updated in $/software/control processor/code/drivers
 * SCR# 853 - UART Interrupt Monitor mods
 *
 * *****************  Version 47  *****************
 * User: Jim Mood     Date: 9/09/10    Time: 4:12p
 * Updated in $/software/control processor/code/drivers
 * SCR 863 Fix for YMODEM lockup issue
 *
 * *****************  Version 46  *****************
 * User: Jeff Vahue   Date: 9/03/10    Time: 8:22p
 * Updated in $/software/control processor/code/drivers
 * SCR# 853 - UART Int Monitor
 *
 * *****************  Version 45  *****************
 * User: Jeff Vahue   Date: 9/02/10    Time: 4:34p
 * Updated in $/software/control processor/code/drivers
 * SCR# 847 - Fix Tx Stops error
 *
 * *****************  Version 44  *****************
 * User: Peter Lee    Date: 8/30/10    Time: 10:24a
 * Updated in $/software/control processor/code/drivers
 * SCR #835 UartMgr - PBIT SysCond for UART_PBIT_REG_INIT_FAIL failure
 *
 * *****************  Version 43  *****************
 * User: Jeff Vahue   Date: 8/29/10    Time: 3:52p
 * Updated in $/software/control processor/code/drivers
 * SCR# 830 - Code Coverage Enhancement
 *
 * *****************  Version 42  *****************
 * User: Jim Mood     Date: 7/30/10    Time: 5:11p
 * Updated in $/software/control processor/code/drivers
 * SCR 760 YMODEM half duplex timing issues
 *
 * *****************  Version 41  *****************
 * User: Peter Lee    Date: 7/29/10    Time: 9:41p
 * Updated in $/software/control processor/code/drivers
 * SCR #737 Remove func with dead code cases
 *
 * *****************  Version 40  *****************
 * User: Peter Lee    Date: 7/29/10    Time: 7:53p
 * Updated in $/software/control processor/code/drivers
 * SCR #698 Code Review Updates
 *
 * *****************  Version 39  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:10a
 * Updated in $/software/control processor/code/drivers
 * SCR #698 - Fix code review findings
 *
 * *****************  Version 38  *****************
 * User: Peter Lee    Date: 7/20/10    Time: 7:24p
 * Updated in $/software/control processor/code/drivers
 * SCR #719 Update code to copy Uart Drv BIT cnt to Uart Mgr Status
 *
 * *****************  Version 37  *****************
 * User: Jeff Vahue   Date: 7/20/10    Time: 2:49p
 * Updated in $/software/control processor/code/drivers
 * SCR# 717,718 - Uart Close always (718) not returning the right readByte
 * count when an error exists.
 *
 * *****************  Version 36  *****************
 * User: Jeff Vahue   Date: 7/19/10    Time: 6:34p
 * Updated in $/software/control processor/code/drivers
 * SCR# 707 - Code Coverage TP
 *
 * *****************  Version 35  *****************
 * User: Jeff Vahue   Date: 7/17/10    Time: 5:46p
 * Updated in $/software/control processor/code/drivers
 * SCR# 707 - Add TestPoints for code coverage.
 *
 * *****************  Version 34  *****************
 * User: Contractor2  Date: 7/14/10    Time: 3:05p
 * Updated in $/software/control processor/code/drivers
 * SCR #181 Enhancements - UART Driver
 * If the UART PBIT declares the UART Bad, UART Open should return
 * failure.
 *
 * *****************  Version 33  *****************
 * User: Contractor2  Date: 7/09/10    Time: 4:31p
 * Updated in $/software/control processor/code/drivers
 * SCR #8 Implementation: External Interrupt Monitors
 *
 * *****************  Version 32  *****************
 * User: Jeff Vahue   Date: 6/07/10    Time: 10:46a
 * Updated in $/software/control processor/code/drivers
 * Remove Window Emulation execution restrictions
 *
 * *****************  Version 31  *****************
 * User: Contractor2  Date: 5/28/10    Time: 1:35p
 * Updated in $/software/control processor/code/drivers
 * SCR #592 UART Control Signal State on Port Close
 * Reset port registers to initial values on port close.
 * Code standard fixes including line length, spacing and elimination of
 * magic numbers.
 *
 * *****************  Version 30  *****************
 * User: Contractor2  Date: 4/15/10    Time: 2:19p
 * Updated in $/software/control processor/code/drivers
 * Fixed SCR 55: UART error counters cleared when a port is opened.
 *
 * *****************  Version 29  *****************
 * User: Contractor3  Date: 4/09/10    Time: 12:17p
 * Updated in $/software/control processor/code/drivers
 * SCR #539 Updated for Code Review
 *
 * *****************  Version 28  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:09p
 * Updated in $/software/control processor/code/drivers
 * SCR #317 Implement safe strncpy,SCR #70 Store/Restore interrupt
 *
 * *****************  Version 27  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/drivers
 * SCR #483 - Function Naming
 * SCR# 452 - Code Coverage Improvements
 *
 * *****************  Version 26  *****************
 * User: Jeff Vahue   Date: 3/05/10    Time: 11:30a
 * Updated in $/software/control processor/code/drivers
 * SCR# 477 - Fix Interrupt Enable/disable sequence and some typos
 *
 * *****************  Version 25  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:22p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 24  *****************
 * User: Jeff Vahue   Date: 2/19/10    Time: 4:58p
 * Updated in $/software/control processor/code/drivers
 * SCR# 455 - remove LINT issues
 *
 * *****************  Version 23  *****************
 * User: Jeff Vahue   Date: 2/19/10    Time: 12:57p
 * Updated in $/software/control processor/code/drivers
 * SCR# 455 - remove LINT issues
 *
 * *****************  Version 22  *****************
 * User: Jeff Vahue   Date: 2/09/10    Time: 12:22p
 * Updated in $/software/control processor/code/drivers
 * SCR# 404 - Change if/then to ASSERT in UART_Transmit
 *
 * *****************  Version 21  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:24p
 * Updated in $/software/control processor/code/drivers
 * SCR 371, 377
 *
 * *****************  Version 20  *****************
 * User: Jeff Vahue   Date: 12/18/09   Time: 1:34p
 * Updated in $/software/control processor/code/drivers
 * SCR# 378
 *
 * *****************  Version 19  *****************
 * User: Contractor V&v Date: 11/18/09   Time: 3:47p
 * Updated in $/software/control processor/code/drivers
 * Implementing SCR 172 ASSERT processing for all "default" case
 * processing
 *
 * *****************  Version 18  *****************
 * User: Peter Lee    Date: 10/30/09   Time: 3:46p
 * Updated in $/software/control processor/code/drivers
 * SCR #315 CBIT Reg Check
 *
 * *****************  Version 17  *****************
 * User: Peter Lee    Date: 10/20/09   Time: 6:49p
 * Updated in $/software/control processor/code/drivers
 * Updates for JV to support PC version of program
 *
 * *****************  Version 16  *****************
 * User: Jim Mood     Date: 10/08/09   Time: 6:13p
 * Updated in $/software/control processor/code/drivers
 * SCR #283
 *
 * *****************  Version 15  *****************
 * User: Peter Lee    Date: 10/01/09   Time: 2:12p
 * Updated in $/software/control processor/code/drivers
 * SCR #283 Misc Code Review Updates
 *
 * *****************  Version 14  *****************
 * User: Peter Lee    Date: 9/15/09    Time: 6:01p
 * Updated in $/software/control processor/code/drivers
 * SCR #94, #95 PBIT returns log structure to Init Mgr
 *
 * *****************  Version 13  *****************
 * User: Peter Lee    Date: 9/14/09    Time: 3:59p
 * Updated in $/software/control processor/code/drivers
 * SCR #94 Updated Init for SET_CHECK() and return ERR CODE Struct
 * Manually initialized declared var in UART_PBIT().
 *
 * *****************  Version 12  *****************
 * User: Jim Mood     Date: 8/31/09    Time: 6:29p
 * Updated in $/software/control processor/code/drivers
 * SCR #202 Extra NULL byte added to long output strings
 *
 * *****************  Version 11  *****************
 * User: Jim Mood     Date: 10/29/08   Time: 2:06p
 * Updated in $/control processor/code/drivers
 * Fixed "processor runs at half speed" problem, caused by an unmasked
 * interrupt.  See SCR #22 and #82.  Also addressed SCR #56
 *
 * *****************  Version 10  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 2:02p
 * Updated in $/control processor/code/drivers
 * SCR #87 Function Prototype
 *
 *
 ***************************************************************************/
 
