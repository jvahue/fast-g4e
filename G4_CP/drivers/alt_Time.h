#ifndef ALT_TIME_H
#define ALT_TIME_H

/******************************************************************************
            Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:        alt_Time.h        
    
    Description: Definitions for using time units.
    
    VERSION
    $Revision: 5 $  $Date: 8/28/12 1:06p $
    
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/

/******************************************************************************
                                 Package Defines
******************************************************************************/
#define BASE_YEAR 1997
#define MAX_YEAR  2088

//---- Time Constants

#define MILLISECONDS_PER_SECOND 1000U
#define MILLISECONDS_PER_HOUR   MILLISECONDS_PER_SECOND * \
                                SECONDS_PER_MINUTE * \
                                MINUTES_PER_HOUR
#define SECONDS_PER_MINUTE      60
#define MINUTES_PER_HOUR        60
#define HOURS_PER_DAY           24
#define MONTHS_PER_YEAR         12

// Misc #define to be moved to other locations
#define SECONDS_120            120
#define SECONDS_10              10
#define SECONDS_20              20

#define DATETIME_STRING_LEN     25      // Number of chars in a date and time
                                        // formatted string
#define TIME_STRING_LEN         14

/******************************************************************************
                                 Package Typedefs
******************************************************************************/
#pragma pack(1) 
typedef struct
{
    UINT32      Timestamp;             // Time / Date relative to Jan 01, BASE_YEAR
    UINT16      MSecond;
} TIMESTAMP;
#pragma pack()

 
typedef struct
{
    UINT16          Year;              // 1997..2082
    UINT16          Month;             // 0..11
    UINT16          Day;               // 0..31   Day of Month
    UINT16          Hour;              // 0..23
    UINT16          Minute;            // 0..59
    UINT16          Second;            // 0..59
    UINT16          MilliSecond;       // 0..999
} TIMESTRUCT;

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( ALT_TIME_BODY )
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


#endif // ALT_TIME_H                             
/*************************************************************************
 *  MODIFICATIONS
 *    $History: alt_Time.h $
 * 
 * *****************  Version 5  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 4  *****************
 * User: Contractor3  Date: 5/27/10    Time: 10:56a
 * Updated in $/software/control processor/code/drivers
 * SCR #617 - Changes made to meet coding standards as a result of code
 * review.
 * 
 * *****************  Version 3  *****************
 * User: Peter Lee    Date: 10/21/09   Time: 4:12p
 * Updated in $/software/control processor/code/drivers
 * SCR #229 Update Ch/Startup TimeOut to 1 sec from 1 msec resolution.
 * Updated GPB channel loss timeout to 1 msec from 1 sec resolution. 
 * 
 * *****************  Version 2  *****************
 * User: Peter Lee    Date: 10/21/09   Time: 2:46p
 * Updated in $/software/control processor/code/drivers
 * SCR #312 Add msec value to logs
 * 
 * *****************  Version 1  *****************
 * User: Peter Lee    Date: 4/29/09    Time: 4:17p
 * Created in $/software/control processor/code/drivers
 * SCR #168 Update original time.h to alt_time.h
 *
 ***************************************************************************/

