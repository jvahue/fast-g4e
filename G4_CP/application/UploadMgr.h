#ifndef UPLOADMGR_H
#define UPLOADMGR_H

/******************************************************************************
            Copyright (C) 2007-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:         UploadMgr.h       
    
    Description: Package system logs and application (flight data)logs
    
    VERSION
    $Revision: 43 $  $Date: 8/28/12 12:43p $    
    
******************************************************************************/

//#define FILE_HEADER_FMT_G4   1    // G4 File header fmt SCR #132 
#define FILE_HEADER_FMT_FAST        // FAST File header format (currently same
                                    // as G4, but this may change)
#ifdef FILE_HEADER_FMT_G4
#define FILE_HEADER_FMT_TYPE_AND_VERSION
#endif
#ifdef FILE_HEADER_FMT_FAST
#define FILE_HEADER_FMT_TYPE_AND_VERSION
#endif
/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "MSInterface.h"

/******************************************************************************
                                 Package Defines                              *
******************************************************************************/
#define UPLOADMGR_FILE_DATA_QUEUE_SIZE 65536

//Micro-server command consecutive command failure tolarences
#define UPLOADMGR_START_LOG_FAIL_MAX      3
#define UPLOADMGR_LOG_DATA_FAIL_MAX       3
#define UPLOADMGR_END_LOG_FAIL_MAX        3

//File transfer fail tolerance.
#define UPLOADMGR_FILE_UL_FAIL_MAX        3

//Verification table max filename size
#define VFY_TBL_FN_SIZE                   64

//Verification table entries
#define UPLOADMGR_VFY_TBL_MAX_ROWS        200


#define UPLOADMGR_CHECK_FILE_DELAY_MS     1000  //Interval to check file location after upload
#define UPLOADMGR_CHECK_FILE_CNT          10    //Number of times to check a file is in the
                                                //verified folder before giving up.
#define UPLOADMGR_READ_LOGS_PER_MIF       100   //Maximum number of logs to try and read
                                                //on each from while building the file header.

//Delay after uploading a file to wait for MSSIM to send a Verify File cmd.
//If upload manager does not recieve the command within this timeframe, it
//will move on to the next file.
#define UPLOADMGR_MS_VFY_TIMEOUT          10000

#define BINARY_DATA_BUF_LEN               32767
#define UPLD_HDR_STR_LEN16                16
#define UPLD_HDR_STR_LEN32                32
#define UPLD_HDR_STR_LEN64                64
/******************************************************************************
                                 Package Typedefs                             *
******************************************************************************/
#define FILE_HEADER_VER   1   // Version identifies changes to the UPLOADMGR_FILE_HEADER
                              //    or data log format.  Stored in FileVersion parameter.  
#pragma pack(1)
typedef struct
{
  UINT32  CheckSum;                     // check sum of this entire data block including
                                        // header. this is long word
                                        // will not be cacluated
                                        // this number is included in the calculation
#ifdef FILE_HEADER_FMT_TYPE_AND_VERSION 
  CHAR    FileType[UPLD_HDR_STR_LEN16]; // ASCII Identifier.  G4, FAST, etc.. 
  UINT32  FileVersion;                  // File Header and Binary Log Data version ID.
                                        //   If the binary header changes or the format of
                                        //   any binary log data changes then this file ver
                                        //   id will be updated. 
#endif

  UINT16  ConfigVersion;                  // version ID
  INT8    Id[UPLD_HDR_STR_LEN64];         // installation identification
  INT8    SerialNumber[UPLD_HDR_STR_LEN16]; // Box Serial Number
  CHAR    MfgDate[UPLD_HDR_STR_LEN16];    // Date of Manufacture
  CHAR    MfgRev[UPLD_HDR_STR_LEN16];     // Revision of the Box
  CHAR    SwVersion[UPLD_HDR_STR_LEN32];  // Software Version in Box
  UINT32  TimeSource;                     // Sync Time Source
  UINT32  SysCondition;                   // Current System Condition of Box
  UINT32  Time;                           // time
  UINT16  TimeMs;                         // time in ms
  UINT32  PoweronCnt;                     // Power-on Count
  UINT32  PoweronTime;                    // Power-on Time

  UINT16  LogType;                        // see DTUMGR_LOG_TYPE
  UINT32  nLogCnt;                        // number of log count of this ACS
  UINT16  Priority;                       // priority of log data of this ACS
                                          // not used when log type is DTUMGR_LOG_TYPE_DTU

  UINT16  AcsSource;                      // which ACS is connected.  see ACS_SOURCE
                                          // not used when log type is DTUMGR_LOG_TYPE_DTU

  INT8    AcsModel[UPLD_HDR_STR_LEN32];   // ACS model id

  CHAR    AcsVerId[UPLD_HDR_STR_LEN32];   // ACS version information
                                          // not used when log type is DTUMGR_LOG_TYPE_DTU
  UINT8   PortType;                       // ACS Port Type from Configuration
  UINT8   PortIndex;                      // ACS Port Index from Configuration

  INT8    ConfigStatus[UPLD_HDR_STR_LEN32]; // Status of the box configuration
  INT8    TailNumber[UPLD_HDR_STR_LEN32]; // Tail Number of Aircraft
  INT8    Operator[UPLD_HDR_STR_LEN32];   // Airline 
  INT8    Style[UPLD_HDR_STR_LEN32];      // Implementation: 
                                          //   FAST: Aircraft Style (i.e. 737-300)
                                          //    G4U: BRC, G3, Bin, 7X, Lear60, etc.. 
  INT8    ConfigFile_QARPartNum[UPLD_HDR_STR_LEN64]; // 
  UINT16  ConfigVer_QARVer;               //
  
  UINT16  nDataOffset;                    // offset (based on this file )
                                          // where log data starts.  Note, ACS headers are
                                          // always at the beginning of data block.
  UINT32  nDataSize;                      // size of data block

}UPLOADMGR_FILE_HEADER;

typedef struct
{
   UINT16 SizeUsed;
   INT8   Buf[BINARY_DATA_BUF_LEN];
} BINARY_DATA_HDR;

/*Upload Manager info log structures*/
//Verify table information long
typedef struct
{
  INT32 Total;                              //Total size of the verify table
  INT32 Free;                               //Total number of rows "deleted"
}UPLOAD_VFY_TABLE_ROWS_LOG;

//File Transmission with Ground Server Successful log
typedef struct
{
  INT8   Filename[VFY_TBL_FN_SIZE];         //File Name
}UPLOAD_GS_TRANSMIT_SUCCESS;

//Server/File Ver Table CRC mismatch log
typedef struct
{
  UINT32 SrcCrc;                            //Source CRC (Micro-Server/Ground-Server)
  UINT32 FvtCrc;                            //File Verification Table CRC
  INT8   Filename[VFY_TBL_FN_SIZE];         //File Name
}UPLOAD_CRC_FAIL;

//Upload started information log
typedef enum
{
  UPLOAD_START_UNKNOWN     = 0,         
  UPLOAD_START_POST_FLIGHT = 1,
  UPLOAD_START_AUTO        = 2,
  UPLOAD_START_FORCED      = 3,
  UPLOAD_START_RECFG       = 4,
  UPLOAD_START_TXTEST      = 5
}UPLOAD_START_SOURCE;

#pragma pack()



/******************************************************************************
                                 Package Exports                              *
******************************************************************************/
#undef EXPORT

#if defined ( UPLOADMGR_BODY )
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
EXPORT void     UploadMgr_Init(void);
EXPORT void     UploadMgr_StartUpload(UPLOAD_START_SOURCE StartSource);
EXPORT BOOLEAN  UploadMgr_IsUploadInProgress(void); 
EXPORT void     UploadMgr_SetUploadEnable(BOOLEAN Enable);
EXPORT void     UploadMgr_WriteVfyTblRowsLog(void);
EXPORT BOOLEAN  UploadMgr_DeleteFVTEntry(const INT8* FN);
EXPORT BOOLEAN  UploadMgr_InitFileVfyTbl(void);
EXPORT INT32    UploadMgr_GetNumFilesPendingRT(void);
EXPORT void     UploadMgr_VfyDeleteIncomplete(void);
EXPORT UINT32   UploadMgr_GetFilesPendingRT(void);
EXPORT void     UploadMgr_FSMRun( BOOLEAN Run, INT32 Param );
EXPORT BOOLEAN  UploadMgr_FSMGetState( INT32 param );
EXPORT BOOLEAN  UploadMgr_FSMGetFilesPendingRT( INT32 param );
EXPORT void     UploadMgr_FSMRunAuto( BOOLEAN Run, INT32 Param );

#ifdef GENERATE_SYS_LOGS
EXPORT void     UploadMgr_GenerateDebugLogs(void);
#endif //GENERATE_SYS_LOGS

#endif // UPLOADMGR_H


/*************************************************************************
 *  MODIFICATIONS
 *    $History: UploadMgr.h $
 * 
 * *****************  Version 43  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 12:43p
 * Updated in $/software/control processor/code/application
 * SCR# 1142
 * 
 * *****************  Version 42  *****************
 * User: Jim Mood     Date: 7/26/12    Time: 4:57p
 * Updated in $/software/control processor/code/application
 * SCR 1076: Code review updates
 * 
 * *****************  Version 41  *****************
 * User: Jim Mood     Date: 7/26/12    Time: 2:08p
 * Updated in $/software/control processor/code/application
 * SCR# 1076 Code Review Updates
 * 
 * *****************  Version 40  *****************
 * User: Jim Mood     Date: 7/20/11    Time: 10:54a
 * Updated in $/software/control processor/code/application
 * SCR 575: GSM Enable when engine status is lost.  (Part of changes for
 * the Fast State Machine)
 * 
 * *****************  Version 39  *****************
 * User: John Omalley Date: 10/21/10   Time: 1:45p
 * Updated in $/software/control processor/code/application
 * SCR 960 - Increased the Log Size buffer to 64K
 * 
 * *****************  Version 38  *****************
 * User: Contractor2  Date: 10/14/10   Time: 1:55p
 * Updated in $/software/control processor/code/application
 * SCR #930 Code Review: UploadMgr
 * 
 * *****************  Version 37  *****************
 * User: Jim Mood     Date: 10/06/10   Time: 7:29p
 * Updated in $/software/control processor/code/application
 * SCR 880: Battery latch while files not round tripped
 * 
 * *****************  Version 36  *****************
 * User: Contractor2  Date: 10/05/10   Time: 2:57p
 * Updated in $/software/control processor/code/application
 * SCR #916 Code Review Updates
 * 
 * *****************  Version 35  *****************
 * User: John Omalley Date: 7/30/10    Time: 8:50a
 * Updated in $/software/control processor/code/application
 * SCR 678 - Remove files in the FVT that are marked not deleted
 * 
 * *****************  Version 34  *****************
 * User: Jim Mood     Date: 7/30/10    Time: 8:49a
 * Updated in $/software/control processor/code/application
 * SCR 327 Fast transmission test functionality.  Added a prototype for
 * the function to get the number of files pending verification, and added
 * an upload start reason
 * 
 * *****************  Version 33  *****************
 * User: Jim Mood     Date: 7/23/10    Time: 7:39p
 * Updated in $/software/control processor/code/application
 * SCR 151: Delete FVT entry on ground server crc good
 * SCR 208: Upload performance enhancement
 * 
 * *****************  Version 32  *****************
 * User: John Omalley Date: 7/02/10    Time: 10:14a
 * Updated in $/software/control processor/code/application
 * SCR 213 - Changed nLogCount for the File Header to UINT32
 * 
 * *****************  Version 31  *****************
 * User: John Omalley Date: 6/18/10    Time: 5:00p
 * Updated in $/software/control processor/code/application
 * SCR 654 - Changed file header version back to 1
 * 
 * *****************  Version 30  *****************
 * User: Contractor2  Date: 6/18/10    Time: 11:39a
 * Updated in $/software/control processor/code/application
 * SCR #611 Write Logs  on CRC Pass/Fail
 * Added sys log for SRS-2465.
 * Minor coding standard fixes.
 * 
 * *****************  Version 29  *****************
 * User: Contractor V&v Date: 6/08/10    Time: 5:51p
 * Updated in $/software/control processor/code/application
 * SCR #614 fast.reset=really should initialize files
 * 
 * *****************  Version 28  *****************
 * User: John Omalley Date: 6/08/10    Time: 12:03p
 * Updated in $/software/control processor/code/application
 * SCR 627 - Updated for LJ60
 * 
 * *****************  Version 27  *****************
 * User: Jim Mood     Date: 6/01/10    Time: 5:31p
 * Updated in $/software/control processor/code/application
 * SCR #619, Added an upload start source
 * 
 * *****************  Version 26  *****************
 * User: Contractor2  Date: 5/25/10    Time: 3:04p
 * Updated in $/software/control processor/code/application
 * SCR #611 Write Logs  on CRC Pass/Fail
 * Added sys logs. Minor coding standard fixes.
 * 
 * *****************  Version 25  *****************
 * User: Jeff Vahue   Date: 4/12/10    Time: 5:43p
 * Updated in $/software/control processor/code/application
 * SCR #541 - all XXX_CreateLogs functions are now dependent on
 * GENERATE_SYS_LOGS
 * 
 * *****************  Version 24  *****************
 * User: Contractor2  Date: 3/02/10    Time: 11:56a
 * Updated in $/software/control processor/code/application
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 23  *****************
 * User: Jim Mood     Date: 1/20/10    Time: 3:46p
 * Updated in $/software/control processor/code/application
 * Added public function to delete an FVT entry by log file name.  Initial
 * user of the function is the MSFileXfr file.
 * 
 * *****************  Version 22  *****************
 * User: Jim Mood     Date: 11/05/09   Time: 3:03p
 * Updated in $/software/control processor/code/application
 * Updated number of file verification entries from 180 to 200 per
 * requirments.
 * 
 * *****************  Version 21  *****************
 * User: Jim Mood     Date: 5/15/09    Time: 5:15p
 * Updated in $/software/control processor/code/application
 * Added user cmds to get verification rows free and total number of rows
 * 
 * *****************  Version 20  *****************
 * User: Jim Mood     Date: 4/16/09    Time: 9:21a
 * Updated in $/control processor/code/application
 * Added logs
 * 
 * *****************  Version 19  *****************
 * User: Jim Mood     Date: 4/02/09    Time: 8:57a
 * Updated in $/control processor/code/application
 * Increased Start Log timeout from 5s to 30s.  
 * 
 * *****************  Version 18  *****************
 * User: Peter Lee    Date: 1/23/09    Time: 10:09a
 * Updated in $/control processor/code/application
 * SCR #132 Support for new G4 file hdr fmt
 * 
 * *****************  Version 17  *****************
 * User: Jim Mood     Date: 1/09/09    Time: 2:19p
 * Updated in $/control processor/code/application
 * Added timeout for Check CRC and Remove File commands sent from the File
 * Verification Table Maintenance task.
 * 
 * *****************  Version 16  *****************
 * User: Jim Mood     Date: 1/06/09    Time: 4:55p
 * Updated in $/control processor/code/application
 * This version includes full round trip verification functionality.  This
 * impliments a 180 entry file verification table, and validates CRC
 * values from the micro-server.
 * 
 * *****************  Version 15  *****************
 * User: Jim Mood     Date: 12/12/08   Time: 6:53p
 * Updated in $/control processor/code/application
 * Buildable version with some round-trip functionality.  Checked in for
 * backup purposes
 * 
 * *****************  Version 13  *****************
 * User: Jim Mood     Date: 10/07/08   Time: 11:45a
 * Updated in $/control processor/code/application
 * Fixed missing '/' in the "History" comment block at the end of this
 * file
 * 
 * *****************  Version 12  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 10:54a
 * Updated in $/control processor/code/application
 * Add History / Revision Block
 * 
 *
 ***************************************************************************/
