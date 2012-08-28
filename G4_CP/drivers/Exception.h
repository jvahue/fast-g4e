#ifndef EXCEPTION_H
#define EXCEPTION_H

/******************************************************************************
            Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:        Exception.h
    
    Description: Process and display debug data for
                 unhandled user interrupts, core processor exceptions,
                 and ASSERT macro failures.

    VERSION
    $Revision: 7 $  $Date: 8/28/12 1:06p $   
    
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
//#define ASSERT_MAX_LOG_SIZE  1024   /* Defined in Assert.h */

/******************************************************************************
                                 Package Typedefs
******************************************************************************/

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( EXCEPTION_BODY )
   #define EXPORT
#else
   #define EXPORT extern
#endif

/******************************************************************************
                             Package Exports Variables
******************************************************************************/
extern CHAR ExOutputBuf[ASSERT_MAX_LOG_SIZE]; // Buffer containing debug data  
                                              // sent to the GSE port after an 
                                              // assert or exception.
extern CHAR ReOutputBuf[ASSERT_MAX_LOG_SIZE]; // Buffer containing register   
                                              // data sent to the GSE port after 
                                              // an unhandled exception.

/******************************************************************************
                             Package Exports Functions
******************************************************************************/
extern void AssertDump(void); //Dump call stack to the ExOutputBuf.


#endif // EXCEPTION_H  
/*****************************************************************************
 *  MODIFICATIONS
 *    $History: Exception.h $
 * 
 * *****************  Version 7  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 6  *****************
 * User: Contractor2  Date: 9/27/10    Time: 2:55p
 * Updated in $/software/control processor/code/drivers
 * SCR #795 Code Review Updates
 * 
 * *****************  Version 5  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:14p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 4  *****************
 * User: Jeff Vahue   Date: 2/19/10    Time: 5:00p
 * Updated in $/software/control processor/code/drivers
 * SCR# 456 - Assert Processing cleanup
 * 
 * *****************  Version 3  *****************
 * User: Contractor V&v Date: 2/03/10    Time: 2:56p
 * Updated in $/software/control processor/code/drivers
 * SCR 279
 * 
 * *****************  Version 2  *****************
 * User: Jim Mood     Date: 9/15/09    Time: 4:57p
 * Updated in $/software/control processor/code/drivers
 * Updated for Assert and Exception requirements
 * 
 * *****************  Version 1  *****************
 * User: Jim Mood     Date: 9/14/09    Time: 5:18p
 * Created in $/software/control processor/code/drivers
 *****************************************************************************/



