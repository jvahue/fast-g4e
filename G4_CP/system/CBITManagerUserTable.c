#define CBITMANAGER_USERTABLE_BODY
/******************************************************************************
            Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:         CBITManagerUserTable.c
    
    Description:  CBIT (Continous Built In Test) Mananger GSE commands
    
    VERSION
      $Revision: 13 $  $Date: 12-11-15 2:59p $
    
******************************************************************************/
#ifndef CBIT_MANAGER_BODY
#error CBITManagerUserTable.c should only be included by CBITManager.c
#endif

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    
#include <stdlib.h>
#include <string.h>


/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "CBITManager.h"
#include "stddefs.h"
#include "user.h"

/*****************************************************************************/
/* Local Defines                                                             */  
/*****************************************************************************/

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
// See Local Variables below Function Prototypes Section

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
//Prototypes for the User Manager message handlers, has to go before
//the local variable tables that use the function pointer.

#ifdef GENERATE_SYS_LOGS 
USER_HANDLER_RESULT CbitMgrMsg_CreateLogs(USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr);
#endif                                        


/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/

USER_MSG_TBL CbitStatusTbl[] = 
{
  {"REGISTERS_TO_CHECK",NO_NEXT_TABLE,User_GenericAccessor, 
          USER_TYPE_UINT16, USER_RO, (void *) &m_CbitMgrStatus.numReg, 
          -1,-1,NO_LIMIT,NULL},

  {"RAM_FAIL_CNT",NO_NEXT_TABLE,User_GenericAccessor, 
          USER_TYPE_UINT32, USER_RO, (void *) &m_CbitMgrStatus.RAMFailCnt, 
          -1,-1,NO_LIMIT,NULL},
          
  {"PROGRAM_CRC_FAIL_CNT",NO_NEXT_TABLE,User_GenericAccessor, 
          USER_TYPE_UINT32, USER_RO, (void *) &m_CbitMgrStatus.ProgramCRCErrCnt, 
          -1,-1,NO_LIMIT,NULL},
          
  {"REG_CHECK_FAIL_CNT",NO_NEXT_TABLE,User_GenericAccessor, 
          USER_TYPE_UINT32, USER_RO, (void *) &m_CbitMgrStatus.RegCheckCnt, 
          -1,-1,NO_LIMIT,NULL},
          
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};


USER_MSG_TBL CbitMgrRoot[] = 
{
   {"STATUS",CbitStatusTbl,NULL,NO_HANDLER_DATA},
   
#ifdef GENERATE_SYS_LOGS  
    {"CREATELOGS",NO_NEXT_TABLE,CbitMgrMsg_CreateLogs,
    USER_TYPE_ACTION,USER_RO,NULL,-1,-1,NULL},
#endif  

   {NULL,NULL,NULL,NO_HANDLER_DATA}
}; 


USER_MSG_TBL CbitMgrRootTblPtr = {"CBIT",CbitMgrRoot,NULL,NO_HANDLER_DATA};



/*****************************************************************************/
/* Public Functions                                                          */ 
/*****************************************************************************/

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:    CbitMgrMsg_CreateLogs (cbit.createlogs)
 *
 * Description: Create all the internal cbit mgr system logs
 *              
 * Parameters:  USER_DATA_TYPE DataType
 *              USER_MSG_PARAM Param
 *              UINT32 Index
 *              const void *SetPtr
 *              void **GetPtr
 *
 * Returns:     USER_RESULT_OK:    Processed successfully
 *              USER_RESULT_ERROR: Error processing command. 
 *
 * Notes:       none
 *
 *****************************************************************************/
#ifdef GENERATE_SYS_LOGS

/*vcast_dont_instrument_start*/
USER_HANDLER_RESULT CbitMgrMsg_CreateLogs(USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_OK; 

  CBIT_CreateAllInternalLogs(); 
  
  return result;
}
/*vcast_dont_instrument_end*/

#endif


/*****************************************************************************
 *  MODIFICATIONS
 *    $History: CBITManagerUserTable.c $
 * 
 * *****************  Version 13  *****************
 * User: Melanie Jutras Date: 12-11-15   Time: 2:59p
 * Updated in $/software/control processor/code/system
 * SCR #1142 File Format Errors
 * 
 * *****************  Version 12  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 11  *****************
 * User: Peter Lee    Date: 9/28/10    Time: 3:55p
 * Updated in $/software/control processor/code/system
 * SCR #892 Code Review Updates
 * 
 * *****************  Version 10  *****************
 * User: Jeff Vahue   Date: 8/28/10    Time: 12:22p
 * Updated in $/software/control processor/code/system
 * SCR# 830 - GENERATE_SYS_LOGS exclude area from coverage results
 * 
 * *****************  Version 9  *****************
 * User: Contractor3  Date: 6/25/10    Time: 9:46a
 * Updated in $/software/control processor/code/system
 * SCR #662 - Changes based on code review
 * 
 * *****************  Version 8  *****************
 * User: Contractor2  Date: 5/06/10    Time: 2:10p
 * Updated in $/software/control processor/code/system
 * SCR #579 Change LogWriteSys to return void
 * 
 * *****************  Version 7  *****************
 * User: Jeff Vahue   Date: 4/12/10    Time: 5:43p
 * Updated in $/software/control processor/code/system
 * SCR #541 - all XXX_CreateLogs functions are now dependent on
 * GENERATE_SYS_LOGS
 * 
 * *****************  Version 6  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 3:56p
 * Updated in $/software/control processor/code/system
 * SCR 106 Fix findings for User_GenericAccessor
 * 
 * *****************  Version 5  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:57p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 4  *****************
 * User: Contractor V&v Date: 1/13/10    Time: 4:58p
 * Updated in $/software/control processor/code/system
 * 
 * *****************  Version 3  *****************
 * User: Jeff Vahue   Date: 12/04/09   Time: 4:32p
 * Updated in $/software/control processor/code/system
 * SCR# 350
 * 
 * *****************  Version 2  *****************
 * User: Contractor V&v Date: 11/30/09   Time: 5:36p
 * Updated in $/software/control processor/code/system
 * SCR 106 XXXUserTable.c consistency
 * 
 * *****************  Version 1  *****************
 * User: Peter Lee    Date: 10/30/09   Time: 3:48p
 * Created in $/software/control processor/code/system
 * Initial Check In 
 * 
 *
 *****************************************************************************/
                                            
