#ifndef TEST_POINTS_H
#define TEST_POINTS_H

/******************************************************************************
            Copyright (C) 2009-2014 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

  File:         TestPoints.h

  Description:  This file defines the test point capability used during system
                certification.

  VERSION
  $Revision: 45 $  $Date: 11/05/14 3:00p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"

/******************************************************************************
                             Package Defines
******************************************************************************/

#ifndef STE_TP
    #define STPU(actual,Id) (actual)
    #define STPUI(actual,tp,v)  (actual)
    #define TPU(actual,Id) (actual)
    #define TPS1(cmdId, Id)
    #define TPTR(Id)
#else
    #define STPU(actual,tp)  StartupTpUint( actual, tp)
    #define STPUI(actual,tp,v)  StartupTpUintInit( actual, tp, v)
    #define TPU(actual,tp)   TpUint( actual, tp)
    #define TPS1(cId, tp) if (cId == (UINT16)tpData[tp].cmdId) \
                          {UINT32 v = TpUint( 987654, tp);\
                           if (v != 987654) return v;}
    #define TPTR(tp) TpUint( 1, tp)
#endif

/******************************************************************************
                             Package Typedefs
******************************************************************************/

// NOTE: The numbers at the end of the enums are the SRS Requirement numbers
typedef enum {
    //>>>>>>>>>>>>>>>> Do not change the lines below <<<<<<<<<<<<<<<<<
    eTpiReserved  = 0, // reserved to indicate no TP active at startup
    //>>>>>>>>>>>>>>>> Do not change the lines above <<<<<<<<<<<<<<<<<
    //-----------------------------------------------------------------
    //-------- ADD StartUp ENUMS HERE & Rerun TestPointGen.py ---------
    //-----------------------------------------------------------------
    eTp429Fifo1r,  // A429 FIFO Rx Test
    eTp429Fifo1t,  // A429 FIFO Tx Test
    eTp429Lfr1,    // A429 LFR test
    eTp429Lfr2,    // A429 LFR test
    eTp429Rx1633,  // A429 Rx Loopback Test
    eTp429Rx1636,  // Internal registers
    eTp429Tx3106,  // A429 Tx Loopback Test
    eTpAcCfg3802,  // Assert Magic number
    eTpAcCfg3803,  // AC Cfg Magic number
    eTpAdc3807,    // ADC Register Test
    eTpCfg3747,    // Fail setting Cfg back to default
    eTpDpr3612,    // DPR Test Fail
    eTpDpr3776,    // DPR Register Test Fail
    eTpEe3730,     // EE device not detected, system eventually asserts
    eTpEe3730_1,   // Fails only one EEPROM device, needed to get SYSLOG
    eTpEeShDev,    // Fail a write back from shadow to dev
    eTpFlash3105a, // Corrupt Amd CFI - FlashGetCFIDeviceGeometry
    eTpFlash3105b, // Corrupt Amd CFI - FlashGetCFIDeviceGeometry
    eTpFlash3105c, // Corrupt Amd CFI - FlashGetCFISystemInterfaceData
    eTpFlash3105d, // Corrupt Amd CFI - FlashGetCFISystemInterfaceData
    eTpFpga3001,   // FPGA interrupt loopback
    eTpFpga3309,   // FPGA version test fail bad version #
    eTpFpga3580,   // FPGA not configured
    eTpFpga3583,   // FPGA register init fail
    eTpFpgaScr891, // FPGA SCR 891 Test
    eTpFpgaRegAll, // fail all FPGA register access/setup tests FPGA, A429, QAR
    eTpLog1871,    // Log corrupted on startup
    eTpLogCheck,   // LogCheck Status Control
    eTpLogCheck1,  // LogCheck Status Control 1
    eTpLogPend,    // Control LogPending flow
    eTpPsc1579,    // PSC Reg Access Test
    eTpPsc1580,    // UART Loop back test
    eTpQar2778,    // QAR Loopback (Rx Test)
    eTpQar3774,    // QAR Memory Test
    eTpRtc1296a,   // RTC Time Error - Path A - can also be used during OpMode
    eTpRtc1299,    // RTC Register Test
    eTpRtc1302,    // RTC time incrementing
    eTpSpi3805,    // SPI Register Test
    eTpTtmr3806,   // Timer Reg Test
    eTpBto3947,    // Bus Timeout Reg Test
    eTpFlashDirect,// FlashGetDirectStatus

    //>>>>>>>>>>>>>>>> Do not change the lines below <<<<<<<<<<<<<<<<<
    eTpMaxStartUp,
    eTpOpMode,
    //>>>>>>>>>>>>>>>> Do not change the lines above <<<<<<<<<<<<<<<<<
    //-----------------------------------------------------------------
    //-------- ADD OpMode ENUMS HERE & Rerun TestPointGen.py ----------
    //-----------------------------------------------------------------
    eTpAmd1503,    // Control AMD Status
    eTpChipErase,  // Force Chip Erase timeout
    eTpCrc1806,    // CRC failure
    eTpDio2481,    // DIO wrap around check
    eTpDprIsr3609, // DPR ISR Monitor
    eTpFpga2997,   // FPGA Cfg retry
    eTpFpga2997a,  // FPGA reload delay
    eTpFpga3703,   // FPGA Interrupt Monitor - test duration 25ms
    eTpGseRead,    // Control GseRead response Ymodem
    eTpGseWrite,   // Control GseWrite Response Ymodem
    eTpLogMark,    // Use as a trigger command to indicate that Log Mark Task running
    eTpMem3104,    // Memory Write Assert
    eTpMem3793,    // Force Log Read Error
    eTpMemCheck,   // force error when running MemCheckAddr
    eTpMemRead,    // Control MemRead Status
    eTpMemWord,    // Force the Write to think its crossing a sector
    eTpMemWord1,   // need to set size to 4
    eTpMsiPutCmd,  // Fail MSI_PutCommand
    eTpPscIsr3711, // PSC ISR Monitor
    eTpRam1811,    // RAM Bit Stuck hi/lo
    eTpRam3162a,   // RAM Test Pattern 1
    eTpRam3162b,   // RAM Test Pattern 2
    eTpRtc1296b,   // RTC Time Error - Path B
    eTpSpiRd1251,  // SPI Read Timeout
    eTpSpiWr1256,  // SPI Write Timeout
    eTpAdc3931,    // ADC CBIT Test fail

    eTpCreateTask, // create a task after init is done
    eTpDtOverrun,  // Cause DT overrun
    eTpNoDtOverrun,// Inhibit DT overruns during testing
    eTpModeNormal, // switch to Normal
    eTpModeTest,   // switch to Test Mode
    eTpRmtMode,    // run RMT for 10s
    eTpRmtOverrun, // Cause RMT overrun
    eTpUnexpected, // unexpected interrupt
    eTpUldScr1245, // UploadMgr SCR 1245 - END_LOG timeout
    eTpLogFull,    // Control Log Flash Full flow
    eTpLogQFull,   // Control Log Queue Full flow


//>>>>>>>>>>>>>>>> Do not change the lines below <<<<<<<<<<<<<<<<<
    eTpMaxValue
} TEST_POINTS;
//>>>>>>>>>>>>>>>> Do not change the lines above <<<<<<<<<<<<<<<<<

typedef enum {
    TP_INACTIVE,  // TP is inactive
    TP_VALUE,     // TP returns a fixed value
    TP_INC,       // TP increments it return value by one
    TP_TOGGLE,    // TP toggles actual and TP value
    TP_TOGGLE_X,  // TP alternates actual and TP value complex
    TP_TRIGGER,   // TP trigger another TP
    TP_ENUM_MAX
} TEST_POINT_ENUM;

typedef union {
    INT32   iVal;
    UINT32  uVal;
    FLOAT32 fVal;
} TEST_POINT_RETURN;

// Test Point Control
// Test Point Setup sets all values and THEN set the 'type'.
// Once the Type is set the counting begins so setup THEN enter 'type'
typedef struct{
    TEST_POINT_ENUM   type;        // type of TP
    TEST_POINT_RETURN rVal;        // value of TP
    INT32             wait;        // wait for M calls to activate TP
    INT32             repeat;      // repeat for N calls of TP when active, the deactivate
    BOOLEAN           toggleState; // indicates which value to return in toggle True=Actual, False=TpValue
    INT32             toggleMax;   // Toggle Cycle Max
    INT32             toggleLim;   // Toggle Lim when TP goes to Actual
    INT32             toggleCur;   // Toggle Cycle position
    UINT32            cmdId;       // S1 Test Point - for MSI_PutCommand & Target TP for Trigger type
} TEST_POINT_DATA;                 // needs to be updated to the terminal

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined( TEST_POINT_BODY )
  #define EXPORT
#else
  #define EXPORT extern
#endif

/******************************************************************************
                             Package Exports Variables
******************************************************************************/
EXPORT  TEST_POINT_DATA tpData[eTpMaxValue];

/******************************************************************************
                             Package Exports Functions
******************************************************************************/
EXPORT  void   TestPointInit( void);

EXPORT  UINT32 TpUint(UINT32 actual, TEST_POINTS tpId);
EXPORT  UINT32 StartupTpUint( UINT32 actual, TEST_POINTS tpId);
EXPORT  UINT32 StartupTpUintInit( UINT32 actual, TEST_POINTS tpId, UINT32 iValue);

/*************************************************************************
 *  MODIFICATIONS
 *    $History: TestPoints.h $
 *
 * *****************  Version 45  *****************
 * User: Contractor2  Date: 11/05/14   Time: 3:00p
 * Updated in $/software/control processor/code/test
 * SCR-1274: v2.1.0 Instrumented Test Point additions
 * Test for SCR-1245: File Upload command "END_LOG" does not timeout
 * properly
 *
 * *****************  Version 44  *****************
 * User: Jeff Vahue   Date: 10/27/11   Time: 9:49p
 * Updated in $/software/control processor/code/test
 * SCR# 1077 - test point
 *
 * *****************  Version 43  *****************
 * User: Jeff Vahue   Date: 10/27/11   Time: 7:46p
 * Updated in $/software/control processor/code/test
 * SCR# 1077 - Test Points
 *
 * *****************  Version 42  *****************
 * User: Jeff Vahue   Date: 10/17/11   Time: 7:45p
 * Updated in $/software/control processor/code/test
 * One more test point and a testpoint id fix
 *
 * *****************  Version 41  *****************
 * User: Jeff Vahue   Date: 10/11/11   Time: 2:10p
 * Updated in $/software/control processor/code/test
 * SCR# 1077 - ADC CBIT Test
 *
 * *****************  Version 40  *****************
 * User: Jeff Vahue   Date: 9/24/11    Time: 8:53a
 * Updated in $/software/control processor/code/test
 * SCR# 1077 - Test Points v1.1.0
 *
 * *****************  Version 39  *****************
 * User: Contractor2  Date: 6/27/11    Time: 1:58p
 * Updated in $/software/control processor/code/test
 * SCR #515 Enhancement - sys.get.mem.l halts processor when invalid
 * memory is referenced
 *
 * *****************  Version 38  *****************
 * User: Jeff Vahue   Date: 11/18/10   Time: 8:18p
 * Updated in $/software/control processor/code/test
 * TP Coverage
 *
 * *****************  Version 37  *****************
 * User: Jeff Vahue   Date: 11/15/10   Time: 8:16p
 * Updated in $/software/control processor/code/test
 *
 * *****************  Version 36  *****************
 * User: Jeff Vahue   Date: 11/12/10   Time: 9:54p
 * Updated in $/software/control processor/code/test
 * SCR# 848 - Code Coverage
 *
 * *****************  Version 35  *****************
 * User: Jeff Vahue   Date: 11/11/10   Time: 9:55p
 * Updated in $/software/control processor/code/test
 * SCR# 848 - Code Coverage
 *
 * *****************  Version 34  *****************
 * User: Jeff Vahue   Date: 11/10/10   Time: 11:18p
 * Updated in $/software/control processor/code/test
 *
 * *****************  Version 33  *****************
 * User: Jeff Vahue   Date: 11/10/10   Time: 11:04p
 * Updated in $/software/control processor/code/test
 * SCR# 848 - Code Coverage
 *
 * *****************  Version 32  *****************
 * User: Jeff Vahue   Date: 11/09/10   Time: 9:08p
 * Updated in $/software/control processor/code/test
 * SCR# 848 - Code Coverage
 *
 * *****************  Version 31  *****************
 * User: Jeff Vahue   Date: 11/09/10   Time: 6:05p
 * Updated in $/software/control processor/code/test
 * SCR# 848 - Code Coverage
 *
 * *****************  Version 30  *****************
 * User: Jeff Vahue   Date: 11/05/10   Time: 9:48p
 * Updated in $/software/control processor/code/test
 * SCR# 848 - Code Coverage
 *
 * *****************  Version 29  *****************
 * User: Jeff Vahue   Date: 11/03/10   Time: 7:03p
 * Updated in $/software/control processor/code/test
 * SCR# 848 - Code Coverage
 *
 * *****************  Version 28  *****************
 * User: Jeff Vahue   Date: 11/01/10   Time: 8:38p
 * Updated in $/software/control processor/code/test
 * Code Coverage
 *
 * *****************  Version 27  *****************
 * User: Jeff Vahue   Date: 11/01/10   Time: 7:50p
 * Updated in $/software/control processor/code/test
 * Code Coverage TP
 *
 * *****************  Version 26  *****************
 * User: Jeff Vahue   Date: 10/19/10   Time: 7:25p
 * Updated in $/software/control processor/code/test
 * SCR# 848 - Code Coverage
 *
 * *****************  Version 25  *****************
 * User: Jeff Vahue   Date: 10/18/10   Time: 8:31p
 * Updated in $/software/control processor/code/test
 * SCR# 848 - Code Coverage
 *
 * *****************  Version 24  *****************
 * User: Jeff Vahue   Date: 10/18/10   Time: 7:42p
 * Updated in $/software/control processor/code/test
 * SCR# 848 - Code Coverage
 *
 * *****************  Version 23  *****************
 * User: Jeff Vahue   Date: 10/18/10   Time: 7:03p
 * Updated in $/software/control processor/code/test
 * SCR# 848: Code Coverage
 *
 * *****************  Version 22  *****************
 * User: Jeff Vahue   Date: 10/17/10   Time: 2:18p
 * Updated in $/software/control processor/code/test
 * SCR# 848 - Code Coverage
 *
 * *****************  Version 21  *****************
 * User: Jeff Vahue   Date: 10/14/10   Time: 8:13p
 * Updated in $/software/control processor/code/test
 * SCR# 848 - Code Coverage
 *
 * *****************  Version 20  *****************
 * User: Jeff Vahue   Date: 10/14/10   Time: 2:27p
 * Updated in $/software/control processor/code/test
 * SCR# 848 - Code Coverage
 *
 * *****************  Version 19  *****************
 * User: Jeff Vahue   Date: 10/07/10   Time: 6:29p
 * Updated in $/software/control processor/code/test
 *
 * *****************  Version 18  *****************
 * User: Jeff Vahue   Date: 10/07/10   Time: 4:16p
 * Updated in $/software/control processor/code/test
 * Code Coverage
 *
 * *****************  Version 17  *****************
 * User: Jeff Vahue   Date: 10/02/10   Time: 5:08p
 * Updated in $/software/control processor/code/test
 * SCR# 848 - Code Coverage
 *
 * *****************  Version 16  *****************
 * User: Jeff Vahue   Date: 10/01/10   Time: 6:14p
 * Updated in $/software/control processor/code/test
 *
 * *****************  Version 15  *****************
 * User: Jeff Vahue   Date: 9/23/10    Time: 5:40p
 * Updated in $/software/control processor/code/test
 * Code Cov Update
 *
 * *****************  Version 14  *****************
 * User: Jeff Vahue   Date: 9/21/10    Time: 5:48p
 * Updated in $/software/control processor/code/test
 * SCR #848 - Code Cov
 *
 * *****************  Version 13  *****************
 * User: Jeff Vahue   Date: 9/14/10    Time: 8:21p
 * Updated in $/software/control processor/code/test
 * SCR# 848 - Code Coverage Mods
 *
 * *****************  Version 12  *****************
 * User: Jeff Vahue   Date: 9/13/10    Time: 4:07p
 * Updated in $/software/control processor/code/test
 *
 * *****************  Version 11  *****************
 * User: Jeff Vahue   Date: 9/03/10    Time: 1:51p
 * Updated in $/software/control processor/code/test
 * Another TP
 *
 * *****************  Version 10  *****************
 * User: Jeff Vahue   Date: 9/03/10    Time: 12:24p
 * Updated in $/software/control processor/code/test
 * Another TP
 *
 * *****************  Version 9  *****************
 * User: Jeff Vahue   Date: 8/29/10    Time: 4:16p
 * Updated in $/software/control processor/code/test
 * Fix Tp Number for test point eQar3774
 *
 * *****************  Version 8  *****************
 * User: Jeff Vahue   Date: 8/09/10    Time: 1:42p
 * Updated in $/software/control processor/code/test
 * SCR# 707 - V&V TP Update
 *
 * *****************  Version 7  *****************
 * User: Jeff Vahue   Date: 8/02/10    Time: 4:53p
 * Updated in $/software/control processor/code/test
 * SCR# 707 - Add test Points
 *
 * *****************  Version 6  *****************
 * User: Jeff Vahue   Date: 7/23/10    Time: 8:14p
 * Updated in $/software/control processor/code/test
 * SCR# 707 - More TP code
 *
 * *****************  Version 5  *****************
 * User: Jeff Vahue   Date: 7/19/10    Time: 6:51p
 * Updated in $/software/control processor/code/test
 * SCR# 707 - Code Coverage TP
 *
 * *****************  Version 4  *****************
 * User: Jeff Vahue   Date: 7/17/10    Time: 5:46p
 * Updated in $/software/control processor/code/test
 * SCR# 707 - Add TestPoints for code coverage.
 *
 * *****************  Version 3  *****************
 * User: Jeff Vahue   Date: 7/16/10    Time: 7:25p
 * Updated in $/software/control processor/code/test
 * Code Coverage development
 *
 * *****************  Version 2  *****************
 * User: Contractor2  Date: 3/02/10    Time: 2:09p
 * Updated in $/software/control processor/code/test
 * SCR# 472 - Fix file/function header
 *
 *************************************************************************/

#endif // TEST_POINTS_H





