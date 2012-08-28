#define DRV_EEPROM_BODY

/******************************************************************************
         Copyright (C) 2007-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.
 
 
  File:        EEPROM.c
 
 
  Description: Device driver for the Microchip 25AA1024/25LC1024
               EEPROM connected via the SPI bus.
               
   VERSION
    $Revision: 24 $  $Date: 3/28/12 4:53p $

 
******************************************************************************/


/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "EEPROM.h"
#include "Assert.h"
#include "TTMR.h"

#include "stddefs.h"

#include "TestPoints.h"


/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/

//This macro proceeds SPI read and write operations.  If a SPI operation fails,
//it returns the SPI error codes to the caller of the EEPROM routine.  A global,
//SPI_Result is defined here to process the result codes from the SPI driver call
RESULT _SPIERR;
#define RETURN_ON_SPI_ERR(A)  if((_SPIERR = A) != DRV_OK){return _SPIERR;}

#define EEPROM_ON_CS5_START_ADDR 0x10000000 // EEPROM ON CS5 is from address 
                                            // 0x10000000 to 
                                            // 0x10000000+EEPROM_SIZE 
 
/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
BOOLEAN EEPROM1_Detected; //EEPROM on CS3
BOOLEAN EEPROM2_Detected; //EEPROM on CS5

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
BOOLEAN EEPROM_IsIDValid(SPI_DEVS Device);
    
/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/*****************************************************************************
 * Function:    EEPROM_Init
 *
 * Description: Setup the initial state for the EEPROM module
 *
 * Parameters:  *SysLogId     pointer to the SYS_APP_ID which is always set to
 *                            DRV_ID_EEPROM_PBIT_DEVICE_NOT_DETECTED but only
 *                            looked at by caller if the RESULT!=DRV_OK
 *              *pdata        pointer to the EEPROM_DRV_PBIT_LOG
 *              *psize        sizeof(EEPROM_DRV_PBIT_LOG)
 *
 * Returns:     RESULT:       DRV_OK
 *                            DRV_EEPROM_ERR_CS3_CS5_NOT_DETECTED
 *                            DRV_EEPROM_ERR_CS3_NOT_DETECTED
 *                            DRV_EERPOM_ERR_CS5_NOT_DETECTED
 *
 * Notes:       
 *
 ****************************************************************************/
//RESULT EEPROM_Init(void)
RESULT EEPROM_Init (SYS_APP_ID *SysLogId, void *pdata, UINT16 *psize)
{
#define MAX_RESULTS 4
  const RESULT resultCode[MAX_RESULTS] = {
      DRV_OK,
      DRV_EEPROM_ERR_CS3_NOT_DETECTED,
      DRV_EERPOM_ERR_CS5_NOT_DETECTED,
      DRV_EEPROM_ERR_CS3_CS5_NOT_DETECTED
  };

  EEPROM_DRV_PBIT_LOG  *pdest;
  UINT8 errIndex = 0;

  pdest = (EEPROM_DRV_PBIT_LOG *) pdata;
  memset ( pdest, 0, sizeof(EEPROM_DRV_PBIT_LOG) ); 
  
  *psize = sizeof(EEPROM_DRV_PBIT_LOG); 
 
  EEPROM1_Detected = STPU( EEPROM_IsIDValid(SPI_DEV_EEPROM1), eTpEe3730_1);
  EEPROM2_Detected = EEPROM_IsIDValid(SPI_DEV_EEPROM2);

  // create the result code index
  errIndex |= (EEPROM1_Detected == TRUE) ? 0 : 1;
  errIndex |= (EEPROM2_Detected == TRUE) ? 0 : 2;

  // set the result code
  pdest->result = resultCode[errIndex];
    
  // Note: If ->result == DRV_OK, then this value is ignored. 
  *SysLogId = DRV_ID_EEPROM_PBIT_DEVICE_NOT_DETECTED;  
  
  return pdest->result;

}



/*****************************************************************************
 * Function:    EEPROM_ReadBlock
 *
 * Description: Read a block of data (the number of requested bytes) from the 
 *              EEPROM at the address specified by the Addr argument.
 *
 * Parameters:  Addr: Starting byte address to read the data from
 *                    EEPROM ON CS3 is from address 0x00000000 to EEPROM_SIZE
 *                    EEPROM ON CS5 is from address 0x10000000 to
 *                                 0x10000000+EEPROM_SIZE 
 *              Buf:      Buffer to store the received bytes
 *                        (must be at least "Size" elements)
 *              Size:     Number of bytes to get
 *
 * Returns:     DRV_OK:                   Data sent to EEPROM
 *              DRV_EEPROM_NOT_DETECTED:  The EEPROM device was not detected
 *                                        at startup
 *
 * Notes:       The EEPROM SPI bus operates at 10MHz.  SPI overhead is 2.4us
 *              to write the command, plus .8us per byte read.  This does not
 *              include CPU overhead to execute this function
 *
 *              This function has more than one possible exit points.  The
 *              macro RETURN_ON_SPI_ERR is defined to return in the case of
 *              an SPI operation failure.
 *
 ****************************************************************************/
RESULT EEPROM_ReadBlock(UINT32 Addr, void* Buf, size_t Size)
{
  UINT8  Write;
  UINT32 i;
  BOOLEAN WriteInProgress;
  SPI_DEVS Device;
  UINT8 *ByteBuf = Buf;
  RESULT result;
  
  ASSERT( Buf != NULL);
  ASSERT_MESSAGE( Size < EEPROM_SIZE, "EE Read Size Error: %d", Size);
  
  result = EEPROM_GetDeviceId(Addr, &Device);
  if (result == DRV_OK)
  {
    RETURN_ON_SPI_ERR(EEPROM_IsWriteInProgress(Device,&WriteInProgress));
    if(WriteInProgress)
    {
      result = DRV_EEPROM_ERR_WRITE_IN_PROGRESS;
    }
    else
    {
      Write = EEPROM_INST_READ;                      //Read instruction
      RETURN_ON_SPI_ERR(SPI_WriteByte(Device,&Write,1)); 
      Write = (UINT8)(0xFF&(Addr>>16));              //Send address, MSB first  
      RETURN_ON_SPI_ERR(SPI_WriteByte(Device,&Write,1));
      Write = (UINT8)(0xFF&(Addr>>8));                          
      RETURN_ON_SPI_ERR(SPI_WriteByte(Device,&Write,1)); 
      Write = (UINT8)(0xFF&(Addr));
      RETURN_ON_SPI_ERR(SPI_WriteByte(Device,&Write,1));

      for(i = 0; i < Size - 1 ; i++)                 //Now ready to read.  Read the
      {                                              //number of requested bytes-1 
        RETURN_ON_SPI_ERR(SPI_ReadByte(Device,&ByteBuf[i],1));//into Buf               
      }
      RETURN_ON_SPI_ERR(SPI_ReadByte(Device,&ByteBuf[Size-1],0)); //Read the final byte, 
                                                                  //and release the chip select
                                                                  //when finished
    }   

  }
  return result;
}



/*****************************************************************************
 * Function:    EEPROM_WriteBlock
 *
 * Description: Write a block of data (the number of requested bytes) to the 
 *              EEPROM at the address specified by the Addr argument.
 *
 * Parameters:  Device: EEPROM Device
 *              Addr:  Starting byte address to read the data from
 *                     EEPROM ON CS3 is from address 0x00000000 to EEPROM_SIZE
 *                     EEPROM ON CS5 is from address 0x10000000 to
 *                              0x10000000+EEPROM_SIZE 
 *              *Buf:  Buffer to be written
 *              Size:   requested size to write
 *
 * Returns:     DRV_OK:                   Data sent to EEPROM
 *              
 *              DRV_SPI_TIMEOUT:          SPI device time out occurred. 
 *              
 * 
 * Notes:       The EEPROM SPI bus operates at 10MHz.  SPI overhead is 3.4us
 *              to write the command, plus .8us per byte written.
 *
 *              This function should ONLY BE CALLED BY SpiManager.
 *              The SPIManager makes calls to EEPROM_GetDeviceId
 *              and EEPROM_IsWriteInProgress as pre-conditions to calling this function.  
 *
 *              This function has more than one possible exit points.  The
 *              macro RETURN_ON_SPI_ERR is defined to return in the case of
 *              an SPI operation failure.
 *
 *              Writes must not cross %256 boundaries, or the function
 *              will ASSERT.  The EEPROM devices do not support writes across
 *              these boundaries, or writes greater than 256 bytes/call
 *
 ****************************************************************************/
RESULT EEPROM_WriteBlock(SPI_DEVS Device, UINT32 Addr, void *Buf, size_t Size)
{
  UINT8 Write;
  UINT32 i;
  UINT8 *ByteBuf = Buf;
  RESULT result = DRV_OK;

  ASSERT( Buf != NULL);

  //Mask off the bits less than a page size, ensure the upper bits are equal
  //when the address and size are added.  If they are different, then the
  //write crosses a page boundary and the write cannot continue
  ASSERT(((Addr&(~(EEPROM_PAGE_WRITE_SIZE-1))) ==
      ( (Addr + Size-1)&(~(EEPROM_PAGE_WRITE_SIZE-1)))) );
    
  Write = EEPROM_INST_WREN;                           //Write Enable instruction
  RETURN_ON_SPI_ERR(SPI_WriteByte(Device,&Write,0));

  Write = EEPROM_INST_WRITE ;                         //Write Enable instruction
  RETURN_ON_SPI_ERR(SPI_WriteByte(Device,&Write,1));

  Write = (UINT8)(0xFF&(Addr>>16));                   //Send address, MSB first   
  RETURN_ON_SPI_ERR(SPI_WriteByte(Device,&Write,1));
  Write = (UINT8)(0xFF&(Addr>>8));                          
  RETURN_ON_SPI_ERR(SPI_WriteByte(Device,&Write,1)); 
  Write = (UINT8)(0xFF&(Addr));
  RETURN_ON_SPI_ERR(SPI_WriteByte(Device,&Write,1));

  for(i = 0; i < Size - 1 ; i++)                      //Now ready to write.  Write the
  {                                                   //number of requested bytes - 1 
    Write = ByteBuf[i];
    RETURN_ON_SPI_ERR(SPI_WriteByte(Device,&Write,1));     //into Buf
  }
  Write = ByteBuf[Size-1];
  RETURN_ON_SPI_ERR(SPI_WriteByte(Device,&Write,0));  //Read the final byte, and release
                                                      //the chip select when finished
   

  return result;
}

/*****************************************************************************
* Function:    EEPROM_GetDeviceId
*
* Description: Determines the EEPROM SPI_DEV of the address passed and whether 
*              the EEPROM is detected.
*
* Parameters:  [in]  EEPROM Address
*                             
*              [out] *Pointer to EEPROM SPI_DEV in which the address is located
*                                         
*              
*
* Returns:     Status of the operation.
* Notes:       
*
****************************************************************************/
RESULT EEPROM_GetDeviceId(UINT32 Addr, SPI_DEVS* Device)
{
  RESULT result = DRV_OK;  

  if((Addr >= EEPROM_ON_CS5_START_ADDR) && EEPROM2_Detected)
  {
    *Device = SPI_DEV_EEPROM2;
  }
  else if(EEPROM1_Detected)
  {
    *Device = SPI_DEV_EEPROM1;
  }
  else
  {
    result = DRV_EEPROM_NOT_DETECTED;
    *Device = SPI_DEV_END;
  }
  return result;
}

/*****************************************************************************
 * Function:    EEPROM_IsWriteInProgress
 *
 * Description: Reads the status register of the EEPROM chip and returns
 *              the state of the WIP (Write In Progress) bit.  A new Write
 *              command cannot be issued until the WIP bit is cleared.
 *              According to the device data sheet (25AA320A) a write operation
 *              can take up to 5ms
 *              
 *
 * Parameters:  [in]  Device:  EEPROM device to check the status of.  Using a
 *                             device other than an EEPROM in this parameter
 *                             can cause unexpected behavior.
 *              [out] *IsWriteInProgress:  Pointer to a BOOLEAN to set the 
 *                                         write the write status into.
 *              
 *
 * Returns:     Result of the SPI read operation.  See drv_ResultCodes.h
 *
 * Notes:
 *              This function has more than one possible exit points.  The
 *              macro RETURN_ON_SPI_ERR is defined to return in the case of
 *              an SPI operation failure.
 *
 ****************************************************************************/
RESULT EEPROM_IsWriteInProgress(SPI_DEVS Device, BOOLEAN *IsWriteInProgress)
{
 UINT8 Byte;

  Byte = EEPROM_INST_RDSR;             //Send address, MSB first
  RETURN_ON_SPI_ERR(SPI_WriteByte(Device,&Byte,1));
  RETURN_ON_SPI_ERR(SPI_ReadByte(Device,&Byte,0));

  if( Byte & EEPROM_SR_WIP )
  {
    *IsWriteInProgress = TRUE;
  }
  else
  {
    *IsWriteInProgress = FALSE;
  }
  return DRV_OK;
}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/*****************************************************************************
 * Function:    EEPROM_IsIDValid
 *
 * Description: Check the mfg id of the passed in EEPROM device on the SPI
 *              bus.  It is expected that the ID is 0x29, per the Microchip
 *              25AA1024/25LC1024 data sheet.
 *              
 *
 * Parameters:  [in] Device: SPI device identifier for the EEPROM device
 *                           to query.  It should be obvious, but using 
 *                           another SPI device type can cause unpredictable
 *                           behavior.
 *
 * Returns:     TRUE:  ID is valid and matches
 *              FALSE: ID did not match.  Device is incorrect type, 
 *                     malfunctioning or missing.
 *                     
 *
 * Notes:       
 *
 ****************************************************************************/
BOOLEAN EEPROM_IsIDValid(SPI_DEVS Device)
{
 UINT8 Byte;
 BOOLEAN result = FALSE;
  
  Byte = EEPROM_INST_RDES;
  if(DRV_OK == SPI_WriteByte(Device, &Byte,1))
  {
    if(DRV_OK == SPI_WriteByte(Device, &Byte,1))
    {
      if(DRV_OK == SPI_WriteByte(Device, &Byte,1))
      {
        if(DRV_OK == SPI_WriteByte(Device, &Byte,1))
        {

          if(DRV_OK == SPI_ReadByte(Device, &Byte,0))
          {
            if(EEPROM_SIGNATURE == STPU( Byte, eTpEe3730))
            {
              result = TRUE;
            }
          }
        }
      }
    }
  }
  return result;
}


/*************************************************************************
 *  MODIFICATIONS
 *    $History: EEPROM.c $
 * 
 * *****************  Version 24  *****************
 * User: Contractor V&v Date: 3/28/12    Time: 4:53p
 * Updated in $/software/control processor/code/drivers
 * SCR #1107 FAST2  Handle  EEPROM files > 256 sectors
 * 
 * *****************  Version 23  *****************
 * User: John Omalley Date: 10/18/10   Time: 11:45a
 * Updated in $/software/control processor/code/drivers
 * SCR 950 - Code Review Updates
 * 
 * *****************  Version 22  *****************
 * User: Jeff Vahue   Date: 10/17/10   Time: 2:17p
 * Updated in $/software/control processor/code/drivers
 * SCR# 848 - Code Coverage
 * 
 * *****************  Version 21  *****************
 * User: Jeff Vahue   Date: 10/16/10   Time: 3:25p
 * Updated in $/software/control processor/code/drivers
 * SCR# 848 - Code Coverage tweak, failing both EE1/2 gets more coverage.
 * 
 * *****************  Version 20  *****************
 * User: Jim Mood     Date: 9/30/10    Time: 4:58p
 * Updated in $/software/control processor/code/drivers
 * SCR 903 Code Coverage Changes
 * 
 * *****************  Version 19  *****************
 * User: Jeff Vahue   Date: 9/16/10    Time: 1:28p
 * Updated in $/software/control processor/code/drivers
 * SCR# 848 - Code Coverage Mod, one LAST time
 * 
 * *****************  Version 18  *****************
 * User: Jeff Vahue   Date: 9/14/10    Time: 8:56p
 * Updated in $/software/control processor/code/drivers
 * SCR #848 - I hope I got this right this time.
 * 
 * *****************  Version 17  *****************
 * User: Jeff Vahue   Date: 9/14/10    Time: 8:47p
 * Updated in $/software/control processor/code/drivers
 * SCR# 848 - Coverage Mod 
 * 
 * *****************  Version 16  *****************
 * User: Jeff Vahue   Date: 9/14/10    Time: 8:21p
 * Updated in $/software/control processor/code/drivers
 * SCR# 848 - Code Coverage Mods
 * 
 * *****************  Version 15  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:10a
 * Updated in $/software/control processor/code/drivers
 * SCR #698 - Fix code review findings
 * 
 * *****************  Version 14  *****************
 * User: Contractor3  Date: 3/31/10    Time: 11:48a
 * Updated in $/software/control processor/code/drivers
 * SCR #521 Code Review Issues
 * 
 * *****************  Version 13  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:36p
 * Updated in $/software/control processor/code/drivers
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 * 
 * *****************  Version 12  *****************
 * User: Contractor V&v Date: 3/19/10    Time: 4:29p
 * Updated in $/software/control processor/code/drivers
 * SCR #67 Log SPI timeouts events
 * 
 * *****************  Version 11  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 3:51p
 * Updated in $/software/control processor/code/drivers
 * SCR 67 Interrupted SPI Access (Multiple / Nested SPI Access)
 * 
 * *****************  Version 10  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:14p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 9  *****************
 * User: Jeff Vahue   Date: 2/24/10    Time: 12:34p
 * Updated in $/software/control processor/code/drivers
 * SCR# 462 - Limit EEPROM read/write block to 128k max
 * 
 * *****************  Version 8  *****************
 * User: Jim Mood     Date: 9/30/09    Time: 5:19p
 * Updated in $/software/control processor/code/drivers
 * Support for devices over 64k Byte
 * 
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 9/15/09    Time: 6:04p
 * Updated in $/software/control processor/code/drivers
 * SCR #94, #95 PBIT returns log structure to Init Mgr
 * 
 * *****************  Version 6  *****************
 * User: Peter Lee    Date: 9/14/09    Time: 3:24p
 * Updated in $/software/control processor/code/drivers
 * SCR #94 Updated Init for SET_CHECK() and return ERR CODE Struct
 * 
 * *****************  Version 5  *****************
 * User: Jim Mood     Date: 2/05/09    Time: 11:25a
 * Updated in $/control processor/code/drivers
 * Added support for 2 EEPROM devices.  Added device detection logic and
 * returns an error code on init if one of the devices is not detected.
 * 
 * 
 *
 ***************************************************************************/
