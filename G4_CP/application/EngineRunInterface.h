#ifndef ENGINERUNINTERFACE_H
#define ENGINERUNINTERFACE_H

/******************************************************************************
            Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:         EngineRunInterface.h

    Description: Interface control EngineRun <-> Cycle

    VERSION
    $Revision: 10 $  $Date: 12-10-31 2:25p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "sensor.h"
/******************************************************************************
                                 Package Defines                              *
******************************************************************************/
#define MAX_CYCLES     32 /* maximum # of cycles supported. See also NV_PER_CYCLE_CNTS */
#define MAX_ENGRUN_SENSORS  16 /* Maximum # of sensor summaries monitored for an EngineRun  */

/******************************************************************************
                                 Package Typedefs                             *
******************************************************************************/
typedef enum
{
  ENGRUN_ID_0   = 0,
  ENGRUN_ID_1   = 1,
  ENGRUN_ID_2   = 2,
  ENGRUN_ID_3   = 3,
  MAX_ENGINES   = 4,
  //   Use 'ENGRUN_ID_0' thru 'MAX_ENGINES' for normal range checking.
  ENGRUN_ID_ANY = 100, // wildcard used to query all engineruns
  ENGRUN_UNUSED = 255
} ENGRUN_INDEX;

/******************************************************************************
                                 Package Exports                              *
******************************************************************************/
#undef EXPORT

#if defined ( ENGINERUN_BODY )
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
EXPORT UINT32* EngRunGetPtrToCycleCounts(ENGRUN_INDEX engId);


#endif // ENGINERUNINTERFACE_H

/*************************************************************************
 *  MODIFICATIONS
 *    $History: EngineRunInterface.h $
 * 
 * *****************  Version 10  *****************
 * User: Melanie Jutras Date: 12-10-31   Time: 2:25p
 * Updated in $/software/control processor/code/application
 * SCR #1142 File Format Error
 * 
 * *****************  Version 9  *****************
 * User: Contractor V&v Date: 10/11/12   Time: 6:55p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 Review Findings
 * 
 * *****************  Version 8  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 12:43p
 * Updated in $/software/control processor/code/application
 * SCR# 1142
 * 
 * *****************  Version 7  *****************
 * User: Contractor V&v Date: 7/18/12    Time: 6:24p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2
 * 
 * *****************  Version 6  *****************
 * User: Contractor V&v Date: 7/11/12    Time: 4:29p
 * Updated in $/software/control processor/code/application
 * Changed data type for persist counts
 * 
 * *****************  Version 5  *****************
 * User: Contractor V&v Date: 6/25/12    Time: 3:01p
 * Updated in $/software/control processor/code/application
 * 
 * *****************  Version 4  *****************
 * User: Contractor V&v Date: 6/18/12    Time: 4:26p
 * Updated in $/software/control processor/code/application
 * FAST 2 Add accessor for running engs
 * 
 * *****************  Version 3  *****************
 * User: Contractor V&v Date: 5/24/12    Time: 3:05p
 * Updated in $/software/control processor/code/application
 * FAST2 Refactoring
 * 
 * *****************  Version 2  *****************
 * User: John Omalley Date: 12-05-22   Time: 2:15p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Check In for Dave
 * 
 * *****************  Version 1  *****************
 * User: Contractor V&v Date: 5/10/12    Time: 6:39p
 * Created in $/software/control processor/code/application
 * Add interface and usertables for enginerun object
 ***************************************************************************/
