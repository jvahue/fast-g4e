#ifndef FASTMGR_H
#define FASTMGR_H

/******************************************************************************
            Copyright (C) 2007-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:         FASTMgr.h

    Description:  Monitors triggers and other inputs to determine when the
                  in-flight, recording, battery latch, and weight on wheel
                  events occur, and notifies other tasks that handle these
                  events.

    VERSION
    $Revision: 28 $  $Date: 8/28/12 12:43p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "ClockMgr.h"   // For TIME_SOURCE_ENUM

/******************************************************************************
                                 Package Defines                              *
******************************************************************************/
#define FAST_TXTEST_SYSCON_FAIL_STR "Fail: System Condition is FAULT"
#define FAST_TXTEST_WOWDIS_FAIL_STR "Fail: WOW discrete is in-air"
#define FAST_TXTEST_ONGRND_FAIL_STR "Fail: On Ground trigger is inactive"
#define FAST_TXTEST_RECORD_FAIL_STR "Fail: Record trigger is active"
#define FAST_TXTEST_MSREDY_FAIL_STR "Fail: Micro-Server ready timeout"
#define FAST_TXTEST_SIMCRD_FAIL_STR "Fail: SIM card not detected"
#define FAST_TXTEST_GSMSIG_FAIL_STR "Fail: GSM signal not connected"
#define FAST_TXTEST_VPNSTA_FAIL_STR "Fail: VPN connect timeout"

#define SW_VERSION_LEN      32

/******************************************************************************
                                 Package Typedefs                             *
******************************************************************************/
typedef struct
{
  BITARRAY128  RecordTriggers;
  BITARRAY128  OnGroundTriggers;
  UINT32 AutoULPer_s;
  TIME_SOURCE_ENUM TimeSource;
  UINT32 TxTestMsRdyTO;
  UINT32 TxTestSIMRdyTO;  
  UINT32 TxTestGSMRdyTO;
  UINT32 TxTestVPNRdyTO;
}FASTMGR_CONFIG;

#define FASTMGR_CONFIG_DEFAULT {0,0,0,0},{0,0,0,0},300,TIME_SOURCE_LOCAL,120,180,300,300

/******************************************************************************
                                 Package Exports                              *
******************************************************************************/
#undef EXPORT

#if defined ( FASTMGR_BODY )
   #define EXPORT
#else
   #define EXPORT extern
#endif

/******************************************************************************
                             Package Exports Variables
******************************************************************************/
#if !defined ( FASTMGR_BODY )
EXPORT USER_ENUM_TBL TimeSourceStrs[];
#endif
/******************************************************************************
                             Package Exports Functions
******************************************************************************/
EXPORT void                 FAST_Init                ( void );
EXPORT TIME_SOURCE_ENUM     FAST_TimeSourceCfg       ( void ); 
EXPORT void                 FAST_GetSoftwareVersion  ( INT8* SwVerStr );
EXPORT void                 FAST_SignalUploadComplete( void);
EXPORT void                 FAST_TurnOffFASTControl  ( void);
EXPORT BOOLEAN              FAST_FSMRfGetState       (INT32 param);
EXPORT void                 FAST_FSMRfRun            (BOOLEAN Run,INT32 param);
EXPORT void                 FAST_FSMEndOfFlightRun   (BOOLEAN Run, INT32 Param);

#endif // FASTMGR_H
/*************************************************************************
 *  MODIFICATIONS
 *    $History: FASTMgr.h $
 * 
 * *****************  Version 28  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 12:43p
 * Updated in $/software/control processor/code/application
 * SCR# 1142
 * 
 * *****************  Version 27  *****************
 * User: Contractor V&v Date: 3/21/12    Time: 6:46p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 Renamed TRIGGER_FLAGS
 * 
 * *****************  Version 26  *****************
 * User: Contractor V&v Date: 3/14/12    Time: 4:51p
 * Updated in $/software/control processor/code/application
 * SCR #1107 Trigger processing
 * 
 * *****************  Version 25  *****************
 * User: John Omalley Date: 10/11/11   Time: 4:59p
 * Updated in $/software/control processor/code/application
 * SCR 1078 - Code Review Update
 * 
 * *****************  Version 24  *****************
 * User: Jim Mood     Date: 7/26/11    Time: 3:06p
 * Updated in $/software/control processor/code/application
 * SCR 575 updates
 * 
 * *****************  Version 23  *****************
 * User: Jim Mood     Date: 7/20/11    Time: 10:54a
 * Updated in $/software/control processor/code/application
 * SCR 575: GSM Enable when engine status is lost.  (Part of changes for
 * the Fast State Machine)
 * 
 * *****************  Version 22  *****************
 * User: Contractor2  Date: 10/05/10   Time: 12:18p
 * Updated in $/software/control processor/code/application
 * SCR #916 Code Review Updates
 * 
 * *****************  Version 21  *****************
 * User: Jeff Vahue   Date: 9/21/10    Time: 5:39p
 * Updated in $/software/control processor/code/application
 * SCR# 880 - Busy Logic
 * 
 * *****************  Version 20  *****************
 * User: Jim Mood     Date: 8/12/10    Time: 5:54p
 * Updated in $/software/control processor/code/application
 * SCR 784 Configuration defaults and a spelling error
 * 
 * *****************  Version 19  *****************
 * User: Jim Mood     Date: 7/30/10    Time: 8:56a
 * Updated in $/software/control processor/code/application
 * SCR 327 Fast transmssion test functionality
 * 
 * *****************  Version 18  *****************
 * User: Contractor V&v Date: 7/21/10    Time: 7:12p
 * Updated in $/software/control processor/code/application
 * SCR #260 Implement BOX status command
 * 
 * *****************  Version 17  *****************
 * User: John Omalley Date: 6/08/10    Time: 12:03p
 * Updated in $/software/control processor/code/application
 * SCR 627 - Updated for LJ60
 * 
 * *****************  Version 16  *****************
 * User: Jeff Vahue   Date: 5/03/10    Time: 3:09p
 * Updated in $/software/control processor/code/application
 * SCR# 580 - code review changes
 * 
 * *****************  Version 15  *****************
 * User: Contractor V&v Date: 4/30/10    Time: 11:28a
 * Updated in $/software/control processor/code/application
 * SCR #574 System Level objects should not be including Applicati
 * 
 * *****************  Version 14  *****************
 * User: Jeff Vahue   Date: 3/30/10    Time: 5:00p
 * Updated in $/software/control processor/code/application
 * SCR# 463 - correct WD headers and other functio headers, other minor
 * changes see Investigator note.
 * 
 * *****************  Version 13  *****************
 * User: Contractor2  Date: 3/02/10    Time: 11:37a
 * Updated in $/software/control processor/code/application
 * SCR# 472 - Fix file/Function headers, footer
 * 
 *
 ***************************************************************************/

