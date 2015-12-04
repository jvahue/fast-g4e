#define DISPLAY_PROCESSING_USERTABLES_BODY
/******************************************************************************
            Copyright (C) 2007-2015 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    ECCN:        90991

    File:        DispProcessingUserTables.c

    Description: Routines to support the user commands for PWC Display Protocol
                 CSC

    VERSION
    $Revision: 2 $  $Date: 15-12-02 6:38p $

******************************************************************************/
#ifndef DISPLAY_PROCESSING_BODY
#error DispProcessingUserTables.c should only be included by DispProcessingApp.c
#endif

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "CfgManager.h"
#include "DispProcessingApp.h"


/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/


/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
static USER_HANDLER_RESULT DispProcessingApp_Status(USER_DATA_TYPE DataType,
                                                    USER_MSG_PARAM Param,
                                                    UINT32 Index,
                                                    const void *SetPtr,
                                                    void **GetPtr);

static USER_HANDLER_RESULT DispProcessingApp_Cfg(USER_DATA_TYPE DataType,
                                                 USER_MSG_PARAM Param,
                                                 UINT32 Index,
                                                 const void *SetPtr,
                                                 void **GetPtr);

static USER_HANDLER_RESULT DispProcessingApp_ShowConfig(USER_DATA_TYPE DataType,
                                                        USER_MSG_PARAM Param,
                                                        UINT32 Index,
                                                        const void *SetPtr,
                                                        void **GetPtr);

static USER_HANDLER_RESULT DispProcessingApp_Debug(USER_DATA_TYPE DataType,
                                                   USER_MSG_PARAM Param,
                                                   UINT32 Index,
                                                   const void *SetPtr,
                                                   void **GetPtr);
/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/

static DISPLAY_SCREEN_STATUS DispProcAppStatusTemp;
static DISPLAY_SCREEN_CONFIG DispProcAppCfgTemp;
static DISPLAY_DEBUG         DispProcAppDebugTemp;


/*****************************************/
/* User Table Definitions                */
/*****************************************/

static USER_MSG_TBL DispProcAppStatusTbl[] = 
{
  /*Str                Next Tbl Ptr   Handler Func.             Data Type          Access   Parameter                                       Index   DataLimit EnumTbl*/
  {"CURRENT_SCREEN",   NO_NEXT_TABLE, DispProcessingApp_Status, USER_TYPE_ENUM,    USER_RO, (void *) &DispProcAppStatusTemp.currentScreen,  -1, -1, NO_LIMIT, screenIndexType},
  {"PREVIOUS_SCREEN",  NO_NEXT_TABLE, DispProcessingApp_Status, USER_TYPE_ENUM,    USER_RO, (void *) &DispProcAppStatusTemp.previousScreen, -1, -1, NO_LIMIT, screenIndexType},
  {"NEXT_SCREEN",      NO_NEXT_TABLE, DispProcessingApp_Status, USER_TYPE_ENUM,    USER_RO, (void *) &DispProcAppStatusTemp.nextScreen,     -1, -1, NO_LIMIT, screenIndexType},
  {"D_HLTH_CODE",      NO_NEXT_TABLE, DispProcessingApp_Status, USER_TYPE_HEX8,    USER_RO, (void *) &DispProcAppStatusTemp.displayHealth,  -1, -1, NO_LIMIT, NULL},
  {"PBIT_COMPLETE",    NO_NEXT_TABLE, DispProcessingApp_Status, USER_TYPE_BOOLEAN, USER_RO, (void *) &DispProcAppStatusTemp.bPBITComplete,  -1, -1, NO_LIMIT, NULL},
  {"BUTTON_ENABLE",    NO_NEXT_TABLE, DispProcessingApp_Status, USER_TYPE_BOOLEAN, USER_RO, (void *) &DispProcAppStatusTemp.bButtonEnable,  -1, -1, NO_LIMIT, NULL},
  {"APAC_PROCESSING",  NO_NEXT_TABLE, DispProcessingApp_Status, USER_TYPE_BOOLEAN, USER_RO, (void *) &DispProcAppStatusTemp.bAPACProcessing,-1, -1, NO_LIMIT, NULL},
  {"BUTTON_INPUT",     NO_NEXT_TABLE, DispProcessingApp_Status, USER_TYPE_ENUM,    USER_RO, (void *) &DispProcAppStatusTemp.buttonInput,    -1, -1, NO_LIMIT, buttonIndexType},
  {"INVALID_BUTTON",   NO_NEXT_TABLE, DispProcessingApp_Status, USER_TYPE_BOOLEAN, USER_RO, (void *) &DispProcAppStatusTemp.bInvalidButton, -1, -1, NO_LIMIT, NULL},
  {"INVALID_D_HLTH",   NO_NEXT_TABLE, DispProcessingApp_Status, USER_TYPE_UINT32,  USER_RO, (void *) &DispProcAppStatusTemp.dispHealthTimer,-1, -1, NO_LIMIT, NULL},
  {"LAST_FRAME_TIME",  NO_NEXT_TABLE, DispProcessingApp_Status, USER_TYPE_UINT32,  USER_RO, (void *) &DispProcAppStatusTemp.lastFrameTime,  -1, -1, NO_LIMIT, NULL},
  {"DISP_PART_NUMBER", NO_NEXT_TABLE, DispProcessingApp_Status, USER_TYPE_HEX8,    USER_RO, (void *) &DispProcAppStatusTemp.partNumber,     -1, -1, NO_LIMIT, NULL},
  {"DISP_VERSION_NUM", NO_NEXT_TABLE, DispProcessingApp_Status, USER_TYPE_HEX8,    USER_RO, (void *) &DispProcAppStatusTemp.versionNumber,  -1, -1, NO_LIMIT, NULL},
  {"MSG_CONTENTS",     NO_NEXT_TABLE, DispProcessingApp_Status, USER_TYPE_STR,     USER_RO, (void *) &DispProcAppStatusTemp.charString,     -1, -1, NO_LIMIT, NULL},
  {NULL,               NULL,          NULL,                     NO_HANDLER_DATA}
};

static USER_MSG_TBL DispProcAppCfgTbl[] =
{
  /*Str                      Next Tbl Ptr   Handler Func.          Data Type         Access   Parameter                                          Index   DataLimit EnumTbl*/
  {"DEFAULT_DCRATE",         NO_NEXT_TABLE, DispProcessingApp_Cfg, USER_TYPE_INT8,   USER_RW, (void *) &DispProcAppCfgTemp.defaultDCRATE,        -1, -1, NO_LIMIT, NULL},
  {"INVALID_BUTTON_TIME_MS", NO_NEXT_TABLE, DispProcessingApp_Cfg, USER_TYPE_UINT32, USER_RW, (void *) &DispProcAppCfgTemp.invalidButtonTime_ms, -1, -1, NO_LIMIT, NULL},
  {"AUTO_ABORT_TIME_S",      NO_NEXT_TABLE, DispProcessingApp_Cfg, USER_TYPE_UINT32, USER_RW, (void *) &DispProcAppCfgTemp.autoAbortTime_s,      -1, -1, NO_LIMIT, NULL},
  {"NO_HS_TIMEOUT_S",        NO_NEXT_TABLE, DispProcessingApp_Cfg, USER_TYPE_UINT32, USER_RW, (void *) &DispProcAppCfgTemp.no_HS_Timeout_s,      -1, -1, NO_LIMIT, NULL},
  {NULL,                  NULL,          NULL,                  NO_HANDLER_DATA}
};

static USER_MSG_TBL DispProcAppDebugTbl[] =
{
  /*Str         Next Tbl Ptr   Handler Func.            Data Type          Access              Parameter                                  Index   DataLimit EnumTbl*/
  {"ENABLE",    NO_NEXT_TABLE, DispProcessingApp_Debug, USER_TYPE_BOOLEAN, (USER_RW|USER_GSE), (void *) &DispProcAppDebugTemp.bDebug,     -1, -1, NO_LIMIT, NULL},
  {NULL,        NULL,          NULL,                    NO_HANDLER_DATA}
};

static USER_MSG_TBL DispProcAppRoot[] = 
{
  /*Str         Next Tbl Ptr          Handler Func. Data Type*/
  {"STATUS",    DispProcAppStatusTbl, NULL,         NO_HANDLER_DATA},
  {"Debug",     DispProcAppDebugTbl,  NULL,         NO_HANDLER_DATA},
  {"CFG",       DispProcAppCfgTbl,    NULL,         NO_HANDLER_DATA},
  {DISPLAY_CFG, NO_NEXT_TABLE, DispProcessingApp_ShowConfig, USER_TYPE_ACTION, (USER_RO|USER_NO_LOG|USER_GSE), NULL, -1, -1, NO_LIMIT, NULL},
  {NULL,        NULL,                 NULL,         NO_HANDLER_DATA}
};

static USER_MSG_TBL DispProcAppRootTblPtr = 
{"DISPLAYAPP", DispProcAppRoot, NULL, NO_HANDLER_DATA};

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:    DispProcessingApp_Status
 *
 * Description: Called by the User.c module from the reference to this function
 *              in the user message tables above.
 *              Retrieves the latest Display Processing Application Status Data
 *
 *Parameters:   [in] DataType:  C type of the data to be read or changed, used
 *                              for casting the data pointers
 *              [in/out] Param: Pointer to the configuration item to be read
 *                              or changed
 *              [in] Index:     Index parameter is used to reference the
 *                              specific sensor to change. Range is validated
 *                              by the user manager
 *              [in] SetPtr:    For write commands, a pointer to the data to
 *                              write to the configuration
 *              [out] GetPtr:   For read commands, UserCfg function will set
 *                              this to the location of the data requested
 *
 * Returns:     USER_RESULT_OK:    Processed Successfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
******************************************************************************/
static
USER_HANDLER_RESULT DispProcessingApp_Status(USER_DATA_TYPE DataType,
                                             USER_MSG_PARAM Param,
                                             UINT32 Index,
                                             const void *SetPtr,
                                             void **GetPtr)
{
  USER_HANDLER_RESULT result ;

  result = USER_RESULT_OK;
  
  DispProcAppStatusTemp = *DispProcessingApp_GetStatus();
  
  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
  
  return result;
}

/******************************************************************************
 * Function:    DispProcessingApp_Cfg
 *
 * Description: Called by the User.c module from the reference to this function
 *              in the user message tables above.
 *              Retrieves and set the latest Display Processing App Cfg Data
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
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:       none
 *
*****************************************************************************/
static
USER_HANDLER_RESULT DispProcessingApp_Cfg(USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr)
{
  USER_HANDLER_RESULT result;
  
  result = USER_RESULT_OK;
  
  // Determine which array element
  memcpy(&DispProcAppCfgTemp, &CfgMgr_ConfigPtr()->DispProcAppConfig, 
         sizeof(DispProcAppCfgTemp));
         
  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
  
  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    memcpy(&CfgMgr_ConfigPtr()->DispProcAppConfig, &DispProcAppCfgTemp,
           sizeof(DISPLAY_SCREEN_CONFIG));
           
    CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(), 
                           &CfgMgr_ConfigPtr()->DispProcAppConfig,
                           sizeof(DISPLAY_SCREEN_CONFIG));
  }
  return(result);
}

/******************************************************************************
 * Function:    DispProcessingApp_Debug
 *
 * Description: Called by the User.c module from the reference to this function
 *              in the user message tables above.
 *              Retrieves the latest Display Processing App Debug Data
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
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
static 
USER_HANDLER_RESULT DispProcessingApp_Debug(USER_DATA_TYPE DataType,
                                            USER_MSG_PARAM Param,
                                            UINT32 Index,
                                            const void *SetPtr,
                                            void **GetPtr)
{
  USER_HANDLER_RESULT result;
  
  result = USER_RESULT_OK;
  
  DispProcAppDebugTemp = *DispProcessingApp_GetDebug();
  
  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
  
  *DispProcessingApp_GetDebug() = DispProcAppDebugTemp;
  
  return result;
}

/******************************************************************************
* Function:    DispProcessingApp_ShowConfig
*
* Description:  Handles User Manager requests to retrieve the configuration
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
*
* Returns:     USER_RESULT_OK:    Processed successfully
*              USER_RESULT_ERROR: Error processing command.
*
* Notes:
*****************************************************************************/
static USER_HANDLER_RESULT DispProcessingApp_ShowConfig(USER_DATA_TYPE DataType,
                                                        USER_MSG_PARAM Param,
                                                        UINT32 Index,
                                                        const void *SetPtr,
                                                        void **GetPtr)
{
  USER_HANDLER_RESULT result;
  CHAR label[USER_MAX_MSG_STR_LEN * 3];

  // Top level name is a single indented space
  CHAR branchName[USER_MAX_MSG_STR_LEN] = " ";

  USER_MSG_TBL *pCfgTable;

  snprintf(label, sizeof(label), "\r\n\r\nDISPPROCAPP.CFG");
  
  // Assume the worst possible result
  result = USER_RESULT_ERROR;
  
  if(User_OutputMsgString(label, FALSE))
  {
    pCfgTable = DispProcAppCfgTbl;

    result = User_DisplayConfigTree(branchName, pCfgTable, 0, 0, NULL);
  }

  return result;
}

/*****************************************************************************
 *  MODIFICATIONS
 *    $History: DispProcessingUserTables.c $
 * 
 * *****************  Version 2  *****************
 * User: Peter Lee    Date: 15-12-02   Time: 6:38p
 * Updated in $/software/control processor/code/application
 * SCR #1303 Display App Updates from Jeremy.  
 * 
 * *****************  Version 1  *****************
 * User: Jeremy Hester Date: 10/5/15    Time: 11:20p
 * Created in $/software/control processor/code/system
 * Initial Check In. Implementation of Display Processing App Requirements.
 *****************************************************************************/