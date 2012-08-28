#define DRV_TTMR_BODY

/******************************************************************************
Copyright (C) 2008-2012 Pratt & Whitney Engine Services, Inc.                 
               All Rights Reserved.  Proprietary and Confidential.


 File:        TTMR.c         

 Description: Task TiMeR driver.  This unit provides a constant interval interrupt
              to TaskManager.c, for generating the minor frame rate.  Currently,
              this driver is setup to produce a MIF rate of 10ms.
              The driver also provides a high-resolution tick counter (100MHz) for 
              task execution timing.  The application can also use this tick
              counter for any other purpose needing high-speed timing

              Hardware Resources used:
              General Purpose Timer 0 - Generates the MIF rate
              Slice Timer 0 - Generates the high speed/high resolution tick counter
              
  VERSION
  $Revision: 35 $  $Date: 8/28/12 1:06p $

******************************************************************************/


/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include "mcf548x.h"
#include <stdio.h>

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "ResultCodes.h"
#include "WatchDog.h"
#include "TTMR.h"
#include "FaultMgr.h"
#include "alt_basic.h"
#include "GSE.h"
#include "InitializationManager.h"

#include "stddefs.h"
#include "UtilRegSetCheck.h"

#include "TestPoints.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static TICK_INTERRUPT_HANDLER TaskMgrTickHandler;
UINT32 TestCounter = 0;
//UINT32 TmExeTime = 0;
BOOLEAN ReEntered = FALSE;


const REG_SETTING TTMR_Registers[] = 
{
  //Setup GPT0 for a frame interval timer
  // Timer GPT0 configured for internal CPU timer operation
  // Timeout value = 10.00 milliseconds IF GPT TIMER IS MODIFIED, ALSO CHANGE
  //                                    TTMR_TASK_MGR_TICK_INT_uS in TTMR.h
  // Interrupt on CPU timer timeout enabled
  // Counter is reset by CPU timer timeout and a new timeout begins
  // GPIO configured as input on TIN0
  
  // SET_AND_CHECK ( MCF_GPT_GPWM0, MCF_GPT_GPWM_WIDTH(0x32), bInitOk) ; 
  {(void *) &MCF_GPT_GPWM0, MCF_GPT_GPWM_WIDTH(0x32), sizeof(UINT32), 0x0, REG_SET, TRUE, 
            RFA_FORCE_UPDATE}, 

  // SET_AND_CHECK ( MCF_GPT_GCIR0, (MCF_GPT_GCIR_PRE(0x64) | MCF_GPT_GCIR_CNT(0x2710)), 
  //                bInitOk ); 
  {(void *) &MCF_GPT_GCIR0, (MCF_GPT_GCIR_PRE(0x64) | MCF_GPT_GCIR_CNT(0x2710)), 
                            sizeof(UINT32), 0x0, REG_SET, TRUE, RFA_FORCE_UPDATE},

  // SET_AND_CHECK ( MCF_GPT_GMS0, (MCF_GPT_GMS_CE | MCF_GPT_GMS_SC | MCF_GPT_GMS_IEN |
  //                               MCF_GPT_GMS_TMS(0x4)), bInitOk ); 
  {(void *) &MCF_GPT_GMS0, (MCF_GPT_GMS_CE | MCF_GPT_GMS_SC | MCF_GPT_GMS_IEN |
                            MCF_GPT_GMS_TMS(0x4)), sizeof(UINT32), 0x00, REG_SET, TRUE, 
                            RFA_FORCE_UPDATE},
                  
  //Setup Slice Timer 0 for a high-speed/high-resolution tick counter
  // Slice Timer 0 Enabled. Timeout = 42 seconds
  // Timer runs continuously - immediately reloaded on timeout
  // SET_AND_CHECK ( MCF_SLT_SLTCNT0, 0xFFFFFFFF, bInitOk ); 
  {(void *) &MCF_SLT_SLTCNT0, 0xFFFFFFFF, sizeof(UINT32), 0x0, REG_SET, FALSE, 
                              RFA_FORCE_NONE}, 
  
  // SET_AND_CHECK ( MCF_SLT_SCR0, (MCF_SLT_SCR_RUN | MCF_SLT_SCR_TEN), bInitOk ); 
  {(void *) &MCF_SLT_SCR0, (MCF_SLT_SCR_RUN | MCF_SLT_SCR_TEN), sizeof(UINT32), 0x00, 
                            REG_SET, TRUE, RFA_FORCE_UPDATE},
  
  //Enable interrupts for the general purpose timer
  // SET_AND_CHECK ( MCF_INTC_ICR62, ( MCF_INTC_ICRn_IL(TTMR_GPT0_INT_LEVEL) |
  //                                  MCF_INTC_ICRn_IP(TTMR_GPT0_INT_PRIORITY)), bInitOk ); 
  {(void *) &MCF_INTC_ICR62, ( MCF_INTC_ICRn_IL(TTMR_GPT0_INT_LEVEL) | 
                               MCF_INTC_ICRn_IP(TTMR_GPT0_INT_PRIORITY)), sizeof(UINT8), 
                               0x00, REG_SET, TRUE, RFA_FORCE_UPDATE},

  // SET_CHECK_MASK_AND ( MCF_INTC_IMRH, MCF_INTC_IMRH_INT_MASK62, bInitOk ); 
  {(void *) &MCF_INTC_IMRH, MCF_INTC_IMRH_INT_MASK62, sizeof(UINT32), 
                            0x00, REG_SET_MASK_AND, FALSE, RFA_FORCE_NONE}
                            
};


/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/

void TTMR_GetLastIntLvl(UINT32* IntLvl);

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/


/*****************************************************************************
 * Function:    TTMR_Init
 *
 * Description: Initial setup for the task timer.  Initializes and starts and
 *              the general purpose timer and the high speed slice timer.
 *
 * Parameters:  SysLogId - Ptr to return System Log ID into 
 *              pdata -    Ptr to buffer to return system log data 
 *              psize -    Ptr to return size (bytes) of system log data
 *
 * Returns:     RESULT: DRV_OK = Success OR 
 *                      DRV_TTMR_REG_INIT_FAIL
 *              
 * Notes:       none
 ****************************************************************************/
RESULT TTMR_Init (SYS_APP_ID *SysLogId, void *pdata, UINT16 *psize) 
{
  UINT8 i; 
  BOOLEAN bInitOk; 
  TTMR_DRV_PBIT_LOG *pdest; 
 
  // Used to extend WD timeout during startup, value ID location in startup
  startupId = 0;
 
  pdest = (TTMR_DRV_PBIT_LOG *) pdata; 
  memset ( pdest, 0, sizeof(TTMR_DRV_PBIT_LOG) ); 
  pdest->result = DRV_OK; 
  
  *psize = sizeof(TTMR_DRV_PBIT_LOG);  
  
  TaskMgrTickHandler = NULL;
  
  // Initialize the TTMR Registers  
  bInitOk = TRUE; 
  for (i=0;i<(sizeof(TTMR_Registers)/sizeof(REG_SETTING));i++) 
  {
    bInitOk &= RegSet( (REG_SETTING_PTR) &TTMR_Registers[i] ); 
  }
  
  
  if (TRUE != STPU( bInitOk, eTpTtmr3806)) 
  {
    pdest->result = DRV_TTMR_REG_INIT_FAIL; 
    *SysLogId = DRV_ID_TTMR_PBIT_REG_INIT_FAIL; 
  }

  return pdest->result;
}


/*****************************************************************************
 * Function:    TTMR_SetTaskManagerTickFunc
 *
 * Description: Sets the address of the Task Manager's tick function.  This
 *              routine is called back when the frame timer rolls over.  
 *              The driver is currently configured to call back to TaskManager
 *              every 10ms
 *
 * Parameters:  TickHandler 
 *
 * Returns:     void
 *
 * Notes:
 *
 ****************************************************************************/
void TTMR_SetTaskManagerTickFunc(TICK_INTERRUPT_HANDLER TickHandler)
{
  TaskMgrTickHandler = TickHandler;
}


/*****************************************************************************
 * Function:    TTMR_GetHSTickCount
 * Description: Gets the current high-speed tick count.  Call only after
 *              successfully calling TTMR_Init, there is no error checking
 *              in this routine for simplicity, it only returns the value
 *              of the slice timer count.  The slice timer runs at 100MHz
 * Parameters:  None
 *
 * Returns:     UINT32: Unsigned 32-bit counter value, 10ns per tick
 *              
 * Notes:
 *
 ****************************************************************************/
UINT32 TTMR_GetHSTickCount(void)
{
#if defined(WIN32)
/*vcast_dont_instrument_start*/
    return clock() * 100000;
/*vcast_dont_instrument_end*/
#else
  //Invert SCNT0 register to convert the down-counter to an up-counter//
  return (~MCF_SLT_SCNT0);
#endif
}

/*vcast_dont_instrument_start*/
/*****************************************************************************
 * Function:    TTMR_Delay
 *
 * Description: Provides a delay based on the TTMR high speed counter 
 *
 * Parameters:  delay time     
 *
 * Returns:     none
 *              
 * Notes:       No VectorCast coverage due to timing issues
 *
 ****************************************************************************/
void TTMR_Delay(UINT32 delay)
{
  UINT32 StartTime; 

  StartTime = TTMR_GetHSTickCount(); 
  while ( (TTMR_GetHSTickCount() - StartTime) < delay ); 
}
/*vcast_dont_instrument_end*/


/*****************************************************************************
 * Function:    TTMR_GetLastIntLvl
 *
 * Description: Get the interrupt level from immediatly before this exception
 *              The 3-bit level is stored in the caller's location pointed
 *              to by IntLvl.  It is not shifted, but stored in bits 8:10
 *              as it is in the SR.  Other bits are masked to zero
 *
 * Parameters:  [in/out] IntLvl: Pointer to a location to store the interrupt
 *                               level
 *
 * Returns:     none
 *              
 * Notes:       GHS asm macro accepts the parameter as a register, constant or
 *              memory. (%reg, %con, %mem)
 *
 ****************************************************************************/
#ifndef WIN32
asm void TTMR_GetLastIntLvl(UINT32* IntLvl)
{
%reg val
  MOVEA.L IntLvl,%A0  /*Store ptr to IntLvl in A0*/
  MOVE.L  4(%A6),%D0  /*Get the interrupt frame containing the SR*/
  ANDI.L  #$700,%D0   /*Mask off to just the Interrupt level*/
  MOVE.L  %D0,(%A0)   /*Store 3-bit interrupt level in caller's location*/
%con val
  MOVEA.L IntLvl,%A0  /*Store ptr to IntLvl in A0*/
  MOVE.L  4(%A6),%D0  /*Get the interrupt frame containing the SR*/
  ANDI.L  #$700,%D0   /*Mask off to just the Interrupt level*/
  MOVE.L  %D0,(%A0)
%mem val
  MOVEA.L IntLvl,%A0  /*Store ptr to IntLvl in A0*/
  MOVE.L  4(%A6),%D0  /*Get the interrupt frame containing the SR*/
  ANDI.L  #$700,%D0   /*Mask off to just the Interrupt level*/
  MOVE.L  %D0,(%A0)

%error
}
#else
/*vcast_dont_instrument_start*/
void TTMR_GetLastIntLvl(UINT32* IntLvl)
{
    *IntLvl = 0;
}
/*vcast_dont_instrument_end*/
#endif

/*****************************************************************************
 * Function:    TTMR_GPT0ISR               INTERRUPT HANDLER
 *
 * Description: General Purpose Timer 0 interrupt.  The GPT0 is initialized
 *              to generate an interrupt every 10ms.  If a TaskManager
 *              routine is set in TaskMgrTickHandler, it is called.  Task
 *              manager execution time is tracked in TmExeTime.
 *
 *
 * Parameters:      
 *
 * Returns:     
 *              
 * Notes:
 *
 ****************************************************************************/
 __interrupt void TTMR_GPT0ISR(void)
{  
  UINT32 LastIntLvl;

  TTMR_GetLastIntLvl(&LastIntLvl);

  MCF_GPT_GSR0 = MCF_GPT_GSR_TEXP;
  TestCounter++;

  if(TaskMgrTickHandler != NULL)
  {
    //TmExeTime = TTMR_GetHSTickCount();
    TaskMgrTickHandler(LastIntLvl);
    //TmExeTime = (TTMR_GetHSTickCount() - TmExeTime)/TTMR_HS_TICKS_PER_uS;
  }
  else
  {
    // Startup not complete, tick-handler not yet assigned.
    // call the startup handler to process interrupt.
    Im_StartupTickHandler(); 
  }
}



/*****************************************************************************
 * Function:    TTMR_Get10msTickCount 
 *
 * Description: Returns the free running 10 msec TestCounter which counts the 
 *              number of 10 msec ticks since TTMR start
 *
 * Parameters:  none
 *
 * Returns:     UINT32 - TestCount 
 *              
 * Notes:       none
 *
 ****************************************************************************/
 UINT32 TTMR_Get10msTickCount ( void ) 
 {
   return (TestCounter); 
 }

/*****************************************************************************/
/* Local Functions                                                           */ 
/*****************************************************************************/ 

/*************************************************************************
 *  MODIFICATIONS
 *    $History: TTMR.c $
 * 
 * *****************  Version 35  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 34  *****************
 * User: Jeff Vahue   Date: 10/15/10   Time: 4:21p
 * Updated in $/software/control processor/code/drivers
 * SCR# 848 - Code Coverage this needed a STPU vs a TPU
 * 
 * *****************  Version 33  *****************
 * User: Jeff Vahue   Date: 10/14/10   Time: 8:13p
 * Updated in $/software/control processor/code/drivers
 * SCR# 848 - Code Coverage
 * 
 * *****************  Version 32  *****************
 * User: Contractor3  Date: 7/29/10    Time: 12:26p
 * Updated in $/software/control processor/code/drivers
 * SCR #703 - Fix for code review findings.
 * 
 * *****************  Version 31  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:13a
 * Updated in $/software/control processor/code/drivers
 * SCR #703 - Fixes for code review findings.
 * 
 * *****************  Version 30  *****************
 * User: Contractor V&v Date: 7/27/10    Time: 2:25p
 * Updated in $/software/control processor/code/drivers
 * SCR #573 Startup WD Management/DT overrun at startup
 * 
 * *****************  Version 29  *****************
 * User: Peter Lee    Date: 7/22/10    Time: 6:09p
 * Updated in $/software/control processor/code/drivers
 * SCR #582 Update Power On Usage count logic to better incl startup
 * timing. 
 * 
 * *****************  Version 28  *****************
 * User: Jim Mood     Date: 7/12/10    Time: 5:46p
 * Updated in $/software/control processor/code/drivers
 * SCR #695 GetLastInterruptLvl() did not return the interrupt from the
 * correct position on the stack
 * 
 * *****************  Version 27  *****************
 * User: Jeff Vahue   Date: 6/18/10    Time: 4:39p
 * Updated in $/software/control processor/code/drivers
 * Windows emulation compatibility
 * 
 * *****************  Version 26  *****************
 * User: Jim Mood     Date: 6/16/10    Time: 4:20p
 * Updated in $/software/control processor/code/drivers
 * SCR 648.  Save pre-interrupt interrupt level to nest interrupts instead
 * of restoring interrupt level to 0 with __EI();
 * 
 * *****************  Version 25  *****************
 * User: Jeff Vahue   Date: 4/28/10    Time: 5:47p
 * Updated in $/software/control processor/code/drivers
 * SCR # 573 - startup WD changes, change the Timer interrupt level to 6
 * 
 * *****************  Version 24  *****************
 * User: Contractor3  Date: 4/09/10    Time: 12:17p
 * Updated in $/software/control processor/code/drivers
 * SCR #539 Updated for Code Review
 * 
 * *****************  Version 23  *****************
 * User: Jeff Vahue   Date: 3/11/10    Time: 11:10a
 * Updated in $/software/control processor/code/drivers
 * SCR# 481 - inhibit VectorCast coverage
 * 
 * *****************  Version 22  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 3:54p
 * Updated in $/software/control processor/code/drivers
 * SCR #67 Interrupted SPI Access Fixed calc error
 * 
 * *****************  Version 21  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:22p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 20  *****************
 * User: Jeff Vahue   Date: 2/26/10    Time: 11:22a
 * Updated in $/software/control processor/code/drivers
 * SCR# 468 - Move minute timer display to Utilities
 * 
 * *****************  Version 19  *****************
 * User: Jeff Vahue   Date: 1/27/10    Time: 6:54p
 * Updated in $/software/control processor/code/drivers
 * SCR# 222
 * 
 * *****************  Version 18  *****************
 * User: Jeff Vahue   Date: 1/23/10    Time: 3:09p
 * Updated in $/software/control processor/code/drivers
 * SCR# 369
 * 
 * *****************  Version 17  *****************
 * User: Jeff Vahue   Date: 12/18/09   Time: 1:34p
 * Updated in $/software/control processor/code/drivers
 * SCR# 378
 * 
 * *****************  Version 16  *****************
 * User: Peter Lee    Date: 12/04/09   Time: 5:45p
 * Updated in $/software/control processor/code/drivers
 * Undo SCR #361.  I've learn the errors of my way. 
 * 
 * *****************  Version 15  *****************
 * User: Peter Lee    Date: 12/04/09   Time: 2:16p
 * Updated in $/software/control processor/code/drivers
 * SCR #361 Delays using TTMR_GetHSTickCount and rollover
 * 
 * *****************  Version 14  *****************
 * User: Peter Lee    Date: 11/02/09   Time: 2:08p
 * Updated in $/software/control processor/code/drivers
 * Misc non code update to support WIN32 and spelling.
 * 
 * *****************  Version 13  *****************
 * User: Peter Lee    Date: 10/23/09   Time: 9:34a
 * Updated in $/software/control processor/code/drivers
 * SCR #315 CBIT / SEU Reg Proccessing
 * 
 * *****************  Version 12  *****************
 * User: Peter Lee    Date: 9/30/09    Time: 9:53a
 * Updated in $/software/control processor/code/drivers
 * Fix header comment for TTMR_Delay
 * 
 * *****************  Version 11  *****************
 * User: Peter Lee    Date: 9/15/09    Time: 6:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #94, #95 PBIT returns log structure to Init Mgr
 * 
 * *****************  Version 10  *****************
 * User: Peter Lee    Date: 9/14/09    Time: 3:54p
 * Updated in $/software/control processor/code/drivers
 * SCR #94 Updated Init for SET_CHECK() and return ERR CODE Struct
 * 
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 6/29/09    Time: 2:57p
 * Updated in $/software/control processor/code/drivers
 * SCR #204 Misc Code Review Updates
 * 
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 1:57p
 * Updated in $/control processor/code/drivers
 * SCR #87 Function Prototype
 * 
 *
 ***************************************************************************/
 
