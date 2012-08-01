#ifndef MSFX_H
#define MSFX_H

/******************************************************************************
            Copyright (C) 2009-2010 Pratt & Whitney Engine Services, Inc. 
                      Altair Engine Diagnostic Solutions
               All Rights Reserved. Proprietary and Confidential.

    File:        MSFileXfr.h
    
    Description: Routines for retrieving files from the Micro-Server to
                 the CP GSE port.

    VERSION
    $Revision: 7 $  $Date: 10/08/10 10:46a $   
    
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
#define MSFX_MAX_PATH_SIZE 64 /*Default max file path and name string
                                length. Rational: Path to CP_SAVED ~28chars 
                                rounded up to even 32 and doubled*/
#define MSFX_MAX_FN_SIZE   (MSFX_MAX_PATH_SIZE*2) //Accomadate path+fn
#define MSFX_CFG_DEFAULT   "/mnt/cf0/1/CP_FILES","/mnt/cf0/1/CP_FILES/CP_SAVED","/tmp"

/******************************************************************************
                                 Package Typedefs
******************************************************************************/
typedef struct{
  INT8 NonTxdFilesPath[MSFX_MAX_PATH_SIZE];
  INT8 TxdFilesPath[MSFX_MAX_PATH_SIZE];
  INT8 TmpPath[MSFX_MAX_PATH_SIZE];
}MSFX_CFG_BLOCK;


/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( MSFX_BODY )
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
EXPORT void MSFX_Init(void);


/*****************************************************************************
 *  MODIFICATIONS
 *    $History: MSFileXfr.h $
 * 
 * *****************  Version 7  *****************
 * User: John Omalley Date: 10/08/10   Time: 10:46a
 * Updated in $/software/control processor/code/application
 * SCR 924 - Code Review Updates
 * 
 * *****************  Version 6  *****************
 * User: Jim Mood     Date: 6/11/10    Time: 10:06a
 * Updated in $/software/control processor/code/application
 * SCR 623 Batch mode.  Implements YMODEM  receiver
 * 
 * *****************  Version 5  *****************
 * User: Contractor2  Date: 3/02/10    Time: 3:40p
 * Updated in $/software/control processor/code/application
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 4  *****************
 * User: Jim Mood     Date: 2/24/10    Time: 10:49a
 * Updated in $/software/control processor/code/application
 * 
 * *****************  Version 3  *****************
 * User: Jim Mood     Date: 1/20/10    Time: 3:45p
 * Updated in $/software/control processor/code/application
 * Initial working build and completion of outstanding TODO:s
 * 
 * *****************  Version 2  *****************
 * User: Jim Mood     Date: 11/17/09   Time: 6:26p
 * Updated in $/software/control processor/code/application
 * YModem transfer works, still need to fill in some todos
 * 
 * *****************  Version 1  *****************
 * User: Jim Mood     Date: 11/10/09   Time: 11:21a
 * Created in $/software/control processor/code/application
 * 
 *
 *****************************************************************************/

#endif // MSFX_H                            
