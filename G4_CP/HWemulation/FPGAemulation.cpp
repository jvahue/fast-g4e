#include "stdafx.h"

#include "HW_Emul.h"
#include "FPGAemulation.h"
#include "DioEmulation.h"

extern "C"
{
    #include "alt_basic.h"
    #include "FPGA.h"
    #include "QAR.h"

    // Make connection to these externally required items
    extern void FPGA_EP6_ISR(void);
    extern const FPGA_RX_REG fpgaRxReg[FPGA_MAX_RX_CHAN]; 
    //extern const FPGA_TX_REG fpgaTxReg_v9[FPGA_MAX_TX_CHAN];


};

#define FPGA_OFFSET ((UINT32)addr - (UINT32)&hw_FPGA)

////////////////////////////////////////////////////////////////////////////////////////////////////
// 32 Bit Wide by 256 deep A429FIFO
////////////////////////////////////////////////////////////////////////////////////////////////////
A429Fifo::A429Fifo(UINT32 maxFifo)
    : m_pushAt(0)
    , m_popAt(0)
    , m_count(0)
    , m_maxFifo(maxFifo)
{
    memset( &m_data, 0, sizeof( m_data));
}

bool A429Fifo::Push( UINT32 value)
{
    bool overflow = false;
    bool status = false;
    if (m_count < m_maxFifo)
    {
        m_data[m_pushAt] = value;
        ++m_count;
        ++m_pushAt;
        if (m_pushAt >= m_maxFifo)
        {
            m_pushAt = 0;
        }
        status = true;
    }
    else
    {
        overflow = true;
    }


    HandleRegisterStates(overflow);

    return status;
}

bool A429Fifo::Pop( UINT32& value)
{
    bool status = false;
    if (m_count > 0)
    {
        value = m_data[m_popAt];
        --m_count;
        ++m_popAt;
        if (m_popAt >= m_maxFifo)
        {
            m_popAt = 0;
        }
        status = true;
    }
    HandleRegisterStates();

    return status;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// FPGA Rx FIFO Emulation
// Specialized for isr_mask and IMR1 interrupt position
////////////////////////////////////////////////////////////////////////////////////////////////////
bool RxFifo::Push( UINT32 value)
{
    *m_pfpgaReg->Status |= RSR_429_DATA_PRESENT;
    return A429Fifo::Push( value);
}

void RxFifo::HandleRegisterStates( bool overflow)
{
    UINT16 imaskLsb = (((m_pfpgaReg->isr_mask - 8)/2) * 3) + 4;
    UINT16 enabledInt = ~((*FPGA_IMR1 >> imaskLsb) & 7);

    // update status based on Cfg
    *m_pfpgaReg->Status |= *m_pfpgaReg->Configuration & 3;

    // check for half and full states
    if ( overflow)
    {
        *m_pfpgaReg->FIFO_Status |= RFSR_429_FIFO_OVERRUN;
        if (enabledInt & 4)
        {
            *FPGA_ISR = ISR_429_RX_FULL_VAL << m_pfpgaReg->isr_mask;
            FPGA_EP6_ISR();
        }
    }
    else
    {
        *m_pfpgaReg->FIFO_Status &= ~RFSR_429_FIFO_OVERRUN;
        if ( IsFull())
        {
            *m_pfpgaReg->FIFO_Status &= ~RFSR_429_FIFO_HALF_FULL;
            if ( !(*m_pfpgaReg->FIFO_Status & RFSR_429_FIFO_FULL))
            {
                *m_pfpgaReg->FIFO_Status |= RFSR_429_FIFO_FULL;
                if (enabledInt & 4)
                {
                    *FPGA_ISR = ISR_429_RX_FULL_VAL << m_pfpgaReg->isr_mask;
                    FPGA_EP6_ISR();
                }
            }
        }
        else if ( IsHalfFull())
        {
            // turn off the full
            *m_pfpgaReg->FIFO_Status &= ~RFSR_429_FIFO_FULL;

            if ( !(*m_pfpgaReg->FIFO_Status & RFSR_429_FIFO_HALF_FULL))
            {
                *m_pfpgaReg->FIFO_Status |= RFSR_429_FIFO_HALF_FULL;
                if (enabledInt & 2)
                {
                    *FPGA_ISR = ISR_429_RX_HALF_FULL_VAL << m_pfpgaReg->isr_mask;
                    FPGA_EP6_ISR();
                }
            }
        }
        else if ( IsEmpty())
        {
            *m_pfpgaReg->FIFO_Status = RFSR_429_FIFO_EMPTY;
        }
        else
        {
            *m_pfpgaReg->FIFO_Status &= ~RFSR_429_FIFO_HALF_FULL;
            *m_pfpgaReg->FIFO_Status &= ~RFSR_429_FIFO_FULL;

            if ( *m_pfpgaReg->FIFO_Status & RFSR_429_FIFO_EMPTY)
            {
                // if the FIFO was empty it's not anymore
                *m_pfpgaReg->FIFO_Status &= ~RFSR_429_FIFO_EMPTY;
                if ( enabledInt & 1)
                {
                    *FPGA_ISR = ISR_429_RX_NOT_EMPTY_VAL << m_pfpgaReg->isr_mask;
                    FPGA_EP6_ISR();
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// FPGA Emulator
////////////////////////////////////////////////////////////////////////////////////////////////////
FPGAemulation::FPGAemulation()
{
    m_fpgaRxReg = (FPGA_RX_REG_PTR)&fpgaRxReg[0];
    m_fpgaTxReg = (FPGA_TX_REG_PTR)&fpgaTxReg_v12[0];

    for ( int i=0; i < FPGA_MAX_RX_CHAN; ++i)
    {
        m_rxFifo[i].SetFpgaRegisters( &m_fpgaRxReg[i]);
    }
}

//--------------------------------------------------------------------------------------------------
void FPGAemulation::Reset()
{
#define XLAT(x) (BYTE*)((&x)+0x20)

    DioEmulation* dio = (DioEmulation*)GetDioObject();

    memset(hw_FPGA, 0, sizeof( hw_FPGA));

    // looks like this inverts every other read
    *FPGA_VER = FPGA_MAJOR_VERSION_COMPATIBILITY; // set the version number the S/w expects

    *FPGA_429_TX1_STATUS = 1;                     // set parity select
    *FPGA_429_TX2_STATUS = 1;                     // set parity select

    *FPGA_429_RX1_CONFIG = 1;                     // set parity select
    *FPGA_429_RX2_CONFIG = 1;                     // set parity select
    *FPGA_429_RX3_CONFIG = 1;                     // set parity select
    *FPGA_429_RX4_CONFIG = 1;                     // set parity select

    *FPGA_429_TX1_CONFIG = 4;                     // Label Filter bits flipped
    *FPGA_429_TX2_CONFIG = 4;                     // Label Filter bits flipped

    // Set all the FIO empty bits
    *FPGA_429_RX1_FIFO_STATUS = RFSR_429_FIFO_EMPTY;
    *FPGA_429_RX2_FIFO_STATUS = RFSR_429_FIFO_EMPTY;
    *FPGA_429_RX3_FIFO_STATUS = RFSR_429_FIFO_EMPTY;
    *FPGA_429_RX4_FIFO_STATUS = RFSR_429_FIFO_EMPTY;

    *FPGA_429_TX1_FIFO_STATUS = TSR_429_FIFO_EMPTY;
    *FPGA_429_TX2_FIFO_STATUS = TSR_429_FIFO_EMPTY;

    // 
    //UINT32* x = (UINT32*)&MCF_INTC_IMRL;
    MCF_INTC_IMRL = 0xFFFFFFFF;

    // Initialize the FPGA   
    dio->SetDio( FPGA_Err, 0);
    dio->SetDio( FPGA_Cfg, 1);
    dio->SetDio( FFD_Enb, 1);
}

//--------------------------------------------------------------------------------------------------
// Determine where in the FPGA we are trying to access
bool FPGAemulation::GetRegisterSet( volatile UINT16* addr, RegisterGroup& group, UINT16& index)
{
    bool status = false;
    group = eInvalidFpgaAddr;
    index = 0;

    if (addr >= FPGA_ISR && addr < (UINT16*)(&hw_FPGA + sizeof(hw_FPGA))) 
    {
        if ( MEMBER( addr, FPGA_429_COMMON_CONTROL, FPGA_429_LOOPBACK_CONTROL))
        {
            group = eArincCommon;
            status = true;
        }
        else if ( MEMBER( addr, FPGA_429_RX1_STATUS, FPGA_429_RX4_MSW_TEST))
        {
            for (int i = 0; i < FPGA_MAX_RX_CHAN; ++i) 
            {
                if ( addr <= m_fpgaRxReg[i].msw_test) 
                {
                    group = eArincRx;
                    index = (UINT16)i;
                    break;
                }
            }
            status = true;
        }
        else if ( MEMBER( addr, FPGA_429_TX1_STATUS, FPGA_429_TX1_MSW))
        {
            group = eArincTx;
            status = true;
        }
        else if ( MEMBER( addr, FPGA_429_TX2_STATUS, FPGA_429_TX2_MSW))
        {
            group = eArincTx;
            index = 1;
            status = true;
       }
        else if ( MEMBER( addr, FPGA_QAR_CONTROL, FPGA_QAR_BARKER4))
        {
            group = eQAR;
            status = true;
        }
        else
        {
            group = eGeneral;
            status = true;
        }
    }

    return status;
}

//--------------------------------------------------------------------------------------------------
void FPGAemulation::Process()
{
    FPGA_RX_REG_PTR pfpgaRxReg = m_fpgaRxReg;
    UINT8 intEn = MCF_EPORT_EPIER & MCF_EPORT_EPIER_EPIE6;
    UINT32 imrl = MCF_INTC_IMRL & (MCF_INTC_IMRL_INT_MASK6|MCF_INTC_IMRL_MASKALL);
    UINT16 imr2 = *FPGA_IMR2;

    // Interrupts for testing during FPGA_Initialize
    if ( intEn && (imrl == 0) && (imr2 & IMR2_INT_TEST))
    {
        // see if we are doing a FIFO test
        if (m_FPGA_Status.nIntTest == FPGA_TEST_INT_SET)
        {
            // trigger the FPGA interrupt
            *FPGA_ISR = ISR_INT_TEST;
            FPGA_EP6_ISR();
        }
    }

    // make it look like we have some data
    for ( int i=0; i < FPGA_MAX_RX_CHAN; ++i)
    {
        // BUG: should we update FIFO status constantly?
        m_rxFifo[i].HandleRegisterStates();
    }

    // Empty the Tx FIFOs as they must be busy transmitting ... 
    // Remove X words every every call based on setup speed, and check for loopback testing
    for (int i=0; i < FPGA_MAX_TX_CHAN; ++i)
    {
        // If TxEnabled
        if ( *m_fpgaTxReg[i].Configuration & TSR_429_CHAN_ENABLE)
        {
            // Get the wordsPerFrame for the selected Tx speed
            UINT16 wordsPer10Ms = (*FPGA_429_COMMON_CONTROL & (1 << (i+4))) ? 31 : 4;
            while ( wordsPer10Ms > 0 && !m_txFifo[i].IsEmpty())
            {
                UINT32 dummy;
                --wordsPer10Ms;
                m_txFifo[i].Pop(dummy);

                // check for wraparound testing
                for (int r=0; r < FPGA_MAX_RX_CHAN; ++r)
                {   
                    UINT16 loopback = (*FPGA_429_LOOPBACK_CONTROL >> (1 + (r*2)) & 3);
                    if ( loopback == 2 && i == 1)
                    {
                        m_rxFifo[r].Push( dummy);
                    }
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//--- READ OPERATIONS ------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////////////////
UINT16 FPGAemulation::Read( volatile UINT16* addr)
{
    RegisterGroup group;
    UINT16 index;

    UINT16 value = 0;

    if ( GetRegisterSet( addr, group, index))
    {
        switch (group)
        {
        case eGeneral:
            value = ReadGeneral( addr);
            break;
        case eArincCommon:
            value = *addr;
            //Error( "Attempted read of Write only FPGA 0x%04x", FPGA_OFFSET);
            break;
        case eArincRx:
            value = ReadArincRx( addr, index);
            break;
        case eArincTx:
            value = ReadArincTx( addr, index);
            break;
        case eQAR:
            value = ReadQAR( addr);
            break;
        }
    }
    return value;
}

//--------------------------------------------------------------------------------------------------
// Handle reading the general registers
UINT16 FPGAemulation::ReadGeneral( volatile UINT16* addr)
{
    static int verReadOne = 0;

    UINT16 value = 0;

    if (addr == FPGA_ISR)
    {
        value = *addr;
        *FPGA_IMR2 &= 0x7FFF; // clear the IRQ Test bit on ISR read
        *addr = 0;            // clear the ISR register
    }
    else if (addr == FPGA_VER)
    {
        if (verReadOne == 0)
        {
            value = FPGA_MAJOR_VERSION_COMPATIBILITY;
        }
        else
        {
            value = (UINT16)(~FPGA_MAJOR_VERSION_COMPATIBILITY);
        }
        verReadOne ^= 1;
    }
    else
    {
        value = *addr;
    }

    return value;
}

//--------------------------------------------------------------------------------------------------
// Handle reading the general registers
UINT16 FPGAemulation::ReadArincRx( volatile UINT16* addr, UINT16 index)
{
    UINT16 value = 0;

    if ( addr == m_fpgaRxReg[index].Status)
    {
        value = *addr;
        *addr = value & 0xF; // turn off A429 Data Present BUG any others?
    }
    else if ( addr == m_fpgaRxReg[index].FIFO_Status)
    {
        value = *addr;
    }
    else if ( addr == m_fpgaRxReg[index].lsw)
    {
        if ( m_rxFifo[index].Pop( m_readBuffer[index]))
        {
            value = m_readBuffer[index] & 0xFFFF;
        }
    }
    else if ( addr == m_fpgaRxReg[index].msw)
    {
        value = (m_readBuffer[index] >> 16) & 0xFFFF;
    }
    else
    {
        value = *addr;
    }

    return value;
}

//--------------------------------------------------------------------------------------------------
// Handle reading the general registers
UINT16 FPGAemulation::ReadArincTx( volatile UINT16* addr, UINT16 index)
{
    UINT16 value = 0;

    value = *addr;

    return value;
}

//--------------------------------------------------------------------------------------------------
UINT16 FPGAemulation::ReadQAR( volatile UINT16* addr)
{
    UINT16 value = 0;
    value = *addr;
    return value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//--- WRITE OPERATIONS -----------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////////////////
void FPGAemulation::Write( volatile UINT16* addr, UINT16 value)
{
    RegisterGroup group;
    UINT16 index = 0;

    if ( GetRegisterSet(addr, group, index))
    {
        switch (group)
        {
        case eGeneral:
            WriteGeneral( addr, value);
            break;
        case eArincCommon:
            *addr = value;
            break;
        case eArincRx:
            WriteArincRx( addr, index, value);
            break;
        case eArincTx:
            WriteArincTx( addr, index, value);
            break;
        case eQAR:
            WriteQAR( addr, value);
            break;
        }
    }
}

//--------------------------------------------------------------------------------------------------
void FPGAemulation::WriteGeneral( volatile UINT16* addr, UINT16 value)
{
    if ( addr == FPGA_IMR2)
    {
        UINT8 intEn = MCF_EPORT_EPIER & MCF_EPORT_EPIER_EPIE6;
        UINT32 imrl = MCF_INTC_IMRL & (MCF_INTC_IMRL_INT_MASK6|MCF_INTC_IMRL_MASKALL);

        // Interrupts for testing during FPGA_Initialize
        if ( intEn && (imrl == 0) && (value & IMR2_INT_TEST))
        {
            // trigger the FPGA interrupt
            *FPGA_ISR = ISR_INT_TEST;
            FPGA_EP6_ISR();
        }
    }
    else
    {
        *addr = value;
    }
}

//--------------------------------------------------------------------------------------------------
void FPGAemulation::WriteArincRx( volatile UINT16* addr, UINT16 index, UINT16 value)
{
    if ( addr == m_fpgaRxReg[index].Configuration)
    {
        *addr = value;
        // echo some status over to the status register
        *m_fpgaRxReg[index].Status = value & 3;  // set parity and enable bits
    }
    else if ( addr == m_fpgaRxReg[index].lsw_test && 
              !(*m_fpgaRxReg[index].Configuration & RCR_429_ENABLE))
    {
        m_rxTestWrite[index] = value;
    }
    else if ( addr == m_fpgaRxReg[index].msw_test && 
              !(*m_fpgaRxReg[index].Configuration & RCR_429_ENABLE))
    {
        m_rxTestWrite[index] |= (value << 16);
        m_rxFifo[index].Push( m_rxTestWrite[index]);
    }
    else
    {
        Error( "Attempted Write of Read Only FPGA 0x%04x", FPGA_OFFSET);
    }
}

//--------------------------------------------------------------------------------------------------
void FPGAemulation::WriteArincTx( volatile UINT16* addr, UINT16 index, UINT16 value)
{
    if ( addr == m_fpgaTxReg[index].Configuration)
    {
        *addr = value;
        // send some cfg bits to the status register
        *m_fpgaTxReg[index].Status = value & 3; // copy parity and enable bits
    }
    else if ( addr == m_fpgaTxReg[index].lsw_test && 
              !(*m_fpgaTxReg[index].Configuration & RCR_429_ENABLE))
    {
        m_txTestWrite[index] = value;
    }
    else if ( addr == m_fpgaTxReg[index].msw_test && 
              !(*m_fpgaTxReg[index].Configuration & RCR_429_ENABLE))
    {
        m_txTestWrite[index] |= (value << 16);
        m_txFifo[index].Push( m_txTestWrite[index]);
    }
    else if ( addr == m_fpgaTxReg[index].lsw)
    {
        m_writeBuffer[index] = value;
    }
    else if ( addr == m_fpgaTxReg[index].msw)
    {
        m_writeBuffer[index] |= value << 16;
        m_txFifo[index].Push( m_writeBuffer[index]);
    }
    else
    {
        Error( "Attempted Write of Read Only FPGA 0x%04x", FPGA_OFFSET);
    }

}

//--------------------------------------------------------------------------------------------------
void FPGAemulation::WriteQAR( volatile UINT16* addr, UINT16 value)
{
    *addr = value;
}

//--------------------------------------------------------------------------------------------------
void FPGAemulation::QarOff()
{
    QAR_W( FPGA_QAR_STATUS, S_DATA_PRESENT, 1);
    QAR_W( FPGA_QAR_STATUS, S_FRAME_VALID,  0);
    QAR_W( FPGA_QAR_STATUS, S_LOST_SYNC,    1);
}

//--------------------------------------------------------------------------------------------------
void FPGAemulation::BarkerError(bool lostSync)
{
    QAR_W( FPGA_QAR_STATUS, S_BARKER_ERR, 1);
    if ( lostSync)
    {
      QAR_W( FPGA_QAR_STATUS, S_LOST_SYNC,  1);
    }
}

//--------------------------------------------------------------------------------------------------
// check for a429 speed match between the IOC and the UUT
// 1 in IOC means hi speed, 0 = lo
BYTE FPGAemulation::A429SpeedMatch(BYTE iocSpeeds)
{
    UINT16 rxStatus;

    BYTE match = 0;
    UINT16 ccr = *FPGA_429_COMMON_CONTROL;

    volatile UINT16* a429RxStatus[4] = {
        FPGA_429_RX1_STATUS,
        FPGA_429_RX2_STATUS,
        FPGA_429_RX3_STATUS,
        FPGA_429_RX4_STATUS
    };

    for (int i = 0; i < 4; ++i)
    {
        rxStatus = *a429RxStatus[i];

        // channel enabled
        if ( BIT(rxStatus, 1) == 1 )
        {
            if ( BIT(iocSpeeds, i) == BIT(ccr,i))
            {
                // clear framing error
                rxStatus &= ~RSR_429_FRAMING_ERROR;
                *a429RxStatus[i] = rxStatus;
                match |= 1 << i;
            }
            else
            {
                // set framing error
                rxStatus |= RSR_429_FRAMING_ERROR;
                *a429RxStatus[i] = rxStatus;
            }
        }
    }

    return match;
}

//--------------------------------------------------------------------------------------------------
// SF 0-3
void FPGAemulation::WriteQarData( int subframe, int words, UINT16* data)
{
    void* dest;
    // verify word count equals our expected word count
    UINT16 qarWords = QAR_R( FPGA_QAR_CONTROL, C_NUM_WORDS);

    // make Frame sync come back on SF1
    if ( QAR_R(FPGA_QAR_STATUS, S_LOST_SYNC) == 1 && subframe != 0)
    {
        // it's a hack ... but I'm not above that
        // .. this keeps us from doing anything until we see SF1
        qarWords = 0;
    }

    if ( qarWords >= 1 && qarWords <= 5 && words == (1 << (qarWords+5)))
    {
        // copy the words
        if (subframe == 0 || subframe == 2)
        {
            dest = &hw_FPGA[0x900];
        }
        else
        {
            dest = &hw_FPGA[0x1100];
        }
        memcpy( dest, data, words*2);

        // set the subframe indication in the status register (+1)
        if (++subframe == 4)
        {
            subframe = 0;
        }

        QAR_W( FPGA_QAR_STATUS, S_SUBFRAME,     (UINT16)subframe);
        //TRACE("SF:%d\n", subframe);
        QAR_W( FPGA_QAR_STATUS, S_FRAME_VALID,  1);
        QAR_W( FPGA_QAR_STATUS, S_LOST_SYNC,    0);
        QAR_W( FPGA_QAR_STATUS, S_DATA_PRESENT, 0);
        QAR_W( FPGA_QAR_STATUS, S_BARKER_ERR,   0);

        // set the FPGA interrupt
        *FPGA_ISR |= 1 << 3;
        FPGA_EP6_ISR();
    }
}

//--------------------------------------------------------------------------------------------------
void FPGAemulation::HandleFifoState( A429Fifo& fifo, FPGA_TX_REG_PTR pfpgaReg)
{
    if ( fifo.IsHalfFull())
    {
        *pfpgaReg->FIFO_Status |= RFSR_429_FIFO_HALF_FULL;
        *FPGA_ISR = ISR_429_RX_HALF_FULL_VAL << pfpgaReg->isr_mask;
        FPGA_EP6_ISR();
    }
}

//--------------------------------------------------------------------------------------------------
// filter raw 429 messages
bool FPGAemulation::MessageFilter( UINT16 chan, UINT32 msg)
{
    UINT16 filterOffset[4] = { 
      0x100, 0x300,
      0x500, 0x700
    };

    int lbl = (msg & 0xFF) << 1;
    UINT16 filter = hw_FPGA[filterOffset[chan] + lbl];
   
    return filter != 0;
}

//--------------------------------------------------------------------------------------------------
// Handle reading the general registers
void FPGAemulation::Error( TCHAR* fmt, ...)
{
    TCHAR newFmt[1024];
    TCHAR buffer[1024];

    sprintf( newFmt, "\r\n*** G4E FPGA Error\r\n%s\r\n", fmt);

    va_list args;
    va_start( args, newFmt );
    vsprintf_s( buffer, 1024, newFmt, args );

    hw_UART_Transmit( 0, (INT8*)buffer, (UINT16)strlen( buffer), 0);
}

