#ifndef HW_EMUL_H
#define HW_EMUL_H

//#if __cplusplus
//extern "C" {
//#endif

#include "alt_stdtypes.h"
#include "ResultCodes.h"


#ifdef G4E
#include "mcf5xxx.h"
#include "mcf548x.h"
#endif

#define GSE_TX_SIZE 1000
#define GSE_RX_SIZE 200
#define MAX_UUT_SENSORS 256
#define SHUTDOWN 0x7EFF

// QAR Status Words
#define QAR_DP_SF        0xC0
#define QAR_DATA_PRESENT 0x80
#define QAR_NEW_SF       0x40
#define BARKER_ERR       0x20
#define NO_FRAME_SYNC    0x10

////////////////////////////////////////////////////////////////////////////////////////////////////
template <class T>
struct CircularBuffer
{
    enum CBconst { eSize = 100};
    T buffer[eSize];
    int writeCount;
    int readCount;
    int readFrom;
    int writeTo;
    int maxDepth;
    bool writeActive;
    bool readActive;

    CString name;

public:
    CircularBuffer();
    void SetName(CString aName) {name = aName;}
    void Write( const T& data);
    bool Read( T& data);
    void Reset();
    void Counts(int& wCount, int& rCount, int& wX, int& rX, int& md)
    { 
      wCount = writeCount;
      rCount = readCount;
      wX = writeTo;
      rX = readFrom;
      md = maxDepth;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// IOC to G4E Message and G4UI to G4E
typedef struct {
    // Data
    FLOAT32 busV;                      // Bus voltage
    FLOAT32 batV;                      // Battery voltage
    UINT16  sensorCount;               // Count of 429 sensors
    UINT8   a429Speeds;                // indicate the rate the bus is being TXed 1 = Hi, 0 = Lo
    BYTE    chan[MAX_UUT_SENSORS];     // channel info (i.e., label)
    UINT32  sensors[MAX_UUT_SENSORS];  // raw data (e.g., this holds the raw A429 word)
    BYTE    QARstatus;                 // Status (see QAR Status Words defines above)
    UINT16  QARwords;                  // QAR words
    UINT16  QAR[1024];                 // QAR data
    UINT32  din;                       // packed din word use DIO_INPUT to unpack
    int     gseRxSize;                 // number of bytes in gseRx
    int     sp0RxSize;                 // number of bytes in gseRx
    int     sp1RxSize;                 // number of bytes in gseRx
    int     sp2RxSize;                 // number of bytes in gseRx
    UINT32  hb;
    char    gseRx[GSE_RX_SIZE];        // GSE Rx Command
    char    sp0Rx[GSE_RX_SIZE];        // GSE Rx Command
    char    sp1Rx[GSE_RX_SIZE];        // GSE Rx Command
    char    sp2Rx[GSE_RX_SIZE];        // GSE Rx Command
} G4I_Data;

////////////////////////////////////////////////////////////////////////////////////////////////////
// G4E to IOC Message and G4E to G4UI
typedef struct {
    // Data
    UINT32 dout;               // packed din word use DIO_INPUT to unpack
    UINT32 gseTxSize;          // number of bytes
    UINT32 sp0TxSize;          // number of bytes
    UINT32 sp1TxSize;          // number of bytes
    UINT32 sp2TxSize;          // number of bytes
    UINT32 hb;
    UINT32 wdResetRequest;     // G4E is waiting for the WD to reset it
    char   gseTx[GSE_TX_SIZE]; // GSE Tx Data
    char   sp0Tx[GSE_TX_SIZE]; // GSE Tx Data
    char   sp1Tx[GSE_TX_SIZE]; // GSE Tx Data
    char   sp2Tx[GSE_TX_SIZE]; // GSE Tx Data
} G4O_Data;

// Create a specific type for each buffer so we can specialize it
typedef CircularBuffer<G4I_Data> BUF_G4I;
typedef CircularBuffer<G4O_Data> BUF_G4O;

////////////////////////////////////////////////////////////////////////////////////////////////////
// G4UI Interface to G4E Code
typedef struct {
    // thread run and frame control
    BYTE runG4thread;
    BYTE holdOnConnection;
    BYTE g4eInitComplete;
    BYTE shutDownComplete;
    UINT32 runSec;

    // Interrupt Requests
    UINT32 irqId;
    UINT32 irqRqst;
    UINT32 irqHandled;

    // misc controls
    INT32 timerDF;  // Do timing for the DFlash in the UI, DF sets a duration UI counts down

    float busV;
    float batV;

    BUF_G4I g4i;
    BUF_G4O g4o;
} SYS_CTRL;

//==================================================================================================
struct SpFifo
{
    enum FifoConstants { eFifoSize = 3*128*1024};
    int size;
    char* buffer;
    int write;
    int read;
    int writeCount;
    int readCount;

    //-----------------------------------------------------
    SpFifo() {
        size = eFifoSize;
        buffer = new char[size];

        Reset();
    }

    //-----------------------------------------------------
    void Reset() {
        memset(buffer, 0, eFifoSize);
        write = read = 0;
        writeCount = readCount = 0;
    }

    //-----------------------------------------------------
    void Push( const char* data, int size) {
        char* src = (char*)data;
        int  left = eFifoSize - ( writeCount % eFifoSize);

        // check for wrap around 
        if ( left < size)
        {
            memcpy( &buffer[write], src, left);
            write = 0;
            writeCount += left;
            size -= left;
            src = &src[left];
        }

        // copy data or remaining
        memcpy( &buffer[write], src, size);
        write += size;
        writeCount += size;
    }

    //-----------------------------------------------------
    int Pop( char* data, int size)
    {
        char* dest = data;
        int left = eFifoSize - ( readCount % eFifoSize);
        int buffered = writeCount - readCount;

        // can only return what we have
        if ( buffered < size)
        {
            size = buffered;
        }

        // check for wrap around 
        if ( left < size)
        {
            memcpy( dest, &buffer[read], left);
            read = 0;
            readCount += left;
            size -= left;
            dest = &dest[left];
        }

        // copy data or remaining
        memcpy( dest, &buffer[read], size);
        read += size;
        readCount += size;

        return size;
    }

};

void SPIE_Reset(void);

void SetBusBatt( float bus, float bat);

// FPGA
EXPORT void* GetFpgaObject();

// DIO
EXPORT void* GetDioObject();

//DPRAM
EXPORT void* GetDprObject();

//////////////////////////////////////////////
// HW Memory Emulation
//////////////////////////////////////////////
/******************************************************************************
Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( HW_EMUL_BODY )
#define EXPORT
#else
#define EXPORT extern
#endif

// System Control between UI and G4E, this is declared in G4EDlg and passed into the G4E thread
EXPORT SYS_CTRL* sysCtrl;

//#if __cplusplus
//}
//#endif

#endif