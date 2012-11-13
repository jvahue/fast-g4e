#define WATCHDOG_BODY
/******************************************************************************
            Copyright (C) 2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:        WatchDog.c 
    
    Description: Prepares for system shutdown and waits for Watchdog Restart

   VERSION
      $Revision: 11 $  $Date: 12-11-13 3:27p $    
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "WatchDog.h"
#include "Assert.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
// Init flag values for normal shutdown/reboot
// Must not be inverse values of each other to avoid assert detection
#define SHUTDOWN_FLAG_1 0x12345678
#define SHUTDOWN_FLAG_2 0xBAD0F00D

#ifdef STE_TP
#include "DIO.h"
#include "FPGA.h"
#include "TTMR.h"

extern BOOLEAN enableSdDump;
extern void ReportCoverage(void);
#endif

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/*****************************************************************************
 * Function:    WatchdogReboot
 *
 * Description: Disables interrupts and waits for the Watchdog Timer to
 *              reboot the system
 *
 * Parameters:  isNormalShutdown - BOOLEAN
 *              
 * Returns:     Does not return
 *
 * Notes:       
 *              
 ****************************************************************************/
void WatchdogReboot(BOOLEAN isNormalShutdown)
{
  __DIR();    // Disable Interrupts.

  if (isNormalShutdown)
  {
    SRAM_INIT_FLAG     = SHUTDOWN_FLAG_1;           // reset for expected reboot
    SRAM_INIT_FLAG_INV = SHUTDOWN_FLAG_2;
  }
  else
  {
    WATCHDOG_RESTART_FLAG = ASSERT_RESTART_SET;     // set for expected assert restart
  }

#ifndef WIN32

  #ifdef STE_TP
  /*vcast_dont_instrument_start*/

    if ( TRUE == enableSdDump || !isNormalShutdown)
    {
        // Latch Battery
        DIO_SetPin( PowerHold, DIO_SetHigh );

        // Mask all but the GSE interrupt so we can dump the coverage data 
        // - order is important when bit 0 set all are set
        MCF_INTC_IMRL = 0xFFFFFFFF;
        MCF_INTC_IMRH = 0xFFFFFFFF & ~MCF_INTC_IMRH_INT_MASK35;

        __EI();

        // WD is pulsed while dumping data
        ReportCoverage();

        // While the GSE Tx reg is not empty (DNW)
        while ( !(MCF_PSC_SR(0) & MCF_PSC_SR_TXEMP_URERR))
        { 
            // kick the WD
            KICKDOG;
        }

        // Un-Latch Battery
        DIO_SetPin( PowerHold, DIO_SetLow );
    }

  /*vcast_dont_instrument_end*/
  #endif

  while(TRUE)
  {
    // wait for watchdog reboot
  }

#else
  /*vcast_dont_instrument_start*/
  wdResetRequest = TRUE;
  /*vcast_dont_instrument_end*/
#endif
}

 
/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/



/*************************************************************************
 *  MODIFICATIONS
 *    $History: WatchDog.c $
 * 
 * *****************  Version 11  *****************
 * User: Melanie Jutras Date: 12-11-13   Time: 3:27p
 * Updated in $/software/control processor/code/system
 * SCR #1142 File Format Error
 * 
 * *****************  Version 10  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 9  *****************
 * User: Jeff Vahue   Date: 10/16/10   Time: 5:27p
 * Updated in $/software/control processor/code/system
 * SCR# 848 - enable Coverage dump when doing an ASSERT shutdown.
 * 
 * *****************  Version 8  *****************
 * User: Contractor2  Date: 10/04/10   Time: 1:25p
 * Updated in $/software/control processor/code/system
 * SCR #916 Code Review Updates
 * 
 * *****************  Version 7  *****************
 * User: Jeff Vahue   Date: 8/22/10    Time: 4:39p
 * Updated in $/software/control processor/code/system
 * SCR #707 - Coverage mods
 * 
 * *****************  Version 6  *****************
 * User: Jeff Vahue   Date: 8/22/10    Time: 1:44p
 * Updated in $/software/control processor/code/system
 * SCR# 707 - try another approach to dumping coverage data at power down,
 * wait 40 s
 * 
 * *****************  Version 5  *****************
 * User: Jeff Vahue   Date: 7/19/10    Time: 6:50p
 * Updated in $/software/control processor/code/system
 * SCR# 707 - Code Coverage TP
 * 
 * *****************  Version 4  *****************
 * User: Contractor2  Date: 7/09/10    Time: 1:23p
 * Updated in $/software/control processor/code/system
 * SCR #638 Startup requests for Degraded Mode
 * Mods to detect restarts during PBIT
 * 
 * *****************  Version 3  *****************
 * User: Jeff Vahue   Date: 6/09/10    Time: 2:58p
 * Updated in $/software/control processor/code/system
 * SCR #638 - modification  made to investigate erroneous degraded mode,
 * seem to have fixed the issue?  Test will detect any occurance in the
 * future.
 * 
 * *****************  Version 2  *****************
 * User: Contractor2  Date: 6/07/10    Time: 11:51a
 * Updated in $/software/control processor/code/system
 * SCR #628 Count SEU resets during data capture
 * 
 * *****************  Version 1  *****************
 * User: Contractor2  Date: 4/27/10    Time: 1:35p
 * Created in $/software/control processor/code/system
 * SCR 187: Degraded mode. Added watchdog reset loop funcction.
 *
 *
 ***************************************************************************/
