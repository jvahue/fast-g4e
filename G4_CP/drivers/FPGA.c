#define FPGA_BODY
/******************************************************************************
            Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:         FPGA.c

    Description:  Contains all functions and data related to the FPGA.

 VERSION
     $Revision: 61 $  $Date: 12-11-02 12:02p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "dio.h"
#include "fpga.h"
#include "qar.h"
#include "resultcodes.h"
#include "stddefs.h"
#include "TTMR.h"
#include "mcf548x.h"
#include "arinc429.h"
#include "UtilRegSetCheck.h"

#include "alt_basic.h"
#include "TestPoints.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define TIMER_3SEC    (TICKS_PER_Sec * 3)
#define CFG_TRY_COUNT 3

#ifndef WIN32
   #define PAUSE(x) StartTime = TTMR_GetHSTickCount();         \
           while((TTMR_GetHSTickCount() - StartTime) < (x))
#else
   #define PAUSE
#endif



/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/


/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
FPGA_SHADOW_RAM m_FPGA_ShadowRam;       // Primary Shadow RAM, accessed by
                                        //   applications
FPGA_SHADOW_RAM m_FPGA_ShadowRamCopy;   // Backup Shadow RAM copy used for
                                        //   internal CBIT HW Reg Comparison.
                                        //   Sync automatically to m_FPGA
                                        //   _ShadowRam only when FPGA_Configure()
                                        //   is called.

/*****************************************************************************/
/* Local Constants                                                           */
/*****************************************************************************/
const REG_SETTING FPGA_Registers[] =
{
   // Set up the chip select
   // SET_AND_CHECK ( MCF_FBCS_CSAR3, MCF_FBCS_CSAR_BA(FPGA_BASE_ADDR), bInitOk );
#ifndef WIN32
   {(void *) &MCF_FBCS_CSAR3, MCF_FBCS_CSAR_BA(FPGA_BASE_ADDR), sizeof(UINT32), 0x0,
             REG_SET, TRUE, RFA_FORCE_UPDATE},
#endif

   //SET_AND_CHECK ( MCF_FBCS_CSCR3, (MCF_FBCS_CSCR_PS_16 | MCF_FBCS_CSCR_AA |
   //                                 MCF_FBCS_CSCR_WS(2)), bInitOk );
   {(void *) &MCF_FBCS_CSCR3, (MCF_FBCS_CSCR_PS_16 | MCF_FBCS_CSCR_AA | MCF_FBCS_CSCR_WS(2)),
             sizeof(UINT32), 0x0, REG_SET, TRUE, RFA_FORCE_UPDATE},

   // SET_AND_CHECK ( MCF_FBCS_CSMR3, (MCF_FBCS_CSMR_BAM_128K | MCF_FBCS_CSMR_V),
   //                                 bInitOk );
   {(void *) &MCF_FBCS_CSMR3, (MCF_FBCS_CSMR_BAM_128K | MCF_FBCS_CSMR_V),sizeof(UINT32), 0x0,
             REG_SET, TRUE, RFA_FORCE_UPDATE},

   //SET_CHECK_MASK_OR ( MCF_EPORT_EPPAR, MCF_EPORT_EPPAR_EPPA6(MCF_EPORT_EPPAR_EPPAx_FALLING),
   //                                     bInitOk );
   {(void *) &MCF_EPORT_EPPAR, MCF_EPORT_EPPAR_EPPA6(MCF_EPORT_EPPAR_EPPAx_FALLING),
             sizeof(UINT16), 0x0, REG_SET_MASK_OR, TRUE, RFA_FORCE_UPDATE},

   //SET_CHECK_MASK_AND ( MCF_EPORT_EPDDR, MCF_EPORT_EPDDR_EPDD6, bInitOk );
   {(void *) &MCF_EPORT_EPDDR, MCF_EPORT_EPDDR_EPDD6, sizeof(UINT8), 0x0, REG_SET_MASK_AND,
             TRUE, RFA_FORCE_UPDATE},

   //SET_CHECK_MASK_OR ( MCF_INTC_IMRL, MCF_INTC_IMRL_INT_MASK6, bInitOk );
   {(void *) &MCF_INTC_IMRL, MCF_INTC_IMRL_INT_MASK6, sizeof(UINT32), 0x0, REG_SET_MASK_OR,
             FALSE, RFA_FORCE_NONE},

   //SET_CHECK_MASK_OR ( MCF_EPORT_EPIER, MCF_EPORT_EPIER_EPIE6, bInitOk );
   {(void *) &MCF_EPORT_EPIER, MCF_EPORT_EPIER_EPIE6, sizeof(UINT8), 0x0, REG_SET_MASK_OR,
             FALSE, RFA_FORCE_NONE}

};


const FPGA_REG_CHECK FPGA_REG_CHECK_TBL[] =
{
  { FPGA_IMR1, &m_FPGA_ShadowRam.IMR1, &m_FPGA_ShadowRamCopy.IMR1, 0xFFFF,
               FPGA_ENUM_IMR1},
  { FPGA_IMR2, &m_FPGA_ShadowRam.IMR2, &m_FPGA_ShadowRamCopy.IMR2, 0x8001,
               FPGA_ENUM_IMR2},
  { FPGA_429_COMMON_CONTROL, &m_FPGA_ShadowRam.CCR, &m_FPGA_ShadowRamCopy.CCR, 0x003F,
               FPGA_ENUM_CCR},
  { FPGA_429_LOOPBACK_CONTROL, &m_FPGA_ShadowRam.LBR, &m_FPGA_ShadowRamCopy.LBR, 0x003F,
               FPGA_ENUM_LBR},
  { FPGA_429_RX1_CONFIG, &m_FPGA_ShadowRam.Rx[0].CREG, &m_FPGA_ShadowRamCopy.Rx[0].CREG,
               0x001F, FPGA_ENUM_RX1_CFG},
  { FPGA_429_RX2_CONFIG, &m_FPGA_ShadowRam.Rx[1].CREG, &m_FPGA_ShadowRamCopy.Rx[1].CREG,
               0x001F, FPGA_ENUM_RX2_CFG},
  { FPGA_429_RX3_CONFIG, &m_FPGA_ShadowRam.Rx[2].CREG, &m_FPGA_ShadowRamCopy.Rx[2].CREG,
               0x001F, FPGA_ENUM_RX3_CFG},
  { FPGA_429_RX4_CONFIG, &m_FPGA_ShadowRam.Rx[3].CREG, &m_FPGA_ShadowRamCopy.Rx[3].CREG,
               0x001F, FPGA_ENUM_RX4_CFG},
  { FPGA_QAR_CONTROL, &m_FPGA_ShadowRam.Qar.QCR, &m_FPGA_ShadowRamCopy.Qar.QCR, 0x00FF,
               FPGA_ENUM_QAR_CCR},
  { FPGA_QAR_BARKER1, &m_FPGA_ShadowRam.Qar.QSFCODE1, &m_FPGA_ShadowRamCopy.Qar.QSFCODE1,
               0x0FFF, FPGA_ENUM_QAR_BARKER1},
  { FPGA_QAR_BARKER2, &m_FPGA_ShadowRam.Qar.QSFCODE2, &m_FPGA_ShadowRamCopy.Qar.QSFCODE2,
               0x0FFF, FPGA_ENUM_QAR_BARKER2},
  { FPGA_QAR_BARKER3, &m_FPGA_ShadowRam.Qar.QSFCODE3, &m_FPGA_ShadowRamCopy.Qar.QSFCODE3,
               0x0FFF, FPGA_ENUM_QAR_BARKER3},
  { FPGA_QAR_BARKER4, &m_FPGA_ShadowRam.Qar.QSFCODE4, &m_FPGA_ShadowRamCopy.Qar.QSFCODE4,
               0x0FFF, FPGA_ENUM_QAR_BARKER4},
};


/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static RESULT FPGA_CheckVersion( UINT16 *pdata, UINT16 *psize );
static RESULT FPGA_CheckStartupConfig( void );
static void FPGA_InitializeShadowRam( void );
static void FPGA_InitializeShadowRamVer12( void );

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/*****************************************************************************
 * Function:   FPGA_EP6_ISR  -  EDGE PORT INTERRUPT
 *
 * Description: Handle edge port interrupts from the QAR and ARINC429.
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:
 *  03/10/2008 PLee Updated to latest FPGA Revision 4
 *
 ****************************************************************************/
__interrupt void FPGA_EP6_ISR(void)
{
    // Local Data
    UINT16  intStatus;
    UINT32  entryTime;
    UINT32  intrTime;

    // Monitor the number of interrupts per specified
    // time period to make sure we don't get swamped
    entryTime = TTMR_GetHSTickCount() / TTMR_HS_TICKS_PER_uS;

    // Acknowledge the external interrupt, here to allow additional INT to be
    //  recognized from FPGA during ISR processing.  Expect CPU to
    //  Pend the additional INT.
    MCF_EPORT_EPFR = MCF_EPORT_EPFR_EPF6;

    // Read FPGA ISR Status, here to allow additional INT to be
    //  recognized from FPGA during ISR processing.  Expect CPU to
    //  Pend the additional INT.
    intStatus = FPGA_R(FPGA_ISR);

    if ( m_FPGA_Status.nIntTest == FPGA_TEST_INT_SET )
    {
      m_FPGA_Status.nIntTestFailCnt++; // By inc and then dec removes "else" case
      if ( (intStatus & ISR_INT_TEST) != 0)
      {
         m_FPGA_Status.nIntTestOkCnt++;
         m_FPGA_Status.nIntTestFailCnt--; // By inc and then dec removes "else" case
      }
      m_FPGA_Status.nIntTest = FPGA_TEST_INT_ACK;
    }
    else
    {
       // Service any QAR Int
       if ( (intStatus & ISR_QAR_SF) != 0 )
       {
          QAR_ProcessISR();
       }

       // Service any Arinc 429 Rx Int
       if ((intStatus & ISR_429_RX_INT) != 0)
       {
          Arinc429DrvProcessISR_Rx(intStatus);
       }

       // Service any Arinc 429 Tx Int
       if ((intStatus & ISR_429_TX_INT) != 0)
       {
         Arinc429DrvProcessISR_Tx(intStatus);
       }
    }

    // Check if interrupts coming too fast
    intrTime = entryTime - m_FPGA_Status.LastTime;
    if ( TPU( intrTime, eTpFpga3703) < FPGA_MIN_INTERRUPT_TIME)
    {
      if (m_FPGA_Status.MinIntrTime > intrTime)
      {
        m_FPGA_Status.MinIntrTime = intrTime;     // save minimum interrupt time
      }
      // check interrupt count limit exceeded
      m_FPGA_Status.IntrCnt++;
      if ( m_FPGA_Status.IntrCnt > FPGA_MAX_INTERRUPT_CNT)
      {
        // yes - FPGAManager will log error
        m_FPGA_Status.IntrFail = TRUE;
        // disable interrupts
        MCF_EPORT_EPIER &= ~MCF_EPORT_EPIER_EPIE6;
      }
    }
    else
    {
      m_FPGA_Status.IntrCnt = 0;    // reset fast interrupt counter
    }
    m_FPGA_Status.LastTime = entryTime;

    // Acknowledge the external interrupt
    // MCF_EPORT_EPFR = MCF_EPORT_EPFR_EPF6;
}


/******************************************************************************
 * Function:     FPGA_GetShadowRam
 *
 * Description:  Returns the current FPGA_ShadowRam() settings
 *
 * Parameters:   None
 *
 * Returns:      Returns the current FPGA_ShadowRam() settings
 *
 * Notes:
 *
 *  1) IMPORTANT !!!!
 *     During normal mode operation, after the m_FPGA_ShadowRam elements have
 *     been updated, the calling application should call FPGA_Configure() which
 *     will sync the HW reg and m_FPGA_ShadowRamCopy with the current values
 *     of m_FPGA_ShadowRam.  Otherwise, CBIT would detect a value mismatch for
 *     the 2 out of 3 comparison for FPGA CBIT processing
 *
 *****************************************************************************/
FPGA_SHADOW_RAM_PTR FPGA_GetShadowRam ( void )
{
    return &m_FPGA_ShadowRam;
}


/******************************************************************************
 * Function:     FPGA_Interrupt_Enable
 *
 * Description:  This function allows the enabling and disabling of interrupts
 *               associated with the FPGA.
 *
 * Parameters:   UINT16  IntBit  - Bit position of interrupt
 *               UINT16  IntMask -
 *               BOOLEAN Enable  - Enable or Disable the Interrupt
 *
 * Returns:      DRV_OK
 *
 * Notes:        This function should only be used during initialization
 *
 *****************************************************************************/
/*
RESULT FPGA_Interrupt_Enable (UINT16 IntBit, UINT16 IntMask, BOOLEAN Enable)
{
   if (Enable)
   {
       // Enable interrupt
       FPGA_IMR |= (IntBit * IntMask);
   }
   else
   {
       // Disable the Interrupt
       FPGA_IMR &= ~(IntBit * IntMask);
   }

   // Initialize the Edge Port Interupt for the FPGA to Disabled
   MCF_EPORT_EPPAR &= ~MCF_EPORT_EPPAR_EPPA6(0x3);  //state = low
   MCF_EPORT_EPDDR &= ~MCF_EPORT_EPDDR_EPDD6;       //Input

   // Check if any FPGA interrupts are enabled
   if (FPGA_IMR != 0)
   {
       // Initialize the Edge Port Interrupt for the FPGA
       MCF_EPORT_EPPAR |= MCF_EPORT_EPPAR_EPPA6(MCF_EPORT_EPPAR_EPPAx_FALLING);
       MCF_EPORT_EPIER |= MCF_EPORT_EPIER_EPIE6;        //Interrupt enable

       //Enable interrupts for the FPGA
       MCF_INTC_IMRL &= ~(MCF_INTC_IMRL_INT_MASK6|MCF_INTC_IMRL_MASKALL);
   }

   return DRV_OK;
}
*/

/******************************************************************************
 * Function:    FPGA_Initialize
 *
 * Description: FPGA Initialize sets up the FPGA chip select parameters and
 *              resets the FPGA.
 *
 * Parameters:  SysLogId   - ptr to return SYS_APP_ID
 *              pdata      - ptr to buffer to return result data
 *              psize      - ptr to return size of result data
 *
 * Returns:     DRV_RESULT if no PBIT error returns DRV_OK
 *                         else see ResultCode.h for result codes for FPGA
 *                         PBIT failure.
 *
 * Notes:
 *   - Setup CSAR3 chip select for
 *          base address = 0x40000000
 *   - Setup CSCR3 chip control reg
 *          PS is 16 bits word wide
 *   - Setup CSMR3 memory reg
 *          for 128 KB memory range
 *   - Setup E Port for Interrupt Falling Edge
 *   - Setup IMRL (Interrupt Mask Register)
 *          mask FPGA Int
 *   - Clear internal registers
 *   - Initialize FPGA Shadow RAM to startup default
 *
 *   - Note: FPGARst Initialized to High by DIO.  It is assumed that
 *     a low to high transition is guaranteed to allow FPGA to reset ?
 *
 *   - PBIT: 1) Check FPGA Config Done and Error Line
 *           2) Perform test of FPGA INT test
 *           3) Set and verify default FPGA Shadow RAM setting
 *
 *****************************************************************************/
RESULT FPGA_Initialize( SYS_APP_ID *SysLogId, void *pdata, UINT16 *psize )
{
   // Local Data
   RESULT Status;
   UINT32 StartTime;
   BOOLEAN bInitOk;

   UINT16 *pdest;
   UINT16 i,j;

   // Initialize Local Data
   pdest = (UINT16 *) pdata;
   *psize = 0;
   Status = DRV_OK;

   // Initialize the Edge Port Interrupt for the FPGA to Disabled
   /*
   MCF_EPORT_EPPAR &= ~MCF_EPORT_EPPAR_EPPA6(0x3);  //state = low
   MCF_EPORT_EPDDR &= ~MCF_EPORT_EPDDR_EPDD6;       //Input
   */

   // Reset the FPGA
   /*
   Status = DIO_SetPin(FPGARst, DIO_SetLow);
   Status = DIO_SetPin(FPGARst, DIO_SetHigh);
   Status = FPGA_Reconfigure( TRUE );
   */

   bInitOk = TRUE;
   for (i = 0; i < (sizeof(FPGA_Registers)/sizeof(REG_SETTING)); i++)
   {
     bInitOk &= RegSet( (REG_SETTING_PTR) &FPGA_Registers[i] );
   }

   // Initialize FPGA Variables
   memset ( (void *) &m_FPGA_Status, 0x00, sizeof(FPGA_STATUS));
   m_FPGA_Status.MinIntrTime = (UINT32)-1;    // init min interrupt time
   memset ( (void *) &m_FPGA_ShadowRam, 0x00, sizeof(FPGA_SHADOW_RAM) );
   memset ( (void *) &m_FPGA_ShadowRamCopy, 0x00, sizeof(FPGA_SHADOW_RAM) );

   // Initialize FPGA Shadow RAM
   FPGA_InitializeShadowRam();

   if ( STPU( !bInitOk, eTpFpga3583) )
   {
      Status = DRV_FPGA_MCF_SETUP_FAIL;
      *SysLogId = DRV_ID_FPGA_MCF_SETUP_FAIL;
   }

   if ( Status == DRV_OK )
   {
     // Check if FPGA HW Configuration is OK
     Status = FPGA_CheckStartupConfig();
     // Note: *psize == 0 at this point.   FPGA_CheckStartupConfig() does not break
     //       down failure to either CONFIG_DONE line never asserted or CRC Error
     //       is always asserted.
     *SysLogId = DRV_ID_FPGA_PBIT_NOT_CONFIGURED;
   }

   // Check compatibility of FPGA
   if ( Status == DRV_OK )
   {
     Status = FPGA_CheckVersion( (UINT16 *) pdest, psize);
     *SysLogId = DRV_ID_FPGA_PBIT_BAD_FPGA_VER;

#ifdef SUPPORT_FPGA_BELOW_V12
/*vcast_dont_instrument_start*/
     if ( m_FPGA_Status.Version >= FPGA_VER_12 )
     {
       FPGA_InitializeShadowRamVer12();
     }
/*vcast_dont_instrument_end*/
#endif
   }

   // Init and Test Watchdog Reg
   if ( Status == DRV_OK )
   {
     // Write and Read from WD_TIME val FPGA_WD_TIMER_VAL
     FPGA_W(FPGA_WD_TIMER, FPGA_WD_TIMER_VAL);
     // For some reason the first write of 0xAAAA gets cleared to 0x0000
     //    Need second write of 0xAAAA.  Need HW explanation.
     FPGA_W(FPGA_WD_TIMER, FPGA_WD_TIMER_VAL);

     // TTMR_Delay(TICKS_PER_mSec);
     bInitOk = (FPGA_R(FPGA_WD_TIMER) == FPGA_WD_TIMER_VAL) ? TRUE : FALSE;

     if ( bInitOk  )
     {
       // Write to WD_TIME_INV val FPGA_WD_TIMER_INV_VAL
       FPGA_W(FPGA_WD_TIMER_INV, FPGA_WD_TIMER_INV_VAL);
       // TTMR_Delay(TICKS_PER_mSec);

       // Read to ensure WD_TIME and WD_TIME_INV cleared by FPGA
       i = FPGA_R(FPGA_WD_TIMER);
       j = FPGA_R(FPGA_WD_TIMER_INV);
#ifndef WIN32
       bInitOk = ((i == 0) && (j == 0)) ? TRUE : FALSE;
#endif
     }

     if (TRUE != STPU(bInitOk, eTpFpgaScr891))    // != TRUE return fail
     {
        Status = DRV_FPGA_WD_FAIL;
        *SysLogId = DRV_ID_FPGA_PBIT_WD_FAIL;
     }
   }

   if ( Status == DRV_OK )
   {
     // Configure FPGA with Start Configuration
     Status = FPGA_Configure();
     // Note: *psize == 0 at this point.
     *SysLogId = DRV_ID_FPGA_PBIT_CONFIGURED_FAIL;
   }

   if ( Status == DRV_OK )
   {
     // Perform INT test !
      // Set flag to indicate INT Test
      // Set edge port Int for falling edge
      // Ping IMR2 to activate INT Test
      // Wait for flag to be cleared.
      m_FPGA_Status.nIntTest = FPGA_TEST_INT_SET;
      m_FPGA_Status.nIntTestOkCnt = 0x00;
      m_FPGA_Status.nIntTestFailCnt = 0x00;

      StartTime = TTMR_GetHSTickCount();

      // Initialize the Edge Port Interrupt for the FPGA
      // MCF_EPORT_EPPAR |= MCF_EPORT_EPPAR_EPPA6(MCF_EPORT_EPPAR_EPPAx_FALLING);
      MCF_EPORT_EPIER |= MCF_EPORT_EPIER_EPIE6;        //Interrupt enable

      //Unmask FPGA INT Level 6
      MCF_INTC_IMRL &= ~(MCF_INTC_IMRL_INT_MASK6|MCF_INTC_IMRL_MASKALL);

      FPGA_W(FPGA_IMR2, FPGA_R(FPGA_IMR2) | IMR2_INT_TEST);

      while ( ( m_FPGA_Status.nIntTestOkCnt < CFG_TRY_COUNT ) &&
              ( (TTMR_GetHSTickCount() - StartTime) < TICKS_PER_500mSec ) )
      {
        // Wait until IRQ occurs
        // Must see INT and ISR indicates Int Test before TestOkCnt is inc !
        //   See FPGA ISR
        if ( m_FPGA_Status.nIntTest == FPGA_TEST_INT_ACK )
        {
           m_FPGA_Status.nIntTest = FPGA_TEST_INT_SET;
           FPGA_W(FPGA_IMR2, FPGA_R(FPGA_IMR2) | IMR2_INT_TEST);
        }
      }

      MCF_INTC_IMRL |= MCF_INTC_IMRL_INT_MASK6;        //Mask interrupts for the FPGA

#ifndef WIN32
      if ( m_FPGA_Status.nIntTestOkCnt < STPU( CFG_TRY_COUNT, eTpFpga3001))
      {
        Status |= DRV_FPGA_INT_FAIL;
        // Note *psize == 0 at this point.
        *SysLogId = DRV_ID_FPGA_PBIT_INT_FAIL;
      }
#endif

   } // End of if ( Status == DRV_OK )

   m_FPGA_Status.nIntTest = FPGA_TEST_NONE;
   m_FPGA_Status.nIntTestOkCnt = 0x00;
   m_FPGA_Status.nIntTestFailCnt = 0x00;

   // Initialize the Edge Port Interrupt for the FPGA to Disabled
   // MCF_EPORT_EPPAR &= ~MCF_EPORT_EPPAR_EPPA6(0x3);  //state = low
   // MCF_EPORT_EPDDR &= ~MCF_EPORT_EPDDR_EPDD6;       //Input

   m_FPGA_Status.InitResult = Status;

   // If FPGA Init Fails then set FPGA STATUS to FAULTED
   if ( m_FPGA_Status.InitResult != DRV_OK )
   {
     m_FPGA_Status.SystemStatus = FPGA_STATUS_FAULTED;
   }
   // Note: m_FPGA_Status init to 0 where .SystemStatus == 0 == FPGA_STATUS_OK

   return (Status);
}


/******************************************************************************
 * Function:    FPGA_Configure
 *
 * Description: Configures the FPGA HW from the FPGA Shadow RAM
 *
 * Parameters:  None
 *
 * Returns:     DRV_OK if no error returned
 *              DRV_FPGA_CONFIGURED_FAIL if register cfg fails
 *
 * Notes:       None
 *
 *****************************************************************************/
RESULT FPGA_Configure (void)
{
   RESULT Status;
   UINT16 *FPGA_429_RX_FILTER_PTR;
   FPGA_SHADOW_ARINC429_RX_CFG_PTR pRxCfg;
   UINT16 i;
   UINT32 StartTime;
   BOOLEAN bInitOk;

   Status = DRV_OK;
   bInitOk = TRUE;

/*
   // Initialize the Edge Port Interrupt for the FPGA to Disabled
   MCF_EPORT_EPPAR &= ~MCF_EPORT_EPPAR_EPPA6(0x3);  //state = low
   MCF_EPORT_EPDDR &= ~MCF_EPORT_EPDDR_EPDD6;       //Input
*/
   // CPU CS and INT set in FPGA_Initialize()
   // When reconfiguring FPGA only need to MASK Interrupt !
     MCF_INTC_IMRL |= MCF_INTC_IMRL_INT_MASK6;        //Mask interrupts for the FPGA

   // Configure the FPGA from FPGA Shadow RAM
     // Configure IMR1
     SET_AND_CHECK ( *FPGA_IMR1, m_FPGA_ShadowRam.IMR1, bInitOk );

     // Configure IMR2
     SET_AND_CHECK ( *FPGA_IMR2, m_FPGA_ShadowRam.IMR2, bInitOk );

     // Configure CCR
     SET_AND_CHECK (*FPGA_429_COMMON_CONTROL, m_FPGA_ShadowRam.CCR, bInitOk );

     // Configure LBR
     SET_AND_CHECK ( *FPGA_429_LOOPBACK_CONTROL, m_FPGA_ShadowRam.LBR, bInitOk );

     // Configure GPIO
     SET_AND_CHECK ( *FPGA_GPIO_0, m_FPGA_ShadowRam.GPIO, bInitOk );

     // Configure Rx 1 MUST CONFIGURE LABEL FILTER REG TABLE BEFORE CREG TO
     //  ENABLE RX !!
     // Must Disable Rx Enb before accessing LFR
     *FPGA_429_RX1_CONFIG = (*FPGA_429_RX1_CONFIG & ~RCR_429_ENABLE) |
                            (CR_429_DISABLE_VAL * RCR_429_ENABLE);
     pRxCfg = &m_FPGA_ShadowRam.Rx[0];
     FPGA_429_RX_FILTER_PTR = (UINT16 *) FPGA_429_RX1_FILTER;
     for (i=0;i<FPGA_429_FILTER_SIZE;i++)
     {
        SET_AND_CHECK ( *(FPGA_429_RX_FILTER_PTR + i), pRxCfg->LFR[i], bInitOk );
     }
     SET_AND_CHECK ( *FPGA_429_RX1_CONFIG, m_FPGA_ShadowRam.Rx[0].CREG, bInitOk );

     // Configure Rx 2 MUST CONFIGURE LABEL FILTER REG TABLE BEFORE CREG TO
     //  ENABLE RX !!
     // Must Disable Rx Enb before accessing LFR
     *FPGA_429_RX2_CONFIG = (*FPGA_429_RX2_CONFIG & ~RCR_429_ENABLE) |
                           (CR_429_DISABLE_VAL * RCR_429_ENABLE);
     pRxCfg = &m_FPGA_ShadowRam.Rx[1];
     FPGA_429_RX_FILTER_PTR = (UINT16 *) FPGA_429_RX2_FILTER;
     for (i=0;i<FPGA_429_FILTER_SIZE;i++)
     {
        SET_AND_CHECK ( *(FPGA_429_RX_FILTER_PTR + i), pRxCfg->LFR[i], bInitOk );
     }
     SET_AND_CHECK ( *FPGA_429_RX2_CONFIG, m_FPGA_ShadowRam.Rx[1].CREG, bInitOk );

     // Configure Rx 3 MUST CONFIGURE LABEL FILTER REG TABLE BEFORE CREG TO
     //  ENABLE RX !!
     // Must Disable Rx Enb before accessing LFR
     *FPGA_429_RX3_CONFIG = (*FPGA_429_RX3_CONFIG & ~RCR_429_ENABLE) |
                           (CR_429_DISABLE_VAL * RCR_429_ENABLE);
     pRxCfg = &m_FPGA_ShadowRam.Rx[2];
     FPGA_429_RX_FILTER_PTR = (UINT16 *) FPGA_429_RX3_FILTER;
     for (i=0;i<FPGA_429_FILTER_SIZE;i++) {
        SET_AND_CHECK ( *(FPGA_429_RX_FILTER_PTR + i), pRxCfg->LFR[i], bInitOk );
     }
     SET_AND_CHECK ( *FPGA_429_RX3_CONFIG, m_FPGA_ShadowRam.Rx[2].CREG, bInitOk );

     // Configure Rx 4 MUST CONFIGURE LABEL FILTER REG TABLE BEFORE CREG TO
     //  ENABLE RX !!
     // Must Disable Rx Enb before accessing LFR
     *FPGA_429_RX4_CONFIG = (*FPGA_429_RX4_CONFIG & ~RCR_429_ENABLE) |
                           (CR_429_DISABLE_VAL * RCR_429_ENABLE);
     pRxCfg = &m_FPGA_ShadowRam.Rx[3];
     FPGA_429_RX_FILTER_PTR = (UINT16 *) FPGA_429_RX4_FILTER;
     for (i=0;i<FPGA_429_FILTER_SIZE;i++)
     {
        SET_AND_CHECK (*(FPGA_429_RX_FILTER_PTR + i), pRxCfg->LFR[i], bInitOk );
     }
     SET_AND_CHECK ( *FPGA_429_RX4_CONFIG, m_FPGA_ShadowRam.Rx[3].CREG, bInitOk );

     // Configure Tx 1
     SET_AND_CHECK ( *FPGA_429_TX1_CONFIG, m_FPGA_ShadowRam.Tx[0].CREG, bInitOk );

     // Configure Tx 2
     SET_AND_CHECK ( *FPGA_429_TX2_CONFIG, m_FPGA_ShadowRam.Tx[1].CREG, bInitOk );

   // Set QAR specific registers
     // Set QCR
     SET_AND_CHECK ( *FPGA_QAR_CONTROL, m_FPGA_ShadowRam.Qar.QCR, bInitOk );

     // Set QSFCODE1, Barker Code 1
     SET_AND_CHECK ( *FPGA_QAR_BARKER1, m_FPGA_ShadowRam.Qar.QSFCODE1, bInitOk );

     // Set QSFCODE2, Barker Code 2
     SET_AND_CHECK ( *FPGA_QAR_BARKER2, m_FPGA_ShadowRam.Qar.QSFCODE2, bInitOk );

     // Set QSFCODE3, Barker Code 3
     SET_AND_CHECK ( *FPGA_QAR_BARKER3, m_FPGA_ShadowRam.Qar.QSFCODE3, bInitOk );

     // Set QSFCODE4, Barker Code 4
     SET_AND_CHECK ( *FPGA_QAR_BARKER4, m_FPGA_ShadowRam.Qar.QSFCODE4, bInitOk );

   // Set return status
   if (bInitOk != STPU( TRUE, eTpFpgaRegAll))
   {
      Status |= DRV_FPGA_CONFIGURED_FAIL;
   }

   // Determine if any interrupt is set
   if ( ( (*FPGA_IMR1 ^ 0xFFFF) != 0x0000) ||
        ( (*FPGA_IMR2 & IMR2_QAR_SF) != 0x0000) )
   {
      // Initialize the Edge Port Interrupt for the FPGA
      // MCF_EPORT_EPPAR |= MCF_EPORT_EPPAR_EPPA6(MCF_EPORT_EPPAR_EPPAx_FALLING);
      // MCF_EPORT_EPIER |= MCF_EPORT_EPIER_EPIE6;        //Interrupt enable

      // Enable interrupts for the FPGA.  NOTE: CPU Port and Int init in
      //  FPGA_Initialize()
      MCF_INTC_IMRL &= ~(MCF_INTC_IMRL_INT_MASK6|MCF_INTC_IMRL_MASKALL);
   }

   // Force QAR to re-sync before exiting
   *FPGA_QAR_CONTROL = (*FPGA_QAR_CONTROL & ~QCR_RESYNC) |
                       (QCR_RESYNC * QCR_RESYNC_REFRAME_VAL);

   // Delay 1 usec
   StartTime = TTMR_GetHSTickCount();
   PAUSE(TICKS_PER_uSec);

   *FPGA_QAR_CONTROL = (*FPGA_QAR_CONTROL & ~QCR_RESYNC) |
                       (QCR_RESYNC * QCR_RESYNC_NORMAL_VAL);

   // Read Status to clear
   i = *FPGA_429_RX1_STATUS;
   i = *FPGA_429_RX1_FIFO_STATUS;
   i = *FPGA_429_RX2_STATUS;
   i = *FPGA_429_RX2_FIFO_STATUS;
   i = *FPGA_429_RX3_STATUS;
   i = *FPGA_429_RX3_FIFO_STATUS;
   i = *FPGA_429_RX4_STATUS;
   i = *FPGA_429_RX4_FIFO_STATUS;
   i = *FPGA_ISR;

   // Update Shadow RAM Copy used for FPGA CBIT
   memcpy ( &m_FPGA_ShadowRamCopy, &m_FPGA_ShadowRam, sizeof(FPGA_SHADOW_RAM) );

   return (Status);
}


/******************************************************************************
 * Function:     FPGA_Monitor_Ok
 *
 * Description:  FPGA Monitor Ok monitors the Error Detected pin of the FPGA
 *
 * Parameters:   None
 *
 * Returns:      TRUE if FPGA OK
 *               FALSE if FPGA NOT OK
 *
 * Notes:
 *   1) As of 04/39/2009 SCR 126 still open regarding proper HW FPGA CRC control
 *      with earlier hw, FPGA CRC always indicated fault.  With later hw FPGA
 *      CRC indicate no fault.  HW still needs to verify that this line works
 *      correctly.
 *
 *****************************************************************************/
BOOLEAN FPGA_Monitor_Ok ( void )
{
  BOOLEAN Status = DIO_ReadPin( FPGA_Err);
  return ( !(Status == FPGA_ERR_TRUE) );
}


/******************************************************************************
 * Function:    FPGA_ForceReload
 *
 * Description: The FPGA Force Reconfigure function will pulse the FPGA
 *              reconfigure line.
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:       FPGA Reconfiguration can take up to 200 mSec to complete.
 *              The FPGA Config Status should be monitored for completion.
 *   12/05/2008 Wait could take between 50 msec to 2.2 sec according to calc
 *              by Adam M. (the fpga guy)
 *
 *****************************************************************************/
void FPGA_ForceReload ( void )
{

  // Set the status to _FORCERELOAD
  m_FPGA_Status.SystemStatus = FPGA_STATUS_FORCERELOAD;

  // Toggle the FPGA Reconfigure Line
  DIO_SetPin(FPGAReconfig, DIO_SetLow);
  TTMR_Delay(25 * TICKS_PER_mSec);
  DIO_SetPin(FPGAReconfig, DIO_SetHigh);

}


/******************************************************************************
 * Function:    FPGA_ForceReload_Status
 *
 * Description: Read the status of the FPGA Configuration Done pin.
 *
 * Parameters:  None
 *
 * Returns:     TRUE if Reload Completed
 *              FALSE if Reload Not Completed
 *
 * Notes:       None.
 *
 *****************************************************************************/
BOOLEAN FPGA_ForceReload_Status ( void )
{
  // Monitor the config done
  BOOLEAN Status = DIO_ReadPin(FPGA_Cfg);
  return ( !( TPU( Status, eTpFpga2997a) == FPGA_CONFIG_NOT_DONE) );
}


/******************************************************************************
 * Function:    FPGA_GetStatus
 *
 * Description: Returns ptr to m_FPGA_Status to access current param values
 *
 * Parameters:  none
 *
 * Returns:     Returns ptr to m_FPGA_Status
 *
 * Notes:       none
 *
 *****************************************************************************/
FPGA_STATUS_PTR FPGA_GetStatus ( void )
{
   return ( (FPGA_STATUS_PTR) &m_FPGA_Status );
}

/******************************************************************************
 * Function:    FPGA_GetRegCheckTbl
 *
 * Description: Returns pointer to FPGA_REG_CHECK_TBL[]
 *
 * Parameters:  nCnt - Pointer to return table size
 *
 * Returns:     Pointer to FPGA_REG_CHECK_TBL[]
 *
 * Notes:       None
 *
 *****************************************************************************/
FPGA_REG_CHECK_PTR FPGA_GetRegCheckTbl ( UINT16 *nCnt )
{

  *nCnt = sizeof(FPGA_REG_CHECK_TBL) / sizeof(FPGA_REG_CHECK);

  return ( (FPGA_REG_CHECK_PTR) &FPGA_REG_CHECK_TBL[0]);

}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:    FPGA_CheckVersion
 *
 * Description: Checks the FPGA version to ensure compatibility
 *
 * Parameters:  pdata - ptr to data structure to return HW FPGA Version
 *              psize - ptr to size where size of HW FPGA Ver is returned
 *
 * Returns:     RESULT to DRV_FPGA_BAD_VERSION if FPGA version "potentially"
 *                        not compatible.
 *                     to DRV_OK if FPGA Configured with no Error
 *
 * Notes:
 *  1) Version check logic based on FPGA Major Version Number
 *  2) 01/06/2008 PLEE Current FPGA does not set Version Info Correctly. Fake
 *     to return DRV_OK
 *  3) FPGA Version Toggles to Invert Bits on each read.  Must accomodate this
 *     FPGA HW feature
 *
 *****************************************************************************/
static
RESULT FPGA_CheckVersion( UINT16 *pdata, UINT16 *psize )
{
  RESULT Status;
  UINT16 Ver;
  UINT16 VerInv;

  Status = DRV_OK;

  Ver = (UINT16)STPU( FPGA_R( FPGA_VER), eTpFpga3309);
  VerInv = FPGA_R( FPGA_VER);

  m_FPGA_Status.Version = Ver;

  // SCR #383, if first read is the inverted value, the use the un-invert version.
  //   NOTE: Limits FPGA versions to 32 K ! Acceptable limitation.
  if ( m_FPGA_Status.Version > 0x8000 )
  {
    m_FPGA_Status.Version = VerInv;
  }

  // Check for proper invert and compatible version
  // Note: Requires the cast to {UINT16} as this compiler uses long word reg for
  //       intermediate "~" operation and then performs a CMP.L
#ifdef SUPPORT_FPGA_BELOW_V12
/*vcast_dont_instrument_start*/
  if (  (Ver == (UINT16)~VerInv)
                 &&
        ((( Ver <= FPGA_MAJOR_VERSION_COMPATIBILITY ) &&
          ( VerInv >= (UINT16) ~(FPGA_MAJOR_VERSION_COMPATIBILITY)) )
                               ||
        (( VerInv <= FPGA_MAJOR_VERSION_COMPATIBILITY ) &&
         ( Ver >= (UINT16) ~(FPGA_MAJOR_VERSION_COMPATIBILITY))))
     )
/*vcast_dont_instrument_end*/
#else
  if (  (Ver == (UINT16)~VerInv)
                 &&
        (m_FPGA_Status.Version == FPGA_VER_12)
     )
#endif
  {
    Status = DRV_OK;
  }
  else
  {
    Status = DRV_FPGA_BAD_VERSION;
    *pdata = Ver;
    *(pdata + 2) = VerInv;
    *psize = sizeof(UINT16) + sizeof(UINT16);
  }

  return (Status);

}

/******************************************************************************
 * Function:    FPGA_CheckStartupConfig
 *
 * Description: Checks the startup state of the FPGA configure and Error line.
 *              Attempts to Force Reconfigure if Conf not Done, Error or TimeOut
 *
 * Parameters:  None
 *
 * Returns:     RESULT to DRV_FPGA_NOT_CONFIGURED if FPGA not configured
 *                     to DRV_OK if FPGA Configured with no Error
 *
 * Notes:
 *  1) FPGA_CheckStartupConfig() does not break down failure to either
 *     CONFIG_DONE line never asserted or CRC Error is always asserted.
 *
 *****************************************************************************/
static
RESULT FPGA_CheckStartupConfig( void )
{
  UINT32 i;
  RESULT Status;
  UINT32 StartTime;
  UINT32 elapsed;
  UINT32 timeLimit = TIMER_3SEC;

  // Wait for FPGA Configure to complete or if Error indications, force a reconfigure
  //    and wait (up to 3 tries).
  Status = DRV_FPGA_NOT_CONFIGURED;
  i = 0;
  while ( (i < CFG_TRY_COUNT) && (Status != DRV_OK) )
  {
    StartTime = TTMR_GetHSTickCount();
    // Loop until Config is DONE, Error indicated or Time Out
    while ( ((TTMR_GetHSTickCount() - StartTime) < timeLimit) &&
            ((FPGA_ForceReload_Status() == FALSE) ||
             (FPGA_Monitor_Ok() == FALSE)) );

    elapsed = STPU( (TTMR_GetHSTickCount() - StartTime), eTpFpga3580);
    if ( (FPGA_Monitor_Ok() == FALSE) || (elapsed >= timeLimit) )
    {
       i++; // Make another attempt at re-configuring
       FPGA_ForceReload();
    }
    else
    {
       Status = DRV_OK;
    }
  }

  return (Status);

}

/******************************************************************************
 * Function:    FPGA_InitializeShadowRam
 *
 * Description: Initializes the FPGA ShadowRam to startup default
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static
void FPGA_InitializeShadowRam( void )
{
  UINT32 i;
  UINT32 j;

#ifndef SUPPORT_FPGA_BELOW_V12

  // Disable Interrupts for Ver12
  FPGA_InitializeShadowRamVer12();

#else

   /*vcast_dont_instrument_start*/

   // Disable Interrupts
   m_FPGA_ShadowRam.IMR1 = (IMR1_MASK_IRQ_VAL * IMR1_429_RX1_FULL)      |
                           (IMR1_MASK_IRQ_VAL * IMR1_429_RX1_HALF_FULL) |
                           (IMR1_MASK_IRQ_VAL * IMR1_429_RX1_NOT_EMPTY) |
                           (IMR1_MASK_IRQ_VAL * IMR1_429_RX2_FULL)      |
                           (IMR1_MASK_IRQ_VAL * IMR1_429_RX2_HALF_FULL) |
                           (IMR1_MASK_IRQ_VAL * IMR1_429_RX2_NOT_EMPTY) |
                           (IMR1_MASK_IRQ_VAL * IMR1_429_RX3_FULL)      |
                           (IMR1_MASK_IRQ_VAL * IMR1_429_RX3_HALF_FULL) |
                           (IMR1_MASK_IRQ_VAL * IMR1_429_RX3_NOT_EMPTY) |
                           (IMR1_MASK_IRQ_VAL * IMR1_429_RX4_FULL)      |
                           (IMR1_MASK_IRQ_VAL * IMR1_429_RX4_HALF_FULL) |
                           (IMR1_MASK_IRQ_VAL * IMR1_429_RX4_NOT_EMPTY) |
                           (IMR1_MASK_IRQ_VAL * IMR1_429_TX1_HALF_FULL_v9) |
                           (IMR1_MASK_IRQ_VAL * IMR1_429_TX1_EMTPY_v9)     |
                           (IMR1_MASK_IRQ_VAL * IMR1_429_TX2_HALF_FULL_v9) |
                           (IMR1_MASK_IRQ_VAL * IMR1_429_TX2_EMTPY_v9);

   m_FPGA_ShadowRam.IMR2 = (IMR2_DSB_IRQ_VAL * IMR2_NOT_USED_v9) |
                           (IMR2_DSB_IRQ_VAL * IMR2_QAR_SF);
   /*vcast_dont_instrument_end*/

#endif

   // Set CCR to Power On Default
   m_FPGA_ShadowRam.CCR = (CCR_429_HIGH_SPEED_VAL * CCR_429_TX2_RATE) |
                          (CCR_429_HIGH_SPEED_VAL * CCR_429_TX1_RATE) |
                          (CCR_429_HIGH_SPEED_VAL * CCR_429_RX4_RATE) |
                          (CCR_429_HIGH_SPEED_VAL * CCR_429_RX3_RATE) |
                          (CCR_429_HIGH_SPEED_VAL * CCR_429_RX2_RATE) |
                          (CCR_429_HIGH_SPEED_VAL * CCR_429_RX1_RATE);


   // Set LBR to Power On Default
   m_FPGA_ShadowRam.LBR = (LBR_429_TX_NORMAL_VAL * LBR_429_TX1_DRIVE) |
                          (LBR_429_TX_NORMAL_VAL * LBR_429_TX2_DRIVE) |
                          (LBR_429_RX_NORMAL_VAL * LBR_429_RX1_SOURCE) |
                          (LBR_429_RX_NORMAL_VAL * LBR_429_RX2_SOURCE) |
                          (LBR_429_RX_NORMAL_VAL * LBR_429_RX3_SOURCE) |
                          (LBR_429_RX_NORMAL_VAL * LBR_429_RX4_SOURCE);

   // Set
   // Reference open SCR #601 for additional
   m_FPGA_ShadowRam.GPIO = (FPGA_GPIO_0_232_485 * FPGA_GPIO_0_232_VAL)       |
                           (FPGA_GPIO_0_CAN0_WAKE * FPGA_GPIO_0_CAN_OFF_VAL) |
                           (FPGA_GPIO_0_CAN1_WAKE * FPGA_GPIO_0_CAN_OFF_VAL);

   // Set Rx Configuration Register
   for (i = 0; i < FPGA_MAX_RX_CHAN; i++)
   {
      m_FPGA_ShadowRam.Rx[i].CREG=
          (RCR_429_UNSET_CLR_VAL * RCR_429_PARITY_ERROR_CLR) |
          (RCR_429_BITS_SWAP_VAL * RCR_429_LABEL_SWAP)       |
          (RCR_429_UNSET_CLR_VAL * RCR_429_FRAME_ERROR_CLR)  |
          (CR_429_DISABLE_VAL * RCR_429_ENABLE)              |
          (CR_429_ODD_PARITY_VAL * RCR_429_PARITY_SELECT);

      // Initialize Label Filter Reg to "Accept" All Arinc429 Data
      for (j = 0; j < FPGA_429_FILTER_SIZE; j++)
      {
         m_FPGA_ShadowRam.Rx[i].LFR[j] = 0x000F;
      }
   }

   // Set Tx Configuration Register
   for (i = 0; i < FPGA_MAX_TX_CHAN; i++)
   {
      m_FPGA_ShadowRam.Tx[i].CREG =
          (CR_429_DISABLE_VAL * TCR_429_ENABLE)  |
          (CR_429_ODD_PARITY_VAL * TCR_429_PARITY_SELECT);
   }

   // Set QAR to Barker Code and 64 Words
   m_FPGA_ShadowRam.Qar.QSFCODE1 = 0x247;
   m_FPGA_ShadowRam.Qar.QSFCODE2 = 0x5b8;
   m_FPGA_ShadowRam.Qar.QSFCODE3 = 0xa47;
   m_FPGA_ShadowRam.Qar.QSFCODE4 = 0xdb8;

   m_FPGA_ShadowRam.Qar.QCR = (QCR_TSTP * QCR_TEST_DISABLED_VAL)        |
                              (QCR_TSTN * QCR_TEST_DISABLED_VAL)        |
                              (QCR_ENABLE * QCR_TEST_DISABLED_VAL)      |
                              (QCR_FORMAT * QCR_FORMAT_BIP_VAL)         |
                              (QCR_BYTES_SF * QCR_BYTES_SF_64WORDS_VAL) |
                              (QCR_RESYNC * QCR_RESYNC_NORMAL_VAL);
}


/******************************************************************************
 * Function:    FPGA_InitializeShadowRamVer12
 *
 * Description: Initializes the version 12 FPGA ShadowRam to startup default
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static
void FPGA_InitializeShadowRamVer12( void )
{
   // Disable Interrupts
   m_FPGA_ShadowRam.IMR1 = (IMR1_MASK_IRQ_VAL * IMR1_429_RX1_FULL)      |
                           (IMR1_MASK_IRQ_VAL * IMR1_429_RX1_HALF_FULL) |
                           (IMR1_MASK_IRQ_VAL * IMR1_429_RX1_NOT_EMPTY) |
                           (IMR1_MASK_IRQ_VAL * IMR1_429_RX2_FULL)      |
                           (IMR1_MASK_IRQ_VAL * IMR1_429_RX2_HALF_FULL) |
                           (IMR1_MASK_IRQ_VAL * IMR1_429_RX2_NOT_EMPTY) |
                           (IMR1_MASK_IRQ_VAL * IMR1_429_RX3_FULL)      |
                           (IMR1_MASK_IRQ_VAL * IMR1_429_RX3_HALF_FULL) |
                           (IMR1_MASK_IRQ_VAL * IMR1_429_RX3_NOT_EMPTY) |
                           (IMR1_MASK_IRQ_VAL * IMR1_429_RX4_FULL)      |
                           (IMR1_MASK_IRQ_VAL * IMR1_429_RX4_HALF_FULL) |
                           (IMR1_MASK_IRQ_VAL * IMR1_429_RX4_NOT_EMPTY) |
                           (IMR1_MASK_IRQ_VAL * IMR1_429_TX1_FULL_v12)      |
                           (IMR1_MASK_IRQ_VAL * IMR1_429_TX1_HALF_FULL_v12) |
                           (IMR1_MASK_IRQ_VAL * IMR1_429_TX1_EMPTY_v12);

   m_FPGA_ShadowRam.IMR2 = (IMR2_DSB_IRQ_VAL * IMR2_NOT_USED_v12)          |
                           (IMR2_DSB_IRQ_VAL * IMR2_429_TX2_FULL_v12)      |
                           (IMR2_DSB_IRQ_VAL * IMR2_429_TX2_HALF_FULL_v12) |
                           (IMR2_DSB_IRQ_VAL * IMR2_429_TX2_EMPTY_v12)     |
                           (IMR2_DSB_IRQ_VAL * IMR2_QAR_SF);
}


/*************************************************************************
 *  MODIFICATIONS
 *    $History: FPGA.c $
 * 
 * *****************  Version 61  *****************
 * User: Melanie Jutras Date: 12-11-02   Time: 12:02p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 File Format Error
 *
 * *****************  Version 60  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 59  *****************
 * User: Jeff Vahue   Date: 10/27/11   Time: 12:48p
 * Updated in $/software/control processor/code/drivers
 * SCR# 1077 Code Coverage - cleaner and no VCAST compile issues
 *
 * *****************  Version 58  *****************
 * User: Jeff Vahue   Date: 10/27/11   Time: 12:34p
 * Updated in $/software/control processor/code/drivers
 * SCR# 1077 - Code Coverage Update to exclude Windows only code
 *
 * *****************  Version 57  *****************
 * User: Jeff Vahue   Date: 10/17/11   Time: 7:46p
 * Updated in $/software/control processor/code/drivers
 * SCR# 1063: Add TP
 *
 * *****************  Version 56  *****************
 * User: Jeff Vahue   Date: 10/05/11   Time: 3:55p
 * Updated in $/software/control processor/code/drivers
 * SCR# 1085 - G4 Emulation modification - no operational impact
 *
 * *****************  Version 55  *****************
 * User: Contractor2  Date: 5/10/11    Time: 2:44p
 * Updated in $/software/control processor/code/drivers
 * SCR #891 Enhancement: Incomplete FPGA initialization functionality
 *
 * *****************  Version 54  *****************
 * User: Jeff Vahue   Date: 10/18/10   Time: 7:52p
 * Updated in $/software/control processor/code/drivers
 * SCR# 848 - Code Coverage, VectorCast issue when a funcitons first
 * statement is not in the build.
 *
 * *****************  Version 53  *****************
 * User: John Omalley Date: 10/14/10   Time: 1:51p
 * Updated in $/software/control processor/code/drivers
 * SCR 879 - Added delays for FPGA CRC Error Processing
 *
 * *****************  Version 52  *****************
 * User: Peter Lee    Date: 9/30/10    Time: 11:26a
 * Updated in $/software/control processor/code/drivers
 * SCR #911 Misc Code Coverage Updates
 *
 * *****************  Version 51  *****************
 * User: Contractor2  Date: 9/27/10    Time: 2:55p
 * Updated in $/software/control processor/code/drivers
 * SCR #795 Code Review Updates
 *
 * *****************  Version 50  *****************
 * User: Contractor2  Date: 9/24/10    Time: 3:02p
 * Updated in $/software/control processor/code/drivers
 * SCR #882 WD - Old Code for pulsing the WD should be removed
 *
 * *****************  Version 49  *****************
 * User: Peter Lee    Date: 9/03/10    Time: 2:07p
 * Updated in $/software/control processor/code/drivers
 * SCR #837 Misc - FPGA Reconfigure Command must be assert for min of 500
 * nsec
 *
 * *****************  Version 48  *****************
 * User: Jeff Vahue   Date: 9/02/10    Time: 4:36p
 * Updated in $/software/control processor/code/drivers
 * SCR# 707 - Code Coverage
 *
 * *****************  Version 47  *****************
 * User: Peter Lee    Date: 8/19/10    Time: 4:48p
 * Updated in $/software/control processor/code/drivers
 * SCR #804 Shutdown DT Task over run for Arinc429 and QAR.
 *
 * *****************  Version 46  *****************
 * User: Jeff Vahue   Date: 8/02/10    Time: 5:36p
 * Updated in $/software/control processor/code/drivers
 * SCR# 707 - Add Test Points
 *
 * *****************  Version 45  *****************
 * User: Peter Lee    Date: 7/23/10    Time: 1:39p
 * Updated in $/software/control processor/code/drivers
 * SCR #688 Remove FPGA Ver 9 support.  SW will only be compatible with
 * Ver 12.
 *
 * *****************  Version 44  *****************
 * User: Jeff Vahue   Date: 7/17/10    Time: 5:46p
 * Updated in $/software/control processor/code/drivers
 * SCR# 707 - Add TestPoints for code coverage.
 *
 * *****************  Version 43  *****************
 * User: Contractor2  Date: 7/09/10    Time: 4:31p
 * Updated in $/software/control processor/code/drivers
 * SCR #8 Implementation: External Interrupt Monitors
 *
 * *****************  Version 42  *****************
 * User: John Omalley Date: 6/08/10    Time: 12:04p
 * Updated in $/software/control processor/code/drivers
 * SCR 627 - Updated for LJ60
 *
 * *****************  Version 41  *****************
 * User: Peter Lee    Date: 5/31/10    Time: 5:50p
 * Updated in $/software/control processor/code/drivers
 * SCR #601 FPGA DIO Logic Updates/Bug fixes
 *
 * *****************  Version 40  *****************
 * User: Contractor3  Date: 5/06/10    Time: 11:27a
 * Updated in $/software/control processor/code/drivers
 * Check In SCR #586
 *
 * *****************  Version 39  *****************
 * User: Peter Lee    Date: 4/07/10    Time: 2:34p
 * Updated in $/software/control processor/code/drivers
 * SCR #495 Additional FPGA PBIT requested by HW
 *
 * *****************  Version 38  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:14p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 37  *****************
 * User: Jeff Vahue   Date: 2/26/10    Time: 3:00p
 * Updated in $/software/control processor/code/drivers
 * SCR# 469 - Change DIO_ReadPin to return the pin value, WIN32 handling
 * of _DIR and _RIR
 *
 * *****************  Version 36  *****************
 * User: Peter Lee    Date: 1/28/10    Time: 6:01p
 * Updated in $/software/control processor/code/drivers
 * SCR #331 Verify QAR Barker settings and use CHECK_AND_SET() macro.
 *
 * *****************  Version 35  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:23p
 * Updated in $/software/control processor/code/drivers
 * SCR 377
 *
 * *****************  Version 34  *****************
 * User: Peter Lee    Date: 12/23/09   Time: 10:17a
 * Updated in $/software/control processor/code/drivers
 * SCR #383 Reliability of reading FPGA ver on first read and returning
 * BIT OR Arinc429 PBIT result to GSE PBIT display.
 *
 * *****************  Version 33  *****************
 * User: Peter Lee    Date: 12/14/09   Time: 1:56p
 * Updated in $/software/control processor/code/drivers
 * SCR #361 Proper use of TTMR_GetHSTickCount() for delays.
 *
 * *****************  Version 32  *****************
 * User: Peter Lee    Date: 12/04/09   Time: 5:45p
 * Updated in $/software/control processor/code/drivers
 * Undo SCR #361.  I've learn the errors of my way.
 *
 * *****************  Version 31  *****************
 * User: Peter Lee    Date: 12/04/09   Time: 5:24p
 * Updated in $/software/control processor/code/drivers
 * SCR #358 New FPGA changes to INT MASK and ISR Status reg
 *
 * *****************  Version 30  *****************
 * User: Peter Lee    Date: 12/04/09   Time: 2:15p
 * Updated in $/software/control processor/code/drivers
 * SCR #361 Error - Delays using TTMR_GetHSTickCount and rollover.
 *
 * *****************  Version 29  *****************
 * User: Peter Lee    Date: 12/02/09   Time: 2:18p
 * Updated in $/software/control processor/code/drivers
 * SCR #350 Misc Code Review Items
 *
 * *****************  Version 28  *****************
 * User: Peter Lee    Date: 11/02/09   Time: 6:09p
 * Updated in $/software/control processor/code/drivers
 * Misc Code update to support WIN32
 *
 * *****************  Version 27  *****************
 * User: Peter Lee    Date: 11/02/09   Time: 1:51p
 * Updated in $/software/control processor/code/drivers
 * Misc Code Update to support WIN32 and spelling corrections
 *
 * *****************  Version 26  *****************
 * User: Peter Lee    Date: 10/30/09   Time: 3:46p
 * Updated in $/software/control processor/code/drivers
 * SCR #315 CBIT Reg Check
 *
 * *****************  Version 25  *****************
 * User: Peter Lee    Date: 10/29/09   Time: 2:39p
 * Updated in $/software/control processor/code/drivers
 * SCR #315 CBIT Reg and SEU Processing
 *
 * *****************  Version 24  *****************
 * User: Peter Lee    Date: 10/15/09   Time: 9:59a
 * Updated in $/software/control processor/code/drivers
 * SCR 273 and FPGA CBIT Requirements
 *
 * *****************  Version 23  *****************
 * User: Peter Lee    Date: 9/15/09    Time: 6:03p
 * Updated in $/software/control processor/code/drivers
 * SCR #94, #95 PBIT returns log structure to Init Mgr
 *
 * *****************  Version 22  *****************
 * User: Peter Lee    Date: 9/14/09    Time: 3:33p
 * Updated in $/software/control processor/code/drivers
 * SCR #94 Updated Init for SET_CHECK() and return ERR CODE Struct
 *
 * *****************  Version 21  *****************
 * User: Peter Lee    Date: 6/29/09    Time: 2:20p
 * Updated in $/software/control processor/code/drivers
 * SCR #204 Misc Code Updates.
 *
 * *****************  Version 20  *****************
 * User: Peter Lee    Date: 4/29/09    Time: 3:46p
 * Updated in $/software/control processor/code/drivers
 * SCR #168 Removed comment from FPGA_Monitor_Ok() to force func to return
 * TRUE as temp fix for FPGA CRC Error always indicating Error.
 *
 * *****************  Version 19  *****************
 * User: Peter Lee    Date: 2/02/09    Time: 11:14a
 * Updated in $/control processor/code/drivers
 * Uncomment monitoring of FPGA Err Input
 *
 * *****************  Version 18  *****************
 * User: Peter Lee    Date: 1/20/09    Time: 6:25p
 * Updated in $/control processor/code/drivers
 * SCR #131
 * Updated FPGA Reg defines for LFR.
 *
 * *****************  Version 17  *****************
 * User: Peter Lee    Date: 1/14/09    Time: 4:45p
 * Updated in $/control processor/code/drivers
 * Allow FPGA Version Check to Always Pass for Development.  Remove before
 * product release.
 *
 * *****************  Version 16  *****************
 * User: Peter Lee    Date: 1/12/09    Time: 11:23a
 * Updated in $/control processor/code/drivers
 * Updated FPGA Register Defines (in FPGA.h) to be constants rather than
 * "Get Content of Address" (prefix by "*") to simply code references in
 * Arinc429, QAR and FPGA.
 *
 * *****************  Version 15  *****************
 * User: Peter Lee    Date: 1/06/09    Time: 4:45p
 * Updated in $/control processor/code/drivers
 * SCR 129 Fix FPGA_CheckVersion() to handle toggle of hw FPGA Version
 * identifier.
 *
 * *****************  Version 14  *****************
 * User: Peter Lee    Date: 1/06/09    Time: 2:42p
 * Updated in $/control processor/code/drivers
 * SCR 129 Misc FPGA Updates
 *
 * *****************  Version 13  *****************
 * User: Peter Lee    Date: 11/24/08   Time: 2:08p
 * Updated in $/control processor/code/drivers
 * SCr #97 Add New FPGA Register bits
 *
 * *****************  Version 12  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 11:58a
 * Updated in $/control processor/code/drivers
 * SCR #87 Function Prototype
 *
 * *****************  Version 11  *****************
 * User: John Omalley Date: 6/11/08    Time: 9:29a
 * Updated in $/control processor/code/drivers
 * Added QAR Enable bit
 *
 *
 *
 ***************************************************************************/
