#define DRV_ADC_BODY

/*----------------------------------------------------------------------------
            Copyright (C) 2007 - 2011 Pratt & Whitney Engine Services, Inc. 
                      Altair Engine Diagnostic Solutions
               All Rights Reserved. Proprietary and Confidential.
 *
 * File:        Adc.c
 *
 * Description: Driver for the LT1595 ADC, connected by the SPI bus
 *              This 12-bit single channel 4-input ADC is assumed to have 
 *              the following signals tied to its 4-input MUX
 *
 *              Mux Select  Channel                       Scale
 *              0           A/C Bus Voltage               
 *              1           Internal Li battery voltage   1V/V
 *              2           A/C Battery Voltage           
 *              3           FAST mainboard temperature    10mV/ *C .750V @ 25*C
 *
 * Requires:    DIO.h - Controlling the Li battery switch
 *              SPI.h - Accessing the SPI bus to read the converter
 *
 * VERSION
 *     $Revision: 27 $  $Date: 10/11/11 2:10p $ 
 *    
 *
 *--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* Compiler Specific Includes                                               */
/*--------------------------------------------------------------------------*/
#include "mcf548x.h"

/*--------------------------------------------------------------------------*/
/* Altair Specific Includes                                                 */
/*--------------------------------------------------------------------------*/
#include "SPI.h"
#include "ADC.h"
#include "DIO.h"
#include "alt_basic.h"
#include "UtilRegSetCheck.h"
#include "Assert.h"

#include "TestPoints.h"

/*--------------------------------------------------------------------------*/
/* Local Defines                                                            */
/*--------------------------------------------------------------------------*/
#define LI_BATT_ON    DIO_SetPin(LiBattToADC,DIO_SetHigh);
#define LI_BATT_OFF   DIO_SetPin(LiBattToADC,DIO_SetLow);

#define TTMR_uSEC_DELAY    50

#define ADC_AD_ENABLE_BIT   0x8  //Bit 3 (0x8) is the A/D ENABLE bit

#define BATT_CAL_FACTOR1    -0.0029f
#define BATT_CAL_FACTOR2     1.1001f
#define MIN_OFF_VOLTAGE      0.5f
/*---------------------------------------------------------------------------*/
/* Local Typedefs                                                            */
/*---------------------------------------------------------------------------*/
typedef struct {
  FLOAT32 adcValue; 
  RESULT  adcResult; 
} ADC_DATA; 

/*--------------------------------------------------------------------------*/
/* Local Variables                                                          */
/*--------------------------------------------------------------------------*/


const REG_SETTING ADC_Registers[] = 
{
  //Set FEC1L7 port to GPIO output.
  // SET_CHECK_MASK_OR(MCF_GPIO_PDDR_FEC1L, MCF_GPIO_PDDR_FEC1L_PDDR_FEC1L7, bInitOk); 
  {(void *) &MCF_GPIO_PDDR_FEC1L, MCF_GPIO_PDDR_FEC1L_PDDR_FEC1L7, sizeof(UINT8), 0x0, 
            REG_SET_MASK_OR, TRUE, RFA_FORCE_UPDATE}

};

static ADC_DATA m_ADC_ChanData[ADC_CHAN_MAX]; 


/*--------------------------------------------------------------------------*/
/* Local Function Prototypes                                                */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* Public Functions                                                         */
/*--------------------------------------------------------------------------*/

/*****************************************************************************
 * Function:    ADC_Init
 *
 * Description: Setup the initial state for the ADC driver module.
 *
 *              Sets the GPIO port FEC1L7 to output low.  This line controls
 *              the switch for applying the Li backup battery voltage to
 *              channel 2 of the ADC.
 *
 * Parameters:  SysLogId - Ptr to return System Log ID into 
 *              pdata - Ptr to buffer to return system log data 
 *              psize - Ptr to return size (bytes) of system log data
 *
 * Returns:     Driver operation result: DRV_OK  : Conversion success
 *                                       !DRV_OK : See ResultCodes.h
 * Notes:       
 ****************************************************************************/
RESULT ADC_Init (SYS_APP_ID *SysLogId, void *pdata, UINT16 *psize)
{
  BOOLEAN bInitOk; 
  ADC_DRV_PBIT_LOG  *pdest; 
  UINT16 i; 

  
  pdest = (ADC_DRV_PBIT_LOG *) pdata;
  memset ( pdest, 0, sizeof(ADC_DRV_PBIT_LOG) ); 
  pdest->result = DRV_OK; 
  
  *psize = sizeof(ADC_DRV_PBIT_LOG); 
  
  
  // Initialize the ADC Registers 
  bInitOk = TRUE; 
  for (i=0;i<(sizeof(ADC_Registers)/sizeof(REG_SETTING));i++) 
  {
    bInitOk &= RegSet( (REG_SETTING_PTR) &ADC_Registers[i] ); 
  }

  if ( TRUE != STPU( bInitOk, eTpAdc3807)) 
  {
    pdest->result = DRV_ADC_PBIT_REG_INIT_FAIL; 
    *SysLogId = DRV_ID_ADC_PBIT_RET_INIT_FAIL; 
  }
  
  //Turn Li backup battery switch off
  LI_BATT_OFF;
  
  // Init module variables 
  for (i=0;i<ADC_CHAN_MAX;i++)
  {
    m_ADC_ChanData[i].adcValue = 0.0f; 
    m_ADC_ChanData[i].adcResult = DRV_OK; 
  }

  return pdest->result;
}

/*****************************************************************************
 * Function:    ADC_GetACBusVoltage
 *
 * Description: Wrapper function for returning the Aircraft Bus voltage.
 *
 * Parameters:  [out] *ACBusVoltage : Aircraft bus voltage
 *
 * Returns:     Driver operation result: DRV_OK  : Conversion success
 *                                       !DRV_OK : See ResultCodes.h
 *
 * Notes:       Wrapper function for values managed by SpiManager 
 *
 ****************************************************************************/
RESULT ADC_GetACBusVoltage(FLOAT32 *ACBusVoltage)
{  
  FLOAT32         currVal; 
  
  *ACBusVoltage = m_ADC_ChanData[ADC_CHAN_AC_BUS_V].adcValue; // Get previous good value
  m_ADC_ChanData[ADC_CHAN_AC_BUS_V].adcResult = SPIMgr_GetACBusVoltage((FLOAT32 *) &currVal); 

  if ( m_ADC_ChanData[ADC_CHAN_AC_BUS_V].adcResult == DRV_OK ) 
  {
    m_ADC_ChanData[ADC_CHAN_AC_BUS_V].adcValue = currVal; // Update current val if good 
    *ACBusVoltage = currVal; 
  }
  // Knowlogic will like this one, no ELSE !! 

  return m_ADC_ChanData[ADC_CHAN_AC_BUS_V].adcResult;  
}


/*****************************************************************************
 * Function:    ADC_GetACBattVoltage
 *
 * Description: Wrapper function for returning the current Aircraft Battery
 *              voltage.
 * Parameters:  [out] *ACBattVoltage : Aircraft battery voltage.
 *
 * Returns:     Driver operation result: DRV_OK  : Conversion success
 *                                       !DRV_OK : See ResultCodes.h
 *
 * Notes:       Wrapper function for values managed via SpiManager
 *
 ****************************************************************************/
RESULT ADC_GetACBattVoltage (FLOAT32* ACBattVoltage)
{  
  FLOAT32         currVal; 
  
  *ACBattVoltage = m_ADC_ChanData[ADC_CHAN_AC_BATT_V].adcValue; // Get previous good value
  m_ADC_ChanData[ADC_CHAN_AC_BATT_V].adcResult = SPIMgr_GetACBattVoltage((FLOAT32 *) &currVal); 

  if ( m_ADC_ChanData[ADC_CHAN_AC_BATT_V].adcResult == DRV_OK ) 
  {
    m_ADC_ChanData[ADC_CHAN_AC_BATT_V].adcValue = currVal; // Update current val if good 
    *ACBattVoltage = currVal; 
  }
  // Knowlogic will like this one, no ELSE !! 

  return m_ADC_ChanData[ADC_CHAN_AC_BATT_V].adcResult;  
}


/*****************************************************************************
 * Function:    ADC_GetLiBattVoltage
 *
 * Description: Wrapper function for returning the lithium battery voltage 
 *              channel and convert A/D result into a floating point value in voltage units
 *
 * Parameters:  [out] *Voltage : Lithium battery voltage
 *
 * Returns:     Driver operation result: DRV_OK  : Conversion success
 *                                       !DRV_OK : See ResultCodes.h
 * Notes:       Wrapper function for values managed via SpiManager
 *
 ****************************************************************************/
RESULT ADC_GetLiBattVoltage(FLOAT32* Voltage)
{
  FLOAT32         currVal; 
  
  *Voltage = m_ADC_ChanData[ADC_CHAN_LI_BATT_V].adcValue; // Get previous good value
  m_ADC_ChanData[ADC_CHAN_LI_BATT_V].adcResult = SPIMgr_GetLiBattVoltage((FLOAT32 *) &currVal); 

  if ( m_ADC_ChanData[ADC_CHAN_LI_BATT_V].adcResult == DRV_OK ) 
  {
    m_ADC_ChanData[ADC_CHAN_LI_BATT_V].adcValue = currVal; // Update current val if good 
    *Voltage = currVal; 
  }
  // Knowlogic will like this one, no ELSE !! 

  return m_ADC_ChanData[ADC_CHAN_LI_BATT_V].adcResult;  

}


/*****************************************************************************
 * Function:    ADC_GetBoardTemp
 *
 * Description: Wrapper function for returning the temperature in *C from the
*               on-board temperature sensor
 *
 * Parameters:  [out] *Temp : Temperature in degrees centigrade
 *
 * Returns:     Driver operation result: DRV_OK  : Conversion success
 *                                       !DRV_OK : See ResultCodes.h
 *
 * Notes:       Wrapper function for values managed via SpiManager
 *
 ****************************************************************************/
RESULT ADC_GetBoardTemp (FLOAT32* Temp)
{
  FLOAT32         currVal; 
  
  *Temp = m_ADC_ChanData[ADC_CHAN_TEMP].adcValue; // Get previous good value
  m_ADC_ChanData[ADC_CHAN_TEMP].adcResult = SPIMgr_GetBoardTemp((FLOAT32 *) &currVal); 

  if ( m_ADC_ChanData[ADC_CHAN_TEMP].adcResult == DRV_OK ) 
  {
    m_ADC_ChanData[ADC_CHAN_TEMP].adcValue = currVal; // Update current val if good 
    *Temp = currVal; 
  }
  // Knowlogic will like this one, no ELSE !! 

  return m_ADC_ChanData[ADC_CHAN_TEMP].adcResult;  
}


/*****************************************************************************
* Function:    ADC_ReadACBusVoltage
*
* Description: Read and calculate the current Aircraft Bus voltage.
*
* Parameters:  [out] *ACBusVoltage : Aircraft bus voltage
*
* Returns:     Driver operation result: DRV_OK  : Conversion success
*                                       !DRV_OK : See ResultCodes.h
*
* Notes:       DO NOT CALL DIRECTLY, Call ADC_GetACBusVoltage 
*
****************************************************************************/
RESULT ADC_ReadACBusVoltage(FLOAT32 *ACBusVoltage)
{
  UINT16 ADReading;
  RESULT result;

  result = ADC_ReadRaw( ADC_CHAN_AC_BUS_V, &ADReading);

  *ACBusVoltage = (ADReading * ADC_V_PER_BIT * ADC_BUS_V_PER_V);
  *ACBusVoltage = (*ACBusVoltage > MIN_OFF_VOLTAGE) ? 
                   *ACBusVoltage + ADC_BUS_V_TRANSITOR_DROP : 0;

  return result;
}


/*****************************************************************************
* Function:    ADC_ReadACBattVoltage
*
* Description: Read and calculate the current Aircraft Battery voltage.
*
* Parameters:  [out] *ACBattVoltage : Aircraft battery voltage.
*
* Returns:     Driver operation result: DRV_OK  : Conversion success
*                                       !DRV_OK : See ResultCodes.h
*
* Notes:       DO NOT CALL DIRECTLY, Call ADC_GetACBattVoltage
*
****************************************************************************/
RESULT ADC_ReadACBattVoltage (FLOAT32* ACBattVoltage)
{
  UINT16 ADReading;
  FLOAT32 TempVoltage;
  RESULT result;

  result = ADC_ReadRaw( ADC_CHAN_AC_BATT_V, &ADReading);

  TempVoltage = ADReading * ADC_V_PER_BIT * ADC_BATT_V_PER_V;

  // Voltage Formula Provided by Hardware Engineering Group
  *ACBattVoltage = (BATT_CAL_FACTOR1 * (TempVoltage * TempVoltage)) + 
                   (BATT_CAL_FACTOR2 * TempVoltage);
  return result;
}


/*****************************************************************************
* Function:    ADC_ReadLiBattVoltage
*
* Description: Read the lithium battery voltage channel and convert A/D
*              result into a floating point value in voltage units
*
* Parameters:  [out] *Voltage : Lithium battery voltage
*
* Returns:     Driver operation result: DRV_OK  : Conversion success
*                                       !DRV_OK : See ResultCodes.h
* Notes:       DO NOT CALL DIRECTLY, Call ADC_GetLiBattVoltage
*
****************************************************************************/
RESULT ADC_ReadLiBattVoltage(FLOAT32* Voltage)
{
  UINT16 ADReading;
  RESULT result = DRV_OK;

  result = ADC_ReadRaw(ADC_CHAN_LI_BATT_V,&ADReading);

  *Voltage = ADReading * ADC_V_PER_BIT;

  return result;
}

/*****************************************************************************
* Function:    ADC_ReadBoardTemp
*
* Description: Read and calculate the temperature in *C from the on-board
*              temperature sensor
*
* Parameters:  [out] *Temp : Temperature in degrees centigrade
*
* Returns:     Driver operation result: DRV_OK  : Conversion success
*                                       !DRV_OK : See ResultCodes.h
*
* Notes:       DO NOT CALL DIRECTLY, Call SPIMgr_GetBoardTemp
*
****************************************************************************/
RESULT ADC_ReadBoardTemp (FLOAT32* Temp)
{
  UINT16 ADReading;
  RESULT result = DRV_OK;

  result = ADC_ReadRaw(ADC_CHAN_TEMP,&ADReading);       

  *Temp = (ADReading * ADC_V_PER_BIT * ADC_TEMP_DEG_PER_V) - ADC_TEMP_DEG_OFFSET;

  return result;
}


/*****************************************************************************
 * Function:    ADC_ReadRaw
 *
 * Description: Reads the 12-bit A/D converter result from the specified
 *              channel
 *
 * Parameters:  [in]  Channel - ADC channel to read. See enum ADC_CHANNEL
 *              [out] *ADCResult - Pointer to a location to save the A/D
 *                                 conversion result
 *
 * Returns:     Driver operation result: DRV_OK  : Conversion success
 *                                       !DRV_OK : See ResultCodes.h
 *
 * Notes:       1.This routine will block until the conversion is complete.
 *                Conversion time depends on the SPI clock rate.  If the SPI
 *                clock rate is 320kHz, the time to complete a conversion
 *                is <60us.
 *
 *                See the SPI driver module and the LT1594 ADC data sheet for
 *                details
 *              2.The routine handles the switch to connect/disconnect the
 *                lithium battery to/from mux channel 1 of the A/D. The A/D
 *                communication overhead (writing 4 bits) provides a delay
 *                of approximately 12us between turning on the switch to 
 *                when the mux switches to channel 1.  This assumes a 320kHz 
 *                SPI clock.
 ****************************************************************************/
RESULT ADC_ReadRaw (ADC_CHANNEL Channel, UINT16* ADCResult)
{
 UINT8  MuxSelect;
 RESULT result = DRV_OK;
 
  if(Channel == ADC_CHAN_LI_BATT_V)             //Check the channel, turn on Li
  {                                             //battery switch if necessary
    LI_BATT_ON;
    TTMR_Delay(TTMR_uSEC_DELAY * TICKS_PER_uSec);  //Delay 50uSecs before reading signal
  }

  MuxSelect = ADC_AD_ENABLE_BIT | (UINT8)Channel;  //Bit 3 (0x8) is the A/D ENABLE bit
  result = SPI_WriteByte(SPI_DEV_ADC_WR,&MuxSelect,TRUE);

  if (result == DRV_OK)
  {
    result = SPI_ReadWord(SPI_DEV_ADC_RD,ADCResult,FALSE);
    *ADCResult &= 0xFFF;
  }

  if(Channel == ADC_CHAN_LI_BATT_V)             //Check the channel and turn
  {                                             //the Li switch back off if necessary
    LI_BATT_OFF;    
  }

  return result;

}


/******************************************************************************
 * Function:     ADC_SensorTest
 *
 * Description:  Determines if sensor data is valid.  
 *
 * Parameters:   nIndex - index of ADC channel
 *
 * Returns:      FALSE - If word data invalid
 *               TRUE  - If word data valid 
 *
 * Notes:        none
 *
 *****************************************************************************/
BOOLEAN ADC_SensorTest (UINT16 nIndex)
{
  BOOLEAN   bValid; 
  UINT32    spiId;
      
  switch (nIndex)
  {
  case ADC_CHAN_AC_BUS_V:
    spiId = SPI_AC_BUS_VOLT;
    break;
  case ADC_CHAN_LI_BATT_V:
    spiId = SPI_LIT_BAT_VOLT;
    break;
  case ADC_CHAN_AC_BATT_V:
    spiId = SPI_AC_BAT_VOLT;
    break;      
  case ADC_CHAN_TEMP:
    spiId = SPI_BOARD_TEMP;
    break;
  default:
    FATAL("Unrecognized ADC channel param = %d", nIndex);
    break;
  }

  bValid = SPIMgr_SensorTest(spiId);

  return TPU( bValid, eTpAdc3931);
}


/*****************************************************************************
 * Function:    ADC_GetValue
 *
 * Description: This function is used by the sensor object to read the 
 *              different channels of the internal ADC.
 *
 * Parameters:  [in]  nIndex - ADC channel to read. See enum ADC_CHANNEL
 *
 * Returns:     fValue - float Value of the discrete
 *
 * Notes:       
 ****************************************************************************/
FLOAT32 ADC_GetValue (UINT16 nIndex)
{
   // Local Data
   FLOAT32 fValue;
      
   switch (nIndex)
   {
      case ADC_CHAN_AC_BUS_V:
         ADC_GetACBusVoltage(&fValue);
         break;
      case ADC_CHAN_LI_BATT_V:
         ADC_GetLiBattVoltage(&fValue);
         break;
      case ADC_CHAN_AC_BATT_V:
         ADC_GetACBattVoltage (&fValue);
         break;      
      case ADC_CHAN_TEMP:
         ADC_GetBoardTemp (&fValue);
         break;
      default:
         FATAL("Unrecognized ADC channel param = %d", nIndex);
         break;
   }
   
   return (fValue);   
}


/*************************************************************************
 *  MODIFICATIONS
 *    $History: ADC.c $
 * 
 * *****************  Version 27  *****************
 * User: Jeff Vahue   Date: 10/11/11   Time: 2:10p
 * Updated in $/software/control processor/code/drivers
 * SCR# 1077 - ADC CBIT Test
 * 
 * *****************  Version 26  *****************
 * User: Contractor2  Date: 5/26/11    Time: 1:17p
 * Updated in $/software/control processor/code/drivers
 * SCR #767 Enhancement - BIT for Analog Sensors
 * 
 * *****************  Version 25  *****************
 * User: John Omalley Date: 10/25/10   Time: 6:03p
 * Updated in $/software/control processor/code/drivers
 * SCR 961 - Code Review Updates
 * 
 * *****************  Version 24  *****************
 * User: John Omalley Date: 10/19/10   Time: 4:40p
 * Updated in $/software/control processor/code/drivers
 * SCR 653 - ADC Bus and Battery Formula Updates
 * 
 * *****************  Version 23  *****************
 * User: Jeff Vahue   Date: 10/15/10   Time: 4:21p
 * Updated in $/software/control processor/code/drivers
 * SCR# 848 - Code Coverage this needed a STPU vs a TPU
 * 
 * *****************  Version 22  *****************
 * User: Contractor2  Date: 10/14/10   Time: 2:39p
 * Updated in $/software/control processor/code/drivers
 * SCR# 848 - Code Coverage
 * Added testpoints.h
 * 
 * *****************  Version 21  *****************
 * User: Jeff Vahue   Date: 10/14/10   Time: 2:26p
 * Updated in $/software/control processor/code/drivers
 * SCR# 848 - Code Coverage
 * 
 * *****************  Version 20  *****************
 * User: Peter Lee    Date: 9/28/10    Time: 1:56p
 * Updated in $/software/control processor/code/drivers
 * SCR #892 Code Review Updates
 * 
 * *****************  Version 19  *****************
 * User: Peter Lee    Date: 8/04/10    Time: 11:32a
 * Updated in $/software/control processor/code/drivers
 * SCR #653 More accurate BATT Volt Reading
 * 
 * *****************  Version 18  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:10a
 * Updated in $/software/control processor/code/drivers
 * SCR #698 - Fix code review findings
 * 
 * *****************  Version 17  *****************
 * User: Peter Lee    Date: 7/28/10    Time: 5:36p
 * Updated in $/software/control processor/code/drivers
 * SCR #699 Return prev ADC value if ADC read fails rather than value off
 * the "stack". 
 * 
 * *****************  Version 16  *****************
 * User: Contractor V&v Date: 3/10/10    Time: 4:40p
 * Updated in $/software/control processor/code/drivers
 * SCR #67 fix funct call and spelling error
 * 
 * *****************  Version 15  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 3:46p
 * Updated in $/software/control processor/code/drivers
 * SCR #67 Interrupted SPI Access (Multiple / Nested SPI Access)
 * SCR #475 Delay 50uSecs before reading LiBat voltage
 * 
 * *****************  Version 14  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:02p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 13  *****************
 * User: Jeff Vahue   Date: 2/09/10    Time: 11:09a
 * Updated in $/software/control processor/code/drivers
 * SCR# 404 - PC-Lint cleanup, formatting
 * 
 * *****************  Version 12  *****************
 * User: Jeff Vahue   Date: 2/01/10    Time: 12:40p
 * Updated in $/software/control processor/code/drivers
 * Typos in comments, remove an empty else.
 * 
 * *****************  Version 11  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:20p
 * Updated in $/software/control processor/code/drivers
 * SCR 371 and 377
 * 
 * *****************  Version 10  *****************
 * User: Contractor V&v Date: 11/18/09   Time: 3:15p
 * Updated in $/software/control processor/code/drivers
 * Fix for SCR 172
 * 
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 10/26/09   Time: 5:28p
 * Updated in $/software/control processor/code/drivers
 * SCR #315 CBIT and SEU Processing 
 * 
 * *****************  Version 8  *****************
 * User: John Omalley Date: 10/01/09   Time: 4:31p
 * Updated in $/software/control processor/code/drivers
 * Added Function for analog sensors.
 * 
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 9/15/09    Time: 6:05p
 * Updated in $/software/control processor/code/drivers
 * SCR #94, #95 PBIT returns log structure to Init Mgr
 * 
 * *****************  Version 6  *****************
 * User: Peter Lee    Date: 9/14/09    Time: 3:21p
 * Updated in $/software/control processor/code/drivers
 * SCR #94 Updated Init for SET_CHECK() and return ERR CODE Struct
 * 
 * *****************  Version 5  *****************
 * User: Peter Lee    Date: 6/29/09    Time: 11:58a
 * Updated in $/software/control processor/code/drivers
 * SCR #204 Misc Code Review Updates. 
 * 
 * *****************  Version 4  *****************
 * User: Peter Lee    Date: 10/08/08   Time: 11:51a
 * Updated in $/control processor/code/drivers
 * Updates required for PM
 * 
 *
 ***************************************************************************/
 

