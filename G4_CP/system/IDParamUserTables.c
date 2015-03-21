#define IDPARAM_USERTABLES_BODY
/******************************************************************************
            Copyright (C) 2007-2015 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    ECCN:        9E991

    File:        IDParamUserTables.c

    Description: Routines to support the user commands for ID Param Protocol CSC

    VERSION
    $Revision: 9 $  $Date: 3/19/15 10:40a $

******************************************************************************/
#ifndef ID_PARAM_PROTOCOL_BODY
#error IDParamUserTables.c should only be included by IDParamProtocol.c
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
#define ID_PARAM_IDS_BUFF_MAX USER_SINGLE_MSG_MAX_SIZE
#define ID_PARAM_LC_BUFF_MAX  64

#define MAX_LABEL_CHAR_ARRAY 128
#define MAX_VALUE_CHAR_ARRAY 32


/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
static USER_HANDLER_RESULT IDParamMsg_Status(USER_DATA_TYPE DataType,
                                             USER_MSG_PARAM Param,
                                             UINT32 Index,
                                             const void *SetPtr,
                                             void **GetPtr);

static USER_HANDLER_RESULT IDParamMsg_Cfg(USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr);

static USER_HANDLER_RESULT IDParamDataMsg_Cfg(USER_DATA_TYPE DataType,
                                              USER_MSG_PARAM Param,
                                              UINT32 Index,
                                              const void *SetPtr,
                                              void **GetPtr);

static USER_HANDLER_RESULT IDParamScrollMsg_Cfg(USER_DATA_TYPE DataType,
                                                USER_MSG_PARAM Param,
                                                UINT32 Index,
                                                const void *SetPtr,
                                                void **GetPtr);

static USER_HANDLER_RESULT IDParamIDSMsg_Status(USER_DATA_TYPE DataType,
                                                USER_MSG_PARAM Param,
                                                UINT32 Index,
                                                const void *SetPtr,
                                                void **GetPtr);

static USER_HANDLER_RESULT IDParamMsg_ShowConfig(USER_DATA_TYPE DataType,
                                                 USER_MSG_PARAM Param,
                                                 UINT32 Index,
                                                 const void *SetPtr,
                                                 void **GetPtr);

static USER_HANDLER_RESULT IDParamMsg_Debug(USER_DATA_TYPE DataType,
                                            USER_MSG_PARAM Param,
                                            UINT32 Index,
                                            const void *SetPtr,
                                            void **GetPtr);


/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static ID_PARAM_CFG cfgIDParamTemp;
static ID_PARAM_DATA_CFG cfgIDParamDataTemp;
static ID_PARAM_STATUS statusIDParamTemp;
static ID_PARAM_SCROLL_CFG cfgIDParamScrollTemp;

static CHAR statusIDS_Buff[ID_PARAM_IDS_BUFF_MAX];
static ID_PARAM_DEBUG debugIDParamTemp;


/*****************************************************************************/
/* Local Constants                                                           */
/*****************************************************************************/
static
USER_ENUM_TBL idParamDebugFrameStrs[] =
{
  {"FRAME_0",  ID_PARAM_FADEC_ENUM},
  {"FRAME_1",  ID_PARAM_PEC_ENUM},
  { NULL,      0}
};

/*****************************************/
/* User Table Defintions                 */
/*****************************************/
static USER_MSG_TBL idParamDataCfgTbl[] =
{ /*Str           Next Tbl Ptr   Handler Func.         Data Type          Access    Parameter                                 IndexRange                DataLimit    EnumTbl*/
  {"ID",          NO_NEXT_TABLE, IDParamDataMsg_Cfg,   USER_TYPE_UINT16,  USER_RW,  (void *) &cfgIDParamDataTemp.id,          0, ID_PARAM_CFG_MAX - 1,  NO_LIMIT,    NULL},\
  {"SCALE",       NO_NEXT_TABLE, IDParamDataMsg_Cfg,   USER_TYPE_FLOAT,   USER_RW,  (void *) &cfgIDParamDataTemp.scale,       0, ID_PARAM_CFG_MAX - 1,  NO_LIMIT,    NULL},\
  {"TOL",         NO_NEXT_TABLE, IDParamDataMsg_Cfg,   USER_TYPE_FLOAT,   USER_RW,  (void *) &cfgIDParamDataTemp.tol,         0, ID_PARAM_CFG_MAX - 1,  NO_LIMIT,    NULL},\
  {"TIMEOUT_MS",  NO_NEXT_TABLE, IDParamDataMsg_Cfg,   USER_TYPE_UINT32,  USER_RW,  (void *) &cfgIDParamDataTemp.timeout_ms,  0, ID_PARAM_CFG_MAX - 1,  NO_LIMIT,    NULL},\
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL idParamScrollCfgTbl[] =
{ /*Str           Next Tbl Ptr   Handler Func.           Data Type          Access    Parameter                               IndexRange    DataLimit    EnumTbl*/
  {"ID",          NO_NEXT_TABLE, IDParamScrollMsg_Cfg,   USER_TYPE_UINT16,  USER_RW,  (void *) &cfgIDParamScrollTemp.id,      -1, -1,       NO_LIMIT,    NULL},\
  {"VALUE",       NO_NEXT_TABLE, IDParamScrollMsg_Cfg,   USER_TYPE_UINT16,  USER_RW,  (void *) &cfgIDParamScrollTemp.value,   -1, -1,       NO_LIMIT,    NULL},\
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

#define P_PARAMS_STR  "P"
static USER_MSG_TBL idParamCfgTbl[] =
{ /*Str           Next Tbl Ptr   Handler Func.     Data Type          Access    Parameter                          IndexRange   DataLimit               EnumTbl*/
  {"MAXWORDS",    NO_NEXT_TABLE, IDParamMsg_Cfg,   USER_TYPE_UINT16,  USER_RW,  (void *) &cfgIDParamTemp.maxWords, -1, -1,      10, ID_PARAM_CFG_MAX,    NULL},\
  {"SCROLL",      idParamScrollCfgTbl, NULL, NO_HANDLER_DATA},\
  {P_PARAMS_STR,  idParamDataCfgTbl, NULL, NO_HANDLER_DATA},\
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL idParamStatusFrame0TypeTbl[] =
{ /*Str               Next Tbl Ptr   Handler Func.      Data Type          Access     Parameter                                                    IndexRange    DataLimit    EnumTbl*/
  {"NUM_WORDS",       NO_NEXT_TABLE, IDParamMsg_Status, USER_TYPE_UINT16,  USER_RO,   (void *) &statusIDParamTemp.frameType[0].nWords,             1,      3,    NO_LIMIT,    NULL},\
  {"NUM_ELEMENTS",    NO_NEXT_TABLE, IDParamMsg_Status, USER_TYPE_UINT16,  USER_RO,   (void *) &statusIDParamTemp.frameType[0].nElements,          1,      3,    NO_LIMIT,    NULL},\
  {"FRAME_CNT",       NO_NEXT_TABLE, IDParamMsg_Status, USER_TYPE_UINT32,  USER_RO,   (void *) &statusIDParamTemp.frameType[0].cntGoodFrames,      1,      3,    NO_LIMIT,    NULL},\
  {"LAST_FRAME_TICK", NO_NEXT_TABLE, IDParamMsg_Status, USER_TYPE_UINT32,  USER_RO,   (void *) &statusIDParamTemp.frameType[0].lastFrameTime_tick, 1,      3,    NO_LIMIT,    NULL},\
  // NOTE:  IDS s/b USER_TYPE_ACTION so when top level idparam[].status is entered, the IDS won't automatically be printed
  {"IDS",             NO_NEXT_TABLE, IDParamIDSMsg_Status, USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG),  (void *) ID_PARAM_FRAME_TYPE_FADEC,          1,      3,    NO_LIMIT,    NULL},\
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL idParamStatusFrame1TypeTbl[] =
{ /*Str               Next Tbl Ptr   Handler Func.      Data Type          Access     Parameter                                                    IndexRange    DataLimit    EnumTbl*/
  {"NUM_WORDS",       NO_NEXT_TABLE, IDParamMsg_Status, USER_TYPE_UINT16,  USER_RO,   (void *) &statusIDParamTemp.frameType[1].nWords,             1,      3,    NO_LIMIT,    NULL},\
  {"NUM_ELEMENTS",    NO_NEXT_TABLE, IDParamMsg_Status, USER_TYPE_UINT16,  USER_RO,   (void *) &statusIDParamTemp.frameType[1].nElements,          1,      3,    NO_LIMIT,    NULL},\
  {"FRAME_CNT",       NO_NEXT_TABLE, IDParamMsg_Status, USER_TYPE_UINT32,  USER_RO,   (void *) &statusIDParamTemp.frameType[1].cntGoodFrames,      1,      3,    NO_LIMIT,    NULL},\
  {"LAST_FRAME_TICK", NO_NEXT_TABLE, IDParamMsg_Status, USER_TYPE_UINT32,  USER_RO,   (void *) &statusIDParamTemp.frameType[1].lastFrameTime_tick, 1,      3,    NO_LIMIT,    NULL},\
  // NOTE:  IDS s/b USER_TYPE_ACTION so when top level idparam[].status is entered, the IDS won't automatically be printed
  {"IDS",             NO_NEXT_TABLE, IDParamIDSMsg_Status, USER_TYPE_ACTION,  (USER_RO|USER_NO_LOG),  (void *) ID_PARAM_FRAME_TYPE_PEC,            1,      3,    NO_LIMIT,    NULL},\
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL idParamStatusTbl[] =
{ /*Str               Next Tbl Ptr   Handler Func.       Data Type          Access      Parameter                                       IndexRange   DataLimit    EnumTbl*/
  {"SYNC",            NO_NEXT_TABLE, IDParamMsg_Status,  USER_TYPE_BOOLEAN, USER_RO,    (void *) &statusIDParamTemp.sync,               1,   3,    NO_LIMIT,    NULL},\
  {"BAD_CHKSUM_CNT",  NO_NEXT_TABLE, IDParamMsg_Status,  USER_TYPE_UINT32,  USER_RO,    (void *) &statusIDParamTemp.cntBadChksum,       1,   3,    NO_LIMIT,    NULL},\
  {"BAD_FRAME_CNT",   NO_NEXT_TABLE, IDParamMsg_Status,  USER_TYPE_UINT32,  USER_RO,    (void *) &statusIDParamTemp.cntBadFrame,        1,   3,    NO_LIMIT,    NULL},\
  {"RESYNC_CNT",      NO_NEXT_TABLE, IDParamMsg_Status,  USER_TYPE_UINT32,  USER_RO,    (void *) &statusIDParamTemp.cntReSync,          1,   3,    NO_LIMIT,    NULL},\
  {"IDLE_TIMEOUT_CNT",NO_NEXT_TABLE, IDParamMsg_Status,  USER_TYPE_UINT32,  USER_RO,    (void *) &statusIDParamTemp.cntIdleTimeOut,     1,   3,    NO_LIMIT,    NULL},\
  {"FRAME_CNT",       NO_NEXT_TABLE, IDParamMsg_Status,  USER_TYPE_UINT32,  USER_RO,    (void *) &statusIDParamTemp.cntGoodFrames,      1,   3,    NO_LIMIT,    NULL},\
  {"LAST_FRAME_TICK", NO_NEXT_TABLE, IDParamMsg_Status,  USER_TYPE_UINT32,  USER_RO,    (void *) &statusIDParamTemp.lastFrameTime_tick, 1,   3,    NO_LIMIT,    NULL},\
  {"FRAME_0"    ,     idParamStatusFrame0TypeTbl, NULL, NO_HANDLER_DATA},
  {"FRAME_1",         idParamStatusFrame1TypeTbl, NULL, NO_HANDLER_DATA},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL idParamDebugTbl[] =
{ /*Str               Next Tbl Ptr   Handler Func.       Data Type          Access              Parameter                                 IndexRange   DataLimit    EnumTbl*/
  {"ENABLE",          NO_NEXT_TABLE, IDParamMsg_Debug,  USER_TYPE_BOOLEAN, (USER_RW|USER_GSE),  (void *) &debugIDParamTemp.bDebug,         -1,   -1,    NO_LIMIT,    NULL},\
  {"CHANNEL",         NO_NEXT_TABLE, IDParamMsg_Debug,  USER_TYPE_UINT16,  (USER_RW|USER_GSE),  (void *) &debugIDParamTemp.ch,             -1,   -1,    1,     3,    NULL},\
  {"FRAME",           NO_NEXT_TABLE, IDParamMsg_Debug,  USER_TYPE_ENUM,    (USER_RW|USER_GSE),  (void *) &debugIDParamTemp.frameType,      -1,   -1,    NO_LIMIT,    idParamDebugFrameStrs},\
  {"FORMATTED",       NO_NEXT_TABLE, IDParamMsg_Debug,  USER_TYPE_BOOLEAN, (USER_RW|USER_GSE),  (void *) &debugIDParamTemp.bFormatted,     -1,   -1,    NO_LIMIT,    NULL},\
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL idParamProtocolRoot[] =
{ /*Str            Next Tbl Ptr    Handler Func.           Data Type          Access                         Parameter     IndexRange  DataLimit    EnumTbl*/
  {"CFG",          idParamCfgTbl,   NULL,NO_HANDLER_DATA},
  {"STATUS",       idParamStatusTbl,NULL,NO_HANDLER_DATA},
  {"DEBUG",        idParamDebugTbl, NULL,NO_HANDLER_DATA},
  {DISPLAY_CFG,    NO_NEXT_TABLE,   IDParamMsg_ShowConfig, USER_TYPE_ACTION, (USER_RO|USER_NO_LOG|USER_GSE), NULL,         -1,  -1,    NO_LIMIT,    NULL},
  {NULL,           NULL,            NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL idParamProtocolRootTblPtr = {"IDParam",idParamProtocolRoot,NULL,NO_HANDLER_DATA};


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
 * Function:    IDParamMsg_Status
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves the latest ID Param Status Data
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
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
static USER_HANDLER_RESULT IDParamMsg_Status(USER_DATA_TYPE DataType,
                                             USER_MSG_PARAM Param,
                                             UINT32 Index,
                                             const void *SetPtr,
                                             void **GetPtr)
{
  USER_HANDLER_RESULT result ;

  result = USER_RESULT_OK;

  statusIDParamTemp = *IDParamProtocol_GetStatus(Index);

  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);

  return result;
}


/******************************************************************************
 * Function:    IDParamMsg_Cfg
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves and set the latest ID Param Cfg Data
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
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:       none
 *
*****************************************************************************/
static USER_HANDLER_RESULT IDParamMsg_Cfg(USER_DATA_TYPE DataType,
                                          USER_MSG_PARAM Param,
                                          UINT32 Index,
                                          const void *SetPtr,
                                          void **GetPtr)
{
  USER_HANDLER_RESULT result;
  UINT16 nSize;

  result = USER_RESULT_OK;


  nSize = sizeof(cfgIDParamTemp);
  nSize = sizeof(cfgIDParamTemp) - sizeof(cfgIDParamTemp.data);

  // Determine which array element
  memcpy( &cfgIDParamTemp, &CfgMgr_ConfigPtr()->IDParamConfig, nSize );

  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    memcpy( &CfgMgr_ConfigPtr()->IDParamConfig, &cfgIDParamTemp, nSize );

    // Persist the CfgMgr's updated copy back to EEPROM.
    CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),&CfgMgr_ConfigPtr()->IDParamConfig, nSize);
  }
  return result;

}


/******************************************************************************
 * Function:    IDParamDataMsg_Cfg
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves and set the latest ID Param Cfg Data
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
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:       none
 *
*****************************************************************************/
static USER_HANDLER_RESULT IDParamDataMsg_Cfg(USER_DATA_TYPE DataType,
                                              USER_MSG_PARAM Param,
                                              UINT32 Index,
                                              const void *SetPtr,
                                              void **GetPtr)
{
  USER_HANDLER_RESULT result;


  result = USER_RESULT_OK;


  // Determine which array element
  memcpy( &cfgIDParamDataTemp, &CfgMgr_ConfigPtr()->IDParamConfig.data[Index],
          sizeof(cfgIDParamDataTemp) );

  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    memcpy( &CfgMgr_ConfigPtr()->IDParamConfig.data[Index], &cfgIDParamDataTemp,
            sizeof(cfgIDParamDataTemp) );

    // Persist the CfgMgr's updated copy back to EEPROM.
    CfgMgr_StoreConfigItem( CfgMgr_ConfigPtr(),&CfgMgr_ConfigPtr()->IDParamConfig.data[Index],
                            sizeof(cfgIDParamDataTemp) );
  }
  return result;

}


/******************************************************************************
 * Function:    IDParamScrollMsg_Cfg
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves and set the latest ID Param Scroll Cfg Data
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
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:       none
 *
*****************************************************************************/
static USER_HANDLER_RESULT IDParamScrollMsg_Cfg(USER_DATA_TYPE DataType,
                                                USER_MSG_PARAM Param,
                                                UINT32 Index,
                                                const void *SetPtr,
                                                void **GetPtr)
{
  USER_HANDLER_RESULT result;


  result = USER_RESULT_OK;


  // Determine which array element
  memcpy( &cfgIDParamScrollTemp, &CfgMgr_ConfigPtr()->IDParamConfig.scroll,
          sizeof(cfgIDParamScrollTemp) );

  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    memcpy( &CfgMgr_ConfigPtr()->IDParamConfig.scroll, &cfgIDParamScrollTemp,
            sizeof(cfgIDParamScrollTemp) );

    // Persist the CfgMgr's updated copy back to EEPROM.
    CfgMgr_StoreConfigItem( CfgMgr_ConfigPtr(),&CfgMgr_ConfigPtr()->IDParamConfig.scroll,
                            sizeof(cfgIDParamScrollTemp) );
  }
  return result;

}


/******************************************************************************
 * Function:    IDParamIDSMsg_Status
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Output the Element IDS found in the FRAME
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
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:       none
 *
*****************************************************************************/
static USER_HANDLER_RESULT IDParamIDSMsg_Status(USER_DATA_TYPE DataType,
                                                USER_MSG_PARAM Param,
                                                UINT32 Index,
                                                const void *SetPtr,
                                                void **GetPtr)
{
  USER_HANDLER_RESULT result;
  ID_PARAM_FRAME_PTR frameData_ptr;
  ID_PARAM_FRAME_DATA_PTR frameElem_ptr;
  UINT16 i;
  UINT16 cnt;
  UINT16 len;
  UINT16 scrollCnt;
  CHAR buff[ID_PARAM_LC_BUFF_MAX];


  result = USER_RESULT_OK;
  frameData_ptr = (ID_PARAM_FRAME_PTR) &m_IDParam_Frame[Index][Param.Int];
  frameElem_ptr = (ID_PARAM_FRAME_DATA_PTR) &frameData_ptr->data[0];
  cnt = 0;
  statusIDS_Buff[cnt] = NULL;

  for (i=0;i<frameData_ptr->size;i++)
  {
    snprintf ( buff, sizeof(buff), "W%03d=%04d\r\n", i, frameElem_ptr->elementID );
    //strcat ( statusIDS_Buff, buff );
    //SuperStrcat ( statusIDS_Buff, buff, sizeof(statusIDS_Buff) );
    //NOTE: strcat and SuperStrcat take 2x more time than memcpy below
    len = strlen (buff);
    memcpy ( (void *) &statusIDS_Buff[cnt], buff, len );
    cnt += len;
    // NOTE: statusIDS_Buff[] needs to be < 8K
    frameElem_ptr++;
  } // end loop thru frame element Id table

  // Are there scrolling IDS to print out ?
  frameData_ptr = (ID_PARAM_FRAME_PTR) &m_IDParam_ScrollList[Index];
  frameElem_ptr = (ID_PARAM_FRAME_DATA_PTR) &frameData_ptr->data[0];
  scrollCnt = 0;
  for (i=0;i<frameData_ptr->size;i++)
  {
    if (frameElem_ptr->frameType == Param.Int)
    {
      if (scrollCnt==0) { // Add CR/LF between main frame and scroll frame
        statusIDS_Buff[cnt++] = '\r';
        statusIDS_Buff[cnt++] = '\n';
      }
      snprintf (buff, sizeof(buff), "SW%02d=%04d\r\n", scrollCnt++, frameElem_ptr->elementID);
      len = strlen (buff);
      memcpy ( (void *) &statusIDS_Buff[cnt], buff, len );
      cnt += len;
      // NOTE: statusIDS_Buff[] needs to be < 8K
    }
    frameElem_ptr++;
  } // end looop thru scroll list table

  statusIDS_Buff[cnt] = NULL;  // Add null terminator
  User_OutputMsgString (statusIDS_Buff, FALSE);

  return result;
}


/******************************************************************************
* Function:    IDParamMsg_ShowConfig
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
static USER_HANDLER_RESULT IDParamMsg_ShowConfig(USER_DATA_TYPE DataType,
                                                 USER_MSG_PARAM Param,
                                                 UINT32 Index,
                                                 const void *SetPtr,
                                                 void **GetPtr)
{
  INT16 idx;
  CHAR  labelStem[MAX_LABEL_CHAR_ARRAY] = "\r\n\r\nIDPARAM.CFG";
  CHAR  label[USER_MAX_MSG_STR_LEN * 3];
  CHAR  value[MAX_VALUE_CHAR_ARRAY];

  UINT16 numParams;
  USER_HANDLER_RESULT result;

  // Declare a GetPtr to be used locally since
  // the GetPtr passed in is "NULL" because this funct is a user-action.
  UINT32 tempInt;
  void*  localPtr = &tempInt;

  //Top-level name is a single indented space
  CHAR branchName[USER_MAX_MSG_STR_LEN] = " ";
  CHAR nextBranchName[USER_MAX_MSG_STR_LEN] = "  ";

  USER_MSG_TBL* pCfgTable;

  ID_PARAM_DATA_CFG_PTR data_ptr;


  result = USER_RESULT_OK;

  /*
  IDPARAM.CFG is of the form:
  Level 1 IDPARAM
    Level 2   CFG
    Level 3     MAXWORDS
    Level 3     SCROLL
      Level 4       ID
      Level 4       VALUE
    Level 3     P[0]
      Level 4       ID
      Level 4       SCALE
      Level 4       TOL
      Level 4       TIMEOUT_MS
         ...
    Level 3     P[255]
      Level 4       ID
      Level 4       SCALE
      Level 4       TOL
      Level 4       TIMEOUT_MS
  */

  // Display label for IDPARAM.CFG
  User_OutputMsgString( labelStem, FALSE);

  /*** Process the leaf elements in Levels 1->3  ***/
  pCfgTable = idParamCfgTbl;

  idx = 0;
  while (pCfgTable->MsgStr != NULL && result == USER_RESULT_OK)
  {
    if (pCfgTable->pNext == NO_NEXT_TABLE) // Level 3 Items
    {
      snprintf(  label, sizeof(label), "\r\n%s%s =", branchName, pCfgTable->MsgStr);
      // Add additional 15 for spaces of indentation 
      PadString(label, USER_MAX_MSG_STR_LEN + 15);
      result = USER_RESULT_ERROR;
      if (User_OutputMsgString( label, FALSE )) {
        // Call user function for the table entry
        if( USER_RESULT_OK != pCfgTable->MsgHandler(pCfgTable->MsgType, pCfgTable->MsgParam,
                                                  idx, NULL, &localPtr))
        {
          User_OutputMsgString( USER_MSG_INVALID_CMD_FUNC, FALSE);
        }
        else
        {
          // Convert the param to string.
          if( User_CvtGetStr(pCfgTable->MsgType, value, sizeof(value),
                                            localPtr, pCfgTable->MsgEnumTbl))
          {
            result = USER_RESULT_ERROR;
            if (User_OutputMsgString( value, FALSE ))
            {
              result = USER_RESULT_OK;
            }
          }
          else
          {
            //Conversion error
            User_OutputMsgString( USER_MSG_RSP_CONVERSION_ERR, FALSE );
          }
        } // end else ->MsgHandler return OK
      } // end if _OutputMsgString (label) return OK
    } // end if == NO_NEXT_TABLE
    else  // Level 4 items
    {
      // If "P" field, loop thru ALL cfg entries
      if (0 == strncmp(pCfgTable->MsgStr, P_PARAMS_STR, sizeof(pCfgTable->MsgStr)) )
      {
        // Determine # of entries
        data_ptr = (ID_PARAM_DATA_CFG_PTR) &m_IDParam_Cfg.data[0];
        numParams = 0;
        while (data_ptr->id != ID_PARAM_NOT_USED) {
          numParams++;
          data_ptr++;  // Move to next entry
        }

        // Loop thru each "defined" entry
        for (idx=0;idx<numParams;idx++)
        {
          // Display element info above each set of data.
          snprintf(label, sizeof(label),  "\r\n\r\n%s%s[%d]",branchName, pCfgTable->MsgStr,
                   idx);

          // Assume the worse, to terminate this for-loop
          result = USER_RESULT_ERROR;
          if ( User_OutputMsgString( label, FALSE ) )
          {
             result = User_DisplayConfigTree(nextBranchName, pCfgTable->pNext, idx, 0, NULL);
          }
        } // end for loop thru numParams
      } // end if "P" found
      else
      {
        snprintf(  label,sizeof(label), "\r\n%s%s", branchName, pCfgTable->MsgStr);
        PadString(label, USER_MAX_MSG_STR_LEN + 15);
        result = USER_RESULT_ERROR;
        if (User_OutputMsgString( label, FALSE )) {
          result = User_DisplayConfigTree(nextBranchName, pCfgTable->pNext, 0, 0, NULL);
        }
      } // end if 4th level != "P" entry

    } // end else != NO_NEXT_TABLE
    pCfgTable++;
  }

  return result;
}


/******************************************************************************
 * Function:    IDParamMsg_Debug
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves the latest ID Param Debug Data
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
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
static USER_HANDLER_RESULT IDParamMsg_Debug(USER_DATA_TYPE DataType,
                                            USER_MSG_PARAM Param,
                                            UINT32 Index,
                                            const void *SetPtr,
                                            void **GetPtr)
{
  USER_HANDLER_RESULT result ;

  result = USER_RESULT_OK;

  debugIDParamTemp = m_IDParam_Debug;

  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);

  m_IDParam_Debug = debugIDParamTemp;

  return result;
}


/*****************************************************************************
 *  MODIFICATIONS
 *    $History: IDParamUserTables.c $
 * 
 * *****************  Version 9  *****************
 * User: John Omalley Date: 3/19/15    Time: 10:40a
 * Updated in $/software/control processor/code/system
 * SCR 1263 - Code Review Updates
 * 
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 15-03-04   Time: 4:52p
 * Updated in $/software/control processor/code/system
 * Updates from Code Review.  No functional / "real" code changes. 
 * 
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 14-12-04   Time: 4:17p
 * Updated in $/software/control processor/code/system
 * Code Review Updates per SCR #1263
 *
 * *****************  Version 6  *****************
 * User: Peter Lee    Date: 14-11-17   Time: 10:36a
 * Updated in $/software/control processor/code/system
 * SCR #1263 ID Param.  Update IDPARAM.CFG.P[x] to max index of
 * ID_PARAM_CFG_MAX - 1 instead of ID_PARAM_CFG_MAX .
 *
 * *****************  Version 5  *****************
 * User: Peter Lee    Date: 14-10-29   Time: 8:29p
 * Updated in $/software/control processor/code/system
 * SCR #1263 Additional Updates for ID Param for JV.
 *
 * *****************  Version 4  *****************
 * User: Peter Lee    Date: 14-10-27   Time: 9:10p
 * Updated in $/software/control processor/code/system
 * SCR #1263 ID Param Protocol, Design Review Updates.
 *
 * *****************  Version 3  *****************
 * User: Peter Lee    Date: 14-10-22   Time: 1:20p
 * Updated in $/software/control processor/code/system
 * SCR #1263.  Several minor updates to table size and format.
 *
 * *****************  Version 2  *****************
 * User: Peter Lee    Date: 14-10-13   Time: 11:25a
 * Updated in $/software/control processor/code/system
 * SCR #1263 IDParam.  Add debug display func.
 *
 * *****************  Version 1  *****************
 * User: Peter Lee    Date: 14-10-08   Time: 7:11p
 * Created in $/software/control processor/code/system
 * SCR #1263 ID Param Protocol Implementation
 *
 *
 *****************************************************************************/
