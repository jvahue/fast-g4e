#ifndef ALT_BASIC_H
#define ALT_BASIC_H
/******************************************************************************
            Copyright (C) 2007-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

 File: alt_basic.h

 Description: This file provides a set of basic defintions that are common to
              all Altair software.

 VERSION
      $Revision: 12 $  $Date: 11/29/12 7:23p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/


/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "compiler.h"

/******************************************************************************
                                   Package Defines
******************************************************************************/
#define NEW_LINE "\r\n"
#define NEWLINE  NEW_LINE
#define NL NEW_LINE

/******************************************************************************
  Return the state of a Bit
******************************************************************************/
#define BIT( word, bit)  ((word)>>(bit) & 1U)

/******************************************************************************
  Create a mask for a bit field
    bit - the start position of the field
    size - the size of the field
******************************************************************************/
#define MASK( bit, size) (((1U<<(size))-1U) << (bit))

/******************************************************************************
  Extract a bit field from a word and place it in the LSB
    word (w) - the word to extract form
    bit  (b) - the start position in the word
    size (s) - the size of the bit field
******************************************************************************/
#define EXTRACT( w, b, s) ( ((w) >> (b)) & ((1U << (s)) - 1U) )

/******************************************************************************
  Position a value in the LSBits of a word into a field
    w - the word to create a field from
    b - the resultant LSB of the bit field
    s - the size of the field
******************************************************************************/
#define FIELD( w, b ,s) ( ((w) & ( (1U << (s)) - 1U)) << (b) )

/******************************************/
/* Reset a goup of bits in a word         */
/* word - the word to reset bits in       */
/* pattern - the pattern of bits to clear */
/******************************************/
#define RESET_BITS( word, pattern) ( (word) & ~(pattern))

/*-----------------------------------------------------------------------------
  Switch and Min/Max Buffer operators.
-----------------------------------------------------------------------------*/
#define SWITCH( if, then, else)   ((if) ? (then) : (else))

#define MAX( a, b) SWITCH( (a) > (b), (a), (b))
#define MIN( a, b) SWITCH( (a) < (b), (a), (b))

/*---------------------------------------------------------------------------------------------
  SET AND CHECK Macro
---------------------------------------------------------------------------------------------*/

#define SET_AND_CHECK( a, b, result)  \
{\
   a = b;\
   result = ((a == b) && result);\
}

#define SET_CHECK_MASK_AND( a, b, result)\
{\
   a &= ~b;\
   result = (((a & b) == 0x00) && result);\
}

#define SET_CHECK_MASK_OR( a, b, result)\
{\
   a |= b;\
   result = (((a & b) == b) && result);\
}

#define CHECK_SETTING( a, b, result )\
{\
  result = ((a == b) && result);\
}

#define CHECK_SETTING_AND_MASK( a, b, result )\
{\
   result = (((a & b) == 0x00) && result);\
}

#define CHECK_SETTING_AND_OR( a, b, result )\
{\
   result = (((a & b) == b) && result);\
}

/******************************************************************************
                                   Package Typedefs
******************************************************************************/

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( ALT_BASIC_BODY )
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

#endif // ALT_BASIC_H
/*************************************************************************
 *  MODIFICATIONS
 *    $History: alt_basic.h $
 * 
 * *****************  Version 12  *****************
 * User: Jim Mood     Date: 11/29/12   Time: 7:23p
 * Updated in $/software/control processor/code/drivers
 * SCR #1197
 *
 * *****************  Version 11  *****************
 * User: Melanie Jutras Date: 12-11-02   Time: 1:38p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 File Format Error
 *
 * *****************  Version 10  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 9  *****************
 * User: Contractor3  Date: 5/27/10    Time: 10:56a
 * Updated in $/software/control processor/code/drivers
 * SCR #617 - Changes made to meet coding standards as a result of code
 * review.
 *
 * *****************  Version 8  *****************
 * User: Jeff Vahue   Date: 4/28/10    Time: 5:42p
 * Updated in $/software/control processor/code/drivers
 * SCR #572 - remove warning about strncpy
 *
 * *****************  Version 7  *****************
 * User: Contractor V&v Date: 4/12/10    Time: 7:19p
 * Updated in $/software/control processor/code/drivers
 * SCR #317 Implement safe strncpy
 *
 * *****************  Version 6  *****************
 * User: Contractor V&v Date: 3/29/10    Time: 6:16p
 * Updated in $/software/control processor/code/drivers
 * SCR #317 Implement safe strncpy
 *
 * *****************  Version 5  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:02p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 *
 *
 ***************************************************************************/
