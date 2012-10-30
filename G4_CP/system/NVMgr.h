#ifndef SYS_NVMGR_H
#define SYS_NVMGR_H
/******************************************************************************
            Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

  File:          NVMgr.h

  Description:   Header file for NVMgr.h This has the definitions for "devices"
                 and "files"  See the descriptions of NV_DEV and NV_FILE for
                 information on adding/modifying files of nv memory devices
                 to this module.

   VERSION
    $Revision: 62 $  $Date: 12-10-27 5:01p $

******************************************************************************/


/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/


/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "ResultCodes.h"
#include "Utility.h"
#include "SPIManager.h"

/******************************************************************************
                                 Package Defines
******************************************************************************/
#define NV_CRC_SIZE (sizeof(UINT16))

//define a kilo-byte for listings below
#undef KB
#define KB *1024

/*Need data to allocate a Device ID label, Shadow Copy array, sector write ref-counts,
  and Device data structure.  This list will automatically allocate all 4 of
  these items.

  NV_DEV table, VVVVV-INSTRUCTIONS FOR ADDING/MODIFYING DEVICES-VVVVV

  This table allocates a Device ID enumeration, Shadow Memory, write ref-counts, and
  a device data structure.
  Id           - A name for the device
  Total Size   - Total size of the device in bytes
  Page Size    - This will be the minimum size written to the external device.
                 The EEPROM for instance has a 256 byte page size, and the write
                 time is the same for 1 - 256 bytes.  A sector size of greater
                 than 1 byte makes processing of large writes faster.
                 This is at the expense of small write
                 performance. See !!IMPORTANT!! below.
  Offset       - An offset added to any access to the device.  This is useful
                 for defining multiple logical devices within one physical
                 device.  The EEPROM for instance is one chip divided two equal
                 size logical devices
  Page TimeOut - Sector Write timeout period in milliseconds.
                  1.) The sector timeout for the EEPROM is stated as 5 ms worst case for
                      a sector write.  With time on the wire at 1.6 us/byte that is
                      410 us Tx time giving us a worst case of ~5.4 ms / sector.
                  2.) The sector time out for the RTC is basically zero so we just use
                      5 ms as a dead device detector.
  Reset        - If a file is to be reset, what value should be written to initialize
                 the data
  Read         - Pointer to a function to read the memory device (see
                 NV_DEV_READ_FUNC prototype)  This function should be added to
                 the local function section of NVMgr.c
  Write        - Pointer to a function to write the memory device (see
                 NV_DEV_WRITE_FUNC prototype) This function should be added to
                 the local function section of NVMgr.c

  !!IMPORTANT!!: THE MINIMUM SECTOR SIZE IS NV_CRC_SIZE (2 bytes) The code
                 assumes the CRC will fit entirely in one sector.
  !!IMPORTANT2!!
*/

#undef NV_DEV
#define NV_DEV(Id,Size,SectorSize,Offset,SectorTimeout,reset,Read,Write)
//                    Total     Page Device      Page
//        Id          Size      Size Offset      Timeout Reset    Read        Write
#define NV_DEV_LIST \
NV_DEV(DEV_NONE,      1,        1,   0,          0,      0,       NULL,       NULL),        \
NV_DEV(DEV_RTC_PRI,   256,      4,   0,          5,      0,       NV_RTCRead, NV_RTCWrite), \
NV_DEV(DEV_EE_PRI,    (128 KB), 256, 0,          10,     0xFF,    NV_EERead,  NV_EEWrite),  \
NV_DEV(DEV_EE_BKUP,   (128 KB), 256, 0x10000000, 10,     0xFF,    NV_EERead,  NV_EEWrite)


/*
  NV_FILE table, VVVVV-INSTRUCTIONS FOR ADDING/MODIFYING FILES-VVVVV

  This table defines the allocation of files in the devices above.  Each file
  size and device can be tailored to the requirements of the software module
  using the file.
  Please keep the file naming consistent (i.e. NV_File#) and add a comment next
  to each file as to what software module is using it.  Note that the files
  are allocated to the devices at runtime from the top to the bottom of this
  table. Modifying a file at the top of the list may change the offset of
  files in the same device that are lower in the list.
  20 files have initially been setup in the list, more can be added later or
  unused files can be removed from the bottom.

  Column Descriptions:
  ID -      A name for the file
  Name -    The display/reporting name of the file
  Primary - The primary devices for storing this file.  This must be defined
            using an Id from the NV_DEV_LIST above
  Backup -  The backup device for storing the file. This can optionally be
            defined as DEV_NONE if dual-redundant storage is not required.
  Init   -  Function pointer to init function for init'ing file upon sys reset.
            NULL if no reset is to be performed on the associated file. 
  Size -    Required size of the file in bytes.  It would be useful to allocate
            some extra space if the file is expected to grow.  Modifying the
            size of a file can affect the allocation of files below it in the
            table.


 !!IMPORTANT!!: The NVMgr reserves the last two bytes of every file for CRC
                information

 !!IMPORTANT!!: The minimum size for a file is the size of the device sector, this is
                enforced by an assert.  This rule exists specifically for the EEPROM,
                but is enforced on all devices.
 
NV_BoxCfg          - Status Manager. This contains information, such as serial # and
                     other data that absolutely cannot move between code revisions
NV_PWR_ON_CNTS_EE  - Power-On counts EE
NV_PWR_ON_CNTS_RTC - Power-On counts RTC
NV_CFG_MGR         - Configuration Manager: Box configuration
NV_UPLOAD_VFY_TBL  - Upload Manager: File verification data
NV_FAULT_LOG       - Fault Manager: Last 10 fault buffer
NV_PWR_MGR         - Power Manager: Power Off State
NV_ASSERT_LOG      - Assert/Exception log buffer.
NV_PWEH_CNTS_SEU   - PWEH SEU
NV_UART_F7X        - Uart F7X Protocol application non-volatile data 
NV_LOG_ERASE       - Log Erase Status file: persisted info re log erase in-prog between starts
NV_AC_CFG          - Type, Fleet, and Number sensor values read from the AC bus 
                       for AircraftConfiguration
NV_LOG_COUNTS      - Saves the errors counts during log manager operations                       
NV_UART_EMU150     - Uart EMU150 Protocol application non-volatile data
NV_PCYCLE_CNTS_EE  - Persisted Cycle Counts EE
NV_PCYCLE_CNTS_RTC - Persisted Cycle Counts RTC Ram
NV_ACT_STATUS_RTC  - Persistent Action Status
NV_ACT_STATUS_EE   - Persistent Action Status
NV_ENGINE_ID       - Engine identification: EngineRun
NV_CREEP_DATA      - Creep Application Data
NV_CREEP_CNTS_RTC  - Creep Application RTC Data
NV_CREEP_HISTORY   - Creep Fault History Data 
*/

#undef NV_FILE
#define NV_FILE(Id, Name, Primary, Backup, CSum, Init, Size)

//                                                                             Check     Initialization
//          Id                Name                 Primary         Backup      Method    Function                     Size
#define NV_FILE_LIST \
NV_FILE(NV_BOX_CFG,        "Status Manager",       DEV_EE_PRI,   DEV_EE_BKUP,  CM_CRC16,  NULL                    ,  (1 KB)), \
NV_FILE(NV_PWR_ON_CNTS_EE, "Power-On Counts EE",   DEV_EE_PRI,   DEV_EE_BKUP,  CM_CRC16,  NULL                    ,     256), \
NV_FILE(NV_PWR_ON_CNTS_RTC,"Power-On Counts RTC",  DEV_RTC_PRI,  DEV_NONE,     CM_CRC16,  NULL                    ,      12), \
NV_FILE(NV_CFG_MGR,        "Configuration Manager",DEV_EE_PRI,   DEV_EE_BKUP,  CM_CRC16,  CfgMgr_FileInit         ,  (88 KB)),\
NV_FILE(NV_UPLOAD_VFY_TBL, "Upload Manager",       DEV_EE_PRI,   DEV_EE_BKUP,  CM_CRC16,  UploadMgr_InitFileVfyTbl,    18176),\
NV_FILE(NV_FAULT_LOG,      "Fault Manager",        DEV_EE_PRI,   DEV_EE_BKUP,  CM_CSUM16, Flt_InitFltBuf          ,      256),\
NV_FILE(NV_PWR_MGR,        "Power Manager",        DEV_EE_PRI,   DEV_EE_BKUP,  CM_CRC16,  PmFileInit              ,      256),\
NV_FILE(NV_ASSERT_LOG,     "Exception Log",        DEV_EE_PRI,   DEV_EE_BKUP,  CM_CRC16,  Assert_InitEE           ,   (2 KB)),\
NV_FILE(NV_PWEH_CNTS_SEU,  "PWEH SEU Counts",      DEV_EE_PRI,   DEV_EE_BKUP,  CM_CRC16,  CBITMgr_PWEHSEU_FileInit,   (1 KB)),\
NV_FILE(NV_UART_F7X,       "Uart F7X Data",        DEV_EE_PRI,   DEV_EE_BKUP,  CM_CRC16,  F7XProtocol_FileInit    ,     256), \
NV_FILE(NV_VER_INFO,       "Version Info",         DEV_EE_PRI,   DEV_EE_BKUP,  CM_CRC16,  NULL                    ,     256), \
NV_FILE(NV_LOG_ERASE,      "Log Erase Status",     DEV_EE_PRI,   DEV_EE_BKUP,  CM_CRC16,  NULL                    ,     256), \
NV_FILE(NV_AC_CFG,         "Recfg Sensor Val",     DEV_EE_PRI,   DEV_EE_BKUP,  CM_CRC16,  AC_FileInit             ,     256), \
NV_FILE(NV_LOG_COUNTS,     "Log Error Counts",     DEV_RTC_PRI,  DEV_NONE,     CM_CRC16,  NULL                    ,      16), \
NV_FILE(NV_UART_EMU150,    "Uart EMU150 Data",     DEV_EE_PRI,   DEV_EE_BKUP,  CM_CSUM16, EMU150_FileInit         ,   (6 KB)),\
NV_FILE(NV_PCYCLE_CNTS_EE, "P-Cycle Counts EE",    DEV_EE_PRI,   DEV_EE_BKUP,  CM_CRC16,  NULL                    ,     256), \
NV_FILE(NV_PCYCLE_CNTS_RTC,"P-Cycle Counts RTC",   DEV_RTC_PRI,  DEV_NONE,     CM_CRC16,  NULL                    ,     196), \
NV_FILE(NV_ACT_STATUS_RTC, "Action Status RTC",    DEV_RTC_PRI,  DEV_NONE,     CM_CRC16,  NULL                    ,       8),\
NV_FILE(NV_ACT_STATUS_EE,  "Action Status EE ",    DEV_EE_PRI,   DEV_EE_BKUP,  CM_CRC16,  NULL                    ,     256),\
NV_FILE(NV_ENGINE_ID,      "Engine Identification",DEV_EE_PRI,   DEV_EE_BKUP,  CM_CRC16,  NULL                    ,     256), \
NV_FILE(NV_CREEP_DATA,     "Creep Data",           DEV_EE_PRI,   DEV_EE_BKUP,  CM_CRC16,  Creep_FileInit          ,     256), \
NV_FILE(NV_CREEP_CNTS_RTC, "Creep Cnts RTC",       DEV_RTC_PRI,  DEV_NONE,     CM_CRC16,  NULL                    ,      20), \
NV_FILE(NV_CREEP_HISTORY,  "Creep Fault History",  DEV_EE_PRI,   DEV_EE_BKUP,  CM_CSUM16, Creep_FaultFileInit     ,     512)



/******************************************************************************
                                 Package Typedefs
******************************************************************************/
/*
Device read driver function prototype:
New devices must implement a read driver for NV Manager, see
existing examples at the end of NVMgr.c.
The Read function should read as much data as it  reasonably
can in one call.  If reading takes a long time, or
if the properties of the external memory device make it
difficult to return any arbitrarily sized block of data, this
routine may read a smaller amount than requested.
The number of bytes actually read is returned to the caller.
If the bytes read is less than the
size parameter, this routine will be called again and again
until the entire block is read.  If an unrecoverable error
occurs, the read function should set the size read value to -1,
signaling the caller to stop trying to read and report an
error.
*/
typedef INT32 (*NV_DEV_READ_FUNC )
              (void* DestAddr, UINT32 PhysicalOffset, UINT32 Size);
/*
Device write driver function prototype:
New devices must implement a write driver for NV Manager, see
existing examples at the end of NVMgr.c.

The Write function attempts to write as much data as it
reasonably can in one call.  If writing takes a long time, or
if the properties of the external memory device make it
difficult to write any arbitrarily large size block of data,
this routine can write an amount smaller than requested.
The number of bytes actually written is returned to the caller.
If the bytes written is less than the
size parameter, this routine will be called again and again
until the entire block is written.  If an unrecoverable error
occurs, the read function should set the size written value to
-1, signaling the caller to stop trying to write and report an
error.
*/
typedef INT32 (*NV_DEV_WRITE_FUNC )
     (void* SourceAddr, UINT32 PhysicalOffset, UINT32 Size, IO_RESULT* ioResult);

/*
  NV_FILE initializing function. Files listed in the NV_FILE_LIST should list
  the address of it's corresponding init function or NULL to flag that the file
  is not initialized during a reset.
*/
typedef BOOLEAN (*NV_INFO_INIT_FUNC ) (void);


/*
Type definitions for string containing the name of a file
*/
#define MAX_NV_FILENAME 32
typedef CHAR NV_FILENAME[MAX_NV_FILENAME];

//List of hardware devices for non-volatile memory storage, populated from
//the NV_DEV_LIST macro above
#undef  NV_DEV
#define NV_DEV(Id,Size,SectorSize,Offset,SectorTimeout,reset,Read,Write) Id
typedef enum {
  NV_DEV_LIST,
  NV_MAX_DEV
}NV_DEVS_ID;

//Device write semaphores (written) and mutex (writing)
//used to sync updates to the external NV memory device.
typedef struct {
  BOOLEAN Writing;
  BOOLEAN Written;
}NV_DEV_SEM;

// Shadow -> Device Update reference counters.
// One entry per sector for each device.
// These values are compared for in-equality to determine if Dev Write is needed
typedef struct {
  UINT8 ShadowCount;  // Incremented on write to Shadow Ram.  (wrap-around)
  UINT8 NvRAMCount;   // Set to ShadowCount on completion of write to NVRam.(wrap-around)
}NV_DEV_REFCOUNT;

//Device info structure
typedef struct {
  UINT32     Size;
  UINT32     SectorSize;
  UINT32     Offset;
  UINT32     SectorTimeoutMs;
  UINT8      resetValue;
  UINT8*     ShadowMem;
  IO_RESULT* sectorIoResult;
  NV_DEV_REFCOUNT* RefCount;
  NV_DEV_SEM* Sem;
  NV_DEV_WRITE_FUNC Write;
  NV_DEV_READ_FUNC  Read;
}NV_DEV_INFO;

//File info structure
typedef struct {
  UINT32            Size;
  NV_DEVS_ID        PrimaryDevID;
  NV_DEVS_ID        BackupDevID;
  CHECK_METHOD      CheckMethod;
  NV_INFO_INIT_FUNC Init;
}NV_FILE_INFO;

//Runtime info, this is created from the FILE_INFO and DEV_INFO data at
//initialization time.
typedef struct {
  BOOLEAN             IsOpen;
  UINT32              Size;
  const NV_DEV_INFO*  PrimaryDev;
  const NV_DEV_INFO*  BackupDev;
  UINT32              PrimaryOffset;
  UINT32              BackupOffset;
  CHECK_METHOD        CheckMethod;
  NV_INFO_INIT_FUNC   Init;
}NV_RUNTIME_INFO;

#undef NV_FILE
#define NV_FILE(Id, Name, Primary, Backup, CSum, InitFunc, Size) Id
//List of files for non-volatile memory storage, populated from
//the NV_FILE macro above
typedef enum {
  NV_FILE_LIST,
  NV_MAX_FILE
} NV_FILE_ID;

typedef enum {
  NV_PRIMARY,
  NV_BACKUP,
  NV_FILE_TYPE_MAX
}NV_FILE_TYPE;

typedef enum {
   NV_TASK_SEARCH,
   NV_TASK_WRITE,
}NV_TASK_STATE;

// Table of Names and CRC values for each file.
typedef struct {
   NV_FILENAME Name;
   UINT16      CRC;
}NV_FILE_CRC;

// NV SysLog error structures
#pragma pack(1)

// Structure for reporting single-file error.
typedef struct {
  NV_FILE_ID  FileID; 
  NV_FILENAME Name;
  UINT16      CRC;
} NV_FILE_OPEN_ERROR_LOG;

// Structure for reporting pri <-> backup file error.
typedef struct {
  NV_FILE_ID  FileID; 
  NV_FILENAME Name;
  UINT16      PriCRC;
  UINT16      BackupCRC;
} NV_PRI_BACKUP_OPEN_ERROR_LOG;

#pragma pack()


/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined( SYS_NVMGR_BODY )
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
EXPORT void NV_Init(void);
EXPORT RESULT NV_Open(NV_FILE_ID FileID);
EXPORT void   NV_Close(NV_FILE_ID FileID);
EXPORT void   NV_Write(NV_FILE_ID FileID, UINT32 Offset, const void* Data, UINT32 Size);
EXPORT RESULT NV_WriteNow(NV_FILE_ID FileID, UINT32 Offset, void* Data, UINT32 Size);
EXPORT RESULT NV_WriteState(NV_FILE_ID FileID, UINT32 Offset, UINT32 Size);
EXPORT void   NV_Read(NV_FILE_ID FileID, UINT32 Offset, void* Buf, UINT32 Size);
EXPORT void   NV_Erase(NV_FILE_ID FileID);
EXPORT void   NV_ResetConfig(void);
//EXPORT RESULT NV_ReadDev(NV_DEVS_ID DevID, void* Data, UINT32 Offset, UINT32 Size);
//EXPORT RESULT NV_WriteDev(NV_DEVS_ID DevID, void* Data, UINT32 Offset, UINT32 Size);
//NV Device driver functions
EXPORT INT32 NV_EERead(void* DestAddr, UINT32 PhysicalOffset, UINT32 Size);
EXPORT INT32 NV_EEWrite(void* SourceAddr, 
                        UINT32 PhysicalOffset, 
                        UINT32 Size, 
                        IO_RESULT* ioResult);
EXPORT INT32 NV_RTCRead(void* DestAddr, UINT32 PhysicalOffset, UINT32 Size);
EXPORT INT32 NV_RTCWrite(void* SourceAddr, 
                         UINT32 PhysicalOffset, 
                         UINT32 Size, 
                         IO_RESULT* ioResult );

EXPORT NV_FILENAME* NV_GetFileName(NV_FILE_ID fileNum);
EXPORT INT32        NV_GetFileCRC(NV_FILE_ID fileNum);

#endif // SYS_NVMGR_H

/*************************************************************************
 *  MODIFICATIONS
 *    $History: NVMgr.h $
 * 
 * *****************  Version 62  *****************
 * User: Peter Lee    Date: 12-10-27   Time: 5:01p
 * Updated in $/software/control processor/code/system
 * SCR #1190 Creep Requirements
 * 
 * *****************  Version 61  *****************
 * User: John Omalley Date: 12-09-27   Time: 9:14a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Added Engine Identification Fields to software and file
 * header
 * 
 * *****************  Version 60  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 59  *****************
 * User: John Omalley Date: 12-08-14   Time: 2:08p
 * Updated in $/software/control processor/code/system
 * SCR 1076 - Code Review Update
 * 
 * *****************  Version 58  *****************
 * User: John Omalley Date: 12-07-27   Time: 3:02p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Action Manager Persistent updates
 * 
 * *****************  Version 57  *****************
 * User: Jim Mood     Date: 7/27/12    Time: 10:06a
 * Updated in $/software/control processor/code/system
 * SCR 1138
 * 
 * *****************  Version 56  *****************
 * User: Contractor V&v Date: 7/11/12    Time: 4:36p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2  Adjusted size of Cycle cnts RTC NVRAM
 * 
 * *****************  Version 55  *****************
 * User: Contractor V&v Date: 5/24/12    Time: 3:06p
 * Updated in $/software/control processor/code/system
 * Resize the CFG for the change of enginerun and cycles UINT8 to ENUM
 * 
 * *****************  Version 54  *****************
 * User: John Omalley Date: 12-05-22   Time: 2:19p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Check in for Dave
 * 
 * *****************  Version 53  *****************
 * User: John Omalley Date: 4/27/12    Time: 5:04p
 * Updated in $/software/control processor/code/system
 * 
 * *****************  Version 52  *****************
 * User: Contractor V&v Date: 4/23/12    Time: 8:01p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2  Cycle 
 * 
 * *****************  Version 51  *****************
 * User: Contractor V&v Date: 4/11/12    Time: 5:02p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST2  NvCfgMgr sizing for 128-sensors, 64 triggers
 * 
 * *****************  Version 50  *****************
 * User: Contractor V&v Date: 3/14/12    Time: 4:52p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST2 EngineRun/Cycle processing
 * 
 * *****************  Version 49  *****************
 * User: Peter Lee    Date: 7/25/11    Time: 5:39p
 * Updated in $/software/control processor/code/system
 * SCR #1037 Increase NVMgr for NV_UART_EMU150 to 6 KB to accomodate the
 * worst case size. 
 * 
 * *****************  Version 48  *****************
 * User: Peter Lee    Date: 4/11/11    Time: 10:13a
 * Updated in $/software/control processor/code/system
 * SCR #1029 EMU150 Download
 * 
 * *****************  Version 47  *****************
 * User: Contractor2  Date: 9/20/10    Time: 4:11p
 * Updated in $/software/control processor/code/system
 * SCR #856 AC Reconfig File should be re-initialized during fast.reset
 * command.
 * 
 * *****************  Version 46  *****************
 * User: Peter Lee    Date: 9/01/10    Time: 3:00p
 * Updated in $/software/control processor/code/system
 * SCR #840 Code Review Updates
 * 
 * *****************  Version 45  *****************
 * User: Jim Mood     Date: 8/10/10    Time: 10:25a
 * Updated in $/software/control processor/code/system
 * SCR 778 Fixed Recfg Sensor Value size 
 * 
 * *****************  Version 44  *****************
 * User: John Omalley Date: 7/23/10    Time: 10:16a
 * Updated in $/software/control processor/code/system
 * SCR 306 / SCR 639 - Added the Log Error Counts
 * 
 * *****************  Version 43  *****************
 * User: Jim Mood     Date: 7/22/10    Time: 10:47a
 * Updated in $/software/control processor/code/system
 * SCR 724 NV_AC_SENSOR file size had a typo
 * 
 * *****************  Version 42  *****************
 * User: Contractor V&v Date: 7/21/10    Time: 7:19p
 * Updated in $/software/control processor/code/system
 * SCR #538 SPIManager startup & NV Mgr Issues
 * 
 * *****************  Version 41  *****************
 * User: Contractor3  Date: 7/16/10    Time: 8:57a
 * Updated in $/software/control processor/code/system
 * SCR #702 - Changes based on code review.
 * 
 * *****************  Version 40  *****************
 * User: Contractor V&v Date: 7/15/10    Time: 4:20p
 * Updated in $/software/control processor/code/system
 * SCR #630 NV_FILENAME is 33 characters, should it be 32
 * 
 * *****************  Version 39  *****************
 * User: Jim Mood     Date: 6/29/10    Time: 5:57p
 * Updated in $/software/control processor/code/system
 * SCR 623.  Added an EEPROM file for aircraft reconfiguration data
 * 
 * *****************  Version 38  *****************
 * User: Contractor V&v Date: 6/16/10    Time: 6:10p
 * Updated in $/software/control processor/code/system
 * SCR #623 SCR #636
 * 
 * *****************  Version 37  *****************
 * User: Contractor2  Date: 6/14/10    Time: 1:08p
 * Updated in $/software/control processor/code/system
 * SCR #483 Function Names must begin with the CSC it belongs with.
 * 
 * *****************  Version 36  *****************
 * User: Contractor V&v Date: 6/09/10    Time: 7:22p
 * Updated in $/software/control processor/code/system
 * SCR #614 fast.reset=really should initialize files
 * 
 * *****************  Version 35  *****************
 * User: Contractor V&v Date: 6/08/10    Time: 5:55p
 * Updated in $/software/control processor/code/system
 * SCR #614 fast.reset=really should initialize files
 * 
 * *****************  Version 34  *****************
 * User: Peter Lee    Date: 5/31/10    Time: 6:20p
 * Updated in $/software/control processor/code/system
 * SCR #618 F7X and UartMgr Requirements Implementation
 * 
 * *****************  Version 33  *****************
 * User: Contractor V&v Date: 5/12/10    Time: 3:49p
 * Updated in $/software/control processor/code/system
 * SCR #548 Strings in Logs
 * 
 * *****************  Version 32  *****************
 * User: Jeff Vahue   Date: 4/13/10    Time: 11:55a
 * Updated in $/software/control processor/code/system
 * SCR #538 - put file sizes back to min == page size.  Add an assert if
 * this is not true.  Fix cut/paste error on check for over allocation on
 * the backup device.
 * 
 * *****************  Version 31  *****************
 * User: Jeff Vahue   Date: 4/09/10    Time: 4:39p
 * Updated in $/software/control processor/code/system
 * SCR #538 - optimize SPI activity, minimize file sizes and page writes
 * 
 * *****************  Version 30  *****************
 * User: Contractor V&v Date: 3/29/10    Time: 6:18p
 * Updated in $/software/control processor/code/system
 * SCR #140 Move Log erase status to own file
 *
 * *****************  Version 29  *****************
 * User: Contractor V&v Date: 3/19/10    Time: 4:31p
 * Updated in $/software/control processor/code/system
 * SCR #279 Making NV filenames uppercase
 *
 * *****************  Version 28  *****************
 * User: Contractor V&v Date: 3/10/10    Time: 4:42p
 * Updated in $/software/control processor/code/system
 * SCR #464 Move Version Info into own file and support it.
 *
 * *****************  Version 27  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 4:01p
 * Updated in $/software/control processor/code/system
 * SCR #279 Resetting of NvMgr / EEPROM application data
 * SCR #67 Interrupted SPI Access (Multiple / Nested SPI Access)
 *
 * *****************  Version 26  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 25  *****************
 * User: Jeff Vahue   Date: 2/11/10    Time: 4:47p
 * Updated in $/software/control processor/code/system
 * SCR# 448 - remove box.dbg.erase_ee_dev cmd
 *
 * *****************  Version 24  *****************
 * User: Contractor V&v Date: 2/03/10    Time: 2:57p
 * Updated in $/software/control processor/code/system
 * SCR 279, 330
 *
 * *****************  Version 23  *****************
 * User: Jeff Vahue   Date: 1/28/10    Time: 12:20p
 * Updated in $/software/control processor/code/system
 * SCR# 372: Misc - Enhancement.  Updating cfg EEPROM data
 *
 * Use NV_Write to erase a file as it knows all about file size, offset,
 * etc.  Works for both startup and Run time reset.
 *
 * *****************  Version 22  *****************
 * User: Jeff Vahue   Date: 1/28/10    Time: 10:24a
 * Updated in $/software/control processor/code/system
 * SCR# 142: Error: Faulted EEPROM "Write In Progress" bit causes infinite
 * loop.
 *
 * Change timeout value on the EEPROM from 5 ms to 10ms.  5ms is the
 * specified worst case, but did not account for time on wire of
 * 1.6us/byte (410 us).  Lab testing should actual write time to be around
 * 4.8ms/page as total write time came in around 5.25m/page.
 *
 * *****************  Version 21  *****************
 * User: Contractor V&v Date: 1/27/10    Time: 5:24p
 * Updated in $/software/control processor/code/system
 * SCR 142, SCR 372, SCR 398
 *
 * *****************  Version 20  *****************
 * User: Jeff Vahue   Date: 1/13/10    Time: 11:21a
 * Updated in $/software/control processor/code/system
 * SCR# 330
 *
 * *****************  Version 19  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:26p
 * Updated in $/software/control processor/code/system
 * SCR 145
 *
 * *****************  Version 18  *****************
 * User: Peter Lee    Date: 12/02/09   Time: 2:17p
 * Updated in $/software/control processor/code/system
 * SCR #350 Misc Code Review Items
 *
 * *****************  Version 17  *****************
 * User: Contractor V&v Date: 11/30/09   Time: 5:32p
 * Updated in $/software/control processor/code/system
 * Implement functions for accessing filenames and CRC
 *
 * *****************  Version 16  *****************
 * User: Peter Lee    Date: 11/19/09   Time: 11:22a
 * Updated in $/software/control processor/code/system
 * SCR #315 Fix bug with maintaining SEU counts across power cycle
 *
 * *****************  Version 15  *****************
 * User: Contractor V&v Date: 11/18/09   Time: 4:18p
 * Updated in $/software/control processor/code/system
 * Implement SCR 145 Configuration CRC read command
 * Correct typos
 *
 * *****************  Version 14  *****************
 * User: John Omalley Date: 10/01/09   Time: 4:27p
 * Updated in $/software/control processor/code/system
 * Updated size for new sensors and tirggers
 *
 * *****************  Version 13  *****************
 * User: Peter Lee    Date: 9/28/09    Time: 4:17p
 * Updated in $/software/control processor/code/system
 * No code update.  Added comment to better identify NV_FILEx usage.
 *
 * *****************  Version 12  *****************
 * User: Jim Mood     Date: 9/25/09    Time: 9:04a
 * Updated in $/software/control processor/code/system
 * Added driver functions and support for accessing the RTC battery-backed
 * RAM
 *
 * *****************  Version 11  *****************
 * User: Jim Mood     Date: 8/18/09    Time: 9:37a
 * Updated in $/software/control processor/code/system
 *
 * *****************  Version 10  *****************
 * User: Jim Mood     Date: 6/05/09    Time: 3:29p
 * Updated in $/software/control processor/code/system
 * NV_WriteNow function errors.  SCR 195
 *
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 5/21/09    Time: 2:45p
 * Updated in $/software/control processor/code/system
 * Power Management Updates
 *
 * *****************  Version 8  *****************
 * User: Jim Mood     Date: 5/01/09    Time: 3:49p
 * Updated in $/software/control processor/code/system
 * Added an externally callable function to erase a device to 0xFF
 *
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 4/29/09    Time: 3:42p
 * Updated in $/software/control processor/code/system
 * SCR #168 Update Comment and put space before "KB"
 *
 * *****************  Version 6  *****************
 * User: Jim Mood     Date: 2/03/09    Time: 5:14p
 * Updated in $/control processor/code/system
 * Updated device table to allocate the backup files to the 2nd EEPROM
 * device.  If the 2nd device is not installed, the backup feature will
 * not work and the primary will be the only copy of data.  See also
 * SCR#142
 *
 * *****************  Version 5  *****************
 * User: Jim Mood     Date: 11/25/08   Time: 6:03p
 * Updated in $/control processor/code/system
 * Changed allocation size for the Upload Manager file
 *
 * *****************  Version 4  *****************
 * User: Jim Mood     Date: 10/08/08   Time: 10:41a
 * Updated in $/control processor/code/system
 *
 * *****************  Version 3  *****************
 * User: Jim Mood     Date: 10/02/08   Time: 9:48a
 * Updated in $/control processor/code/system
 *
 * *****************  Version 2  *****************
 * User: Jim Mood     Date: 9/19/08    Time: 5:58p
 * Updated in $/control processor/code/system
 *
 * *****************  Version 1  *****************
 * User: Jim Mood     Date: 9/16/08    Time: 10:26a
 * Created in $/control processor/code/system
 *
 *
 ***************************************************************************/

