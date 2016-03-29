#define VERSION_BODY
/******************************************************************************
            Copyright (C) 2009-2016 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.
  
  ECCN:        9D991

  File:        Version.c
 
  Description: Product Name and Build Version and Date strings.
 
    VERSION
      $Revision: 46 $  $Date: 3/29/16 8:33a $
    
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    
#include <string.h>

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/

#include "alt_basic.h"
#include "Version.h"
#include "NVMgr.h"
#include "GSE.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define SW_HEADER_MAX_LEN 32 

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
typedef struct
{
  CHAR SwBuildDate[SW_HEADER_MAX_LEN];
  CHAR SwVersion  [SW_HEADER_MAX_LEN];
} VERSION_INFO;

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
const CHAR *PRODUCT_COPYRIGHT = "Pratt & Whitney Engine Services\r\n"
                                "Copyright (c) 2007-2016 \r\n";

const CHAR *PRODUCT_NAME      = "FAST II Software";

const CHAR *PRODUCT_VERSION   = VER_FUNC"."VER_MAJOR"."VER_MINOR DEV_BUILD;  

// Build Date and Time Stamp.
// DATE and TIME are inserted at compile time, but will not affect the Checksum or CRC.
// This string MUST not be longer than 32 characters (31 char + terminator).

#ifndef WIN32
#pragma ghs section text=".buildDate"
#endif

const CHAR DATETIME[SW_HEADER_MAX_LEN] = __DATE__ " " __TIME__;

#ifndef WIN32
#pragma ghs section text=default
#endif



/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
* Function:    Ver_CheckVerAndDateMatch
*
* Description: Check the software build date and versions strings against
*              the ones stored in EEPROM.  If they do not match, the
*              configuration is reset to the default value, including the
*              current software build date and version strings.
*
* Parameters:  none
* Returns:     TRUE: The version and date strings in EE match the current
*                    version of software
*              FALSE:The version and date strings in EE do not match the
*                    current version of software
*
* Notes:
*
******************************************************************************/
BOOLEAN Ver_CheckVerAndDateMatch(void)
{
  VERSION_INFO verInfo;
  BOOLEAN      result = FALSE;
   
  memset(&verInfo,0,sizeof(verInfo));

  if( DRV_OK == NV_Open(NV_VER_INFO) )
  {
    NV_Read(NV_VER_INFO, 0, &verInfo, sizeof(verInfo) );
    // Check for version file in sync with build-info in code.
    if( (strncmp(DATETIME,        verInfo.SwBuildDate, sizeof(verInfo.SwBuildDate) ) == 0) &&
        (strncmp(PRODUCT_VERSION, verInfo.SwVersion,   sizeof(verInfo.SwVersion)   ) == 0) )
    {      
      result = TRUE;
    }
    else
    {
      GSE_DebugStr(NORMAL,TRUE, "Software Version change detected.");
    }
  }
  else
  {
    GSE_DebugStr(NORMAL,TRUE, "%s file open failed.",NV_GetFileName(NV_VER_INFO));
  }

  // If result is false, either the open failed(CRC mismatch) OR the comparison failed.
  // Either way, update RAM with the current build info.
  if (!result)
  {
    // Write back the new version/build-date info to the version info file.
    strncpy_safe(verInfo.SwBuildDate,sizeof(verInfo.SwBuildDate),DATETIME,        _TRUNCATE);
    strncpy_safe(verInfo.SwVersion,  sizeof(verInfo.SwVersion  ),PRODUCT_VERSION, _TRUNCATE);
    NV_Write(NV_VER_INFO,0, &verInfo,sizeof(VERSION_INFO) );
    NV_Close(NV_VER_INFO);
  } 
  return result;
}

/*****************************************************************************/
/* Local Functions                                                           */ 
/*****************************************************************************/

/*************************************************************************
 *  MODIFICATIONS
 *    $History: Version.c $
 * 
 * *****************  Version 46  *****************
 * User: John Omalley Date: 3/29/16    Time: 8:33a
 * Updated in $/software/control processor/code/drivers
 * SCR 1325 - Update the copyright date
 * 
 * *****************  Version 45  *****************
 * User: John Omalley Date: 1/21/15    Time: 1:48p
 * Updated in $/software/control processor/code/drivers
 * 
 * *****************  Version 44  *****************
 * User: John Omalley Date: 12/15/14   Time: 2:30p
 * Updated in $/software/control processor/code/drivers
 * 
 * *****************  Version 43  *****************
 * User: John Omalley Date: 12-10-08   Time: 10:48a
 * Updated in $/software/control processor/code/drivers
 * SCR 1121 - Changed the PRODUCT COPYRIGHT per legal.
 * 
 * *****************  Version 42  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 41  *****************
 * User: Jim Mood     Date: 2/09/12    Time: 2:23p
 * Updated in $/software/control processor/code/drivers
 * SCR:1110 Update UART Driver to toggle each duplex and direction control
 * line at startup init. Update SPI ADC clock to 200kHz
 * 
 * *****************  Version 40  *****************
 * User: Jeff Vahue   Date: 8/04/11    Time: 7:38p
 * Updated in $/software/control processor/code/drivers
 * SCR# 1051 - test version identification
 * 
 * *****************  Version 39  *****************
 * User: Jeff Vahue   Date: 8/04/11    Time: 7:32p
 * Updated in $/software/control processor/code/drivers
 * SCR# 1051 - simplify test versioning
 * 
 * *****************  Version 38  *****************
 * User: Peter Lee    Date: 8/09/10    Time: 6:37p
 * Updated in $/software/control processor/code/drivers
 * Update to v2.0.12 Release
 * 
 * *****************  Version 37  *****************
 * User: Contractor2  Date: 8/09/10    Time: 10:30a
 * Updated in $/software/control processor/code/drivers
 * SCR #775 Misc - Software Build Date should not be included in the
 * Program CRC
 * 
 * *****************  Version 36  *****************
 * User: Peter Lee    Date: 7/29/10    Time: 7:56p
 * Updated in $/software/control processor/code/drivers
 * SCR #698 Code Review Updates 
 * 
 * *****************  Version 35  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:09p
 * Updated in $/software/control processor/code/drivers
 * SCR #317 Implement safe strncpy
 * 
 * *****************  Version 34  *****************
 * User: Contractor3  Date: 3/31/10    Time: 11:48a
 * Updated in $/software/control processor/code/drivers
 * SCR #521 Code Review Issues
 * 
 * *****************  Version 33  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:36p
 * Updated in $/software/control processor/code/drivers
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 * 
 * *****************  Version 32  *****************
 * User: Contractor V&v Date: 3/19/10    Time: 4:30p
 * Updated in $/software/control processor/code/drivers
 * SCR #464 Move Version Info into own file and support it. Remove
 * 
 * *****************  Version 31  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/drivers
 * SCR# 483 - Function Names
 * 
 * *****************  Version 30  *****************
 * User: Contractor V&v Date: 3/10/10    Time: 4:40p
 * Updated in $/software/control processor/code/drivers
 * SCR #464 Move Version Info into own file and support it.
 * 
 * *****************  Version 29  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:22p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 * 
 *
 ***************************************************************************/
                                            
// End Of File
