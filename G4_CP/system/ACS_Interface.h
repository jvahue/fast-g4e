#ifndef ACS_INTERFACE_H
#define ACS_INTERFACE_H
/******************************************************************************
            Copyright (C) 2008-2011 Pratt & Whitney Engine Services, Inc. 
                      Altair Engine Diagnostic Solutions
               All Rights Reserved. Proprietary and Confidential.

    File:        ACS_Interface.h
    
    Description: All data elements required for an ACS are defined here.
    
    VERSION
      $Revision: 2 $  $Date: 10/26/11 6:44p $     
    
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
#define ACS_CONFIG_DEFAULT    "None",         /* Model              */\
                              "FAST-None",    /* ACS ID             */\
                              ACS_PORT_NONE,  /* ACS Port Type      */\
                              0,              /* Port Index         */\
                              1000,           /* Packet Size in Ms  */\
                              STA_NORMAL,     /* System Condition   */\
                              ACS_RECORD      /* ACS Mode           */
                              
//Default configuration for the sensors
#define ACS_CONFIGS_DEFAULT    {ACS_CONFIG_DEFAULT},\
                               {ACS_CONFIG_DEFAULT},\
                               {ACS_CONFIG_DEFAULT},\
                               {ACS_CONFIG_DEFAULT},\
                               {ACS_CONFIG_DEFAULT},\
                               {ACS_CONFIG_DEFAULT},\
                               {ACS_CONFIG_DEFAULT},\
                               {ACS_CONFIG_DEFAULT}


#define MAX_ACS_CONFIG          8
#define MAX_ACS_NAME            32

typedef enum
{
   ACS_PORT_NONE,
   ACS_PORT_ARINC429,
   ACS_PORT_QAR,
   ACS_PORT_UART,
   ACS_PORT_MAX
} ACS_PORT_TYPE;

typedef enum
{
   ACS_RECORD,
   ACS_DOWNLOAD
} ACS_MODE;

typedef enum
{
   DL_IN_PROGRESS,
   DL_NO_RECORDS,
   DL_RECORD_FOUND
} DL_STATUS;

typedef enum
{
   DL_RECORD_REQUEST,
   DL_RECORD_CONTINUE,
   DL_END
} DL_STATE;

typedef enum
{
   DL_WRITE_IN_PROGRESS,
   DL_WRITE_SUCCESS,
   DL_WRITE_FAIL
} DL_WRITE_STATUS;

// Definition of an ACS configuration.
typedef struct
{
  CHAR          Model[MAX_ACS_NAME]; 
  CHAR          ID[MAX_ACS_NAME];
  ACS_PORT_TYPE PortType;
  UINT8         PortIndex;
  UINT16        PacketSize_ms;
  FLT_STATUS    SystemCondition; 
  ACS_MODE      Mode;
} ACS_CONFIG;

//A type for an array of the maximum number of ACSs
//Used for storing ACS configurations in the configuration manager
typedef ACS_CONFIG ACS_CONFIGS[MAX_ACS_CONFIG];

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined( ACS_INTERFACE_BODY )
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

#endif 


/*************************************************************************
 *  MODIFICATIONS
 *    $History: ACS_Interface.h $
 * 
 * *****************  Version 2  *****************
 * User: John Omalley Date: 10/26/11   Time: 6:44p
 * Updated in $/software/control processor/code/system
 * SCR 1097 - Dead Code Update
 * 
 * *****************  Version 1  *****************
 * User: John Omalley Date: 4/11/11    Time: 10:28a
 * Created in $/software/control processor/code/system
 * SCR 1029 - EMU Download
 * 
 *
 ***************************************************************************/
 



