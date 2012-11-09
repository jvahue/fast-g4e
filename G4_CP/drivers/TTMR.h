#ifndef TTMR_H
#define TTMR_H

/******************************************************************************
            Copyright (C) 2007-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

  File:         TTMR.h

  Description: This header file includes the variables and funtions necessary
               for the Task Timer Driver. See the c module for a detailed
               description.

   VERSION
   $Revision: 29 $  $Date: 12-11-09 1:59p $

******************************************************************************/



/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "ResultCodes.h"
#include "mcf548x.h"
#include "SystemLog.h"

/******************************************************************************
                              Package Defines
******************************************************************************/
#define TICKS_PER_10nSec    1
#define TICKS_PER_100nSec   10
#define TICKS_PER_500nSec   50
#define TICKS_PER_uSec      100
#define TICKS_PER_mSec      100000
#define TICKS_PER_10msec    1000000
#define TICKS_PER_100msec   10000000
#define TICKS_PER_500mSec   50000000
#define TICKS_PER_Sec       100000000

#define TTMR_GPT0_INT_LEVEL    0x06
#define TTMR_GPT0_INT_PRIORITY 0x03

//Ticks per uS for the TTMR_GetHSTickCount function
#define TTMR_HS_TICKS_PER_uS   100

//Tick interrupt (typically will be used for the TaskManager) interval in uS.
#define TTMR_TASK_MGR_TICK_INTERRUPT_INTERVAL_uS 10000 //10000us or 10ms
#define TTMR_TASK_MGR_TICK_INTERRUPT_INTERVAL (TTMR_TASK_MGR_TICK_INTERRUPT_INTERVAL_uS \
                                               * TICKS_PER_uSec)

#define MAX_STARTUP_DURATION 1000  // 10 seconds (1000 MIFs)

#ifdef SUPPORT_FPGA_BELOW_V12
#define OLDDOG(a,b) DIO_SetPin(a,b)
#else
#define OLDDOG(a,b)
#endif

#define KICKDOG \
{ \
  OLDDOG(WDog,DIO_Toggle);                           \
  FPGA_W(FPGA_WD_TIMER, FPGA_WD_TIMER_VAL);          \
  FPGA_W(FPGA_WD_TIMER_INV, FPGA_WD_TIMER_INV_VAL);  \
}

/******************************************************************************
                              Package Typedefs
******************************************************************************/
typedef void (*TICK_INTERRUPT_HANDLER)(UINT32 LastIntLvl);

#pragma pack(1)
typedef struct {
  RESULT  result;
} TTMR_DRV_PBIT_LOG;
#pragma pack()

/******************************************************************************
                              Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( TTMR_BODY )
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
EXPORT RESULT           TTMR_Init(SYS_APP_ID *SysLogId, void *pdata, UINT16 *psize);
EXPORT void             TTMR_SetTaskManagerTickFunc(
                                           TICK_INTERRUPT_HANDLER TickHandler
                                                   );
EXPORT UINT32           TTMR_GetHSTickCount        (void);
EXPORT void             TTMR_Delay                 (UINT32 delay);
EXPORT __interrupt void TTMR_GPT0ISR               (void);

EXPORT UINT32           TTMR_Get10msTickCount      (void);


#endif // TTMR_H
/******************************************************************************
 *  MODIFICATIONS
 *    $History: TTMR.h $
 * 
 * *****************  Version 29  *****************
 * User: Melanie Jutras Date: 12-11-09   Time: 1:59p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 File Format Error
 * 
 * *****************  Version 28  *****************
 * User: Melanie Jutras Date: 12-11-02   Time: 1:15p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 File Format Error
 *
 * *****************  Version 27  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 26  *****************
 * User: Contractor2  Date: 9/24/10    Time: 3:02p
 * Updated in $/software/control processor/code/drivers
 * SCR #882 WD - Old Code for pulsing the WD should be removed
 *
 * *****************  Version 25  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:13a
 * Updated in $/software/control processor/code/drivers
 * SCR #703 - Fixes for code review findings.
 *
 * *****************  Version 24  *****************
 * User: Contractor V&v Date: 7/27/10    Time: 2:26p
 * Updated in $/software/control processor/code/drivers
 * SCR #573 Startup WD Management/DT overrun at startup
 *
 * *****************  Version 23  *****************
 * User: Peter Lee    Date: 7/22/10    Time: 6:09p
 * Updated in $/software/control processor/code/drivers
 * SCR #582 Update Power On Usage count logic to better incl startup
 * timing.
 *
 * *****************  Version 22  *****************
 * User: Jim Mood     Date: 6/16/10    Time: 4:20p
 * Updated in $/software/control processor/code/drivers
 * SCR 648.  Save pre-interrupt interrupt level to nest interrupts instead
 * of restoring interrupt level to 0 with __EI();
 *
 * *****************  Version 21  *****************
 * User: Jeff Vahue   Date: 4/28/10    Time: 5:47p
 * Updated in $/software/control processor/code/drivers
 * SCR # 573 - startup WD changes, change the Timer interrupt level to 6
 *
 * *****************  Version 20  *****************
 * User: Contractor3  Date: 4/09/10    Time: 12:17p
 * Updated in $/software/control processor/code/drivers
 * SCR #539 Updated for Code Review
 *
 * *****************  Version 19  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:22p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 18  *****************
 * User: Jeff Vahue   Date: 2/26/10    Time: 11:22a
 * Updated in $/software/control processor/code/drivers
 * SCR# 468 - Move minute timer display to Utilities
 *
 * *****************  Version 17  *****************
 * User: Peter Lee    Date: 12/04/09   Time: 5:45p
 * Updated in $/software/control processor/code/drivers
 * Undo SCR #361.  I've learn the errors of my way.
 *
 * *****************  Version 16  *****************
 * User: Peter Lee    Date: 12/04/09   Time: 2:16p
 * Updated in $/software/control processor/code/drivers
 * SCR #361 Delays using TTMR_GetHSTickCount and rollover
 *
 * *****************  Version 15  *****************
 * User: John Omalley Date: 10/01/09   Time: 4:31p
 * Updated in $/software/control processor/code/drivers
 * Added tick interval for timing sensors and triggers
 *
 *
 *****************************************************************************/

