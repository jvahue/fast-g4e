#include "stdafx.h"

#include "alt_stdtypes.h"
#include "EEPROM.h"

#include "SPI_Devices.h"


////////////////////////////////////////////////////////////////////////////////////////////////////
SPI_Device::SPI_Device(const char* name)
{
    m_name = name;
    m_address = 0;
}

SPI_Device::~SPI_Device()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ADCdevice::ADCdevice()
    : SPI_Device( "ADC")
{
    Reset();
}

void ADCdevice::Reset()
{
    // set some default values for these items
    m_data[ADC_CHAN_AC_BUS_V]  = 0; 
    m_data[ADC_CHAN_AC_BATT_V] = 0;  
    m_data[ADC_CHAN_LI_BATT_V] = 0; 
    m_data[ADC_CHAN_TEMP]      = 0;  
}

void ADCdevice::WriteByte( const UINT8* value, BOOLEAN CS)
{
    // compute the address of the data to be read next
    m_address = *value & ~0x8;
}

bool ADCdevice::ReadWord( UINT16* value, BOOLEAN CS)
{
    *value = m_data[m_address];
    return true;
}

void ADCdevice::WriteWord( const UINT16* value, BOOLEAN CS)
{
    m_data[m_address] = *value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
RTCdevice::RTCdevice()
    : SPI_Device( "RTC")
    , PersistFile( eTotalSize)
	, m_flushAll( true)
{
    Init( m_name);
}

void RTCdevice::Reset()
{
    if ( IsOpen())
    {
        // recall user data
        Read( m_data, eTotalSize);

        // fill in the registers
        FillClock();

        // CRATE0/1 Set
        m_data[eStatus] = 0x30;
    }
}

RTCdevice::~RTCdevice()
{
    WriteFile();
}

void RTCdevice::WriteFile()
{
    // save the current state of the device
    if (IsOpen())
    {
        SeekToBegin();
        Write( m_data, eTotalSize);
    }
}

//Convert BCD values (used in RTC alarm and clock regs) to a binary value
#define BCD2BIN(_Bcd)   ( ((_Bcd/16)*10) + (_Bcd & 0xF))
//Convert Binary values (used in TIMESTRUCT) to a BCD for the RTC regs
#define BIN2BCD(_Bin)   (((_Bin/10)*16)+(_Bin%10))

void RTCdevice::FillClock()
{
    SYSTEMTIME sysTime;
    // load up the clock based on current time
    GetSystemTime( &sysTime);

    m_data[eSeconds] = (UINT8)BIN2BCD( sysTime.wSecond);
    m_data[eMinutes] = (UINT8)BIN2BCD( sysTime.wMinute);
    m_data[eHours]   = (UINT8)BIN2BCD( sysTime.wHour);
    m_data[eDay]     = (UINT8)BIN2BCD( sysTime.wDayOfWeek);
    m_data[eDate]    = (UINT8)BIN2BCD( sysTime.wDay);
    m_data[eMonth]   = (UINT8)BIN2BCD( sysTime.wMonth);
    m_data[eYear]    = (UINT8)BIN2BCD( sysTime.wYear % 100);     
}

void RTCdevice::WriteByte( const UINT8* value, BOOLEAN CS)
{
    if ( CS == 1)
    {
        // it's an address, MSB is masked off as 0x0=0x80, 0x1=0x81, etc.
		// MSB off is read, MSB on is write - we don't care about that.
        m_address = *value & 0x7F;

        // check the address, if it is a clock register fill the clock in case the s/w reads it
        if ( m_address < eAlarm0Sec)
        {
            FillClock();
        }
    }
    else
    {
        m_data[m_address] = *value;

        // check for write to the data register
        if (m_address == eData)
        {
            m_data[eSram+m_data[eAddr]] = *value;
            m_data[eAddr] += 1;

			// for debug to see every update to RTC keep the following line
			if ( m_flushAll) 
			{
                WriteFile();
				Flush();
			} 
        }
    }
}

void RTCdevice::WriteBlock( const BYTE* value, UINT16 cnt, BOOLEAN CS)
{
    // check write protect is off
    if (((m_data[eControl] >> 6) & 1) == 0)
    {
        if (m_address == eData)
        {
            int addr = m_data[eAddr];
            for (int i = 0; i< cnt; ++i)
            {
                m_data[eData] = value[i];
                m_data[eSram+addr+i] = value[i];
            }
            m_data[eAddr] = addr+cnt;

			// for debug to see every update to RTC keep the following line
			if ( m_flushAll) 
			{
                WriteFile();
				Flush();
			}
        }
        else
        {
            memcpy( &m_data[m_address], value, cnt);
        }
    }
}

bool RTCdevice::ReadByte( UINT8* value, BOOLEAN CS)
{
    if (m_address == eData)
    {
        *value = m_data[eSram+m_data[eAddr]];
        m_data[eData] = *value;
        m_data[eAddr] += 1;
    }
    else
    {
        *value = m_data[m_address];
    }
    return true;
}

bool RTCdevice::ReadBlock( BYTE* value, UINT16 cnt, BOOLEAN CS)
{
    if (m_address == eData)
    {
        int addr = m_data[eAddr];
        for (int i = 0; i< cnt; ++i)
        {
            value[i] = m_data[eSram+addr+i];
            m_data[eData] = value[i];
        }
        m_data[eAddr] = addr+cnt;
    }
    else
    {
        memcpy( value, &m_data[m_address], cnt);
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
EEPROMdevice::EEPROMdevice( UINT32 id)
    : SPI_Device( "EEPROM")
    , PersistFile( EEPROM_SIZE)
    , m_id(id)
    , m_state(eWaitingCmd)
    , m_reading( false)
    , m_writeEnabled(false)
    , m_status(2)
    , m_eSignature( false)
{
    CString newName;
    newName.Format("%s%d", m_name, m_id);
    m_name = newName;
    Init( m_name, 0xFF);
}

void EEPROMdevice::Reset()
{
    m_state = eWaitingCmd;
    m_reading = false;
    m_writeEnabled = false;
    m_status = 2;
    m_eSignature = false;
}

void EEPROMdevice::WriteByte( const UINT8* value, BOOLEAN CS)
{
    if ( IsOpen())
    {
        switch (m_state)
        {
        case eWaitingCmd:
            switch (*value)
            {
            case EEPROM_INST_WRSR:
                m_state = eWriteStatus;
                break;
            case EEPROM_INST_RDSR:
                m_state = eReadStatus;
                break;
            case EEPROM_INST_READ:
                m_state = eAddr0;
                m_reading = true;
                break;
            case EEPROM_INST_WRITE:
                m_state = eAddr0;
                m_reading = false;
                break;
            case EEPROM_INST_WREN:
                m_writeEnabled = true;
                m_status = 2;
                break;
            case EEPROM_INST_WRDI:
                m_writeEnabled = false;
                break;
            case EEPROM_INST_RDES:
                m_eSignature = true;
                m_state = eAddr0;
                break;
            default:
                printf( "Unknown EEPROM Command %d", *value);
                break;
            }
            break;
        case eAddr0:
            m_address = *value;
            m_state = eAddr1;
            break;
        case eAddr1:
            m_address = (m_address << 8) | *value;
            m_state = eAddr2;
            break;
        case eAddr2:
            m_address = (m_address << 8) | *value;
            m_state = m_reading ? eRead : eWrite;
            Seek( m_address, CFile::begin);
            break;
        case eRead:
            break;
        case eWrite:
            if (m_writeEnabled)
            {
                Write( value, 1);
            }
            if (!CS)
            {
                m_state = eWaitingCmd;
            }
            break;
        case eReadStatus:
            break;
        case eWriteStatus:
            m_status = *value;
            m_state = eWaitingCmd;
            break;
        }
    }
}

bool EEPROMdevice::ReadByte( UINT8* value, BOOLEAN CS)
{
    bool status = true;
    if (IsOpen())
    {
        if ( m_eSignature)
        {
            m_eSignature = false;
            *value = EEPROM_SIGNATURE;
        }
        else if ( m_state == eReadStatus)
        {
            *value = m_status;
            m_state = eWaitingCmd;
        }
        else
        {
            status = (Read( value, 1) == 1);
        }

        if (!CS)
        {
            m_state = eWaitingCmd;
        }
    }
    else
    {
        status = false;
    }

    return status;
}
