#ifndef TIMEHISTORY_H
#define TIMEHISTORY_H
/******************************************************************************
           Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
              All Rights Reserved. Proprietary and Confidential.

    File:        TimeHistory.h

    Description: Public functions/defintions.  See header in TimeHistory.c

    VERSION
    $Revision: 8 $  $Date: 11/06/12 11:49a $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "fifo.h"
#include "sensor.h"

/******************************************************************************
                                       Package Defines
******************************************************************************/
#define TH_MAX_RATE                 20
#define TH_PRE_HISTORY_S            360 //Time max pre duration in seconds
                                        //(6m*60s/m=360s)
#define TH_POST_HISTORY_S           360 //Time max post duration in seconds
                                        //(6m*60s/m=360s)

/*Padding rationale:
  1 minute padding sufficient to allow logwrite time to
  finish writing 6m without oldest data being overwritten.
  Empirical measurement puts write time for the 6 minute
  buffer at ~30s, so I roughly doubled it for padding.
  This agrees with the other measurment of about 30ms per
  10 records written.  7200 records/10 *0.03s = 22 seconds */
#define TH_PRE_HISTORY_BUF_PAD_S    60

#define TH_PRE_HISTORY_REC_CNT      (TH_MAX_RATE*(TH_PRE_HISTORY_S+TH_PRE_HISTORY_BUF_PAD_S))

#define TH_MAX_LOG_WRITE_PER_FRAME  10 //Maximum number of TH records to write to log memory
                                       //per MIF
#define TH_REC_SEARCH_PER_FRAME     100 //Number of records to search per frame, in the TH
                                        //buffer to see if they are pending write to log memeory

#define TH_LOG_PRIORITY             LOG_PRIORITY_3

#define TIMEHISTORY_DEFAULT        TH_OFF,                 /* bEnabled                   */\
                                   0,                      /* TimeHistory Rate           */\

/******************************************************************************
                                       Package Typedefs
******************************************************************************/
typedef enum
{
   TH_OFF  =  0,            /*  Disabled */
   TH_1HZ  =  1,            /*  1Hz Rate */
   TH_2HZ  =  2,            /*  2Hz Rate */
   TH_4HZ  =  4,            /*  4Hz Rate */
   TH_5HZ  =  5,            /*  5Hz Rate */
   TH_10HZ = 10,            /* 10Hz Rate */
   TH_20HZ = TH_MAX_RATE,   /* 20Hz Rate */
} TIMEHISTORY_SAMPLERATE;


typedef struct
{
   TIMEHISTORY_SAMPLERATE  sample_rate;        /* Rate to sample (1,2,4,5,10Hz)     */
   UINT32                  sample_offset_ms;  /* Offset to start sampling in mS    */
}  TIMEHISTORY_CONFIG;

typedef struct
{
   TIMEHISTORY_SAMPLERATE  sampleRate;
} TH_HDR;

/******************************************************************************
                                       Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( TIMEHISTORY_BODY )
   #define EXPORT
#else
   #define EXPORT extern
#endif
/******************************************************************************
                             Package Exports Variables
******************************************************************************/

/**********************************************************************************************
                                     Package Exports Variables
**********************************************************************************************/
/**********************************************************************************************
                                     Package Exports Functions
**********************************************************************************************/
EXPORT BOOLEAN TH_FSMAppBusyGetState(INT32 param);
EXPORT UINT16  TH_GetBinaryHeader    ( void *pDest, UINT16 nMaxByteSize );
EXPORT void TH_Open(UINT32 pre_s);
EXPORT void TH_Close(UINT32 time_s);
EXPORT void TH_Init(void);
EXPORT void TH_SetRecStateChangeEvt(INT32 tag,void (*func)(INT32,BOOLEAN));

#endif // TIMEHISTORY_H
/******************************************************************************
 *  MODIFICATIONS
 *    $History: TimeHistory.h $
 * 
 * *****************  Version 8  *****************
 * User: Jim Mood     Date: 11/06/12   Time: 11:49a
 * Updated in $/software/control processor/code/application
 * SCR 1107
 *
 * *****************  Version 7  *****************
 * User: Jim Mood     Date: 9/27/12    Time: 4:51p
 * Updated in $/software/control processor/code/application
 * SCR 1107
 *
 * *****************  Version 6  *****************
 * User: Jim Mood     Date: 9/21/12    Time: 5:22p
 * Updated in $/software/control processor/code/application
 * SCR 1107: 2.0.0 Requirements - Time History
 *
 * *****************  Version 5  *****************
 * User: John Omalley Date: 12-09-11   Time: 2:00p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Added Time History Binary Header Function
 *
 * *****************  Version 4  *****************
 * User: Jim Mood     Date: 9/06/12    Time: 5:59p
 * Updated in $/software/control processor/code/application
 * SCR #1107 Time History implementation changes
 *
 * *****************  Version 3  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 12:43p
 * Updated in $/software/control processor/code/application
 * SCR# 1142
 *
 * *****************  Version 2  *****************
 * User: John Omalley Date: 4/27/12    Time: 5:04p
 * Updated in $/software/control processor/code/application
 *
 * *****************  Version 1  *****************
 * User: John Omalley Date: 4/24/12    Time: 10:59a
 * Created in $/software/control processor/code/application
 *
 *
 *****************************************************************************/



