#ifndef AMDFUJISCS_H
#define AMDFUJISCS_H

/******************************************************************************
            Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:        AmdFujiSCS.h
    
    Description: This file contains the support definitions for the AMD,
                 FUJITISU Flash Standard Command Set.
   VERSION
      $Revision: 9 $  $Date: 12-10-31 2:34p $    
    
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "flash.h"

/******************************************************************************
                                 Package Defines
******************************************************************************/
#define FLASH_UNLOCK_ADDR1               0x00000555
#define FLASH_UNLOCK_ADDR2               0x000002AA

/* data masks */
#define DQ0_MASK                         (0x01)
#define DQ1_MASK                         (0x02)
#define DQ2_MASK                         (0x04)
#define DQ3_MASK                         (0x08)
#define DQ4_MASK                         (0x10)
#define DQ5_MASK                         (0x20)
#define DQ6_MASK                         (0x40)
#define DQ7_MASK                         (0x80)

#define TARGET_DQ0_MASK                  (DQ0_MASK*FLASH_CFG_MULTIPLIER)
#define TARGET_WRITE_BUF_ABORT           (DQ1_MASK*FLASH_CFG_MULTIPLIER)
#define TOGGLE_BIT_2_MASK                (DQ2_MASK*FLASH_CFG_MULTIPLIER)
#define TARGET_DQ3_MASK                  (DQ3_MASK*FLASH_CFG_MULTIPLIER)
#define TARGET_DQ4_MASK                  (DQ4_MASK*FLASH_CFG_MULTIPLIER)
#define EXCEED_TIME_LIMIT_MASK           (DQ5_MASK*FLASH_CFG_MULTIPLIER)
#define TOGGLE_BIT_1_MASK                (DQ6_MASK*FLASH_CFG_MULTIPLIER)
#define TARGET_DQ7_MASK                  (DQ7_MASK*FLASH_CFG_MULTIPLIER)

/* Address/Command Info */
#define NOR_AUTOSELECT_CMD               ((FLASHDATA)0x90909090)
#define NOR_CHIP_ERASE_CMD               ((FLASHDATA)0x10101010)
#define NOR_ERASE_SETUP_CMD              ((FLASHDATA)0x80808080)
#define NOR_PROGRAM_CMD                  ((FLASHDATA)0xA0A0A0A0)
#define NOR_RESET_CMD                    ((FLASHDATA)0xF0F0F0F0)
#define NOR_RESUME_CMD                   ((FLASHDATA)0x30303030)
#define NOR_SECSI_SECTOR_ENTRY_CMD       ((FLASHDATA)0x88888888)
#define NOR_SECSI_SECTOR_EXIT_SETUP_CMD  ((FLASHDATA)0x90909090)
#define NOR_SECSI_SECTOR_EXIT_CMD        ((FLASHDATA)0x00000000)
#define NOR_SECTOR_ERASE_CMD             ((FLASHDATA)0x30303030)
#define NOR_SUSPEND_CMD                  ((FLASHDATA)0xB0B0B0B0)
#define NOR_UNLOCK_BYPASS_ENTRY_CMD      ((FLASHDATA)0x20202020)
#define NOR_UNLOCK_BYPASS_PROGRAM_CMD    ((FLASHDATA)0xA0A0A0A0)
#define NOR_UNLOCK_BYPASS_RESET_CMD1     ((FLASHDATA)0x90909090)
#define NOR_UNLOCK_BYPASS_RESET_CMD2     ((FLASHDATA)0x00000000)
#define NOR_UNLOCK_DATA1                 ((FLASHDATA)0xAAAAAAAA)
#define NOR_UNLOCK_DATA2                 ((FLASHDATA)0x55555555)
#define NOR_WRITE_BUFFER_ABORT_RESET_CMD ((FLASHDATA)0xF0F0F0F0)
#define NOR_WRITE_BUFFER_LOAD_CMD        ((FLASHDATA)0x25252525)
#define NOR_WRITE_BUFFER_PGM_CONFIRM_CMD ((FLASHDATA)0x29292929)

// Extended CFI Offsets
#define CFI_PET_MAJOR_VERSION             0x0003
#define CFI_PET_MINOR_VERSION             0x0004
#define CFI_PET_ASU_TECHNOLOGY            0x0005
#define CFI_PET_ERASE_SUSPEND             0x0006
#define CFI_PET_SECTOR_PROTECT            0x0007
#define CFI_PET_SECTOR_TEMP_UNPROTECT     0x0008
#define CFI_PET_SECTOR_PROTECTION_SCHEME  0x0009
#define CFI_PET_SIMULOP                   0x000A
#define CFI_PET_BURST_MODE_TYPE           0x000B
#define CFI_PET_PAGE_MODE_TYPE            0x000C
#define CFI_PET_BOOT_SECTOR_FLAG          0x000F
#define CFI_PET_PROGRAM_SUSPEND           0x0010


/******************************************************************************
                                 Package Typedefs
******************************************************************************/

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( AMD_FUJI_SCS_BODY )
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
EXPORT void AmdFujiSCS_Reset                (FLASHDATA *BaseAddr, FLASHADDR offset);
EXPORT void AmdFujiSCS_Program              (FLASHDATA *BaseAddr, 
                                             FLASHADDR offset, 
                                             FLASHDATA *pData);
EXPORT void AmdFujiSCS_WriteToBuffer        (FLASHDATA *BaseAddr, FLASHADDR offset);
EXPORT void AmdFujiSCS_ProgramBufferToFlash (FLASHDATA *BaseAddr, FLASHADDR offset);
EXPORT void AmdFujiSCS_WriteBufferAbortReset(FLASHDATA *BaseAddr);
EXPORT void AmdFujiSCS_SectorEraseSetup     (FLASHDATA *BaseAddr);
EXPORT void AmdFujiSCS_SectorEraseCmd       (FLASHDATA *BaseAddr, FLASHADDR offset);
EXPORT void AmdFujiSCS_ChipErase            (FLASHDATA *BaseAddr);
EXPORT FLASH_STATUS AmdFujiSCS_GetStatus    (FLASHDATA *BaseAddr, FLASHADDR offset, 
                                             BOOLEAN WriteBufferProgramming);


#endif // AMDFUJISCS_H                             
/*************************************************************************
 *  MODIFICATIONS
 *    $History: AmdFujiSCS.h $
 * 
 * *****************  Version 9  *****************
 * User: Melanie Jutras Date: 12-10-31   Time: 2:34p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 File format error
 * 
 * *****************  Version 8  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 7  *****************
 * User: John Omalley Date: 10/01/10   Time: 5:23p
 * Updated in $/software/control processor/code/drivers
 * SCR 893 - Updates for Code Coverage - Removed commented out code
 * 
 * *****************  Version 6  *****************
 * User: Contractor3  Date: 3/31/10    Time: 11:48a
 * Updated in $/software/control processor/code/drivers
 * SCR #521 Code Review Issues
 * 
 * *****************  Version 5  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:07p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 4  *****************
 * User: Jeff Vahue   Date: 2/24/10    Time: 12:58p
 * Updated in $/software/control processor/code/drivers
 * SCR# 364 - remove unused functions 
 * 
 * *****************  Version 3  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:21p
 * Updated in $/software/control processor/code/drivers
 * SCR 31
 * 
 * *****************  Version 2  *****************
 * User: John Omalley Date: 8/06/09    Time: 3:43p
 * Updated in $/software/control processor/code/drivers
 * SCR 179 - Updated the FlashSectorErase function to remove the unused
 * nOffset parameter.
 * 
 *
 *
 ***************************************************************************/   
