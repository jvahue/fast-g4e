#ifndef DRV_EEPROM_H
#define DRV_EEPROM_H
/******************************************************************************
            Copyright (C) 2008-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

   File:        EEPROM.h
  
   Description: This file defines the interface for the EEPROM Microchip 
                connected via the SPI Bus. See the c module for a detailed 
                description.

  VERSION
    $Revision: 14 $  $Date: 8/28/12 1:06p $
  
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    
#include <stdlib.h>

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "ResultCodes.h"
#include "SPI.h"
/******************************************************************************
                                 Package Defines
******************************************************************************/
#define EEPROM_SIZE 131072
#define EEPROM_MAX_ADDR EEPROM_SIZE-1
#define EEPROM_MAX_WRITE_TIME_MS      20 //Datasheet specifies 5ms, however it
                                         //needs to be long enough to ensure at
                                         //least one timer tick goes by in the
                                         //system to be valid for timeout use

//EEPROM chip instructions with the data format for data out(MOSI) and
//data in (MISO)
#define EEPROM_INST_READ  0x03    //Read EEPROM array
                                  //MOSI:[0x03][ADDR][ADDR][ADDR]xxxxxxxxxxxx
                                  //MISO:xxxxxxxxxxxxxxxxxxxxxxxx[DATA..DATA]
#define EEPROM_INST_WRITE 0x02    //Write EEPROM array
                                  //MOSI:[0x02][ADDR][ADDR][ADDR][DATA..DATA]
                                  //MISO:xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
#define EEPROM_INST_WRDI  0x04    //Write Disable
                                  //MOSI:[0x04]
                                  //MISO:xxxxxx                                  
#define EEPROM_INST_WREN  0x06    //Write Enable
                                  //MOSI:[0x06]
                                  //MISO:xxxxxx
#define EEPROM_INST_RDSR  0x05    //Read status register
                                  //MOSI:[0x05]
                                  //MISO:xxxxxx[SR]
#define EEPROM_INST_WRSR  0x01    //Write status register
                                  //MOSI:[0x01][SR]
                                  //MISO:xxxxxxxxxx
#define EEPROM_INST_RDES  0xAB    //Read Electronic Signature
                                  //MOSI:[0xAB]xxxxxxxxxxxxxx
                                  //MISO:xxxxxx[0x29]
#define EEPROM_SIGNATURE  0x29    //Electronic signature returned by the
                                  //Microchip 25LC1024 or 25AA1024
//EEPROM status register bit defs
#define EEPROM_SR_WIP     0x01
#define EEPROM_SR_WEL     0x02
#define EEPROM_SR_BP0     0x04
#define EEPROM_SR_BP1     0x08
#define EEPROM_SR_WPEN    0x80
//Size of a write page for the EEPROM.  Code using this value expects that
//it is an even power of 2
#define EEPROM_PAGE_WRITE_SIZE 256
/******************************************************************************
                                 Package Typedefs
******************************************************************************/
#pragma pack(1)
typedef struct {
  RESULT  result; 
} EEPROM_DRV_PBIT_LOG; 
#pragma pack()


/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined( DRV_EEPROM_BODY )
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
EXPORT RESULT EEPROM_Init             (SYS_APP_ID *SysLogId, void *pdest, UINT16 *psize);
EXPORT RESULT EEPROM_ReadBlock        (UINT32 Addr, void* Buf, size_t Size);
EXPORT RESULT EEPROM_WriteBlock       (SPI_DEVS Device, UINT32 Addr, void* Buf, size_t Size);
EXPORT RESULT EEPROM_GetDeviceId(UINT32 Addr, SPI_DEVS* Device);
EXPORT RESULT EEPROM_IsWriteInProgress(SPI_DEVS Device,
                                       BOOLEAN *IsWriteInProgress);


#endif // DRV_EEPROM_H 
/*************************************************************************
 *  MODIFICATIONS
 *    $History: EEPROM.h $
 * 
 * *****************  Version 14  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 13  *****************
 * User: Contractor3  Date: 3/31/10    Time: 11:48a
 * Updated in $/software/control processor/code/drivers
 * SCR #521 Code Review Issues
 * 
 * *****************  Version 12  *****************
 * User: Contractor V&v Date: 3/19/10    Time: 4:29p
 * Updated in $/software/control processor/code/drivers
 * SCR #67 Log SPI timeouts events Moved checks for Device and Wri
 * 
 * *****************  Version 11  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 3:52p
 * Updated in $/software/control processor/code/drivers
 * SCR 67 Interrupted SPI Access (Multiple / Nested SPI Access)
 * 
 * *****************  Version 10  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:14p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 9/15/09    Time: 6:10p
 * Updated in $/software/control processor/code/drivers
 * SCR #94, #95 PBIT returns log structure to Init Mgr
 * 
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 9/14/09    Time: 3:24p
 * Updated in $/software/control processor/code/drivers
 * SCR #94 Updated Init for SET_CHECK() and return ERR CODE Struct
 * 
 * *****************  Version 7  *****************
 * User: Jim Mood     Date: 2/05/09    Time: 11:25a
 * Updated in $/control processor/code/drivers
 * Added support for 2 EEPROM devices.  Added device detection logic and
 * returns an error code on init if one of the devices is not detected.
 * 
 * *****************  Version 6  *****************
 * User: Jim Mood     Date: 9/04/08    Time: 9:33a
 * Updated in $/control processor/code/drivers
 * Updated EEPROM size to 128kB.  Updated code comment style from Altair
 * to PWES style blocks.
 *
 ***************************************************************************/
