#define TREND_USERTABLES_BODY
/******************************************************************************
       Copyright (C) 2012 - 2016 Pratt & Whitney Engine Services, Inc.
            All Rights Reserved. Proprietary and Confidential.

  ECCN:        9D991

  File:        TrendUserTables.c

  Description: User command structures and functions for the trend processing

  VERSION
    $Revision: 41 $  $Date: 3/18/16 5:43p $
******************************************************************************/
#ifndef TREND_BODY
#error TrendUserTables.c should only be included by Trend.c
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

#define SECS_PER_DAY   86400
#define MAX_GET_HIST_LINE_LEN 60
#define GET_SAMPLE_ENTRY_LINE_CNT 10
#define MAX_GET_SAMPLE_LINE_LEN  (MAX_GET_HIST_LINE_LEN * GET_SAMPLE_ENTRY_LINE_CNT)
/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
// See Local Variables below Function Prototypes Section

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
//Prototype for the User Manager message handlers, has to go before
//the local variable tables that use the function pointer.
static USER_HANDLER_RESULT Trend_UserCfg ( USER_DATA_TYPE DataType,
                                           USER_MSG_PARAM Param,
                                           UINT32 Index,
                                           const void *SetPtr,
                                           void **GetPtr );

static USER_HANDLER_RESULT  Trend_State ( USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr );

static USER_HANDLER_RESULT Trend_ShowConfig( USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr );

static USER_HANDLER_RESULT Trend_Start( USER_DATA_TYPE DataType,
                                            USER_MSG_PARAM Param,
                                            UINT32 Index,
                                            const void *SetPtr,
                                            void **GetPtr );
static USER_HANDLER_RESULT Trend_Stop( USER_DATA_TYPE DataType,
                                            USER_MSG_PARAM Param,
                                            UINT32 Index,
                                            const void *SetPtr,
                                            void **GetPtr );

static USER_HANDLER_RESULT Trend_DsplyStabilityHist     ( USER_DATA_TYPE DataType,
                                                   USER_MSG_PARAM Param,
                                                   UINT32 Index,
                                                   const void *SetPtr,
                                                   void **GetPtr );

static USER_HANDLER_RESULT Trend_DsplySampleData( USER_DATA_TYPE DataType,
                                                   USER_MSG_PARAM Param,
                                                   UINT32 Index,
                                                   const void *SetPtr,
                                                   void **GetPtr );

static USER_HANDLER_RESULT Trend_DsplyStabilityState(USER_DATA_TYPE DataType,
                                                     USER_MSG_PARAM Param,
                                                     UINT32 Index,
                                                     const void *SetPtr,
                                                     void **GetPtr);

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/


/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static TREND_CFG         m_ConfigTrendTemp;      // Trend Configuration temp Storage

static TREND_DATA        m_StateTrendTemp;       // Trend State Temp Storage

static USER_ENUM_TBL trendRateType[] =
{
  { "1HZ"      , TREND_1HZ          },
  { "2HZ"      , TREND_2HZ          },
  { "4HZ"      , TREND_4HZ          },
  { "5HZ"      , TREND_5HZ          },
  { "10HZ"     , TREND_10HZ         },
  { "20HZ"     , TREND_20HZ         },
  { "50HZ"     , TREND_50HZ         },
  { NULL       , 0                  }
};

static USER_ENUM_TBL trendEngRunIdEnum[] =
{
  { "0"     , ENGRUN_ID_0  },
  { "1"     , ENGRUN_ID_1  },
  { "2"     , ENGRUN_ID_2  },
  { "3"     , ENGRUN_ID_3  },
  { "ANY"   , ENGRUN_ID_ANY},
  { "UNUSED", ENGRUN_UNUSED},
  { NULL, 0 }
};


static USER_ENUM_TBL trendStateEnum[] =
{
  { "INACTIVE"       , TR_TYPE_INACTIVE       },
  { "MANUAL_TREND"   , TR_TYPE_STANDARD_MANUAL},
  { "AUTO_TREND"     , TR_TYPE_STANDARD_AUTO  },
  { "COMMANDED_TREND", TR_TYPE_COMMANDED      },
  { NULL,  0 }
};

static USER_ENUM_TBL trendStabStateEnum[] =
{
  { "IDLE",   STB_STATE_IDLE  },
  { "READY" , STB_STATE_READY },
  { "SAMPLE", STB_STATE_SAMPLE},
  { NULL,  0  }
};

static USER_ENUM_TBL trendStableRuleEnum[] =
{
  { "INCREMENTAL", STB_STRGY_INCRMNTL},
  { "ABSOLUTE",    STB_STRGY_ABSOLUTE},
  { NULL,  0                         }
};

static USER_ENUM_TBL trendSampleResultEnum[] =
{
  { "UNKNOWN",  SMPL_RSLT_UNK    },
  { "SUCCESS",  SMPL_RSLT_SUCCESS},
  { "FAULTED",  SMPL_RSLT_FAULTED},
  { NULL,  0                     }
};

#pragma ghs nowarning 1545 //Suppress packed structure alignment warning

// Macro defines the 'prototype' declaration for entry in stabCritTbl
#define DECL_TREND_STAB_TBL_ENTRY( n )\
  static USER_MSG_TBL trendStabTbl##n [] =\
{\
  /*Str             Next Tbl Ptr   Handler Func     Data Type          Access    Parameter                                          IndexRange          DataLimit     EnumTbl*/\
  {"SENSORID",      NO_NEXT_TABLE, Trend_UserCfg,   USER_TYPE_ENUM,    USER_RW,  &m_ConfigTrendTemp.stability[n].sensorIndex,      0,(MAX_TRENDS-1),    NO_LIMIT,     SensorIndexType },\
  {"DELAYED",       NO_NEXT_TABLE, Trend_UserCfg,   USER_TYPE_BOOLEAN, USER_RW,  &m_ConfigTrendTemp.stability[n].bDelayed,         0,(MAX_TRENDS-1),    NO_LIMIT,     NULL            },\
  {"VAR_OUTLIER_MS",NO_NEXT_TABLE, Trend_UserCfg,   USER_TYPE_UINT16,  USER_RW,  &m_ConfigTrendTemp.stability[n].absOutlier_ms,    0,(MAX_TRENDS-1),    0,60000,      NULL            },\
  {"LOWER",         NO_NEXT_TABLE, Trend_UserCfg,   USER_TYPE_FLOAT,   USER_RW,  &m_ConfigTrendTemp.stability[n].criteria.lower,   0,(MAX_TRENDS-1),    NO_LIMIT,     NULL            },\
  {"UPPER",         NO_NEXT_TABLE, Trend_UserCfg,   USER_TYPE_FLOAT,   USER_RW,  &m_ConfigTrendTemp.stability[n].criteria.upper,   0,(MAX_TRENDS-1),    NO_LIMIT,     NULL            },\
  {"VARIANCE",      NO_NEXT_TABLE, Trend_UserCfg,   USER_TYPE_FLOAT,   USER_RW,  &m_ConfigTrendTemp.stability[n].criteria.variance,0,(MAX_TRENDS-1),    NO_LIMIT,     NULL            },\
  { NULL,           NULL,          NULL,            NO_HANDLER_DATA}\
}

  /* TREND STABILITY CRITERIA ELEMENTS 0 - 15 */
  DECL_TREND_STAB_TBL_ENTRY(0);
  DECL_TREND_STAB_TBL_ENTRY(1);
  DECL_TREND_STAB_TBL_ENTRY(2);
  DECL_TREND_STAB_TBL_ENTRY(3);
  DECL_TREND_STAB_TBL_ENTRY(4);
  DECL_TREND_STAB_TBL_ENTRY(5);
  DECL_TREND_STAB_TBL_ENTRY(6);
  DECL_TREND_STAB_TBL_ENTRY(7);
  DECL_TREND_STAB_TBL_ENTRY(8);
  DECL_TREND_STAB_TBL_ENTRY(9);
  DECL_TREND_STAB_TBL_ENTRY(10);
  DECL_TREND_STAB_TBL_ENTRY(11);
  DECL_TREND_STAB_TBL_ENTRY(12);
  DECL_TREND_STAB_TBL_ENTRY(13);
  DECL_TREND_STAB_TBL_ENTRY(14);
  DECL_TREND_STAB_TBL_ENTRY(15);


static USER_MSG_TBL stabCritTbl[] =
{
  { "S0",   trendStabTbl0 ,  NULL, NO_HANDLER_DATA },
  { "S1",   trendStabTbl1 ,  NULL, NO_HANDLER_DATA },
  { "S2",   trendStabTbl2 ,  NULL, NO_HANDLER_DATA },
  { "S3",   trendStabTbl3 ,  NULL, NO_HANDLER_DATA },
  { "S4",   trendStabTbl4 ,  NULL, NO_HANDLER_DATA },
  { "S5",   trendStabTbl5 ,  NULL, NO_HANDLER_DATA },
  { "S6",   trendStabTbl6 ,  NULL, NO_HANDLER_DATA },
  { "S7",   trendStabTbl7 ,  NULL, NO_HANDLER_DATA },
  { "S8",   trendStabTbl8 ,  NULL, NO_HANDLER_DATA },
  { "S9",   trendStabTbl9 ,  NULL, NO_HANDLER_DATA },
  { "S10",  trendStabTbl10,  NULL, NO_HANDLER_DATA },
  { "S11",  trendStabTbl11,  NULL, NO_HANDLER_DATA },
  { "S12",  trendStabTbl12,  NULL, NO_HANDLER_DATA },
  { "S13",  trendStabTbl13,  NULL, NO_HANDLER_DATA },
  { "S14",  trendStabTbl14,  NULL, NO_HANDLER_DATA },
  { "S15",  trendStabTbl15,  NULL, NO_HANDLER_DATA },
  { NULL,   NULL,            NULL, NO_HANDLER_DATA }
};

// Trends - TREND User and Configuration Table
static USER_MSG_TBL trendCmd [] =
{
  /* Str              Next Tbl Ptr               Handler Func.          Data Type          Access     Parameter                           IndexRange           DataLimit            EnumTbl*/
  { "NAME",           NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_STR,     USER_RW,   m_ConfigTrendTemp.trendName,          0,(MAX_TRENDS-1),    0,MAX_TREND_NAME,    NULL                 },
  { "COMMANDED",      NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_BOOLEAN, USER_RW,   &m_ConfigTrendTemp.bCommanded,        0,(MAX_TRENDS-1),    NO_LIMIT,            NULL                 },
  { "RATE",           NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &m_ConfigTrendTemp.rate,              0,(MAX_TRENDS-1),    NO_LIMIT,            trendRateType        },
  { "RATEOFFSET_MS",  NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_UINT32,  USER_RW,   &m_ConfigTrendTemp.nOffset_ms,        0,(MAX_TRENDS-1),    NO_LIMIT,            NULL                 },
  { "SAMPLEPERIOD_S", NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_UINT16,  USER_RW,   &m_ConfigTrendTemp.nSamplePeriod_s,   0,(MAX_TRENDS-1),    1,3600,              NULL                 },
  { "OFFSETSTABLE_S", NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_INT32,   USER_RW,   &m_ConfigTrendTemp.nOffsetStability_s,0,(MAX_TRENDS-1),    -1,3599,             NULL                 },
  { "ENGINEID",       NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &m_ConfigTrendTemp.engineRunId,       0,(MAX_TRENDS-1),    NO_LIMIT,            trendEngRunIdEnum    },
  { "MAXSAMPLECOUNT", NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_UINT16,  USER_RW,   &m_ConfigTrendTemp.maxTrends,         0,(MAX_TRENDS-1),    NO_LIMIT,            NULL                 },
  { "STARTTRIGID",    NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &m_ConfigTrendTemp.startTrigger,      0,(MAX_TRENDS-1),    0,MAX_TRIGGERS,      triggerIndexType     },
  { "RESETTRIGID",    NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &m_ConfigTrendTemp.resetTrigger,      0,(MAX_TRENDS-1),    0,MAX_TRIGGERS,      triggerIndexType     },
  { "INTERVAL_S",     NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_UINT32,  USER_RW,   &m_ConfigTrendTemp.trendInterval_s,   0,(MAX_TRENDS-1),    0,SECS_PER_DAY,      NULL                 },
  { "SENSORS",        NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_SNS_LIST,USER_RW,   m_ConfigTrendTemp.sensorMap,          0,(MAX_TRENDS-1),    0,MAX_TREND_SENSORS, NULL                 },
  { "CYCLEA",         NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &m_ConfigTrendTemp.cycle[0],          0,(MAX_TRENDS-1),    NO_LIMIT,            cycleEnumType        },
  { "CYCLEB",         NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &m_ConfigTrendTemp.cycle[1],          0,(MAX_TRENDS-1),    NO_LIMIT,            cycleEnumType        },
  { "CYCLEC",         NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &m_ConfigTrendTemp.cycle[2],          0,(MAX_TRENDS-1),    NO_LIMIT,            cycleEnumType        },
  { "CYCLED",         NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &m_ConfigTrendTemp.cycle[3],          0,(MAX_TRENDS-1),    NO_LIMIT,            cycleEnumType        },
  { "ACTION",         NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_UINT8,   USER_RW,   &m_ConfigTrendTemp.nAction,           0,(MAX_TRENDS-1),    NO_LIMIT,            NULL                 },
  { "STABLEPERIOD_S", NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_UINT16,  USER_RW,   &m_ConfigTrendTemp.stabilityPeriod_s, 0,(MAX_TRENDS-1),    0,3600,              NULL                 },
  { "STABILITY_MODE", NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &m_ConfigTrendTemp.stableStrategy,    0,(MAX_TRENDS-1),    NO_LIMIT,            trendStableRuleEnum},
  { "STABILITY",      stabCritTbl,              NULL,                   NO_HANDLER_DATA,                                                                                                                   },
  { NULL,             NULL,                     NULL,                   NO_HANDLER_DATA }
};


static USER_MSG_TBL trendStatus [] =
{
  /* Str                    Next Tbl Ptr       Handler Func.    Data Type          Access     Parameter                              IndexRange           DataLimit   EnumTbl*/
   { "STATE",               NO_NEXT_TABLE,     Trend_State,     USER_TYPE_ENUM,    USER_RO,   &m_StateTrendTemp.trendState,            0,(MAX_TRENDS-1),    NO_LIMIT,   trendStateEnum    },
   { "ENG_STATE",           NO_NEXT_TABLE,     Trend_State,     USER_TYPE_ENUM,    USER_RO,   &m_StateTrendTemp.prevEngState,          0,(MAX_TRENDS-1),    NO_LIMIT,   engineRunStateEnum},
   { "TRENDCOUNT",          NO_NEXT_TABLE,     Trend_State,     USER_TYPE_UINT16,  USER_RO,   &m_StateTrendTemp.trendCnt,              0,(MAX_TRENDS-1),    NO_LIMIT,   NULL              },
   { "AT_TRENDCOUNT",       NO_NEXT_TABLE,     Trend_State,     USER_TYPE_UINT16,  USER_RO,   &m_StateTrendTemp.autoTrendCnt,          0,(MAX_TRENDS-1),    NO_LIMIT,   NULL              },
   { "TIMESINCELAST_MS",    NO_NEXT_TABLE,     Trend_State,     USER_TYPE_UINT32,  USER_RO,   &m_StateTrendTemp.timeSinceLastTrendMs,  0,(MAX_TRENDS-1),    NO_LIMIT,   NULL              },
   // Stability data
   { "STABILITY_CNT",       NO_NEXT_TABLE,     Trend_State,     USER_TYPE_UINT16,  USER_RO,   &m_StateTrendTemp.curStability.stableCnt,0,(MAX_TRENDS-1),    NO_LIMIT,   NULL              },
   { "TIMESTABLE_MS",       NO_NEXT_TABLE,     Trend_State,     USER_TYPE_UINT32,  USER_RO,   &m_StateTrendTemp.nTimeStableMs,         0,(MAX_TRENDS-1),    NO_LIMIT,   NULL              },
   { "STABLE_STATE",        NO_NEXT_TABLE,     Trend_State,     USER_TYPE_ENUM,    USER_RO,   &m_StateTrendTemp.stableState,           0,(MAX_TRENDS-1),    NO_LIMIT,   trendStabStateEnum},
   { "LAST_SAMPLE_RESULT",  NO_NEXT_TABLE,     Trend_State,     USER_TYPE_ENUM,    USER_RO,   &m_StateTrendTemp.lastSampleResult,      0,(MAX_TRENDS-1),    NO_LIMIT,   trendSampleResultEnum},
   { NULL,                  NULL,              NULL ,           NO_HANDLER_DATA }
};

static USER_MSG_TBL trendDebugTbl [] =
{
  /* Str           Next Tbl Ptr       Handler Func.             Data Type          Access                           Parameter    IndexRange       DataLimit  EnumTbl */
  { "START",       NO_NEXT_TABLE,     Trend_Start,              USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG|USER_GSE),   NULL,     0,(MAX_TRENDS-1),  NO_LIMIT,  NULL},
  { "STOP",        NO_NEXT_TABLE,     Trend_Stop,               USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG|USER_GSE),   NULL,     0,(MAX_TRENDS-1),  NO_LIMIT,  NULL},
  { "GET_HIST",    NO_NEXT_TABLE,     Trend_DsplyStabilityHist, USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG|USER_GSE),   NULL,     0,(MAX_TRENDS-1),  NO_LIMIT,  NULL},
  { "GET_SAMPLE",  NO_NEXT_TABLE,     Trend_DsplySampleData,    USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG|USER_GSE),   NULL,     0,(MAX_TRENDS-1),  NO_LIMIT,  NULL},
  {"GET_STABILITY",NO_NEXT_TABLE,     Trend_DsplyStabilityState,USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG|USER_GSE),   NULL,     0,(MAX_TRENDS-1),  NO_LIMIT,  NULL},
  { NULL,          NULL,              NULL,                     NO_HANDLER_DATA}
};

static USER_MSG_TBL trendRoot [] =
{ /* Str            Next Tbl Ptr       Handler Func.             Data Type          Access                       Parameter       IndexRange     DataLimit  EnumTbl*/
   { "CFG",         trendCmd     ,     NULL,                     NO_HANDLER_DATA},
   { "STATUS",      trendStatus  ,     NULL,                     NO_HANDLER_DATA},
   { "DEBUG",       trendDebugTbl,     NULL,                     NO_HANDLER_DATA},
   { DISPLAY_CFG,   NO_NEXT_TABLE,     Trend_ShowConfig,         USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG|USER_GSE),   NULL,    -1, -1,             NO_LIMIT,  NULL},
   { NULL,          NULL,              NULL,                     NO_HANDLER_DATA}
};

static USER_MSG_TBL  rootTrendMsg = {"TREND",trendRoot,NULL,NO_HANDLER_DATA};

#pragma ghs endnowarning

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
* Function:     Trend_UserCfg
*
* Description:  Handles User Manager requests to change trend configuration
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
******************************************************************************/
static USER_HANDLER_RESULT Trend_UserCfg ( USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr)
{
   USER_HANDLER_RESULT result;

   result = USER_RESULT_OK;

   //Load trend structure into the temporary location based on index param
   //Param.Ptr points to the struct member to be read/written
   //in the temporary location
   memcpy(&m_ConfigTrendTemp,
          &CfgMgr_ConfigPtr()->TrendConfigs[Index],
          sizeof(m_ConfigTrendTemp));

   result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
   if(SetPtr != NULL && USER_RESULT_OK == result)
   {
     memcpy ( &CfgMgr_ConfigPtr()->TrendConfigs[Index],
              &m_ConfigTrendTemp,
              sizeof(TREND_CFG));
     //Store the modified temporary structure in the EEPROM.
     CfgMgr_StoreConfigItem ( CfgMgr_ConfigPtr(),
                              &CfgMgr_ConfigPtr()->TrendConfigs[Index],
                              sizeof(m_ConfigTrendTemp));
   }
   return result;
}

/******************************************************************************
* Function:     Trend_State
*
* Description:  Handles User Manager requests to retrieve the current Trend
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
static USER_HANDLER_RESULT Trend_State(USER_DATA_TYPE DataType,
                                  USER_MSG_PARAM Param,
                                  UINT32 Index,
                                  const void *SetPtr,
                                  void **GetPtr)
{
   USER_HANDLER_RESULT result;

   result = USER_RESULT_ERROR;

   memcpy(&m_StateTrendTemp, &m_TrendData[Index], sizeof(m_StateTrendTemp));

   result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

   return result;
}

/******************************************************************************
* Function:    Trend_ShowConfig
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
******************************************************************************/
static USER_HANDLER_RESULT Trend_ShowConfig ( USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr)
{
   CHAR  labelStem[] = "\r\n\r\nTREND.CFG";
   // Ensure buffer is lare enought for a long label. Factor of 3 is sufficient.
   CHAR  label[USER_MAX_MSG_STR_LEN * 3];
   INT16 i;

   USER_HANDLER_RESULT result = USER_RESULT_OK;
   USER_MSG_TBL*  pCfgTable;

   //Top-level name is a single indented space
   CHAR branchName[USER_MAX_MSG_STR_LEN] = " ";

   pCfgTable = &trendCmd[0];  // Get pointer to config entry

   for (i = 0; i < MAX_TRENDS && result == USER_RESULT_OK; ++i)
   {
      // Display element info above each set of data.
      snprintf(label, sizeof(label), "%s[%d]", labelStem, i);

      result = USER_RESULT_ERROR;
      if ( User_OutputMsgString( label, FALSE ) )
      {
         result = User_DisplayConfigTree(branchName, pCfgTable, i, 0, NULL);
      }
   }
   return result;
}

/******************************************************************************
 * Function:     Trend_Start()
 *
 * Description:  Test function to simulate call to start a command trend
 *
 * Parameters:   USER_DATA_TYPE DataType
 *               USER_MSG_PARAM Param
 *               UINT32         Index
 *               const void     *SetPtr
 *               void           **GetPtr
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static USER_HANDLER_RESULT Trend_Start(USER_DATA_TYPE DataType, USER_MSG_PARAM Param,
                                       UINT32 Index, const void *SetPtr, void **GetPtr)
{
   static CHAR  outputBuffer[USER_SINGLE_MSG_MAX_SIZE];

   if (TRUE != m_TrendCfg[Index].bCommanded)
   {
       snprintf(outputBuffer, sizeof(outputBuffer), 
                "Trend[%d] is not command type\r\n", Index);
   }
   else
   {
       TrendAppStartTrend( (TREND_INDEX)Index, TRUE);
       snprintf(outputBuffer, sizeof(outputBuffer), 
                "Trend[%d] Started\r\n", Index);
   }

   User_OutputMsgString(outputBuffer, FALSE);
   return USER_RESULT_OK;
}

/******************************************************************************
 * Function:     Trend_Stop()
 *
 * Description:  Test function to simulate call to start a command trend
 *
 * Parameters:   USER_DATA_TYPE DataType
 *               USER_MSG_PARAM Param
 *               UINT32         Index
 *               const void     *SetPtr
 *               void           **GetPtr
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static USER_HANDLER_RESULT Trend_Stop(USER_DATA_TYPE DataType,
                                      USER_MSG_PARAM Param,
                                      UINT32 Index,
                                      const void *SetPtr,
                                      void **GetPtr)
{
  static CHAR  outputBuffer[USER_SINGLE_MSG_MAX_SIZE];

  if (TRUE != m_TrendCfg[Index].bCommanded)
  {
    snprintf(outputBuffer, sizeof(outputBuffer), 
             "Trend[%d] is not command type\r\n", Index);
  }
  else
  {
    TrendAppStartTrend( (TREND_INDEX)Index, FALSE);
    snprintf(outputBuffer, sizeof(outputBuffer), 
             "Trend[%d] Stopped\r\n", Index);
  }
  
  User_OutputMsgString(outputBuffer, FALSE);
  return USER_RESULT_OK;
}

/******************************************************************************
 * Function:     Trend_DsplyStabilityHist()
 *
 * Description:  Test function to simulate call to TrendGetStabilityHistory
 *
 * Parameters:   USER_DATA_TYPE DataType
 *               USER_MSG_PARAM Param
 *               UINT32         Index
 *               const void     *SetPtr
 *               void           **GetPtr
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static USER_HANDLER_RESULT Trend_DsplyStabilityHist(USER_DATA_TYPE DataType,
                                                    USER_MSG_PARAM Param,
                                                    UINT32 Index,
                                                    const void *SetPtr,
                                                    void **GetPtr)
{
  UINT16 i;
  STABLE_HISTORY stbHist;
  CHAR         tempStr[ MAX_GET_HIST_LINE_LEN ];
  static CHAR  outputBuffer[USER_SINGLE_MSG_MAX_SIZE];
  UINT32 len;
  UINT32 nOffset = 0;
  BOOLEAN bResult;


  bResult = TrendGetStabilityHistory( (TREND_INDEX)Index, &stbHist ) ? TRUE : FALSE;
  // Build up the lines of stability history entries.
  for( i = 0;  bResult && i < MAX_STAB_SENSORS; ++i)
  {
    if (SENSOR_UNUSED == stbHist.sensorID[i])
    {
      break;
    }
    else
    {
      snprintf(tempStr, sizeof( tempStr),
               "\r\nEntry[%d]: Snsr: %d Stable: %s", i, stbHist.sensorID[i],
               TRUE == stbHist.bStable[i] ? "TRUE" : "FALSE");
      len = strlen ( tempStr );
      memcpy ( (void *) &outputBuffer[nOffset], tempStr, len );
      nOffset += len;
    }
  }

  snprintf ( tempStr, sizeof(tempStr), "\r\nStability History Display completed.\r\n");
  len = strlen ( tempStr );
  memcpy ( (void *) &outputBuffer[nOffset], tempStr, len );
  nOffset += len;
  outputBuffer[nOffset] = NULL;
  User_OutputMsgString(outputBuffer, FALSE);

  return USER_RESULT_OK;
}

/******************************************************************************
 * Function:     Trend_DsplySampleData()
 *
 * Description:  Test function to simulate call to TrendGetSampleData and display results
 *
 * Parameters:   USER_DATA_TYPE DataType
 *               USER_MSG_PARAM Param
 *               UINT32         Index
 *               const void     *SetPtr
 *               void           **GetPtr
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static USER_HANDLER_RESULT Trend_DsplySampleData(USER_DATA_TYPE DataType,
                                                 USER_MSG_PARAM Param,
                                                 UINT32 Index,
                                                 const void *SetPtr,
                                                 void **GetPtr)
{
  UINT16 i;
  TREND_SAMPLE_DATA sampleData;
  CHAR              tempStr[MAX_GET_SAMPLE_LINE_LEN];
  static CHAR       outputBuffer[USER_SINGLE_MSG_MAX_SIZE];
  UINT32 len;
  UINT32 nOffset = 0;


  TrendGetSampleData( (TREND_INDEX)Index, &sampleData );

  for( i = 0; i < MAX_TREND_SENSORS; ++i)
  {
     if (SENSOR_UNUSED == sampleData.snsrSummary[i].SensorIndex)
     {
       // All buffered, break out and print the msg on return.
       break;
     }
     else
     {
       snprintf(tempStr, sizeof( tempStr),
                "\r\nEntry[%d]:\r\n Snsr: %d\r\n SmplCnt: %d\r\n Vld: %s\r\n tmin: %d %d\r\n"
                " minVal: %8.2f\r\n tmax: %d %d\r\n maxVal: %8.2f\r\n avgVal: %8.2f\r\n"
                " totVal: %8.2f",
                i,
                sampleData.snsrSummary[i].SensorIndex,
                sampleData.snsrSummary[i].nSampleCount,
                (sampleData.snsrSummary[i].bValid == 1) ? "TRUE":"FALSE",
                sampleData.snsrSummary[i].timeMinValue.Timestamp,
                sampleData.snsrSummary[i].timeMinValue.MSecond,
                sampleData.snsrSummary[i].fMinValue,
                sampleData.snsrSummary[i].timeMaxValue.Timestamp,
                sampleData.snsrSummary[i].timeMaxValue.MSecond,
                sampleData.snsrSummary[i].fMaxValue,
                sampleData.snsrSummary[i].fAvgValue,
                sampleData.snsrSummary[i].fTotal);

       len = strlen ( tempStr );
       memcpy ( (void *) &outputBuffer[nOffset], tempStr, len );
       nOffset += len;
     }
   }

  // Terminate the output buffer and send it off for outputting
  snprintf( tempStr, sizeof(tempStr), "\r\nDisplay Sample Data completed\r\n");
  len = strlen ( tempStr );
  memcpy ( (void *) &outputBuffer[nOffset], tempStr, len );
  nOffset += len;
  outputBuffer[nOffset] = NULL;
  User_OutputMsgString(outputBuffer, FALSE);

  return USER_RESULT_OK;
}

/******************************************************************************
 * Function:     Trend_DsplyStabilityState()
 *
 * Description:  Test function to simulate APAC call to TrendGetStabilityState
 *
 * Parameters:   USER_DATA_TYPE DataType
 *               USER_MSG_PARAM Param
 *               UINT32         Index
 *               const void     *SetPtr
 *               void           **GetPtr
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static USER_HANDLER_RESULT Trend_DsplyStabilityState(USER_DATA_TYPE DataType,
                                                     USER_MSG_PARAM Param, UINT32 Index,
                                                     const void *SetPtr,  void **GetPtr)
{
  STABILITY_STATE stabState;
  UINT32          durMs;
  SAMPLE_RESULT   sampleResult;
  static CHAR  outputBuffer[USER_SINGLE_MSG_MAX_SIZE];

  TrendGetStabilityState( (TREND_INDEX)Index, &stabState, &durMs, &sampleResult );

  snprintf(outputBuffer, sizeof( outputBuffer),
             "StableState: %s\r\nDuration: %d\r\nLastResult: %s\r\n",
                              trendStabStateEnum[stabState].Str,
                              durMs,
                              trendSampleResultEnum[sampleResult].Str );
  User_OutputMsgString(outputBuffer, FALSE);  
  return USER_RESULT_OK;
}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: TrendUserTables.c $
 * 
 * *****************  Version 41  *****************
 * User: Contractor V&v Date: 3/18/16    Time: 5:43p
 * Updated in $/software/control processor/code/application
 * SCR #1300 - Removed unreachable code
 * 
 * *****************  Version 40  *****************
 * User: John Omalley Date: 3/18/16    Time: 9:23a
 * Updated in $/software/control processor/code/application
 * SCR 1300 - Code Review Updates
 * 
 * *****************  Version 39  *****************
 * User: Contractor V&v Date: 3/14/16    Time: 3:13p
 * Updated in $/software/control processor/code/application
 * SCR #1300 - Fix buffer overrun on Get_Sample command
 * 
 * *****************  Version 38  *****************
 * User: Contractor V&v Date: 3/07/16    Time: 8:08p
 * Updated in $/software/control processor/code/application
 * Updated Trend.debug cmds to output checksum
 * 
 * *****************  Version 37  *****************
 * User: Contractor V&v Date: 2/25/16    Time: 12:04p
 * Updated in $/software/control processor/code/application
 * SCR #1300 - Remove ASSERT check in TrendGetStabilityHistory
 * 
 * *****************  Version 36  *****************
 * User: Contractor V&v Date: 2/23/16    Time: 4:07p
 * Updated in $/software/control processor/code/application
 * SCR #1300 - Add a GSE debug to retrieve stability state, dur and last
 * result
 * 
 * *****************  Version 35  *****************
 * User: Contractor V&v Date: 12/15/15   Time: 5:06p
 * Updated in $/software/control processor/code/application
 * SCR #1300 updated/deleted GSE trend.cfg as identified in review.
 * 
 * *****************  Version 34  *****************
 * User: Contractor V&v Date: 12/11/15   Time: 5:29p
 * Updated in $/software/control processor/code/application
 * SCR #1300 Trend.debug section and stablie duration changed to all
 * 
 * *****************  Version 33  *****************
 * User: Contractor V&v Date: 12/07/15   Time: 6:06p
 * Updated in $/software/control processor/code/application
 * SCR #1300 CR review fixes for recently enabled debug functions
 * 
 * *****************  Version 32  *****************
 * User: Contractor V&v Date: 12/07/15   Time: 3:10p
 * Updated in $/software/control processor/code/application
 * SCR #1300 - CR compliance update, enabled trend debug cmds
 *
 * *****************  Version 31  *****************
 * User: Contractor V&v Date: 12/03/15   Time: 6:04p
 * Updated in $/software/control processor/code/application
 * SCR #1300 CR compliance changes
 *
 * *****************  Version 30  *****************
 * User: Contractor V&v Date: 11/30/15   Time: 3:34p
 * Updated in $/software/control processor/code/application
 * CodeReview compliance change - no logic changes.
 *
 * *****************  Version 29  *****************
 * User: Contractor V&v Date: 11/17/15   Time: 11:53a
 * Updated in $/software/control processor/code/application
 * SCR #1300 - Added limits to offset for sampling and outliners, misc
 * debug updates
 *
 * *****************  Version 28  *****************
 * User: Contractor V&v Date: 11/09/15   Time: 2:58p
 * Updated in $/software/control processor/code/application
 * Fixed abs stable count off-by-one bug and consolidation struct
 *
 * *****************  Version 27  *****************
 * User: Contractor V&v Date: 11/05/15   Time: 6:38p
 * Updated in $/software/control processor/code/application
 * SCR #1300 - Formal Implementation of variance outlier tolerance
 *
 * *****************  Version 26  *****************
 * User: Contractor V&v Date: 11/02/15   Time: 5:50p
 * Updated in $/software/control processor/code/application
 * SCR #1300 - StabilityHistory and cmd buffering
 *
 * *****************  Version 25  *****************
 * User: Contractor V&v Date: 10/21/15   Time: 7:07p
 * Updated in $/software/control processor/code/application
 * SCR #1300 - StabilityHistory and cmd buffering
 *
 * *****************  Version 24  *****************
 * User: Contractor V&v Date: 10/19/15   Time: 6:27p
 * Updated in $/software/control processor/code/application
 * SCR #1300 - APAC updates for API and stability calc
 *
 * *****************  Version 23  *****************
 * User: Contractor V&v Date: 10/05/15   Time: 6:35p
 * Updated in $/software/control processor/code/application
 * SCR #1300 - P100 Trend Processing Updates initial update
 *
 * *****************  Version 22  *****************
 * User: John Omalley Date: 2/05/15    Time: 10:48a
 * Updated in $/software/control processor/code/application
 * SCR 1267 - Code Review Updates
 *
 * *****************  Version 21  *****************
 * User: John Omalley Date: 12/05/14   Time: 4:33p
 * Updated in $/software/control processor/code/application
 * SCR 1267 - Updated showcfg access type
 *
 * *****************  Version 20  *****************
 * User: Contractor V&v Date: 13-01-08   Time: 12:00p
 * Updated in $/software/control processor/code/application
 * SCR #1197  Added comment for magic 3
 *
 * *****************  Version 19  *****************
 * User: Contractor V&v Date: 12/28/12   Time: 5:51p
 * Updated in $/software/control processor/code/application
 * SCR #1197 Pass eng state to cycle.
 *
 * *****************  Version 18  *****************
 * User: Contractor V&v Date: 12/28/12   Time: 11:36a
 * Updated in $/software/control processor/code/application
 * Add AutoTrend count to GSE
 *
 * *****************  Version 17  *****************
 * User: Contractor V&v Date: 12/05/12   Time: 4:16p
 * Updated in $/software/control processor/code/application
 * SCR #1107 Code Review
 *
 * *****************  Version 16  *****************
 * User: Contractor V&v Date: 12/03/12   Time: 5:30p
 * Updated in $/software/control processor/code/application
 * SCR #1200 Requirements: GSE command to view the number of trend
 *
 * *****************  Version 15  *****************
 * User: Contractor V&v Date: 11/29/12   Time: 7:46p
 * Updated in $/software/control processor/code/application
 * SCR #1107 Code Review changes
 *
 * *****************  Version 14  *****************
 * User: Contractor V&v Date: 11/29/12   Time: 4:20p
 * Updated in $/software/control processor/code/application
 * SCR #1200 Requirements: GSE command to view the number of trend
 *
 * *****************  Version 13  *****************
 * User: Contractor V&v Date: 11/26/12   Time: 12:33p
 * Updated in $/software/control processor/code/application
 * SCR #  BitBucket #61 Cycle does not work with ER_ANY
 *
 * *****************  Version 12  *****************
 * User: Melanie Jutras Date: 12-11-15   Time: 2:55p
 * Updated in $/software/control processor/code/application
 * SCR #1142 File Format Errors
 *
 * *****************  Version 11  *****************
 * User: Contractor V&v Date: 11/12/12   Time: 6:39p
 * Updated in $/software/control processor/code/application
 * Code Review
 *
 * *****************  Version 10  *****************
 * User: John Omalley Date: 12-10-30   Time: 5:48p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Changed Actions to UINT8
 *
 * *****************  Version 9  *****************
 * User: Contractor V&v Date: 10/30/12   Time: 4:01p
 * Updated in $/software/control processor/code/application
 *
 * *****************  Version 8  *****************
 * User: John Omalley Date: 12-10-23   Time: 2:19p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Updates per Software Design Review
 * Dave
 * 1. Removed Trends is Active Function
 * 2. Fixed bug with deactivating manual trend because no stability
 * JPO
 * 1. Updated logs
 *    a. Combined Auto and Manual Start into one log
 *    b. Change fail log from name to index
 *    c. Trend not detected added index
 * 2. Removed Trend Lamp
 * 3. Updated Configuration defaults
 * 4. Updated user tables per design review
 *
 * *****************  Version 7  *****************
 * User: Contractor V&v Date: 12-10-02   Time: 1:19p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 Implement Trend Action
 *
 * *****************  Version 6  *****************
 * User: Contractor V&v Date: 12-09-19   Time: 3:22p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2  Fix AutoTrends
 *
 * *****************  Version 5  *****************
 * User: Contractor V&v Date: 9/14/12    Time: 4:42p
 * Updated in $/software/control processor/code/application
 *
 * *****************  Version 4  *****************
 * User: John Omalley Date: 12-09-11   Time: 2:20p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - General Trend Development and Binary ETM Header
 *
 * *****************  Version 3  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 12:43p
 * Updated in $/software/control processor/code/application
 * SCR# 1142
 *
 * *****************  Version 2  *****************
 * User: Contractor V&v Date: 8/08/12    Time: 5:40p
 * Updated in $/software/control processor/code/application
 *
 * *****************  Version 1  *****************
 * User: John Omalley Date: 12-06-07   Time: 11:32a
 * Created in $/software/control processor/code/application
 *
 *
 *
 *
 ***************************************************************************/


