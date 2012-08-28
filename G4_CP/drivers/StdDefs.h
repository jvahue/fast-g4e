#ifndef STDDEFS_H
#define STDDEFS_H

/******************************************************************************
Copyright (C) 2003-2012 Pratt & Whitney Engine Services, Inc.                 
All Rights Reserved. Proprietary and Confidential.
 
 File:        StdDefs.h
 
 Description: Standard "C" definitions to be used through out the DTU 
              firmware.
 
 VERSION
 $Revision: 6 $  $Date: 8/28/12 1:06p $
******************************************************************************/



/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include <string.h>
#include <stdio.h>



/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
// None



/******************************************************************************
                              Package Defines
******************************************************************************/
typedef unsigned char STATUS;



#ifndef NULL
#define NULL    ((void *) 0)
#endif


#define UINT8MAX        (0xFF)
#define INT8MAX         (0x7F)
#define INT8MIN         (0x80)

#define UINT16MAX       (0xFFFF)
#define INT16MAX        (0x7FFF)
#define INT16MIN        (0x8000)

#define UINT32MAX       (0xFFFFFFFF)
#define INT32MAX        (0x7FFFFFFF)
#define INT32MIN        (0x80000000)

#define INVALID_FLOAT   (9999.999F)
#define INVALID_FIX32   (FLT2FIX32(INVALID_FLOAT))


// Logical Defines
#define DISABLED        ((BOOLEAN) 0)
#define ENABLED         ((BOOLEAN) 1)

#undef ERROR
#define ERROR           -1



// Offset to member in structure from the base of the structure
#define OFFSET(Structure, Member)  ( (int)&( ((Structure*)0) -> Member) )


// Bit positions
#define B0      (1)
#define B1      (1<<1)
#define B2      (1<<2)
#define B3      (1<<3)
#define B4      (1<<4)
#define B5      (1<<5)
#define B6      (1<<6)
#define B7      (1<<7)
#define B8      (1<<8)
#define B9      (1<<9)
#define B10     (1<<10)
#define B11     (1<<11)
#define B12     (1<<12)
#define B13     (1<<13)
#define B14     (1<<14)
#define B15     (1<<15)
#define B16     (1<<16)
#define B17     (1<<17)
#define B18     (1<<18)
#define B19     (1<<19)
#define B20     (1<<20)
#define B21     (1<<21)
#define B22     (1<<22)
#define B23     (1<<23)
#define B24     (1<<24)
#define B25     (1<<25)
#define B26     (1<<26)
#define B27     (1<<27)
#define B28     (1<<28)
#define B29     (1<<29)
#define B30     (1<<30)
#define B31     (1<<31)


// Size abbreviations
#define _KB     1024
#define _64KB   (64*_KB)
#define _MB     (_KB*_KB)


// Macros to manipulate BCD digits within bytes
//   LOBCD(byte) returns BCD value in bits3:0
//   HIBCD(byte) returns BCD value in bits7:4
//   MAKEBCDBYTE(low_value,high_value) returns byte with low value in bits3:0
//   and high_value in bits7:4
#define LOBCD(v) ((UINT8) ((UINT8)(v)&0x0F))
#define HIBCD(v) ((UINT8) (((UINT8)(v) >> 4) & 0x0F))
#define MAKEBCDBYTE(lo,hi) ((UINT8) ( ((UINT8)(lo)&0x0F) | (((UINT8)(hi)&0x0F)<<4) ))


// Extract bytes from word data
#define HIGH_BYTE(a) ((a) >> 8)
#define LOW_BYTE(a)  ((a) & 0xFF)


// Integer divide with rounding
#define DIVR(_num_,_denom_) (((_num_)+((_denom_)>>1))/(_denom_))

// Round a number
#define ROUND(_num_,_denom_) (((_num_)+((_denom_)/2))/_denom_)


// Interrupt Service Routine wrapper
//   NOTE: Compiler dependent interface.
#define ISR(_func_) _IH extern void _func_


// TEST COMPILING under Microsoft Dev Studio
#ifdef WIN32
# define _IH            // Tasking compiler interrupt handler interface
# define _SPL(_x_) (0)  // Tasking compiler processor service level
#endif

//
extern BOOLEAN g_DebugMsgEnable;
 
#ifdef WIN32
    #define PrintDebugString(str)  if ( g_DebugMsgEnable ) printf( (str) );
    #define PRINT_DEBUG_STRING(str)      printf( (str) );
#else
    #define PrintDebugString(str)  if ( g_DebugMsgEnable ) SciWriteString( (str) );
    #define PRINT_DEBUG_STRING(str)      SciWriteString( (str) );
#endif



/******************************************************************************
                              Package Typedefs
******************************************************************************/



/******************************************************************************
                              Package Exports
******************************************************************************/
#undef EXPORT

#if defined( STDDEFS_BODY )
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

/*****************************************************************************
*  MODIFICATIONS
*    $History: StdDefs.h $
 * 
 * *****************  Version 6  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 5  *****************
 * User: Contractor3  Date: 6/25/10    Time: 9:46a
 * Updated in $/software/control processor/code/drivers
 * SCR #662 - Changes based on code review
*
*
*****************************************************************************/

#endif    //  STDDEFS_H


// End Of File

