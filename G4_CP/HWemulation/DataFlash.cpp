#include "stdafx.h"

#include "alt_stdtypes.h"
#include "DataFlash.h"

#include "HW_Emul.h"

char* modeNames[] = {
    "eNormal",
    "eCfiMode",
    "eUnlockBypass",
    "eSingleWrite",
    "eBufferSA_WC",   // get the sector address and word count
    "eBufferWrite",
    "eAutoselect",
    "eSectorErase",
    "eChipErase",
    "eProgram",
    "eAnyMode"
};

//--------------------------------------------------------------------------------------------------
DataFlashMemory::DataFlashMemory()
    : PersistFile(eFlashSize)
    , m_mode( eNormal)
    , m_modeLast( eNormal)
    , m_cmdCycle( 0)
    , m_addr(0)
    , m_sector(0)
    , m_lastAddr(0)
    , m_wordCount(0)
    , m_lastDataWritten(0)
{
    UINT flags = (CFile::modeCreate|CFile::modeWrite|CFile::shareDenyWrite);
    m_name = "DataFlash";
    Init( m_name, 0xFF);
    m_dfActivity.Open("Persist\\DFactivity.dat", flags);
    m_debug = false;

	// which sectors are to be erased ?
	for (int i=0; i < eMaxSectors; ++i)
	{
		m_sectorErase[i] = 0xFFFFFFFF;
	}
}

//--------------------------------------------------------------------------------------------------
DataFlashMemory::~DataFlashMemory()
{
    if ( m_dfActivity.m_hFile != CFile::hFileNull)
    {
        m_dfActivity.Close();
    }
}

//--------------------------------------------------------------------------------------------------
void DataFlashMemory::Write( UINT32* base, UINT32 offset, UINT32 data)
{
    UINT32 addr = ((UINT32)base + offset) - 0x10000000;

	// DEBUG
    if ( addr == 0x2b)
    {
        addr = addr;
    }

    m_debugMsg.Format( "Write( 0x%08x, 0x%08x) ", addr, data);

    if (m_mode == eSingleWrite || !IsCommand( addr, data))
    {
        if (m_mode == eBufferWrite)
        {
            // verify address is in the current write sector
            if ( SameSector( addr) && m_wordCount > 0)
            {
                DoWrite(addr, data);
                m_wordCount -= 1;
                m_lastAddr = addr;
            }
            else if (addr == m_lastAddr && data == NOR_WRITE_BUFFER_PGM_CONFIRM_CMD)
            {
                Debug( "Program Buffer Confirm Mode Program");
                m_mode = eProgram;
                sysCtrl->timerDF = 0;
            }
        }
        else if (m_mode == eSingleWrite)
        {
            DoWrite( addr, data);
            m_mode = eProgram;
            Debug( "Single Write Mode %s", modeNames[m_mode]);
        }
    }
	else if ( m_mode == eSectorErase && data == NOR_SECTOR_ERASE_CMD)
	{
		// adding more sectors to be erased
		SectorErase( addr);
	}

    WriteDebug();
}

//--------------------------------------------------------------------------------------------------
void DataFlashMemory::DoWrite( UINT32 addr, UINT32 data)
{
    addr = addr * 4;
    Seek( addr, CFile::begin);
    CFile::Write( &data, sizeof(UINT32));
    //CFile::Flush();
    m_lastDataWritten = data;
    Debug( "Written at %d (0x%x)", addr, addr);
}

//--------------------------------------------------------------------------------------------------
UINT32 DataFlashMemory::Read(UINT32* base, UINT32 offset)
{
    UINT32 dq1_mask = (DQ1_MASK*FLASH_CFG_MULTIPLIER);
    UINT32 dq2_mask = (DQ2_MASK*FLASH_CFG_MULTIPLIER);
    UINT32 dq5_mask = (DQ5_MASK*FLASH_CFG_MULTIPLIER);
    UINT32 dq6_mask = (DQ6_MASK*FLASH_CFG_MULTIPLIER);

    static INT32 lastTime = -1;
    static INT32 writeCount = 2;
    static int toggleState = 0;
    static int timer0Reads = 0;
    bool issueMsg = true;
    UINT32 value;
    UINT32 addr = ((UINT32)base + offset) - 0x10000000;

    m_debugMsg.Format("Read( 0x%08x) ", addr);

    if ( addr == 0xFFFF)
    {
        value = 0;
    }

    if ( m_mode == eCfiMode)
    {
        value = ProcessCommand( addr);
    }
    else if (( m_mode == eProgram) || ( m_mode == eSectorErase) || ( m_mode == eChipErase))
    {
        if (sysCtrl->timerDF > 0)
        {
            value = m_lastDataWritten;
            value &= ~dq5_mask;
            value &= ~dq1_mask;
            if (toggleState)
            {
                value |= (dq6_mask | dq2_mask);
            }
            else
            {
                value &= ~(dq6_mask | dq2_mask);
            }
            toggleState ^= 1;

            // only record one full toggle set per count down timer
            if (lastTime != sysCtrl->timerDF)
            {
                writeCount = 2;
                lastTime = sysCtrl->timerDF;
            }

            if ( writeCount > 0)
            {
                Debug("Data Read is 0x%08x Timer: %d", value, sysCtrl->timerDF);
                --writeCount;
            }
            else
            {
                issueMsg = false;
            }
        }
        else
        {
            // now that the timer is expired return two reads of last data written 
            // no toggle indicates tot he app we are done programming
            if (timer0Reads == 0)
            {
                value = m_lastDataWritten;
                ++timer0Reads;
            }
            else
            {
                // second read after timer expire code now knows the device is done
                value = m_lastDataWritten;
                timer0Reads = 0;
                m_mode = m_modeLast;
                m_cmdCycle = 0;
                writeCount = 2;
            }
        }
    }
    else
    {
        value = DoRead( addr);
    }

    if (issueMsg) WriteDebug();

    return value;
}

//--------------------------------------------------------------------------------------------------
UINT32 DataFlashMemory::DoRead( UINT32 addr)
{
    UINT32 value;
    addr = addr * 4;
    Seek( addr, CFile::begin);
    int size = CFile::Read( &value, sizeof(UINT32));
    ASSERT(size == sizeof(UINT32));

    Debug( "File offset %d [0x%08x] Mode: %s", addr, value, modeNames[m_mode]);
    return value;
}

//--------------------------------------------------------------------------------------------------
// Determine if we are receiving a command
bool DataFlashMemory::IsCommand( UINT32 addr, UINT32 data)
{
#define DONT_CARE 0xFFFFFFFF

    struct CmdMap {
        UINT32 addr;
        UINT32 data;
        UINT32 cycle;
        FlashMode inMode;
        FlashMode outMode;
        char* debug;
    };

    static CmdMap cmdMap[] = {
        // Prefix for lots of commands
        { FLASH_UNLOCK_ADDR1, NOR_UNLOCK_DATA1,  0, eAnyMode, eAnyMode, "STD1 "},
        { FLASH_UNLOCK_ADDR2, NOR_UNLOCK_DATA2,  1, eAnyMode, eAnyMode, "STD2 "},

        // Program
        { FLASH_UNLOCK_ADDR1, NOR_PROGRAM_CMD,   2, eAnyMode, eSingleWrite, "PROGRAM "},

        // Write to Buffer
        { DONT_CARE, NOR_WRITE_BUFFER_LOAD_CMD,  2, eAnyMode, eBufferSA_WC, "WRITE2BUFFER "},

        // Program Buffer to Flash (Confirm)
        // Handled in Write

        // Unlock Bypass (Enter)
        { FLASH_UNLOCK_ADDR1, NOR_UNLOCK_BYPASS_ENTRY_CMD, 2, eAnyMode, eUnlockBypass, "UB(ENTER) "},

        // Unlock Bypass (Program)
        { DONT_CARE, NOR_PROGRAM_CMD, 0, eUnlockBypass, eSingleWrite, "UB(PROGRAM) "},

        // Unlock Bypass (Sector Erase)
        { DONT_CARE, NOR_ERASE_SETUP_CMD,  0, eUnlockBypass, eUnlockBypass, "UB(SE1) "},
        { DONT_CARE, NOR_SECTOR_ERASE_CMD, 1, eUnlockBypass, eSectorErase, "UB(SE1) "},

        // Unlock Bypass (Chip Erase)
        { DONT_CARE, NOR_ERASE_SETUP_CMD, 0, eUnlockBypass, eUnlockBypass, "UB(CE1) "},
        { DONT_CARE, NOR_CHIP_ERASE_CMD,  1, eUnlockBypass, eChipErase, "UB(CE2) "},

        // Unlock Bypass (Reset)
        { DONT_CARE, NOR_UNLOCK_BYPASS_RESET_CMD1, 0, eUnlockBypass, eUnlockBypass, "UB(RESET1) "},
        { DONT_CARE, NOR_UNLOCK_BYPASS_RESET_CMD2, 1, eUnlockBypass, eNormal, "UB(RESET2)"},

        // Chip Erase
        { FLASH_UNLOCK_ADDR1, NOR_ERASE_SETUP_CMD, 2, eAnyMode, eAnyMode, "CE1 "},
        { FLASH_UNLOCK_ADDR1, NOR_UNLOCK_DATA1,    3, eAnyMode, eAnyMode, "CE2 "},
        { FLASH_UNLOCK_ADDR2, NOR_UNLOCK_DATA2,    4, eAnyMode, eAnyMode, "CE3 "},
        { FLASH_UNLOCK_ADDR1, NOR_CHIP_ERASE_CMD,  5, eAnyMode, eChipErase, "CE4 "},

        // Sector Erase
        { FLASH_UNLOCK_ADDR1, NOR_ERASE_SETUP_CMD,  2, eAnyMode, eAnyMode, "SE1 "},
        { FLASH_UNLOCK_ADDR1, NOR_UNLOCK_DATA1,     3, eAnyMode, eAnyMode, "SE2 "},
        { FLASH_UNLOCK_ADDR2, NOR_UNLOCK_DATA2,     4, eAnyMode, eAnyMode, "SE3 "},
        { DONT_CARE,          NOR_SECTOR_ERASE_CMD, 5, eAnyMode, eSectorErase, "SE4 "},
    };
    bool cmd = false;

    if (addr == FLASH_CFI_ADDRESS)
    {
        if (data == FLASH_CFI_QUERY_CMD)
        {
            Debug( "Mode switched to CFI ");

            m_mode = eCfiMode;
            cmd = true;
        }
    }
    else if ( m_mode == eBufferSA_WC)
    {
        m_sector = addr;
        m_wordCount = (data & 0xFFFF) + 1;
        Debug( "Sector 0x%08x WC: %d Mode BufferWrite ", m_sector, m_wordCount);
        if (m_wordCount == 6)
            m_wordCount = 6;
        m_mode = eBufferWrite;
        cmd = true;
    }
    else if ( addr == 0 && data == 0xF0F0F0F0) // reset command
    {
        Debug( "Chip Reset Mode Normal ");
        m_mode = eNormal;
        cmd = true;
    }
    else
    {
        for ( int i = 0; cmdMap[i].addr != 0; ++i)
        {
            if ( (cmdMap[i].addr == addr || cmdMap[i].addr == DONT_CARE) && 
                cmdMap[i].data == data && 
                cmdMap[i].cycle == m_cmdCycle && 
                (cmdMap[i].inMode == m_mode || cmdMap[i].inMode == eAnyMode))
            {
                cmd = true;
                m_cmdCycle += 1;

                Debug( cmdMap[i].debug);

                if (cmdMap[i].outMode != eAnyMode)
                {
                    m_modeLast = m_mode;
                    m_mode = cmdMap[i].outMode;
                    m_cmdCycle = 0;
                    Debug("Mode %s ", modeNames[m_mode]);
                    if ( m_mode == eSectorErase)
                    {
                        // set the cycle counter back to 5 as multiple sectors may be requested for
                        // erasure, this will be reset to zero when the erase cycle ends
                        m_cmdCycle = 5;
                    }
                }

                break;
            }
        }
    }

    if ( m_mode == eSectorErase)
    {
        SectorErase( addr);
    }

    if ( m_mode == eChipErase)
    {
        ChipErase();
    }

    return cmd;
}

//--------------------------------------------------------------------------------------------------
UINT32 DataFlashMemory::ProcessCommand( UINT32 addr)
{
#define ULV(x) ((x) << 16 | (x))

    struct CmdResponseMap {
        UINT32 addr;
        UINT32 response;
    };

    static CmdResponseMap cmdResponse[] = {
        {CFI_COMMAND_SET_LSB,              ULV(FCS_AMD_FUJITSU_STANDARD)},
        {CFI_COMMAND_SET_MSB,              0},
        {CFI_PRIMARY_EXTENDED_TABLE_LSB,   ULV(4)},
        {CFI_PRIMARY_EXTENDED_TABLE_MSB,   0},
        {CFI_ALTERNATE_COMMAND_SET_LSB,    0},
        {CFI_ALTERNATE_COMMAND_SET_MSB,    0},
        {CFI_ALTERNATE_EXTENDED_TABLE_LSB, 0},
        {CFI_ALTERNATE_EXTENDED_TABLE_MSB, 0},

        {CFI_VCC_MIN,                  ULV(0x27)},   // addr = 0x1B
        {CFI_VCC_MAX,                  ULV(0x36)},
        {CFI_VPP_MIN,                  0},
        {CFI_VPP_MAX,                  0},
        {CFI_TYP_SINGLE_WRITE_TIMEOUT, ULV(6)},
        {CFI_MIN_BUFFER_TIMEOUT,       ULV(6)},      // 0x20
        {CFI_TYP_SECTOR_ERASE_TIMEOUT, ULV(9)},
        {CFI_TYP_CHIP_ERASE_TIMEOUT,   ULV(0x13)},
        {CFI_MAX_SINGLE_WRITE_TIMEOUT, ULV(3)},
        {CFI_MAX_BUFFER_TIMEOUT,       ULV(5)},
        {CFI_MAX_SECTOR_ERASE_TIMEOUT, ULV(3)},
        {CFI_MAX_CHIP_ERASE_TIMEOUT,   ULV(2)},

        {CFI_DEVICE_SIZE,                             ULV(0x1B)},   // 0x27
        {CFI_DEVICE_INTERFACE_LSB,                    ULV(2)},
        {CFI_DEVICE_INTERFACE_MSB,                    0},
        {CFI_BYTES_PER_BUFFERED_WRITE_LSB,            ULV(6)},      // 0x2A
        {CFI_BYTES_PER_BUFFERED_WRITE_MSB,            0},
        {CFI_NUMBER_OF_ERASE_BLOCK_REGIONS,           ULV(1)},

        {CFI_EBR_START+CFI_EBR_NUMBER_OF_SECTORS_LSB, ULV(0xFF)},           // 0x2D
        {CFI_EBR_START+CFI_EBR_NUMBER_OF_SECTORS_MSB, ULV(3)},
        {CFI_EBR_START+CFI_EBR_SECTOR_SIZE_LSB,       0},
        {CFI_EBR_START+CFI_EBR_SECTOR_SIZE_MSB,       ULV(2)},

        { 0x31, ULV(0)},
        { 0x32, ULV(0)},
        { 0x33, ULV(0)},
        { 0x34, ULV(0)},
        { 0x35, ULV(0)},
        { 0x36, ULV(0)},
        { 0x37, ULV(0)},
        { 0x38, ULV(0)},
        { 0x39, ULV(0)},
        { 0x3a, ULV(0)},
        { 0x3b, ULV(0)},
        { 0x3c, ULV(0)},

        {0,0},
    };
    static int count = 0;

    UINT32 value = 0;
    for ( int i = 0; cmdResponse[i].addr != 0; ++i)
    {
        if ( addr == cmdResponse[i].addr)
        {
            value = cmdResponse[i].response;
            Debug( "CFI Value 0x%x", value);
            break;
        }
    }


    return value;
}

//--------------------------------------------------------------------------------------------------
void DataFlashMemory::SectorErase( UINT32 addr)
{
    UINT32 buffer[1024];
    UINT32 start = (addr * 4) & 0xFFFC0000;
    UINT32 endAt = (start + 0x40000);  // 256 kbytes

	memset( buffer, 0xFF, sizeof(buffer));

    Seek( start, CFile::begin);
    while ( start < endAt)
    {
        CFile::Write( buffer, sizeof(buffer));
        start += sizeof(buffer);
    }

    sysCtrl->timerDF = 50;
}

//--------------------------------------------------------------------------------------------------
void DataFlashMemory::ChipErase()
{
    Reset( 0xFF);
    sysCtrl->timerDF = 1500;
}

//--------------------------------------------------------------------------------------------------
bool DataFlashMemory::SameSector( UINT32 addr)
{
    return true;
}

//--------------------------------------------------------------------------------------------------
void DataFlashMemory::Debug(TCHAR* format, ...)
{
    if ( m_debug)
    {
        TCHAR buffer[1024];

        va_list args;
        va_start( args, format );
        vsprintf_s( buffer, 1024, format, args );

        m_debugMsg += buffer;
    }
}

//--------------------------------------------------------------------------------------------------
void DataFlashMemory::WriteDebug()
{
    if ( m_debug)
    {
        m_dfActivity.WriteString( m_debugMsg + '\n');
    }
}
