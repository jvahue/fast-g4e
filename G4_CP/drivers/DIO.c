#define DIO_BODY

/******************************************************************************
            Copyright (C) 2007 - 2016 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

  ECCN:        9D991

  File:        DIO.c

  Description: Discrete Input and Output driver.  This module maintains
               a list of all GPIO discrete pins specific to the FAST II.
               Access to reading or setting pins is performed
               by addressing pins by their enumerated list index
               and calling the read or write function.  Refer to DIO.h for
               adding or removing pin definitions.
               A shadow value is maintained for the configuration of each
               pin and the outputs have the programmed output value shadowed,
               this facilitates the DIO CBIT function of the system level.

   VERSION
   $Revision: 59 $  $Date: 2/10/16 9:20a $


******************************************************************************/


/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include "mcf548x.h"

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "alt_basic.h"
#include "ResultCodes.h"
#include "DIO.h"
#include "UtilRegSetCheck.h"
#include "Assert.h"
#include "GSE.h"
#include "ClockMgr.h"

#include "TestPoints.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define DIO_GPIO_PODR_OFFSET   0x00 //Output data register
#define DIO_GPIO_PDDR_OFFSET   0x10 //Data direction register
#define DIO_GPIO_PPDSDR_OFFSET 0x20 //Input pin read/Output data register write
#define DIO_GPIO_PCLRR_OFFSET  0x30 //Port clear register (write 0 to clear, 1 no effect)

#define DIO_GPT_OUT_HI      3   //For timer GPIO, sets the GPIO field of GMSn
                                //register to output high
#define DIO_GPT_OUT_LO      2   //For timer GPIO, sets the GPIO field of GSMn
                                //register to output low

// WA Testing Constants
#define MAX_WA_FAILS 3
#define MAX_WRAPAROUNDS  4
#define MAX_DISPLAY_DISCRETES 8

#define DIO_MAX_OUTPUTS_LIMIT  32  // DIO_MAX_OUTPUTS must be < 32
#define DIO_MAX_INPUTS_LIMIT  32   // DIO_MAX_INPUTS must be < 32

#define LOG_BUFFER_SIZE 128

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
typedef struct
{
  UINT32 dioRegAddr;
} DIO_SHADOW_REG;

typedef struct
{
  UINT32 mask;
  UINT32 value1;
  UINT32 value2;
} DIO_SHADOW_DATA;

// Structure def for mapping wrapped output pins to the
// corresponding input pin.
typedef struct
{
  DIO_OUTPUT outputPin;
  DIO_INPUT  inputPin;
} WRAP_AROUND_MAP;

// The following enum must match the DioShadowReg[] one for one.
typedef enum
{
  DIO_PODR_FEC0L=0,
  DIO_PODR_FEC1H,
  DIO_PODR_FEC1L,
  DIO_PODR_PCIBR,
  DIO_PODR_PCIBG,
  DIO_PODR_PSC1PSC0,
  DIO_PODR_PSC3PSC2,
  DIO_PODR_FEC0H,
  DIO_PODR_FECI2C,
  //DIO_GPT_GMS2,
  DIO_ENUM_MAX
} DIO_ENUM;

static const DIO_SHADOW_REG dioShadowReg[DIO_ENUM_MAX] =
{
  (UINT32) &MCF_GPIO_PODR_FEC0L,
  (UINT32) &MCF_GPIO_PODR_FEC1H,
  (UINT32) &MCF_GPIO_PODR_FEC1L,
  (UINT32) &MCF_GPIO_PODR_PCIBR,
  (UINT32) &MCF_GPIO_PODR_PCIBG,
  (UINT32) &MCF_GPIO_PODR_PSC1PSC0,
  (UINT32) &MCF_GPIO_PODR_PSC3PSC2,
  (UINT32) &MCF_GPIO_PODR_FEC0H,
  (UINT32) &MCF_GPIO_PODR_FECI2C
  //(UINT32) &MCF_GPT_GMS2
};

// Protect shadow data structs from ram test
#ifndef WIN32
#pragma ghs section bss=".pbss"
#endif

static DIO_SHADOW_DATA dioShadowData_PDDR[DIO_ENUM_MAX];
static DIO_SHADOW_DATA dioShadowData_PODR[DIO_ENUM_MAX];
static DIO_SHADOW_DATA dioShadowData_PPDSDR[DIO_ENUM_MAX];
static DIO_SHADOW_DATA dioShadowData_GMS2;
static DIO_SHADOW_DATA dioShadowData_FPGA;

// Associate Reg Pin to enum for quicker access
static DIO_ENUM dioShadowOutputPinReg[DIO_MAX_OUTPUTS];

#ifndef WIN32
#pragma ghs section bss=default
#endif

static BOOLEAN bDebounceActive;
/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
// Protect data structs from ram test
#ifndef WIN32
#pragma ghs section bss=".pbss"
#endif

#undef  DIO_PIN
//Set the list macro to stringify the enumerated list
#define DIO_PIN(Name,Dir,State,Periph,AccMethod,Addr,Bm)\
        {#Name,Dir,State,Periph,AccMethod,Addr,Bm},
STATIC DIO_CONFIG dio_OutputPins[DIO_MAX_OUTPUTS] = { DIO_OUTPUTS_LIST };

STATIC DIO_CONFIG dio_InputPins[DIO_MAX_INPUTS]   = { DIO_INPUTS_LIST };

STATIC DIO_DISPLAY_SENSOR_STATUS m_DisplayDiscretes[MAX_DISPLAY_DISCRETES] =
{
  {(UINT16)DispDisc0, 0, 0, FALSE},
  {(UINT16)DispDisc1, 0, 0, FALSE},
  {(UINT16)DispDisc2, 0, 0, FALSE},
  {(UINT16)DispDisc3, 0, 0, FALSE},
  {(UINT16)DispDisc4, 0, 0, FALSE},
  {(UINT16)DispDisc5, 0, 0, FALSE},
  {(UINT16)DispDisc6, 0, 0, FALSE},
  {(UINT16)DispDisc7, 0, 0, FALSE},
};

STATIC volatile UINT8* displayValidationFlag      = NULL;
STATIC volatile UINT8* displayHealth              = NULL;
STATIC volatile UINT8* displayStatus              = NULL;
                                                  
#undef DIO_PIN

static BOOLEAN bDIO_OutputShadow[DIO_MAX_OUTPUTS];

#ifndef WIN32
#pragma ghs section bss=default
#endif

// Array of "filtered" output pins
static DIO_DEBOUNCED dio_FilteredPins[DIO_MAX_INPUTS];

// Fast-lookup table for getting the 8-bit value at a specific port.
static DIO_PORT_DATA dioInputStore[DIO_ENUM_MAX];

static const REG_SETTING defDIORegData =
{
  NULL,            // Pointer to HW Register
  NULL,            // Value to set
  sizeof(UINT8),   // Size of parameter
  NULL,            // Identifies the Reg
  REG_SET,         // Set the Register Only
  TRUE,            // Reg Entry is checkable
  RFA_FORCE_UPDATE,// Action to take if CBIT check of reg fails
  TRUE,            // Value is stored and owned by external entity
  NULL,            // External entity Shadow 1 copy
  NULL             // External entity Shadow 2 copy
};

// Wrap Around Testing
static const WRAP_AROUND_MAP wrapAround[MAX_WRAPAROUNDS] = {
  {LSS0, LSS0_WA},
  {LSS1, LSS1_WA},
  {LSS2, LSS2_WA},
  {LSS3, LSS3_WA}
};
static INT8 wrapFailCounts[MAX_WRAPAROUNDS];
static BOOLEAN waFailLogRecorded[MAX_WRAPAROUNDS];

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void DIO_InitShadowReg( void *pRegAddr, UINT16 RegOffset,
                               UINT32 mask, UINT32 value );
static void DIO_InitShadowRegGMS2( UINT32 mask, UINT32 value );
static void DIO_InitShadowRegFPGA( UINT32 mask, UINT32 value );

static void DioRegCheck_Init( void );
static void DIO_DebounceState(DIO_INPUT Pin, BOOLEAN State,
                                          UINT32 currentTimeMs);
static DIO_PORT_DATA* DIO_AddPortToList(volatile UINT8* addr);
static void DIO_CheckWrapAround( void );
static BOOLEAN DIO_InitPin(const DIO_CONFIG *PinConfig); 
/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/*****************************************************************************
 * Function:    DIO_Init
 *
 * Description: Setup the initial state for the GPIO/Discrete I/O driver
 *
 * Parameters:  SysLogId - Ptr to return System Log ID into
 *              pdata -    Ptr to buffer to return system log data
 *              psize -    Ptr to return size (bytes) of system log data
 *
 * Returns:     DRV_OK -  DIO Init Success
 *              DRV_DIO_REGISTER_INIT_FAIL - DIO Init Fails
 *
 * Notes:
 *   - PBIT test performed is SET and CHECK of Initial Value (in DIO_InitPin())
 *   - If the number of DIO > 32, the algorithm (to record PBIT failures)
 *     below must be updated.
 *
 ****************************************************************************/
RESULT DIO_Init (SYS_APP_ID *SysLogId, void *pdata, UINT16 *psize)
{
  UINT32 i;

  UINT32 setup_pin_result;
  UINT32 pin_result_summary;

  DIO_DRV_PBIT_LOG  *pdest;

  // Init DOUT WA testing data
  for ( i = 0; i < MAX_WRAPAROUNDS; ++i)
  {
    wrapFailCounts[i] = 0;
    waFailLogRecorded[i] = FALSE;
  }

  bDebounceActive = FALSE;

  pdest = (DIO_DRV_PBIT_LOG *) pdata;
  memset ( pdest, 0, sizeof(DIO_DRV_PBIT_LOG) );
  pdest->result = DRV_OK;

  memset(dioInputStore, 0, sizeof(dioInputStore) );

  *psize = sizeof(DIO_DRV_PBIT_LOG);

  //Initialize discrete outputs.  DIO_MAX_OUTPUTS must be < 32
  pin_result_summary = 0;

  ASSERT_MESSAGE(DIO_MAX_OUTPUTS < DIO_MAX_OUTPUTS_LIMIT,
          "DIO_MAX_OUTPUTS exceeds 32. (Value: %d)",DIO_MAX_OUTPUTS);

  for(i = 0; i < DIO_MAX_OUTPUTS; i++)
  {
    setup_pin_result = DIO_InitPin(&dio_OutputPins[i]) ? 1 : 0;
    //NOTE: The ordering in DIO_OutputPins determine the bit ordering in
    //      PinResultSummary.  See DIO.h
    pin_result_summary |= (setup_pin_result << i);
    bDIO_OutputShadow[i] = dio_OutputPins[i].bInitialState;
  }
  // Set Return Structure Here
  pdest->dioOutResults = pin_result_summary;


  //Initialize the discrete inputs. DIO_MAX_INPUTS must be < 32
  pin_result_summary = 0;

  ASSERT_MESSAGE( DIO_MAX_INPUTS < DIO_MAX_INPUTS_LIMIT,
          "DIO_MAX_INPUTS exceeds 32. (Value: %d)",DIO_MAX_INPUTS);

  //Initialize discrete inputs
  for(i = 0; i < DIO_MAX_INPUTS; i++)
  {
    setup_pin_result = DIO_InitPin(&dio_InputPins[i]) ? 1 : 0;
    //NOTE: The ordering in DIO_InputPins determine the bit ordering in
    //      PinResultSummary.  See DIO.h
    pin_result_summary |= (setup_pin_result << i);

    // If the initialization was successful, set the initial state
    if (setup_pin_result == 0)
    {
      dio_FilteredPins[i].timeStampMs    = CM_GetTickCount();

      // Add this port to the array of PODRs and store the returned ptr.
      // DIO_AddPortToList will only add addresses not already added to list.
      dio_FilteredPins[i].portData       = DIO_AddPortToList( dio_InputPins[i].dataReg );

      // Read DIN for initial state.
      dio_InputPins[i].bInitialState      = DIO_ReadPin((DIO_INPUT)i);
      dio_FilteredPins[i].filteredState  = dio_InputPins[i].bInitialState;
      dio_FilteredPins[i].lastRecvdState = dio_InputPins[i].bInitialState;
    }
  }
  // Set Return Structure Here
  pdest->dioInResults = pin_result_summary;

  if ( ( pdest->dioInResults != 0) || (pdest->dioOutResults != 0) )
  {
     pdest->result = DRV_DIO_REG_INIT_FAIL;
     *SysLogId = DRV_ID_DIO_PBIT_REG_INIT_FAIL;
  }

  DioRegCheck_Init();

  return  pdest->result;
}



/*****************************************************************************
 * Function:    DIO_SetPin
 *
 * Description: Sets the pin and the shadow value.  This is
 *              only way an output pin should be set, because bypassing this
 *              routine can leave the shadow and pin state out of sync causing
 *              a CBIT error.
 *
 * Parameters:  Pin: The pin to modify (see DIO_OUTPUT enumeration)
 *              Op:  The type of modification, either DIO_SetHigh, DIO_SetLow
 *
 *
 *
 *
 * Returns:
 *
 * Notes:       MSRst pin is handled as a special case because it's GPIO
 *              pin is not actually included in the GPIO peripheral.  It is
 *              controlled by the General Purpose Timer 2 peripheral.
 *
 ****************************************************************************/
void DIO_SetPin(DIO_OUTPUT Pin, DIO_OUT_OP Op)
{
  UINT32 value;
  INT32 intrLevel;

  // Sanity check on the pin index
  ASSERT_MESSAGE (Pin < DIO_MAX_OUTPUTS, "Invalid DIO_OUTPUT Pin: %d", Pin);


  intrLevel = __DIR();
  bDIO_OutputShadow[Pin] = Op == DIO_SetHigh ? ON : OFF;
  if(dio_OutputPins[Pin].peripheral == DIO_TMR2)
  {
      value = Op == DIO_SetHigh ?
                   (MCF_GPT_GMS_TMS_GPIO | MCF_GPT_GMS_GPIO(DIO_GPT_OUT_HI)) :
                   (MCF_GPT_GMS_TMS_GPIO | MCF_GPT_GMS_GPIO(DIO_GPT_OUT_LO));

      DIO_S( &MCF_GPT_GMS2, value);

      // Update Shadow copies
      DIO_S( &dioShadowData_GMS2.value1, value);
      DIO_S( &dioShadowData_GMS2.value2, value);

  }
  else if(dio_OutputPins[Pin].peripheral == DIO_GPIO)
  {
      DIO_W((dio_OutputPins[Pin].dataReg+DIO_GPIO_PODR_OFFSET),
             dio_OutputPins[Pin].pinMask,
             Op);

      // Update Shadow copies
      DIO_W((UINT32 *)&dioShadowData_PODR[dioShadowOutputPinReg[Pin]].value1,
            dio_OutputPins[Pin].pinMask,
            Op);
      DIO_W((UINT32 *)&dioShadowData_PODR[dioShadowOutputPinReg[Pin]].value2,
            dio_OutputPins[Pin].pinMask,
            Op);
      DIO_W((UINT32 *)&dioShadowData_PPDSDR[dioShadowOutputPinReg[Pin]].value1,
            dio_OutputPins[Pin].pinMask,
            Op);
      DIO_W((UINT32 *)&dioShadowData_PPDSDR[dioShadowOutputPinReg[Pin]].value2,
            dio_OutputPins[Pin].pinMask,
            Op);

  }
  else if(dio_OutputPins[Pin].peripheral == DIO_FPGA)
  {
      DIO_W16( (UINT16 *) dio_OutputPins[Pin].dataReg, dio_OutputPins[Pin].pinMask, Op);
  }
  __RIR(intrLevel);

  //return DRV_OK;

}


/*****************************************************************************
 * Function:    DIO_ReadPin
 *
 * Description: Retrieves the current state of a pin from the DIO_DEBOUNCED
 *              array or directly from the discrete input register depending on
 *              ACCESS_METHOD set for the specified Pin param
 *
 * Parameters:  [in] Pin:   The pin to read (see DIO_INPUT enumeration)
 *
 * Returns:     the state of the PIN
 *
 * Notes:
 *
 ****************************************************************************/
BOOLEAN DIO_ReadPin( DIO_INPUT Pin)
{
  BOOLEAN bState;
  UINT8 value;
  UINT16 value16;
  DIO_ACC_METHOD access;


  ASSERT_MESSAGE(Pin < DIO_MAX_INPUTS, "Invalid DIO_INPUT Pin: %d", Pin);

  access = bDebounceActive ? dio_InputPins[Pin].accessMethod : DIO_RAW;

  // Retrieve the bit according to the configured access method
  switch (access)
  {
    case DIO_FILTERED:
      bState = dio_FilteredPins[Pin].filteredState;
      break;

    case DIO_NOT_APPLIC: //DIN discrete access method not set, handle as RAW

      // Currently the DIO_NOT_APPLIC case not used.  In future if this setting is used,
      //   code below should be re-enabled. Comment out for coverage issues.
      // GSE_DebugStr(NORMAL,TRUE,"AccessMethod for %s  not set, defaulting to RAW",
      // DIO_InputPins[Pin].Name);
      // No break. Deliberate Fall through.

    case DIO_RAW:   //Read the value directly off the register and mask the bit
      if(dio_InputPins[Pin].peripheral == DIO_FPGA)
      {
        value16 = DIO_R16((UINT16 *)dio_FilteredPins[Pin].portData->portAddr);
        bState = ( value16 & dio_InputPins[Pin].pinMask) ? ON : OFF;
      }
      else if(dio_InputPins[Pin].peripheral == DIO_DISP)
      {
        value = DIO_R(dio_InputPins[Pin].dataReg + 
                      dio_InputPins[Pin].pinMask);
        bState = (value) ? ON : OFF;
      }
      else
      {
        value = DIO_R( dio_FilteredPins[Pin].portData->portAddr + 
                       DIO_GPIO_PPDSDR_OFFSET );
        bState = ( value & dio_InputPins[Pin].pinMask) ? ON : OFF;
      }
      break;

    default:
      FATAL("Invalid AccessMethod: %d", dio_InputPins[Pin].accessMethod);
      break;
  }


  return bState;
}

/*****************************************************************************
 * Function:    DIO_GetValue
 *
 * Description: Used to get the pin value for the sensor object.
 *
 *
 * Parameters:  Pin: The pin to read (see DIO_INPUT enumeration)
 *              *null: Not used.  Add to support common i/f.
 *
 * Returns:     FLOAT32 - The state of the Pin returned from DIO_ReadPin
 *
 * Notes:
 *
 ****************************************************************************/
FLOAT32 DIO_GetValue (UINT16 Pin, UINT32 *null)
{
   // Local Data
   BOOLEAN bState;
   FLOAT32 fValue;

   bState = DIO_ReadPin ((DIO_INPUT)Pin);

   fValue = (FLOAT32)bState;

   return fValue;
}


/*****************************************************************************
* Function:    DIO_UpdateDiscreteInputs
*
* Description: Reads each GPIO pin state register and
*              de-bounce / filter the state of each Pin.
*              Compares the current state of the pin with the previous state and
*              uses time to determine whether the state is in a different, stable
*              state.
*
* Parameters:  none
*
*
* Returns:     none
*
* Notes:       none
*
****************************************************************************/
void DIO_UpdateDiscreteInputs(void)
{
  UINT32 i = 0;
  UINT32 pin = 0;
  BOOLEAN state;
  UINT32  currentTimeMs;

  // Read each of the GPIO pin state registers
  // This allows each port to be read once, rather than
  // re-reading the same port for each discrete bit associated with the port.
  while (dioInputStore[i].portAddr != NULL)
  {
    dioInputStore[i].value =
      DIO_R( dioInputStore[i].portAddr + DIO_GPIO_PPDSDR_OFFSET );
    ++i;
  }

  // Process the input discretes.
  // Each discrete entry holds a pointer to the DioInputStore[i].value
  // containing it's bit.

  currentTimeMs = CM_GetTickCount();
  // Debounce/filter the state of the pin
  for(pin = 0; pin < DIO_MAX_INPUTS; ++pin)
  {
    state = dio_FilteredPins[pin].portData->value & dio_InputPins[pin].pinMask ? ON : OFF;
    // De-bounce/filter the state of this pin
    DIO_DebounceState  ((DIO_INPUT)pin, state, currentTimeMs);
  }

  // verify the Discrete outputs wraparounds match the output
  DIO_CheckWrapAround();
}

/*****************************************************************************
* Function:    DIO_GetOutputPin
*
* Description: Returns the state of the passed Output pin
*
*
* Parameters:  [in]  Pin - Discrete Output value to be returned.
*              [out] PinState - Boolean value of the output pin
*
*
* Returns:     None
*
* Notes:
****************************************************************************/
void DIO_GetOutputPin(DIO_OUTPUT Pin, BOOLEAN *PinState )
{
  ASSERT(Pin < DIO_MAX_OUTPUTS );

  *PinState = bDIO_OutputShadow[Pin];
}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/*****************************************************************************
 * Function:    DIO_InitPin
 *
 * Description: Configures the hardware to setup the pin with data direction and
 *              initial state as defined in the DIO_CONFIG block.  This can
 *              be used to initially setup or to re-initialize a pin with a
 *              different configuration
 *
 * Parameters:  PinConfig:  Pointer to a pin configuration block, see DIO_CONFIG
 *                          in DIO.h
 *
 * Returns:     TRUE  (1) - Init Reg FAILED
 *              FALSE (0) - Init Reg PASS
 *
 * Notes:
 *
 * History:
 *  08/31/2009 PL
 *        - DRV_DIO_INIT_ERR_BAD_DATA_REG_PTR should be an ASSERT rather than
 *          a return.  Only two possible ways for DRV_DIO_INIT_ERR_BAD_DATA_REG_PTR
 *           1) Programmed incorrectly.
 *           2) Bit flip in the value passed in.
 *          In either cases an ASSERT should be the appropriate err handling processing.
 *
 ****************************************************************************/
static
BOOLEAN DIO_InitPin(const DIO_CONFIG *PinConfig)
{
  BOOLEAN bFailed;
  BOOLEAN bInitOk;
  //REG_SETTING RegData;
  UINT8 set_val = 0;

  bInitOk = TRUE;
  bFailed = FALSE;

  if(DIO_GPIO == PinConfig->peripheral)
  {

    ASSERT(  (PinConfig->dataReg == &MCF_GPIO_PODR_FBCTL)    ||
             (PinConfig->dataReg == &MCF_GPIO_PODR_FBCS)     ||
             (PinConfig->dataReg == &MCF_GPIO_PODR_DMA)      ||
             (PinConfig->dataReg == &MCF_GPIO_PODR_FEC0H)    ||
             (PinConfig->dataReg == &MCF_GPIO_PODR_FEC0L)    ||
             (PinConfig->dataReg == &MCF_GPIO_PODR_FEC1H)    ||
             (PinConfig->dataReg == &MCF_GPIO_PODR_FEC1L)    ||
             (PinConfig->dataReg == &MCF_GPIO_PODR_FECI2C)   ||
             (PinConfig->dataReg == &MCF_GPIO_PODR_PCIBG)    ||
             (PinConfig->dataReg == &MCF_GPIO_PODR_PCIBR)    ||
             (PinConfig->dataReg == &MCF_GPIO_PODR_PSC3PSC2) ||
             (PinConfig->dataReg == &MCF_GPIO_PODR_PSC1PSC0) ||
             (PinConfig->dataReg == &MCF_GPIO_PODR_DSPI) );

#ifdef WIN32
/*vcast_dont_instrument_start*/
  DIO_W((PinConfig->dataReg+DIO_GPIO_PDDR_OFFSET),PinConfig->pinMask,PinConfig->direction);
  DIO_W((PinConfig->dataReg+DIO_GPIO_PODR_OFFSET),PinConfig->pinMask,PinConfig->bInitialState);
/*vcast_dont_instrument_end*/
#endif

    //Set data output value before setting direction of pin
    switch(PinConfig->bInitialState)
    {
      case ON:
        SET_CHECK_MASK_OR ( *(PinConfig->dataReg+DIO_GPIO_PODR_OFFSET),
                            PinConfig->pinMask,
                            bInitOk );
        break;

      case OFF:
        SET_CHECK_MASK_AND ( *(PinConfig->dataReg+DIO_GPIO_PODR_OFFSET),
                             PinConfig->pinMask,
                             bInitOk );
        break;

      default:
         FATAL("Unsupported InitialState = %d", PinConfig->bInitialState);
        break;
    }


    //Set the pin direction
    switch(PinConfig->direction)
    {
      case DIO_In:
        SET_CHECK_MASK_AND ( *(PinConfig->dataReg+DIO_GPIO_PDDR_OFFSET),
                             PinConfig->pinMask,
                             bInitOk );
        break;

      case DIO_Out:
        SET_CHECK_MASK_OR ( *(PinConfig->dataReg+DIO_GPIO_PDDR_OFFSET),
                            PinConfig->pinMask,
                            bInitOk );
        break;

      default:
         FATAL("Unsupported pin direction = %d", PinConfig->direction);
         break;
    }

    DIO_InitShadowReg( (void*) PinConfig->dataReg, DIO_GPIO_PDDR_OFFSET, PinConfig->pinMask,
                       PinConfig->direction);

    //  Check the Pin Setup
    //  For Input check Dir Reg Setup only ! (Performed above)
    //  For Output check Dir Reg Setup (Performed above),
    //             PODR (Output Data Reg) and PPDSDR (Output Pin)
    if (PinConfig->direction == DIO_Out)
    {
      switch (PinConfig->bInitialState)
      {
        case ON:
          if ( ((*(PinConfig->dataReg+DIO_GPIO_PODR_OFFSET) & PinConfig->pinMask) !=
                 PinConfig->pinMask) ||
               ((*(PinConfig->dataReg+DIO_GPIO_PPDSDR_OFFSET) & PinConfig->pinMask) !=
                 PinConfig->pinMask) )
          {
            bInitOk = FALSE;
          }
          set_val = 1;
          break;

        case OFF:
          if ( ((*(PinConfig->dataReg+DIO_GPIO_PODR_OFFSET) | ~PinConfig->pinMask) !=
                 ~PinConfig->pinMask) ||
               ((*(PinConfig->dataReg+DIO_GPIO_PPDSDR_OFFSET) | ~PinConfig->pinMask) !=
                 ~PinConfig->pinMask) )
          {
            bInitOk = FALSE;
          }
          set_val = 0;
          break;

        default:
          FATAL("Unsupported InitialState = %d", PinConfig->bInitialState);
          break;

      } // End switch (PinConfig->InitialState)

      DIO_InitShadowReg( (void *) PinConfig->dataReg,
                          DIO_GPIO_PODR_OFFSET,
                          PinConfig->pinMask, set_val);
      DIO_InitShadowReg( (void *) PinConfig->dataReg,
                          DIO_GPIO_PPDSDR_OFFSET,
                          PinConfig->pinMask, set_val);

    } // End if (PinConfig->Direction == DIO_Out)

  } // End of if(DIO_GPIO == PinConfig->Peripheral)

  else if(DIO_TMR2 == PinConfig->peripheral)
  {
    //If the peripheral is TMR2, this is a "special case" pin that is not
    //under control of the GPIO peripheral, but is a GPIO mode of another
    //Coldfire peripheral.  This ugly hack is thanks to Freescale!
    //Special case 1: Setrix reset is the Timer 1 GPIO
#ifdef WIN32
/*vcast_dont_instrument_start*/
    DIO_S(&MCF_GPT_GMS2,(MCF_GPT_GMS_TMS_GPIO | MCF_GPT_GMS_GPIO(DIO_GPT_OUT_HI)));
/*vcast_dont_instrument_end*/
#endif

    //SET_AND_CHECK ( MCF_GPT_GMS2,
    //               (MCF_GPT_GMS_TMS_GPIO | MCF_GPT_GMS_GPIO(DIO_GPT_OUT_HI)),
    //                bInitOk );
    switch (PinConfig->bInitialState)
    {
      case ON:
        set_val = (MCF_GPT_GMS_TMS_GPIO | MCF_GPT_GMS_GPIO(DIO_GPT_OUT_HI));
        break;

      case OFF:
        // Currently the OFF case not used.  In future if this setting is used,
        //   code below should be re-enabled. Comment out for coverage issues.
        // SetVal = (MCF_GPT_GMS_TMS_GPIO | MCF_GPT_GMS_GPIO(DIO_GPT_OUT_LO));
        // break;
      default:
        FATAL("Unsupported InitialState = %d", PinConfig->bInitialState);
        break;
    }

    SET_AND_CHECK ( MCF_GPT_GMS2, set_val, bInitOk );
    DIO_InitShadowRegGMS2( 0xFFFFFFFFUL, set_val);

  }
  else if(DIO_FPGA == PinConfig->peripheral)
  {
    //FPGA already has direction hardwired, so just need to set initial value
    //Set data output value
    switch(PinConfig->bInitialState)
    {
      case ON:
        /*FPGA not init yet, need to figure something out here
         *PinConfig->DataReg |= PinConfig->PinMask;*/
         set_val = (UINT8) DIO_SetHigh;
        break;

      case OFF:
        /*FPGA not init yet, need to figure something out here
        *PinConfig->DataReg &= ~PinConfig->PinMask; */
        set_val = (UINT8) DIO_SetLow;
        break;

      default:
        FATAL("Unsupported InitialState = %d", PinConfig->bInitialState);
        break;
    }

    //Only attempt to setup discreet output registers
    if (PinConfig->direction == DIO_Out)
    {
       DIO_InitShadowRegFPGA( PinConfig->pinMask, set_val );
    }

  }
  else if (DIO_DISP == PinConfig->peripheral)
  {
    //Set data output value
    switch (PinConfig->bInitialState)
    {
      case ON:
        set_val = (UINT8) DIO_SetHigh;
        break;
      case OFF:
        set_val = (UINT8) DIO_SetLow;
        break;
      default:
        FATAL("Unsupported InitialState = %d", PinConfig->bInitialState);
        break;
    }
  }

  if (!bInitOk)
  {
    bFailed = TRUE;
  }

  return (bFailed);

}


/*****************************************************************************
 * Function:    DIO_InitShadowReg
 *
 * Description: Initializes the shadow object using the values from the
                pRegAddr passed in.
 *
 * Parameters:  pRegAddr   Pointer to the Pin's PORT data Register
 *              RegOffset  Reg offset value to set PDDR, PODR or PPDSDR
 *              mask       Reg mask
 *              value      used to set/clear shadow's value1 and set
 *                         shadow's value2
 *
 * Returns:     None
 *
 * Notes:
 *
 ****************************************************************************/
static
void DIO_InitShadowReg( void *pRegAddr, UINT16 RegOffset, UINT32 mask, UINT32 value )
{
  DIO_ENUM dioReg;
  DIO_SHADOW_DATA *pDioShadowData;
  const DIO_SHADOW_REG *pDioShadowReg;
  UINT32 i;

  pDioShadowReg = &dioShadowReg[0];

  for (i = 0; i < (UINT32)DIO_ENUM_MAX; i++)
  {
    if ( pDioShadowReg->dioRegAddr == (UINT32) pRegAddr )
    {
      break;
    }
    else
    {
      pDioShadowReg++;
    }
  }

  // if i is not less than DIO_ENUM_MAX then we have an unknown register address
  ASSERT_MESSAGE( i < DIO_ENUM_MAX, "Unknown DioRegAddr : %d", i);

  dioReg = (DIO_ENUM) i;

  switch (RegOffset)
  {
    case DIO_GPIO_PDDR_OFFSET:
      pDioShadowData = &dioShadowData_PDDR[dioReg];
      break;

    case DIO_GPIO_PODR_OFFSET:
      pDioShadowData = &dioShadowData_PODR[dioReg];
      break;

    case DIO_GPIO_PPDSDR_OFFSET:
      pDioShadowData = &dioShadowData_PPDSDR[dioReg];
      break;

    default:
      FATAL("Invalid Register Offset = %d", RegOffset);
      break;
  }

  // Update Mask
  pDioShadowData->mask |= mask;

  // Update value
  if ( value == 1 )
  {
    pDioShadowData->value1 |= mask;   // Set the bit
  }
  else
  {
    pDioShadowData->value1 &= (~mask);  // Clr the bit
  }

  pDioShadowData->value2 = pDioShadowData->value1;

}


/*****************************************************************************
 * Function:    DIO_InitShadowRegGMS2
 *
 * Description: Updates the GMS2 DIO Shadow Register
 *
 * Parameters:  mask - Reg mask
 *              value - Shadow value to set
 *
 * Returns:     none
 *
 * Notes:       none
 *
 ****************************************************************************/
static
void DIO_InitShadowRegGMS2( UINT32 mask, UINT32 value )
{

  // Update Mask
  dioShadowData_GMS2.mask = mask;

  // Update value
  dioShadowData_GMS2.value1 = value;
  dioShadowData_GMS2.value2 = value;

}


/*****************************************************************************
 * Function:    DIO_InitShadowRegFPGA
 *
 * Description: Updates the GMS2 DIO Shadow Register
 *
 * Parameters:  mask - Reg mask
 *              value - Shadow value to set
 *
 * Returns:     none
 *
 * Notes:       none
 *
 ****************************************************************************/
static
void DIO_InitShadowRegFPGA( UINT32 mask, UINT32 value )
{

  // Update Mask
  dioShadowData_FPGA.mask |= mask;

  // Update value
  DIO_W( (UINT32 *) &dioShadowData_FPGA.value1, mask, (DIO_OUT_OP) value);
  DIO_W( (UINT32 *) &dioShadowData_FPGA.value2, mask, (DIO_OUT_OP) value);

}

/*****************************************************************************
 * Function:    DioRegCheck_Init
 *
 * Description: Registers the Reg and Shadow Values to UtilReg
 *
 * Parameters:  none
 *
 * Returns:     TRUE if Initialization is OK
 *              FALSE if Initialization is FAIL
 *
 * Notes:       none
 *
 ****************************************************************************/
static
void DioRegCheck_Init( void )
{
  const DIO_SHADOW_REG *pDioShadowReg;
  UINT32 i;
  UINT32 j;
  BOOLEAN bInitOk;
  REG_SETTING regData;
  const DIO_CONFIG *pPinConfig;

  pDioShadowReg = &dioShadowReg[0];
  regData = defDIORegData;

  bInitOk = TRUE;

  // Register DIO GPIO registers with UtilRegCheck.
  for (i = 0; i < (UINT32)DIO_ENUM_MAX; i++)
  {
    // Register PDDR
    regData.ptrReg = (void *) (pDioShadowReg->dioRegAddr + DIO_GPIO_PDDR_OFFSET);
    regData.value = dioShadowData_PDDR[i].mask;
    regData.Addr1 = (void *) &dioShadowData_PDDR[i].value1;
    regData.Addr2 = (void *) &dioShadowData_PDDR[i].value2;
    bInitOk &= RegSet ( &regData );

    // Register PODR
    regData.ptrReg = (void *) (pDioShadowReg->dioRegAddr + DIO_GPIO_PODR_OFFSET);
    regData.value = dioShadowData_PODR[i].mask;
    regData.Addr1 = (void *) &dioShadowData_PODR[i].value1;
    regData.Addr2 = (void *) &dioShadowData_PODR[i].value2;
    bInitOk &= RegSet ( &regData );

    // Register PPDSDR
    regData.ptrReg = (void *) (pDioShadowReg->dioRegAddr + DIO_GPIO_PPDSDR_OFFSET);
    regData.value = dioShadowData_PPDSDR[i].mask;
    regData.Addr1 = (void *) &dioShadowData_PPDSDR[i].value1;
    regData.Addr2 = (void *) &dioShadowData_PPDSDR[i].value2;
    bInitOk &= RegSet ( &regData );

    pDioShadowReg++;
  }

  // Register DIO GMS2 registers with UtilRegCheck.
  regData = defDIORegData;
  regData.ptrReg = (void *) &MCF_GPT_GMS2;
  regData.value = dioShadowData_GMS2.mask;
  regData.Addr1 = &dioShadowData_GMS2.value1;
  regData.Addr2 = &dioShadowData_GMS2.value2;
  regData.size = sizeof(UINT32);
  bInitOk &= RegSet ( &regData );


  // Register DIO FPGA registers with UtilRegCheck, but do not check as FPGA init
  //   probably not completed yet.
  regData = defDIORegData;
  regData.ptrReg = (void *) FPGA_GPIO_0;
  regData.value = dioShadowData_FPGA.mask;
  regData.Addr1 = &dioShadowData_FPGA.value1;
  regData.Addr2 = &dioShadowData_FPGA.value2;
  regData.size = sizeof(UINT16);
  bInitOk &= RegAdd ( &regData );

  // Setup DioShadowOutputPinReg[] for quicker run time access
  pPinConfig = &dio_OutputPins[0];

  for (i = 0; i < DIO_MAX_OUTPUTS; i++)
  {
    pDioShadowReg = &dioShadowReg[0];

    // Loop thru and find Reg
    for (j = 0; j < (UINT32)DIO_ENUM_MAX; j++)
    {
      if ( pDioShadowReg->dioRegAddr == (UINT32) pPinConfig->dataReg )
      {
        dioShadowOutputPinReg[i] = (DIO_ENUM) j;
        break;
      }
      else
      {
        // Point to next register
        pDioShadowReg++;
      }
    } // End for loop j

    pPinConfig++;

  } // End for loop i

}

/*****************************************************************************
* Function:    DIO_DebounceState
*
* Description: De-bounces the state of the Pin and updates the filtered value if
*              needed.
*              Compares the current state of the pin with the prev state and
*              uses time to determine whether the state is at a different, stable
*              state.
*
* Parameters:  [in] Pin           - enum value of the Pin to be debounced
*              [in] recvdState    - The current state of the Pin
*              [in] currentTimeMs - The time of this update in clock tick msec.
*
*
* Returns:     none
*
* Notes:       none
*
****************************************************************************/
static void DIO_DebounceState(DIO_INPUT Pin, BOOLEAN recvdState,
                              UINT32 currentTimeMs)
{
  // Set to true after first call(at runtime)
  bDebounceActive = TRUE;

  if(recvdState != dio_FilteredPins[Pin].filteredState)
  {
    if(recvdState == dio_FilteredPins[Pin].lastRecvdState)
    {
      // Same value as last sampled.
      // If it has been stable, designate it as the valid state, otherwise wait
      if(currentTimeMs - dio_FilteredPins[Pin].timeStampMs > DEBOUNCE_PERIOD_MS)
      {
        dio_FilteredPins[Pin].filteredState = recvdState;
        GSE_DebugStr(VERBOSE,TRUE,"DIO Input %s, FilteredState Change: %d",
                                dio_InputPins[Pin].name,
                                dio_FilteredPins[Pin].filteredState);
      }
    }
    else // The state has changed. Store state and time-of-change.
    {
      dio_FilteredPins[Pin].lastRecvdState = recvdState;
      dio_FilteredPins[Pin].timeStampMs    = currentTimeMs;
    }
  }
  else // no change from filtered value. Update the last received.
  {
    dio_FilteredPins[Pin].lastRecvdState = recvdState;
  }
}

/*****************************************************************************
* Function:    DIO_AddPortToList
*
* Description: Adds passed port addr to an array "uniquely".
*              (i.e. If the port is already in the list, it is not added again.)
*
* Parameters:  addr - [in] pointer to a discrete input data port.
*
*
* Returns:     Pointer to the DIO_PORT_DATA struct where the port address was
*              stored.
*
* Notes:       The DioInputStore array is built during init time to
*              act as a fast-array to allow Discrete input data ports
*              to be read directly without needing to call a given port
*              repetitively for each of it's contained discrete bits.
*              The returned pointer is used to link each discrete entry with the
*              DIO_PORT_DATA structure holding it's relevant data.
****************************************************************************/
static DIO_PORT_DATA* DIO_AddPortToList( volatile UINT8* addr)
{
  DIO_PORT_DATA* ptr = NULL;
  UINT32 i = 0;
  BOOLEAN bFoundInList = FALSE;

  while (i < DIO_ENUM_MAX && bFoundInList == FALSE)
  {
    if( dioInputStore[i].portAddr == addr)
    {
      bFoundInList = TRUE; // Address already in list
      ptr = &dioInputStore[i];
    }
    else if (dioInputStore[i].portAddr == NULL )
    {
      break; // Reached end of used entries.. break and add this entry
    }
    else
    {
      ++i;  // Keep searching
    }
  }

  // Check to see if we exited the while loop without
  // finding the address... add it.
  if( i < DIO_ENUM_MAX && bFoundInList == FALSE)
  {
    dioInputStore[i].portAddr = addr;
    ptr = &dioInputStore[i];
  }

  // Check if we didn't find the port in the list and didn't have room to
  // to add it. THIS SHOULD NEVER BE TRUE because DIO_ENUM_MAX is the total
  // of input and output ports. More than enough room.
  ASSERT(ptr != NULL);

  return ptr;
}

/*****************************************************************************
* Function:    DIO_CheckWrapAround
*
* Description: Verify the LSS_WA signals match the output value.  Fail if
*              mismatched for MAX_WA_FAILS consecutive checks.
*
* Parameters:  None
*
* Returns:     None
*
* Notes:
*   Interrupts are not disabled because this is a DT task so there should
*    be no change in DOUT state while this runs.
****************************************************************************/
static void DIO_CheckWrapAround( void)
{
  INT32 i;
  CHAR logBuffer[LOG_BUFFER_SIZE];
  DIO_WRAPAROUND_FAIL_LOG logEntry;

  // Loop for all entries in the wrap-around pin table
  // if this pin is in the table, verify that the associated out pin has the same
  // value as this input pin.
  for ( i = 0; i < MAX_WRAPAROUNDS; ++i )
  {
    logEntry.dinPin        = wrapAround[i].inputPin;
    logEntry.inputWAState  = TPU( DIO_ReadPin( logEntry.dinPin), eTpDio2481);
    logEntry.doutPin       = wrapAround[i].outputPin;
    logEntry.outputState   = bDIO_OutputShadow[logEntry.doutPin];

    if ( logEntry.inputWAState != logEntry.outputState)
    {
      if ( ++wrapFailCounts[i] > MAX_WA_FAILS && !waFailLogRecorded[i])
      {
        snprintf(logBuffer, sizeof(logBuffer),
                "Discrete Wrap Around Mismatch, %s: %d, %s: %d",
                 dio_InputPins[logEntry.dinPin].name,
                 logEntry.inputWAState,
                 dio_OutputPins[logEntry.doutPin].name,
                 logEntry.outputState);

        LogWriteSystem(SYS_DIO_DISCRETE_WRAP_FAIL, LOG_PRIORITY_LOW,
                       &logEntry, sizeof(DIO_WRAPAROUND_FAIL_LOG), NULL);
        GSE_DebugStr(NORMAL, TRUE, logBuffer);
        waFailLogRecorded[i] = TRUE;
      }
    }
    else
    {
      wrapFailCounts[i] = 0;
    }
  }
}

/*****************************************************************************
* Function:    DIO_DispProtocolSetAddress
*
* Description: Update the virtual register for the Display Discrete DIO Inputs
*
* Parameters:  const char* address - The address of local Display Discrete 
*                                    storage.
*              const CHAR* flagAddress – Address of the sensor validation flag.
*              const CHAR* d_HLTHAddress – Address of the D_HLTH cod byte.
*              const CHAR* dispStatus – The address of the Display Status Enum.
*  
*
* Returns:     None
*
* Notes:       None
****************************************************************************/
void DIO_DispProtocolSetAddress(const CHAR* address,
                                const CHAR* flagAddress, 
                                const CHAR* d_HLTHAddress,
                                const CHAR* dispStatus)
{
  UINT16 i = 0;
  for (i = 0; i < DIO_MAX_INPUTS; i++)
  {
    if (dio_InputPins[i].peripheral == DIO_DISP)
    {
      dio_InputPins[i].dataReg = (volatile UINT8*)(void*)(&address[0]);
    }
  }
  displayValidationFlag = (volatile UINT8*)(void*)(flagAddress);
  displayHealth         = (volatile UINT8*)(void*)(d_HLTHAddress);
  displayStatus         = (volatile UINT8*)(void*)(dispStatus);
}

/*****************************************************************************
* Function:    DIO_SensorTest
*
* Description: Returns whether or not DIO input is considered valid or 
*              invalid by sensor.c.
*
* Parameters:  UINT16 nIndex - Index of the DIO channel
*
* Returns:     BOOLEAN bValid - Valid(TRUE) or Invalid(FALSE).
*
* Notes:       None
****************************************************************************/
BOOLEAN DIO_SensorTest(UINT16 nIndex)
{
  BOOLEAN bValid;
  DIO_PERIPHERAL peripheralID;
  DIO_DISPLAY_FAIL_LOG displayLog;
  DIO_DISPLAY_SENSOR_STATUS_PTR pDisp;

  peripheralID = dio_InputPins[nIndex].peripheral;
  bValid = TRUE;

  //There is Validity to Check for the Display Discretes
  switch(peripheralID)
  {
    case DIO_DISP:
      pDisp = (DIO_DISPLAY_SENSOR_STATUS_PTR)&m_DisplayDiscretes
              [nIndex - (UINT16)DispDisc0];

      // Return status flag or NULL if status flag has not been initialized
      bValid = DIO_R(displayValidationFlag);
      ++(pDisp->testCnt);

      if (!bValid && pDisp->bValid)
      {
        displayLog.discreteID    = pDisp->id;
        displayLog.displayHealth = DIO_R(displayHealth);
        displayLog.failCount     = ++(pDisp->failCnt);
        displayLog.totalTests    = pDisp->testCnt;
        displayLog.failReason    = 
          (DIO_DISPLAY_LOG_RESULT_ENUM)(DIO_R(displayStatus));
        LogWriteSystem(SYS_ID_DIO_DISPLAY_FAIL, LOG_PRIORITY_LOW,
                       &displayLog, sizeof(DIO_DISPLAY_FAIL_LOG), NULL);
      }
      pDisp->bValid = bValid;
      break;
    case DIO_GPIO:
    case DIO_FPGA:
    case DIO_TMR2:
    default:
      break;
  }
  return(bValid);
}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: DIO.c $
 * 
 * *****************  Version 59  *****************
 * User: John Omalley Date: 2/10/16    Time: 9:20a
 * Updated in $/software/control processor/code/drivers
 * SCR 1303 - Code Review Updates
 * 
 * *****************  Version 58  *****************
 * User: John Omalley Date: 1/29/16    Time: 9:00a
 * Updated in $/software/control processor/code/drivers
 * SCR 1302 - Display Protocol Updates
 * 
 * *****************  Version 57  *****************
 * User: John Omalley Date: 1/21/16    Time: 4:35p
 * Updated in $/software/control processor/code/drivers
 * SCR 1302 - Display Protocol Discrete Inputs
 * 
 * *****************  Version 56  *****************
 * User: Contractor V&v Date: 12/12/12   Time: 7:20p
 * Updated in $/software/control processor/code/drivers
 * Code Review  add STATIC macro support for G4E
 *
 * *****************  Version 55  *****************
 * User: John Omalley Date: 12-12-11   Time: 4:27p
 * Updated in $/software/control processor/code/drivers
 * SCR 1197 - Code Review Updates
 *
 * *****************  Version 54  *****************
 * User: Jim Mood     Date: 12/05/12   Time: 8:07p
 * Updated in $/software/control processor/code/drivers
 * SCR 1153.  Reg Check failure caused by new FPGA FFD Input
 *
 * *****************  Version 53  *****************
 * User: John Omalley Date: 12-11-14   Time: 7:12p
 * Updated in $/software/control processor/code/drivers
 * SCR 1076 - Code Review Updates
 *
 * *****************  Version 52  *****************
 * User: Melanie Jutras Date: 12-11-05   Time: 2:21p
 * Updated in $/software/control processor/code/drivers
 * SCR #1196 PCLint 641 Warning copying enum into a UINT16 could cause a
 * problem.  Modified target variable to be a UINT32.
 *
 * *****************  Version 51  *****************
 * User: Melanie Jutras Date: 12-11-01   Time: 3:19p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 File Format Errors
 *
 * *****************  Version 50  *****************
 * User: Peter Lee    Date: 12-10-27   Time: 5:09p
 * Updated in $/software/control processor/code/drivers
 * SCR #1191 Returns update time of param
 *
 * *****************  Version 49  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 7:08p
 * Updated in $/software/control processor/code/drivers
 * SCR# 1142 - Windows Emulation Update and Typos
 *
 * *****************  Version 48  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 47  *****************
 * User: John Omalley Date: 12-08-16   Time: 4:17p
 * Updated in $/software/control processor/code/drivers
 * SCR 1153 - Temp fix to stop reg check init failure on startup
 *
 * *****************  Version 46  *****************
 * User: Jim Mood     Date: 7/19/12    Time: 10:42a
 * Updated in $/software/control processor/code/drivers
 * SCR 1107: Data Offload changes for 2.0.0
 *
 * *****************  Version 45  *****************
 * User: Jeff Vahue   Date: 10/18/10   Time: 7:03p
 * Updated in $/software/control processor/code/drivers
 * SCR# 848: Code Coverage
 *
 * *****************  Version 44  *****************
 * User: Jeff Vahue   Date: 10/18/10   Time: 4:50p
 * Updated in $/software/control processor/code/drivers
 * SCR # 848 - Code Coverage
 *
 * *****************  Version 43  *****************
 * User: Jim Mood     Date: 10/18/10   Time: 11:59a
 * Updated in $/software/control processor/code/drivers
 * SCR 943
 *
 * *****************  Version 42  *****************
 * User: Peter Lee    Date: 9/30/10    Time: 11:12a
 * Updated in $/software/control processor/code/drivers
 * SCR #908 Misc Code Coverage Issues
 *
 * *****************  Version 40  *****************
 * User: Contractor2  Date: 9/27/10    Time: 2:55p
 * Updated in $/software/control processor/code/drivers
 * SCR #795 Code Review Updates
 *
 * *****************  Version 39  *****************
 * User: John Omalley Date: 9/17/10    Time: 1:38p
 * Updated in $/software/control processor/code/drivers
 * SCR 871 - Removed WOW check
 *
 * *****************  Version 38  *****************
 * User: John Omalley Date: 9/10/10    Time: 10:00a
 * Updated in $/software/control processor/code/drivers
 * SCR 858 - Added FATAL to defaults
 *
 * *****************  Version 37  *****************
 * User: Jeff Vahue   Date: 7/29/10    Time: 4:04p
 * Updated in $/software/control processor/code/drivers
 * SCR# 698 - clean up
 *
 * *****************  Version 36  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:10a
 * Updated in $/software/control processor/code/drivers
 * SCR #698 - Fix code review findings
 *
 * *****************  Version 35  *****************
 * User: Jeff Vahue   Date: 7/17/10    Time: 5:46p
 * Updated in $/software/control processor/code/drivers
 * SCR# 707 - Add TestPoints for code coverage.
 *
 * *****************  Version 34  *****************
 * User: Peter Lee    Date: 5/31/10    Time: 5:37p
 * Updated in $/software/control processor/code/drivers
 * SCR #601 FPGA DIO Logic Updates/Bug fixes
 *
 * *****************  Version 33  *****************
 * User: Contractor2  Date: 5/25/10    Time: 3:01p
 * Updated in $/software/control processor/code/drivers
 * SCR #563 Implementation: Req SRS-1811,1813,1814,1815,3162 - CBIT RAM
 * Tests
 * Added ram test protected sections. Coding standard fixes.
 *
 * *****************  Version 32  *****************
 * User: Contractor V&v Date: 5/07/10    Time: 4:35p
 * Updated in $/software/control processor/code/drivers
 * SCR #548 Strings in Logs
 *
 * *****************  Version 31  *****************
 * User: Contractor3  Date: 4/09/10    Time: 12:17p
 * Updated in $/software/control processor/code/drivers
 * SCR #539 Updated for Code Review
 *
 * *****************  Version 30  *****************
 * User: Jeff Vahue   Date: 4/08/10    Time: 10:47a
 * Updated in $/software/control processor/code/drivers
 * SCR #533 - Only check WA when WOW is enabled.
 *
 * *****************  Version 29  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:08p
 * Updated in $/software/control processor/code/drivers
 * SCR  #70, SCR #533
 *
 * *****************  Version 28  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:36p
 * Updated in $/software/control processor/code/drivers
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 *
 * *****************  Version 27  *****************
 * User: Contractor V&v Date: 3/19/10    Time: 4:29p
 * Updated in $/software/control processor/code/drivers
 * SCR #377 Debounce Discrete Pins Moved DebounceState CM_GetHsCou
 *
 * *****************  Version 26  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/drivers
 * SCR# 483 - Function Names
 *
 * *****************  Version 25  *****************
 * User: Jeff Vahue   Date: 3/10/10    Time: 2:34p
 * Updated in $/software/control processor/code/drivers
 * SCR# 480 - add semi's after ASSERTs
 *
 * *****************  Version 24  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 5:55p
 * Updated in $/software/control processor/code/drivers
 * SCR 450 DIN state should be initialized right after DIO init is
 *
 * *****************  Version 23  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 3:47p
 * Updated in $/software/control processor/code/drivers
 * SCR #371 Debounce Discrete Pins
 *
 * *****************  Version 22  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:14p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 21  *****************
 * User: Jeff Vahue   Date: 2/26/10    Time: 3:00p
 * Updated in $/software/control processor/code/drivers
 * SCR# 469 - Change DIO_ReadPin to return the pin value, WIN32 handling
 * of _DIR and _RIR
 *
 * *****************  Version 20  *****************
 * User: Jeff Vahue   Date: 2/19/10    Time: 12:57p
 * Updated in $/software/control processor/code/drivers
 * SCR# 455 - remove LINT issues
 *
 * *****************  Version 19  *****************
 * User: Jeff Vahue   Date: 2/17/10    Time: 12:57p
 * Updated in $/software/control processor/code/drivers
 * SCR# 452 - Code coverage if/then/else removal
 *
 * *****************  Version 18  *****************
 * User: Contractor V&v Date: 1/27/10    Time: 5:21p
 * Updated in $/software/control processor/code/drivers
 * SCR 340, SCR 371
 *
 * *****************  Version 17  *****************
 * User: Contractor V&v Date: 1/13/10    Time: 4:58p
 * Updated in $/software/control processor/code/drivers
 *
 * *****************  Version 16  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:23p
 * Updated in $/software/control processor/code/drivers
 * SCR 371
 *
 * *****************  Version 15  *****************
 * User: Contractor V&v Date: 12/22/09   Time: 2:16p
 * Updated in $/software/control processor/code/drivers
 * SCR 337 Debounce DIN
 *
 * *****************  Version 14  *****************
 * User: Jeff Vahue   Date: 12/18/09   Time: 1:34p
 * Updated in $/software/control processor/code/drivers
 * SCR# 378
 *
 * *****************  Version 13  *****************
 * User: Contractor V&v Date: 11/18/09   Time: 3:36p
 * Updated in $/software/control processor/code/drivers
 * Implement SCR 172 ASSERT processing for all "default" case processing
 * Implement FATAL fof SCR 123 DIO_ReadPin() returns RESULT
 *
 * *****************  Version 12  *****************
 * User: Peter Lee    Date: 11/05/09   Time: 5:30p
 * Updated in $/software/control processor/code/drivers
 * SCR #315 DIO CBIT for SEU Processing
 *
 * *****************  Version 11  *****************
 * User: Peter Lee    Date: 10/23/09   Time: 10:01a
 * Updated in $/software/control processor/code/drivers
 * Minor updates to support JV's PC program.
 *
 * *****************  Version 10  *****************
 * User: Peter Lee    Date: 10/20/09   Time: 6:49p
 * Updated in $/software/control processor/code/drivers
 * Updates for JV to support PC version of program
 *
 * *****************  Version 9  *****************
 * User: John Omalley Date: 10/01/09   Time: 4:31p
 * Updated in $/software/control processor/code/drivers
 * Added function for discrete sensors
 *
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 9/15/09    Time: 6:01p
 * Updated in $/software/control processor/code/drivers
 * SCR #94, #95 PBIT returns log structure to Init Mgr
 *
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 9/14/09    Time: 3:23p
 * Updated in $/software/control processor/code/drivers
 * SCR #94 Updated Init for SET_CHECK() and return ERR CODE Struct
 *
 * *****************  Version 6  *****************
 * User: Jim Mood     Date: 2/05/09    Time: 11:25a
 * Updated in $/control processor/code/drivers
 * Added support for FPGA GPIO.  Added the 232/422 mode select, CAN0 wake,
 * and CAN1 wage pins in the FPGA GPIO register.
 *
 * *****************  Version 5  *****************
 * User: Jim Mood     Date: 12/05/08   Time: 5:13p
 * Updated in $/control processor/code/drivers
 * Added the DIO_Out_MSRst pin (MCF5485 - T2OUT) and the code to control
 * that pin in the GPT2 (general purpose timer) peripheral.
 *
 * *****************  Version 4  *****************
 * User: Jim Mood     Date: 10/01/08   Time: 4:39p
 * Updated in $/control processor/code/drivers
 * Fixed the DIO_SetPin toggle function.  Updated comment blocks to the
 * PWES style and added the VSS version/date/history macros
 *
 *
 ****************************************************************************/
