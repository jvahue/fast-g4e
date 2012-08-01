#pragma once

#include "mcf5xxx.h"

extern "C"
{
#include "dio.h"
};

class DioEmulation
{
public:
    DioEmulation();
    void Reset();
    void ProcessDio(UINT32& dins, UINT32& douts);

    // These are opposite because we are on the other side
    void SetDio( DIO_INPUT Pin, UINT8 state);
    UINT8 GetDio( DIO_OUTPUT Pin);

    UINT8 Read( vuint8* addr);
    void Write( vuint8* addr, UINT8 mask, UINT8 op, bool wrap=true);
    void Set( vuint8* addr, UINT8 value, bool wrap=true);
    void Wraparounds();
    void BitState(vuint8* addr, UINT8 mask, UINT8 op );
    void SetPinStates( vuint8* addr);

protected:
    UINT32 m_wrapDin;

    static const int ODR_SIZE = 8;
    vuint8* m_ODR[ODR_SIZE];
};

