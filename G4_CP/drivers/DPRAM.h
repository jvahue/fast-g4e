#ifndef DRV_DPRAM_H
#define DRV_DPRAM_H
/******************************************************************************
            Copyright (C) 2008-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

  File:          DPRAM.h
 
  Description:   Dual-port RAM driver
 
  VERSION
  $Revision: 20 $  $Date: 9/27/10 2:55p $

 ******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/ 
#include "mcf548x.h"

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "ResultCodes.h"
#include "SystemLog.h"

/******************************************************************************
                                 Package Defines
******************************************************************************/
//DPRAM Buffer and control flag locations.
//Two buffers are allocated for
//full-duplex operation.  Each buffer has an interrupt flag, ownership flag
//and a data count.  The buffers have a fixed starting point, data always
//is read from the start address (lowest value address) to start+count-1
//The control processor interrupt flag is set by the microserver and cleared
//by the contol processor
//The ownership flag indicates if the buffer's transmitter or receiver owns
//the buffer.  It is expected that while idle, the tranmsmitter owns its buffer
//and the receiver will own the buffer while copying data out of it.
#ifndef WIN32
  #define DPRAM_BASE_ADDR         0x40100000
#else
  #include "HwSupport.h"
#endif

#define DPRAM_SIZE              16380       //Total buffer size, minus
                                            //interrupt control words.
#define DPRAM_TX_RX_SIZE        8180        //DPRAM size for the rx or tx buffer
                                            //space only
//CP transmit to MS buffer definitions
#define DPRAM_CP_TO_MS_BUF_ADDR (DPRAM_BASE_ADDR+0x2000)
#define DPRAM_CP_TO_MS_INT_FLAG (DPRAM_BASE_ADDR+0x3FFE)
#define DPRAM_CP_TO_MS_OWN_FLAG (DPRAM_BASE_ADDR+0x3FF6)  
#define DPRAM_CP_TO_MS_COUNT    (DPRAM_BASE_ADDR+0x3FFA)  
#define DPRAM_CP_TO_MS_BUF_SIZE (0x2000-6)                

//MS transmit to CP buffer definitions
#define DPRAM_MS_TO_CP_BUF_ADDR (DPRAM_BASE_ADDR+0x0000)
#define DPRAM_MS_TO_CP_INT_FLAG (DPRAM_BASE_ADDR+0x3FFC)
#define DPRAM_MS_TO_CP_OWN_FLAG (DPRAM_BASE_ADDR+0x3FF4)
#define DPRAM_MS_TO_CP_COUNT    (DPRAM_BASE_ADDR+0x3FF8)
#define DPRAM_MS_TO_CP_BUF_SIZE (0x2000-6)

//Definition for the ownership flags for both buffers
#define DPRAM_TX_OWNS 0
#define DPRAM_RX_OWNS 1

//Size for the local receive and transmit buffers
#define DPRAM_LOCAL_BUF_SIZE 32768

#define DPRAM_MIN_INTERRUPT_TIME 2500 // min accepted time between interrupts (uSec)
#define DPRAM_MAX_INTERRUPT_CNT  50   // max number of consecutive fast interrupts

/******************************************************************************
                                 Package Typedefs
******************************************************************************/

#pragma pack(1)

typedef struct {
  UINT8 expectData; 
  UINT8 actualData; 
} DPRAM_RAM_TEST_RESULT;

typedef struct {
  RESULT  result; 
  DPRAM_RAM_TEST_RESULT dpram_result; 
} DPRAM_DRV_PBIT_LOG; 

typedef struct {
  INT32 size;
  INT8  data[DPRAM_TX_RX_SIZE];
}DPRAM_WRITE_BLOCK;

#pragma pack()

typedef struct {
  UINT32  lastTime;         // Last Interrupt Time (uS)
  UINT32  minIntrTime;      // Minimum time between interrupts (uS)
  UINT32  maxExeTime;       // Maximum execution time (uS)
  UINT32  intrCnt;          // Consecutive Fast Interrupt Counter
  BOOLEAN intrFail;         // Excessive Interrupt Failure Flag
} DPRAM_STATUS, *DPRAM_STATUS_PTR;

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined( DRV_DPRAM_BODY )
  #define EXPORT
#else
  #define EXPORT extern
#endif

/******************************************************************************
                             Package Exports Variables
******************************************************************************/
EXPORT DPRAM_STATUS m_DPRAM_Status;

/******************************************************************************
                             Package Exports Functions
******************************************************************************/
EXPORT RESULT DPRAM_Init (SYS_APP_ID *SysLogId, void *pdata, UINT16 *psize); 
EXPORT RESULT DPRAM_WriteBlock(const DPRAM_WRITE_BLOCK* block);
EXPORT RESULT DPRAM_ReadBlock  (INT8* Data, UINT32 Cnt, UINT32* BytesRead);
EXPORT UINT32 DPRAM_WriteFreeCnt(void);
/*EXPORT UINT32 DPRAM_ReadCnt();*/
EXPORT void DPRAM_GetTxInfo(UINT32* TxCnt, BOOLEAN* MsgPending);
/*EXPORT void DPRAM_GetRxInfo(UINT32* RxCnt, BOOLEAN* MsgPending);*/
EXPORT void DPRAM_KickMS(void);
EXPORT RESULT DPRAM_InitStatus(void);
EXPORT __interrupt void DPRAM_EP5_ISR(void);


#endif // DRV_DPRAM_H
/*************************************************************************
 *  MODIFICATIONS
 *    $History: DPRAM.h $
 * 
 * *****************  Version 20  *****************
 * User: Contractor2  Date: 9/27/10    Time: 2:55p
 * Updated in $/software/control processor/code/drivers
 * SCR #795 Code Review Updates
 * 
 * *****************  Version 19  *****************
 * User: Contractor2  Date: 8/05/10    Time: 11:19a
 * Updated in $/software/control processor/code/drivers
 * SCR #243 Implementation: CBIT of mssim SRS-3625
 * 
 * *****************  Version 18  *****************
 * User: Jim Mood     Date: 7/13/10    Time: 10:56a
 * Updated in $/software/control processor/code/drivers
 * SCR 698 modfix
 * 
 * *****************  Version 17  *****************
 * User: Contractor2  Date: 7/09/10    Time: 4:31p
 * Updated in $/software/control processor/code/drivers
 * SCR #8 Implementation: External Interrupt Monitors
 * 
 * *****************  Version 16  *****************
 * User: Jim Mood     Date: 5/18/10    Time: 6:24p
 * Updated in $/software/control processor/code/drivers
 * SCR 594.  YMODEM/FIFO preemption issue
 * 
 * *****************  Version 15  *****************
 * User: Jim Mood     Date: 3/12/10    Time: 11:08a
 * Updated in $/software/control processor/code/drivers
 * SCR #465 and #466 modfix for reentrancy of WriteBlock and size of dpram
 * TX buffer
 * 
 * *****************  Version 14  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:14p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 13  *****************
 * User: Peter Lee    Date: 11/02/09   Time: 2:08p
 * Updated in $/software/control processor/code/drivers
 * Misc non code update to support WIN32 and spelling.
 * 
 * *****************  Version 12  *****************
 * User: Jim Mood     Date: 10/08/09   Time: 6:13p
 * Updated in $/software/control processor/code/drivers
 * SCR #283
 * 
 * *****************  Version 11  *****************
 * User: Peter Lee    Date: 9/15/09    Time: 6:10p
 * Updated in $/software/control processor/code/drivers
 * SCR #94, #95 PBIT returns log structure to Init Mgr
 * 
 * *****************  Version 10  *****************
 * User: Peter Lee    Date: 9/14/09    Time: 3:24p
 * Updated in $/software/control processor/code/drivers
 * SCR #94 Updated Init for SET_CHECK() and return ERR CODE Struct
 * 
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 6/29/09    Time: 2:24p
 * Updated in $/software/control processor/code/drivers
 * SCR #204 Misc Code Updates
 * 
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 11:44a
 * Updated in $/control processor/code/drivers
 * SCR #87 Function Prototype
 * 
 *
 ***************************************************************************/

