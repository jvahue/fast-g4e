#define FASTSTATEMGR_BODY
/******************************************************************************
            Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:         FASTStateMgr.c

    Description:  Programmable state machine that changes states based on data
                  received from a list of inputs for controlling a set of tasks.
                  The definition of the inputs and tasks are defined in
                  FASTStateMgrInterfaces.h

     
   VERSION
   $Revision: 18 $  $Date: 8/29/12 12:50p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include <stdlib.h>
#include <string.h>

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "FASTStateMgr.h"
#include "FASTStateMgrInterfaces.h"
#include "TaskManager.h"
#include "utility.h"
#include "cfgmanager.h"
#include "utility.h"
#include "gse.h"
#include "user.h"
#include "FASTMgr.h"
/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
                                   //If task enums are equal...           && 
//..If numerated the parmams are equal (last part only evaluated if IsNumerated is TRUE)
#define IS_TASKS_EQUAL(A,Ai,B,Bi) ( (A->RunTasks[Ai] == B->RunTasks[Bi])  && \
(!m_Tasks[A->RunTasks[Ai]].IsNumerated || (A->RunTasks[Ai+1] == B->RunTasks[Bi+1])) )


/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
//Data type to hold a binary transition condition expression

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
//Array of Tasks
#undef FSM_TASK_ITEM
#define FSM_TASK_ITEM(Name,IsNumerated,MaxNum,RunFunc,GetStateFunc)\
{#Name,IsNumerated,MaxNum,RunFunc,GetStateFunc},

//Array of Transition Criteria 
#undef FSM_TC_ITEM
#define FSM_TC_ITEM(Name,IsNumerated,GetStateFunc)\
{#Name,IsNumerated,GetStateFunc},
 
//List of all FAST Task definitions
static FSM_TASK  m_Tasks[] = {FSM_TASK_LIST {"",FALSE,NULL,NULL}};
//List of all FAST Transition Criteria definitions
static FSM_TC_INPUT m_TransitionCriteria[] = { FSM_TC_LIST
                                             {"",FALSE,NULL}};

//Index of the current state the FCS is in.
static INT32      m_CurState;
static CHAR       *m_CurStateStr;
//Flag set/cleared by task to indicate the current state's timer has expired
static BOOLEAN    m_IsTimerExpired;

//Runtime copy of configuration data
static FSM_CONFIG m_Cfg;

//Time (in ms) the current state was entered
static UINT32 m_StateStartTime;
static const FSM_STATE m_InitState = FSM_INIT_STATE;


/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/

static INT32 FSM_EvalTCExpr(const FSM_TC* tc_desc, CHAR* input_states);
static INT32 FSM_TCStrToBin(CHAR* str, FSM_TC* bin);
static void  FSM_TCBinToStr( CHAR* str, FSM_TC* bin );
static void  FSM_TaskBinToStr( CHAR* str, FSM_STATE* bin );
static INT32 FSM_TaskStrToBin( CHAR* str, FSM_STATE* bin );
static INT32 FSM_ConvertTCInputStrToBin(CHAR* str, FSM_TC* bin);
static void  FSM_StopThisStateTasks(const FSM_STATE* current,
    const FSM_STATE* next);
static void FSM_StartNextStateTasks(const FSM_STATE* current,
    const FSM_STATE* next);

static void FSM_ChangeState(const FSM_STATE* current, const FSM_STATE* next,
    INT32 tc, CHAR* input_states);

static INT32 FSM_EvaluateTCExprInput(const BYTE* data, BYTE* state);
static void FSM_TaskFunc(void* pParam);


#include "FASTStateMgrUserTables.c"

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:    FSM_Init
 *
 * Description: Fast State Manager initilization.  Initalize global data,
 *              get configuration and register a User Manager command
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:
 *
 *****************************************************************************/
void FSM_Init(void)
{
  TCB TaskInfo;
  USER_MSG_TBL RootMsg = {"FSM", FSMRoot, NULL, NO_HANDLER_DATA};
  static CHAR CRCMismatchStateStr[64];
  UINT16 CalcCRC;  
  FSM_CRC_MISMATCH_LOG log;
    
  m_CurState = 0;
  m_StateStartTime = 0;
  m_IsTimerExpired = FALSE;
  m_CurStateStr = m_Cfg.States[0].Name;
  memcpy(&m_Cfg,&CfgMgr_ConfigPtr()->FSMConfig,sizeof(m_Cfg));

  //Compute CRC
#ifndef WIN32
  CalcCRC = CRC16(&m_Cfg, sizeof(m_Cfg)-sizeof(m_Cfg.CRC));
#else
  // For G4E assume the configured CRC is valid, since the CalcCRC will not contain the
  // actual value except on FAST box.
  CalcCRC = m_Cfg.CRC;
#endif

  if((CalcCRC == m_Cfg.CRC) && m_Cfg.Enabled)
  {
    //Enable Fast State Machine
    memset(&TaskInfo, 0, sizeof(TaskInfo));
    strncpy_safe(TaskInfo.Name, sizeof(TaskInfo.Name),"FSM",_TRUNCATE);
    TaskInfo.TaskID         = FSM_Task;
    TaskInfo.Function       = FSM_TaskFunc;
    TaskInfo.Priority       = taskInfo[FSM_Task].priority;
    TaskInfo.Type           = taskInfo[FSM_Task].taskType;
    TaskInfo.modes          = taskInfo[FSM_Task].modes;
    TaskInfo.MIFrames       = taskInfo[FSM_Task].MIFframes;
    TaskInfo.Rmt.InitialMif = taskInfo[FSM_Task].InitialMif;
    TaskInfo.Rmt.MifRate    = taskInfo[FSM_Task].MIFrate;
    TaskInfo.Enabled        = TRUE;
    TaskInfo.Locked         = FALSE;
    TaskInfo.pParamBlock    = NULL;
    TmTaskCreate (&TaskInfo);
    
    //Disable old FAST Manager.
    FAST_TurnOffFASTControl();
  }
  else if((CalcCRC != m_Cfg.CRC) && m_Cfg.Enabled)
  {
    //CRC mismatch, do not enable
    snprintf(CRCMismatchStateStr,
        sizeof(CRCMismatchStateStr),
        "Err: CRC mismatch Exp: 0x%04X Calc:0x%04X",m_Cfg.CRC,CalcCRC);

    m_CurStateStr = CRCMismatchStateStr;
    
    log.ExpectedCRC = m_Cfg.CRC;
    log.CalculatedCRC = CalcCRC;
    LogWriteSystem(APP_ID_FSM_CRC_MISMATCH, LOG_PRIORITY_LOW, &log,
        sizeof(log), NULL );
  }
  
  User_AddRootCmd(&RootMsg);

    
}



/******************************************************************************
 * Function:      FSM_GetIsTimerExpired  | IMPLEMENTS GetState INTERFACE to 
 *                                       | FAST STATE MGR
 *
 * Description:   Returns the status of the timer flag that is set/cleared
 *                by FSM_TaskFunc.
 *
 * Parameters:    [in] param: Not used, to match call signature only
 *
 * Returns:       .  
 *
 * Notes:         Binary form is assumed parseable, since it was checked when
 *                parsed in from a string
 *
 *****************************************************************************/
BOOLEAN FSM_GetIsTimerExpired(INT32 param)
{

  return m_IsTimerExpired;
  
}



/******************************************************************************
 * Function:      FSM_GetStateFALSE  | IMPLEMENTS GetState INTERFACE to 
 *                                   | FAST STATE MGR
 *
 * Description:   Generic interface function that always returns false.
 *                One use is for tasks that perform a simple in-line function
 *                when they are run and there for never return an active status
 *                when GetState is called. 
 *
 * Parameters:    [in] param: Not used, to match call signature only. 
 *
 * Returns:       FALSE: always returns false  
 *
 * Notes:         Binary form is assumed parseable, since it was checked when
 *                parsed in from a string
 *
 *****************************************************************************/
BOOLEAN FSM_GetStateFALSE(INT32 param)
{
  return FALSE;
}



/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:    FSM_TaskFunc
 *
 * Description: Main periodic task for the state manager.
 *               
 *
 * Parameters:  [] pParam: Not used, only to match TaskManager Task prototype
 *
 * Returns:     None
 *
 * Notes:
 *
 *****************************************************************************/
void FSM_TaskFunc(void* pParam)
{
  FSM_STATE* current = &m_Cfg.States[m_CurState];
  static FSM_STATE* next;
  static INT32      TCIndex;
  static BOOLEAN    bPwrOnInit = TRUE;
  INT32             i;
  static CHAR       input_states[FSM_NUM_OF_INPUTS_PER_TC+1];

  //Run this one time only after power on to "transition" into State0
  if(bPwrOnInit)
  {
    //Stop tasks that are running at power on
    FSM_StopThisStateTasks(&m_InitState,&m_Cfg.States[0]);         
    FSM_StartNextStateTasks(&m_InitState,&m_Cfg.States[0]);
    bPwrOnInit = FALSE;
  }
  
  //Evaluate timer if enabled
  if( current->Timer > 0 )
  {
    m_IsTimerExpired = ((CM_GetTickCount() - m_StateStartTime)/
        MILLISECONDS_PER_SECOND)        >= current->Timer;
  }

  //Evaluate each transition critera that is assigned to this state
  for( i = 0; i < FSM_NUM_OF_TC; i++)
  {
    if((m_Cfg.TCs[i].size != 0) &&  (m_Cfg.TCs[i].CurState == m_CurState))
    {
      memset(input_states,0,sizeof(input_states));
      //Transition criteria evaluates to TRUE, switch states!
      if(FSM_EvalTCExpr(&m_Cfg.TCs[i],input_states) == 1)
      {
        TCIndex = i;
        next = &m_Cfg.States[m_Cfg.TCs[i].NextState];
          
        //Stop current tasks
        FSM_StopThisStateTasks(current,next);
          
        FSM_ChangeState(current,next,i,input_states);
        m_CurState = m_Cfg.TCs[TCIndex].NextState;
        m_CurStateStr = m_Cfg.States[m_CurState].Name;
        break;        
      }
    }    
  } 
}


/******************************************************************************
 * Function:    FSM_ChangeState
 *  
 * Description: Sets the next state as the current state.  This is the
 *              final call after stopping tasks and waiting for them to
 *              stop (if synchronous)
 *
 *              
 *
 * Parameters:  [in] current: Pointer to location for the current state
 *              [in] next: Pointer to location for the next state
 *              [in] tc: Index of the TC that caused the state change
 *              [in] input_states: pointer to string containing the input
 *                                 states to the TC that caused the transition.
 *
 * Returns:    none
 *
 * Notes:
 *
 *****************************************************************************/
void FSM_ChangeState(const FSM_STATE* current, const FSM_STATE* next,
    INT32 tc, CHAR* input_states)
{
  FSM_STATE_CHANGE_LOG log;
  

  //If logged transition, log it.
  strncpy_safe(log.current,sizeof(log.current),
      m_Cfg.States[m_CurState].Name,_TRUNCATE);
  strncpy_safe(log.next,sizeof(log.next),
      m_Cfg.States[m_Cfg.TCs[tc].NextState].Name,_TRUNCATE);
  log.duration_ms = CM_GetTickCount() - m_StateStartTime;
  log.TC = tc;
  memcpy(log.tc_inputs,input_states,sizeof(log.tc_inputs));
  if(m_Cfg.TCs[tc].IsLogged)
  {
    LogWriteSystem( APP_ID_FSM_STATE_CHANGE, LOG_PRIORITY_LOW, &log,
      sizeof(log), NULL );
  }
  GSE_DebugStr(NORMAL,TRUE,"FSM: Changed state to %s because of TC #%d [%s]",
      log.next,log.TC,input_states);

  //Reset timer and flag
  m_IsTimerExpired = FALSE;
  m_StateStartTime = CM_GetTickCount();

  FSM_StartNextStateTasks(current,next);

}


/******************************************************************************
 * Function:    FSM_StopThisStateTasks
 *  
 * Description: Stops tasks running in the current state that are not in the
 *              next state.
 *
 *               For each task defined in the 'current' state
 *                If it is defined in the next state, do nothing
 *                If it is not defined in the next state, stop it
 *              
 *
 * Parameters:  [in] current: Pointer to location for the current state
 *              [in] next: Pointer to location for the next state
 *
 * Returns:    
 *
 * Notes:
 *
 *****************************************************************************/
void FSM_StopThisStateTasks(const FSM_STATE* current, const FSM_STATE* next)
{
  INT32 n,c;
  BOOLEAN bTaskNotInNext;
  INT32   param;
  
  //For every task in the current state.....
  for(c = 0; c < current->Size; c++)
  {
    bTaskNotInNext = TRUE;
    //....check if it exists in the next state.....
    for(n = 0; n < next->Size; n++)
    {
      if(IS_TASKS_EQUAL(next,n,current,c))
      {
        bTaskNotInNext = FALSE;
      }
      //Double-increment n if "IsNumerated" to skip the number param.
      n = m_Tasks[next->RunTasks[n]].IsNumerated ? n+1 : n;
    }
    //.....and if it is not in the next state, stop it.
    if(bTaskNotInNext)
    {
      //Get param value if the task is the "numerated" type,else use zero
      param = m_Tasks[current->RunTasks[c]].IsNumerated ? current->RunTasks[c+1]:0;
      m_Tasks[current->RunTasks[c]].Run(FALSE,param);
    }
    //Double-increment n if "IsNumerated" to skip the number param.
    c = m_Tasks[current->RunTasks[c]].IsNumerated ? c+1 : c;
  }
}



/******************************************************************************
 * Function:    FSM_StartNextStateTasks
 *  
 * Description: Starts all tasks in the next state that are not already defined
 *              in the current state.
 *
 *               For each task defined in the 'next' state
 *                If it is not defined in the 'current' state, start it
 *              
 *
 * Parameters:  [in] current: Pointer to location for the current state
 *              [in] next: Pointer to location for the next state
 *
 * Returns:    
 *
 * Notes:
 *
 *****************************************************************************/
void FSM_StartNextStateTasks(const FSM_STATE* current, const FSM_STATE* next)
{
  INT32 n,c,param;
  BOOLEAN bTaskNotInCurrent;
  
  //for each task in the next state....
  for(n = 0; n < next->Size; n++)
  {
    bTaskNotInCurrent = TRUE;
    //....check if it is running the the current state....
    for(c = 0; c < current->Size; c++)
    {
      if(IS_TASKS_EQUAL(next,n,current,c))
      {
        bTaskNotInCurrent = FALSE;
      }
      //Double-increment c if "IsNumerated" to skip the number param.
      c = m_Tasks[current->RunTasks[c]].IsNumerated ? c+1 : c;
    }
    //.....and if it is not in the current state, start running it.
    if(bTaskNotInCurrent)
    {
      //Get param value if the task is the "numerated" type,else use zero
      param = m_Tasks[next->RunTasks[n]].IsNumerated ? next->RunTasks[n+1]:0;
      m_Tasks[next->RunTasks[n]].Run(TRUE,param);
    }
    //Double-increment c if "IsNumerated" to skip the number param.
    n = m_Tasks[next->RunTasks[n]].IsNumerated ? n+1 : n;
  }

}



/******************************************************************************
 * Function:    FSM_EvalTCExpr
 *
 * Description: Take a binary format boolean expression of transition criteria 
 *              Evaluate that expression based on the current state of all the 
 *              inputs.  
 *
 *              The function evaluates the RPN boolean expression
 *
 *              Error checking is for use during system initlization/
 *              configuration.  If an error is detected at runtime the
 *              caller should assert
 *              
 *
 * Parameters:  [in] tc_desc: A Transition Critera to evaluate, must not be an 
 *                            empty set.
 *              [out] input_states: Optional, will be populated with the
 *                                  input states in the same order as
 *                                  TC expression. TRUE populates with a "1"
 *                                  FALSE populates with an "0"
 *
 * Returns:     0: Expression evaluates to FALSE
 *              1: Expression evaluates to TRUE
 *             -1: Insufficient values for number of operators in expression
 *             -2: Too many values for number of operators in expression
 *
 * Notes:
 *
 *****************************************************************************/
#define RPN_PUSH(num) (rpn_stack[rpn_stack_pos++] = (num))
#define RPN_POP       (rpn_stack[--rpn_stack_pos])
#define RPN_STACK_CNT (rpn_stack_pos)
INT32 FSM_EvalTCExpr(const FSM_TC* tc_desc, CHAR* input_states)
{
  //Okay to skip init, this routine is not called with an empty
  //expressions, so the for() loop will be entered at least once.
  INT16  rpn_stack[FSM_TC_BIN_LEN];
  INT32 rpn_stack_pos = 0;
  INT32 i,input_state_idx = 0;
  BOOLEAN a,b;
  BYTE operand;
    
  for(i = 0; i < tc_desc->size; i++)
  {
    //If input operand
    if(tc_desc->data[i] < FSM_TC_END)
    {
      //Add extra to i if 2 bytes decoded
      i += FSM_EvaluateTCExprInput(&tc_desc->data[i],&operand) - 1; 
      if(input_states != NULL)
      {
        input_states[input_state_idx++] = operand ? '1' : '0';
      }
      RPN_PUSH(operand);
    }
    //Else, operator (!, &, or |)
    else
    {
      //NOT Operator "!"
      if(FSM_OP_NOT == tc_desc->data[i]) 
      {
        if(RPN_STACK_CNT < 1)  
        {
          RPN_PUSH(-1);
          break;
        }
        a = RPN_POP;
        a = !a;
        RPN_PUSH(a);
      }
      //AND or OR operator "&" or "|"
      else
      {
        if(RPN_STACK_CNT < 2) 
        {
          RPN_PUSH(-1);
          break;
        }
        a = RPN_POP;
        b = RPN_POP;
        RPN_PUSH(tc_desc->data[i] == FSM_OP_AND ? a && b : a || b);
      }
    }
  }
  
  //ERROR, too many values in expression
  if(RPN_STACK_CNT != 1)
  {
    RPN_PUSH(-2);
  }
  return RPN_POP;
 
}



/******************************************************************************
 * Function:    FSM_EvaluateTCExprInput
 *
 * Description: Evaluates the an input defined by a binary format TC Expression 
 *              and returns the state of the input into 'state'.  The number
 *              of bytes decoded in the input expression is the return value of
 *              the function.  Currently returns 1 for inputs or 2 for inputs
 *              that have an additional numeration parameter.
 *
 * Parameters:  [in] data: Pointer to location contianing the 1 or 2 byte
 *                         TC Input code
 *              [out] state: TRUE/FALSE State of the TC input
 *
 * Returns:     1: Decoded 1 byte from the TC expression
 *              2: Decoded 2 bytes from the TC expression
 *
 * Notes:
 *
 *****************************************************************************/
INT32 FSM_EvaluateTCExprInput(const BYTE* data, BYTE* state)
{
  INT32 retval = 0;

  //Decode 2-byte input with number parameter, else decode 1-byte input
  //with no number.  Input state must be stricly 1 or 0 to comply with
  //the byte-coded TC
  if(m_TransitionCriteria[data[0]].IsNumerated)
  {
    *state = m_TransitionCriteria[data[0]].GetState(data[1]) ? 1 : 0;
    retval = 2;
  }
  else
  {
    *state = m_TransitionCriteria[data[0]].GetState(0) ? 1 : 0;
    retval = 1;    
  }
  return retval;
}



/******************************************************************************
 * Function:    FSM_TCStrToBin
 *
 * Description: Convert a transition criteria binary expression from string
 *              form (i.e that with 4-char names and !,&,| operators) to a 
 *              binary form used at runtime.
 *
 *
 * Parameters:  [in]  str: String of transition criteria to convert with the
 *                         form of "[4-char TC][Operator][4-char TC]....."
 *              [out] bin: a location to store the converted result
 *
 * Returns:     1 or 0  Parse sucessful
 *              -1 Unrecognized TC input name or name more than 4 chars
 *              -2 Extra operator
 *              -3 Extra TC input
 *
 * Notes:
 *
 *****************************************************************************/
INT32 FSM_TCStrToBin(CHAR* str, FSM_TC* bin)
{
  INT32 j,len;
  INT32 retval = 0;
  INT32 op_lim = 0;
  INT32 input_lim = 0;
  const struct {
                 CHAR* str;
                 FSM_BIN_OP bin;
               } OpTbl[] = {{FSM_TC_OP_AND_CHAR, FSM_OP_AND},
                            {FSM_TC_OP_OR_CHAR , FSM_OP_OR },
                            {FSM_TC_OP_NOT_CHAR, FSM_OP_NOT},
                            {NULL,FSM_OP_END_OF_EXP }};
  
  bin->size = 0;
  //Start looking for operators and inputs
  while((*str != '\0') && (retval == 0))
  {
    //Ignore spaces between operators and inputs.
    if(*str == ' ')
    {
      str++;
      continue;
    }

    //See if string at current position is TC input name
    len = FSM_ConvertTCInputStrToBin(str, bin);
    if(len != 0)
    { 
      str += len;
      retval = input_lim++ < FSM_NUM_OF_INPUTS_PER_TC ? 0 : -3;
    }
    else
    {
      //Search operator characters, if found, convert to bin and
      //store in buffer
      for(j = 0;(OpTbl[j].str != NULL) && (OpTbl[j].str[0] != *str);j++)
      {
        
      }   
      if(OpTbl[j].str != NULL)
      {
        bin->data[bin->size++] = OpTbl[j].bin;
        retval = op_lim++ < (FSM_TC_OPS_PER_TC*FSM_NUM_OF_INPUTS_PER_TC) ? 0 : -2;
        str++;
      }
      else
      {
        //If neither a name or operator is found return an error.
        retval = -1;
      }
    }
  }
  
  //Run through RPN evaluator to check for errors.  
  if((retval != -1 && bin->size != 0))
  {
    retval = FSM_EvalTCExpr(bin,NULL);
  }
  
  return retval;
}


/******************************************************************************
 * Function:    FSM_ConvertTCInputStrToBin
 *
 * Description: Converts a 4 character TC input string and number if 
 *              "IsNumerated" to its respective enumerated binary value.  
 *              Characters beyond 4 are ignored.
 *
 *
 * Parameters:  [in]  str: String of transition criteria to convert with the
 *                         form of "[4-char TC][Operator][4-char TC]....."
 *              [out] bin: a location to store the converted result
 *
 * Returns:     > 0  Number of bytes in str successfully parsed into binary
 *                   form
 *              0    Not recognized TC string, or param invalid/out of range
 *
 * Notes:
 *
 *****************************************************************************/
INT32 FSM_ConvertTCInputStrToBin(CHAR* str, FSM_TC* bin)
{
  INT32 i,num = 0;
  INT32 retval = 0;
  CHAR* ptr;

  //seach for an operator character 4 chars after str*
  //should be: LLLL[!&|] or LLnn[!&|]  
  //Compare string to list of known TC inputs
  for(i = 0; m_TransitionCriteria[i].Name[0] != '\0';i++)
  {
    if(0 == strncmp(m_TransitionCriteria[i].Name,str,4))
    {
      bin->data[bin->size++] = i;
      retval = 4;
      break;
    }    
  }
  //If found (found 4 characters), then check if it is follwed by a number
  if(m_TransitionCriteria[i].IsNumerated && (retval == 4))
  {
    //This task must be followed by a 3-digit number as:
    //LLLLnnn, so +4 to skip over the letters and 7 total chars
    num = strtoul(str+4,&ptr,10);
    if(((ptr-str) == 7) && (num < 255))
    {
      //Number and name was decoded
      bin->data[bin->size++] = num;
      retval = 7;
    }
    else
    {
      retval = 0;
    }
  }
  
  return retval;
}



/******************************************************************************
 * Function:      FSM_TaskStrToBin
 *
 * Description:   Convert a string type list of tasks to execute in a state to
 *                a byte coded list
 *
 * Parameters:    [in/out] Str: String of "," delmited task names to parse
 *                [out]    bin: Location of a state structure to store the
 *                              list of tasks into.
 *
 * Returns:        >0: Parse successful, value is the number of chars decoded
 *                -1: Unrecognized task string in the "str" parameter or
 *                    param value out of range.
 *
 * Notes:
 *  
 *****************************************************************************/
INT32 FSM_TaskStrToBin(CHAR* str, FSM_STATE* bin)
{
  INT32 i,lim = 0;
  INT32 retval = 0;
  CHAR *ptr,*end;
  BOOLEAN TaskFound;
  UINT32 num;
  
  bin->Size = 0;
  str = strtok_r(str,",",&ptr);
  
  
  while( ( str != NULL ) && ( lim++ <= FSM_NUM_OF_TASK ) && 
         ( retval != -1 )                                   )
  {
    TaskFound = FALSE;
    //Compare string to list of known task names
    for(i = 0; m_Tasks[i].Name[0] != '\0' ;i++)
    {
      if(0 == strncmp(m_Tasks[i].Name,str,4))
      {
        //Task found, add to the bin list.
        bin->RunTasks[bin->Size++] = i;
        TaskFound = TRUE;
        retval += 4;
        break;
      }
    }

    //If task is supposed to have a number, decode it
    if(m_Tasks[i].IsNumerated && TaskFound)
    {
      //This task must be followed by a 3-digit number as:
      //LLLLnnn, so +4 to skip over the letters and 7 total chars
      num = strtoul(str+4,&end,10);
      if(((end-str) == 7) && (num < 255))
      {
        //Number and name was decoded
        bin->RunTasks[bin->Size++] = num;
        retval += 3;
      }
      else
      {
        retval = -1;
      }
    }
    //Else if task is not supposed to have a number, 
    //ensure no trailing characters after the 4
    else if(!m_Tasks[i].IsNumerated && TaskFound && (strlen(str) != 4))
    {
      retval = -1;
    }
    
    //Error, could not find task string or max # of tasks exceeded.
    if(!TaskFound || (lim > FSM_NUM_OF_TASK) )
    {
      retval = -1;
    }
        
    str = strtok_r(NULL,",",&ptr) ;
  }
  
  return retval;
}



/******************************************************************************
 * Function:      FSM_TCBinToStr
 *
 * Description:   Convert the binary representation of a transition critera
 *                to a string.  This would be done for reading the configuration
 *                data back to the user through User Manager
 *
 * Parameters:    [out]    str: Location to store the result.  Size must be
 *                              at least FSM_TC_STR_LEN
 *                [in]     bin: Binary transition critera expression
 *
 * Returns:        none.  
 *
 * Notes:         Binary form is assumed parseable, since it was checked when
 *                parsed in from a string
 *
 *****************************************************************************/
void FSM_TCBinToStr( CHAR* str, FSM_TC* bin )
{
  INT32 i;
  CHAR numstr[4];
  CHAR *op_chars[3] = {"!","&","|"};

  //Clear str
  *str = '\0';
  
                      
  for(i = 0; i < bin->size; i++)
  {
    //Test if TC Input or Operator
    if(bin->data[i] < FSM_TC_END)
    {
      SuperStrcat(str, m_TransitionCriteria[bin->data[i]].Name , FSM_TC_STR_LEN);
      if(m_TransitionCriteria[bin->data[i]].IsNumerated)
      {
        //Get param following the TC Input code
        i++;
        snprintf(numstr,sizeof(numstr),"%03d",bin->data[i]);
        SuperStrcat(str,numstr,FSM_TC_STR_LEN);
      }
    }
    else
    {     
      //an operator
      SuperStrcat(str, op_chars[bin->data[i] - FSM_TC_OP_BYTE_CODE_START],
          FSM_TC_STR_LEN);
    }
    
    if( i != (bin->size - 1))
    {
      SuperStrcat(str, " ", FSM_TC_STR_LEN);
    }
  }
}



/******************************************************************************
 * Function:      FSM_TaskBinToStr
 *
 * Description:   Convert the binary representation of a set of tasks to run to
 *                string.  This would be done for reading the configuration
 *                data back to the user through User Manager
 *
 * Parameters:    [out]    str: Location to store the result.  Size must be
 *                              at least FSM_TASK_LIST_STR_LEN
 *                [in]     bin: Binary transition critera expression
 *
 * Returns:       none.  
 *
 * Notes:         Binary form is assumed parseable, since it was checked when
 *                parsed in from a string
 *
 *****************************************************************************/
void FSM_TaskBinToStr( CHAR* str, FSM_STATE* bin )
{
  INT32 i;
  CHAR numstr[4];

  //Clear str
  *str = '\0';
  
  for(i = 0; i < bin->Size; i++)
  {
    SuperStrcat(str, m_Tasks[bin->RunTasks[i]].Name , FSM_TASK_LIST_STR_LEN);
    if(m_Tasks[bin->RunTasks[i]].IsNumerated)
    {
      i++;
      snprintf(numstr,sizeof(numstr),"%03d",bin->RunTasks[i]);
      SuperStrcat(str,numstr, FSM_TASK_LIST_STR_LEN);
    }

    if( i != (bin->Size - 1))
    {
      SuperStrcat(str, ",", FSM_TASK_LIST_STR_LEN );
    }
  }
}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: FASTStateMgr.c $
 * 
 * *****************  Version 18  *****************
 * User: Jeff Vahue   Date: 8/29/12    Time: 12:50p
 * Updated in $/software/control processor/code/application
 * Code Review Tool Findings
 * 
 * *****************  Version 17  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 12:43p
 * Updated in $/software/control processor/code/application
 * SCR# 1142
 * 
 * *****************  Version 16  *****************
 * User: Jim Mood     Date: 2/24/12    Time: 10:32a
 * Updated in $/software/control processor/code/application
 * SCR 1114 - Re labeled after v1.1.1 release
 * 
 * *****************  Version 14  *****************
 * User: Contractor V&v Date: 12/14/11   Time: 6:49p
 * Updated in $/software/control processor/code/application
 * SCR #1105 End of Flight Log Race Condition
 * 
 * *****************  Version 13  *****************
 * User: Jim Mood     Date: 10/27/11   Time: 9:58p
 * Updated in $/software/control processor/code/application
 * SCR #1100 Loop error in StartNextStateTasks
 * 
 * *****************  Version 12  *****************
 * User: Jim Mood     Date: 10/12/11   Time: 6:52p
 * Updated in $/software/control processor/code/application
 * SCR 1084 fsm.cfg.states.run parsing error when comma seperator is
 * missing or there are extra characters in the task name
 * 
 * *****************  Version 11  *****************
 * User: Jim Mood     Date: 10/11/11   Time: 5:31p
 * Updated in $/software/control processor/code/application
 * SCR 1078 Update for Code Review Findings
 * 
 ***************************************************************************/
