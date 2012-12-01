#ifndef ADC_H
#define ADC_H
/******************************************************************************
            Copyright (C) 2007-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

  File:        ADC.h

  Description: Driver for the LT1594 ADC connected by the SPI bus

  VERSION
      $Revision: 21 $  $Date: 12-12-01 10:10a $


******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "ResultCodes.h"
#include "SystemLog.h"
#include "SPIManager.h"

/******************************************************************************
                                 Package Defines
******************************************************************************/
//-------------A/D channel scaling and conversion factors--------------------
//
//The drv_SPI.c module has functions that impliment these values
//and provide example for how to use them

//To convert A/D result codes to a voltage
#define ADC_REF_V         3.0F                      // A/D reference voltage
#define ADC_NUM_OF_CODES  4096                      // 12-bit A/D, so 2^12 codes
#define ADC_V_PER_BIT (ADC_REF_V/ADC_NUM_OF_CODES)  // Ref/Codes = 3.0V/4096

//To convert A/D measured voltage on channel 3 to temperature
#define ADC_TEMP_DEG_PER_V  100.0F                  // 100*c/V
#define ADC_TEMP_DEG_OFFSET 50.0F                   // 0.75V @ 25C, so 50*C offset

//To convert A/D voltage to A/C Bus voltage         //Voltage divider ratio:
#define ADC_BUS_V_PER_V           12.2987F          // 1/(1.54K/(1.54K+17.4K))
#define ADC_BUS_V_TRANSITOR_DROP  0.4F              // Transitor Drop

//To convert A/D voltage to A/C Battery voltage     //Voltage divider ratio:
#define ADC_BATT_V_PER_V          12.2842F          // 1/(4.75K/4.75K+53.6K)



/******************************************************************************
                                 Package Typedefs
******************************************************************************/
typedef enum
{
    ADC_CHAN_AC_BUS_V   = 0,   // Aircraft Bus Voltage
    ADC_CHAN_LI_BATT_V  = 1,   // Li Battery
    ADC_CHAN_AC_BATT_V  = 2,   // Aircraft Battery Voltage
    ADC_CHAN_TEMP       = 3,   // Internal ambient temperature
    ADC_CHAN_MAX        = 4    // Delimiter for MAX ADC Channels
} ADC_CHANNEL;

#pragma pack(1)
typedef struct {
  RESULT  result;
} ADC_DRV_PBIT_LOG;
#pragma pack()


/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( ADC_BODY )
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
EXPORT FLOAT32 ADC_GetValue         (UINT16 nIndex, UINT32 *null);
EXPORT RESULT  ADC_Init             (SYS_APP_ID *SysLogId, void *pdata, UINT16 *psize);
EXPORT BOOLEAN ADC_SensorTest       (UINT16 nIndex);

// -- Functions for returning analog values to apps.
EXPORT RESULT ADC_GetACBusVoltage  (FLOAT32* ACBusVoltage);
EXPORT RESULT ADC_GetACBattVoltage (FLOAT32* ACBattVoltage);
EXPORT RESULT ADC_GetLiBattVoltage (FLOAT32* Voltage);
EXPORT RESULT ADC_GetBoardTemp     (FLOAT32* Temp);

// -- Functions to be used ONLY by SpiManager for maintaining analog values.
// -- Apps should call corresponding ADC_GetXXX function declared above.
EXPORT RESULT ADC_ReadACBusVoltage  (FLOAT32* ACBusVoltage);
EXPORT RESULT ADC_ReadACBattVoltage (FLOAT32* ACBattVoltage);
EXPORT RESULT ADC_ReadLiBattVoltage (FLOAT32* Voltage);
EXPORT RESULT ADC_ReadBoardTemp     (FLOAT32* Temp);


#endif // ADC_H

/*************************************************************************
 *  MODIFICATIONS
 *    $History: ADC.h $
 * 
 * *****************  Version 21  *****************
 * User: John Omalley Date: 12-12-01   Time: 10:10a
 * Updated in $/software/control processor/code/drivers
 * SCR 1197 - Code Review Updates
 * 
 * *****************  Version 20  *****************
 * User: Melanie Jutras Date: 12-11-01   Time: 2:44p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 File Format Error
 *
 * *****************  Version 19  *****************
 * User: Peter Lee    Date: 12-10-27   Time: 5:08p
 * Updated in $/software/control processor/code/drivers
 * SCR #1191 Returns update time of param
 *
 * *****************  Version 18  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 17  *****************
 * User: Contractor2  Date: 5/26/11    Time: 1:17p
 * Updated in $/software/control processor/code/drivers
 * SCR #767 Enhancement - BIT for Analog Sensors
 *
 * *****************  Version 16  *****************
 * User: John Omalley Date: 10/19/10   Time: 4:40p
 * Updated in $/software/control processor/code/drivers
 * SCR 653 - ADC Bus and Battery Formula Updates
 *
 * *****************  Version 15  *****************
 * User: Peter Lee    Date: 8/04/10    Time: 11:32a
 * Updated in $/software/control processor/code/drivers
 * SCR #653 More accurate BATT Volt Reading
 *
 * *****************  Version 14  *****************
 * User: Peter Lee    Date: 7/28/10    Time: 5:36p
 * Updated in $/software/control processor/code/drivers
 * SCR #699 Return prev ADC value if ADC read fails rather than value off
 * the "stack".
 *
 * *****************  Version 13  *****************
 * User: Contractor V&v Date: 3/10/10    Time: 4:40p
 * Updated in $/software/control processor/code/drivers
 * SCR #67 fix funct call and spelling error
 *
 * *****************  Version 12  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 3:46p
 * Updated in $/software/control processor/code/drivers
 * SCR 67 Interrupted SPI Access (Multiple / Nested SPI Access)
 *
 * *****************  Version 11  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:02p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 10  *****************
 * User: Jeff Vahue   Date: 2/09/10    Time: 11:08a
 * Updated in $/software/control processor/code/drivers
 * SCR# 404 - PC-Lint parens around macro ADC_V_PER_BIT, other various
 * cleanup, formatting
 *
 * *****************  Version 9  *****************
 * User: John Omalley Date: 10/01/09   Time: 4:31p
 * Updated in $/software/control processor/code/drivers
 * Added Function for analog sensors.
 *
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 9/15/09    Time: 6:11p
 * Updated in $/software/control processor/code/drivers
 * SCR #94, #95 PBIT returns log structure to Init Mgr
 *
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 9/14/09    Time: 3:21p
 * Updated in $/software/control processor/code/drivers
 * SCR #94 Updated Init for SET_CHECK() and return ERR CODE Struct
 *
 * *****************  Version 6  *****************
 * User: Peter Lee    Date: 6/29/09    Time: 12:03p
 * Updated in $/software/control processor/code/drivers
 * SCR #204 Misc Code Review Updates
 *
 * *****************  Version 5  *****************
 * User: Peter Lee    Date: 10/08/08   Time: 11:51a
 * Updated in $/control processor/code/drivers
 * Updates required for PM
 *
 *
 ***************************************************************************/



