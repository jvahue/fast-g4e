#define SPI_BODY

/******************************************************************************
            Copyright (C) 2007 - 2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.


 File:        SPI.c

 Description: Driver for the MCF548x SPI peripheral.  This implementation uses
              the double-buffered output mode (non-DMA and non-FIFO.)  Calls to
              send or receive data are synchronous, the call will block until
              the bus transaction is complete.

              By default the SPI peripheral is initialized to communicate the
              FAST ADC, the serial EEPROM, and the Real-Time clock.

              Chip selects are controlled by GPIO. This is a workaround for
              MCF548x Errata titled: "DSPI: FIFO Underflow Ends Continuous Chip
              Select"

 Requires:    none

 VERSION
    $Revision: 26 $  $Date: 12-11-02 12:55p $


******************************************************************************/


/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include "mcf548x.h"

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "ResultCodes.h"
#include "SPI.h"
#include "Utility.h"

#include "stddefs.h"
#include "alt_basic.h"
#include "UtilRegSetCheck.h"

#include "TestPoints.h"


/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
//List of the 8 possible SPI devices.  The number of items in the list are
//limited to the number of attributes regs in the CF 548x.
//See DCTARn in the 548x reference manual.
const SPI_DEV_INFO SPI_Devices[SPI_MAX_DEVS] =
        {{SPI_DEV_DEFAULT_0_DCTAR, SPI_DEV_DEFAULT_0_DTFRCmd, SPI_DEV_DEFAULT_0_CS},
         {SPI_DEV_DEFAULT_1_DCTAR, SPI_DEV_DEFAULT_1_DTFRCmd, SPI_DEV_DEFAULT_1_CS},
         {SPI_DEV_DEFAULT_2_DCTAR, SPI_DEV_DEFAULT_2_DTFRCmd, SPI_DEV_DEFAULT_2_CS},
         {SPI_DEV_DEFAULT_3_DCTAR, SPI_DEV_DEFAULT_3_DTFRCmd, SPI_DEV_DEFAULT_3_CS},
         {SPI_DEV_DEFAULT_4_DCTAR, SPI_DEV_DEFAULT_4_DTFRCmd, SPI_DEV_DEFAULT_4_CS},
         {SPI_DEV_DEFAULT_5_DCTAR, SPI_DEV_DEFAULT_5_DTFRCmd, SPI_DEV_DEFAULT_5_CS},
         {SPI_DEV_DEFAULT_6_DCTAR, SPI_DEV_DEFAULT_6_DTFRCmd, SPI_DEV_DEFAULT_6_CS},
         {SPI_DEV_DEFAULT_7_DCTAR, SPI_DEV_DEFAULT_7_DTFRCmd, SPI_DEV_DEFAULT_7_CS}
        };


const REG_SETTING SPI_Registers[] =
{
  // DSPI is in master mode
  //
  // DSPICSn chip select(s) 0,3,5 are inactive high; DSPICSn chip select(s) 2 are inactive low
  // Transmit and receive FIFOs disabled
  // Overwrite existing data if new data arrives when receive FIFO is full

  // SET_AND_CHECK ( MCF_DSPI_DMCR, MCF_DSPI_DMCR_VAL, bInitOk );
  {(void *) &MCF_DSPI_DMCR, MCF_DSPI_DMCR_VAL, sizeof(UINT32), 0x0, REG_SET, TRUE,
            RFA_FORCE_UPDATE},

  // SET_AND_CHECK ( MCF_DSPI_DIRSR, MCF_DSPI_DIRSR_VAL, bInitOk );
  {(void *) &MCF_DSPI_DIRSR, MCF_DSPI_DIRSR_VAL, sizeof(UINT32), 0x0, REG_SET, TRUE,
            RFA_FORCE_UPDATE},

  //Initialize transfer attribute registers for each
  //"transfer attribute" register in the ColdFire.
  {(void *) &MCF_DSPI_DCTARn(0), SPI_DEV_DEFAULT_0_DCTAR, sizeof(UINT32), 0x0, REG_SET, TRUE,
            RFA_FORCE_UPDATE},

  {(void *) &MCF_DSPI_DCTARn(1), SPI_DEV_DEFAULT_1_DCTAR, sizeof(UINT32), 0x0, REG_SET, TRUE,
            RFA_FORCE_UPDATE},

  {(void *) &MCF_DSPI_DCTARn(2), SPI_DEV_DEFAULT_2_DCTAR, sizeof(UINT32), 0x0, REG_SET, TRUE,
            RFA_FORCE_UPDATE},

  {(void *) &MCF_DSPI_DCTARn(3), SPI_DEV_DEFAULT_3_DCTAR, sizeof(UINT32), 0x0, REG_SET, TRUE,
            RFA_FORCE_UPDATE},

  {(void *) &MCF_DSPI_DCTARn(4), SPI_DEV_DEFAULT_4_DCTAR, sizeof(UINT32), 0x0, REG_SET, TRUE,
            RFA_FORCE_UPDATE},

  {(void *) &MCF_DSPI_DCTARn(5), SPI_DEV_DEFAULT_5_DCTAR, sizeof(UINT32), 0x0, REG_SET, TRUE,
            RFA_FORCE_UPDATE},

  {(void *) &MCF_DSPI_DCTARn(6), SPI_DEV_DEFAULT_6_DCTAR, sizeof(UINT32), 0x0, REG_SET, TRUE,
            RFA_FORCE_UPDATE},

  {(void *) &MCF_DSPI_DCTARn(7), SPI_DEV_DEFAULT_7_DCTAR, sizeof(UINT32), 0x0, REG_SET, TRUE,
            RFA_FORCE_UPDATE},

  //Set the SPI pins from the reset state of GPIO to DSPI mode
  // SET_AND_CHECK ( MCF_GPIO_PDDR_DSPI, 0x00, bInitOk );
  {(void *) &MCF_GPIO_PDDR_DSPI, 0x00, sizeof(UINT8), 0x0, REG_SET, FALSE,
            RFA_FORCE_UPDATE},

  // SET_AND_CHECK ( MCF_GPIO_PAR_DSPI, MCF_GPIO_PAR_DSPI_VAL, bInitOk );
  {(void *) &MCF_GPIO_PAR_DSPI, MCF_GPIO_PAR_DSPI_VAL, sizeof(UINT16), 0x0, REG_SET, TRUE,
            RFA_FORCE_UPDATE},

  //Set the chip select pins to general purpose output
  //This is a workaround for MCF548x Errata titled:
  //"DSPI: FIFO Underflow Ends Continuous Chip Select"
  // SET_AND_CHECK ( MCF_GPIO_PDDR_DSPI, MCF_GPIO_PDDR_DSPI_VAL, bInitOk );
  {(void *) &MCF_GPIO_PDDR_DSPI, MCF_GPIO_PDDR_DSPI_VAL, sizeof(UINT8), 0x0, REG_SET, TRUE,
            RFA_FORCE_UPDATE},


};


/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/
#ifndef WIN32
/*****************************************************************************
 * Function:    SPI_Init
 *
 * Description: Set the DSPI peripheral to the initial startup state.
 *
 * Parameters:  SysLogId  ptr to return SYS_APP_ID
 *              pdata     ptr to buffer to return result data
 *              psize     ptr to return size of result data
 *
 * Returns:     DRV_RESULT : DRV_OK  : Init success
 *                           !DRV_OK : See ResultCodes.h
 *
 * Notes:       None
 *
 ****************************************************************************/
//RESULT SPI_Init(void)
RESULT SPI_Init (SYS_APP_ID *SysLogId, void *pdata, UINT16 *psize)
{
  UINT32 i;
  BOOLEAN bInitOk;
  SPI_DRV_PBIT_LOG  *pdest;


  pdest = (SPI_DRV_PBIT_LOG *) pdata;
  memset ( pdest, 0, sizeof(SPI_DRV_PBIT_LOG) );
  pdest->result = DRV_OK;

  *psize = sizeof(SPI_DRV_PBIT_LOG);

  // Initialize the SPI Registers
  bInitOk = TRUE;
  for (i=0;i<(sizeof(SPI_Registers)/sizeof(REG_SETTING));i++)
  {
    bInitOk &= RegSet( (REG_SETTING_PTR) &SPI_Registers[i] );
  }


  if ( TRUE != STPU( bInitOk, eTpSpi3805) )
  {
    pdest->result = DRV_SPI_REG_INIT_FAIL;
    *SysLogId = DRV_ID_SPI_PBIT_REG_INIT_FAIL;
  }

  return pdest->result;

}

/*****************************************************************************
 * Function:    SPI_WriteByte
 * Description: Writes a single byte onto the SPI bus to the selected device
 * Parameters:  Device: The device to chip select for this write
 *              Byte*: A pointer to the byte to write to the device
 *              CS:     True-Hold CS active after this transfer
 *                      False-Set CS to inactive after this transfer
 * Returns:     DRV_OK:  Write succeeded
 *              !DRV_OK: Write failed
 * Notes:       The Byte returned into MISO while writing is copied into the
 *              Byte parameter
 ****************************************************************************/
RESULT SPI_WriteByte(SPI_DEVS Device, const UINT8* Byte, BOOLEAN CS)
{
  const  UINT32 IoTimeLimit = TICKS_PER_uSec * 100;
  static UINT32 StartTime;
  BOOLEAN timeoutFlag = FALSE;
  INT32 intsave;

  volatile INT8 clear;

  MCF_GPIO_PODR_DSPI = SPI_Devices[Device].CS;              //Ensure the chip select for only
                                                            //the one device is is set

  MCF_DSPI_DSR = MCF_DSPI_DSR_TCF;                          //Clear transfer complete flag by
                                                            //writing a 1 to the bit location
                                                            //of TCF

  MCF_DSPI_DTFR = (SPI_Devices[Device].DTFRCmd | *Byte);    //Send it!

  intsave = __DIR();
  TimeoutEx(TO_START, IoTimeLimit, &StartTime, NULL);
  while(!(MCF_DSPI_DSR & MCF_DSPI_DSR_TCF))                 //Wait for completion
  {
    if ( TPU( TimeoutEx(TO_CHECK, IoTimeLimit, &StartTime, NULL), eTpSpiWr1256) )
    {
      timeoutFlag = TRUE;
      break;
    }
  }
  __RIR(intsave);

  clear = MCF_DSPI_DRFR;                                    //Read to clear the RX buffer

  if(CS == FALSE)                                           //Set CS inactive if the caller
  {                                                         //requests
    MCF_GPIO_PODR_DSPI = SPI_ALL_CS_INACTIVE;
  }

  return TPU( timeoutFlag, eTpSpiWr1256) ? DRV_SPI_TIMEOUT : DRV_OK;
}


/*****************************************************************************
 * Function:    SPI_WriteWord
 * Description: Writes a 16-bit word onto the SPI bus, to the selected device
 * Parameters:  Device: The device to chip select for this write
 *              Word*:  A pointer to the 16-bit word to write to the device
 *              CS:     True-Hold CS active after this transfer
 *                      False-Set CS to inactive after this transfer
 * Returns:     DRV_OK:  Write suceeded
 *              !DRV_OK: Write failed
 * Notes:       The word returned into MISO while writing is copied into the
 *              Word parameter
 ****************************************************************************/
/*
RESULT SPI_WriteWord(SPI_DEVS Device, UINT16* Word, BOOLEAN CS)
{
  MCF_GPIO_PODR_DSPI = SPI_Devices[Device].CS;              //Ensure the chip select for only
                                                            //the one device is is set

  MCF_DSPI_DSR = MCF_DSPI_DSR_TCF;                          //Clear transfer complete flag by
                                                            //writing a 1 to the bit location
                                                            //of TCF

  MCF_DSPI_DTFR = (SPI_Devices[Device].DTFRCmd | *Word);    //Send it!

  while(!(MCF_DSPI_DSR & MCF_DSPI_DSR_TCF))                 //Wait for completion
  {
  }

  *Word = MCF_DSPI_DRFR;                                    //Dummy read to clear the data

  if(CS == FALSE)                                           //Set CS inactive if the caller
  {                                                         //requests
    MCF_GPIO_PODR_DSPI = SPI_ALL_CS_INACTIVE;
  }

  return DRV_OK;

}
*/


/*****************************************************************************
 * Function:    SPI_WriteBlock
 * Description: Writes a variable length block of data onto the SPI bus.
 *              Block size is limited to 65535 bytes by the 16-bit count
 *              parameter
 * Parameters:  Device: The device to chip select for this write
 *              Data*:  A pointer to a UINT8 array containing the data to write
 *              Cnt:    The length of data (in bytes) to write
 *              CS:     True-Hold CS active after this transfer
 *                      False-Set CS to inactive after this transfer
 *
 * Returns:     DRV_OK:  Write succeeded
 *              !DRV_OK: Write failed
 * Notes:
 ****************************************************************************/
RESULT SPI_WriteBlock(SPI_DEVS Device, const BYTE* Data, UINT16 Cnt, BOOLEAN CS)
{
  RESULT result = DRV_OK;
  for(;Cnt>0;Cnt--)                   //Transmit all bytes in the block array
  {
    if(Cnt == 1)                      //On the last byte, set the chip select to
    {                                 //the state requested by the caller
      result = SPI_WriteByte(Device,Data,CS);
    }
    else
    {                                 //For all other bytes, set the CS to active
      result = SPI_WriteByte(Device,Data,TRUE);
    }
    Data++;
  }
  return result;
}


/*****************************************************************************
 * Function:    SPI_ReadByte
 * Description: Reads a single byte from the selected device on the SPI bus
 * Parameters:  Device: The device to chip select for reading
 *              Byte:   Pointer to a byte to store the result
 *              CS:     True-Hold CS active after this transfer
 *                      False-Set CS to inactive after this transfer
 *
 * Returns:     DRV_OK:  Write succeeded
 *              !DRV_OK: Write failed
 * Notes:
 ****************************************************************************/
RESULT SPI_ReadByte(SPI_DEVS Device, UINT8* Byte, BOOLEAN CS)
{
  const  UINT32 IoTimeLimit = TICKS_PER_uSec * 100;
  static UINT32 StartTime;
  BOOLEAN timeoutFlag = FALSE;
  INT32 intsave;


  MCF_GPIO_PODR_DSPI = SPI_Devices[Device].CS;          //Ensure the chip select for only
                                                        //the one device is is set

  MCF_DSPI_DSR = MCF_DSPI_DSR_TCF;                      //Clear transfer complete flag by
                                                        //writing a 1 to the bit location
                                                        //of TCF

  MCF_DSPI_DTFR = (SPI_Devices[Device].DTFRCmd | 0);    //Send a zero byte to shift out
                                                        //the byte

  intsave = __DIR();
  TimeoutEx(TO_START, IoTimeLimit, &StartTime, NULL);
  while(!(MCF_DSPI_DSR & MCF_DSPI_DSR_TCF))             //Wait for current transfers to
                                                        //complete,
  {                                                     //TCF will go to one when no transfers
                                                        //are occurring
    if( TPU( TimeoutEx(TO_CHECK, IoTimeLimit,&StartTime,NULL), eTpSpiRd1251))
    {
      timeoutFlag = TRUE;
      break;
    }
  }
  __RIR(intsave);

  *Byte = MCF_DSPI_DRFR;

  if(CS == FALSE)                                       //Set CS inactive if the caller
  {                                                     //requests
    MCF_GPIO_PODR_DSPI = SPI_ALL_CS_INACTIVE;
  }
  return TPU( timeoutFlag, eTpSpiRd1251) ? DRV_SPI_TIMEOUT : DRV_OK;
}


/*****************************************************************************
 * Function:    SPI_ReadWord
 * Description: Read a single 16-bit word from a SPI device
 * Parameters:  Device: The device to chip select for reading
 *              Word:   Pointer to a 16-bit word to store the result
 *              CS:     True-Hold CS active after this transfer
 *                      False-Set CS to inactive after this transfer
 * Returns:     DRV_OK:  Write succeeded
 *              !DRV_OK: Write failed
 * Notes:
 ****************************************************************************/
RESULT SPI_ReadWord(SPI_DEVS Device, UINT16* Word, BOOLEAN CS)
{
  const  UINT32 IoTimeLimit = TICKS_PER_uSec * 100;
  static UINT32 StartTime;
  BOOLEAN timeoutFlag = FALSE;
  INT32 intsave;

  MCF_GPIO_PODR_DSPI = SPI_Devices[Device].CS;          //Ensure the chip select for only
                                                        //the one device is is set

  MCF_DSPI_DSR = MCF_DSPI_DSR_TCF;                      //Clear transfer complete flag by
                                                        //writing a 1 to the bit location
                                                        //of TCF

  MCF_DSPI_DTFR = (SPI_Devices[Device].DTFRCmd | 0);    //Send a zero byte to shift out
                                                        //the byte

  intsave = __DIR();
  TimeoutEx(TO_START, IoTimeLimit, &StartTime, NULL);
  while(!(MCF_DSPI_DSR & MCF_DSPI_DSR_TCF))             //Wait for current transfers
                                                        //to complete,
  {
    //TCF will go to one when no transfers
    if( TPU( TimeoutEx(TO_CHECK, IoTimeLimit,&StartTime,NULL), eTpSpiRd1251))
    {                                                   //are occurring
      timeoutFlag = TRUE;
      break;
    }
  }
  __RIR(intsave);


  *Word = MCF_DSPI_DRFR;                                //Return word to caller

  if(CS == FALSE)                                       //Set CS inactive if the caller
  {                                                     //requests
    MCF_GPIO_PODR_DSPI = SPI_ALL_CS_INACTIVE;
  }
  return TPU( timeoutFlag, eTpSpiRd1251) ? DRV_SPI_TIMEOUT : DRV_OK;        //Status == Success
}



/*****************************************************************************
 * Function:    SPI_ReadBlock
 * Description: Read a variable length block of data from a SPI device
 * Parameters:  Device: The device to chip select for reading
 *              Data:   Pointer to store the result
 *              Cnt:    The length of data (in bytes) to write
 *              CS:     True-Hold CS active after this transfer
 *                      False-Set CS to inactive after this transfer
 * Returns:     DRV_OK:  Write succeeded
 *              !DRV_OK: Write failed
 * Notes:
 ****************************************************************************/
RESULT SPI_ReadBlock(SPI_DEVS Device, BYTE* Data, UINT16 Cnt, BOOLEAN CS)
{
  RESULT result = DRV_OK;

  for(;Cnt>0;Cnt--)                                     //Transmit all bytes in the block array
  {
    if(Cnt == 1)                                        //On the last byte, set the chip select
    {                                                   //to the state requested by the caller
      result = SPI_ReadByte(Device,Data,CS);
    }
    else
    {                                                   //For all other bytes, set the CS
                                                        //to active
      result = SPI_ReadByte(Device,Data,TRUE);
    }
    Data++;
  }

  return result;

}
#else

#include "HwSupport.h"

#endif

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/*****************************************************************************
 *  MODIFICATIONS
 *    $History: SPI.c $
 * 
 * *****************  Version 26  *****************
 * User: Melanie Jutras Date: 12-11-02   Time: 12:55p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 File Format Error
 *
 * *****************  Version 25  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 24  *****************
 * User: Jeff Vahue   Date: 11/03/10   Time: 7:03p
 * Updated in $/software/control processor/code/drivers
 * SCR# 848 - Code Coverage
 *
 * *****************  Version 23  *****************
 * User: Jeff Vahue   Date: 10/18/10   Time: 3:16p
 * Updated in $/software/control processor/code/drivers
 * SCR# 848 - Code Coverage
 *
 * *****************  Version 22  *****************
 * User: Jeff Vahue   Date: 9/09/10    Time: 1:03p
 * Updated in $/software/control processor/code/drivers
 * SCR# 859 - Breakout of SPI wait complete loops on timeout
 *
 * *****************  Version 21  *****************
 * User: Peter Lee    Date: 9/03/10    Time: 11:15a
 * Updated in $/software/control processor/code/drivers
 * SCR #806
 *
 * *****************  Version 20  *****************
 * User: Peter Lee    Date: 8/27/10    Time: 6:39p
 * Updated in $/software/control processor/code/drivers
 * SCR #786 ModFix.  Make intsave local.
 *
 * *****************  Version 19  *****************
 * User: Peter Lee    Date: 8/27/10    Time: 3:29p
 * Updated in $/software/control processor/code/drivers
 * SCR #786 False ADC SPI Timeouts
 *
 * *****************  Version 18  *****************
 * User: Jeff Vahue   Date: 7/17/10    Time: 5:46p
 * Updated in $/software/control processor/code/drivers
 * SCR# 707 - Add TestPoints for code coverage.
 *
 * *****************  Version 17  *****************
 * User: Contractor3  Date: 5/27/10    Time: 10:56a
 * Updated in $/software/control processor/code/drivers
 * SCR #617 - Changes made to meet coding standards as a result of code
 * review.
 *
 * *****************  Version 16  *****************
 * User: Contractor V&v Date: 3/19/10    Time: 4:30p
 * Updated in $/software/control processor/code/drivers
 * SCR #67 Log SPI timeouts events Fix ReadBlock to return result
 *
 * *****************  Version 15  *****************
 * User: Contractor V&v Date: 3/10/10    Time: 7:32p
 * Updated in $/software/control processor/code/drivers
 * SCR #67 Adding timeout for GetWord
 *
 * *****************  Version 14  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 3:54p
 * Updated in $/software/control processor/code/drivers
 * SCR 67 Added timeout to WriteByte and ReadByte
 *
 * *****************  Version 13  *****************
 * User: Jeff Vahue   Date: 12/18/09   Time: 1:34p
 * Updated in $/software/control processor/code/drivers
 * SCR# 378
 *
 * *****************  Version 12  *****************
 * User: Peter Lee    Date: 12/01/09   Time: 5:36p
 * Updated in $/software/control processor/code/drivers
 * SCR #350 Misc code review items
 *
 * *****************  Version 11  *****************
 * User: Peter Lee    Date: 11/02/09   Time: 1:51p
 * Updated in $/software/control processor/code/drivers
 * Misc Code Update to support WIN32 and spelling corrections
 *
 * *****************  Version 10  *****************
 * User: Peter Lee    Date: 10/26/09   Time: 4:49p
 * Updated in $/software/control processor/code/drivers
 * SCR #315 CBIT and SEU Processing
 *
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 9/29/09    Time: 1:57p
 * Updated in $/software/control processor/code/drivers
 * SCR #280 Fix misc compiler warnings
 *
 * *****************  Version 8  *****************
 * User: Jim Mood     Date: 9/25/09    Time: 9:00a
 * Updated in $/software/control processor/code/drivers
 * SCR #269
 *
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 9/15/09    Time: 6:05p
 * Updated in $/software/control processor/code/drivers
 * SCR #94, #95 PBIT returns log structure to Init Mgr
 *
 * *****************  Version 6  *****************
 * User: Peter Lee    Date: 9/14/09    Time: 3:54p
 * Updated in $/software/control processor/code/drivers
 * SCR #94 Updated Init for SET_CHECK() and return ERR CODE Struct
 *
 * *****************  Version 5  *****************
 * User: Peter Lee    Date: 6/29/09    Time: 2:51p
 * Updated in $/software/control processor/code/drivers
 * SCR #204 Misc Code Review Updates
 *
 *
 *****************************************************************************/
