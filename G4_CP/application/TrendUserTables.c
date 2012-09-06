#define TREND_USERTABLES_BODY
/******************************************************************************
         Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
            All Rights Reserved. Proprietary and Confidential.

  File:        TrendUserTables.c

Description:   User command structures and functions for the trend processing

VERSION
$Revision: 3 $  $Date: 8/28/12 12:43p $
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
/* Local Defines                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
//Prototype for the User Manager message handlers, has to go before
//the local variable tables that use the function pointer.
USER_HANDLER_RESULT Trend_UserCfg       ( USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr );

USER_HANDLER_RESULT  Trend_State         ( USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr );

USER_HANDLER_RESULT Trend_ShowConfig       ( USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr );

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static TREND_CFG         ConfigTrendTemp;      // Trend Configuration temp Storage

static TREND_DATA        StateTrendTemp;       // Trend State Temp Storage


USER_ENUM_TBL TrendType[]   =
{ 
   { "0"  , TREND_0   }, { "1"  ,  TREND_1   }, {  "2"  , TREND_2   },
   { "3"  , TREND_3   }, { "4"  ,  TREND_4   }, { "UNUSED", TREND_UNUSED },
   { NULL, 0}
};


USER_ENUM_TBL TrendRateType[] =
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

USER_ENUM_TBL TrendEngRunIdEnum[] =
{  
  { "0"     , ENGRUN_ID_0  },
  { "1"     , ENGRUN_ID_1  },
  { "2"     , ENGRUN_ID_2  },
  { "3"     , ENGRUN_ID_3  },
  { "100"   , ENGRUN_ID_ANY},
  { "UNUSED", ENGRUN_UNUSED},
  { NULL, 0 }
};



#pragma ghs nowarning 1545 //Suppress packed structure alignment warning




// Macro defines the 'prototype' declaration for entry in StabCritTbl
#define TREND_STAB_TBL_ENTRY_DECL( n )\
  static USER_MSG_TBL TrendStabTbl##n [] =\
{\
  {"SENSORID",   NO_NEXT_TABLE, Trend_UserCfg,   USER_TYPE_ENUM,    USER_RW,  &ConfigTrendTemp.Stability[n].SensorIndex,      0,(MAX_TRENDS-1),    NO_LIMIT,     SensorIndexType },\
  {"LOWER",      NO_NEXT_TABLE, Trend_UserCfg,   USER_TYPE_FLOAT,   USER_RW,  &ConfigTrendTemp.Stability[n].Criteria.Lower,   0,(MAX_TRENDS-1),    NO_LIMIT,     NULL            },\
  {"UPPER",      NO_NEXT_TABLE, Trend_UserCfg,   USER_TYPE_FLOAT,   USER_RW,  &ConfigTrendTemp.Stability[n].Criteria.Upper,   0,(MAX_TRENDS-1),    NO_LIMIT,     NULL            },\
  {"VARIANCE",   NO_NEXT_TABLE, Trend_UserCfg,   USER_TYPE_FLOAT,   USER_RW,  &ConfigTrendTemp.Stability[n].Criteria.Variance,0,(MAX_TRENDS-1),    NO_LIMIT,     NULL            },\
  { NULL,        NULL,          NULL, NO_HANDLER_DATA}\
}

  /* TREND STABILITY CRITERIA ELEMENTS 0 - 15 */
  TREND_STAB_TBL_ENTRY_DECL(0);
  TREND_STAB_TBL_ENTRY_DECL(1);
  TREND_STAB_TBL_ENTRY_DECL(2);
  TREND_STAB_TBL_ENTRY_DECL(3);
  TREND_STAB_TBL_ENTRY_DECL(4);
  TREND_STAB_TBL_ENTRY_DECL(5);
  TREND_STAB_TBL_ENTRY_DECL(6);
  TREND_STAB_TBL_ENTRY_DECL(7);
  TREND_STAB_TBL_ENTRY_DECL(8);
  TREND_STAB_TBL_ENTRY_DECL(9);
  TREND_STAB_TBL_ENTRY_DECL(10);
  TREND_STAB_TBL_ENTRY_DECL(11);
  TREND_STAB_TBL_ENTRY_DECL(12);
  TREND_STAB_TBL_ENTRY_DECL(13);
  TREND_STAB_TBL_ENTRY_DECL(14);
  TREND_STAB_TBL_ENTRY_DECL(15);


static USER_MSG_TBL StabCritTbl[] =
{
  { "S0",   TrendStabTbl0 ,  NULL, NO_HANDLER_DATA },
  { "S1",   TrendStabTbl1 ,  NULL, NO_HANDLER_DATA },
  { "S2",   TrendStabTbl2 ,  NULL, NO_HANDLER_DATA },
  { "S3",   TrendStabTbl3 ,  NULL, NO_HANDLER_DATA },
  { "S4",   TrendStabTbl4 ,  NULL, NO_HANDLER_DATA },
  { "S5",   TrendStabTbl5 ,  NULL, NO_HANDLER_DATA },
  { "S6",   TrendStabTbl6 ,  NULL, NO_HANDLER_DATA },
  { "S7",   TrendStabTbl7 ,  NULL, NO_HANDLER_DATA },
  { "S8",   TrendStabTbl8 ,  NULL, NO_HANDLER_DATA },
  { "S9",   TrendStabTbl9 ,  NULL, NO_HANDLER_DATA },
  { "S10",  TrendStabTbl10,  NULL, NO_HANDLER_DATA },
  { "S11",  TrendStabTbl11,  NULL, NO_HANDLER_DATA },
  { "S12",  TrendStabTbl12,  NULL, NO_HANDLER_DATA },
  { "S13",  TrendStabTbl13,  NULL, NO_HANDLER_DATA },
  { "S14",  TrendStabTbl14,  NULL, NO_HANDLER_DATA },
  { "S15",  TrendStabTbl15,  NULL, NO_HANDLER_DATA },  
  { NULL,   NULL,            NULL, NO_HANDLER_DATA }
};


// Trends - TREND User and Configuration Table
static USER_MSG_TBL TrendCmd [] =
{
  /* Str              Next Tbl Ptr               Handler Func.          Data Type          Access     Parameter                           IndexRange           DataLimit            EnumTbl*/
  { "NAME",           NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_STR,     USER_RW,   &ConfigTrendTemp.TrendName,         0,(MAX_TRENDS-1),    0,MAX_TREND_NAME,    NULL                 },
  { "RATE",           NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &ConfigTrendTemp.Rate,              0,(MAX_TRENDS-1),    NO_LIMIT,            TrendRateType        },
  { "RATEOFFSET_MS",  NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_UINT32,  USER_RW,   &ConfigTrendTemp.nOffset_ms,        0,(MAX_TRENDS-1),    NO_LIMIT,            NULL                 },
  { "SAMPLEPERIOD_S", NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_UINT16,  USER_RW,   &ConfigTrendTemp.nSamplePeriod_s,   0,(MAX_TRENDS-1),    NO_LIMIT,            NULL                 },
  { "ENGINEID",       NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &ConfigTrendTemp.EngineRunId,       0,(MAX_TRENDS-1),    NO_LIMIT,            TrendEngRunIdEnum    },
  { "SAMPLECOUNT",    NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_UINT16,  USER_RW,   &ConfigTrendTemp.nTrendSamples,     0,(MAX_TRENDS-1),    NO_LIMIT,            NULL                 },
  { "STARTTRIGID",    NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &ConfigTrendTemp.StartTrigger,      0,(MAX_TRENDS-1),    NO_LIMIT,            TriggerIndexType     },
  { "ENDTRIGID",      NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &ConfigTrendTemp.EndTrigger,        0,(MAX_TRENDS-1),    NO_LIMIT,            TriggerIndexType     },
  { "INTERVAL_S",     NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_UINT32,  USER_RW,   &ConfigTrendTemp.TrendInterval_s,   0,(MAX_TRENDS-1),    NO_LIMIT,            NULL                 },
  { "SENSORS",        NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_128_LIST,USER_RW,   &ConfigTrendTemp.SensorMap,         0,(MAX_TRENDS-1),    0,MAX_TREND_SENSORS, NULL                 },
  { "CYCLEA",         NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &ConfigTrendTemp.nCycleA,           0,(MAX_TRENDS-1),    NO_LIMIT,            CycleEnumType        },
  { "CYCLEB",         NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &ConfigTrendTemp.nCycleB,           0,(MAX_TRENDS-1),    NO_LIMIT,            CycleEnumType        },
  { "CYCLEC",         NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &ConfigTrendTemp.nCycleC,           0,(MAX_TRENDS-1),    NO_LIMIT,            CycleEnumType        },
  { "CYCLED",         NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_ENUM,    USER_RW,   &ConfigTrendTemp.nCycleD,           0,(MAX_TRENDS-1),    NO_LIMIT,            CycleEnumType        },
  { "LSS_MASK",       NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_HEX8,    USER_RW,   &ConfigTrendTemp.LssMask,           0,(MAX_TRENDS-1),    NO_LIMIT,            NULL                 },
  { "STABLE_PERIOD_S",NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_UINT16,  USER_RW,   &ConfigTrendTemp.nTimeStable_s,     0,(MAX_TRENDS-1),    NO_LIMIT,            NULL                 },
  { "LAMP_ENABLED",   NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_BOOLEAN, USER_RW,   &ConfigTrendTemp.lampEnabled,       0,(MAX_TRENDS-1),    NO_LIMIT,            NULL                 },
  { "STABILITY",      StabCritTbl,              NULL,                   NO_HANDLER_DATA,                                                                                                                 },
  { NULL,             NULL,                     NULL,                   NO_HANDLER_DATA }
};


static USER_MSG_TBL TrendStatus [] =
{
  /* Str              Next Tbl Ptr       Handler Func.    Data Type          Access     Parameter                           IndexRange           DataLimit   EnumTbl*/
   { "ID",            NO_NEXT_TABLE,     Trend_State,     USER_TYPE_ENUM,    USER_RO,   &StateTrendTemp.TrendIndex,         0,(MAX_TRENDS-1),    NO_LIMIT,   TrendType         },
   { "ACTIVE",        NO_NEXT_TABLE,     Trend_State,     USER_TYPE_BOOLEAN, USER_RO,   &StateTrendTemp.bTrendActive,       0,(MAX_TRENDS-1),    NO_LIMIT,   NULL              },
   { "AUTO_TREND",    NO_NEXT_TABLE,     Trend_State,     USER_TYPE_BOOLEAN, USER_RO,   &StateTrendTemp.bIsAutoTrend,       0,(MAX_TRENDS-1),    NO_LIMIT,   NULL              },
   { "NEXT_SAMPLE_MS",NO_NEXT_TABLE,     Trend_State,     USER_TYPE_UINT32,  USER_RO,   &StateTrendTemp.TimeNextSample,     0,(MAX_TRENDS-1),    NO_LIMIT,   NULL              },
   {" ENG_STATE",     NO_NEXT_TABLE,     Trend_State,     USER_TYPE_ENUM,    USER_RO,   &StateTrendTemp.PrevEngState,       0,(MAX_TRENDS-1),    NO_LIMIT,   EngineRunStateEnum},
   { NULL,            NULL,              NULL ,           NO_HANDLER_DATA } 
};

static USER_MSG_TBL TrendRoot [] =
{ /* Str            Next Tbl Ptr       Handler Func.        Data Type          Access            Parameter      IndexRange   DataLimit  EnumTbl*/
   { "CFG",         TrendCmd     ,     NULL,                NO_HANDLER_DATA},
   { "STATUS",      TrendStatus  ,     NULL,                NO_HANDLER_DATA},
   { DISPLAY_CFG,   NO_NEXT_TABLE,     Trend_ShowConfig,    USER_TYPE_ACTION,  USER_RO|USER_GSE, NULL,          -1, -1,      NO_LIMIT,  NULL},
   { NULL,          NULL,              NULL,                NO_HANDLER_DATA}
};

static USER_MSG_TBL  RootTrendMsg = {"TREND",TrendRoot,NULL,NO_HANDLER_DATA};

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
USER_HANDLER_RESULT Trend_UserCfg ( USER_DATA_TYPE DataType,
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
   memcpy(&ConfigTrendTemp,
          &CfgMgr_ConfigPtr()->TrendConfigs[Index],
          sizeof(ConfigTrendTemp));

   result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
   if(SetPtr != NULL && USER_RESULT_OK == result)
   {
     memcpy ( &CfgMgr_ConfigPtr()->TrendConfigs[Index],
              &ConfigTrendTemp,
              sizeof(TREND_CFG));
     //Store the modified temporary structure in the EEPROM.
     CfgMgr_StoreConfigItem ( CfgMgr_ConfigPtr(),
                              &CfgMgr_ConfigPtr()->TrendConfigs[Index],
                              sizeof(ConfigTrendTemp));
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

USER_HANDLER_RESULT Trend_State(USER_DATA_TYPE DataType,
                                  USER_MSG_PARAM Param,
                                  UINT32 Index,
                                  const void *SetPtr,
                                  void **GetPtr)
{
   USER_HANDLER_RESULT result;

   result = USER_RESULT_ERROR;

   memcpy(&StateTrendTemp, &m_TrendData[Index], sizeof(StateTrendTemp));

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
USER_HANDLER_RESULT Trend_ShowConfig ( USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr)
{
   CHAR  LabelStem[] = "\r\n\r\nTREND.CFG";
   CHAR  Label[USER_MAX_MSG_STR_LEN * 3];
   INT16 i;

   USER_HANDLER_RESULT result = USER_RESULT_OK;
   USER_MSG_TBL*  pCfgTable;

   //Top-level name is a single indented space
   CHAR BranchName[USER_MAX_MSG_STR_LEN] = " ";

   pCfgTable = &TrendCmd[0];  // Get pointer to config entry

   for (i = 0; i < MAX_EVENTS && result == USER_RESULT_OK; ++i)
   {
      // Display element info above each set of data.
      sprintf(Label, "%s[%d]", LabelStem, i);

      result = USER_RESULT_ERROR;
      if ( User_OutputMsgString( Label, FALSE ) )
      {
         result = User_DisplayConfigTree(BranchName, pCfgTable, i, 0, NULL);
      }
   }
   return result;
}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: TrendUserTables.c $
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


