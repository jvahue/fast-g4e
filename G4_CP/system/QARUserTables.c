#define QAR_USERTABLES_BODY
/******************************************************************************
            Copyright (C) 2007-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:        QARUserTables.c
    
    Description: User commands definition for QAR Manager

    VERSION
    $Revision: 39 $  $Date: 12-11-13 2:42p $
    
******************************************************************************/
#ifndef QAR_MANAGER_BODY
#error QARUserTables.c should only be included by QARManager.c
#endif

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "CfgManager.h"

/*****************************************************************************/
/* Local Defines                                                             */  
/*****************************************************************************/

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/

USER_HANDLER_RESULT QARMsg_Register(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr);
USER_HANDLER_RESULT QARMsg_State(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr);
USER_HANDLER_RESULT QARMsg_State_SubFrameOk(USER_DATA_TYPE DataType,
                                            USER_MSG_PARAM Param,
                                            UINT32 Index,
                                            const void *SetPtr,
                                            void **GetPtr);
/* Function not defined
USER_HANDLER_RESULT QARMsg_State_DebugCntl(USER_DATA_TYPE DataType,
                                           USER_MSG_PARAM Param,
                                           UINT32 Index,
                                           const void *SetPtr,
                                           void **GetPtr);
*/
USER_HANDLER_RESULT QARMsg_Cfg(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr);
USER_HANDLER_RESULT QARMsg_CfgBarker(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr);
USER_HANDLER_RESULT QARMsg_Reconfigure(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr);
#ifdef GENERATE_SYS_LOGS 
USER_HANDLER_RESULT QARMsg_CreateLogs(USER_DATA_TYPE DataType,
                                      USER_MSG_PARAM Param,
                                      UINT32 Index,
                                      const void *SetPtr,
                                      void **GetPtr);
#endif
USER_HANDLER_RESULT QARMsg_ShowConfig(USER_DATA_TYPE DataType,
                                      USER_MSG_PARAM Param,
                                      UINT32 Index,
                                      const void *SetPtr,
                                      void **GetPtr);

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
typedef enum {
 QAR_REG_TSTP, 
 QAR_REG_TSTN, 
 QAR_REG_EN, 
 QAR_REG_FORMAT, 
 QAR_REG_NUMWORDS, 
 QAR_REG_RESYNC, 
 QAR_REG_BIPP,
 QAR_REG_BIIN,
 QAR_REG_SUBFRAME, 
 QAR_REG_LOSSOFFRAMESYNC, 
 QAR_REG_FRAMEVALID, 
 QAR_REG_BARKER, 
 QAR_REG_BARKER_ERROR, 
 QAR_REG_DATA_PRESENT
} QAR_REG_ENUM;

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
QAR_STATE QAR_StateTemp;
QAR_CONFIGURATION QAR_CfgTemp;


QAR_REGISTERS QAR_register;
UINT16 QarRegData;   

USER_ENUM_TBL FormatStrs[] =
{
  {"BIPOLAR_RETURN_ZERO",QAR_BIPOLAR_RETURN_ZERO},
  {"HARVARD_BIPHASE",QAR_HARVARD_BIPHASE},
  {NULL,0}
};
USER_ENUM_TBL NumWordsStrs[] =
{
  {"64_WORDS",QAR_64_WORDS},
  {"128_WORDS",QAR_128_WORDS},
  {"256_WORDS",QAR_256_WORDS},
  {"512_WORDS",QAR_512_WORDS},
  {"1024_WORDS",QAR_1024_WORDS},
  {NULL,0}
};
USER_ENUM_TBL ResyncStrs[] =
{
  {"NORMAL_OPERATION",QAR_NORMAL_OPERATION},
  {"FORCE_REFRAME",QAR_FORCE_REFRAME},
  {NULL,0}
};
USER_ENUM_TBL SyncStatusStrs[] =
{
  {"FRAME_INVALID",QAR_FRAME_INVALID},
  {"FRAME_SYNC",QAR_FRAME_SYNC},
  {NULL,0}
};
USER_ENUM_TBL SFStrs[] =
{
  {"SF1",QAR_SF1},
  {"SF2",QAR_SF2},
  {"SF3",QAR_SF3},
  {"SF4",QAR_SF4},
  {NULL,0}
};
USER_ENUM_TBL SFStateStrs[] =
{
  {"NO_STATE",QAR_NO_STATE},
  {"SYNCD",QAR_SYNCD},
  {"LOSF",QAR_LOSF},
  {NULL,0}
};
USER_ENUM_TBL OutputStrs[] =
{
//{"ASCII",QAR_UART1_ASCII},
//{"BINARY",QAR_UART1_BINARY},
  {"GSE_ASCII",QAR_GSE_ASCII},
  {"NONE", QAR_GSE_NONE}, 
  {NULL,0}
};

USER_ENUM_TBL QarSysStatusStrs[] = 
{
  {"OK",QAR_STATUS_OK},
  {"FAULTED_PBIT",QAR_STATUS_FAULTED_PBIT},
  {"FAULTED_CBIT",QAR_STATUS_FAULTED_CBIT},
  {"DISABLED_OK",QAR_STATUS_DISABLED_OK},
  {"DISABLED_FAULTED",QAR_STATUS_DISABLED_FAULTED},
  {NULL,0}
};



static USER_MSG_TBL QarStatusTbl [] =
{/* Str                     Next Tbl Ptr    Handler Func.            Data Type           Access    Parameter                             IndexRange   DataLimit  EnumTbl*/
  {"CURRENT_FRAME"          ,NO_NEXT_TABLE ,QARMsg_State            ,USER_TYPE_ENUM     ,USER_RO  ,&QAR_StateTemp.CurrentFrame           ,-1,-1      ,NO_LIMIT  ,SFStrs},
  {"PREVIOUS_FRAME"         ,NO_NEXT_TABLE ,QARMsg_State            ,USER_TYPE_ENUM     ,USER_RO  ,&QAR_StateTemp.PreviousFrame          ,-1,-1      ,NO_LIMIT  ,SFStrs},
  {"LAST_SERVICED"          ,NO_NEXT_TABLE ,QARMsg_State            ,USER_TYPE_ENUM     ,USER_RO  ,&QAR_StateTemp.LastServiced           ,-1,-1      ,NO_LIMIT  ,SFStrs},
  {"TOTAL_WORDS"            ,NO_NEXT_TABLE ,QARMsg_State            ,USER_TYPE_UINT16   ,USER_RO  ,&QAR_StateTemp.TotalWords             ,-1,-1      ,NO_LIMIT  ,SFStrs},
  {"LOSS_OF_FRAME"          ,NO_NEXT_TABLE ,QARMsg_State            ,USER_TYPE_BOOLEAN  ,USER_RO  ,&QAR_StateTemp.LossOfFrame            ,-1,-1      ,NO_LIMIT  ,NULL},
  {"LOSS_OF_FRAME_CNT"      ,NO_NEXT_TABLE ,QARMsg_State            ,USER_TYPE_UINT32   ,USER_RO  ,&QAR_StateTemp.LossOfFrameCount       ,-1,-1      ,NO_LIMIT  ,NULL},
  // New P Lee 04/02/2008 
  {"BARKER_ERROR"           ,NO_NEXT_TABLE ,QARMsg_State            ,USER_TYPE_BOOLEAN  ,USER_RO  ,&QAR_StateTemp.BarkerError            ,-1,-1      ,NO_LIMIT  ,NULL},
  {"BARKER_ERROR_CNT"       ,NO_NEXT_TABLE ,QARMsg_State            ,USER_TYPE_UINT32   ,USER_RO  ,&QAR_StateTemp.BarkerErrorCount       ,-1,-1      ,NO_LIMIT  ,NULL},
  // New P Lee 04/02/2008 
  {"FRAME_SYNC"             ,NO_NEXT_TABLE ,QARMsg_State            ,USER_TYPE_BOOLEAN  ,USER_RO  ,&QAR_StateTemp.FrameSync              ,-1,-1      ,NO_LIMIT  ,NULL},
  {"FRAME_STATE"            ,NO_NEXT_TABLE ,QARMsg_State            ,USER_TYPE_ENUM     ,USER_RO  ,&QAR_StateTemp.FrameState             ,-1,-1      ,NO_LIMIT  ,SFStateStrs},
  {"DATA_PRESENT"           ,NO_NEXT_TABLE ,QARMsg_State            ,USER_TYPE_BOOLEAN  ,USER_RO  ,&QAR_StateTemp.DataPresent            ,-1,-1      ,NO_LIMIT  ,NULL},
  {"DATA_PRES_LOST_CNT"     ,NO_NEXT_TABLE ,QARMsg_State            ,USER_TYPE_UINT32   ,USER_RO  ,&QAR_StateTemp.DataPresentLostCount   ,-1,-1      ,NO_LIMIT  ,NULL},
  // New P Lee 04/02/2008 
  {"PREVIOUS_SUBFRAME_OK"   ,NO_NEXT_TABLE ,QARMsg_State            ,USER_TYPE_ENUM     ,USER_RO  ,&QAR_StateTemp.PreviousSubFrameOk     ,-1,-1      ,NO_LIMIT  ,SFStrs},
  {"LAST_INT_TIME"          ,NO_NEXT_TABLE ,QARMsg_State            ,USER_TYPE_UINT32   ,USER_RO  ,&QAR_StateTemp.LastIntTime            ,-1,-1      ,NO_LIMIT  ,NULL},
  {"BAD_INT_FREQ_CNT"       ,NO_NEXT_TABLE ,QARMsg_State            ,USER_TYPE_UINT32   ,USER_RO  ,&QAR_StateTemp.BadIntFreqCnt          ,-1,-1      ,NO_LIMIT  ,NULL},
  {"SUBFRAME_OK"            ,NO_NEXT_TABLE ,QARMsg_State_SubFrameOk ,USER_TYPE_BOOLEAN  ,USER_RO  ,&QAR_StateTemp.bSubFrameOk[0]         , 0, 3      ,NO_LIMIT  ,NULL},
  {"QARENABLE"              ,NO_NEXT_TABLE ,QARMsg_State            ,USER_TYPE_BOOLEAN  ,USER_RO  ,&QAR_StateTemp.bQAREnable             ,-1,-1      ,NO_LIMIT  ,NULL},
  {"LASTACTIVITYTIME"       ,NO_NEXT_TABLE ,QARMsg_State            ,USER_TYPE_UINT32   ,USER_RO  ,&QAR_StateTemp.LastActivityTime       ,-1,-1      ,NO_LIMIT  ,NULL},
  {"CHANNELSTARTUP_TIMEOUT" ,NO_NEXT_TABLE ,QARMsg_State            ,USER_TYPE_BOOLEAN  ,USER_RO  ,&QAR_StateTemp.bChannelStartupTimeOut ,-1,-1      ,NO_LIMIT  ,NULL},
  {"CHANNEL_TIMEOUT"        ,NO_NEXT_TABLE ,QARMsg_State            ,USER_TYPE_BOOLEAN  ,USER_RO  ,&QAR_StateTemp.bChannelTimeOut        ,-1,-1      ,NO_LIMIT  ,NULL},
  {"INTERRUPT_CNT"          ,NO_NEXT_TABLE ,QARMsg_State            ,USER_TYPE_UINT32   ,USER_RO  ,&QAR_StateTemp.InterruptCnt           ,-1,-1      ,NO_LIMIT  ,NULL},
  {"INTERRUPT_CNT_LOFS"     ,NO_NEXT_TABLE ,QARMsg_State            ,USER_TYPE_UINT32   ,USER_RO  ,&QAR_StateTemp.InterruptCntLOFS       ,-1,-1      ,NO_LIMIT  ,NULL},
  // New P Lee 04/02/2008 
  {"SYSTEM_STATUS"          ,NO_NEXT_TABLE ,QARMsg_State            ,USER_TYPE_ENUM     ,USER_RO  ,&QAR_StateTemp.SystemStatus           ,-1,-1      ,NO_LIMIT  ,QarSysStatusStrs},
    
  {NULL                     ,NULL          ,NULL                    ,NO_HANDLER_DATA}
};
static USER_MSG_TBL QarCfgTbl [] =
{/* Str                Next Tbl Ptr   Handler Func.     Data Type          Access    Parameter                       IndexRange  DataLimit       EnumTbl*/
  {"NAME"             ,NO_NEXT_TABLE ,QARMsg_Cfg       ,USER_TYPE_STR     ,USER_RW  ,&QAR_CfgTemp.Name             ,-1,-1      ,0,QAR_MAX_NAME ,NULL},
  {"FORMAT"           ,NO_NEXT_TABLE ,QARMsg_Cfg       ,USER_TYPE_ENUM    ,USER_RW  ,&QAR_CfgTemp.Format           ,-1,-1      ,NO_LIMIT       ,FormatStrs},
  {"NUMWORDS"         ,NO_NEXT_TABLE ,QARMsg_Cfg       ,USER_TYPE_ENUM    ,USER_RW  ,&QAR_CfgTemp.NumWords         ,-1,-1      ,NO_LIMIT       ,NumWordsStrs},
  {"BARKER"           ,NO_NEXT_TABLE ,QARMsg_CfgBarker ,USER_TYPE_HEX16   ,USER_RW  ,&QAR_CfgTemp.BarkerCode[0]    , 0, 3      ,0,0xFFF        ,NULL},
  // New P Lee 04/02/2008 
  {"CHANNELSTARTUP_S" ,NO_NEXT_TABLE ,QARMsg_Cfg       ,USER_TYPE_UINT32  ,USER_RW  ,&QAR_CfgTemp.ChannelStartup_s ,-1,-1      ,NO_LIMIT       ,NULL},
  {"CHANNELTIMEOUT_S" ,NO_NEXT_TABLE ,QARMsg_Cfg       ,USER_TYPE_UINT32  ,USER_RW  ,&QAR_CfgTemp.ChannelTimeOut_s ,-1,-1      ,NO_LIMIT       ,NULL},    
  {"CHANNELSYSCOND"   ,NO_NEXT_TABLE ,QARMsg_Cfg       ,USER_TYPE_ENUM    ,USER_RW  ,&QAR_CfgTemp.ChannelSysCond   ,-1,-1      ,NO_LIMIT       ,Flt_UserEnumStatus},
  {"PBITSYSCOND"      ,NO_NEXT_TABLE ,QARMsg_Cfg       ,USER_TYPE_ENUM    ,USER_RW  ,&QAR_CfgTemp.PBITSysCond      ,-1,-1      ,NO_LIMIT       ,Flt_UserEnumStatus},    
  // New P Lee 04/02/2008     
  // New P Lee 11/18/2008
  {"ENABLE"           ,NO_NEXT_TABLE ,QARMsg_Cfg       ,USER_TYPE_BOOLEAN ,USER_RW  ,&QAR_CfgTemp.Enable           ,-1,-1      ,NO_LIMIT       ,NULL}, 
  // New P Lee 11/18/2008
  {NULL               ,NULL          ,NULL             ,NO_HANDLER_DATA}
};

static USER_MSG_TBL QarRegisterControlTbl[] =
{/* Str        Next Tbl Ptr    Handler Func.     Data Type          Access    Parameter                IndexRange  DataLimit   EnumTbl*/
  {"TSTP"     ,NO_NEXT_TABLE  ,QARMsg_Register,  USER_TYPE_HEX16  ,USER_RO  ,(void*)QAR_REG_TSTP      ,-1,-1      ,NO_LIMIT   ,NULL},
  {"TSTN"     ,NO_NEXT_TABLE  ,QARMsg_Register,  USER_TYPE_HEX16  ,USER_RO  ,(void*)QAR_REG_TSTN      ,-1,-1      ,NO_LIMIT   ,NULL},
  {"ENABLE"   ,NO_NEXT_TABLE  ,QARMsg_Register,  USER_TYPE_HEX16  ,USER_RO  ,(void*)QAR_REG_EN        ,-1,-1      ,NO_LIMIT   ,NULL},
  {"FORMAT"   ,NO_NEXT_TABLE  ,QARMsg_Register,  USER_TYPE_HEX16  ,USER_RO  ,(void*)QAR_REG_FORMAT    ,-1,-1      ,NO_LIMIT   ,NULL},
  {"NUMWORDS" ,NO_NEXT_TABLE  ,QARMsg_Register,  USER_TYPE_HEX16  ,USER_RO  ,(void*)QAR_REG_NUMWORDS  ,-1,-1      ,NO_LIMIT   ,NULL},
  {"RESYNC"   ,NO_NEXT_TABLE  ,QARMsg_Register,  USER_TYPE_HEX16  ,USER_RO  ,(void*)QAR_REG_RESYNC    ,-1,-1      ,NO_LIMIT   ,NULL},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL QarRegisterStatusTbl[] =
{/* Str            Next Tbl Ptr      Handler Func.       Data Type         Access    Parameter                    IndexRange  DataLimit  EnumTbl*/
  {"BIIP",         NO_NEXT_TABLE,    QARMsg_Register,  USER_TYPE_HEX16,  USER_RO,  (void*)QAR_REG_BIPP,           -1,-1,      NO_LIMIT,  NULL},
  {"BIIN",         NO_NEXT_TABLE,    QARMsg_Register,  USER_TYPE_HEX16,  USER_RO,  (void*)QAR_REG_BIIN,           -1,-1,      NO_LIMIT,  NULL},
  {"SUBFRAME",     NO_NEXT_TABLE,    QARMsg_Register,  USER_TYPE_HEX16,  USER_RO,  (void*)QAR_REG_SUBFRAME,       -1,-1,      NO_LIMIT,  NULL},
  {"LOFS",         NO_NEXT_TABLE,    QARMsg_Register,  USER_TYPE_HEX16,  USER_RO,  (void*)QAR_REG_LOSSOFFRAMESYNC,-1,-1,      NO_LIMIT,  NULL},
  {"FRAME_VALID",  NO_NEXT_TABLE,    QARMsg_Register,  USER_TYPE_HEX16,  USER_RO,  (void*)QAR_REG_FRAMEVALID,     -1,-1,      NO_LIMIT,  NULL},
  {"BARKER_ERROR", NO_NEXT_TABLE,    QARMsg_Register,  USER_TYPE_HEX16,  USER_RO,  (void*)QAR_REG_BARKER_ERROR,   -1,-1,      NO_LIMIT,  NULL},
  {"DATA_PRESENT", NO_NEXT_TABLE,    QARMsg_Register,  USER_TYPE_HEX16,  USER_RO,  (void*)QAR_REG_DATA_PRESENT,   -1,-1,      NO_LIMIT,  NULL},
  {NULL,           NULL,             NULL,             NO_HANDLER_DATA}
};
USER_MSG_TBL QarRegisterTbl[] =
{/* Str         Next Tbl Ptr           Handler Func.       Data Type         Access    Parameter              IndexRange  DataLimit  EnumTbl*/
  {"CONTROL",   QarRegisterControlTbl, NULL,               NO_HANDLER_DATA},
  {"STATUS",    QarRegisterStatusTbl,  NULL,               NO_HANDLER_DATA},
  {"BARKER",    NO_NEXT_TABLE,         QARMsg_Register,    USER_TYPE_HEX16,  USER_RO,  (void*)QAR_REG_BARKER, 0,3,        NO_LIMIT,  NULL},
  { NULL,       NULL,                  NULL,               NO_HANDLER_DATA}
};

static USER_MSG_TBL QarRoot[] =
{/* Str            Next Tbl Ptr   Handler Func.         Data Type         Access                         Parameter   IndexRange  DataLimit  EnumTbl*/
  {"REGISTER",     QarRegisterTbl,NULL,                 NO_HANDLER_DATA},
  {"STATUS",       QarStatusTbl,  NULL,                 NO_HANDLER_DATA},
  {"CFG",          QarCfgTbl,     NULL,                 NO_HANDLER_DATA},
  {"RECONFIGURE",  NO_NEXT_TABLE, QARMsg_Reconfigure  , USER_TYPE_ACTION, USER_RO,                        NULL,       -1,-1,     NO_LIMIT, NULL},
  {"OUTPUT"     ,  NO_NEXT_TABLE ,User_GenericAccessor, USER_TYPE_ENUM  , USER_RW  ,&m_QarOutputType,    -1,-1        ,NO_LIMIT ,OutputStrs},
#ifdef GENERATE_SYS_LOGS  
  {"CREATELOGS",   NO_NEXT_TABLE, QARMsg_CreateLogs,  USER_TYPE_ACTION, USER_RO,                          NULL,       -1,-1,     NO_LIMIT, NULL},
#endif
  {DISPLAY_CFG,    NO_NEXT_TABLE, QARMsg_ShowConfig,  USER_TYPE_ACTION, USER_RO|USER_NO_LOG|USER_GSE,     NULL,       -1,-1,    NO_LIMIT, NULL},  
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL QarRootTblPtr = {"QAR",QarRoot,NULL,NO_HANDLER_DATA};



/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/
/******************************************************************************
 * Function:    QARMsg_Register (qar.register)
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retreives the latest register values from the QAR driver and
 *              returns the result the user module
 *              
 * Parameters:                DataType  
 *                       [in] Param     indicates what register to get
 *                       [in] Index.    Used for Barker codes array, range is 0-3
 *                            SetPtr    
 *                       [out]GetPtr    Writes the register value to the
 *                                             integer pointed to by User.c
 *
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
 *****************************************************************************/
USER_HANDLER_RESULT QARMsg_Register(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr)
{
  QAR_GetRegisters( &QAR_register);

  switch(Param.Int)
  {
    // Note: Have to use switch case since .Tstp, .Tstn, etc are bit fields and pointers to
    //       bit fields is not allowed, which limits a "cleaner" implementation. 
  
    case QAR_REG_TSTP:
      QarRegData = QAR_R((UINT16 *) &QAR_register.Control, C_TSTP);
      break; 
  
    case QAR_REG_TSTN:
      QarRegData = QAR_R((UINT16 *) &QAR_register.Control, C_TSTN); 
      break; 
  
    case QAR_REG_EN:
      QarRegData = QAR_R((UINT16 *) &QAR_register.Control, C_ENABLE); 
      break; 
  
    case QAR_REG_FORMAT:
      QarRegData = QAR_R((UINT16 *) &QAR_register.Control, C_FORMAT); 
      break;
    
    case QAR_REG_NUMWORDS:
      QarRegData = QAR_R((UINT16 *) &QAR_register.Control, C_NUM_WORDS); 
      break;
    
    case QAR_REG_RESYNC:
      QarRegData = QAR_R((UINT16 *) &QAR_register.Control, C_RESYNC); 
      break;
    
    case QAR_REG_BIPP:
      QarRegData = QAR_R((UINT16 *) &QAR_register.Status, S_BIIP); 
      break; 
    
    case QAR_REG_BIIN:
      QarRegData = QAR_R((UINT16 *) &QAR_register.Status, S_BIIN); 
      break;
    
    case QAR_REG_SUBFRAME:
      QarRegData = QAR_R((UINT16 *) &QAR_register.Status, S_SUBFRAME); 
      break;
    
    case QAR_REG_LOSSOFFRAMESYNC:
      QarRegData = QAR_R((UINT16 *) &QAR_register.Status, S_LOST_SYNC); 
      break;
    
    case QAR_REG_FRAMEVALID:
      QarRegData = QAR_R((UINT16 *) &QAR_register.Status, S_FRAME_VALID); 
      break;
    
    // New P Lee 04/02/2008 
    case QAR_REG_BARKER_ERROR:
      QarRegData = QAR_R((UINT16 *) &QAR_register.Status, S_BARKER_ERR); 
      break;
    
    case QAR_REG_DATA_PRESENT:
      QarRegData = QAR_R((UINT16 *) &QAR_register.Status, S_DATA_PRESENT); 
      break;
      
    // New P Lee 04/02/2008
    case QAR_REG_BARKER:
      QarRegData = QAR_register.Barker[Index];
      break;  
    
  }
  
  *GetPtr = (UINT16 *) &QarRegData; 
  
  return USER_RESULT_OK;
}


/******************************************************************************
 * Function:    QARMsg_State (qar.state)
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retreives the latest state values from the QAR driver and
 *              returns the specific state value to the user module
 *              
 * Parameters:  [in/out] DataType
 *              [in]  Param - TableData.MsgParam indicates what state member
 *                                               to get
 *                    Index            
 *                    SetPtr           
 *              [out] GetPtr - TableData.GetPtr Writes the state GetPtr to 
 *                                             reference the State member
 *                                             requested
 *
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
 *****************************************************************************/
USER_HANDLER_RESULT QARMsg_State(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr)
{
  QAR_StateTemp = *QAR_GetState();  
  return  User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr); 
}

/******************************************************************************
 * Function:    QARMsg_State_SubFrameOk (qar.state)
 *
 * Description: 
 *              
 * Parameters:  [in/out] DataType
 *              [in]  Param - TableData.MsgParam indicates what state member
 *                                               to get
 *                    Index     
 *                    SetPtr    
 *              [out] GetPtr - TableData.GetPtr Writes the state GetPtr to 
 *                                             reference the State member
 *                                             requested
 *
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
 *****************************************************************************/
USER_HANDLER_RESULT QARMsg_State_SubFrameOk(USER_DATA_TYPE DataType,
                                            USER_MSG_PARAM Param,
                                            UINT32 Index,
                                            const void *SetPtr,
                                            void **GetPtr)
{
  QAR_StateTemp = *QAR_GetState(); 
  
  **(UINT8**)GetPtr = ((UINT8*)Param.Ptr)[Index];
  
  // *GetPtr = Param.Ptr;
  return USER_RESULT_OK;
}

/******************************************************************************
 * Function:    QARMsg_Cfg (qar.cfg)
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retreives a cfg member value from the QAR driver and
 *              returns the result the user module, or it sets the cfg member
 *              value if the SetPtr in the Data parameter is not null.
 *              
 * Parameters:  [in/out] DataType
 *              [in]  Param - TableData.MsgParam indicates what register to get
 *                    Index     
 *              [in]  SetPtr - indicates if this is a "set" or "get"
 *              [out] GetPtr - TableData.GetPtr Writes the register value to the
 *                                             integer pointed to by User.c
 *
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
 *****************************************************************************/
USER_HANDLER_RESULT QARMsg_Cfg(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr)
{
   USER_HANDLER_RESULT result = USER_RESULT_OK;

   memcpy(&QAR_CfgTemp, &CfgMgr_ConfigPtr()->QARConfig, sizeof(QAR_CfgTemp));
  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
  // If performing a "set" and it was successful, write the update.
  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    memcpy(&CfgMgr_ConfigPtr()->QARConfig,
      &QAR_CfgTemp,
      sizeof(QAR_CfgTemp));

    CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
      &CfgMgr_ConfigPtr()->QARConfig,
      sizeof(QAR_CfgTemp));
  }

  QAR_SetCfg(&QAR_CfgTemp);   
  return result;
}

/******************************************************************************
 * Function:    QARMsg_CfgBarker (qar.cfg.barker[x])
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retreives a cfg.barker member value from the QAR driver and
 *              returns the result the user module, or it sets the cfg member
 *              value if the SetPtr in the Data parameter is not null.
 *              
 * Parameters:  [in/out] DataType
 *              [in]  Param - TableData.MsgParam indicates what register to get
 *              [in]  Index - indicates which barker code to get 0-3
 *              [in]  SetPtr - indicates if this is a "set" or "get"
 *              [out] GetPtr - TableData.GetPtr Writes the register value to the
 *                                             integer pointed to by User.c
 *
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
 *****************************************************************************/
USER_HANDLER_RESULT QARMsg_CfgBarker(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr)
{
  //QAR_GetCfg(&QAR_CfgTemp);
  QAR_CfgTemp = *QAR_GetCfg(); 
  //Setting or getting this value?  (if the SetPtr is null, assume this is
  //to "get" the cfg value)
  if(SetPtr == NULL)
  {
   //Set the GetPtr to the Cfg member referenced in the table
    **(UINT16**)GetPtr = ((UINT16*)Param.Ptr)[Index];
  }
  else
  {
    //Write the set param value to cfg member referenced by the table
    ((UINT16*)Param.Ptr)[Index] = *(UINT16*)SetPtr;
    
     memcpy(&CfgMgr_ConfigPtr()->QARConfig,
            &QAR_CfgTemp,
            sizeof(QAR_CfgTemp));

     CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
                            &CfgMgr_ConfigPtr()->QARConfig,
                            sizeof(QAR_CfgTemp));
    
    QAR_SetCfg(&QAR_CfgTemp);
  }
  return USER_RESULT_OK;
}



/******************************************************************************
 * Function:    QARMsg_Reconfigure (qar.reconfigure)
 *
 * Description: Executes the reconfigure function of QAR.c
 *              
 * Parameters:  USER_DATA_TYPE     DataType
 *              USER_MSG_PARAM     Param
 *              UINT32             Index
 *              const void         *SetPtr
 *              void               **GetPtr
 *
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
 *****************************************************************************/
USER_HANDLER_RESULT QARMsg_Reconfigure(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr)
{
  QAR_Reconfigure();
  
  return USER_RESULT_OK;
  
}


/******************************************************************************
 * Function:    QARMsg_CreateLogs (qar.createlogs)
 *
 * Description: Create all the internall QAR system logs
 *              
 * Parameters:  USER_DATA_TYPE    DataType
 *              USER_MSG_PARAM    Param
 *              UINT32            Index
 *              const void        *SetPtr
 *              void              **GetPtr
 *
 * Returns:     USER_RESULT_OK:    Processed successfully
 *              USER_RESULT_ERROR: Error processing command. 
 *
 * Notes:
 *
 *****************************************************************************/
#ifdef GENERATE_SYS_LOGS

/*vcast_dont_instrument_start*/
USER_HANDLER_RESULT QARMsg_CreateLogs(USER_DATA_TYPE DataType,
                                      USER_MSG_PARAM Param,
                                      UINT32 Index,
                                      const void *SetPtr,
                                      void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_OK; 

  QAR_CreateAllInternalLogs();
  
  return result;
}
/*vcast_dont_instrument_end*/

#endif
/******************************************************************************
* Function:    QARMsg_ShowConfig
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
USER_HANDLER_RESULT QARMsg_ShowConfig(USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr)
{
   CHAR  LabelStem[] = "\r\n\r\nQAR.CFG";   
   USER_HANDLER_RESULT result = USER_RESULT_OK;
   USER_MSG_TBL*  pCfgTable;

   //Top-level name is a single indented space
   CHAR BranchName[USER_MAX_MSG_STR_LEN] = " ";   

   pCfgTable = QarCfgTbl;  // Get pointer to config entry
   
   result = USER_RESULT_ERROR;
  if (User_OutputMsgString(LabelStem, FALSE ) )
  {
     result = User_DisplayConfigTree(BranchName, pCfgTable, 0, 0, LabelStem);
  }  
  return result;
}

/*************************************************************************
*  MODIFICATIONS
*    $History: QARUserTables.c $
 * 
 * *****************  Version 39  *****************
 * User: Melanie Jutras Date: 12-11-13   Time: 2:42p
 * Updated in $/software/control processor/code/system
 * SCR #1142 File Format Errors
 * 
 * *****************  Version 38  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 37  *****************
 * User: Peter Lee    Date: 11/18/10   Time: 4:44p
 * Updated in $/software/control processor/code/system
 * SCR #1004 "qar.status.subframe_ok[0]"  s/b UINT8 for BOOLEAN and not
 * UINT16
 * 
 * *****************  Version 36  *****************
 * User: Peter Lee    Date: 9/01/10    Time: 3:38p
 * Updated in $/software/control processor/code/system
 * SCR #840 Code Review Updates
 * 
 * *****************  Version 35  *****************
 * User: Jeff Vahue   Date: 8/28/10    Time: 12:22p
 * Updated in $/software/control processor/code/system
 * SCR# 830 - GENERATE_SYS_LOGS exclude area from coverage results
 * 
 * *****************  Version 34  *****************
 * User: Contractor3  Date: 7/16/10    Time: 8:57a
 * Updated in $/software/control processor/code/system
 * SCR #702 - Changes based on code review.
 * 
 * *****************  Version 33  *****************
 * User: Contractor V&v Date: 6/23/10    Time: 2:23p
 * Updated in $/software/control processor/code/system
 * SCR #485 Escape Sequence and Box Configuration fixed merge error -
 * moved OUTPUT frm qar.cfg. to qar.
 * 
 * *****************  Version 32  *****************
 * User: Contractor V&v Date: 6/08/10    Time: 5:55p
 * Updated in $/software/control processor/code/system
 * SCR #615 Showcfg/Long msg enhancement
 * 
 * *****************  Version 31  *****************
 * User: Peter Lee    Date: 5/24/10    Time: 6:37p
 * Updated in $/software/control processor/code/system
 * SCR #608 Update QAR Interface Status logic to match Arinc / UART. 
 * 
 * *****************  Version 30  *****************
 * User: Contractor V&v Date: 5/19/10    Time: 6:11p
 * Updated in $/software/control processor/code/system
 * SCR #404 Misc - Preliminary Code Review Issues
 * 
 * *****************  Version 29  *****************
 * User: Contractor2  Date: 5/06/10    Time: 2:10p
 * Updated in $/software/control processor/code/system
 * SCR #579 Change LogWriteSys to return void
 * 
 * *****************  Version 28  *****************
 * User: Jeff Vahue   Date: 4/12/10    Time: 5:43p
 * Updated in $/software/control processor/code/system
 * SCR #541 - all XXX_CreateLogs functions are now dependent on
 * GENERATE_SYS_LOGS
 * 
 * *****************  Version 27  *****************
 * User: Contractor V&v Date: 3/10/10    Time: 4:42p
 * Updated in $/software/control processor/code/system
 * SCR #437 All passive user commands should be RO
 * 
 * *****************  Version 26  *****************
 * User: Jeff Vahue   Date: 3/05/10    Time: 12:16p
 * Updated in $/software/control processor/code/system
 * SCR# 413 - Actions cmds are RO
 * 
 * *****************  Version 25  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 4:03p
 * Updated in $/software/control processor/code/system
 * SCR 106 Remove USE_GENERIC_ACCESSOR macro
 * 
 * *****************  Version 24  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 23  *****************
 * User: Jeff Vahue   Date: 2/17/10    Time: 1:29p
 * Updated in $/software/control processor/code/system
 * SCR# 452 - code coverage changes
 * 
 * *****************  Version 22  *****************
 * User: Peter Lee    Date: 2/03/10    Time: 5:14p
 * Updated in $/software/control processor/code/system
 * SCR #439 Remove unneeded  NO_BYTES setting. 
 * 
 * *****************  Version 20  *****************
 * User: Contractor V&v Date: 1/13/10    Time: 4:58p
 * Updated in $/software/control processor/code/system
 * 
 * *****************  Version 19  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:27p
 * Updated in $/software/control processor/code/system
 * SCR 333
 * 
 * *****************  Version 18  *****************
 * User: Contractor V&v Date: 12/10/09   Time: 5:46p
 * Updated in $/software/control processor/code/system
 * SCR 42 and SCR 106
*
****************************************************************************/

