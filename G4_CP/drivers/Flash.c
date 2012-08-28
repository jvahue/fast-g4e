#define FLASH_BODY
/******************************************************************************
            Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:        Flash.c
    
    Description: Flash Memory Driver functions.

                 The Flash Memory Driver functions are a standard set of
                 functions that allow access to the data flash. These higher
                 level functions use the low level drivers that are specific
                 to the flash device detected during initialization.

    VERSION
    $Revision: 30 $  $Date: 8/28/12 1:06p $
    
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "flash.h"
#include "AmdFujiSCS.h"
#include "utility.h"

#include "stddefs.h"

#include "TestPoints.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define MAX_POSSIBLE_SECTORS 0x8000
#define MIN_POSSIBLE_SECTORS 32

/*****************************************************************************/
/* Local Typedefs                                                            */ 
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
//---- DATA FLASH Command Set Parameter Definitions --------------------------
// This table was created to make it easier to add new flash devices in the 
// future. All other flash parts that are part of the CFI standard have been 
// assigned a position in the FlashCmdSet Table and then commented out.
static
const FLASH_CMDS FlashCmdSet [] =
{
//   {  "No Command Set",                 
//      FCS_NO_CMD_SET,   
      // Function pointers
//      NULL,             // FlashReset
//      NULL,             // FlashProgram
//      NULL,             // FlashWriteBuffer
//      NULL,             // FlashProgamBuffer
//      NULL,             // FlashAbortBuffer
//      NULL,             // FlashSectorEraseSetup
//      NULL,             // FlashSectorEraseCmd
//      NULL,             // FlashChipErase
//      NULL              // FlashGetStatus      
  // },
  // {  "Intel Sharp Extended Command Set",
  //    FCS_INTEL_SHARP_EXTENDED,
      // Function pointers
  // },
   {  "AMD Fujitsu Standard Command Set",
      FCS_AMD_FUJITSU_STANDARD,
      // Function pointers
      AmdFujiSCS_Reset,                 // FlashReset
      AmdFujiSCS_Program,               // FlashProgram
      AmdFujiSCS_WriteToBuffer,         // FlashWriteBuffer
      AmdFujiSCS_ProgramBufferToFlash,  // FlashProgamBuffer
      AmdFujiSCS_WriteBufferAbortReset, // FlashAbortBuffer
      AmdFujiSCS_SectorEraseSetup,      // FlashSectorEraseSetup
      AmdFujiSCS_SectorEraseCmd,        // FlashSectorEraseCmd
      AmdFujiSCS_ChipErase,             // FlashChipErase
      AmdFujiSCS_GetStatus              // FlashGetStatus
   }//,
//   {  "Intel Standard Command Set",
//      FCS_INTEL_STANDARD,
      // Function pointers
//   },
//   {  "AMD Fujitsu Extended Command Set",
//      FCS_AMD_FUJITSU_EXTENDED,
      // Function pointers
//   },
//   {  "Windbond Standard Command Set",
//      FCS_WINDBOND_STANDARD,
      // Function pointers

//   },
//   {  "Mitsubishi Standard Command Set",
//      FCS_MITSUBISHI_STANDARD,
      // Function pointers
//   },
//   {  "Mitsubishi Extended Command Set",
//      FCS_MITSUBISHI_EXTENDED,
      // Function pointers
//   },
//   {  "SST Page Write Command Set",
//      FCS_SST_PAGE_WRITE,
      // Function pointers
//   },
//   {  "Intel Performance Command Set",
//      FCS_INTEL_PERFORMANCE,
      // Function pointers
//   },
//   {  "Intel Data Command Set",    
//      FCS_INTEL_DATA,
      // Function pointers
//   }
};

// Local Flash Description Table
static FLASH_DESC FlashDescription;

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static RESULT FlashPopulateCFIData           ( FLASH_ID   *pId,
                                               FLASH_SYS  *pSys,
                                               FLASH_INFO *pInfo );
static void   FlashGetCFIIdentification      ( FLASH_ID *pId);

static RESULT FlashGetCFISystemInterfaceData ( FLASH_SYS *pSys);

static RESULT FlashGetCFIDeviceGeometry      ( FLASH_INFO *pInfo);

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/
/*****************************************************************************
 * Function:    FlashInitialize
 *
 * Description: Initialize all data and control structures necessary for FLASH.
 *              operation.
 *
 * Parameters:  SYS_APP_ID *SysLogId     ptr to return SYS_APP_ID 
 *              void *pdata              ptr to buffer to return result data
 *              UINT16 *psize            ptr to return size of result data 
 *
 * Returns:     RESULT DrvStatus         Status of called flash routines
 *
 * Notes:       None.
 *
 ****************************************************************************/
//RESULT FlashInitialize (void)
RESULT FlashInitialize (SYS_APP_ID *SysLogId, void *pdata, UINT16 *psize)
{
  // Local Data
  FLASH_DESC     *pFlashDesc;
  FLASH_ID       *pId;
  FLASH_SYS      *pSys;
  FLASH_INFO     *pInfo;
  RESULT         DrvStatus;
  UINT32         i;
  
  FLASH_DRV_PBIT_LOG  *pdest; 
  
  
  // Initialize Local Data 
  pFlashDesc = &FlashDescription;
  pInfo      = &pFlashDesc->Info;
  pId        = &pFlashDesc->Id;
  pSys       = &pFlashDesc->System;
  DrvStatus  = DRV_OK;


  pdest = (FLASH_DRV_PBIT_LOG *) pdata;
  memset ( pdest, 0, sizeof(FLASH_DRV_PBIT_LOG) ); 
  pdest->result = DRV_OK; 
  
  *psize = sizeof(FLASH_DRV_PBIT_LOG); 


  // Set the Flash Base Address
  pFlashDesc->BaseAddr = (FLASHDATA*)0x10000000;

  // Read and Populate the CFI Data Structures
  DrvStatus = FlashPopulateCFIData(pId, pSys, pInfo);
  
  // Check that the Driver Status is OK
  if ( DrvStatus == DRV_OK)
  {
     // Find the pointer to the Flash Command Set Table
     DrvStatus = FlashFindCmdSet(pId->PrimaryCmdSet, &(pFlashDesc->pCmdSet));
     
     // Check that driver completes OK
     if (DrvStatus == DRV_OK)
     {
        // Initialize the Sector Status
        for (i=0; i < pInfo->EraseBlockInfo[0].NumSectors; i++ )
        {
           // Set all sectors to not busy 
           pFlashDesc->SectorStatus[i] =  FSECT_IDLE;
        }
     }
  }
  
  // If the Driver fails initialization then set the Status in the 
  // Flash Description Structure to Faulted.
  if (DRV_OK != DrvStatus)
  {
     pInfo->DrvStatus = FLASH_DRV_STATUS_FAULTED;
     pdest->result = DrvStatus; 
     *SysLogId = DRV_ID_DF_PBIT_CFI_BAD; 
  }
   
  return pdest->result; 
  
}   // End of FlashInitialize()



/*****************************************************************************
 * Function:    FlashFindCmdSet
 *
 * Description: The FlashFindCmdSet will search the Command Set Table
 *              for the Flash Devices Commands. 
 *
 * Parameters:  FLASH_CMD_SET CmdSet   - The Command Set Id of the Device
 *              FLASH_CMDS **pTable    - Pointer to Flash commands in the Table
 *
 * Returns:     DRV_RESULTS - [DRV_OK if command set found
 *                             DRV_FLASH_CANNOT_FIND_CMD_SET if not found]
 *
 * Notes:       
 *
 ****************************************************************************/
RESULT FlashFindCmdSet(FLASH_CMD_SET CmdSet, FLASH_CMDS **pTable)
{
   // Local Data
   UINT32     i;
   FLASH_CMDS *pCmdSet;
   RESULT     DrvStatus;
   BOOLEAN    bFound;

   // Local Data Initialization
   pCmdSet   = (FLASH_CMDS *)&FlashCmdSet[0];
   DrvStatus = DRV_FLASH_CANNOT_FIND_CMD_SET;
   bFound    = FALSE;
   
   // Loop through Table of Device Command Sets
   for (i=0; (i < (sizeof(FlashCmdSet)/sizeof(FLASH_CMDS))) &&
	          (FALSE == bFound);
        i++)
   {
      // Check if the Command Set is Found
      if (pCmdSet->FlashCmdSetId == CmdSet)
      {
         // Command Set Found, exit loop
         *pTable   = pCmdSet;
         DrvStatus = DRV_OK;
         bFound    = TRUE;
      }
      // Increment to Next Entry
      pCmdSet++;
   }
   
   // Return the driver status
   return (DrvStatus);
}

/*****************************************************************************
 * Function:    FlashWriteBufferMode
 *
 * Description: This function provides an interface to the flash write 
 *              buffer capability. 
 *              There are 3 main sections to the function:
 *              * Determine the number of locations to program
 *              * Set-up and write command sequence
 *              * Start programming operation with "Program Buffer to Flash"
 *
 * Parameters:  FLASHADDR Offset        - Offset from from Base Address to Write
 *              FLASHDATA *pDataBuf     - Pointer to the data to write
 *              BYTECOUNT ByteCount     - Number of Flash Locations to Write
 *              BYTECOUNT *TotalWritten - Total Bytes written to flash 
 *
 * Returns:     DRV_RESULT [DRV_OK, DRV_FLASH_BUF_WR_COUNT_INVALID,
 *                                  DRV_FLASH_BUF_WR_DEV_TIMEOUT,
 *                                  DRV_FLASH_BUF_WR_ABORT,
 *                                  DRV_FLASH_BUF_WR_SW_TIMEOUT ]
 *
 *              There is another return value for the case FS_SUSPEND which 
 *              sets the DrvStatus to DRV_FLASH_BUF_WR_SUSPENDED but is 
 *              preceeded by the comment that we'll never reach this case.
 *
 * Notes:       The data MUST be properly aligned with the Flash bus width.
 *              Address and Offset Parameter Checking is not performed.
 *              The Driver will return an error if the WordCount is 0.
 *              The data writes in this system should not span more than one
 *              sector. That means the maximum amount of data that we can write
 *              will be 256Kbytes. The sector boundary condition should be
 *              checked before calling this function. Writes that try to
 *              cross a sector boundary will end in an ABORT.
 *
  ****************************************************************************/
RESULT FlashWriteBufferMode ( FLASHADDR FlashOffset,
                              FLASHDATA *pData,
                              BYTECOUNT ByteCount,
                              BYTECOUNT *TotalWritten)
{       
   // Local Data
   FLASH_DESC          *pFlashDesc;
   FLASH_CMDS          *pCmd;
   FLASH_INFO          *pInfo;
   FLASH_SYS           *pSys;
   RESULT              DrvStatus;
   UINT32              nTotalBytes;
   FLASHADDR           LastLoadedOffset;
   FLASHADDR           CurrentOffset;
   FLASHADDR           EndOffset;
   FLASHDATA           WriteCount;   
   UINT32              nBytesWrite;
   UINT32              nRemainPageBytes;
   FLASH_STATUS        FlashStatus;
   UINT32              StartTime;

   // Initialize Local Data
   pFlashDesc        = &FlashDescription;
   pCmd              = pFlashDesc->pCmdSet;
   pInfo             = &pFlashDesc->Info;
   pSys              = &pFlashDesc->System;
   DrvStatus         = DRV_OK;
   FlashStatus       = FS_BUSY;
   *TotalWritten     = 0;
   // Initialize in case the flash was busy before entering this function
   StartTime         = TTMR_GetHSTickCount();
    
   // Check that the Byte Count is Valid 
   ASSERT (0 != ByteCount);
   
   // Total Bytes that are to be written to the data flash
   nTotalBytes       = ByteCount;
         
   // Mark the sector status as programming
   pFlashDesc->SectorStatus[FlashOffset >> FLASH_SECTOR_SHIFT] = FSECT_PROGRAM;
      
   // Find the long word offset
   CurrentOffset    = FlashOffset / FLASH_BYTES_PER_OP;
   LastLoadedOffset = CurrentOffset;

   // Loop until all requested bytes have been written or
   // the Flash detects a failure condition
   while ( (0 != nTotalBytes) && (DRV_OK  == DrvStatus ) ||
           (0 == nTotalBytes) && (FS_BUSY == FlashStatus) )
   {
      // Check the status of the flash
      // To check the status during a buffer write, the last loaded address
      // must be examined.....
      FlashStatus = pCmd->FlashGetStatus( pFlashDesc->BaseAddr,
                                          LastLoadedOffset,
                                          TRUE);
      switch (FlashStatus)
      {
         case FS_NOT_BUSY:
            // If flash is no longer busy begin buffer write sequence
   
            // All buffer writes must be aligned on 128 byte page boundaries
            // Check if at the end of a page and finish writing just that page
            if ((FlashOffset % pInfo->MaxByteWrite) != 0)
            {
               nRemainPageBytes = pInfo->MaxByteWrite -
                                 (FlashOffset % pInfo->MaxByteWrite);
               // If the total is greater than the remaining then just fill in
               // the end with number of bytes left
               if (nRemainPageBytes <= nTotalBytes)
               {
                  nBytesWrite = nRemainPageBytes;
               }
               else // All remaining bytes can be written
               {
                  nBytesWrite = nTotalBytes;
               }
            }
            // If the number of total bytes to be written exceeds the
            // Buffer then limit the write to maximum buffer size.
            else if (nTotalBytes > pInfo->MaxByteWrite)
            {
               nBytesWrite = pInfo->MaxByteWrite;
            }
            else
            {
               // Data to be written will fit in one buffered write
               nBytesWrite = nTotalBytes;
            }

            // Calculate the end offset of the data to write 
            EndOffset        = CurrentOffset +
                               (nBytesWrite/FLASH_BYTES_PER_OP) - 1;

            // Set the last loaded offset to the beginning 
            LastLoadedOffset = CurrentOffset;

            // Unlocks the buffer write sequence
            pCmd->FlashWriteBuffer(pFlashDesc->BaseAddr, CurrentOffset);

            // Calculate the write count for the 2 parts and write the value
            WriteCount  = (nBytesWrite/FLASH_BYTES_PER_OP) - 1;
            WriteCount *= FLASH_CFG_MULTIPLIER;
            FLASH_WR(pFlashDesc->BaseAddr, CurrentOffset, WriteCount);

            // Load the Data into the Buffer
            while(CurrentOffset <= EndOffset)
            {
               // Saved the last loaded address
               LastLoadedOffset = CurrentOffset;
               // Write the Data to the flash buffer
               FLASH_WR(pFlashDesc->BaseAddr, CurrentOffset++, *pData++);
            }
             
            // Command the Program Buffer to Flash
            pCmd->FlashProgamBuffer(pFlashDesc->BaseAddr, LastLoadedOffset);
            // Record the time the write started
            StartTime     = TTMR_GetHSTickCount();

            // Update the flash offset, total bytes to write and the
            // total bytes that have been written
            FlashOffset    += nBytesWrite;
            nTotalBytes    -= nBytesWrite;
            *TotalWritten  += nBytesWrite;
            break;
         case FS_EXCEEDED_TIME_LIMITS:
            // Internal Flash Part has timed out
            DrvStatus = DRV_FLASH_BUF_WR_DEV_TIMEOUT;
            // Issue a Reset Command to go to Read Mode
            pCmd->FlashReset(pFlashDesc->BaseAddr, LastLoadedOffset);
            break;
         case FS_WRITE_BUFFER_ABORT:
            // The Write buffer command has aborted
            DrvStatus = DRV_FLASH_BUF_WR_ABORT;
            // Issue a Flash Buffer Write Abort Reset
            pCmd->FlashAbortBuffer(pFlashDesc->BaseAddr);
            break;
         case FS_BUSY:
         case FS_SUSPEND: 
         default:
            // Still busy programming, or somehow suspended
            // Check if the timeout has been exceeded
            if (((TTMR_GetHSTickCount() - StartTime) / TICKS_PER_uSec) >
                  pSys->MaxMinBufferTime_uS)
            {
               // Double check the status of the flash because we may have 
               // been interrupted.
               FlashStatus = pCmd->FlashGetStatus( pFlashDesc->BaseAddr,
                                                   LastLoadedOffset,
                                                   TRUE);
               if (FS_BUSY == FlashStatus)
               {
                  // Software has decided that the programming has timed out
                  DrvStatus = DRV_FLASH_BUF_WR_SW_TIMEOUT;
                  // Issue a Reset Command to go to Read Mode
                  pCmd->FlashReset(pFlashDesc->BaseAddr, LastLoadedOffset);
               }
            }
            break;
      }
   }
   // Mark the sector status as idle
   pFlashDesc->SectorStatus[FlashOffset >> FLASH_SECTOR_SHIFT] = FSECT_IDLE;

   return(DrvStatus);
}

/*****************************************************************************
 * Function:    FlashWriteWordMode
 *
 * Description: This function is used for writing to one single location.
 *
 * Parameters:  FLASHADDR  Offset   - Offset from from Base Address to Write
 *              FLASHDATA *pData    - Pointer to the data to write
 *              BYTECOUNT ByteCount - Number of bytes to write, should always
 *                                    be four otherwise a error will be
 *                                    generated.
 *              BYTECOUNT *TotalWritten - Total bytes written
 *
 * Returns:     DRV_RESULT [DRV_OK, DRV_FLASH_WORD_WR_COUNT_INVALID,
 *                                  DRV_FLASH_WORD_WR_DEV_TIMEOUT,
 *                                  DRV_FLASH_WORD_WR_SW_TIMEOUT ] 
 *
 * Notes:       
 *
 ****************************************************************************/
RESULT FlashWriteWordMode ( FLASHADDR Offset,  FLASHDATA *pData,
                            BYTECOUNT ByteCount, BYTECOUNT *TotalWritten )
{
   // Local Data
   RESULT         DrvStatus;
   UINT32         StartTime;
   FLASH_DESC     *pFlashDesc;
   FLASH_CMDS     *pCmd;
   FLASH_SYS      *pSys;
   FLASH_STATUS   FlashStatus;
   
   // Local Data Initialization
   pFlashDesc    = &FlashDescription;
   pCmd          = pFlashDesc->pCmdSet;
   pSys          = &pFlashDesc->System;
   DrvStatus     = DRV_OK;
   FlashStatus   = FS_BUSY;
   *TotalWritten = 0;
   // Initialize in case the flash was busy before entering this function
   StartTime     = TTMR_GetHSTickCount();
   
   // Check that the Byte Count is Valid 
   ASSERT ((sizeof(FLASHDATA)) == ByteCount);

   // Mark the sector as being in program mode
   pFlashDesc->SectorStatus[Offset >> FLASH_SECTOR_SHIFT] = FSECT_PROGRAM;
   // Program Location in the Data Flash 
   pCmd->FlashProgram (pFlashDesc->BaseAddr, Offset/FLASH_BYTES_PER_OP, pData); 
   // Record the Start Time
   StartTime = TTMR_GetHSTickCount();
   // Increment the number of bytes written   
   *TotalWritten += FLASH_BYTES_PER_OP;
      
   while ( (FlashStatus == FS_BUSY) && (DrvStatus == DRV_OK) )
   {
      // Get the flash status
      FlashStatus = pCmd->FlashGetStatus(pFlashDesc->BaseAddr,
                                         Offset/FLASH_BYTES_PER_OP, FALSE);
      switch (FlashStatus)
      {
         case FS_EXCEEDED_TIME_LIMITS:
            // The Flash device has exceeded the internal timeout
            DrvStatus = DRV_FLASH_WORD_WR_DEV_TIMEOUT;
            // Issue a Reset Command to go to Read Mode
            pCmd->FlashReset(pFlashDesc->BaseAddr, Offset/FLASH_BYTES_PER_OP);
            break;
         case FS_WRITE_BUFFER_ABORT:
            // The Write buffer command has aborted
            DrvStatus = DRV_FLASH_BUF_WR_ABORT;
            // Issue a Flash Buffer Write Abort Reset
            pCmd->FlashAbortBuffer(pFlashDesc->BaseAddr);
            break;
         case FS_BUSY:
         case FS_SUSPEND:
         default:
            // Check if a software timeout has been detected
            if (((TTMR_GetHSTickCount() - StartTime) / TICKS_PER_uSec) >
                  pSys->MaxWriteTime_uS)
            {
               // Double check the status of the flash because we may have 
               // been interrupted.
               FlashStatus = pCmd->FlashGetStatus( pFlashDesc->BaseAddr,
                                                   Offset/FLASH_BYTES_PER_OP,
                                                   TRUE);
               if (FS_BUSY == FlashStatus)
               {                  
                  // The write has taken longer the maximum time
                  DrvStatus = DRV_FLASH_WORD_WR_SW_TIMEOUT;
                  // Issue a Reset Command to go to Read Mode
                  pCmd->FlashReset(pFlashDesc->BaseAddr, Offset/FLASH_BYTES_PER_OP);
               }
            }
            break;
      }
   }
   // Update the sector status
   pFlashDesc->SectorStatus[Offset >> FLASH_SECTOR_SHIFT] = FSECT_IDLE;

   return (DrvStatus);
}

/******************************************************************************
 * Function:    FlashRead
 *
 * Description: The Flash Read function is for handling reads from the data
 *              flash memory. The offset into the flash must be passed along
 *              with a location to store the data being read and the size to
 *              read.
 *
 * Parameters:  FLASHADDR nOffset   - Offset from the flash base address to read
 *              FLASHDATA *pData    - Pointer to the storage location
 *              BYTECOUNT ByteCount - Number of bytes to read
 *
 * Returns:     RESULT      [ DRV_OK ]
 *
 * Notes:       The Address and size must be aligned correctly before calling
 *              this function.
 *
 *****************************************************************************/
 void FlashRead (FLASHADDR nOffset, FLASHDATA *pData, BYTECOUNT ByteCount )
 {
   // Local Data
   FLASH_DESC     *pFlashDesc;
   UINT32         i;
   
   // Local Data Initialization
   pFlashDesc  = &FlashDescription;

   // Loop through the locations to read
   for ( i = 0; i < ByteCount/FLASH_BYTES_PER_OP; i++)
   {
      // Copy data to storage location
      *(pData + i) = FLASH_RD( pFlashDesc->BaseAddr,
                               ((nOffset/FLASH_BYTES_PER_OP) + i));
   }

}

/*****************************************************************************
 * Function:    FlashChipErase
 *
 * Description: This Function command and erase of the entire device located 
 *              at the base address.
 *
 * Parameters:  UINT32 *pTimeout_mS  pointer to the maximum chip erase time
 *                                   in mS.  This can be used by the caller
 *                                   to monitor the status of the erase
 *                                   with respect to the timeout condition.
 *
 * Returns:     DRV_RESULT [DRV_OK, DRV_FLASH_CHIP_ERASE_FAIL]
 *
 * Notes:       
 *
 ****************************************************************************/
RESULT FlashChipErase (UINT32 *pTimeout_mS)
{
   // Local Data
   FLASH_DESC     *pFlashDesc;
   FLASH_SYS      *pSys;
   FLASH_CMDS     *pCmd;
   RESULT         DrvStatus;

   // Local Data Initialization
   pFlashDesc   = &FlashDescription;
   pSys         = &pFlashDesc->System;
   pCmd         = pFlashDesc->pCmdSet;
   *pTimeout_mS = pSys->MaxChipErase_mS;
  
   // Command the Flash Chip Erase
   pCmd->FlashChipErase (pFlashDesc->BaseAddr);
   
   // Erase never started set a driver status error or OK
   DrvStatus = (pCmd->FlashGetStatus(pFlashDesc->BaseAddr, 0, FALSE) != FS_BUSY) ?
                DRV_FLASH_CHIP_ERASE_FAIL : DRV_OK;
   
   return (DrvStatus);
}

/*****************************************************************************
 * Function:    FlashGetDirectStatus
 *
 * Description: The Flash Get Direct Status function returns a complete 
 *              indication when called. If the flash is not busy this 
 *              function will return true for the complete indication. If
 *              Complete indicates false then the calling function should 
 *              check the driver status to verify there isn't an error 
 *              condition.  
 *
 * Parameters:  UINT32  Offset     - Flash Address Offset
 *              BOOLEAN *pComplete - [in/out] Storage location for complete
 *                                   flag.
 *
 * Returns:     RESULT - Driver Status [DRV_OK, DRV_FLASH_WR_DEV_TIMEOUT]  
 *
 * Notes:       This function doers not perform a software timeout. The 
 *              calling function should consider bounding the amount of time 
 *              this function is called.
 *
 ****************************************************************************/
RESULT FlashGetDirectStatus (UINT32 nOffset, BOOLEAN *pComplete)
{
   // Local Data
   FLASH_DESC   *pFlashDesc;
   FLASH_CMDS   *pCmd;
   FLASH_STATUS FlashStatus;
   RESULT       DrvStatus;
   
   // Initialize Local Data
   DrvStatus  = DRV_OK;
   pFlashDesc = &FlashDescription;
   pCmd       = pFlashDesc->pCmdSet;   
      
   // Get the flash status
   FlashStatus = pCmd->FlashGetStatus(pFlashDesc->BaseAddr,
                                      nOffset/FLASH_BYTES_PER_OP, FALSE);
   switch ( STPUI( FlashStatus, eTpFlashDirect, 2))
   {
      case FS_NOT_BUSY:
         *pComplete = TRUE;
         break;
      case FS_EXCEEDED_TIME_LIMITS:
         *pComplete = FALSE;
         // The Flash part decided it has timed out
         DrvStatus = DRV_FLASH_WORD_WR_DEV_TIMEOUT;
         // Issue a Reset Command to go to Read Mode
         pCmd->FlashReset(pFlashDesc->BaseAddr, nOffset/FLASH_BYTES_PER_OP);
         break;
      case FS_BUSY:
      default:
         // NO FATAL here because logic handles all unexpected cases
         *pComplete = FALSE;
         break;
   }

   return DrvStatus;
}

/*****************************************************************************
 * Function:    FlashSectorErase
 *
 * Description: This function commands a sector erase of the sector specified 
 *              by the Address Offset.
 *
 * Parameters:  FLASHADDR       Offset   - Offset of sector from base address
 *              FLASH_ERASE_CMD EraseCmd - Command to initiate 
 *                                         [FE_SETUP_CMD, FE_SECTOR_CMD, 
 *                                          FE_STATUS_CMD, FE_COMPLETE_CMD]
 *              BOOLEAN *bDone           - Pointer to storage location for 
 *                                         Done flag.
 *
 * Returns:     DRV_RESULT [DRV_OK, DRV_FLASH_WORD_WR_DEV_TIMEOUT,
 *                                  DRV_FLASH_WORD_WR_SW_TIMEOUT ] 
 *
 *              There is another return value for the case FS_SUSPEND which 
 *              sets the DrvStatus to DRV_FLASH_BUF_WR_SUSPENDED but is 
 *              preceeded by the comment that we'll never reach this case.
 *
 * Notes:       The calling function must first set up the sector erase by
 *              issuing the SETUP_CMD once. That can then be followed by
 *              one or more FET_SECTOR_CMDs. Each one of these commands
 *              would provide the offset to a new sector. A final command must
 *              be issued on erase completion for each sector,
 *              FET_COMPLETE_CMD, this will set the sector status back to
 *              IDLE.
 *  
 *              This function will keep track of the overall software timeout.
 *
 ****************************************************************************/
RESULT FlashSectorErase (FLASHADDR Offset, FLASH_ERASE_CMD EraseCmd, BOOLEAN *bDone )
{
   // Local Data
   FLASH_DESC     *pFlashDesc;
   FLASH_SYS      *pSys;
   FLASH_CMDS     *pCmd;
   RESULT         DrvStatus;
   static UINT32  SectorCount;
   static UINT32  StartTime;
   FLASH_STATUS   FlashStatus;

   // Local Data Initialization
   pFlashDesc = &FlashDescription;
   pSys       = &pFlashDesc->System;
   pCmd       = pFlashDesc->pCmdSet;
   DrvStatus  = DRV_OK;
   *bDone     = FALSE;
   
   switch (EraseCmd)
   {
      case FE_SETUP_CMD:
         // Issue the erase setup command
         pCmd->FlashSectorEraseSetup(pFlashDesc->BaseAddr);
         SectorCount = 0;
         break;
      case FE_SECTOR_CMD:
         // Count the sectors as they are commanded to be erased
         SectorCount++;

         // Update the sector status
         pFlashDesc->SectorStatus[Offset >> FLASH_SECTOR_SHIFT] = FSECT_ERASE;

         // Issue the erase command for the sector
         pCmd->FlashSectorEraseCmd(pFlashDesc->BaseAddr, Offset/FLASH_BYTES_PER_OP);
         // Record the time the last sector erase command was issued
         StartTime = TTMR_GetHSTickCount();
         break;
      case FE_STATUS_CMD:
         // Get the flash status
         FlashStatus = pCmd->FlashGetStatus(pFlashDesc->BaseAddr,
                                               Offset/FLASH_BYTES_PER_OP, FALSE);
         switch (FlashStatus)
         {
            case FS_EXCEEDED_TIME_LIMITS:
               // The Flash part decided it has timed out
               DrvStatus = DRV_FLASH_WORD_WR_DEV_TIMEOUT;
               // Issue a Reset Command to go to Read Mode
               pCmd->FlashReset(pFlashDesc->BaseAddr, Offset/FLASH_BYTES_PER_OP);
               *bDone = TRUE;
               break;
            case FS_NOT_BUSY:
               *bDone = TRUE;
               break;
            case FS_BUSY:
            case FS_SUSPEND:
            default:
               // If we remain busy or become suspended keep checking the time until we
               // reach a timeout.
               if (((TTMR_GetHSTickCount() - StartTime)/TICKS_PER_mSec) >
                    (pSys->MaxSectorErase_mS * SectorCount))
               {
                  // Double check the status of the flash because we may have 
                  // been interrupted.
                  FlashStatus = pCmd->FlashGetStatus( pFlashDesc->BaseAddr,
                                                      Offset/FLASH_BYTES_PER_OP,
                                                      TRUE);
                  if (FS_BUSY == FlashStatus)
                  {                  
                     // The flash has taken much longer to erase than expected
                     DrvStatus = DRV_FLASH_WORD_WR_SW_TIMEOUT;
                     // Issue a Reset Command to go to Read Mode
                     pCmd->FlashReset(pFlashDesc->BaseAddr, Offset/FLASH_BYTES_PER_OP);
                     *bDone = TRUE;
                  }
               }
               break;
         }
         break;
      case FE_COMPLETE_CMD:
         // The Erase is complete now update the sector status
         // This must be done by the calling function for each sector
         pFlashDesc->SectorStatus[Offset >> FLASH_SECTOR_SHIFT] = FSECT_IDLE;
         *bDone = TRUE;
         break;
      default:
         FATAL("Unexpected FLASH_ERASE_CMD = %d", EraseCmd );
         break;
   }

   return (DrvStatus);
}

/*****************************************************************************
 * Function:    FlashCheckSameSector
 *
 * Description: Verifies that two offsets from the base flash address are
 *              within the same sector. This is used to verify if a read
 *              or write are going to successful.
 *
 * Parameters:  nFirstOffset
 *              nLastOffset
 *
 * Returns:     BOOLEAN      [TRUE - Same sector, FALSE - Crosses boundary]
 *
 * Notes:       None.
 *
 ****************************************************************************/
BOOLEAN FlashCheckSameSector(FLASHADDR nFirstOffset, FLASHADDR nLastOffset)
{
   // Compare Sectors and Return
   return ((nFirstOffset>>FLASH_SECTOR_SHIFT)==(nLastOffset>>FLASH_SECTOR_SHIFT));
}

/*****************************************************************************
 * Function:    FlashGetInfo
 *
 * Description: Returns the geometry and the current state of the flash 
 *              driver.
 *
 * Parameters:  None.
 *
 * Returns:     FLASH_INFO - Geometry and State of Driver
 *
 * Notes:       None.
 *
 ****************************************************************************/
FLASH_INFO FlashGetInfo ( void )
{
   return (FlashDescription.Info);
}


/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/*****************************************************************************
 * Function:    FlashPopulateCFIData
 *
 * Description: This function will populate the Common Flash Interface
 *              Information. It includes Identification, Interface, and 
 *              Geometry of the Flash Device; 
 *
 * Parameters:  FLASH_ID  *pId  - Pointer to the Identification Storage Area
 *              FLASH_SYS *pSys - Pointer to the System Interace Storage Area
 *              FLASH_INFO *pInfo - Ptr to Device Geometry Storage Structure
 *
 * Returns:     RESULT
 *
 * Notes:       This function initiates the CFI Query Mode that the 
 *              Sub-functions require to retrieve the CFI data.
 *
 ****************************************************************************/
static
RESULT FlashPopulateCFIData( FLASH_ID *pId, FLASH_SYS *pSys,
                             FLASH_INFO *pInfo )
{
   // Local Data
   FLASH_DESC     *pFlashDesc;
   RESULT         DrvStatus;
  
   // Local Data Initialization
   pFlashDesc = &FlashDescription;
   DrvStatus  = DRV_OK;
   
   // Command the Flash Device to Enter CFI Mode
   FLASH_WR(pFlashDesc->BaseAddr, FLASH_CFI_ADDRESS, FLASH_CFI_QUERY_CMD);
   
   // Retrieve the Flash Identification Data
   FlashGetCFIIdentification (pId);
   
   // Retrieve the System Interface Data
   DrvStatus = FlashGetCFISystemInterfaceData (pSys);
      
   // Check that the System Interface Data is OK
   if (DrvStatus == DRV_OK)
   {
      // Retrieve the Flash Device Geometry
      DrvStatus = FlashGetCFIDeviceGeometry(pInfo);
   }
   
   /* Write Software RESET command */
   FLASH_WR(pFlashDesc->BaseAddr, 0, NOR_RESET_CMD);  

   // Return the Driver Status to the calling function
   return DrvStatus;
}


/*****************************************************************************
 * Function:    FlashGetCFIIdentification
 *
 * Description: The FlashGetCFIIdentification will retrieve the CFI
 *              Identification parameters. These parameters include the
 *              flash command set, Primary Extended Table Offset,  
 *              Alternate Command Set and Alternate Table Offset.
 *
 * Parameters:  FLASH_ID  *pId  - Pointer to the Identification Storage Area
 *
 * Returns:     None.
 *
 * Notes:       The CFI Query Command must be performed before entering 
 *              this function.
 *
 ****************************************************************************/
static 
void FlashGetCFIIdentification ( FLASH_ID *pId)
{
   // Local Data
   FLASH_DESC *pFlashDesc;

   // Local Data Initialization
   pFlashDesc = &FlashDescription;

   //Command Set
   pId->PrimaryCmdSet = (FLASH_CMD_SET)
           ((FLASH_RD(pFlashDesc->BaseAddr, CFI_COMMAND_SET_LSB) + 
            (FLASH_RD(pFlashDesc->BaseAddr, CFI_COMMAND_SET_MSB) <<
             CFI_MSB_SHIFT)) & FLASH_DEV_READ_MASK);
   
   //Primary Extended Table Offset
   pId->ExtendedTableOffset = (UINT16)
      (FLASH_RD(pFlashDesc->BaseAddr, CFI_PRIMARY_EXTENDED_TABLE_LSB) + 
      (FLASH_RD(pFlashDesc->BaseAddr, CFI_PRIMARY_EXTENDED_TABLE_MSB) <<
       CFI_MSB_SHIFT)) & FLASH_DEV_READ_MASK;

   //Alternate Command Set
   pId->AlternateCmdSet = (FLASH_CMD_SET)
           (FLASH_RD(pFlashDesc->BaseAddr, CFI_ALTERNATE_COMMAND_SET_LSB) + 
           (FLASH_RD(pFlashDesc->BaseAddr, CFI_ALTERNATE_COMMAND_SET_MSB) <<
            CFI_MSB_SHIFT)) & FLASH_DEV_READ_MASK;
   
   //Alternate Extended Table Offset
   pId->AlternateTableOffset = (UINT16)
      (FLASH_RD(pFlashDesc->BaseAddr, CFI_ALTERNATE_EXTENDED_TABLE_LSB) + 
      (FLASH_RD(pFlashDesc->BaseAddr, CFI_ALTERNATE_EXTENDED_TABLE_MSB) <<
       CFI_MSB_SHIFT)) & FLASH_DEV_READ_MASK;
}

/*****************************************************************************
 * Function:    FlashGetCFISystemInterfaceData
 *
 * Description: The FlashGetCFISystemInterfaceData retrieves the common
 *              flash interface System Interface Data.
 *
 * Parameters:  FLASH_SYS *pSys - Pointer to the System Interface Storage Area
 *
 *
 * Returns:     DRV_RESULT [DRV_OK, DRV_FLASH_TYP_WRITE_TIME_NOT_VALID,
 *                                  DRV_FLASH_TYP_BUF_TIME_NOT_VALID,
 *                                  DRV_FLASH_TYP_SECTOR_ERASE_NOT_VALID,
 *                                  DRV_FLASH_TYP_CHIP_ERASE_NOT_VALID,
 *                                  DRV_FLASH_MAX_WRITE_NOT_VALID,
 *                                  DRV_FLASH_MAX_BUF_TIME_NOT_VALID,
 *                                  DRV_FLASH_MAX_SECTOR_ERASE_NOT_VALID,
 *                                  DRV_FLASH_MAX_CHIP_ERASE_NOT_VALID ]
 *
 * Notes:       The CFI Query Command must be performed before entering 
 *              this function.
 *
 ****************************************************************************/
static 
RESULT FlashGetCFISystemInterfaceData ( FLASH_SYS *pSys )
{
   // Local Data
   FLASH_DESC     *pFlashDesc;
   UINT32         DataRaw;
   RESULT         DrvStatus;        
   UINT8          i;
   UINT32         *pTypicalValue;
   FLASH_SYS_CALC *pTable;
    
   // Local Data Initialization
   
   // Create Local Table so that a nested if structure is not needed to 
   // check for CFI errors.
   FLASH_SYS_CALC SystemDataTable [8] = 
   { 
      // Parameter Offset,           BCD digits / Index to Typical,
      // Pointer to Parameter to Update,
      // Type of Conversion to Perform, Fault Code
      {CFI_TYP_SINGLE_WRITE_TIMEOUT, 0, NULL, TIME_2n_TYPICAL,
       DRV_FLASH_TYP_WRITE_TIME_NOT_VALID                                   },
      {CFI_MIN_BUFFER_TIMEOUT,       0, NULL, TIME_2n_TYPICAL,
       DRV_FLASH_TYP_BUF_TIME_NOT_VALID                                     },
      {CFI_TYP_SECTOR_ERASE_TIMEOUT, 0, NULL, TIME_2n_TYPICAL,
       DRV_FLASH_TYP_SECTOR_ERASE_NOT_VALID                                 },
      {CFI_TYP_CHIP_ERASE_TIMEOUT,   0, NULL, TIME_2n_TYPICAL,
       DRV_FLASH_TYP_CHIP_ERASE_NOT_VALID                                   },
      {CFI_MAX_SINGLE_WRITE_TIMEOUT, 0, NULL, TIME_2n_TIMES_TYPICAL,
       DRV_FLASH_MAX_WRITE_NOT_VALID                                        },
      {CFI_MAX_BUFFER_TIMEOUT,       1, NULL, TIME_2n_TIMES_TYPICAL,
       DRV_FLASH_MAX_BUF_TIME_NOT_VALID                                     },
      {CFI_MAX_SECTOR_ERASE_TIMEOUT, 2, NULL, TIME_2n_TIMES_TYPICAL,
       DRV_FLASH_MAX_SECTOR_ERASE_NOT_VALID                                 },
      {CFI_MAX_CHIP_ERASE_TIMEOUT,   3, NULL, TIME_2n_TIMES_TYPICAL,
       DRV_FLASH_MAX_CHIP_ERASE_NOT_VALID                                   } 
   };
   
   pFlashDesc = &FlashDescription;

   // Initialize the pointers to the parameters to store
   SystemDataTable[0].pParam  = &pSys->TypWriteTime_uS;
   SystemDataTable[1].pParam  = &pSys->TypMinBufferTime_uS;
   SystemDataTable[2].pParam  = &pSys->TypSectorErase_mS;
   SystemDataTable[3].pParam  = &pSys->TypChipErase_mS;
   SystemDataTable[4].pParam  = &pSys->MaxWriteTime_uS;
   SystemDataTable[5].pParam  = &pSys->MaxMinBufferTime_uS;
   SystemDataTable[6].pParam  = &pSys->MaxSectorErase_mS;
   SystemDataTable[7].pParam  = &pSys->MaxChipErase_mS;
      
   DrvStatus  = DRV_OK;
   
   // Loop through the System Data Table and retrieve CFI data and
   // check for possible errors
   for ( i = 0; i <  (sizeof(SystemDataTable)/sizeof(FLASH_SYS_CALC)) && 
                     (DrvStatus == DRV_OK); i++ )
   {
      pTable = &SystemDataTable[i];
      
      // Read the CFI data and mask it
      DataRaw  = (UINT32)(FLASH_RD(pFlashDesc->BaseAddr, pTable->ParameterOffset));
      DataRaw &= FLASH_DEV_READ_MASK; 
      
      // Check if calculating typical time (2^n)
      if (TIME_2n_TYPICAL == pTable->Conversion)
      {
         *(pTable->pParam) = 1 << DataRaw; 
         // Check that the time value is at least 1
         if ( *(pTable->pParam) < STPU( 1, eTpFlash3105c) )
         {
            // Set the Status of the Driver Fault
            DrvStatus = pTable->DrvFaultCode;
         }         
      }
      else // Calculating Max Time (2^n * Typical)
      {
         pTypicalValue     = SystemDataTable[pTable->BCDSize].pParam; 
         *(pTable->pParam) = (1 << DataRaw )* *pTypicalValue; 
            
         // Check that the time value is at lease 1
         if ( *(pTable->pParam) < STPU( 1, eTpFlash3105d) )
         {
            // Set Status of the Driver Fault
            DrvStatus = pTable->DrvFaultCode;
         }        
      }
   }
   
   return DrvStatus;
}

/*****************************************************************************
 * Function:    FlashGetCFIDeviceGeometry
 *
 * Description: The FlashGetCFIDeviceGeometry will read the device geometry 
 *              parameters from the Flash Device.
 *              
 * Parameters:  FLASH_INFO *pInfo - Ptr to Device Geometry
 *
 * Returns:     DRV_RESULT [DRV_OK, DRV_FLASH_GEOMETRY_NOT_VALID, 
 *                                  DRV_FLASH_SECTOR_DATA_NOT_VALID ]
 *
 * Notes:       The CFI Query Command must be performed before entering 
 *              this function.
 *
 ****************************************************************************/
static 
RESULT FlashGetCFIDeviceGeometry ( FLASH_INFO *pInfo )
{
   // Local Data
   FLASH_DESC *pFlashDesc;
   UINT16     i;
   UINT16     EbrOffset;
   RESULT     DrvStatus;
   UINT32     PowerTemp;
   UINT32     DeviceSize;
   UINT32     MaxBytes;
     
   // Initialize Local Data
   DrvStatus  = DRV_OK;
   pFlashDesc = &FlashDescription;
   
   // Flash Device Size in Bytes
   DeviceSize = FLASH_RD(pFlashDesc->BaseAddr, CFI_DEVICE_SIZE) & FLASH_DEV_READ_MASK;
   PowerTemp  = 1 << DeviceSize; 
   
   pInfo->DeviceSize = PowerTemp * FLASH_NUMBER_OF_PARTS;
                            
   // Flash Device Width
   pInfo->InterfaceDesc = (FLASH_INTERFACE)
                 ((FLASH_RD(pFlashDesc->BaseAddr, CFI_DEVICE_INTERFACE_LSB) +
                  (FLASH_RD(pFlashDesc->BaseAddr, CFI_DEVICE_INTERFACE_MSB)
                 << CFI_MSB_SHIFT)) & FLASH_DEV_READ_MASK);

   // Max Number of Byte Writes
   MaxBytes = (FLASH_RD(pFlashDesc->BaseAddr, CFI_BYTES_PER_BUFFERED_WRITE_LSB) +
              (FLASH_RD(pFlashDesc->BaseAddr, CFI_BYTES_PER_BUFFERED_WRITE_MSB)
              << CFI_MSB_SHIFT)) & FLASH_DEV_READ_MASK;
   PowerTemp = 1 << MaxBytes; 

   pInfo->MaxByteWrite = (UINT16)(PowerTemp * FLASH_NUMBER_OF_PARTS);

   // Number of Erase Block Regions
   pInfo->NumEraseBlocks = (UINT16)
                 (FLASH_RD(pFlashDesc->BaseAddr, CFI_NUMBER_OF_ERASE_BLOCK_REGIONS) 
                 & FLASH_DEV_READ_MASK);
   
   if ( (pInfo->DeviceSize     == 0) || (pInfo->MaxByteWrite <= 1) ||
        (pInfo->NumEraseBlocks == 0) || 
        (STPU( pInfo->NumEraseBlocks, eTpFlash3105a) > CFI_MAX_ERASE_BLOCKS) )
   {
      DrvStatus = DRV_FLASH_GEOMETRY_NOT_VALID;
   }   
   
   // Erase Block Information
   for (i = 0; (i < pInfo->NumEraseBlocks) && (DrvStatus == DRV_OK); i++)
   {
      // EbrOffset = i * 4;
      EbrOffset = i * sizeof(UINT32);
    
      // Sector Size
      pInfo->EraseBlockInfo[i].SectorSize = 
                 ((FLASH_RD(pFlashDesc->BaseAddr, EbrOffset + CFI_EBR_START + 
                                  CFI_EBR_SECTOR_SIZE_LSB) +
                  (FLASH_RD(pFlashDesc->BaseAddr, EbrOffset+CFI_EBR_START+
                                  CFI_EBR_SECTOR_SIZE_MSB) << CFI_MSB_SHIFT)) 
                 & FLASH_DEV_READ_MASK) * CFI_EBR_SECTOR_MULTIPLIER * 
                 FLASH_NUMBER_OF_PARTS;
     
      // Number of Sectors
      pInfo->EraseBlockInfo[i].NumSectors = 
                 ((FLASH_RD(pFlashDesc->BaseAddr, EbrOffset + CFI_EBR_START + 
                                       CFI_EBR_NUMBER_OF_SECTORS_LSB) +
                  (FLASH_RD(pFlashDesc->BaseAddr, EbrOffset + CFI_EBR_START + 
                                       CFI_EBR_NUMBER_OF_SECTORS_MSB) << CFI_MSB_SHIFT)) + 
                  CFI_EBR_SECTOR_ADJUST) & FLASH_DEV_READ_MASK;
                  
       if ( (pInfo->EraseBlockInfo[i].SectorSize == 0) || 
            (pInfo->EraseBlockInfo[i].NumSectors< MIN_POSSIBLE_SECTORS) || 
            (STPU( pInfo->EraseBlockInfo[i].NumSectors, eTpFlash3105b) > MAX_POSSIBLE_SECTORS) )
       {
          DrvStatus = DRV_FLASH_SECTOR_DATA_NOT_VALID;
       }
   }
    
   return (DrvStatus);
}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: Flash.c $
 * 
 * *****************  Version 30  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 29  *****************
 * User: Jeff Vahue   Date: 10/27/11   Time: 9:48p
 * Updated in $/software/control processor/code/drivers
 * SCR# 1077 - Code Coverage need a startup TP
 * 
 * *****************  Version 28  *****************
 * User: Jeff Vahue   Date: 10/27/11   Time: 7:45p
 * Updated in $/software/control processor/code/drivers
 * SCR# 1077 - Code Coverage Test Points
 * 
 * *****************  Version 27  *****************
 * User: Jeff Vahue   Date: 11/01/10   Time: 7:50p
 * Updated in $/software/control processor/code/drivers
 * SCR# 975 - Code Coverage refactor
 * 
 * *****************  Version 26  *****************
 * User: Jeff Vahue   Date: 10/18/10   Time: 8:31p
 * Updated in $/software/control processor/code/drivers
 * SCR# 848 - Code Coverage
 * 
 * *****************  Version 25  *****************
 * User: Jeff Vahue   Date: 10/18/10   Time: 7:42p
 * Updated in $/software/control processor/code/drivers
 * SCR# 848 - Code Coverage
 * 
 * *****************  Version 24  *****************
 * User: Jeff Vahue   Date: 10/18/10   Time: 7:03p
 * Updated in $/software/control processor/code/drivers
 * SCR# 848: Code Coverage
 * 
 * *****************  Version 23  *****************
 * User: John Omalley Date: 10/18/10   Time: 6:47p
 * Updated in $/software/control processor/code/drivers
 * SCR 956 - Code Coverage Update
 * 
 * *****************  Version 22  *****************
 * User: John Omalley Date: 10/01/10   Time: 5:24p
 * Updated in $/software/control processor/code/drivers
 * SCR 893 - Updates for Code Coverage - Removed Commented out code
 * 
 * *****************  Version 21  *****************
 * User: John Omalley Date: 9/10/10    Time: 10:00a
 * Updated in $/software/control processor/code/drivers
 * SCR 858 - Added FATAL to defaults
 * 
 * *****************  Version 20  *****************
 * User: John Omalley Date: 9/03/10    Time: 9:45a
 * Updated in $/software/control processor/code/drivers
 * SCR 825 - Removed debug write checks
 * 
 * *****************  Version 19  *****************
 * User: John Omalley Date: 9/03/10    Time: 9:42a
 * Updated in $/software/control processor/code/drivers
 * SCR 825 - Checking in Debug Version that verifies buffer writes
 * 
 * *****************  Version 18  *****************
 * User: Peter Lee    Date: 9/01/10    Time: 2:50p
 * Updated in $/software/control processor/code/drivers
 * SCR #840 Code Review Updates
 * *****************  Version 17  *****************
 * User: Peter Lee    Date: 8/31/10    Time: 3:29p
 * Updated in $/software/control processor/code/drivers
 * SCR #806 Code Review Updates
 * 
 * *****************  Version 16  *****************
 * User: Contractor3  Date: 5/06/10    Time: 11:27a
 * Updated in $/software/control processor/code/drivers
 * Check In SCR #586
 * 
 * *****************  Version 15  *****************
 * User: John Omalley Date: 5/05/10    Time: 1:14p
 * Updated in $/software/control processor/code/drivers
 * SCR# 350 - Changed FlashWriteBufferMode if ByteCount<=0 to ByteCount ==
 * 0
 * 
 * *****************  Version 14  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:14p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 13  *****************
 * User: Jeff Vahue   Date: 2/24/10    Time: 12:58p
 * Updated in $/software/control processor/code/drivers
 * SCR# 364 - remove unused functions 
 * 
 * *****************  Version 12  *****************
 * User: Jeff Vahue   Date: 2/19/10    Time: 12:57p
 * Updated in $/software/control processor/code/drivers
 * SCR# 455 - remove LINT issues
 * 
 * *****************  Version 11  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:23p
 * Updated in $/software/control processor/code/drivers
 * SCR 31
 * 
 * *****************  Version 10  *****************
 * User: Peter Lee    Date: 9/16/09    Time: 1:15p
 * Updated in $/software/control processor/code/drivers
 * SCR #94, #95 System Log Id returned. 
 * 
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 9/15/09    Time: 6:09p
 * Updated in $/software/control processor/code/drivers
 * SCR #94, #95 PBIT returns log structure to Init Mgr
 * 
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 9/14/09    Time: 3:33p
 * Updated in $/software/control processor/code/drivers
 * SCR #94 Updated Init for SET_CHECK() and return ERR CODE Struct
 * 
 * *****************  Version 7  *****************
 * User: John Omalley Date: 8/06/09    Time: 3:45p
 * Updated in $/software/control processor/code/drivers
 * SCR 179
 *    Removed Unused flash devices from the device table
 *    Added logic to not use the Flash and Memory Manager if the flash
 * driver initialization fails.
 *    Initialized any unitialized variables.
 * SCR 234 
 *    Removed the math.h power function and used a shift instead.
 * 
 * *****************  Version 6  *****************
 * User: Peter Lee    Date: 6/29/09    Time: 2:34p
 * Updated in $/software/control processor/code/drivers
 * SCR #204 Misc Code Updates
 * 
 * *****************  Version 5  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 11:57a
 * Updated in $/control processor/code/drivers
 * SCR #87 Function Prototype
 * 
 *
 ***************************************************************************/
