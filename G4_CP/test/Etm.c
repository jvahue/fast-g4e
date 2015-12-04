#define ETM_BODY
/******************************************************************************
Copyright (C) 2009-2010 Pratt & Whitney Engine Services, Inc.
Altair Engine Diagnostic Solutions
All Rights Reserved. Proprietary and Confidential.

File: Etm.c

Description: The file implements the Environmental Test Manager (ETM)

VERSION
$Revision: 16 $  $Date: 11/19/15 5:17p $

******************************************************************************/
#ifdef ENV_TEST

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include <stdio.h>

#include "mcf548x_gpio.h"
#include "math.h"

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "Etm.h"

#include "DIO.h"
#include "FPGA.h"
#include "QAR.h"
#include "ResultCodes.h"
#include "TaskManager.h"
#include "GSE.h"
#include "User.h"
#include "ClockMgr.h"
#include "CfgManager.h"
#include "PowerManager.h"
#include "ADC.h"
#include "FaultMgr.h"
#include "MSStsCtl.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define TEST_ENABLED(t) BIT(testControl.tests, (t))

// Serial Port COnstants
#define MAX_SP 3
#define FIFO_SP 500

// Arinc 429 COnstants
#define A429_TX_CHAN 2
#define A429_RX_CHAN 4
#define FIFO_429x 200
#define DELAY_TIME TICKS_PER_100nSec
#define DELAY StartTime = TTMR_GetHSTickCount();while((TTMR_GetHSTickCount() < StartTime + (DELAY_TIME)))


// QAR Defines
#define QAR_SAME_FRAME 3
#define QAR_SYNC_COUNT 8

// SDRAM Constants
#define RAM_BLOCK_SIZE 128
#define MAX_PATTERNS 2
#define MAX_ADDR_BUFFERS 4

// delay two minutes before declaring a HB failure
#define TWO_MINUTES (120)

// ADC
#define MAX_ADC 4

// Address Lines = log2( (RamEnd-RamStart)+1) - 1 # as the MSB line is not really involved
//                        |<---- size ---->|
#ifndef WIN32
    //#define RAM_BASE (BYTE*)0x30100000
    #define RAM_BASE (BYTE*)0x30000000
    #define RAM_END  (BYTE*)0x31FFFFFF
    #define ADDRESS_LINES 24
#else
    BYTE ram[0x400];
    #define RAM_BASE &ram[0]
    #define RAM_END  &ram[0x3FF]
    #define ADDRESS_LINES 9
#endif
#define RAM_SIZE ((RAM_END - RAM_BASE)+1)
#define ADDRESS_LOC (ADDRESS_LINES+1)

// DIO Constants
//-----------------------------------------------------------------------------
//                   Out       In
//                             LSS WA    |<- DIO 0..7->|
#define DIO_PAT(x) {(x)<<4, ((((x)<<8) | ((x)<<4)) | (x))}
#define DIO_GPIO_PODR_OFFSET   0x00 // Output data register
#define DIO_GPIO_PPDSDR_OFFSET 0x20 // Input pin read/Output pin write

// Unexpected Reset
#ifdef WIN32
    UINT32 unexpectedPOR;
    #define STATIC_RAM (&unexpectedPOR)
#else
    #define STATIC_RAM (UINT32*)0x80010000
#endif
#define UNEXPECTED_RESET 0x1234BEEF

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
typedef struct dioPatternsTag {
    UINT32 outValue;
    UINT32 inValue;
} DioPattern;

typedef struct FpgaRegInfoTag {
    volatile UINT16 *sts;
    volatile UINT16 *lsw;
    volatile UINT16 *msw;
} FpgaRegInfo;

typedef struct AdcTag {
    UINT32 channel;
    FLOAT32 lower;
    FLOAT32 upper;
} AdcInfo;

typedef struct TestInfoTag {
    UINT32  spFail[MAX_SP];     // Fail Counter SP port specific
    UINT32  a429[A429_RX_CHAN]; // Fail counter A429 port specific
    UINT32  a429Words[A429_RX_CHAN];
    UINT32  a717;               // Fail counter A717
    UINT32  a717wrongFrame;     // wrong sub-frame encounters
    UINT32  a717barkerError;    // bad barker code
    UINT32  a717missingSf;      // missing SF receipt
    UINT32  lss;                // LSS wraparound fail count
    UINT32  gpio;               // GPIO fail count
    UINT32  overCurrent;        // was the over current on?
    UINT32  gpioWr;             // GPIO write pattern
    UINT32  ramPat;             // fail counter RAM Pattern Test
    UINT32  ramAdd;             // RAM Address fail count
    UINT32  dpram;              // DPRAM fail count
    UINT32  adc[eMaxAdcSignal]; // ADC fail count (channel specific)
    FLOAT32 batVolt;            // battery voltage
    FLOAT32 busVolt;            // bus voltage
    FLOAT32 intVolt;            // internal battery
    FLOAT32 tmpVolt;            // board temp
} TestInfo;


/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
// Test Control Structure
TestControl testControl;
UINT32 lastRunState = FALSE;
UINT32 rateScheduler = 0;
UINT32 msStartupCountDown;

// Edit buffer for Test results
TestResults tempResults;

// keep track of the number of failures within a second
TestInfo testInfo;

//--------------------------------------------------------------------------------------------------
//UART configuration data
static UART_CONFIG AcsUartCfg[3];
BYTE spTxValue[MAX_SP] = {0,0,0};
BYTE spRxExpected[MAX_SP] = {0,0,0};
UINT32 spFail[MAX_SP] = {0,0,0};     // Fail Counter SP port specific
UINT32 spFailHw[MAX_SP] = {0,0,0};   // Fail Hw Err Counter SP port specific
UINT32 spFail0[MAX_SP] = {0,0,0};    // Fail Rx 0 Counter SP port specific
UINT32 spTx[MAX_SP] = {0,0,0};       // Tx byte count SP port specific
UINT32 spRx[MAX_SP] = {0,0,0};       // Rx byte count SP port specific
UINT32 spFrames = 0;                 // Number of Sp Rx Frames
BYTE spExp = 0;
BYTE spAct = 0;

// A429 Test
static FpgaRegInfo regRxInfo[A429_RX_CHAN] = {
    { FPGA_429_RX1_FIFO_STATUS, FPGA_429_RX1_LSW, FPGA_429_RX1_MSW},
    { FPGA_429_RX2_FIFO_STATUS, FPGA_429_RX2_LSW, FPGA_429_RX2_MSW},
    { FPGA_429_RX3_FIFO_STATUS, FPGA_429_RX3_LSW, FPGA_429_RX3_MSW},
    { FPGA_429_RX4_FIFO_STATUS, FPGA_429_RX4_LSW, FPGA_429_RX4_MSW},
};

UINT32 a429Fail[A429_RX_CHAN]; // Fail counter A429 port specific
UINT32 a429Fail0[A429_RX_CHAN] = {0,0,0,0};    // Fail Rx 0 Counter 429 specific
UINT32 a429Tx[A429_TX_CHAN] = {0,0};
UINT32 a429RxExpected[A429_RX_CHAN] = {0,0,0,0};
UINT32 a429Frames;
UINT32 a429TxBufferCount[A429_RX_CHAN] = {0,0,0,0};

//--------------------------------------------------------------------------------------------------
// Data for A717 test
UINT16 qarDataFrames[QAR_SF_MAX_SIZE];
UINT32 qarFrameSynced;        // count of synced frames
UINT8  sameFrame = 0;
QAR_SF lastFrame717;
QAR_SF rxFrame717;
UINT16 words717;
UINT16 lastFrameValue = 0;
UINT32 a717wrongFrame = 0;     // wrong sub-frame encounters
UINT32 a717barkerError = 0;    // bad barker code
UINT32 a717missingSf = 0;      // missing SF receipt
UINT32 a717errorWord = 0;      // info on bad data RXed

//--------------------------------------------------------------------------------------------------
// Data for DIO test
DioPattern dioPattern[16] = {
    DIO_PAT( 0), DIO_PAT( 1), DIO_PAT( 2), DIO_PAT( 3),
    DIO_PAT( 4), DIO_PAT( 5), DIO_PAT( 6), DIO_PAT( 7),
    DIO_PAT( 8), DIO_PAT( 9), DIO_PAT(10), DIO_PAT(11),
    DIO_PAT(12), DIO_PAT(13), DIO_PAT(14), DIO_PAT(15),
};

UINT32 gpioSetAt;
UINT32 gpioSetIndex;
UINT32 lssWr;
UINT32 lssRd;
UINT32 lssOverCurrent;
UINT32 gpioWr;
UINT32 gpioRd;

//--------------------------------------------------------------------------------------------------
// RAM Test
UINT32* ramPatternAddr;
UINT32  ramComplete;
UINT32  doingPatternTest;  // start out doing the pattern test

// Fail info
UINT32  ramPatFail;
UINT32  ramAddFail;
UINT32  ramPatExp;          //.
UINT32  ramPatAct;          //. > - RAM Pattern fail info
UINT32* ramPatLocation;     //.
UINT32  ramAddPat;          //. value read on fail of address test
UINT32  ramAddLine;         //. location of failure on address test

// DPRAM Test
UINT32 msIsAlive;

// ADC
UINT32 adcChanFails;
FLOAT32 avgAdc[eMaxAdcSignal]; // running average

////////////////////////////////////////////////////////////////////////////////////////////////////
// User Interface Tables
USER_HANDLER_RESULT EtmUserMsg(USER_DATA_TYPE DataType,
                               USER_MSG_PARAM Param,
                               UINT32 Index,
                               const void *SetPtr,
                               void **GetPtr);

USER_HANDLER_RESULT EtmShowA429(USER_DATA_TYPE DataType,
                               USER_MSG_PARAM Param,
                               UINT32 Index,
                               const void *SetPtr,
                               void **GetPtr);

static USER_MSG_TBL result[] =
{
    { "PASS",  NO_NEXT_TABLE, EtmUserMsg, USER_TYPE_UINT32, USER_RO, &tempResults.pass, 0, eMaxTestId, 0, 0, NULL },
    { "FAIL",  NO_NEXT_TABLE, EtmUserMsg, USER_TYPE_UINT32, USER_RO, &tempResults.fail, 0, eMaxTestId, 0, 0, NULL },
    { "MSG",   NO_NEXT_TABLE, EtmUserMsg, USER_TYPE_STR,    USER_RO, &tempResults.msg,  0, eMaxTestId, 0, 0, NULL },
    { NULL    , NO_NEXT_TABLE, NULL, NO_HANDLER_DATA }
};

static USER_MSG_TBL etm[] =
{
    { "ACTIVE",  NO_NEXT_TABLE, EtmUserMsg, USER_TYPE_BOOLEAN, USER_RW,   &testControl.active,         -1, -1, 0, 0, NULL },
    { "TESTS",   NO_NEXT_TABLE, EtmUserMsg, USER_TYPE_UINT32, USER_RW,    &testControl.tests,          -1, -1, 0, 0, NULL },
    { "RESET",   NO_NEXT_TABLE, EtmUserMsg, USER_TYPE_UINT32, USER_RW,    &testControl.reset,          -1, -1, 0, 0, NULL },
    { "MS_STARTUP", NO_NEXT_TABLE, EtmUserMsg, USER_TYPE_UINT32, USER_RW, &testControl.msStartupTimer, -1, -1, 0, 0, NULL },
    { "RESULTS", result, NULL,  NO_HANDLER_DATA},
    { "A429",   NO_NEXT_TABLE, EtmShowA429, USER_TYPE_ACTION, USER_RW, &testControl.msStartupTimer, -1, -1, 0, 0, NULL },
    { NULL    , NO_NEXT_TABLE, NULL, NO_HANDLER_DATA }
};

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void EnvTestMgr( void *pParam);
static void ResetEtm( void);
static void UpdateTestStatus( void);
static void Average( FLOAT32* avg, FLOAT32 value);
static void TestState( BOOLEAN run);

// Serial Port Test
static void SetSp( void);
static void GetSp( void);

// Arinc 429 Test
static void Set429( void);
static void Get429( void);

// Arinc 717 Test
static void Get717( void);

// LSS/GPIO Test
static void SetGpio(void);
static void GetGpio(void);

// SDRAM Test Pattern/Address
static void SdramTest(void);
static UINT32 RamPatternTest(void);
static void RamAddressTest(void);

// ADC Test
static void AdcTest( void);

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
* Function: InitEtm
* Description: Initialize the Environmental Test Manager
* Parameters: None
* Returns: None
* Notes:
*
*****************************************************************************/
void InitEtm(void)
{
    UINT8 i;
    TCB TaskInfo;

    USER_MSG_TBL etmTblPtr = { "ETM", etm, NULL, NO_HANDLER_DATA};

    // bring back last test state active (and enabled tests)/inactive
    memcpy( &testControl, &CfgMgr_ConfigPtr()->etm, sizeof( testControl));

    // check for normal shutdown
    if ( *STATIC_RAM == UNEXPECTED_RESET)
    {
        ++testControl.results[eReset].fail;
        GSE_DebugStr(NORMAL, TRUE, "Shutdown: UNEXPECTED" NEW_LINE);
    }
    else
    {
        *STATIC_RAM = UNEXPECTED_RESET;
        ++testControl.results[eReset].pass;
        GSE_DebugStr(NORMAL, TRUE, "Shutdown: NORMAL" NEW_LINE);
    }

    //Add an entry in the user message handler table
    User_AddRootCmd(&etmTblPtr);

    //Setup initial configuration structure basics for ACS ports
    for(i = 0; i < 3; i++)
    {
        AcsUartCfg[i].Open =      FALSE;
        AcsUartCfg[i].Port =      i+1;  // GSE = 0
        AcsUartCfg[i].Duplex =    UART_CFG_DUPLEX_FULL;
        AcsUartCfg[i].BPS =       UART_CFG_BPS_57600;
        AcsUartCfg[i].DataBits =  UART_CFG_DB_8;
        AcsUartCfg[i].StopBits =  UART_CFG_SB_1;
        AcsUartCfg[i].Parity =    UART_CFG_PARITY_NONE;
        AcsUartCfg[i].RxFIFO =    NULL;
        AcsUartCfg[i].TxFIFO =    NULL;
        AcsUartCfg[i].LocalLoopback = FALSE;
    }

    // Create ETM Task - DT
    memset(&TaskInfo, 0, sizeof(TaskInfo));
    strncpy_safe(TaskInfo.Name, sizeof(TaskInfo.Name), "ETM", _TRUNCATE);
    TaskInfo.TaskID           = (TASK_INDEX)TP_SUPPORT_DT1;
    TaskInfo.Function         = EnvTestMgr;
    TaskInfo.Priority         = taskInfo[TP_SUPPORT_DT1].priority;
    TaskInfo.Type             = taskInfo[TP_SUPPORT_DT1].taskType;
    TaskInfo.modes            = taskInfo[TP_SUPPORT_DT1].modes;
    TaskInfo.MIFrames         = taskInfo[TP_SUPPORT_DT1].MIFframes;
    TaskInfo.Rmt.InitialMif   = taskInfo[TP_SUPPORT_DT1].InitialMif;
    TaskInfo.Rmt.MifRate      = taskInfo[TP_SUPPORT_DT1].MIFrate;
    TaskInfo.Enabled          = TRUE;
    TaskInfo.Locked           = FALSE;
    TaskInfo.pParamBlock      = NULL;

    TmTaskCreate(&TaskInfo);

    // Enable Tracking of Normal Shutdowns
    PmInsertAppShutDownNormal( EnvNormalShutDown);

    // Init All Tests
    ResetEtm();

    // load the delay for MS startup to complete
    testControl.msStartupTimer = TWO_MINUTES;
    msStartupCountDown = 0;
}

/******************************************************************************
* Function: EnvNormalShutDown
* Description: Indicate the shutdown was expected.
* Parameters: None
* Returns: None
* Notes:
*
*****************************************************************************/
BOOLEAN EnvNormalShutDown(PM_APPSHUTDOWN_REASON reason)
{
    *STATIC_RAM = ~UNEXPECTED_RESET;
    return TRUE;
}

/******************************************************************************
* Function: HeartbeatFailSignal
* Description: Called when a DPRAM HB message failure occurs
* Parameters: None
* Returns: None
* Notes:
*
*****************************************************************************/
void HeartbeatFailSignal(void)
{
    if ( MSSC_GetIsAlive())
    {
        msIsAlive = TRUE;
    }

    if ( msIsAlive == TRUE)
    {
        ++testInfo.dpram;
    }
}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
* Function: EnvTestMgr
* Description: Controls the execution of the environmental tests
* Parameters: pParam - only to meet the requirements of the task manager
* Returns: None
* Notes:
*
*****************************************************************************/
static void EnvTestMgr( void *pParam)
{
    // time to MS startup complete
    if ( ++msStartupCountDown >= 100)
    {
        msStartupCountDown = 0;
        if ( testControl.msStartupTimer > 0)
        {
            --testControl.msStartupTimer;
        }
    }

    // did the user request a reset ?
    if ( testControl.reset > 0)
    {
        TestState( FALSE);
        ResetEtm();
        memset( &testControl.results[0], 0, sizeof(testControl.results));
        --testControl.reset;
    }
    else if ( testControl.active && testControl.tests != 0)
    {
        TestState( TRUE);

        // Output serial/A429 data at 100 Hz
        SetSp();
        Set429();

        // 100 Hz Tests
        SdramTest();
        AdcTest();

        // 50  Hz (20 msec) do these every other frame
        if ( (rateScheduler & 1) == 1)
        {
            // check SP Rx Data
            GetSp();

            // this ensures some data is RXed at test start
            Get429();
        }

        // 2 Hz (500  msec) do these every 50 frames
        if ( rateScheduler == 0 || rateScheduler == 50)
        {
            // Run A717 at 2Hz
            Get717();

            // run the DIO Test
            SetGpio();
            // get DOUT wraparounds
            GetGpio();
        }

        // 1   Hz (1000  msec ) do these every 100
        if ( rateScheduler == 99)
        {
            // Update Test Status once a second
            UpdateTestStatus();
        }

        // update scheduler word
        if (++rateScheduler >= 100)
        {
            rateScheduler = 0;
        }
    }
    else
    {
        TestState( FALSE);

        // prepare for next run
        ResetEtm();

        rateScheduler = 0;
    }
}

/******************************************************************************
* Function: ResetEtm
* Description: Reset all the pass fail counts and the debug messages for all
*   tests.
* Parameters: None
* Returns: None
* Notes:
*
*****************************************************************************/
static void ResetEtm( void)
{
    BYTE actualSp;
    UINT32 actual429;
    UINT32 i;
    UINT32 port;
    UINT32 count;

    memset( &testInfo, 0, sizeof(testInfo));

    // ETM Variables
    lastRunState = FALSE;
    rateScheduler = 0;

    // Serial Port Init
    memset( spTxValue, 0, sizeof(spTxValue));
    memset( spRxExpected, 0, sizeof(spRxExpected));
    memset( spFail, 0, sizeof(spFail));
    memset( spFailHw, 0, sizeof(spFailHw));
    memset( spFail0, 0, sizeof(spFail0));
    memset( spTx, 0, sizeof(spTx));
    memset( spRx, 0, sizeof(spRx));
    spFrames = 0;
    spExp = 0;
    spAct = 0;

    // ensure the SP Rx FIFO is empty
    for ( i = 0; i < MAX_SP; ++i)
    {
        count = 0;
        port = i + 1;
        while ( MCF_PSC_RFCNT(port) > 0 && count < FIFO_SP)
        {
            actualSp = *((UINT8 *) &MCF_PSC_RB(port));
            ++count;
        }
    }

    // A429 Init
    memset( a429Fail, 0, sizeof(a429Fail));
    memset( a429Fail0, 0, sizeof( a429Fail0));
    memset( a429Tx, 0, sizeof(a429Tx));
    memset( a429RxExpected, 0, sizeof( a429RxExpected));
    memset( a429TxBufferCount, 0, sizeof( a429TxBufferCount));
    a429Frames = 0;

    // ensure the 429 Rx FIFOs are empty
    for ( i = 0; i < A429_RX_CHAN; ++i)
    {
        count = 0;
        while ( !(FPGA_R(regRxInfo[i].sts) & RFSR_429_FIFO_EMPTY) &&
            count < FIFO_429x)
        {
            // read the LSW & MSW and turn off parity
            actual429  = FPGA_R( regRxInfo[i].lsw);
            actual429 |= ((FPGA_R( regRxInfo[i].msw) << 16) & 0x7FFF);
            ++count;
        }
    }

    // reset QAR error counters
    memset( &qarDataFrames, 0, sizeof(qarDataFrames));

    qarFrameSynced = 0;     // we are not synced, start at whatever sub-frame we Rx first
    sameFrame = 0;
    lastFrame717 = QAR_SF_MAX; // set to invalid sub-frame value
    rxFrame717 = QAR_SF_MAX;   // what sub-frame did we get last
    words717 = 0;
    a717wrongFrame = 0;     // wrong sub-frame encounters
    a717barkerError = 0;    // bad barker code
    a717missingSf = 0;      // missed SF receipt
    a717errorWord = 0;      // packed data on an error word value

    // Init GPIO test
    gpioSetAt = 0;   // filled in with elapsed frames CM_GetTickCount
    gpioSetIndex = 0;
    lssWr = 0;
    lssRd = 0;
    lssOverCurrent = 0;
    gpioWr = 0;
    gpioRd = 0;

    // Init SDRAM Pattern Test
    ramPatternAddr = (UINT32*)RAM_BASE;
    ramComplete = 0;
    doingPatternTest = 1;
    ramPatFail = 0;
    ramAddFail = 0;
    ramPatExp = 0;
    ramPatAct = 0;
    ramPatLocation = NULL;
    ramAddPat = 0;
    ramAddLine = 0;


    // indicate we need to wait for the MS to be alive
    msIsAlive = FALSE;

    // init the ADC averages
    adcChanFails = 0;

    ADC_GetACBattVoltage( &avgAdc[eBatt]);
    ADC_GetACBusVoltage( &avgAdc[eBus]);
    ADC_GetLiBattVoltage( &avgAdc[eLiBat]);
    ADC_GetBoardTemp( &avgAdc[eTemp]);

    // Sanity check on the values
    if ( avgAdc[eBatt] < 10.0f || avgAdc[eBatt] > 36.0f)
    {
        testInfo.adc[eBatt] = 1;
    }

    if ( avgAdc[eBus] < 10.0f || avgAdc[eBus] > 36.0f)
    {
        testInfo.adc[eBus] = 1;
    }

    if ( avgAdc[eTemp] < -55.0f || avgAdc[eTemp] > 100.0f)
    {
        testInfo.adc[eTemp] = 1;
    }
}

/******************************************************************************
* Function: TestState
* Description: Controls the execution of the environmental tests
* Parameters: pParam - only to meet the requirements of the task manager
* Returns: None
* Notes:
*
*****************************************************************************/
static void TestState( BOOLEAN run)
{
    UINT8 i;

    DIO_OUTPUT txPin[ MAX_SP] = {
        ACS1_TxEnb,
        ACS2_TxEnb,
        ACS3_TxEnb
    };

    TASK_ID tasks[] = {
        //QAR_Read_Sub_Frame,   // We want this on, it does the real Rxing we get Display
        Arinc429_Process_Msgs,  // Don't read the 429Rx Data
        Ram_CBIT_Task,          // Don't perform the RAM test
        CBIT_Manager,           // Don't fix the DOUT port mismatch
        MAX_TASK
    };

    if ( lastRunState != run)
    {
        // enable/disable tasks
        for ( i=0; tasks[i] != MAX_TASK; ++i)
        {
            TmTaskEnable( tasks[i], !run);
        }

        // Enable Arinc 429 Power
        DIO_SetPin(ARINC429_Pwr, run ? DIO_SetHigh : DIO_SetLow);

        // ensure the 429 loopback is off
        FPGA_W( FPGA_429_LOOPBACK_CONTROL, 0);

        // open/close serial ports
        for ( i=0; i < 3; ++i)
        {
            if ( run)
            {
               UART_OpenPort( &AcsUartCfg[i]);
               // enable TX
               DIO_SetPin( txPin[i], DIO_SetHigh);
            }
            else
            {
               UART_ClosePort( i+1);
            }
        }
    }

    lastRunState = run;
}

/******************************************************************************
* Function: SetSp
* Description: Fill the Tx FIFO with data
* Parameters: None
* Returns: None
* Notes: No S/W FIFO is required for this test
*
*****************************************************************************/
static void SetSp( void)
{
    UINT32 i;
    UINT32 port;
    UINT32 count;

    if ( TEST_ENABLED(eSp))
    {
        for ( i=0; i < MAX_SP; ++i)
        {
            port = i + 1;
            count = 0;
            while (MCF_PSC_TFCNT(port) < FIFO_SP && count < FIFO_SP)
            {
                *((UINT8 *)&MCF_PSC_TB(port)) = spTxValue[i]++;
                ++count;
            }
            spTx[i] = count;
        }

    }
}

/******************************************************************************
* Function: GetSp
* Description: Read the SP FIFO until empty validating each input.  Failures
* reported against the port the data is read from.
* Parameters: None
* Returns: None
* Notes: No S/W FIFO is required for this test
*
*****************************************************************************/
static void GetSp( void)
{
    BYTE actual = 0;
    UINT32 i;
    UINT32 port;
    UINT32 count;
    BYTE oe, ph, cr;

    if ( TEST_ENABLED(eSp))
    {
        ++spFrames;
        //GSE_DebugStr(NORMAL, TRUE, "\n");
        for ( i = 0; i < MAX_SP; ++i)
        {
            port = i + 1;

            //Check for hardware or RX overflow errors
            oe = (MCF_PSC_SR(port) & MCF_PSC_SR_OE);
            ph = (MCF_PSC_SR(port) & MCF_PSC_SR_FE_PHYERR);
            cr = (MCF_PSC_SR(port) & MCF_PSC_SR_PE_CRCERR);
            if ( oe || ph || cr)
            {
                //Reset RX and RX FIFO, then reset error flags and re-enable RX.
                MCF_PSC_CR(port) = MCF_PSC_CR_RESET_RX;
                MCF_PSC_CR(port) = MCF_PSC_CR_RESET_ERROR;
                MCF_PSC_CR(port) = MCF_PSC_CR_RX_ENABLED;

                ++spFailHw[i];
                ++testInfo.spFail[i];
            }
            else
            {
                count = 0;
                while ( MCF_PSC_RFCNT(port) > 0 && count < FIFO_SP)
                {
                    actual = *((UINT8 *) &MCF_PSC_RB(port));
                    if ( actual != spRxExpected[i])
                    {
                        if ( spFrames > 5)
                        {
                            ++spFail[i];
                            ++testInfo.spFail[i];
                            spExp = spRxExpected[i];
                            spAct = actual;
                        }
                        spRxExpected[i] = actual + 1;
                    }
                    else
                    {
                        ++spRxExpected[i];
                    }
                    ++count;
                }

                spRx[i] = count;

                if (count == 0)
                {
                    ++spFail0[i];  // cumulative failure count
                    ++testInfo.spFail[i];
                }
            }
        }
    }
}

/******************************************************************************
* Function: Set429
* Description: Completely fill the Arinc 429 Tx Buffers with an incrementing
*   pattern.  The content of the A429 word is written raw to the 429 FIFO
*   and this same raw value is verified on wraparound.
* Parameters: None
* Returns: None
* Notes:
*
*****************************************************************************/
static void Set429( void)
{
    static FpgaRegInfo regTxInfo[A429_TX_CHAN] = {
        { FPGA_429_TX1_FIFO_STATUS, FPGA_429_TX1_LSW, FPGA_429_TX1_MSW},
        { FPGA_429_TX2_FIFO_STATUS, FPGA_429_TX2_LSW, FPGA_429_TX2_MSW},
    };
    int i;
    UINT16 lsw;
    UINT16 msw;
    UINT32 rxA;
    UINT32 rxB;

    if ( TEST_ENABLED(e429))
    {
        for ( i = 0; i < A429_TX_CHAN; ++i)
        {
            rxA = (i*2);
            rxB = rxA+1;
            while ( !( FPGA_R(regTxInfo[i].sts) & RFSR_429_FIFO_FULL) &&
                    a429TxBufferCount[rxA] < FIFO_429x &&
                    a429TxBufferCount[rxB] < FIFO_429x)
            {
                lsw = a429Tx[i] & 0xFFFF;
                msw = a429Tx[i] >> 16;
                FPGA_W( regTxInfo[i].lsw, lsw);
                FPGA_W( regTxInfo[i].msw, msw);
                a429Tx[i] = (a429Tx[i] + 1) & 0x7FFFFFFF;

                // manage the TX FIFO
                ++a429TxBufferCount[(i*2)];
                ++a429TxBufferCount[(i*2)+1];
            }
        }
    }
}

/******************************************************************************
* Function: Get429
* Description: Empty the four A429 Rx FIFOs while verifying each value read
*   increments by one
* Parameters: None
* Returns: None
* Notes:
*
*****************************************************************************/
static void Get429( void)
{
#define MAX_TX_IN_10MS 50 // actual at 100k 36 bits = ~28 words
    UINT32 actual = 0;
    int i;
    int count;
    UINT16 lsw;
    UINT16 msw;

    if ( TEST_ENABLED(e429))
    {
        ++a429Frames;
        for ( i = 0; i < A429_RX_CHAN; ++i)
        {
            count = 0;
            while ( !(FPGA_R(regRxInfo[i].sts) & RFSR_429_FIFO_EMPTY) &&
                    count < (FIFO_429x+MAX_TX_IN_10MS))
            {
                ++testInfo.a429Words[i];

                // read the LSW & MSW and turn off parity
                lsw = FPGA_R( regRxInfo[i].lsw);
                msw = FPGA_R( regRxInfo[i].msw);

                // remove the parity
                actual = ((msw << 16) | lsw) & 0x7FFFFFFF;

                // keep track of where the TX buffer might be
                if (a429TxBufferCount[i] >= 1)
                {
                    --a429TxBufferCount[i];
                }

                if ( actual != a429RxExpected[i])
                {
                    if ( a429Frames > 50)
                    {
                        ++a429Fail[i];
                        ++testInfo.a429[i];
                    }
                    // no parity on expected
                    a429RxExpected[i] = (actual + 1) & 0x7FFFFFFF;
                }
                else
                {
                    a429RxExpected[i] = (a429RxExpected[i] + 1) & 0x7FFFFFFF;
                }
                ++count;
            }

            // we should have received something
            if (count == 0)
            {
                ++a429Fail0[i];
                ++testInfo.a429[i];
            }
        }
    }
}

/******************************************************************************
* Function: Get717
* Description: Called at 2 Hz to try and read the next sub-frame. Read each
*   717 sub-frame when it comes in and validate
*     1. the right barker code is present.
*     2. each value is incremented by one starting.
*     3. The right sub-frame is read
*     4. The same frame is not present three calls in a
*
*
* Verify the barker code.
* Parameters: None
* Returns: None
* Notes:
*
*****************************************************************************/
static void Get717( void)
{
    static const UINT16 frameStartValue[QAR_SF_MAX] = {1, 0x401, 0x801, 0xC01};
    static const UINT16 barker[QAR_SF_MAX] = {0x247, 0x5b8, 0xa47, 0xdb8};

    UINT16 i;
    UINT16 expected;
    UINT16 data[QAR_SF_MAX_SIZE];

    if ( TEST_ENABLED(e717))
    {
        words717 = QAR_GetSubFrameDisplay( data, &rxFrame717, sizeof( data));
        if ( words717 == (QAR_SF_MAX_SIZE*2) && rxFrame717 != lastFrame717)
        {
            if ( qarFrameSynced > QAR_SYNC_COUNT)
            {
                if ( (lastFrame717 == QAR_SF4 && (rxFrame717 != QAR_SF1)) ||
                     (lastFrame717 != QAR_SF4 && (rxFrame717 != (lastFrame717+1))))
                {
                    ++a717wrongFrame;
                    testInfo.a717 = 1;
                }

                // validate data
                expected = frameStartValue[rxFrame717];
                if ( barker[rxFrame717] != data[0])
                {
                    ++a717barkerError;
                    testInfo.a717 = 1;
                }

                for (i = 1; i < QAR_SF_MAX_SIZE; ++i)
                {
                    if ( data[i] != expected)
                    {
                        a717errorWord = rxFrame717 << 30 | i << 16 | data[i];
                        testInfo.a717 = 1;
                    }
                    lastFrameValue = data[i];
                    ++expected;
                }
            }

            // update sub-frame info
            lastFrame717 = rxFrame717;
            qarFrameSynced += 1;
            sameFrame = 0;
        }
        else if ( ++sameFrame >= QAR_SAME_FRAME && qarFrameSynced > QAR_SYNC_COUNT)
        {
            // missed a sub-frame
            ++a717missingSf;
            testInfo.a717 = 1;
            sameFrame = 0;
        }
    }
}

/******************************************************************************
* Function: SetGpio
* Description:
* Parameters:
* Returns:
* Notes:
*
*****************************************************************************/
static void SetGpio(void)
{
    if ( TEST_ENABLED(eGpio))
    {
        // if we registered no set time, send the next pattern
        // GetGpio will clear this 50 ms after setting or when it sees a W/A match
        if ( gpioSetAt == 0)
        {
            if (++gpioSetIndex > 15)
            {
                gpioSetIndex = 0;
            }

            // write the pattern to the LSS outputs
            DIO_S((&MCF_GPIO_PODR_FEC0L+DIO_GPIO_PODR_OFFSET), dioPattern[gpioSetIndex].outValue);
            gpioSetAt = CM_GetTickCount();
        }
    }
}

/******************************************************************************
* Function: GetGpio
* Description:
* Parameters:
* Returns:
* Notes:
*
*****************************************************************************/
static void GetGpio(void)
{
    BOOLEAN overCurrent;
    UINT8 lss;
    UINT32 din;
    UINT32 exp;

    if ( TEST_ENABLED(eGpio))
    {
        if (gpioSetAt != 0)
        {
            overCurrent = DIO_ReadPin( LSS_OvI);

            if ( overCurrent)
            {
                testInfo.overCurrent = 1;
                lssOverCurrent += 1;
            }

            lss = DIO_R(&MCF_GPIO_PODR_FEC0L+DIO_GPIO_PPDSDR_OFFSET) & 0xF;
            din = DIO_R(&MCF_GPIO_PODR_FEC0H+DIO_GPIO_PPDSDR_OFFSET) & 0xFF;
            din |= lss << 8;
            exp = dioPattern[gpioSetIndex].inValue;

            // check for the correct value with in 50 ms
            if ( (CM_GetTickCount() - gpioSetAt) < 50)
            {
                // before 50 ms is all we can do is succeed, otherwise wait
                // ... and at end of wait we will determine what failed
                if ( din == exp)
                {
                    gpioSetAt = 0;
                }
            }
            else
            {
                gpioSetAt = 0;

                // after 50 ms if it's not a match then failure
                testInfo.lss = (din & 0xF00) == (exp & 0xF00) ? 0 : 1;
                if (testInfo.lss)
                {
                    lssWr = exp & 0xF;
                    lssRd = lss;
                }

                testInfo.gpio = (din & 0x0FF) == (exp & 0x0FF) ? 0 : 1;
                if (testInfo.gpio)
                {
                    gpioWr = exp & 0xFF;
                    gpioRd = din & 0xFF;
                }
            }
        }
    }
}

/******************************************************************************
* Function: SdramTest
* Description: Perform a RAM pattern test and RAM address line test.
* Parameters: None
* Returns: None
* Notes:
*
*****************************************************************************/
static void SdramTest(void)
{
    if ( TEST_ENABLED(eSdram))
    {
        if (doingPatternTest)
        {
            doingPatternTest = RamPatternTest();
        }
        else
        {
            RamAddressTest();
            doingPatternTest = 1;
            ramComplete += 1;
        }
    }
}

/******************************************************************************
* Function: RamPatternTest
* Description: Perform a RAM pattern test and RAM address line test.
* Parameters: None
* Returns: True while more RAM to pattern test, false when done
* Notes:
*
*****************************************************************************/
static UINT32 RamPatternTest(void)
{
    register UINT32* addrPtr = ramPatternAddr;
    register UINT32 data;
    UINT32   i;
    UINT32   intrLevel;

    // test a block of RAM with interrupts disabled
    intrLevel = __DIR();
    for ( i=0; i < RAM_BLOCK_SIZE && ramPatternAddr < (UINT32*)RAM_END; ++i)
    {
        // see if our word buffer is in the test area
        data = *addrPtr;

        // this is done manually (instead of in a loop)
        // in case the address we are testing is the address of patternIndex
        *addrPtr = 0xAAAAAAAA;
        if ( *addrPtr != 0xAAAAAAAA)
        {
            ++testInfo.ramPat;
            ramPatAct = *addrPtr;
            ramPatExp = 0xAAAAAAAA;
            ramPatLocation = addrPtr;
            break;
        }

        *addrPtr = 0x55555555;
        if ( *addrPtr != 0x55555555)
        {
            ++testInfo.ramPat;
            ramPatAct = *addrPtr;
            ramPatExp = 0x55555555;
            ramPatLocation = addrPtr;
            break;
        }

        // NOTE: should check that addr ptr is none of the result vars, but
        // if it failed it won't hold the real data anyway
        *addrPtr++ = data;
    }
    __RIR(intrLevel);

    // get it back from the register and put it in memory
    ramPatternAddr = addrPtr;

    if (ramPatternAddr >= (UINT32*)RAM_END)
    {
        ramPatternAddr = (UINT32*)RAM_BASE;
    }

    // when we are done return false
    return ramPatternAddr != (UINT32*)RAM_BASE;
}

/******************************************************************************
* Function: RamAddressTest
* Description: Do a complete address test
* Parameters:
* Returns: None
* Notes:
*
*****************************************************************************/
static void RamAddressTest(void)
{
    register INT32   i;
    register BYTE*   location = RAM_BASE;
    register UINT32  offset;
    register UINT32  bufferBase = 0;
    UINT32           intrLevel;

    BOOLEAN conflict = TRUE;
    BYTE    buffer[ADDRESS_LOC * MAX_ADDR_BUFFERS];

    // test to see if any address line locations are in buffer[0..ADDR_LINES-1]
    while (conflict && bufferBase < MAX_ADDR_BUFFERS)
    {
        conflict = FALSE;
        for ( i = ADDRESS_LINES; i >= 0; --i)
        {
            offset = (1u << i);
            if ( &location[offset] >= &buffer[bufferBase] &&
                 &location[offset] <= &buffer[bufferBase+(ADDRESS_LOC-1)])
            {
                ++bufferBase;
                conflict = TRUE;
                break;
            }
        }
    }

    // Perform address test with interrupts off
    if ( bufferBase < MAX_ADDR_BUFFERS)
    {
        intrLevel = __DIR();
        buffer[bufferBase] = *RAM_BASE;
        *RAM_BASE = 1;
        for ( i = 0; i < ADDRESS_LINES; ++i)
        {
            offset = (1u << i);
            buffer[bufferBase+(i+1)] = location[offset];
            location[offset]= i+2;
        }

        // check uniqueness of address lines
        if ( *RAM_BASE == 1)
        {
            for ( i = 0; i < ADDRESS_LINES; ++i)
            {
                offset = (1u << i);
                if ( location[offset] != i+2)
                {
                    ++testInfo.ramAdd;
                    ramAddLine = offset;
                    ramAddPat = location[offset];
                    break;
                }
            }
        }
        else
        {
            ++testInfo.ramAdd;
            ramAddLine = 0;
            ramAddPat = location[0];
        }

        // Restore data
        *RAM_BASE = buffer[bufferBase];
        for ( i = 0; i < ADDRESS_LINES; ++i)
        {
            offset = (1u << i);
            location[offset] = buffer[bufferBase+(i+1)];
        }
        __RIR(intrLevel);
    }
    else
    {
        ++testInfo.ramAdd;
        ramAddLine = 0;
        ramAddPat = 0;
    }
}

/******************************************************************************
* Function: AdcTest
* Description: Monitor the values read from the ADC
* Parameters:
* Returns: None
* Notes:
*
*****************************************************************************/
static void AdcTest( void)
{
    FLOAT32 value;

    if ( TEST_ENABLED(eAdc))
    {
        if (ADC_GetACBusVoltage( &value) == SYS_OK)
        {
            // 2v upper/lower limit on difference from avg
            ADC_GetBoardTemp( &avgAdc[eTemp]);
            if ( fabs(value - avgAdc[eBus]) > 2.0f)
            {
                ++testInfo.adc[eBus];
            }
            testInfo.busVolt = value;
            Average( &avgAdc[eBus], value);
        }
        else
        {
            ++testInfo.adc[eBus];
            testInfo.busVolt = -1.0f;
        }

        if (ADC_GetACBattVoltage( &value) == SYS_OK)
        {
            // 2v upper/lower limit on difference from avg
            if ( fabs(value - avgAdc[eBatt]) > 2.0f)
            {
                ++testInfo.adc[eBatt];
            }
            testInfo.batVolt = value;
            Average( &avgAdc[eBatt], value);
        }
        else
        {
            ++testInfo.adc[eBatt];
            testInfo.batVolt = -1.0f;
        }

        if (ADC_GetLiBattVoltage( &value) == SYS_OK)
        {
            // 100 mV upper/lower limit on difference from avg
            if ( fabs(value - avgAdc[eLiBat]) > 0.1f)
            {
                ++testInfo.adc[eLiBat];
            }
            testInfo.intVolt = value;
            Average( &avgAdc[eLiBat], value);
        }
        else
        {
            ++testInfo.adc[eLiBat];
            testInfo.intVolt = -1.0f;
        }

        if (ADC_GetBoardTemp( &value) == SYS_OK)
        {
            // Temp has 2degC difference from avg and range test (-55..100decC)
            if ( fabs(value - avgAdc[eTemp]) > 2.0f || value < -55.0f || value > 100.0f)
            {
                ++testInfo.adc[eTemp];
            }
            testInfo.tmpVolt = value;
            Average( &avgAdc[eTemp], value);
        }
        else
        {
            ++testInfo.adc[eTemp];
            testInfo.tmpVolt = -1.0f;
        }
    }
}

/******************************************************************************
* Function: Average
* Description: Update the average value for the ADC signals
* Parameters:
*   *avg (i/o): the average value to be updated
*   value(i)  : the latest value for the signal
* Returns: None
* Notes:
*
*****************************************************************************/
static void Average( FLOAT32* avg, FLOAT32 value)
{
    *avg = (*avg * 0.8f) + (value * 0.2f);
}

/******************************************************************************
* Function: UpdateTestStatus
* Description: Once a second update the pass / fail counters for each test and
* build the debug messages
* Parameters: None
* Returns: None
* Notes: All Responses MUST start with a '>' as the ETC uses this to ID msg
* responses
*
*****************************************************************************/
static void UpdateTestStatus( void)
{
    INT8 i;
    INT8 c;
    INT8 chanFails = 0;
    int frameFail = 0;

    for (i=0; i < eMaxTestId; ++i)
    {
        chanFails = 0;
        frameFail = 0;
        if ( BIT(testControl.tests, i))
        {
            switch (i)
            {
            case eSp:
                for ( c=0; c < MAX_SP; ++c)
                {
                    // failures this second ?
                    if ( testInfo.spFail[c] != 0)
                    {
                        ++frameFail;
                    }
                }

                if (frameFail == 0)
                {
                    ++testControl.results[i].pass;
                }
                else
                {
                    ++testControl.results[i].fail;
                }

                if ( testControl.results[i].fail == 0)
                {
                    snprintf( testControl.results[i].msg,
                        MAX_MSG_SIZE, ">Bytes TXed/RXed 1:%d/%d 2:%d/%d 3:%d/%d",
                        spTx[0], spRx[0],
                        spTx[1], spRx[1],
                        spTx[2], spRx[2]
                    );
                }
                else
                {
                    snprintf( testControl.results[i].msg,
                        MAX_MSG_SIZE, ">Fail/HW/0s 1:%d/%d/%d 2:%d/%d/%d 3:%d/%d/%d A/E:%d/%d",
                        spFail[0], spFailHw[0], spFail0[0],
                        spFail[1], spFailHw[1], spFail0[1],
                        spFail[2], spFailHw[2], spFail0[2],
                        spAct, spExp
                    );
                }
                break;
            case e429:
                for ( c=0; c < A429_RX_CHAN; ++c)
                {
                    // failures this second ?
                    if ( testInfo.a429[c] != 0)
                    {
                        ++frameFail;
                    }
                }

                if ( frameFail == 0)
                {
                    ++testControl.results[i].pass;
                }
                else
                {
                    ++testControl.results[i].fail;
                }

                if ( testControl.results[i].fail == 0)
                {
                    snprintf( testControl.results[i].msg,
                        MAX_MSG_SIZE, ">1:0x%08X 2:0x%08X 3:0x%08X 4:0x%08X",
                        a429RxExpected[0],
                        a429RxExpected[1],
                        a429RxExpected[2],
                        a429RxExpected[3]
                    );
                }
                else
                {
                    snprintf( testControl.results[i].msg,
                        MAX_MSG_SIZE, ">Fail/0s 1:%d/%d 2:%d/%d 3:%d/%d 4:%d/%d %d",
                        a429Fail[0], a429Fail0[0],
                        a429Fail[1], a429Fail0[1],
                        a429Fail[2], a429Fail0[2],
                        a429Fail[3], a429Fail0[3],
                        a429Frames
                    );
                }
                break;
            case e717:
                if ( qarFrameSynced > QAR_SYNC_COUNT)
                {
                    if ( testInfo.a717 == 0)
                    {
                        ++testControl.results[i].pass;
                    }
                    else
                    {
                        ++testControl.results[i].fail;
                    }

                    if ( testControl.results[i].fail == 0)
                    {
                        snprintf( testControl.results[i].msg,
                            MAX_MSG_SIZE, ">Last Frame: %d, Last Frame Value: 0x%0x",
                            lastFrame717,
                            lastFrameValue
                            );
                    }
                    else
                    {
                        snprintf( testControl.results[i].msg,
                            MAX_MSG_SIZE, ">[SF:%d Wd:%d=%x] Barker:%d WrongSF:%d Missed:%d",
                            (a717errorWord >> 30) & 0x3,
                            (a717errorWord >> 16) & 0x3FFF,
                            (a717errorWord & 0xFFFF),
                            a717barkerError,
                            a717wrongFrame,
                            a717missingSf
                            );
                    }
                }
                else
                {
                    snprintf( testControl.results[i].msg,
                        MAX_MSG_SIZE, ">Waiting for QAR Sync %d/%d LF:%d RxF:%d Words:%d",
                        qarFrameSynced, QAR_SYNC_COUNT,
                        lastFrame717,
                        rxFrame717,
                        words717
                        );
                }
                break;
            case eLss:
                if ( testInfo.lss == 0 && testInfo.overCurrent == 0)
                {
                    ++testControl.results[i].pass;
                }
                else
                {
                    ++testControl.results[i].fail;
                }

                // DEBUG Message
                if (testControl.results[i].fail == 0)
                {
                    // show output pattern
                    snprintf( testControl.results[i].msg,
                        MAX_MSG_SIZE, ">Write:0x%X",
                        gpioSetIndex);
                }
                else
                {
                    snprintf( testControl.results[i].msg,
                        MAX_MSG_SIZE, ">Write:0x%X - Read:0x%X : OverCurrent: %d",
                        lssWr,
                        lssRd,
                        lssOverCurrent);
                }
                break;
            case eGpio:
                if ( testInfo.gpio == 0)
                {
                    ++testControl.results[i].pass;
                }
                else
                {
                    ++testControl.results[i].fail;
                }

                if (testControl.results[i].fail == 0)
                {
                    // show output pattern
                    snprintf( testControl.results[i].msg,
                        MAX_MSG_SIZE, ">Write:0x%X",
                        gpioSetIndex);
                }
                else
                {
                    snprintf( testControl.results[i].msg,
                        MAX_MSG_SIZE, ">Write:0x%X - Read:0x%X",
                        gpioWr, gpioRd);
                }
                break;
            case eSdram:
                if ( testInfo.ramPat == 0 && testInfo.ramAdd == 0)
                {
                    ++testControl.results[i].pass;
                }
                else
                {
                    ++testControl.results[i].fail;
                    ramPatFail += testInfo.ramPat;
                    ramAddFail += testInfo.ramAdd;
                }

                if ( testControl.results[i].fail == 0)
                {
                    snprintf( testControl.results[i].msg, MAX_MSG_SIZE,
                        ">Testing Address 0x%08X Completed %d",
                        (UINT32)ramPatternAddr, ramComplete);
                }
                else
                {
                    if ( testInfo.ramPat != 0 && testInfo.ramAdd != 0)
                    {
                        snprintf( testControl.results[i].msg, MAX_MSG_SIZE,
                            ">Pat:%x Wr:%x Rd:%x / Addr:%x Rd:%x",
                            ramPatLocation, ramPatExp, ramPatAct,
                            ramAddLine, ramAddPat);
                    }
                    else if ( testInfo.ramPat != 0)
                    {
                        snprintf( testControl.results[i].msg, MAX_MSG_SIZE,
                            ">Loc:%x Wr:%x Rd:%x",
                            ramPatLocation, ramPatExp, ramPatAct);
                    }
                    else
                    {
                        snprintf( testControl.results[i].msg, MAX_MSG_SIZE,
                            ">Loc:%x Rd:%x",
                            ramAddLine, ramAddPat);
                    }
                }
                break;
            case eDpram:
                if ( msIsAlive == FALSE)
                {
                    if (testControl.msStartupTimer == 0 || MSSC_GetIsAlive())
                    {
                        msIsAlive = TRUE;
                        testControl.msStartupTimer = 0;
                        // now that we are alive clear the msg
                        snprintf( testControl.results[i].msg, MAX_MSG_SIZE, "> Running");
                    }
                    else
                    {
                        snprintf( testControl.results[i].msg,
                            MAX_MSG_SIZE, ">DPRAM HB Test Waiting for MS startup %d sec left",
                            testControl.msStartupTimer);
                    }
                }
                else
                {
                    if (testInfo.dpram == 0)
                    {
                        ++testControl.results[i].pass;
                    }
                    else
                    {
                        ++testControl.results[i].fail;
                        snprintf( testControl.results[i].msg,
                            MAX_MSG_SIZE, ">DPRAM HB Failure Count: %d",
                            testInfo.dpram);
                    }
                }
                break;
            case eAdc:
                for ( c=0; c < eMaxAdcSignal; ++c)
                {
                    if ( testInfo.adc[c] != 0)
                    {
                        chanFails |= (1u << c);
                    }
                }

                if (chanFails == 0)
                {
                    ++testControl.results[i].pass;
                }
                else
                {
                    ++testControl.results[i].fail;
                    adcChanFails |= chanFails;
                }

                snprintf( testControl.results[i].msg,
                    MAX_MSG_SIZE, ">Fails:0x%X Bat:%.1fv Bus:%.1fv LI:%.1fv Tmp:%.1f C",
                    adcChanFails,
                    testInfo.batVolt,
                    testInfo.busVolt,
                    testInfo.intVolt,
                    testInfo.tmpVolt
                );
                break;
            case eReset:
                break;
            case eNet:
            default:
                break;
            }
        }
    }

    // clear out the results
    memset( &testInfo, 0, sizeof( testInfo));
}

/******************************************************************************
* Function:     EtmUserMsg
* Description:
* Parameters:
* Returns:
* Notes:
*
*****************************************************************************/
USER_HANDLER_RESULT EtmUserMsg(USER_DATA_TYPE DataType,
                               USER_MSG_PARAM Param,
                               UINT32 Index,
                               const void *SetPtr,
                               void **GetPtr)
{
    USER_HANDLER_RESULT result = USER_RESULT_OK;

    //Get sensor structure based on index
    if ( (SINT32)Index != -1)
    {
        memcpy( &tempResults, &testControl.results[Index], sizeof(TestResults));
    }

    //If "get" command, point the GetPtr to the sensor config element defined by
    //Param
    if( SetPtr == NULL )
    {
        *GetPtr = Param.Ptr;
    }
    else
    {
        switch(DataType)
        {
        case USER_TYPE_STR:
            strncpy( Param.Ptr, SetPtr, sizeof(tempResults.msg));
            break;
        case USER_TYPE_BOOLEAN:
            *(BOOLEAN*)Param.Ptr = *(BOOLEAN*)SetPtr;
            break;
        case USER_TYPE_UINT32:
        case USER_TYPE_HEX32:
            *(UINT32*)Param.Ptr = *(UINT32*)SetPtr;
            break;
        default:
            result = USER_RESULT_ERROR;
            break;
        }

        if(USER_RESULT_OK == result)
        {
            if ( (SINT32)Index != -1)
            {
                memcpy( &testControl.results[Index], &tempResults, sizeof(TestResults));
            }

            if ( Param.Ptr == &testControl.active)
            {
                memcpy( &CfgMgr_ConfigPtr()->etm, &testControl, sizeof( testControl));

                //Store the modified temporary structure in the EEPROM.
                CfgMgr_StoreConfigItem( CfgMgr_ConfigPtr(),
                                        &CfgMgr_ConfigPtr()->etm,
                                        sizeof(testControl));
            }
        }
    }

    return result;
}

/******************************************************************************
* Function:     EtmUserMsg
* Description:
* Parameters:
* Returns:
* Notes:
*
*****************************************************************************/
USER_HANDLER_RESULT EtmShowA429(USER_DATA_TYPE DataType,
                               USER_MSG_PARAM Param,
                               UINT32 Index,
                               const void *SetPtr,
                               void **GetPtr)
{
    GSE_DebugStr(DBGOFF, TRUE,"TxWord[0]=%6d, TxWord[1]=%d\n", a429Tx[0], a429Tx[1]);
    
    GSE_DebugStr(DBGOFF, TRUE,"What     0      1      2      3");
    GSE_DebugStr(DBGOFF, TRUE,"-------- ------ ------ ------ ------");
    GSE_DebugStr(DBGOFF, TRUE,"RxCnt    %6d %6d %6d %6d", testInfo.a429Words[0], testInfo.a429Words[1], testInfo.a429Words[2], testInfo.a429Words[3]);
    GSE_DebugStr(DBGOFF, TRUE,"RxExp    %6d %6d %6d %6d", a429RxExpected[0],     a429RxExpected[1],     a429RxExpected[2],     a429RxExpected[3]);
    GSE_DebugStr(DBGOFF, TRUE,"TxFifo   %6d %6d %6d %6d", a429TxBufferCount[0],  a429TxBufferCount[1],  a429TxBufferCount[2],  a429TxBufferCount[3]);
    
    return USER_RESULT_OK;
}

/******************************************************************************
*  MODIFICATIONS
*    $History: Etm.c $
 * 
 * *****************  Version 16  *****************
 * User: Jeff Vahue   Date: 11/19/15   Time: 5:17p
 * Updated in $/software/control processor/code/test
 * Updates to ETM for New MS testing
 * 
 * *****************  Version 15  *****************
 * User: Jeff Vahue   Date: 7/14/15    Time: 10:27a
 * Updated in $/software/control processor/code/test
 * Update to run with v2.1.0
 *
 * *****************  Version 14  *****************
 * User: Jeff Vahue   Date: 4/14/11    Time: 7:04p
 * Updated in $/software/control processor/EnvTest/code/test
 * Fix DIO Tests and QAR Test
 *
 * *****************  Version 13  *****************
 * User: Jeff Vahue   Date: 4/13/11    Time: 1:06p
 * Updated in $/software/control processor/code/test
 * Update Env Test Software to work with code release v1.0.0
 *
 * *****************  Version 12  *****************
 * User: Jeff Vahue   Date: 7/19/10    Time: 6:50p
 * Updated in $/software/control processor/code/test
 * SCR# 707 - Code Coverage TP
 *
 * *****************  Version 11  *****************
 * User: Jeff Vahue   Date: 6/18/10    Time: 3:20p
 * Updated in $/software/control processor/code/test
 * Make the ETM code compatible with the current verison of the
 * Operational code.
 *
 * *****************  Version 10  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:12p
 * Updated in $/software/control processor/code/test
 * SCR #70 Store/Restore interrupt level
 *
 * *****************  Version 9  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:36p
 * Updated in $/software/control processor/code/test
 * SCR# 496 - move gse to system
 *
 * *****************  Version 8  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/test
 * SCR# 483 - Function Names
 *
 * *****************  Version 7  *****************
 * User: Contractor2  Date: 3/02/10    Time: 2:09p
 * Updated in $/software/control processor/code/test
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 6  *****************
 * User: Jeff Vahue   Date: 2/26/10    Time: 3:01p
 * Updated in $/software/control processor/code/test
 * SCR# 429 - change DIO_ReadPin to return the pin state
 *
 * *****************  Version 5  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 5:16p
 * Updated in $/software/control processor/code/test
 * SCR# 397
 *
 * *****************  Version 4  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:28p
 * Updated in $/software/control processor/code/test
 *
 * *****************  Version 3  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 2:12p
 * Updated in $/software/control processor/code/test
 * SCR# 326
 *
 * *****************  Version 2  *****************
 * User: Jeff Vahue   Date: 9/15/09    Time: 5:33p
 * Updated in $/software/control processor/code/test
*
*
*****************************************************************************/
#endif // ENV_TEST
