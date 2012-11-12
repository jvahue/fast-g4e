#ifndef INITIALIZATION_MANAGER_H
#define INITIALIZATION_MANAGER_H
/******************************************************************************
            Copyright (C) 2007-2010 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.
 
  File:        InitializationManager.h
 
  Description: Initialization Manager definitions
 
 VERSION
     $Revision: 7 $  $Date: 12-11-12 4:46p $

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

/******************************************************************************
                                 Package Typedefs
******************************************************************************/


/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( INITIALIZATION_MANAGER_BODY )
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
EXPORT void Im_InitializeControlProcessor(void);
EXPORT void Im_StartupTickHandler        (void);


#endif // INITIALIZATION_MANAGER_H
/*************************************************************************
 *  MODIFICATIONS
 *    $History: InitializationManager.h $
 * 
 * *****************  Version 7  *****************
 * User: John Omalley Date: 12-11-12   Time: 4:46p
 * Updated in $/software/control processor/code/system
 * SCR 1142 - Formatting Error
 * 
 * *****************  Version 6  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 5  *****************
 * User: Contractor V&v Date: 7/27/10    Time: 2:26p
 * Updated in $/software/control processor/code/system
 * SCR #573 Startup WD Management/DT overrun at startup
 * 
 * *****************  Version 4  *****************
 * User: Contractor2  Date: 6/14/10    Time: 1:11p
 * Updated in $/software/control processor/code/system
 * SCR #483 Function Names must begin with the CSC it belongs with.
 * 
 * *****************  Version 3  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 * 
 *
 ***************************************************************************/
 

// End Of File


