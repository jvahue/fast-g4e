#define MEM_MNGR_BODY
/******************************************************************************
            Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:        MemManager.c
    
    Description: The Memory Manager Package is responsible for managing the
                 interface from the application code to the Data Flash.
                 The memory manager implements a state machine that will
                 only allow operations to be performed if memory is in the
                 correct state.

                 The Memory Manager does not allow for suspending of erase
                 or program operations. 

    Notes:       
                 
  VERSION
    $Revision: 44 $  $Date: 12-11-13 2:09p $                 
                 
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    
#include "mcf548x.h"
#include <string.h>

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "memmanager.h"
#include "GSE.h"
#include "resultcodes.h"
#include "ttmr.h"
#include "Watchdog.h"

#include "TestPoints.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/

#define MEM_MGR_BUFFER_SIZE       1024
#define MEM_MGR_NUM_SECTORS       1024

#define MEM_CHIPERASE_WD_STARUP      300000
#define MEM_FINDCURROFFSET_WD_STARUP 400000

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
 
/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
// Memory Description - { NumBlocks, { nSectors, StartAddress, EndAddress, 
//                                     CurrentAddress, Used, Free } }
const  MEM_MAP               ConstMemMap      = { 1, { MEM_MGR_NUM_SECTORS, 0, 0, 0, 0, 0 } };
static MEM_DESC              MemDescription;
static MEM_STATE             MemCurrentState;
static UINT32                MemClearBuffer[MEM_MGR_BUFFER_SIZE];

static MEM_PROG_TASK_PARMS   MemProgPBlock;
static MEM_ERASE_TASK_PARMS  MemErasePBlock;
static MEM_CLEAR_TASK_PARMS  MemClearPBlock;

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
//static BOOLEAN MemStateNotBusy ( MEM_CMD Cmd, MEM_BLOCK_TYPE nBlock,
//                                 UINT32 *nBlockOffset, UINT32 *pData,
//                                 UINT32 nData, RESULT *ResultCode ); 

static BOOLEAN MemStateProgram ( MEM_CMD Cmd, MEM_BLOCK_TYPE nBlock,
                                 UINT32 *nBlockOffset, UINT32 *pData,
                                 UINT32 nData, RESULT *ResultCode );

static BOOLEAN MemStateErase   ( MEM_CMD Cmd, MEM_BLOCK_TYPE nBlock,
                                 UINT32 *nBlockOffset, UINT32 *pData,
                                 UINT32 nData, RESULT *ResultCode );

static BOOLEAN MemStateRead    ( MEM_CMD Cmd, MEM_BLOCK_TYPE nBlock,
                                 UINT32 *nBlockOffset, UINT32 *pData,
                                 UINT32 nData, RESULT *ResultCode );

// Array of pointers to Memory State Functions
//static BOOLEAN ( *state_table[MEM_MAX_STATE] ) (MEM_CMD Cmd,   MEM_BLOCK_TYPE nBlock,
//                                                UINT32 *nBlockOffset,
//                                                UINT32 *pData, UINT32 nData,
//                                                RESULT *ResultCode );

// Background tasks which are used to program, erase and clear 
static void    MemProgTask           (void* pPBlock);
static void    MemClearTask          (void* pPBlock);

// Local function used to verify passed in address is OK
static RESULT  MemCheckAddr      ( MEM_BLOCK_TYPE nBlock, UINT32 nBlockOffset,
                                   UINT32 nSize );
// Finds the next valid location that is erased
static UINT32  MemFindCurrentOffset ( UINT32 nBlock );

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/*****************************************************************************
 * Function:    MemInitialize
 *
 * Description: Initialize all memory data and control structures necessary for 
 *              managing the memory and underlying flash device.
 *
 *              The MemInitialize function will call the Flash Initialize
 *              function and the Flash initialize function will only return
 *              information needed by the memory manager. The memory manager
 *              only needs the size of the device and the size of the
 *              sectors to manage the memory. All other Flash specific
 *              information such as the algorithms and time limits will remain
 *              in the flash driver.
 *
 * Parameters:  None
 *
 * Returns:     RESULT [ SYS_OK, SYS_MEM_INIT_FAIL ]
 *
 * Notes:      
 *
 ****************************************************************************/
void MemInitialize ( void )
{
  // Local Data
  MEM_DESC   *pMemDesc;
  MEM_MAP    *pMap;
  MEM_BLOCK  *pBlock;
  FLASH_INFO FlashInfo;
  UINT32     BlockIndex;
  RESULT     SysStatus;
  UINT32     nSize;
  TCB        TaskInfo;
    
   // Initialize Local Data 
  pMemDesc   = &MemDescription;
  pMap       = &pMemDesc->MemMap;
  SysStatus  = SYS_OK;
  
  // Clear the MemClearBuffer
  memset (MemClearBuffer, 0, sizeof(MemClearBuffer));
  
  // Copy the constant memory map to a RAM copy
  memcpy (pMap, &ConstMemMap, sizeof(pMemDesc->MemMap));
  
  // Initialize the State Machine
  MemCurrentState           = MEM_IDLE;
  //state_table[MEM_NOT_BUSY] = MemStateNotBusy;
  //state_table[MEM_ERASE   ] = MemStateErase;
  //state_table[MEM_PROG    ] = MemStateProgram;
  //state_table[MEM_READ    ] = MemStateRead;
   
  // Read the current state of the Flash Device
  FlashInfo = FlashGetInfo();
  
  // Check that the Flash Driver is OK
  if ( FLASH_DRV_STATUS_OK == FlashInfo.DrvStatus )
  {
     // Populate Memory Description Geometry
     pMemDesc->nTotalSize   = FlashInfo.DeviceSize;
     pMemDesc->nAllocated   = 0;
     pMemDesc->nFree        = pMemDesc->nTotalSize;
     // Note: This logic and the flash logic assumes only one
     //       erase block is available in the flash part.
     pMemDesc->nSectorSize  = FlashInfo.EraseBlockInfo[0].SectorSize;

     // Verify that the number of blocks in the memory map is not 0
     ASSERT( pMap->nBlocks != 0 );
        
     // Loop through the memory blocks
     for (BlockIndex = 0; BlockIndex < pMap->nBlocks; BlockIndex++)
     {
        // Set pointer to the current block being populated
        pBlock = &pMap->MemBlock[BlockIndex];
   
        // Verify the block uses at least one sector
        ASSERT( pBlock->nSectors != 0 );

        // Set the size of the current block
        nSize = pBlock->nSectors * pMemDesc->nSectorSize;

        // Verify that we don't over allocate memory
        ASSERT ( pMemDesc->nAllocated + nSize <= pMemDesc->nTotalSize);

        // Set up the blocks data         
        pBlock->nStartOffset    = pMemDesc->nAllocated;
        pBlock->nEndOffset      = pBlock->nStartOffset + nSize;
        // Find the next free address
        pBlock->nCurrentOffset  = MemFindCurrentOffset(BlockIndex);
        pBlock->nUsed           = pBlock->nCurrentOffset - pBlock->nStartOffset;
        pBlock->nFree           = pBlock->nEndOffset - pBlock->nUsed;
        pMemDesc->nAllocated   += nSize;
        pMemDesc->nFree        -= nSize;
     }   
  }
  else // The Flash Initialization Failed
  {
     SysStatus = SYS_MEM_INIT_FAIL;
  }
  
  pMemDesc->MemMgrStatus = SysStatus;

  // Create the Flash Program Task
  memset(&TaskInfo, 0, sizeof(TaskInfo));
  strncpy_safe(TaskInfo.Name, sizeof(TaskInfo.Name), "Mem Program Task",_TRUNCATE);
  TaskInfo.TaskID         = Mem_Program_Task;
  TaskInfo.Function       = MemProgTask;
  TaskInfo.Priority       = taskInfo[Mem_Program_Task].priority;
  TaskInfo.Type           = taskInfo[Mem_Program_Task].taskType;
  TaskInfo.modes          = taskInfo[Mem_Program_Task].modes;
  TaskInfo.MIFrames       = taskInfo[Mem_Program_Task].MIFframes;
  TaskInfo.Rmt.InitialMif = taskInfo[Mem_Program_Task].InitialMif;
  TaskInfo.Rmt.MifRate    = taskInfo[Mem_Program_Task].MIFrate;
  TaskInfo.Enabled        = TRUE;
  TaskInfo.Locked         = FALSE;
  TaskInfo.pParamBlock    = &MemProgPBlock;

  TmTaskCreate (&TaskInfo);

  // Create the Flash Erase Task
  memset(&TaskInfo, 0, sizeof(TaskInfo));
  strncpy_safe(TaskInfo.Name, sizeof(TaskInfo.Name), "Mem Erase Task",_TRUNCATE);  
  TaskInfo.TaskID         = Mem_Erase_Task;
  TaskInfo.Function       = MemEraseTask;
  TaskInfo.Priority       = taskInfo[Mem_Erase_Task].priority;
  TaskInfo.Type           = taskInfo[Mem_Erase_Task].taskType;
  TaskInfo.modes          = taskInfo[Mem_Erase_Task].modes;
  TaskInfo.MIFrames       = taskInfo[Mem_Erase_Task].MIFframes;
  TaskInfo.Rmt.InitialMif = taskInfo[Mem_Erase_Task].InitialMif;
  TaskInfo.Rmt.MifRate    = taskInfo[Mem_Erase_Task].MIFrate;
  TaskInfo.Enabled        = FALSE;
  TaskInfo.Locked         = FALSE;
  TaskInfo.pParamBlock    = &MemErasePBlock;

  TmTaskCreate (&TaskInfo);

  // Create the Flash Clear Task
  memset(&TaskInfo, 0, sizeof(TaskInfo));  
  strncpy_safe(TaskInfo.Name, sizeof(TaskInfo.Name), "Mem Clear Task",_TRUNCATE);
  TaskInfo.TaskID         = Mem_Clear_Task;
  TaskInfo.Function       = MemClearTask;
  TaskInfo.Priority       = taskInfo[Mem_Clear_Task].priority;
  TaskInfo.Type           = taskInfo[Mem_Clear_Task].taskType;
  TaskInfo.modes          = taskInfo[Mem_Clear_Task].modes;
  TaskInfo.MIFrames       = taskInfo[Mem_Clear_Task].MIFframes;
  TaskInfo.Rmt.InitialMif = taskInfo[Mem_Clear_Task].InitialMif;
  TaskInfo.Rmt.MifRate    = taskInfo[Mem_Clear_Task].MIFrate;
  TaskInfo.Enabled        = FALSE;
  TaskInfo.Locked         = FALSE;
  TaskInfo.pParamBlock    = &MemClearPBlock;

  TmTaskCreate (&TaskInfo);
  
} // End of MemInitialize()

/******************************************************************************
 * Function:     MemSetupErase
 *
 * Description:  Calculates beginning and end flash addresses based on nBlock
 *               and nOffset passed in.  Uses these values to call the function
 *               FlashSectorErase() with the FE_SETUP_CMD which is required by
 *               that function for setup before sector erase calls occur.
 *
 * Parameters:   MEM_BLOCK_TYPE nBlock
 *               UINT32         nOffset
 *               BOOLEAN        *bDone
 *
 * Returns:      None
 *
 * Notes:        This function must be called once before MemCommandErase calls
 *               are made.
 *
 *
 *****************************************************************************/
void MemSetupErase (MEM_BLOCK_TYPE nBlock, UINT32 nOffset, BOOLEAN *bDone)
{
   // Local Data
   MEM_DESC   *pMemDesc;
   MEM_BLOCK  *pBlock; 
   UINT32     nStart;  
   
   pMemDesc     = &MemDescription;
   pBlock       = &pMemDesc->MemMap.MemBlock[nBlock];
   
   // Calculate the begining and end flash addresses
   nStart = pBlock->nStartOffset + nOffset;
   
   FlashSectorErase (nStart, FE_SETUP_CMD, bDone);
   // Always returns DRV_OK when FE_SETUP_CMD or FE_SECTOR_CMD   
   // no need to do anything with the Result
}

/******************************************************************************
 * Function:     MemCommandErase
 *
 * Description:  Calculates beginning and end flash addresses based on nBlock,
 *               nOffset and nSize passed in.  Loops through each sector in the
 *               range, calling FlashSectorErase() with the FE_SECTOR_CMD for
 *               each sector to command the erase.
 *
 * Parameters:   MEM_BLOCK_TYPE nBlock
 *               UINT32         nOffset
 *               UINT32         nSize
 *               BOOLEAN        *bDone
 *
 * Returns:      None
 *
 * Notes:        Calling function must call MemSetupErase once first before 
 *               making calls to MemCommandErase.
 *
 *
 *****************************************************************************/
void MemCommandErase (MEM_BLOCK_TYPE nBlock, UINT32 nOffset,
                      UINT32 nSize, BOOLEAN *bDone)
{
   // Local Data
   MEM_DESC  *pMemDesc;
   MEM_BLOCK *pBlock;
   UINT32    i;
   UINT32    nStart;
   UINT32    nEnd;
   
   // Local Data Initialization
   pMemDesc     = &MemDescription;
   pBlock       = &pMemDesc->MemMap.MemBlock[nBlock];
   
   // Calculate the begining and end flash addresses
   nStart = pBlock->nStartOffset + nOffset;
   nEnd   = nStart + nSize;
   
   // Loop through each sector in the range
   for ( i = nStart; (i < nEnd); i += pMemDesc->nSectorSize )
   {
      // This function will program each sector to start the erase
      FlashSectorErase(i, FE_SECTOR_CMD, bDone);
      // Always returns DRV_OK when FE_SETUP_CMD or FE_SECTOR_CMD   
      // no need to do anything with the Result
   }
}

/******************************************************************************
 * Function:     MemStatusErase
 *
 * Description:  Calculates beginning and end flash addresses based on nBlock,
 *               nOffset and nSize passed in.  Uses these values to call the
 *               function FlashSectorErase() with the FE_STATUS_CMD to get the
 *               status of the sector erases.  Follows up with a call to 
 *               FlashSectorErase() with the FE_COMPLETE_CMD which is necessary
 *               to set the sector status back to IDLE.
 *
 * Parameters:   MEM_BLOCK_TYPE nBlock
 *               UINT32         nOffset
 *               UINT32         nSize
 *               BOOLEAN        *bDone
 *
 * Returns:      RESULT
 *
 * Notes:        None
 *
 *
 *****************************************************************************/
RESULT MemStatusErase (MEM_BLOCK_TYPE nBlock, UINT32 nOffset,
                       UINT32 nSize, BOOLEAN *bDone)
{
   // Local Data
   RESULT    Result;
   MEM_DESC  *pMemDesc;
   MEM_BLOCK *pBlock;   
   UINT32    i;
   UINT32    nStart;
   UINT32    nEnd;

   // Local Data Initialization
   pMemDesc     = &MemDescription;
   pBlock       = &pMemDesc->MemMap.MemBlock[nBlock];
   
   // Calculate the begining and end flash addresses
   nStart = pBlock->nStartOffset + nOffset;
   nEnd   = nStart + nSize;
     
   // Check the status of the sector erases
   Result = FlashSectorErase (nStart, FE_STATUS_CMD, bDone);
   
   if (TRUE == *bDone)
   {
      // loop through all sectors to set status back to idle
      for ( i = nStart; i < nEnd; i += pMemDesc->nSectorSize )
      {
         // This will set all the erased sectors back to idle
         FlashSectorErase(i, FE_COMPLETE_CMD, bDone);
      }   
   }
   
   return Result;
}

/******************************************************************************
 * Function:     MemRead
 *
 * Description:  The Memory read function validates the read request and then
 *               initiates the read. If the read exceeds the current sector
 *               boundary the read will move the Offset to the next sector and
 *               return the data from that sector. The calling function must
 *               pass a pointer with the offset.
 *
 * Parameters:   UINT32 nBlock        - Memory block to perform read in
 *               UINT32 *nBlockOffset - Offset into the block
 *               UINT32 *pBuf         - pointer to storage location
 *               UINT32 nSize         - Size in bytes of data to read
 *               RESULT *ResultCode   - Storage location for any result code
 *
 * Returns:      BOOLEAN bOK    [ TRUE  - Read is initiated
 *                                FALSE - Read Could not start ]
 *
 * Notes:       
 *
 *
 *****************************************************************************/
BOOLEAN MemRead (MEM_BLOCK_TYPE nBlock, UINT32 *nBlockOffset,
                 void *pBuf, UINT32 nSize, RESULT *ResultCode)  
{
   // Local Data
   BOOLEAN bOK;
   UINT32 *pData = pBuf;
   INT32  intsave;

   // Initialize Local Data
   bOK = FALSE;
   
   intsave = __DIR();
   
   // Check that the memory manager is not busy
   if (MEM_IDLE == TPU( MemCurrentState, eTpMem3793))   
   {
      // Set the new state to MEM_READ
      MemCurrentState = MEM_READ;
      __RIR(intsave);
      
      // Calculate the offset within the flash memory
      *ResultCode = MemCheckAddr (nBlock, *nBlockOffset, nSize);

      // Check the address range and alignment is valid
      if (SYS_OK == *ResultCode)
      {
         // Everything is good now initiate the read
         //bOK = state_table[MemCurrentState] (MEM_READ_CMD, nBlock, nBlockOffset,
         //                                     pData, nSize, ResultCode );
         bOK = MemStateRead ( MEM_READ_CMD, nBlock, nBlockOffset,
                                              pData, nSize, ResultCode );
      }
      // The read has completed, return to not busy
      MemCurrentState = MEM_IDLE;
   }
   else // Memory is already busy doing other things
   {
      __RIR(intsave);
      *ResultCode = SYS_OK;
   }

   // Return the read status
   return (BOOLEAN)TPU( bOK, eTpMemRead);
}

/******************************************************************************
 * Function:     MemWrite
 *
 * Description:  Initiates a Write Operation and returns. The status of the
 *               write must then be polled to determine when the action is
 *               complete. The calling function should check the returned
 *               status to verify if the write is successfully started.
 *               Reasons for a write not starting include; invalid block passed,
 *               offset outside of block, sector currently being erased or
 *               programmed, OR another program operation already in progress.
 *
 *               The MemWrite function will manage when a write is requested
 *               that will exceed the current sector boundary. The MemWrite
 *               function will clear the memory to the end of the sector and
 *               return fail with a new offset. The calling function must then
 *               re-request the write. The MemWrite function shall place a
 *               header on the cleared section of memory so that MemRead
 *               can jump over this section when it is encountered.
 *
 * Parameters:   UINT32 nBlock        - Memory block to perform write in
 *               UINT32 *nBlockOffset - Offset into the block
 *               UINT32 *pBuf         - pointer to the data to write
 *               UINT32 nSize         - Size in bytes of data to write
 *               RESULT *ResultCode   - Storage location for any result code
 *               BOOLEAN bDirectWrite - TRUE = One 4 byte location
 *
 * Returns:      BOOLEAN bOK - [ TRUE  - If the memwrite was started
 *                               FALSE - If the memwrite could not be started ] 
 *
 * Notes:        After a MemWrite is started the calling function must use
 *               MemGetStatus to verify the write completes. If the calling
 *               function does not call MemGetStatus the MemWrite will never
 *               exit the program state.
 *   
 *
 *****************************************************************************/
BOOLEAN MemWrite ( MEM_BLOCK_TYPE nBlock, UINT32 *nBlockOffset,
                   void *pBuf, UINT32 nSize, RESULT *ResultCode,
                   BOOLEAN bDirectWrite )
{
   // Local Data
   BOOLEAN   bOK;
   MEM_BLOCK *pBlock;
   UINT32    nStartOffset;
   UINT32    nEndOffset;
   UINT32    nBoundarySize;
   UINT32    nWritten;
   RESULT    Result;
   UINT32    *pData = pBuf;
   INT32     intsave;

   // Intialize Local Data
   bOK    = FALSE;
   Result = SYS_OK;
   
   intsave = __DIR();
   // Check that the memory manager is not performing any other operation
   if (MEM_IDLE == MemCurrentState)   
   {
      // Set the current state to Program
      MemCurrentState = MEM_PROG;
      __RIR(intsave);

      // Check the memory range and alignment is valid
      Result = MemCheckAddr (nBlock, *nBlockOffset, nSize);

      // If the system status is ok 
      if (SYS_OK == Result)
      {
         // Write parameters are valid calculate the addresses 
         pBlock       = &MemDescription.MemMap.MemBlock[nBlock];
         nStartOffset = pBlock->nStartOffset + *nBlockOffset;
         // The last address is 4 bytes less than just adding up the offset and size
         nEndOffset   = pBlock->nStartOffset + *nBlockOffset +
                        MEM_ALIGN_SIZE(nSize) - sizeof(UINT32);

         // Check that the write will not exceed the sector boundary
         if (FlashCheckSameSector(nStartOffset, nEndOffset))
         {
            // If direct write then one 4 byte location can be written
            if (TRUE == TPU(bDirectWrite, eTpMemWord))
            {
                // Only one location to write 
                *ResultCode = FlashWriteWordMode ( nStartOffset, pData,
                                                   MEM_ALIGN_SIZE( TPU(nSize, eTpMemWord1)),
                                                   &nWritten );
                // Check that the bytes were written
                if (0 != nWritten)
                {
                   bOK = TRUE;
                }
            }
            else
            {
               // Data doesn't exit sector it is ok to write now
               //bOK = state_table[MemCurrentState] (MEM_PROG_CMD, nBlock,
               //                                    nBlockOffset,
               //                                    pData, MEM_ALIGN_SIZE(nSize),
               //                                    ResultCode );
               bOK = MemStateProgram( MEM_PROG_CMD, nBlock, 
                                      nBlockOffset,
                                      pData,
                                      MEM_ALIGN_SIZE(nSize),
                                      ResultCode );
            }
         }
         else // Data won't fit in the sector we need to spin off a task to 
              // clear to the end of the sector. A special header is written
              // so that a MemRead can jump over the cleared sections.
         {
             // Setup the amount of data to clear at the end of the sector.
             nBoundarySize               = MemDescription.nSectorSize;
             MemClearPBlock.State        = MEM_CLR_PROG_START;
             MemClearPBlock.nBlock       = nBlock;
             MemClearPBlock.nBlockOffset = *nBlockOffset;
             MemClearPBlock.nData        = nBoundarySize -
                                           (*nBlockOffset % nBoundarySize);
             // Kick-off the MemClear task
             TmTaskEnable ( Mem_Clear_Task, TRUE );

             // Once the clearing task is kicked off calculate the new offset.
             // This new offset is returned to the calling function so that
             // the new location to write is known.
             *nBlockOffset = *nBlockOffset + MemClearPBlock.nData;
             *ResultCode   = SYS_OK;
         }
      }
      else // Memory Address error detected
      {
         // Set the state to not busy. ResultCode is returned in passed in addr.
         MemCurrentState = MEM_IDLE;
         *ResultCode     = Result;
      }

      if (TRUE == bDirectWrite)
      {
         MemCurrentState = MEM_IDLE;
      }
   }
   else // Memory is already busy doing other things
   {
      __RIR(intsave);     
      *ResultCode = SYS_OK;
   }

   // return the status of the write 
   return bOK;
}

/******************************************************************************
 * Function:     MemErase
 *
 * Description:  Initiates an Erase Operation and returns. The status of the
 *               erase must then be polled to determine when the action is
 *               complete. The calling function should check the returned
 *               status to verify if the erase is successfully started. Reason
 *               for an erase not starting include; not a valid block to erase,
 *               addresses exceed the request block, sectors already in the
 *               program or erase state, a program operation is being performed
 *               on another area in memory.
 *
 * Parameters:   UINT32 nBlock        - Index of block to perform erase
 *               UINT32 *nBlockOffset - Offset into block to start erase
 *               UINT32 nSize         - Size of the erase 
 *               RESULT *ResultCode   - Storage location of any error detected
 *
 * Returns:      BOOLEAN bOK          - [ TRUE  - If the Erase can start,
 *                                        FALSE - If the mem manager is busy ]
 *
 * Notes:        After a MemErase is started the calling function must use
 *               MemGetStatus to verify the erase completes. If the calling
 *               function does not call MemGetStatus the MemErase will never
 *               exit the erase state.
 *
 *
 *****************************************************************************/
BOOLEAN MemErase (MEM_BLOCK_TYPE nBlock, UINT32 *nBlockOffset,
                  UINT32 nSize,  RESULT *ResultCode)
{
   // Local Data
   BOOLEAN   bOK;
   INT32 intsave;

   // Initialize Local Data
   bOK = FALSE;

   // Default - Memory is already busy doing other things
   *ResultCode = SYS_OK;
   
   intsave = __DIR();   
   // Make sure the Mem Manager is not program or erase
   if (MEM_IDLE == MemCurrentState)
   {
      // Set the current state
      MemCurrentState = MEM_ERASE;

      __RIR(intsave);
      // Check the memory range and alignment is valid
      *ResultCode   = MemCheckAddr (nBlock, *nBlockOffset, nSize);

      // If the system status is ok 
      if (SYS_OK == *ResultCode)
      {
         // Start the erase
         //bOK = state_table[MemCurrentState] ( MEM_ERASE_CMD, nBlock, nBlockOffset,
         //                                      NULL, nSize, ResultCode);
         bOK = MemStateErase ( MEM_ERASE_CMD, nBlock, nBlockOffset, NULL, nSize, ResultCode );
      }
      else // system error detected
      {
         // An error was detected go back to not busy
         MemCurrentState = MEM_IDLE;
      }
   }
   else
   {
       __RIR(intsave);
   }

   // return the status of the MemErase starting 
   return (bOK);
}

/******************************************************************************
 * Function:    MemGetStatus
 *
 * Description: The MemGetStatus function must be used to verify that
 *              Programs and Erases have completed. If this function is
 *              not called the Memory manager will remain in the Program
 *              or Erase state. 
 *
 * Parameters:  MEM_STATUS_TYPE StatusType - MEM_STATUS_PROG or MEM_STATUS_ERASE
 *              UINT32          nBlock     - Memory block to check
 *              UINT32       *nBlockOffset - Pointer to location of offset
 *              RESULT         *ResultCode - Pointer to location to store
 *                                           any error codes.
 *
 * Returns:     BOOLEAN bDone              - Program or Erase Completed
 *
 * Notes:       This function must be called after a MemWrite or MemErase
 *              is started. If this function is not called the memory
 *              manager will not return to the Memory not busy state.
 *
 *
 *****************************************************************************/
BOOLEAN MemGetStatus (MEM_STATUS_TYPE StatusType, MEM_BLOCK_TYPE nBlock,
                      UINT32 *nBlockOffset, RESULT *ResultCode)
{
   // Local Data
   BOOLEAN bDone;

   // Initialize Local Data
   bDone       = FALSE;
   *ResultCode = SYS_OK;
   
   // Check if the Current State is one that a status can be checked
   if ( ((MEM_PROG  == MemCurrentState) && (MEM_STATUS_PROG  == StatusType )) ||
        ((MEM_ERASE == MemCurrentState) && (MEM_STATUS_ERASE == StatusType )) )         
   {
      // Goto the state and get the status of the command
      // bDone = state_table[MemCurrentState] ( MEM_STATUS_CMD, nBlock,
      //                                        nBlockOffset,
      //                                        NULL, 0, ResultCode );
      if ( MEM_PROG == MemCurrentState )
      {
         bDone = MemStateProgram ( MEM_STATUS_CMD, nBlock, nBlockOffset, NULL, 0, ResultCode );
      }
      else if ( MEM_ERASE == MemCurrentState )
      {
         bDone = MemStateErase ( MEM_STATUS_CMD, nBlock, nBlockOffset, NULL, 0, ResultCode );
      }
      // If the operation is complete return the state machine
      // to not busy.
      if (TRUE == bDone)
      {
         // Set the state to read
         MemCurrentState = MEM_IDLE;
      }
   }

   // Return the status of the command
   return (bDone);
}

/******************************************************************************
 * Function:    MemGetBlockSize
 *
 * Description: Gets the value of the block size of the block of memory 
 *              specified as input to this function.
 *
 * Parameters:  MEM_BLOCK_TYPE Block - the specific block whose size is being 
 *                                     requested
 *
 * Returns:     UINT32 - the size of the block of memory based on difference
 *              between endOffset and StartOffset
 *
 * Notes:      
 *
 *
 *****************************************************************************/
UINT32 MemGetBlockSize (MEM_BLOCK_TYPE Block)
{
   return (MemDescription.MemMap.MemBlock[Block].nEndOffset -
           MemDescription.MemMap.MemBlock[Block].nStartOffset);
}

/******************************************************************************
 * Function:    MemChipErase
 *
 * Description: This function will immediately initiate an erase and not
 *              return until completion.
 *
 * Parameters:  None
 *
 * Returns:     RESULT
 *
 * Notes:       WARNING - ONLY TO BE CALLED DURING INITIALIZATION IF
 *                        AN ERASE PROBLEM IS DETECTED.
 *
 *
 *****************************************************************************/
RESULT MemChipErase (void) 
{
   // Local Data
   MEM_DESC     *pMemDesc;
   MEM_BLOCK    *pBlock;
   BOOLEAN      bComplete;
   UINT32       UpdateTime;
   UINT32       nMinutes;
   UINT32       BlockIndex;
   RESULT       Result;
   UINT32       Timeout_mS;

   UINT32       startup_Id = MEM_CHIPERASE_WD_STARUP;

   // Initialize Local Data
   pMemDesc      = &MemDescription;
   bComplete     = FALSE;
        
   // Command a Flash Chip Erase
   Result = FlashChipErase(&Timeout_mS);

   if (DRV_OK == Result)
   {
      GSE_DebugStr(NORMAL,TRUE, "MemChipErase: Max Time = %d minutes", 
               ((Timeout_mS/MILLISECONDS_PER_SECOND)/SECONDS_PER_MINUTE));

      UpdateTime = TTMR_GetHSTickCount();
      // Set the Start Timer to TRUE
      nMinutes = UTIL_MinuteTimerDisplay (TRUE);
   }
   
   // Check the status of the erase in a loop
   while ((DRV_OK == Result) && (FALSE == bComplete))
   {
      Result = FlashGetDirectStatus (0, &bComplete);

      nMinutes = UTIL_MinuteTimerDisplay (FALSE);

      if ( STPUI( nMinutes, eTpChipErase, 500) > ((Timeout_mS/MILLISECONDS_PER_SECOND)/SECONDS_PER_MINUTE))
      {
         GSE_DebugStr(NORMAL,TRUE, "MemChipErase: Erase Timeout");
         Result = SYS_MEM_ERASE_TIMEOUT;
         break;
      }

      // Periodically hit the watchdog
      if ((TTMR_GetHSTickCount() - UpdateTime) > TICKS_PER_500mSec)
      {
         UpdateTime = TTMR_GetHSTickCount();
         // Hit the Watchdog
         STARTUP_ID(startup_Id++); 
      }
   }

   if (DRV_OK == Result)
   {
      for (BlockIndex = 0; BlockIndex < pMemDesc->MemMap.nBlocks; BlockIndex++)
      {
         // Set pointer to the current block being populated
         pBlock = &pMemDesc->MemMap.MemBlock[BlockIndex];
   
         // Find the next free address
         pBlock->nCurrentOffset  = MemFindCurrentOffset(BlockIndex);
         pBlock->nUsed           = pBlock->nCurrentOffset - pBlock->nStartOffset;
         pBlock->nFree           = pBlock->nEndOffset - pBlock->nUsed;
      }
   }

   return (Result);
}

/******************************************************************************
 * Function:    MemGetSectorSize
 *
 * Description: Gets the value of the sector size of the memory
 *
 * Parameters:  None
 *
 * Returns:     UINT32 - the sector size of the memory being managed
 *
 * Notes:       None
 *
 *
 *****************************************************************************/

UINT32 MemGetSectorSize ( void )
{
   return (MemDescription.nSectorSize);
}

/******************************************************************************
 * Function:    MemMgrStatus
 *
 * Description: Gets the operational status of the memory manager from the
 *              MemDescription structure
 *
 * Parameters:  None
 *
 * Returns:     MEM_MGR_STATUS - may be MEM_MGR_STATUS_OK, 
 *                                      MEM_MGR_STATUS_DISABLED or 
 *                                      MEM_MGR_STATUS_FAULTED
 *
 * Notes:       None
 *
 *
 *****************************************************************************/
RESULT MemMgrStatus ( void )
{
   return (MemDescription.MemMgrStatus);
}

/******************************************************************************
 * Function:    MemEraseTask
 *
 * Description: The MemEraseTask is an RMT that runs in the background. This
 *              task is initiated by the MemStateErase function. Once initiated
 *              the initiating application must keep checking the status.
 *
 * Parameters:  pPBlock - Pointer to task's parameter block
 *
 * Returns:     void
 *
 * Notes:       This task will continue to run in the background until
 *              completion or a timeout. This is an RMT task and will only run
 *              when there is time remaining at the end of a frame.
 *                     
 *****************************************************************************/
void MemEraseTask (void* pPBlock)
{
   // Local Data
   MEM_ERASE_TASK_PARMS *pPB;
   MEM_DESC             *pMemDesc;
   MEM_BLOCK            *pBlock;
   UINT32               i;
   BOOLEAN              bDone;
   UINT32               intrLevel;
   
   // Initialize Local Data
   pPB          = pPBlock;
   pMemDesc     = &MemDescription;
   pBlock       = &pMemDesc->MemMap.MemBlock[pPB->nBlock];
   
   if (FALSE == pPB->bCheckStatus)
   {
      // Calculate the begining and end flash addresses
      pPB->nFlashOffset = pBlock->nStartOffset + pPB->nBlockOffset;
      pPB->nEndOffset   = pPB->nFlashOffset + pPB->nData;
     
      // Setup the sector erase
      pPB->EStatus = MEM_ERASE_IN_PROGRESS;

      // Start the sector erase sequence
      pPB->ResultCode = FlashSectorErase (pPB->nFlashOffset,
                                          FE_SETUP_CMD, &bDone);
 
      // loop through all sectors to erase
      intrLevel = __DIR();
      for ( i = pPB->nFlashOffset; i < pPB->nEndOffset; i += pMemDesc->nSectorSize )
      {
         // This function will program each sector to start the erase
         FlashSectorErase(i, FE_SECTOR_CMD, &bDone);
      }
      __RIR(intrLevel);

      pPB->bCheckStatus = TRUE;
   }
   else // Check the Status
   {
      // Now wait for the status to complete
      pPB->ResultCode = FlashSectorErase (pPB->nFlashOffset,
                                          FE_STATUS_CMD, &bDone);

      if (TRUE == bDone)
      {
         // loop through all sectors to set status back to idle
         for ( i = pPB->nFlashOffset;
              (i < pPB->nEndOffset);
              i += pMemDesc->nSectorSize )
         {
            // This will set all the erased sectors back to idle
            // Note: We don't need the RESULT status because it will 
            // always be OK for this call.
            FlashSectorErase(i, FE_COMPLETE_CMD, &bDone);
         }   
         
         
         // Determine the Erase Status
         pPB->EStatus = (pPB->ResultCode == DRV_OK) ? 
                        MEM_ERASE_COMPLETE_PASS : MEM_ERASE_FAIL;

         // Kill Self
         TmTaskEnable (Mem_Erase_Task, FALSE);
      }
   }
}    // End MemEraseTask()  

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/
      
/******************************************************************************
 * Function:    MemStateProgram
 *
 * Description: The MemStateProgram function is the state used to
 *              initiate the background program task and check the
 *              status of the background programming.
 *
 * Parameters:  MEM_CMD Cmd             - Memory Command to perform
 *              UINT32  nBlock          - Index of the block to use
 *              UINT32  *nBlockOffset   - Offset into the block
 *              UINT32  *pData          - Location of buffer containing data
 *              UINT32  nData           - Number of bytes in the buffer
 *              RESULT  *ResultCode     - Storage Location for Result Code
 * 
 * Returns:     BOOLEAN bDone      - Operation has completed [ TRUE, FALSE ]
 *
 * Notes:       This version of software will not allow a program suspend
 *              operation. The current implementation handles all programming
 *              in a background task that could be anywhere in the process.
 *              Without adding more complexity to the MemManager the software
 *              has no way of knowing when it is safe to the suspend the
 *              background writes. Therefore this implementation will not
 *              allow program suspend for reads. The current system architecture
 *              does not require this because of how it operates. Writing while
 *              in the air and reading while on the ground.
 *
 *****************************************************************************/
static
BOOLEAN MemStateProgram (MEM_CMD Cmd,   MEM_BLOCK_TYPE nBlock, UINT32 *nBlockOffset,
                         UINT32* pData, UINT32 nData,  RESULT *ResultCode )        
{
   // Local Data
   BOOLEAN         bDone;
   MEM_PROG_STATUS ProgramStatus;
   MEM_BLOCK       *pMemBlock;

   // Initialize Local Data
   bDone = FALSE;
   
   *ResultCode = SYS_OK;

   // Check if the command is to program
   if (MEM_PROG_CMD == Cmd)
   {
      // Set up the Program TCB
      MemProgPBlock.nBlock       = nBlock;
      MemProgPBlock.nBlockOffset = *nBlockOffset;
      MemProgPBlock.pData        = pData;
      MemProgPBlock.nData        = MEM_ALIGN_SIZE(nData);
      MemProgPBlock.PStatus      = MEM_PROG_PENDING;       
      // Kick off the Program Task
      TmScheduleTask ( Mem_Program_Task );
      // Return that the command was initiated
      bDone = TRUE;
   }
   // Check if the command is to check the status
   else if (MEM_STATUS_CMD == Cmd)
   {
      // Check the status from the Program TCB
      ProgramStatus = MemProgPBlock.PStatus; 

      // Check if the program command has completed
      if ( (ProgramStatus == MEM_PROG_COMPLETE_PASS) ||
           (ProgramStatus == MEM_PROG_FAIL) )
      {
         // Returns the result code and how much was actually written
         *ResultCode   = MemProgPBlock.ResultCode;
         *nBlockOffset = MemProgPBlock.nBlockOffset;

         // Update the free, used and current offset for the block
         pMemBlock = &MemDescription.MemMap.MemBlock[nBlock];
         pMemBlock->nUsed          += (*nBlockOffset - pMemBlock->nCurrentOffset);
         pMemBlock->nFree          -= (*nBlockOffset - pMemBlock->nCurrentOffset);
         pMemBlock->nCurrentOffset  = *nBlockOffset;
         
         // Programming is done
         bDone         = TRUE;
      }
   }
   // Return the programming status
   return (bDone);
}

/******************************************************************************
 * Function:    MemStateErase
 *
 * Description: The MemStateErase function is the state used to initiate and
 *              check the status of the erase task.
 *
 * Parameters:  MEM_CMD Cmd             - Memory Command to perform
 *              UINT32  nBlock          - Index of the block to use
 *              UINT32  *nBlockOffset   - Offset into the block
 *              UINT32  *pData          - Not Used
 *              UINT32  nData           - Number of bytes to erase
 *              RESULT  *ResultCode     - Storage Location for Result Code
 * 
 * Returns:     BOOLEAN bDone      - Operation has completed [ TRUE, FALSE ]
 *
 * Notes:       This version of software will not allow an erase suspend
 *              operation. The current implementation handles all erasing
 *              in a background task that could be anywhere in the process.
 *              Without adding more complexity to the MemManager the software
 *              has no way of knowing when it is safe to the suspend the
 *              background erase. Therefore this implementation will not
 *              allow erase suspend for reads. The current system architecture
 *              does not require this because of how it operates. 
 *
 *****************************************************************************/
static
BOOLEAN MemStateErase ( MEM_CMD Cmd, MEM_BLOCK_TYPE nBlock, UINT32 *nBlockOffset,
                        UINT32 *pData,  UINT32 nData, RESULT *ResultCode )
{
   // Local Data
   BOOLEAN          bDone;
   MEM_ERASE_STATUS EraseStatus;
   MEM_BLOCK       *pMemBlock;            

   // Initialize Local Data
   bDone       = FALSE;
   // Set the result code to SYS_OK
   *ResultCode = SYS_OK;   
 
   // Check if the command is to start the erase
   if (MEM_ERASE_CMD == Cmd)
   {
      // Set up the Erase Task Control Block      
      MemErasePBlock.nBlock       = nBlock;
      MemErasePBlock.nBlockOffset = *nBlockOffset;
      MemErasePBlock.nData        = MEM_ALIGN_SIZE(nData);
      MemErasePBlock.EStatus      = MEM_ERASE_PENDING;
      MemErasePBlock.ResultCode   = *ResultCode;
      MemErasePBlock.bCheckStatus = FALSE;
      // Enable the background erase
      TmTaskEnable (Mem_Erase_Task, TRUE);
      // Erase command was intiated
      bDone                       = TRUE;
   }
   // Check if the command is to poll the status
   else if (MEM_STATUS_CMD == Cmd)
   {
      EraseStatus = MemErasePBlock.EStatus;

      if ( (MEM_ERASE_COMPLETE_PASS == EraseStatus) ||
           (MEM_ERASE_FAIL == EraseStatus) )
      {
         // Returns the result code and how much was actually written
         *ResultCode   = MemErasePBlock.ResultCode;

         if (MEM_ERASE_COMPLETE_PASS == EraseStatus)
         {
            // Update the free and used
            pMemBlock = &MemDescription.MemMap.MemBlock[nBlock];
            pMemBlock->nCurrentOffset = MemErasePBlock.nBlockOffset; 
            pMemBlock->nUsed         -= nData;
            pMemBlock->nFree         += nData;
         }
         
         // Erase has completed
         bDone = TRUE;
      }
   }

   return (bDone);
}

/******************************************************************************
 * Function:    MemStateRead
 *
 * Description: The MemStateRead function is the state used to read
 *              data from the data flash. The read command is the only
 *              valid command for this state.
 *
 *              The MemStateRead function checks the address to be read
 *              for a memory cleared header before each block read. If the
 *              memory cleared header is found the read will automattically
 *              advance to the next sector.
 *
 * Parameters:  MEM_CMD Cmd           - Memory Command to perform
 *              UINT32  nBlock        - Index of the block to use
 *              UINT32  *nBlockOffset - Offset into the block
 *              UINT32  *pData        - Storage location for the read
 *              UINT32  nData         - Number of bytes to read
 *              RESULT  *ResultCode   - Storage Location for Result Code
 *
 * Returns:     BOOLEAN bOK           - [ TRUE  - Read Initiated
 *                                        FALSE - Unable to perform command ]
 *
 * Notes:       Function will advance the offset to the next sector if
 *              the reamiaing locations in the sector are cleared and the
 *              memory cleared header is found.
 *
 *****************************************************************************/
static
BOOLEAN MemStateRead ( MEM_CMD Cmd, MEM_BLOCK_TYPE nBlock, UINT32 *nBlockOffset,
                        UINT32 *pData,  UINT32 nData, RESULT *ResultCode )
{
   // Local Data
   UINT32    TempStorage[2];
   MEM_BLOCK *pBlock;
   BOOLEAN   bOK;

   // Initialize Local Data
   pBlock      = &MemDescription.MemMap.MemBlock[nBlock];
   bOK         = FALSE;
   *ResultCode = SYS_OK;
   
   // Only valid command while in the Read state is the Read Command
   if (MEM_READ_CMD == Cmd)
   {
      bOK = TRUE;
       
      // Peek into the memory to check for the end of the sector
      FlashRead(pBlock->nStartOffset + *nBlockOffset, &TempStorage[0],
                sizeof(TempStorage));
 
      // Check the first address of the mem read to see if the
      // process needs to move to the next sector.
      if (MEM_END_HDR1 == (TempStorage[0] & MEM_MSW_MASK))
      {
         // First Header Found now check if more than one location has been
         // cleared
         if ((TempStorage[0] & MEM_LSW_MASK) > 1)
         {
            // More than one lcation cleared now check the 2nd header
            if (MEM_END_HDR2 == TempStorage[1])
            {
               // Second header found move read to next sector
               *nBlockOffset += ((TempStorage[0] & MEM_LSW_MASK) * sizeof(UINT32));
            }
         }
         else // Only one location filled 
         {
            // Move to the next sector
            *nBlockOffset += sizeof(UINT32);
         }
      }

      // Check and Adjustment complete now read the data   
      FlashRead(pBlock->nStartOffset + *nBlockOffset, pData, MEM_ALIGN_SIZE(nData));

      // Increment the offset by the amount read
      *nBlockOffset += MEM_ALIGN_SIZE(nData);
   }

   return (bOK);
}

/******************************************************************************
 * Function:    MemCheckAddr
 *
 * Description: The MemCheckAddr function will verify that the address
 *              to be operated on is valid and in range. It will verify
 *              the block is valid, the offset is aligned and the address is
 *              within the blocks range. A system fault code will be returned
 *              if any problem is detected.
 *             
 *
 * Parameters:  UINT32 nBlock       - Block in the memory map
 *              UINT32 nBlockOffset - Offset into the block
 *              UINT32 nSize        - Size of the memory operation
 *
 * Returns:     RESULT              - [SYS_OK, SYS_MEM_ADDRESS_RANGE_INVALID ]
 *
 * Notes:
 *
 *****************************************************************************/
static
RESULT MemCheckAddr (MEM_BLOCK_TYPE nBlock, UINT32 nBlockOffset, UINT32 nSize)
{
   // Local Data
   MEM_DESC   *pMemDesc;
   MEM_MAP    *pMemMap;
   MEM_BLOCK  *pBlock;
   RESULT     SysStatus;
   UINT32     nStartOffset;
   UINT32     nEndOffset;

   // Initialize Local Data
   SysStatus      = SYS_OK;
   pMemDesc       = &MemDescription;
   pMemMap        = &pMemDesc->MemMap;
   pBlock         = &pMemMap->MemBlock[nBlock];

   //Calculate start and end offsets
   nStartOffset  = pBlock->nStartOffset + nBlockOffset;
   nEndOffset    = pBlock->nStartOffset + nBlockOffset +
                   MEM_ALIGN_SIZE(nSize) - sizeof(UINT32);
      
   // Verify the block is valid
   ASSERT ( nBlock < MEM_BLOCK_MAX);
   // Verify the memory offset is aligned correctly
   ASSERT ( 0 == (nBlockOffset & MEM_ALIGN32_MASK));

   // Verify the address range is within the block
   if ( (nStartOffset > pBlock->nEndOffset) ||
        (nEndOffset   > pBlock->nEndOffset)    )
   {     
      SysStatus   = SYS_MEM_ADDRESS_RANGE_INVALID;
   }

   return TPU(SysStatus, eTpMemCheck);
}

/******************************************************************************
 * Function:    MemFindCurrentOffset
 *
 * Description: The MemFindCurrentOffset function will search through the
 *              provided address range and find the next available address.
 *
 * Parameters:  UINT32 nBlock
 *
 * Returns:     UINT32
 *
 * Notes:       None
 *
 *****************************************************************************/
static
UINT32 MemFindCurrentOffset(UINT32 nBlock)
{
   // Local Data
   MEM_DESC  *pMemDesc;
   MEM_BLOCK *pBlock;
   UINT32    Offset;
   UINT32    ErasedCount;
   UINT32    StoredValue;

   UINT32    startup_Id = MEM_FINDCURROFFSET_WD_STARUP;

   pMemDesc    = &MemDescription;
   pBlock      = &pMemDesc->MemMap.MemBlock[nBlock];
   ErasedCount = 0;
 
   for (Offset = 0; Offset < pBlock->nEndOffset; )
   {
      // Watchdog - Smack the DOG
      STARTUP_ID(startup_Id++); 
      
      FlashRead(pBlock->nStartOffset + Offset, &StoredValue, sizeof(UINT32));

      if (StoredValue != MEM_LOC_ERASED)
      {
         Offset += pMemDesc->nSectorSize - (Offset % pMemDesc->nSectorSize);
         ErasedCount = 0;
      }
      else // Erased Location
      {
         Offset += sizeof(UINT32);

         ErasedCount++;

         if ( ErasedCount >= (pMemDesc->nSectorSize / sizeof(UINT32)) &&
              ((Offset % pMemDesc->nSectorSize) == 0))
         {
            // At the begining of next erased sector, now go back to last non
            // erased sector and find where the section starts
            Offset -= pMemDesc->nSectorSize;

            // Make sure we are not in the first sector
            if (Offset > pBlock->nStartOffset)
            {
               do
               {
                  Offset -= sizeof(UINT32);
                  FlashRead(pBlock->nStartOffset + Offset, &StoredValue, sizeof(UINT32));
               
               } while(StoredValue == MEM_LOC_ERASED);
               // Correct for the last decremented location
               Offset += sizeof(UINT32);
            }
            break;
         }
      }
   }
   return (Offset);
}

/*****************************************************************************
 * Function:    MemProgTask
 *
 * Description: RMT Task which performs the FLASH programming.
 *
 * Parameters:  void *pPBlock - Pointer to task's parameter block
 *
 * Returns:     void
 *
 * Notes:       Mem programming is initiated by calling MemWrite() which
 *              saves data needed by the Program Task and then "wakes up" the
 *              Program Task and returns immediately in order to not hang
 *              up the task initiating the operation.
 *
 *              This task runs in the background until it is complete.
 *              The initiating function should periodically check the status.
 *              This is an RMT task and will only run when there is time
 *              remaining at the end of a frame.
 *
 ****************************************************************************/
static
void MemProgTask (void* pPBlock)
{
   // Local Data
   MEM_PROG_TASK_PARMS *pPB;
   MEM_DESC            *pMemDesc;
   MEM_BLOCK           *pBlock;
   UINT32              nFlashOffset;
   UINT32              nTotalBytesWritten;
  
   // Initialize Local Data
   pPB          = pPBlock;
   pMemDesc     = &MemDescription;
   pBlock       = &pMemDesc->MemMap.MemBlock[pPB->nBlock];
   
   // Calculate the starting offset in flash
   nFlashOffset = pBlock->nStartOffset + pPB->nBlockOffset;

   // Set the task status to in progress
   pPB->PStatus = MEM_PROG_IN_PROGRESS;
   
   // This function will not return until all data is written or
   // a failure is detected
   pPB->ResultCode = FlashWriteBufferMode ( nFlashOffset, pPB->pData,
                                            pPB->nData, &nTotalBytesWritten );
   // The Flash Write Buffer Logic will always advance nTotalBytesWritten to the next 
   // location and the calling function will retry at the next offset.
 
   // Update the offset by the number of bytes written or attempted
   pPB->nBlockOffset += nTotalBytesWritten;
   
   // Check the status of the write
   pPB->PStatus = (pPB->ResultCode == DRV_OK) ? MEM_PROG_COMPLETE_PASS : MEM_PROG_FAIL;

   // Go back to "Sleep" and wait for the next program FLASH request

}  // End of MemProgTask()


/*****************************************************************************
 * Function:    MemClearTask
 *
 * Description: The MemClearTask is kicked off to zero out any remaining
 *              locations at the end of the sector. It will first add
 *              a special header and then program the reamining locations
 *              to 0.
 *
 * Parameters:  void *pPBlock - Pointer to task's parameter block
 *
 * Returns:     void
 *
 * Notes:       None.
 *
 ****************************************************************************/
static
void MemClearTask (void* pPBlock)
{
   // Local Data
   MEM_CLEAR_TASK_PARMS *pPB;
   RESULT               ResultCode;
   BOOLEAN              bDone;

   // Initialize Local Data
   pPB = pPBlock;
   
   // Check for the clear section start state
   if (MEM_CLR_PROG_START == pPB->State)
   {
      // Create the clear header
      MemClearBuffer[0] = MEM_END_HDR1 | (pPB->nData / sizeof(UINT32));
      MemClearBuffer[1] = MEM_END_HDR2;
      // Set the state to program data
      pPB->State        = MEM_CLR_PROG_DATA;
   }

   // Check for the clearing of program data
   if (MEM_CLR_PROG_DATA == pPB->State)
   {
      // Check if the amount to clear is less than one buffer
      if (pPB->nData < sizeof(MemClearBuffer))
      {
         // Set the Write size to the total size
         pPB->nWriteSize = pPB->nData;
      }
      else // Clear is larger than one buffer of data
      {
         // Set the clear size to one full buffer
         pPB->nWriteSize = sizeof(MemClearBuffer);
      }

      // Kick off the programming of the end of the sector
      // Note: We are already in the program state because the MemWrite
      //       could not be performed until the end of sector is cleared
      // bDone = state_table[MemCurrentState] (MEM_PROG_CMD, pPB->nBlock,
      //                                       &pPB->nBlockOffset,
      //                                       &MemClearBuffer[0],
      //                                       pPB->nWriteSize, &ResultCode);
      bDone = MemStateProgram ( MEM_PROG_CMD, pPB->nBlock,
                                &pPB->nBlockOffset,
                                &MemClearBuffer[0],
                                pPB->nWriteSize, &ResultCode);

      // if bDone is true then the programming has started
      if (TRUE == bDone)
      {     
         // Move to the state to check the status of the program
         pPB->State  = MEM_CLR_DATA_STATUS;
      }
   }

   // Check the Status the initiated programming sequence
   if (MEM_CLR_DATA_STATUS == pPB->State)
   {
      // We are still in the program state now check the status
      // bDone = state_table[MemCurrentState] (MEM_STATUS_CMD, pPB->nBlock,
      //                                       &pPB->nBlockOffset,
      //                                      NULL, 0, &ResultCode);
      bDone = MemStateProgram ( MEM_STATUS_CMD, pPB->nBlock,
                                &pPB->nBlockOffset,
                                NULL, 0, &ResultCode);

      // If the status is done and there is no error
      if ((bDone) && (ResultCode == SYS_OK))
      {
         // Update the amount of data left to write
         pPB->nData -= pPB->nWriteSize;
         
         // Check if there are still addresses to clear
         if (pPB->nData != 0)
         {
            // More addresses to clear so erase the header from the
            // begining of the buffer.
            MemClearBuffer[0] = 0;
            MemClearBuffer[1] = 0;
            // Go back to the Program state and initiate the next clear
            pPB->State        = MEM_CLR_PROG_DATA;            
         }
      }
      // Else the program ended and we have a problem
      else if ((bDone) && (ResultCode != SYS_OK))
      {
         // We didn't complete dump a debug message as there is nobody
         // monitoring this task. 
         GSE_DebugStr( NORMAL,
                       TRUE, 
                       "MemClearTask: Clear Failed. ResultCode: 0x%08X", 
                       ResultCode );
         
         // Update the state machine to not busy and Kill Task. The next write 
         // will start in the next sector even if this doesn't complete 
         // successfully. If this happens then this area will be detected as a
         // corrupted area of memory. 
         MemCurrentState = MEM_IDLE;         
         // This Task will kill itself now
         TmTaskEnable ( Mem_Clear_Task, FALSE );
      }

      // Check if the clear task is complete and no more addresses to clear
      if ((bDone) && (MEM_CLR_PROG_DATA != pPB->State))
      {
         // Must update the state machine to not busy
         MemCurrentState = MEM_IDLE;         
         // This Task will kill itself now
         TmTaskEnable ( Mem_Clear_Task, FALSE );
      }
   }
}  // End of MemClearTask()



/*****************************************************************************
 *  MODIFICATIONS
 *    $History: MemManager.c $
 * 
 * *****************  Version 44  *****************
 * User: Melanie Jutras Date: 12-11-13   Time: 2:09p
 * Updated in $/software/control processor/code/system
 * SCR #1142 File Format and Dead Code
 * 
 * *****************  Version 43  *****************
 * User: Melanie Jutras Date: 12-10-10   Time: 1:05p
 * Updated in $/software/control processor/code/system
 * SCR 1172 PCLint 545 Suspicious use of & Error
 * 
 * *****************  Version 42  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 41  *****************
 * User: John Omalley Date: 6/27/11    Time: 10:50a
 * Updated in $/software/control processor/code/system
 * SCR 1026 - Added Interrupt protection and removed State Table.
 * 
 * *****************  Version 40  *****************
 * User: John Omalley Date: 5/31/11    Time: 3:40p
 * Updated in $/software/control processor/code/system
 * SCR 1026 - Added Not Busy State
 * 
 * *****************  Version 39  *****************
 * User: Jeff Vahue   Date: 11/23/10   Time: 1:57p
 * Updated in $/software/control processor/code/system
 * SCR# 848 - Code Coverage
 * 
 * *****************  Version 38  *****************
 * User: Jeff Vahue   Date: 11/12/10   Time: 9:54p
 * Updated in $/software/control processor/code/system
 * SCR# 848 - Code Coverage
 * 
 * *****************  Version 37  *****************
 * User: Jeff Vahue   Date: 11/09/10   Time: 6:05p
 * Updated in $/software/control processor/code/system
 * SCR# 848 - Code Coverage
 * 
 * *****************  Version 36  *****************
 * User: Jeff Vahue   Date: 11/05/10   Time: 9:48p
 * Updated in $/software/control processor/code/system
 * SCR# 848 - Code Coverage
 * 
 * *****************  Version 35  *****************
 * User: Jeff Vahue   Date: 11/01/10   Time: 7:50p
 * Updated in $/software/control processor/code/system
 * SCR# 975 - Code Coverage refactor
 * 
 * *****************  Version 34  *****************
 * User: Jeff Vahue   Date: 11/01/10   Time: 4:34p
 * Updated in $/software/control processor/code/system
 * SCR# 848 - Code Coverage
 * 
 * *****************  Version 33  *****************
 * User: Jeff Vahue   Date: 10/19/10   Time: 7:25p
 * Updated in $/software/control processor/code/system
 * SCR# 848 - Code Coverage
 * 
 * *****************  Version 32  *****************
 * User: John Omalley Date: 10/04/10   Time: 3:09p
 * Updated in $/software/control processor/code/system
 * SCR 895 - Code Coverage
 * 
 * *****************  Version 31  *****************
 * User: John Omalley Date: 10/04/10   Time: 10:49a
 * Updated in $/software/control processor/code/system
 * SCR 895 - Removed Dead Code
 * 
 * *****************  Version 30  *****************
 * User: John Omalley Date: 9/30/10    Time: 1:25p
 * Updated in $/software/control processor/code/system
 * SCR 894 - LogManager Code Coverage and Code Review Updates
 * 
 * *****************  Version 29  *****************
 * User: John Omalley Date: 9/09/10    Time: 2:14p
 * Updated in $/software/control processor/code/system
 * SCR 855 - Changed NOT_BUSY to READ state and removed the MEM_NOT_BUSY
 * function.
 * 
 * *****************  Version 28  *****************
 * User: John Omalley Date: 9/03/10    Time: 9:47a
 * Updated in $/software/control processor/code/system
 * SCR 825 - Incremented the number of bytes written before updating the
 * status in MemProgTask
 * 
 * *****************  Version 27  *****************
 * User: John Omalley Date: 7/30/10    Time: 5:04p
 * Updated in $/software/control processor/code/system
 * SCR 699 - Change MemInitialize to return void
 * 
 * *****************  Version 26  *****************
 * User: John Omalley Date: 7/30/10    Time: 4:32p
 * Updated in $/software/control processor/code/system
 * SCR 699 - Cleaned up TODO and TBD comments
 * 
 * *****************  Version 25  *****************
 * User: John Omalley Date: 7/29/10    Time: 4:22p
 * Updated in $/software/control processor/code/system
 * SCR 698
 * 
 * *****************  Version 24  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:10a
 * Updated in $/software/control processor/code/system
 * SCR #698 - Fix code review findings
 * 
 * *****************  Version 23  *****************
 * User: John Omalley Date: 7/23/10    Time: 10:16a
 * Updated in $/software/control processor/code/system
 * SCR 306 - Updates for error paths
 * 
 * *****************  Version 22  *****************
 * User: Contractor3  Date: 6/25/10    Time: 9:46a
 * Updated in $/software/control processor/code/system
 * SCR #662 - Changes based on code review
 * 
 * *****************  Version 21  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:55p
 * Updated in $/software/control processor/code/system
 * SCR #587 Change TmTaskCreate to return void
 * 
 * *****************  Version 20  *****************
 * User: Jeff Vahue   Date: 4/28/10    Time: 5:51p
 * Updated in $/software/control processor/code/system
 * SCR #573 - Startup WD changes
 * 
 * *****************  Version 19  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:10p
 * Updated in $/software/control processor/code/system
 * SCR #317 Implement safe strncpy,SCR #70 Store/Restore interrupt
 * 
 * *****************  Version 18  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:36p
 * Updated in $/software/control processor/code/system
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 * 
 * *****************  Version 17  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/system
 * SCR# 483 - Function Names
 * 
 * *****************  Version 16  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 15  *****************
 * User: Jeff Vahue   Date: 2/26/10    Time: 11:24a
 * Updated in $/software/control processor/code/system
 * SCR# 467 - move minute timer display to Utitlities and rename TTMR to
 * UTIL in function name
 * 
 * *****************  Version 14  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 5:08p
 * Updated in $/software/control processor/code/system
 * SCR# 397
 * 
 * *****************  Version 13  *****************
 * User: Jeff Vahue   Date: 12/29/09   Time: 2:18p
 * Updated in $/software/control processor/code/system
 * Tabs vs. Spaces Fix - No Code change
 * 
 * *****************  Version 12  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 2:11p
 * Updated in $/software/control processor/code/system
 * SCR# 326
 * 
 * *****************  Version 11  *****************
 * User: Peter Lee    Date: 11/20/09   Time: 2:35p
 * Updated in $/software/control processor/code/system
 * SCR #345 Implement FPGA Watchdog Requirements
 * 
 * *****************  Version 10  *****************
 * User: John Omalley Date: 8/06/09    Time: 3:58p
 * Updated in $/software/control processor/code/system
 * SCR 179 
 *    Flash Fail now disables the memory manager.
 *    MEM_ERASE_TASK removed from the initialization path.
 *    Fixed all uninitialized variables
 * 
 * 
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 7/06/09    Time: 5:51p
 * Updated in $/software/control processor/code/system
 * SCR #204 Misc Code Review Updates
 * 
 * *****************  Version 8  *****************
 * User: Jim Mood     Date: 10/08/08   Time: 9:41a
 * Updated in $/control processor/code/system
 * SCR #87
 * 
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 3:18p
 * Updated in $/control processor/code/system
 * SCR #87 Function Prototype
 * 
 *
 ***************************************************************************/
