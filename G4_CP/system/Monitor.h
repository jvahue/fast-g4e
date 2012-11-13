#ifndef MONITOR_H
#define MONITOR_H

/******************************************************************************
            Copyright (C) 2008-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

 
  File:        Monitor.h
 
  Description: GSE Communications port processing defines.
 
  VERSION
      $Revision: 18 $  $Date: 12-11-13 2:19p $    
 
******************************************************************************/


/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    


/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "TaskManager.h"

/******************************************************************************
                                 Package Defines
******************************************************************************/
#define GSE_PROMPT      "\r\nFAST> "

/******************************************************************************
                                 Package Typedefs
******************************************************************************/
typedef struct
{
  CHAR* pCmd; 
  void (*Function)( CHAR* Token[] );
} LDS_CMD;

typedef BOOLEAN (*MSG_HANDLER_CALLBACK)(INT8* MsgStr);

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( MONITOR_BODY )
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
EXPORT void       MonitorInitialize  ( void);
EXPORT TASK_INDEX MonitorGetTaskId   ( void);
EXPORT void       MonitorPrintVersion( void);
EXPORT void       MonitorSetMsgCallback(MSG_HANDLER_CALLBACK MsgHandler);


#endif // MONITOR_H
/*************************************************************************
 *  MODIFICATIONS
 *    $History: Monitor.h $
 * 
 * *****************  Version 18  *****************
 * User: Melanie Jutras Date: 12-11-13   Time: 2:19p
 * Updated in $/software/control processor/code/system
 * SCR #1142 File Format Errors
 * 
 * *****************  Version 17  *****************
 * User: John Omalley Date: 12-11-12   Time: 4:46p
 * Updated in $/software/control processor/code/system
 * SCR 1142 - Formatting Error
 * 
 * *****************  Version 16  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 15  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:10a
 * Updated in $/software/control processor/code/system
 * SCR #698 - Fix code review findings
 * 
 * *****************  Version 14  *****************
 * User: Contractor3  Date: 7/06/10    Time: 10:38a
 * Updated in $/software/control processor/code/system
 * SCR #672 - Changes based on Code Review.
 * 
 * *****************  Version 13  *****************
 * User: Jeff Vahue   Date: 4/07/10    Time: 12:09p
 * Updated in $/software/control processor/code/system
 * SCR #534 - Cleanup Program CRC messages
 * 
 * *****************  Version 12  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/system
 * SCR# 483 - Function Names
 * 
 * *****************  Version 11  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 10  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 5:09p
 * Updated in $/software/control processor/code/system
 * SCR# 397
 * 
 * *****************  Version 9  *****************
 * User: Jim Mood     Date: 10/07/08   Time: 5:26p
 * Updated in $/control processor/code/system
 * Updated comment style.  Removed the Debug verbosity enum from this
 * header, because it has been declared in FaultMgr.h
 * 
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 3:40p
 * Updated in $/control processor/code/system
 * SCR #87 Function Prototype
 * 
 *
 ***************************************************************************/ 

