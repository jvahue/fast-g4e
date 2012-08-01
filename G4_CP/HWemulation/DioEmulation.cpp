#include "stdafx.h"

#include "HW_Emul.h"
#include "DioEmulation.h"

extern "C"
{
#include "alt_basic.h"
#include "mcf548x_gpt.h"
#include "DIO.h"

#define DIO_GPIO_PODR_OFFSET 0x00   //Output data register
#define DIO_GPIO_PDDR_OFFSET 0x10   //Data direction register
#define DIO_GPIO_PPDSDR_OFFSET 0x20 //Input pin read/Output data register write
#define DIO_GPIO_PCLRR_OFFSET 0x30  //Port clear register (write 0 to clear, 1 no effect)

#define DIO_GPT_OUT_HI      3   //For timer GPIO, sets the GPIO field of GMSn
    //register to output high
#define DIO_GPT_OUT_LO      2   //For timer GPIO, sets the GPIO field of GSMn
    //register to output low

extern DIO_CONFIG DIO_OutputPins[DIO_MAX_OUTPUTS];
extern DIO_CONFIG DIO_InputPins[DIO_MAX_INPUTS]; 
};

//--------------------------------------------------------------------------------------------------
DioEmulation::DioEmulation()
    : m_wrapDin(0)
{
    Reset();
}

//--------------------------------------------------------------------------------------------------
void DioEmulation::Reset()
{
    m_wrapDin = 0;
    memset( hw_MBAR, 0, sizeof(hw_MBAR));
    m_ODR[0] = &MCF_GPIO_PODR_FEC1L;
    m_ODR[1] = &MCF_GPIO_PODR_FEC1H;
    m_ODR[2] = &MCF_GPIO_PODR_FEC0L;
    m_ODR[3] = &MCF_GPIO_PODR_FEC0H;
    m_ODR[4] = &MCF_GPIO_PODR_PCIBR;
    m_ODR[5] = &MCF_GPIO_PODR_PCIBG;
    m_ODR[6] = &MCF_GPIO_PODR_PSC1PSC0;
    m_ODR[7] = &MCF_GPIO_PODR_PSC3PSC2;

    for (int i=0; i < ODR_SIZE; ++i)
    {
      *m_ODR[i] = 0xFF;
    }
      
    SetDio( HWPFEN_Sta,      1);
    SetDio( LSS_OvI,         0);
    SetDio( SW_PFEN_Sta,     1);

    SetDio( WOW,             0);
    SetDio( WLAN_WOW_Enb,    0);

    Wraparounds();
}

//--------------------------------------------------------------------------------------------------
void DioEmulation::ProcessDio(UINT32& dins, UINT32& douts)
{
#define DOUTM 0x7AAFFE7F
    static UINT32 lastDout = 0;

    // Unpack the real DINs to the G4 memory map, (i.e., not wraparounds)
    for (int i = 0; i < LSS0_WA; ++i)
    {
        SetDio( (DIO_INPUT)i, BIT( dins, i));
    }

    // a few other we can control externally
    SetDio( LSS_OvI,     BIT( dins, LSS_OvI));
    SetDio( WOW,         BIT( dins, WOW));
    SetDio( SW_PFEN_Sta, BIT( dins, SW_PFEN_Sta));

    // give back the wraparounds to the UI
    dins |= m_wrapDin;

    // Get the DOUT values from the G4 memory map
    UINT32 temp = 0;
    for ( int i=0; i < DIO_MAX_OUTPUTS; ++i)
    {
        if ( GetDio((DIO_OUTPUT)i))
        {
            temp |= FIELD(1, i, 1);
        }
    }

    if ((temp & DOUTM) != lastDout)
    {
        int x = (temp  & DOUTM) ^ lastDout;
        //TRACE( "0x%08X - %d\n", x, clock());
    }

    lastDout = temp & DOUTM;

    douts = temp;
}

//--------------------------------------------------------------------------------------------------
void DioEmulation::Wraparounds()
{
    // the following outputs are wrapped around as inputs
    struct WrapInfo {
        vuint8* src;
        UINT32  srcBit;
        DIO_INPUT dinId;
        vuint8* dest;
        UINT32  destBit;
        UINT8   invert;
    };

    const WrapInfo wrapFast[] = {
        // FAST DOUT Wraparounds
        { &MCF_GPIO_PODR_FEC0L, 0x10,    // LSS0
          LSS0_WA, &MCF_GPIO_PODR_FEC0L, 0x01, 0},
        { &MCF_GPIO_PODR_FEC0L, 0x20,    // LSS1
          LSS1_WA, &MCF_GPIO_PODR_FEC0L, 0x02, 0},
        { &MCF_GPIO_PODR_FEC0L, 0x40,    // LSS2
          LSS2_WA, &MCF_GPIO_PODR_FEC0L, 0x04, 0},
        { &MCF_GPIO_PODR_FEC0L, 0x80,    // LSS3
          LSS3_WA, &MCF_GPIO_PODR_FEC0L, 0x08, 0},
        // TBD: What else is a wraparound???
        { 0, 0xFFFF, DIO_MAX_INPUTS, 0,  0xFFFF, 0} // Terminate the table
    };

    for ( int i = 0; wrapFast[i].src != 0 && wrapFast[i].dest != 0; ++i)
    {
        if (*wrapFast[i].src & wrapFast[i].srcBit)
        {
            Write( wrapFast[i].dest, wrapFast[i].destBit, 1 ^ wrapFast[i].invert, false);
        }
        else
        {
            Write( wrapFast[i].dest, wrapFast[i].destBit, 0 ^ wrapFast[i].invert, false);
        }
    }

    // always set FGPA Cfg = True
    Write( &MCF_GPIO_PODR_FEC1H, 0x10, 1, false);
    // always set FGPA Err = False
    Write( &MCF_GPIO_PODR_FEC1H, 0x08, 0, false);
}

//--------------------------------------------------------------------------------------------------
//Used by G4E UI
void DioEmulation::SetDio( DIO_INPUT Pin, UINT8 state)
{
    if(DIO_InputPins[Pin].Peripheral == DIO_TMR2)
    {
        UINT32 value = state ? 
            (MCF_GPT_GMS_TMS_GPIO | MCF_GPT_GMS_GPIO(DIO_GPT_OUT_HI)) : 
            (MCF_GPT_GMS_TMS_GPIO | MCF_GPT_GMS_GPIO(DIO_GPT_OUT_LO));

        MCF_GPT_GMS2 = value;
    }
    else if(DIO_InputPins[Pin].Peripheral == DIO_GPIO)
    {
        DIO_W((DIO_InputPins[Pin].DataReg+DIO_GPIO_PODR_OFFSET), 
              DIO_InputPins[Pin].PinMask,
              state);
    }
    else if(DIO_InputPins[Pin].Peripheral == DIO_FPGA)
    {
        DIO_W( DIO_InputPins[Pin].DataReg, DIO_InputPins[Pin].PinMask, state);
    }
}

//--------------------------------------------------------------------------------------------------
//Used by G4E UI
UINT8 DioEmulation::GetDio( DIO_OUTPUT Pin)
{
    if (DIO_OutputPins[Pin].DataReg != NULL)
    {
        return (*(DIO_OutputPins[Pin].DataReg+DIO_GPIO_PODR_OFFSET)) & DIO_OutputPins[Pin].PinMask;
    }
    else
    {
        return 0;
    }
}


//--------------------------------------------------------------------------------------------------
UINT8 DioEmulation::Read(vuint8* addr)
{
    UINT8 value = *addr;
    return value;
}

//--------------------------------------------------------------------------------------------------
void DioEmulation::Write(vuint8* addr, UINT8 mask, UINT8 op, bool wrap)
{
    if (addr >= &MCF_GPIO_PODR_FBCTL && addr <= &MCF_GPIO_PODR_DSPI)
    {
        BitState( addr, mask, op);
        SetPinStates( addr);
    }
    else if (addr >= &MCF_GPIO_PDDR_FBCTL && addr <= &MCF_GPIO_PDDR_DSPI)
    {
        BitState( addr, mask, op);
        wrap = false; // not needed here
    }
    else if (addr >= &MCF_GPIO_PPDSDR_FBCTL && addr <= &MCF_GPIO_PPDSDR_DSPI)
    {
        // this down and see if we can set this bit
        Set( addr, mask, wrap);
        wrap = false; // already done by Set
    }
    else if (addr >= &MCF_GPIO_PCLRR_FBCTL && addr <= &MCF_GPIO_PCLRR_DSPI)
    {
        // this down and see if we can clear this bit
        Set( addr, ~mask, wrap);
        wrap = false; // already done by Set
    }
    else // other memory locations just use our Set function to toggle bits
    {
        // this down and see if we can clear this bit
        BitState( addr, mask, op);
        wrap = false; // already done by Set
    }

    if (wrap)
    {
        Wraparounds();
    }
}

//--------------------------------------------------------------------------------------------------
void DioEmulation::BitState(vuint8* addr, UINT8 mask, UINT8 op )
{
    if ( op == 0)
    {
        *addr = *addr & ~mask;
    }
    else
    {
        *addr = *addr | mask;
    }
}

//--------------------------------------------------------------------------------------------------
void DioEmulation::SetPinStates( vuint8* addr)
{
    vuint8* regPtr;
    vuint8* ddr = addr + DIO_GPIO_PDDR_OFFSET;
    UINT8 ipp;
    UINT8 opp;

    ipp = (*addr | ~(*ddr));
    opp = (*addr & *ddr);

    // keep the CLRR and DSDR in sync
    regPtr = addr + DIO_GPIO_PPDSDR_OFFSET;
    *regPtr = *addr;
    regPtr = addr + DIO_GPIO_PCLRR_OFFSET;
    *regPtr = *addr;
}

//--------------------------------------------------------------------------------------------------
// Set write directly to the register in question with the following conditions
// ODR only writes to output bits based on DDR
// DSDR only sets output bits based on DDR
// CLR only clears output bits based on DDR
void DioEmulation::Set(vuint8* addr, UINT8 value, bool wrap)
{
    vuint8* regPtr;

    if (addr >= &MCF_GPIO_PODR_FBCTL && addr <= &MCF_GPIO_PODR_DSPI)
    {
        // turn off any input bits in the value
        *addr = value & *(addr+DIO_GPIO_PDDR_OFFSET);
        SetPinStates(addr);
    }
    else if (addr >= &MCF_GPIO_PDDR_FBCTL && addr <= &MCF_GPIO_PDDR_DSPI)
    {
        *addr = value;
    }
    else if (addr >= &MCF_GPIO_PPDSDR_FBCTL && addr <= &MCF_GPIO_PPDSDR_DSPI)
    {
        // convert to ODR base address
        regPtr = addr - DIO_GPIO_PPDSDR_OFFSET;

        // Can Only SET pins value must be non-zero after being ANDed with DDR (1=o/p)
        value = value & *(regPtr+DIO_GPIO_PDDR_OFFSET);
        if (value)
        {
            *regPtr = *regPtr | value;
            SetPinStates( regPtr);
        }
    }
    else if (addr >= &MCF_GPIO_PCLRR_FBCTL && addr <= &MCF_GPIO_PCLRR_DSPI)
    {
        // convert to ODR base address
        regPtr = addr - DIO_GPIO_PCLRR_OFFSET;

        // only turn off what we are allowed to turn off
        value = ~(~value & *(regPtr+DIO_GPIO_PDDR_OFFSET));

        // value must be non-zero after being ANDed with DDR (1=o/p)
        if ( value)
        {
            *regPtr = *regPtr & value;
            SetPinStates( regPtr);
        }
    }

    if (wrap)
    {
        Wraparounds();
    }
}

