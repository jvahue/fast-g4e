#ifndef TIMEHISTORY_H
#define TIMEHISTORY_H
/**********************************************************************************************
                    Copyright (C) 2012 Pratt & Whitney Engine Services, Inc. 
                              Altair Engine Diagnostic Solutions
                       All Rights Reserved. Proprietary and Confidential.

    File:        TimeHistory.h
    
    Description: 

    VERSION
    $Revision: 2 $  $Date: 4/27/12 5:04p $   
    
**********************************************************************************************/

/*********************************************************************************************/
/* Compiler Specific Includes                                                                */
/*********************************************************************************************/    
 
/*********************************************************************************************/
/* Software Specific Includes                                                                */
/*********************************************************************************************/
#include "fifo.h"
#include "sensor.h"

/**********************************************************************************************
                                       Package Defines                                       
**********************************************************************************************/
#define MAX_PREHISTORY_MINUTES   6
#define MAX_TIMEHISTORY_BUFFER   (UINT32)((MAX_PREHISTORY_MINUTES * 60) * THR_10HZ * sizeof(TIMEHISTORYRECORD) )

//*********************************************************************************************
// TIMEHISTORY CONFIGURATION DEFAULT
//*********************************************************************************************
#define TIMEHISTORY_DEFAULT        FALSE,                 /* bEnabled                   */\
                                   THR_1HZ,               /* TimeHistory Rate           */\
                                   0                      /* TimeHistory Offset         */

/**********************************************************************************************
                                       Package Typedefs
**********************************************************************************************/
typedef enum 
{
   THR_1HZ  =  1,  /*  1Hz Rate */
   THR_2HZ  =  2,  /*  2Hz Rate */
   THR_4HZ  =  4,  /*  4Hz Rate */
   THR_5HZ  =  5,  /*  5Hz Rate */
   THR_10HZ = 10,  /* 10Hz Rate */
} TIMEHISTORY_SAMPLERATE;

typedef enum
{
   TH_NONE,
   TH_ALL,
   TH_PORTION
} TIMEHISTORY_TYPE;

typedef enum 
{
   TH_END_IMMEDIATELY,
   TH_END_AFTER_PORTION
} TIMEHISTORY_END;

typedef struct
{
   UINT8   Index;
   FLOAT32 Value;
   BOOLEAN Validity;
} TIMEHISTORY_SENSOR_SUMMARY; // TBD: Maybe this should be in the sensor object

typedef struct
{
   BOOLEAN                 bEnabled;
   TIMEHISTORY_SAMPLERATE  SampleRate;        /* Rate to sample (1,2,4,5,10Hz)     */
   UINT16                  nSampleOffset_ms;  /* Offset to start sampling in mS    */
}  TIMEHISTORY_CONFIG;

typedef struct
{
   TIMESTAMP  Time;                 /* time for log entry       */
   UINT16     nNumSensors;
   UINT8      RawSensorBuffer[sizeof(TIMEHISTORY_SENSOR_SUMMARY) * MAX_SENSORS];
} TIMEHISTORYRECORD, *TIMEHISTORYRECORD_PTR;

typedef struct
{
   UINT32 Head;
   UINT32 Tail;
   UINT8  Buffer[MAX_TIMEHISTORY_BUFFER];
} TIMEHISTORY_QUEUE;


/**********************************************************************************************
                                       Package Exports
**********************************************************************************************/
#undef EXPORT

#if defined ( TIMEHISTORY_BODY )
   #define EXPORT
#else
   #define EXPORT extern
#endif

/******************************************************************************
                             Package Exports Variables
******************************************************************************/
#if defined ( TIMEHISTORY_BODY )
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

/**********************************************************************************************
                                     Package Exports Variables
**********************************************************************************************/
EXPORT void        TimeHistoryStart     ( TIMEHISTORY_TYPE Type, UINT16 Time_s );
EXPORT void        TimeHistoryEnd       ( TIMEHISTORY_TYPE Type, UINT16 Time_s );
/**********************************************************************************************
                                     Package Exports Functions
**********************************************************************************************/

/**********************************************************************************************
 *  MODIFICATIONS
 *    $History: TimeHistory.h $
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
 *********************************************************************************************/
 
#endif // TIMEHISTORY_H                             

