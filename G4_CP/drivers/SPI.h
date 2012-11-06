#ifndef SPI_H
#define SPI_H
/******************************************************************************
         Copyright (C) 2007-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

  File:        SPI.h

  Description: SPI driver for the MCF5485 QSPI peripheral

   VERSION
    $Revision: 14 $  $Date: 12-11-02 12:55p $


******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "ResultCodes.h"
#include "mcf548x.h"
#include "SystemLog.h"

/******************************************************************************
                                 Package Defines
******************************************************************************/
//The Coldfire hardware handle up to 8 SPI devices automatically in the DCTARn
//registers
#define SPI_MAX_DEVS 8

//Value assigned to the DSPI GPIO register to make all the
//chip select lines inactive.
#define SPI_ALL_CS_INACTIVE  (MCF_GPIO_PODR_DSPI_PODR_DSPI3 | \
                              MCF_GPIO_PODR_DSPI_PODR_DSPI4 | \
                              MCF_GPIO_PODR_DSPI_PODR_DSPI5 | \
                              MCF_GPIO_PODR_DSPI_PODR_DSPI6)

/******************************************************************************
                                 Package Typedefs
******************************************************************************/
//Lists of the device types on the SPI bus
typedef enum {  SPI_DEV_ADC_WR = 0, //-ADC Read/Write are seperate devices because
                SPI_DEV_ADC_RD,     // the read/write ops. of the LTC1594 are different
                SPI_DEV_RTC,        //-Real-time clock, Maxim DS1306
                SPI_DEV_EEPROM1,    //-EEPROM Microchip 25LC1024
                SPI_DEV_EEPROM2,    //-EEPROM Microchip 25LC1024
                SPI_DEV_END
             } SPI_DEVS;


//Information needed for exchanging data with a device on the SPI bus
//See Coldfire 548x Reference manual for register details
typedef struct { UINT32 DCTAR;       //-"Transfer Attributes"/DCTARn register
                 UINT32 DTFRCmd;     //-upper word of DTFR aka SPI Command register
                 UINT8  CS;          //-GPIO register value to chip select the device
               } SPI_DEV_INFO;


//--------Initalialization of SPI_DEV_INFO for each of the 8 possible SPI bus devices---------

//--DEVICE 0 - LTC1594 (A/D Converter) WRITE--
//DCTAR register settings
// Transfer size = 4 bits; data transmitted MSB first
// The inactive state of DSPISCK is low
// Data is captured on the leading edge of DSPISCK and changed on the following edge
// Baud rate = 195.31 Kbps
// DSPICSn to DSPISCK delay = 20.00 nanoseconds
// After DSPISCK delay = 20.00 nanoseconds
// Delay after transfer = 20.00 nanoseconds
//DFTRCmd register Settings
// DCTAR0
// No chip select to write the MUX
#define SPI_DEV_DEFAULT_0_DCTAR   (MCF_DSPI_DCTARn_TRSZ(0x3) | \
                                   MCF_DSPI_DCTARn_PBR(0x0)  | \
                                   MCF_DSPI_DCTARn_BR(0x8))
#define SPI_DEV_DEFAULT_0_DTFRCmd (MCF_DSPI_DTFR_CTAS(0))
#define SPI_DEV_DEFAULT_0_CS       SPI_ALL_CS_INACTIVE



//--DEVICE 1 - LTC1594 (A/D Converter) READ--
//DCTAR register settings
// Transfer size = 15 bits; data transmitted MSB first
// The inactive state of DSPISCK is low
// Data is captured on the leading edge of DSPISCK and changed on the following edge
// Baud rate = 195.31 Kbps
// DSPICSn to DSPISCK delay = 3.20 microseconds
// After DSPISCK delay = 3.20 microseconds
// Delay after transfer = 3.20 microseconds
//DTFRCmd register settings
// DCTAR1
// Chip Select 0 (GPIO 3)
#define SPI_DEV_DEFAULT_1_DCTAR   (MCF_DSPI_DCTARn_TRSZ(0xe)   | \
                                   MCF_DSPI_DCTARn_PCSSCK(0x2) | \
                                   MCF_DSPI_DCTARn_PASC(0x2)   | \
                                   MCF_DSPI_DCTARn_PDT(0x2)    | \
                                   MCF_DSPI_DCTARn_PBR(0x0)    | \
                                   MCF_DSPI_DCTARn_CSSCK(0x5)  | \
                                   MCF_DSPI_DCTARn_ASC(0x5)    | \
                                   MCF_DSPI_DCTARn_DT(0x5)     | \
                                   MCF_DSPI_DCTARn_BR(0x8))
#define SPI_DEV_DEFAULT_1_DTFRCmd  (MCF_DSPI_DTFR_CTAS(1))
#define SPI_DEV_DEFAULT_1_CS       ~MCF_GPIO_PODR_DSPI_PODR_DSPI3



//--DEVICE 2 - DS1306/DS3234 (Real-time Clock)--
//NOTE: 3234 is underclocked as configured, but it is to remain backwards
//      compatible with the 1306.
//DCTAR register settings
// Transfer size = 8 bits; data transmitted MSB first
// The inactive state of DSPISCK is low
// Data is changed on the leading edge of DSPISCK and captured on the following edge
// Baud rate = 520.83 Kbps
// DSPICSn to DSPISCK delay = 4.48 microseconds
// After DSPISCK delay = 280.00 nanoseconds
// Delay after transfer = 4.48 microseconds
//DTFRCmd register settings
// Continous CS per transfer
// DCTAR2
// Chip Select 2 (GPIO 4)
#define SPI_DEV_DEFAULT_2_DCTAR  (MCF_DSPI_DCTARn_TRSZ(0x7)   |\
                                  MCF_DSPI_DCTARn_CPHA        |\
                                  MCF_DSPI_DCTARn_PCSSCK(0x3) |\
                                  MCF_DSPI_DCTARn_PASC(0x3)   |\
                                  MCF_DSPI_DCTARn_PDT(0x3)    |\
                                  MCF_DSPI_DCTARn_PBR(0x1)    |\
                                  MCF_DSPI_DCTARn_CSSCK(0x5)  |\
                                  MCF_DSPI_DCTARn_ASC(0x1)    |\
                                  MCF_DSPI_DCTARn_DT(0x5)     |\
                                  MCF_DSPI_DCTARn_BR(0x6))
#define SPI_DEV_DEFAULT_2_DTFRCmd (MCF_DSPI_DTFR_CTAS(2))
#define SPI_DEV_DEFAULT_2_CS      ~MCF_GPIO_PODR_DSPI_PODR_DSPI4



//--DEVICE 3 - 25LC1024 (SPI EEPROM)--
//DCTAR register settings
// Transfer size = 8 bits; data transmitted MSB first
// The inactive state of DSPISCK is low
// Data is captured on the leading edge of DSPISCK and changed on the following edge
// Baud rate = 5MHz
// DSPICSn to DSPISCK delay = 100.00 nanoseconds
// After DSPISCK delay = 200.00 nanoseconds
// Delay after transfer = 60.00 nanoseconds
//DTFRCmd register settings
// Continous CS per transfer
// DCTAR3
// Chip Select 3 (GPIO 5)
#define SPI_DEV_DEFAULT_3_DCTAR   (MCF_DSPI_DCTARn_TRSZ(0x7)   | \
                                   MCF_DSPI_DCTARn_PCSSCK(0x2) | \
                                   MCF_DSPI_DCTARn_PASC(0x2)   | \
                                   MCF_DSPI_DCTARn_PDT(0x1)    | \
                                   MCF_DSPI_DCTARn_PBR(0x2)    | \
                                   MCF_DSPI_DCTARn_ASC(0x1)    | \
                                   MCF_DSPI_DCTARn_BR(0x1))
#define SPI_DEV_DEFAULT_3_DTFRCmd (MCF_DSPI_DTFR_CTAS(3))
#define SPI_DEV_DEFAULT_3_CS      ~MCF_GPIO_PODR_DSPI_PODR_DSPI5



//--DEVICE 4 - 25LC1024 (SPI EEPROM)--
//DCTAR register settings
// Transfer size = 8 bits; data transmitted MSB first
// The inactive state of DSPISCK is low
// Data is captured on the leading edge of DSPISCK and changed on the following edge
// Baud rate = 5MHz
// DSPICSn to DSPISCK delay = 100.00 nanoseconds
// After DSPISCK delay = 200.00 nanoseconds
// Delay after transfer = 60.00 nanoseconds
//DTFRCmd register settings
// Continous CS per transfer
// DCTAR3
// Chip Select 5 (GPIO 6)
#define SPI_DEV_DEFAULT_4_DCTAR  (MCF_DSPI_DCTARn_TRSZ(0x7)   | \
                                  MCF_DSPI_DCTARn_PCSSCK(0x2) | \
                                  MCF_DSPI_DCTARn_PASC(0x2)   | \
                                  MCF_DSPI_DCTARn_PDT(0x1)    | \
                                  MCF_DSPI_DCTARn_PBR(0x2)    | \
                                  MCF_DSPI_DCTARn_ASC(0x1)    | \
                                  MCF_DSPI_DCTARn_BR(0x1))
#define SPI_DEV_DEFAULT_4_DTFRCmd (MCF_DSPI_DTFR_CTAS(3))
#define SPI_DEV_DEFAULT_4_CS     ~MCF_GPIO_PODR_DSPI_PODR_DSPI6



//-No information defined for SPI
// devices 4-7, reserved for
// future use

#define SPI_DEV_DEFAULT_5_DCTAR   (0)
#define SPI_DEV_DEFAULT_5_DTFRCmd (0)
#define SPI_DEV_DEFAULT_5_CS      (0)

#define SPI_DEV_DEFAULT_6_DCTAR   (0)
#define SPI_DEV_DEFAULT_6_DTFRCmd (0)
#define SPI_DEV_DEFAULT_6_CS      (0)

#define SPI_DEV_DEFAULT_7_DCTAR   (0)
#define SPI_DEV_DEFAULT_7_DTFRCmd (0)
#define SPI_DEV_DEFAULT_7_CS      (0)



// SPI Initialization value
#define MCF_DSPI_DMCR_VAL  (MCF_DSPI_DMCR_MSTR  | \
                            MCF_DSPI_DMCR_ROOE  | \
                            MCF_DSPI_DMCR_CSIS5 | \
                            MCF_DSPI_DMCR_CSIS3 | \
                            MCF_DSPI_DMCR_CSIS0 | \
                            MCF_DSPI_DMCR_DTXF  | \
                            MCF_DSPI_DMCR_DRXF)

#define MCF_DSPI_DIRSR_VAL  0x0000


#define MCF_GPIO_PAR_DSPI_VAL (MCF_GPIO_PAR_DSPI_PAR_SCK(0x3)  | \
                               MCF_GPIO_PAR_DSPI_PAR_SIN(0x3)  | \
                               MCF_GPIO_PAR_DSPI_PAR_SOUT(0x3))

#define MCF_GPIO_PDDR_DSPI_VAL 0x78


#pragma pack(1)
typedef struct {
  RESULT  result;
} SPI_DRV_PBIT_LOG;
#pragma pack()



/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( SPI_BODY )
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
EXPORT RESULT SPI_Init      (SYS_APP_ID *SysLogId, void *pdata, UINT16 *psize);
EXPORT RESULT SPI_WriteByte (SPI_DEVS Device, const UINT8* Byte,  BOOLEAN CS);
//EXPORT RESULT SPI_WriteWord (SPI_DEVS Device, UINT16* Word, BOOLEAN CS);
EXPORT RESULT SPI_WriteBlock(SPI_DEVS Device, const BYTE* Data,   UINT16 Cnt, BOOLEAN CS);
EXPORT RESULT SPI_ReadByte  (SPI_DEVS Device, UINT8* Byte,  BOOLEAN CS);
EXPORT RESULT SPI_ReadWord  (SPI_DEVS Device, UINT16* Word, BOOLEAN CS);
EXPORT RESULT SPI_ReadBlock (SPI_DEVS Device, BYTE* Data,   UINT16 Cnt, BOOLEAN CS);


#endif   // SPI_H
/*************************************************************************
 *  MODIFICATIONS
 *    $History: SPI.h $
 * 
 * *****************  Version 14  *****************
 * User: Melanie Jutras Date: 12-11-02   Time: 12:55p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 File Format Error
 *
 * *****************  Version 13  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 12  *****************
 * User: Jim Mood     Date: 2/09/12    Time: 11:20a
 * Updated in $/software/control processor/code/drivers
 * SCR:1110 Update UART Driver to toggle each duplex and direction control
 * line at startup init. Update SPI ADC clock to 200kHz
 *
 * *****************  Version 11  *****************
 * User: Contractor3  Date: 5/27/10    Time: 10:56a
 * Updated in $/software/control processor/code/drivers
 * SCR #617 - Changes made to meet coding standards as a result of code
 * review.
 *
 * *****************  Version 10  *****************
 * User: Peter Lee    Date: 12/01/09   Time: 5:36p
 * Updated in $/software/control processor/code/drivers
 * SCR #350 Misc code review items
 *
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 10/26/09   Time: 4:49p
 * Updated in $/software/control processor/code/drivers
 * SCR #315 CBIT and SEU Processing
 *
 * *****************  Version 8  *****************
 * User: Jim Mood     Date: 9/25/09    Time: 9:00a
 * Updated in $/software/control processor/code/drivers
 * SCR #269
 *
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 9/15/09    Time: 6:12p
 * Updated in $/software/control processor/code/drivers
 * SCR #94, #95 PBIT returns log structure to Init Mgr
 *
 * *****************  Version 6  *****************
 * User: Peter Lee    Date: 9/14/09    Time: 3:54p
 * Updated in $/software/control processor/code/drivers
 * SCR #94 Updated Init for SET_CHECK() and return ERR CODE Struct
 *
 * *****************  Version 5  *****************
 * User: Jim Mood     Date: 5/01/09    Time: 9:34a
 * Updated in $/software/control processor/code/drivers
 * SCR #189 EEPROM SPI Clock
 *
 * *****************  Version 4  *****************
 * User: Jim Mood     Date: 2/03/09    Time: 5:09p
 * Updated in $/control processor/code/drivers
 * Added 2nd EEPROM device on CS5.  Updated comment style
 *
 *
 *
 ***************************************************************************/


