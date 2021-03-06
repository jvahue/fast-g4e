#ifndef DATA_MNG_H
#define DATA_MNG_H
/******************************************************************************
            Copyright (C) 2008-2015 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    ECCN:        9E991

    File:        DataManager.h

    Description: The data manager contains the functions used to record
                 data from the various interfaces.

    VERSION
      $Revision: 46 $  $Date: 3/09/15 10:57a $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_basic.h"
#include "FaultMgr.h"
#include "ACS_Interface.h"

/******************************************************************************
                                 Package Defines
******************************************************************************/
#define DATA_MGR_MAX_CHANNELS   8
#define MAX_BUFF_SIZE           32768 // Size = Maximum UART 115,200bps
                                      // Size = 11,520bytes/second / 2bytes/parameter
                                      // Size = 5,760 parameters/second
                                      // Size = 5,760 records/second * 5bytes/record
                                      // Size = 28,800 bytes = 32K
                                      // The user could still possibly configure
                                      // the system so that this will overflow.
                                      // Because the buffers are static this will
                                      // result in 640KBytes of RAM being
                                      // allocated.
                                      // RAM Allocation = 10Chan * 2 buff/chan *
                                      //                  32768 bytes/chan
                                      // RAM Allocation = 655360
                                      // This is 2% of the available 32MB
// NOTE: There may be a limitation in the system when moving large buffers of data. Tests
//       on the production FAST hardware have shown that memcpy takes 230nSeconds per byte.
//       This means that if 32K buffer is passed to the log manager to write to Data Flash
//       we will incur a penalty of 7.5 milliseconds. This copy could cause DT overruns.
#define MAX_DOWNLOAD_CHUNK     ( MAX_BUFF_SIZE / 2 )

// Packet Size is the entire packet size minus the data area + written bytes
#define DM_PACKET_SIZE(x) ((sizeof(DM_PACKET)-MAX_BUFF_SIZE)+(x))

// Number of Data Manager Packet Buffers - Accessed as a circular buffer.
#define MAX_DM_BUFFERS  60

#define MAX_LOG_STR_LG  16
#define MAX_LOG_STR_SM  8

#define HDR_VERSION   3

/******************************************************************************
                                 Package Typedefs
******************************************************************************/
typedef UINT16 (*GET_DATA) ( void *ptr, UINT32 port, UINT16 size );
typedef UINT16 (*GET_SNAP) ( void *ptr, UINT32 port, UINT16 size, BOOLEAN bStartSnap );
typedef void   (*DO_SYNC ) ( UINT32 port );
typedef UINT16 (*GET_HDR ) ( void *ptr, UINT32 port, UINT16 size );

typedef void (*DATATASK)( void *pParam );

typedef enum
{
   DM_RECORD_IDLE,
   DM_RECORD_IN_PROGRESS
}DATA_MNG_RECORD;

typedef enum
{
   DM_BUF_EMPTY,
   DM_BUF_BUILD,
   DM_BUF_STORE
}DATA_MNG_BUF_STATUS;

typedef enum
{
   DM_START_SNAP,
   DM_STD_PACKET,
   DM_END_SNAP,
   DM_DOWNLOADED_RECORD,
   DM_DOWNLOADED_RECORD_CONTINUED
} DM_PACKET_TYPE;

typedef struct
{
   UINT32    nNumber;
   UINT32    nSize;
   UINT32    nStart_ms;
   UINT32    nDuration_ms;
   TIMESTAMP requestTime;
   TIMESTAMP foundTime;
} DATA_MNG_DOWNLOAD_RECORD_STATS;

#pragma pack(1)
typedef struct
{
   CHAR   model[MAX_ACS_NAME];
   CHAR   id[MAX_ACS_NAME];
   UINT32 nChannel;
   CHAR   recordStatus[MAX_LOG_STR_LG];
   CHAR   currentBuffer[MAX_LOG_STR_SM];
   CHAR   bufferStatus[MAX_LOG_STR_SM];
   CHAR   writeStatus[MAX_LOG_STR_LG];
} DM_FAIL_DATA;

typedef struct
{
   UINT32                         nChannel;
   UINT32                         nRcvd;
   UINT32                         nTotalBytes;
   TIMESTAMP                      startTime;
   TIMESTAMP                      endTime;
   UINT32                         nStart_ms;
   UINT32                         nDuration_ms;
   DATA_MNG_DOWNLOAD_RECORD_STATS largestRecord;
   DATA_MNG_DOWNLOAD_RECORD_STATS smallestRecord;
   DATA_MNG_DOWNLOAD_RECORD_STATS longestRequest;
   DATA_MNG_DOWNLOAD_RECORD_STATS shortestRequest;
} DM_DOWNLOAD_STATISTICS;

typedef struct
{
   UINT32        nChannel;
   CHAR          model[MAX_ACS_NAME];
   CHAR          id[MAX_ACS_NAME];
   ACS_PORT_TYPE portType;
   UINT8         portIndex;
} DM_DOWNLOAD_START_LOG;
#pragma pack()

//A type for an array of the maximum number of ACSs
//Used for storing ACS configurations in the configuration manager
//typedef ACS_CONFIG ACS_CONFIGS[MAX_ACS_CONFIG];
typedef struct
{
   UINT32         checksum;
   TIMESTAMP      timeStamp;
   UINT16         channel;
   UINT16         size;
   UINT16         packetType;
   UINT8          data[MAX_BUFF_SIZE];
} DM_PACKET;

typedef struct
{
   DM_PACKET           packet;
   DATA_MNG_BUF_STATUS status;
   UINT16              index;
   TIMESTAMP           packetTs;
   UINT32              tryTime_mS;
   LOG_REQ_STATUS      wrStatus;
   DL_WRITE_STATUS     *pDL_Status;
} DATA_MNG_BUFFER;

typedef struct
{
   BOOLEAN                bDownloading;
   BOOLEAN                bDownloadStop;
   DL_STATE               state;
   UINT8                  *pDataSrc;
   UINT32                 nBytes;
   DM_DOWNLOAD_STATISTICS statistics;
} DATA_MNG_DOWNLOAD_STATUS;

typedef struct
{
   ACS_CONFIG               acs_Config;
   DATA_MNG_RECORD          recordStatus;
   UINT32                   nStartTime_mS;
   UINT32                   nCurrTime_mS;
   UINT32                   nNextTime_mS;
   BOOLEAN                  bBufferOverflow;
   UINT16                   nCurrentBuffer;
   DATA_MNG_BUFFER          msgBuf[MAX_DM_BUFFERS];
   DATA_MNG_DOWNLOAD_STATUS dl;
}DATA_MNG_INFO;

typedef struct
{
   UINT16          nChannel;
   GET_DATA        pGetData;
   GET_SNAP        pGetSnapShot;
   DO_SYNC         pDoSync;
   GET_HDR         pGetDataHdr;
} DATA_MNG_TASK_PARMS;

typedef struct
{
   UINT32        version;
   ACS_PORT_TYPE type;
} DATA_MNG_HDR;

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( DATA_MNG_BODY )
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
EXPORT void       DataMgrIntialize      ( void );
EXPORT ACS_CONFIG DataMgrGetACSDataSet  ( LOG_ACS_FIELD ACS );
EXPORT UINT16     DataMgrGetHeader      ( INT8 *pDest, LOG_ACS_FIELD ACS,
                                          UINT16 nMaxByteSize );
EXPORT BOOLEAN    DataMgrRecordFinished ( void );
EXPORT void       DataMgrRecord         ( BOOLEAN bEnable );
EXPORT BOOLEAN    DataMgrStartDownload  ( void );
EXPORT void       DataMgrStopDownload   ( void );
EXPORT BOOLEAN    DataMgrDownloadingACS ( void );
EXPORT BOOLEAN    DataMgrDownloadGetState ( INT32 param );
EXPORT void       DataMgrRecRun         ( BOOLEAN Run, INT32 param );
EXPORT void       DataMgrDownloadRun    ( BOOLEAN Run, INT32 param );
EXPORT void       DataMgrSetRecStateChangeEvt(INT32 tag,void (*func)(INT32,BOOLEAN));

#endif // DATA_MNG_H
/*************************************************************************
 *  MODIFICATIONS
 *    $History: DataManager.h $
 * 
 * *****************  Version 46  *****************
 * User: John Omalley Date: 3/09/15    Time: 10:57a
 * Updated in $/software/control processor/code/system
 * SCR 1255 - Updated Port Index to an enumeration to handle 0,1,2,3,10
 * 
 * *****************  Version 45  *****************
 * User: John Omalley Date: 1/12/15    Time: 1:21p
 * Updated in $/software/control processor/code/system
 * SCR 1146 - Code Review Update
 * 
 * *****************  Version 44  *****************
 * User: John Omalley Date: 11/17/14   Time: 2:09p
 * Updated in $/software/control processor/code/system
 * SCR 1146 - Updated the Binary Header Version for new or modified fields
 * 
 * *****************  Version 43  *****************
 * User: John Omalley Date: 12-11-28   Time: 4:50p
 * Updated in $/software/control processor/code/system
 * SCR 1197 - Code Review Updates
 * 
 * *****************  Version 42  *****************
 * User: John Omalley Date: 12-11-13   Time: 11:10a
 * Updated in $/software/control processor/code/system
 * SCR 1142 - Code Review Formatting Issues
 * 
 * *****************  Version 41  *****************
 * User: John Omalley Date: 12-11-12   Time: 4:46p
 * Updated in $/software/control processor/code/system
 * SCR 1142 - Formatting Error
 * 
 * *****************  Version 40  *****************
 * User: Jim Mood     Date: 11/09/12   Time: 6:37p
 * Updated in $/software/control processor/code/system
 * SCR 1131 Record busy status
 *
 * *****************  Version 39  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 38  *****************
 * User: Jim Mood     Date: 7/19/12    Time: 11:07a
 * Updated in $/software/control processor/code/system
 * SCR 1107: Data Offload changes for 2.0.0
 *
 * *****************  Version 37  *****************
 * User: John Omalley Date: 10/26/11   Time: 6:44p
 * Updated in $/software/control processor/code/system
 * SCR 1097 - Dead Code Update
 *
 * *****************  Version 36  *****************
 * User: John Omalley Date: 9/12/11    Time: 2:23p
 * Updated in $/software/control processor/code/system
 * SCR 1073
 *
 * *****************  Version 35  *****************
 * User: John Omalley Date: 9/12/11    Time: 2:14p
 * Updated in $/software/control processor/code/system
 * SCR 1073 - Made Download Stop Function Global
 *
 * *****************  Version 34  *****************
 * User: Contractor2  Date: 8/15/11    Time: 1:47p
 * Updated in $/software/control processor/code/system
 * SCR #1021 Error: Data Manager Buffers Full Fault Detected
 *
 * *****************  Version 33  *****************
 * User: Jim Mood     Date: 7/20/11    Time: 10:57a
 * Updated in $/software/control processor/code/system
 * SCR 575: GSM Enable when engine status is lost.  (Part of changes for
 * the Fast State Machine)
 *
 * *****************  Version 32  *****************
 * User: John Omalley Date: 6/21/11    Time: 10:33a
 * Updated in $/software/control processor/code/system
 * SCR 1029 - Added User command to stop download per the Preliminary
 * Software Design Review.
 *
 * *****************  Version 31  *****************
 * User: John Omalley Date: 4/11/11    Time: 10:07a
 * Updated in $/software/control processor/code/system
 * SCR 1029 - Added EMU Download logic to the Data Manager
 *
 * *****************  Version 30  *****************
 * User: John Omalley Date: 11/05/10   Time: 11:16a
 * Updated in $/software/control processor/code/system
 * SCR 985 - Data Manager Code Coverage Updates
 *
 * *****************  Version 29  *****************
 * User: Contractor2  Date: 10/12/10   Time: 3:44p
 * Updated in $/software/control processor/code/system
 * SCR #931 Code Review: DataManager
 *
 * *****************  Version 28  *****************
 * User: Contractor2  Date: 10/07/10   Time: 11:48a
 * Updated in $/software/control processor/code/system
 * SCR #916 Code Review Updates
 *
 * *****************  Version 27  *****************
 * User: John Omalley Date: 8/03/10    Time: 2:15p
 * Updated in $/software/control processor/code/system
 * SCR 769 - Updated Buffer Sizes and fixed ARINC429 FIFO Index during
 * initialization.
 *
 * *****************  Version 26  *****************
 * User: John Omalley Date: 6/08/10    Time: 12:14p
 * Updated in $/software/control processor/code/system
 * SCR 627 - Updated for LJ60
 *
 * *****************  Version 25  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:54p
 * Updated in $/software/control processor/code/system
 * SCR #587 Change TmTaskCreate to return void
 *
 * *****************  Version 24  *****************
 * User: Jeff Vahue   Date: 4/07/10    Time: 11:47a
 * Updated in $/software/control processor/code/system
 * SCR #535 - reduce packet type size from 4 bytes to two in Data Logs
 *
 * *****************  Version 23  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:57p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 22  *****************
 * User: Jeff Vahue   Date: 2/19/10    Time: 12:58p
 * Updated in $/software/control processor/code/system
 * SCR# 455 - remove LINT issues
 *
 * *****************  Version 21  *****************
 * User: Jeff Vahue   Date: 2/18/10    Time: 5:18p
 * Updated in $/software/control processor/code/system
 * SCR #454 - correct size error when recording a buffer overflow
 *
 * *****************  Version 20  *****************
 * User: Jeff Vahue   Date: 2/17/10    Time: 1:24p
 * Updated in $/software/control processor/code/system
 * SCR# 454 - DM buffer overflow mods
 *
 * *****************  Version 19  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 5:06p
 * Updated in $/software/control processor/code/system
 * SCR# 397
 *
 * *****************  Version 18  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 3:38p
 * Updated in $/software/control processor/code/system
 * SCR# 326 cleanup
 *
 * *****************  Version 17  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 2:11p
 * Updated in $/software/control processor/code/system
 * SCR# 326
 *
 * *****************  Version 16  *****************
 * User: Peter Lee    Date: 12/21/09   Time: 6:19p
 * Updated in $/software/control processor/code/system
 * SCR #380 Implement Real Time Sys Condition Logic
 *
 * *****************  Version 15  *****************
 * User: John Omalley Date: 10/19/09   Time: 1:35p
 * Updated in $/software/control processor/code/system
 * SCR 283
 *  - Added logic to record a record a fail log if the buffers are full
 * when data recording is started.
 * - Updated the logic to record an error only once when the full
 * condition is detected.
 *
 * *****************  Version 14  *****************
 * User: Peter Lee    Date: 10/01/09   Time: 5:00p
 * Updated in $/software/control processor/code/system
 * SCR #283 Misc Code Review Updates
 *
 * *****************  Version 13  *****************
 * User: John Omalley Date: 8/06/09    Time: 3:51p
 * Updated in $/software/control processor/code/system
 * SCR 221
 *    Resized buffers so the QAR Snapshot wasn't truncated.
 *    Updated Calls to the interface reads to pass and return bytes rather
 * than words.
 * SCR 153
 *    Added a break out of the Data Manager loop when the buffers overflow
 * because of a flash backup.
 *    Fixed the logic so that a buffer overflow will end a packet and
 * start a new one on an overflow. (Flash is working)
 * SCR 190
 *    Rescheduled the DataManager tasks to eliminate DT Task overruns.
 *    Moved the buffer swap check.
 *
 * *****************  Version 12  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 2:53p
 * Updated in $/control processor/code/system
 * SCR #87 Function Prototype
 *
 *
 ***************************************************************************/



