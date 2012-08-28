#ifndef QAR_MANAGER_H
#define QAR_MANAGER_H

/******************************************************************************
            Copyright (C) 2008-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:         QARManager.h
    
    Description:  Header file for system level QAR manager functions
    
    VERSION
      $Revision: 16 $  $Date: 9/01/10 3:19p $      
    
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "qar.h"

/******************************************************************************
                                 Package Defines
******************************************************************************/
#define QAR_BUF_SIZE 2048;
/******************************************************************************
                                 Package Typedefs
******************************************************************************/
typedef struct
{
    QAR_SF_STATE State;
} QAR_MGR_TASK_PARMS;
/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( QAR_MANAGER_BODY )
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
EXPORT void QARMgr_Task      ( void *pParam );
//EXPORT void QARMgr_CBIT_Task ( void *pParam );
//EXPORT void QARMgr_SensorTestTask ( void *pParam ); 
//EXPORT USER_HANDLER_RESULT QARMgr_UserRegisterMsg(USER_MSG_HANDLER_PARAMS* Params);
//EXPORT USER_HANDLER_RESULT QARMgr_UserCfgMsg(USER_MSG_HANDLER_PARAMS* Params);
//EXPORT USER_HANDLER_RESULT QARMgr_UserStateMsg(USER_MSG_HANDLER_PARAMS* Params);
EXPORT void QARMgr_Initialize (void);
EXPORT void QARMgr_DisableLiveStream(void);


#endif // QAR_MANAGER_H        
/*****************************************************************************
 *  MODIFICATIONS
 *    $History: QARManager.h $
 * 
 * *****************  Version 16  *****************
 * User: Peter Lee    Date: 9/01/10    Time: 3:19p
 * Updated in $/software/control processor/code/system
 * SCR #840 Code Review Updates
 * 
 * *****************  Version 15  *****************
 * User: Contractor3  Date: 7/16/10    Time: 8:57a
 * Updated in $/software/control processor/code/system
 * SCR #702 - Changes based on code review.
 * 
 * *****************  Version 14  *****************
 * User: Contractor V&v Date: 6/22/10    Time: 6:25p
 * Updated in $/software/control processor/code/system
 * SCR #485 Escape Sequence and Box Configuration
 * 
 * *****************  Version 13  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:55p
 * Updated in $/software/control processor/code/system
 * SCR #587 Change TmTaskCreate to return void
 * 
 * *****************  Version 12  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 11  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 5:11p
 * Updated in $/software/control processor/code/system
 * SCR# 397
 * 
 * *****************  Version 10  *****************
 * User: Peter Lee    Date: 11/12/09   Time: 10:12a
 * Updated in $/software/control processor/code/system
 * SCR #315 PWEH SEU Count Processing Support
 * 
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 3:56p
 * Updated in $/control processor/code/system
 * SCR #87 Function Prototype
 * 
 *
 *****************************************************************************/
