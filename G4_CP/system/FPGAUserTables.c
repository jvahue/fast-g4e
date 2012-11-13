#define FPGA_USERTABLES_BODY
/******************************************************************************
            Copyright (C) 2009-2010 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:   FPGAUserTables.c
    
    Description: FPGA User command definitions
    
 VERSION
     $Revision: 20 $  $Date: 12-11-13 2:01p $

******************************************************************************/
#ifndef FPGA_MANAGER_BODY
#error FPGAUserTables.c should only be included by FPGAManager.c
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
/* Local Function Prototypes                                                 */
/*****************************************************************************/
USER_HANDLER_RESULT FPGAMsg_Status(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr); 
                                        
USER_HANDLER_RESULT FPGAMsg_Register(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr);
                                        
#ifdef GENERATE_SYS_LOGS
USER_HANDLER_RESULT FPGAMsg_CreateLogs(USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr); 
#endif                                        
                                        
/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
typedef enum {
 FPGA_REG_ISR, 
 FPGA_REG_IMR1, 
 FPGA_REG_IMR2, 
 FPGA_REG_VER, 
 FPGA_REG_WD_TIME, 
 FPGA_REG_WD_TIME_INV
} FPGA_REG_ENUM;


/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
FPGA_STATUS FPGA_StatusTemp;
UINT16 FpgaRegData; 

USER_ENUM_TBL FPGASystemStrs[] =
{
  {"OK",FPGA_STATUS_OK},
  {"FAULTED",FPGA_STATUS_FAULTED},
  {"FORCERELOAD", FPGA_STATUS_FORCERELOAD},
  {NULL,0}
};

USER_MSG_TBL FpgaRegisterTbl[] =
{
  {"ISR",NO_NEXT_TABLE,FPGAMsg_Register,
  USER_TYPE_HEX16,USER_RO,(void*)FPGA_REG_ISR,-1,-1,NO_LIMIT,NULL},
  {"IMR1",NO_NEXT_TABLE,FPGAMsg_Register,
  USER_TYPE_HEX16,USER_RO,(void*)FPGA_REG_IMR1,-1,-1,NO_LIMIT,NULL},
  {"IMR2",NO_NEXT_TABLE,FPGAMsg_Register,
  USER_TYPE_HEX16,USER_RO,(void*)FPGA_REG_IMR2,-1,-1,NO_LIMIT,NULL},
  {"VER",NO_NEXT_TABLE,FPGAMsg_Register,
  USER_TYPE_HEX16,USER_RO,(void*)FPGA_REG_VER,-1,-1,NO_LIMIT,NULL},
  {"WD_TIME",NO_NEXT_TABLE,FPGAMsg_Register,
  USER_TYPE_HEX16,USER_RO,(void*)FPGA_REG_WD_TIME,-1,-1,NO_LIMIT,NULL},
  {"WD_TIME_INV",NO_NEXT_TABLE,FPGAMsg_Register,
  USER_TYPE_HEX16,USER_RO,(void*)FPGA_REG_WD_TIME_INV,-1,-1,NO_LIMIT,NULL},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL FpgaStatusTbl [] =
{
  {"CRC_ERROR_CNT", NO_NEXT_TABLE,FPGAMsg_Status,
  USER_TYPE_UINT32,USER_RO,&FPGA_StatusTemp.CRCErrCnt,-1,-1,NO_LIMIT,NULL},
  
  {"REG_CHECK_FAIL_CNT", NO_NEXT_TABLE,FPGAMsg_Status,
  USER_TYPE_UINT32,USER_RO,&FPGA_StatusTemp.RegCheckFailCnt,-1,-1,NO_LIMIT,NULL},
  
  {"STATE", NO_NEXT_TABLE,FPGAMsg_Status,
  USER_TYPE_ENUM,USER_RO,&FPGA_StatusTemp.SystemStatus,-1,-1,NO_LIMIT,FPGASystemStrs},

  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL FpgaRoot[] =
{
  {"REGISTER",FpgaRegisterTbl,NULL,NO_HANDLER_DATA},
  {"STATUS",FpgaStatusTbl,NULL,NO_HANDLER_DATA},
  //{"CFG",QarCfgTbl,NULL,NO_HANDLER_DATA},
  //{"RECONFIGURE",NO_NEXT_TABLE,QARMsg_Reconfigure,
  //USER_TYPE_ACTION,USER_RW,NULL,-1,-1,NULL},
#ifdef GENERATE_SYS_LOGS  
    {"CREATELOGS",NO_NEXT_TABLE,FPGAMsg_CreateLogs,
    USER_TYPE_ACTION,USER_RO,NULL,-1,-1,NO_LIMIT,NULL},
#endif  
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static USER_MSG_TBL FpgaRootTblPtr = {"FPGA",FpgaRoot,NULL,NO_HANDLER_DATA};


/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/
/******************************************************************************
 * Function:    FPGAMsg_Status 
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retreives the latest state values from the FPGA driver and
 *              returns the specific state value to the user module
 *              
 * Parameters:                DataType
 *                       [in] TableData.MsgParam indicates what state member
 *                                               to get
 *                            Index,
 *                            SetPtr,
 *                       [out]TableData.GetPtr Writes the state GetPtr to 
 *                                             reference the State member
 *                                             requested
 *
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *
 * Notes:
 *
 *****************************************************************************/
USER_HANDLER_RESULT FPGAMsg_Status(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr)
{
  FPGA_StatusTemp = *FPGA_GetStatus(); 
  
  //Setting or getting this value?  (if the SetPtr is null, assume this is
  //to "get" the cfg value)
  if(SetPtr == NULL)
  {
   //Set the GetPtr to the Cfg member referenced in the table
    *GetPtr = Param.Ptr;
  }
  /*  
  else
  {
    // Read only variable ! 
  }
  */
  
  return USER_RESULT_OK;  
  
}


/******************************************************************************
 * Function:    FPGAMsg_Register (qar.register)
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              
 * Parameters:  
 *                            DataType
 *                       [in] Param indicates what register to get
 *                       [in] Index. Used for Barker codes array, range is 0-3
 *                            SetPtr
 *                       [out]GetPtr Writes the register value to the
 *                                             integer pointed to by User.c
 *
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *
 * Notes:
 *
 *****************************************************************************/
USER_HANDLER_RESULT FPGAMsg_Register(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr)
{

  switch(Param.Int)
  {
    case FPGA_REG_ISR:
      FpgaRegData = FPGA_R(FPGA_ISR);
      break; 
  
    case FPGA_REG_IMR1:
      FpgaRegData = FPGA_R(FPGA_IMR1);
      break; 
  
    case FPGA_REG_IMR2:
      FpgaRegData = FPGA_R(FPGA_IMR2);
      break; 
  
    case FPGA_REG_VER:
      FpgaRegData = FPGA_R(FPGA_VER);
      break;
  
    case FPGA_REG_WD_TIME: 
      FpgaRegData = FPGA_R(FPGA_WD_TIMER); 
      break;
      
    case FPGA_REG_WD_TIME_INV: 
      FpgaRegData = FPGA_R(FPGA_WD_TIMER_INV); 
      break; 
    
    default: 
      FATAL ("Unexpected FPGA_REG = % d", Param.Int);
      break; 
  }
  
  *GetPtr = (UINT16 *) &FpgaRegData; 
  
  return USER_RESULT_OK;
}

/******************************************************************************
 * Function:    FPGAMsg_CreateLogs (fpga.createlogs)
 *
 * Description: Create all the internal FPGA system logs
 *              
 * Parameters:  DataType
 *              Param
 *              Index
 *              *SetPtr
 *              **GetPtr
 *
 * Returns:     USER_RESULT_OK:    Processed successfully
 *              USER_RESULT_ERROR: Error processing command. 
 *
 * Notes:
 *
 *****************************************************************************/
#ifdef GENERATE_SYS_LOGS
/*vcast_dont_instrument_start*/
USER_HANDLER_RESULT FPGAMsg_CreateLogs(USER_DATA_TYPE DataType,
                                       USER_MSG_PARAM Param,
                                       UINT32 Index,
                                       const void *SetPtr,
                                       void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_OK; 

  FPGA_CreateAllInternalLogs();
  
  return result;
}
/*vcast_dont_instrument_end*/

#endif


/*****************************************************************************
 *  MODIFICATIONS
 *    $History: FPGAUserTables.c $
 * 
 * *****************  Version 20  *****************
 * User: Melanie Jutras Date: 12-11-13   Time: 2:01p
 * Updated in $/software/control processor/code/system
 * SCR #1142 File Format Error
 * 
 * *****************  Version 19  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 18  *****************
 * User: John Omalley Date: 9/10/10    Time: 10:01a
 * Updated in $/software/control processor/code/system
 * SCR 858 - Added FATAL to default cases
 * 
 * *****************  Version 17  *****************
 * User: Peter Lee    Date: 8/31/10    Time: 1:40p
 * Updated in $/software/control processor/code/system
 * SCR #840 Code Review Update
 * 
 * *****************  Version 16  *****************
 * User: Jeff Vahue   Date: 8/28/10    Time: 12:22p
 * Updated in $/software/control processor/code/system
 * SCR# 830 - GENERATE_SYS_LOGS exclude area from coverage results
 * 
 * *****************  Version 15  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:10a
 * Updated in $/software/control processor/code/system
 * SCR #698 - Fix code review findings
 * 
 * *****************  Version 14  *****************
 * User: Contractor3  Date: 6/25/10    Time: 9:46a
 * Updated in $/software/control processor/code/system
 * SCR #662 - Changes based on code review
 * 
 * *****************  Version 13  *****************
 * User: Contractor2  Date: 5/06/10    Time: 2:10p
 * Updated in $/software/control processor/code/system
 * SCR #579 Change LogWriteSys to return void
 * 
 * *****************  Version 12  *****************
 * User: Jeff Vahue   Date: 4/12/10    Time: 5:43p
 * Updated in $/software/control processor/code/system
 * SCR #541 - all XXX_CreateLogs functions are now dependent on
 * GENERATE_SYS_LOGS
 * 
 * *****************  Version 11  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 10  *****************
 * User: Jeff Vahue   Date: 2/17/10    Time: 1:41p
 * Updated in $/software/control processor/code/system
 * Add Version Info to the file
 * 
 * *****************  Version 9  *****************
 * User: Jeff Vahue   Date: 2/17/10    Time: 1:24p
 * Updated in $/software/control processor/code/system
 * SCR# 452 - code coverage mods
 * 
 * *****************  Version 8  *****************
 * User: Contractor V&v Date: 12/10/09   Time: 5:46p
 * Updated in $/software/control processor/code/system
 * SCR 106
 * 
 * *****************  Version 7  *****************
 * User: Jeff Vahue   Date: 12/04/09   Time: 4:32p
 * Updated in $/software/control processor/code/system
 * SCR# 350
 * 
 * *****************  Version 6  *****************
 * User: Contractor V&v Date: 11/30/09   Time: 5:37p
 * Updated in $/software/control processor/code/system
 * SCR 10 XXXUserTable.c consistency
 * 
 * *****************  Version 5  *****************
 * User: Peter Lee    Date: 11/23/09   Time: 2:04p
 * Updated in $/software/control processor/code/system
 * Updates to support WIN32 version
 * 
 * *****************  Version 4  *****************
 * User: Peter Lee    Date: 10/15/09   Time: 9:58a
 * Updated in $/software/control processor/code/system
 * SCR 115, 116 and FPGA CBIT requirements
 * 
 * *****************  Version 3  *****************
 * User: Jim Mood     Date: 9/29/09    Time: 11:58a
 * Updated in $/software/control processor/code/system
 * SCR# 178 Updated User Manager Tables
 * 
 * *****************  Version 2  *****************
 * User: Peter Lee    Date: 1/12/09    Time: 11:24a
 * Updated in $/control processor/code/system
 * Updated FPGA Register Defines (in FPGA.h) to be constants rather than
 * "Get Content of Address" (prefix by "*") to simply code references in
 * Arinc429, QAR and FPGA.
 * 
 * *****************  Version 1  *****************
 * User: Peter Lee    Date: 1/06/09    Time: 2:43p
 * Created in $/control processor/code/system
 * SCR 129 Misc FPGA Updates
 * 
 *
 *****************************************************************************/
                                            
