#ifndef DIOMGR_H
#define DIOMGR_H

/******************************************************************************
            Copyright (C) 2007-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:    DioMgr.h     
    
    Description: Header file with declarations for the DioMgr.c which 
                 provides diagnostic interface to the discrete I/O.
                  

   VERSION
    $Revision: 6 $  $Date: 7/26/11 3:06p $
    
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/

/******************************************************************************
                                 Package Defines                              *
******************************************************************************/

/******************************************************************************
                                 Package Typedefs                             *
******************************************************************************/

/******************************************************************************
                                 Package Exports                              *
******************************************************************************/
#undef EXPORT

#if defined ( DIOMGR_BODY )
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
EXPORT void DIOMgr_Init(void);
EXPORT void DIOMgr_FSMRunDIO(BOOLEAN Run, INT32 param);
EXPORT BOOLEAN DIOMgr_FSMGetStateDIO(INT32 param);


#endif // DIOMGR_H
/*************************************************************************
*  MODIFICATIONS
*    $History: DIOMgr.h $
 * 
 * *****************  Version 6  *****************
 * User: Jim Mood     Date: 7/26/11    Time: 3:06p
 * Updated in $/software/control processor/code/system
 * SCR 575 updates
 * 
 * *****************  Version 5  *****************
 * User: Peter Lee    Date: 8/30/10    Time: 6:05p
 * Updated in $/software/control processor/code/system
 * SCR #806 Code Review Updates
 * 
 * *****************  Version 4  *****************
 * User: Contractor3  Date: 5/27/10    Time: 10:57a
 * Updated in $/software/control processor/code/system
 * SCR #617 - Changes made to meet coding standards as a result of code
 * review.
 * 
 * *****************  Version 3  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:57p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
* 
*
*
***************************************************************************/

