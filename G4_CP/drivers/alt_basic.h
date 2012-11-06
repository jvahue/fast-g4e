#ifndef ALT_BASIC_H
#define ALT_BASIC_H
/******************************************************************************
            Copyright (C) 2007-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

 File: alt_basic.h

 Description: This file provides a set of basic defintions that are common to all Altair
 software.

 VERSION
      $Revision: 11 $  $Date: 12-11-02 1:38p $

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
#define CR '\r'
#define LF '\n'
#define TAB '\t'
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


/******************************************************************************
  Set one bit in a word
    word - the word to set the bit in
    bit  - the bit to set
******************************************************************************/
#define SET_BIT( word, bit) ( (word) | (1U << (bit)) )

/*************************************/
/* Set a group of bits in a word     */
/* word - the word to set bits in    */
/* pattern - the bit patterns to set */
/*************************************/
#define SET_BITS( word, pattern) ( (word) | (pattern))

/***************************************************/
/* Reset a single bit in a word                    */
/* word - the word that contains the bit to reset  */
/* bit - the bit in the word to reset, set to zero */
/***************************************************/
#define RESET_BIT(  word, bit)     ( ~(1U << (bit)) & word)

/******************************************/
/* Reset a goup of bits in a word         */
/* word - the word to reset bits in       */
/* pattern - the pattern of bits to clear */
/******************************************/
#define RESET_BITS( word, pattern) ( (word) & ~(pattern))

/*------------------------ Byte (un)Packing ---------------------*/
/* NOTE: these values should be used in all (UN)PACK_BYTE calls  */
/*---------------------------------------------------------------*/
#define BYTE0   0
#define BYTE1   8
#define BYTE2  16
#define BYTE3  24

/**********************************************************/
/* Pack a byte into a word                                */
/* pw - the word to pack the byte into                    */
/* b  - the byte to pack, the LSbyte of this word is used */
/* p  - position of the byte (i.e., BYTE0 .. BYTE3)       */
/**********************************************************/
#define PACK_BYTE( pw, b, p) ( (pw) | (( (b) & 0xFF) << p) )

/***************************************/
/* Extract a byte from a word          */
/* uw - word to extract the byte from  */
/* p  - the byte position to extract   */
/***************************************/
#define UNPACK_BYTE( pw, p)  ( ((pw) >> (p)) & 0xFF)

/*--------------------- Half Word (un)Packing -------------------*/
/* NOTE: these values should be used in all (UN)PACK_HALF calls  */
/*---------------------------------------------------------------*/
#define LOWER   0
#define UPPER  16

/**********************************************************/
/* Pack a half word byte into a word                      */
/* pw - the word to pack the half into                    */
/* b  - the half to pack, the LOWER  of this word is used */
/* p  - position of the half (i.e., LOWER .. UPPER)       */
/**********************************************************/
#define PACK_HALF( pw, b, p) ( (pw) | (( (b) & 0xFFFF) << p) )

/***************************************/
/* Extract a HALF from a word          */
/* uw - word to extract the HALF from  */
/* p  - the HALF position to extract   */
/***************************************/
#define UNPACK_HALF( pw, p) ( ((pw) >> (p)) & 0xFFFF)

/**************************************************/
/* Word Part Manipulation                         */
/**************************************************/
#define LSHALF( word) ((word) & 0xFFFF)
#define MSHALF( word) (((word) >> 16) & 0xFFFF)

#define GET_BYTE( word, byte) (((word) >> (byte)) & 0xFF)

/*-----------------------------------------------------------------------------
  BOOLEAN operations
   NOT  - inverts the input
   XOR  - exclusive OR
   AND  - standard AND function
    OR  - standard OR  function
   ANDN - standard AND with 2nd input NOTed
    ORN - standard OR  with 2nd input NOTed
------------------------------------------------------------------------------*/
#define  NOT( i1)     ((i1) ^ 1)
#define  XOR( i1, i2) ((i1) ^ (i2))
#define  AND( i1, i2) ((i1) & (i2))
#define   OR( i1, i2) ((i1) | (i2))
#define ANDN( i1, i2) ((i1) & NOT(i2))
#define  ORN( i1, i2) ((i1) & NOT(i2))

/*-----------------------------------------------------------------------------
  Latches
   SLATCH - Set   Dominate Latch, output will be true  whenever set   is true
   RLATCH - Reset Dominate Latch, output will be false whenever reset is true

  Usage:
   value = RLATCH( setCondition, resetCondition, value);
   value = SLATCH( setCondition, resetCondition, value);
-------------------------------------------------------------------------------*/
#define RLATCH( set, reset, last) ANDN( OR( set, last), reset)
#define SLATCH( set, reset, last) ANDN( OR( set, last), ANDN( reset, set) )

/*-----------------------------------------------------------------------------
  Switch, Min/Max, and Circular Buffer operators.
-----------------------------------------------------------------------------*/
#define SWITCH( if, then, else)   ((if) ? (then) : (else))

#define MAX( a, b) SWITCH( (a) > (b), (a), (b))
#define MIN( a, b) SWITCH( (a) < (b), (a), (b))

#define INC_WRAP( v, m)  ((v) >= ((m)-1)) ? 0 : ((v)+1)
#define DEC_WRAP( v, m)  ((v) == 0) ? ((m)-1) : ((v)-1)

#define MEMBER(  m, l, u) (((m) >= (l)) & ((m) <= (u)))
#define BETWEEN( m, l, u) (((m) >  (l)) & ((m) <  (u)))
#define ARRAY( m, u)      MEMBER( m, 0, u-1)


/*---------------------------------------------------------------------------------------------
  SET AND CHECK Macro
---------------------------------------------------------------------------------------------*/

#define SET_AND_CHECK( a, b, result)  \
{\
   a = b;\
   result &= (a == b);\
}

#define SET_CHECK_MASK_AND( a, b, result)\
{\
   a &= ~b;\
   result &= ((a & b) == 0x00);\
}

#define SET_CHECK_MASK_OR( a, b, result)\
{\
   a |= b;\
   result &= ((a & b) == b);\
}

#define CHECK_SETTING( a, b, result )\
{\
  result &= (a == b);\
}

#define CHECK_SETTING_AND_MASK( a, b, result )\
{\
   result &= ((a & b) == 0x00);\
}

#define CHECK_SETTING_AND_OR( a, b, result )\
{\
   result &= ((a & b) == b);\
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
