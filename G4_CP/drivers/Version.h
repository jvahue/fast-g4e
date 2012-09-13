#ifndef VERSION_H
#define VERSION_H
/******************************************************************************
            Copyright (C) 2007-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.
 
  File:        Version.h
 
  Description: Build related external references.
 
    VERSION
      $Revision: 63 $  $Date: 8/29/12 2:56p $
    
******************************************************************************/
/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/

/******************************************************************************
                                 Package Defines
******************************************************************************/
#define VER_FUNC    "2"       // FN - Functional Build
#define VER_MAJOR   "0"       // MJ - Major Functional Change
#define VER_MINOR   "0"       // MN - Minor Functional Change
#define DEV_BUILD   " Dev 148dfcd2a115+"  // ER - Development Build = "" for release

/******************************************************************************
                                 Package Typedefs
******************************************************************************/

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined( VERSION_BODY )
  #define EXPORT
#else
  #define EXPORT extern
#endif

/******************************************************************************
                             Package Exports Variables
******************************************************************************/
extern const CHAR *PRODUCT_COPYRIGHT;
extern const CHAR *PRODUCT_NAME;

extern const CHAR *PRODUCT_VERSION;


// Build Date and Time Stamp
extern const CHAR DATETIME[];

/******************************************************************************
                             Package Exports Functions
******************************************************************************/
EXPORT BOOLEAN Ver_CheckVerAndDateMatch(void);



#endif // VERSION_H
/*************************************************************************
 *  MODIFICATIONS
 *    $History: Version.h $
 * 
 * *****************  Version 63  *****************
 * User: Contractor V&v Date: 8/29/12    Time: 2:56p
 * Updated in $/software/control processor/code/drivers
 * SCR #1107 FAST 2 Updated to reflect build node #
 * 
 * *****************  Version 62  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 61  *****************
 * User: Contractor V&v Date: 7/18/12    Time: 6:26p
 * Updated in $/software/control processor/code/drivers
 * SCR #1107 updating to Dev 3
 * 
 * *****************  Version 60  *****************
 * User: Contractor V&v Date: 7/11/12    Time: 4:30p
 * Updated in $/software/control processor/code/drivers
 * 
 * *****************  Version 59  *****************
 * User: Contractor V&v Date: 6/25/12    Time: 2:30p
 * Updated in $/software/control processor/code/drivers
 * SCR #1107 Fix Dev build tag string
 * 
 * *****************  Version 58  *****************
 * User: Contractor V&v Date: 6/07/12    Time: 7:13p
 * Updated in $/software/control processor/code/drivers
 * Modify version string to DEV 1
 * 
 * *****************  Version 57  *****************
 * User: Jim Mood     Date: 2/09/12    Time: 11:20a
 * Updated in $/software/control processor/code/drivers
 * SCR:1110 Update UART Driver to toggle each duplex and direction control
 * line at startup init. Update SPI ADC clock to 200kHz
 * 
 * *****************  Version 56  *****************
 * User: Contractor V&v Date: 10/21/11   Time: 5:05p
 * Updated in $/software/control processor/code/drivers
 * 
 * *****************  Version 55  *****************
 * User: Jeff Vahue   Date: 10/17/11   Time: 7:46p
 * Updated in $/software/control processor/code/drivers
 * SCR# 1077: Dev 28
 * 
 * *****************  Version 54  *****************
 * User: Contractor V&v Date: 10/12/11   Time: 7:23p
 * Updated in $/software/control processor/code/drivers
 * Dev 27
 * 
 * *****************  Version 53  *****************
 * User: Contractor V&v Date: 10/11/11   Time: 6:24p
 * Updated in $/software/control processor/code/drivers
 * Dev 26
 * 
 * *****************  Version 52  *****************
 * User: Jeff Vahue   Date: 10/10/11   Time: 6:55p
 * Updated in $/software/control processor/code/drivers
 * SCR# 1063 - v1.1.0 Dev 25
 * 
 * *****************  Version 51  *****************
 * User: Jeff Vahue   Date: 10/06/11   Time: 5:56p
 * Updated in $/software/control processor/code/drivers
 * SCR# 1063 - v1.1.0 Dev 24
 * 
 * *****************  Version 50  *****************
 * User: Jeff Vahue   Date: 10/05/11   Time: 9:07p
 * Updated in $/software/control processor/code/drivers
 * v1.1.0 Dev 23
 * 
 * *****************  Version 49  *****************
 * User: Jeff Vahue   Date: 10/04/11   Time: 8:46p
 * Updated in $/software/control processor/code/drivers
 * v1.1.0 Dev 22
 * 
 * *****************  Version 48  *****************
 * User: Jeff Vahue   Date: 9/30/11    Time: 3:30p
 * Updated in $/software/control processor/code/drivers
 * SCR# 1063 - version update
 * 
 * *****************  Version 47  *****************
 * User: Contractor V&v Date: 9/29/11    Time: 5:53p
 * Updated in $/software/control processor/code/drivers
 * Dev 20
 * 
 * *****************  Version 46  *****************
 * User: Jeff Vahue   Date: 9/27/11    Time: 6:52p
 * Updated in $/software/control processor/code/drivers
 * v1.1.0 Dev 19
 * 
 * *****************  Version 45  *****************
 * User: Contractor V&v Date: 9/27/11    Time: 3:49p
 * Updated in $/software/control processor/code/drivers
 * Dev 18
 * 
 * *****************  Version 44  *****************
 * User: Contractor V&v Date: 9/26/11    Time: 6:24p
 * Updated in $/software/control processor/code/drivers
 * Dev 17
 * 
 * *****************  Version 43  *****************
 * User: Jeff Vahue   Date: 9/24/11    Time: 8:52a
 * Updated in $/software/control processor/code/drivers
 * SCR# 1063 - Version tracking
 * 
 * *****************  Version 42  *****************
 * User: Jeff Vahue   Date: 9/20/11    Time: 8:05p
 * Updated in $/software/control processor/code/drivers
 * Dev 15 - Emu UI Fix
 * 
 * *****************  Version 41  *****************
 * User: Contractor V&v Date: 9/19/11    Time: 2:56p
 * Updated in $/software/control processor/code/drivers
 * Dev 14
 * 
 * *****************  Version 40  *****************
 * User: Jeff Vahue   Date: 9/16/11    Time: 1:46p
 * Updated in $/software/control processor/code/drivers
 * Dev 13
 * 
 * *****************  Version 39  *****************
 * User: Jeff Vahue   Date: 9/15/11    Time: 8:34p
 * Updated in $/software/control processor/code/drivers
 * Dev 12
 * 
 * *****************  Version 38  *****************
 * User: Jeff Vahue   Date: 9/15/11    Time: 2:43p
 * Updated in $/software/control processor/code/drivers
 * Dev 11
 * 
 * *****************  Version 37  *****************
 * User: Jeff Vahue   Date: 9/13/11    Time: 7:46p
 * Updated in $/software/control processor/code/drivers
 * Dev 10
 * 
 * *****************  Version 36  *****************
 * User: Jeff Vahue   Date: 9/13/11    Time: 2:34p
 * Updated in $/software/control processor/code/drivers
 * Dev 9
 * 
 * *****************  Version 35  *****************
 * User: Jeff Vahue   Date: 9/13/11    Time: 11:33a
 * Updated in $/software/control processor/code/drivers
 * Dev 8
 * 
 * *****************  Version 34  *****************
 * User: Jeff Vahue   Date: 9/07/11    Time: 5:42p
 * Updated in $/software/control processor/code/drivers
 * Dev 7
 * 
 * *****************  Version 33  *****************
 * User: Jeff Vahue   Date: 9/05/11    Time: 11:03a
 * Updated in $/software/control processor/code/drivers
 * SCR# 1063: Dev Tag Update
 * 
 * *****************  Version 32  *****************
 * User: Jeff Vahue   Date: 9/01/11    Time: 12:20p
 * Updated in $/software/control processor/code/drivers
 * Dev 5 Tag
 * 
 * *****************  Version 31  *****************
 * User: Jeff Vahue   Date: 8/31/11    Time: 3:59p
 * Updated in $/software/control processor/code/drivers
 * SCR# 1063: Version tracking
 * 
 * *****************  Version 30  *****************
 * User: Jeff Vahue   Date: 8/27/11    Time: 4:32p
 * Updated in $/software/control processor/code/drivers
 * Tag v1.1.0 Dev 3
 * 
 * *****************  Version 29  *****************
 * User: Contractor V&v Date: 8/22/11    Time: 7:19p
 * Updated in $/software/control processor/code/drivers
 * Changing to v 1.1.0 Dev 2
 * 
 * *****************  Version 28  *****************
 * User: Jeff Vahue   Date: 8/04/11    Time: 7:38p
 * Updated in $/software/control processor/code/drivers
 * SCR# 1051 - test version identification
 * 
 * *****************  Version 27  *****************
 * User: Jeff Vahue   Date: 8/04/11    Time: 7:32p
 * Updated in $/software/control processor/code/drivers
 * SCR# 1051 - simplify test versioning
 * 
 * *****************  Version 26  *****************
 * User: John Omalley Date: 4/20/11    Time: 8:18a
 * Updated in $/software/control processor/code/drivers
 * SCR 1029
 * 
 * *****************  Version 25  *****************
 * User: Peter Lee    Date: 11/23/10   Time: 1:52p
 * Updated in $/software/control processor/code/drivers
 * Initial Release of v1.0.0 
 * 
 * *****************  Version 24  *****************
 * User: Peter Lee    Date: 11/18/10   Time: 11:15a
 * Updated in $/software/control processor/code/drivers
 * v0.0.8
 * 
 * *****************  Version 22  *****************
 * User: John Omalley Date: 10/22/10   Time: 2:34p
 * Updated in $/software/control processor/code/drivers
 * Weekly Software Build v0.0.7
 * 
 * *****************  Version 21  *****************
 * User: Peter Lee    Date: 9/10/10    Time: 4:06p
 * Updated in $/software/control processor/code/drivers
 * Weekly Software Build 0006
 * 
 * *****************  Version 20  *****************
 * User: Peter Lee    Date: 9/03/10    Time: 8:18p
 * Updated in $/software/control processor/code/drivers
 * Version 0.0.5 Build
 * 
 * *****************  Version 19  *****************
 * User: Peter Lee    Date: 8/27/10    Time: 5:02p
 * Updated in $/software/control processor/code/drivers
 * Weekly Build v0.0.4
 * 
 * *****************  Version 17  *****************
 * User: Jeff Vahue   Date: 8/21/10    Time: 5:51p
 * Updated in $/software/control processor/code/drivers
 * SCR# 811 - bump version number
 * 
 * *****************  Version 16  *****************
 * User: Peter Lee    Date: 8/13/10    Time: 4:38p
 * Updated in $/software/control processor/code/drivers
 * Weekly Build v0.0.2
 * 
 * *****************  Version 15  *****************
 * User: Peter Lee    Date: 8/10/10    Time: 9:53a
 * Updated in $/software/control processor/code/drivers
 * Label for v0.0.1 Release.  No actual code change from v2.0.12
 * 
 * *****************  Version 14  *****************
 * User: Peter Lee    Date: 8/09/10    Time: 6:37p
 * Updated in $/software/control processor/code/drivers
 * Update to v2.0.12 Release
 * 
 * *****************  Version 13  *****************
 * User: Jim Mood     Date: 7/30/10    Time: 5:50p
 * Updated in $/software/control processor/code/drivers
 * Update eng version string
 * 
 * *****************  Version 12  *****************
 * User: Jim Mood     Date: 7/22/10    Time: 12:01p
 * Updated in $/software/control processor/code/drivers
 * Update version string
 * 
 * *****************  Version 11  *****************
 * User: Jim Mood     Date: 7/13/10    Time: 7:04p
 * Updated in $/software/control processor/code/drivers
 * Update version string
 * 
 * *****************  Version 10  *****************
 * User: Jim Mood     Date: 6/16/10    Time: 6:44p
 * Updated in $/software/control processor/code/drivers
 * updated version string
 * 
 * *****************  Version 9  *****************
 * User: Jim Mood     Date: 5/25/10    Time: 5:24p
 * Updated in $/software/control processor/code/drivers
 * updated version for eng build
 * 
 * *****************  Version 8  *****************
 * User: Jim Mood     Date: 3/26/10    Time: 6:09p
 * Updated in $/software/control processor/code/drivers
 * 
 * *****************  Version 7  *****************
 * User: Contractor V&v Date: 3/10/10    Time: 4:40p
 * Updated in $/software/control processor/code/drivers
 * SCR #464 Move Version Info into own file and support it.
 * 
 * *****************  Version 6  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:22p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 * 
 *
 ***************************************************************************/
