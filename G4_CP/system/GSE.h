#ifndef GSE_H
#define GSE_H

/******************************************************************************
          Copyright (C) 2007-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.
 
  File:        GSE.h
 
  Description: Provides a GSE specific interface to UART port 0.
 
 
   VERSION
    $Revision: 10 $  $Date: 8/28/12 1:43p $
    
******************************************************************************/



/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "UART.h"
#include "ResultCodes.h"
//#include "SystemLog.h"
#include "FaultMgr.h"

/******************************************************************************
                                 Package Defines
******************************************************************************/
#define GSE_UART_PORT       0
#define GSE_UART_DUPLEX     UART_CFG_DUPLEX_HALF  
#define GSE_UART_BPS        UART_CFG_BPS_115200
#define GSE_UART_DATA_BITS  UART_CFG_DB_8
#define GSE_UART_STOP_BITS  UART_CFG_SB_1
#define GSE_UART_PARITY     UART_CFG_PARITY_NONE

#define ESC_CHAR 0x1B

#define GSE_GET_LINE_BUFFER_SIZE 256
#define GSE_TX_BUFFER_SIZE (8192*2)

#define GSE_TX_BUFFER_AVAIL_WAIT_MS 1000


/******************************************************************************
                                 Package Typedefs
******************************************************************************/

#pragma pack(1)

typedef struct {
  RESULT  result; 
} GSE_DRV_PBIT_LOG; 

#pragma pack()
             

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined( GSE_BODY )
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
EXPORT RESULT GSE_Init    (SYS_APP_ID *SysLogId, void *pdata, UINT16 *psize);
EXPORT void   GSE_PutLine (const CHAR* Str);
EXPORT RESULT GSE_PutLineBlocked(const CHAR* Str, BOOLEAN bBlock);
EXPORT RESULT GSE_GetLine (CHAR* Str);
EXPORT INT16  GSE_getc    (void);
EXPORT BOOLEAN GSE_write   (const void *buf, UINT16 size);
EXPORT INT16  GSE_read    (void *buf, UINT16 size);

EXPORT void GSE_DebugStr( const FLT_DBG_LEVEL DbgLevel, const BOOLEAN Timestamp, 
                          const CHAR* str, ...);
EXPORT void GSE_StatusStr( const FLT_DBG_LEVEL DbgLevel, const CHAR* str, ...);

EXPORT void GSE_ToggleDisplayLiveStream(void);


#endif // GSE_H
/*************************************************************************
 *  MODIFICATIONS
 *    $History: GSE.h $
 * 
 * *****************  Version 10  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 9  *****************
 * User: Jeff Vahue   Date: 9/03/10    Time: 8:22p
 * Updated in $/software/control processor/code/system
 * SCR# 853 - UART Int Monitor
 * 
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 8/31/10    Time: 2:39p
 * Updated in $/software/control processor/code/system
 * SCR #840 Code Review Updates
 * 
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 7/29/10    Time: 8:06p
 * Updated in $/software/control processor/code/system
 * SCR #698 Code Review Updates
 * 
 * *****************  Version 6  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:10a
 * Updated in $/software/control processor/code/system
 * SCR #698 - Fix code review findings
 * 
 * *****************  Version 5  *****************
 * User: Contractor2  Date: 7/09/10    Time: 4:32p
 * Updated in $/software/control processor/code/system
 * SCR #8 Implementation: External Interrupt Monitors
 * 
 * *****************  Version 4  *****************
 * User: Contractor V&v Date: 6/22/10    Time: 6:25p
 * Updated in $/software/control processor/code/system
 * SCR #485 Escape Sequence and Box Configuration
 * 
 * *****************  Version 3  *****************
 * User: Contractor3  Date: 5/27/10    Time: 10:57a
 * Updated in $/software/control processor/code/system
 * SCR #617 - Changes made to meet coding standards as a result of code
 * review.
 * 
 * *****************  Version 2  *****************
 * User: Jeff Vahue   Date: 4/06/10    Time: 3:49p
 * Updated in $/software/control processor/code/system
 * SCR# 526 - Increase Max GSE Tx FIFO size
 * 
 * *****************  Version 1  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:35p
 * Created in $/software/control processor/code/system
 * SCR# 496 - Move GSE files from driver to system
 * 
 * *****************  Version 15  *****************
 * User: Contractor V&v Date: 3/19/10    Time: 4:30p
 * Updated in $/software/control processor/code/drivers
 * SCR #493 Code Review compliance
 * 
 * *****************  Version 14  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/drivers
 * SCR# 483 - Function Names
 * 
 * *****************  Version 13  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:22p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 12  *****************
 * User: Jim Mood     Date: 2/24/10    Time: 10:49a
 * Updated in $/software/control processor/code/drivers
 * 
 * *****************  Version 11  *****************
 * User: Contractor V&v Date: 12/10/09   Time: 5:42p
 * Updated in $/software/control processor/code/drivers
 * SCR 42
 * 
 * *****************  Version 10  *****************
 * User: Contractor V&v Date: 11/18/09   Time: 3:25p
 * Updated in $/software/control processor/code/drivers
 * Implementing SCR 196
 * 
 * *****************  Version 9  *****************
 * User: Jim Mood     Date: 11/17/09   Time: 4:10p
 * Updated in $/software/control processor/code/drivers
 * SCR #283-item 16 GSE Max Length magic number.  Also, added get char and
 * write binary fuctions for the binary log file transfer process
 * 
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 9/15/09    Time: 6:11p
 * Updated in $/software/control processor/code/drivers
 * SCR #94, #95 PBIT returns log structure to Init Mgr
 * 
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 9/14/09    Time: 3:48p
 * Updated in $/software/control processor/code/drivers
 * SCR #94 Updated Init for SET_CHECK() and return ERR CODE Struct
 * 
 * *****************  Version 6  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 12:11p
 * Updated in $/control processor/code/drivers
 * SCR #87 Function Prototype
 * 
 *
 ***************************************************************************/

