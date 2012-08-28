#define DIOMGR_BODY
/******************************************************************************
            Copyright (C) 2007-2010 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File: DIOMgr.c               
    
    Description:  Provides a diagnostic interface to the discrete I/O to the
                  USER.c   Also will provide DIO CBIT functionality in the
                  future
                  

   VERSION
   $Revision: 15 $  $Date: 7/26/11 3:06p $

    
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "DIOMgr.h"
#include "dio.h"
#include "user.h"
#include "Utility.h"
#include "UART.h"

#include "DioMgrUserTables.c"

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
void DIOMgr_UpdateDiscreteInputs(void*);
/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:    DIOMgr_Init
 *  
 * Description: Setup the user tables with USER.  Setup the CBIT test
 *
 * Parameters:  none
 *
 * Returns:     void
 *
 * Notes:
 *
 *****************************************************************************/
void DIOMgr_Init(void)
{ 
  TCB TaskInfo;
  User_AddRootCmd(&DioMgr_RootMsg);

  // Create DIO Read DINs Task
  memset(&TaskInfo, 0, sizeof(TaskInfo));
  strncpy_safe(TaskInfo.Name,sizeof(TaskInfo.Name),"DIO Update DINs",_TRUNCATE);
  TaskInfo.TaskID         = DIO_Update_DINs;
  TaskInfo.Function       = DIOMgr_UpdateDiscreteInputs;
  TaskInfo.Priority       = taskInfo[DIO_Update_DINs].priority;
  TaskInfo.Type           = taskInfo[DIO_Update_DINs].taskType;
  TaskInfo.modes          = taskInfo[DIO_Update_DINs].modes;
  TaskInfo.MIFrames       = taskInfo[DIO_Update_DINs].MIFframes;   
  TaskInfo.Rmt.InitialMif = taskInfo[DIO_Update_DINs].InitialMif;  
  TaskInfo.Rmt.MifRate    = taskInfo[DIO_Update_DINs].MIFrate;    
  TaskInfo.Enabled        = TRUE;
  TaskInfo.Locked         = FALSE;
  TaskInfo.pParamBlock    = NULL;
  TmTaskCreate(&TaskInfo);
}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
* Function:    DIOMgr_UpdateDiscreteInputs
*  
* Description: Calls Dio function to read all Discrete input ports and debounce
*              the value of the individual DIN bits.
*
* Parameters:  None
*
* Returns:     None
*
* Notes:       None
*
*****************************************************************************/
void DIOMgr_UpdateDiscreteInputs (void* pParam)
{
  DIO_UpdateDiscreteInputs();

  // ensure Tx lines are released for Half-duplex Serial ports
  UART_CheckForTXDone();
}



/******************************************************************************
 * Function:    DIOMgr_FSMRunDIO  | IMPLEMENTS Run() INTERFACE to 
 *                                | FAST STATE MGR
 *
 * Description: Signals the DIO Manager to turn on or turn off a selected DIO
 *
 *
 * Parameters:  [in] Run: TRUE: Turn on Discrete Output
 *                        FALSE: Turn off Discrete Output
 *              [in] param: 0-4 is the valid range.  Values outside the range
 *                          are ignored and no action is taken
 *
 * Notes:       Selected Discrete Outs by Param:
 *              Param       Pin
 *              0           LSS0
 *              1           LSS1
 *              2           LSS2
 *              3           LSS3
 *              4           MSRst
 *
 *****************************************************************************/
void DIOMgr_FSMRunDIO(BOOLEAN Run, INT32 param)
{
  const DIO_OUTPUT DIOTable[] = {LSS0,LSS1,LSS2,LSS3,MSRst};
  

  if(param < 4)
  {
    DIO_SetPin(DIOTable[param],Run ? DIO_SetHigh : DIO_SetLow);
  }
  else if(param == 4)
  {
    //MSRst is active low, need to invert logic for the FSM
    DIO_SetPin(DIOTable[param],Run ? DIO_SetLow : DIO_SetHigh );    
  }
}



/******************************************************************************
 * Function:    DIOMgr_FSMGetStateDIO   | IMPLEMENTS GetState() INTERFACE to 
 *                                      | FAST STATE MGR
 *
 * Description: Returns the on/off state of a discrete output.
 *
 *
 * Parameters: [in] param: 0-4 is the valid range.  Values outside the range
 *                          always return FALSE
 *
 * Returns:     BOOLEAN: TRUE:  Output is ON
 *                       FALSE: Output is OFF or index is out of range.
 *
 * Notes:       Selected Discrete Outs by Param:
 *              Param       Pin
 *              0           LSS0
 *              1           LSS1
 *              2           LSS2
 *              3           LSS3
 *              4           MSRst
 *
 *****************************************************************************/
BOOLEAN DIOMgr_FSMGetStateDIO(INT32 param)
{
  const DIO_OUTPUT DIOTable[] = {LSS0,LSS1,LSS2,LSS3,MSRst};
  BOOLEAN retval = FALSE;

  if(param < 4)
  {
    DIO_GetOutputPin(DIOTable[param],&retval);
  }
  else if(param == 4)
  {
    DIO_GetOutputPin(DIOTable[param],&retval);    
    retval = !retval;
  }
  
  return retval;
}



/*************************************************************************
*  MODIFICATIONS
*    $History: DIOMgr.c $
 * 
 * *****************  Version 15  *****************
 * User: Jim Mood     Date: 7/26/11    Time: 3:06p
 * Updated in $/software/control processor/code/system
 * SCR 575 updates
 * 
 * *****************  Version 14  *****************
 * User: Jeff Vahue   Date: 7/26/10    Time: 2:38p
 * Updated in $/software/control processor/code/system
 * SCR# 740 - Move Tx line disable to 10ms DT task that is always enabled
 * 
 * *****************  Version 13  *****************
 * User: Contractor3  Date: 5/27/10    Time: 10:57a
 * Updated in $/software/control processor/code/system
 * SCR #617 - Changes made to meet coding standards as a result of code
 * review.
 * 
 * *****************  Version 12  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:54p
 * Updated in $/software/control processor/code/system
 * SCR #587 Change TmTaskCreate to return void
 * 
 * *****************  Version 11  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:10p
 * Updated in $/software/control processor/code/system
 * SCR #317 Implement safe strncpy
 * 
 * *****************  Version 10  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:57p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 9  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 5:06p
 * Updated in $/software/control processor/code/system
 * SCR# 397
 * 
 * *****************  Version 8  *****************
 * User: Jeff Vahue   Date: 12/29/09   Time: 4:16p
 * Updated in $/software/control processor/code/system
 * SCR# 377
 * 
 * *****************  Version 7  *****************
 * User: Contractor V&v Date: 12/22/09   Time: 2:17p
 * Updated in $/software/control processor/code/system
 * SCR 337 Debounce DIN
* 
*
*
***************************************************************************/
