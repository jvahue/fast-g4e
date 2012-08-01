#ifndef FIFO_H
#define FIFO_H
/******************************************************************************
            Copyright (C) 2007-2010 Pratt & Whitney Engine Services, Inc. 
                      Altair Engine Diagnostic Solutions
               All Rights Reserved. Proprietary and Confidential.
 
  File:        FIFO.h
 
  Description: This file defines the interface for the software FIFO. See the 
                .c module for a detailed description.
  
    VERSION
    $Revision: 9 $  $Date: 10/26/10 12:29p $       
  
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/

/******************************************************************************
                                 Package Defines
******************************************************************************/

/******************************************************************************
                                 Package Typedefs
******************************************************************************/
typedef struct {   //A structure containing data needed to implement the FIFO
  INT8*  Buf;      //Pointer to the buffer space allocated to the FIFO
  UINT32 Size;     //Size of the buffer space
  INT8*  Head;     //Pointer to the next empty space to PUSH new data on
  INT8*  Tail;     //Pointer to the next char to be POP data off
  UINT32 Cnt;      //Count of characters in the buffer (changed before memcpy on push)
  UINT32 CmpltCnt; //Count of characters completely copied into FIFO 
                   //(changed after memcpy on push)
}FIFO;

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined( FIFO_BODY )
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
EXPORT void    FIFO_Init      (FIFO* FIFO, INT8* Buf, UINT32 Size);
//EXPORT BOOLEAN FIFO_Push      (FIFO* FIFO, const INT8* Data);
//EXPORT BOOLEAN FIFO_Pop       (FIFO* FIFO, INT8* Data);
EXPORT BOOLEAN FIFO_PushBlock (FIFO* FIFO, const void* Data, UINT32 Len);
EXPORT void    FIFO_PopBlock  (FIFO* FIFO, void* Data, UINT32 Len);
EXPORT UINT32  FIFO_FreeBytes (FIFO* FIFO);
EXPORT void    FIFO_Flush     (FIFO* FIFO);

/*****************************************************************************
 *  MODIFICATIONS
 *    $History: FIFO.h $
 * 
 * *****************  Version 9  *****************
 * User: Jim Mood     Date: 10/26/10   Time: 12:29p
 * Updated in $/software/control processor/code/drivers
 * SCR 965 UART Direction Control and dead code removal
 * 
 * *****************  Version 8  *****************
 * User: John Omalley Date: 10/15/10   Time: 9:45a
 * Updated in $/software/control processor/code/drivers
 * SCR 933 - Dead Code Removal
 * 
 * *****************  Version 7  *****************
 * User: Jim Mood     Date: 9/30/10    Time: 5:42p
 * Updated in $/software/control processor/code/drivers
 * SCR 904 Code Coverage changes
 * 
 * *****************  Version 6  *****************
 * User: Contractor3  Date: 3/31/10    Time: 11:48a
 * Updated in $/software/control processor/code/drivers
 * SCR #521 Code Review Issues
 * 
 * *****************  Version 5  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:14p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 * 
 *
 *****************************************************************************/

#endif  //FIFO_H
