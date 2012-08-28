#ifndef FASTSTATEMGR_H
#define FASTSTATEMGR_H

/******************************************************************************
            Copyright (C) 2007-2011 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:         FASTStateMgr.h

    Description:  

    VERSION
    $Revision: 7 $  $Date: 8/28/12 12:43p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
//Includes for Task or Transition Critera Interfaces
#include "FASTStateMgrInterfaces.h"

/******************************************************************************
                                 Package Defines                              *
******************************************************************************/
/*
 The defines below describe the sizes needed to store configuraion of each
 state.
*/
//Maximum number of states that can be configured
#define FSM_NUM_OF_STATES 16
//Maximum number of tasks that can be configured for each state.
#define FSM_NUM_OF_TASK 8
//Each state may define X number of inputs to each transition criteria
#define FSM_NUM_OF_INPUTS_PER_TC 8
//Maximum number of transition critera that can be configured
#define FSM_NUM_OF_TC 48
//Each transition criteria is defined in the configuration with an X char
//string
#define FSM_TC_INPUT_STR_LEN    8
#define FSM_TASK_STR_LEN        8
#define FSM_STATE_NAME_STR_LEN  16
//Each transition critera must also accommodate X operators per input.
//  Defined as 2, each criteria can have 1 unary ! and 2 criteria have one binary & or |
//  so 2 actual defines one extra operator per input.
#define FSM_TC_OPS_PER_TC 2
#define FSM_TC_OP_AND_CHAR "&"
#define FSM_TC_OP_OR_CHAR  "|"
#define FSM_TC_OP_NOT_CHAR "!"
#define FSM_TC_OP_BYTE_CODE_START 100
//The maximum length of the transition critera in string form
#define FSM_TC_STR_LEN ((FSM_TC_INPUT_STR_LEN+FSM_TC_OPS_PER_TC)*FSM_NUM_OF_INPUTS_PER_TC)

//The maximum length of the transition critera in string form (NAME+NUMBERS+COMMA CHAR)
#define FSM_TASK_LIST_STR_LEN (((FSM_TASK_STR_LEN)*FSM_NUM_OF_TASK)+1)

//The maximum length of the transition critera in binary form, where this form is
//[Unary Operation][Input][optional number][Binary Operation]....*total number of inputs
//For each input Reserve: 1 byte to define the input
//                        1 byte for optional number
//                        X bytes for the operation
#define FSM_TC_BIN_LEN ((1+1+FSM_TC_OPS_PER_TC)*FSM_NUM_OF_INPUTS_PER_TC)

//1 Byte for each task and 1 byte for each task's parameter.
#define FSM_TASK_BIN_LEN (FSM_NUM_OF_TASK*2)

//Power-on state (the state to transition into state 0 from at init time)
//Define tasks that need to be shut off at init time here.
#define FSM_NUM_TASKS_TO_SHUT_OFF 1
#define FSM_TASKS_TO_SHUT_OFF     FSM_TASK_ACFG
#define FSM_INIT_STATE {"_INIT",FSM_NUM_TASKS_TO_SHUT_OFF,FSM_TASKS_TO_SHUT_OFF,TRUE,0}

/******************************************************************************
                                 Package Typedefs                             *
******************************************************************************/
typedef void    TASK_RUN(BOOLEAN run,INT32 param);
typedef BOOLEAN GET_STATE(INT32 param);

//--Information for the Transition Criteria and Tasks defined in the system 
//  (see StateManagerInterfaces.h)
typedef struct{
  CHAR       Name[FSM_TC_INPUT_STR_LEN +1];//4-char name of the Transition Criteria Input
  BOOLEAN    IsNumerated;  //Indicates part of the 4-char Name string is reserved 
                           //numerating multiple instances of this TC
  GET_STATE* GetState;     //function that returns the active/inactive status
}FSM_TC_INPUT;

typedef struct{
  CHAR       Name[FSM_TASK_STR_LEN +1]; //4 (or 7-char) name of the Task
  BOOLEAN    IsNumerated;  //Indicates 3 extra characters in the string name are reserved 
                           //to for enumerating multiple instances of this task.
  INT32      MaxNum;       //Maximum number of instances for the task.
  TASK_RUN*  Run;          //Function that signals this task to run or stop
  GET_STATE* GetState;     //Function that returns the active/inactive status
}FSM_TASK;

//--Define enum lists for the Tasks and Transition Critera defined in
//  StateManagerInterfaces.h
#undef FSM_TC_ITEM     //Set the macro to populate the tc structure list
#define FSM_TC_ITEM(Name,IsNumerated,GetStateFunc) FSM_TC_##Name,

#undef FSM_TASK_ITEM   //Set the macro to populate the tc structure list
#define FSM_TASK_ITEM(Name,IsNumerated,MaxNum,RunFunc,GetStateFunc) FSM_TASK_##Name,

//FSM_TCS and FSM_BIN_OP form the Byte-Code for TC expressions
typedef enum {
  FSM_TC_LIST
  FSM_TC_END
}FSM_TCS;

//Keep in this order (see TCBinToStr)
typedef enum{
  FSM_OP_NOT = FSM_TC_OP_BYTE_CODE_START ,
  FSM_OP_AND,
  FSM_OP_OR,
  FSM_OP_END_OF_EXP = 0xFF,
}FSM_BIN_OP;

//FSM_TASKS form the Byte-Code for tasks to run
typedef enum {
  FSM_TASK_LIST
  FSM_TASK_END
}FSM_TASKS;



//--Configuration structures
#pragma pack(1)
typedef struct{
  CHAR      Name[FSM_STATE_NAME_STR_LEN];
  BYTE      Size;
  BYTE      RunTasks[FSM_TASK_BIN_LEN];
  UINT32    Timer;
}FSM_STATE;

typedef struct{
  BYTE      CurState;           
  BYTE      NextState;
  BYTE      IsLogged;
  BYTE      size;
  BYTE      data[FSM_TC_BIN_LEN];
}FSM_TC;

typedef struct{
  FSM_STATE States[FSM_NUM_OF_STATES];
  FSM_TC    TCs[FSM_NUM_OF_TC];
  BOOLEAN   Enabled;
  UINT16    CRC;  
}FSM_CONFIG;

//Task state changed log.
typedef struct{
  CHAR      current[FSM_STATE_NAME_STR_LEN+1];  //string for the current state
  CHAR      next[FSM_STATE_NAME_STR_LEN+1];     //string for the next state
  UINT32    duration_ms;                        //duration current ran for
  BYTE      TC;                                 //Index of the TC that caused the switch
  CHAR      tc_inputs[FSM_NUM_OF_INPUTS_PER_TC]; //String indicating the state of the TC inputs when it evaluated to TRUE.
}FSM_STATE_CHANGE_LOG;

//CRC Mismatch log
typedef struct{
  UINT16 ExpectedCRC;                     //The expected CRC value in the configuration.
  UINT16 CalculatedCRC;                   //The CRC calculated by the FSM on the actual configuration data. 
}FSM_CRC_MISMATCH_LOG;

#pragma pack()

#define FSM_DEFAULT_CFG 0

/******************************************************************************
                                 Package Exports                              *
******************************************************************************/
#undef EXPORT

#if defined ( FASTSTATEMGR_BODY )
   #define EXPORT
#else
   #define EXPORT extern
#endif

/******************************************************************************
                             Package Exports Variables
******************************************************************************/

/******************************************************************************
                             Package Exports Functions
******************************************************************************/
EXPORT void FSM_Init(void);
EXPORT BOOLEAN FSM_GetIsTimerExpired(INT32 param);
EXPORT BOOLEAN FSM_GetStateFALSE(INT32 param);


#endif // FASTSTATEMGR_H
/*************************************************************************
 *  MODIFICATIONS
 *    $History: FASTStateMgr.h $
 * 
 * *****************  Version 7  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 12:43p
 * Updated in $/software/control processor/code/application
 * SCR# 1142
 * 
 * *****************  Version 6  *****************
 * User: Jim Mood     Date: 9/23/11    Time: 8:02p
 * Updated in $/software/control processor/code/application
 * SCR 575 Modfix for the list of running task not being reliable in the
 * fsm.status user command.
 * 
 * *****************  Version 5  *****************
 * User: Jim Mood     Date: 8/25/11    Time: 6:07p
 * Updated in $/software/control processor/code/application
 * SCR 575 Modfix
 * 
 * *****************  Version 4  *****************
 * User: Jim Mood     Date: 8/22/11    Time: 6:24p
 * Updated in $/software/control processor/code/application
 * SCR 575 Modfixes
 * 
 * *****************  Version 3  *****************
 * User: Contractor V&v Date: 8/22/11    Time: 12:35p
 * Updated in $/software/control processor/code/application
 * SCR #575 Add Showcfg to FastStateMgr
 * 
 * *****************  Version 2  *****************
 * User: Jim Mood     Date: 7/26/11    Time: 3:05p
 * Updated in $/software/control processor/code/application
 * SCR 575 updates
 * 
 * *****************  Version 1  *****************
 * User: Jim Mood     Date: 7/20/11    Time: 10:03a
 * Created in $/software/control processor/code/application
 ***************************************************************************/
