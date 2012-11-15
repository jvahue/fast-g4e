#define SYSTEMLOG_BODY
/******************************************************************************
            Copyright (C) 2008-2012 Pratt & Whitney Engine Services, Inc. 
                  All Rights Reserved. Proprietary and Confidential.

    File:        SystemLog.c 
    
    Description: 

   VERSION
      $Revision: 10 $  $Date: 12-11-14 7:20p $    
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    
#include <string.h>

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "systemlog.h"
#include "monitor.h"
#include "logmanager.h"
#include "ClockMgr.h"
#include "Utility.h"
#include "Assert.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#undef  SYS_LOG_ID
//Set the system code macro to stringify the enumerated list and include log count
#define SYS_LOG_ID(label,value,count) {label,count,#label}, 
//define a constant for the number of elements in the SystemLogStrings array
#define NUM_OF_LOG_IDS (sizeof(systemLogStrings)/sizeof(systemLogStrings[0]))

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
typedef struct{
  SYS_APP_ID id;
  UINT16     cnt;
  CHAR*      str;
}SYS_LOG_ID_STRS;

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static const SYS_LOG_ID_STRS systemLogStrings[] = {SYS_LOG_IDS};
static const CHAR  systemLogUndefindedID[] = "Undefined System Log ID";
static UINT16 logWriteCounts[NUM_OF_LOG_IDS];


/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/*****************************************************************************
 * Function:    SystemLogIDString
 *
 * Description: Returns the stringification of the SYS_APP_ID log code.  This
 *              is to assist in reporting of LOG IDs to the user
 *
 * Parameters:  [in] LogID: Any SYS_APP_ID log ID
 *              
 * Returns:     CHAR* Stringification of the Log ID passed in the parameter.
 *                    If the ID does not exist in the SYS_APP_ID enum, a string
 *                    indicating the ID is undefined is returned.
 *
 * Notes:       
 *              
 ****************************************************************************/
const CHAR* SystemLogIDString(SYS_APP_ID LogID)
{
  const CHAR* result = systemLogUndefindedID;
  UINT32 i;

  for(i = 0; i < NUM_OF_LOG_IDS; i++)
  {
    if(systemLogStrings[i].id == LogID)
    {
      //String found, return string pointer
      result = systemLogStrings[i].str; 
      break;
    }
  }
  
  return result;
}

 
/*****************************************************************************
 * Function:    SystemLogInitLimitCheck
 *
 * Description: Init system log write limit check counts
 *
 * Parameters:  None
 *              
 * Returns:     None
 *
 * Notes:       
 *              
 ****************************************************************************/
void SystemLogInitLimitCheck(void)
{
  memset(logWriteCounts, 0, sizeof(logWriteCounts));
}

/*****************************************************************************
 * Function:    SystemLogResetLimitCheck
 *
 * Description: Resets write limit count of the specified log id
 *
 * Parameters:  [in] LogID: Any SYS_APP_ID log ID
 *              
 * Returns:     None
 *
 * Notes:       
 *              
 ****************************************************************************/
void SystemLogResetLimitCheck(SYS_APP_ID LogID)
{
  BOOLEAN bFound = FALSE;
  UINT32 i;

  const SYS_LOG_ID_STRS *pSysLogStr = &systemLogStrings[0];

  for(i = 0; !bFound && (i < NUM_OF_LOG_IDS); ++i, ++pSysLogStr)
  {
    if(pSysLogStr->id == LogID)
    {
      //ID found - resest count
      bFound = TRUE;
      logWriteCounts[i] = 0;
    }
  }
}

/*****************************************************************************
 * Function:    SystemLogLimitCheck
 *
 * Description: Checks if the specified log id has met its output limit
 *
 * Parameters:  [in] LogID: Any SYS_APP_ID log ID
 *              
 * Returns:     TRUE if limit not met
 *              FALSE if limit not met
 *
 * Notes:       Each call will increment the limit check count
 *              
 ****************************************************************************/
BOOLEAN SystemLogLimitCheck(SYS_APP_ID LogID)
{
  BOOLEAN result = FALSE;
  BOOLEAN bFound = FALSE;
  UINT32 i;

  const SYS_LOG_ID_STRS *pSysLogStr = &systemLogStrings[0];

  for(i = 0; !bFound && (i < NUM_OF_LOG_IDS); ++i, ++pSysLogStr)
  {
    if(pSysLogStr->id == LogID)
    {
      //ID found - check count
      bFound = TRUE;
      if (pSysLogStr->cnt == 0)
      {
        // count limit is 0 - always write
        result = TRUE;
      }
      else      // check if write limit hit
      if (logWriteCounts[i] < pSysLogStr->cnt)
      {
        result = TRUE;          // allow write
        ++logWriteCounts[i];    // inc write count
      }
    }
  }
  
  return result;
}

 
/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/



/*************************************************************************
 *  MODIFICATIONS
 *    $History: SystemLog.c $
 * 
 * *****************  Version 10  *****************
 * User: John Omalley Date: 12-11-14   Time: 7:20p
 * Updated in $/software/control processor/code/system
 * SCR 1076 - Code Review Updates
 * 
 * *****************  Version 9  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 8  *****************
 * User: Contractor2  Date: 6/01/11    Time: 1:45p
 * Updated in $/software/control processor/code/system
 * SCR #473 Enhancement: Restriction of System Logs
 * 
 * *****************  Version 7  *****************
 * User: Contractor3  Date: 6/10/10    Time: 10:44a
 * Updated in $/software/control processor/code/system
 * SCR #642 - Changes based on Code Review
 * 
 * *****************  Version 6  *****************
 * User: Jeff Vahue   Date: 12/17/09   Time: 12:13p
 * Updated in $/software/control processor/code/system
 * SCR# 364 INT8 -> CHAR
 * 
 * *****************  Version 5  *****************
 * User: Jeff Vahue   Date: 12/07/09   Time: 4:26p
 * Updated in $/software/control processor/code/system
 * SCR# 364 - assert on invalid logID
 * 
 * *****************  Version 4  *****************
 * User: John Omalley Date: 8/06/09    Time: 3:58p
 * Updated in $/software/control processor/code/system
 * SCR 179 
 * Removed Unused variables.
 * 
 * *****************  Version 3  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 4:09p
 * Updated in $/control processor/code/system
 * SCR #87 Function Prototype
 * 
 * *****************  Version 2  *****************
 * User: Jim Mood     Date: 10/02/08   Time: 9:51a
 * Updated in $/control processor/code/system
 * Moved SysLogWrite function to Log Manager.  Provided stringify macros
 * for each log id.
 * 
 * *****************  Version 1  *****************
 * User: John Omalley Date: 6/18/08    Time: 2:48p
 * Created in $/control processor/code/system
 * SCR-0044 System Log Addition
 * 
 * 
 * 
 *
 *
 ***************************************************************************/
