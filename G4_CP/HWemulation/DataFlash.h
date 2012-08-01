#pragma once

#include "Flash.h"
#include "AmdFujiSCS.h"

#include "PersistFile.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
class DataFlashMemory : public PersistFile
{
public:
    enum FLASH_CMD_SET
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
    } ;

    // From the JEDEC Standard - CFI Device Geometry
    // CFI Device Interface
    enum FLASH_INTERFACE
    {
        FI_X8_ONLY              = 0,       // x8 - Only asynchronous interface
        FI_X16_ONLY             = 1,       // x16 - Only asynchronous interface
        FI_X8_X16               = 2,       // x8 and x16 via BYTE# 
        FI_X32_ONLY             = 3,       // x32 - Only asynchronous interface
        FI_X16_X32              = 4        // x16 and x32 via WORD#
    } ;

    // Status of the Flash Device
    enum FLASH_STATUS
    {
        FS_NOT_BUSY,                       // Flash is not busy
        FS_BUSY,                           // Flash is Busy
        FS_EXCEEDED_TIME_LIMITS,           // Flash has exceeded its internal timeout
        FS_SUSPEND,                        // Flash is Suspended
        FS_WRITE_BUFFER_ABORT              // Flash Write Buffer has aborted
    } ;

    // Conversion Type for CFI 
    enum FLASH_SYS_CONV_TYPE
    {
        BCD_TO_mV             = 1,         // Convert BCD to millivolts
        HEX_BCD_TO_mV         = 2,         // Converts BCD nibble and adds in HEX
        TIME_2n_TYPICAL       = 3,         // Converts value using 2^n
        TIME_2n_TIMES_TYPICAL = 4          // Converts value using 2^n * Typical
    } ;

    // Software Monitored and Controlled Sector Status 
    enum FLASH_SECTOR_STATUS
    {
        FSECT_IDLE,                        // Sector is not busy doing anything
        FSECT_ERASE,                       // Sector is currently being erased
        FSECT_PROGRAM,                     // Sector is currently being programmed
        FSECT_ERASE_SUSPEND,               // Sector is in an erase suspend
        FSECT_PROGRAM_SUSPEND              // Sector is in a program Suspend
    };

    // Flash Sector Erase Sequence Commands
    // The Flash sector erase should be setup first and then each subsequent
    // sector should be initiate the FE_SECTOR_CMD within 50uS. The status
    // of the erase should then be monitored and then the complete command
    // initiated for each sector to update the SW controlled status.
    enum FLASH_ERASE_CMD
    {
        FE_SETUP_CMD,                       // Perform the sector erase setup cmds
        FE_SECTOR_CMD,                      // Perform only the sector erase cmd
        FE_STATUS_CMD,                      // Check the status of the sector erase
        FE_COMPLETE_CMD                     // Complete the erase by updating the
        // the flash sector status
    } ;

	enum DataFlashConstants {
		eMaxSectors = 1024
	};

    DataFlashMemory();
    ~DataFlashMemory();

    void Write( UINT32* base, UINT32 offset, UINT32 data);
    UINT32 Read(UINT32* base, UINT32 offset);
    void DFtiming();

protected:
    enum Constants {
        eFlashSize = 256*1024*1024
    };

    enum FlashMode {
        eNormal,
        eCfiMode,
        eUnlockBypass,
        eSingleWrite,
        eBufferSA_WC,   // get the sector address and word count
        eBufferWrite,
        eAutoselect,
        eSectorErase,
        eChipErase,
        eProgram,
        eAnyMode
    };

    FlashMode m_mode;
    FlashMode m_modeLast;
    CString m_name;
    int m_cmdCycle;
    UINT32 m_addr;
    UINT32 m_sector;
    UINT32 m_sectorErase[eMaxSectors];
    UINT32 m_lastAddr;
    UINT32 m_wordCount;
    CStdioFile m_dfActivity;
    bool m_debug;
    UINT32 m_lastDataWritten;
    CString m_debugMsg;

    bool IsCommand( UINT32 addr, UINT32 data);
    UINT32 ProcessCommand( UINT32 addr);
    void DoWrite( UINT32 addr, UINT32 data);
    UINT32 DoRead( UINT32 addr);
    bool SameSector( UINT32 addr);
    void SectorErase( UINT32 addr);
    void ChipErase();
    void Debug( TCHAR* format, ...);
    void WriteDebug();
};