/******************************************************************************
            Copyright (C) 2008-2011 Pratt & Whitney Engine Services, Inc.
                      Altair Engine Diagnostic Solutions
               All Rights Reserved. Proprietary and Confidential.

    File: BoxUserTables.c

    Description:

    VERSION
    $Revision: 28 $  $Date: 7/26/12 2:10p $


******************************************************************************/
#ifndef BOXMGR_BODY
#error BoxUserTables.c should only be included by Box.c
#endif

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include <stdlib.h>
#include <string.h>
#include "FastMgr.h"
#include "AircraftConfigMgr.h"
#include "CfgManager.h"
#include "MSStsCtl.h"
#include "FaultMgr.h"
#include "LogManager.h"
#include "UploadMgr.h"

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#ifndef INSTALL_STR_LIMIT
#define INSTALL_STR_LIMIT 0,TEXT_BUF_SIZE
#endif

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/


/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
extern UINT32        __CRC_END; // address provided by linker
extern USER_ENUM_TBL MSSC_MssimStatusEnum[];
extern FLT_STATUS    FaultSystemStatusTemp;

// Declare storage for values
static LOG_CONFIG        LogConfigTemp;
static AIRCRAFT_CONFIG   AircraftConfigTemp;
static FASTMGR_CONFIG    FASTConfigTemp;


/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/

USER_HANDLER_RESULT Box_BoxUserMsg(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr);

USER_HANDLER_RESULT Box_BoardUserMsg(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr);

USER_HANDLER_RESULT Box_SysTimeMsg(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr);

USER_HANDLER_RESULT Box_PowerOnMsg(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr);

USER_HANDLER_RESULT Box_VersionCmd(USER_DATA_TYPE DataType,
                                   USER_MSG_PARAM Param,
                                   UINT32 Index,
                                   const void *SetPtr,
                                   void **GetPtr);

USER_HANDLER_RESULT Box_InstallIdCmd(USER_DATA_TYPE DataType,
                                     USER_MSG_PARAM Param,
                                     UINT32 Index,
                                     const void *SetPtr,
                                     void **GetPtr);

USER_HANDLER_RESULT Box_FastUserCfg(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr);

USER_HANDLER_RESULT Box_AircraftUserCfg(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr);

USER_HANDLER_RESULT Box_LogGetStatus (USER_DATA_TYPE DataType,
                                      USER_MSG_PARAM Param,
                                      UINT32 Index,
                                      const void *SetPtr,
                                      void **GetPtr);

USER_HANDLER_RESULT Box_GetSystemStatus (USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr);

USER_HANDLER_RESULT Box_GetMssimStatus(USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr);

USER_HANDLER_RESULT Box_GetFastSwVer(USER_DATA_TYPE DataType,
                                     USER_MSG_PARAM Param,
                                     UINT32 Index,
                                     const void *SetPtr,
                                     void **GetPtr);

USER_HANDLER_RESULT Box_GetMsPwVer(USER_DATA_TYPE DataType,
                                   USER_MSG_PARAM Param,
                                   UINT32 Index,
                                   const void *SetPtr,
                                   void **GetPtr);

USER_HANDLER_RESULT Box_GetUlFilesPending(USER_DATA_TYPE DataType,
                                              USER_MSG_PARAM Param,
                                              UINT32 Index,
                                              const void *SetPtr,
                                              void **GetPtr);

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
    
// box.status implements an "aggregate" command which displays a subset of values from 
// system, Config, Fast, Aircraft, Log, Fault and Ms.
static USER_MSG_TBL BoxStatusMsgs[] =
{ /*Str                Next Tbl Ptr   Handler Func.         Data Type          Access    Parameter                                   IndexRange  DataLimit          EnumTbl*/
  // Box status values
  {"SERIAL",           NO_NEXT_TABLE, Box_BoxUserMsg,       USER_TYPE_STR,     USER_RO,  &Box_Info.SN,                               -1, -1,     BOX_STR_LIMIT,     NULL},
  {"TIME",             NO_NEXT_TABLE, Box_SysTimeMsg,       USER_TYPE_STR,     USER_RO,  NULL,                                       -1, -1,     NO_LIMIT,          NULL},
  {"TIME_SOURCE",      NO_NEXT_TABLE, Box_FastUserCfg,      USER_TYPE_ENUM,    USER_RO,  &FASTConfigTemp.TimeSource,                -1, -1,      NO_LIMIT,          TimeSourceStrs },
  {"CP_VER",           NO_NEXT_TABLE, Box_GetFastSwVer,     USER_TYPE_STR,     USER_RO,  NULL,                                      -1, -1,      NO_LIMIT,          NULL },
  {"CP_CRC",           NO_NEXT_TABLE, User_GenericAccessor, USER_TYPE_HEX32,   USER_RO,  &__CRC_END,                                -1, -1,      NO_LIMIT,          NULL },
  {"CFG_INSTALL_ID",   NO_NEXT_TABLE, Box_InstallIdCmd,     USER_TYPE_STR,     USER_RO,  NULL,                                      -1, -1,      INSTALL_STR_LIMIT, NULL },  
  {"CFG_VER",          NO_NEXT_TABLE, Box_VersionCmd,       USER_TYPE_UINT16,  USER_RO,  NULL,                                      -1, -1,      NO_LIMIT,          NULL },
  {"POWER_ON_COUNT"  , NO_NEXT_TABLE, Box_PowerOnMsg,       USER_TYPE_UINT32,  USER_RO,  &Box_PowerOn_RunTime.PowerOnCount,          -1, -1,     NO_LIMIT,          NULL },
  {"POWER_ON_TIME_S",  NO_NEXT_TABLE, Box_PowerOnMsg,       USER_TYPE_UINT32,  USER_RO,  &Box_PowerOn_RunTime.ElapsedPowerOnUsage_s, -1, -1,     NO_LIMIT,          NULL },
  {"PW_VER",           NO_NEXT_TABLE, Box_GetMsPwVer,       USER_TYPE_STR,     USER_RO,  NULL,                                      -1, -1,      NO_LIMIT,          NULL },
  //aircraft.cfg
  {"AC_TAIL_NUMBER",   NO_NEXT_TABLE, Box_AircraftUserCfg,  USER_TYPE_STR,     USER_RO,  &AircraftConfigTemp.TailNumber,            -1, -1,      AC_STR_LIMIT,      NULL }, 
  {"AC_OPERATOR",      NO_NEXT_TABLE, Box_AircraftUserCfg,  USER_TYPE_STR,     USER_RO,  &AircraftConfigTemp.Operator,              -1, -1,      AC_STR_LIMIT,      NULL },
  {"AC_OWNER",         NO_NEXT_TABLE, Box_AircraftUserCfg,  USER_TYPE_STR,     USER_RO,  &AircraftConfigTemp.Owner,                 -1, -1,      AC_STR_LIMIT,      NULL },
   //log.status
  {"LOG_COUNT",        NO_NEXT_TABLE, Box_LogGetStatus,     USER_TYPE_UINT32,  USER_RO,  &LogConfigTemp.nLogCount,                  -1, -1,      NO_LIMIT,          NULL },
  {"LOG_MEM_USED_PCT", NO_NEXT_TABLE, Box_LogGetStatus,     USER_TYPE_FLOAT,   USER_RO,  &LogConfigTemp.fPercentUsage,              -1, -1,      NO_LIMIT,          NULL },
  {"UNTXD_FILES",      NO_NEXT_TABLE, Box_GetUlFilesPending, USER_TYPE_UINT32, USER_RO,  NULL,                                      -1, -1,      NO_LIMIT,          NULL },
  //fault.status
  {"SYS_CONDITION",    NO_NEXT_TABLE, Box_GetSystemStatus,  USER_TYPE_ENUM,    USER_RO,  NULL,                                      -1, -1,      NO_LIMIT,          Flt_UserEnumStatus },
  //ms.status
  {"MS_RDY",           NO_NEXT_TABLE, Box_GetMssimStatus,   USER_TYPE_ENUM,    USER_RO,  NULL,                                      -1, -1,      NO_LIMIT,          MSSC_MssimStatusEnum },
  {NULL              , NULL         , NULL,                 NO_HANDLER_DATA }
};

static USER_MSG_TBL BoxBoardMsgs[] =
{/* Str       Next Tbl Ptr   Handler Func.       Data Type       Access                 Parameter                   IndexRange                DataLimit       EnumTbl*/
  { "NAME"  , NO_NEXT_TABLE, Box_BoardUserMsg  , USER_TYPE_STR,  USER_RW | USER_FACT,   &Box_Info.Board[0].Name   , 0 , BOX_NUM_OF_BOARDS-1,  BOX_STR_LIMIT,  NULL },
  { "SN"    , NO_NEXT_TABLE, Box_BoardUserMsg  , USER_TYPE_STR,  USER_RW | USER_FACT,   &Box_Info.Board[0].SN     , 0 , BOX_NUM_OF_BOARDS-1,  BOX_STR_LIMIT,  NULL },
  { "REV"   , NO_NEXT_TABLE, Box_BoardUserMsg  , USER_TYPE_STR,  USER_RW | USER_FACT,   &Box_Info.Board[0].Rev    , 0 , BOX_NUM_OF_BOARDS-1,  BOX_STR_LIMIT,  NULL },
  { NULL    , NULL,          NULL              , NO_HANDLER_DATA }
};

static USER_MSG_TBL BoxMsgs[] =
{/* Str                Next Tbl Ptr   Handler Func.   Data Type          Access                Parameter                                   IndexRange    DataLimit       EnumTbl */
  {"STATUS",           BoxStatusMsgs, NULL,           NO_HANDLER_DATA},
  {"SERIAL"          , NO_NEXT_TABLE, Box_BoxUserMsg, USER_TYPE_STR,     USER_RW | USER_FACT,  &Box_Info.SN,                               -1, -1,       BOX_STR_LIMIT,  NULL},
  {"REV"             , NO_NEXT_TABLE, Box_BoxUserMsg, USER_TYPE_STR,     USER_RW | USER_FACT,  &Box_Info.Rev,                              -1, -1,       BOX_STR_LIMIT,  NULL},
  {"MFG"             , NO_NEXT_TABLE, Box_BoxUserMsg, USER_TYPE_STR,     USER_RW | USER_FACT,  &Box_Info.MfgDate,                          -1, -1,       BOX_STR_LIMIT,  NULL},
  {"LAST_REPAIR"     , NO_NEXT_TABLE, Box_BoxUserMsg, USER_TYPE_STR,     USER_RW | USER_FACT,  &Box_Info.RepairDate,                       -1, -1,       BOX_STR_LIMIT,  NULL},
  {"TIME"            , NO_NEXT_TABLE, Box_SysTimeMsg, USER_TYPE_STR,     USER_RW,              NULL,                                       -1, -1,       NO_LIMIT,       NULL},
  {"BOARD"           , BoxBoardMsgs , NULL          , NO_HANDLER_DATA},
  {"POWER_ON_USAGE_S", NO_NEXT_TABLE, Box_PowerOnMsg, USER_TYPE_UINT32,  USER_RW | USER_FACT,  &Box_PowerOn_RunTime.ElapsedPowerOnUsage_s, -1, -1,       NO_LIMIT,       NULL},
  {"POWER_ON_COUNT"  , NO_NEXT_TABLE, Box_PowerOnMsg, USER_TYPE_UINT32,  USER_RW | USER_FACT,  &Box_PowerOn_RunTime.PowerOnCount,          -1, -1,       NO_LIMIT,       NULL},
  {NULL              , NULL         , NULL          , NO_HANDLER_DATA}
};

USER_MSG_TBL BoxRoot   = {"BOX",BoxMsgs,NULL,NO_HANDLER_DATA};




/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/
/******************************************************************************
 * Function:    Box_BoxUserMsg
 *
 * Description: Handles user manager requests for BOX_UNIT_INFO
 *
 * Parameters:  USER_DATA_TYPE DataType
 *              USER_MSG_PARAM Param
 *              UINT32 Index
 *              const void *SetPtr
 *              void **GetPtr
 *
 * Returns:     USER_HANDLER_RESULT
 *
 * Notes:
 *
 *****************************************************************************/
USER_HANDLER_RESULT Box_BoxUserMsg(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr)
{ 
  if(SetPtr == NULL)
  {
    //Read into the global structure from NV memory, Return the parameter
    //to user manager.
    NV_Read(NV_BOX_CFG,0,&Box_Info,sizeof(Box_Info));    
    *GetPtr = Param.Ptr;    
  }
  else
  {
    //Copy the new data into the global struct and write it to NV memory
    strncpy_safe(Param.Ptr,BOX_INFO_STR_LEN, SetPtr, _TRUNCATE );
    NV_Write(NV_BOX_CFG,0,&Box_Info,sizeof(Box_Info));       
  }

  return USER_RESULT_OK;
}



/******************************************************************************
 * Function:    Box_BoardUserMsg
 *
 * Description: Handles user manager requests for BOX_UNIT_INFO's 
 *              BOX_BOARD_INFO member.
 *
 * Parameters:  USER_DATA_TYPE DataType
 *              USER_MSG_PARAM Param
 *              UINT32 Index
 *              const void *SetPtr
 *              void **GetPtr
 *
 * Returns:     USER_HANDLER_RESULT
 *
 * Notes:
 *
 *****************************************************************************/
USER_HANDLER_RESULT Box_BoardUserMsg(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr)
{

  //Offsets of each member of .Board is relative to Box_Info.Board[0].
  //Offset by the user's entered index to get the .Board[Index]
  Param.Ptr = (char*)Param.Ptr + sizeof(Box_Info.Board[0]) * Index;

  if(SetPtr == NULL)
  {
    //Read into the global structure from NV memory, Return the parameter
    //to user manager.
    NV_Read(NV_BOX_CFG,0,&Box_Info,sizeof(Box_Info));    
    *GetPtr = Param.Ptr;   
  }
  else
  {
    //Copy the new data into the global struct and write it to NV memory
    //Offsets of each member of .Board is relative to Box_Info.Board[0].
    //Offset by the user's entered index to get the .Board[Index]
    strncpy_safe(Param.Ptr,BOX_INFO_STR_LEN, SetPtr, _TRUNCATE );
    NV_Write(NV_BOX_CFG,0,&Box_Info,sizeof(Box_Info));    
  }
  return USER_RESULT_OK;
}



/******************************************************************************
 * Function:    Box_SysTimeMsg
 *
 * Description: Set the system and rtc time and date.  This User Handler
 *              validates the time/date with a basic check to ensure
 *              all time and date parameters are entered, and each is within
 *              a valid range.
 *              A system log APP_FM_INFO_USER_UPDATED_CLOCK is written when the
 *              time is sucessfully updated
 *
 * Parameters:  USER_DATA_TYPE DataType
 *              USER_MSG_PARAM Param
 *              UINT32 Index
 *              const void *SetPtr
 *              void **GetPtr
 *
 * Returns:     USER_HANDLER_RESULT
 *
 * Notes:
 *
 *****************************************************************************/
USER_HANDLER_RESULT Box_SysTimeMsg(USER_DATA_TYPE DataType,
                                   USER_MSG_PARAM Param,
                                   UINT32 Index,
                                   const void *SetPtr,
                                   void **GetPtr)
{
  INT8 i;
  CHAR *value;
  TIMESTRUCT Time;
  static CHAR TimeStr[80]; // TimeStr needs to be static since,
                           // GetPtr will hold a ref to it on return.

  CHAR *TokPtr = NULL;
  CHAR delims[] = "/ :";
  USER_HANDLER_RESULT UserResult = USER_RESULT_ERROR;
  UINT32 intrLevel;

  if(SetPtr == NULL)
  {
    CM_GetSystemClock(&Time);

    snprintf(TimeStr,80,"%02d/%02d/%04d %02d:%02d:%02d.%03d",
          Time.Month,
          Time.Day,
          Time.Year,
          Time.Hour,
          Time.Minute,
          Time.Second,
          Time.MilliSecond);

    *GetPtr = TimeStr;
     UserResult = USER_RESULT_OK;

  }
  else
  {
    UINT16* timePart[6];

    timePart[0] = &Time.Month;
    timePart[1] = &Time.Day;
    timePart[2] = &Time.Year;
    timePart[3] = &Time.Hour;
    timePart[4] = &Time.Minute;
    timePart[5] = &Time.Second;

    // if the entire structure is not filled in
    // this will ensure the validity checks fail
    memset( &Time, 0xFF, sizeof(Time));

    strncpy_safe(TimeStr, 80,(INT8*)SetPtr, _TRUNCATE);

    value = strtok_r(TimeStr, delims, &TokPtr);
    for (i=0; i < 6 && value != NULL; ++i)
    {
      *timePart[i] = (UINT16)atoi( value);
      value = strtok_r( NULL, delims, &TokPtr);
    }

    Time.MilliSecond = 0;

    if( ((Time.Second   <= 59))  &&
        ((Time.Minute   <= 59))  &&
        ((Time.Hour     <= 23))  &&
        ((Time.Day      <= 31))  &&
        ((Time.Month    <= 12))  &&
        ((Time.Year     >= 2000) && (Time.Year <= MAX_YEAR) ) )
    {
      intrLevel = __DIR();
      CM_SetSystemClock(&Time, FALSE);
      CM_SetRTCClock(&Time, FALSE);
      __RIR(intrLevel);
      //Write a time change log
      LogWriteSystem(APP_FM_INFO_USER_UPDATED_CLOCK,LOG_PRIORITY_LOW,NULL,0,0);
      UserResult = USER_RESULT_OK;
    }
    else
    {
      GSE_DebugStr( NORMAL, FALSE, "Expected 'MM/DD/YYYY HH:MM:SS'"NL);
    }
  }

  return UserResult;
}


/******************************************************************************
 * Function:    Box_PowerOnMsg
 *
 * Description: Accessing Power On Count Values
 *
 * Parameters:  USER_DATA_TYPE DataType
 *              USER_MSG_PARAM Param
 *              UINT32 Index
 *              const void *SetPtr
 *              void **GetPtr
 *
 * Returns:     USER_HANDLER_RESULT
 *
 * Notes:       none
 *
 *****************************************************************************/
USER_HANDLER_RESULT Box_PowerOnMsg(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr)
{
 //USER_HANDLER_RESULT UserResult = USER_RESULT_ERROR;
 //UINT32 PowerOnUsage;
 UINT32 TickCount;

  if(SetPtr == NULL)
  {
    // If Power On Usage, we need to calculate current elapsed power on usage
    if ( Param.Ptr == (void *) &Box_PowerOn_RunTime.ElapsedPowerOnUsage_s )
    {
      // Call _UpdateElapsedUsage to update ->ElapsedPowerOnUsage
      //   to current box usage
      Box_PowerOn_UpdateElapsedUsage ( FALSE, POWERCOUNT_UPDATE_BACKGROUND,
                                       NV_PWR_ON_CNTS_RTC );
      Box_PowerOn_UpdateElapsedUsage ( FALSE, POWERCOUNT_UPDATE_BACKGROUND,
                                       NV_PWR_ON_CNTS_EE );
    }

    *GetPtr = Param.Ptr;

  }
  else
  {
    // If Power On Usage, we need to update some internal variables
    if ( Param.Ptr == (void *) &Box_PowerOn_RunTime.ElapsedPowerOnUsage_s )
    {
      TickCount = CM_GetTickCount();

      Box_PowerOn_RunTime.PrevPowerOnUsage = *(UINT32*)SetPtr;

      // Update UserSetAdjustment so when "box.poweronusage" is read again, it will be =
      //    user set value + time since user updated box.poweronusage.
      Box_PowerOn_RunTime.UserSetAdjustment = TickCount + 
                                              Box_PowerOn_RunTime.CPU_StartupBeforeClockInit;

      // Update the non-volatile versions (and indirectly .ElapsedPowerOnUsage var),
      //    based on update .PrevPowerOnUsage and .UsersetAdjustment.
      Box_PowerOn_UpdateElapsedUsage ( FALSE, POWERCOUNT_UPDATE_BACKGROUND,
                                       NV_PWR_ON_CNTS_RTC );
      Box_PowerOn_UpdateElapsedUsage ( FALSE, POWERCOUNT_UPDATE_BACKGROUND,
                                       NV_PWR_ON_CNTS_EE );
    }

    *(UINT32*)Param.Ptr = *(UINT32*)SetPtr;

  }

  //return UserResult;
  return USER_RESULT_OK;

}

/******************************************************************************
 * Function:    Box_VersionCmd()
 *  
 * Description:  Handles User Manager request to access the Version ID
 *               setting. 
 *               
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used 
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                               or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific sensor to change.  Range is validated
 *                               by the user manager
 *               [in] SetPtr:    For write commands, a pointer to the data to
 *                               write to the configuration.
 *               [out] GetPtr:   For read commands, UserCfg function will set
 *                               this to the location of the data requested.
 *
 * Returns:     USER_RESULT_OK:    Processed successfully
 *              USER_RESULT_ERROR: Error processing command.       
 *
 * Notes: Same as FAST_VersionCmd modified to only implement "get" behavior
 *
 *****************************************************************************/
USER_HANDLER_RESULT Box_VersionCmd(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr)
{
  UINT16 Ver;
  
  //Load Version Id into the temporary location
  memcpy(&Ver, &CfgMgr_ConfigPtr()->VerId,  sizeof(Ver));

  // Only implement the "get" command
  if( SetPtr == NULL )
  {
    **(UINT16**)GetPtr = Ver;
  }

  return USER_RESULT_OK;
}


/******************************************************************************
 * Function: Box_InstallIdCmd    
 *
 * Description:  Handles User Manager request to access the Installation ID
 *               setting. 
 *               
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used 
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                               or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific sensor to change.  Range is validated
 *                               by the user manager
 *               [in] SetPtr:    For write commands, a pointer to the data to
 *                               write to the configuration.
 *               [out] GetPtr:   For read commands, UserCfg function will set
 *                               this to the location of the data requested.
 *
 * Returns:     USER_RESULT_OK:    Processed successfully
 *              USER_RESULT_ERROR: Error processing command.       
 *
 * Notes:     Same as FAST_InstallIdCmd modified to only implement "get" behavior   
 *****************************************************************************/
USER_HANDLER_RESULT Box_InstallIdCmd(USER_DATA_TYPE DataType,
                                     USER_MSG_PARAM Param,
                                     UINT32 Index,
                                     const void *SetPtr,
                                     void **GetPtr)
{
  USER_HANDLER_RESULT UserResult = USER_RESULT_ERROR;  

  // If "get", set the get ptr to the address of the Installation id string
  // in the config manager structure.
  if( SetPtr == NULL )
  {
    *GetPtr = &CfgMgr_ConfigPtr()->Id;
    UserResult = USER_RESULT_OK;
  }  
  return UserResult;
}

/******************************************************************************
 * Function:    Box_FastUserCfg()
 *  
 * Description:  Handles User Manager request to access the Fast config
 *               settings. 
 *               
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used 
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                               or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific sensor to change.  Range is validated
 *                               by the user manager
 *               [in] SetPtr:    For write commands, a pointer to the data to
 *                               write to the configuration.
 *               [out] GetPtr:   For read commands, UserCfg function will set
 *                               this to the location of the data requested.
 *
 * Returns:     USER_RESULT_OK:    Processed successfully
 *              USER_RESULT_ERROR: Error processing command.       
 *
 * Notes: Same as FAST_UserCfg modified to only implement "get" behavior
 *
 *****************************************************************************/
USER_HANDLER_RESULT Box_FastUserCfg(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_ERROR;

  //Load trigger structure into the temporary location based on index param
  //Param.Ptr points to the struct member to be read/written
  //in the temporary location
  memcpy(&FASTConfigTemp, &CfgMgr_ConfigPtr()->FASTCfg, sizeof(FASTConfigTemp));
  if (SetPtr == NULL)
  {
    result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
  }
  return result;  
}

/******************************************************************************
 * Function:     Box_AircraftUserCfg
 *
 * Description:  Handles User Manager request to access the Fast config
 *               settings. 
 *               
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used 
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                               or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific sensor to change.  Range is validated
 *                               by the user manager
 *               [in] SetPtr:    For write commands, a pointer to the data to
 *                               write to the configuration.
 *               [out] GetPtr:   For read commands, UserCfg function will set
 *                               this to the location of the data requested.
 *
 * Returns:     USER_RESULT_OK:    Processed successfully
 *              USER_RESULT_ERROR: Error processing command.       
 *
 * Notes: Same as AircraftUserCfg modified to only implement "get" behavior
 *
 *****************************************************************************/
USER_HANDLER_RESULT Box_AircraftUserCfg(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_OK;

  memcpy(&AircraftConfigTemp, &CfgMgr_ConfigPtr()->Aircraft, sizeof(AircraftConfigTemp));

  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);  
  
  return result;
}

/******************************************************************************
 * Function:     Box_LogGetStatus
 *
 * Description:  Handles User Manager requests to retrieve the log
 *               status.  
 *               
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used 
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                               or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific sensor to change.  Range is validated
 *                               by the user manager
 *               [in] SetPtr:    For write commands, a pointer to the data to
 *                               write to the configuration.
 *               [out] GetPtr:   For read commands, UserCfg function will set
 *                               this to the location of the data requested.
 *
 * Returns:      USER_HANDLER_RESULT - USER_RESULT_ERROR or USER_RESULT_OK
 *
 * Notes:        Same as Log_GetStatus defined for box.status
 *****************************************************************************/
USER_HANDLER_RESULT Box_LogGetStatus (USER_DATA_TYPE DataType,
                                   USER_MSG_PARAM Param,
                                   UINT32 Index,
                                   const void *SetPtr,
                                   void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_ERROR;
  memcpy(&LogConfigTemp, LogGetConfigPtr(), sizeof(LogConfigTemp) );
  
  if(SetPtr == NULL)
  {
    //Set the GetPtr to the Cfg member referenced in the table
    *GetPtr = Param.Ptr;
    result = USER_RESULT_OK;
  }

  return result;
}


/******************************************************************************
 * Function:     Box_GetSystemStatus
 *
 * Description:  Handles User Manager requests to retrieve the log
 *               status.  
 *               
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used 
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                                or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific sensor to change.  Range is validated
 *                               by the user manager
 *               [in] SetPtr:    For write commands, a pointer to the data to
 *                               write to the configuration.
 *               [out] GetPtr:   For read commands, UserCfg function will set
 *                               this to the location of the data requested.
 *
 * Returns:      USER_HANDLER_RESULT - USER_RESULT_ERROR or USER_RESULT_OK
 *
 * Notes:         
 *****************************************************************************/
USER_HANDLER_RESULT Box_GetSystemStatus (USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                          const void *SetPtr,
                                         void **GetPtr)
{
  static FLT_STATUS FaultSystemStatusTemp;
  USER_HANDLER_RESULT result = USER_RESULT_ERROR;

  if(SetPtr == NULL)
  {
    // Get the sys condition to the param variable
    FaultSystemStatusTemp = Flt_GetSystemStatus();
    
    //Set the GetPtr to the temp variable.     
    *GetPtr = &FaultSystemStatusTemp;
    result = USER_RESULT_OK;
  }
  return result;
}


/******************************************************************************
* Function:     Box_GetMssimStatus
*
* Description:  Handles User Manager requests to retrieve the MSSMIM connect status
*               status.  
*               
* Parameters:   [in] DataType:  C type of the data to be read or changed, used 
*                               for casting the data pointers
*               [in/out] Param: Pointer to the configuration item to be read
*                                or changed
*               [in] Index:     Index parameter is used to reference the
*                               specific sensor to change.  Range is validated
*                               by the user manager
*               [in] SetPtr:    For write commands, a pointer to the data to
*                               write to the configuration.
*               [out] GetPtr:   For read commands, UserCfg function will set
*                               this to the location of the data requested.
*
* Returns:      USER_HANDLER_RESULT - USER_RESULT_ERROR or USER_RESULT_OK
*
* Notes:         
*****************************************************************************/
USER_HANDLER_RESULT Box_GetMssimStatus(USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr)
{
  static MSSC_MSSIM_STATUS MssimStatusTemp;
  USER_HANDLER_RESULT result = USER_RESULT_ERROR;
 
  if(SetPtr == NULL)
  {
    // Set the mssim status to the temp variable
    MssimStatusTemp = MSSC_GetMsStatus();

    //Set the GetPtr to the var referenced in the table     
    *GetPtr = &MssimStatusTemp;
    result = USER_RESULT_OK;
  }
  return result;
}


/******************************************************************************
 * Function:     Box_GetFastSwVer
 *
 * Description:  Handles User Manager requests to retrieve the software version.  
 *               
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used 
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                                or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific sensor to change.  Range is validated
 *                               by the user manager
 *               [in] SetPtr:    For write commands, a pointer to the data to
 *                               write to the configuration.
 *               [out] GetPtr:   For read commands, UserCfg function will set
 *                               this to the location of the data requested.
 *
 * Returns:      USER_HANDLER_RESULT - USER_RESULT_ERROR or USER_RESULT_OK
 *
 * Notes:         
 *****************************************************************************/
USER_HANDLER_RESULT Box_GetFastSwVer(USER_DATA_TYPE DataType,
                                     USER_MSG_PARAM Param,
                                     UINT32 Index,
                                     const void *SetPtr,
                                     void **GetPtr)
{
  static CHAR SwVersionTemp[SW_VERSION_LEN];
  USER_HANDLER_RESULT result = USER_RESULT_ERROR;

  if(SetPtr == NULL)
  {
    // Get the software version string
    FAST_GetSoftwareVersion(SwVersionTemp);
    //Set the GetPtr to the var referenced in the table     
    *GetPtr = &SwVersionTemp;
    result = USER_RESULT_OK;
  }
  return result;
}


/******************************************************************************
 * Function:     Box_GetMsPwVer
 *
 * Description:  Handles User Manager requests to retrieve the MS PW version.  
 *               
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used 
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                                or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific sensor to change.  Range is validated
 *                               by the user manager
 *               [in] SetPtr:    For write commands, a pointer to the data to
 *                               write to the configuration.
 *               [out] GetPtr:   For read commands, UserCfg function will set
 *                               this to the location of the data requested.
 *
 * Returns:      USER_HANDLER_RESULT - USER_RESULT_ERROR or USER_RESULT_OK
 *
 * Notes:         
 *****************************************************************************/
USER_HANDLER_RESULT Box_GetMsPwVer(USER_DATA_TYPE DataType,
                                   USER_MSG_PARAM Param,
                                   UINT32 Index,
                                   const void *SetPtr,
                                   void **GetPtr)
{
  static CHAR MsPwVersion[MSSC_CFG_STR_MAX_LEN];
  USER_HANDLER_RESULT result = USER_RESULT_ERROR;

  if(SetPtr == NULL)
  {
    // Get the PW version string
    MSSC_GetMsPwVer(MsPwVersion);
    //Set the GetPtr to the var referenced in the table     
    *GetPtr = &MsPwVersion;
    result = USER_RESULT_OK;
  }
  return result;
}


/******************************************************************************
 * Function:     Box_GetUlFilesPending
 *
 * Description:  Handles User Manager requests to retrieve number of files pending transmission.
 *               
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used 
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                                or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific sensor to change.  Range is validated
 *                               by the user manager
 *               [in] SetPtr:    For write commands, a pointer to the data to
 *                               write to the configuration.
 *               [out] GetPtr:   For read commands, UserCfg function will set
 *                               this to the location of the data requested.
 *
 * Returns:      USER_HANDLER_RESULT - USER_RESULT_ERROR or USER_RESULT_OK
 *
 * Notes:         
 *****************************************************************************/
USER_HANDLER_RESULT Box_GetUlFilesPending(USER_DATA_TYPE DataType,
                                              USER_MSG_PARAM Param,
                                              UINT32 Index,
                                              const void *SetPtr,
                                              void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_ERROR;

  if(SetPtr == NULL)
  {
    **(UINT32**)GetPtr = UploadMgr_GetFilesPendingRT();
    result = USER_RESULT_OK;
  }
  return result;
}


/*************************************************************************
 *  MODIFICATIONS
 *    $History: BoxUserTables.c $
 * 
 * *****************  Version 28  *****************
 * User: Jim Mood     Date: 7/26/12    Time: 2:10p
 * Updated in $/software/control processor/code/system
 * SCR 1076 Code review changes
 * 
 * *****************  Version 27  *****************
 * User: John Omalley Date: 10/11/11   Time: 4:58p
 * Updated in $/software/control processor/code/system
 * SCR 1078 - Code Review Updates
 * 
 * *****************  Version 26  *****************
 * User: Contractor2  Date: 6/29/11    Time: 3:20p
 * Updated in $/software/control processor/code/system
 * SCR #1031 Enhancement: Add PW_Version to box.status
 * Moved variables from file to function scope
 * 
 * *****************  Version 25  *****************
 * User: Contractor2  Date: 5/23/11    Time: 1:02p
 * Updated in $/software/control processor/code/system
 * SCR #862 Enhancement: Box Status Information
 * 
 * *****************  Version 24  *****************
 * User: Contractor2  Date: 5/18/11    Time: 2:38p
 * Updated in $/software/control processor/code/system
 * SCR #1031 Enhancement: Add PW_Version to box.status
 * 
 * *****************  Version 23  *****************
 * User: Peter Lee    Date: 8/30/10    Time: 4:02p
 * Updated in $/software/control processor/code/system
 * SCR #806 Code Review Updates
 * 
 * *****************  Version 22  *****************
 * User: Jeff Vahue   Date: 8/18/10    Time: 6:22p
 * Updated in $/software/control processor/code/system
 * SCR# 801 - Add NL to error message for bad box time
 * 
 * *****************  Version 21  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:13a
 * Updated in $/software/control processor/code/system
 * SCR #703 - Fixes for code review findings.
 * 
 * *****************  Version 20  *****************
 * User: John Omalley Date: 7/27/10    Time: 6:55p
 * Updated in $/software/control processor/code/system
 * SCR 260 - Corrected the User commands to match the GSE.
 * 
 * *****************  Version 19  *****************
 * User: Peter Lee    Date: 7/22/10    Time: 5:48p
 * Updated in $/software/control processor/code/system
 * SCR #582   Update Power On Usage to better measure processor init time.
 * Fix bug when transitioning to use CM_GetClock(). 
 * 
 * *****************  Version 18  *****************
 * User: Contractor V&v Date: 7/21/10    Time: 7:16p
 * Updated in $/software/control processor/code/system
 * SCR #260 SCR #538
 * 
 * *****************  Version 17  *****************
 * User: Contractor3  Date: 6/10/10    Time: 11:14a
 * Updated in $/software/control processor/code/system
 * SCR #642 - Code Review changes
 * 
 * *****************  Version 16  *****************
 * User: Contractor3  Date: 6/10/10    Time: 10:44a
 * Updated in $/software/control processor/code/system
 * SCR #642 - Changes based on Code Review
 * 
 * *****************  Version 15  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:09p
 * Updated in $/software/control processor/code/system
 * SCR #70 Store/Restore interrupt level
 * 
 * *****************  Version 14  *****************
 * User: Contractor V&v Date: 3/29/10    Time: 6:17p
 * Updated in $/software/control processor/code/system
 * SCR #512 Password cannot be visible/reverse-eng
 *
 * *****************  Version 13  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:36p
 * Updated in $/software/control processor/code/system
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 *
 * *****************  Version 12  *****************
 * User: Contractor V&v Date: 3/19/10    Time: 4:30p
 * Updated in $/software/control processor/code/system
 * SCR #279 Making NV filenames uppercase
 *
 * *****************  Version 11  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/system
 * SCR# 483 - Function Names
 *
 * *****************  Version 10  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 3:56p
 * Updated in $/software/control processor/code/system
 * SCR #67 Interrupted SPI Access changed TimeStruct to static
 *
 * *****************  Version 9  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:57p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 8  *****************
 * User: Jeff Vahue   Date: 2/17/10    Time: 1:24p
 * Updated in $/software/control processor/code/system
 * SCR# 364 - remove unused code
 *
 * *****************  Version 7  *****************
 * User: Jeff Vahue   Date: 2/11/10    Time: 6:17p
 * Updated in $/software/control processor/code/system
 * SCR# 350 - cleanup a GHS compiler issue
 *
 * *****************  Version 6  *****************
 * User: Jeff Vahue   Date: 2/11/10    Time: 4:47p
 * Updated in $/software/control processor/code/system
 * SCR# 448 - remove box.dbg.erase_ee_dev cmd
 *
 * *****************  Version 5  *****************
 * User: Jeff Vahue   Date: 2/11/10    Time: 1:42p
 * Updated in $/software/control processor/code/system
 * SCR# 350 - Item 2 (Fix box.time) to not call atoi with NULL ptr
 *
 * *****************  Version 4  *****************
 * User: Contractor V&v Date: 2/03/10    Time: 2:57p
 * Updated in $/software/control processor/code/system
 * SCR 279, SCR 18
 *
 * *****************  Version 3  *****************
 * User: Peter Lee    Date: 12/17/09   Time: 1:38p
 * Updated in $/software/control processor/code/system
 * Requirements 3046, 3047 and 2215
 *
 * *****************  Version 2  *****************
 * User: Contractor V&v Date: 12/10/09   Time: 5:42p
 * Updated in $/software/control processor/code/system
 * SCR 106
 *
 * *****************  Version 1  *****************
 * User: Jim Mood     Date: 2/05/09    Time: 11:30a
 * Created in $/control processor/code/system
 *
 * *****************  Version 1  *****************
 * User: Peter Lee    Date: 10/08/08   Time: 11:45a
 * Created in $/control processor/code/system
 * Initial Check In
 *
 *
 ***************************************************************************/


