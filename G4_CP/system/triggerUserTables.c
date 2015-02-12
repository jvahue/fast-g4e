#define TRIGGER_USERTABLES_BODY
/******************************************************************************
         Copyright (C) 2008-2015 Pratt & Whitney Engine Services, Inc.
            All Rights Reserved. Proprietary and Confidential.
  
  ECCN:          9D991

  File:          triggerUserTables.c

  Description:   User command structures and functions for the trigger processing

  VERSION
  $Revision: 34 $  $Date: 2/05/15 10:30a $
******************************************************************************/
#ifndef TRIGGER_BODY
#error triggerUserTables.c should only be included by trigger.c
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
static TRIGGER_CONFIG m_configTriggerTemp; // Trigger Configuration
                                         // temp storage
static TRIGGER_DATA   m_stateTriggerTemp;
static BOOLEAN        m_triggerValidTemp;

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
//Prototype for the User Manager message handlers, has to go before
//the local variable tables that use the function pointer.
static USER_HANDLER_RESULT Trigger_UserCfg(USER_DATA_TYPE DataType,
                                            USER_MSG_PARAM Param,
                                            UINT32 Index,
                                            const void *SetPtr,
                                            void **GetPtr);

static USER_HANDLER_RESULT Trigger_State(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr);

static USER_HANDLER_RESULT Trigger_ShowConfig(USER_DATA_TYPE DataType,
                                              USER_MSG_PARAM Param,
                                              UINT32 Index,
                                              const void *SetPtr,
                                              void **GetPtr);

static USER_HANDLER_RESULT Trigger_CfgExprStrCmd(USER_DATA_TYPE DataType,
                                                 USER_MSG_PARAM Param,
                                                 UINT32 Index,
                                                 const void *SetPtr,
                                                 void **GetPtr);

static USER_HANDLER_RESULT Trigger_Valid(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr);



/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/

static
USER_ENUM_TBL trigRateType[]    =  { { "1HZ"    , TR_1HZ          },
                                     { "2HZ"    , TR_2HZ          },
                                     { "4HZ"    , TR_4HZ          },
                                     { "5HZ"    , TR_5HZ          },
                                     { "10HZ"   , TR_10HZ         },
                                     { "20HZ"   , TR_20HZ         },
                                     { "50HZ"   , TR_50HZ         },
                                     { NULL     , 0               }
                                   };

static
USER_ENUM_TBL trigStateEnum[]  = { {"NOT_USED",     TRIG_NONE },
                                   {"NOT_ACTIVE",   TRIG_START },
                                   {"ACTIVE",       TRIG_ACTIVE},
                                   {NULL,           0}
                                 };

#pragma ghs nowarning 1545 //Suppress packed structure alignment warning

// Triggers - TRIGGER User and Configuration Table
static
USER_MSG_TBL triggerSensorCfgCmd [] =
{
     /*Str             Next Tbl Ptr   Handler Func.    Data Type        Access     Parameter                                        IndexRange           DataLimit EnumTbl*/
    { "INDEXA"       , NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_ENUM,  USER_RW,   &m_configTriggerTemp.trigSensor[0].sensorIndex,    0,(MAX_TRIGGERS-1),  NO_LIMIT, SensorIndexType },
    { "STARTVALUEA"  , NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_FLOAT, USER_RW,   &m_configTriggerTemp.trigSensor[0].start.fValue,   0,(MAX_TRIGGERS-1),  NO_LIMIT, NULL },
    { "STARTCOMPAREA", NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_ENUM,  USER_RW,   &m_configTriggerTemp.trigSensor[0].start.compare,  0,(MAX_TRIGGERS-1),  NO_LIMIT, comparisonEnum},
    { "ENDVALUEA"    , NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_FLOAT, USER_RW,   &m_configTriggerTemp.trigSensor[0].end.fValue,     0,(MAX_TRIGGERS-1),  NO_LIMIT, NULL },
    { "ENDCOMPAREA"  , NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_ENUM,  USER_RW,   &m_configTriggerTemp.trigSensor[0].end.compare,    0,(MAX_TRIGGERS-1),  NO_LIMIT, comparisonEnum},
    { "INDEXB"       , NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_ENUM,  USER_RW,   &m_configTriggerTemp.trigSensor[1].sensorIndex,    0,(MAX_TRIGGERS-1),  NO_LIMIT, SensorIndexType },
    { "STARTVALUEB"  , NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_FLOAT, USER_RW,   &m_configTriggerTemp.trigSensor[1].start.fValue,   0,(MAX_TRIGGERS-1),  NO_LIMIT, NULL },
    { "STARTCOMPAREB", NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_ENUM,  USER_RW,   &m_configTriggerTemp.trigSensor[1].start.compare,  0,(MAX_TRIGGERS-1),  NO_LIMIT, comparisonEnum},
    { "ENDVALUEB"    , NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_FLOAT, USER_RW,   &m_configTriggerTemp.trigSensor[1].end.fValue,     0,(MAX_TRIGGERS-1),  NO_LIMIT, NULL },
    { "ENDCOMPAREB"  , NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_ENUM,  USER_RW,   &m_configTriggerTemp.trigSensor[1].end.compare,    0,(MAX_TRIGGERS-1),  NO_LIMIT, comparisonEnum},
    { "INDEXC"       , NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_ENUM,  USER_RW,   &m_configTriggerTemp.trigSensor[2].sensorIndex,    0,(MAX_TRIGGERS-1),  NO_LIMIT, SensorIndexType},
    { "STARTVALUEC"  , NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_FLOAT, USER_RW,   &m_configTriggerTemp.trigSensor[2].start.fValue,   0,(MAX_TRIGGERS-1),  NO_LIMIT, NULL },
    { "STARTCOMPAREC", NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_ENUM,  USER_RW,   &m_configTriggerTemp.trigSensor[2].start.compare,  0,(MAX_TRIGGERS-1),  NO_LIMIT, comparisonEnum},
    { "ENDVALUEC"    , NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_FLOAT, USER_RW,   &m_configTriggerTemp.trigSensor[2].end.fValue,     0,(MAX_TRIGGERS-1),  NO_LIMIT, NULL },
    { "ENDCOMPAREC"  , NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_ENUM,  USER_RW,   &m_configTriggerTemp.trigSensor[2].end.compare,    0,(MAX_TRIGGERS-1),  NO_LIMIT, comparisonEnum},
    { "INDEXD"       , NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_ENUM,  USER_RW,   &m_configTriggerTemp.trigSensor[3].sensorIndex,    0,(MAX_TRIGGERS-1),  NO_LIMIT, SensorIndexType },
    { "STARTVALUED"  , NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_FLOAT, USER_RW,   &m_configTriggerTemp.trigSensor[3].start.fValue,   0,(MAX_TRIGGERS-1),  NO_LIMIT, NULL },
    { "STARTCOMPARED", NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_ENUM,  USER_RW,   &m_configTriggerTemp.trigSensor[3].start.compare,  0,(MAX_TRIGGERS-1),  NO_LIMIT, comparisonEnum},
    { "ENDVALUED"    , NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_FLOAT, USER_RW,   &m_configTriggerTemp.trigSensor[3].end.fValue,     0,(MAX_TRIGGERS-1),  NO_LIMIT, NULL },
    { "ENDCOMPARED"  , NO_NEXT_TABLE, Trigger_UserCfg, USER_TYPE_ENUM,  USER_RW,   &m_configTriggerTemp.trigSensor[3].end.compare,    0,(MAX_TRIGGERS-1),  NO_LIMIT, comparisonEnum},
    { NULL,            NULL,          NULL,            NO_HANDLER_DATA }
};

// Trigger user and configuration Table
static USER_MSG_TBL triggerCmd [] =
{
  /* Str              Next Tbl Ptr               Handler Func.             Data Type          Access     Parameter                           IndexRange           DataLimit            EnumTbl*/
  { "NAME",           NO_NEXT_TABLE,            Trigger_UserCfg,           USER_TYPE_STR,     USER_RW,   m_configTriggerTemp.triggerName,      0,(MAX_TRIGGERS-1),  0,MAX_TRIGGER_NAME,  NULL },
  { "SENSOR",         &triggerSensorCfgCmd[0],  NULL,                      NO_HANDLER_DATA },
  { "DURATION_MS",    NO_NEXT_TABLE,            Trigger_UserCfg,           USER_TYPE_UINT32,  USER_RW,   &m_configTriggerTemp.nMinDuration_ms, 0,(MAX_TRIGGERS-1),  NO_LIMIT,            NULL },
  { "RATE",           NO_NEXT_TABLE,            Trigger_UserCfg,           USER_TYPE_ENUM,    USER_RW,   &m_configTriggerTemp.rate,            0,(MAX_TRIGGERS-1),  NO_LIMIT,            trigRateType },
  { "RATEOFFSET_MS",  NO_NEXT_TABLE,            Trigger_UserCfg,           USER_TYPE_UINT32,  USER_RW,   &m_configTriggerTemp.nOffset_ms,      0,(MAX_TRIGGERS-1),  0,1000,              NULL },
  { "SC",             NO_NEXT_TABLE,            Trigger_CfgExprStrCmd,     USER_TYPE_STR,     USER_RW,   &m_configTriggerTemp.startExpr,       0,(MAX_TRIGGERS-1),  NO_LIMIT,            NULL },
  { "EC",             NO_NEXT_TABLE,            Trigger_CfgExprStrCmd,     USER_TYPE_STR,     USER_RW,   &m_configTriggerTemp.endExpr,         0,(MAX_TRIGGERS-1),  NO_LIMIT,            NULL },
  { NULL,             NULL,                     NULL,                      NO_HANDLER_DATA }
};


static USER_MSG_TBL triggerStatus [] =
{
  /* Str            Next Tbl Ptr       Handler Func.    Data Type        Access     Parameter                           IndexRange           DataLimit   EnumTbl*/
  { "STATE"       , NO_NEXT_TABLE,     Trigger_State,  USER_TYPE_ENUM,   USER_RO    , &m_stateTriggerTemp.state,          0, MAX_TRIGGERS - 1, NO_LIMIT,   trigStateEnum },
  { "VALID"       , NO_NEXT_TABLE,     Trigger_Valid,  USER_TYPE_BOOLEAN,USER_RO    , &m_triggerValidTemp,                0, MAX_TRIGGERS - 1, NO_LIMIT,   NULL },
  { "STARTTIME_MS", NO_NEXT_TABLE,     Trigger_State,  USER_TYPE_UINT32, USER_RO    , &m_stateTriggerTemp.nStartTime_ms,  0, MAX_TRIGGERS - 1, NO_LIMIT,   NULL },
  { "DURATION_MS" , NO_NEXT_TABLE,     Trigger_State,  USER_TYPE_UINT32, USER_RO    , &m_stateTriggerTemp.nDuration_ms,   0, MAX_TRIGGERS - 1, NO_LIMIT,   NULL },
  { NULL          , NO_NEXT_TABLE,     Trigger_State,  USER_TYPE_NONE,   USER_RW    , NULL,                             0, MAX_TRIGGERS - 1, NO_LIMIT,   NULL },
};

static USER_MSG_TBL triggerRoot [] =
{ /* Str            Next Tbl Ptr       Handler Func.        Data Type           Access            Parameter        IndexRange   DataLimit          EnumTbl*/
   { "CFG",         triggerCmd   ,     NULL,                NO_HANDLER_DATA},
   { "STATUS",      triggerStatus,     NULL,                NO_HANDLER_DATA},
   { "FLAGS",       NO_NEXT_TABLE,     Trigger_State,       USER_TYPE_128_LIST, USER_RO,          m_triggerFlags,  -1, -1,      0,MAX_TRIGGERS-1,  NULL },
   { DISPLAY_CFG,   NO_NEXT_TABLE,     Trigger_ShowConfig,  USER_TYPE_ACTION,   USER_RO|USER_GSE|USER_NO_LOG, NULL,            -1, -1,      NO_LIMIT,          NULL},
   { NULL,          NULL,              NULL,                NO_HANDLER_DATA}
};

static
USER_MSG_TBL    rootTriggerMsg = {"TRIGGER",triggerRoot,NULL,NO_HANDLER_DATA};

#pragma ghs endnowarning

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

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
static USER_HANDLER_RESULT Trigger_UserCfg(USER_DATA_TYPE DataType,
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
   memcpy(&m_configTriggerTemp,
      &CfgMgr_ConfigPtr()->TriggerConfigs[Index],
      sizeof(m_configTriggerTemp));

   result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
   if(SetPtr != NULL && USER_RESULT_OK == result)
   {
     memcpy(&CfgMgr_ConfigPtr()->TriggerConfigs[Index],
            &m_configTriggerTemp,
            sizeof(TRIGGER_CONFIG));

     //Store the modified temporary structure in the EEPROM.
     CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
                            &CfgMgr_ConfigPtr()->TriggerConfigs[Index],
                            sizeof(m_configTriggerTemp));
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

static USER_HANDLER_RESULT Trigger_State(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr)
{
   USER_HANDLER_RESULT result;

   result = USER_RESULT_ERROR;

   memcpy(&m_stateTriggerTemp, &m_triggerData[Index], sizeof(m_stateTriggerTemp));

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
static USER_HANDLER_RESULT Trigger_ShowConfig(USER_DATA_TYPE DataType,
                                              USER_MSG_PARAM Param,
                                              UINT32 Index,
                                              const void *SetPtr,
                                              void **GetPtr)
{
   CHAR  labelStem[] = "\r\n\r\nTRIGGER.CFG";
   CHAR  label[USER_MAX_MSG_STR_LEN * 3];
   INT16 i;

   USER_HANDLER_RESULT result = USER_RESULT_OK;
   USER_MSG_TBL*  pCfgTable;

   //Top-level name is a single indented space
   CHAR branchName[USER_MAX_MSG_STR_LEN] = " ";

   pCfgTable = &triggerCmd[0];  // Get pointer to config entry

   for (i = 0; i < MAX_TRIGGERS && result == USER_RESULT_OK; ++i)
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
static USER_HANDLER_RESULT Trigger_CfgExprStrCmd(USER_DATA_TYPE DataType,
                                                 USER_MSG_PARAM Param,
                                                 UINT32 Index,
                                                 const void *SetPtr,
                                                 void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_OK;
  INT32 strToBinResult;

  //User Mgr uses this obj outside of this function's scope
  static CHAR str[EVAL_MAX_EXPR_STR_LEN];


  // Move the current trigger[x] config data from shadow to the local temp storage.
  // ('Param' is pointing to either the startExpr or endExpr struct
  // in the m_configTriggerTemp; see TriggerCmd Usertable )

  memcpy(&m_configTriggerTemp,
         &CfgMgr_ConfigPtr()->TriggerConfigs[Index],
         sizeof(m_configTriggerTemp));

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

    strToBinResult = EvalExprStrToBin(EVAL_CALLER_TYPE_PARSE, (INT32)Index,
                                      str, (EVAL_EXPR*) Param.Ptr, MAX_TRIG_EXPR_OPRNDS);

    if( strToBinResult >= 0 )
    {
      // Move the successfully updated binary expression to the m_configTriggerTemp storage
      // memcpy(&Param.Ptr, &tempExpr, sizeof(EVAL_EXPR) );

      memcpy(&CfgMgr_ConfigPtr()->TriggerConfigs[Index],
             &m_configTriggerTemp,
             sizeof(m_configTriggerTemp));

      //Store the modified structure in the EEPROM.
      CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
                             &CfgMgr_ConfigPtr()->TriggerConfigs[Index],
                             sizeof(m_configTriggerTemp) );
    }
    else
    {
      GSE_DebugStr( NORMAL,FALSE,"EvalExprStrToBin Failed: %s"NEW_LINE,
                    EvalGetMsgFromErrCode(strToBinResult));
      result = USER_RESULT_ERROR;
    }
  }

  return result;
}
/******************************************************************************
 * Function:     Trigger_Valid | USER COMMAND HANDLER
 *
 * Description:  Returns the validity for the trigger passed in "Index'
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
static USER_HANDLER_RESULT Trigger_Valid(USER_DATA_TYPE DataType,
                                         USER_MSG_PARAM Param,
                                         UINT32 Index,
                                         const void *SetPtr,
                                         void **GetPtr)

{
   // Use the supplied Accessor to return the validity of the indicated trigger.
   m_triggerValidTemp = TriggerValidGetState((TRIGGER_INDEX)Index);
   *GetPtr = Param.Ptr;
   return USER_RESULT_OK;
}

/*************************************************************************
*  MODIFICATIONS
*    $History: triggerUserTables.c $
 * 
 * *****************  Version 34  *****************
 * User: John Omalley Date: 2/05/15    Time: 10:30a
 * Updated in $/software/control processor/code/system
 * SCR 1267 - Code Review Updates.
 * 
 * *****************  Version 33  *****************
 * User: John Omalley Date: 12/05/14   Time: 4:26p
 * Updated in $/software/control processor/code/system
 * SCR 1267 - Fixed showcfg Access flags
 * 
 * *****************  Version 32  *****************
 * User: Contractor V&v Date: 12/05/12   Time: 5:03p
 * Updated in $/software/control processor/code/system
 * SCR #1107 Code Review  Removed tabs
 *
 * *****************  Version 31  *****************
 * User: Contractor V&v Date: 12/05/12   Time: 4:17p
 * Updated in $/software/control processor/code/system
 * SCR #1107 Code Review
 *
 * *****************  Version 30  *****************
 * User: Contractor V&v Date: 11/26/12   Time: 12:44p
 * Updated in $/software/control processor/code/system
 * SCR #1167 SensorSummary
 *
 * *****************  Version 29  *****************
 * User: Jeff Vahue   Date: 11/10/12   Time: 4:48p
 * Updated in $/software/control processor/code/system
 * SCR# 1107 - nmistype on Trigger32 enum mapping.
 *
 * *****************  Version 28  *****************
 * User: Contractor V&v Date: 11/08/12   Time: 4:28p
 * Updated in $/software/control processor/code/system
 * Code Review
 *
 * *****************  Version 27  *****************
 * User: John Omalley Date: 12-10-23   Time: 2:56p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review Updates
 *
 * *****************  Version 26  *****************
 * User: Jeff Vahue   Date: 8/29/12    Time: 6:20p
 * Updated in $/software/control processor/code/system
 * SCR# 1107 - !P Table Full processing
 *
 * *****************  Version 25  *****************
 * User: Contractor V&v Date: 8/29/12    Time: 3:09p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 !P Processing
 *
 * *****************  Version 24  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 23  *****************
 * User: Contractor V&v Date: 8/15/12    Time: 7:23p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 BITARRAY128 input as integer list
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
****************************************************************************/
