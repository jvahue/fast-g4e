#ifndef MEM_MNGR_H
#define MEM_MNGR_H
/******************************************************************************
            Copyright (C) 2008-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:        MemManager.h
    
    Description: Non-Volatile memory management implementation
    
  VERSION
    $Revision: 16 $  $Date: 6/27/11 10:50a $                 
                 
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "flash.h"
#include "taskmanager.h"
#include "resultcodes.h"

/******************************************************************************
                            Package Defines                             
******************************************************************************/
#define MEM_ALIGN32_MASK        0x03          // Mask to check 32 bit alignment
#define MEM_END_HDR1            0x96690000    // 1st Unique Header Pattern
#define MEM_END_HDR2            0xB5E24A1D    // 2nd Unique Header Pattern
#define MEM_MSW_MASK            0xFFFF0000    // Most Significant Word Mask
#define MEM_LSW_MASK            0x0000FFFF    // Least Significant Word Mask
#define MEM_LOC_ERASED          0xFFFFFFFF    // Memory Location Erased

// Macro to align sizes to long word
#define MEM_ALIGN_SIZE(size)    ((size + MEM_ALIGN32_MASK) & ~MEM_ALIGN32_MASK)

/******************************************************************************
                             Package Typedefs                            
******************************************************************************/
// Memory Block type describes the blocks within the Flash Memory 
typedef enum
{
   MEM_BLOCK_LOG = 0,       // block for all written logs
   MEM_BLOCK_MAX            // total number of defined blocks
} MEM_BLOCK_TYPE;

// Memory State Machine States
typedef enum
{
   MEM_IDLE      = 0,  // Memory Process Not Busy
   MEM_PROG      = 1,  // Memory in Program State
   MEM_ERASE     = 2,  // Memory in Erase State
   MEM_READ      = 3,  // Memory in Read State
 //  MEM_MAX_STATE = 4,  // Maximum Number of States
} MEM_STATE;

// Valid Memory Commands
typedef enum
{
   MEM_PROG_CMD,            // Memory Program Command
   MEM_ERASE_CMD,           // Memory Erase Command
   MEM_READ_CMD,            // Memory Read Command
   MEM_STATUS_CMD           // Memory Status Command
}MEM_CMD;

// Memory Status Types
typedef enum
{
   MEM_STATUS_PROG,         // Memory Status Type Program
   MEM_STATUS_ERASE         // Memory Status Type Erase
} MEM_STATUS_TYPE;

// Memory Programming Status
typedef enum
{
   MEM_PROG_PENDING,        // Programming Pending 
   MEM_PROG_IN_PROGRESS,    // Programming in Progress
   MEM_PROG_COMPLETE_PASS,  // Programming Completed and Passed
   MEM_PROG_FAIL            // Programming Completed and Failed
} MEM_PROG_STATUS;

// Memory Erase Status
typedef enum
{
   MEM_ERASE_PENDING,       // Erase is Pending  
   MEM_ERASE_IN_PROGRESS,   // Erase is in Progress
   MEM_ERASE_COMPLETE_PASS, // Erase Completed successfully
   MEM_ERASE_FAIL           // Erase Completed and failed
} MEM_ERASE_STATUS ;

// Memory States for the Clear Task
typedef enum
{
   MEM_CLR_PROG_START,      // Start the Memory Clear Task
   MEM_CLR_PROG_DATA,       // Clear all the data
   MEM_CLR_DATA_STATUS      // Check the status of the clear
}MEM_CLR_STATES;

// Memory Block Definition
typedef struct
{
  UINT16 nSectors;          // Number of sectors for Memory Block
  UINT32 nStartOffset;      // Offset from the memory base to the start of Block
  UINT32 nEndOffset;        // Offset from the memory base to the end of Block
  UINT32 nCurrentOffset;    // Current Offset within the block
  UINT32 nUsed;             // Amount of Block that has been written
  UINT32 nFree;             // Amount of Block that remains free
}MEM_BLOCK;

// Memory Map for all blocks
typedef struct
{
  UINT16      nBlocks;                  // Number of memory blocks in system
  MEM_BLOCK   MemBlock[MEM_BLOCK_MAX];  // Array of memory blocks
}MEM_MAP;  

// Memory Description 
typedef struct
{
   RESULT         MemMgrStatus; // Operational Status of the Memory Manager
   UINT32         nTotalSize;   // Total size in bytes of the memory
   UINT32         nSectorSize;   // Size in bytes of a sector
   UINT32         nAllocated;   // Amount of memory in bytes that has been allocated
   UINT32         nFree;        // Amount of memory in bytes that is not allocated
   MEM_MAP        MemMap;       // Memory Map of all blocks
 } MEM_DESC;

// Memory Program RMT Task Control Block 
typedef struct
{
   UINT32          *pData;         // Pointer to the data to program
   UINT32          nData;          // Size of the data to program in bytes
   MEM_BLOCK_TYPE  nBlock;         // Block to write data into
   UINT32          nBlockOffset;   // Offset into the block in bytes
   MEM_PROG_STATUS PStatus;        // Program Status  
   RESULT          ResultCode;     // Result Code storage for any errors
} MEM_PROG_TASK_PARMS;

// Memory Erase RMT Task Control Block
typedef struct
{
   /**************************************************************************/
   // Populated before running task
   MEM_BLOCK_TYPE   nBlock;        // Block where the erase should be done
   UINT32           nBlockOffset;  // Offset to start of erase in block
   UINT32           nData;         // Size of the erase to be performed
   /**************************************************************************/
   // Status returned from the task
   MEM_ERASE_STATUS EStatus;       // Status of the Erase
   RESULT           ResultCode;    // Result Code Storage for any erase errors
   /**************************************************************************/
   // Variables for the task
   BOOLEAN          bCheckStatus;  // Check status flag
   UINT32           nFlashOffset;  // Beginning offset in flash
   UINT32           nEndOffset;    // End Offset in Flash
} MEM_ERASE_TASK_PARMS;

// Memory Clear RMT Task Control Block
typedef struct
{
   MEM_CLR_STATES   State;         // Current State of the clear
   MEM_BLOCK_TYPE   nBlock;        // Block to perform clear
   UINT32           nBlockOffset;  // Offset in block to perform clear
   UINT32           nData;         // Size of clear to be performed
   UINT32           nWriteSize;     // Place holder for data chunks written
} MEM_CLEAR_TASK_PARMS;

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( MEM_MNGR_BODY )
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
EXPORT void    MemInitialize   ( void );
EXPORT RESULT  MemChipErase    ( void );
EXPORT UINT32  MemGetBlockSize ( MEM_BLOCK_TYPE Block );
EXPORT UINT32  MemGetSectorSize( void );
EXPORT RESULT  MemMgrStatus ( void );

EXPORT BOOLEAN MemRead         ( MEM_BLOCK_TYPE nBlock, UINT32 *nBlockOffset,
                                 void *pData, UINT32 nSize, RESULT *ResultCode );  
EXPORT BOOLEAN MemWrite        ( MEM_BLOCK_TYPE nBlock, UINT32 *nBlockOffset,
                                 void *pData, UINT32 nSize,
                                 RESULT *ResultCode, BOOLEAN bDirectWrite );
EXPORT BOOLEAN MemErase        ( MEM_BLOCK_TYPE nBlock, UINT32 *nBlockOffset,
                                 UINT32 nSize,  RESULT *ResultCode );
EXPORT BOOLEAN MemGetStatus    ( MEM_STATUS_TYPE StatusType, MEM_BLOCK_TYPE nBlock,
                                 UINT32 *nBlockOffset, RESULT *ResultCode );
EXPORT void    MemEraseTask    ( void* pPBlock );

EXPORT void    MemSetupErase   ( MEM_BLOCK_TYPE nBlock, UINT32 nOffset, BOOLEAN *bDone);
EXPORT void    MemCommandErase ( MEM_BLOCK_TYPE nBlock, UINT32 nOffset,
                                 UINT32 nSize, BOOLEAN *bDone);
EXPORT RESULT  MemStatusErase  ( MEM_BLOCK_TYPE nBlock, UINT32 nOffset,
                                 UINT32 nSize, BOOLEAN *bDone);


#endif // MEM_MNGR_H 
/*************************************************************************
 *  MODIFICATIONS
 *    $History: MemManager.h $
 * 
 * *****************  Version 16  *****************
 * User: John Omalley Date: 6/27/11    Time: 10:50a
 * Updated in $/software/control processor/code/system
 * SCR 1026 - Added Interrupt protection and removed State Table.
 * 
 * *****************  Version 15  *****************
 * User: John Omalley Date: 5/31/11    Time: 3:40p
 * Updated in $/software/control processor/code/system
 * SCR 1026 - Added not Busy State
 * 
 * *****************  Version 14  *****************
 * User: John Omalley Date: 10/25/10   Time: 6:04p
 * Updated in $/software/control processor/code/system
 * SCR 961 - Code Review Updates
 * 
 * *****************  Version 13  *****************
 * User: John Omalley Date: 9/30/10    Time: 1:25p
 * Updated in $/software/control processor/code/system
 * SCR 894 - LogManager Code Coverage and Code Review Updates
 * 
 * *****************  Version 12  *****************
 * User: John Omalley Date: 9/09/10    Time: 2:14p
 * Updated in $/software/control processor/code/system
 * SCR 855 - Changed NOT_BUSY to READ state and removed the MEM_NOT_BUSY
 * function.
 * 
 * *****************  Version 11  *****************
 * User: John Omalley Date: 7/30/10    Time: 5:04p
 * Updated in $/software/control processor/code/system
 * SCR 699 - Change MemInitialize to return void
 * 
 * *****************  Version 10  *****************
 * User: John Omalley Date: 7/23/10    Time: 10:16a
 * Updated in $/software/control processor/code/system
 * SCR 306 - Updates for error paths
 * 
 * *****************  Version 9  *****************
 * User: Contractor3  Date: 6/25/10    Time: 9:46a
 * Updated in $/software/control processor/code/system
 * SCR #662 - Changes based on code review
 * 
 * *****************  Version 8  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:55p
 * Updated in $/software/control processor/code/system
 * SCR #587 Change TmTaskCreate to return void
 * 
 * *****************  Version 7  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 * 
 *
 ***************************************************************************/

