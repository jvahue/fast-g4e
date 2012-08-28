#ifndef WATCHDOG_H
#define WATCHDOG_H
/******************************************************************************
            Copyright (C) 2009-2010 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:         WatchDog.h      
    
    Description:  Routines to toggle the external watchdog monitor
    
   VERSION
    $Revision: 10 $  $Date: 8/28/12 1:43p $
    
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "DIO.h"

/******************************************************************************
                                 Package Defines
******************************************************************************/
#define STARTUP_ID( x) startupId = x

// Internal SRAM initialized flags
// Used to detect unexpected system reboots and put system in degraded mode
#define SRAM_INIT_FLAG        (*(vuint32*)(void*)(&__MBAR[0x010000]))
#define SRAM_INIT_FLAG_INV    (*(vuint32*)(void*)(&__MBAR[0x010004]))
#define WATCHDOG_RESTART_FLAG (*(vuint32*)(void*)(&__MBAR[0x010008]))
#define ASSERT_COUNT          (*(vuint32*)(void*)(&__MBAR[0x01000C]))
#define UNKNOWN_RESTART_COUNT (*(vuint32*)(void*)(&__MBAR[0x010010]))
#define INIT_RESTART_COUNT    (*(vuint32*)(void*)(&__MBAR[0x010014]))
#define SRAM_INIT_SAVE        (*(vuint32*)(void*)(&__MBAR[0x010018]))
#define SRAM_INV_SAVE         (*(vuint32*)(void*)(&__MBAR[0x01001C]))

#define ASSERT_RESTART_SET    1     // Code for watchdog expected (assert) restart
#define UNKNOWN_RESTART_SET   2     // Code for unexpected restart
#define INIT_RESTART_SET      3     // Code for unexpected restart during init

/******************************************************************************
                                 Package Typedefs
******************************************************************************/

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( WATCHDOG_BODY )
   #define EXPORT
#else
   #define EXPORT extern
#endif

/******************************************************************************
                             Package Exports Variables
******************************************************************************/
EXPORT UINT32 startupId;
EXPORT BOOLEAN isWatchdogRestart;

/******************************************************************************
                             Package Exports Functions
******************************************************************************/
EXPORT void WatchdogReboot(BOOLEAN isNormalShutdown);

#endif // WATCHDOG_H                             


/*************************************************************************
 *  MODIFICATIONS
 *    $History: WatchDog.h $
 * 
 * *****************  Version 10  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 9  *****************
 * User: Contractor2  Date: 7/13/10    Time: 2:33p
 * Updated in $/software/control processor/code/system
 * SCR #638 Startup requests for Degraded Mode
 * Added init restart counter
 * 
 * *****************  Version 8  *****************
 * User: Contractor2  Date: 7/09/10    Time: 1:23p
 * Updated in $/software/control processor/code/system
 * SCR #638 Startup requests for Degraded Mode
 * Mods to detect restarts during PBIT
 * 
 * *****************  Version 7  *****************
 * User: Jeff Vahue   Date: 6/09/10    Time: 2:58p
 * Updated in $/software/control processor/code/system
 * SCR #638 - modification  made to investigate erroneous degraded mode,
 * seem to have fixed the issue?  Test will detect any occurance in the
 * future.
 * 
 * *****************  Version 6  *****************
 * User: Contractor2  Date: 6/07/10    Time: 11:51a
 * Updated in $/software/control processor/code/system
 * SCR #628 Count SEU resets during data capture
 * 
 * *****************  Version 5  *****************
 * User: Jeff Vahue   Date: 4/28/10    Time: 5:53p
 * Updated in $/software/control processor/code/system
 * SCR #573 - Startup WD changes
 * 
 * *****************  Version 4  *****************
 * User: Contractor2  Date: 4/27/10    Time: 1:36p
 * Updated in $/software/control processor/code/system
 * SCR 187: Degraded mode. Added watchdog reset loop function.
 * 
 * *****************  Version 3  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:59p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 2  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:28p
 * Updated in $/software/control processor/code/system
 * SCR 377
 * 
 * *****************  Version 1  *****************
 * User: Peter Lee    Date: 11/24/09   Time: 2:21p
 * Created in $/software/control processor/code/system
 * FPGA Watchdog Implementation
 * 
 ***************************************************************************/
