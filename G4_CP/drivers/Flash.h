#ifndef FLASH_H
#define FLASH_H
/******************************************************************************
            Copyright (C) 2009-2010 Pratt & Whitney Engine Services, Inc. 
                      Altair Engine Diagnostic Solutions
               All Rights Reserved. Proprietary and Confidential.

    File:        Flash.h
    
    Description: Flash Memory Driver functions.
    
    VERSION
    $Revision: 15 $  $Date: 10/01/10 5:24p $       
    
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "ResultCodes.h"
#include "SystemLog.h"

/******************************************************************************
                                 Package Defines
******************************************************************************/
// 2 x 16 as 32
#define FLASH_CFG_MULTIPLIER              0x00010001  // Multi-Part Multiplier
#define FLASH_BYTES_PER_OP                0x00000004  // Number of bytes per op
#define FLASH_SECTOR_SHIFT                0x00000012  // Shift to find sector
#define FLASH_MAX_SECTORS                 0x00000400  // Max Sectors  
#define FLASH_DEV_READ_MASK               0x0000FFFF  // Device Read Mask
#define FLASH_NUMBER_OF_PARTS             0x00000002  // Number of flash parts

typedef UINT32 FLASHDATA;                             // Flash Data type def
typedef UINT32 FLASHADDR;                             // Flash Addr type def
typedef UINT32 BYTECOUNT;                             // Byte count type def

/*****************************************************************************/
/* The following defines are address offset as defined                       */
/* in JEDEC Publication JEP137B                                              */
/* "Common Flash Interface (CFI) ID Codes"                                   */
/*****************************************************************************/
#define FLASH_CFI_ADDRESS                 0x00550055  // Offset for CFI Data
#define FLASH_CFI_QUERY_CMD   ((FLASHDATA)0x98989898) // Command to initiate CFI

/*****************************************************************************/
/*                    CFI Identification - Address Offsets                   */
/*****************************************************************************/
#define CFI_COMMAND_SET_LSB               0x0013      // Primary Algorithm
                                                      // Command Set and Control
                                                      // Interface ID Code
                                                      // Offset - LSB
#define CFI_COMMAND_SET_MSB               0x0014      // Offset - MSB
#define CFI_PRIMARY_EXTENDED_TABLE_LSB    0x0015      // Address of Primary
                                                      // Algorithm Extended
                                                      // Query Table
                                                      // Offset - LSB
#define CFI_PRIMARY_EXTENDED_TABLE_MSB    0x0016      // Offset - MSB
#define CFI_ALTERNATE_COMMAND_SET_LSB     0x0017      // Alternate Algorithm
                                                      // Command Set and Control
                                                      // Interface ID Code
                                                      // Offset - LSB
#define CFI_ALTERNATE_COMMAND_SET_MSB     0x0018      // Offset - MSB
#define CFI_ALTERNATE_EXTENDED_TABLE_LSB  0x0019      // Address of Alternate
                                                      // Algorithm Extended
                                                      // Query Table
                                                      // Offset - LSB
#define CFI_ALTERNATE_EXTENDED_TABLE_MSB  0x001A      // Offset - MSB
/*****************************************************************************/
/*                   CFI System Interface - Address Offsets                  */
/*****************************************************************************/
#define CFI_VCC_MIN                       0x001B      // Vcc Logic Supply
                                                      // Minimum Offset
                                                      // bits 7-4 BCD volts
                                                      // bits 3-0 BCD 100 mV
#define CFI_VCC_MAX                       0x001C      // Vcc Logic Supply
                                                      // Maximum Offset
                                                      // bits 7-4 BCD volts
                                                      // bits 3-0 BCD 100 mV
#define CFI_VPP_MIN                       0x001D      // Vpp Logic Supply
                                                      // Minimum Offset
                                                      // bits 7-4 BCD volts
                                                      // bits 3-0 BCD 100 mV
#define CFI_VPP_MAX                       0x001E      // Vpp Logic Supply
                                                      // Maximum Offset
                                                      // bits 7-4 BCD volts
                                                      // bits 3-0 BCD 100 mV
#define CFI_TYP_SINGLE_WRITE_TIMEOUT      0x001F      // Typical Timeout per
                                                      // single location write
                                                      // offset 2^n uS
#define CFI_MIN_BUFFER_TIMEOUT            0x0020      // Typical Timeout per
                                                      // buffer write offset
                                                      // 2^n uS
#define CFI_TYP_SECTOR_ERASE_TIMEOUT      0x0021      // Typical Sector Erase
                                                      // Timeout Offset
                                                      // 2^n mS
#define CFI_TYP_CHIP_ERASE_TIMEOUT        0x0022      // Typical Timeout for
                                                      // full chip offset
                                                      // 2^n mS
#define CFI_MAX_SINGLE_WRITE_TIMEOUT      0x0023      // Maximum Single Write
                                                      // Timeout Offset
                                                      // 2^n * Typical
#define CFI_MAX_BUFFER_TIMEOUT            0x0024      // Maximum Buffer Write
                                                      // Timeout Offset
                                                      // 2^n * Typical
#define CFI_MAX_SECTOR_ERASE_TIMEOUT      0x0025      // Maximum Sector Erase
                                                      // Timeout Offset
                                                      // 2^n * Typical
#define CFI_MAX_CHIP_ERASE_TIMEOUT        0x0026      // Maximum Chip Erase
                                                      // Timeout Offset
                                                      // 2^n * Typical
/*****************************************************************************/
/*                      CFI Device Geometry - Address Offsets                */
/*****************************************************************************/
#define CFI_DEVICE_SIZE                   0x0027	// Device Size Offset
                                                      // 2^n in number of bytes
#define CFI_DEVICE_INTERFACE_LSB          0x0028      // Flash Device Interface
                                                      // Description Offset LSB
#define CFI_DEVICE_INTERFACE_MSB          0x0029      // Offset MSB
#define CFI_BYTES_PER_BUFFERED_WRITE_LSB  0x002A      // Bytes per Buffered
                                                      // write Offset - LSB
#define CFI_BYTES_PER_BUFFERED_WRITE_MSB  0x002B      // Offset - MSB
#define CFI_NUMBER_OF_ERASE_BLOCK_REGIONS 0x002C      // Number of Erase Block
                                                      // Regions Offset
                                                      // bits 7-0 = x = # of
                                                      // Erase block regions
#define CFI_EBR_START                     0x002D      // Erase Block Region
                                                      // Start Offset
                                                      // bit 31-16 = z where
                                                      // the erase block region
                                                      // is z*256bytes
                                                      // bit 15-0 = y where
                                                      // y+1 = number of erase
                                                      // blocks
                                                      // there may be erase
                                                      // block region info at
                                                      // 2Dh, 31h, 35h, and 39h
#define CFI_EBR_NUMBER_OF_SECTORS_LSB     0x0000      // Number of Sectors
                                                      // Offset from Start LSB
#define CFI_EBR_NUMBER_OF_SECTORS_MSB     0x0001      // Number of Sectors
                                                      // Offset from Start MSB
#define CFI_EBR_SECTOR_SIZE_LSB           0x0002      // Sector Size
                                                      // Offset from Start LSB
#define CFI_EBR_SECTOR_SIZE_MSB           0x0003      // Sector Size
                                                      // Offset from Start MSB

/*****************************************************************************/
/*                      CFI Conversion Defines                               */
/*****************************************************************************/
#define CFI_MSB_SHIFT                     0x0008      // CFI MSB Shift 
#define CFI_NIBBLE_SHIFT                  0x0004      // CFI Nibble Shift
#define CFI_MAX_ERASE_BLOCKS              0x0004      // Max # of erase blocks
#define CFI_EBR_SECTOR_MULTIPLIER         0x0100      // Sector Size Multiplier
#define CFI_EBR_SECTOR_ADJUST             0x0001
#define CFI_VOLTS_TO_100MV                0x000A      // CFI Conversion for
                                                      // volts to millivolts
#define CFI_100MV_TO_MV                   100         // CFI Conversion for
                                                      // 100mV to millivolts

/*****************************************************************************/
/*                     Flash Read and Write Macros                           */
/*****************************************************************************/
// The following calculates the flash address based on the flash part width
// and the offset 
#define FLASH_OFFSET(b,o)       (*(( (volatile FLASHDATA*)(b) ) + (o)))

// Macros to perform Flash Reads and Flash Writes
#ifndef WIN32

#define FLASH_WR(b,o,d)         FLASH_OFFSET((b),(o)) = (d)
#define FLASH_RD(b,o)           FLASH_OFFSET((b),(o))

#else

#include "HwSupport.h"
#define FLASH_WR(b,o,d)         FlashFileWrite((b),(o),(d))
#define FLASH_RD(b,o)           FlashFileRead((b),(o))

#endif

/******************************************************************************
                                 Package Typedefs
******************************************************************************/
// From the JEDEC Standard - CFI Identification
typedef enum
{
   FCS_NO_CMD_SET           = 0,      // No Command Set 
   FCS_INTEL_SHARP_EXTENDED = 1,      // Intel Sharp Extended Command Set
   FCS_AMD_FUJITSU_STANDARD = 2,      // AMD Fujitsu Standard Command Set
   FCS_INTEL_STANDARD       = 3,      // Intel Standard Command Set
   FCS_AMD_FUJITSU_EXTENDED = 4,      // AMD Fujitsu Extended Command Set
   FCS_WINDBOND_STANDARD    = 6,      // Windbond Standard Command Set
   FCS_MITSUBISHI_STANDARD  = 256,    // Mitsubishi Standard Command Set
   FCS_MITSUBISHI_EXTENDED  = 257,    // Mitsubishi Extended Command Set
   FCS_SST_PAGE_WRITE       = 258,    // SST Page Write Command Set
   FCS_INTEL_PERFORMANCE    = 512,    // Intel Performance Code Command Set
   FCS_INTEL_DATA           = 528     // Intel Data Command Set
} FLASH_CMD_SET;

// From the JEDEC Standard - CFI Device Geometry
// CFI Device Interface
typedef enum
{
   FI_X8_ONLY              = 0,       // x8 - Only asynchronous interface
   FI_X16_ONLY             = 1,       // x16 - Only asynchronous interface
   FI_X8_X16               = 2,       // x8 and x16 via BYTE# 
   FI_X32_ONLY             = 3,       // x32 - Only asynchronous interface
   FI_X16_X32              = 4        // x16 and x32 via WORD#
} FLASH_INTERFACE;

// Status of the Flash Device
typedef enum 
{
   FS_NOT_BUSY,                       // Flash is not busy
   FS_BUSY,                           // Flash is Busy
   FS_EXCEEDED_TIME_LIMITS,           // Flash has exceeded its internal timeout
   FS_SUSPEND,                        // Flash is Suspended
   FS_WRITE_BUFFER_ABORT              // Flash Write Buffer has aborted
} FLASH_STATUS;

// Conversion Type for CFI 
typedef enum
{
   TIME_2n_TYPICAL       = 1,         // Converts value using 2^n
   TIME_2n_TIMES_TYPICAL = 2          // Converts value using 2^n * Typical
} FLASH_SYS_CONV_TYPE;

// Software Monitored and Controlled Sector Status 
typedef enum
{
   FSECT_IDLE,                        // Sector is not busy doing anything
   FSECT_ERASE,                       // Sector is currently being erased
   FSECT_PROGRAM,                     // Sector is currently being programmed
   FSECT_ERASE_SUSPEND,               // Sector is in an erase suspend
   FSECT_PROGRAM_SUSPEND              // Sector is in a program Suspend
}FLASH_SECTOR_STATUS;

// Flash Sector Erase Sequence Commands
// The Flash sector erase should be setup first and then each subsequent
// sector should be initiate the FE_SECTOR_CMD within 50uS. The status
// of the erase should then be monitored and then the complete command
// initiated for each sector to update the SW controlled status.
typedef enum
{
  FE_SETUP_CMD,                       // Perform the sector erase setup cmds
  FE_SECTOR_CMD,                      // Perform only the sector erase cmd
  FE_STATUS_CMD,                      // Check the status of the sector erase
  FE_COMPLETE_CMD                     // Complete the erase by updating the
                                      // the flash sector status
} FLASH_ERASE_CMD;

// Flash Driver Status
typedef enum
{
  FLASH_DRV_STATUS_OK      = 0,
  FLASH_DRV_STATUS_FAULTED, 
  FLASH_DRV_STATUS_DISABLED,
  FLASH_DRV_STATUS_MAX   
}FLASH_DRV_STATUS;

// Flash Function pointer Definition
typedef void (*FLASH_RESET)                (FLASHDATA *BaseAddr, 
                                            FLASHADDR offset);
typedef void (*FLASH_UNLOCK_BYPASS_ENTRY)  (FLASHDATA *BaseAddr);
typedef void (*FLASH_UNLOCK_BYPASS_PROG)   (FLASHDATA *BaseAddr, 
                                            FLASHADDR   offset, 
                                            FLASHDATA *pData);
typedef void (*FLASH_UNLOCK_BYPASS_RESET)  (FLASHDATA *BaseAddr);
typedef void (*FLASH_PROGRAM)              (FLASHDATA *BaseAddr, 
                                            FLASHADDR offset, 
                                            FLASHDATA *pData);
typedef void (*FLASH_WRITE_BUFFER)         (FLASHDATA *BaseAddr,
                                            FLASHADDR offset);
typedef void (*FLASH_PROGRAM_BUFFER)       (FLASHDATA *BaseAddr,
                                            FLASHADDR offset);
typedef void (*FLASH_ABORT_BUFFER)         (FLASHDATA *BaseAddr);
typedef void (*FLASH_PROGRAM_SUSPEND)      (FLASHDATA *BaseAddr,
                                            FLASHADDR offset);
typedef void (*FLASH_PROGRAM_RESUME)       (FLASHDATA *BaseAddr,
                                            FLASHADDR offset);
typedef void (*FLASH_SECTOR_ERASE_SETUP)   (FLASHDATA *BaseAddr);
typedef void (*FLASH_SECTOR_ERASE_CMD)     (FLASHDATA *BaseAddr,
                                            FLASHADDR offset);
typedef void (*FLASH_CHIP_ERASE)           (FLASHDATA *BaseAddr);
typedef void (*FLASH_ERASE_SUSPEND)        (FLASHDATA *BaseAddr,
                                            FLASHADDR offset);
typedef void (*FLASH_ERASE_RESUME)         (FLASHDATA *BaseAddr,
                                            FLASHADDR offset);
typedef FLASH_STATUS (*FLASH_GET_STATUS)   (FLASHDATA *BaseAddr,
                                            FLASHADDR offset, 
                                            BOOLEAN WriteBufferProgramming);
                                     
// Flash Commands Structure
typedef struct
{
  char                      *FlashCmdSetName;         // Text Command set Name
  FLASH_CMD_SET             FlashCmdSetId;            // Command Set ID 
  FLASH_RESET               FlashReset;               // PTR to function
  FLASH_PROGRAM             FlashProgram;             // PTR to function
  FLASH_WRITE_BUFFER        FlashWriteBuffer;         // PTR to function
  FLASH_PROGRAM_BUFFER      FlashProgamBuffer;        // PTR to function
  FLASH_ABORT_BUFFER        FlashAbortBuffer;         // PTR to function
  FLASH_SECTOR_ERASE_SETUP  FlashSectorEraseSetup;    // PTR to function
  FLASH_SECTOR_ERASE_CMD    FlashSectorEraseCmd;      // PTR to function
  FLASH_CHIP_ERASE          FlashChipErase;           // PTR to function
  FLASH_GET_STATUS          FlashGetStatus;           // PTR to function
} FLASH_CMDS;

// Flash Erase Block Definition
typedef struct
{
  UINT32 SectorSize;                     // Size of Sector in Bytes
  UINT32 NumSectors;                     // Number of sectors in block
}FLASH_ERASE_BLOCK;

// Flash Identification Storage Structure
typedef struct
{
   FLASH_CMD_SET PrimaryCmdSet;          // ID of Primary Command Set
   UINT16        ExtendedTableOffset;    // Offset to extended table
   UINT16        AlternateCmdSet;        // ID of Alternate Command Set
   UINT16        AlternateTableOffset;   // Offset to alternate table 
} FLASH_ID;

// Flash System Storage Structure
typedef struct
{
   UINT32 TypWriteTime_uS;               // Typical Write Time in microseconds
   UINT32 TypMinBufferTime_uS;           // Typical Buffer Write Time in uS
   UINT32 TypSectorErase_mS;             // Typical Sector Erase time in mS
   UINT32 TypChipErase_mS;               // Typical Chip Erase Time in mS
   UINT32 MaxWriteTime_uS;               // Maximum Write Time in uS
   UINT32 MaxMinBufferTime_uS;           // Maximum Buffer write time in uS
   UINT32 MaxSectorErase_mS;             // Maximum Sector Erase time in mS
   UINT32 MaxChipErase_mS;               // Maximum Chip Erase Time in mS
} FLASH_SYS;

// Flash Geometry Storage Structure
typedef struct
{
   FLASH_DRV_STATUS  DrvStatus;          // Current status of the device driver
   UINT32            DeviceSize;         // Size of Device in Bytes
   FLASH_INTERFACE   InterfaceDesc;      // Interface description width
   UINT16            MaxByteWrite;       // Max # of bytes written in buffer mode
   UINT16            NumEraseBlocks;     // Total Number of erase blocks
   FLASH_ERASE_BLOCK EraseBlockInfo[CFI_MAX_ERASE_BLOCKS]; // Erase Block Info
} FLASH_INFO;

// Flash System Conversion Parameters
// Table was created based on this structure so that the code could loop
// through the parameters and update the system storage structure.
typedef struct 
{
   UINT16              ParameterOffset;   // Offset of the parameter in Flash
   UINT8               BCDSize;           // Number of BCD Digits
   UINT32              *pParam;           // Storage location of parameter
   FLASH_SYS_CONV_TYPE Conversion;        // Type of conversion to perform
   RESULT              DrvFaultCode;      // Fault code to report upon error
} FLASH_SYS_CALC;

// Flash Description Table 
typedef struct
{
   FLASH_CMDS          *pCmdSet;          // Pointer to location in table that
                                          // contains the command set for the
                                          // detected flash part
   FLASHDATA           *BaseAddr;         // Base Address of the flash device
   FLASH_ID            Id;                // Identification Info Storage
   FLASH_SYS           System;            // System Info Storage
   FLASH_INFO          Info;              // Flash Geometry and Status Info Storage
   FLASH_SECTOR_STATUS SectorStatus[FLASH_MAX_SECTORS]; // SW Sector Status Array
}FLASH_DESC;

#pragma pack(1)
typedef struct {
  RESULT result; 
} FLASH_DRV_PBIT_LOG; 
#pragma pack()


/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( FLASH_BODY )
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
EXPORT RESULT FlashInitialize (SYS_APP_ID *SysLogId, void *pdata, UINT16 *psize);

EXPORT RESULT FlashFindCmdSet            ( FLASH_CMD_SET CmdSet, 
                                           FLASH_CMDS **pTable);
EXPORT RESULT FlashWriteBufferMode       ( FLASHADDR FlashOffset, 
                                           FLASHDATA *pData,
                                           BYTECOUNT ByteCount,
                                           BYTECOUNT *TotalWritten );
EXPORT RESULT FlashWriteWordMode         ( FLASHADDR Offset,  
                                           FLASHDATA *pData,
                                           BYTECOUNT ByteCount,
                                           BYTECOUNT *TotalWritten );
EXPORT void   FlashRead                  ( FLASHADDR nOffset, FLASHDATA *pData,
                                           BYTECOUNT ByteCount );
EXPORT RESULT FlashChipErase             ( UINT32 *pTimeout_mS );
EXPORT RESULT FlashGetDirectStatus       ( UINT32 nOffset, BOOLEAN *pComplete);
EXPORT RESULT FlashSectorErase           ( FLASHADDR Offset,
                                           FLASH_ERASE_CMD EraseCmd,
                                           BOOLEAN *bDone);
EXPORT RESULT FlashResumeWrite           ( FLASHADDR Offset );


EXPORT FLASH_INFO          FlashGetInfo         ( void );
EXPORT FLASH_SECTOR_STATUS FlashGetSectorStatus ( FLASHADDR nOffset );
EXPORT BOOLEAN             FlashCheckSameSector ( FLASHADDR nFirstOffset,
                                                  FLASHADDR nLastOffset );
                                                  
/*****************************************************************************
 *  MODIFICATIONS
 *    $History: Flash.h $
 * 
 * *****************  Version 15  *****************
 * User: John Omalley Date: 10/01/10   Time: 5:24p
 * Updated in $/software/control processor/code/drivers
 * SCR 893 - Updates for Code Coverage - Removed Commented out code
 * 
 * *****************  Version 14  *****************
 * User: Peter Lee    Date: 9/01/10    Time: 2:50p
 * Updated in $/software/control processor/code/drivers
 * SCR #840 Code Review Updates
 * 
 * *****************  Version 13  *****************
 * User: Contractor3  Date: 5/06/10    Time: 11:27a
 * Updated in $/software/control processor/code/drivers
 * Check In SCR #586
 * 
 * *****************  Version 12  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:14p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 11  *****************
 * User: Jeff Vahue   Date: 2/24/10    Time: 12:58p
 * Updated in $/software/control processor/code/drivers
 * SCR# 364 - remove unused functions 
 * 
 * *****************  Version 10  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:23p
 * Updated in $/software/control processor/code/drivers
 * SCR 31
 * 
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 11/02/09   Time: 1:51p
 * Updated in $/software/control processor/code/drivers
 * Misc Code Update to support WIN32 and spelling corrections
 * 
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 9/15/09    Time: 6:07p
 * Updated in $/software/control processor/code/drivers
 * SCR #94, #95 PBIT returns log structure to Init Mgr
 * 
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 9/14/09    Time: 3:33p
 * Updated in $/software/control processor/code/drivers
 * SCR #94 Updated Init for SET_CHECK() and return ERR CODE Struct
 * 
 * *****************  Version 6  *****************
 * User: John Omalley Date: 8/06/09    Time: 3:46p
 * Updated in $/software/control processor/code/drivers
 * SCR 179
 *    Removed Unused flash devices from the device table
 *    Added logic to not use the Flash and Memory Manager if the flash
 * driver initialization fails.
 *    Initialized any unitialized variables.
 * 
 *
 *****************************************************************************/

#endif // FLASH_H                             
