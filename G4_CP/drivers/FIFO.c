#define FIFO_BODY
/******************************************************************************
            Copyright (C) 2007-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.



 File:        FIFO.c

 Description: This module manages a charcter array buffer as a FIFO.
              1. The caller allocates a buffer of any size from 2-2^32
                 charcters, and a FIFO structure, then calls the Init function
                 with those objects.
              2. The caller uses the push/pop functions to add and remove data
                 from the FIFO

              These functions do not dynamically allocate any data, so FIFOs
              are only valid within the scope in which the buffer and FIFO
              struct are defined.
              
 VERSION
 $Revision: 15 $  $Date: 8/28/12 1:06p $              

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include "mcf548x.h"
#include "assert.h"
#include <string.h>

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/

#include "FIFO.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/*****************************************************************************
 * Function:    FIFO_Init
 * Description: Initialize a new FIFO.  Requres a statically allocated buffer
 *              of type CHAR, the size of the buffer and a tFIFO structure.
 *              After calling Init, the FIFO structure may then be used for calls
 *              to FIFO_Push and FIFO_Pop
 * Parameters:  FIFO: A tFIFO type to store the FIFO state in
 *              *Buf: A buffer where the FIFO data will be located
 *              Size: Size of the FIFO buffer
 * Returns:     none, there are not error states, make sure your parameters are correct.
 * Notes:
 ****************************************************************************/
void FIFO_Init(FIFO* FIFO, INT8* Buf, UINT32 Size)
{
  //ASSERT(FIFO != NULL);
  FIFO->Buf = Buf;
  FIFO->Size = Size;
  FIFO->Head = Buf;    //FIFO is empty, the head and tail both point to the beginning
  FIFO->Tail = Buf;
  FIFO->Cnt  = 0;
  FIFO->CmpltCnt = 0;
}


/*****************************************************************************
 * Function:    FIFO_Push
 * Description: Add a single char at the head of the FIFO buffer.
 * Parameters:  FIFO: A tFIFO type to store the FIFO state in
 *              Char*:  A pointer to the char to push into the buffer
 * Returns:     TRUE: Char was added to the head of the buffer
 *              FALSE: Buffer is full, or not initialized
 * Notes:
 ***************************************************************************
BOOLEAN FIFO_Push(FIFO* FIFO, const INT8* Data)
{
  BOOLEAN result = FALSE;

  //Test if the Head/Tail pointers are initialized
  ASSERT((FIFO->Tail != NULL) && (FIFO->Head != NULL));

  //Test for free room in the buffer.
  //If the Head is before the tail OR if the head is at the end of the buffer
  //AND the tail is at the beginning, the buffer is full.
  //Otherwise, push the byte on at the tail
  if(FIFO->Cnt == FIFO->Size ) 
  {    
    result = FALSE;
  }
  else 
  {
    //Add the char, check for wrap-around and return TRUE
    *FIFO->Head++ = *Data;
    FIFO->Cnt++;
    FIFO->CmpltCnt++;    
    if (FIFO->Head >= FIFO->Buf+FIFO->Size) 
    {
      FIFO->Head = FIFO->Buf;
    }
    result = TRUE;
  }
 
  return(result);
}*/


/*****************************************************************************
 * Function:    FIFO_Pop
 * Description: Remove a single charcter from the tail of the FIFO
 * Parameters:  FIFO: A tFIFO type to store the FIFO state in
 *              Char*:  A pointer to a charcter location to return the data to
 * Returns:     TRUE: Char was removed from the FIFO and is pointed to by *Data
 *              FALSE: Buffer is empty, or uninitialized.
 * Notes:
 ***************************************************************************
BOOLEAN FIFO_Pop(FIFO* FIFO, INT8* Data)
{
  BOOLEAN result = FALSE;

  //Test if the Head/Tail pointers are initialized
  ASSERT((FIFO->Tail != NULL) && (FIFO->Head != NULL));

  //Check if the FIFO is empty
  if(FIFO->CmpltCnt == 0) 
  {
    result = FALSE;
  }

  //If there is data, return the byte, incement the tail and check for wrap-around
  else 
  {
    *Data = *FIFO->Tail++;
    FIFO->Cnt--;
    FIFO->CmpltCnt--;
    if (FIFO->Tail >= FIFO->Buf+FIFO->Size ) 
    {
      FIFO->Tail = FIFO->Buf;
    }
    result = TRUE;
  }
  return(result);
}
*/


/*****************************************************************************
 * Function:    FIFO_PushBlock
 * Description: Add a block of data to the FIFO.  This method is simmilar
 *              to FIFO_Push, but it is much more efficient when adding
 *              multiple bytes of data from another memory buffer.
 *
 * Parameters:  [in/out] FIFO: A tFIFO type to store the FIFO state in
 *              [in] Char*:A pointer to a block of chars to add to the fifo
 *              [in] Len:  Size of the block to add (size in bytes)
 * Returns:     TRUE: Char was added to the head of the buffer
 *              FALSE: Buffer is full, or not initialized
 *
 * Notes:       Multi-producer/Single consumer preemptive safe queue routine.
 *              Interrupts are disabled while 'claiming' space in the queue,
 *              then re-enabled after the space is claimed.  Memcpy is
 *              performed with interrupts on for performance
 ****************************************************************************/
BOOLEAN FIFO_PushBlock(FIFO* FIFO, const void* Data, UINT32 Len)
{
  UINT32 FirstChunkSize;
  UINT32 SecondChunkSize;
  INT8   *TempHead;
  BOOLEAN result;
  INT32 intsave;
 
  intsave = __DIR();

  //Test if the Head/Tail pointers are initialized
  ASSERT((FIFO->Tail != NULL) && (FIFO->Head != NULL));

  result = FALSE;
  if(Len <= (FIFO->Size-FIFO->Cnt))
  { //if the Data fits with no FIFO buf wrapping, just copy it
    if( (FIFO->Head + Len) < (FIFO->Size + FIFO->Buf) )
    {
      //Move head and inc length before memcopy so ints can be
      //re-enabled faster
      TempHead = FIFO->Head;
      FIFO->Head += Len;
      FIFO->Cnt += Len;      
      __RIR(intsave);
      memcpy(TempHead, Data, Len);
      FIFO->CmpltCnt += Len;
    }
    else // else, break the Data up into two parts
    {
      FirstChunkSize = FIFO->Size - (FIFO->Head - FIFO->Buf);
      SecondChunkSize = Len - FirstChunkSize;

      //Move head and inc length before memcopy so ints can be
      //re-enabled faster
      TempHead = FIFO->Head;
      FIFO->Head = FIFO->Buf + SecondChunkSize;
      FIFO->Cnt += Len;      
      __RIR(intsave);
      memcpy( TempHead, Data, FirstChunkSize );
      memcpy( FIFO->Buf, &((INT8*)Data)[FirstChunkSize], SecondChunkSize );
      FIFO->CmpltCnt += Len;
    }

    result = TRUE;
  }
  __RIR(intsave);
  return result;
}


/*****************************************************************************
 * Function:    FIFO_PopBlock
 * Description: Remove a block of data from the FIFO.  This method is simmilar
 *              to FIFO_Pop, but it is much more efficient when reading multiple
 *              bytes of data.
 * Parameters:  [in/out] FIFO: A tFIFO type to store the FIFO state in
 *              [out]    *Data:  A pointer to return the data
 *              [in]     Len: Length to read, must be = or < num of bytes in
 *                            the FIFO (passing FIFO.Cnt for this
 *                            param is okay).
 *
 * Returns:     
 *
 * Notes:       Muli producer/Single consumer premptive safe queue routine.  
 *              Space is not released until after the memcpy has completed
 *              ASSERT on underflow
 *
 ****************************************************************************/
void FIFO_PopBlock(FIFO* FIFO, void* Data, UINT32 Len)
{
  UINT32 FirstChunkSize;
  UINT32 SecondChunkSize;
  INT32 intsave;

  //Test if the Head/Tail pointers are initialized
  ASSERT((FIFO->Tail != NULL) && (FIFO->Head != NULL));

  //Check if the FIFO is empty
  ASSERT_MESSAGE(FIFO->CmpltCnt >= Len,"FIFO Underflow at 0x%x",FIFO);
  //If the requested block does not wrap past the end of the FIFO
  if( FIFO->Tail + Len <= (FIFO->Buf + FIFO->Size) )
  {
    memcpy( Data, FIFO->Tail, Len );
    intsave = __DIR();
    FIFO->Tail += Len;
    FIFO->Cnt -= Len;
    FIFO->CmpltCnt -= Len;      
    __RIR(intsave);      
   }
  else //Else, the data wraps back to the start of FIFO.Buf
  {
    FirstChunkSize = FIFO->Size - (FIFO->Tail - FIFO->Buf);
    SecondChunkSize = Len - FirstChunkSize;
    memcpy(Data, FIFO->Tail, FirstChunkSize);
    memcpy(&((INT8*)Data)[FirstChunkSize], FIFO->Buf, SecondChunkSize);
    intsave = __DIR();
    FIFO->Tail = FIFO->Buf + SecondChunkSize;
    FIFO->Cnt -= Len;
    FIFO->CmpltCnt -= Len;            
    __RIR(intsave);      
  }
}


/*****************************************************************************
 * Function:    FIFO_FreeBytes
 * Description: Returns the number of free bytes, i.e. the maximum number that
 *              can be written by calling Push or PushBlock
 * Parameters:  [in]  FIFO: Pointer to a FIFO structure

 * Returns:     FreeCount: Number of free bytes in the FIFO
 *                         Returns zero if the FIFO is not initialized or
 *                         if the FIFO is not initialized
 * Notes:
 ****************************************************************************/
UINT32 FIFO_FreeBytes(FIFO* FIFO)
{
  UINT32 result;
  INT32 intsave;
  
  intsave = __DIR();

  
  result = FIFO->Size - FIFO->Cnt;
  
  __RIR(intsave);
  
  return result;
}



/*****************************************************************************
 * Function:    FIFO_Flush
 * Description: Allows the caller to flush any data in the FIFO and reset
 *              it back to the empty condition.
 * Parameters:  FIFO: Pointer to the FIFO to be flushed
 * Returns:     None
 * Notes:
 ****************************************************************************/
void FIFO_Flush(FIFO* FIFO)
{
  INT32 intsave;
  
  intsave = __DIR();
  FIFO->Head = FIFO->Buf;    //FIFO is empty, the head and tail both point to the beginning
  FIFO->Tail = FIFO->Buf;
  FIFO->Cnt  = 0;
  FIFO->CmpltCnt = 0;
  __RIR(intsave);
}

/*****************************************************************************/
/* Local Functions                                                           */ 
/*****************************************************************************/

/*************************************************************************
 *  MODIFICATIONS
 *    $History: FIFO.c $
 * 
 * *****************  Version 15  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 14  *****************
 * User: Jeff Vahue   Date: 11/01/10   Time: 7:50p
 * Updated in $/software/control processor/code/drivers
 * SCR# 975 - Code Coverage refactor
 * 
 * *****************  Version 13  *****************
 * User: Jim Mood     Date: 10/26/10   Time: 12:29p
 * Updated in $/software/control processor/code/drivers
 * SCR 965 UART Direction Control and dead code removal
 * 
 * *****************  Version 12  *****************
 * User: John Omalley Date: 10/15/10   Time: 9:45a
 * Updated in $/software/control processor/code/drivers
 * SCR 933 - Dead Code Removal
 * 
 * *****************  Version 11  *****************
 * User: Jim Mood     Date: 9/30/10    Time: 5:42p
 * Updated in $/software/control processor/code/drivers
 * SCR 904 Code Coverage changes
 * 
 * *****************  Version 10  *****************
 * User: Jim Mood     Date: 8/12/10    Time: 4:09p
 * Updated in $/software/control processor/code/drivers
 * SCR 757 Change Null pointer checks to asserts
 * 
 * *****************  Version 9  *****************
 * User: Contractor3  Date: 7/29/10    Time: 12:33p
 * Updated in $/software/control processor/code/drivers
 * SCR #698 - Fixes based on code review findings.
 * 
 * *****************  Version 8  *****************
 * User: Contractor3  Date: 3/31/10    Time: 11:48a
 * Updated in $/software/control processor/code/drivers
 * SCR #521 Code Review Issues
 * 
 * *****************  Version 7  *****************
 * User: Jim Mood     Date: 3/12/10    Time: 11:03a
 * Updated in $/software/control processor/code/drivers
 * 
 * *****************  Version 6  *****************
 * User: Jim Mood     Date: 2/26/10    Time: 10:45a
 * Updated in $/software/control processor/code/drivers
 * SCR # 465
 * 
 * *****************  Version 5  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 11:53a
 * Updated in $/control processor/code/drivers
 * SCR #87 Function Prototype
 * 
 *
 ***************************************************************************/
