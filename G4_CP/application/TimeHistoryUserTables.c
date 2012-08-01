/******************************************************************************
         Copyright (C) 2012 Pratt & Whitney Engine Services, Inc. 
                     Altair Engine Diagnostic Solutions
            All Rights Reserved. Proprietary and Confidential.

  File:        TimeHistoryUserTables.c 

Description:   User command structures and functions for the event processing

VERSION
$Revision: 1 $  $Date: 4/24/12 10:59a $    
******************************************************************************/
#ifndef TIMEHISTORY_BODY
#error TimeHistoryUserTables.c should only be included by TimeHistory.c
#endif

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static TIMEHISTORY_CFG   ConfigTimeHistoryTemp;// Configuration temp Storage

static TIMEHISTORY_DATA  StateTimeHistoryTemp; // State Temp Storage

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
//Prototype for the User Manager message handlers, has to go before
//the local variable tables that use the function pointer.
USER_HANDLER_RESULT TimeHistory_UserCfg ( USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr );

USER_HANDLER_RESULT TimeHistory_State   ( USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr );

USER_HANDLER_RESULT TimeHistory_ShowCfg ( USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr );

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
USER_ENUM_TBL TimeHistoryRateType[]   =  { { "1HZ"      , TH_1HZ          },
                                           { "2HZ"      , TH_2HZ          }, 
                                           { "4HZ"      , TH_4HZ          }, 
                                           { "5HZ"      , TH_5HZ          }, 
                                           { "10HZ"     , TH_10HZ         }, 
                                           { NULL       , 0               }
                                         };
  
// Events - EVENT User and Configuration Table
static USER_MSG_TBL TimeHistoryCmd [] =
{
  /* Str              Next Tbl Ptr               Handler Func.          Data Type          Access     Parameter                           IndexRange           DataLimit            EnumTbl*/
  { "ENABLE",         NO_NEXT_TABLE,            TimeHistory_UserCfg,    USER_TYPE_BOOLEAN, USER_RW,   &ConfigTimeHistoryTemp.Enable,      -1,-1,               NO_LIMIT,            NULL               },
  { "RATE",           NO_NEXT_TABLE,            TimeHistory_UserCfg,    USER_TYPE_ENUM,    USER_RW,   &ConfigTimeHistoryTemp.Rate,        -1,-1,               NO_LIMIT,            NULL               },
  { "RATEOFFSET_MS",  NO_NEXT_TABLE,            TimeHistory_UserCfg,    USER_TYPE_UINT32,  USER_RW,   &ConfigTimeHistoryTemp.nOffset_ms,  -1,-1,               NO_LIMIT,            NULL               },
  { NULL,             NULL,                     NULL,                   NO_HANDLER_DATA }
};

static USER_MSG_TBL TimeHistoryStatus [] =
{
  /* Str            Next Tbl Ptr       Handler Func.    Data Type        Access     Parameter                           IndexRange           DataLimit   EnumTbl*/
  { NULL          , NO_NEXT_TABLE,     Event_State,     USER_TYPE_NONE,  USER_RW,   NULL,                               0,(MAX_EVENTS-1),    NO_LIMIT,   NULL },
};

static USER_MSG_TBL TimeHistoryRoot [] =
{ /* Str            Next Tbl Ptr       Handler Func.           Data Type          Access            Parameter      IndexRange   DataLimit  EnumTbl*/
   { "CFG",         TimeHistoryCmd,    NULL,                   NO_HANDLER_DATA},
   { "STATUS",      TimeHistoryStatus, NULL,                   NO_HANDLER_DATA},
   { DISPLAY_CFG,   NO_NEXT_TABLE,     TimeHistory_ShowConfig, USER_TYPE_ACTION,  USER_RO|USER_GSE, NULL,          -1, -1,      NO_LIMIT,  NULL},
   { NULL,          NULL,              NULL,                   NO_HANDLER_DATA}
};

static
USER_MSG_TBL    RootTimeHistoryMsg = { "TIMEHISTORY", TimeHistoryRoot, NULL, NO_HANDLER_DATA };

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
* Function:     TimeHistory_UserCfg
*
* Description:  Handles User Manager requests to change event configuration
*               items.  
*               
* Parameters:   [in] DataType:  C type of the data to be read or changed, used 
*                               for casting the data pointers
*               [in/out] Param: Pointer to the configuration item to be read
*                               or changed
*               [in] Index:     Index parameter is used to reference the
*                               specific Event to change. Range is validated
*                               by the user manager
*               [in] SetPtr:    For write commands, a pointer to the data to
*                               write to the configuration.
*               [out] GetPtr:   For read commands, UserCfg function will set
*                               this to the location of the data requested.

*
* Returns:      USER_HANDLER_RESULT
*
* Notes:        
*****************************************************************************/
USER_HANDLER_RESULT TimeHistory_UserCfg ( USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr)
{
   USER_HANDLER_RESULT result;

   result = USER_RESULT_OK;

   //Load Event structure into the temporary location based on index param
   //Param.Ptr points to the struct member to be read/written
   //in the temporary location
   memcpy ( &ConfigTimeHistoryTemp,
            &CfgMgr_ConfigPtr()->TimeHistoryConfig,
            sizeof(ConfigTimeHistoryTemp));

   result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
   if(SetPtr != NULL && USER_RESULT_OK == result)
   {
     memcpy ( &CfgMgr_ConfigPtr()->TimeHistoryConfig,
              &ConfigTimeHistoryTemp,
              sizeof(TIMEHISTORY_CFG));
     //Store the modified temporary structure in the EEPROM.       
     CfgMgr_StoreConfigItem ( CfgMgr_ConfigPtr(),
                              &CfgMgr_ConfigPtr()->TimeHistoryConfig,
                              sizeof(ConfigTimeHistoryTemp));
   }
   return result;  
}

/******************************************************************************
* Function:     TimeHistory_State
*
* Description:  Handles User Manager requests to retrieve the current Event
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
* Returns:      USER_HANDLER_RESULT
*
* Notes:        
*****************************************************************************/
USER_HANDLER_RESULT TimeHistory_State ( USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr)
{
   USER_HANDLER_RESULT result;

   result = USER_RESULT_ERROR;

   memcpy(&StateTimeHistoryTemp, &TimeHistoryData, sizeof(StateTimeHistoryTemp));

   result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

   return result;
}
/******************************************************************************
* Function:    TimeHistory_ShowConfig
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
* Returns:     USER_RESULT_OK:    Processed successfully
*              USER_RESULT_ERROR: Error processing command.       
*
* Notes:        
*****************************************************************************/
USER_HANDLER_RESULT TimeHistory_ShowConfig ( USER_DATA_TYPE DataType,
                                             USER_MSG_PARAM Param,
                                             UINT32 Index,
                                             const void *SetPtr,
                                             void **GetPtr )
{
   CHAR  LabelStem[] = "\r\n\r\nTIMEHISTORY.CFG";
   CHAR  Label[USER_MAX_MSG_STR_LEN * 3];   
   INT16 i;

   USER_HANDLER_RESULT result = USER_RESULT_OK;
   USER_MSG_TBL*  pCfgTable;

   //Top-level name is a single indented space
   CHAR BranchName[USER_MAX_MSG_STR_LEN] = " ";   

   pCfgTable = &TimeHistoryCmd;  // Get pointer to config entry

   result = USER_RESULT_ERROR;
   if ( User_OutputMsgString( Label, FALSE ) )
   {
      result = User_DisplayConfigTree(BranchName, pCfgTable, i, 0, NULL);
   }   
   return result;
}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: TimeHistoryUserTables.c $
 * 
 * *****************  Version 1  *****************
 * User: John Omalley Date: 4/24/12    Time: 10:59a
 * Created in $/software/control processor/code/application
 * 
 *
 *
 **************************************************************************/ 


