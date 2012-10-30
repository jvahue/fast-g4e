#define SYS_NVMGR_BODY
/******************************************************************************
            Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

 File:       NVMgr.c

 Description: NV Manager is a fault-tolerant data storage system that
              allocates and manages access to non-volatile memory devices.
              The manager allocates shadow memory for the devices with
              cached read and write-back functionality.  A background task takes
              care of writing cached data out to the devices.  A dual-redundant
              storage mode provides fault-tolerance against unexpected reset and
              power down.

              To modify the file allocation, see NVMgr.h
              To add or remove non-volatile memory devices, also see NVMgr.h



 Requires: ResultCodes
           LogManager
           FaultManager
           Utility
           TaskManager
           EEPROM
           RTC

   VERSION
    $Revision: 72 $  $Date: 12-10-27 5:02p $


******************************************************************************/


/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include <string.h>
#include <stdio.h>

#include "alt_stdtypes.h"
#include "alt_basic.h"
#include "EEPROM.h" //Only needed for SPI EEPROM write driver
#include "NVMgr.h"
#include "RTC.h"    //Only needed for RTC read/write driver
#include "TaskManager.h"
#include "LogManager.h"
#include "GSE.h"
#include "Assert.h"

// only needed for XXX_FileInit functions
#include "CfgManager.h"
#include "UploadMgr.h"
#include "FaultMgr.h"
#include "CBITManager.h"
#include "PowerManager.h"
#include "F7XProtocol.h"

#include "TestPoints.h"

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
//Shadow array allocates a struct of two UINT8 vars for each sector to
//maintain ref-counts. One for writes to Shadow, the other for writes to NVRAM
//this macro calculates the number of sectors for each device.

#define NV_SECTOR_ARRAY_SIZE(MemSize,SectorSize) ( MemSize / SectorSize)

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
typedef enum
{
  NV_FILE_PRI,
  NV_FILE_BKUP,
  NV_FILE_DEV_MAX
}NV_FILE_DEVS;


/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
/*
  Data structure:
  These macros allocate data based on  the File and Device tables in NVMgr.h
  The NV_DevInfo and NV_FileInfo contain const data.  ShadowMem, ref-counts,Sem
  and NV_RuntimeInfo are allocated in RAM.  The RuntimeInfo contains the file
  allocation data computed by NV_Init()

                  NV_DevInfo                  NV_FileInfo
                  |                           |
                  |->ShadowMem array          |
                  |->DirtyBit array           |
                  |->Sem structure            |__________
                         |                          |
                         ----------------------------
                                      |
                                      V
                               NV_RuntimeInfo

  */

//          ShadowMem allocation
#undef NV_DEV
#define NV_DEV(Id,Size,SectorSize,Offset,SectorTimeoutMs,reset,Read,Write) Id##_Shadow[Size]
static UINT8 NV_DEV_LIST;

//          Sector  Shadow-NVRAM reference-count allocation
// Setup the NV_DEV macro to create a dirty-sector ref table. This allocates a
// structure of two bytes for every sector.
// NvMgr increments one value every time Shadow RAM is touched. The other is
// synchronized to the ShadowCount value each time the sector is written to device.
// Each sector is en-queue to be written to dev as long as the two
// values are unequal. Since the values are only compared for inequality.
// they can safely wrap-around. The signaling value which allows the MvMgr to observe
// the state of the Dev-write being performed by SpiManager is the IO_RESULT. (See below)
#undef NV_DEV
#define NV_DEV(Id,Size,SectorSize,Offset,SectorTimeoutMs,reset,Read,Write)\
  Id##_RefCount[NV_SECTOR_ARRAY_SIZE(Size,SectorSize)]
static NV_DEV_REFCOUNT NV_DEV_LIST;


//          Semaphores allocation
#undef NV_DEV
#define NV_DEV(Id,Size,SectorSize,Offset,SectorTimeoutMs,reset,Read,Write)\
Id##_DevSem
static NV_DEV_SEM NV_DEV_LIST;

//          I/O Result  (IO_RESULT) allocation
// Set up the NV_DEV macro to create a i/o result table. This is an array of
// IO_RESULT enums (one entry per sector) for each device. A pointer to the applicable
// sector's IO_RESULT entry is passed to the SPiManager during Read/Write operations. The
// SpiManager updates the status entry, thereby allowing the NVMgr to
// monitor/manage the state of issued i/o operations being handled in the SpiManager frame.
#undef NV_DEV
#define NV_DEV(Id,Size,SectorSize,Offset,SectorTimeoutMs,reset,Read,Write)\
  Id##_sectorIoResult[NV_SECTOR_ARRAY_SIZE(Size,SectorSize)]
static IO_RESULT NV_DEV_LIST;



//          NV_DevInfo[] allocation and initialization
#undef NV_DEV
#define NV_DEV(Id,Size,SectorSize,Offset,SectorTimeoutMs,reset,Read,Write)\
{Size,SectorSize,Offset,SectorTimeoutMs,reset,Id##_Shadow, Id##_sectorIoResult, Id##_RefCount, &Id##_DevSem, Write, Read}
static const NV_DEV_INFO  NV_DevInfo[NV_MAX_DEV]  = {NV_DEV_LIST};


//          NV_FileInfo[] allocation and initialization
#undef NV_FILE
#define NV_FILE(Id, Name, Primary, Backup, CSum, ResetFlag, Size) {Size, Primary, Backup, CSum, ResetFlag}
static const NV_FILE_INFO NV_FileInfo[NV_MAX_FILE] = {NV_FILE_LIST};


//  PromFileName[] Table of Names for files
//  NV_FileCRC[] Table of CRCs for files ( excluding Program )
#undef NV_FILE
#define NV_FILE(Id, Name, Primary, Backup, CSum, ResetFlag, Size) {Name}
NV_FILENAME PromFileName[NV_MAX_FILE] = {NV_FILE_LIST};
static NV_FILE_CRC NV_FileCRC[NV_MAX_FILE];


//          NV_RuntimeInfo[] allocation
//The runtime file data uses the device and file info tables to pre-compute
//each file's offset at initialization time and speed up runtime operation.
static NV_RUNTIME_INFO NV_RuntimeInfo[NV_MAX_FILE];

//Keep track of the device space actually used when all the files are allocated
//at initialization time.
static INT32 NV_DevSpaceUsed[NV_MAX_DEV];

//#define DEBUG_NVMGR_TASK

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
void  NV_MgrTask(void *pParam);
INT32 NV_FileDevToShadow(NV_FILE_ID File, NV_FILE_TYPE FileType);
INT32 NV_FileShadowToDev(NV_FILE_ID File, NV_FILE_TYPE FileType);
void  NV_CopyBackupToPrimaryShadow(NV_RUNTIME_INFO* File);
void  NV_CopyPrimaryToBackupShadow(NV_RUNTIME_INFO* File);

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/*****************************************************************************
 * Function:    NV_Init
 *
 * Description: Init sets up the initial state for the NV manager.
 *              1. Setup of the SpaceUsed array that tracks the used space
 *                 in files
 *              2. Allocates files into the devices using the table of devices
 *                 and the table of files.  Both of the Device and File tables
 *                 are located in NVMgr.h The DevSpaceUsed var in each device
 *                 is incremented as files are allocated into the
 *                 devices
 *              3. Sets up the task responsible for copying changes to the
 *                 local shadow memory into the non-volatile devices.
 *
 *
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:
 *
 ****************************************************************************/
void NV_Init(void)
{
  TCB     TaskInfo;
  INT32   i;
  INT32   j;
  NV_DEVS_ID primary;

  //Prepare Device data, clear used space to zero
  //Initialize device semaphores
  for(i = 0; i < NV_MAX_DEV; i++)
  {
    NV_DevSpaceUsed[i] = 0;
    NV_DevInfo[i].Sem->Writing = FALSE;
    NV_DevInfo[i].Sem->Written = FALSE;

    for(j = 0; j < NV_SECTOR_ARRAY_SIZE(NV_DevInfo[i].Size, NV_DevInfo[i].SectorSize); ++j )
    {
      NV_DevInfo[i].sectorIoResult[j] = IO_RESULT_READY;
      NV_DevInfo[i].RefCount[j].ShadowCount = 0;
      NV_DevInfo[i].RefCount[j].NvRAMCount  = 0;
    }
  }

  //Initialize the runtime file table from the File and Device tables
  //The offset that a file is located at within a device is tables and
  //stored in the runtime structure for use during runtime.
  for(i = 0; i < NV_MAX_FILE; i++)
  {
    primary = NV_FileInfo[i].PrimaryDevID;

    // ensure the file size is at least one Sector big
    ASSERT_MESSAGE( NV_FileInfo[i].Size >= NV_DevInfo[primary].SectorSize,
      "Bad File Size: %s Size: %d Minimum: %d",
      PromFileName[i], NV_FileInfo[i].Size, NV_DevInfo[primary].SectorSize);

    //Copy file size from the file table.
    NV_RuntimeInfo[i].Size        = NV_FileInfo[i].Size;
    NV_RuntimeInfo[i].CheckMethod = NV_FileInfo[i].CheckMethod;
    NV_RuntimeInfo[i].Init   = NV_FileInfo[i].Init;

    //Initialize the Filename and CRC-value for each file.
    strncpy_safe( NV_FileCRC[i].Name, sizeof(NV_FILENAME), PromFileName[i], _TRUNCATE);
    NV_FileCRC[i].CRC = 0x0000;

    //Allocate the files in to the devices.  Store the file location offsets
    //into the Runtime table.
    if( NV_FileInfo[i].PrimaryDevID != DEV_NONE)
    {
      NV_RuntimeInfo[i].PrimaryOffset = NV_DevSpaceUsed[primary];
      NV_RuntimeInfo[i].PrimaryDev = &NV_DevInfo[primary];

      NV_DevSpaceUsed[primary] += NV_FileInfo[i].Size;

      ASSERT( NV_DevSpaceUsed[primary] <= NV_RuntimeInfo[i].PrimaryDev->Size);

      if ( NV_FileInfo[i].BackupDevID != DEV_NONE)
      {
        NV_DEVS_ID backup = NV_FileInfo[i].BackupDevID;

        NV_RuntimeInfo[i].BackupOffset = NV_DevSpaceUsed[backup];
        NV_RuntimeInfo[i].BackupDev = &NV_DevInfo[backup];

        NV_DevSpaceUsed[backup] += NV_FileInfo[i].Size;

        ASSERT( NV_DevSpaceUsed[ backup] <= NV_RuntimeInfo[i].BackupDev->Size);
      }
    }
  }

  // Create the NV Manager Task
  memset(&TaskInfo, 0, sizeof(TaskInfo));
  strcpy(TaskInfo.Name, "NV Mem Manager");
  TaskInfo.TaskID         = NV_Mem_Manager;
  TaskInfo.Function       = NV_MgrTask;
  TaskInfo.Priority       = taskInfo[NV_Mem_Manager].priority;
  TaskInfo.Type           = taskInfo[NV_Mem_Manager].taskType;
  TaskInfo.modes          = taskInfo[NV_Mem_Manager].modes;
  TaskInfo.MIFrames       = taskInfo[NV_Mem_Manager].MIFframes;
  TaskInfo.Rmt.InitialMif = taskInfo[NV_Mem_Manager].InitialMif;
  TaskInfo.Rmt.MifRate    = taskInfo[NV_Mem_Manager].MIFrate;
  TaskInfo.Enabled        = TRUE;
  TaskInfo.Locked         = FALSE;
  TaskInfo.pParamBlock    = NULL;

  TmTaskCreate(&TaskInfo);
}



/*****************************************************************************
 * Function:    NV_Open
 *
 * Description: Open reads the file from the primary device and backup device
 *              (if a backup file is defined) into the local shadow copies.
 *              The file verification checks the 16-bit CRC of each file,
 *              and if both files are valid an identical
 *
 * Parameters:  NV_FILE_ID: File ID of the file to open
 *
 * Returns:     SYS_OK: File opened successfully
 *              SYS_NV_FILE_OPEN_ERR: Could not open a valid copy of the file.
 *                                    Calling application should re-write the
 *                                    file to default values to make it valid
 *                                    again.
 *
 * Notes: Open SHALL only be called during system initialization.  Open
 *        does not handle Task Manager preemption.
 *
 ****************************************************************************/
RESULT NV_Open(NV_FILE_ID FileID)
{
  INT32 PrimaryCRC, BackupCRC;
  RESULT result = SYS_OK;
  CHAR   debugBuffer[128];
  NV_FILE_OPEN_ERROR_LOG       fileErrorLog;
  NV_PRI_BACKUP_OPEN_ERROR_LOG priBkupErrorLog;

  ASSERT_MESSAGE(!NV_RuntimeInfo[FileID].IsOpen,"NVMgr: Attempt to reopen %s",
      NV_GetFileName(FileID));
  NV_RuntimeInfo[FileID].IsOpen = TRUE;
  PrimaryCRC = NV_FileDevToShadow( FileID, NV_PRIMARY);

  //Check if a Backup file is defined, and if so, open it.
  //Else set the BackupCRC to PrimaryCRC for the dual-redundant logic
  if(NV_RuntimeInfo[FileID].BackupDev != NULL)
  {
    BackupCRC = NV_FileDevToShadow( FileID, NV_BACKUP);
  }
  else
  {
    BackupCRC = PrimaryCRC;
  }


  NV_FileCRC[FileID].CRC = PrimaryCRC; // Assume go with Primary, adjust as
  // required by decision tree below.

  //Primary and backup decision tree.  Validate the Primary file copy and the
  //backup file copy have good CRC sums.  Also validate that those CRCs match
  //an therefore the is a high likelihood that the files match also.  Bad CRC
  //and other discrepancies are handled per the comments below.
  if(-1 != PrimaryCRC)
  {
    if(-1 != BackupCRC)
    {
      if(PrimaryCRC == BackupCRC)
      {
        //Best-case scenario, both copies are valid and CRCs match.
        result = SYS_OK;
      }
      else
      {
        //Primary and Backup valid, but do not match. Copy Primary to Backup
        priBkupErrorLog.FileID    = FileID;
        priBkupErrorLog.PriCRC    = PrimaryCRC;
        priBkupErrorLog.BackupCRC = BackupCRC;
        strncpy_safe(priBkupErrorLog.Name, sizeof(priBkupErrorLog.Name),
            (CHAR*)NV_GetFileName(FileID), _TRUNCATE);

        LogWriteSystem(SYS_NVM_PRIMARY_NEQ_BACKUP_FILE,
            LOG_PRIORITY_LOW,
            &priBkupErrorLog,
            sizeof(NV_PRI_BACKUP_OPEN_ERROR_LOG),
            NULL);

        snprintf(debugBuffer, sizeof(debugBuffer),
            "NVMgr: File Id 0x%02x (%s): Primary != Backup,"\
            " restoring..",FileID, NV_GetFileName(FileID) );
        GSE_DebugStr(NORMAL,TRUE,debugBuffer);

        NV_CopyPrimaryToBackupShadow(&NV_RuntimeInfo[FileID]);
        if(0==NV_FileShadowToDev( FileID, NV_BACKUP))
        {
          GSE_StatusStr(NORMAL," OK!");
        }
        else
        {
          GSE_StatusStr(NORMAL," FAILED");
        }
        result = SYS_OK;
      }
    }
    else
    {
      //Backup CRC fail, Copy Primary to Backup
      snprintf(debugBuffer, sizeof(debugBuffer),
          "NVMgr: File Id 0x%02x (%s): Backup CRC failed"\
          " restoring..",FileID, NV_GetFileName(FileID) );
      GSE_DebugStr(NORMAL,TRUE,debugBuffer);

      fileErrorLog.FileID = FileID;
      fileErrorLog.CRC    = BackupCRC;
      strncpy_safe(fileErrorLog.Name, sizeof(fileErrorLog.Name),
          (CHAR*)NV_GetFileName(FileID), _TRUNCATE);

      LogWriteSystem(SYS_NVM_BACKUP_FILE_BAD, LOG_PRIORITY_LOW,
          &fileErrorLog, sizeof(NV_FILE_OPEN_ERROR_LOG), NULL);

      NV_CopyPrimaryToBackupShadow(&NV_RuntimeInfo[FileID]);
      if(0 == NV_FileShadowToDev( FileID, NV_BACKUP))
      {
        GSE_StatusStr(NORMAL," OK!");
      }
      else
      {
        GSE_StatusStr(NORMAL," FAILED");
      }
      result = SYS_OK;
    }
  }
  else
  {
    if(-1 != BackupCRC)
    {
      //Primary CRC fail, backup okay, Copy Backup to Primary
      fileErrorLog.FileID = FileID;
      fileErrorLog.CRC    = BackupCRC;
      strncpy_safe(fileErrorLog.Name, sizeof(fileErrorLog.Name),
          (CHAR*)NV_GetFileName(FileID), _TRUNCATE);
      LogWriteSystem(SYS_NVM_PRIMARY_FILE_BAD,
          LOG_PRIORITY_LOW,
          &fileErrorLog,
          sizeof(NV_FILE_OPEN_ERROR_LOG),
          NULL);

      snprintf(debugBuffer, sizeof(debugBuffer),
          "NVMgr: File Id 0x%02x (%s): Primary CRC failed,"\
          " restoring..",FileID, NV_GetFileName(FileID) );

      GSE_DebugStr(NORMAL,TRUE,debugBuffer);

      NV_CopyBackupToPrimaryShadow(&NV_RuntimeInfo[FileID]);
      if(0 == NV_FileShadowToDev( FileID, NV_PRIMARY))
      {
        GSE_StatusStr(NORMAL," OK!");
      }
      else
      {
        GSE_StatusStr(NORMAL," FAILED");
      }

      NV_FileCRC[FileID].CRC = BackupCRC;

    }
    else
    {
      //Uh-oh, both copies failed.  Return file open error to caller
      priBkupErrorLog.FileID    = FileID;
      priBkupErrorLog.PriCRC    = PrimaryCRC;
      priBkupErrorLog.BackupCRC = BackupCRC;
      strncpy_safe(priBkupErrorLog.Name, sizeof(priBkupErrorLog.Name),
          (CHAR*)NV_GetFileName(FileID), _TRUNCATE);

      LogWriteSystem(SYS_NVM_PRIMARY_BACKUP_FILES_BAD,
          LOG_PRIORITY_LOW,
          &priBkupErrorLog,
          sizeof(NV_PRI_BACKUP_OPEN_ERROR_LOG),
          NULL);

      snprintf(debugBuffer, sizeof(debugBuffer),
          "NVMgr: File Id 0x%02x (%s): Primary and Backup"\
          " CRC failed",FileID, NV_FileCRC[FileID].Name);
      GSE_DebugStr(NORMAL,TRUE,debugBuffer);

      result = SYS_NV_FILE_OPEN_ERR;
    }
  }
  return result;
}

/*****************************************************************************
* Function:    NV_Close
*
* Description: Close sets the open flag to false
*
* Parameters:  NV_FILE_ID: File ID of the file to open
*
* Returns:     none
*
* Notes:       none
*
****************************************************************************/
void NV_Close(NV_FILE_ID FileID)
{
  NV_RuntimeInfo[FileID].IsOpen = FALSE;
}

/*****************************************************************************
 * Function:    NV_Read
 *
 * Description: Read data from a file.  The data is returned from the primary
 *              shadow copy.  The dirty bits for the sectors read are tested,
 *              and if the sectors are dirty, the function return value
 *              will indicate "NV_WRITE_PENDING" that.  In this case, the
 *              data returned is still valid, however it has not yet been
 *              written to the external memory.
 *              This return status is useful for confirming if data written
 *              to the shadow copy has been copied to the external NV memory.
 *
 *
 *
 * Parameters:  [in] FileID: ID of the file to read. The file must be open
 *              [in] Offset: Offset to read,in bytes,from the start of the file
 *              [out] Buf: Pointer to a location to store the read data
 *              [in] Size: Number of bytes to read
 *
 * Returns:     None
 *
 *
 * Notes:
 *
 ****************************************************************************/
void NV_Read(NV_FILE_ID FileID, UINT32 Offset, void* Buf, UINT32 Size)
{
  NV_RUNTIME_INFO* File;

  File = &NV_RuntimeInfo[FileID];

  //File Open Check
  ASSERT_MESSAGE( File->IsOpen == TRUE, "%s: Attempt read on closed file.",
    PromFileName[FileID]);

  //Boundary Check
  ASSERT_MESSAGE( (Offset+Size) <= (File->Size-NV_CRC_SIZE),
    "%s: read %d bytes beyond EOF",
    PromFileName[FileID], (File->Size-NV_CRC_SIZE) - (Offset+Size));

  //Copy data back to caller
  memcpy(Buf, &File->PrimaryDev->ShadowMem[ File->PrimaryOffset+Offset ],
         Size);


}

/******************************************************************************
 * Function:    NV_Write
 *
 * Description: Writes a block of data to a file at a specific offset.  Data
 *              is copied to the primary and backup (if a backup is defined)
 *              device, and the ShadowRam ref count for the written sectors are
 *              incremented.
 *              The new CRC value of the file is written to the last 2 bytes
 *              of the file by the write routine.  This routine does not
 *              directly access the external NV memory, therefore the overhead
 *              should be deterministic.
 *
 * Parameters:  [in] FileID: ID of the file to write
 *              [in] Offset: Offset, in bytes, to write the block to from the
 *                           start if the file
 *              [in] Data:   Pointer to the block of data to write
 *                           If NULL and Size == 0, reset the file
 *              [in] Size:   Size of the data block to write
 *                           If 0 and Data == NULL0, reset the file
 *
 * Returns:     None
 *
 * Notes:       The last two bytes of a file are reserved for CRC information
 *              and cannot be written by the calling function.
 *
 *              THIS FUNC IS NOT PROTECTED AGAINST INTERRUPT FILE ACCESS OF
 *              THE SAME FILE.  IT IS EXPECTED THE CALLING APPLICATION WILL
 *              MANAGE SINGLE ACCESS TO A FILE.  MULT (INTERRUPTED)
 *              ACESSS TO MULT FILES IS ALLOWED.  JUST NOT MULT (INTERRUPTED)
 *              ACCESS TO THE SAME FILE
 *
 *****************************************************************************/
void NV_Write(NV_FILE_ID FileID, UINT32 Offset, const void* Data, UINT32 Size)
{
  UINT32 i;
  UINT32 FirstSector;
  UINT32 SectorsWritten;
  UINT32 CRCOffset;
  UINT32 CRCSector;
  UINT16 CRC; // temp variable for calculated CRC or CSUM.
  NV_RUNTIME_INFO* File;

  BOOLEAN resetting = FALSE;

  File = &NV_RuntimeInfo[FileID];

  //File Open Check
  ASSERT_MESSAGE( File->IsOpen == TRUE, "%s: Attempt write on closed file.",
                  PromFileName[FileID]);

  //Boundary Check
  ASSERT_MESSAGE( (Offset+Size) <= (File->Size-NV_CRC_SIZE),
    "%s: write %d bytes beyond EOF",
    PromFileName[FileID], (File->Size-NV_CRC_SIZE) - (Offset+Size));

  // Check special case of file reset
  // caller requested a reset of the entire area offset holds
  if ( Data == NULL && Size == 0 && File->Init != NULL )
  {
    UINT8 reset = File->PrimaryDev->resetValue;

    memset( &File->PrimaryDev->ShadowMem[ File->PrimaryOffset], reset, File->Size);
    if(File->BackupDev != NULL)
    {
        memset( &File->BackupDev->ShadowMem[ File->BackupOffset],  reset, File->Size);
    }
    Size = File->Size - NV_CRC_SIZE;
    resetting = TRUE;
  }

  //Critical section. Order of operations here is very important.  The
  //ShadowRam Ref counts are only incremented after all changes have been
  //written to shadow memory. Otherwise, the NV write task may interrupt
  //and clear the dirty bits before changes are written to flash.
  File->PrimaryDev->Sem->Writing = TRUE;
  if (!resetting)
  {
      memcpy(&File->PrimaryDev->ShadowMem[ File->PrimaryOffset+Offset ],Data, Size);
  }

  //Find the first sector written to
  FirstSector = (File->PrimaryOffset+Offset) / File->PrimaryDev->SectorSize;

  // Compute the number of times the written block crosses a sector boundary.
  // This will be the total number of sectors that need to be flagged.

  SectorsWritten = 1 +
    ((File->PrimaryOffset+Offset+Size)/File->PrimaryDev->SectorSize) -
    ((File->PrimaryOffset+Offset)/File->PrimaryDev->SectorSize);

  for( i = FirstSector; i < (FirstSector + SectorsWritten); i++)
  {
    ++File->PrimaryDev->RefCount[i].ShadowCount;
  }

  //Write CRC to the last 2 bytes of the file.

  if (resetting)
  {
    // When initializing the file, use the reset value as the CRC
    CRC = File->PrimaryDev->resetValue;
  }
  else
  {
    CRC =  CalculateCheckSum( File->CheckMethod,
                          &File->PrimaryDev->ShadowMem[ File->PrimaryOffset ],
                          File->Size - NV_CRC_SIZE);
  }

  CRCOffset = File->PrimaryOffset+File->Size-NV_CRC_SIZE;
  CRCSector = (CRCOffset / File->PrimaryDev->SectorSize);

  *(UINT16*)&File->PrimaryDev->ShadowMem[CRCOffset] = CRC;


  // Update the CRC value in the file CRC table.
  NV_FileCRC[FileID].CRC = CRC;

  // Flag the sector containing the CRC for updating.
  ++File->PrimaryDev->RefCount[CRCSector].ShadowCount;

  File->PrimaryDev->Sem->Writing = FALSE;
  File->PrimaryDev->Sem->Written = TRUE;

  //*************** Update Backup copy ****************

  if(File->BackupDev != NULL)
  {
    // Critical section.  Order of operations here is very important.  The
    // Write counts must only be updated after all changes have been written to
    // shadow memory or the NV write task may interrupt and queue writes before
    // changes are written
    File->BackupDev->Sem->Writing = TRUE;
    if (!resetting)
    {
        memcpy(&File->BackupDev->ShadowMem[ File->BackupOffset+Offset ],Data, Size);
    }

    //Find the first sector written to
    FirstSector = (File->PrimaryOffset+Offset) / File->PrimaryDev->SectorSize;

    // Compute the number of times the written block crosses a sector boundary.
    // This will be the total number of sectors that need to be flagged.
    SectorsWritten = 1 +
      ((File->BackupOffset+Offset+Size)/File->BackupDev->SectorSize) -
      ((File->BackupOffset+Offset)/File->BackupDev->SectorSize);

    for( i = FirstSector; i < (FirstSector + SectorsWritten); i++)
    {
       ++File->BackupDev->RefCount[i].ShadowCount;
    }

    //Write CRC to the last 2 bytes of the file
    CRCOffset = File->BackupOffset+File->Size-NV_CRC_SIZE;
    CRCSector = (CRCOffset / File->BackupDev->SectorSize);

    *(UINT16*)&File->BackupDev->ShadowMem[CRCOffset] = CRC;

    // All Writes Done: Flag the sector containing the CRC for updating.
    ++File->BackupDev->RefCount[CRCSector].ShadowCount;

    File->BackupDev->Sem->Writing = FALSE;
    File->BackupDev->Sem->Written = TRUE;
  }
}

/******************************************************************************
 * Function:    NV_WriteNow
 *
 * Description: Copies a block of data to the shadow memory and to the
 *              external NV device immediately.  This function should be used
 *              for time-critical shutdown or startup tasks only.
 *              All other writes should use the normal write function.
 *
 *              The primary purpose is for writing data on power down,
 *              and therefore the first priority of this function is to update
 *              the primary file.  The data is copied to shadow memory, the
 *              CRC is updated and the change data and CRC are written out to
 *              the device. The second priority, if time allows, is to also
 *              update the backup.
 *
 * Parameters:  [in] FileID: ID of the file to write
 *              [in] Offset: Offset, in bytes, to write the block to from the
 *                           start if the file
 *              [in] Data:   Pointer to the block of data to write
 *              [in] Size:   Size of the data block to write
 *
 * Returns:     SYS_OK:              Write successful, data has been copied
 *                                   to the shadow.
 * Notes:
 *
 *****************************************************************************/
RESULT NV_WriteNow(NV_FILE_ID FileID, UINT32 Offset, void* Data, UINT32 Size)
{
  INT32  ShadowOffset, PhysicalOffset;
  INT32  SizeWritten = 0, TotalWritten = 0;
  UINT32 CRCOffset;
  UINT8*  ShadowMem;
  NV_DEV_WRITE_FUNC WriteFunc;
  NV_FILE_TYPE FileType;
  NV_RUNTIME_INFO* File;
  RESULT result = SYS_OK;
  UINT16 CRC;

  File = &NV_RuntimeInfo[FileID];

  //File Open Check
  ASSERT_MESSAGE( File->IsOpen == TRUE, "%s: Attempt write on closed file.",
    PromFileName[FileID]);

  //Boundary Check
  ASSERT_MESSAGE( (Offset+Size) <= (File->Size-NV_CRC_SIZE),
    "%s: write %d bytes beyond EOF",
    PromFileName[FileID], (File->Size-NV_CRC_SIZE) - (Offset+Size));

  for(FileType = NV_PRIMARY; FileType < NV_FILE_TYPE_MAX; FileType++)
  {
    //Setup pointers to the requested device, either Primary or Backup
    if(NV_PRIMARY == FileType)
    {
      ShadowOffset   = File->PrimaryOffset;
      PhysicalOffset = File->PrimaryOffset + File->PrimaryDev->Offset;
      ShadowMem      = File->PrimaryDev->ShadowMem;
      WriteFunc      = File->PrimaryDev->Write;
    }//Do backup only if a backup is defined
    else if(File->BackupDev != NULL)
    {
      ShadowOffset   = File->BackupOffset;
      PhysicalOffset = File->BackupOffset + File->BackupDev->Offset;
      ShadowMem      = File->BackupDev->ShadowMem;
      WriteFunc      = File->BackupDev->Write;
    }
    else
    {//Not the primary and no backup defined.
      break;
    }

    //Copy data and CRC to shadow
    memcpy(&ShadowMem[ShadowOffset+Offset],Data,Size);

    CRC = CalculateCheckSum( File->CheckMethod,
                             &ShadowMem[ShadowOffset],
                             File->Size-NV_CRC_SIZE );

    CRCOffset = ShadowOffset+File->Size-NV_CRC_SIZE;
    *(UINT16*)&ShadowMem[CRCOffset] = CRC;

    // Instruct the SpiManager to stop processing thru queues and send
    // subsequent writes directly to NVRAM.
    SPIMgr_SetModeDirectToDevice();

    //Write data to External NV memory
    for(TotalWritten = 0;TotalWritten < Size;)
    {
      SizeWritten = WriteFunc(&ShadowMem[ShadowOffset+Offset+TotalWritten],
          PhysicalOffset+Offset+TotalWritten,Size-TotalWritten,NULL);
      //If write fatal error
      if(-1 == SizeWritten)
      {
        result = SYS_NV_WRITE_DRIVER_FAIL;
        break;
      }
      TotalWritten += SizeWritten;
    }

    //Write CRC to EExternal NV memory
    for(TotalWritten = 0;TotalWritten < NV_CRC_SIZE;)
    {
      SizeWritten = WriteFunc(&ShadowMem[ShadowOffset+File->Size-NV_CRC_SIZE],
                              PhysicalOffset+File->Size-NV_CRC_SIZE+TotalWritten,
                              NV_CRC_SIZE-TotalWritten,NULL);
      //If write fatal error
      if(-1 == SizeWritten)
      {
        result = SYS_NV_WRITE_DRIVER_FAIL;
        break;
      }
      TotalWritten += SizeWritten;
    }
  }

  return result;
}

/******************************************************************************
* Function:    NV_WriteState
*
* Description:
*
* Parameters:  [in] FileID: ID of the file to write
*              [in] Offset: Offset, in bytes, to write the block to from the
*                           start if the file*
*              [in] Size:   Size of the data block to write
*
* Returns:     SYS_OK:              Write successful, data has been copied
*                                   to the shadow.
*              SYS_NV_WRITE_PENDING Write operation is pending.
* Notes:
*
*****************************************************************************/
/*
RESULT NV_WriteState(NV_FILE_ID FileID, UINT32 Offset, UINT32 Size)
{
  NV_RUNTIME_INFO* File;
  INT32 SectorsWritten, FirstSector,i,j;
  RESULT result = SYS_OK;

  //Check dirty bits in the primary file
  FirstSector = (File->PrimaryOffset + Offset) / File->PrimaryDev->SectorSize;
  SectorsWritten = 1 +
    ((File->PrimaryOffset + Offset + Size) / File->PrimaryDev->SectorSize) -
    ((File->PrimaryOffset + Offset) / File->PrimaryDev->SectorSize);

  DirtyBitByteOffset = FirstSector / 8;

  //Turn the Bit Offset into a bit mask.
  j = BitToBitMask(FirstSector % 8);

  //Search for set dirty bits in the section read by the caller
  for( i = 0; i < SectorsWritten; i++)
  {
    if(File->PrimaryDev->DirtyBits[DirtyBitByteOffset] & j)
    {
      //If a set dirty bit is found, set a flag and exit the search loop
      result = SYS_NV_WRITE_PENDING;
      break;
    }
    j = j << 1;

    //Increment to the next byte on once the last bit has been tested
    if(j == 256)
    {
      DirtyBitByteOffset++;
      j = 1;
    }
  }
  return result;
}
*/

/*****************************************************************************
* Function:    NV_Erase
*
* Description: Erases the file from the primary device and backup device
*              (if a backup file is defined) into the local shadow copies.
*              The file verification checks the 16-bit CRC of each file,
*              and if both files are valid an identical
*
* Parameters:  NV_FILE_ID: File ID of the file to erase
*
* Returns:     none
*
* Notes:       None
*
****************************************************************************/
void NV_Erase(NV_FILE_ID FileID)
{
  GSE_DebugStr( NORMAL,TRUE,"NVMgr: Reset %s.", NV_GetFileName(FileID));
  NV_Write( FileID, 0, NULL, 0);
}

/*****************************************************************************
* Function:    NV_ResetConfig
*
* Description: Erases all resettable files from the primary device and backup
*              device (if a backup file is defined) into the local shadow copies.
*
* Parameters:  none
*
* Returns:     none
*
* Notes:       none
*
****************************************************************************/
void NV_ResetConfig(void)
{
  INT16 i;
  BOOLEAN forcedOpen;
  NV_RUNTIME_INFO* File;

#ifndef WIN32
  // Array of files id's representing file-writes awaiting completion to NVRAM
  BOOLEAN FilesWriteInProgress[NV_MAX_FILE];
  INT8 fileBusyCnt = 0;
#endif

  for(i = 0; i < NV_MAX_FILE; ++i)
  {
    //Initialize the forced-open tracking flag.
    forcedOpen = FALSE;

    File = &NV_RuntimeInfo[i];

#ifndef WIN32
    // Initialize the file as not having a write-in-progress
    FilesWriteInProgress[i] = FALSE;
#endif

    // If this file is configured to be
    // cleared during a reset, do it!
    if( NULL != File->Init )
    {
      // If the FILE is not yet opened, try to open it now and flag it as
      // "forced open" so it can be closed after completion of reset.
      if (!File->IsOpen)
      {
        forcedOpen = (NV_Open((NV_FILE_ID)i) == SYS_OK) ? TRUE : FALSE;
      }
      // Erase the file content
      NV_Erase((NV_FILE_ID)i);

      // If the erase succeeded (func always return OK), call the specific
      // initialization function for this file
      // per NV_FILE_LIST macro
      File->Init();
#ifndef WIN32
      FilesWriteInProgress[i] = TRUE;
      fileBusyCnt++;
#endif

      if(forcedOpen == TRUE)
      {
        NV_Close( (NV_FILE_ID)i );
      }
    }
  }

// The following block won't work in G4E emulation  because of a deadly-embrace
// in emulation:
//
// NV_MgrTask will not have an opportunity to run and performs EEPROM writes
// until GSE command task/thread exits.
//
// 'This' function is called in the GSE command task and the following block would wait for
// NV_MgrTask/thread to finish writing to "EEPROM" before exiting.

#ifndef WIN32
  // Wait for any/all File writes to complete to EEPROM before returning
  while(fileBusyCnt > 0 )
  {
    // Reset the count to zero each time, loop terminates if count of busy-files stays at zero.
    fileBusyCnt = 0;
    for (i = 0; i < NV_MAX_FILE; ++i)
    {
      // if the file is flagged as being written-to, see if still busy
      if( TRUE == FilesWriteInProgress[i] )
      {
        if( SYS_NV_WRITE_PENDING == NV_WriteState( (NV_FILE_ID)i, 0, 0) )
        {
          // Still writing, add to tally
          ++fileBusyCnt;
        }
        else
        {
          // This file's EEPROM has been written, flag it.
          FilesWriteInProgress[i] = FALSE;
        }
      }
    }// for all files
  }// while
#endif
  //GSE_DebugStr(OFF,TRUE, "All resets committed to NV Memory");
}

/******************************************************************************
 * Function:    NV_ReadDev
 *
 * Description: Low-level raw read of data from an NV storage device.
 *              ONLY TO BE USED FOR DEBUG AND TESTING
 *
 * Parameters:  [in] DevID: ID of the file to write
 *              [out]Data:  Pointer to a buffer to receive the data to.
 *              [in] Offset:Location to start read
 *              [in] Size:  Number of bytes to read
 *
 * Returns:     SYS_OK:                   Erase successful
 *              SYS_NV_READ_DRIVER_FAIL: Erase fail
 * Notes:
 *
 *****************************************************************************/
/*
RESULT NV_ReadDev(NV_DEVS_ID DevID, void* Data, UINT32 Offset, UINT32 Size)
{
 INT32  TotalRead = 0,SizeRead = 0;
 RESULT result = SYS_OK;

  //Read data from external device.
  for(TotalRead = 0;TotalRead < Size;)
  {
    SizeRead = NV_DevInfo[DevID].Read(Data,
        NV_DevInfo[DevID].Offset+Offset+TotalRead,
        Size-TotalRead);

    //If write fatal error
    if(-1 == SizeRead)
    {
      result = SYS_NV_WRITE_DRIVER_FAIL;
      break;
    }
    TotalRead += SizeRead;
  }
  return result;
}
*/


/******************************************************************************
 * Function:    NV_WriteDev
 *
 * Description: Low-level raw read of write from an NV storage device.
 *              ONLY TO BE USED FOR DEBUG AND TESTING
 *
 * Parameters:  [in] DevID: ID of the file to write
 *              [out]Data:  Pointer to a buffer to receive the data to.
 *              [in] Offset:Location to start read
 *              [in] Size:  Number of bytes to read
 *
 * Returns:     SYS_OK:                   Erase successful
 *              SYS_NV_READ_DRIVER_FAIL: Erase fail
 * Notes:
 *
 *****************************************************************************/
 /*
RESULT NV_WriteDev(NV_DEVS_ID DevID, void* Data, UINT32 Offset, UINT32 Size)
{
 INT32  TotalWritten = 0,SizeWritten = 0;
 RESULT result = SYS_OK;

  //Read data from external device.
  for(TotalWritten = 0;TotalWritten < Size;)
  {
    SizeWritten = NV_DevInfo[DevID].Write(Data,
        NV_DevInfo[DevID].Offset+Offset+TotalWritten,
        Size-TotalWritten);

    //If write fatal error
    if(-1 == SizeWritten)
    {
      result = SYS_NV_WRITE_DRIVER_FAIL;
      break;
    }
    TotalWritten += SizeWritten;
  }
  return result;
}
*/



/******************************************************************************
 * Function:    NV_EERead
 *
 * Description: Device read function.  See NVMGR_DEV_READ_FUNC typedef.
 *              This function interfaces the SPI EEPROM device.
 *              See the NV_READ_FUNC typedef for functional description.
 *
 * Parameters:  [in] DestAddr: Pointer to a block to receive the read data
 *              [in] PhyOffset:  Offset to start read from within this device
 *              [in} Size:       Requested number of bytes to read
 *
 * Returns:     >= 0: Size read
 *              -1:   Fatal device error.
 *
 * Notes:       See the NV_READ_FUNC typedef for functional description.
 *
 *****************************************************************************/
INT32 NV_EERead(void* DestAddr, UINT32 PhysicalOffset, UINT32 Size)
{
  INT32 Read;
  RESULT ReadResult;

  ReadResult = SPIMgr_ReadBlock(PhysicalOffset, DestAddr, Size);

  if(DRV_OK == ReadResult )
  {
    Read = Size;
  }
  else if(DRV_EEPROM_NOT_DETECTED == ReadResult)
  {
    Read = -1;
  }
  else
  {
    Read = 0;
  }

  return Read;
}
/******************************************************************************
 * Function:    NV_EEWrite
 *
 * Description: Device write function. See NVMGR_DEV_WRITE_FUNC typedef.
 *              This function interfaces the SPI EEPROM device
 *
 *
 * Parameters:  [in] SourceAddr: Pointer to the block to be written
 *              [in] PhyOffset:  Offset to write within this device
 *              [in] Size:       Requested number of bytes to write
 *              [in] ioResult:   Pointer to a enum passed by caller to
 *                               signal the completion status of the write.
 *
 * Returns:     >= 0: Size written
 *              -1:   Fatal device error.
 *
 * Notes:       See the NV_WRITE_FUNC typedef for functional description.
 *
 *****************************************************************************/
INT32 NV_EEWrite(void* SourceAddr, UINT32 PhysicalOffset, UINT32 Size, IO_RESULT* ioResult)
{
  const UINT32 PageMask = EEPROM_PAGE_WRITE_SIZE-1;

  INT32 Written;
  UINT32 NextPageBoundary;
  RESULT WriteResult;

  //For the EEPROM, write operations cannot cross page boundaries.  Page
  //size is defined by the EEPROM driver.  For this function, if a write
  //goes over 1 or more boundaries, write as much data as possible, up to
  //the first boundary, then return the number of bytes actually written

  //If the PhysicalOffset and the PhysicalOffset + Size crosses a page,
  //then only write up to the end of the first page.
  if((PhysicalOffset & ~PageMask) != ((PhysicalOffset + Size-1) & ~PageMask))
  {
    NextPageBoundary = (PhysicalOffset & ~PageMask) + EEPROM_PAGE_WRITE_SIZE;
    Size = NextPageBoundary - PhysicalOffset;
  }

  WriteResult = SPIMgr_WriteEEPROM(PhysicalOffset, SourceAddr, Size, ioResult);

  if(DRV_OK == WriteResult)
  {
    Written = Size;
  }
  else if(DRV_EEPROM_NOT_DETECTED == WriteResult)
  {
    Written = -1;
  }
  else
  {
    Written = 0;
  }
  return Written;
}



/******************************************************************************
 * Function:    NV_RTCRead
 *
 * Description: Device read function.  See NVMGR_DEV_READ_FUNC typedef.
 *              This function interfaces the SPI EEPROM device.
 *              See the NV_READ_FUNC typedef for functional description.
 *
 * Parameters:  [in] DestAddr: Pointer to a block to receive the read data
 *              [in] PhyOffset:  Offset to start read from within this device
 *              [in} Size:       Requested number of bytes to read
 *
 * Returns:     >= 0: Size read
 *              -1  : Fatal device error.
 *
 * Notes:       See the NV_READ_FUNC typedef for functional description.
 *
 *****************************************************************************/
INT32 NV_RTCRead(void* DestAddr, UINT32 PhysicalOffset, UINT32 Size)
{
  INT32 Read;
  RESULT ReadResult;

  ReadResult = RTC_ReadNVRam(PhysicalOffset, DestAddr, Size);
  if(DRV_OK == ReadResult)
  {
    Read = Size;
  }
  else
  {
    Read = -1;
  }

  return Read;
}



/******************************************************************************
 * Function:    NV_RTCWrite
 *
 * Description: Device write function. See NVMGR_DEV_WRITE_FUNC typedef.
 *              This function interfaces the SPI EEPROM device
 *
 *
 * Parameters:  [in] SourceAddr: Pointer to the block to be written
 *              [in] PhyOffset:  Offset to write within this device
 *              [in} Size:       Requested number of bytes to write
 *              [in] ioResult:   Pointer to a enum passed by caller to
 *                               signal the completion status of the write.
 *
 * Returns:     >= 0: Size written
 *              -1  : Fatal device error.
 *
 * Notes:       See the NV_WRITE_FUNC typedef for functional description.
 *
 *****************************************************************************/
INT32  NV_RTCWrite(void* SourceAddr, UINT32 PhysicalOffset, UINT32 Size, IO_RESULT* ioResult)
{
 // const UINT32 PageMask = EEPROM_PAGE_WRITE_SIZE-1;
 // UINT32 NextPageBoundary;
 INT32 Written;
 RESULT WriteResult;

  // Written = -1;
  Written = 0; 

  WriteResult = SPIMgr_WriteRTCNvRam(PhysicalOffset,SourceAddr,Size, ioResult);
  if(DRV_OK == WriteResult)
  {
    Written = Size;
  }

  return Written;
}


/******************************************************************************
 * Function:    NV_GetFileName
 *
 * Description: Returns a string pointer of the name for the passed file
 *              indicator.
 *
 *
 * Parameters:  [in] file number::
 *
 *
 * Returns:     pointer to the file name or NULL if the passed file # is
 *              out of range.
 *
 *****************************************************************************/
NV_FILENAME* NV_GetFileName(NV_FILE_ID fileNum)
{
   NV_FILENAME* fileName = NULL;
   if (fileNum < NV_MAX_FILE)
   {
      fileName = (NV_FILENAME *) NV_FileCRC[fileNum].Name;
   }
   return fileName;
}

/******************************************************************************
 * Function:    NV_GetFileCRC
 *
 * Description: Returns the CRC of the name for the passed file
 *              indicator.
 *
 *
 * Parameters:  [in] file number:
 *
 *
 * Returns:     String pointer to the file name or NULL if the passed file # is
 *              out of range.
 *
 *****************************************************************************/
INT32 NV_GetFileCRC(NV_FILE_ID fileNum)
{
   INT32 Crc = 0x0000;
   if (fileNum < NV_MAX_FILE)
   {
      Crc = NV_FileCRC[fileNum].CRC;
   }
   return Crc;
}

/******************************************************************************
 * Function:    NV_WriteState
 *
 * Description:
 *
 * Parameters:  [in] FileID: ID of the file to to check for pending write
 *                           operations.
 *              [in] Offset: Offset, in bytes marking the starting point to
 *                            check for pending Writes
 *              [in] Size:   Size of the data block to to check for pending
 *                           writes.
 * Returns:     SYS_OK:      All writes to EE are completed for the specified
 *                           memory area
 *
 *              SYS_NV_WRITE_PENDING Write operation is pending.
 * Notes:       To check all sectors in a file, specified a size of zero.
 *
 *
 *****************************************************************************/
RESULT NV_WriteState( NV_FILE_ID fileNum, UINT32 Offset, UINT32 Size )
{
  UINT32 firstSector;
  UINT32 endSector;
  UINT32 i;
  BOOLEAN writesComplete = TRUE;
  NV_RUNTIME_INFO* File;

  File = &NV_RuntimeInfo[fileNum];

  // If size is zero,
  // or the offset + the size exceeds the end of the file,
  // just check the entire file.
  if (Size == 0 || ( Offset + Size) > File->Size )
  {
    Offset = 0;
    Size   = File->Size;
  }

  //================================
  // Test file sectors on PRIMARY DEV
  //================================
  // Calculate the first sector for this file on the device
  firstSector = (File->PrimaryOffset + Offset) / File->PrimaryDev->SectorSize;
  endSector  = (File->PrimaryOffset + Offset + Size) / File->PrimaryDev->SectorSize;

  // Check the ready state of all sectors OR if the reference counts are different, the
  // sector has changed but not yet been enqueued for writing.
  // quit if a pending-busy/busy sector is detected.
  for (i = firstSector; i < endSector; ++i)
  {
    if( IO_RESULT_READY != File->PrimaryDev->sectorIoResult[i] ||
        File->PrimaryDev->RefCount[i].NvRAMCount != File->PrimaryDev->RefCount[i].ShadowCount)
    {
      writesComplete = FALSE;
      break;
    }
  }

  //================================
  // Test file sectors on BACKUP DEV
  //================================
  if (writesComplete && File->BackupDev != NULL)
  {
    firstSector = (File->BackupOffset + Offset) / File->BackupDev->SectorSize;
    endSector   = (File->BackupOffset + Offset + Size) / File->BackupDev->SectorSize;
    // Check the ready state of all sectors OR if the reference counts are different, the
    // sector has changed but not yet been enqueued for writing.
    // quit if a pending-busy/busy sector is detected.
    for (i = firstSector; i < endSector; ++i)
    {
      if( IO_RESULT_READY != File->BackupDev->sectorIoResult[i] ||
          File->BackupDev->RefCount[i].NvRAMCount != File->BackupDev->RefCount[i].ShadowCount)
      {
        writesComplete = FALSE;
        break;
      }
    }
  }

  return writesComplete ? SYS_OK : SYS_NV_WRITE_PENDING;
}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:    NV_MgrTask
 *
 * Description: This task is responsible for copying changed shadow memory
 *              sectors to the external memory devices.  First, the task
 *              SEARCHs the list of devices, looking for ones that have been
 *              written.
 *              If a written device is found, the next state WRITEs dirty
 *              sectors to the external memory devices
 *              The task continues through, from device 0 to n until all the
 *              dirty sectors are written.
 *              If a devices write driver cannot complete the write in this
 *              frame, the task state is switched to WRITE_WAIT, to attempt the
 *              write on the next frame.
 *              Synchronization with the NV_Write routine is provided with
 *              two semaphores, Written and Writing.
 *              To provide data integrity, the task will ensure that all dirty
 *              sectors of a device have been written before moving onto the
 *              next device.  This ensures that when a file has a primary and
 *              backup device, one valid copy of a file is written before
 *              altering the other.
 *
 * Parameters:  [in/out] pParam: Task manager parameter (not used)
 *
 * Returns:     none
 *
 * Notes:
 *
 *****************************************************************************/
void NV_MgrTask(void *pParam)
{
  static UINT32 DevSize;
  static UINT32 DevSectorSize;
  static UINT32 DirtySectorOffset;
  static UINT32 Dev;
  static UINT32 i;
  static UINT32 DeviceSectorArraySize;
  static INT32 TotalWritten;
  static const NV_DEV_INFO* CurrentDev;
  static NV_TASK_STATE NVTaskState;

#ifdef DEBUG_NVMGR_TASK
/*vcast_dont_instrument_start*/
  CHAR* DevName[] = {"DEV_NONE","DEV_RTC_PRI", "DEV_EE_PRI ", "DEV_EE_BKUP "};
/*vcast_dont_instrument_end*/
#endif

  switch(NVTaskState)
  {
    case NV_TASK_SEARCH:
#ifdef DEBUG_NVMGR_TASK
      /*vcast_dont_instrument_start*/
      GSE_DebugStr(NORMAL, TRUE, "NVTaskState: NV_TASK_SEARCH");
      /*vcast_dont_instrument_end*/
#endif
      //FOR each device in the device list
      for(Dev = 0; Dev < NV_MAX_DEV; Dev++)
      {
        //If the device has been written, switch to the WRITE state
        if(NV_DevInfo[Dev].Sem->Written)
        {
          NVTaskState = NV_TASK_WRITE;
          CurrentDev = &NV_DevInfo[Dev];
          //Clear the written flag,
          CurrentDev->Sem->Written = FALSE;
          break;
        }
      }
      break;

    case NV_TASK_WRITE:
#ifdef DEBUG_NVMGR_TASK
/*vcast_dont_instrument_start*/
      GSE_DebugStr(NORMAL, TRUE, "NVTaskState: NV_TASK_WRITE Device: %s", DevName[Dev]);
/*vcast_dont_instrument_end*/
#endif
      //Assign to some local vars to clarify the code below
      DevSize = CurrentDev->Size;
      DevSectorSize = CurrentDev->SectorSize;

      DeviceSectorArraySize = NV_SECTOR_ARRAY_SIZE(DevSize,DevSectorSize);

      // Assume we will finish, adjust below as required.
      NVTaskState = NV_TASK_SEARCH;

      //FOR every sector owned by a device.
      for(i = 0; i < DeviceSectorArraySize; ++i)
      {
        // If the Shadow <-> Dev ref-counts are different, process the change.
        if( CurrentDev->RefCount[i].NvRAMCount != CurrentDev->RefCount[i].ShadowCount )
        {
          // Dirty Sector offset is the sector # x sector size.
          DirtySectorOffset = (i * DevSectorSize);

          switch(CurrentDev->sectorIoResult[i])
          {
            case IO_RESULT_PENDING:
              // This sector is already en-queued, but not sent.
              // Nothing to do since queue entry is pointing to the "changed" data.
              CurrentDev->RefCount[i].NvRAMCount = CurrentDev->RefCount[i].ShadowCount;
              break;

            case IO_RESULT_READY:
              // The write has completed (TRANSMITTED->READY)
              // Fall-through, handle same as transmitted...
            case IO_RESULT_TRANSMITTED:
              // This sector is not en-queued OR is already on the way to EEPROM.
              // Either way, we need to en-queue the sector to send out the change.

              // Stay in write state
              NVTaskState = NV_TASK_WRITE;

              // Issue write (to SpiManager's queue)
              TotalWritten = CurrentDev->Write(&CurrentDev->ShadowMem[DirtySectorOffset],
                                                CurrentDev->Offset + DirtySectorOffset,
                                                DevSectorSize,
                                                &CurrentDev->sectorIoResult[i]);
              if( 0 == TotalWritten )
              {
                // If zero bytes written, SpiManager queue is full.
                // Break-out, wait for SpiManager to handle some write requests in queue
                // and free-up some queue entries.
                // Don't sync the ref counts; we want this sector's write retried.
                break;
              }
              else
              {
                // Sector update is en-queued. Sync ref-counts
                CurrentDev->RefCount[i].NvRAMCount = CurrentDev->RefCount[i].ShadowCount;
              }
              break;

            default: FATAL("Unrecognized IORESULT state: %d",CurrentDev->sectorIoResult[i]);
              break;
          }
          /*
          if (Dev == DEV_EE_PRI || Dev == DEV_EE_BKUP && i == 4)
          {
            UINT16 x;
            CHAR buffer[256];
            GSE_DebugStr( NORMAL, TRUE, "Q%d-%d "NL, Dev, i);
            DumpMemory(&CurrentDev->ShadowMem[DirtySectorOffset], 256,"NV_MgrTask");
          }
          */
        }
      }
      // Normally transition to NV_TASK_SEARCH state
      // The "Sem.Writing" mutex catches cases where NV_Write has preempted another
      // task currently writing to this device.  Return to NV_TASK_WRITE state
      // until the other task has finished writing.
      // Check that we have caught up writing to Dev, if not stay in write for this
      // device
      if(CurrentDev->Sem->Writing)
      {
        NVTaskState = NV_TASK_WRITE;
      }
      break;

    default:
      FATAL("Invalid NV_TASK_STATE = %d", NVTaskState);
      break;
  }
}


/*****************************************************************************
 * Function:    NV_FileDevToShadow()
 *
 * Description: Copies a particular file from the external NV device to the
 *              local shadow copy.  The CRC is calculated for the file data in
 *              the external device and the value is returned if it matches
 *              the CRC stored.  -1 is returned on CRC mismatch or device error
 *              It is intended to be used during initialization only.
 *
 * Parameters:  [in] FileID: File ID being copied
 *              [in] FileType: Indicates which copy to ready, the primary
 *                              or backup.  The caller is responsible for
 *                              knowing of a backup copy is available or not.
 *
 * Returns:     INT32: 1: [0..65535] Valid file CRC16
 *                     2: [-1]       File CRC16 is invalid, or read driver
 *                                   error.
 *
 * Notes:
 *
 ****************************************************************************/
INT32 NV_FileDevToShadow(NV_FILE_ID FileID, NV_FILE_TYPE FileType)
{
  INT32  ShadowOffset, PhysicalOffset;
  INT32  RetVal = 0;
  INT32  SizeRead = 0, TotalRead = 0;
  INT32  SectorSize, SectorTimeout;
  UINT32  IoTimeLimit;
  UINT8* ShadowMem;
  NV_DEV_READ_FUNC ReadFunc;
  UINT16 FileCRC, ComputedCRC;
  static UINT32 StartTime;
  UINT32 endTime;
  NV_RUNTIME_INFO* File;

  File = &NV_RuntimeInfo[ FileID];

  //Setup pointers to the requested device, either Primary or Backup
  if(NV_PRIMARY == FileType)
  {
    ShadowOffset   = File->PrimaryOffset;
    PhysicalOffset = File->PrimaryOffset + File->PrimaryDev->Offset;
    ShadowMem      = File->PrimaryDev->ShadowMem;
    ReadFunc       = File->PrimaryDev->Read;
    SectorSize     = File->PrimaryDev->SectorSize;
    SectorTimeout  = File->PrimaryDev->SectorTimeoutMs;
  }
  else
  {
    ShadowOffset   = File->BackupOffset;
    PhysicalOffset = File->BackupOffset + File->BackupDev->Offset;
    ShadowMem      = File->BackupDev->ShadowMem;
    ReadFunc       = File->BackupDev->Read;
    SectorSize     = File->BackupDev->SectorSize;
    SectorTimeout  = File->PrimaryDev->SectorTimeoutMs;
  }

  ASSERT(NULL != ReadFunc);
  ASSERT( 0 != SectorTimeout);

  // Calculate the number of ticks needed to perform the I/O and start timeout
  // timer.
  IoTimeLimit = ((File->Size / SectorSize) * SectorTimeout) * TICKS_PER_mSec;
  TimeoutEx(TO_START, IoTimeLimit, &StartTime, NULL);

  //Read the data from the physical device into the shadow memory.
  while( ((UINT32)TotalRead < File->Size) &&
          !TimeoutEx(TO_CHECK, IoTimeLimit, &StartTime, &endTime) )
  {
    SizeRead = ReadFunc(&ShadowMem[ShadowOffset+TotalRead],
                        PhysicalOffset+SizeRead,File->Size-TotalRead);

    //Set file CRC to bad on read error
    if(-1 == SizeRead)
    {
      RetVal = -1;
      break;
    }
    else
    {
      TotalRead += SizeRead;
    }
  }

  if( (UINT32)TotalRead < File->Size )
  {
      GSE_DebugStr( NORMAL,TRUE, "Timeout NVDev->Shadow (%s) %u/%u %d/%d",
              PromFileName[FileID],
              IoTimeLimit,
              endTime-StartTime,
              TotalRead, File->Size);
      RetVal = -1;
  }

  //Compute Primary CRC
  ComputedCRC = CalculateCheckSum(File->CheckMethod,
                                  &ShadowMem[ShadowOffset],
                                  File->Size - NV_CRC_SIZE);

  FileCRC = *(UINT16*)(&ShadowMem[ShadowOffset+File->Size-NV_CRC_SIZE]);

  //GSE_DebugStr(NORMAL,TRUE,"%s FileCRC: x%04X Computed: x%04X%",
  //         PromFileName[FileID], FileCRC, ComputedCRC);

  if(-1 == RetVal)  //if some error occurred, leave retval -1
  {
    RetVal = -1;
  }
  else if((ComputedCRC == FileCRC))  //If the CRCs match, return CRC
  {
    RetVal = FileCRC;
  }
  else                              //else, CRCs don't match, set retval to -1
  {
    GSE_DebugStr(NORMAL,TRUE,"%s Mismatch FileCRC: x%04X Computed: x%04X%",
             PromFileName[FileID], FileCRC, ComputedCRC);
    RetVal = -1;
  }

  return RetVal;
}


/*****************************************************************************
 * Function:    NV_FileShadowToDev
 *
 * Description: Copies a particular file from the local shadow copy to the
 *              external memory device.  The shadow copy should be valid
 *              with no dirty sectors set before calling this function.  It is
 *              intended to be used during initialization only.
 *
 * Parameters:  [in] FileID: File ID being copied
 *              [in] FileType: Which device to copy to, the primary or backup
 *                             device
 *
 * Returns:     0 - Success
 *             -1 - Device write function returned a non-recoverable error
 *
 * Notes:
 *
 ****************************************************************************/
INT32 NV_FileShadowToDev(NV_FILE_ID FileID, NV_FILE_TYPE FileType)
{
  INT32  ShadowOffset, PhysicalOffset;
  INT32  SizeWritten = 0, TotalWritten = 0;
  INT32  SectorSize, SectorTimeout;
  INT32  RetVal = 0;
  INT32  IoTimeLimit;
  UINT8*  ShadowMem;
  NV_DEV_WRITE_FUNC WriteFunc;
  static UINT32 StartTime;
  UINT32 endTime;
  NV_RUNTIME_INFO* File;

  File = &NV_RuntimeInfo[ FileID];

  //Setup pointers to the requested device, either Primary or Backup
  if(NV_PRIMARY == FileType)
  {
    ShadowOffset   = File->PrimaryOffset;
    PhysicalOffset = File->PrimaryOffset + File->PrimaryDev->Offset;
    ShadowMem      = File->PrimaryDev->ShadowMem;
    WriteFunc      = File->PrimaryDev->Write;
    SectorSize     = File->PrimaryDev->SectorSize;
    SectorTimeout  = File->PrimaryDev->SectorTimeoutMs;
  }
  else
  {
    ShadowOffset   = File->BackupOffset;
    PhysicalOffset = File->BackupOffset + File->BackupDev->Offset;
    ShadowMem      = File->BackupDev->ShadowMem;
    WriteFunc      = File->BackupDev->Write;
    SectorSize     = File->BackupDev->SectorSize;
    SectorTimeout  = File->PrimaryDev->SectorTimeoutMs;
  }

  ASSERT(NULL != WriteFunc);
  ASSERT( 0 != SectorTimeout);

  // Calculate the number of ticks needed to perform the I/O and start timeout
  // timer.
  IoTimeLimit = ((File->Size / SectorSize) * SectorTimeout) * TICKS_PER_mSec;
  TimeoutEx(TO_START, IoTimeLimit, &StartTime, NULL);

  //Read the data from the shadow memory into the physical device.
  while( ((UINT32)TotalWritten < File->Size) &&
          !TimeoutEx(TO_CHECK, IoTimeLimit, &StartTime, &endTime))
  {
    SizeWritten = WriteFunc(&ShadowMem[ShadowOffset+TotalWritten],
                            PhysicalOffset+TotalWritten,File->Size-TotalWritten,NULL);

    if(-1 == STPUI(SizeWritten, eTpEeShDev, -1))
    {
      RetVal = -1;
      break;
    }
    else
    {
      TotalWritten += SizeWritten;
    }
  }

  if( (UINT32)TotalWritten < File->Size )
  {
      GSE_DebugStr( NORMAL,TRUE, "Timeout NVShadow->Dev (%s) %u/%u %d/%d",
              PromFileName[FileID],
              IoTimeLimit,
              endTime-StartTime,
              TotalWritten, File->Size);
     RetVal = -1;
  }

  return RetVal;
}



/*****************************************************************************
 * Function:    NV_CopyBackupToPrimaryShadow
 *
 * Description: Copies the data from a file's Backup shadow memory to a file's
 *              Primary shadow memory.  This routine is intended for startup
 *              use only, to restore file data from the backup copy.
 *
 * Parameters:  [in] File: Pointer to the file to copy
 *
 * Returns:     none
 *
 * Notes:       none
 *
 ****************************************************************************/
void NV_CopyBackupToPrimaryShadow(NV_RUNTIME_INFO* File)
{
  memcpy((File->PrimaryDev->ShadowMem+File->PrimaryOffset),
         (File->BackupDev->ShadowMem+File->BackupOffset),
          File->Size);
}



/*****************************************************************************
 * Function:    NV_CopyPrimaryToBackupShadow
 *
 * Description: Copies the data from a file's Primary shadow memory to a file's
 *              Backup shadow memory.  This routine is intended for startup
 *              use only, to restore file data from the primary copy.
 *
 * Parameters:  [in] File: Pointer to the file to copy
 *
 * Returns:     none
 *
 * Notes:       none
 *
 ****************************************************************************/
void NV_CopyPrimaryToBackupShadow(NV_RUNTIME_INFO* File)
{
  memcpy((File->BackupDev->ShadowMem+File->BackupOffset),
         (File->PrimaryDev->ShadowMem+File->PrimaryOffset),
          File->Size);
}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: NVMgr.c $
 *
 * *****************  Version 72  *****************
 * User: Peter Lee    Date: 12-10-27   Time: 5:02p
 * Updated in $/software/control processor/code/system
 * SCR #1178  NV_RTCWrite() Error
 * 
 * *****************  Version 71  *****************
 * User: Melanie Jutras Date: 12-10-19   Time: 10:21a
 * Updated in $/software/control processor/code/system
 * SCR #1172 PCLint Suspicious use of & Error 545.  Eliminated & and added
 * cast in NV_GetFileName().
 * 
 * *****************  Version 70  *****************
 * User: John Omalley Date: 12-10-16   Time: 2:53p
 * Updated in $/software/control processor/code/system
 * SCR 1172 - Undo fix because it caused a compiler warning
 * 
 * *****************  Version 69  *****************
 * User: Melanie Jutras Date: 12-10-10   Time: 1:13p
 * Updated in $/software/control processor/code/system
 * SCR 1172 PCLint 545 Suspicious use of & Error
 *
 * *****************  Version 68  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 67  *****************
 * User: Contractor V&v Date: 7/11/12    Time: 4:36p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2  Fixed debug "write" should be "read"
 *
 * *****************  Version 66  *****************
 * User: Jeff Vahue   Date: 11/18/10   Time: 8:17p
 * Updated in $/software/control processor/code/system
 * SCR# 848 - TP Coverage
 *
 * *****************  Version 65  *****************
 * User: Jim Mood     Date: 11/18/10   Time: 4:32p
 * Updated in $/software/control processor/code/system
 * SCR 1003: had to fix up the comment header too
 *
 * *****************  Version 64  *****************
 * User: Jim Mood     Date: 11/18/10   Time: 4:30p
 * Updated in $/software/control processor/code/system
 * SCR 1003: NV_Open Code Coverage
 *
 * *****************  Version 63  *****************
 * User: Jeff Vahue   Date: 11/01/10   Time: 7:50p
 * Updated in $/software/control processor/code/system
 * SCR# 975 - Code Coverage refactor
 *
 * *****************  Version 62  *****************
 * User: Jim Mood     Date: 9/30/10    Time: 6:04p
 * Updated in $/software/control processor/code/system
 * SCR 906 Code Coverage Changes
 *
 * *****************  Version 61  *****************
 * User: Peter Lee    Date: 9/01/10    Time: 3:00p
 * Updated in $/software/control processor/code/system
 * SCR #840 Code Review Updates
 *
 * *****************  Version 60  *****************
 * User: Contractor V&v Date: 7/21/10    Time: 7:19p
 * Updated in $/software/control processor/code/system
 * SCR #538 SPIManager startup & NV Mgr Issues
 * Also removed temporary default stub function Default_FileInit.
 *
 * *****************  Version 59  *****************
 * User: Contractor3  Date: 7/16/10    Time: 8:57a
 * Updated in $/software/control processor/code/system
 * SCR #702 - Changes based on code review.
 *
 * *****************  Version 58  *****************
 * User: Contractor V&v Date: 7/14/10    Time: 2:22p
 * Updated in $/software/control processor/code/system
 * SCR #674 fast.reset=really needs to wait until EEPROMcommitted
 *
 * *****************  Version 57  *****************
 * User: Contractor V&v Date: 6/30/10    Time: 3:19p
 * Updated in $/software/control processor/code/system
 * SCR #531 Log Erase and NV_Write Timing
 *
 * *****************  Version 56  *****************
 * User: Jeff Vahue   Date: 6/18/10    Time: 4:41p
 * Updated in $/software/control processor/code/system
 * Remove unused variable, linker warning
 *
 * *****************  Version 55  *****************
 * User: Contractor V&v Date: 6/16/10    Time: 6:10p
 * Updated in $/software/control processor/code/system
 * SCR #623, SCR #636
 *
 * *****************  Version 54  *****************
 * User: Contractor2  Date: 6/14/10    Time: 1:08p
 * Updated in $/software/control processor/code/system
 * SCR #483 Function Names must begin with the CSC it belongs with.
 *
 * *****************  Version 53  *****************
 * User: Contractor V&v Date: 6/09/10    Time: 7:22p
 * Updated in $/software/control processor/code/system
 * SCR #614 fast.reset=really should initialize files
 *
 * *****************  Version 52  *****************
 * User: Contractor V&v Date: 6/09/10    Time: 4:59p
 * Updated in $/software/control processor/code/system
 * SCR #614 fast.reset=really should initialize files
 *
 * *****************  Version 51  *****************
 * User: Contractor V&v Date: 6/08/10    Time: 5:55p
 * Updated in $/software/control processor/code/system
 * SCR #614 fast.reset=really should initialize files
 *
 * *****************  Version 50  *****************
 * User: Contractor V&v Date: 5/19/10    Time: 6:11p
 * Updated in $/software/control processor/code/system
 * SCR #548 Strings in Logs
 *
 * *****************  Version 49  *****************
 * User: Contractor V&v Date: 5/12/10    Time: 3:48p
 * Updated in $/software/control processor/code/system
 * SCR #548 Strings in Logs
 *
 * *****************  Version 48  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:55p
 * Updated in $/software/control processor/code/system
 * SCR #587 Change TmTaskCreate to return void
 *
 * *****************  Version 47  *****************
 * User: Contractor V&v Date: 4/28/10    Time: 4:54p
 * Updated in $/software/control processor/code/system
 * SCR #571 SysCond should be fault when default config is loaded
 *
 * *****************  Version 46  *****************
 * User: Jeff Vahue   Date: 4/13/10    Time: 11:55a
 * Updated in $/software/control processor/code/system
 * SCR #538 - put file sizes back to min == page size.  Add an assert if
 * this is not true.  Fix cut/paste error on check for over allocation on
 * the backup device.
 *
 * *****************  Version 45  *****************
 * User: Contractor V&v Date: 4/12/10    Time: 7:22p
 * Updated in $/software/control processor/code/system
 * SCR #317 Implement safe strncpy
 *
 * *****************  Version 44  *****************
 * User: Jeff Vahue   Date: 4/09/10    Time: 5:07p
 * Updated in $/software/control processor/code/system
 * SCR #538 - turn ifs to ASSERTs in NV_Write(Now) and NV_Read
 *
 * *****************  Version 43  *****************
 * User: Jeff Vahue   Date: 4/09/10    Time: 4:39p
 * Updated in $/software/control processor/code/system
 * SCR #538 - optimize SPI activity, minimize file sizes and page writes
 *
 * *****************  Version 42  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:36p
 * Updated in $/software/control processor/code/system
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 *
 * *****************  Version 41  *****************
 * User: Contractor V&v Date: 3/19/10    Time: 4:31p
 * Updated in $/software/control processor/code/system
 * SCR #378 add VectorCast Statements to control instrumentation
 *
 * *****************  Version 40  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/system
 * SCR# 483 - Function Names
 *
 * *****************  Version 39  *****************
 * User: Contractor V&v Date: 3/10/10    Time: 7:32p
 * Updated in $/software/control processor/code/system
 * SCR #67 expanding direct write-to-device mode for all devices.
 *
 * *****************  Version 38  *****************
 * User: Jeff Vahue   Date: 3/10/10    Time: 2:34p
 * Updated in $/software/control processor/code/system
 * SCR# 480 - add semi's after ASSERTs
 *
 * *****************  Version 37  *****************
 * User: Jeff Vahue   Date: 3/04/10    Time: 6:24p
 * Updated in $/software/control processor/code/system
 * SCR #67 Error changing an if to and ASSERT, s/b if
 *
 * *****************  Version 36  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 4:01p
 * Updated in $/software/control processor/code/system
 * SCR #279 Resetting of NvMgr / EEPROM application data
 * SCR #67 Interrupted SPI Access (Multiple / Nested SPI Access)
 *
 * *****************  Version 35  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 34  *****************
 * User: Jeff Vahue   Date: 2/11/10    Time: 4:47p
 * Updated in $/software/control processor/code/system
 * SCR# 448 - remove box.dbg.erase_ee_dev cmd
 *
 * *****************  Version 33  *****************
 * User: Contractor V&v Date: 2/03/10    Time: 2:57p
 * Updated in $/software/control processor/code/system
 * SCR 279, SCR 330
 *
 * *****************  Version 32  *****************
 * User: Jeff Vahue   Date: 1/28/10    Time: 12:20p
 * Updated in $/software/control processor/code/system
 * SCR# 372: Misc - Enhancement.  Updating cfg EEPROM data
 *
 * Use NV_Write to erase a file as it knows all about file size, offset,
 * etc.  Works for both startup and Run time reset.
 *
 * *****************  Version 31  *****************
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
 * *****************  Version 30  *****************
 * User: Jeff Vahue   Date: 1/27/10    Time: 7:01p
 * Updated in $/software/control processor/code/system
 * SCR# 142 - added the name of the file to the error msg when the timeout
 * occurs.
 *
 * *****************  Version 29  *****************
 * User: Contractor V&v Date: 1/27/10    Time: 5:24p
 * Updated in $/software/control processor/code/system
 * SCR 142, SCR 372, SCR 398
 *
 * *****************  Version 28  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 5:10p
 * Updated in $/software/control processor/code/system
 * SCR# 397
 *
 * *****************  Version 27  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:26p
 * Updated in $/software/control processor/code/system
 * SCR 145, SCR 216, SCR 371
 *
 * *****************  Version 26  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 2:11p
 * Updated in $/software/control processor/code/system
 * SCR# 326
 *
 * *****************  Version 25  *****************
 * User: Contractor V&v Date: 12/18/09   Time: 7:00p
 * Updated in $/software/control processor/code/system
 * SCR 31 Added ASSERT test for NULL pointer
 *
 * *****************  Version 24  *****************
 * User: Jim Mood     Date: 12/16/09   Time: 3:29p
 * Updated in $/software/control processor/code/system
 * SCR #375
 *
 * *****************  Version 23  *****************
 * User: Jeff Vahue   Date: 12/07/09   Time: 4:28p
 * Updated in $/software/control processor/code/system
 * SCR# 364 - remove WIN32 code
 *
 * *****************  Version 22  *****************
 * User: Peter Lee    Date: 12/02/09   Time: 2:17p
 * Updated in $/software/control processor/code/system
 * SCR #350 Misc Code Review Items
 *
 * *****************  Version 21  *****************
 * User: Contractor V&v Date: 11/30/09   Time: 5:32p
 * Updated in $/software/control processor/code/system
 * Implement functions for accessing filenames and CRC
 *
 * *****************  Version 20  *****************
 * User: Contractor V&v Date: 11/18/09   Time: 4:17p
 * Updated in $/software/control processor/code/system
 * Implement SCR 172 ASSERT processing for all "default" case processing
 * Implement SCR 145 Configuration CRC read command
 *
 * *****************  Version 19  *****************
 * User: Peter Lee    Date: 9/29/09    Time: 1:25p
 * Updated in $/software/control processor/code/system
 * SCR #280 Fix Compiler Warnings
 *
 * *****************  Version 18  *****************
 * User: Jim Mood     Date: 9/25/09    Time: 9:04a
 * Updated in $/software/control processor/code/system
 * Added driver functions and support for accessing the RTC battery-backed
 * RAM
 *
 * *****************  Version 17  *****************
 * User: Peter Lee    Date: 6/30/09    Time: 3:41p
 * Updated in $/software/control processor/code/system
 * SCR #204 Misc Code Review Udates
 *
 * *****************  Version 16  *****************
 * User: Jim Mood     Date: 6/05/09    Time: 3:30p
 * Updated in $/software/control processor/code/system
 * NV_WriteNow function errors.  SCR 195
 *
 * *****************  Version 15  *****************
 * User: Peter Lee    Date: 5/21/09    Time: 5:28p
 * Updated in $/software/control processor/code/system
 * SCR #193 Syntax Error
 *
 * *****************  Version 14  *****************
 * User: Peter Lee    Date: 5/21/09    Time: 4:26p
 * Updated in $/software/control processor/code/system
 * SCR #193 Informal Code Review Findings
 *
 * *****************  Version 13  *****************
 * User: Jim Mood     Date: 5/01/09    Time: 3:49p
 * Updated in $/software/control processor/code/system
 * Added an externally callable function to erase a device to 0xFF
 *
 * *****************  Version 12  *****************
 * User: Jim Mood     Date: 2/05/09    Time: 11:29a
 * Updated in $/control processor/code/system
 * Correcting some debug string spacing errors Added support for the new
 * EEPROM driver, which included automatic EEPROM device detection.
 *
 * *****************  Version 11  *****************
 * User: Jim Mood     Date: 2/03/09    Time: 5:14p
 * Updated in $/control processor/code/system
 * Updated device table to allocate the backup files to the 2nd EEPROM
 * device.  If the 2nd device is not installed, the backup feature will
 * not work and the primary will be the only copy of data.  See also
 * SCR#142
 *
 * *****************  Version 10  *****************
 * User: Jim Mood     Date: 12/19/08   Time: 5:52p
 * Updated in $/control processor/code/system
 *
 * *****************  Version 9  *****************
 * User: Jim Mood     Date: 12/19/08   Time: 3:35p
 * Updated in $/control processor/code/system
 *
 * *****************  Version 8  *****************
 * User: Jim Mood     Date: 12/12/08   Time: 7:00p
 * Updated in $/control processor/code/system
 * Fixed Write() and WriteNow() functions to pre-initialize the result
 * code to SYS_OK
 *
 * *****************  Version 7  *****************
 * User: Jim Mood     Date: 10/16/08   Time: 3:38p
 * Updated in $/control processor/code/system
 * NV_Open - Corrected system log ID for the case where the primary and
 * backup file copies are bad.
 *
 * *****************  Version 6  *****************
 * User: Jim Mood     Date: 10/13/08   Time: 10:32a
 * Updated in $/control processor/code/system
 * Removed some lines of code that had no effect (noticed after
 * single-step debugging)
 *
 * *****************  Version 5  *****************
 * User: Jim Mood     Date: 10/10/08   Time: 5:33p
 * Updated in $/control processor/code/system
 * Preliminarily tested version, can Open, Write, and Read a
 * dual-redundant file from EEPROM.  CRC file validation works.  RTC RAM
 * driver still needs to be added
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

