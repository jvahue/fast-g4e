#define RESULTCODES_BODY

/******************************************************************************
Copyright (C) 2003-2012 Pratt & Whitney Engine Services, Inc.
All Rights Reserved. Proprietary and Confidential.


 File:        ResultCodes.c

 Description: Provides stringification of the driver result codes.  Calling
              GetResultCodeString with the result of a driver function
              call returns a pointer to the string of that enumeration label.


  VERSION
    $Revision: 17 $  $Date: 11/06/12 10:11a $

******************************************************************************/


/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include <stdio.h>

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "ResultCodes.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/


/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
typedef struct{
  CHAR   Str[RESULTCODES_MAX_STR_LEN];
  RESULT Result;
} RESULT_CODE_DESC;

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
#undef  RESULT_CODE
//Set the result code macro to stringify the enumerated list
#define RESULT_CODE(label,value) {#label,label},
static const RESULT_CODE_DESC CodeDescriptions[] = {RESULT_CODES};
static const CHAR  UndefinedResultCode[] = "Undefined result code";
#undef RESULT_CODE
CHAR resultStr[RESULTCODES_MAX_STR_LEN];

//define a constant for the number of elements in the ResultCodesStrings array
#define NUM_OF_RESULT_CODES (sizeof(CodeDescriptions)/sizeof(RESULT_CODE_DESC))

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/*****************************************************************************
 * Function:    RcGetResultCodeString
 *
 * Description: Returns the stringification of the DRV_RESULT code.  This
 *              is to assist in reporting of low-level driver errors to
 *              the programmer.
 *
 * Parameters:  [in]  Result: DRV_RESULT: Any driver result code
 *              [out] outstr: pointer to a location to store the string
 *
 * Returns:     const CHAR*  The string corresponding to the DRV_RESULT code
 *
 * Notes:       outstr should point to a string RESULTCODES_MAX_STR_LEN
 *              bytes long
 *
 *****************************************************************************/
const CHAR* RcGetResultCodeString(RESULT Result, CHAR* outstr)
{
  UINT32 i;

  sprintf(outstr,"%s : 0x%08X", UndefinedResultCode, Result);

  for(i = 0; i < NUM_OF_RESULT_CODES; i++)
  {
    if(CodeDescriptions[i].Result == Result)
    {
      //Print out string, skipping SYS_, or DRV_
      sprintf (outstr, "%s", (CodeDescriptions[i].Str + RESULT_CODE_PREFIX));
      break;
    }
  }

  return outstr;
}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 *  MODIFICATIONS
 *    $History: ResultCodes.c $
 * 
 * *****************  Version 17  *****************
 * User: Jeff Vahue   Date: 11/06/12   Time: 10:11a
 * Updated in $/software/control processor/code/drivers
 * SCR# 1196
 * 
 * *****************  Version 16  *****************
 * User: Melanie Jutras Date: 12-11-02   Time: 12:48p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 File Format Error
 *
 * *****************  Version 15  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 14  *****************
 * User: Jim Mood     Date: 10/26/10   Time: 5:12p
 * Updated in $/software/control processor/code/drivers
 * SCR 958 Result Codes reentrancy
 *
 * *****************  Version 13  *****************
 * User: John Omalley Date: 10/18/10   Time: 10:22a
 * Updated in $/software/control processor/code/drivers
 * SCR 941 - Printout Number with Undefined ResultCode
 *
 * *****************  Version 12  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:10a
 * Updated in $/software/control processor/code/drivers
 * SCR #698 - Fix code review findings
 *
 * *****************  Version 11  *****************
 * User: Contractor2  Date: 6/14/10    Time: 1:03p
 * Updated in $/software/control processor/code/drivers
 * SCR #483 Function Names must begin with the CSC it belongs with.
 * Also added short circuit to search loop.
 *
 * *****************  Version 10  *****************
 * User: Contractor3  Date: 5/27/10    Time: 10:56a
 * Updated in $/software/control processor/code/drivers
 * SCR #617 - Changes made to meet coding standards as a result of code
 * review.
 *
 * *****************  Version 9  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:22p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 *
 *
 ******************************************************************************/
