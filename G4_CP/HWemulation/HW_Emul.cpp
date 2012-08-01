#define HW_EMUL_BODY

#include "stdafx.h"

#include "alt_stdtypes.h"
#include "ResultCodes.h"

#include "HW_Emul.h"
#include "HwSupport.h"
#include "SPI_Devices.h"
#include "DataFlash.h"
#include "FPGAemulation.h"
#include "DioEmulation.h"
#include "DPRAMemul.h"

extern "C" {
    #include "PowerManager.h"
    #include "FPGA.h"
    #include "InitializationManager.h"
    #include "UART.h"
    #include "DPRAM.h"
    #include "SPI.h"
    #include "QAR.h"
    #include "ADC.h"

    extern void PowerFailIsr(void);
    extern void DPRAM_EP5_ISR(void);
    extern void FPGA_EP6_ISR(void);
}

#undef strncpy

////////////////////////////////////////////////////////////////////////////////////////////////////
// DATA
////////////////////////////////////////////////////////////////////////////////////////////////////

// dio MUST be declared before fpga so its constructor runs first.
// DIO zeros MBAR memory, then FPGA set MCF_GPIO_PODR_FEC1H for FPGA Cfg Done
DioEmulation dio;
DPRamEmulation dpr;
FPGAemulation fpga;

// SPI Devices
ADCdevice    ADC;
RTCdevice    RTC;
EEPROMdevice EEdev1(1);
EEPROMdevice EEdev2(2);

// sequence MUST match SPI_DEVS in SPI.h
SPI_Device* devices[] = {
    &ADC,    // Both the read and write ADC operate on the same device in emulation
    &ADC,
    &RTC,    // RTC device
    &EEdev1, // EEPROM1
    &EEdev2  // EEPROM2
};

SpFifo _bufferedGseTx[4];
SpFifo _bufferedGseRx[4];

////////////////////////////////////////////////////////////////////////////////////////////////////
class Timing
{
public:
    UINT32 sleepCount;
    UINT32 noSleepCount;
    UINT32 _10msCount;
    UINT32 secCount;
    time_t start;

    Timing();
    void KeepTime( void);
};

//--------------------------------------------------------------------------------------------------
Timing::Timing()
{
    sleepCount = 0;
    noSleepCount = 0;
    _10msCount = 0;
    secCount = 0;
    start = 0;
}

//--------------------------------------------------------------------------------------------------
void Timing::KeepTime( void)
{
    UINT32 frameExpected;

    if (start == 0)
    {
        start = clock();
    }

    frameExpected = (UINT32)((clock() - start)/10);
    if ( frameExpected <= _10msCount)
    {
        Sleep(10);
        ++sleepCount;
    }
    else
    {
        ++noSleepCount;
    }

    ++_10msCount;

    ++secCount;
    if (secCount == 100)
    {
        ++sysCtrl->runSec;
        secCount = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// CODE
////////////////////////////////////////////////////////////////////////////////////////////////////
// Main Program for the G4 Application Process
void WinG4E(void* pParam)
{
    const int GSE_PORT = 0;
    static int hb = 0;
    static int haltCount = 0;

    Timing timer;
    G4I_Data g4i;
    G4O_Data g4o;

    sysCtrl = (SYS_CTRL*)pParam;

    sysCtrl->shutDownComplete = FALSE;
    SPIE_Reset();

    TTMR_GetHSTickCount();

    // wait for an IOC connection, up to 5 seconds
    time_t end = clock() + 5;
    while (sysCtrl->holdOnConnection && clock() < end);

    Im_InitializeControlProcessor();

    sysCtrl->g4eInitComplete = TRUE;
    wdResetRequest = FALSE;

    while ( haltCount < 2) // one more frame to do PM_HALT processing
    {
        timer.KeepTime();

        memset(&g4o, 0, sizeof(g4o));

        int size = _bufferedGseTx[GSE_PORT].Pop( g4o.gseTx, GSE_TX_SIZE);
        g4o.gseTxSize = size;
        if ( g4o.gseTxSize > 0)
        {
            //TRACE("Send %d bytes to IOC", g4o.gseTxSize);
        }

        // stop talking to the IOC if we are shutting down
        if (haltCount == 0)
        {
            g4o.hb = ++hb;
            if ( wdResetRequest)
            {
                // let's shut our self down
                Pm.State = PM_HALT; 
            }
            sysCtrl->g4o.Write(g4o);
        }

        const int GSE_PORT = 0;

        // TODO: shrink the GSE Rx size in the so we can send all SPs in
        while ( sysCtrl->g4i.Read(g4i))
        {
            _bufferedGseRx[GSE_PORT].Push( g4i.gseRx, g4i.gseRxSize);
            _bufferedGseRx[1].Push( g4i.sp0Rx, g4i.sp0RxSize);
            _bufferedGseRx[2].Push( g4i.sp1Rx, g4i.sp1RxSize);
            _bufferedGseRx[3].Push( g4i.sp2Rx, g4i.sp2RxSize);
        }

        // simulate the timer interrupt -
        //   All DT and RMT will run to completion

        //TRACE("%d - ", clock());
        if (!wdResetRequest)
        {
            TTMR_GPT0ISR();
        }
        //TRACE("%d\n", clock());

        // if the user wants to shut down then shutdown
        if ( !sysCtrl->runG4thread)
        {
            SetBusBatt(0.0f, 0.0f);
        }
        else
        {
            // Set Bus and Bat V
            SetBusBatt(sysCtrl->busV, sysCtrl->batV);
        }

        // give it some time in halt 
        if ( Pm.State == PM_HALT)
        {
            ++haltCount;
        }

        if ( sysCtrl->irqRqst > sysCtrl->irqHandled)
        {
            // see G4EDlg.cpp IRQ_Names for the order
            /*
            0: "None",
            1: "FPGA",
            2: "DPRAM",
            3: "Power Fail",
            4: "GSE",
            5: "SP0",
            6: "SP1",
            7: "SP2",
            */

            switch (sysCtrl->irqId)
            {
            case 1:
                FPGA_EP6_ISR();
                break;
            case 2:
                DPRAM_EP5_ISR();
                break;
            case 3:
                PowerFailIsr();
                break;
            case 4:
                // setup some conditions
                MCF_PSC_ISR0 = MCF_PSC_ISR_TXRDY | MCF_PSC_ISR_RXRDY_FU;
                UART_PSC0_ISR();
                break;
            case 5:
                UART_PSC1_ISR();
                break;
            case 6:
                UART_PSC2_ISR();
                break;
            case 7:
                UART_PSC3_ISR();
                break;
            default:
                break;
            }

            ++sysCtrl->irqHandled;
        }
        UART_CheckForTXDone();
    }

    sysCtrl->shutDownComplete = TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UINT16 hw_UART_Transmit(UINT8 Port, const INT8* Data, UINT16 Size, UINT16 testMode)
{
    if ( testMode == 0)
    {
        _bufferedGseTx[Port].Push( (char*)Data, Size);
	}
	else
	{
	    _bufferedGseRx[Port].Push( (char*)Data, Size);
	}
    return Size;
}

UINT16 hw_UART_Receive(UINT8 Port, INT8* Data, UINT16 Size)
{
    int bytes = _bufferedGseRx[Port].Pop( (char*)Data, Size);
    if ( Port == 3)
    {
        TRACE( "B: %d %d\n", bytes, clock());
    }
    return bytes;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void SPIE_Reset(void)
{
    ADC.Reset();
    SetBusBatt(28.0f, 28.0f);
    RTC.Reset();
    EEdev1.Reset();
    EEdev2.Reset();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void SetBusBatt(float bus, float bat)
{
    static bool powerInt = true;

    UINT16 busCnt = (UINT16)(bus / (ADC_V_PER_BIT*ADC_BUS_V_PER_V));
    UINT16 batCnt = (UINT16)(bat / (ADC_V_PER_BIT*ADC_BATT_V_PER_V));

    ADC.m_data[ADCdevice::ADC_CHAN_AC_BUS_V] = busCnt;
    ADC.m_data[ADCdevice::ADC_CHAN_AC_BATT_V] = batCnt;
    ADC.m_data[ADC_CHAN_LI_BATT_V] = (UINT16)(2.95f / (ADC_V_PER_BIT)); 
    ADC.m_data[ADC_CHAN_TEMP]      = (UINT16)((30.0f+ADC_TEMP_DEG_OFFSET) / 
                                              (ADC_V_PER_BIT*ADC_TEMP_DEG_PER_V));  

    if (bus < 10.4 && powerInt)
    {
        // start the shut down process
        PowerFailIsr();
        powerInt = false;
    }
    else if ( bus > 25)
    {
        powerInt = true;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" {

RESULT SPI_Init(SYS_APP_ID *SysLogId, void *pdata, UINT16 *psize)
{
    return DRV_OK;
}
RESULT SPI_WriteByte(SPI_DEVS Device, const UINT8* Byte,  BOOLEAN CS)
{
    devices[Device]->WriteByte( Byte, CS);
    return  DRV_OK;
}

RESULT SPI_WriteWord (SPI_DEVS Device, UINT16* Word, BOOLEAN CS)
{
    devices[Device]->WriteWord( Word, CS);
    return  DRV_OK;
}

RESULT SPI_WriteBlock(SPI_DEVS Device, const BYTE* Data,   UINT16 Cnt, BOOLEAN CS)
{
    devices[Device]->WriteBlock( Data, Cnt, CS);
    return  DRV_OK;
}

RESULT SPI_ReadByte(SPI_DEVS Device, UINT8* Byte,  BOOLEAN CS)
{
    devices[Device]->ReadByte( Byte, CS);
    return  DRV_OK;
}

RESULT SPI_ReadWord( SPI_DEVS Device, UINT16* Word, BOOLEAN CS)
{
    devices[Device]->ReadWord( Word, CS);
    return  DRV_OK;
}

RESULT SPI_ReadBlock (SPI_DEVS Device, BYTE* Data,   UINT16 Cnt, BOOLEAN CS)
{
    devices[Device]->ReadBlock( Data, Cnt, CS);
    return  DRV_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void hw_QARwrite( volatile UINT16* addr, int field, UINT16 data)
{
    QARwrite( addr, (FIELD_NAMES)field, data);
    if ( addr == (FPGA_QAR_CONTROL))
    {
        if ( !QAR_R( addr, C_ENABLE) && ((field == C_TSTN) || (field == C_TSTP)))
        {
            // wrap around mode
            if ( field == C_TSTP)
            {
                QARwrite( addr+1, S_BIIP, data);
            }
            else
            {
                QARwrite( addr+1, S_BIIN, data);
            }

            // TEST Both high then status goes low
            if ( (*addr & 0xc0) == 0xc0)
            {
                *(addr+1) = *(addr+1) & 0x3F;
            }
        }
    }
}

UINT16 hw_QARread( volatile UINT16* addr, int field)
{
    return QARread( addr, (FIELD_NAMES)field);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
double g4eStrtod( const char* src, char** end)
{
    // trigger.cfg.sensor.endvaluea[0]=0xa.ff => 10.996094
    // trigger.cfg.sensor.endvaluea[0]=0xa.4  => 10.250000
#undef strtod
    double x = strtod( src, end);
    if ( x == 0.0 && **end == 'X')
    {
        unsigned int y = strtoul(src, end, 16);
        x = double(y);
        if (**end == '.')
        {
            double power = 1.0/16.0;
            // move to the next char after the '.'
            (*end)++;
            while ( **end != '\0' && strchr("0123456789ABCDEF", **end) != NULL)
            {
                double num;
                if (**end >= 'A')
                {
                   num = double((**end - 'A')+10);
                }
                else
                {
                    num = double((**end - '0'));
                }
                x += num*power;
                power /= 16.0;
                (*end)++;
           }
        }
    }
    return x;
}

// extern "C"
}
////////////////////////////////////////////////////////////////////////////////////////////////////
DataFlashMemory DataFlash;

void FlashFileWrite( UINT32* base, UINT32 offset, UINT32 data)
{
    DataFlash.Write( base, offset, data);
}

UINT32 FlashFileRead( UINT32* base, UINT32 offset)
{
    return DataFlash.Read( base, offset);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void* GetFpgaObject()
{
    return &fpga;
}

UINT16 hw_FpgaRead( volatile UINT16* addr)
{
    return fpga.Read( addr);
}

void hw_FpgaWrite( volatile UINT16* addr, UINT16 value)
{
    fpga.Write( addr, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void* GetDioObject()
{
    return &dio;
}
UINT8 hw_DioRead( vuint8* addr)
{
    return dio.Read( addr);
}
void hw_DioWrite(vuint8* addr, UINT8 mask, UINT8 op)
{
    dio.Write(addr, mask, op);
}
void hw_DioSet(vuint8* addr, UINT8 value)
{
    dio.Set( addr, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void* GetDprObject()
{
    return &dpr;
}



