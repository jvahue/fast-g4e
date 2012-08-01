#ifndef HW_MEM_LAYOUT
#define HW_MEM_LAYOUT

#if __cplusplus
extern "C" {
#endif

#include "alt_stdtypes.h"
#include "ResultCodes.h"


#ifdef G4E
#include "mcf5xxx.h"
#include "mcf548x.h"
#endif

#undef EXPORT
#if defined ( HW_EMUL_BODY )
#define EXPORT
#else
#define EXPORT extern
#endif

// WIN32 Main Functions
void WinG4E( void* pParam);
double g4eStrtod( const char* src, char** end);

EXPORT BOOLEAN wdResetRequest;

// SPI

// Data Flash
void FlashFileWrite( UINT32* base, UINT32 offset, UINT32 data);
UINT32 FlashFileRead( UINT32* base, UINT32 offset);

// UART
UINT16 hw_UART_Transmit(UINT8 Port, const INT8* Data, UINT16 Size, UINT16 testMode);
UINT16 hw_UART_Receive(UINT8 Port,  INT8* Data, UINT16 Size);

// FPGA
EXPORT UINT16 hw_FpgaRead( volatile UINT16* addr);
EXPORT void hw_FpgaWrite( volatile UINT16* addr, UINT16 value);

// DIO
EXPORT UINT8 hw_DioRead( volatile UINT8* addr);
EXPORT void hw_DioWrite(volatile UINT8* addr, UINT8 mask, UINT8 op);
EXPORT void hw_DioSet(volatile UINT8* addr, UINT8 value);

// QAR
void   hw_QARwrite( volatile UINT16 * addr, int field, UINT16 data);
UINT16 hw_QARread( volatile UINT16 * addr, int field);

// FPGA
EXPORT unsigned char hw_FPGA[0x4000];
#define FPGA_BASE_ADDR (unsigned int)((const unsigned char*)&hw_FPGA)

// MCF Registers
EXPORT unsigned char hw_MBAR[0x40000];
#define __MBAR ((const unsigned char*)&hw_MBAR)

// DPRAM
EXPORT unsigned char hw_DPRAM[0x4000];
#define DPRAM_BASE_ADDR ((const unsigned char*)&hw_DPRAM)

#if __cplusplus
}
#endif


#endif
