#ifndef AIRCRAFTCONFIGMGR_H
#define AIRCRAFTCONFIGMGR_H

/******************************************************************************
          Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc.                 
                All Rights Reserved. Proprietary and Confidential.

 File: AircraftConfigMgr.h

 Description: This file defines structures for the AirCraft reconfigure module. 

   VERSION
   $Revision: 20 $  $Date: 12-11-01 12:37p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
 
/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_basic.h"
#include "sensor.h"
//#include "DtuInterface.h"

/******************************************************************************
                                   Package Defines
******************************************************************************/

#define AC_VER_DEFAULT            "v0.0.0"
#define AC_ERROR                  "ERR"
#define AC_ERROR_CHAR_COUNT       3
#define AC_DEFAULT_NO_ERR         "CLEAR"
//Timeout for request cfg files from ground server 
#define AC_REQ_FILES_FROM_GROUND_TO 300000  //300k ms or 5 minutes.
//note: 2648
#define AC_CONFIG_DEFAULT TRUE,             /*bCfgEnabled*/\
                          FALSE,            /*ValidateData*/\
                          AC_DEFAULT_NO_ERR,/*TailNumber*/\
                          AC_DEFAULT_NO_ERR,/*Operator*/\
                          AC_DEFAULT_NO_ERR,/*Type*/\
                          SENSOR_UNUSED,    /*TypeSensorIndex*/\
                          AC_DEFAULT_NO_ERR,/*FleetIdent*/\
                          SENSOR_UNUSED,    /*FleetSensorIndex*/\
                          AC_DEFAULT_NO_ERR,/*Number*/\
                          SENSOR_UNUSED,    /*NumberSensorIndex*/\
                          AC_DEFAULT_NO_ERR,/*Owner*/\
                          AC_DEFAULT_NO_ERR,/*Style*/

                              
/******************************************************************************
                                   Package Typedefs
******************************************************************************/
typedef enum 
{
   AIRCRAFT_ID_OK,
   AIRCRAFT_ID_NO_VALIDATE,
   AIRCRAFT_ID_MISMATCH,
   AIRCRAFT_ID_CONFIG_ERR,
   AIRCRAFT_ID_NOT_FOUND,
   AIRCRAFT_ID_NO_CONFIG
} AIRCRAFT_ID_STATUS;   

typedef enum
{
   AC_CFG_STATE_NO_CONFIG,
   AC_CFG_STATE_CONFIG_OK,
   AC_CFG_STATE_RETRIEVING,
} AIRCRAFT_CONFIG_STATE;

#define AIRCRAFT_CONFIG_STR_LEN 32
#define AC_STR_LIMIT 0,AIRCRAFT_CONFIG_STR_LEN

//note: 
typedef struct 
{
   BOOLEAN bCfgEnabled;
   BOOLEAN bValidateData;
   INT8    TailNumber[AIRCRAFT_CONFIG_STR_LEN];
   INT8    Operator[AIRCRAFT_CONFIG_STR_LEN];
   INT8    Type[AIRCRAFT_CONFIG_STR_LEN];
   SENSOR_INDEX  TypeSensorIndex;
   INT8    FleetIdent[AIRCRAFT_CONFIG_STR_LEN];
   SENSOR_INDEX  FleetSensorIndex;
   INT8    Number[AIRCRAFT_CONFIG_STR_LEN];
   SENSOR_INDEX  NumberSensorIndex;
   INT8    Owner[AIRCRAFT_CONFIG_STR_LEN];
   INT8    Style[AIRCRAFT_CONFIG_STR_LEN];
} AIRCRAFT_CONFIG;

#pragma pack(1)
typedef struct 
{
   BOOLEAN bCfgEnabled;
   BOOLEAN bValidateData;
   INT8    TailNumber[AIRCRAFT_CONFIG_STR_LEN];
   INT8    Operator[AIRCRAFT_CONFIG_STR_LEN];
   INT8    Type[AIRCRAFT_CONFIG_STR_LEN];
   SENSOR_INDEX  TypeSensorIndex;
   INT8    FleetIdent[AIRCRAFT_CONFIG_STR_LEN];
   SENSOR_INDEX  FleetSensorIndex;
   INT8    Number[AIRCRAFT_CONFIG_STR_LEN];
   SENSOR_INDEX  NumberSensorIndex;
   INT8    Owner[AIRCRAFT_CONFIG_STR_LEN];
   INT8    Style[AIRCRAFT_CONFIG_STR_LEN];
} AIRCRAFT_CONFIG_PACKED; 

typedef struct
{
  AIRCRAFT_CONFIG_PACKED Config;
  FLOAT32         TypeValue;
  FLOAT32         FleetValue;
  FLOAT32         NumberValue;
} AIRCRAFT_MISMATCH_LOG;
#pragma pack()

#define AIRCRAFT_AUTO_ERR_STR_LEN    80
typedef INT8 AIRCRAFT_AUTO_ERROR_LOG[AIRCRAFT_AUTO_ERR_STR_LEN];


#define AC_FAULT_STR_LEN    64
typedef struct
{
   INT8 FaultMsg[AC_FAULT_STR_LEN];
   INT8 Debug1[AC_FAULT_STR_LEN];
   INT8 Debug2[AC_FAULT_STR_LEN];
}AC_FAULT_LOG;

#define AC_CLEAR                    "CLEAR"
#define AC_XML_SERVER_NOT_FOUND_STR "ERR002"
#define AC_XML_FILE_NOT_FOUND_STR   "ERR003"
#define AC_XML_FAILED_TO_OPEN_STR   "ERR004"
#define AC_XML_FAST_NOT_FOUND_STR   "ERR005"
#define AC_XML_SN_NOT_FOUND_STR     "ERR006"
#define AC_CFG_SERVER_NOT_FOUND_STR "ERR007"
#define AC_CFG_FILE_NOT_FOUND_STR   "ERR008"
#define AC_CFG_FILE_NO_VER_STR      "ERR009"
#define AC_CFG_FILE_FMT_ERR_STR     "ERR010"
#define AC_CFG_FAILED_TO_UPDATE_STR "ERR011"
#define AC_ID_MISMATCH_STR          "ERR012"
#define AC_CFG_NO_VALIDATE_STR      "ERR013"

      
/******************************************************************************
                                 Package Exports                              *
******************************************************************************/
#undef EXPORT

#if defined ( AIRCRAFTCONFIGMGR_BODY )
   #define EXPORT
#else
   #define EXPORT extern
#endif


/******************************************************************************
                              Package Exports Variables
******************************************************************************/
EXPORT void AircraftConfigInitialization( void );
EXPORT void AircraftConfigStartTask( void *pParam );
EXPORT void AircraftConfigMgrTask( void *pParam );
EXPORT void AircraftConfigUpdateTask( void *pParam );
EXPORT void AircraftConfigEnable( BOOLEAN Enable);
EXPORT BOOLEAN AC_FileInit(void);
EXPORT void AC_SetIsGroundServerConnected(BOOLEAN State);
EXPORT void AC_SetIsMSConnected(BOOLEAN State);
EXPORT void AC_SetIsOnGround(BOOLEAN State);
EXPORT AIRCRAFT_CONFIG_STATE AC_GetConfigState(void);
EXPORT BOOLEAN AC_StartAutoRecfg(void);
EXPORT BOOLEAN AC_StartManualRecfg(void);
EXPORT BOOLEAN AC_CancelRecfg(void);
EXPORT BOOLEAN AC_SendClearCfgDir(void);
EXPORT void    AC_GetConfigStatus(INT8* str);
EXPORT void    AC_FSMAutoRun(BOOLEAN Run, INT32 param);
EXPORT BOOLEAN AC_FSMGetState(INT32 param);

/******************************************************************************
                              Package Exports Functions
******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif




#ifdef __cplusplus
}
#endif

#endif // AIRCRAFTCONFIGMGR_H

/*************************************************************************
 *  MODIFICATIONS
 *    $History: AircraftConfigMgr.h $
 * 
 * *****************  Version 20  *****************
 * User: Melanie Jutras Date: 12-11-01   Time: 12:37p
 * Updated in $/software/control processor/code/application
 * SCR #1142 File Format Errors
 * 
 * *****************  Version 19  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 2:36p
 * Updated in $/software/control processor/code/application
 * SCR# 1142 - more format corrections
 * 
 * *****************  Version 18  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 12:43p
 * Updated in $/software/control processor/code/application
 * SCR# 1142
 * 
 * *****************  Version 17  *****************
 * User: Jim Mood     Date: 7/20/11    Time: 10:54a
 * Updated in $/software/control processor/code/application
 * SCR 575: GSM Enable when engine status is lost.  (Part of changes for
 * the Fast State Machine)
 * 
 * *****************  Version 16  *****************
 * User: Contractor2  Date: 9/20/10    Time: 4:10p
 * Updated in $/software/control processor/code/application
 * SCR #856 AC Reconfig File should be re-initialized during fast.reset
 * command.
 * 
 * *****************  Version 15  *****************
 * User: Jeff Vahue   Date: 9/09/10    Time: 11:53a
 * Updated in $/software/control processor/code/application
 * SCR# 833 - Perform only three attempts to get the Cfg files from the
 * MS.  Fix various typos.
 * 
 * *****************  Version 14  *****************
 * User: Peter Lee    Date: 9/03/10    Time: 11:58a
 * Updated in $/software/control processor/code/application
 * SCR #806 Code Review Updates
 * 
 * *****************  Version 13  *****************
 * User: Peter Lee    Date: 7/30/10    Time: 10:25a
 * Updated in $/software/control processor/code/application
 * SCR #739 AIRCRAFT_CONFIG packed structure
 * 
 * *****************  Version 12  *****************
 * User: Jim Mood     Date: 7/22/10    Time: 11:50a
 * Updated in $/software/control processor/code/application
 * SCR 728 Exit task when Validate Data is no
 * 
 * *****************  Version 11  *****************
 * User: Contractor V&v Date: 7/21/10    Time: 7:12p
 * Updated in $/software/control processor/code/application
 * SCR #260 Implement BOX status command
 * 
 * *****************  Version 10  *****************
 * User: Jim Mood     Date: 7/08/10    Time: 3:47p
 * Updated in $/software/control processor/code/application
 * SCR 623 Auto reconfiguraion implementation
 * 
 * *****************  Version 9  *****************
 * User: Jim Mood     Date: 6/29/10    Time: 6:29p
 * Updated in $/software/control processor/code/application
 * SCR 623.  Automatic reconfiguration requirments complete
 * 
 * *****************  Version 8  *****************
 * User: Jim Mood     Date: 6/16/10    Time: 6:25p
 * Updated in $/software/control processor/code/application
 * SCR 623 Batch configuration load 
 * 
 * *****************  Version 7  *****************
 * User: Jim Mood     Date: 6/11/10    Time: 10:00a
 * Updated in $/software/control processor/code/application
 * SCR 623 Batch Configuration changes
 * 
 * *****************  Version 6  *****************
 * User: Contractor2  Date: 3/02/10    Time: 11:37a
 * Updated in $/software/control processor/code/application
 * SCR# 472 - Fix file/Function headers, footer
 * 
 * *****************  Version 5  *****************
 * User: Contractor V&v Date: 2/03/10    Time: 2:56p
 * Updated in $/software/control processor/code/application
 * SCR 18
 * 
 * *****************  Version 4  *****************
 * User: John Omalley Date: 10/01/09   Time: 4:26p
 * Updated in $/software/control processor/code/application
 * Updated sensor indexes to new enumeration
 * 
 *
 *
 ***************************************************************************/

