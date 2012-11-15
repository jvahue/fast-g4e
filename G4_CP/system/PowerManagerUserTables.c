#define POWERMANAGER_USERTABLES_BODY
/******************************************************************************
            Copyright (C) 2008-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:        PowerManagerUserTables.c
    
    Description: Power Management Status and Control User Commands.
    
    VERSION
      $Revision: 30 $  $Date: 12-11-15 2:48p $
    
******************************************************************************/
#ifndef POWERMANAGER_BODY
#error PowerManagerUserTables.c should only be included by PowerManager.c
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
// See Local Variables below Function Prototypes Section

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
//Prototypes for the User Manager message handlers, has to go before
//the local variable tables that use the function pointer.

USER_HANDLER_RESULT PMMsg_State(USER_DATA_TYPE DataType,
                                USER_MSG_PARAM Param,
                                UINT32 Index,
                                const void *SetPtr,
                                void **GetPtr);
                                
USER_HANDLER_RESULT PMMsg_Cfg(USER_DATA_TYPE DataType,
                              USER_MSG_PARAM Param,
                              UINT32 Index,
                              const void *SetPtr,
                              void **GetPtr);
                                
#ifdef GENERATE_SYS_LOGS
USER_HANDLER_RESULT PMMsg_CreateLogs(USER_DATA_TYPE DataType,
                                     USER_MSG_PARAM Param,
                                     UINT32 Index,
                                     const void *SetPtr,
                                     void **GetPtr); 
#endif

USER_HANDLER_RESULT PMMsg_ShowConfig(USER_DATA_TYPE DataType,
                                     USER_MSG_PARAM Param,
                                     UINT32 Index,
                                     const void *SetPtr,
                                     void **GetPtr);



/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static POWERMANAGER_CFG PowerManager_CfgTemp;

/********************************/
/* Local Str Defintions Tables  */
/********************************/
static USER_ENUM_TBL PowerManagerStateStrs[] =
{
  {"NORMAL",      PM_NORMAL},
  {"GLITCH",      PM_GLITCH},
  {"BATTERY",     PM_BATTERY}, 
  {"SHUTDOWN",    PM_SHUTDOWN},
  {"HALT",        PM_HALT},
  {NULL,          0}
};


/*****************************************/
/* User Table Defintions                 */
/*****************************************/
static USER_MSG_TBL PowerManagerCfgTbl[] = 
{ /*Str                  Next Tbl Ptr    Handler Func. Data Type           Access    Parameter                                        IndexRange  DataLimit    EnumTbl*/
  {"BATTERY",            NO_NEXT_TABLE,  PMMsg_Cfg,    USER_TYPE_BOOLEAN  ,USER_RW  ,(void *) &PowerManager_CfgTemp.Battery         , -1,-1    ,  NO_LIMIT   , NULL},
  {"BATTERYLATCH_TIME_S",NO_NEXT_TABLE,  PMMsg_Cfg,    USER_TYPE_UINT32   ,USER_RW  ,(void *) &PowerManager_CfgTemp.BatteryLatchTime, -1,-1    ,  NO_LIMIT   , NULL},
  {NULL                 ,NULL         ,  NULL     ,    NO_HANDLER_DATA}
};


static USER_MSG_TBL PowerManagerRoot[] = 
{  /*Str                  Next Tbl Ptr    Handler Func.     Data Type           Access                        Parameter                     IndexRange  DataLimit   EnumTbl*/
   {"BUS"                ,NO_NEXT_TABLE      ,PMMsg_State,  USER_TYPE_FLOAT    ,USER_RO                       ,(void *) &Pm.BusVoltage     , -1,-1      ,NO_LIMIT   ,NULL},
   {"BATTERY"            ,NO_NEXT_TABLE      ,PMMsg_State,  USER_TYPE_FLOAT    ,USER_RO                       ,(void *) &Pm.BatteryVoltage , -1,-1      ,NO_LIMIT   ,NULL},
   {"INTERNALTEMP"       ,NO_NEXT_TABLE      ,PMMsg_State,  USER_TYPE_FLOAT    ,USER_RO                       ,(void *) &Pm.InternalTemp   , -1,-1      ,NO_LIMIT   ,NULL},
   {"LITHIUMBATTERY"     ,NO_NEXT_TABLE      ,PMMsg_State,  USER_TYPE_FLOAT    ,USER_RO                       ,(void *) &Pm.LithiumBattery , -1,-1      ,NO_LIMIT   ,NULL},
   {"STATE"              ,NO_NEXT_TABLE      ,PMMsg_State,  USER_TYPE_ENUM     ,USER_RO                       ,(void *) &Pm.State          , -1,-1      ,NO_LIMIT   ,PowerManagerStateStrs},
   {"STATE_TIMER"        ,NO_NEXT_TABLE      ,PMMsg_State,  USER_TYPE_UINT32   ,USER_RO                       ,(void *) &Pm.PmStateTimer   , -1,-1      ,NO_LIMIT   ,NULL},
   {"CFG"                ,PowerManagerCfgTbl ,NULL       ,  NO_HANDLER_DATA},
   {"LATCHBATTERY"       ,NO_NEXT_TABLE      ,PMMsg_State,  USER_TYPE_BOOLEAN  ,USER_RW                       ,(void *) &Pm.bLatchToBattery, -1,-1      ,NO_LIMIT   ,NULL},
#ifdef GENERATE_SYS_LOGS  
   {"CREATELOGS"         ,NO_NEXT_TABLE    ,PMMsg_CreateLogs, USER_TYPE_ACTION ,USER_RO                       ,NULL                        ,-1,-1       ,NO_LIMIT   ,NULL},
#endif
   { DISPLAY_CFG        ,NO_NEXT_TABLE     ,PMMsg_ShowConfig, USER_TYPE_ACTION,(USER_RO|USER_NO_LOG|USER_GSE) ,NULL                        ,-1,-1        ,NO_LIMIT   ,NULL},
   {NULL                ,NULL              ,NULL            , NO_HANDLER_DATA}
}; 

static USER_MSG_TBL PowerManagerTblPtr = {"PM",PowerManagerRoot,NULL,NO_HANDLER_DATA};
 


/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:    PMMsg_State
 *
 * Description: Handles User Manager requests to retrieve the Power Management state.
 *              
 * Parameters:  USER_DATA_TYPE  DataType,
 *              USER_MSG_PARAM  Param,
 *              UINT32          Index,
 *              const void      *SetPtr,
 *              void            **GetPtr
 *
 * Returns:     USER_RESULT_OK:    Processed successfully
 *              USER_RESULT_ERROR: Error processing command. 
 *
 * Notes:
 *
 *****************************************************************************/
USER_HANDLER_RESULT PMMsg_State(USER_DATA_TYPE DataType,
                                USER_MSG_PARAM Param,
                                UINT32 Index,
                                const void *SetPtr,
                                void **GetPtr)
{
   USER_HANDLER_RESULT result;
   RESULT adc_result; 

   result = USER_RESULT_OK;
   
   if(SetPtr == NULL)
   {
      //Set the GetPtr to the Cfg member referenced in the table
      adc_result = DRV_OK; 
      
      // "Pm.Bus"
      if ( (UINT32) Param.Ptr == (UINT32) &Pm.BusVoltage ) 
      {
        adc_result = ADC_GetACBusVoltage (&Pm.BusVoltage); 
      }
      
      // "Pm.Battery"
      if ( (UINT32) Param.Ptr == (UINT32) &Pm.BatteryVoltage) 
      {
        adc_result = ADC_GetACBattVoltage (&Pm.BatteryVoltage);
      }
      
      // "Pm.InternalTemp"
      if ( (UINT32) Param.Ptr == (UINT32) &Pm.InternalTemp)
      {
        adc_result = ADC_GetBoardTemp(&Pm.InternalTemp); 
      }
      
      // "Pm.LithiumBattery"
      if ( (UINT32) Param.Ptr == (UINT32) &Pm.LithiumBattery) 
      {
        adc_result = ADC_GetLiBattVoltage(&Pm.LithiumBattery); 
      }

      *GetPtr = Param.Ptr; 
      
      if (adc_result != DRV_OK ) 
      {
        result = USER_RESULT_ERROR; 
      }
   }
   else
   {
     result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
   }   
   return result;
}


/******************************************************************************
 * Function:    PMMsg_Cfg 
 *
 * Description: Handles User Manager requests to retrieve/set the
 *              Power Management configuration.
 *              
 * Parameters:  [in/out] DataType
 *              [in]     TableData.MsgParam indicates what register to get
 *              [in]     Index
 *              [in]     SetPtr indicates if this is a "set" or "get"
 *              [out]    TableData.GetPtr Writes the register value to the
 *                                        integer pointed to by User.c
 *
 * Returns:     USER_RESULT_OK:    Processed successfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:       None
 *
 *****************************************************************************/
USER_HANDLER_RESULT PMMsg_Cfg(USER_DATA_TYPE DataType,
                              USER_MSG_PARAM Param,
                              UINT32 Index,
                              const void *SetPtr,
                              void **GetPtr)
{
   USER_HANDLER_RESULT result = USER_RESULT_OK;

   memcpy(&PowerManager_CfgTemp, &CfgMgr_ConfigPtr()->PowerManagerCfg, 
          sizeof(POWERMANAGER_CFG));

   result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);
   if(SetPtr != NULL && USER_RESULT_OK == result)
   {
     memcpy(&CfgMgr_ConfigPtr()->PowerManagerCfg,
       &PowerManager_CfgTemp,
       sizeof(PowerManager_CfgTemp));

     CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),
       &CfgMgr_ConfigPtr()->PowerManagerCfg,
       sizeof(PowerManager_CfgTemp));
   }

  PmSetCfg(&PowerManager_CfgTemp);
   
  return result;
}


/******************************************************************************
 * Function:    PMMsg_CreateLogs (pm.createlogs)
 *
 * Description: Create all the internal PM system logs
 *              
 * Parameters:  [none]
 *
 * Returns:     USER_RESULT_OK:    Processed successfully
 *              USER_RESULT_ERROR: Error processing command. 
 *
 * Notes:
 *
 *****************************************************************************/
#ifdef GENERATE_SYS_LOGS

/*vcast_dont_instrument_start*/
USER_HANDLER_RESULT PMMsg_CreateLogs(USER_DATA_TYPE DataType,
                                     USER_MSG_PARAM Param,
                                     UINT32 Index,
                                     const void *SetPtr,
                                     void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_OK; 

  PmCreateAllInternalLogs();
  
  return result;
}
/*vcast_dont_instrument_end*/

#endif

/******************************************************************************
* Function:    PMMsg_ShowConfig
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
USER_HANDLER_RESULT PMMsg_ShowConfig(USER_DATA_TYPE DataType,
                                     USER_MSG_PARAM Param,
                                     UINT32 Index,
                                     const void *SetPtr,
                                     void **GetPtr)
{
   CHAR  LabelStem[] = "\r\n\r\nPM.CFG";
   
   USER_HANDLER_RESULT result = USER_RESULT_OK;
   USER_MSG_TBL*  pCfgTable;

   //Top-level name is a single indented space
   CHAR BranchName[USER_MAX_MSG_STR_LEN] = " ";   

   pCfgTable = &PowerManagerCfgTbl[0];  // Get pointer to config entry

   result = USER_RESULT_ERROR;
   if (User_OutputMsgString( LabelStem, FALSE ) )
   {
      result = User_DisplayConfigTree(BranchName, pCfgTable, 0, 0, NULL);
   }   
   return result;
}


/*************************************************************************
 *  MODIFICATIONS
 *    $History: PowerManagerUserTables.c $
 * 
 * *****************  Version 30  *****************
 * User: Melanie Jutras Date: 12-11-15   Time: 2:48p
 * Updated in $/software/control processor/code/system
 * SCR #1142 File Format Errors
 * 
 * *****************  Version 29  *****************
 * User: John Omalley Date: 12-11-14   Time: 7:20p
 * Updated in $/software/control processor/code/system
 * SCR 1076 - Code Review Updates
 * 
 * *****************  Version 28  *****************
 * User: Melanie Jutras Date: 12-11-13   Time: 2:31p
 * Updated in $/software/control processor/code/system
 * SCR #1142 File Format Errors
 * 
 * *****************  Version 27  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 26  *****************
 * User: Contractor2  Date: 5/05/11    Time: 2:23p
 * Updated in $/software/control processor/code/system
 * SCR #989 Enhancement - pm.state type should be ENUM
 * 
 * *****************  Version 25  *****************
 * User: Contractor2  Date: 10/14/10   Time: 3:26p
 * Updated in $/software/control processor/code/system
 * SCR #934 Code Review: PowerManager
 * 
 * *****************  Version 24  *****************
 * User: Contractor2  Date: 10/14/10   Time: 11:49a
 * Updated in $/software/control processor/code/system
 * SCR #934 Code Review: PowerManager
 * 
 * *****************  Version 23  *****************
 * User: Jeff Vahue   Date: 8/28/10    Time: 12:22p
 * Updated in $/software/control processor/code/system
 * SCR# 830 - GENERATE_SYS_LOGS exclude area from coverage results
 * 
 * *****************  Version 22  *****************
 * User: Contractor3  Date: 7/16/10    Time: 8:57a
 * Updated in $/software/control processor/code/system
 * SCR #702 - Changes based on code review.
 * 
 * *****************  Version 21  *****************
 * User: Contractor V&v Date: 6/08/10    Time: 5:55p
 * Updated in $/software/control processor/code/system
 * SCR #615 Showcfg/Long msg enhancement
 * 
 * *****************  Version 20  *****************
 * User: Contractor2  Date: 5/06/10    Time: 2:10p
 * Updated in $/software/control processor/code/system
 * SCR #579 Change LogWriteSys to return void
 * 
 * *****************  Version 19  *****************
 * User: Contractor V&v Date: 4/22/10    Time: 6:41p
 * Updated in $/software/control processor/code/system
 * SCR #453 Misc fix
 * 
 * *****************  Version 18  *****************
 * User: Jeff Vahue   Date: 4/12/10    Time: 5:43p
 * Updated in $/software/control processor/code/system
 * SCR #541 - all XXX_CreateLogs functions are now dependent on
 * GENERATE_SYS_LOGS
 * 
 * *****************  Version 17  *****************
 * User: Contractor V&v Date: 3/10/10    Time: 4:42p
 * Updated in $/software/control processor/code/system
 * SCR #67 fix spelling error in funct name
 * 
 * *****************  Version 16  *****************
 * User: Jeff Vahue   Date: 3/05/10    Time: 12:16p
 * Updated in $/software/control processor/code/system
 * SCR# 413 - Actions cmds are RO
 * 
 * *****************  Version 15  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 4:02p
 * Updated in $/software/control processor/code/system
 * SCR 106 Remove USE_GENERIC_ACCESSOR macro
 * 
 * *****************  Version 14  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 13  *****************
 * User: Jeff Vahue   Date: 2/17/10    Time: 1:24p
 * Updated in $/software/control processor/code/system
 * SCR# 452 - code coverage mods
 * 
 * *****************  Version 12  *****************
 * User: Contractor V&v Date: 1/13/10    Time: 4:58p
 * Updated in $/software/control processor/code/system
 * 
 * *****************  Version 11  *****************
 * User: Contractor V&v Date: 12/10/09   Time: 5:46p
 * Updated in $/software/control processor/code/system
 * SCR 42
 * 
 * *****************  Version 10  *****************
 * User: Jeff Vahue   Date: 12/04/09   Time: 4:32p
 * Updated in $/software/control processor/code/system
 * SCR# 350
 * 
 * *****************  Version 9  *****************
 * User: Contractor V&v Date: 11/30/09   Time: 5:41p
 * Updated in $/software/control processor/code/system
 * SCR 106  XXXUserTable.c consistency
 * 
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 10/05/09   Time: 5:57p
 * Updated in $/software/control processor/code/system
 * SCR #219 PM Battery Latch User Cmd
 * 
 * *****************  Version 6  *****************
 * User: Peter Lee    Date: 10/01/09   Time: 3:09p
 * Updated in $/software/control processor/code/system
 * SCR #283 Misc Code Updates
 * 
 * *****************  Version 5  *****************
 * User: Jim Mood     Date: 9/29/09    Time: 11:58a
 * Updated in $/software/control processor/code/system
 * SCR# 178 Updated User Manager Tables
 * 
 * *****************  Version 4  *****************
 * User: Peter Lee    Date: 9/28/09    Time: 6:26p
 * Updated in $/software/control processor/code/system
 * scr #277 Power Management Requirements Implemented 
 * 
 * *****************  Version 3  *****************
 * User: Peter Lee    Date: 9/15/09    Time: 2:25p
 * Updated in $/software/control processor/code/system
 * Fix Compiler Warnings on assigning different enum types. 
 * 
 * *****************  Version 2  *****************
 * User: Peter Lee    Date: 6/08/09    Time: 11:49a
 * Updated in $/software/control processor/code/system
 * Integrate NvMgr() to store PM shutdown state.  Complete system log
 * generation.
 * 
 * *****************  Version 1  *****************
 * User: Peter Lee    Date: 10/08/08   Time: 11:45a
 * Created in $/control processor/code/system
 * Initial Check In
 * 
 *
 ***************************************************************************/


