#define FASTSTATEMGR_USERTABLES_BODY
/******************************************************************************
            Copyright (C) 2007-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:        FASTStateMgrUserTables.c
    
    Description: Tables and functions for FastMgr User Commands  

   VERSION
   $Revision: 8 $  $Date: 8/28/12 2:36p $
    
******************************************************************************/
#ifndef FASTSTATEMGR_BODY
#error FastStateMgrUserTables.c should only be included by FASTStateMgr.c
#endif

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/

#include "utility.h"
/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define MAX_LABEL_CHAR_ARRAY 128
#define MAX_VALUE_CHAR_ARRAY 32

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
USER_ENUM_TBL AnEnum[] =
{
  {NULL,0}
};

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/

USER_HANDLER_RESULT FSM_CfgTCStrCmd(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr);
USER_HANDLER_RESULT FSM_CfgTaskStrCmd(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr);
USER_HANDLER_RESULT FSM_CfgStateCmd(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr);
USER_HANDLER_RESULT FSM_CfgTCCmd(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr);
USER_HANDLER_RESULT FSM_GetActvTimeCmd(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr);
USER_HANDLER_RESULT FSM_CfgOther(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr);
USER_HANDLER_RESULT FSM_GetRunningTasks(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr);
USER_HANDLER_RESULT FSM_GetCRCofCfg(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr);
USER_HANDLER_RESULT FSM_GetStateStr(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr);
USER_HANDLER_RESULT FSM_ShowConfig(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr);

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static FSM_STATE m_StateTmp;
static FSM_TC    m_TCTmp;
static BOOLEAN   m_CfgEnbTmp;
static UINT16    m_CfgCRCTmp;


#pragma ghs nowarning 1545 //Suppress packed structure alignment warning

static USER_MSG_TBL CfgTCCmd [] =
{  /*Str             Next Tbl Ptr   Handler Func.       Data Type         Access      Parameter               IndexRange             DataLimit          EnumTbl*/
  { "THIS",          NO_NEXT_TABLE, FSM_CfgTCCmd,       USER_TYPE_UINT8,  USER_RW,    &m_TCTmp.CurState,      0,(FSM_NUM_OF_TC-1),   0,(FSM_NUM_OF_STATES-1), NULL },
  { "NEXT",          NO_NEXT_TABLE, FSM_CfgTCCmd,       USER_TYPE_UINT8,  USER_RW,    &m_TCTmp.NextState,     0,(FSM_NUM_OF_TC-1),   0,(FSM_NUM_OF_STATES-1), NULL },
  { "TC",            NO_NEXT_TABLE, FSM_CfgTCStrCmd,    USER_TYPE_STR,    USER_RW,    NULL,                   0,(FSM_NUM_OF_TC-1),   0,FSM_TC_STR_LEN,        NULL },            
  { "IS_LOGGED",     NO_NEXT_TABLE, FSM_CfgTCCmd,       USER_TYPE_YESNO,  USER_RW,    &m_TCTmp.IsLogged,      0,(FSM_NUM_OF_TC-1),   NO_LIMIT               , NULL }, 
  { NULL,            NULL,          NULL,               NO_HANDLER_DATA } 
};
static USER_MSG_TBL CfgStateCmd [] =
{ /*Str               Next Tbl Ptr   Handler Func.      Data Type         Access      Parameter               IndexRange                DataLimit          EnumTbl*/
  { "NAME",           NO_NEXT_TABLE, FSM_CfgStateCmd,   USER_TYPE_STR,    USER_RW,    &m_StateTmp.Name,       0,(FSM_NUM_OF_STATES-1),  0,FSM_STATE_NAME_STR_LEN, NULL },
  { "RUN",            NO_NEXT_TABLE, FSM_CfgTaskStrCmd, USER_TYPE_STR,    USER_RW,    NULL,                   0,(FSM_NUM_OF_STATES-1),  0,FSM_TASK_LIST_STR_LEN , NULL },
  { "TIMER_S",        NO_NEXT_TABLE, FSM_CfgStateCmd,   USER_TYPE_UINT32, USER_RW,    &m_StateTmp.Timer,      0,(FSM_NUM_OF_STATES-1),  NO_LIMIT,          NULL },
  { NULL,             NULL,          NULL,              NO_HANDLER_DATA } 
};

// IMPORTANT NOTE: Must keep CfgCmd table organized such that scalar variables are listed
//                 first, followed by pointers to array variables.
//                 This makes the results displayed by the FSM_ShowCfg function easier to read.
//                 FSM_ShowCfg IS DEPENDING ON THIS!!!!! 
static USER_MSG_TBL CfgCmd [] =
  {  /*Str            Next Tbl Ptr    Handler Func.         Data Type          Access      Parameter          IndexRange            DataLimit          EnumTbl*/
  { "ENABLED",        NO_NEXT_TABLE,  FSM_CfgOther,         USER_TYPE_YESNO,   USER_RW,    &m_CfgEnbTmp,      -1,-1,                NO_LIMIT,          NULL }, 
  { "CRC16",          NO_NEXT_TABLE,  FSM_CfgOther,         USER_TYPE_HEX16,   USER_RW,    &m_CfgCRCTmp,      -1,-1,                NO_LIMIT,          NULL },
  { "STATES",         CfgStateCmd,    NULL,                 NO_HANDLER_DATA},
  { "TCS",            CfgTCCmd,       NULL,                 NO_HANDLER_DATA},   
  { NULL,             NULL,           NULL,                 NO_HANDLER_DATA}, 
};

static USER_MSG_TBL StatusCmd [] =
  {  /*Str                Next Tbl Ptr   Handler Func.          Data Type          Access      Parameter      IndexRange            DataLimit          EnumTbl*/
  { "STATE",              NO_NEXT_TABLE, FSM_GetStateStr,       USER_TYPE_STR,     USER_RO,    NULL,          -1,-1,                NO_LIMIT,         NULL },
  { "STATE_IDX",          NO_NEXT_TABLE, User_GenericAccessor,  USER_TYPE_INT32,   USER_RO,    &m_CurState,   -1,-1,                NO_LIMIT,         NULL },
  { "STATE_ACTV_TIME_S",  NO_NEXT_TABLE, FSM_GetActvTimeCmd,    USER_TYPE_UINT32,  USER_RO,    NULL,          -1,-1,                NO_LIMIT,         NULL },
  { "RUNNING_TASKS",      NO_NEXT_TABLE, FSM_GetRunningTasks,   USER_TYPE_STR,     USER_RO,    NULL,          -1,-1,                NO_LIMIT,         NULL },
  { "CALC_CFG_CRC",       NO_NEXT_TABLE, FSM_GetCRCofCfg,       USER_TYPE_HEX16,   USER_RO,    NULL,          -1,-1,                NO_LIMIT,         NULL },
/*{ "TC_EVAL"             NO_NEXT_TABLE, User_GenericAccessor,  USER_TYPE_BOOLEAN, USER_RO,    NULL,           0,(FSM_NUM_OF_TC-1), NO_LIMIT,          NULL },*/
  { NULL,                 NULL,          NULL,                  NO_HANDLER_DATA } 
};

static USER_MSG_TBL FSMRoot[] =
{  /*Str           Next Tbl Ptr     Handler Func.         Data Type          Access        Parameter      IndexRange            DataLimit         EnumTbl*/
  { "CFG",         CfgCmd,          NULL,                 NO_HANDLER_DATA},
  { "STATUS",      StatusCmd,       NULL,                 NO_HANDLER_DATA},
  { DISPLAY_CFG,   NO_NEXT_TABLE,   FSM_ShowConfig,       USER_TYPE_ACTION, USER_RO|USER_NO_LOG|USER_GSE,     NULL,       -1,-1,    NO_LIMIT, NULL},
  { NULL,          NULL,            NULL,                 NO_HANDLER_DATA}
};

#pragma ghs endnowarning 


//****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:     FSM_CfgTCStrCmd | USER COMMAND HANDLER
 *  
 * Description:  Takes or returns a transition critera string.  Converts to
 *               and from the binary form of the string as it is stored
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
 * Returns:     USER_RESULT_OK:    Processed successfully
 *              USER_RESULT_ERROR: Error processing command.       
 *
 * Notes:
 *
 *****************************************************************************/
USER_HANDLER_RESULT FSM_CfgTCStrCmd(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_OK;
  INT32 StrToBinResult;

   FSM_TC tc_bin;
   //User Mgr uses this obj outside of this function's scope
   static CHAR str[FSM_TC_STR_LEN];  
   
   if(SetPtr == NULL)
   {
     //Convert binary TC to string, return
     FSM_TCBinToStr(str, &CfgMgr_ConfigPtr()->FSMConfig.TCs[Index]);
     *GetPtr = str;
   }
   else
   {
     //Convert string TC to binary and store
     strncpy_safe(str, sizeof(str), SetPtr,_TRUNCATE);

     memcpy(&tc_bin,&CfgMgr_ConfigPtr()->FSMConfig.TCs[Index],
         sizeof(tc_bin));
     
     StrToBinResult = FSM_TCStrToBin(str,&tc_bin);
     if(StrToBinResult >= 0)
     {
       memcpy(&CfgMgr_ConfigPtr()->FSMConfig.TCs[Index],&tc_bin,
           sizeof(CfgMgr_ConfigPtr()->FSMConfig.TCs[Index]));
       
       CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
        &CfgMgr_ConfigPtr()->FSMConfig.TCs[Index],
       sizeof(CfgMgr_ConfigPtr()->FSMConfig.TCs[Index]));
     }
     else
     {
       GSE_DebugStr(NORMAL,TRUE,"FCS: TCStrToBin returned %d",StrToBinResult);
       result = USER_RESULT_ERROR;
     }
   }

  return result;  
}



/******************************************************************************
 * Function:     FSM_CfgTaskStrCmd | USER COMMAND HANDLER
 *  
 * Description:  Takes or returns a RunTask string.  Converts to
 *               and from the binary form of the string as it is stored
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
 * Returns:     USER_RESULT_OK:    Processed successfully
 *              USER_RESULT_ERROR: Error processing command.       
 *
 * Notes:
 *
 *****************************************************************************/
USER_HANDLER_RESULT FSM_CfgTaskStrCmd(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_OK;
  INT32 StrToBinResult;

  FSM_STATE state_bin;
  static CHAR str[FSM_TASK_LIST_STR_LEN];
     
  if(SetPtr == NULL)
  {
    //Convert binary TC to string, return
    FSM_TaskBinToStr(str, &CfgMgr_ConfigPtr()->FSMConfig.States[Index] );
    *GetPtr = str;
  }
  else
  {
    //Convert string TC to binary and store
    strncpy_safe(str, sizeof(str), SetPtr,_TRUNCATE);
    state_bin = CfgMgr_ConfigPtr()->FSMConfig.States[Index];
    
    StrToBinResult = FSM_TaskStrToBin(str,&state_bin);
    if(StrToBinResult >= 0)
    {
       memcpy(&CfgMgr_ConfigPtr()->FSMConfig.States[Index],&state_bin,
           sizeof(CfgMgr_ConfigPtr()->FSMConfig.States[Index]));
       
      CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
       &CfgMgr_ConfigPtr()->FSMConfig.States[Index],
      sizeof(CfgMgr_ConfigPtr()->FSMConfig.States[Index]));
    }
    else
    {
      GSE_DebugStr(NORMAL,TRUE,"FCS: TaskStrToBin returned %d",StrToBinResult);
      result = USER_RESULT_ERROR;
    }
  }

  return result;  
}



/******************************************************************************
 * Function:     FSM_CfgStateCmd | USER COMMAND HANDLER
 *  
 * Description:  Configuration of any TC config item beside the TC string
 *               it self.  The TC string is configured separately by CfgTCStr
 *               because it must be converted and validated first, whereas the
 *               parameters handled by this func need to special conversion.
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
 *
 *****************************************************************************/
USER_HANDLER_RESULT FSM_CfgStateCmd(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_OK;

  
  memcpy(&m_StateTmp,&CfgMgr_ConfigPtr()->FSMConfig.States[Index],
      sizeof(m_StateTmp));

  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
  
  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    memcpy(&CfgMgr_ConfigPtr()->FSMConfig.States[Index],&m_StateTmp,
      sizeof(CfgMgr_ConfigPtr()->FSMConfig.States[Index]));
    
    //Store the modified temporary structure in the EEPROM.       
    CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
        &CfgMgr_ConfigPtr()->FSMConfig.States[Index],
        sizeof(m_StateTmp));
  }
  
  return result;
}



/******************************************************************************
 * Function:     FSM_CfgTCCmd | USER COMMAND HANDLER
 *  
 * Description:  Configuration of any TC config item beside the TC string
 *               it self.  The TC string is configured separately by CfgTCStr
 *               because it must be converted and validated first, whereas the
 *               parameters handled by this func need to special conversion.
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
 *
 *****************************************************************************/
USER_HANDLER_RESULT FSM_CfgTCCmd(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_OK;

  
  memcpy(&m_TCTmp,&CfgMgr_ConfigPtr()->FSMConfig.TCs[Index],
      sizeof(m_TCTmp));

  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
  
  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    memcpy(&CfgMgr_ConfigPtr()->FSMConfig.TCs[Index],&m_TCTmp,
      sizeof(CfgMgr_ConfigPtr()->FSMConfig.TCs[Index]));
    
    //Store the modified temporary structure in the EEPROM.       
    CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
        &CfgMgr_ConfigPtr()->FSMConfig.TCs[Index],
        sizeof(m_TCTmp));
  }

  return result;
}



/******************************************************************************
 * Function:     FSM_GetActvTimeCmd | USER COMMAND HANDLER
 *  
 * Description:  Converts the milliseconds the current state has been active
 *               into seconds and returns the value the User Manager.
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
 *
 *****************************************************************************/
USER_HANDLER_RESULT FSM_GetActvTimeCmd(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr)
{
  **(INT32**)GetPtr = ((CM_GetTickCount() - m_StateStartTime) /
                            MILLISECONDS_PER_SECOND);

  return USER_RESULT_OK;
}



/******************************************************************************
 * Function:     FSM_CfgOther | USER COMMAND HANDLER
 *  
 * Description:  Catch all for "Other" configuration items not covered above
 *               including the CRC and the enable flag
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
 *
 *****************************************************************************/
USER_HANDLER_RESULT FSM_CfgOther(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_OK;

  //Preset temp variables to current config
  m_CfgEnbTmp = CfgMgr_ConfigPtr()->FSMConfig.Enabled;
  m_CfgCRCTmp = CfgMgr_ConfigPtr()->FSMConfig.CRC;
  
  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

  CfgMgr_ConfigPtr()->FSMConfig.CRC = m_CfgCRCTmp;
  CfgMgr_ConfigPtr()->FSMConfig.Enabled = m_CfgEnbTmp;

  if(SetPtr != NULL)
  {
    //Store the modified temporary structure in the EEPROM.       
#pragma ghs nowarning 1545 //Suppress packed structure alignment warning
    CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
          &CfgMgr_ConfigPtr()->FSMConfig.CRC,
          sizeof(CfgMgr_ConfigPtr()->FSMConfig.CRC));
#pragma ghs endnowarning 
     
    CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
          &CfgMgr_ConfigPtr()->FSMConfig.Enabled,
          sizeof(CfgMgr_ConfigPtr()->FSMConfig.Enabled));
  }
  return result;
}



/******************************************************************************
 * Function:     FSM_GetRunningTasks | USER COMMAND HANDLER
 *  
 * Description:  Handles the command to get the list of tasks currently running
 *               
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
 * Returns:     USER_RESULT_OK:    Processed successfully
 *              USER_RESULT_ERROR: Error processing command.       
 *
 * Notes:
 *
 *****************************************************************************/
//            [     Number of tasks defined     ] * [task str len     ] +[NULL]
#define TOTAL_TASKS_STR_LEN\
           ((sizeof(m_Tasks)/sizeof(m_Tasks[0]))*(FSM_TASK_STR_LEN+1)) + 1

USER_HANDLER_RESULT FSM_GetRunningTasks(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr)
{
  INT32 i,j;
  static CHAR str[TOTAL_TASKS_STR_LEN];
  CHAR num_str[FSM_TASK_STR_LEN+1];                       
  str[0] = '\0';
  
  for(i = 0;m_Tasks[i].Name[0] != '\0';i++)
  {
    for(j = 0; j <= m_Tasks[i].MaxNum; j++)
    {      
      if(m_Tasks[i].GetState(j))
      {
        //For snprintf and Strcat return values below, the return is ignored
        //because the error state causes string truncation.  No further action
        //beyond the truncation is needed in the event of a string overflow.
        SuperStrcat(str,m_Tasks[i].Name,sizeof(str));
        if(m_Tasks[i].IsNumerated)
        {
          num_str[FSM_TASK_STR_LEN] = '\0';
          snprintf(num_str,sizeof(num_str),"%03d",j);
          SuperStrcat(str,num_str,sizeof(str));
        }
        SuperStrcat(str," ",sizeof(str));
      }
    }
  }

  *GetPtr = str;

  return USER_RESULT_OK;
}



/******************************************************************************
 * Function:     FSM_GetCRCofCfg | USER COMMAND HANDLER
 *  
 * Description:  Compute the CRC of the FSM configuration area in EEPROM and
 *               return it to the user manager.  This is the EEPROM copy being
 *               returned and not the run-time copy in RAM.
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
 * Returns:     USER_RESULT_OK:    Processed successfully
 *              USER_RESULT_ERROR: Error processing command.       
 *
 * Notes:
 *
 *****************************************************************************/
USER_HANDLER_RESULT FSM_GetCRCofCfg(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr)
{
  UINT16 CRC;

  //Compute CRC
  CRC = CRC16(&CfgMgr_ConfigPtr()->FSMConfig,
      sizeof(CfgMgr_ConfigPtr()->FSMConfig)-
      sizeof(CfgMgr_ConfigPtr()->FSMConfig.CRC));

  **(UINT16**)GetPtr = CRC;
  
  return USER_RESULT_OK;
}

/******************************************************************************
 * Function:     FSM_GetStateStr | USER COMMAND HANDLER
 *  
 * Description:  Return the string name of the state currently running.  The
 *               name is taken from the name this state was given in the
 *               configuration.
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
 * Returns:     USER_RESULT_OK:    Processed successfully
 *              USER_RESULT_ERROR: Error processing command.       
 *
 * Notes:
 *
 *****************************************************************************/
USER_HANDLER_RESULT FSM_GetStateStr(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr)
{
  *(CHAR**)GetPtr = m_CurStateStr;
  
  return USER_RESULT_OK;
}

/******************************************************************************
* Function:    FSM_ShowConfig | USER COMMAND HANDLER
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
USER_HANDLER_RESULT FSM_ShowConfig(USER_DATA_TYPE DataType,
                                      USER_MSG_PARAM Param,
                                      UINT32 Index,
                                      const void *SetPtr,
                                      void **GetPtr)
{
  INT16 idx;
  CHAR  LabelStem[MAX_LABEL_CHAR_ARRAY] = "\r\n\r\nFSM.CFG";
  CHAR  Label[USER_MAX_MSG_STR_LEN * 3];
  CHAR  Value[MAX_VALUE_CHAR_ARRAY];
     
  USER_HANDLER_RESULT result; 
  
  // Declare a GetPtr to be used locally since
  // the GetPtr passed in is "NULL" because this funct is a user-action.
  UINT32 TempInt;
  void*  LocalPtr = &TempInt;
  
  //Top-level name is a single indented space
  CHAR BranchName[USER_MAX_MSG_STR_LEN] = " ";
  CHAR NextBranchName[USER_MAX_MSG_STR_LEN] = "  ";

  USER_MSG_TBL* pCfgTable;
  result = USER_RESULT_OK;

  /*
  FSM.CFG is of the form:
  Level 1 FSM
    Level 2   CFG
    Level 3     ENABLED
    Level 3     CRC16
    Level 3     STATES[0]
      Level 4       NAME
      Level 4       ASYNC_STOP
      Level 4       TIMER_S
   {Repeat for STATES[1] - STATES[15]}
    Level 3     TCS[0]
      Level 4       THIS
      Level 4       NEXT
      Level 4       TC
      Level 4       IS_LOGGED
    {Repeat for TCS[1] - TCS[47]}
  */
  
  // Display label for FSM.CFG
  User_OutputMsgString( LabelStem, FALSE);
  
  /*** Process the leaf (scalar) elements in Levels 1->3  ***/ 
  pCfgTable = CfgCmd;
  
  idx = 0;  
  while ( pCfgTable->MsgStr  != NULL && pCfgTable->pNext   == NO_NEXT_TABLE   &&
          pCfgTable->MsgType != USER_TYPE_ACTION && result == USER_RESULT_OK )
  {    
    sprintf(  Label,"\r\n%s%s =", BranchName, pCfgTable->MsgStr);
    PadString(Label, USER_MAX_MSG_STR_LEN + 15);
    result = USER_RESULT_ERROR;
    if (User_OutputMsgString( Label, FALSE ))
    {
      result = USER_RESULT_OK; 

      // Call user function for the table entry
      if( USER_RESULT_OK != pCfgTable->MsgHandler(pCfgTable->MsgType, pCfgTable->MsgParam,
                                                idx, NULL, &LocalPtr))
      {
        User_OutputMsgString( USER_MSG_INVALID_CMD_FUNC, FALSE);
      }
      else
      {
        // Convert the param to string.
        if( User_CvtGetStr(pCfgTable->MsgType, Value, sizeof(Value), 
                                          LocalPtr, pCfgTable->MsgEnumTbl))
        {
          result = USER_RESULT_ERROR;
          if (User_OutputMsgString( Value, FALSE ))
          {
            result = USER_RESULT_OK; 
          }
        }
        else
        {
          //Conversion error
          User_OutputMsgString( USER_MSG_RSP_CONVERSION_ERR, FALSE );
        }
      }
    }
    pCfgTable++;
  }

  
  // Build up the list of tables following the scalar entries.
  // The CFG table must be organized such that the arrays, follow the scalars.
  // See IMPORTANT NOTE for CfgCmd 
    
  // From this point on in the CFG table, the entries contain ptrs to tables of
  // arrays (STATES  and TCS)
  // See IMPORTANT NOTE for CfgCmd

  while ( result == USER_RESULT_OK &&
          pCfgTable->pNext != NULL && pCfgTable->pNext != NO_NEXT_TABLE )
  { 
     // Substruct name was the first entry to terminate the while-loop above
     strncpy_safe(LabelStem, sizeof(LabelStem), pCfgTable->MsgStr, _TRUNCATE);

     for (idx =  pCfgTable->pNext->MsgIndexMin;
          idx <= pCfgTable->pNext->MsgIndexMax && result == USER_RESULT_OK; ++idx)
     {
       // Display element info above each set of data.
       sprintf(Label, "\r\n\r\n%s%s[%d]",BranchName, LabelStem, idx);

       // Assume the worse, to terminate this for-loop 
       result = USER_RESULT_ERROR;
       if ( User_OutputMsgString( Label, FALSE ) )
       {
         result = User_DisplayConfigTree(NextBranchName, pCfgTable->pNext, idx, 0, NULL);
       }
     }
     pCfgTable++;
  }  

  return result;
}

/*****************************************************************************/
/* Local Functions                                                           */ 
/*****************************************************************************/

/*************************************************************************
*  MODIFICATIONS
*    $History: FASTStateMgrUserTables.c $
 * 
 * *****************  Version 8  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 2:36p
 * Updated in $/software/control processor/code/application
 * SCR# 1142 - more format corrections
 * 
 * *****************  Version 7  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 12:43p
 * Updated in $/software/control processor/code/application
 * SCR# 1142
 * 
 * *****************  Version 6  *****************
 * User: Jim Mood     Date: 10/11/11   Time: 5:31p
 * Updated in $/software/control processor/code/application
 * SCR 1078 Update for Code Review Findings
 * 
 * *****************  Version 5  *****************
 * User: Jim Mood     Date: 9/23/11    Time: 8:02p
 * Updated in $/software/control processor/code/application
 * SCR 575 Modfix for the list of running task not being reliable in the
 * fsm.status user command.
 * 
 * *****************  Version 4  *****************
 * User: Jim Mood     Date: 8/25/11    Time: 6:06p
 * Updated in $/software/control processor/code/application
 * SCR 575 Modfix
 * 
 * *****************  Version 3  *****************
 * User: Jim Mood     Date: 8/22/11    Time: 6:24p
 * Updated in $/software/control processor/code/application
 * SCR 575 Modfixes
 * 
 * *****************  Version 2  *****************
 * User: Contractor V&v Date: 8/22/11    Time: 12:35p
 * Updated in $/software/control processor/code/application
 * SCR #575 Add Showcfg to FastStateMgr
 * 
 * *****************  Version 1  *****************
 * User: Jim Mood     Date: 7/20/11    Time: 10:03a
 * Created in $/software/control processor/code/application
****************************************************************************/
