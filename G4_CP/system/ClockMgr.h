#ifndef SYS_CLOCKMGR_H
#define SYS_CLOCKMGR_H

/******************************************************************************
            Copyright (C) 2007-2010 Pratt & Whitney Engine Services, Inc. 
                      Altair Engine Diagnostic Solutions
               All Rights Reserved. Proprietary and Confidential.

  File:         ClockMgr.h
 
  Description: Provides real-time and wall clock resources for the
               system. 
 
  VERSION
      $Revision: 19 $  $Date: 4/28/10 5:56p $ 
 
******************************************************************************/


/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    


/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "ResultCodes.h"

/******************************************************************************
                                 Package Defines
******************************************************************************/
//Time error thresholds for performing the CBIT
//Two error levels are defined, a smaller error that must occur multiple times to fail
//that only has to occur once to fail the CBIT
#define CM_CBIT_ERR_THRESHOLD_1   1 //Time error that must occur multiple times to fail CBIT
#define CM_CBIT_ERR_THRESHOLD_2   6 //Time error that must occur once to fail CBIT          

#define CM_CBIT_SEQ_ERR_THRESHOLD 2 //Number of time the clock must be off by more than
                                    //CM_CBIT_ERR_THRESHOLD_1 to signal a fault

//Interval, in seconds, to copy power on time to NVRAM
#define CM_SYS_ON_TIME_STORE_INTERVAL_SEC 10

// Macro for generating Clock delay in seconds 
#define CM_DELAY_IN_SECS(x)  (MILLISECONDS_PER_SECOND*(x))
#define CM_DELAY_IN_MSEC(x)  (x)

#define CM_BASE_YEAR 1997
#define CM_BASE_YEAR_MAX 2082

/******************************************************************************
                               Package Typedefs
******************************************************************************/
#pragma pack(1)
typedef struct {
  RESULT      Result;
  TIMESTRUCT  TimeRead;
}CM_TIME_INVALID_LOG;

typedef struct {
  TIMESTRUCT  RTC;
  TIMESTRUCT  System;
  UINT16      MaxAllowedDriftSecs;
}CM_RTC_SYS_TIME_DRIFT_LOG;

typedef struct {  
  TIMESTRUCT  MS;
  TIMESTRUCT  System;
  UINT16      MaxAllowedDriftSecs;
}CM_MS_SYS_TIME_DRIFT_LOG;

typedef struct {
  TIMESTRUCT currTime;
  TIMESTRUCT newTime; 
} CM_CLOCKUPDATE_LOG;

typedef struct {
  RESULT  result; 
} CM_RTC_DRV_PBIT_LOG; 
#pragma pack()

typedef enum {
  TIME_SOURCE_LOCAL,    // Time source will be FAST RTC
  TIME_SOURCE_MS,       // Time source will be mssim 
  TIME_SOURCE_REMOTE,   // Time source will be Remote Time Server
  TIME_SOURCE_MAX
} TIME_SOURCE_ENUM;


/******************************************************************************
                               Package Exports
******************************************************************************/
#undef EXPORT

#if defined( SYS_CLOCKMGR_BODY )
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
EXPORT void CM_Init(void);
EXPORT void CM_PreInit(void);
EXPORT void CM_SynchRTCtoWallClock(void);
EXPORT void CM_GetRTCClock(TIMESTRUCT* pTime);
EXPORT void CM_SetRTCClock(TIMESTRUCT* pTime, BOOLEAN bRecordLog);
EXPORT void CM_GetSystemClock(TIMESTRUCT* pTime);
EXPORT void CM_SetSystemClock(TIMESTRUCT* pTime, BOOLEAN bRecordLog);
EXPORT void CM_GetTimeAsTimestamp (TIMESTAMP* TimeStamp);
EXPORT UINT32 CM_GetTickCount(void);
EXPORT void CM_RTCTick(void);
EXPORT void CM_ConvertTimeStamptoTimeStruct(const TIMESTAMP* Tp,TIMESTRUCT* Ts);
// EXPORT void CM_ConvertTimeStructToTimeStamp(const TIMESTRUCT* Ts, TIMESTAMP* Tp); 

EXPORT UINT32    CM_GetSecSinceBaseYr ( TIMESTAMP *ts, TIMESTRUCT *time_s ); 
EXPORT TIMESTAMP CM_ConvertSecToTimeStamp( UINT32 sec );
EXPORT void      CM_CompareSystemToMsClockDrift(void);

#endif // SYS_CLOCKMGR_H


/*************************************************************************
 *  MODIFICATIONS
 *    $History: ClockMgr.h $
 * 
 * *****************  Version 19  *****************
 * User: Contractor V&v Date: 4/28/10    Time: 5:56p
 * Updated in $/software/control processor/code/system
 * SCR #559 Check clock diff CP-MS on startup
 * 
 * *****************  Version 18  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:57p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 17  *****************
 * User: Peter Lee    Date: 1/28/10    Time: 9:20a
 * Updated in $/software/control processor/code/system
 * SCR #427 ClockMgr.c Issues and SRS-3090
 * 
 * *****************  Version 16  *****************
 * User: Peter Lee    Date: 1/27/10    Time: 4:31p
 * Updated in $/software/control processor/code/system
 * SCR #427 Misc Clock Updates 
 * 
 * *****************  Version 15  *****************
 * User: Jeff Vahue   Date: 1/23/10    Time: 3:13p
 * Updated in $/software/control processor/code/system
 * SCR# 397 - CBIT scheduling moved to TM no need for this #define
 * 
 * *****************  Version 14  *****************
 * User: Peter Lee    Date: 12/01/09   Time: 5:15p
 * Updated in $/software/control processor/code/system
 * SCR #347 Time Sync Requirements
 * 
 * *****************  Version 13  *****************
 * User: Peter Lee    Date: 11/23/09   Time: 5:57p
 * Updated in $/software/control processor/code/system
 * SCR #347 Time Sync Requirements
 * 
 * *****************  Version 12  *****************
 * User: Peter Lee    Date: 11/19/09   Time: 10:54a
 * Updated in $/software/control processor/code/system
 * SCR #315 Fix bug with maintaining SEU counts across power cycle
 * 
 * *****************  Version 11  *****************
 * User: Peter Lee    Date: 11/02/09   Time: 2:08p
 * Updated in $/software/control processor/code/system
 * Misc non-code update to support WIN32 and spelling.
 * 
 * *****************  Version 10  *****************
 * User: Peter Lee    Date: 9/28/09    Time: 4:16p
 * Updated in $/software/control processor/code/system
 * SCR 275 Support routines for Power On Usage Requirements
 * 
 * *****************  Version 9  *****************
 * User: Jim Mood     Date: 4/20/09    Time: 9:31a
 * Updated in $/control processor/code/system
 * SCR #161, #51 and added required logs
 * 
 * *****************  Version 8  *****************
 * User: Jim Mood     Date: 10/23/08   Time: 3:07p
 * Updated in $/control processor/code/system
 * Added clock pre-init that forces clock manager to server "get" time
 * requests from the RTC chip.  This is useful for maintaining time during
 * system startup, when the interrupt driven clock is not yet running
 * 
 * *****************  Version 7  *****************
 * User: Jim Mood     Date: 10/16/08   Time: 5:46p
 * Updated in $/control processor/code/system
 * Added ConvertTimeStampToTimeStruct function and updated the SDD to
 * refelect the new function.
 * 
 * *****************  Version 6  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 2:46p
 * Updated in $/control processor/code/system
 * SCR #87 Function Prototype
 * 
 * 
 *
 ***************************************************************************/
 
