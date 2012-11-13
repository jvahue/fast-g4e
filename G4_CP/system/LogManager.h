#ifndef LOGMNG_H
#define LOGMNG_H
/******************************************************************************
            Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:          LogManager.h

    Description:   This file contains helper functions for reading,
                   writing and erasing logs to the data flash memory.

  VERSION
    $Revision: 54 $  $Date: 12-11-13 5:46p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "resultcodes.h"
#include "taskmanager.h"
#include "systemlog.h"
#include "user.h"
/******************************************************************************
                                 Package Defines
******************************************************************************/
#define ASSERT_ON_LOG_REQ_FAILED

#define LOG_EEPROM_CFG_DEFAULT   0, /*Start Offset*/\
                                 0  /*Size        */


// Queue SIZE Must be a power of 2 (2, 4, 8, 16, 32, 64, 128....)
#define LOG_QUEUE_SIZE          512   // Number of possible pending requests
#define SYSTEM_TABLE_MAX_SIZE   (LOG_QUEUE_SIZE + 4)
#define LOG_SIZE_CHECK_OK       0xFFFF
#define LOG_SIZE_BLANK          0xFFFF
#define LOG_CHKSUM_BLANK        0xFFFFFFFF
#define LOG_LOCAL_BUFFER_SIZE   0x4000    // 16K locations
#define LOG_NEXT                0xFFFFFFFF
#define LOG_SOURCE_DONT_CARE    0xFFFFFFFF
#define LOG_MAX_FAIL            2
#define LOG_BUF_MAX             0x10000
#define LOG_INDEX_NOT_SET       UINT32_MAX
#define LOG_SYSTEM_ETM_MAX_SIZE 1024
/******************************************************************************
                                 Package Typedefs
******************************************************************************/
typedef void   (*DO_CLEANUP ) ( void );

typedef enum
{
   ACS_1,
   ACS_2,
   ACS_3,
   ACS_4,
   ACS_5,
   ACS_6,
   ACS_7,
   ACS_8,
   ACS_DONT_CARE = 0xFFFFFFFF // Note the don't care in this enumeration
                              // must match the don't care in the SystemLog.h
                              // SYS_APP_ID enumeration and the Source
                              // #define LOG_SOURCE_DONT_CARE in LogManager.h
} LOG_ACS_FIELD;

typedef INT8 LOG_BUF[LOG_BUF_MAX];

typedef enum
{
   LOG_READ_SUCCESS,
   LOG_READ_BUSY,
   LOG_READ_FAIL
} LOG_READ_STATUS;

typedef enum
{
  LOG_BUSY,
  LOG_FOUND,
  LOG_NOT_FOUND
} LOG_FIND_STATUS;

typedef enum
{
   LOG_HDR_FOUND,
   LOG_HDR_FREE,
   LOG_HDR_NOT_FOUND
} LOG_HDR_STATUS;

typedef enum
{
  LOG_REQ_NONE,
  LOG_REQ_WRITE,
  LOG_REQ_ERASE
} LOG_REQ_TYPE;

typedef enum
{
  LOG_REQ_NULL,
  LOG_REQ_PENDING,
  LOG_REQ_INPROG,
  LOG_REQ_COMPLETE,
  LOG_REQ_FAILED
} LOG_REQ_STATUS;

typedef enum
{
   LOG_MNG_IDLE,
   LOG_MNG_PENDING,
   LOG_MNG_SERVICING
} LOG_MNG_STATE;

typedef enum
{
   LOG_MARK_NONE,
   LOG_MARK_IN_PROGRESS,
   LOG_MARK_COMPLETE,
   LOG_MARK_FAIL
} LOG_MARK_STATUS;

typedef enum
{
   LOG_QUEUE_OK         = 0,
   LOG_QUEUE_FULL       = 1,
   LOG_QUEUE_EMPTY      = 2
} LOG_QUEUE_STATUS;

typedef enum
{
   LOG_TYPE_DONT_CARE   = 0,
   LOG_TYPE_SYSTEM      = 0x1234,
   LOG_TYPE_DATA        = 0x5678,
   LOG_TYPE_ETM         = 0xE187,
   LOG_TYPE_NONE        = 0xffff     // indicates unused flash area
} LOG_TYPE;

typedef enum
{
   LOG_PRIORITY_1       = 1,
   LOG_PRIORITY_2       = 2,
   LOG_PRIORITY_3       = 3,
   LOG_PRIORITY_4       = 4,
   LOG_PRIORITY_DONT_CARE,
   LOG_PRIORITY_LOW     = LOG_PRIORITY_3, /*Enums for legacy System logs*/
   LOG_PRIORITY_HIGH    = LOG_PRIORITY_3  /*Enums for legacy System logs*/
} LOG_PRIORITY;

typedef enum
{
   LOG_DONT_CARE       = 0,
   LOG_DELETED         = 0xFF8008FF,
   LOG_NEW             = 0xFFF88FFF,
   LOG_NONE            = 0xFFFFFFFF
} LOG_STATE;

typedef enum
{
   LOG_ERASE_NONE,
   LOG_ERASE_NORMAL,
   LOG_ERASE_QUICK,
} LOG_ERASE_TYPE;

typedef enum
{
  LOG_ERASE_VERIFY_NONE,
  LOG_VERIFY_DELETED,     //Verify all logs are marked for deletion.
  LOG_ERASE_STATUS_WRITE, //Update the log erase status file to persist log-erase state.
  LOG_ERASE_MEMORY,
  LOG_ERASE_STATUS
} LOG_ERASE_STATE;

// The following enums are indexes into
// LOG_MNG_TASK_PARMS.LogWritePending[].
typedef enum
{
  LOG_REGISTER_END_FLIGHT,  // End-Flight-Log index
  // ... Add additional log types here
  LOG_REGISTER_END
} LOG_REGISTER_TYPE;


typedef union
{
  LOG_ACS_FIELD acs;
  SYS_APP_ID    id;
  UINT32        nNumber;
} LOG_SOURCE;

typedef struct
{
  UINT16    nSize;
  UINT16    nSizeInverse;
  LOG_STATE nState;
  UINT16    nType;
  UINT16    nPriority;
  UINT32    nSource;
  UINT32    nChecksum;
} LOG_HEADER;

typedef struct
{
   UINT32 nStartOffset;
   UINT32 *pFoundOffset;
   LOG_ERASE_TYPE type;
} LOG_ERASE_DATA;

typedef struct
{
   LOG_HEADER hdr;
   UINT32     *pData;
   UINT32     nSize;
} LOG_WRITE_DATA;

typedef struct
{
  LOG_REQ_TYPE    reqType;
  LOG_REQ_STATUS  *pStatus;
  union
  {
     LOG_ERASE_DATA     erase;
     LOG_WRITE_DATA     write;
  }request;
} LOG_REQUEST;

typedef struct
{
  UINT32 head;
  UINT32 tail;
  LOG_REQUEST buffer[LOG_QUEUE_SIZE];
} LOG_REQ_QUEUE;

typedef struct
{
  UINT32  nWrOffset;
  UINT32  nStartOffset;
  UINT32  nEndOffset;
  UINT32  nLogCount;
  UINT32  nDeleted;
  UINT32  nCorrupt;
  FLOAT32 fPercentUsage;
  BOOLEAN bEightyFiveExceeded;
} LOG_CONFIG;

typedef struct
{
  UINT32  nRdAccess;
  UINT32  nErAccess;
  UINT32  nDiscarded;
} LOG_ERROR_CNTS;

typedef struct
{
  LOG_ERROR_CNTS counts;
  BOOLEAN bReadAccessFail;
  BOOLEAN bWriteAccessFail;
  BOOLEAN bEraseAccessFail;
} LOG_ERROR_STATUS;

typedef struct
{
   BOOLEAN inProgress;
   BOOLEAN bChipErase;
   UINT32  savedStartOffset;
   UINT32  savedSize;
} LOG_EEPROM_CONFIG;

typedef struct
{
   LOG_TYPE         logType;
   SYS_APP_ID       source;
   LOG_PRIORITY     priority;
   LOG_REQ_STATUS   wrStatus;
   RESULT           result;
   SYS_LOG_PAYLOAD  payload;
} SYSTEM_LOG;

typedef struct
{
    LOG_MNG_STATE state;
    LOG_REQUEST   currentEntry;
    UINT32        faultCount;
    // Array of indexes into the the SystemTable which contains an outstanding
    // write pending/in-progress for the corresponding LOG_REGISTER_TYPE.
    // A value of LOG_INDEX_NOT_SET means the associated log type is not active.
    UINT32       logWritePending[LOG_REGISTER_END];

} LOG_MNG_TASK_PARMS;

typedef struct
{
    LOG_HEADER      hdrCriteria;
    UINT32          nStartOffset;
    UINT32          nEndOffset;
    UINT32          nRdOffset;
    LOG_MARK_STATUS *pStatus;
} LOG_STATE_TASK_PARMS;

typedef struct
{
    UINT32          nOffset;
    UINT32          nRdOffset;
    UINT32          nSize;
    LOG_ERASE_TYPE  type;
    LOG_ERASE_STATE state;
    UINT32          *pFound;
    BOOLEAN         bVerifyStarted;
    BOOLEAN         bUpdatingStatusFile;
    BOOLEAN         bDone;
    RESULT          result;
} LOG_CMD_ERASE_PARMS;

typedef struct
{
    LOG_REQ_STATUS  erStatus;
    UINT32          found;
} LOG_MON_ERASE_PARMS;

typedef struct
{
   TASK_INDEX taskID;
   BOOLEAN bDeleteVerified;
   LOG_MNG_TASK_PARMS **pTaskParams;
} LOG_BCKGRD_ERASE_PARMS;

// Log CBIT Health Status
typedef struct
{
  UINT32    nCorruptCnt;
} LOG_CBIT_HEALTH_COUNTS;

#pragma pack(1)
// Log Manager Fail Logs
typedef struct
{
   RESULT         result;
   UINT32         offset;
} LOG_READ_FAIL_LOG;

typedef struct
{
   RESULT         result;
   LOG_ERASE_DATA eraseData;
} LOG_ERASE_FAIL_LOG;
#pragma pack()

typedef struct
{
   BOOLEAN enable;
   UINT16  delay_S;
   UINT32  startTime_ms;
} LOG_PAUSE_WRITE;
/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( LOGMNG_BODY )
   #define EXPORT
#else
   #define EXPORT extern
#endif

/******************************************************************************
                             Package Exports Variables
******************************************************************************/
#if defined ( LOGMNG_BODY )
// Note: Updates to LOG_PRIORITY has dependency to LM_UserEnumPriority[]
EXPORT USER_ENUM_TBL lm_UserEnumPriority[] =
{ { "1",   LOG_PRIORITY_1      },
  { "2",   LOG_PRIORITY_2      },
  { "3",   LOG_PRIORITY_3      },
  { "4",   LOG_PRIORITY_4      },
  { NULL,     0                      }
};
#else
EXPORT USER_ENUM_TBL lm_UserEnumPriority[];
#endif

/******************************************************************************
                             Package Exports Functions
******************************************************************************/
EXPORT void            LogInitialize     ( void );
EXPORT void            LogPreInit        ( void );
EXPORT LOG_FIND_STATUS LogFindNextRecord ( LOG_STATE State, LOG_TYPE Type,
                                           LOG_SOURCE Source,
                                           LOG_PRIORITY Priority,
                                           UINT32 *pRdOffset,
                                           UINT32 EndOffset,
                                           UINT32 *pFoundOffset,
                                           UINT32 *pSize);
EXPORT LOG_READ_STATUS LogRead           ( UINT32 nOffset, LOG_BUF *pBuf );
EXPORT void            LogWrite          ( LOG_TYPE Type, LOG_SOURCE Source,
                                           LOG_PRIORITY Priority,
                                           void *pBuf, UINT16 nSize, UINT32 Checksum,
                                           LOG_REQ_STATUS *pWrStatus );
EXPORT void            LogErase          ( UINT32 *pFoundLogOffset,
                                           LOG_REQ_STATUS *pErStatus );
EXPORT void            LogMarkState      ( LOG_STATE State, LOG_TYPE Type,
                                           LOG_SOURCE Source, LOG_PRIORITY Priority,
                                           UINT32 StartOffset, UINT32 EndOffset,
                                           LOG_MARK_STATUS *pStatus );
EXPORT UINT32          LogWriteSystemEx  ( SYS_APP_ID logID, LOG_PRIORITY priority,
                                           void *pData, UINT16 nSize, const TIMESTAMP *pTs );
EXPORT void            LogWriteSystem    ( SYS_APP_ID logID, LOG_PRIORITY priority,
                                           void *pData, UINT16 nSize, const TIMESTAMP *pTs );
EXPORT void            LogWriteETM       ( SYS_APP_ID logID, LOG_PRIORITY priority,
                                           void *pData, UINT16 nSize, const TIMESTAMP *pTs );
EXPORT BOOLEAN         LogIsLogEmpty     ( void );
EXPORT BOOLEAN         LogIsEraseInProgress( void );
EXPORT void            LogPauseWrites ( BOOLEAN Enable, UINT16 Delay_S );
EXPORT BOOLEAN         LogWriteIsPaused ( void );
EXPORT LOG_CBIT_HEALTH_COUNTS LogGetCBITHealthStatus( void );
EXPORT LOG_CBIT_HEALTH_COUNTS LogCalcDiffCBITHealthStatus(LOG_CBIT_HEALTH_COUNTS PrevCount);
EXPORT LOG_CBIT_HEALTH_COUNTS LogAddPrevCBITHealthStatus(LOG_CBIT_HEALTH_COUNTS CurrCnt,
                                                         LOG_CBIT_HEALTH_COUNTS PrevCnt);

EXPORT void    LogRegisterIndex  ( LOG_REGISTER_TYPE regType, UINT32 SystemTableIndex);
EXPORT BOOLEAN LogIsWriteComplete( LOG_REGISTER_TYPE regType );

EXPORT const LOG_CONFIG* LogGetConfigPtr         ( void );
EXPORT void              LogRegisterEraseCleanup ( DO_CLEANUP Func );
EXPORT UINT32            LogGetLastAddress       ( void );
EXPORT LOG_QUEUE_STATUS  LogQueuePut             ( LOG_REQUEST Entry );
EXPORT UINT32            LogGetLogCount          ( void );
EXPORT void              LogETM_SetRecStateChangeEvt(INT32 tag,void (*func)(INT32,BOOLEAN));

#endif // LOGMNG_H

/*************************************************************************
 *  MODIFICATIONS
 *    $History: LogManager.h $
 *
 * *****************  Version 54  *****************
 * User: John Omalley Date: 12-11-13   Time: 5:46p
 * Updated in $/software/control processor/code/system
 * SCR 1197 - Code Review Updates
 *
 * *****************  Version 53  *****************
 * User: John Omalley Date: 12-11-12   Time: 9:48a
 * Updated in $/software/control processor/code/system
 * SCR 1105, 1107, 1131, 1154 - Code Review Updates
 *
 * *****************  Version 52  *****************
 * User: John Omalley Date: 12-11-08   Time: 3:03p
 * Updated in $/software/control processor/code/system
 * SCR 1131 - Added logic for ETM Log Write Busy
 *                    updated Code Review findings
 *
 * *****************  Version 51  *****************
 * User: John Omalley Date: 12-11-07   Time: 8:28a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review Updates
 *
 * *****************  Version 50  *****************
 * User: John Omalley Date: 12-11-06   Time: 11:19a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review Updates
 *
 * *****************  Version 49  *****************
 * User: John Omalley Date: 12-09-05   Time: 9:42a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Added ASSERT for ETM and System Logs larger than 1K
 *
 * *****************  Version 48  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 47  *****************
 * User: Jim Mood     Date: 7/19/12    Time: 11:07a
 * Updated in $/software/control processor/code/system
 * SCR 1107: Data Offload changes for 2.0.0
 *
 * *****************  Version 46  *****************
 * User: John Omalley Date: 12-05-24   Time: 9:38a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Added the ETM Write function
 *
 * *****************  Version 45  *****************
 * User: John Omalley Date: 4/24/12    Time: 12:01p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Cessna Caravan
 *
 * *****************  Version 44  *****************
 * User: Jim Mood     Date: 2/24/12    Time: 10:39a
 * Updated in $/software/control processor/code/system
 * SCR 1114 - Re labeled after v1.1.1 release
 *
 * *****************  Version 42  *****************
 * User: Contractor V&v Date: 12/14/11   Time: 6:50p
 * Updated in $/software/control processor/code/system
 * SCR #1105 End of Flight Log Race Condition
 *
 * *****************  Version 41  *****************
 * User: Contractor2  Date: 8/25/11    Time: 1:17p
 * Updated in $/software/control processor/code/system
 * SCR #1021 Error: Data Manager Buffers Full Fault Detected
 *
 * *****************  Version 40  *****************
 * User: Jeff Vahue   Date: 11/01/10   Time: 4:21p
 * Updated in $/software/control processor/code/system
 * SCR# 974 - remove LogFull Log
 *
 * *****************  Version 39  *****************
 * User: John Omalley Date: 10/21/10   Time: 1:46p
 * Updated in $/software/control processor/code/system
 * SCR 960 - Modified the checksum to load it across frames
 *
 * *****************  Version 38  *****************
 * User: Jim Mood     Date: 10/21/10   Time: 9:22a
 * Updated in $/software/control processor/code/system
 * SCR 959 Upload Manager code coverage changes
 *
 * *****************  Version 37  *****************
 * User: John Omalley Date: 10/07/10   Time: 11:38a
 * Updated in $/software/control processor/code/system
 * SCR 923 - Code Review Updates
 *
 * *****************  Version 36  *****************
 * User: John Omalley Date: 9/30/10    Time: 1:25p
 * Updated in $/software/control processor/code/system
 * SCR 894 - LogManager Code Coverage and Code Review Updates
 *
 * *****************  Version 35  *****************
 * User: John Omalley Date: 9/09/10    Time: 9:01a
 * Updated in $/software/control processor/code/system
 * SCR 790 - Added function to return the last used address
 *
 * *****************  Version 34  *****************
 * User: John Omalley Date: 7/30/10    Time: 5:03p
 * Updated in $/software/control processor/code/system
 * SCR 699 - Changed the LogInitialize return value to void.
 *
 * *****************  Version 33  *****************
 * User: John Omalley Date: 7/30/10    Time: 8:51a
 * Updated in $/software/control processor/code/system
 * SCR 678 - Remove files in the FVT that are marked not deleted
 *
 * *****************  Version 32  *****************
 * User: Jim Mood     Date: 7/28/10    Time: 8:02p
 * Updated in $/software/control processor/code/system
 * SCR 639 ModFix: Added function prototypes for the LogPauseWrite
 * commands
 *
 * *****************  Version 31  *****************
 * User: John Omalley Date: 7/27/10    Time: 4:42p
 * Updated in $/software/control processor/code/system
 * SCR 730 - Removed Log Write Failed Log
 *
 * *****************  Version 30  *****************
 * User: John Omalley Date: 7/26/10    Time: 10:37a
 * Updated in $/software/control processor/code/system
 * SCR 306 - ASSERT on three write attempts failing
 *
 * *****************  Version 29  *****************
 * User: John Omalley Date: 7/23/10    Time: 10:15a
 * Updated in $/software/control processor/code/system
 * SCR 306 / SCR 639 - Updated the error paths, Added pause feature and
 * fixed the Queue ASSERT
 *
 * *****************  Version 28  *****************
 * User: Contractor V&v Date: 7/21/10    Time: 7:18p
 * Updated in $/software/control processor/code/system
 * SCR #260 Implement BOX status command
 *
 * *****************  Version 27  *****************
 * User: Contractor V&v Date: 6/30/10    Time: 3:19p
 * Updated in $/software/control processor/code/system
 * SCR #531 Log Erase and NV_Write Timing
 *
 * *****************  Version 26  *****************
 * User: Contractor2  Date: 6/30/10    Time: 11:59a
 * Updated in $/software/control processor/code/system
 * SCR #150 Implementation: SRS-3662 - Add Data Flash corruption CBIT
 * counts.
 * Corrected variable name.
 *
 * *****************  Version 25  *****************
 * User: Contractor2  Date: 6/30/10    Time: 11:04a
 * Updated in $/software/control processor/code/system
 * SCR #150 Implementation: SRS-3662 - Add Data Flash corruption CBIT
 * counts.
 *
 * *****************  Version 24  *****************
 * User: Contractor V&v Date: 5/12/10    Time: 3:48p
 * Updated in $/software/control processor/code/system
 * SCR #351 fast.reset=really housekeeping
 *
 * *****************  Version 23  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:55p
 * Updated in $/software/control processor/code/system
 * SCR #587 Change TmTaskCreate to return void
 *
 * *****************  Version 22  *****************
 * User: Contractor2  Date: 5/06/10    Time: 2:08p
 * Updated in $/software/control processor/code/system
 * SCR #579 Change LogWriteSys to return void
 *
 * *****************  Version 21  *****************
 * User: Contractor V&v Date: 4/22/10    Time: 6:41p
 * Updated in $/software/control processor/code/system
 * SCR #306 Log Manager Errors
 *
 * *****************  Version 20  *****************
 * User: Contractor V&v Date: 3/29/10    Time: 6:18p
 * Updated in $/software/control processor/code/system
 * SCR #140 Move Log erase status to own file
 *
 * *****************  Version 19  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 18  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 5:07p
 * Updated in $/software/control processor/code/system
 * SCR# 397
 *
 * *****************  Version 17  *****************
 * User: John Omalley Date: 8/06/09    Time: 3:56p
 * Updated in $/software/control processor/code/system
 * SCR 23
 *    Added 85% full and full logic.
 * SCR 179
 *    Created a wrapper to protect against memcpy with size 0
 * SCR 231
 *    Added logic to return complete to tasks using the log manager when
 * the memory becomes full.
 *    Fixed memory advancement past end of memory.
 *
 * *****************  Version 16  *****************
 * User: Jim Mood     Date: 10/23/08   Time: 3:05p
 * Updated in $/control processor/code/system
 * Added a "pre-initialize" function that sets up the log write queue and
 * the system log buffer.  This is used during initilization to allow PBIT
 * log writes
 *
 * *****************  Version 15  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 3:15p
 * Updated in $/control processor/code/system
 * Update call LogWriteSystem() to include ts param
 *
 * *****************  Version 14  *****************
 * User: Jim Mood     Date: 10/07/08   Time: 2:54p
 * Updated in $/control processor/code/system
 * Changed LogWriteSystem prototype 'pData' parameter type from UINT32* to
 * void*
 *
 * *****************  Version 13  *****************
 * User: Jim Mood     Date: 8/26/08    Time: 11:29a
 * Updated in $/control processor/code/system
 * Moved LogWriteSystem function into this module from SystemLog.c
 *
 * *****************  Version 12  *****************
 * User: John Omalley Date: 6/18/08    Time: 2:47p
 * Updated in $/control processor/code/system
 * SCR-0044 Added System Log Processing
 *
 * *****************  Version 11  *****************
 * User: John Omalley Date: 6/10/08    Time: 11:21a
 * Updated in $/control processor/code/system
 * SCR-0040 - LogFindNext Updates for Read Pointer
 *
 *
 ***************************************************************************/
