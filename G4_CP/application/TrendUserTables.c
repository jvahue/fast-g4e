/******************************************************************************
         Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
                     Altair Engine Diagnostic Solutions
            All Rights Reserved. Proprietary and Confidential.

  File:        TrendUserTables.c

Description:   User command structures and functions for the trend processing

VERSION
$Revision: 2 $  $Date: 8/08/12 5:40p $
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


USER_ENUM_TBL TendType[]   =
{  { "0"  , TREND_0   }, { "1"  ,  TREND_1   }, {  "2"  , TREND_2   },
   { "3"  , TREND_3   }, { "4"  ,  TREND_4   }, { "UNUSED", TREND_UNUSED }, { NULL, 0}
};


USER_ENUM_TBL TrendRateType[]   =  { { "1HZ"      , TREND_1HZ          },
                                     { "2HZ"      , TREND_2HZ          },
                                     { "4HZ"      , TREND_4HZ          },
                                     { "5HZ"      , TREND_5HZ          },
                                     { "10HZ"     , TREND_10HZ         },
                                     { "20HZ"     , TREND_20HZ         },
                                     { "50HZ"     , TREND_50HZ         },
                                     { NULL       , 0                  }
                                   };



#pragma ghs nowarning 1545 //Suppress packed structure alignment warning

// Trends - TREND User and Configuration Table
static USER_MSG_TBL TrendCmd [] =
{
  /* Str              Next Tbl Ptr               Handler Func.          Data Type          Access     Parameter                           IndexRange           DataLimit            EnumTbl*/
  { "NAME",           NO_NEXT_TABLE,            Trend_UserCfg,          USER_TYPE_STR,     USER_RW,   &ConfigTrendTemp.TrendName,         0,(MAX_TRENDS-1),    0,MAX_TREND_NAME,    NULL                },
  { NULL,             NULL,                     NULL,                   NO_HANDLER_DATA }
};


static USER_MSG_TBL TrendStatus [] =
{
  /* Str             Next Tbl Ptr       Handler Func.    Data Type          Access     Parameter                           IndexRange           DataLimit   EnumTbl*/
   { NULL          , NULL,              NULL ,           NO_HANDLER_DATA }
};

static USER_MSG_TBL TrendRoot [] =
{ /* Str            Next Tbl Ptr       Handler Func.        Data Type          Access            Parameter      IndexRange   DataLimit  EnumTbl*/
   { "CFG",         TrendCmd     ,     NULL,                NO_HANDLER_DATA},
   { "STATUS",      TrendStatus  ,     NULL,                NO_HANDLER_DATA},
   { DISPLAY_CFG,   NO_NEXT_TABLE,     Trend_ShowConfig,    USER_TYPE_ACTION,  USER_RO|USER_GSE, NULL,          -1, -1,      NO_LIMIT,  NULL},
   { NULL,          NULL,              NULL,                NO_HANDLER_DATA}
};

static
USER_MSG_TBL  RootTrendMsg = {"TREND",TrendRoot,NULL,NO_HANDLER_DATA};

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
*****************************************************************************/
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
*****************************************************************************/

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
*****************************************************************************/
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
 **************************************************************************/


