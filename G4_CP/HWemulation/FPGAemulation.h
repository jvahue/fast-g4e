#pragma once

extern "C"
{
#include "arinc429.h"
};

class A429Fifo
{
public:
    A429Fifo(UINT32 maxFifo = 256);

    virtual bool Push( UINT32 value);
    virtual bool Pop( UINT32& value);
    virtual void HandleRegisterStates(bool overflow = false) {};

    // inline functions
    inline bool SetMaxDepth( UINT32 value) {m_maxFifo = value;}
    inline bool IsEmpty() { return m_count == 0;}
    inline bool IsHalfFull() { return m_count >= m_maxFifo/2;}
    inline bool IsFull() { return m_count == m_maxFifo;}

    UINT32 m_data[256];
    UINT32 m_pushAt;
    UINT32 m_popAt;
    UINT32 m_count;
    UINT32 m_maxFifo;
};

class RxFifo : public A429Fifo
{
public:
    virtual bool Push( UINT32 value);
    void SetFpgaRegisters( FPGA_RX_REG_PTR pfpgareg) {m_pfpgaReg = pfpgareg;}
    virtual void HandleRegisterStates(bool overflow = false);

    FPGA_RX_REG_PTR m_pfpgaReg;
};

class FPGAemulation
{
public:
    enum RegisterGroup {
        eGeneral,
        eArincCommon,
        eArincRx,
        eArincTx,
        eQAR,
        eInvalidFpgaAddr
    };

    FPGAemulation();

    void Reset();
    void Process();
    UINT16 Read( volatile UINT16* addr);
    void Write( volatile UINT16* addr, UINT16 value);
    bool MessageFilter( UINT16 chan, UINT32 msg);
    void WriteQarData( int subframe, int words, UINT16* data);
    void QarOff();
    void BarkerError(bool lostSync);
    BYTE A429SpeedMatch(BYTE iocSpeeds);

    RxFifo m_rxFifo[4];
    A429Fifo m_txFifo[2];

private:
    FPGA_RX_REG_PTR m_fpgaRxReg;
    FPGA_TX_REG_PTR m_fpgaTxReg;

    UINT32 m_readBuffer[4];
    UINT32 m_rxTestWrite[4];

    UINT32 m_writeBuffer[2];
    UINT32 m_txTestWrite[2];

    // compute the index to which Rx/Tx register set we are using
    bool GetRegisterSet( volatile UINT16* addr, RegisterGroup& group, UINT16& index);

    UINT16 ReadGeneral( volatile UINT16* addr);
    UINT16 ReadArincRx( volatile UINT16* addr, UINT16 index);
    UINT16 ReadArincTx( volatile UINT16* addr, UINT16 index);
    UINT16 ReadQAR( volatile UINT16* addr);

    void WriteGeneral( volatile UINT16* addr, UINT16 value);
    void WriteArincRx( volatile UINT16* addr, UINT16 index, UINT16 value);
    void WriteArincTx( volatile UINT16* addr, UINT16 index, UINT16 value);
    void WriteQAR( volatile UINT16* addr, UINT16 value);

    void HandleFifoState( A429Fifo& fifo, FPGA_RX_REG_PTR pfpgaRxReg);
    void HandleFifoState( A429Fifo& fifo, FPGA_TX_REG_PTR pfpgaTxReg);

    void Error( TCHAR* msg, ...);
};