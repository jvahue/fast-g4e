#define AMD_FUJI_SCS_BODY
/******************************************************************************
            Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:        AmdFujiSCS.c       
     
    Description: This package contains all flash driver software needed to 
                 use the AMD and FUJITSU parts that use the Standard Command 
                 Set.
    VERSION
      $Revision: 11 $  $Date: 12-11-08 2:27p $   
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "AmdFujiSCS.h"

#include "TestPoints.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
// None.

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
// None.

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
// None.

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
// None.

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:    AmdFujiSCS_Reset
 *
 * Description: Writes a Software Reset command to the flash device.
 *
 * Parameters:  FLASHDATA *BaseAddr - Pointer to flash base address
 *              FLASHADDR offset - Offset of address
 *
 * Returns:     None.
 *
 * Notes:       None.
 *
 *****************************************************************************/
void AmdFujiSCS_Reset (FLASHDATA *BaseAddr, FLASHADDR offset)
{       
    /* Just in case we are suspended, try a resume */
    FLASH_WR(BaseAddr, offset, NOR_RESUME_CMD);
    /* Write Software RESET command */
    FLASH_WR(BaseAddr, 0, NOR_RESET_CMD);
}

/******************************************************************************
 * Function:    AmdFujiSCS_Program
 *
 * Description: The following function is used to program a single location
 *              in the data flash. This function should only be used for 
 *              performing single word updates. Any larger packages of data
 *              can be more efficiently programmed using the Program Buffer
 *              algorithm.
 *
 * Parameters:  FLASHDATA *BaseAddr   - Pointer to flash base address
 *              FLASHADDR   offset      - Offset to start writing data
 *              FLASHDATA *pData      - Pointer to Data to write to Flash
 *
 * Returns:     None
 *
 * Notes:
 *
 *****************************************************************************/
void AmdFujiSCS_Program(FLASHDATA *BaseAddr, FLASHADDR offset, FLASHDATA *pData)
{       
  FLASH_WR(BaseAddr, FLASH_UNLOCK_ADDR1, NOR_UNLOCK_DATA1);
  FLASH_WR(BaseAddr, FLASH_UNLOCK_ADDR2, NOR_UNLOCK_DATA2);
  /* Write Program Command */
  FLASH_WR(BaseAddr, FLASH_UNLOCK_ADDR1, NOR_PROGRAM_CMD);
  /* Write Data */
  FLASH_WR(BaseAddr, offset, *pData);
}

/******************************************************************************
 * Function:    AmdFujiSCS_WriteToBuffer
 *
 * Description: This function issues the command to begin write buffer 
 *              programming. 
 *
 * Parameters:  FLASHDATA *BaseAddr   - Pointer to flash base address
 *              FLASHADDR   offset      - Offset to start writing data
 *
 * Returns:     None
 *
 * Notes:
 *
 *****************************************************************************/
void AmdFujiSCS_WriteToBuffer (FLASHDATA *BaseAddr, FLASHADDR offset)
{       
  FLASH_WR(BaseAddr, FLASH_UNLOCK_ADDR1, NOR_UNLOCK_DATA1);
  FLASH_WR(BaseAddr, FLASH_UNLOCK_ADDR2, NOR_UNLOCK_DATA2);
  /* Write Write Buffer Load Command */
  FLASH_WR(BaseAddr, offset, NOR_WRITE_BUFFER_LOAD_CMD);
}

/******************************************************************************
 * Function:    AmdFujiSCS_ProgramBufferToFlash
 *
 * Description: This function commands the data written to the buffer to be
 *              written to the flash device.
 *
 * Parameters:  FLASHDATA *BaseAddr   - Pointer to flash Base Address
 *              FLASHADDR   offset      - Offset to start writing data
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
void AmdFujiSCS_ProgramBufferToFlash (FLASHDATA *BaseAddr, FLASHADDR offset)
{       
  /* Transfer Buffer to Flash Command */
  FLASH_WR(BaseAddr, offset, NOR_WRITE_BUFFER_PGM_CONFIRM_CMD);
}

/******************************************************************************
 * Function:    AmdFujiSCS_WriteBufferAbortReset
 *
 * Description: This function issues the abort buffer programming command.
 *
 * Parameters:  FLASHDATA *BaseAddr   - Pointer to flash Base Address
 *
 * Returns:     None
 *
 * Notes:
 *
 *****************************************************************************/
void AmdFujiSCS_WriteBufferAbortReset (FLASHDATA *BaseAddr)
{       
  /* Issue Write Buffer Abort Reset Command Sequence */
  FLASH_WR(BaseAddr, FLASH_UNLOCK_ADDR1, NOR_UNLOCK_DATA1);
  FLASH_WR(BaseAddr, FLASH_UNLOCK_ADDR2, NOR_UNLOCK_DATA2);
  /* Write to Buffer Abort Reset Command */
  FLASH_WR(BaseAddr, FLASH_UNLOCK_ADDR1, NOR_WRITE_BUFFER_ABORT_RESET_CMD);
}

/******************************************************************************
 * Function:    AmdFujiSCS_GetStatus
 *
 * Description: This function determines the flash status. 
 *
 * Parameters:  FLASHDATA *BaseAddr - Pointer to flash base address
 *              FLASHADDR   offset                 - Offset of address
 *              BOOLEAN   WriteBufferProgramming - WriteBuffer Status Check
 *
 * Returns:     FLASH_STATUS  - Status of the flash device
 *
 * Notes:
 *
 *****************************************************************************/
FLASH_STATUS AmdFujiSCS_GetStatus (FLASHDATA *BaseAddr, FLASHADDR offset, 
                                   BOOLEAN WriteBufferProgramming)
{
   // Local Data   
   FLASHDATA          dq1_mask; 
   FLASHDATA          dq2_mask; 
   FLASHDATA          dq5_mask; 
   FLASHDATA          dq6_mask; 
   FLASHDATA          dq6_toggles;
   volatile FLASHDATA status_read_1;
   volatile FLASHDATA status_read_2;
   FLASH_STATUS       DeviceStatus;

   // Local Data Initialization
   dq1_mask      = (DQ1_MASK*FLASH_CFG_MULTIPLIER);
   dq2_mask      = (DQ2_MASK*FLASH_CFG_MULTIPLIER);
   dq5_mask      = (DQ5_MASK*FLASH_CFG_MULTIPLIER);
   dq6_mask      = (DQ6_MASK*FLASH_CFG_MULTIPLIER);
   DeviceStatus  = FS_BUSY;
   // Read Device twice to get the DQ Status
   status_read_1 = FLASH_RD(BaseAddr, offset);
   status_read_2 = FLASH_RD(BaseAddr, offset);

   /* DQ6 toggles ? */
   dq6_toggles   = (status_read_1 ^ status_read_2) & dq6_mask;

   if (dq6_toggles)
   {
      /* at least one device's DQ6 toggles */

      /* Checking WriteBuffer Abort condition:          */ 
	  /* only check on the device that has DQ6 toggling */
      /* check only when doing write buffer operation   */
      if (WriteBufferProgramming && ((dq6_toggles >> 5) & status_read_2))
      {
         /* read again to make sure WriteBuffer error is correct */
         status_read_1   = FLASH_RD(BaseAddr, offset);
         status_read_2   = FLASH_RD(BaseAddr, offset);
         dq6_toggles     = (status_read_1 ^ status_read_2) & dq6_mask;
         // Don't return WBA if other device DQ6 and DQ1 
         // are not the same. They may still be busy.
         if ( (dq6_toggles && ((dq6_toggles >> 5) & status_read_2)) && 
             !((dq6_toggles >> 5) ^ (status_read_2 & dq1_mask))) 
         {
            DeviceStatus = FS_WRITE_BUFFER_ABORT;
         }
      }
      /* Checking Timeout condition: only check on the device that has DQ6 toggling */
      else if ((dq6_toggles >> 1) & status_read_2)
      {
         /* read again to make sure Timeout Error is correct */
         status_read_1   = FLASH_RD(BaseAddr, offset);
         status_read_2   = FLASH_RD(BaseAddr, offset);
         dq6_toggles     = (status_read_1 ^ status_read_2) & dq6_mask;
         // Don't return TimeOut if other device DQ6 and DQ5 
         // are not the same. They may still be busy.
         if ((dq6_toggles && ((dq6_toggles >> 1) & status_read_2)) &&
             !((dq6_toggles >> 1) ^ (status_read_2 & dq5_mask))) 
         {
            DeviceStatus = FS_EXCEEDED_TIME_LIMITS;
         }
      }
   }
   /* no DQ6 toggles on all devices */
   else
   {
      /* Checking Erase Suspend condition */
      status_read_1 = FLASH_RD(BaseAddr, offset);
      status_read_2 = FLASH_RD(BaseAddr, offset);
      if (((status_read_1 ^ status_read_2) & dq2_mask) == dq2_mask)   
      {
         // All devices DQ2 toggling
         DeviceStatus = FS_SUSPEND;
      }
      else if (((status_read_1 ^ status_read_2) & dq2_mask) == 0)    
      {
         // All devices DQ2 not toggling
         DeviceStatus = FS_NOT_BUSY;
      }
   }
  
   return TPU(DeviceStatus, eTpAmd1503);
}

/******************************************************************************
 * Function:    AmdFujiSCS_SectorEraseSetup
 *
 * Description: This function issues the sector erase setup commands. This
 *              function is used in conjunction with the erase command
 *              function. These two commands will allow for multiple
 *              sectors to be commanded into erase without having to
 *              issue erase setup command for each sector. The time between
 *              sector erase commands must be less than 50uSecs.
 *
 * Parameters:  FLASHDATA *BaseAddr - Pointer to flash base address
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
void AmdFujiSCS_SectorEraseSetup (FLASHDATA *BaseAddr)
{       
  /* Issue Sector Erase Command Sequence */
  FLASH_WR(BaseAddr, FLASH_UNLOCK_ADDR1, NOR_UNLOCK_DATA1);
  FLASH_WR(BaseAddr, FLASH_UNLOCK_ADDR2, NOR_UNLOCK_DATA2);
  FLASH_WR(BaseAddr, FLASH_UNLOCK_ADDR1, NOR_ERASE_SETUP_CMD);
  FLASH_WR(BaseAddr, FLASH_UNLOCK_ADDR1, NOR_UNLOCK_DATA1);
  FLASH_WR(BaseAddr, FLASH_UNLOCK_ADDR2, NOR_UNLOCK_DATA2);
}

/******************************************************************************
 * Function:    AmdFujiSCS_SectorEraseCmd
 *
 * Description: This function issues the sector erase command for one sector
 *              without doing the sector erase setup. This function should
 *              only be called after the Sector Erase Setup has been performed.
 *              Multiple sector erase commands can be performed as long
 *              as the 50uSec delay is not exceeded. 
 *
 * Parameters:  FLASHDATA *BaseAddr - Pointer to flash base address
 *              FLASHADDR   offset  - Offset of address
 *
 * Returns:     None
 *
 * Notes:       This driver can be called for each sector as long as the
 *              50 uS delay time does not expire.
 *
 *****************************************************************************/
void AmdFujiSCS_SectorEraseCmd (FLASHDATA *BaseAddr, FLASHADDR offset)
{       
  /* Write Sector Erase Command to Offset */
  FLASH_WR(BaseAddr, offset, NOR_SECTOR_ERASE_CMD);
}

/******************************************************************************
 * Function:    AmdFujiSCS_ChipErase
 *
 * Description: This function issues the flash chip erase command. 
 *
 * Parameters:  FLASHDATA *BaseAddr - Pointer to flash base address
 *
 * Returns:     None
 *
 * Notes:
 *
 *****************************************************************************/
void AmdFujiSCS_ChipErase (FLASHDATA *BaseAddr)
{       
  /* Issue Chip Erase Command Sequence */
  FLASH_WR(BaseAddr, FLASH_UNLOCK_ADDR1, NOR_UNLOCK_DATA1);
  FLASH_WR(BaseAddr, FLASH_UNLOCK_ADDR2, NOR_UNLOCK_DATA2);
  FLASH_WR(BaseAddr, FLASH_UNLOCK_ADDR1, NOR_ERASE_SETUP_CMD);
  FLASH_WR(BaseAddr, FLASH_UNLOCK_ADDR1, NOR_UNLOCK_DATA1);
  FLASH_WR(BaseAddr, FLASH_UNLOCK_ADDR2, NOR_UNLOCK_DATA2);
  /* Write Chip Erase Command to Base Address */
  FLASH_WR(BaseAddr, FLASH_UNLOCK_ADDR1, NOR_CHIP_ERASE_CMD);
}

// END: AMD_FUJI_SCS_BODY

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/*************************************************************************
 *  MODIFICATIONS
 *    $History: AmdFujiSCS.c $
 * 
 * *****************  Version 11  *****************
 * User: Melanie Jutras Date: 12-11-08   Time: 2:27p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 File Format Errors found during Code Review
 * 
 * *****************  Version 10  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 9  *****************
 * User: Jeff Vahue   Date: 10/18/10   Time: 7:42p
 * Updated in $/software/control processor/code/drivers
 * SCR# 848 - Code Coverage
 * 
 * *****************  Version 8  *****************
 * User: John Omalley Date: 10/01/10   Time: 5:23p
 * Updated in $/software/control processor/code/drivers
 * SCR 893 - Updates for Code Coverage - Removed commented out code
 * 
 * *****************  Version 7  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:10a
 * Updated in $/software/control processor/code/drivers
 * SCR #698 - Fix code review findings
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
 * User: John Omalley Date: 8/06/09    Time: 3:42p
 * Updated in $/software/control processor/code/drivers
 * SCR 179 - Updated the FlashSectorErase function to remove the unused
 * nOffset parameter.
 * 
  *
 ***************************************************************************/
