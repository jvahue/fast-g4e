#ifndef PROCESSOR_INIT_H
#define PROCESSOR_INIT_H

/******************************************************************************
            Copyright (C) 2010 Pratt & Whitney Engine Services, Inc. 
                      Altair Engine Diagnostic Solutions
               All Rights Reserved. Proprietary and Confidential.

    File:        ProcessorInit.h
    
    Description: Processor Initialization and Cache Manager

    VERSION
    $Revision: 2 $  $Date: 9/27/10 2:55p $   
    
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

#if defined ( PROCESSORINIT_BODY )
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
EXPORT void disable_cache(void);
EXPORT void init_cache(void);

EXPORT void PI_FlushCache(void);
EXPORT void PI_FlushInstructionCache(void);
EXPORT void PI_FlushBranchCache(void);


/*****************************************************************************
 *  MODIFICATIONS
 *    $History: ProcessorInit.h $
 * 
 * *****************  Version 2  *****************
 * User: Contractor2  Date: 9/27/10    Time: 2:55p
 * Updated in $/software/control processor/code/drivers
 * SCR #795 Code Review Updates
 * 
 * *****************  Version 1  *****************
 * User: Contractor2  Date: 6/14/10    Time: 4:10p
 * Created in $/software/control processor/code/drivers
 * SCR #645 Periodically flush instruction/branch caches
 * 
 *****************************************************************************/

#endif // PROCESSOR_INIT_H                             


