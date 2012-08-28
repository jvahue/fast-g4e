#define DIOMGR_USERTABLE_BODY
/******************************************************************************
Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc. 
All Rights Reserved. Proprietary and Confidential.

File:        DioMgrUserTables.c

Description: The data manager contains the functions used to record
data from the various interfaces.


VERSION
$Revision: 9 $  $Date: 10/18/10 11:59a $ 

******************************************************************************/

#ifndef DIOMGR_BODY
#error DioMgrUserTables.c should only be included by DioMgr.c
#endif

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    
#include <stdlib.h>
#include <string.h>

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Local Defines                                                             */ 
/*****************************************************************************/

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
//Input and output enumeration tables, used for reading and writing.  Third
//element (toggle) is used only for writing
USER_ENUM_TBL HighLow[] = { {"LO",DIO_SetLow},
                            {"HI",DIO_SetHigh},
                            {NULL,0} };

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/

//Prototype for the User Manager message handlers, has to go before
//the local variable tables that use the function pointer.
USER_HANDLER_RESULT DIOMgr_OutputPin(USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr);

USER_HANDLER_RESULT DIOMgr_InputPin(USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr);


//Define DIO_PIN list macro to generate USER_MSG_TBL entries for input pins
//with the pin name and enumeration
#undef DIO_PIN
#define DIO_PIN(Name,Dir,State,Periph,AccMethod,Addr,Bm) {#Name,NO_NEXT_TABLE,\
DIOMgr_InputPin,USER_TYPE_ENUM,USER_RO,(void*)Name,-1,-1,NO_LIMIT,HighLow},

static USER_MSG_TBL DIOInTbl[]  =
{
  DIO_INPUTS_LIST
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

//Define DIO_PIN list macro to generate USER_MSG_TBL entries for output pins
//with the pin name and enumeration
#undef DIO_PIN
#define DIO_PIN(Name,Dir,State,Periph,AccMethod,Addr,Bm) {#Name,NO_NEXT_TABLE,\
DIOMgr_OutputPin,USER_TYPE_ENUM,(USER_RW|USER_GSE),(void*)Name,-1,-1,NO_LIMIT,HighLow},

static USER_MSG_TBL DIOOutTbl[] = 
{
  DIO_OUTPUTS_LIST
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL DIOCmd [] =
{
  { "IN"   , DIOInTbl,  NULL, NO_HANDLER_DATA },
  { "OUT"  , DIOOutTbl, NULL, NO_HANDLER_DATA },
  { NULL   ,     NULL,  NULL, NO_HANDLER_DATA}
};


static USER_MSG_TBL DioMgr_RootMsg = {"DIO",DIOCmd,NULL,NO_HANDLER_DATA};

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
* Function:    DIOMgr_OutputPin
*  
* Description: Handles callbacks from the User.c component for all output
*              pins in the discrete I/O List
*
* Parameters:  DataType - the user data type
*              Param    - the user msg param
*              Index    - the msg index
*              *SetPtr  - NULL, low, high or toggle
*              **GetPtr - Pin State
*
* Returns:     USER_HANDLER_RESULT
*
* Notes:
*
*****************************************************************************/
USER_HANDLER_RESULT DIOMgr_OutputPin(USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr)
{
  BOOLEAN PinState;
  if(SetPtr == NULL)
  {
    DIO_GetOutputPin((DIO_OUTPUT) Param.Int, &PinState);
    **(UINT32**)GetPtr = PinState;
  }
  else
  {
    DIO_SetPin( (DIO_OUTPUT) Param.Int,*(DIO_OUT_OP*)SetPtr);
  }  
  return USER_RESULT_OK;
}                                       



/******************************************************************************
 * Function:    DIOMgr_InputPin
 *  
 * Description: Handles callbacks from the User.c component for all input
 *              pins in the discrete I/O List
 *
 * Parameters:  DataType - the user data type
 *              Param    - the user msg param
 *              Index    - the msg index
 *              *SetPtr  - NULL, low, high or toggle
 *              **GetPtr - Pin State
 *
 * Returns:     USER_HANDLER_RESULT
 *
 * Notes:
 *
 *****************************************************************************/
USER_HANDLER_RESULT DIOMgr_InputPin(USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr)
{
  BOOLEAN PinState = DIO_ReadPin( (DIO_INPUT) Param.Int);

  //Must convert BOOLEAN to UINT32 type to return to User.c
  **(UINT32**)GetPtr = PinState;
  
  return USER_RESULT_OK;
}

/*****************************************************************************
*  MODIFICATIONS
*    $History: DioMgrUserTables.c $
 * 
 * *****************  Version 9  *****************
 * User: Jim Mood     Date: 10/18/10   Time: 11:59a
 * Updated in $/software/control processor/code/system
 * SCR 943
 * 
 * *****************  Version 8  *****************
 * User: Jeff Vahue   Date: 7/29/10    Time: 4:05p
 * Updated in $/software/control processor/code/system
 * SCR# 685 - cleanup
 * 
 * *****************  Version 7  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:01a
 * Updated in $/software/control processor/code/system
 * SCR #685 - Add USER_GSE flag
 * 
 * *****************  Version 6  *****************
 * User: Contractor3  Date: 5/27/10    Time: 10:57a
 * Updated in $/software/control processor/code/system
 * SCR #617 - Changes made to meet coding standards as a result of code
 * review.
 * 
 * *****************  Version 5  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:57p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 4  *****************
 * User: Jeff Vahue   Date: 2/26/10    Time: 3:00p
 * Updated in $/software/control processor/code/system
 * SCR# 469 - change DIO_ReadPin to return the pin state
 * 
 * *****************  Version 3  *****************
 * User: Contractor V&v Date: 1/27/10    Time: 5:23p
 * Updated in $/software/control processor/code/system
 *  SCR 340 DIO_OUT GSE command should be readable
*
*
*****************************************************************************/
