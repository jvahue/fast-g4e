#ifndef COMPILER_H
#define COMPILER_H
/******************************************************************************
            Copyright (C) 2007-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

  File:          Compiler.h
 
  Description:   Defines the processor and compiler used in this project.

  Note:          
 
  VERSION
      $Revision: 5 $  $Date: 12-11-12 4:46p $ 
 
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
#define CONTROL_PROCESSOR_GHS

/******************************************************************************
                                 Package Typedefs
******************************************************************************/

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( COMPILER_BODY )
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


#endif // COMPILER_H
/*************************************************************************
 *  MODIFICATIONS
 *    $History: compiler.h $
 * 
 * *****************  Version 5  *****************
 * User: John Omalley Date: 12-11-12   Time: 4:46p
 * Updated in $/software/control processor/code/drivers
 * SCR 1142 - Formatting Error
 * 
 * *****************  Version 4  *****************
 * User: Melanie Jutras Date: 12-11-09   Time: 2:10p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 File Format Error
 * 
 * *****************  Version 3  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 2  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:07p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 * 
 *
 ***************************************************************************/


