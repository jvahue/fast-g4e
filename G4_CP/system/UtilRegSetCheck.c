#define UTIL_REG_SET_CHECK_BODY
/******************************************************************************
            Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:         UtilRegSetCheck.c
    
    Description: 
          Utility functions to support generic Register Set and then 
          verify the register value. 
          
    VERSION
      $Revision: 17 $  $Date: 11/09/10 4:41p $
    
******************************************************************************/


/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    
// #include "mcf548x.h"

#include <stdio.h>


/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "UtilRegSetCheck.h"
#include "stddefs.h"
#include "utility.h"
#include "Assert.h"


/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define MAX_SYS_REG_CHECK 1000 


/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
typedef struct 
{
  UINT16 numReg; 
  UINT16 nCurrCheckEntry;  
} REG_CHECK_STATUS; 


/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static REG_SETTING SystemRegTbl[MAX_SYS_REG_CHECK]; 
static REG_SETTING SystemRegTblShadow[MAX_SYS_REG_CHECK]; 
static REG_CHECK_STATUS RegCheckStatus;

/*****************************************************************************/
/* Local Constants                                                           */
/*****************************************************************************/


/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static BOOLEAN RegSetUintX  ( REG_SETTING_PTR pRegData );
static BOOLEAN RegSetUint8  ( REG_SETTING_PTR pRegData );
static BOOLEAN RegSetUint16 ( REG_SETTING_PTR pRegData );
static BOOLEAN RegSetUint32 ( REG_SETTING_PTR pRegData );

static BOOLEAN RegCheckUint8  ( REG_SETTING_PTR pRegData, BOOLEAN bUpdate ); 
static BOOLEAN RegCheckUint16 ( REG_SETTING_PTR pRegData, BOOLEAN bUpdate ); 
static BOOLEAN RegCheckUint32 ( REG_SETTING_PTR pRegData, BOOLEAN bUpdate ); 

static void RegUpdateEntryChksum ( UINT16 index  ); 
static BOOLEAN RegCmpEntries ( UINT16 index  ); 



/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/*****************************************************************************
 * Function:   RegSetCheck_Init
 *
 * Description: Initializes data variables for the RegSetCheck Module 
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:       none
 *
 ****************************************************************************/
void RegSetCheck_Init( void ) 
{
  memset ( &RegCheckStatus, 0x00, sizeof(REG_CHECK_STATUS) ); 
  memset ( &SystemRegTbl, 0x00, sizeof(REG_SETTING) * MAX_SYS_REG_CHECK); 
  memset ( &SystemRegTblShadow, 0x00, sizeof(REG_SETTING) * MAX_SYS_REG_CHECK); 
}


/*****************************************************************************
 * Function:    RegSet
 *
 * Description: Sets the Register passed in
 *
 * Parameters:  ptr to register data object to set 
 *
 * Returns:     TRUE  - If Set of Register SUCCESS 
 *              FALSE - If Set of Register FAILS
 *
 * Notes:       none
 *
 ****************************************************************************/
BOOLEAN RegSet ( REG_SETTING_PTR pRegData ) 
{
  BOOLEAN bInitOk; 

  bInitOk = TRUE; 
    
  // Determine the Reg size and update, if not a dynamic val 
  //   For dynamic val just add entry to table 
  bInitOk = RegSetUintX ( pRegData ); 

  // Only if Reg Init success will we add to the system reg table 
  if ( (bInitOk) && (pRegData->bCheckAble) )
  {
    // Add entry to the SystemReg[] table and Update Reg Count 
    SystemRegTblShadow[RegCheckStatus.numReg] = *pRegData; 
    SystemRegTbl[RegCheckStatus.numReg] = *pRegData; 
    
    // Update the Chksum for both copies 
    RegUpdateEntryChksum ( RegCheckStatus.numReg ); 
    
    // Point to next open entry 
    RegCheckStatus.numReg++; 
  }
  
  // ASSERT(RegCheckStatus.numReg >= MAX_SYS_REG_CHECK
  
  return (bInitOk); 
}


/*****************************************************************************
 * Function:    RegAdd
 *
 * Description: Adds the Reg info to the Check Tbl 
 *
 * Parameters:  ptr to register data object to set 
 *
 * Returns:     TRUE  - If Set of Register SUCCESS 
 *              FALSE - If Set of Register FAILS
 *
 * Notes:       none
 *
 ****************************************************************************/
BOOLEAN RegAdd ( REG_SETTING_PTR pRegData ) 
{
  BOOLEAN bInitOk; 

  bInitOk = TRUE; 
    
  // Only if Reg Init success will we add to the system reg table 
  if ( (bInitOk) && (pRegData->bCheckAble) )
  {
    // Add entry to the SystemReg[] table and Update Reg Count 
    SystemRegTblShadow[RegCheckStatus.numReg] = *pRegData; 
    SystemRegTbl[RegCheckStatus.numReg] = *pRegData; 
    
    // Update the Chksum for both copies 
    RegUpdateEntryChksum ( RegCheckStatus.numReg ); 
    
    // Point to next open entry 
    RegCheckStatus.numReg++; 
  }
  
  // ASSERT(RegCheckStatus.numReg >= MAX_SYS_REG_CHECK
  
  return (bInitOk); 
}


/*****************************************************************************
 * Function:    RegCheck
 *
 * Description: Scolls thru the SystemRegTbl[] and checks the entry 
 *
 * Parameters:  Cnt - Numbers of entries to check 
 *              *RegAddr - ptr to return the Reg Addr which had failed 
 *              *RegEnum - ptr to return the Reg Enum Description which had failed 
 *
 * Returns:     nFailures - Number of entry checks that had failed 
 *
 * Notes:       none
 *
 ****************************************************************************/
UINT16 RegCheck ( UINT16 Cnt, UINT32 *RegAddr, UINT32 *RegEnum ) 
{
  UINT16 nFailures; 
  REG_SETTING_PTR pRegData;  
  REG_SETTING_PTR pRegDataShadow;  
  BOOLEAN bCheckOk; 
  UINT16 i,j; 
  BOOLEAN bBadEntry; 

  
  pRegData = &SystemRegTbl[RegCheckStatus.nCurrCheckEntry]; 
  pRegDataShadow = &SystemRegTblShadow[RegCheckStatus.nCurrCheckEntry]; 
  
  nFailures = 0; 
  *RegAddr = 0; 
  *RegEnum = RegCheckStatus.nCurrCheckEntry; 
  
  Cnt = MIN( Cnt, RegCheckStatus.numReg );
  
  for (i=0;i<Cnt;i++) 
  {
    bCheckOk = FALSE; 
    bBadEntry = FALSE; 
    
    j = 0; 
    
    // Check twice if failed to ensure we have not been interrupted. 
    while ( (j < 2) && (bCheckOk == FALSE) )
    {
      j++; 
    
      // Compare the two sw register first, FALSE if compare is bad 
      bCheckOk = RegCmpEntries( RegCheckStatus.nCurrCheckEntry ); 
      
      // At this point if bCheckOk indicates FALSE, then the register can not be verified, 
      //   because the reg info is corrupted in both primary and shadow copies ! 
      if (bCheckOk == TRUE) 
      {
        bBadEntry = FALSE; 
      
        // Check if entry is verifiable 
        if ( pRegData->bCheckAble == TRUE)
        {
          // Compare sw reg with hw reg 
          // Check the Reg based on size !
          switch ( pRegData->size ) 
          {
            case (sizeof(UINT32)):
              bCheckOk = RegCheckUint32 ( pRegData, FALSE ); 
              break; 
              
            case (sizeof(UINT16)):
              bCheckOk = RegCheckUint16 ( pRegData, FALSE ); 
              break; 
              
            case (sizeof(UINT8)):
              bCheckOk = RegCheckUint8 ( pRegData, FALSE ); 
              break; 
            
            default:
              FATAL("Unsupported Reg Data size = %d", pRegData->size);
              break;
          } // End switch()
        } // End of if Reg is checkable 
        
      } // End if (bCheckOk == TRUE) 
      else 
      {
        bBadEntry = TRUE; 
      }
      
    } // End while(j) loop 

    
    if ( bCheckOk == FALSE ) 
    {
      nFailures++; 
      
      // If entry is good then failure must be mis match, make attempt to update register !
      if (bBadEntry == FALSE) 
      {
        if (*RegAddr == 0) // Set err addr ret to the first err
        {
          *RegAddr = (UINT32) pRegData->ptrReg; 
          // *RegEnum = (UINT32) pRegData->enumReg; 
        }
      
        // Check with hw failed.   Perform required action.  
        if ( pRegData->FailAction == RFA_FORCE_UPDATE ) 
        {
          // This is a hack but there is code in RegCheckUintXX() to update the shadow copies, 
          //    rather then create additional func, just re-call again, with update TRUE
          switch ( pRegData->size ) 
          {
            case (sizeof(UINT32)):
              bCheckOk = RegCheckUint32 ( pRegData, TRUE ); 
              break; 
              
            case (sizeof(UINT16)):
              bCheckOk = RegCheckUint16 ( pRegData, TRUE ); 
              break; 
              
            case (sizeof(UINT8)):
              bCheckOk = RegCheckUint8 ( pRegData, TRUE ); 
              break; 
            
            default:
              FATAL("Unsupported Reg Data size = %d", pRegData->size);
              break;
          } // End switch()
          
          // Always call RegSetUintX() to force update of HW reg in this case. 
          bCheckOk = RegSetUintX ( pRegData ); 
        }
  
        if ( ( pRegData->FailAction == RFA_FORCE_RESET ) || (bCheckOk == FALSE) )
        {
          // We can't successfully set the Reg Again 
          // ASSERT and indicate which Reg
        }
      }
      
    } // End of bCheckOk == FALSE     
    
    // Advance the pointer to next entry 
    if ( ++RegCheckStatus.nCurrCheckEntry >= RegCheckStatus.numReg ) 
    {
      RegCheckStatus.nCurrCheckEntry = 0;  // Reset pointer to start 
      pRegData = &SystemRegTbl[RegCheckStatus.nCurrCheckEntry]; 
      pRegDataShadow = &SystemRegTblShadow[RegCheckStatus.nCurrCheckEntry]; 
    }
    else 
    { 
      pRegData++;
      pRegDataShadow++; 
    }

  } // End for i loop 
  
  return (nFailures); 
  
}


/*****************************************************************************
 * Function:    RegSetOrUpdate
 *
 * Description: Scrolls thru the SystemRegTbl[] and updates the entry where 
 *              first address is encountered.  
 *              Tbl entry must be a REG_SET type else ASSERTS ! 
 *
 *              If entry is not found, will insert new entry.  For entry set
 *              defaults to REG_SET, is CheckAble and RFA_FORCE_UPDATE ! 
 *
 * Parameters:  void *ptrReg
 *              UINT32 value
 *              UINT16 size
 *              UINT32 enumReg
 *
 * Returns:     BOOLEAN bInitOk - TRUE  on success
 *                                FALSE on failure
 *
 * Notes:       none
 *
 ****************************************************************************/
BOOLEAN RegSetOrUpdate ( void *ptrReg, UINT32 value, UINT16 size, UINT32 enumReg ) 
{
  BOOLEAN bEntryFound; 
  REG_SETTING RegData; 
  REG_SETTING_PTR pRegData;  
  UINT16 i; 
  BOOLEAN bInitOk; 
    
  bEntryFound = FALSE; 
  
  pRegData = &SystemRegTbl[0]; 

  // initialize as done in RegSetCheck_Init
  memset( &RegData, 0, sizeof( RegData));
  
  // Scroll thru SystemRegTbl[] for entry
  for (i=0;i<RegCheckStatus.numReg;i++) 
  {
    if ( ptrReg == pRegData->ptrReg ) 
    {
      bEntryFound = TRUE; 
      break;
    }
    else 
    {
      pRegData++; 
    }
  }
  
  if ( bEntryFound == TRUE ) 
  {
    // NOTE: For entry found, setAction must be REG_SET or use of this func is "invalid"
    // ASSERT( pRegData->setAction != REG_SET );

      // JV need these set
      RegData.setAction = pRegData->setAction; 
      RegData.bCheckAble = pRegData->bCheckAble; 
      RegData.FailAction = pRegData->FailAction;
  }
  else 
  {
    // New Entry set setAction, bCheckAble and FailAction to defaults ! 
    RegData.setAction = REG_SET; 
    RegData.bCheckAble = TRUE; 
    RegData.FailAction = RFA_FORCE_UPDATE; 
  }
  
  // Create table entry 
  RegData.ptrReg = ptrReg; 
  RegData.value = value; 
  RegData.size = size; 
  RegData.enumReg = enumReg; 

  // Attempt to set entry, based on the Reg size 
  bInitOk = RegSetUintX ( &RegData ); 
  
  if (bInitOk)
  {
    // Update SystemRegTbl[] 
    memcpy ( (void *) &SystemRegTbl[i], (void *) &RegData, sizeof(REG_SETTING) ); 
    // Update SystemRegTblShadow[]
    memcpy ( (void *) &SystemRegTblShadow[i], (void *) &RegData, sizeof(REG_SETTING) ); 
    
    // Update the Chksum for both copies 
    RegUpdateEntryChksum ( i ); 

    if ( bEntryFound == FALSE ) 
    {
      // New Entry.  Update entry count 
      RegCheckStatus.numReg++; 
    }    
  }

  return bInitOk; 
  
}


/*****************************************************************************
 * Function:    RegSetDynamicObj 
 *
 * Description: Adds Reg entry for dynamic type objects (i.e. DIO, etc) 
 *              where the value are stored in external variables.  
 *              Also initially sets / checks the register.  
 *
 * Parameters:  *ptrReg
 *              mask
 *              size 
 *              enumReg
 *              *Addr1
 *              *Addr2
  *
 * Returns:     
 *
 * Notes:       
 *  1) Defaults RFA_FORCE_UPDATE for failed action always
 *
 ****************************************************************************/
/* 
BOOLEAN RegSetDynamicObj ( void *ptrReg, UINT32 value, UINT16 size, UINT32 enumReg, 
                                 void *Addr1,  void *Addr2 ) 
{
  
} 
*/
 
 


/*****************************************************************************
 * Function:    GetRegCheckNumReg
 *
 * Description: Return the number of Registers to check in SystemRegTbl[]
 *
 * Parameters:  none
 *
 * Returns:     Number of Registers to Check in SystemRegTbl[]
 *
 * Notes:       none
 *
 ****************************************************************************/
UINT16 GetRegCheckNumReg ( void ) 
{
  return (RegCheckStatus.numReg); 
}


/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/


/*****************************************************************************
 * Function:    RegSetUintX
 *
 * Description: Sets the Register based on its size 
 *
 * Parameters:  ptr to register data object to set 
 *
 * Returns:     TRUE if set was successful 
 *              FALSE if set was not successful 
 *
 * Notes:       none
 *
 ****************************************************************************/
static BOOLEAN RegSetUintX ( REG_SETTING_PTR pRegData )
{
  BOOLEAN bInitOk; 
  
  bInitOk = TRUE; 

  // Set the Register 
  switch ( pRegData->size ) 
  {
    case (sizeof(UINT32)):
      bInitOk = RegSetUint32 ( pRegData ); 
      break; 
      
    case (sizeof(UINT16)):
      bInitOk = RegSetUint16 ( pRegData ); 
      break; 
      
    case (sizeof(UINT8)):
      bInitOk = RegSetUint8 ( pRegData ); 
      break; 
    
    default:
       FATAL("Unsupported Reg Data size = %d", pRegData->size);
       break;
  }
  
  return (bInitOk); 
  
} 


/*****************************************************************************
 * Function:    RegSetUint8
 *
 * Description: Sets the 8 bit Register passed in
 *
 * Parameters:  ptr to register data object to set 
 *
 * Returns:     BOOLEAN bInitOk - TRUE on success
 *                                FALSE on failure
 *
 * Notes:       none
 *
 ****************************************************************************/
static BOOLEAN RegSetUint8 ( REG_SETTING_PTR pRegData )
{
  UINT8 *pDest;
  UINT8 data; 
  BOOLEAN bInitOk; 
  
  pDest = (UINT8 *) pRegData->ptrReg; 
  data = (UINT8) pRegData->value; 
  bInitOk = TRUE; 
  
  if ( pRegData->bDynamicValue == TRUE )
  {
    // Update the Reg 
    *pDest = (UINT8) (*pDest & (~data)) | (UINT8) *((UINT32 *) pRegData->Addr1); 
    
    // Check the setting 
    CHECK_SETTING( (UINT8) (*(UINT32 *) pRegData->Addr1), (UINT8) (*pDest & data), bInitOk); 
  }
  else 
  {
    switch ( pRegData->setAction ) 
    {
      case REG_SET:
        SET_AND_CHECK( *pDest, data, bInitOk); 
        break; 
        
      case REG_SET_MASK_AND:
        SET_CHECK_MASK_AND( *pDest, data, bInitOk); 
        break;
        
      case REG_SET_MASK_OR:
        SET_CHECK_MASK_OR( *pDest, data, bInitOk); 
        break; 
        
      default: 
       FATAL("Unsupported Reg Data size = %d", pRegData->size);
        break; 
    }
  }
  
  return (bInitOk); 
  
}


/*****************************************************************************
 * Function:    RegSetUint16
 *
 * Description: Sets the 16 bit Register passed in
 *
 * Parameters:  ptr to register data object to set 
 *
 * Returns:     BOOLEAN bInitOk - TRUE on success
 *                                FALSE on failure
 *
 * Notes:       none
 *
 ****************************************************************************/
static BOOLEAN RegSetUint16 ( REG_SETTING_PTR pRegData )
{
  UINT16 *pDest;
  UINT16 data; 
  BOOLEAN bInitOk; 
  
  pDest = (UINT16 *) pRegData->ptrReg; 
  data = (UINT16) pRegData->value; 
  bInitOk = TRUE; 
  
  switch ( pRegData->setAction ) 
  {
    case REG_SET:
      SET_AND_CHECK( *pDest, data, bInitOk); 
      break; 
      
    case REG_SET_MASK_AND:
      SET_CHECK_MASK_AND( *pDest, data, bInitOk); 
      break;
      
    case REG_SET_MASK_OR:
      SET_CHECK_MASK_OR( *pDest, data, bInitOk); 
      break; 
      
    default: 
       FATAL("Invalid SET_ACTION = %d", pRegData->setAction);
  }
  
  return (bInitOk); 
  
}


/*****************************************************************************
 * Function:    RegSetUint32
 *
 * Description: Sets the 32 bit Register passed in
 *
 * Parameters:  ptr to register data object to set 
 *
 * Returns:     BOOLEAN bInitOk - TRUE on success
 *                                FALSE on failure
 *
 * Notes:       none
 *
 ****************************************************************************/
static BOOLEAN RegSetUint32 ( REG_SETTING_PTR pRegData )
{
  UINT32 *pDest;
  UINT32 data; 
  BOOLEAN bInitOk; 
  
  pDest = (UINT32 *) pRegData->ptrReg; 
  data = (UINT32) pRegData->value; 
  bInitOk = TRUE; 
  
  
  if ( pRegData->bDynamicValue == TRUE )
  {
    // Update the Reg 
    *pDest = (UINT32) (*pDest & (~data)) | *((UINT32 *) pRegData->Addr1); 
    
    // Check the setting 
    CHECK_SETTING( *(UINT32 *) pRegData->Addr1, (UINT32) (*pDest & data), bInitOk); 
  }
  else 
  {  
    switch ( pRegData->setAction ) 
    {
      case REG_SET:
        SET_AND_CHECK( *pDest, data, bInitOk); 
        break; 
        
      case REG_SET_MASK_AND:
        SET_CHECK_MASK_AND( *pDest, data, bInitOk); 
        break;
        
      case REG_SET_MASK_OR:
        SET_CHECK_MASK_OR( *pDest, data, bInitOk); 
        break; 
        
      default: 
       FATAL("Invalid SET_ACTION = %d", pRegData->setAction);
        break; 
    }
  }
  
  return (bInitOk); 
  
}


/*****************************************************************************
 * Function:    RegCheckUint8
 *
 * Description: Checks the 8 bit Register passed against SystemRegTbl[], 
 *              SystemRegTblShadow[] and HW Reg
 *
 * Parameters:  ptr to register data object to check
 *
 * Returns:     bCheckOk - TRUE if OK 
 *
 * Notes:       none
 *
 ****************************************************************************/
static BOOLEAN RegCheckUint8 ( REG_SETTING_PTR pRegData, BOOLEAN bUpdate )
{
  UINT8 *pDest;
  UINT8 val1; 
  UINT8 val2; 
  UINT8 data; 
  BOOLEAN bInitOk; 
  UINT8 regVal; 
  
  bInitOk = TRUE; 
  pDest = (UINT8 *) pRegData->ptrReg; 
  data = (UINT8) pRegData->value; 
  
  
  if ( pRegData->bDynamicValue == TRUE ) 
  {
    // ASSERT if not REG_SET
  
    // Get value from external location and mask 
    val1 = (UINT8) *((UINT32 *) pRegData->Addr1); 
    val2 = (UINT8) *((UINT32 *) pRegData->Addr2); 
    if ( val1 != val2 ) 
    {
      bInitOk = FALSE;
      if (bUpdate == TRUE) 
      {
        // Perform 2 out of 3 and update the 'bad' shadow copy, if selected 
        regVal = *pDest & data;
        if ( val1 == regVal ) 
        {
          *(UINT32 *) pRegData->Addr2 = (UINT8) regVal; 
        }
        else 
        {
          *(UINT32 *) pRegData->Addr1 = (UINT8) regVal; 
        }
      } // End of if bUpdate == TRUE
    }
    else 
    {
      CHECK_SETTING( (UINT8) (*pDest & data), val1, bInitOk ); 
    }
  }
  else 
  {
    switch ( pRegData->setAction ) 
    {
      case REG_SET:
        CHECK_SETTING( *pDest, data, bInitOk); 
        break; 
        
      case REG_SET_MASK_AND:
        CHECK_SETTING_AND_MASK( *pDest, data, bInitOk); 
        break;
        
      case REG_SET_MASK_OR:
        CHECK_SETTING_AND_OR( *pDest, data, bInitOk); 
        break; 
        
      default: 
        FATAL("Invalid SET_ACTION = %d", pRegData->setAction);
        break; 
    }
  }  
  return (bInitOk);  
}


/*****************************************************************************
 * Function:    RegCheckUint16
 *
 * Description: Checks the 16 bit Register passed against SystemRegTbl[], 
 *              SystemRegTblShadow[] and HW Reg
 *
 * Parameters:  ptr to register data object to check
 *
 * Returns:     bCheckOk - TRUE if OK 
 *
 * Notes:       none
 *
 ****************************************************************************/
static BOOLEAN RegCheckUint16 ( REG_SETTING_PTR pRegData, BOOLEAN bUpdate )
{
  UINT16 *pDest;
  UINT16 val1;
  UINT16 val2;
  UINT16 data; 
  BOOLEAN bInitOk; 
  UINT16 regVal; 
  
  bInitOk = TRUE; 
  pDest = (UINT16 *) pRegData->ptrReg; 
  data = (UINT16) pRegData->value; 
  
  
  if ( pRegData->bDynamicValue == TRUE )
  {
    // ASSERT if not REG_SET 
    
    // Get value from external location and mask 
    val1 = (UINT16) *((UINT32 *) pRegData->Addr1); 
    val2 = (UINT16) *((UINT32 *) pRegData->Addr2); 
    if ( val1 != val2 ) 
    {
      bInitOk = FALSE;
      if (bUpdate == TRUE)
      {
        // Perform 2 out of 3 and update the 'bad' shadow copy, if selected 
        regVal = *pDest & data;
        if ( val1 == regVal ) 
        {
          *(UINT32 *) pRegData->Addr2 = (UINT16) regVal; 
        }
        else 
        {
          *(UINT32 *) pRegData->Addr1 = (UINT16) regVal; 
        }
      } // End of if bUpdate == TRUE 
    }
    else 
    {
      CHECK_SETTING( (UINT16) (*pDest & data), val1, bInitOk ); 
    }
  }
  else 
  {
    switch ( pRegData->setAction ) 
    {
      case REG_SET:
        CHECK_SETTING( *pDest, data, bInitOk); 
        break; 
        
/* Not Used.  In future if used, uncomment.  Add ASSERT() as warning
      case REG_SET_MASK_AND:
        CHECK_SETTING_AND_MASK( *pDest, data, bInitOk); 
        break;
*/        
      case REG_SET_MASK_OR:
        CHECK_SETTING_AND_OR( *pDest, data, bInitOk); 
        break; 
        
      default: 
         FATAL("Invalid SET_ACTION = %d", pRegData->setAction);
    }
  }
  
  return (bInitOk); 
  
}


/*****************************************************************************
 * Function:    RegCheckUint32
 *
 * Description: Checks the 32 bit Register passed against SystemRegTbl[], 
 *              SystemRegTblShadow[] and HW Reg
 *
 * Parameters:  ptr to register data object to check
 *
 * Returns:     bCheckOk - TRUE if OK 
 *
 * Notes:       none
 *
 ****************************************************************************/
static BOOLEAN RegCheckUint32 ( REG_SETTING_PTR pRegData, BOOLEAN bUpdate )
{
  UINT32 *pDest;
  UINT32 data;
  UINT32 val1;
  UINT32 val2;
  BOOLEAN bInitOk;
  UINT32 regVal; 
  
  pDest = (UINT32 *) pRegData->ptrReg; 
  data = (UINT32) pRegData->value; 
  bInitOk = TRUE; 
  
  if ( pRegData->bDynamicValue == TRUE ) 
  {
    // ASSERT if not REG_SET
  
    // Get value from external location and mask 
    val1 = (UINT32) *((UINT32 *) pRegData->Addr1); 
    val2 = (UINT32) *((UINT32 *) pRegData->Addr2); 
    if ( val1 != val2 ) 
    {
      bInitOk = FALSE;
      if ( bUpdate == TRUE ) 
      {
        // Perform 2 out of 3 and update the 'bad' shadow copy, if selected
        regVal = *pDest & data;
        if ( val1 == regVal ) 
        {
          *(UINT32 *) pRegData->Addr2 = (UINT32) regVal; 
        }
        else 
        {
          *(UINT32 *) pRegData->Addr1 = (UINT32) regVal; 
        }
      }
    }
    else 
    {
      CHECK_SETTING( (UINT32) (*pDest & data), val1, bInitOk ); 
    }
  }
  else 
  {
    switch ( pRegData->setAction ) 
    {
      case REG_SET:
        CHECK_SETTING( *pDest, data, bInitOk); 
        break; 
        
      case REG_SET_MASK_AND:
        CHECK_SETTING_AND_MASK( *pDest, data, bInitOk); 
        break;
        
/* Not Used.  In future if used, uncomment.  Add ASSERT() as warning        
      case REG_SET_MASK_OR:
        CHECK_SETTING_AND_OR( *pDest, data, bInitOk); 
        break; 
*/        

      default: 
       FATAL("Invalid SET_ACTION = %d", pRegData->setAction); 
        break; 
    }
  }
  
  return (bInitOk); 
  
}


/*****************************************************************************
 * Function:    RegUpdateEntryChksum
 *
 * Description: Updates the Chksum of the passed in Entry
 *
 * Parameters:  index - index into SystemRegTbl[] and SystemRegTblShadow[]
 *
 * Returns:     none
 *
 * Notes:       none
 *
 ****************************************************************************/
 static 
 void RegUpdateEntryChksum ( UINT16 index  )
{
  SystemRegTbl[index].cksum = (UINT16) ChecksumBuffer( &SystemRegTbl[index], 
                                       sizeof(REG_SETTING) - sizeof(UINT16), 0xFFFF );
                                       
  SystemRegTblShadow[index].cksum = (UINT16) ChecksumBuffer( &SystemRegTblShadow[index], 
                                             sizeof(REG_SETTING) - sizeof(UINT16), 0xFFFF );
}


/*****************************************************************************
 * Function:    RegCmpEntries
 *
 * Description: Compares the SystemRegTbl[] with SystemRegTblShadow[] based on 
 *              it's checksum 
 *              Also updates the 'bad' entry if checksum fails
 *              If both entries bad, then return FALSE for 'Can't resolve entry 
 *              data'. 
 *
 * Parameters:  index - index into SystemRegTbl[] and SystemRegTblShadow[]
 *
 * Returns:     TRUE if Entries are OK
 *
 * Notes:       none
 *
 ****************************************************************************/
 static 
 BOOLEAN RegCmpEntries ( UINT16 index  )
{
  BOOLEAN bCksum1Ok, bCksum2Ok; 
  UINT16 cksum1, cksum2; 
  BOOLEAN bOk; 
  UINT8 *pdest;
  UINT8 *psrc; 
  

  bOk = TRUE; 

  // Get and  Validate each buffer's chksum 
  cksum1 = (UINT16) ChecksumBuffer( &SystemRegTbl[index], 
                                    sizeof(REG_SETTING) - sizeof(UINT16), 0xFFFF );
                                    
  cksum2 = (UINT16) ChecksumBuffer( &SystemRegTblShadow[index], 
                                    sizeof(REG_SETTING) - sizeof(UINT16), 0xFFFF );           
                                    
  bCksum1Ok = (cksum1 == SystemRegTbl[index].cksum) ? TRUE : FALSE; 
  bCksum2Ok = (cksum2 == SystemRegTblShadow[index].cksum) ? TRUE : FALSE;  

  if (!(bCksum1Ok && bCksum2Ok))
  {
    // At least one of the cksum is bad 
    pdest = NULL; 
    psrc = NULL; 
    
    bOk = FALSE; 
  
    if ( ( bCksum1Ok == FALSE ) && ( bCksum2Ok == TRUE ) )
    {
      // Copy good one to bad 
      pdest = (UINT8 *) &SystemRegTbl[index]; 
      psrc = (UINT8 *) &SystemRegTblShadow[index]; 
    }
    else if ( ( bCksum1Ok == TRUE ) && ( bCksum2Ok == FALSE ) )
    {
      // Copy good one to bad 
      pdest = (UINT8 *) &SystemRegTblShadow[index]; 
      psrc = (UINT8 *) &SystemRegTbl[index]; 
    }
    
    if ( (pdest != NULL) && (psrc != NULL) )
    {
      // Copy src to dest, the two copies should be synced and good. 
      memcpy ( pdest, psrc, sizeof(REG_SETTING) );
      bOk = TRUE; 
    }
  } // End of if at least one ofthe cksum is bad 
  
  return (bOk); 
  
}


/*****************************************************************************
 *  MODIFICATIONS
 *    $History: UtilRegSetCheck.c $
 * 
 * *****************  Version 17  *****************
 * User: Peter Lee    Date: 11/09/10   Time: 4:41p
 * Updated in $/software/control processor/code/system
 * SCR #991 Code Coverage Issues
 * 
 * *****************  Version 16  *****************
 * User: Peter Lee    Date: 9/28/10    Time: 9:50a
 * Updated in $/software/control processor/code/system
 * SCR #892 Code Review Updates
 * 
 * *****************  Version 15  *****************
 * User: Peter Lee    Date: 7/08/10    Time: 3:05p
 * Updated in $/software/control processor/code/system
 * SCR #683 Fix constant recording of SYS_CBIT_REG_CHECK_FAIL log. 
 * 
 * *****************  Version 14  *****************
 * User: Peter Lee    Date: 6/17/10    Time: 3:21p
 * Updated in $/software/control processor/code/system
 * SCR #634 Update to perform 2 out of 3 check/update for "dynamic" reg
 * val (i.e. dio).  
 * 
 * *****************  Version 13  *****************
 * User: Contractor3  Date: 6/10/10    Time: 10:44a
 * Updated in $/software/control processor/code/system
 * SCR #642 - Changes based on Code Review
 * 
 * *****************  Version 12  *****************
 * User: Peter Lee    Date: 5/31/10    Time: 6:22p
 * Updated in $/software/control processor/code/system
 * SCR #601 FPGA DIO Logic Updates/Bug fixes
 * 
 * *****************  Version 11  *****************
 * User: Jeff Vahue   Date: 4/06/10    Time: 4:01p
 * Updated in $/software/control processor/code/system
 * SCR #527 - init local data
 * 
 * *****************  Version 10  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:59p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 9  *****************
 * User: Jeff Vahue   Date: 2/17/10    Time: 1:24p
 * Updated in $/software/control processor/code/system
 * SCR# 452 - code coverage mods
 * 
 * *****************  Version 8  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:28p
 * Updated in $/software/control processor/code/system
 * SCR 371
 * 
 * *****************  Version 7  *****************
 * User: Jeff Vahue   Date: 12/03/09   Time: 5:00p
 * Updated in $/software/control processor/code/system
 * SCR# 350
 * 
 * *****************  Version 6  *****************
 * User: Peter Lee    Date: 11/20/09   Time: 10:45a
 * Updated in $/software/control processor/code/system
 * Fix bug on Counting number of Reg Check executed per loop.  
 * Added duplicate check before declaring Fail
 * 
 * *****************  Version 5  *****************
 * User: Contractor V&v Date: 11/18/09   Time: 4:23p
 * Updated in $/software/control processor/code/system
 * Implement SCR 172 ASSERT processing for all "default" case processing
 * 
 * *****************  Version 4  *****************
 * User: Peter Lee    Date: 11/05/09   Time: 5:30p
 * Updated in $/software/control processor/code/system
 * SCR #315 DIO CBIT for SEU Processing
 * 
 * *****************  Version 2  *****************
 * User: Peter Lee    Date: 10/26/09   Time: 4:35p
 * Updated in $/software/control processor/code/system
 * SCR #315 Add logic to determine if Reg is checkable before checking
 * register. 
 * 
 * *****************  Version 1  *****************
 * User: Peter Lee    Date: 10/23/09   Time: 9:34a
 * Created in $/software/control processor/code/system
 * Initial Check In.  SCR #315
 * 
 *
 *****************************************************************************/
                                            
