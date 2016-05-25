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
    $Revision: 33 $  $Date: 5/24/16 9:34a $

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
#define SYS_ETM_NAME_LEN    32

/******************************************************************************
                                 Package Typedefs                             *
******************************************************************************/
typedef struct
{
  BITARRAY128  record_triggers;
  BITARRAY128  on_ground_triggers;
  UINT32 auto_ul_per_s;
  TIME_SOURCE_ENUM time_source;
  UINT32 tx_test_msrdy_to;
  UINT32 tx_test_SIM_rdy_to;
  UINT32 tx_test_GSM_rdy_to;
  UINT32 tx_test_VPN_rdy_to;
  CHAR sysFilename[SYS_ETM_NAME_LEN];
  CHAR etmFilename[SYS_ETM_NAME_LEN];  
}FASTMGR_CONFIG;

#define FASTMGR_CONFIG_DEFAULT {0,0,0,0},{0,0,0,0},300,TIME_SOURCE_LOCAL,120,180,300,300,"SYS","ETM"

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
EXPORT USER_ENUM_TBL time_source_strs[];
#endif
/******************************************************************************
                             Package Exports Functions
******************************************************************************/
EXPORT void                 FAST_Init                ( void );
EXPORT TIME_SOURCE_ENUM     FAST_TimeSourceCfg       ( void );
EXPORT void                 FAST_GetSoftwareVersion  ( INT8* SwVerStr );
EXPORT void                 FAST_SignalUploadComplete( void );
EXPORT void                 FAST_TurnOffFASTControl  ( void );
EXPORT BOOLEAN              FAST_FSMRfGetState       (INT32 param);
EXPORT void                 FAST_FSMRfRun            (BOOLEAN Run,INT32 param);
EXPORT void                 FAST_FSMEndOfFlightRun   (BOOLEAN Run, INT32 Param);
EXPORT BOOLEAN              FAST_FSMRecordGetState   (INT32 param);
EXPORT void                 FAST_GetSYSName          (INT8* nameStr);
EXPORT void                 FAST_GetETMName          (INT8* nameStr);

#endif // FASTMGR_H
/*************************************************************************
 *  MODIFICATIONS
 *    $History: FASTMgr.h $
 * 
 * *****************  Version 33  *****************
 * User: John Omalley Date: 5/24/16    Time: 9:34a
 * Updated in $/software/control processor/code/application
 * SCR 1334 - Added configurable ETM and SYS filenames
 * 
 * *****************  Version 32  *****************
 * User: Contractor V&v Date: 11/03/14   Time: 5:24p
 * Updated in $/software/control processor/code/application
 * SCR #1092 - Forceupload recording-in-progress notification. Modfix-rec
 * flag change
 *
 * *****************  Version 31  *****************
 * User: Contractor V&v Date: 10/06/14   Time: 4:04p
 * Updated in $/software/control processor/code/application
 * SCR #1092 - Forceupload recording-in-progress notification.
 *
 * *****************  Version 30  *****************
 * User: Jim Mood     Date: 12/13/12   Time: 2:56p
 * Updated in $/software/control processor/code/application
 * SCR #1197
 *
 * *****************  Version 29  *****************
 * User: Jim Mood     Date: 11/09/12   Time: 6:16p
 * Updated in $/software/control processor/code/application
 * SCR 1131 Record busy status
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

