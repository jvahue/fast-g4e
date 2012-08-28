#define FAULT_USERTABLES_BODY
/******************************************************************************
          Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

File:        FaultUserTables.c

Description: The fault processing object is currenlty used to determine the 
                 validity of a specified sensor based on the criteria of other 
                 sensors. (Sensor consistency check).


VERSION
$Revision: 9 $  $Date: 8/31/10 11:45a $ 

******************************************************************************/
#ifndef FAULT_BODY
#error FaultUserTables.c should only be included by Fault.c
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

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
// Sensor User Configuration Support
USER_HANDLER_RESULT Fault_UserCfg(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr);
USER_HANDLER_RESULT Fault_ShowConfig(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr);
                                     
USER_ENUM_TBL SenseType[]         =  { { "FAULT"     , HAS_A_FAULT     },
                                       { "NOFAULT"   , HAS_NO_FAULT    },
                                       { NULL        , 0               }
                                     }; 

// ~~~~~~~~~ Fault Commands ~~~~~~~~~~~~~~
static USER_MSG_TBL TriggerConfigCmd [] =
{
  { "SENSEOFA", NO_NEXT_TABLE,Fault_UserCfg, USER_TYPE_ENUM,USER_RW,
    &ConfigTemp.faultTrigger[0].SenseOfSignal,0,MAX_FAULTS-1,NO_LIMIT,SenseType },
  { "SENSORINDEXA", NO_NEXT_TABLE,Fault_UserCfg, USER_TYPE_ENUM,USER_RW,
    &ConfigTemp.faultTrigger[0].SensorIndex,0,MAX_FAULTS-1,NO_LIMIT,SensorIndexType },
  { "STARTVALUEA", NO_NEXT_TABLE,Fault_UserCfg, USER_TYPE_FLOAT,USER_RW,
    &ConfigTemp.faultTrigger[0].SensorTrigger.fValue,0,MAX_FAULTS-1,NO_LIMIT,NULL },
  { "STARTCOMPAREA", NO_NEXT_TABLE, Fault_UserCfg, USER_TYPE_ENUM, USER_RW,
    &ConfigTemp.faultTrigger[0].SensorTrigger.Compare,0,MAX_FAULTS-1,NO_LIMIT,ComparisonEnum},
  { "SENSEOFB", NO_NEXT_TABLE,Fault_UserCfg, USER_TYPE_ENUM,USER_RW,
    &ConfigTemp.faultTrigger[1].SenseOfSignal,0,MAX_FAULTS-1,NO_LIMIT,SenseType },
  { "SENSORINDEXB", NO_NEXT_TABLE,Fault_UserCfg, USER_TYPE_ENUM,USER_RW,
    &ConfigTemp.faultTrigger[1].SensorIndex,0,MAX_FAULTS-1,NO_LIMIT,SensorIndexType },
  { "STARTVALUEB", NO_NEXT_TABLE,Fault_UserCfg, USER_TYPE_FLOAT,USER_RW,
    &ConfigTemp.faultTrigger[1].SensorTrigger.fValue,0,MAX_FAULTS-1,NO_LIMIT,NULL },
  { "STARTCOMPAREB", NO_NEXT_TABLE, Fault_UserCfg, USER_TYPE_ENUM, USER_RW,
    &ConfigTemp.faultTrigger[1].SensorTrigger.Compare,0,MAX_FAULTS-1,NO_LIMIT,ComparisonEnum},
  { "SENSEOFC", NO_NEXT_TABLE,Fault_UserCfg, USER_TYPE_ENUM,USER_RW,
    &ConfigTemp.faultTrigger[2].SenseOfSignal,0,MAX_FAULTS-1,NO_LIMIT,SenseType },
  { "SENSORINDEXC", NO_NEXT_TABLE,Fault_UserCfg, USER_TYPE_ENUM,USER_RW,
    &ConfigTemp.faultTrigger[2].SensorIndex,0,MAX_FAULTS-1,NO_LIMIT,SensorIndexType },
  { "STARTVALUEC", NO_NEXT_TABLE,Fault_UserCfg, USER_TYPE_FLOAT,USER_RW,
    &ConfigTemp.faultTrigger[2].SensorTrigger.fValue,0,MAX_FAULTS-1,NO_LIMIT,NULL },
  { "STARTCOMPAREC", NO_NEXT_TABLE, Fault_UserCfg, USER_TYPE_ENUM, USER_RW,
    &ConfigTemp.faultTrigger[2].SensorTrigger.Compare,0,MAX_FAULTS-1,NO_LIMIT,ComparisonEnum},
  { "SENSEOFD", NO_NEXT_TABLE,Fault_UserCfg, USER_TYPE_ENUM,USER_RW,
    &ConfigTemp.faultTrigger[3].SenseOfSignal,0,MAX_FAULTS-1,NO_LIMIT,SenseType },
  { "SENSORINDEXD", NO_NEXT_TABLE,Fault_UserCfg, USER_TYPE_ENUM,USER_RW,
    &ConfigTemp.faultTrigger[3].SensorIndex,0,MAX_FAULTS-1,NO_LIMIT,SensorIndexType },
  { "STARTVALUED", NO_NEXT_TABLE,Fault_UserCfg, USER_TYPE_FLOAT,USER_RW,
    &ConfigTemp.faultTrigger[3].SensorTrigger.fValue,0,MAX_FAULTS-1,NO_LIMIT,NULL },
  { "STARTCOMPARED", NO_NEXT_TABLE, Fault_UserCfg, USER_TYPE_ENUM, USER_RW,
    &ConfigTemp.faultTrigger[3].SensorTrigger.Compare,0,MAX_FAULTS-1,NO_LIMIT,ComparisonEnum},              
  { NULL    , NO_NEXT_TABLE, NULL, NO_HANDLER_DATA }  
};

static USER_MSG_TBL FaultCmd [] =
{
  { "NAME"        , NO_NEXT_TABLE,Fault_UserCfg, USER_TYPE_STR, USER_RW,
    &ConfigTemp.faultName,0,MAX_FAULTS-1,0,MAX_FAULTNAME,NULL },
  { "TRIGGER" , TriggerConfigCmd , NULL, NO_HANDLER_DATA },
  { NULL          , NO_NEXT_TABLE,Fault_UserCfg, USER_TYPE_NONE,USER_RW,
    NULL,0,MAX_FAULTS-1,NO_LIMIT,NULL  } 
};

static USER_MSG_TBL FaultRoot [] =
{/*Str          Next Tbl Ptr   Handler Func.     Data Type          Access                          Parameter      IndexRange        DataLimit  EnumTbl*/

 { "CFG",       FaultCmd      ,NULL              ,NO_HANDLER_DATA},
 { DISPLAY_CFG, NO_NEXT_TABLE ,Fault_ShowConfig  ,USER_TYPE_ACTION, (USER_RO|USER_NO_LOG|USER_GSE) ,NULL           ,-1,-1            ,NO_LIMIT, NULL},
 { NULL,        NULL          ,NULL              ,NO_HANDLER_DATA}
};

USER_MSG_TBL RootFaultMsg = {"SIGNAL",FaultRoot,NULL,NO_HANDLER_DATA};

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:     Fault_UserCfg
 *
 * Description:  Handles User Manager requests to change fault configuration
 *               items.  
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
 * Returns:      
 *
 * Notes:        
 *****************************************************************************/
USER_HANDLER_RESULT Fault_UserCfg(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_OK;

  //Get sensor structure based on index
  memcpy( &ConfigTemp,
          &CfgMgr_ConfigPtr()->FaultConfig[Index],
          sizeof(ConfigTemp));

  result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

  if ( SetPtr != NULL && USER_RESULT_OK == result)
  {
      memcpy( &CfgMgr_ConfigPtr()->FaultConfig[Index], &ConfigTemp, sizeof(FAULT_CONFIG));

      CfgMgr_StoreConfigItem( CfgMgr_ConfigPtr(), 
                              &CfgMgr_ConfigPtr()->FaultConfig[Index],
                              sizeof(ConfigTemp));
  }

  return result;
}

/******************************************************************************
* Function:    Fault_ShowConfig
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
USER_HANDLER_RESULT Fault_ShowConfig(USER_DATA_TYPE DataType,
                                     USER_MSG_PARAM Param,
                                     UINT32 Index,
                                     const void *SetPtr,
                                     void **GetPtr)
{
  CHAR  LabelStem[] = "\r\n\r\nSIGNAL.CFG";
  CHAR  Label[USER_MAX_MSG_STR_LEN * 3];   
  INT16 i;

  USER_HANDLER_RESULT result = USER_RESULT_OK;
  USER_MSG_TBL*  pCfgTable;

  //Top-level name is a single indented space
  CHAR BranchName[USER_MAX_MSG_STR_LEN] = " ";   

  pCfgTable = &FaultCmd[0];  // Get pointer to config entry

  for (i = 0; i < MAX_FAULTS && result == USER_RESULT_OK; ++i)
  {
     // Display element info above each set of data.
     sprintf(Label, "%s[%d]", LabelStem, i);

     result = USER_RESULT_ERROR;

     if ( User_OutputMsgString( Label, FALSE ))
     {
       result = User_DisplayConfigTree(BranchName, pCfgTable, i, 0, NULL);
     }     
   }   
   return result;
}

/*****************************************************************************/
/* Local Functions                                                           */  
/*****************************************************************************/

/*****************************************************************************
*  MODIFICATIONS
*    $History: FaultUserTables.c $
 * 
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 8/31/10    Time: 11:45a
 * Updated in $/software/control processor/code/system
 * SCR #840 Code Review Updates
 * 
 * *****************  Version 7  *****************
 * User: Contractor V&v Date: 6/08/10    Time: 5:54p
 * Updated in $/software/control processor/code/system
 * SCR #615 Showcfg/Long msg enhancement
 * 
 * *****************  Version 6  *****************
 * User: Jeff Vahue   Date: 4/05/10    Time: 10:07a
 * Updated in $/software/control processor/code/system
 * SCR #413 - use generic accessor optimize if/then/else code
 * 
 * *****************  Version 5  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:36p
 * Updated in $/software/control processor/code/system
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 * 
 * *****************  Version 4  *****************
 * User: Contractor V&v Date: 3/19/10    Time: 4:31p
 * Updated in $/software/control processor/code/system
 * SCR #248 Parameter Log Change
 * 
 * *****************  Version 3  *****************
 * User: Jeff Vahue   Date: 3/05/10    Time: 12:17p
 * Updated in $/software/control processor/code/system
 * SCR# 413 - Actions cmds are RO
 * 
 * *****************  Version 2  *****************
 * User: John Omalley Date: 12/16/09   Time: 3:37p
 * Updated in $/software/control processor/code/system
 * SCR 374
 * - Removed the Fault check type and deleted the Fault Update function.
 * 
 * *****************  Version 1  *****************
 * User: Contractor V&v Date: 12/10/09   Time: 5:44p
 * Created in $/software/control processor/code/system
 * SCR 106
*
***************************************************************************/
