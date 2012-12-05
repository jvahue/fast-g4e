#define ENGINERUN_USERTABLES_BODY
/******************************************************************************
Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
All Rights Reserved. Proprietary and Confidential.

File:          EngineRunUserTables.c

Description:

VERSION
$Revision: 24 $  $Date: 12/05/12 4:16p $

******************************************************************************/
#ifndef ENGINERUN_BODY
#error EngineRunUserTables.c should only be included by EngineRun.c
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

static ENGRUN_CFG      m_CfgTemp;
static ENGRUN_DATA     m_DataTemp;

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static USER_HANDLER_RESULT  EngRunUserCfg( USER_DATA_TYPE DataType,
                                           USER_MSG_PARAM Param,
                                           UINT32 Index,
                                           const void *SetPtr,
                                           void **GetPtr);

static USER_HANDLER_RESULT  EngRunState(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr);

static USER_HANDLER_RESULT EngRunShowConfig(USER_DATA_TYPE DataType,
                                            USER_MSG_PARAM Param,
                                            UINT32 Index,
                                            const void *SetPtr,
                                            void **GetPtr);

static USER_HANDLER_RESULT EngUserInfo(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr);

static USER_HANDLER_RESULT EngSPUserInfo(USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr);

/*****************************************************************************/
/* User command tables                                                       */
/*****************************************************************************/
//static USER_ENUM_TBL EngineRunStateEnum[] =
//{
//  { "STOPPED",  ER_STATE_STOPPED  },
//  { "STARTING", ER_STATE_STARTING },
//  { "RUNNING",  ER_STATE_RUNNING  },
//  { NULL, 0 }
//};

USER_ENUM_TBL engRunIdEnum[] =
{
  { "0"     , ENGRUN_ID_0  },
  { "1"     , ENGRUN_ID_1  },
  { "2"     , ENGRUN_ID_2  },
  { "3"     , ENGRUN_ID_3  },
  { "UNUSED", ENGRUN_UNUSED},
  { NULL, 0 }
};

static USER_ENUM_TBL engRunRateType[] =
{ { "1HZ"    , ER_1HZ          },
  { "2HZ"    , ER_2HZ          },
  { "4HZ"    , ER_4HZ          },
  { "5HZ"    , ER_5HZ          },
  { "10HZ"   , ER_10HZ         },
  { "20HZ"   , ER_20HZ         },
  { "50HZ"   , ER_50HZ         },
  { NULL     , 0               }
};

#pragma ghs nowarning 1545 //Suppress packed structure alignment warning

// Macro defines the 'prototype' declaration for entry in EngRunSensrTbl
#define DECL_SNSR_SUMMARY_ENTRY( n )\
static USER_MSG_TBL snsrSummaryEntry##n [] =\
{\
  /*Str          Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                                    IndexRange            DataLimit     EnumTbl*/\
  {"SENSORID",    NO_NEXT_TABLE, EngRunState,   USER_TYPE_ENUM,    USER_RO,  &m_DataTemp.snsrSummary[n].SensorIndex,      0,(MAX_ENGINES-1),    NO_LIMIT,     SensorIndexType },\
  {"VALID",       NO_NEXT_TABLE, EngRunState,   USER_TYPE_BOOLEAN, USER_RO,  &m_DataTemp.snsrSummary[n].bValid,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },\
  {"MIN",         NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[n].fMinValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },\
  {"MAX",         NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[n].fMaxValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },\
  {"AVG",         NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[n].fAvgValue,        0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },\
  {"TOT",         NO_NEXT_TABLE, EngRunState,   USER_TYPE_FLOAT,   USER_RO,  &m_DataTemp.snsrSummary[n].fTotal,           0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },\
  {"SAMPLE_COUNT",NO_NEXT_TABLE, EngRunState,   USER_TYPE_UINT32,  USER_RO,  &m_DataTemp.snsrSummary[n].nSampleCount,     0,(MAX_ENGINES-1),    NO_LIMIT,     NULL            },\
  { NULL,         NULL,          NULL, NO_HANDLER_DATA}\
}
// Declare formal entries in EngRunSensrTbl0...15
  DECL_SNSR_SUMMARY_ENTRY(0);
  DECL_SNSR_SUMMARY_ENTRY(1);
  DECL_SNSR_SUMMARY_ENTRY(2);
  DECL_SNSR_SUMMARY_ENTRY(3);
  DECL_SNSR_SUMMARY_ENTRY(4);
  DECL_SNSR_SUMMARY_ENTRY(5);
  DECL_SNSR_SUMMARY_ENTRY(6);
  DECL_SNSR_SUMMARY_ENTRY(7);
  DECL_SNSR_SUMMARY_ENTRY(8);
  DECL_SNSR_SUMMARY_ENTRY(9);
  DECL_SNSR_SUMMARY_ENTRY(10);
  DECL_SNSR_SUMMARY_ENTRY(11);
  DECL_SNSR_SUMMARY_ENTRY(12);
  DECL_SNSR_SUMMARY_ENTRY(13);
  DECL_SNSR_SUMMARY_ENTRY(14);
  DECL_SNSR_SUMMARY_ENTRY(15);


// Declare EngRunSensrTbl referencing entries from above
static USER_MSG_TBL engRunSensrTbl[] =
{
  { "S0",   snsrSummaryEntry0 , NULL, NO_HANDLER_DATA },
  { "S1",   snsrSummaryEntry1 , NULL, NO_HANDLER_DATA },
  { "S2",   snsrSummaryEntry2 , NULL, NO_HANDLER_DATA },
  { "S3",   snsrSummaryEntry3 , NULL, NO_HANDLER_DATA },
  { "S4",   snsrSummaryEntry4 , NULL, NO_HANDLER_DATA },
  { "S5",   snsrSummaryEntry5 , NULL, NO_HANDLER_DATA },
  { "S6",   snsrSummaryEntry6 , NULL, NO_HANDLER_DATA },
  { "S7",   snsrSummaryEntry7 , NULL, NO_HANDLER_DATA },
  { "S8",   snsrSummaryEntry8 , NULL, NO_HANDLER_DATA },
  { "S9",   snsrSummaryEntry9 , NULL, NO_HANDLER_DATA },
  { "S10",  snsrSummaryEntry10, NULL, NO_HANDLER_DATA },
  { "S11",  snsrSummaryEntry11, NULL, NO_HANDLER_DATA },
  { "S12",  snsrSummaryEntry12, NULL, NO_HANDLER_DATA },
  { "S13",  snsrSummaryEntry13, NULL, NO_HANDLER_DATA },
  { "S14",  snsrSummaryEntry14, NULL, NO_HANDLER_DATA },
  { "S15",  snsrSummaryEntry15, NULL, NO_HANDLER_DATA },
  { NULL,   NULL,               NULL, NO_HANDLER_DATA }
};


static USER_MSG_TBL engRunStatusCmd [] =
{
  /*Str               Next Tbl Ptr    Handler Func Data Type         Access    Parameter                        IndexRange              DataLimit   EnumTbl*/
  {"ENGINEID",        NO_NEXT_TABLE,  EngRunState, USER_TYPE_ENUM,   USER_RO,  &m_DataTemp.erIndex,             0,(MAX_ENGINES-1),     NO_LIMIT,   engRunIdEnum      },
  {"STATE",           NO_NEXT_TABLE,  EngRunState, USER_TYPE_ENUM,   USER_RO,  &m_DataTemp.erState,             0,(MAX_ENGINES-1),     NO_LIMIT,   engineRunStateEnum},
  {"STARTINGTIME_MS", NO_NEXT_TABLE,  EngRunState, USER_TYPE_UINT32, USER_RO,  &m_DataTemp.startingTime_ms,     0,(MAX_ENGINES-1),     NO_LIMIT,   NULL              },
  {"START_DUR_MS",    NO_NEXT_TABLE,  EngRunState, USER_TYPE_UINT32, USER_RO,  &m_DataTemp.startingDuration_ms, 0,(MAX_ENGINES-1),     NO_LIMIT,   NULL              },
  {"RUN_DUR_MS",      NO_NEXT_TABLE,  EngRunState, USER_TYPE_UINT32, USER_RO,  &m_DataTemp.erDuration_ms,       0,(MAX_ENGINES-1),     NO_LIMIT,   NULL              },
  {"MINVALUE",        NO_NEXT_TABLE,  EngRunState, USER_TYPE_FLOAT,  USER_RO,  &m_DataTemp.monMinValue,         0,(MAX_ENGINES-1),     NO_LIMIT,   NULL              },
  {"MAXVALUE",        NO_NEXT_TABLE,  EngRunState, USER_TYPE_FLOAT, USER_RO,   &m_DataTemp.monMaxValue,         0,(MAX_ENGINES-1),     NO_LIMIT,   NULL              },
  {"SENSOR",          engRunSensrTbl, NULL,        NO_HANDLER_DATA,                                                                                                  },
  { NULL,             NULL,           NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL engRunCfgCmd[] =
{
  /*Str              Next Tbl Ptr   Handler Func     Data Type        Access    Parameter                  IndexRange         DataLimit             EnumTbl*/
  {"NAME",           NO_NEXT_TABLE, EngRunUserCfg, USER_TYPE_STR,     USER_RW,  m_CfgTemp.engineName,      0,(MAX_ENGINES-1), 0,MAX_ENGINERUN_NAME, NULL            },
  {"STARTTRIGID",    NO_NEXT_TABLE, EngRunUserCfg, USER_TYPE_ENUM,    USER_RW,  &m_CfgTemp.startTrigID,    0,(MAX_ENGINES-1), NO_LIMIT,             triggerIndexType},
  {"RUNTRIGID",      NO_NEXT_TABLE, EngRunUserCfg, USER_TYPE_ENUM,    USER_RW,  &m_CfgTemp.runTrigID,      0,(MAX_ENGINES-1), NO_LIMIT,             triggerIndexType},
  {"STOPTRIGID",     NO_NEXT_TABLE, EngRunUserCfg, USER_TYPE_ENUM,    USER_RW,  &m_CfgTemp.stopTrigID,     0,(MAX_ENGINES-1), NO_LIMIT,             triggerIndexType},
  {"RATE",           NO_NEXT_TABLE, EngRunUserCfg, USER_TYPE_ENUM,    USER_RW,  &m_CfgTemp.erRate,         0,(MAX_ENGINES-1), NO_LIMIT,             engRunRateType  },
  {"RATEOFFSET_MS",  NO_NEXT_TABLE, EngRunUserCfg, USER_TYPE_UINT32,  USER_RW,  &m_CfgTemp.nOffset_ms,     0,(MAX_ENGINES-1), NO_LIMIT,             NULL            },
  {"MAXSENSORID",    NO_NEXT_TABLE, EngRunUserCfg, USER_TYPE_ENUM,    USER_RW,  &m_CfgTemp.monMaxSensorID, 0,(MAX_ENGINES-1), NO_LIMIT,             SensorIndexType },
  {"MINSENSORID",    NO_NEXT_TABLE, EngRunUserCfg, USER_TYPE_ENUM,    USER_RW,  &m_CfgTemp.monMinSensorID, 0,(MAX_ENGINES-1), NO_LIMIT,             SensorIndexType },
  {"SENSORS",        NO_NEXT_TABLE, EngRunUserCfg, USER_TYPE_SNS_LIST,USER_RW,  m_CfgTemp.sensorMap,       0,(MAX_ENGINES-1), 0,MAX_ENGRUN_SENSORS, NULL            },
  { NULL,            NULL,          NULL, NO_HANDLER_DATA}
};

static USER_MSG_TBL engIdCmd[] =
{
   /*Str      Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                            IndexRange         DataLimit            EnumTbl*/
   {"SN",     NO_NEXT_TABLE, EngUserInfo,   USER_TYPE_STR,     USER_RW,  m_EngineInfo.engine[0].serialNumber, 0,(MAX_ENGINES-1), 0,MAX_ENGINE_ID,     NULL            },
   {"MODEL",  NO_NEXT_TABLE, EngUserInfo,   USER_TYPE_STR,     USER_RW,  m_EngineInfo.engine[0].modelNumber,  0,(MAX_ENGINES-1), 0,MAX_ENGINE_ID,     NULL            },
   {NULL,     NULL,          NULL,          NO_HANDLER_DATA}
};



static USER_MSG_TBL engInfoCmd[] =
{
   /*Str             Next Tbl Ptr   Handler Func   Data Type          Access    Parameter                    IndexRange         DataLimit             EnumTbl       */
   {"SERVICE_PLAN",  NO_NEXT_TABLE, EngSPUserInfo, USER_TYPE_STR,     USER_RW,  m_EngineInfo.servicePlan,    -1,-1,             0,MAX_ENGINE_ID,      NULL          },
   {"ENGINE",        engIdCmd,      NULL,          NO_HANDLER_DATA,                                                                                                 },
   {NULL,            NULL,          NULL,          NO_HANDLER_DATA}
};

static USER_MSG_TBL engRunCmd [] =
{
  /*Str             Next Tbl Ptr      Handler Func      Data Type             Access                         Parameter   IndexRange  DataLimit   EnumTbl*/
  {"CFG",           engRunCfgCmd,     NULL,             NO_HANDLER_DATA,                                                                                },
  {"STATUS",        engRunStatusCmd,  NULL,             NO_HANDLER_DATA,                                                                                },
  {"INFO",          engInfoCmd,       NULL,             NO_HANDLER_DATA,                                                                                },
  {DISPLAY_CFG,     NO_NEXT_TABLE,    EngRunShowConfig, USER_TYPE_ACTION,    (USER_RO|USER_NO_LOG|USER_GSE)  ,NULL,      -1,-1,      NO_LIMIT,   NULL   },
  { NULL,           NULL,             NULL,             NO_HANDLER_DATA}
};
#pragma ghs endnowarning

static USER_MSG_TBL rootEngRunMsg = {"ENG",engRunCmd, NULL,NO_HANDLER_DATA};


/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:     EngRunUserCfg
 *
 * Description:  User message handler to set/get configuration items of the
 *               EngineRun object
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
 * Returns:      USER_RESULT_OK:    Processed successfully
 *               USER_RESULT_ERROR: Error processing command.
 *
 * Notes:        none
 *
 *****************************************************************************/
static USER_HANDLER_RESULT EngRunUserCfg(USER_DATA_TYPE DataType,
                                  USER_MSG_PARAM Param,
                                  UINT32 Index,
                                  const void *SetPtr,
                                  void **GetPtr)
{
 USER_HANDLER_RESULT result = USER_RESULT_OK;

  memcpy(&m_CfgTemp,
         &CfgMgr_ConfigPtr()->EngineRunConfigs[Index],
         sizeof(m_CfgTemp));

  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    // Copy the updated temp  back to the shadow ram store.
    memcpy(&CfgMgr_ConfigPtr()->EngineRunConfigs[Index],
           &m_CfgTemp,
           sizeof(m_CfgTemp));

    // Persist the CfgMgr's updated copy back to EEPROM.
    CfgMgr_StoreConfigItem( CfgMgr_ConfigPtr(),
                            &CfgMgr_ConfigPtr()->EngineRunConfigs[Index],
                            sizeof(m_CfgTemp) );
  }
  return result;
}

/******************************************************************************
 * Function:     EngUserInfo
 *
 * Description:  User message handler to set/get engine Identification items
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
 * Returns:      USER_RESULT_OK:    Processed successfully
 *               USER_RESULT_ERROR: Error processing command.
 *
 * Notes:        none
 *
 *****************************************************************************/
static USER_HANDLER_RESULT EngUserInfo(USER_DATA_TYPE DataType,
                                  USER_MSG_PARAM Param,
                                  UINT32 Index,
                                  const void *SetPtr,
                                  void **GetPtr)
{
   //Offsets of each member of .Board is relative to Box_Info.Board[0].
   //Offset by the user's entered index to get the .Board[Index]
   Param.Ptr = (char*)Param.Ptr + sizeof(m_EngineInfo.engine[0]) * Index;

   if(SetPtr == NULL)
   {
      //Read into the global structure from NV memory, Return the parameter
      //to user manager.
      NV_Read(NV_ENGINE_ID,0,&m_EngineInfo,sizeof(m_EngineInfo));
      *GetPtr = Param.Ptr;
   }
   else
   {
      //Copy the new data into the global struct and write it to NV memory
      //Offsets of each member of .Board is relative to Box_Info.Board[0].
      //Offset by the user's entered index to get the .Board[Index]
      strncpy_safe(Param.Ptr,MAX_ENGINE_ID, SetPtr, _TRUNCATE );
      NV_Write(NV_ENGINE_ID,0,&m_EngineInfo,sizeof(m_EngineInfo));
   }
   return USER_RESULT_OK;
}

/******************************************************************************
 * Function:     EngSPUserInfo
 *
 * Description:  User message handler to set/get engine Service Plan
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
 * Returns:      USER_RESULT_OK:    Processed successfully
 *               USER_RESULT_ERROR: Error processing command.
 *
 * Notes:        none
 *
 *****************************************************************************/
static USER_HANDLER_RESULT EngSPUserInfo(USER_DATA_TYPE DataType,
                                  USER_MSG_PARAM Param,
                                  UINT32 Index,
                                  const void *SetPtr,
                                  void **GetPtr)
{
  if(SetPtr == NULL)
  {
    //Read into the global structure from NV memory, Return the parameter
    //to user manager.
    NV_Read(NV_ENGINE_ID,0,&m_EngineInfo,sizeof(m_EngineInfo));
    *GetPtr = Param.Ptr;
  }
  else
  {
    //Copy the new data into the global struct and write it to NV memory
    strncpy_safe(Param.Ptr,MAX_ENGINE_ID, SetPtr, _TRUNCATE );
    NV_Write(NV_BOX_CFG,0,&m_EngineInfo,sizeof(m_EngineInfo));
  }

  return USER_RESULT_OK;
}

/******************************************************************************
* Function:     EngRunState
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

static USER_HANDLER_RESULT EngRunState(USER_DATA_TYPE DataType,
                                  USER_MSG_PARAM Param,
                                  UINT32 Index,
                                  const void *SetPtr,
                                  void **GetPtr)
{
   USER_HANDLER_RESULT result;

   result = USER_RESULT_ERROR;

   memcpy(&m_DataTemp, &m_engineRunData[Index], sizeof(m_DataTemp));

   result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

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
static USER_HANDLER_RESULT EngRunShowConfig(USER_DATA_TYPE DataType,
                                     USER_MSG_PARAM Param,
                                     UINT32 Index,
                                     const void *SetPtr,
                                     void **GetPtr)
{
  USER_HANDLER_RESULT result;
  CHAR label[USER_MAX_MSG_STR_LEN * 3];

  //Top-level name is a single indented space
  CHAR branchName[USER_MAX_MSG_STR_LEN] = " ";

  USER_MSG_TBL*  pCfgTable;
  INT16 engIdx;

  result = USER_RESULT_OK;

  // Loop for each sensor.
  for (engIdx = 0; engIdx < MAX_ENGINES && result == USER_RESULT_OK; ++engIdx)
  {
    // Display sensor id info above each set of data.
    snprintf(label, sizeof(label), "\r\n\r\nENG[%d].CFG", engIdx);
    result = USER_RESULT_ERROR;
    if (User_OutputMsgString( label, FALSE ) )
    {
      pCfgTable = engRunCfgCmd;  // Re-set the pointer to beginning of CFG table

      // User_DisplayConfigTree will invoke itself recursively to display all fields.
      result = User_DisplayConfigTree(branchName, pCfgTable, engIdx, 0, NULL);
    }
  }
  return result;
}
/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/*************************************************************************
*  MODIFICATIONS
*    $History: EngineRunUserTables.c $
 * 
 * *****************  Version 24  *****************
 * User: Contractor V&v Date: 12/05/12   Time: 4:16p
 * Updated in $/software/control processor/code/application
 * SCR #1107 Code Review 
 * 
 * *****************  Version 23  *****************
 * User: Contractor V&v Date: 11/26/12   Time: 6:06p
 * Updated in $/software/control processor/code/application
 * SCR #1107 Code Review
 * 
 * *****************  Version 22  *****************
 * User: Contractor V&v Date: 11/26/12   Time: 12:31p
 * Updated in $/software/control processor/code/application
 * SCR #1167 SensorSummary
 *
 * *****************  Version 21  *****************
 * User: Contractor V&v Date: 11/16/12   Time: 8:11p
 * Updated in $/software/control processor/code/application
 * CodeReview
 *
 * *****************  Version 20  *****************
 * User: Contractor V&v Date: 11/12/12   Time: 6:39p
 * Updated in $/software/control processor/code/application
 * Code Review
 *
 * *****************  Version 19  *****************
 * User: Contractor V&v Date: 11/08/12   Time: 4:26p
 * Updated in $/software/control processor/code/application
 * Code review
 *
 * *****************  Version 18  *****************
 * User: Melanie Jutras Date: 12-10-31   Time: 12:57p
 * Updated in $/software/control processor/code/application
 * SCR #1142 File Format Error
 *
 * *****************  Version 17  *****************
 * User: John Omalley Date: 12-10-23   Time: 2:41p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Design and Code Review Updates
 *
 * *****************  Version 16  *****************
 * User: Contractor V&v Date: 10/11/12   Time: 6:55p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 Review Findings
 *
 * *****************  Version 15  *****************
 * User: Contractor V&v Date: 12-10-02   Time: 1:18p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 Coding standard compliance
 *
 * *****************  Version 14  *****************
 * User: John Omalley Date: 12-09-27   Time: 9:13a
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Added Engine Identification Fields to software and file
 * header
 *
 * *****************  Version 13  *****************
 * User: Jeff Vahue   Date: 9/17/12    Time: 10:53a
 * Updated in $/software/control processor/code/application
 * SCR# 1107 - Add ACT_LIST, clean up msgs
 *
 * *****************  Version 12  *****************
 * User: Contractor V&v Date: 9/14/12    Time: 4:03p
 * Updated in $/software/control processor/code/application
 * FAST 2 fixes for sensor list
 *
 * *****************  Version 11  *****************
 * User: Jeff Vahue   Date: 9/07/12    Time: 4:05p
 * Updated in $/software/control processor/code/application
 * SCR# 1107 - V&V fixes, code review updates
 *
 * *****************  Version 10  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 12:43p
 * Updated in $/software/control processor/code/application
 * SCR# 1142
 *
 * *****************  Version 9  *****************
 * User: Contractor V&v Date: 8/15/12    Time: 7:17p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 BITARRAY128 input as integer list
 *
 * *****************  Version 8  *****************
 * User: Contractor V&v Date: 8/08/12    Time: 3:41p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 make min/max generic
 *
 * *****************  Version 7  *****************
 * User: Contractor V&v Date: 7/18/12    Time: 6:24p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2
 *
 * *****************  Version 6  *****************
 * User: Contractor V&v Date: 6/25/12    Time: 2:28p
 * Updated in $/software/control processor/code/application
 *
 * *****************  Version 5  *****************
 * User: Contractor V&v Date: 6/18/12    Time: 4:27p
 * Updated in $/software/control processor/code/application
 *
 * *****************  Version 4  *****************
 * User: Contractor V&v Date: 6/05/12    Time: 6:41p
 * Updated in $/software/control processor/code/application
 * Engine ID change
 *
 * *****************  Version 3  *****************
 * User: Contractor V&v Date: 5/24/12    Time: 3:05p
 * Updated in $/software/control processor/code/application
 * FAST2 Refactoring
 *
 * *****************  Version 2  *****************
 * User: John Omalley Date: 12-05-22   Time: 2:15p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Check In for Dave
 *
 * *****************  Version 1  *****************
 * User: Contractor V&v Date: 5/10/12    Time: 6:39p
 * Created in $/software/control processor/code/application
 * Add interface and usertables for enginerun object
*
****************************************************************************/
