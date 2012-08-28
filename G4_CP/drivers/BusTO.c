#define BUSTO_BODY

/******************************************************************************
Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.                 
               All Rights Reserved.  Proprietary and Confidential.


 File:        BusTO.c         

 Description: Bus TimeOut handler.  This unit provides a handler for the
              XL Arbiter Bus Timeout. A memory address that does not acknowledge
              an access will cause an interrupt after a programmable delay.
              The delay must be shorter than the watchdog timer but longer than
              the slowest device on the bus. This allows debug info to be recorded
              before allowing a watchdog reset of the system.

              
  VERSION
  $Revision: 4 $  $Date: 8/28/12 1:06p $

******************************************************************************/


/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include "mcf548x.h"
#include <stdio.h>

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "BusTO.h"
#include "FaultMgr.h"
#include "WatchDog.h"
#include "alt_basic.h"
#include "RTC.h"
#include "GSE.h"
#include "Exception.h"

#include "stddefs.h"
#include "UtilRegSetCheck.h"

#include "TestPoints.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/

// Bus Timeout Time in clock cycles (Bus clock = 100MHz)
#define BTO_TIME 50000

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/

INT8  msgBuf[ASSERT_MAX_LOG_SIZE];

#ifdef WIN32
/*vcast_dont_instrument_start*/
CHAR ExOutputBuf[ASSERT_MAX_LOG_SIZE];
CHAR ReOutputBuf[ASSERT_MAX_LOG_SIZE];
/*vcast_dont_instrument_end*/
#endif

// Bus Timeout Access Type
const CHAR* AccessType[] =
{
  "Read",
  "Write-Flush",
  "Write-Kill",
  "Unknown"
};

const REG_SETTING BTO_Registers[] = 
{
  // Setup XL Bus Arbiter
  // Configure for Address/Data Tenure Time-out
  // Disable Pipeline
  // Interrupt on Address/Data Timeout enabled
  // Note: Data Timeout must be slightly longer than address timeout
  
  // SET_AND_CHECK ( MCF_XARB_ADRTO,  MCF_XARB_ADRTO_ADRTO(BTO_TIME), bInitOk ); 
  {(void *) &MCF_XARB_ADRTO, MCF_XARB_ADRTO_ADRTO(BTO_TIME), sizeof(UINT32), 0x00,
            REG_SET, TRUE, RFA_FORCE_UPDATE},

  // SET_AND_CHECK ( MCF_XARB_ADRTO,  MCF_XARB_ADRTO_ADRTO(55000), bInitOk ); 
  {(void *) &MCF_XARB_DATTO, MCF_XARB_DATTO_DATTO((BTO_TIME + (BTO_TIME / 10))),
            sizeof(UINT32), 0x00, REG_SET, TRUE, RFA_FORCE_UPDATE},

  // SET_AND_CHECK ( MCF_XARB_CFG,
  //               (MCF_XARB_CFG_PLDIS | MCF_XARB_CFG_AT | MCF_XARB_CFG_DT), bInitOk);
  {(void *) &MCF_XARB_CFG, (MCF_XARB_CFG_PLDIS | MCF_XARB_CFG_AT | MCF_XARB_CFG_DT),
            sizeof(UINT32), 0x0, REG_SET, TRUE, RFA_FORCE_UPDATE}, 

  // SET_AND_CHECK ( MCF_XARB_IMR, MCF_XARB_IMR_ATE | MCF_XARB_IMR_DTE, bInitOk ); 
  {(void *) &MCF_XARB_IMR, (MCF_XARB_IMR_ATE | MCF_XARB_IMR_DTE), sizeof(UINT32),
            0x0, REG_SET, TRUE, RFA_FORCE_UPDATE},

  // SET_AND_CHECK(MCF_INTC_ICR47, MCF_INTC_ICRn_IL(0x6), bInitOk); 
  {(void *) &MCF_INTC_ICR47, MCF_INTC_ICRn_IL(0x6), sizeof(UINT8), 0x0, REG_SET,
            TRUE, RFA_FORCE_UPDATE}, 
  
  // SET_CHECK_MASK_AND(MCF_INTC_IMRH, MCF_INTC_IMRH_INT_MASK47, bInitOk); 
  {(void *) &MCF_INTC_IMRH, MCF_INTC_IMRH_INT_MASK47, sizeof(UINT32), 0x0,
            REG_SET_MASK_AND, TRUE, RFA_FORCE_UPDATE}
};


/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/


/*****************************************************************************
 * Function:    BTO_Init
 *
 * Description: Initial setup for the Bus Timeout Interrupt.
 *
 * Parameters:  SysLogId - Ptr to return System Log ID into 
 *              pdata -    Ptr to buffer to return system log data 
 *              psize -    Ptr to return size (bytes) of system log data
 *
 * Returns:     RESULT: DRV_OK = Success OR 
 *                      DRV_BTO_REG_INIT_FAIL
 *              
 * Notes:       none
 ****************************************************************************/
RESULT BTO_Init (SYS_APP_ID *SysLogId, void *pdata, UINT16 *psize) 
{
  UINT8 i; 
  BOOLEAN bInitOk; 
  BTO_DRV_PBIT_LOG *pdest; 
 
  pdest = (BTO_DRV_PBIT_LOG *) pdata; 
  memset ( pdest, 0, sizeof(BTO_DRV_PBIT_LOG) ); 
  pdest->result = DRV_OK; 
  
  *psize = sizeof(BTO_DRV_PBIT_LOG);  
  
  // Initialize the BTO Registers  
  bInitOk = TRUE; 
  for (i = 0; i < (sizeof(BTO_Registers) / sizeof(REG_SETTING)); ++i) 
  {
    bInitOk &= RegSet( (REG_SETTING_PTR) &BTO_Registers[i] ); 
  }
  
  if (TRUE != STPU( bInitOk, eTpBto3947)) 
  {
    pdest->result = DRV_BTO_REG_INIT_FAIL; 
    *SysLogId = DRV_ID_PRC_PBIT_BTO_REG_INIT_FAIL; 
  }

  return pdest->result;
}


/*****************************************************************************
 * Function:    BTO_Handler
 *
 * Description: XL Arbiter Bus Timeout handler.
 *              Reads data from XARB registers and formats into error message.
 *              Called from exception.s, which also output the error message.
 *
 * Parameters:  pcAddr - PC address at point of exception   
 *
 * Returns:     pointer to error message buffer
 *              
 * Notes:
 *
 ****************************************************************************/
INT8* BTO_Handler(void)
{  
  UINT32 failAddr;
  UINT32 failSize;
  UINT32 failType;
  const CHAR*  access;

  failAddr = MCF_XARB_ADRCAP;
  failSize = MCF_XARB_SIGCAP_TSIZ(MCF_XARB_SIGCAP) >> MCF_XARB_SIGCAP_TSIZ_OFST;
  failType = MCF_XARB_SIGCAP_TT(MCF_XARB_SIGCAP);

  switch (failType)
  {
  case MCF_XARB_SIGCAP_TT_RD:
      access = AccessType[0];
      break;
  case MCF_XARB_SIGCAP_TT_WWF:
      access = AccessType[1];
      break;
  case MCF_XARB_SIGCAP_TT_WWK:
      access = AccessType[2];
      break;
  default:
      access = AccessType[3];
      break;
  }

  // check if burst mode access
  if (MCF_XARB_SIGCAP_TSIZ(MCF_XARB_SIGCAP) == 0)
  {
    failSize = 32;
  }
  else if (failSize == 0)
  {
    failSize = 8;  // 8 byte access encoded as 0
  }

  // output failed address
  sprintf(msgBuf, "\r\n\r\n\r\n\r\n"
                  "FATAL ERROR - Bus Timeout at Addr=0x%08X,\r\n"
                  "              Access Size=%d, Access Type=%s (0x%X)",
    failAddr, failSize, access, failType);
  return  msgBuf;
}


/******************************************************************************
 * Function:    BTO_WriteToEE
 *
 * Description: Grabs the Exception data buffer after Exception.s traps a Bus
 *              Timeout interrupt and writes it to EEPROM.  This routine
 *              is expected to be called from Exception.s and will not return
 *              The reentered flag prevents an exception-on-exception infinite
 *              loop.
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:       DOES NOT RETURN, SYSTEM IS REBOOTED!
 *              WD MUST BE ENABLED
 *
 *****************************************************************************/
void BTO_WriteToEE(void)
{
  TIMESTRUCT Ts;
  static BOOLEAN reentered = FALSE;
 
  __DIR(); // Interrupt level not needed, task will be disabled.
  if(!reentered)
  {
    reentered = TRUE;
    //Print out assert msg
    RTC_GetTime(&Ts);

    snprintf( ExOutputBuf, ASSERT_MAX_LOG_SIZE, 
      "%04d/%02d/%02d %02d:%02d:%02d \r\n%s%s\r\n",
      Ts.Year, Ts.Month, Ts.Day, Ts.Hour, Ts.Minute, Ts.Second,
      msgBuf, ReOutputBuf);

    Assert_WriteToEE(ExOutputBuf, ASSERT_MAX_LOG_SIZE);
  }

  WatchdogReboot(FALSE);
}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/*************************************************************************
 *  MODIFICATIONS
 *    $History: BusTO.c $
 * 
 * *****************  Version 4  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 3  *****************
 * User: Jeff Vahue   Date: 10/17/11   Time: 7:45p
 * Updated in $/software/control processor/code/drivers
 * Fix tp reference
 * 
 * *****************  Version 2  *****************
 * User: Contractor2  Date: 6/29/11    Time: 11:33a
 * Updated in $/software/control processor/code/drivers
 * SCR #515 Enhancement: sys.get.mem.l halts processor when invalid memory
 * is referenced
 * Add pipeline diable cfg bit to initialization
 * 
 * *****************  Version 1  *****************
 * User: Contractor2  Date: 6/27/11    Time: 1:42p
 * Created in $/software/control processor/code/drivers
 * SCR #515 Enhancement - sys.get.mem.l halts processor when invalid
 * memory is referenced
 * 
 *
 ***************************************************************************/
 
