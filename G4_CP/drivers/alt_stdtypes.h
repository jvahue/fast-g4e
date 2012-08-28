#ifndef alt_stdtypes_H
#define alt_stdtypes_H
/******************************************************************************
            Copyright (C) 2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:         alt_stdtypes.h

    Description: This file provides the standard data types for all Altair Software systems.
 
 VERSION
      $Revision: 18 $  $Date: 8/28/12 1:06p $ 
 
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include <limits.h>

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/

/******************************************************************************
                                 Package Defines
******************************************************************************/
#undef FALSE
#undef TRUE
#define FALSE 0
#define TRUE  1

#undef OFF
#undef ON
#undef TOGGLE
#define OFF    FALSE
#define ON     TRUE
#define TOGGLE 2

#undef FAIL
#undef PASS
#define FAIL FALSE
#define PASS TRUE

#undef NO
#undef YES
#define NO FALSE
#define YES TRUE 

#undef ON
#undef OFF
#define ON TRUE
#define OFF FALSE

#define UINT8_MAX          UCHAR_MAX
#define INT8_MIN           SCHAR_MIN
#define INT8_MAX           SCHAR_MAX
#define UINT16_MAX         USHRT_MAX
#define INT16_MIN          SHRT_MIN
#define INT16_MAX          SHRT_MAX
#define UINT32_MAX         UINT_MAX
#define INT32_MIN          INT_MIN
#define INT32_MAX          INT_MAX

/******************************************************************************
                                 Package Typedefs
******************************************************************************/

/******************************************************************************
Package Exports                               
******************************************************************************/
#undef EXPORT                                                                  

#if defined ( ALT_STDTYPES_BODY )                                                  
#define EXPORT                                                                 
#else                                                                          
#define EXPORT extern                                                          
#endif                                                                         

/******************************************************************************
                                   Numeric typedefs

  The following standard types are allowed.

  BOOLEAN : 0 .. 1, FALSE, TRUE

  INT8    : -128 .. 127
  INT16   : -32768 .. 32767
  INT32   : -2147483648 .. 2147483647

  UINT8   : 0 .. 255
  UINT16  : 0 .. 65535
  UINT32  : 0 .. 4294967296

  FLOAT32 : 24 bit mantissa, 8 bit exponent  (six significant digits)
  FLOAT64 : 53 bit mantissa, 10 bit exponent (15  significant digits)

  BITARRAY128 : 4 x UINT32 representing a 128 bit array for object state and validity mgmt. 
                     
                           

  If the compiler does not support the type it should not be defined thus allowing the compiler
  to identify where changes to the code are required.

  For a specific embedded program the else portion of the conditional compilation should be
  defined, this allows projects source code to be run/debugged on a PC and the target if
  needed.

******************************************************************************/
typedef char               CHAR;
typedef unsigned char      UINT8;
typedef unsigned char      BYTE;

typedef short int          INT16;
typedef unsigned short int UINT16;
typedef signed short       SINT16;

typedef int                INT32;
typedef unsigned int       UINT32;
typedef signed long        SINT32;

typedef float              FLOAT32;
typedef double             FLOAT64;

typedef  UINT32 BITARRAY128[4];

#if defined( WIN32)
    #define __interrupt
    #define __EI()
    #define __DI()
    #define __RIR(x) 
    #define __DIR()  0

    #define strtok_r(a,b,c) strtok(a,b)
    #define snprintf _snprintf
    #define strcasecmp strcmpi
    #define strtod g4eStrtod

    typedef BYTE          BOOLEAN;
    typedef signed char   INT8;

#else

    #define strtok_r strtok_r
    #define snprintf snprintf
    #define strcasecmp strcasecmp
    #define strtod strtod

    typedef char          BOOLEAN;
    typedef char          INT8;


#endif

#include "alt_Time.h"




/******************************************************************************
                             Package Exports Variables
******************************************************************************/

/******************************************************************************
                             Package Exports Functions
******************************************************************************/

#endif // alt_stdtypes_H
/******************************************************************************
 *  MODIFICATIONS
 *    $History: alt_stdtypes.h $
 * 
 * *****************  Version 18  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 17  *****************
 * User: Contractor V&v Date: 3/21/12    Time: 6:47p
 * Updated in $/software/control processor/code/drivers
 * SCR #1107 FAST 2  Refactoring, bug fixes
 * 
 * *****************  Version 16  *****************
 * User: Contractor V&v Date: 3/14/12    Time: 4:51p
 * Updated in $/software/control processor/code/drivers
 * SCR #1107 Trigger processing
 * 
 * *****************  Version 15  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:10a
 * Updated in $/software/control processor/code/drivers
 * SCR #698 - Fix code review findings
 * 
 * *****************  Version 14  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:02p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 13  *****************
 * User: Jeff Vahue   Date: 2/26/10    Time: 3:00p
 * Updated in $/software/control processor/code/drivers
 * SCR# 469 - Change DIO_ReadPin to return the pin value, WIN32 handling
 * of _DIR and _RIR 
 * 
 * *****************  Version 12  *****************
 * User: Jeff Vahue   Date: 2/11/10    Time: 1:40p
 * Updated in $/software/control processor/code/drivers
 * WIN32 fix for strtok_r, cleans up compiler messages in Windows
 * 
 * *****************  Version 11  *****************
 * User: Jeff Vahue   Date: 12/07/09   Time: 4:24p
 * Updated in $/software/control processor/code/drivers
 * SCR# 364 - WIN32 operation of strtod,same as target
 * 
 * *****************  Version 10  *****************
 * User: Jim Mood     Date: 10/01/09   Time: 9:25a
 * Updated in $/software/control processor/code/drivers
 * 
 * *****************  Version 9  *****************
 * User: Jim Mood     Date: 9/29/09    Time: 6:20p
 * Updated in $/software/control processor/code/drivers
 * 
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 6/29/09    Time: 1:25p
 * Updated in $/software/control processor/code/drivers
 * SCR #204 Misc Code Review Updates
 * 
 * 
 *
 *****************************************************************************/

