#ifndef TIMEHISTORY_H
#define TIMEHISTORY_H
/******************************************************************************
                    Copyright (C) 2012 Pratt & Whitney Engine Services, Inc. 
                       All Rights Reserved. Proprietary and Confidential.

    File:        TimeHistory.h
    
    Description: 

    VERSION
    $Revision: 4 $  $Date: 9/06/12 5:59p $   
    
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
**********************************************************************************************/
#define TH_MAX_RATE                 20
#define TH_PRE_HISTORY_S            360 //Time max pre duration in seconds (6m*60s/m=360s)
#define TH_POST_HISTORY_S           360 //Time max post duration in seconds (6m*60s/m=360s)
#define TH_PRE_HISTORY_DATA_SZ      100     
#define TH_MAX_RECORD_SZ            100 //TODO: Define acutal size 

#define TH_PRE_HISTORY_REC_CNT      (TH_MAX_RATE*TH_PRE_HISTORY_S)

#define TH_MAX_LOG_WRITE_PER_FRAME  10 //Maximum number of TH records to write to log memory
                                       //per MIF
#define TH_REC_SEARCH_PER_FRAME     10 //Number of records to search per frame, in the TH 
                                       //buffer to see if they are pending write to log memeory

#define TH_LOG_PRIORITY             LOG_PRIORITY_3
//*********************************************************************************************
// TIMEHISTORY CONFIGURATION DEFAULT
//*********************************************************************************************
#define TIMEHISTORY_DEFAULT        TH_1HZ,                 /* bEnabled                   */\
                                   0,                      /* TimeHistory Rate           */\

/******************************************************************************
                                       Package Typedefs
******************************************************************************/
typedef enum 
{
   TH_1HZ  =  1,            /*  1Hz Rate */
   TH_2HZ  =  2,            /*  2Hz Rate */
   TH_4HZ  =  4,            /*  4Hz Rate */
   TH_5HZ  =  5,            /*  5Hz Rate */
   TH_10HZ = 10,            /* 10Hz Rate */
   TH_20HZ = TH_MAX_RATE,   /* 20Hz Rate */
} TIMEHISTORY_SAMPLERATE;


typedef struct
{
   TIMEHISTORY_SAMPLERATE  SampleRate;        /* Rate to sample (1,2,4,5,10Hz)     */
   UINT16                  nSampleOffset_ms;  /* Offset to start sampling in mS    */
}  TIMEHISTORY_CONFIG;




/******************************************************************************
                                       Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( TIMEHISTORY_BODY )
   #define EXPORT
#else
   #define EXPORT extern
#endif
#define TH_NONE 0  //TODO Temporary
#define TH_ALL 1
#define TH_PORTION 2

#define TimeHistoryStart(A, B) 
#define TimeHistoryEnd(A, B)      
/******************************************************************************
                             Package Exports Variables
******************************************************************************/
#if defined ( TIMEHISTORY_BODY )
// TODO: Remove and use just the time in seconds instead. 
// Note: Updates to TIMEHISTORY_TYPE has dependency to TH_UserEnumType[]

EXPORT USER_ENUM_TBL TH_UserEnumType[] =
{ { "NONE",     TH_NONE    },
  { "ALL",      TH_ALL     },
  { "PORTION",  TH_PORTION },
  { NULL,       0          }
};
#else
EXPORT USER_ENUM_TBL TH_UserEnumType[];
#endif

typedef struct
{int A; } TIMEHISTORY_TYPE;
/**********************************************************************************************
                                     Package Exports Variables
**********************************************************************************************/
/**********************************************************************************************
                                     Package Exports Functions
**********************************************************************************************/
EXPORT BOOLEAN TH_FSMAppBusyGetState(INT32 param);
EXPORT void TH_Open(INT32 pre_s);
EXPORT void TH_Close(INT32 time_s);

#define TimeHistoryStart(A, B) 
#define TimeHistoryEnd(A, B)                

#endif // TIMEHISTORY_H 
/******************************************************************************
 *  MODIFICATIONS
 *    $History: TimeHistory.h $
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
 
 

