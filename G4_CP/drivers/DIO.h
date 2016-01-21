#ifndef DIO_H
#define DIO_H
/******************************************************************************
            Copyright (C) 2007-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

  File:        DIO.h

  Description: This header file includes the variables and funtions necessary
               for acceess to all GPIO discrete pins specific to the FAST II.
               See the c module for a detailed description.

  VERSION
      $Revision: 46 $  $Date: 1/21/16 4:35p $
******************************************************************************/


/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include "mcf548x.h"
#include "ResultCodes.h"
#include "FPGA.h"
#include "SystemLog.h"
#include "LogManager.h"

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/

/******************************************************************************
                                 Package Defines                              *
******************************************************************************/
#define PIN_LABEL_LEN 24
#define DEBOUNCE_PERIOD_MS 50
#define DISP_DISCRETE_BASE 0x3179ff34 
#define DISP_GPIO_0 (volatile UINT8 *)(DISP_DISCRETE_BASE + 0x00)
#ifndef WIN32
    #define DIO_R(a) *(a)
    #define DIO_W(a,m,o) (*(a) = ((o) == DIO_SetHigh) ? (*(a) | (m)) : (*(a) & ~(m)))
    #define DIO_S(a,v) (*(a) = (v))

    #define DIO_R16(a) *(a)
    #define DIO_W16(a,m,o) (*(a) = ((o) == DIO_SetHigh) ? (*(a) | (m)) : (*(a) & ~(m)))
    #define DIO_S16(a,v) (*(a) = (v))
#else
    #define DIO_R(a) hw_DioRead(a)
    #define DIO_W(a,m,o) hw_DioWrite(a,m,o)
    #define DIO_S(a,v) hw_DioSet(a,v)

    #define DIO_R16(a) hw_DioRead16(a)
    #define DIO_W16(a,m,o) hw_DioWrite16(a,m,o)
    #define DIO_S16(a,v) hw_DioSet16(a,v)
#endif
//Input and output pin lists are used to generate an enumerated type.
//
//The purpose of creating the list in this convoluted manner is that it allows
//the same list to generate both an enumerated type, a string list that
//consists of the stringified enum labels, and also contain the initializers for
//each pin's data structure.
//
//Adding, removing or modifying a pin definition below will automatically
//changes the input or output enumerated type and the const array that holds
//the pin configuration at run time.
#undef DIO_PIN           //Set the pin macro to return labels for Enumerated Lists
#define DIO_PIN(Name,Dir,State,Periph,AccMethod,Addr,Bm) Name,




//NOTE: DRV Init PBIT Test DRV_DIO_REGISTER_INIT_FAIL (display of
//      test results) is dependent on the ordering of the DIO below.
//      New DIO should be added to the end of the list (to prevent having
//      to constantly update the PBIT log bit description).
//

#define DIO_INPUTS_LIST\
/*  Name (<24 chars).....Dir      Init  Peripheral Access Method  PODR addr         BitMask*/\
DIO_PIN(Discrete0,       DIO_In,  OFF,  DIO_GPIO, DIO_FILTERED,   &MCF_GPIO_PODR_FEC0H, 0x01)\
DIO_PIN(Discrete1,       DIO_In,  OFF,  DIO_GPIO, DIO_FILTERED,   &MCF_GPIO_PODR_FEC0H, 0x02)\
DIO_PIN(Discrete2,       DIO_In,  OFF,  DIO_GPIO, DIO_FILTERED,   &MCF_GPIO_PODR_FEC0H, 0x04)\
DIO_PIN(Discrete3,       DIO_In,  OFF,  DIO_GPIO, DIO_FILTERED,   &MCF_GPIO_PODR_FEC0H, 0x08)\
DIO_PIN(Discrete4,       DIO_In,  OFF,  DIO_GPIO, DIO_FILTERED,   &MCF_GPIO_PODR_FEC0H, 0x10)\
DIO_PIN(Discrete5,       DIO_In,  OFF,  DIO_GPIO, DIO_FILTERED,   &MCF_GPIO_PODR_FEC0H, 0x20)\
DIO_PIN(Discrete6,       DIO_In,  OFF,  DIO_GPIO, DIO_FILTERED,   &MCF_GPIO_PODR_FEC0H, 0x40)\
DIO_PIN(Discrete7,       DIO_In,  OFF,  DIO_GPIO, DIO_FILTERED,   &MCF_GPIO_PODR_FEC0H, 0x80)\
DIO_PIN(LSS0_WA,         DIO_In,  OFF,  DIO_GPIO, DIO_RAW,        &MCF_GPIO_PODR_FEC0L, 0x01)\
DIO_PIN(LSS1_WA,         DIO_In,  OFF,  DIO_GPIO, DIO_RAW,        &MCF_GPIO_PODR_FEC0L, 0x02)\
DIO_PIN(LSS2_WA,         DIO_In,  OFF,  DIO_GPIO, DIO_RAW,        &MCF_GPIO_PODR_FEC0L, 0x04)\
DIO_PIN(LSS3_WA,         DIO_In,  OFF,  DIO_GPIO, DIO_RAW,        &MCF_GPIO_PODR_FEC0L, 0x08)\
DIO_PIN(TP48,            DIO_In,  OFF,  DIO_GPIO, DIO_RAW,        &MCF_GPIO_PODR_FEC1H, 0x01)\
DIO_PIN(TP49,            DIO_In,  OFF,  DIO_GPIO, DIO_RAW,        &MCF_GPIO_PODR_FEC1H, 0x02)\
DIO_PIN(FPGA_Err,        DIO_In,  OFF,  DIO_GPIO, DIO_RAW,        &MCF_GPIO_PODR_FEC1H, 0x08)\
DIO_PIN(FPGA_Cfg,        DIO_In,  OFF,  DIO_GPIO, DIO_RAW,        &MCF_GPIO_PODR_FEC1H, 0x10)\
DIO_PIN(HWPFEN_Sta,      DIO_In,  OFF,  DIO_GPIO, DIO_FILTERED,   &MCF_GPIO_PODR_FEC1L, 0x01)\
DIO_PIN(LSS_OvI,         DIO_In,  OFF,  DIO_GPIO, DIO_RAW,        &MCF_GPIO_PODR_FEC1L, 0x08)\
DIO_PIN(WOW,             DIO_In,  OFF,  DIO_GPIO, DIO_FILTERED,   &MCF_GPIO_PODR_FEC1L, 0x20)\
DIO_PIN(WLAN_WOW_Enb,    DIO_In,  OFF,  DIO_GPIO, DIO_FILTERED,   &MCF_GPIO_PODR_FECI2C,0x04)\
DIO_PIN(SW_PFEN_Sta,     DIO_In,  OFF,  DIO_GPIO, DIO_RAW,        &MCF_GPIO_PODR_FECI2C,0x08)\
/*FPGA GPIO input.  Typecast 16-bit GPIO_1 reg to fit in 8-bit ptr type. Should think about\
  adding reg width column in the future if more 16-but registers are needed*/\
DIO_PIN(FFD_Enb,         DIO_In,  ON,   DIO_FPGA, DIO_RAW,        FPGA_GPIO_1, FPGA_GPIO_1_FFDI)\
DIO_PIN(DispDisc0,       DIO_In,  OFF,  DIO_DISP, DIO_RAW,        NULL,                 0x00)\
DIO_PIN(DispDisc1,       DIO_In,  OFF,  DIO_DISP, DIO_RAW,        NULL,                 0x01)\
DIO_PIN(DispDisc2,       DIO_In,  OFF,  DIO_DISP, DIO_RAW,        NULL,                 0x02)\
DIO_PIN(DispDisc3,       DIO_In,  OFF,  DIO_DISP, DIO_RAW,        NULL,                 0x03)\
DIO_PIN(DispDisc4,       DIO_In,  OFF,  DIO_DISP, DIO_RAW,        NULL,                 0x04)\
DIO_PIN(DispDisc5,       DIO_In,  OFF,  DIO_DISP, DIO_RAW,        NULL,                 0x05)\
DIO_PIN(DispDisc6,       DIO_In,  OFF,  DIO_DISP, DIO_RAW,        NULL,                 0x06)\
DIO_PIN(DispDisc7,       DIO_In,  OFF,  DIO_DISP, DIO_RAW,        NULL,                 0x07)


#define DIO_OUTPUTS_LIST \
/*  Name (<24 chars).... Dir      Init  Peripheral Access Method  PODR addr         BitMask*/\
DIO_PIN(LSS0,            DIO_Out, OFF,  DIO_GPIO, DIO_NOT_APPLIC, &MCF_GPIO_PODR_FEC0L, 0x10)\
DIO_PIN(LSS1,            DIO_Out, OFF,  DIO_GPIO, DIO_NOT_APPLIC, &MCF_GPIO_PODR_FEC0L, 0x20)\
DIO_PIN(LSS2,            DIO_Out, OFF,  DIO_GPIO, DIO_NOT_APPLIC, &MCF_GPIO_PODR_FEC0L, 0x40)\
DIO_PIN(LSS3,            DIO_Out, OFF,  DIO_GPIO, DIO_NOT_APPLIC, &MCF_GPIO_PODR_FEC0L, 0x80)\
DIO_PIN(FPGAReconfig,    DIO_Out, ON,   DIO_GPIO, DIO_NOT_APPLIC, &MCF_GPIO_PODR_FEC1H, 0x20)\
DIO_PIN(WLANEnb,         DIO_Out, OFF,  DIO_GPIO, DIO_NOT_APPLIC, &MCF_GPIO_PODR_FEC1H, 0x40)\
DIO_PIN(GSMEnb,          DIO_Out, OFF,  DIO_GPIO, DIO_NOT_APPLIC, &MCF_GPIO_PODR_FEC1H, 0x80)\
DIO_PIN(TP73,            DIO_Out, OFF,  DIO_GPIO, DIO_NOT_APPLIC, &MCF_GPIO_PODR_FEC1L, 0x40)\
DIO_PIN(LiBattToADC,     DIO_Out, OFF,  DIO_GPIO, DIO_NOT_APPLIC, &MCF_GPIO_PODR_FEC1L, 0x80)\
DIO_PIN(TP67,            DIO_Out, OFF,  DIO_GPIO, DIO_NOT_APPLIC, &MCF_GPIO_PODR_PCIBR, 0x01)\
DIO_PIN(TP68,            DIO_Out, OFF,  DIO_GPIO, DIO_NOT_APPLIC, &MCF_GPIO_PODR_PCIBR, 0x02)\
DIO_PIN(ARINC429_Pwr,    DIO_Out, OFF,  DIO_GPIO, DIO_NOT_APPLIC, &MCF_GPIO_PODR_PCIBR, 0x04)\
DIO_PIN(LED_CFG,         DIO_Out, ON,   DIO_GPIO, DIO_NOT_APPLIC, &MCF_GPIO_PODR_PCIBR, 0x08)\
DIO_PIN(LED_FLT,         DIO_Out, ON,   DIO_GPIO, DIO_NOT_APPLIC, &MCF_GPIO_PODR_PCIBR, 0x10)\
DIO_PIN(SW_PFEN1,        DIO_Out, OFF,  DIO_GPIO, DIO_NOT_APPLIC, &MCF_GPIO_PODR_PCIBG, 0x01)\
DIO_PIN(SW_PFEN2,        DIO_Out, ON,   DIO_GPIO, DIO_NOT_APPLIC, &MCF_GPIO_PODR_PCIBG, 0x02)\
DIO_PIN(FPGARst,         DIO_Out, ON,   DIO_GPIO, DIO_NOT_APPLIC, &MCF_GPIO_PODR_PCIBG, 0x04)\
DIO_PIN(LED_XFR,         DIO_Out, ON,   DIO_GPIO, DIO_NOT_APPLIC, &MCF_GPIO_PODR_PCIBG, 0x08)\
DIO_PIN(LED_STS,         DIO_Out, ON,   DIO_GPIO, DIO_NOT_APPLIC, &MCF_GPIO_PODR_PCIBG, 0x10)\
DIO_PIN(PowerHold,       DIO_Out, OFF,  DIO_GPIO, DIO_NOT_APPLIC, &MCF_GPIO_PODR_PSC1PSC0,0x04)\
DIO_PIN(GSE_TxEnb,       DIO_Out, OFF,  DIO_GPIO, DIO_NOT_APPLIC, &MCF_GPIO_PODR_PSC1PSC0,0x08)\
DIO_PIN(ACS1_DX,         DIO_Out, OFF,  DIO_GPIO, DIO_NOT_APPLIC, &MCF_GPIO_PODR_PSC1PSC0,0x40)\
DIO_PIN(ACS1_TxEnb,      DIO_Out, OFF,  DIO_GPIO, DIO_NOT_APPLIC, &MCF_GPIO_PODR_PSC1PSC0,0x80)\
DIO_PIN(ACS2_DX,         DIO_Out, OFF,  DIO_GPIO, DIO_NOT_APPLIC, &MCF_GPIO_PODR_PSC3PSC2,0x04)\
DIO_PIN(ACS2_TxEnb,      DIO_Out, OFF,  DIO_GPIO, DIO_NOT_APPLIC, &MCF_GPIO_PODR_PSC3PSC2,0x08)\
DIO_PIN(ACS3_DX,         DIO_Out, OFF,  DIO_GPIO, DIO_NOT_APPLIC, &MCF_GPIO_PODR_PSC3PSC2,0x40)\
DIO_PIN(ACS3_TxEnb,      DIO_Out, OFF,  DIO_GPIO, DIO_NOT_APPLIC, &MCF_GPIO_PODR_PSC3PSC2,0x80)\
DIO_PIN(MSRst,           DIO_Out, ON,   DIO_TMR2, DIO_NOT_APPLIC, NULL                   ,0xFF)\
/* Note: for ACS1_232_485 control,\
         DIO Init does not actually init this pin, there is code in FPGA Init*/\
/*reference SCR #601 */\
DIO_PIN(ACS1_232_485,   DIO_Out, ON,   DIO_FPGA, DIO_NOT_APPLIC, (void*)FPGA_GPIO_0    ,FPGA_GPIO_0_232_485)


/******************************************************************************
                                 Package Typedefs                             *
******************************************************************************/
//Enumeration for input ports as defined in FAST-T-310-1
typedef enum{
  DIO_INPUTS_LIST
  DIO_MAX_INPUTS
}DIO_INPUT;

//Enumeration for output ports as defined in FAST-T-310-1
typedef enum{
  DIO_OUTPUTS_LIST
  DIO_MAX_OUTPUTS
}DIO_OUTPUT;

#ifdef SUPPORT_FPGA_BELOW_V12
// following restores support for old hardware
#define CAN0_Err  TP48
#define CAN1_Err  TP49
#define CAN0Enb   TP67
#define CAN1Enb   TP68
#define WDog      TP73
#endif

#undef DIO_PIN       //All done making enums, release the list macro

typedef enum{        //For defining a pin's direction
 DIO_In = 0,
 DIO_Out = 1
} DIO_DIRECTION;

typedef enum{        //Output operation, for calls to "SetOutput"
  DIO_SetLow,
  DIO_SetHigh,
} DIO_OUT_OP;

typedef enum{
  DIO_GPIO,
  DIO_TMR2,
  DIO_FPGA,
  DIO_DISP,
}DIO_PERIPHERAL;

typedef enum{         // How should discrete values be accessed.
  DIO_NOT_APPLIC,     // Not used/not-applicable. Setting for DOUT values.
  DIO_RAW,            // Get discrete value directly from register.
  DIO_FILTERED        // Used the filtered value from the debounced table.
}DIO_ACC_METHOD;

//Defines the configuration a single discrete input/output
typedef struct {
  CHAR name[PIN_LABEL_LEN];  //Pin label (for reporting errors, status, etc)
  DIO_DIRECTION direction;   //Direction of the bit, input or output
  BOOLEAN  bInitialState;     //For outputs, this is the initial programmed state of the pin
  DIO_PERIPHERAL peripheral; //Peripheral (GPIO, TMR, FPGA) that contains pin control register
  DIO_ACC_METHOD accessMethod; //Access method(NOT_APPLIC, RAW, FILTERED)
  volatile UINT8* dataReg;   //Port data register (PODR_X)(output register read/write)
                             //..other registers associated with the port are
                             //inferred PODR address.
  UINT32 pinMask;            //Bit mask for the bit that is used by this DIO
} DIO_CONFIG;


typedef struct               // Fast-lookup structure of port addr and
{                            // the current value.
  volatile UINT8* portAddr;  // PODR addr obtained from DIO_CONFIG
  UINT8 value;               // value returned from call to DIO_R.
} DIO_PORT_DATA;

typedef struct {             // Structure for storing hysteresis info for DIN
  BOOLEAN filteredState;     // The official 'debounced' state of the DIO
  BOOLEAN lastRecvdState;    // The most recently received DIO state from PODR
  UINT32  timeStampMs;       // Timestamp in milliseconds when the lastRecvdState changed.
  DIO_PORT_DATA* portData;   // Pointer to the DIO_PORT_DATA containing data for this discrete.
}DIO_DEBOUNCED;



#pragma pack(1)
typedef struct {
  RESULT  result;
  UINT32  dioOutResults;
  UINT32  dioInResults;
} DIO_DRV_PBIT_LOG;

typedef struct {
  DIO_OUTPUT doutPin;
  BOOLEAN    outputState;
  DIO_INPUT  dinPin;
  BOOLEAN    inputWAState;
} DIO_WRAPAROUND_FAIL_LOG;
#pragma pack()

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( DIO_BODY )
  #define EXPORT
#else
  #define EXPORT extern
#endif


/******************************************************************************
                             Package Exports Variables
******************************************************************************/

/******************************************************************************
                             Package Exports Functions
******************************************************************************/
EXPORT  RESULT  DIO_Init     ( SYS_APP_ID *SysLogId, void *pdata, UINT16 *psize );
EXPORT  void    DIO_SetPin   ( DIO_OUTPUT Pin, DIO_OUT_OP Op) ;
EXPORT  BOOLEAN DIO_ReadPin  ( DIO_INPUT Pin);
EXPORT  FLOAT32 DIO_GetValue ( UINT16 Pin, UINT32 *null );
EXPORT  void    DIO_GetOutputPin(DIO_OUTPUT Pin, BOOLEAN *PinState );

EXPORT  void    DIO_UpdateDiscreteInputs ( void );
EXPORT  void    DIO_DispProtocolSetAddress(const char* address, UINT16 discreteMax);
EXPORT  BOOLEAN DIO_SensorTest(UINT16 nIndex);
EXPORT  void    DIO_DispValidationFlags(const char* flagAddress);

#endif // DIO_H


/*************************************************************************
 *  MODIFICATIONS
 *    $History: DIO.h $
 * 
 * *****************  Version 46  *****************
 * User: John Omalley Date: 1/21/16    Time: 4:35p
 * Updated in $/software/control processor/code/drivers
 * SCR 1302 - Display Protocol Discrete Inputs
 * 
 * *****************  Version 45  *****************
 * User: John Omalley Date: 12-12-11   Time: 4:27p
 * Updated in $/software/control processor/code/drivers
 * SCR 1197 - Code Review Updates
 * 
 * *****************  Version 44  *****************
 * User: John Omalley Date: 12-12-11   Time: 2:07p
 * Updated in $/software/control processor/code/drivers
 * SCR 1197 - Code Review Update
 * 
 * *****************  Version 43  *****************
 * User: Jim Mood     Date: 12/05/12   Time: 8:07p
 * Updated in $/software/control processor/code/drivers
 * SCR 1153.  Reg Check failure caused by new FPGA FFD Input
 *
 * *****************  Version 42  *****************
 * User: John Omalley Date: 12-11-14   Time: 7:12p
 * Updated in $/software/control processor/code/drivers
 * SCR 1076 - Code Review Updates
 *
 * *****************  Version 41  *****************
 * User: Melanie Jutras Date: 12-11-01   Time: 3:19p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 File Format Errors
 *
 * *****************  Version 40  *****************
 * User: Peter Lee    Date: 12-10-27   Time: 5:09p
 * Updated in $/software/control processor/code/drivers
 * SCR #1191 Returns update time of param
 *
 * *****************  Version 39  *****************
 * User: Jim Mood     Date: 10/23/12   Time: 10:50a
 * Updated in $/software/control processor/code/drivers
 * SCR 1107: Rename FFD input in the dio input table
 *
 * *****************  Version 38  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 7:08p
 * Updated in $/software/control processor/code/drivers
 * SCR# 1142 - Windows Emulation Update and Typos
 *
 * *****************  Version 37  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 36  *****************
 * User: Jim Mood     Date: 7/19/12    Time: 10:42a
 * Updated in $/software/control processor/code/drivers
 * SCR 1107: Data Offload changes for 2.0.0
 *
 * *****************  Version 35  *****************
 * User: Jim Mood     Date: 10/18/10   Time: 11:59a
 * Updated in $/software/control processor/code/drivers
 * SCR 943
 *
 * *****************  Version 34  *****************
 * User: John Omalley Date: 10/12/10   Time: 5:14p
 * Updated in $/software/control processor/code/drivers
 * SCR 932 - Remove DOUT CANWake
 *
 * *****************  Version 33  *****************
 * User: Contractor2  Date: 9/27/10    Time: 2:55p
 * Updated in $/software/control processor/code/drivers
 * SCR #795 Code Review Updates
 *
 * *****************  Version 32  *****************
 * User: Contractor2  Date: 9/24/10    Time: 3:02p
 * Updated in $/software/control processor/code/drivers
 * SCR #882 WD - Old Code for pulsing the WD should be removed
 *
 * *****************  Version 31  *****************
 * User: Jim Mood     Date: 9/07/10    Time: 6:57p
 * Updated in $/software/control processor/code/drivers
 * SCR 802 Remove pflash and dflash dio inputs
 *
 * *****************  Version 30  *****************
 * User: Contractor V&v Date: 7/07/10    Time: 6:19p
 * Updated in $/software/control processor/code/drivers
 * SCR #673 Error - On init GSM Output Pin Enb should be OFF
 *
 * *****************  Version 29  *****************
 * User: Peter Lee    Date: 5/31/10    Time: 5:46p
 * Updated in $/software/control processor/code/drivers
 * SCR #601 FPGA DIO Logic Updates/Bug fixes
 *
 * *****************  Version 28  *****************
 * User: Contractor V&v Date: 5/07/10    Time: 4:35p
 * Updated in $/software/control processor/code/drivers
 * SCR #548 Strings in Logs
 *
 * *****************  Version 27  *****************
 * User: Contractor3  Date: 4/09/10    Time: 12:17p
 * Updated in $/software/control processor/code/drivers
 * SCR #539 Updated for Code Review
 *
 * *****************  Version 26  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:08p
 * Updated in $/software/control processor/code/drivers
 * SCR #533 DOUT wrap-around
 *
 * *****************  Version 25  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:14p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 24  *****************
 * User: Jeff Vahue   Date: 2/26/10    Time: 3:00p
 * Updated in $/software/control processor/code/drivers
 * SCR# 469 - Change DIO_ReadPin to return the pin value, WIN32 handling
 * of _DIR and _RIR
 *
 * *****************  Version 23  *****************
 * User: Jeff Vahue   Date: 2/26/10    Time: 11:42a
 * Updated in $/software/control processor/code/drivers
 * Set LEDS on at startup per SRS-2207, Set SW_PFEN1/2 at startup per
 * SRS-1436 and SRS-1441
 *
 * *****************  Version 22  *****************
 * User: John Omalley Date: 1/29/10    Time: 9:34a
 * Updated in $/software/control processor/code/drivers
 * SCR 395
 * - Updated the LED names to the externally labeled LEDs
 *
 * *****************  Version 21  *****************
 * User: Contractor V&v Date: 1/27/10    Time: 5:22p
 * Updated in $/software/control processor/code/drivers
 * SCR 340
 *
 * *****************  Version 20  *****************
 * User: Contractor V&v Date: 1/13/10    Time: 4:58p
 * Updated in $/software/control processor/code/drivers
 *
 * *****************  Version 19  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:23p
 * Updated in $/software/control processor/code/drivers
 * SCR 371
 *
 * *****************  Version 18  *****************
 * User: Contractor V&v Date: 12/22/09   Time: 2:16p
 * Updated in $/software/control processor/code/drivers
 * SCR 337 Debounce DIN
 *
 * *****************  Version 17  *****************
 * User: Contractor V&v Date: 11/18/09   Time: 3:40p
 * Updated in $/software/control processor/code/drivers
 * Fixed typo. No code changed for SCR 172.
 *
 * *****************  Version 16  *****************
 * User: Peter Lee    Date: 11/05/09   Time: 5:30p
 * Updated in $/software/control processor/code/drivers
 * SCR #315 DIO CBIT for SEU Processing
 *
 * *****************  Version 15  *****************
 * User: Peter Lee    Date: 10/20/09   Time: 6:49p
 * Updated in $/software/control processor/code/drivers
 * Updates for JV to support PC version of program
 *
 * *****************  Version 14  *****************
 * User: John Omalley Date: 10/01/09   Time: 4:31p
 * Updated in $/software/control processor/code/drivers
 * Added function for discrete sensors
 *
 * *****************  Version 13  *****************
 * User: Peter Lee    Date: 9/15/09    Time: 6:13p
 * Updated in $/software/control processor/code/drivers
 * SCR #94, #95 PBIT returns log structure to Init Mgr
 *
 * *****************  Version 12  *****************
 * User: Peter Lee    Date: 9/14/09    Time: 3:23p
 * Updated in $/software/control processor/code/drivers
 * SCR #94 Updated Init for SET_CHECK() and return ERR CODE Struct
 *
 * *****************  Version 11  *****************
 * User: Jim Mood     Date: 2/05/09    Time: 11:25a
 * Updated in $/control processor/code/drivers
 * Added support for FPGA GPIO.  Added the 232/422 mode select, CAN0 wake,
 * and CAN1 wage pins in the FPGA GPIO register.
 *
 * *****************  Version 10  *****************
 * User: Peter Lee    Date: 1/06/09    Time: 2:55p
 * Updated in $/control processor/code/drivers
 * SCR 129 Misc FPGA Updates
 *
 * *****************  Version 9  *****************
 * User: Jim Mood     Date: 12/05/08   Time: 5:13p
 * Updated in $/control processor/code/drivers
 * Added the DIO_Out_MSRst pin (MCF5485 - T2OUT) and the code to control
 * that pin in the GPT2 (general purpose timer) peripheral.
 *
 * *****************  Version 8  *****************
 * User: Jim Mood     Date: 10/01/08   Time: 4:39p
 * Updated in $/control processor/code/drivers
 * Fixed the DIO_SetPin toggle function.  Updated comment blocks to the
 * PWES style and added the VSS version/date/history macros
 *
 * *****************  Version 7  *****************
 * User: Jim Mood     Date: 8/08/08    Time: 11:26a
 * Updated in $/control processor/code/drivers
 * Fixes SCR 64 "DIO pin mapping for ACS3 DX control and ACS2 TX Enb"  The
 * cause was incorrect pin mapping on ACS3 DX and ACS3 TX Enb.
 *
 *
 *
 ***************************************************************************/

