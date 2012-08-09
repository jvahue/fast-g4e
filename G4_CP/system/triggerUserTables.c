/******************************************************************************
         Copyright (C) 2008-2010 Pratt & Whitney Engine Services, Inc. 
                     Altair Engine Diagnostic Solutions
            All Rights Reserved. Proprietary and Confidential.

  File:        triggerUserTables.c 

Description:   User command structures and functions for the trigger processing

VERSION
$Revision: 22 $  $Date: 8/08/12 3:49p $    
******************************************************************************/
#ifndef TRIGGER_BODY
#error triggerUserTables.c should only be included by trigger.c
#endif

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static TRIGGER_CONFIG ConfigTriggerTemp; // Trigger Configuration
                                         // temp storage
static TRIGGER_DATA   StateTriggerTemp;

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
//Prototype for the User Manager message handlers, has to go before
//the local variable tables that use the function pointer.
USER_HANDLER_RESULT Trigger_UserCfg(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr);

USER_HANDLER_RESULT Trigger_State(USER_DATA_TYPE DataType,
                                      USER_MSG_PARAM Param,
                                      UINT32 Index,
                                      const void *SetPtr,
                                      void **GetPtr);

USER_HANDLER_RESULT Trigger_ShowConfig(USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr);

USER_HANDLER_RESULT Trigger_CfgExprStrCmd(USER_DATA_TYPE DataType,
                                             USER_MSG_PARAM Param,
                                             UINT32 Index,
                                             const void *SetPtr,
                                             void **GetPtr);



/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/

USER_ENUM_TBL TriggerIndexType[]   = 
{ { "0"  , TRIGGER_0   }, { "1"  , TRIGGER_1   }, { "2"  , TRIGGER_2   }, 
  { "3"  , TRIGGER_3   }, { "4"  , TRIGGER_4   }, { "5"  , TRIGGER_5   }, 
  { "6"  , TRIGGER_6   }, { "7"  , TRIGGER_7   }, { "8"  , TRIGGER_8   },
  { "9"  , TRIGGER_9   }, { "10" , TRIGGER_10  }, { "11" , TRIGGER_11  },
  { "12" , TRIGGER_12  }, { "13" , TRIGGER_13  }, { "14" , TRIGGER_14  },
  { "15" , TRIGGER_15  }, { "16" , TRIGGER_16  }, { "17" , TRIGGER_17  },
  { "18" , TRIGGER_18  }, { "19" , TRIGGER_19  }, { "20" , TRIGGER_20  },
  { "21" , TRIGGER_21  }, { "22" , TRIGGER_22  }, { "23" , TRIGGER_23  },
  { "24" , TRIGGER_24  }, { "25" , TRIGGER_25  }, { "26" , TRIGGER_26  },
  { "27" , TRIGGER_27  }, { "28" , TRIGGER_28  }, { "29" , TRIGGER_29  },
  { "30" , TRIGGER_30  }, { "31" , TRIGGER_31  }, { "32" , TRIGGER_31  },
  { "33" , TRIGGER_33  }, { "34" , TRIGGER_34  }, { "35" , TRIGGER_35  },
  { "36" , TRIGGER_36  }, { "37" , TRIGGER_37  }, { "38" , TRIGGER_38  },
  { "39" , TRIGGER_39  }, { "40" , TRIGGER_40  }, { "41" , TRIGGER_41  },
  { "42" , TRIGGER_42  }, { "43" , TRIGGER_43  }, { "44" , TRIGGER_44  },
  { "45" , TRIGGER_45  }, { "46" , TRIGGER_46  }, { "47" , TRIGGER_47  },
  { "48" , TRIGGER_48  }, { "49" , TRIGGER_49  }, { "50" , TRIGGER_50  },
  { "51" , TRIGGER_51  }, { "52" , TRIGGER_52  }, { "53" , TRIGGER_53  },
  { "54" , TRIGGER_54  }, { "55" , TRIGGER_55  }, { "56" , TRIGGER_56  },
  { "57" , TRIGGER_57  }, { "58" , TRIGGER_58  }, { "59" , TRIGGER_59  },
  { "60" , TRIGGER_60  }, { "61" , TRIGGER_61  }, { "62" , TRIGGER_62  },
  { "63" , TRIGGER_63  },

  { "UNUSED", TRIGGER_UNUSED }, { NULL, 0 }
};


USER_ENUM_TBL TrigRateType[]    =  { { "1HZ"    , TR_1HZ          },
                                     { "2HZ"    , TR_2HZ          }, 
                                     { "4HZ"    , TR_4HZ          }, 
                                     { "5HZ"    , TR_5HZ          }, 
                                     { "10HZ"   , TR_10HZ         }, 
                                     { "20HZ"   , TR_20HZ         }, 
                                     { "50HZ"   , TR_50HZ         },
                                     { NULL     , 0               }
                                   };

USER_ENUM_TBL TrigStateEnum[]  = { {"NOT_USED",     TRIG_NONE },
                                   {"NOT_ACTIVE",   TRIG_START },
                                   {"ACTIVE",       TRIG_ACTIVE},
                                   {NULL,           0}
                                 };

#pragma ghs nowarning 1545 //Suppress packed structure alignment warning

// Triggers - TRIGGER User and Configuration Table
static
USER_MSG_TBL TriggerSensorCfgCmd [] =
{
     /*Str             Next Tbl Ptr   Handler Func.    Data Type        Access     Parameter                                        IndexRange           DataLimit EnumTbl*/ 
    { "INDEXA"       , NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_ENUM,  USER_RW,   &ConfigTriggerTemp.TrigSensor[0].SensorIndex,    0,(MAX_TRIGGERS-1),  NO_LIMIT, SensorIndexType },
    { "STARTVALUEA"  , NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_FLOAT, USER_RW,   &ConfigTriggerTemp.TrigSensor[0].Start.fValue,   0,(MAX_TRIGGERS-1),  NO_LIMIT, NULL },
    { "STARTCOMPAREA", NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_ENUM,  USER_RW,   &ConfigTriggerTemp.TrigSensor[0].Start.Compare,  0,(MAX_TRIGGERS-1),  NO_LIMIT, ComparisonEnum},
    { "ENDVALUEA"    , NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_FLOAT, USER_RW,   &ConfigTriggerTemp.TrigSensor[0].End.fValue,     0,(MAX_TRIGGERS-1),  NO_LIMIT, NULL },
    { "ENDCOMPAREA"  , NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_ENUM,  USER_RW,   &ConfigTriggerTemp.TrigSensor[0].End.Compare,    0,(MAX_TRIGGERS-1),  NO_LIMIT, ComparisonEnum},
    { "INDEXB"       , NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_ENUM,  USER_RW,   &ConfigTriggerTemp.TrigSensor[1].SensorIndex,    0,(MAX_TRIGGERS-1),  NO_LIMIT, SensorIndexType },
    { "STARTVALUEB"  , NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_FLOAT, USER_RW,   &ConfigTriggerTemp.TrigSensor[1].Start.fValue,   0,(MAX_TRIGGERS-1),  NO_LIMIT, NULL },
    { "STARTCOMPAREB", NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_ENUM,  USER_RW,   &ConfigTriggerTemp.TrigSensor[1].Start.Compare,  0,(MAX_TRIGGERS-1),  NO_LIMIT, ComparisonEnum},
    { "ENDVALUEB"    , NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_FLOAT, USER_RW,   &ConfigTriggerTemp.TrigSensor[1].End.fValue,     0,(MAX_TRIGGERS-1),  NO_LIMIT, NULL },
    { "ENDCOMPAREB"  , NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_ENUM,  USER_RW,   &ConfigTriggerTemp.TrigSensor[1].End.Compare,    0,(MAX_TRIGGERS-1),  NO_LIMIT, ComparisonEnum},
    { "INDEXC"       , NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_ENUM,  USER_RW,   &ConfigTriggerTemp.TrigSensor[2].SensorIndex,    0,(MAX_TRIGGERS-1),  NO_LIMIT, SensorIndexType},
    { "STARTVALUEC"  , NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_FLOAT, USER_RW,   &ConfigTriggerTemp.TrigSensor[2].Start.fValue,   0,(MAX_TRIGGERS-1),  NO_LIMIT, NULL },
    { "STARTCOMPAREC", NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_ENUM,  USER_RW,   &ConfigTriggerTemp.TrigSensor[2].Start.Compare,  0,(MAX_TRIGGERS-1),  NO_LIMIT, ComparisonEnum},
    { "ENDVALUEC"    , NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_FLOAT, USER_RW,   &ConfigTriggerTemp.TrigSensor[2].End.fValue,     0,(MAX_TRIGGERS-1),  NO_LIMIT, NULL },
    { "ENDCOMPAREC"  , NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_ENUM,  USER_RW,   &ConfigTriggerTemp.TrigSensor[2].End.Compare,    0,(MAX_TRIGGERS-1),  NO_LIMIT, ComparisonEnum},
    { "INDEXD"       , NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_ENUM,  USER_RW,   &ConfigTriggerTemp.TrigSensor[3].SensorIndex,    0,(MAX_TRIGGERS-1),  NO_LIMIT, SensorIndexType },
    { "STARTVALUED"  , NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_FLOAT, USER_RW,   &ConfigTriggerTemp.TrigSensor[3].Start.fValue,   0,(MAX_TRIGGERS-1),  NO_LIMIT, NULL },
    { "STARTCOMPARED", NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_ENUM,  USER_RW,   &ConfigTriggerTemp.TrigSensor[3].Start.Compare,  0,(MAX_TRIGGERS-1),  NO_LIMIT, ComparisonEnum},
    { "ENDVALUED"    , NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_FLOAT, USER_RW,   &ConfigTriggerTemp.TrigSensor[3].End.fValue,     0,(MAX_TRIGGERS-1),  NO_LIMIT, NULL },
    { "ENDCOMPARED"  , NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_ENUM,  USER_RW,   &ConfigTriggerTemp.TrigSensor[3].End.Compare,    0,(MAX_TRIGGERS-1),  NO_LIMIT, ComparisonEnum},
    { NULL,            NULL,          NULL,            NO_HANDLER_DATA }
};

// Trigger user and configuration Table
static USER_MSG_TBL TriggerCmd [] =
{
  /* Str              Next Tbl Ptr               Handler Func.             Data Type          Access     Parameter                           IndexRange           DataLimit            EnumTbl*/
  { "NAME",           NO_NEXT_TABLE,            Trigger_UserCfg,           USER_TYPE_STR,     USER_RW,   &ConfigTriggerTemp.TriggerName,     0,(MAX_TRIGGERS-1),  0,MAX_TRIGGER_NAME,  NULL },
  { "SENSOR",         &TriggerSensorCfgCmd[0],  NULL,                      NO_HANDLER_DATA },
  { "DURATION_MS",    NO_NEXT_TABLE,            Trigger_UserCfg,           USER_TYPE_UINT32,  USER_RW,   &ConfigTriggerTemp.nMinDuration_ms, 0,(MAX_TRIGGERS-1),  NO_LIMIT,            NULL },
  { "RATE",           NO_NEXT_TABLE,            Trigger_UserCfg,           USER_TYPE_ENUM,    USER_RW,   &ConfigTriggerTemp.Rate,            0,(MAX_TRIGGERS-1),  NO_LIMIT,            TrigRateType },
  { "RATEOFFSET_MS",  NO_NEXT_TABLE,            Trigger_UserCfg,           USER_TYPE_UINT32,  USER_RW,   &ConfigTriggerTemp.nOffset_ms,      0,(MAX_TRIGGERS-1),  0,1000,              NULL },
  { "SC",             NO_NEXT_TABLE,            Trigger_CfgExprStrCmd,     USER_TYPE_STR,     USER_RW,   &ConfigTriggerTemp.StartExpr,       0,(MAX_TRIGGERS-1),  NO_LIMIT,            NULL },
  { "EC",             NO_NEXT_TABLE,            Trigger_CfgExprStrCmd,     USER_TYPE_STR,     USER_RW,   &ConfigTriggerTemp.EndExpr,         0,(MAX_TRIGGERS-1),  NO_LIMIT,            NULL },
  { NULL,             NULL,                     NULL,                      NO_HANDLER_DATA }
};


static USER_MSG_TBL TriggerStatus [] =
{
  /* Str            Next Tbl Ptr       Handler Func.    Data Type        Access     Parameter                           IndexRange           DataLimit   EnumTbl*/
  { "STATE"       , NO_NEXT_TABLE,     Trigger_State,  USER_TYPE_ENUM,   USER_RO    , &StateTriggerTemp.State,          0, MAX_TRIGGERS - 1, NO_LIMIT,   TrigStateEnum },
  { "STARTTIME_MS", NO_NEXT_TABLE,     Trigger_State,  USER_TYPE_UINT32, USER_RO    , &StateTriggerTemp.nStartTime_ms,  0, MAX_TRIGGERS - 1, NO_LIMIT,   NULL },
  { "DURATION_MS" , NO_NEXT_TABLE,     Trigger_State,  USER_TYPE_UINT32, USER_RO    , &StateTriggerTemp.nDuration_ms,   0, MAX_TRIGGERS - 1, NO_LIMIT,   NULL },
  { "SAMPLECOUNT" , NO_NEXT_TABLE,     Trigger_State,  USER_TYPE_UINT16, USER_RO    , &StateTriggerTemp.nSampleCount,   0, MAX_TRIGGERS - 1, NO_LIMIT,   NULL },
  { NULL          , NO_NEXT_TABLE,     Trigger_State,  USER_TYPE_NONE,   USER_RW    , NULL,                             0, MAX_TRIGGERS  -1, NO_LIMIT,   NULL },
};

static USER_MSG_TBL TriggerRoot [] =
{ /* Str            Next Tbl Ptr       Handler Func.        Data Type          Access            Parameter      IndexRange   DataLimit      EnumTbl*/
   { "CFG",         TriggerCmd   ,     NULL,                NO_HANDLER_DATA},
   { "STATUS",      TriggerStatus,     NULL,                NO_HANDLER_DATA},
   { "FLAGS",       NO_NEXT_TABLE,     Trigger_State,       USER_TYPE_HEX128,  USER_RO,          &TriggerFlags, -1, -1,      HEX128_LIMIT,  NULL },
   { DISPLAY_CFG,   NO_NEXT_TABLE,     Trigger_ShowConfig,  USER_TYPE_ACTION,  USER_RO|USER_GSE, NULL,          -1, -1,      NO_LIMIT,      NULL},
   { NULL,          NULL,              NULL,                NO_HANDLER_DATA}
};

static
USER_MSG_TBL    RootTriggerMsg = {"TRIGGER",TriggerRoot,NULL,NO_HANDLER_DATA};

#pragma ghs endnowarning

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
* Function:     Trigger_UserCfg
*
* Description:  Handles User Manager requests to change trigger configuration
*               items.  
*               
* Parameters:   [in] DataType:  C type of the data to be read or changed, used 
*                               for casting the data pointers
*               [in/out] Param: Pointer to the configuration item to be read
*                               or changed
*               [in] Index:     Index parameter is used to reference the
*                               specific trigger to change. Range is validated
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
USER_HANDLER_RESULT Trigger_UserCfg(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr)
{
   USER_HANDLER_RESULT result;

   result = USER_RESULT_OK;

   //Load trigger structure into the temporary location based on index param
   //Param.Ptr points to the struct member to be read/written
   //in the temporary location
   memcpy(&ConfigTriggerTemp,
      &CfgMgr_ConfigPtr()->TriggerConfigs[Index],
      sizeof(ConfigTriggerTemp));

   result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
   if(SetPtr != NULL && USER_RESULT_OK == result)
   {
     memcpy(&CfgMgr_ConfigPtr()->TriggerConfigs[Index],
            &ConfigTriggerTemp,
            sizeof(TRIGGER_CONFIG));

     //Store the modified temporary structure in the EEPROM.       
     CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
                            &CfgMgr_ConfigPtr()->TriggerConfigs[Index],
                            sizeof(ConfigTriggerTemp));
   }
   return result;  
}

/******************************************************************************
* Function:     Trigger_State
*
* Description:  Handles User Manager requests to retrieve the current trigger
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

USER_HANDLER_RESULT Trigger_State(USER_DATA_TYPE DataType,
                                  USER_MSG_PARAM Param,
                                  UINT32 Index,
                                  const void *SetPtr,
                                  void **GetPtr)
{
   USER_HANDLER_RESULT result;

   result = USER_RESULT_ERROR;

   memcpy(&StateTriggerTemp, &TriggerData[Index], sizeof(StateTriggerTemp));

   result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

   return result;
}
/******************************************************************************
* Function:    Trigger_ShowConfig
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
USER_HANDLER_RESULT Trigger_ShowConfig(USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr)
{
   CHAR  LabelStem[] = "\r\n\r\nTRIGGER.CFG";
   CHAR  Label[USER_MAX_MSG_STR_LEN * 3];   
   INT16 i;

   USER_HANDLER_RESULT result = USER_RESULT_OK;
   USER_MSG_TBL*  pCfgTable;

   //Top-level name is a single indented space
   CHAR BranchName[USER_MAX_MSG_STR_LEN] = " ";   

   pCfgTable = &TriggerCmd[0];  // Get pointer to config entry

   for (i = 0; i < MAX_TRIGGERS && result == USER_RESULT_OK; ++i)
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

/******************************************************************************
 * Function:     Trigger_CfgExprStrCmd | USER COMMAND HANDLER
 *  
 * Description:  Takes or returns a trigger criteria expression string. Converts
 *               to and from the binary form of the string as it is stored
 *               in the configuration memory
 *           
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
 *
 *****************************************************************************/
USER_HANDLER_RESULT Trigger_CfgExprStrCmd(USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_OK;
  INT32 StrToBinResult;  

  //User Mgr uses this obj outside of this function's scope
  static CHAR str[EVAL_MAX_EXPR_STR_LEN];  
  

  // Move the current trigger[x] config data from shadow to the local temp storage.
  // ('Param' is pointing to either the StartExpr or EndExpr struct
  // in the ConfigTriggerTemp; see TriggerCmd Usertable )

  memcpy(&ConfigTriggerTemp,
         &CfgMgr_ConfigPtr()->TriggerConfigs[Index],
         sizeof(ConfigTriggerTemp));
   
  if(SetPtr == NULL)
  {
    // Getter - Convert binary expression in Param to string, return
    EvalExprBinToStr(str, (EVAL_EXPR*) Param.Ptr );
    *GetPtr = str;    
  }
  else
  {
    // Performing a Set - Make copies of the input string and the dest structure.
    // Convert from string to binary form.
    // If successful, transfer the temp dest structure to the real one.

    strncpy_safe(str, sizeof(str), SetPtr, _TRUNCATE);    
     
    StrToBinResult = EvalExprStrToBin(str, (EVAL_EXPR*) Param.Ptr, MAX_TRIG_EXPR_OPRNDS);

    if( StrToBinResult >= 0 )
    {
      // Move the successfully updated binary expression to the ConfigTriggerTemp storage
      // memcpy(&Param.Ptr, &tempExpr, sizeof(EVAL_EXPR) );
      
      memcpy(&CfgMgr_ConfigPtr()->TriggerConfigs[Index],
             &ConfigTriggerTemp,
             sizeof(ConfigTriggerTemp));
      
      //Store the modified structure in the EEPROM.       
      CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
                             &CfgMgr_ConfigPtr()->TriggerConfigs[Index],
                             sizeof(ConfigTriggerTemp) );
    }
    else
    {  
      GSE_DebugStr( NORMAL,FALSE,"EvalExprStrToBin Failed: %s"NEW_LINE,
                    EvalGetMsgFromErrCode(StrToBinResult));     
      result = USER_RESULT_ERROR;
    }
  }

  return result;
}
/*************************************************************************
*  MODIFICATIONS
*    $History: triggerUserTables.c $
 * 
 * *****************  Version 22  *****************
 * User: Contractor V&v Date: 8/08/12    Time: 3:49p
 * Updated in $/software/control processor/code/system
 * SCR #1107 add line break to output EvalExprStrToBin failed
 * 
 * *****************  Version 21  *****************
 * User: John Omalley Date: 12-07-13   Time: 9:03a
 * Updated in $/software/control processor/code/system
 * SCR 1124 - Packed the configuration structures for ARINC429 Mgr,
 * Sensors and Triggers. Suppressed the alignment warnings for those three
 * objects also.
 * 
 * *****************  Version 20  *****************
 * User: Contractor V&v Date: 4/23/12    Time: 8:02p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST2 Evaluator cleanup
 * 
 * *****************  Version 19  *****************
 * User: Contractor V&v Date: 4/11/12    Time: 5:08p
 * Updated in $/software/control processor/code/system
 * FAST2  Trigger uses Evaluator
 * 
 * *****************  Version 18  *****************
 * User: Contractor V&v Date: 3/21/12    Time: 6:49p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST2 Refactoring names
 * 
 * *****************  Version 17  *****************
 * User: Contractor V&v Date: 3/14/12    Time: 4:52p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 Trigger processing
 * 
 * *****************  Version 16  *****************
 * User: Peter Lee    Date: 9/01/10    Time: 6:57p
 * Updated in $/software/control processor/code/system
 * SCR #845 Code Review Updates
 * 
 * *****************  Version 15  *****************
 * User: Jeff Vahue   Date: 7/29/10    Time: 4:06p
 * Updated in $/software/control processor/code/system
 * SCR# 698 - cleanup
 * 
 * *****************  Version 14  *****************
 * User: Contractor3  Date: 7/29/10    Time: 1:31p
 * Updated in $/software/control processor/code/system
 * SCR #698 - Fix for code review findings.
 * 
 * *****************  Version 13  *****************
 * User: Jeff Vahue   Date: 6/18/10    Time: 4:48p
 * Updated in $/software/control processor/code/system
 * SCR# 649 - add NO_COMPARE back in for use in Signal Tests.
 * 
 * *****************  Version 12  *****************
 * User: Contractor3  Date: 6/10/10    Time: 10:44a
 * Updated in $/software/control processor/code/system
 * SCR #642 - Changes based on Code Review
 * 
 * *****************  Version 11  *****************
 * User: Contractor V&v Date: 6/08/10    Time: 5:55p
 * Updated in $/software/control processor/code/system
 * SCR #615 Showcfg/Long msg enhancement
 * 
 * *****************  Version 10  *****************
 * User: Contractor2  Date: 5/03/10    Time: 4:07p
 * Updated in $/software/control processor/code/system
 * SCR 530: Error: Trigger NO_COMPARE infinite logs
 * Removed NO_COMPARE from code.
 * 
 * *****************  Version 9  *****************
 * User: Contractor V&v Date: 3/19/10    Time: 4:32p
 * Updated in $/software/control processor/code/system
 * SCR #248 Parameter Log Change Add no log for showcfg
 * 
 * *****************  Version 8  *****************
 * User: Jeff Vahue   Date: 3/05/10    Time: 12:16p
 * Updated in $/software/control processor/code/system
 * SCR# 413 - Actions cmds are RO
 * 
 * *****************  Version 7  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 4:05p
 * Updated in $/software/control processor/code/system
 * SCR 67 Interrupted SPI Access (Multiple / Nested SPI Access)
 * 
 * *****************  Version 6  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:59p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 5  *****************
 * User: Jeff Vahue   Date: 2/17/10    Time: 1:24p
 * Updated in $/software/control processor/code/system
 * SCR# 452 - code coverage mods
 * 
 * *****************  Version 4  *****************
 * User: Contractor V&v Date: 1/13/10    Time: 4:58p
 * Updated in $/software/control processor/code/system
 * 
 * *****************  Version 3  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:28p
 * Updated in $/software/control processor/code/system
 * SCR 333
 * 
 * *****************  Version 2  *****************
 * User: John Omalley Date: 12/17/09   Time: 5:49p
 * Updated in $/software/control processor/code/system
 * SCR 214
 * Removed Trigger END state because it was not used
 * 
 * *****************  Version 1  *****************
 * User: Contractor V&v Date: 12/10/09   Time: 5:44p
 * Created in $/software/control processor/code/system
 * SCR 106
*
*
**************************************************************************/ 
