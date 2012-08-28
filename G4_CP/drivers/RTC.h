#ifndef DRV_RTC_H
#define DRV_RTC_H

/******************************************************************************
Copyright (C) 2007-20012 Pratt & Whitney Engine Services, Inc.                 
All Rights Reserved. Proprietary and Confidential.

 File:        RTC.h    

 Description: This file defines the interface for the the 
              Dallas DS1306 or DS3234 real-time clock chip 
              connected by the SPI bus. See the c module 
              for a detailed description.

  VERSION
   $Revision: 11 $  $Date: 7/29/10 12:12p $
******************************************************************************/


/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include <stddef.h>

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "ResultCodes.h"
#include "SystemLog.h"
#include "SPIManager.h"

/******************************************************************************
                              Package Defines
******************************************************************************/

//Size of all the clock registers.  Used for multi-byte burst reads of clock data
#define RTC_SIZE_OF_CLOCK_DATA 7 

/******************************************************************************
                              Package Typedefs
******************************************************************************/


typedef enum {
  RTC_OK                    = 0,
  RTC_ERR_SPI_BUS,
  RTC_TIME_RESET_NOT_VALID,
  RTC_SETTINGS_RESET_NOT_VALID,
} RTC_RESULT;

typedef struct {
  RESULT pbit_status; 
} RTC_STATUS; 


#pragma pack(1)
typedef struct {
  RESULT  result; 
  UINT8   ReadArr[RTC_SIZE_OF_CLOCK_DATA];  // For DRV_RTC_TIME_NOT_VALID 
                                            // store from RTC_ADDR_CLK_SEC to RTC_ADDR_CLK_YR
} RTC_DRV_PBIT_LOG; 
#pragma pack()



/******************************************************************************
                              Package Exports
******************************************************************************/
#undef EXPORT

#if defined( DRV_RTC_BODY )
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
EXPORT RESULT RTC_Init               (SYS_APP_ID *SysLogId, void *pdata, UINT16 *psize); 
EXPORT RESULT RTC_GetTime            (TIMESTRUCT *Ts);
EXPORT RESULT RTC_SetTime            (TIMESTRUCT *Ts);


// Functions used by SpiManger to read and write to the RTC.
// Applications should call RTC_GetTime RTC_SetTime respectively.
EXPORT RESULT RTC_ReadTime(TIMESTRUCT *Ts);
EXPORT RESULT RTC_WriteTime(TIMESTRUCT *Ts);

/*EXPORT RESULT RTC_SetTrickleChargeOn (void);*/
/*EXPORT RESULT RTC_SetTrickleChargeOff(void);*/
RESULT RTC_ReadNVRam(UINT8 offset, void* data, size_t size);
RESULT RTC_WriteNVRam(UINT8 offset, void* data, size_t size);
RESULT RTC_GetStatus ( void ); 


#endif // DRV_RTC_H 
/******************************************************************************
 *  MODIFICATIONS
 *    $History: RTC.h $
 * 
 * *****************  Version 11  *****************
 * User: Contractor3  Date: 7/29/10    Time: 12:12p
 * Updated in $/software/control processor/code/drivers
 * SCR #698 -- Fixes for code review findings
 * 
 * *****************  Version 10  *****************
 * User: Contractor3  Date: 3/31/10    Time: 11:48a
 * Updated in $/software/control processor/code/drivers
 * SCR #521 Code Review Issues
 * 
 * *****************  Version 9  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 3:54p
 * Updated in $/software/control processor/code/drivers
 * SCR 67 Interrupted SPI Access (Multiple / Nested SPI Access)
 * 
 * *****************  Version 8  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:22p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 * 
 *
 *****************************************************************************/


