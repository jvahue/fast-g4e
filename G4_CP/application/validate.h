#ifndef VALIDATE_H
#define	VALIDATE_H
/******************************************************************************
Copyright (C) 2008-2012 Pratt & Whitney Engine Services, Inc.
All Rights Reserved. Proprietary and Confidential.

File:       Validate.h

Description:
VERSION
$Revision: 2 $  $Date: 4/30/10 6:06p $
******************************************************************************/
/*< Generated code - Password Authentication block  start>*/
  const UINT8 key[] =
  {
    0xAD, 0xD2, 0x8F, 0x2D, 0x44, 0x5C, 0xF8, 0x09,
    0xDB, 0xB8, 0x8F, 0xFA, 0xC3, 0x30, 0x58, 0xCE,
    0x81, 0x04, 0xA3, 0xDD, 0xAA, 0xF4, 0x13, 0x17,
    0x85, 0x9C, 0x32, 0x4E, 0x99, 0xBA, 0x28, 0x57,
  };

  // Encrypted factory password and offset for: "offwf" 
  const UINT8 fact[] = {0x42, 0x22, 0x3A, 0x8F, 0x6F};
  const UINT8 factoryKeyOffset = 3;

  // Encrypted privileged password and offset for: "oflgs" 
  const UINT8 priv[] = {0x33, 0x9E, 0x65, 0xBC, 0xCB};
  const UINT8 privilegedKeyOffset = 5;

/*< Generated code - Password Authentication block  end>*/

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

#if defined ( VALIDATE_BODY )                                                  
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

#endif // VALIDATE_H
/*************************************************************************
 *  MODIFICATIONS
 *    $History: validate.h $
 * 
 * *****************  Version 2  *****************
 * User: Contractor V&v Date: 4/30/10    Time: 6:06p
 * Updated in $/software/control processor/code/application
 * Changed passwords. See validate.h for clear text values
 *
 * 
 ***************************************************************************/


