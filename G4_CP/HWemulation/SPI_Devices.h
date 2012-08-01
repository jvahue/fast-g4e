#include "PersistFile.h"

class SPI_Device
{
public:
    SPI_Device( const char* name);
    virtual ~SPI_Device();

    virtual void Reset() {}
    virtual void WriteByte( const UINT8* value, BOOLEAN CS) {}
    virtual void WriteWord( UINT16* value, BOOLEAN CS) {}
    virtual void WriteBlock( const BYTE* value, UINT16 cnt, BOOLEAN CS) {}
    virtual bool ReadByte( UINT8* value, BOOLEAN CS) {return true;}
    virtual bool ReadWord( UINT16* value, BOOLEAN CS) {return true;}
    virtual bool ReadBlock( BYTE* value, UINT16 cnt, BOOLEAN CS) {return true;}

protected:
    CString m_name;    // device name
    UINT32  m_address; // the address to read/write
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class ADCdevice : public SPI_Device
{
public:
    enum ADC_CHANNEL{
        ADC_CHAN_AC_BUS_V   = 0,   // Aircraft Bus Voltage
        ADC_CHAN_LI_BATT_V  = 1,   // Li Battery
        ADC_CHAN_AC_BATT_V  = 2,   // Aircraft Battery Voltage
        ADC_CHAN_TEMP       = 3    // Internal ambient temperature
    };    
    
    ADCdevice();

    virtual void Reset();
    virtual void WriteByte( const UINT8* value, BOOLEAN CS);
    virtual void WriteWord( const UINT16* value, BOOLEAN CS);
    virtual bool ReadWord( UINT16* value, BOOLEAN CS);

    UINT16 m_data[ADC_CHAN_TEMP+1];
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class RTCdevice : public SPI_Device, public PersistFile
{
public:
    RTCdevice();
    ~RTCdevice();

    virtual void Reset();
    virtual void WriteByte( const UINT8* value, BOOLEAN CS);
    virtual void WriteBlock( const BYTE* value, UINT16 cnt, BOOLEAN CS);
    virtual bool ReadByte( UINT8* value, BOOLEAN CS);
    virtual bool ReadBlock( BYTE* value, UINT16 cnt, BOOLEAN CS);

protected:
    enum Registers {
        eSeconds = 0,
        eMinutes = 1,
        eHours   = 2,
        eDay     = 3,
        eDate    = 4,
        eMonth   = 5,
        eYear    = 6,
        eAlarm0Sec   = 7,
        eAlarm0Min   = 8,
        eAlarm0Hr    = 9,
        eAlarm0Day   = 10,  // A
        eAlarm1Min   = 11,  // B
        eAlarm1Hr    = 12,
        eAlarm1Day   = 13,
        eControl     = 14,
        eStatus      = 15,  // F
        eAge         = 16,  // 10
        eTempMsb     = 17,  // 11
        eTempLsb     = 18,  // 12
        eDisableTemp = 19,  // 13
        eResv0       = 20,  // 14
        eResv1       = 21,
        eResv2       = 22,
        eResv3       = 23,  // 17
        eAddr        = 24,  // 18
        eData        = 25,  // 19
        eSram        = 26,  // 20
        eSramSize    = 256, // Alarm registers through user data
        eTotalSize   = eSram+eSramSize

    };

    UINT8 m_data[eTotalSize];
	bool m_flushAll;

    void FillClock();
    void WriteFile();
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class EEPROMdevice : public SPI_Device, public PersistFile
{
public:
    EEPROMdevice(UINT32 id);

    virtual void Reset();
    virtual void WriteByte( const UINT8* value, BOOLEAN CS);
    virtual bool ReadByte( UINT8* value, BOOLEAN CS);

protected:
    enum EEstate {
        eWaitingCmd,
        eAddr0,
        eAddr1,
        eAddr2,
        eRead,
        eWrite,
        eReadStatus,
        eWriteStatus,
    };

    UINT32 m_id;
    EEstate m_state;
    bool m_reading;
    bool m_writeEnabled;
    BYTE m_status;
    bool m_eSignature;
};

