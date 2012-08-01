#define DRV_RTC_BODY

/******************************************************************************
         Copyright (C) 2007 - 2010 Pratt & Whitney Engine Services, Inc. 
                      Altair Engine Diagnostic Solutions
               All Rights Reserved. Proprietary and Confidential.
 
 
  File:        RTC.c
 
 
  Description: Driver for the Dallas DS1306 or DS3234
               real-time clock chip connected by the SPI bus.  The same driver
               is used because the format and addresses of the time registers
               are the same, the difference being the configuration and
               status registers.  The type of RTC is auto-detected
               heuristically in the INIT routine and the part specific correct
               configuration is loaded into the registers.
 
   VERSION
    $Revision: 30 $  $Date: 10/18/10 2:25p $
 
******************************************************************************/


/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
//#include "mcf548x.h"

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "ResultCodes.h"
#include "SPI.h"
#include "RTC.h"
#include "TTMR.h"
#include "Assert.h"

#include "stddefs.h"

#include "TestPoints.h"


/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
//Addresses of data within the DS1306/3234 RTC chip.  All data fields are 8-bits
//Read addresses
#define RTC_ADDR_CLK_SEC   0x00  // ******************************************
#define RTC_ADDR_CLK_MIN   0x01  // Note: RTC_ConvertRTC_BCDToTs() depends on
#define RTC_ADDR_CLK_HR    0x02  //       _CLK ordering from _SEC to 
#define RTC_ADDR_CLK_DAY   0x03  //       _YR.  Func must be updated if this 
#define RTC_ADDR_CLK_DATE  0x04  //       ordering changes !! 
#define RTC_ADDR_CLK_MON   0x05  // 
#define RTC_ADDR_CLK_YR    0x06  // ******************************************
#define RTC_ADDR_ALRM0_SEC 0x07
#define RTC_ADDR_ALRM0_MIN 0x08
#define RTC_ADDR_ALRM0_HR  0x09
#define RTC_ADDR_ALRM0_DAY 0x0A
#define RTC_ADDR_ALRM1_SEC 0x0B  //NOTE:DS3234 does not have ALRM1_SEC register
#define RTC_ADDR_ALRM1_MIN 0x0C  //     ALRM1 regs not valid for 3234
#define RTC_ADDR_ALRM1_HR  0x0D
#define RTC_ADDR_ALRM1_DAY 0x0E
#define RTC_ADDR_1306_CTRL      0x0F
#define RTC_ADDR_1306_STATUS    0x10
#define RTC_ADDR_1306_CHRG      0x11
#define RTC_ADDR_1306_USER_RAM  0x20

/*DS3234-specific control registers*/
#define RTC_ADDR_3234_CTRL       0x0E
#define RTC_ADDR_3234_STATUS     0x0F
#define RTC_ADDR_3234_XTL_OFFSET 0x10
#define RTC_ADDR_3234_T_MSB      0x11
#define RTC_ADDR_3234_T_LSB      0x12
#define RTC_ADDR_3234_BB_TD      0x13
#define RTC_ADDR_3234_RAM_ADDR   0x18
#define RTC_ADDR_3234_RAM_DATA   0x19


//Write addresses - Same as read addresses, offset +128 bytes
#define WRITE_OFFSET 0x80

#define RTC_ADDR_CLK_SEC_WR        (RTC_ADDR_CLK_SEC       + WRITE_OFFSET)
#define RTC_ADDR_CLK_MIN_WR        (RTC_ADDR_CLK_MIN       + WRITE_OFFSET)   
#define RTC_ADDR_CLK_HR_WR         (RTC_ADDR_CLK_HR        + WRITE_OFFSET)
#define RTC_ADDR_CLK_DAY_WR        (RTC_ADDR_CLK_DAY       + WRITE_OFFSET) 
#define RTC_ADDR_CLK_DATE_WR       (RTC_ADDR_CLK_DATE      + WRITE_OFFSET)
#define RTC_ADDR_CLK_MON_WR        (RTC_ADDR_CLK_MON       + WRITE_OFFSET)
#define RTC_ADDR_CLK_YR_WR         (RTC_ADDR_CLK_YR        + WRITE_OFFSET)
#define RTC_ADDR_ALRM0_SEC_WR      (RTC_ADDR_ALRM0_SEC     + WRITE_OFFSET)
#define RTC_ADDR_ALRM0_MIN_WR      (RTC_ADDR_ALRM0_MIN     + WRITE_OFFSET)
#define RTC_ADDR_ALRM0_HR_WR       (RTC_ADDR_ALRM0_HR      + WRITE_OFFSET)
#define RTC_ADDR_ALRM0_DAY_WR      (RTC_ADDR_ALRM0_DAY     + WRITE_OFFSET)
#define RTC_ADDR_ALRM1_SEC_WR      (RTC_ADDR_ALRM1_SEC     + WRITE_OFFSET)
#define RTC_ADDR_ALRM1_MIN_WR      (RTC_ADDR_ALRM1_MIN     + WRITE_OFFSET)
#define RTC_ADDR_ALRM1_HR_WR       (RTC_ADDR_ALRM1_HR      + WRITE_OFFSET)
#define RTC_ADDR_ALRM1_DAY_WR      (RTC_ADDR_ALRM1_DAY     + WRITE_OFFSET)
#define RTC_ADDR_1306_CTRL_WR      (RTC_ADDR_1306_CTRL     + WRITE_OFFSET)
#define RTC_ADDR_1306_STATUS_WR    (RTC_ADDR_1306_STATUS   + WRITE_OFFSET)
#define RTC_ADDR_1306_CHRG_WR      (RTC_ADDR_1306_CHRG     + WRITE_OFFSET)
#define RTC_ADDR_1306_USER_RAM_WR  (RTC_ADDR_1306_USER_RAM + WRITE_OFFSET)

/*DS3234-specific control registers*/
#define RTC_ADDR_3234_CTRL_WR       (RTC_ADDR_3234_CTRL       + WRITE_OFFSET)  
#define RTC_ADDR_3234_STATUS_WR     (RTC_ADDR_3234_STATUS     + WRITE_OFFSET)  
#define RTC_ADDR_3234_XTL_OFFSET_WR (RTC_ADDR_3234_XTL_OFFSET + WRITE_OFFSET)  
#define RTC_ADDR_3234_T_MSB_WR      (RTC_ADDR_3234_T_MSB      + WRITE_OFFSET)  
#define RTC_ADDR_3234_T_LSB_WR      (RTC_ADDR_3234_T_LSB      + WRITE_OFFSET)  
#define RTC_ADDR_3234_BB_TD_WR      (RTC_ADDR_3234_BB_TD      + WRITE_OFFSET)  
#define RTC_ADDR_3234_RAM_ADDR_WR   (RTC_ADDR_3234_RAM_ADDR   + WRITE_OFFSET)  
#define RTC_ADDR_3234_RAM_DATA_WR   (RTC_ADDR_3234_RAM_DATA   + WRITE_OFFSET)

#define RTC_INIT_CLK_SEC     0
#define RTC_INIT_CLK_MIN     0
#define RTC_INIT_CLK_HR      0
#define RTC_INIT_CLK_DAY     1
#define RTC_INIT_CLK_DATE    1
#define RTC_INIT_CLK_MON     1
#define RTC_INIT_CLK_YR      97
#define RTC_INIT_ALRM0_SEC   0
#define RTC_INIT_ALRM0_MIN   0
#define RTC_INIT_ALRM0_HR    0
#define RTC_INIT_ALRM0_DAY   1
#define RTC_INIT_ALRM1_SEC   0  
#define RTC_INIT_ALRM1_MIN   0
#define RTC_INIT_ALRM1_HR    0
#define RTC_INIT_ALRM1_DAY   1
#define RTC_INIT_1306_CTRL   0   //Disable alarm int, disable 1Hz output, disable write protect
#define RTC_INIT_1306_STATUS 0
#define RTC_INIT_1306_CHRG       RTC_CHARGE_1360_ENABLE_VAL 

//DS1360 RTC register bit fields
//Time and alarm fields
#define RTC_REG_1360_HR_12_24_BIT      0x40  //12HR = 1, 24HR = 0
#define RTC_REG_1360_HR_AM_PM_BIT      0x20  //PM = 1, AM = 0
#define RTC_REG_1360_ALRM_SEC_MASK_BIT 0x80
#define RTC_REG_1360_ALRM_MIN_MASK_BIT 0x80
#define RTC_REG_1360_ALRM_HR_MASK_BIT  0x80
#define RTC_REG_1360_ALRM_DAY_MASK_BIT 0x80

//Control register fields
#define RTC_REG_1360_CTL_AIE0_BIT      0x01   
#define RTC_REG_1360_CTL_AIE1_BIT      0x02
#define RTC_REG_1360_CTL_1HZ_BIT       0x04
#define RTC_REG_1360_CTL_WP_BIT        0x40

//Status register fields
#define RTC_REG_1360_STA_IRQF0_BIT     0x01
#define RTC_REG_1360_STA_IRQF1_BIT     0x02

//Charge control fields
#define RTC_CHRG_1360_ENAB             0xA0  //0xA in upper nibble enables the charge circuit
#define RTC_CHRG_1360_RES_2K           0x01  //Bits [1:0] select the current-limiting resistor,
#define RTC_CHRG_1360_RES_4K           0x02  // zero is not valid
#define RTC_CHRG_1360_RES_8K           0x02
#define RTC_CHRG_1360_DIODE_1          0x04  //Bits [3:2] select 1 or 2 diode drops, 
                                             //zero is not valid
#define RTC_CHRG_1360_DIODE_2          0x08

//Charge register values for FAST
#define RTC_CHARGE_1360_ENABLE_VAL \
(RTC_CHRG_1360_ENAB|RTC_CHRG_1360_RES_4K|RTC_CHRG_1360_DIODE_1)

#define RTC_CHARGE_1360_DISABLE_VAL    0x00


/*DS3234 RTC Register bit fields*/
#define RTC_CTRL_3234_A1IE       0x01 /*Alarm 1 Interrupt Enable*/
#define RTC_CTRL_3234_A2IE       0x02 /*Alarm 2 Interrupt Enable*/
#define RTC_CTRL_3234_INTCN      0x04 /*!Interrupt/Square wave output select*/
#define RTC_CTRL_3234_RS1        0x08 /*Square wave rate select 1*/
#define RTC_CTRL_3234_RS2        0x10 /*Square wave rate select 2*/
#define RTC_CTRL_3234_CONV       0x20 /*Force temperature conversion*/
#define RTC_CTRL_3234_BBSQW      0x40 /*Enable sq. wave output when on batt*/
#define RTC_CTRL_3234_EOSC       0x80 /*Enable oscillator*/

#define RTC_STATUS_3234_A1F      0x01 /*Alarm 1 Flag*/
#define RTC_STATUS_3234_A2F      0x02 /*Alarm 2 Flag*/
#define RTC_STATUS_3234_BSY      0x04 /*Temperature conversion busy*/
#define RTC_STATUS_3234_EN32KHZ  0x08 /*Enable 32kHz output*/
#define RTC_STATUS_3234_CRATE0   0x10 /*Compensation conversion rate 0*/
#define RTC_STATUS_3234_CRATE1   0x20 /*Compensation conversion rate 1*/
#define RTC_STATUS_3234_BB32KHZ  0x40 /*Enable 23kHz output when on Vbatt*/
#define RTC_STATUS_3234_OSF      0x80 /*Oscillator stopped since last cleared*/

// #define RTC_ADDR_3234_XTL_OFFSET 0x10

#define RTC_BB_TD_DISABLE        0x01 /*Disable temperature compensation
                                        conversions*/

/*Register values defaults for the 3234*/
#define RTC_INIT_3234_CTRL       0
#define RTC_INIT_3234_STATUS     (RTC_STATUS_3234_CRATE0|RTC_STATUS_3234_CRATE1)
#define RTC_INIT_3234_BB_TD      0
#define RTC_INIT_3234_XTL_OFFSET 0


#define RTC_INIT_READARR_LEN  18

#define RTC_VALID_SEC_MAX     59
#define RTC_VALID_MINUTE_MAX  59
#define RTC_VALID_HOUR_MAX    23
#define RTC_VALID_DAY_MAX     31
#define RTC_VALID_MONTH_MAX   12

#define RTC_VALID_YEAR_MIN  1997
#define RTC_VALID_YEAR_MAX  2096
#define RTC_YEAR_1900       1900
#define RTC_YEAR_2000       2000

#define STPU_MAX_VALID_CHAR 0x09
#define TPU_MAX_VALID_CHAR  0x90

//Convert BCD values (used in RTC alarm and clock regs) to a binary value
#define BCD2BIN(_Bcd)   ((((_Bcd)/16)*10) + ((_Bcd) & 0xF))
//Convert Binary values (used in TIMESTRUCT) to a BCD for the RTC regs
//Note: 2 digit output only. Upper 2 digits of 4 digit date lost.
#define BIN2BCD(_Bin)   ((((_Bin)/10)*16) + ((_Bin)%10))

//This macro proceeds SPI read, write, and calls to internal utility routines
//If an internal routine SPI operation fails, it returns the SPI error codes
//to the caller of the RTC routine.  A global, SPI_Result is defined here to
//process the result codes from the SPI driver call
RESULT _ERR;
#define HNDL_ERR(A)  if((_ERR = A) != DRV_OK){return _ERR;}


/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
static const TIMESTRUCT RTC_DEFAULT_TIME = 
{
  RTC_INIT_CLK_YR,  // Year   1997..2082
  RTC_INIT_CLK_MON, // Month  0..11
  RTC_INIT_CLK_DAY, // Day    0..31   Day of Month
  RTC_INIT_CLK_HR,  // Hour   0..23
  RTC_INIT_CLK_MIN, // Minute 0..59
  RTC_INIT_CLK_SEC, // Second 0..59
  0                 // MilliSecond 0..999
};

typedef struct 
{
  UINT8 addr_w;    // Write Addr in RTC 
  UINT8 addr_r;    // Read Addr in RTC
  UINT8 mask;      // Mask value 
  UINT8 val;       // Val expected 
  UINT8 val_s;     // Val to set, if 'bad' 
  RESULT res_code; // Result code to return 
} RTC_3234_CONFIG; 

typedef enum 
{
  RTC_3234_VERIFY_OSC_REG,          // 0
  RTC_3234_VERIFY_CTL_STATUS_REG,   // 1
  RTC_3234_VERIFY_CTL_REG,          // 2
  RTC_3234_VERIFY_XTL_REG,          // 3
  RTC_3234_VERIFY_TEMP_REG,         // 4
  RTC_3234_VERIFY_MAX               // 5
} RTC_3234_VERIFY_ENUM; 

static const RTC_3234_CONFIG RtcVerify3234Cfg[RTC_3234_VERIFY_MAX] = 
{
  //Special check also for OSC Stopped flag. Bit must be cleared ! Otherwise OSC has stoppped. 
  {RTC_ADDR_3234_STATUS_WR, RTC_ADDR_3234_STATUS, RTC_STATUS_3234_OSF, 0x00, 
        RTC_INIT_3234_STATUS, DRV_RTC_OSC_STOPPED_FLAG}, 
  
  //Check control/status (only CRATE0/1,EN32KHZ,BB32KHZ) and 
  //reset it if it is not correct.    Bit must be cleared ! 
  {RTC_ADDR_3234_STATUS_WR, RTC_ADDR_3234_STATUS, 
       (RTC_STATUS_3234_CRATE0 | RTC_STATUS_3234_CRATE1 | RTC_STATUS_3234_EN32KHZ | 
        RTC_STATUS_3234_BB32KHZ), RTC_INIT_3234_STATUS, RTC_INIT_3234_STATUS, DRV_RTC_SETTINGS_RESET_NOT_VALID}, 

  //Check control register and reset control register if it is not correct.
  {RTC_ADDR_3234_CTRL_WR, RTC_ADDR_3234_CTRL, 0xFF, RTC_INIT_3234_CTRL, RTC_INIT_3234_CTRL, 
        DRV_RTC_SETTINGS_RESET_NOT_VALID},
  
  //Check crystal aging and reset if it is not correct
  {RTC_ADDR_3234_XTL_OFFSET_WR, RTC_ADDR_3234_XTL_OFFSET, 0xFF, RTC_INIT_3234_XTL_OFFSET, 
        RTC_INIT_3234_XTL_OFFSET, DRV_RTC_SETTINGS_RESET_NOT_VALID}, 
  
  //Check Disable Temp Conversions
  {RTC_ADDR_3234_BB_TD_WR, RTC_ADDR_3234_BB_TD, 0xFF, RTC_INIT_3234_BB_TD, RTC_INIT_3234_BB_TD, 
        DRV_RTC_SETTINGS_RESET_NOT_VALID}, 
  
}; 


/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static RTC_STATUS m_RTC_Status; 

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static  RESULT  RTC_ResetTime(void);
static  RESULT  RTC_Verify3234Config(void);
static  BOOLEAN RTC_ConvertRTC_BCDToTs ( UINT8 ReadArr[], TIMESTRUCT *Ts ); 


/*--------------------------------------------------------------------------*/
/* Public Functions                                                         */
/*--------------------------------------------------------------------------*/

/*****************************************************************************
 * Function:    RTC_Init
 *
 * Description: Setup the initial state for the RTC module.  Verifies the
 *              correct configuration is programed into the Dallas DS1306
 *              registers and the current time is a sane value.  If the
 *              configuration or time is not valid, it is reset to the "INIT"
 *              values defined above.
 *
 *
 * Parameters:  SysLogId - Ptr to return System Log ID into 
 *              pdata -    Ptr to buffer to return system log data 
 *              psize -    Ptr to return size (bytes) of system log data
 *
 * Returns:     RESULT  Specifies result of the operation, either sucess or
 *                      a specific failure code.
 *
 * Notes:       There are other requirements for PBIT testing that should
 *              be performed at the system or application level.
 *              See FAST-T-310-1.doc
 *
 *  09/12/2009 PL TTMR_GetHSTickCount() is used and must be init before use. 
 *                TTMR_Init() inits the high speed timer. 
 *
 *              This function has more than one possible exit points.  The
 *              macro HNDL_ERR is defined to return in the case of
 *              an SPI operation failure.
 *
 ****************************************************************************/
RESULT RTC_Init (SYS_APP_ID *SysLogId, void *pdata, UINT16 *psize)
{
 UINT8 ReadArr[RTC_INIT_READARR_LEN];
 UINT8 ReadAddr;
 TIMESTRUCT Ts; 
  
 UINT16  sec; 
 RTC_DRV_PBIT_LOG *pdest; 
 
 BOOLEAN bOk; 
 

  pdest = (RTC_DRV_PBIT_LOG *) pdata;
  memset ( pdest, 0, sizeof(RTC_DRV_PBIT_LOG) ); 
  pdest->result = DRV_OK; 
  
  *psize = sizeof(RTC_DRV_PBIT_LOG); 
  
  pdest->result = RTC_Verify3234Config();
  *SysLogId = DRV_ID_RTC_PBIT_REG_INIT_FAIL;  // Note: If ->result == DRV_OK, 
  
  ReadAddr = 0;  //Read out all the time and configuration registers
  HNDL_ERR(SPI_WriteByte(SPI_DEV_RTC,&ReadAddr,TRUE));
  HNDL_ERR(SPI_ReadBlock(SPI_DEV_RTC,ReadArr,RTC_INIT_READARR_LEN,FALSE));

  //Checks SPI RAW data validity and convert to Ts
  if ( !RTC_ConvertRTC_BCDToTs((UINT8 *) &ReadArr[0], &Ts) )
  {
    HNDL_ERR(RTC_ResetTime());
    pdest->result = DRV_RTC_TIME_NOT_VALID;  // Overrides other Errs !
    *SysLogId = DRV_ID_RTC_PBIT_TIME_NOT_VALID; 
    memcpy ( pdest->ReadArr, ReadArr, (RTC_ADDR_CLK_YR + 1) );
  }
  
  if (pdest->result == DRV_OK) 
  {
    // Verify seconds are incrementing 
    sec = Ts.Second; 
    TTMR_Delay ( TICKS_PER_Sec ); 
        
    ReadAddr = 0;  //Read out all the time and configuration registers 
    HNDL_ERR(SPI_WriteByte(SPI_DEV_RTC,&ReadAddr,TRUE));
    HNDL_ERR(SPI_ReadBlock(SPI_DEV_RTC,ReadArr,RTC_INIT_READARR_LEN,FALSE));
    
    bOk = RTC_ConvertRTC_BCDToTs((UINT8 *) &ReadArr[0], &Ts); 
    
    if ( ( sec == Ts.Second ) || ( STPU( !bOk, eTpRtc1302) ))
    {
      pdest->result = DRV_RTC_CLK_NOT_INC; 
      *SysLogId = DRV_ID_RTC_PBIT_CLK_NOT_INC; 
    }
  } // End of if (pdest->result == DRV_OK) 

  m_RTC_Status.pbit_status = pdest->result; 
  
  return pdest->result;
}


/*****************************************************************************
 * Function:    RTC_GetStatus 
 *
 * Description: Returns the PBIT status of the RTC 
 *
 * Parameters:  none
 *
 * Returns:     RESULTS of RTC PBIT status 
 *
 * Notes:       none 
 *
 ****************************************************************************/
RESULT RTC_GetStatus ( void ) 
{
  return ( m_RTC_Status.pbit_status ); 
}


/*****************************************************************************
 * Function:    RTC_GetTime
 *
 * Description: Queries the Dallas DS1306 for the current time and sets the
 *              time parameters in the time structure passed by the Ts parameter
 *
 * Parameters:  *Ts: Pointer to a time structure that the time shall be
 *                   returned into
 *
 * Returns:     RESULT - DRV_RTC_TIME_NOT_VALID or DRV_OK
 *
 * Notes:       This function wraps the retrieving of the RTC from the SpiManager.
 ****************************************************************************/
RESULT RTC_GetTime(TIMESTRUCT *Ts)
{
 return SPIMgr_GetTime(Ts);
}

/*****************************************************************************
* Function:    RTC_ReadTime
*
* Description: Queries the Dallas DS1306 for the current time and sets the
*              time parameters in the time structure passed by the Ts parameter
*
* Parameters:  *Ts: Pointer to a time structure that the time shall be
*                   returned into
*
* Returns:     RESULT - DRV_RTC_TIME_NOT_VALID or DRV_OK
*
* Notes:       This routine incurs a SPI bus overhead of ~123us
*              Assumes the RTC SPI clock is 520kHz and 8 bytes are transfered
*              to read the time.
*
*              This function has more than one possible exit points.  The
*              macro HNDL_ERR is defined to return in the case of
*              an SPI operation failure.
*
****************************************************************************/
RESULT RTC_ReadTime(TIMESTRUCT *Ts)
{
  UINT8 Write;
  UINT8 ReadArr[RTC_SIZE_OF_CLOCK_DATA];
  RESULT result;

  ASSERT(Ts != NULL);
  
  // Init Ts to 1/1/1997 00:00:00 
  *Ts = RTC_DEFAULT_TIME; 

  Write = RTC_ADDR_CLK_SEC;                                   //Read clock values out
  HNDL_ERR(SPI_WriteByte(SPI_DEV_RTC,&Write,TRUE));           //in a multi-byte block
  HNDL_ERR(SPI_ReadBlock(SPI_DEV_RTC,ReadArr,                 //starting at addr 0,
                          RTC_SIZE_OF_CLOCK_DATA,FALSE))      //the seconds reg.

  result = DRV_OK;
  if ( !RTC_ConvertRTC_BCDToTs ( (UINT8 *) &ReadArr[0], Ts ) )
  {
    *Ts = RTC_DEFAULT_TIME; // Set time back to 1/1/1997 00:00:00   
    result = DRV_RTC_TIME_NOT_VALID;
  }

  return result;
}


/*****************************************************************************
 * Function:    RTC_SetTime
 *
 * Description: Takes a TIMESTUCT argument and sets the parameters of the argument
 *              are set in the Dallas DS1306 real-time clock
 *
 * Parameters:  *Ts: Pointer to a time structure that contains the time values
 *                   to set the DS1306 to
 *
 * Returns:     RESULT - Returns the result of the SPIMgr_SetRTC() function call.
 *
 * Notes:       This routine is a wrapper to write buffered RTC set times via
 *              SPI Manager
 ****************************************************************************/
RESULT RTC_SetTime(TIMESTRUCT *Ts)
{
  return SPIMgr_SetRTC(Ts, NULL);
}

/*****************************************************************************
* Function:    RTC_WriteTime
*
* Description: Takes a TIMESTUCT argument and sets the parameters of the argument
*              are set in the Dallas DS1306 real-time clock
*
* Parameters:  *Ts: Pointer to a time structure that contains the time values
*                   to set the DS1306 to
*
* Returns:     RESULT - Returns DRV_OK or DRV_RTC_ERR_CANNOT_SET_TIME or may return
*              an internal SPI error code in the case of an SPI error encountered
*              during SPI_Write operation.
*
* Notes:       This routine incurs a SPI bus overhead of ~246us
*              Assumes the RTC SPI clock is 520kHz and 8 bytes are transfered
*              to read the time.
*
*              This function has more than one possible exit points.  The
*              macro HNDL_ERR is defined to return in the case of
*              an SPI operation failure.
*
****************************************************************************/
RESULT RTC_WriteTime(TIMESTRUCT *Ts)
{
  UINT8      Write;
  UINT8      WriteArr[RTC_SIZE_OF_CLOCK_DATA];
  TIMESTRUCT TmChk;
  RESULT     result;

//TODO: Uncomment this in system ASSERT(Ts != NULL)
  ASSERT(Ts != NULL);

  WriteArr[RTC_ADDR_CLK_SEC]  = (UINT8)BIN2BCD(Ts->Second);
  WriteArr[RTC_ADDR_CLK_MIN]  = (UINT8)BIN2BCD(Ts->Minute);
  WriteArr[RTC_ADDR_CLK_HR]   = (UINT8)BIN2BCD(Ts->Hour);
  WriteArr[RTC_ADDR_CLK_DATE] = (UINT8)BIN2BCD(Ts->Day);
  WriteArr[RTC_ADDR_CLK_MON]  = (UINT8)BIN2BCD(Ts->Month);
  WriteArr[RTC_ADDR_CLK_YR]   = (UINT8)BIN2BCD(Ts->Year % 100); //Convert 4-digit year to
                                                                //2-digit year
  
  Write = RTC_ADDR_CLK_SEC_WR;                                  //Read clock values out in a
  HNDL_ERR(SPI_WriteByte(SPI_DEV_RTC, &Write, TRUE));           //multi-byte block starting
  HNDL_ERR(SPI_WriteBlock(SPI_DEV_RTC, WriteArr,
              RTC_SIZE_OF_CLOCK_DATA, FALSE));                  //at addr 0, the seconds reg.


  HNDL_ERR(RTC_GetTime(&TmChk));  //Verify the time was set correctly
                                  //accounting for time roll over
  result = DRV_RTC_ERR_CANNOT_SET_TIME;
  if(
    ( (Ts->Second == TmChk.Second)  || (((Ts->Second+1)%60) == TmChk.Second) ) &&
    ( (Ts->Minute == TmChk.Minute)  || (((Ts->Minute+1)%60) == TmChk.Minute) ) &&
    ( (Ts->Hour   == TmChk.Hour  )  || (((Ts->Hour  +1)%24) == TmChk.Hour  ) ) &&
    ( (Ts->Year   == TmChk.Year  )  || (((Ts->Year  +1)%100)== TmChk.Year  ) ) &&    
    ( (Ts->Month  == TmChk.Month )  || ((((Ts->Month +1)%13)+1) == TmChk.Month ) ) &&
    ( (Ts->Day==TmChk.Day) || ((Ts->Day+1)==TmChk.Day) || (Ts->Day==1) )
    )
  {
    result = DRV_OK;
  }

  return result;
}



/*****************************************************************************
 * Function:    RTC_ReadNVRam
 *
 * Description: Read a block of data from the 256-byte general purpose RAM
 *              in the DS3234 chip
 *
 * Parameters:  [in] offset: Offset address to start reading the data from.  
 *                           valid range is 0-255
 *              [out]*data:  Pointer to a buffer to read the data into
 *              [in] size:   Length of data to be read in bytes
 *
 * Returns:     Driver operation result code
 *
 * Notes:       This routine incurs a SPI bus overhead of ~45us+15us per byte
 *              read
 *              Assumes the RTC SPI clock is 520kHz 
 *
 *              This function has more than one possible exit points.  The
 *              macro HNDL_ERR is defined to return in the case of
 *              an SPI operation failure.
 *
 *
 ****************************************************************************/
RESULT RTC_ReadNVRam(UINT8 offset, void* data, size_t size)
{
  UINT8 reg;
  
  reg = RTC_ADDR_3234_RAM_ADDR_WR;
  HNDL_ERR(SPI_WriteByte(SPI_DEV_RTC,&reg,TRUE));      
  HNDL_ERR(SPI_WriteByte(SPI_DEV_RTC,&offset,FALSE));      

  reg = RTC_ADDR_3234_RAM_DATA;
  HNDL_ERR(SPI_WriteByte(SPI_DEV_RTC,&reg,TRUE));
  HNDL_ERR(SPI_ReadBlock(SPI_DEV_RTC,data,size,FALSE));

  return DRV_OK;
}



/*****************************************************************************
 * Function:    RTC_WriteNVRam
 *
 * Description: Copies a block of data to the 96 byte NVRAM on-board the DS1306
 *
 * Parameters:  [in] offset: Offset address to start reading the data from.  
 *                           valid range is 0-255
 *              [out]*data:  Pointer to a buffer to read the data into
 *              [in] size:   Length of data to be read in bytes
 *
 * Returns:     
 *
 * Notes:       This routine incurs a SPI bus overhead of ~45us+ 15us per byte
 *              written
 *              Assumes the RTC SPI clock is 520kHz 
 *
 *              This function has more than one possible exit points.  The
 *              macro HNDL_ERR is defined to return in the case of
 *              an SPI operation failure.
 *
 ****************************************************************************/
RESULT RTC_WriteNVRam(UINT8 offset, void* data, size_t size)
{
  UINT8 reg;
  
  reg = RTC_ADDR_3234_RAM_ADDR_WR;
  HNDL_ERR(SPI_WriteByte(SPI_DEV_RTC,&reg,TRUE));      
  HNDL_ERR(SPI_WriteByte(SPI_DEV_RTC,&offset,FALSE));      

  reg = RTC_ADDR_3234_RAM_DATA_WR;
  HNDL_ERR(SPI_WriteByte(SPI_DEV_RTC,&reg,TRUE));
  HNDL_ERR(SPI_WriteBlock(SPI_DEV_RTC,data,size,FALSE));

  return DRV_OK;
}



/*****************************************************************************
 * Function:    RTC_SetTrickleChargeOn
 *
 * Description: Sets the DS1306 trickle charge register to the configuration
 *              specified to enable proper charging of the real-time clock 
 *              backup battery.
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:       DEAD CODE, CAN BE UNCOMMENTED IF NEEDED
 *
 *              This function has more than one possible exit points.  The
 *              macro HNDL_ERR is defined to return in the case of
 *              an SPI operation failure.
 *
 ***************************************************************************
RESULT RTC_SetTrickleChargeOn(void)
{
  UINT8 Byte;

  Byte = RTC_ADDR_1306_CHRG;
  HNDL_ERR(SPI_WriteByte(SPI_DEV_RTC,&Byte,TRUE));

  Byte = RTC_CHARGE_1360_ENABLE_VAL;
  HNDL_ERR(SPI_WriteByte(SPI_DEV_RTC,&Byte,FALSE));

  return DRV_OK;
}
*/


/*****************************************************************************
 * Function:    RTC_SetTrickleChargeOff
 *
 * Description: Sets the trickle charge control register to the value specified
 *              that disables the trickle charge circuit.
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:       DEAD CODE, CAN BE UNCOMMENTED IF NEEDED
 *
 *              This function has more than one possible exit points.  The
 *              macro HNDL_ERR is defined to return in the case of
 *              an SPI operation failure.
 *
 ****************************************************************************
RESULT RTC_SetTrickleChargeOff(void)
{
  UINT8 Byte;

  Byte = RTC_ADDR_1306_CHRG;
  HNDL_ERR(SPI_WriteByte(SPI_DEV_RTC,&Byte,TRUE));

  Byte = RTC_CHARGE_1360_DISABLE_VAL;
  HNDL_ERR(SPI_WriteByte(SPI_DEV_RTC,&Byte,FALSE));

  return DRV_OK;
}
*/
/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/


/*****************************************************************************
 * Function:    RTC_ResetTime
 *
 * Description: Resets the time to initial default value
 *
 * Parameters:  None
 *
 * Returns:     RESULT - returns DRV_OK or SPI error code encountered during 
 *              Write operation.
 *
 * Notes:       
 *              This function has more than one possible exit points.  The
 *              macro HNDL_ERR is defined to return in the case of
 *              an SPI operation failure.
 *
 ****************************************************************************/
static
RESULT RTC_ResetTime(void)
{
  UINT8 WriteAddr;
  UINT8 WriteArr[RTC_SIZE_OF_CLOCK_DATA];

  WriteArr[RTC_ADDR_CLK_SEC]    = RTC_INIT_CLK_SEC;
  WriteArr[RTC_ADDR_CLK_MIN]    = RTC_INIT_CLK_MIN;
  WriteArr[RTC_ADDR_CLK_HR]     = RTC_INIT_CLK_HR;
  WriteArr[RTC_ADDR_CLK_DAY]    = RTC_INIT_CLK_DAY;
  WriteArr[RTC_ADDR_CLK_DATE]   = RTC_INIT_CLK_DATE;
  WriteArr[RTC_ADDR_CLK_MON]    = RTC_INIT_CLK_MON;
  WriteArr[RTC_ADDR_CLK_YR]     = RTC_INIT_CLK_YR;

  //Set the internal clock registers of the DS1306 real-time clock to the
  //initial values defined at the top of this file.  Write the values
  //to the clock in an SPI burst instead of one byte at a time for
  //efficientcy. 
  WriteAddr = RTC_ADDR_CLK_SEC;
  HNDL_ERR(SPI_WriteByte(SPI_DEV_RTC,&WriteAddr,TRUE));
  HNDL_ERR(SPI_WriteBlock(SPI_DEV_RTC,WriteArr,RTC_SIZE_OF_CLOCK_DATA,FALSE));

  return DRV_OK;
}


/*****************************************************************************
 * Function:    RTC_Verify3234Config
 *
 * Description: This verifies the configuration registers of the DS3234 are
 *              set to the initial values.  If the value in the register does
 *              not match the initial configuration, the register is written
 *              with that configuration.  This routine checks and writes the
 *              Control, Control/Status(CRATE1/0,BB32kHz,EN32kHz and OSF bits),
 *              Disable Temp Conversions, and Crystal Aging Offset registers.
 *
 * Parameters:  none
 *
 * Returns:     Result: SPI error code
 *                      DRV_RTC_SETTINGS_RESET_NOT_VALID: A register was not
 *                                                        correct and was reset
 *                      DRV_RTC_OSC_STOPPED_FLAG: The oscillator stopped latch
 *                                                was set, no other
 *                                                configuration error was
 *                                                detected
 *
 * Notes:
 *
 *              This function has more than one possible exit points.  The
 *              macro HNDL_ERR is defined to return in the case of
 *              an SPI operation failure.
 *
 ****************************************************************************/
static
RESULT RTC_Verify3234Config(void)
{
  UINT8 Byte;
  RESULT result = DRV_OK;
 
  RTC_3234_VERIFY_ENUM Regs; 
 

  for (Regs=RTC_3234_VERIFY_OSC_REG; Regs < RTC_3234_VERIFY_MAX; Regs++)
  {
     Byte = RtcVerify3234Cfg[Regs].addr_r; 
     HNDL_ERR(SPI_WriteByte(SPI_DEV_RTC,&Byte,TRUE));
     HNDL_ERR(SPI_ReadByte(SPI_DEV_RTC,&Byte,FALSE));
    
     if ( (Byte & RtcVerify3234Cfg[Regs].mask) !=
             STPU( RtcVerify3234Cfg[Regs].val, eTpRtc1299) )
     {
       Byte = RtcVerify3234Cfg[Regs].addr_w; 
       HNDL_ERR(SPI_WriteByte(SPI_DEV_RTC,&Byte,TRUE));
       Byte = RtcVerify3234Cfg[Regs].val_s;
       HNDL_ERR(SPI_WriteByte(SPI_DEV_RTC,&Byte,FALSE));
       result = RtcVerify3234Cfg[Regs].res_code;
     } // End if failed comparison 
     
  } // End for loop 

  return result;
}


/*****************************************************************************
 * Function:    RTC_ConvertRTC_BCDToTs
 *
 * Description: Converts RAW clock data from DS1306 into Ts and checks validity
 *
 * Parameters:  ReadArr - Ptr to the RAW clock data 
 *              Ts      - Ptr to return converted timestruct 
 *
 * Returns:     BOOLEAN - TRUE Converted Ts is Ok 
 *                        FALSE Converted Ts is Bad 
 * Notes:       
 *   1) If RAW BCD RTC is bad then Ts is not updated
 *
 ****************************************************************************/
static
BOOLEAN RTC_ConvertRTC_BCDToTs ( UINT8 ReadArr[], TIMESTRUCT *Ts ) 
{
  BOOLEAN bResultOk; 
  UINT16  i;
  UINT16  year;

  // Check to ensure that RAW RTC BCD data does not have invalid char 
  //    BCD should only be 0..9 as valid digits
  bResultOk = TRUE; 
  for ( i=RTC_ADDR_CLK_SEC; i<=RTC_ADDR_CLK_YR; i++) 
  {
    // STPU is for PBIT, TPU is for CBIT
    if ( (( STPU( ReadArr[i], eTpRtc1296a) & 0x0F) > STPU_MAX_VALID_CHAR) || 
         ( ( TPU(ReadArr[i], eTpRtc1296a) & 0xF0) > TPU_MAX_VALID_CHAR) )
    {
      bResultOk = FALSE; 
      break; 
    }
  }
  
  // Convert RAW RTC BCD data to Ts if RAW RTC BCD format is OK
  if (bResultOk) 
  {
    Ts->Second      = BCD2BIN(ReadArr[RTC_ADDR_CLK_SEC]);
    Ts->Minute      = BCD2BIN(ReadArr[RTC_ADDR_CLK_MIN]);
    Ts->Hour        = BCD2BIN(ReadArr[RTC_ADDR_CLK_HR]);
    Ts->Day         = TPU( BCD2BIN(ReadArr[RTC_ADDR_CLK_DATE]), eTpRtc1296b);
    Ts->Month       = BCD2BIN(ReadArr[RTC_ADDR_CLK_MON]);
    Ts->MilliSecond = 0;
  
    // Adjust two digit year to century value using base year.
    // * NOTE: DS1306 RTC is designed for 21st century where the year 00 is
    // internally assumed to be 2000.

    year = BCD2BIN(ReadArr[RTC_ADDR_CLK_YR]);     // convert year from bcd to bin
    Ts->Year = RTC_YEAR_1900 + year;              // init to 1997 year base
    if (year < RTC_INIT_CLK_YR)         // RTC_INIT_CLK_YR is 97
    {
      Ts->Year = RTC_YEAR_2000 + year ; // 00 to 96 = 2000 to 2096
    }

    // Check Ts for validity
    //Note: Hour check also verifies 12/24HR bit
    if( (!((Ts->Second) <= RTC_VALID_SEC_MAX))                ||
        (!((Ts->Minute) <= RTC_VALID_MINUTE_MAX))             ||
        (!((Ts->Hour)   <= RTC_VALID_HOUR_MAX))               ||
        (!((RTC_INIT_CLK_DAY <= Ts->Day) && (Ts->Day <= RTC_VALID_DAY_MAX)))     ||
        (!((RTC_INIT_CLK_MON <= Ts->Month) && (Ts->Month <= RTC_VALID_MONTH_MAX))) ||
        (!((RTC_VALID_YEAR_MIN<=Ts->Year) && 
           (Ts->Year<=RTC_VALID_YEAR_MAX)))   )  //Valid years 1997 - 2096
    {
      bResultOk = FALSE;
    }
  } 

  return (bResultOk); 

}


/*************************************************************************
 *  MODIFICATIONS
 *    $History: RTC.c $
 * 
 * *****************  Version 30  *****************
 * User: Contractor2  Date: 10/18/10   Time: 2:25p
 * Updated in $/software/control processor/code/drivers
 * SCR #942 RTC - PBIT failure of RTC_Verify3234Config
 * 
 * *****************  Version 29  *****************
 * User: Contractor2  Date: 9/30/10    Time: 3:54p
 * Updated in $/software/control processor/code/drivers
 * SCR #841 PBIT - PBIT RTC faults are recorded with invalid time
 * Fixed BCD year conversion error.
 * Fixed code coverage issue for year 1997 check
 * Fixed code review issues (magic numbers, indentation, static functions,
 * multiple variables declared on one line)
 * 
 * *****************  Version 28  *****************
 * User: Jeff Vahue   Date: 9/14/10    Time: 8:47p
 * Updated in $/software/control processor/code/drivers
 * SCR# 848 - Coverage Mod 
 * 
 * *****************  Version 27  *****************
 * User: Jeff Vahue   Date: 9/14/10    Time: 8:21p
 * Updated in $/software/control processor/code/drivers
 * SCR# 848 - Code Coverage Mods
 * 
 * *****************  Version 26  *****************
 * User: Peter Lee    Date: 8/04/10    Time: 1:34p
 * Updated in $/software/control processor/code/drivers
 * SCR #765 Removed unused func RTC_ResetRTCSettings()
 * 
 * *****************  Version 25  *****************
 * User: Peter Lee    Date: 7/29/10    Time: 8:45p
 * Updated in $/software/control processor/code/drivers
 * SCR #708 Refactor RTC_Verify3234Config() for V&V Testing.
 * 
 * *****************  Version 24  *****************
 * User: Peter Lee    Date: 7/29/10    Time: 8:36p
 * Updated in $/software/control processor/code/drivers
 * SCR #699 Re-add changes from version 21 that was accidently removed
 * during ver 22 check in. 
 * 
 * *****************  Version 23  *****************
 * User: Peter Lee    Date: 7/29/10    Time: 7:42p
 * Updated in $/software/control processor/code/drivers
 * SCR #698 Code Review Updates
 * 
 * *****************  Version 22  *****************
 * User: Contractor3  Date: 7/29/10    Time: 12:12p
 * Updated in $/software/control processor/code/drivers
 * SCR #698 -- Fixes for code review findings
 * 
 * *****************  Version 21  *****************
 * User: Peter Lee    Date: 7/29/10    Time: 9:26a
 * Updated in $/software/control processor/code/drivers
 * SCR #699 Processing when RTC_ReadTime() returns fail.  
 * 
 * *****************  Version 20  *****************
 * User: Contractor V&v Date: 7/27/10    Time: 2:25p
 * Updated in $/software/control processor/code/drivers
 * SCR #713 Misc - Remove RTC_Is3234 from the code
 * 
 * *****************  Version 19  *****************
 * User: Jeff Vahue   Date: 7/19/10    Time: 6:33p
 * Updated in $/software/control processor/code/drivers
 * SCR# 707 - Code Coverage TP
 * 
 * *****************  Version 18  *****************
 * User: Jeff Vahue   Date: 7/17/10    Time: 5:46p
 * Updated in $/software/control processor/code/drivers
 * SCR# 707 - Add TestPoints for code coverage.
 * 
 * *****************  Version 17  *****************
 * User: Contractor3  Date: 3/31/10    Time: 11:48a
 * Updated in $/software/control processor/code/drivers
 * SCR #521 Code Review Issues
 * 
 * *****************  Version 16  *****************
 * User: Jeff Vahue   Date: 3/15/10    Time: 4:12p
 * Updated in $/software/control processor/code/drivers
 * SCR# 487 - remove unreachable code.
 * 
 * *****************  Version 15  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 3:54p
 * Updated in $/software/control processor/code/drivers
 * SCR 67 Interrupted SPI Access (Multiple / Nested SPI Access)
 * 
 * *****************  Version 14  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:22p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 13  *****************
 * User: Jeff Vahue   Date: 2/24/10    Time: 1:01p
 * Updated in $/software/control processor/code/drivers
 * 
 * *****************  Version 12  *****************
 * User: Jeff Vahue   Date: 2/19/10    Time: 12:57p
 * Updated in $/software/control processor/code/drivers
 * SCR# 455 - remove LINT issues
 * 
 * *****************  Version 11  *****************
 * User: Peter Lee    Date: 1/28/10    Time: 9:19a
 * Updated in $/software/control processor/code/drivers
 * SCR #427 ClockMgr.c Issues and SRS-3090
 * 
 * *****************  Version 10  *****************
 * User: Jim Mood     Date: 10/08/09   Time: 6:13p
 * Updated in $/software/control processor/code/drivers
 * SCR #283
 * 
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 9/29/09    Time: 9:30a
 * Updated in $/software/control processor/code/drivers
 * SCR #272 Max Test Time must be < WATCHDOG timer of 1.6 sec or WATCHDOG
 * has to be serviced. 
 * 
 * *****************  Version 8  *****************
 * User: Jim Mood     Date: 9/25/09    Time: 8:59a
 * Updated in $/software/control processor/code/drivers
 * Added NV RAM access for the DS3234.  Removed access for the DS1306
 * 
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 9/15/09    Time: 6:05p
 * Updated in $/software/control processor/code/drivers
 * SCR #94, #95 PBIT returns log structure to Init Mgr
 * 
 * *****************  Version 6  *****************
 * User: Peter Lee    Date: 9/14/09    Time: 3:50p
 * Updated in $/software/control processor/code/drivers
 * SCR #94 Updated Init for SET_CHECK() and return ERR CODE Struct
 * Set Default Date Time to 1/1/1997 00:00:00 from 1/1/2000 00:00:00
 * 
 * *****************  Version 5  *****************
 * User: Peter Lee    Date: 5/21/09    Time: 2:35p
 * Updated in $/software/control processor/code/drivers
 * SCR #183 Bad BCD clock data comparison in RTC_GetTime()
 *
 ***************************************************************************/


