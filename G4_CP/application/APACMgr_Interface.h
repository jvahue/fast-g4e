#ifndef APAC_MGR_INTERFACE_H
#define APAC_MGR_INTERFACE_H
/******************************************************************************
            Copyright (C) 2015 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:        APACMgr_Interface.h

    Description: Contains data structures related to the APACMgr Interface function

    VERSION
      $Revision: 3 $  $Date: 2/02/16 9:43a $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/


/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"


/******************************************************************************
                                 Package Defines
******************************************************************************/


/******************************************************************************
                                 Package Typedefs
******************************************************************************/
typedef enum {
  APAC_IF_RJCT,   // Ref APACMgr_IF_ResultCommit()
  APAC_IF_ACPT,   // Ref APACMgr_IF_ResultCommit()
  APAC_IF_INVLD,  // Ref APACMgr_IF_ResultCommit()
  APAC_IF_VLD,    // Ref APACMgr_IF_ResultCommit()
  APAC_IF_DATE,   // Ref APACMgr_IF_HistoryData()
  APAC_IF_DAY,    // Ref APACMgr_IF_HistoryData()
  APAC_IF_DCLK,   // Ref APACMgr_IF_Reset()
  APAC_IF_TIMEOUT,// Ref APACMgr_IF_Reset()
  APAC_IF_MAX     // 
} APAC_IF_ENUM; 


/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( APAC_MGR_BODY )
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
EXPORT BOOLEAN APACMgr_IF_Config ( CHAR *msg1, CHAR *msg2, CHAR *msg3, CHAR *ms4, 
                                   APAC_IF_ENUM option );
EXPORT BOOLEAN APACMgr_IF_Setup ( CHAR *msg1, CHAR *msg2, CHAR *msg3, CHAR *ms4, 
                                  APAC_IF_ENUM option  ); 
EXPORT BOOLEAN APACMgr_IF_ErrMsg ( CHAR *msg1, CHAR *msg2, CHAR *msg3, CHAR *ms4, 
                                   APAC_IF_ENUM option  ); 
EXPORT BOOLEAN APACMgr_IF_ValidateManual ( CHAR *msg1, CHAR *msg2, CHAR *msg3, CHAR *ms4, 
                                           APAC_IF_ENUM option  );
EXPORT BOOLEAN APACMgr_IF_RunningPAC ( CHAR *msg1, CHAR *msg2, CHAR *msg3, CHAR *ms4, 
                                       APAC_IF_ENUM option  ); 
EXPORT BOOLEAN APACMgr_IF_CompletedPAC ( CHAR *msg1, CHAR *msg2, CHAR *msg3, CHAR *ms4, 
                                         APAC_IF_ENUM option  ); 
EXPORT BOOLEAN APACMgr_IF_Result ( CHAR *msg1, CHAR *msg2, CHAR *msg3, CHAR *ms4, 
                                   APAC_IF_ENUM option  ); 
EXPORT BOOLEAN APACMgr_IF_ResultCommit ( CHAR *msg1, CHAR *msg2, CHAR *msg3, CHAR *ms4, 
                                         APAC_IF_ENUM option  ); 
EXPORT BOOLEAN APACMgr_IF_HistoryData ( CHAR *msg1, CHAR *msg2, CHAR *msg3, CHAR *ms4, 
                                        APAC_IF_ENUM option  );
EXPORT BOOLEAN APACMgr_IF_HistoryAdv ( CHAR *msg1, CHAR *msg2, CHAR *msg3, CHAR *ms4, 
                                       APAC_IF_ENUM option  );
EXPORT BOOLEAN APACMgr_IF_HistoryDec ( CHAR *msg1, CHAR *msg2, CHAR *msg3, CHAR *msg4, 
                                       APAC_IF_ENUM option  );
EXPORT BOOLEAN APACMgr_IF_HistoryReset ( CHAR *msg1, CHAR *msg2, CHAR *msg3, CHAR *msg4, 
                                         APAC_IF_ENUM option  );
EXPORT BOOLEAN APACMgr_IF_Reset ( CHAR *msg1, CHAR *msg2, CHAR *msg3, CHAR *ms4, 
                                  APAC_IF_ENUM option  ); 

EXPORT void APACMgr_IF_InvldDelayTimeOutVal ( UINT32 *timeoutVal_ptr );

#endif // APAC_MGR_INTERFACE_H


/*****************************************************************************
 *  MODIFICATIONS
 *    $History: APACMgr_Interface.h $
 * 
 * *****************  Version 3  *****************
 * User: Peter Lee    Date: 2/02/16    Time: 9:43a
 * Updated in $/software/control processor/code/application
 * SCR #1308 Item #19 and #20.  Check in Cycle CBIT again.  ReOrder
 * History back to most recent when RCL selected.  
 * Add ifdef of Debug Code. 
 * 
 * *****************  Version 2  *****************
 * User: Peter Lee    Date: 2/01/16    Time: 2:04p
 * Updated in $/software/control processor/code/application
 * SCR #1308 Item #16.  Additional updates.  Remove unused func
 * APACMgr_IF_Select()
 * 
 * *****************  Version 1  *****************
 * User: Peter Lee    Date: 15-10-14   Time: 9:19a
 * Created in $/software/control processor/code/application
 * SCR #1304 APAC Processing Initial Check In
 *
 *****************************************************************************/

