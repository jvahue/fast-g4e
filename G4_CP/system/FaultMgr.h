#ifndef FAULTMGR_H
#define FAULTMGR_H
/******************************************************************************
            Copyright (C) 2008-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:          FaultMgr.h

    Description:

  VERSION
    $Revision: 32 $  $Date: 12-11-09 4:41p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "systemlog.h"
#include "user.h"
#include "Dio.h"
/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/

/******************************************************************************
                                 Package Defines
******************************************************************************/
#define SYS_COND_OUTPUT_DISABLED DIO_MAX_OUTPUTS

#define FAULTMGR_CONFIG_DEFAULT DBGOFF,                   /* Debug Verbosity       */\
                                FLT_ANUNC_DIRECT,         /* DIRECT Mode           */\
                                SYS_COND_OUTPUT_DISABLED, /* Default DIO Out Pin   */\
                                0,                        /* NORMAL Action         */\
                                0,                        /* CAUTION Action        */\
                                0                         /* FAULT Action          */

/******************************************************************************
                                 Package Typedefs
******************************************************************************/
// Note: Updates to FLT_STATUS has dependency to Flt_UserEnumStatus[]
typedef enum {
  STA_NORMAL,
  STA_CAUTION,
  STA_FAULT,
  STA_MAX
} FLT_STATUS;

typedef enum
{
  DBGOFF = 0,
  NORMAL,
  VERBOSE
} FLT_DBG_LEVEL;

typedef enum {
  FLT_ANUNC_DIRECT,
  FLT_ANUNC_ACTION
} FLT_ANUNC_MODE;

typedef struct
{
  FLT_DBG_LEVEL DebugLevel;
  FLT_ANUNC_MODE Mode;
  DIO_OUTPUT    SysCondDioOutPin;
  UINT8         action[STA_MAX];                    /* Action to perform for FAULT/CAUTION */
} FAULTMGR_CONFIG;

#pragma pack(1)
// System Status Update Log structure
typedef struct
{
  SYS_APP_ID logID;            // Id of the CSC which issued the Sys Status
  FLT_STATUS statusReq;        // The status cmd issued in last SetStatus/ClrStatus
  FLT_STATUS sysStatus;        // The currently active System Status;
  FLT_STATUS prevStatus;       // The prior System Status. Could be same as
                               // StatusReq if StatusReq did not result in status change
  UINT32     statusNormalCnt;  // The current count of Normal  statuses(should be zero)
  UINT32     statusCautionCnt; // The current count of Caution statuses
  UINT32     statusFaultCnt;   // The current count of Fault   statuses
}INFO_SYS_STATUS_UPDATE_LOG;
#pragma pack()

typedef struct
{
   UINT8      nAction;
   INT8       nID;
   BOOLEAN    state;
   FLT_STATUS sysCond;
} FAULTMGR_ACTION;

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( FAULTMGR_BODY )
   #define EXPORT
#else
   #define EXPORT extern
#endif

/******************************************************************************
                             Package Exports Variables
******************************************************************************/
#if defined (FAULTMGR_BODY)
// Note: Updates to FLT_STATUS has dependency to Flt_UserEnumStatus[]
EXPORT USER_ENUM_TBL Flt_UserEnumStatus[] =
{ {"NORMAL",      STA_NORMAL},
  {"CAUTION",     STA_CAUTION},
  {"FAULT",       STA_FAULT},
  {NULL,0}
};
#else
EXPORT USER_ENUM_TBL Flt_UserEnumStatus[];
#endif
/******************************************************************************
                             Package Exports Functions
******************************************************************************/
EXPORT void Flt_Init(void);
EXPORT FLT_STATUS Flt_GetSystemStatus(void);

EXPORT void Flt_SetStatus(FLT_STATUS Status, SYS_APP_ID LogID, void *LogData,
                          INT32 LogDataSize);
EXPORT void Flt_ClrStatus(FLT_STATUS Status);

EXPORT void Flt_SetDebugVerbosity(FLT_DBG_LEVEL NewLevel);
EXPORT FLT_DBG_LEVEL Flt_GetDebugVerbosity( void);

EXPORT void Flt_InitDebugVerbosity( void);

EXPORT void Flt_PreInitFaultMgr(void);

EXPORT DIO_OUTPUT Flt_GetSysCondOutputPin(void);
EXPORT BOOLEAN Flt_InitFltBuf(void);
EXPORT void Flt_UpdateAction (FLT_STATUS sysCond);
FLT_ANUNC_MODE Flt_GetSysAnunciationMode( void );

#endif // FAULTMGR_H

/*************************************************************************
 *  MODIFICATIONS
 *    $History: FaultMgr.h $
 *
 * *****************  Version 32  *****************
 * User: John Omalley Date: 12-11-09   Time: 4:41p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review Updates
 * 
 * *****************  Version 31  *****************
 * User: John Omalley Date: 12-10-30   Time: 5:48p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Changed Actions to UINT8
 *
 * *****************  Version 30  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 29  *****************
 * User: John Omalley Date: 12-08-28   Time: 8:34a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Added ETM Fault Logic
 *
 * *****************  Version 28  *****************
 * User: John Omalley Date: 12-08-24   Time: 9:30a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - ETM Fault Action Logic
 *
 * *****************  Version 27  *****************
 * User: John Omalley Date: 12-08-16   Time: 4:15p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Fault Action Processing
 *
 * *****************  Version 26  *****************
 * User: Jim Mood     Date: 2/24/12    Time: 10:39a
 * Updated in $/software/control processor/code/system
 * SCR 1114 - Re labeled after v1.1.1 release
 *
 * *****************  Version 24  *****************
 * User: Contractor V&v Date: 12/14/11   Time: 6:50p
 * Updated in $/software/control processor/code/system
 * SCR #307 Add System Status to Fault / Event / etc logs
 *
 * *****************  Version 23  *****************
 * User: John Omalley Date: 9/09/10    Time: 2:12p
 * Updated in $/software/control processor/code/system
 * SCR 855 - Removed unused functions
 *
 * *****************  Version 22  *****************
 * User: Peter Lee    Date: 8/30/10    Time: 6:55p
 * Updated in $/software/control processor/code/system
 * SCR #806 Code Review Updates
 *
 * *****************  Version 21  *****************
 * User: Contractor V&v Date: 6/08/10    Time: 5:54p
 * Updated in $/software/control processor/code/system
 * SCR #614 fast.reset=really should initialize files
 *
 * *****************  Version 20  *****************
 * User: Contractor2  Date: 6/07/10    Time: 1:28p
 * Updated in $/software/control processor/code/system
 * SCR #485 Escape Sequence & Box Config
 *
 * *****************  Version 19  *****************
 * User: Jeff Vahue   Date: 4/26/10    Time: 1:27p
 * Updated in $/software/control processor/code/system
 * SCR #566 - disable interrupts in Put/Get operations on Log Q.  Set
 * default fault.cfg.verbosity to OFF
 *
 * *****************  Version 18  *****************
 * User: Contractor V&v Date: 3/29/10    Time: 6:17p
 * Updated in $/software/control processor/code/system
 * SCR #513 Configure fault status  LED
 *
 * *****************  Version 17  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:36p
 * Updated in $/software/control processor/code/system
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 *
 * *****************  Version 16  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/system
 * SCR# 483 - Function Names
 *
 * *****************  Version 15  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:57p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 14  *****************
 * User: Jeff Vahue   Date: 1/13/10    Time: 11:21a
 * Updated in $/software/control processor/code/system
 * SCR# 330
 *
 * *****************  Version 13  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:25p
 * Updated in $/software/control processor/code/system
 * SCR 386
 *
 * *****************  Version 12  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 3:11p
 * Updated in $/software/control processor/code/system
 * SCR# 326 cleanup and remerge
 *
 * *****************  Version 11  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 2:11p
 * Updated in $/software/control processor/code/system
 * SCR# 326
 *
 * *****************  Version 10  *****************
 * User: Peter Lee    Date: 12/21/09   Time: 6:19p
 * Updated in $/software/control processor/code/system
 * SCR #380 Implement Real Time Sys Condition Logic
 *
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 12/18/09   Time: 4:34p
 * Updated in $/software/control processor/code/system
 * SCR #368 Calling Flt_SetStatus() before Flt_Init().  Added additional
 * debug statements.
 *
 * *****************  Version 8  *****************
 * User: Jeff Vahue   Date: 12/17/09   Time: 12:12p
 * Updated in $/software/control processor/code/system
 * SCR# 364 - INT8 -> CHAR type
 *
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 12/02/09   Time: 2:17p
 * Updated in $/software/control processor/code/system
 * SCR #350 Misc Code Review Items
 *
 * *****************  Version 6  *****************
 * User: John Omalley Date: 10/19/09   Time: 3:28p
 * Updated in $/software/control processor/code/system
 * Removed the MAINTENANCE state from the FLT_STATUS Enum.
 *
 * *****************  Version 5  *****************
 * User: John Omalley Date: 10/01/09   Time: 4:27p
 * Updated in $/software/control processor/code/system
 * Updated the System Status for sensors
 *
 * *****************  Version 4  *****************
 * User: Jim Mood     Date: 1/26/09    Time: 4:39p
 * Updated in $/control processor/code/system
 * Changed StatusStr prototype to accept const class strings
 *
 * *****************  Version 3  *****************
 * User: Jim Mood     Date: 10/24/08   Time: 10:30a
 * Updated in $/control processor/code/system
 *
 * *****************  Version 2  *****************
 * User: Jim Mood     Date: 10/07/08   Time: 5:56p
 * Updated in $/control processor/code/system
 *
 * *****************  Version 1  *****************
 * User: Jim Mood     Date: 9/16/08    Time: 10:26a
 * Created in $/control processor/code/system
 *
 *
 *
 *
 ***************************************************************************/
