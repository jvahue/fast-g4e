#ifndef ASSERT_H
#define ASSERT_H

/******************************************************************************
            Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:        Assert.h
    
    Description: Handles, displays, and logs Assert and Unhandled Exceptions to EE

    VERSION
    $Revision: 15 $  $Date: 8/28/12 1:06p $   
    
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/

/******************************************************************************
                                 Package Defines
******************************************************************************/
#define ASSERT_MAX_LOG_SIZE  1024
#define NO_PARAMS NULL      // Required for variadic ASSERT/FATAL macro when
                            // no params passed          
#if defined(ASSERT)
#undef ASSERT
#endif

// Call the Assert function without passing the variadic param
#define ASSERT(_COND_)                               \
if (!(_COND_))                                       \
{                                                    \
    Assert("[ASSERT]", #_COND_, __FILE__, __LINE__); \
}

#if defined(ASSERT_MESSAGE)
#undef ASSERT_MESSAGE
#endif

// Call the AssertMessage function
// If no variadic params are needed for user msg string, caller should supply 'NULL'
#define ASSERT_MESSAGE(_COND_, _USER_MSG_STRING_, ...)                                      \
if (!(_COND_))                                                                              \
{                                                                                           \
    AssertMessage("[ASSERT]", #_COND_, _USER_MSG_STRING_, __FILE__, __LINE__, __VA_ARGS__); \
}

#if defined(FATAL)
#undef FATAL
#endif
// Call the assert function passing the variadic function.
#define FATAL(_USER_MSG_STRING_, ...)                                       \
    Assert("[FATAL]",  #_USER_MSG_STRING_, __FILE__, __LINE__, __VA_ARGS__);  

/******************************************************************************
                                 Package Typedefs
******************************************************************************/

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( ASSERT_BODY )
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
EXPORT void   AssertHandler( CHAR *AssertStr, CHAR* UserMsg, CHAR *File,
                             INT32 Line, va_list arg);
EXPORT void   Assert( CHAR *AssertStr, CHAR* UserMsg, CHAR *File, INT32 Line, ...);
EXPORT void   AssertMessage( CHAR *AssertStr, CHAR* Cond, CHAR* UserMsg, CHAR *File, 
                             INT32 Line, ...);

EXPORT void   Assert_Init(void);
EXPORT size_t Assert_GetLog( void* buf );
EXPORT void   Assert_ClearLog(void);
EXPORT void   Assert_MarkLogRead(void);
EXPORT void   Assert_UnhandledException(void);
EXPORT BOOLEAN   Assert_InitEE(void);
EXPORT void   Assert_WriteToEE(void *data, size_t size);

#endif // ASSERT_H     
/*****************************************************************************
 *  MODIFICATIONS
 *    $History: Assert.h $
 * 
 * *****************  Version 15  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 14  *****************
 * User: Contractor2  Date: 6/27/11    Time: 1:29p
 * Updated in $/software/control processor/code/drivers
 * SCR #515 Enhancement - sys.get.mem.l halts processor when invalid
 * memory is referenced
 * 
 * *****************  Version 13  *****************
 * User: Contractor2  Date: 9/27/10    Time: 2:55p
 * Updated in $/software/control processor/code/drivers
 * SCR #795 Code Review Updates
 * 
 * *****************  Version 12  *****************
 * User: Contractor V&v Date: 7/21/10    Time: 7:14p
 * Updated in $/software/control processor/code/drivers
 * SCR #260 Implement BOX status command
 * 
 * *****************  Version 11  *****************
 * User: Contractor V&v Date: 6/08/10    Time: 5:52p
 * Updated in $/software/control processor/code/drivers
 * SCR #614 fast.reset=really should initialize files
 * 
 * *****************  Version 10  *****************
 * User: Contractor V&v Date: 4/28/10    Time: 4:50p
 * Updated in $/software/control processor/code/drivers
 * SCR #543 Misc - Add comment about mandatory NULL for empty variadic
 * param.
 * 
 * *****************  Version 9  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:07p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 8  *****************
 * User: Jeff Vahue   Date: 2/19/10    Time: 5:00p
 * Updated in $/software/control processor/code/drivers
 * SCR# 456 - Assert Processing cleanup
 * 
 * *****************  Version 7  *****************
 * User: Contractor V&v Date: 2/03/10    Time: 2:56p
 * Updated in $/software/control processor/code/drivers
 * SCR 279
 * 
 * *****************  Version 6  *****************
 * User: Contractor V&v Date: 1/13/10    Time: 4:58p
 * Updated in $/software/control processor/code/drivers
 * 
 * *****************  Version 5  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:22p
 * Updated in $/software/control processor/code/drivers
 * SCR 371
 * 
 * *****************  Version 4  *****************
 * User: Contractor V&v Date: 11/18/09   Time: 3:23p
 * Updated in $/software/control processor/code/drivers
 * Implement SCR 172
 * 
 * *****************  Version 3  *****************
 * User: John Omalley Date: 10/07/09   Time: 5:47p
 * Updated in $/software/control processor/code/drivers
 * SCR 283 - Need to add the #include stddef.h
 * 
 * *****************  Version 2  *****************
 * User: Jim Mood     Date: 9/15/09    Time: 4:56p
 * Updated in $/software/control processor/code/drivers
 * Completed Assert and Exception requrement implimentation
 * 
 * *****************  Version 1  *****************
 * User: Jim Mood     Date: 9/08/09    Time: 6:32p
 * Created in $/software/control processor/code/drivers
 * First pass, writes assert message to EE and GSE.  
 * 
 *
 *****************************************************************************/

