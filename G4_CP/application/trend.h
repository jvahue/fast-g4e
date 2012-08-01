#ifndef TREND_H
#define TREND_H
/**********************************************************************************************
                Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
                          Altair Engine Diagnostic Solutions
                    All Rights Reserved. Proprietary and Confidential.


    File:        trend.h


    Description: Function prototypes and defines for the trend processing.

  VERSION
  $Revision: 1 $  $Date: 12-06-07 11:32a $

**********************************************************************************************/

/*********************************************************************************************/
/* Compiler Specific Includes                                                                */
/*********************************************************************************************/

/*********************************************************************************************/
/* Software Specific Includes                                                                */
/*********************************************************************************************/
#include "alt_basic.h"


/*********************************************************************************************/
/*                                 Package Defines                                           */
/*********************************************************************************************/
#define MAX_TREND_NAME      32


//*********************************************************************************************
// TREND CONFIGURATION DEFAULT
//*********************************************************************************************
#define TREND_DEFAULT              "Unused",              /* &TrendName[MAX_TREND_NAME] */\

/*********************************************************************************************/
/*                                   Package Typedefs                                        */
/*********************************************************************************************/
typedef enum
{
   TREND_1HZ            =  1,  /*  1Hz Rate    */
   TREND_2HZ            =  2,  /*  2Hz Rate    */
   TREND_4HZ            =  4,  /*  4Hz Rate    */
   TREND_5HZ            =  5,  /*  5Hz Rate    */
   TREND_10HZ           = 10,  /* 10Hz Rate    */
   TREND_20HZ           = 20,  /* 20Hz Rate    */
   TREND_50HZ           = 50   /* 50Hz Rate    */
} TREND_RATE;

typedef enum
{
   TREND_0 = 0,
   TREND_1 = 1,
   TREND_2 = 2,
   TREND_3 = 3,
   TREND_4 = 4,
   MAX_TRENDS = 5,
   TREND_UNUSED = 255
} TREND_INDEX;

#pragma pack(1)
typedef struct
{
   INT8   TrendName[MAX_TREND_NAME];   /* the name of the trend */

}TREND_CFG;

// LOGS HERE

//A type for an array of the maximum number of trends
//Used for storing Trend configurations in the configuration manager
typedef TREND_CFG TREND_CONFIGS[MAX_TRENDS];

#pragma pack()

typedef struct
{
   // Run Time Data
   TREND_INDEX      TrendIndex;
} TREND_DATA;

/**********************************************************************************************
                                      Package Exports
**********************************************************************************************/
#undef EXPORT

#if defined( TREND_BODY )
#define EXPORT
#else
#define EXPORT extern
#endif

/**********************************************************************************************
                                  Package Exports Variables
**********************************************************************************************/

/**********************************************************************************************
                                  Package Exports Functions
**********************************************************************************************/


/**********************************************************************************************
 *  MODIFICATIONS
 *    $History: trend.h $
 * 
 * *****************  Version 1  *****************
 * User: John Omalley Date: 12-06-07   Time: 11:32a
 * Created in $/software/control processor/code/application
 *
 *
 *
 *********************************************************************************************/
#endif  /* TREND_H */




