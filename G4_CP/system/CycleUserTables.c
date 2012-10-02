#define CYCLE_USERTABLE_BODY
/******************************************************************************
Copyright (C) 2012 Pratt & Whitney Engine Services, Inc. 
All Rights Reserved. Proprietary and Confidential.

File:          CycleUserTables.c

Description: 

VERSION
$Revision: 11 $  $Date: 12-10-02 1:24p $ 

******************************************************************************/
#ifndef CYCLE_BODY
#error CycleUserTables.c should only be included by EngineRun.c
#endif

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/


/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/

static CYCLE_CFG   m_CfgTemp;     // Temp buffer for config data
static CYCLE_DATA  m_DataTemp;    // Temp buffer for runtime data 
static CYCLE_ENTRY m_persistTemp; // Temp buffer for persisted counts info.

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/

USER_HANDLER_RESULT       CycleState(USER_DATA_TYPE DataType,
                                     USER_MSG_PARAM Param,
                                     UINT32 Index,
                                     const void *SetPtr,
                                     void **GetPtr);

USER_HANDLER_RESULT       CycleUserCfg(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr);


USER_HANDLER_RESULT       CyclePCount(USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr);

USER_HANDLER_RESULT       CycleShowConfig(USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr);

/*****************************************************************************/
/* User command tables                                                       */
/*****************************************************************************/

static USER_ENUM_TBL CycleTypeEnum[] =
{
  { "NONE",             CYC_TYPE_NONE_CNT             },
  { "SIMPLE",           CYC_TYPE_SIMPLE_CNT           },
  { "PERSIST_SIMPLE",   CYC_TYPE_PERSIST_SIMPLE_CNT   },
  { "DURATION",         CYC_TYPE_DURATION_CNT         },
  { "PERSIST_DURATION", CYC_TYPE_PERSIST_DURATION_CNT },
  { NULL,               0                             }
};

#pragma ghs nowarning 1545 //Suppress packed structure alignment warning
static USER_MSG_TBL CycleStatusCmd [] =
{
  /*Str               Next Tbl Ptr    Handler Func           Data Type          Access    Parameter                     IndexRange         DataLimit   EnumTbl*/
  {"CYCLE_ACTIVE",    NO_NEXT_TABLE,  CycleState,            USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.cycleActive,      0,(MAX_CYCLES-1),  NO_LIMIT,   NULL},
  {"CYCLE_LAST_TIME", NO_NEXT_TABLE,  CycleState,            USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.cycleLastTime_ms, 0,(MAX_CYCLES-1),  NO_LIMIT,   NULL},
  { NULL,             NULL,           NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL CycleCfgCmd[] =
{
  /*Str          Next Tbl Ptr   Handler Func     Data Type       Access    Parameter               IndexRange          DataLimit              EnumTbl*/
  {"NAME",       NO_NEXT_TABLE, CycleUserCfg, USER_TYPE_STR,    USER_RW,  &m_CfgTemp.name,         0,(MAX_CYCLES-1),   0,MAX_CYCLENAME,       NULL             },
  {"TYPE",       NO_NEXT_TABLE, CycleUserCfg, USER_TYPE_ENUM,   USER_RW,  &m_CfgTemp.type,         0,(MAX_CYCLES-1),   NO_LIMIT,              CycleTypeEnum    },
  {"COUNT",      NO_NEXT_TABLE, CycleUserCfg, USER_TYPE_UINT32, USER_RW,  &m_CfgTemp.nCount,       0,(MAX_CYCLES-1),   NO_LIMIT,              NULL             },
  {"TRIGGERID",  NO_NEXT_TABLE, CycleUserCfg, USER_TYPE_ENUM,   USER_RW,  &m_CfgTemp.nTriggerId,   0,(MAX_CYCLES-1),   NO_LIMIT,              TriggerIndexType },
  {"ENGINERUNID",NO_NEXT_TABLE, CycleUserCfg, USER_TYPE_ENUM,   USER_RW,  &m_CfgTemp.nEngineRunId, 0,(MAX_CYCLES-1),   NO_LIMIT,              EngRunIdEnum     },
  { NULL,        NULL,          NULL,         NO_HANDLER_DATA}
};

// Persisted Counts
static USER_MSG_TBL CyclePCountsCmd[] =
{
  /*Str        Next Tbl Ptr   Handler Func  Data Type         Access    Parameter                 IndexRange          DataLimit         EnumTbl*/
  {"COUNT",    NO_NEXT_TABLE, CyclePCount,  USER_TYPE_UINT32, USER_RW,  &m_persistTemp.count.n,   0,(MAX_CYCLES-1),   NO_LIMIT,       NULL },
  {"CHECKID",  NO_NEXT_TABLE, CyclePCount,  USER_TYPE_UINT16, USER_RW,  &m_persistTemp.checkID,   0,(MAX_CYCLES-1),   NO_LIMIT,       NULL },  
  { NULL,        NULL,          NULL,       NO_HANDLER_DATA}
};


static USER_MSG_TBL CycleCmd [] =
{
  /*Str             Next Tbl Ptr      Handler Func      Data Type             Access                         Parameter   IndexRange  DataLimit   EnumTbl*/                        
  {"CFG",           CycleCfgCmd,      NULL,             NO_HANDLER_DATA,                                                                                },
  {"STATUS",        CycleStatusCmd,   NULL,             NO_HANDLER_DATA,                                                                                },
  {"PERSIST",       CyclePCountsCmd,  NULL,             NO_HANDLER_DATA,                                                                                },
  {DISPLAY_CFG,     NO_NEXT_TABLE,    CycleShowConfig, USER_TYPE_ACTION,    (USER_RO|USER_NO_LOG|USER_GSE)  ,NULL       ,-1,-1,     NO_LIMIT,   NULL   },
  { NULL,           NULL,             NULL,             NO_HANDLER_DATA}
};
#pragma ghs endnowarning

static USER_MSG_TBL RootCycleMsg = {"CYCLE",CycleCmd, NULL,NO_HANDLER_DATA};


/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
* Function:     CycleState
*
* Description:  Handles User Manager requests to retrieve the current Cycle status
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
******************************************************************************/

USER_HANDLER_RESULT CycleState(USER_DATA_TYPE DataType,
                                USER_MSG_PARAM Param,
                                UINT32 Index,
                                const void *SetPtr,
                                void **GetPtr)
{
  USER_HANDLER_RESULT result;

  result = USER_RESULT_ERROR;

  memcpy(&m_DataTemp, &m_Data[Index], sizeof(m_DataTemp));

  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

  return result;
}


/******************************************************************************
* Function:     CyclePCount
*
* Description:  Handles User Manager requests to retrieve the current Cycle status
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
******************************************************************************/

USER_HANDLER_RESULT CyclePCount(USER_DATA_TYPE DataType,
                                USER_MSG_PARAM Param,
                                UINT32 Index,
                                const void *SetPtr,
                                void **GetPtr)
{  
  USER_HANDLER_RESULT result;
 
  result = USER_RESULT_ERROR;
  
  memcpy(&m_persistTemp, &m_CountsEEProm.data[Index], sizeof(m_persistTemp));

  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    // safeguard against updating counts for an active cycle.
    if ( !m_Data[Index].cycleActive )
    {
      // Copy the updated temp  back to EEPROM then persist it to both EEPROM RTC files.
      memcpy( &m_CountsEEProm.data[Index], &m_persistTemp, sizeof(m_persistTemp));
      CycleSyncPersistFiles(CYC_COUNT_UPDATE_BACKGROUND);
    }
    else
    {
      GSE_DebugStr(NORMAL, TRUE, "Cycle cannot be reset while cycle is active\n"); 
    } 
  }
  return result;
}


/******************************************************************************
 * Function:     CycleUserCfg
 *
 * Description:  User message handler to set/get configuration items of the
 *               EngineRun object
 *
 * Parameters:   See user.h command handler prototype
 *
 * Returns:      USER_RESULT_OK:    Processed successfully
 *               USER_RESULT_ERROR: Error processing command. 
 *
 * Notes:        none
 *
 *****************************************************************************/
USER_HANDLER_RESULT CycleUserCfg( USER_DATA_TYPE DataType,
                                  USER_MSG_PARAM Param,
                                  UINT32 Index,
                                  const void *SetPtr,
                                  void **GetPtr)
{
 USER_HANDLER_RESULT result = USER_RESULT_OK; 
 
  memcpy(&m_CfgTemp,
         &CfgMgr_ConfigPtr()->CycleConfigs[Index],
         sizeof(m_CfgTemp));

  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    // Copy the updated temp  back to the shadow ram store.
    memcpy(&CfgMgr_ConfigPtr()->CycleConfigs[Index],
           &m_CfgTemp,
           sizeof(m_CfgTemp));

    // Persist the CfgMgr's updated copy back to EEPROM.       
    CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
      &CfgMgr_ConfigPtr()->CycleConfigs[Index],
      sizeof(m_CfgTemp));
  }
  return result;  
}

/******************************************************************************
 * Function:    EngRunShowConfig
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
USER_HANDLER_RESULT CycleShowConfig(USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr)
{
  USER_HANDLER_RESULT result;   
  CHAR Label[USER_MAX_MSG_STR_LEN * 3];   

  //Top-level name is a single indented space
  CHAR BranchName[USER_MAX_MSG_STR_LEN] = " ";

  USER_MSG_TBL*  pCfgTable;
  INT16 cycIdx;

  result = USER_RESULT_OK;   

  // Loop for each cycle.
  for (cycIdx = 0; cycIdx < MAX_CYCLES && result == USER_RESULT_OK; ++cycIdx)
  {
    // Display sensor id info above each set of data.
    sprintf(Label, "\r\n\r\nCYCLE[%d].CFG", cycIdx);
    result = USER_RESULT_ERROR;
    if (User_OutputMsgString( Label, FALSE ) )
    {
      pCfgTable = CycleCfgCmd; // Re-set the pointer to beginning of CFG table       

      // User_DisplayConfigTree will invoke itself recursively to display all fields.
      result = User_DisplayConfigTree(BranchName, pCfgTable, cycIdx, 0, NULL);
    }     
  } 
  return result;
}



/*****************************************************************************/
/* Local Functions                                                           */ 
/*****************************************************************************/


/*************************************************************************
*  MODIFICATIONS
*    $History: CycleUserTables.c $
 * 
 * *****************  Version 11  *****************
 * User: Contractor V&v Date: 12-10-02   Time: 1:24p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 Coding standard compliance
 * 
 * *****************  Version 10  *****************
 * User: Contractor V&v Date: 12-09-19   Time: 6:49p
 * Updated in $/software/control processor/code/system
 * SCR #   FAST 2 add end NULL entry to cyle type table
 * 
 * *****************  Version 9  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 8  *****************
 * User: Contractor V&v Date: 8/08/12    Time: 3:29p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2  Fixed bug in persist count writing
 * 
 * *****************  Version 7  *****************
 * User: Contractor V&v Date: 7/18/12    Time: 6:26p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 GSE fix
 * 
 * *****************  Version 6  *****************
 * User: Contractor V&v Date: 7/11/12    Time: 4:35p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2  Added missing cfg type to table
 * 
 * *****************  Version 5  *****************
 * User: Contractor V&v Date: 6/25/12    Time: 7:16p
 * Updated in $/software/control processor/code/system
 * 
 * *****************  Version 4  *****************
 * User: Contractor V&v Date: 6/25/12    Time: 2:41p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 Rename rename cmd table
 * 
 * *****************  Version 3  *****************
 * User: Contractor V&v Date: 5/24/12    Time: 3:05p
 * Updated in $/software/control processor/code/system
 * FAST2 Refactoring
 * 
 * *****************  Version 2  *****************
 * User: John Omalley Date: 12-05-22   Time: 2:17p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Check in for Dave
 * 
 * *****************  Version 1  *****************
 * User: Contractor V&v Date: 5/10/12    Time: 6:40p
 * Created in $/software/control processor/code/system
 * Add usertable for cycles
*
***************************************************************************/
