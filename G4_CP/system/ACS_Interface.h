#ifndef ACS_INTERFACE_H
#define ACS_INTERFACE_H
/******************************************************************************
            Copyright (C) 2008-2015 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    ECCN:        9E991

    File:        ACS_Interface.h
    
    Description: All data elements required for an ACS are defined here.
    
    VERSION
      $Revision: 7 $  $Date: 3/09/15 10:53a $     
    
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
#define ACS_CONFIG_DEFAULT    "None",           /* Model              */\
                              "FAST-None",      /* ACS ID             */\
                              LOG_PRIORITY_3,   /* ACS Priority       */\
                              ACS_PORT_NONE,    /* ACS Port Type      */\
                              ACS_PORT_INDEX_0, /* Port Index         */\
                              1000,             /* Packet Size in Ms  */\
                              STA_NORMAL,       /* System Condition   */\
                              ACS_RECORD        /* ACS Mode           */
                              
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
   ACS_PORT_INDEX_0,
   ACS_PORT_INDEX_1,
   ACS_PORT_INDEX_2,
   ACS_PORT_INDEX_3,
   ACS_PORT_INDEX_10 = 10 /* Virtual Port for Multiplexing single UART Channel */
   // This enumeration must fit in an UINT8 so don't make value larger than 255
} ACS_PORT_INDEX;

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

#pragma pack(1)
// Definition of an ACS configuration.
typedef struct
{
  CHAR           sModel[MAX_ACS_NAME];
  CHAR           sID[MAX_ACS_NAME];
  LOG_PRIORITY   priority;
  ACS_PORT_TYPE  portType;
  ACS_PORT_INDEX nPortIndex;
  UINT16         nPacketSize_ms;
  FLT_STATUS     systemCondition;
  ACS_MODE       mode;
} ACS_CONFIG;
#pragma pack()

//A type for an array of the maximum number of ACSs
//Used for storing ACS configurations in the configuration manager
typedef ACS_CONFIG ACS_CONFIGS[MAX_ACS_CONFIG];

/******************************************************************************
                                 Package Typedefs
******************************************************************************/

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( ACS_INTERFACE_BODY )
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


#endif // ACS_INTERFACE_H
/*************************************************************************
 *  MODIFICATIONS
 *    $History: ACS_Interface.h $
 * 
 * *****************  Version 7  *****************
 * User: John Omalley Date: 3/09/15    Time: 10:53a
 * Updated in $/software/control processor/code/system
 * SCR 1255 - Changed Port Index to an enumeration for values 0,1,2,3,10
 * 
 * *****************  Version 6  *****************
 * User: John Omalley Date: 12-11-12   Time: 4:30p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review Update
 * 
 * *****************  Version 5  *****************
 * User: John Omalley Date: 12-11-09   Time: 10:34a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review Updates
 * 
 * *****************  Version 4  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 3  *****************
 * User: Jim Mood     Date: 7/19/12    Time: 11:07a
 * Updated in $/software/control processor/code/system
 * SCR 1107: Data Offload changes for 2.0.0
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
 



