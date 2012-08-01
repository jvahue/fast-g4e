#define MAIN_BODY
/******************************************************************************
            Copyright (C) 2007-2010 Pratt & Whitney Engine Services, Inc. 
                      Altair Engine Diagnostic Solutions
               All Rights Reserved. Proprietary and Confidential.

    File:        Main.c
    
    Description: Entry point for C runtime code.
                 
    VERSION
       $Revision: 17 $  $Date: 10/04/10 3:41p $    
    
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "InitializationManager.h"

#ifndef WIN32
#include "ProcessorInit.h"
#endif

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

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

#ifndef WIN32
/******************************************************************************
 * Function:    main.c
 * Description: Initialize system and enter Idle Loop
 * Parameters:  Unused
 * Returns:     None
 * Notes:
 *
 *****************************************************************************/
int main(int argc, char *argv[])
{
    disable_cache();
    init_cache();

    Im_InitializeControlProcessor();

    while(TRUE)
    {
    }
}

#else
/*vcast_dont_instrument_start*/
#include "HwSupport.h"
unsigned int _cdecl RunMain( void* pParam)
{
    WinG4E(pParam);
    return 0;
}
/*vcast_dont_instrument_end*/
#endif

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/


/*************************************************************************
 *  MODIFICATIONS
 *    $History: main.c $
 * 
 * *****************  Version 17  *****************
 * User: Contractor2  Date: 10/04/10   Time: 3:41p
 * Updated in $/software/control processor/code/system
 * SCR #916 Code Review Updates
 * 
 * *****************  Version 16  *****************
 * User: Jeff Vahue   Date: 7/26/10    Time: 2:38p
 * Updated in $/software/control processor/code/system
 * SCR# 740 - Move Tx line disable to 10ms DT task that is always enabled
 * 
 * *****************  Version 15  *****************
 * User: Contractor2  Date: 6/14/10    Time: 4:12p
 * Updated in $/software/control processor/code/system
 * SCR #645 Periodically flush instruction/branch caches
 * Moved cache control asm code to ProcessorInit.s
 * 
 * *****************  Version 14  *****************
 * User: Contractor2  Date: 6/14/10    Time: 1:10p
 * Updated in $/software/control processor/code/system
 * SCR #483 Function Names must begin with the CSC it belongs with.
 * 
 * *****************  Version 13  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 11:01a
 * Updated in $/software/control processor/code/system
 * SCR# 481 - Inhibit VectorCast Coverage
 * 
 * *****************  Version 12  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 11  *****************
 * User: Jeff Vahue   Date: 1/21/10    Time: 12:59p
 * Updated in $/software/control processor/code/system
 * SCR# 369
 * 
 * *****************  Version 10  *****************
 * User: Contractor V&v Date: 11/30/09   Time: 5:40p
 * Updated in $/software/control processor/code/system
 * Added comment line to toggle cache enable setting
 * 
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 11/02/09   Time: 1:57p
 * Updated in $/software/control processor/code/system
 * Misc non code update to support WIN32 version and spelling. 
 * 
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 6/30/09    Time: 3:04p
 * Updated in $/software/control processor/code/system
 * SCR #204 Misc Code Review Updates
 * 
 ***************************************************************************/


