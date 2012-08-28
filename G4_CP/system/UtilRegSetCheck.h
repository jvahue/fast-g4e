#ifndef UTIL_REG_SET_CHECK_H
#define UTIL_REG_SET_CHECK_H

/******************************************************************************
            Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:         UtilRegSetCheck.h
    
    Description: 
          Function prototypes of functions to support generic Register Set 
          and then verify the register value. 
    
    VERSION
      $Revision: 7 $  $Date: 8/28/12 1:43p $
    
******************************************************************************/


/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    


/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_basic.h"


/******************************************************************************
                                 Package Defines
******************************************************************************/
typedef enum {
  REG_SET,
  REG_SET_MASK_AND,
  REG_SET_MASK_OR,
  REG_MAX
} SET_ACTION; 

typedef enum {
  RFA_FORCE_UPDATE, 
  RFA_FORCE_RESET,
  RFA_FORCE_NONE
} REG_FAIL_ACTION; 


/******************************************************************************
                                 Package Typedefs
******************************************************************************/
#pragma pack(1)
typedef struct {
  void *ptrReg;              // Pointer to HW Register 
  UINT32 value;              // Value to set
  UINT16 size;               // Size of parameter (UINT32, UINT16, UINT8)
  UINT32 enumReg;            // Identifies the Reg that for the Reg CBIT check 
  SET_ACTION setAction;      // SET, MASK_AND, MASK_OR 
  BOOLEAN bCheckAble;        // Is this register checkable ? 
  REG_FAIL_ACTION FailAction; // Action to take if CBIT check of reg fails 
  BOOLEAN bDynamicValue;     // Value is stored and owned by external entity 
  void *Addr1;               // External entity Shadow 1 copy 
  void *Addr2;               // External entity Shadow 2 copy 
  UINT16 cksum;              // Checksum of this buffer 
} REG_SETTING, *REG_SETTING_PTR; 
#pragma pack()


/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( UTIL_REG_SET_CHECK_BODY )
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
EXPORT void RegSetCheck_Init( void ); 

EXPORT BOOLEAN RegSet( REG_SETTING_PTR pRegData ); 
EXPORT UINT16 RegCheck( UINT16 Cnt, UINT32 *RegAddr, UINT32 *RegEnum ); 
EXPORT BOOLEAN RegAdd ( REG_SETTING_PTR pRegData ); 

EXPORT BOOLEAN RegSetOrUpdate( void *ptrReg, UINT32 value, UINT16 size, UINT32 enumReg ); 

EXPORT UINT16 GetRegCheckNumReg( void ); 
                              


     

#endif // UTIL_REG_SET_CHECK_H       
/*************************************************************************
 *  MODIFICATIONS
 *    $History: UtilRegSetCheck.h $
 * 
 * *****************  Version 7  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 6  *****************
 * User: Peter Lee    Date: 9/28/10    Time: 9:53a
 * Updated in $/software/control processor/code/system
 * SCR #892 Code Review Updates
 * 
 * *****************  Version 5  *****************
 * User: Peter Lee    Date: 6/17/10    Time: 3:21p
 * Updated in $/software/control processor/code/system
 * SCR #634 Update to perform 2 out of 3 check/update for "dynamic" reg
 * val (i.e. dio).  
 * 
 * *****************  Version 4  *****************
 * User: Contractor3  Date: 6/10/10    Time: 10:44a
 * Updated in $/software/control processor/code/system
 * SCR #642 - Changes based on Code Review
 * 
 * *****************  Version 3  *****************
 * User: Peter Lee    Date: 11/05/09   Time: 5:30p
 * Updated in $/software/control processor/code/system
 * SCR #315 DIO CBIT for SEU Processing
 * 
 * *****************  Version 1  *****************
 * User: Peter Lee    Date: 10/23/09   Time: 9:34a
 * Created in $/software/control processor/code/system
 * Initial Check In.  SCR #315
 * 
 *
 ***************************************************************************/
                                            
