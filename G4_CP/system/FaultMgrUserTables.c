/******************************************************************************
         Copyright (C) 2008-2011 Pratt & Whitney Engine Services, Inc.
                      Altair Engine Diagnostic Solutions
              All Rights Reserved. Proprietary and Confidential.

         File:        FaultMgrUserTables.c

         Description:

         VERSION
         $Revision: 27 $  $Date: 12-08-28 8:34a $
******************************************************************************/
#ifndef FAULTMGR_BODY
#error FaultMgrUserTables.c should only be included by FaultMgr.c
#endif


/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
//Enumeration tables for the User Messages
USER_ENUM_TBL Flt_UserEnumVerbosityTbl[] =
{ {"OFF",    DBGOFF},
  {"NORMAL", NORMAL},
  {"VERBOSE",VERBOSE},
  {NULL,0}
};

USER_ENUM_TBL Flt_SysCondOutPin[] =
{
  {"LSS0",     LSS0},
  {"LSS1",     LSS1},
  {"LSS2",     LSS2},
  {"LSS3",     LSS3},
  {"DISABLED", SYS_COND_OUTPUT_DISABLED},
  {NULL,0}
};

USER_ENUM_TBL Flt_AnuncMode[] =
{
   {"NONE",   FLT_ANUNC_NONE},
   {"DIRECT", FLT_ANUNC_DIRECT},
   {"ACTION", FLT_ANUNC_ACTION},
   {NULL,0}
};

static FAULTMGR_CONFIG FaultMgrConfigTemp;
static FLT_DBG_LEVEL   DebugLevelTemp;

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
USER_HANDLER_RESULT Flt_UserMessage(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr);

USER_HANDLER_RESULT Flt_UserMessageRecentAll (USER_DATA_TYPE DataType,
                                              USER_MSG_PARAM Param,
                                              UINT32 Index,
                                              const void *SetPtr,
                                              void **GetPtr);

USER_HANDLER_RESULT Flt_ShowConfig(USER_DATA_TYPE DataType,
                                   USER_MSG_PARAM Param,
                                   UINT32 Index,
                                   const void *SetPtr,
                                   void **GetPtr);


static CHAR *Flt_GetFltBuffEntry ( UINT8 index );

USER_HANDLER_RESULT Flt_UserCfg  (USER_DATA_TYPE DataType,
                                  USER_MSG_PARAM Param,
                                  UINT32 Index,
                                  const void *SetPtr,
                                  void **GetPtr);

USER_HANDLER_RESULT Flt_UserDebugLvl(USER_DATA_TYPE DataType,
                                     USER_MSG_PARAM Param,
                                     UINT32 Index,
                                     const void *SetPtr,
                                     void **GetPtr);


//User manager message tables

static USER_MSG_TBL CfgCmd [] =
{  /*Str                Next Tbl Ptr     Handler Func.    Data Type             Access     Parameter                             IndexRange     DataLimit    EnumTbl*/
  { "VERBOSITY"       , NO_NEXT_TABLE,   Flt_UserCfg,     USER_TYPE_ENUM,       USER_RW,   &FaultMgrConfigTemp.DebugLevel,       -1,-1,         NO_LIMIT,    Flt_UserEnumVerbosityTbl },
  { "ANUNCIATION",      NO_NEXT_TABLE,   Flt_UserCfg,     USER_TYPE_ENUM,       USER_RW,   &FaultMgrConfigTemp.Mode,             -1,-1,         NO_LIMIT,    Flt_AnuncMode },
  { "SYS_COND_OUTPUT" , NO_NEXT_TABLE,   Flt_UserCfg,     USER_TYPE_ENUM,       USER_RW,   &FaultMgrConfigTemp.SysCondDioOutPin, -1,-1,         NO_LIMIT,    Flt_SysCondOutPin},
  { "NORMAL_ACTION",    NO_NEXT_TABLE,   Flt_UserCfg,     USER_TYPE_UINT16,     USER_RW,   &FaultMgrConfigTemp.action[0],        -1,-1,         NO_LIMIT,    NULL },
  { "CAUTION_ACTION",   NO_NEXT_TABLE,   Flt_UserCfg,     USER_TYPE_UINT16,     USER_RW,   &FaultMgrConfigTemp.action[1],        -1,-1,         NO_LIMIT,    NULL },
  { "FAULT_ACTION",     NO_NEXT_TABLE,   Flt_UserCfg,     USER_TYPE_UINT16,     USER_RW,   &FaultMgrConfigTemp.action[2],        -1,-1,         NO_LIMIT,    NULL },
  { NULL              , NULL,            NULL,            NO_HANDLER_DATA }
};

static USER_MSG_TBL FltCmd [] =
{   /*Str         Next Tbl Ptr           Handler Func.             Data Type           Access                          Parameter            IndexRange                DataLimit     EnumTbl*/
  { "RECENT",     NO_NEXT_TABLE,         Flt_UserMessage,          USER_TYPE_STR,      USER_RO,                        NULL,                0, FLT_LOG_BUF_ENTRIES-1,  NO_LIMIT,    NULL },
  { "STATUS",     NO_NEXT_TABLE,         User_GenericAccessor,     USER_TYPE_ENUM,     (USER_RW|USER_GSE),             &FaultSystemStatus,  -1,-1,                     NO_LIMIT,    Flt_UserEnumStatus },
  { "CFG",        CfgCmd,                NULL,                     NO_HANDLER_DATA},
  { "VERBOSITY",  NO_NEXT_TABLE,         User_GenericAccessor,     USER_TYPE_ENUM,     USER_RW,                        &DebugLevel,         -1, -1,                    NO_LIMIT,    Flt_UserEnumVerbosityTbl },
  { DISPLAY_CFG,  NO_NEXT_TABLE,         Flt_ShowConfig,           USER_TYPE_ACTION,   (USER_RO|USER_NO_LOG|USER_GSE), NULL,                -1, -1,                    NO_LIMIT,    NULL },
  { "RECENTALL",  NO_NEXT_TABLE,         Flt_UserMessageRecentAll, USER_TYPE_ACTION,   (USER_RO|USER_NO_LOG),          NULL,                -1, -1,                    NO_LIMIT,    NULL},
  { NULL ,        NULL,                  NULL,                     NO_HANDLER_DATA }
};


static USER_MSG_TBL FaultMgr_RootMsg = {"FAULT", FltCmd, NULL, NO_HANDLER_DATA};

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/
/******************************************************************************
* Function:     Flt_UserMessage
*
* Description:  Handles User Manager requests.
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
USER_HANDLER_RESULT Flt_UserMessage(USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr)
{
  *GetPtr = Flt_GetFltBuffEntry ( (UINT8)Index );
  return USER_RESULT_OK;
}


/******************************************************************************
* Function:     Flt_UserMessageRecentAll
*
* Description:  Handles User Manager requests to retrieve all the current entries
*               in the Flt Buffer.
*
* Parameters:   [in] DataType:  C type of the data to be read or changed, used
*                               for casting the data pointers
*               [in/out] Param: Pointer to the param item to be read
*                               or changed
*               [in] Index:     Index parameter is used to reference the
*                               specific param to change.  Range is validated
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
USER_HANDLER_RESULT Flt_UserMessageRecentAll(USER_DATA_TYPE DataType,
                                             USER_MSG_PARAM Param,
                                             UINT32 Index,
                                             const void *SetPtr,
                                             void **GetPtr)
{
   CHAR  HdrStr[USER_MAX_MSG_STR_LEN * 2];
   static CHAR  ResultBuffer[USER_SINGLE_MSG_MAX_SIZE];
   USER_HANDLER_RESULT result = USER_RESULT_OK;
   CHAR  *pStr;
   UINT8 i;
   UINT32 len, nOffset;

   // Loop thru all entries
   nOffset = 0;
   for (i = 0; i < FLT_LOG_BUF_ENTRIES; i++)
   {
      // Create Header "fault.recent[i]"
      sprintf ( HdrStr, "\r\nFAULT.RECENT[%d]\r\n", i);

      // Append header
      len = strlen ( HdrStr );
      memcpy ( (void *) &ResultBuffer[nOffset], HdrStr, len);
      nOffset += len;

      // Get Entry i data
      pStr = Flt_GetFltBuffEntry( i );

      // Append Entry i data
      len = strlen ( pStr );
      memcpy ( (void *) &ResultBuffer[nOffset], pStr, len );
      nOffset += len;
   }

   ResultBuffer[nOffset] = NULL;  // Terminate String
   User_OutputMsgString(ResultBuffer, FALSE);

   return result;
}


/******************************************************************************
* Function:     Flt_GetFltBuffEntry
*
* Description:  Returns the selected Fault Entry from the non-volatile memory
*
* Parameters:   [in] Index:     Index parameter selecting specified entry
*
* Returns:      [out] ptr to RecentLogStr[] containing decoded entry
*
* Notes:        none
*
*****************************************************************************/
static CHAR *Flt_GetFltBuffEntry ( UINT8 index )
{
  CHAR *pStr;
  INT8 i, UsrIdx;
  TIMESTRUCT Ts;
  static CHAR  RecentLogStr[256];
  static CHAR* FltEmptyStr = "Fault entry empty\r\n";

  // faultIndex points to the oldest fault as shown below
  // |2nd newest|newest|oldest|2nd oldest|...
  //                     ^
  i = faultIndex;

  // The Fault data is returned in chronological order, with the most recent
  // fault at index 1, and the oldest fault at index FLT_LOG_BUF_ENTRIES.
  // Compute the LOG_BUF index, "i" based on the user manager index "Index".
  UsrIdx = index + 1;
  i = (i - UsrIdx) < 0 ? FLT_LOG_BUF_ENTRIES + (i - UsrIdx) : i - UsrIdx;

  if ( 0 != faultHistory[i].count )
  {
    CM_ConvertTimeStamptoTimeStruct( &faultHistory[i].Ts, &Ts);

    // Stringify the Log data
    snprintf( RecentLogStr, 80, "%s(%x) at %02d:%02d:%02d %02d/%02d/%4d\r\n",
        SystemLogIDString( faultHistory[i].Id),
        faultHistory[i].Id,
        Ts.Hour,
        Ts.Minute,
        Ts.Second,
        Ts.Month,
        Ts.Day,
        Ts.Year);

    // Return log data string
    pStr = RecentLogStr;
  }
  else
  {
    pStr = FltEmptyStr;
  }

  return pStr;
}

/******************************************************************************
* Function:    Flt_ShowConfig
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

USER_HANDLER_RESULT Flt_ShowConfig(USER_DATA_TYPE DataType,
                                   USER_MSG_PARAM Param,
                                   UINT32 Index,
                                   const void *SetPtr,
                                   void **GetPtr)
{
  const CHAR LabelStem[] = "\r\n\r\nFAULT";
  USER_HANDLER_RESULT result;
  CHAR Label[USER_MAX_MSG_STR_LEN * 3];

  //Top-level name is a single indented space
  CHAR BranchName[USER_MAX_MSG_STR_LEN] = " ";

  // Set pointer into my CFG table.
  USER_MSG_TBL*  pCfgTbl = CfgCmd;

  result = USER_RESULT_OK;

  // Display element info above each set of data.
  sprintf(Label, "%s", LabelStem);

  result = USER_RESULT_ERROR;
  if (User_OutputMsgString(Label, FALSE ) )
  {
    result = User_DisplayConfigTree(BranchName, pCfgTbl, 0, 0, NULL);
  }
  return result;
}

/******************************************************************************
* Function:    Flt_UserCfg
*
* Description:  Handles commands from the User Manager to configure NV settings
*               for the Fault manager.
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

USER_HANDLER_RESULT Flt_UserCfg(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_OK;

  //Load trigger structure into the temporary location based on index param
  //Param.Ptr points to the struct member to be read/written
  //in the temporary location
  memcpy(&FaultMgrConfigTemp, &CfgMgr_ConfigPtr()->FaultMgrCfg, sizeof(FaultMgrConfigTemp));

  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
  if ((SetPtr != NULL) && (USER_RESULT_OK == result))
  {
    // Copy modified local data back to config mgr structure
    memcpy(&CfgMgr_ConfigPtr()->FaultMgrCfg,
      &FaultMgrConfigTemp, sizeof(FaultMgrConfigTemp));

    //Store the modified structure to the EEPROM.
    CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
      &CfgMgr_ConfigPtr()->FaultMgrCfg, sizeof(FaultMgrConfigTemp));

  }

  return result;
}


/*************************************************************************
*  MODIFICATIONS
*    $History: FaultMgrUserTables.c $
 *
 * *****************  Version 27  *****************
 * User: John Omalley Date: 12-08-28   Time: 8:34a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Added ETM Fault Logic
 *
 * *****************  Version 26  *****************
 * User: John Omalley Date: 12-08-16   Time: 4:15p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Fault Action Processing
 *
 * *****************  Version 25  *****************
 * User: Contractor2  Date: 9/12/11    Time: 1:42p
 * Updated in $/software/control processor/code/system
 * SCR #815 Error: Checksum on response to fault.recentall error
 *
 * *****************  Version 24  *****************
 * User: Contractor2  Date: 4/29/11    Time: 11:11a
 * Updated in $/software/control processor/code/system
 * SCR #815 Error: Checksum on response to fault.recentall error
 *
 * *****************  Version 23  *****************
 * User: Jim Mood     Date: 9/10/10    Time: 2:33p
 * Updated in $/software/control processor/code/system
 * SCR 726.  Setting fault.cfg.verbosity changed real-time verbosity.
 *
 * *****************  Version 22  *****************
 * User: Peter Lee    Date: 8/30/10    Time: 6:55p
 * Updated in $/software/control processor/code/system
 * SCR #806 Code Review Updates
 *
 * *****************  Version 21  *****************
 * User: Peter Lee    Date: 8/04/10    Time: 6:01p
 * Updated in $/software/control processor/code/system
 * SCR #765 Remove "fault.clear".
 *
 * *****************  Version 20  *****************
 * User: Jeff Vahue   Date: 7/29/10    Time: 4:05p
 * Updated in $/software/control processor/code/system
 * SCR# 685 - cleanup
 *
 * *****************  Version 19  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:01a
 * Updated in $/software/control processor/code/system
 * SCR #685 - Add USER_GSE flag
 *
 * *****************  Version 18  *****************
 * User: Contractor V&v Date: 7/07/10    Time: 6:19p
 * Updated in $/software/control processor/code/system
 * SCR #682 Delete function Flt_UserDebugLvl
 *
 * *****************  Version 17  *****************
 * User: Contractor3  Date: 6/25/10    Time: 9:46a
 * Updated in $/software/control processor/code/system
 * SCR #662 - Changes based on code review
 *
 * *****************  Version 16  *****************
 * User: Contractor V&v Date: 6/08/10    Time: 5:54p
 * Updated in $/software/control processor/code/system
 * SCR #615 Showcfg/Long msg enhancement
 *
 * *****************  Version 15  *****************
 * User: Contractor2  Date: 6/07/10    Time: 1:28p
 * Updated in $/software/control processor/code/system
 * SCR #485 Escape Sequence & Box Config
 *
 * *****************  Version 14  *****************
 * User: Contractor V&v Date: 3/29/10    Time: 6:18p
 * Updated in $/software/control processor/code/system
 * SCR #513 Configure fault status  LED
 *
 * *****************  Version 13  *****************
 * User: Contractor V&v Date: 3/19/10    Time: 4:31p
 * Updated in $/software/control processor/code/system
 * SCR #248 Parameter Log Change
 *
 * *****************  Version 12  *****************
 * User: Contractor V&v Date: 3/10/10    Time: 4:41p
 * Updated in $/software/control processor/code/system
 * SCR #248 Parameter Log Change
 *
 * *****************  Version 11  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:57p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 10  *****************
 * User: Jeff Vahue   Date: 2/19/10    Time: 12:58p
 * Updated in $/software/control processor/code/system
 * SCR# 455 - remove LINT issues
 *
 * *****************  Version 9  *****************
 * User: Jeff Vahue   Date: 2/17/10    Time: 1:24p
 * Updated in $/software/control processor/code/system
 * SCR# 452 - code coverage mods
 *
 * *****************  Version 8  *****************
 * User: Contractor V&v Date: 2/03/10    Time: 2:57p
 * Updated in $/software/control processor/code/system
 * SCR 42, SCR 279
 *
 * *****************  Version 7  *****************
 * User: Jeff Vahue   Date: 1/19/10    Time: 3:52p
 * Updated in $/software/control processor/code/system
 * SCR# 350
 *
 * *****************  Version 6  *****************
 * User: Jeff Vahue   Date: 1/13/10    Time: 11:21a
 * Updated in $/software/control processor/code/system
 * SCR# 330
 *
 * *****************  Version 5  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:25p
 * Updated in $/software/control processor/code/system
 * SCR 371
 *
 * *****************  Version 4  *****************
 * User: Peter Lee    Date: 12/18/09   Time: 4:34p
 * Updated in $/software/control processor/code/system
 * SCR #368 Calling Flt_SetStatus() before Flt_Init().  Added additional
 * debug statements.
 *
 * *****************  Version 3  *****************
 * User: Peter Lee    Date: 12/18/09   Time: 9:56a
 * Updated in $/software/control processor/code/system
 * SCR #138 ModFix of logic for re-indexing from [1] to [0] in
 * fault.recent[1].
 *
 * *****************  Version 2  *****************
 * User: Jeff Vahue   Date: 12/17/09   Time: 12:12p
 * Updated in $/software/control processor/code/system
 * SCR# 364 - INT8 -> CHAR type
 *
 * *****************  Version 1  *****************
 * User: Contractor V&v Date: 12/10/09   Time: 5:44p
 * Created in $/software/control processor/code/system
 * SCR 106
*
*
**************************************************************************/
