#define EVALUATOR_BODY
/******************************************************************************
              Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
                 All Rights Reserved. Proprietary and Confidential.

    File:        evaluator.c

    Description: Expression Evaluator service module.
                 This module provides a generic framework for evaluating
                 the start and end conditions as defined by an expression
                 structure

     Notes:

  VERSION
  $Revision: 22 $  $Date: 11/13/12 6:27p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "GSE.h"
#include "Evaluator.h"
#include "EvaluatorInterface.h"
#include "assert.h"
#include "Utility.h"

#include "trigger.h"
#include "sensor.h"
#include "ActionManager.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define MAX_FP_STRING         32
#define MAX_PRIOR_VALUES      50


#define RPN_PUSH(obj)  (rpn_stack[rpn_stack_pos++] = (obj))
#define RPN_POP        (rpn_stack[--rpn_stack_pos])
#define RPN_STACK_CNT  (rpn_stack_pos)
#define RPN_STACK_PURGE rpn_stack_pos = 0;


// Macro for building GUID table key for PRIOR_SENSOR_ENTRY
#define KEY_MASK_TYPE   0x00FF0000
#define KEY_MASK_OBJID  0x0000FF00
#define KEY_MASK_SNSR   0x000000FF

#define EVAL_MAKE_LOOKUP_KEY(type,obj,snsr) ((type << 16) | (obj  <<  8) | (snsr))
#define EVAL_GET_TYPE(key) ((key & KEY_MASK_TYPE) >> 16)

//#define DEBUG_EVALUATOR
/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/


// Keep this list in sync with enum RPN_ERR
const CHAR* EvalRetValEnumString[12] =
{
  "RPN_ERR_UNKNOWN"                  ,  // place unused.
  "RPN_ERR_INDEX_NOT_NUMERIC"        ,  // Unrecognized operand input name
  "RPN_ERR_INVALID_TOKEN"            ,  // Unrecognized operand input name
  "RPN_ERR_INDEX_OTRNG"              ,  // Index out of range
  "RPN_ERR_TOO_MANY_OPRNDS"          ,  // Too many operands in the expression.
  "RPN_ERR_OP_REQUIRES_SENSOR_OPRND" ,  // Operation is invalid on this operand
  "RPN_ERR_CONST_VALUE_OTRG"         ,  // Const value out of range for FLOAT32
  "RPN_ERR_TOO_FEW_STACK_VARS"       ,  // Operation requires more vars on stack than present
  "RPN_ERR_INV_OPRND_TYPE"           ,  // One or more operands are invalid for operation
  "RPN_ERR_TOO_MANY_TOKENS_IN_EXPR"  ,  // Too many tokens to fit on stack
  "RPN_ERR_TOO_MANY_STACK_VARS"      ,  // Too many stack vars were present at end of eval
  "RPN_ERR_PRIOR_SENSR_TABLE_FULL"      // The table storing Prior-sensor values is full.
};


//  OpCodeTable[] allocation and initialization
#undef OPCMD
#define OPCMD(OpCode, Token, TokenLen, AddCmd, FmtString, ExeCmd)\
             {OpCode, Token, TokenLen, AddCmd, FmtString, ExeCmd}
const EVAL_OPCODE_TBL_ENTRY OpCodeTable[EVAL_OPCODE_MAX] = {EVAL_OPCODE_LIST};


//  DataAccessTable[] allocation and initialization
#undef DAI
#define DAI(OpCode, IsConfigured, RetValueFunc, RetBoolFunc, ValidFunc) \
           {OpCode, IsConfigured, RetValueFunc, RetBoolFunc, ValidFunc}
const EVAL_DATAACCESS dataAccessTable[EVAL_DAI_MAX] = {EVAL_DAI_LIST};

// Evaluation rpn stack used at runtime
EVAL_RPN_ENTRY rpn_stack[EVAL_EXPR_BIN_LEN];
INT32          rpn_stack_pos = 0;


// Array for storing prior sensors values for all managed expressions.

static PRIOR_SENSOR_ENTRY m_masterTbl[MAX_PRIOR_VALUES];
static UINT8              m_masterTblCnt       = 0;

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static const EVAL_DATAACCESS* EvalGetDataAccess(BYTE  OpCode);

static INT32   EvalParseObjectIndex( const CHAR* str, INT32 maxIndex);
static INT8    EvalGetValidCnt     ( const EVAL_RPN_ENTRY* operandA,
                                     const EVAL_RPN_ENTRY* operandB);
static BOOLEAN EvalVerifyDataType  ( DATATYPE expectedType, const EVAL_RPN_ENTRY* opA,
                                     const EVAL_RPN_ENTRY* opB);

static void    EvalGetPrevSensorValue(UINT32 key,
                                      FLOAT32* fPriorValue,
                                      BOOLEAN* bPriorValid);
static void    EvalSetPrevSensorValue(UINT32 key,
                                      EVAL_EXE_CONTEXT* context,
                                      FLOAT32 fValue,
                                      BOOLEAN bValid);
static BOOLEAN EvalUpdatePrevSensorList( const EVAL_EXE_CONTEXT* context);

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:      EvalGetDataAccess
 *
 * Description:
 *
 * Parameters:    [in] OpCode - the command who data-access function is to
 *                              returned.
 *
 *
 * Returns: A pointer to the function to be used to retrieve the data for the
 *          the passed OpCode.
 *
 * Notes:
 *
 *
 *****************************************************************************/
static const EVAL_DATAACCESS* EvalGetDataAccess(BYTE  OpCode)
{
  INT32   i;
  BOOLEAN bFound = FALSE;
  const EVAL_DATAACCESS* pDataAcc = NULL;

  // Search until found or end-of-table

  for(i = 0; !bFound  && i < (INT32) EVAL_DAI_MAX; ++i)
  {
    if( OpCode == dataAccessTable[i].OpCode)
    {
      pDataAcc = (const EVAL_DATAACCESS*) &dataAccessTable[i];
      bFound = TRUE;
    }
  }
  return pDataAcc;
}

/******************************************************************************
 * Function:      EvalParseObjectIndex
 *
 * Description:
 *
 * Parameters:    [in] str:      string containing object index
 *                [in] maxIndex: max value limit expected for object index
 *                [out]
 *
 * Returns:       The integer value of the object index.
 *
 * Notes:
 *
 *
 *****************************************************************************/
#define BASE_10 10
static INT32 EvalParseObjectIndex( const CHAR* str, INT32 maxIndex)
{
  INT32  temp_int;
  INT32 len;
  INT32  retVal;
  CHAR*  end;

  // The index must start at str and be left-padded, 3 character wide
  // (000 -> 255)

  if ( !isdigit(*str) )
  {
    retVal = (INT32) RPN_ERR_INDEX_NOT_NUMERIC;
  }
  else
  {
    temp_int = strtol(str, &end, BASE_10);
    len = (end - str);

    if( len == EVAL_OPERAND_DIGIT_LEN &&
        temp_int < maxIndex && temp_int >= 0)
    {
      retVal = (INT8)temp_int;
    }
    else
    {
      retVal = (INT8)RPN_ERR_INDEX_OTRNG;
    }
  }
  return retVal;
}


/******************************************************************************
 * Function:      EvalGetValidCnt
 *
 * Description:
 *
 * Parameters:    [in] operandA: Pointer to the RPN entry containing 1st operand.
 *                [in] operandB: Pointer to the RPN entry containing 2nd operand.

 *
 * Returns:      The number of valid operands passed.
 *
 * Notes:        This function is used to pre-establish whether the input(s) to be
 *               evaluated are valid. operandB can be optional.
 *
 *****************************************************************************/
static INT8 EvalGetValidCnt(const EVAL_RPN_ENTRY* operandA, const EVAL_RPN_ENTRY* operandB)
{
  INT8 cnt = 0;

  // If operandA was passed(should always be true), check it's validity
  if (NULL != operandA)
  {
    cnt  += operandA->Validity ? 1 : 0;
  }
  // If operandB was passed, check it's validity
  if (NULL != operandB)
  {
    cnt  += operandB->Validity ? 1 : 0;
  }

  return cnt;
}

/******************************************************************************
* Function:      EvalVerifyDataType
*
* Description:   Verify that the passed operands types are the expected type
*
* Parameters:    [in] expectedType - expected data type.
*                [in] opA          - first operand.
*                [in] opB          - second operand. (may be optional)
*
*                [out]
*
* Returns:       TRUE if all operands are of the expected type.
*                FALSE if one or both are not of the expected type.
* Notes:
*
*
*****************************************************************************/
static BOOLEAN EvalVerifyDataType(DATATYPE expectedType,
                                 const EVAL_RPN_ENTRY* opA,
                                 const EVAL_RPN_ENTRY* opB)
{
  BOOLEAN bState = TRUE;
  // operand A is required in any expression, operand B is
  // optional depending on whether the op is binary or unary.

  if( opA->DataType != expectedType ||
    !(NULL == opB) && opB->DataType != expectedType )
  {
    bState = FALSE;
  }
  // return the result of the check.
  return bState;
}


/*****************************************************************************/
/* Public  Functions                                                         */
/*****************************************************************************/

/******************************************************************************
 * Function:      EvalGetMsgFromErrCode
 *
 * Description:
 *
 * Parameters:    [in] errNum: The evaluator error number from RPN_ERR to be
 *                             converted into descriptive string.
 *                [out]
 *
 * Returns:   pointer to a string constant containing the descriptive msg
 *            (see EvalRetValEnumString array)
 * Notes:
 *
 *
 *****************************************************************************/
const CHAR* EvalGetMsgFromErrCode(INT32 errNum)
{
  // Check index range.
  // Invert the negative errNum into the positive index into the string table.
  INT32 i = (errNum < (INT32)RPN_ERR_MAX || errNum >= (INT32) RPN_ERR_UNKNOWN)
			 ? (INT32) RPN_ERR_UNKNOWN : -errNum;
  return EvalRetValEnumString[i];
}

/******************************************************************************
 * Function:    EvalExeExpression
 *
 * Description: Take a binary format boolean expression of transition criteria
 *              Evaluate that expression based on the current state of all the
 *              inputs conditions.
 *
 *              The function evaluates the RPN boolean expression.
 *
 *              Error checking is for use during system initialization
 *              configuration.  If an error is detected at runtime the
 *              caller should assert.
 *
 *
 * Parameters:  [in]     objType:  enum indicating the caller-type/context to perform.
 *              [in]       objId:  the index of the object indicated by objType.
 *              [in/out]    expr:  The expression object to be parsed.
 *              [in/out]validity:  The validity of the resulting expression based
 *                                 on the validity rules of the data sources.
 *
 * Returns:     0: Expression evaluates to FALSE
 *              1: Expression evaluates to TRUE
 *            < 0: see RPN_ERR in Evaluator.h for list of error codes.
 *
 * Notes:
 *
 *****************************************************************************/
INT32 EvalExeExpression (EVAL_CALLER_TYPE objType, INT32 objId,
                         const EVAL_EXPR* expr, BOOLEAN* validity)
{
  static BOOLEAN fullTableReported = FALSE;
  INT32   i;
  const EVAL_OPCODE_TBL_ENTRY* pOpCodeTbl = NULL;
  const EVAL_CMD* cmd;
  EVAL_RPN_ENTRY  result;
  EVAL_EXE_CONTEXT  context;
#ifdef DEBUG_EVALUATOR
  /*vcast_dont_instrument_start*/
  CHAR expString[256];
  EvalExprBinToStr(expString,expr);
  /*vcast_dont_instrument_end*/
#endif

  // Setup the context structure
  context.objType    = (UINT8)objType;
  context.objId      = (UINT8)objId;
  context.tempTblCnt = 0;

  // Init the results
  result.DataType = DATATYPE_BOOL;
  result.Data     = 0.f;
  result.Validity = TRUE;

  // Init the stack to empty
  rpn_stack_pos = 0;

  // Process the commands in this expression.
  for( i = 0; i < expr->Size; i++ )
  {
    // Get a ptr to next cmd in list.
    cmd = &expr->CmdList[i];

    // The OpCode value is the index into the OpCodeTable for this operation
    // see: EVAL_OPCODE_LIST in EvaluatorInterface.h
    pOpCodeTbl = &OpCodeTable[cmd->opCode];

    // Set the current command ptr into the context struct
    // and process it.
    context.cmd = cmd;
    if ( !pOpCodeTbl->ExeCmd( &context ) )
    {
      // Something failed, break out.
      // the most recent stack-push entry has details.
      break;
    }
  } // for-loop cmd list


  // If this expression has added entries to the temp prev-sensor table,
  // merge the temp into the master.
  if(context.tempTblCnt > 0 )
  {
    // The update function will return false if table full. Only report this once per run.
    if ( !EvalUpdatePrevSensorList(&context) )
    {
      RPN_STACK_PURGE;
      result.Data     = (INT32) RPN_ERR_NOT_PREV_TABLE_FULL;
      result.DataType = DATATYPE_RPN_PROC_ERR;
      result.Validity = FALSE;
      RPN_PUSH(result);
      if (!fullTableReported)
      {
        GSE_DebugStr(NORMAL,
					 TRUE,
					 "Expression Runtime Error. %s",
					 EvalGetMsgFromErrCode( (INT32) RPN_ERR_NOT_PREV_TABLE_FULL));
        fullTableReported = TRUE;
      }
    }
  }

  // At this point there should be only a single item on the stack with the
  // results of the expression processing or in the event of an error, the error status.
  // If more than one, clear the stack and push an error msg on
  if (RPN_STACK_CNT > 1)
  {
    RPN_STACK_PURGE;
    result.Data     = (INT32) RPN_ERR_TOO_MANY_STACK_VARS;
    result.DataType = DATATYPE_RPN_PROC_ERR;
    result.Validity = FALSE;
    RPN_PUSH(result);

  }

  // Pop the 'final' entry.
  result = RPN_POP;
  *validity = result.Validity;
  return (INT32)result.Data;
}

/******************************************************************************
 * Function:    EvalExprStrToBin
 *
 * Description: Convert a evaluation expression from RPN string
 *              form to a binary form for use at runtime.
 *
 * Parameters:  [in]     objType:  enum indicating the caller-type/context to perform.
 *              [in]       objID:  the index of the object indicated by objType.
 *              [in]         str:  The expression string to be converted.
 *              [in/out]    expr:  The expression object to be parsed.
 *              [in] maxOperands:  The max operands to be allowed in the binary
 *                                 expression.
 *
 * Returns:     0: Expression evaluates to FALSE
 *              1: Expression evaluates to TRUE
 *            < 0: see RPN_ERR in Evaluator.h for list of error codes.
 *              --------------------------------------------------------------------
 * Notes:
 *
 *****************************************************************************/

INT32 EvalExprStrToBin( EVAL_CALLER_TYPE objType, INT32 objID,
                        CHAR* str,  EVAL_EXPR* expr, UINT8 maxOperands)
{
  INT16 i;
  INT32 len;
  INT32 retval = 0;
  BOOLEAN temp = TRUE;

  // Init the expression-obj as empty
  expr->Size        = 0;
  expr->MaxOperands = maxOperands; // Maximum # operands for this type
  expr->OperandCnt = 0;
  memset( expr->CmdList, 0, EVAL_EXPR_BIN_LEN );

  // Shift to uppercase for comparisons
  Supper(str);

  //Start looking for operators and operands
  while((*str != '\0') )
  {
    //Ignore spaces between operators and inputs.
    if(*str == ' ')
    {
      str++;
      continue;
    }

    // More tokens to process but Cmd list full!
    // break out of while-loop
    if(expr->Size >= EVAL_EXPR_BIN_LEN)
    {
      retval = (INT32) RPN_ERR_TOO_MANY_TOKENS_IN_EXPR;
      break;
    }

    len = 0;
    // If the token matches a token in table, call the associated
    // function to add a command to the expression's cmd-list.
    for ( i = 0; i < (INT16) EVAL_OPCODE_MAX; ++i )
    {
      if( 0 != OpCodeTable[i].TokenLen  &&
          0 == strncmp(OpCodeTable[i].Token, str, OpCodeTable[i].TokenLen))
      {
        len = OpCodeTable[i].AddCmd(i, str, expr);
        break; // Processed a token, break out of lookup loop
      }
    }

    // For-loop ended, check for failure.
    if(len < 0)
    {
      // Negative length means a parsing error...
      // save it and break out.
      retval = len;
      break;
    }

    // No failure, if zero chars were processed, perhaps this is a
    // constant float value?
    if ( 0 == len )
    {
      len = EvalAddConst( (INT16) OP_CONST_VAL, str, expr );
      // Negative result indicates that it WAS a constant float
      // but it was a bad value.
      if ( len < 0 )
      {
        retval = len;
        break;
      }
    }

    // Still no luck parsing... its an unrecognized token.
    // stop parsing.
    if(0 == len)
    {
      retval = (INT32) RPN_ERR_INVALID_TOKEN;
      break;
    }
    else // Token has been encoded into list... increment to next.
    {
      str += len;
    }
  }// while str has tokens.


  // If replacement-parse went ok and
  // the expression is not empty then execute the expression
  // to check for syntactic/logic errors. Validity errors are
  // not important during init, this is to check the grammar of
  // of the cmd sequence.
  if((retval >= 0 && expr->Size != 0))
  {
    retval = EvalExeExpression( objType, objID, expr, &temp);
  }

  // If expression is invalid, disable it
  if (retval < 0)
  {
    expr->Size = 0;
    GSE_DebugStr(NORMAL,FALSE,"Expression Parsing failed. retval = %s",
                 EvalGetMsgFromErrCode(retval));
  }
  return retval;
}


/******************************************************************************
 * Function:      EvalExprBinToStr
 *
 * Description:   Convert the binary representation of an expression
 *                to a string.  This would be done for reading the configuration
 *                data back to the user through User Manager
 *
 * Parameters:    [out]    str: Location to store the result.  Size must be
 *                              at least FSM_TC_STR_LEN
 *                [in]     bin: Binary expression
 *
 * Returns:        none.
 *
 * Notes:         Binary form is assumed parseable, since it was checked when
 *                parsed in from a string
 *
 *****************************************************************************/
void  EvalExprBinToStr( CHAR* str, const EVAL_EXPR* expr )
{
  INT16   i;
  INT16   j;
  INT32   cnt;
  CHAR*   pStr = str;

  //Clear str
  *str = '\0';


  for(i = 0; i < expr->Size; i++)
  {
    j = expr->CmdList[i].opCode;
    cnt = OpCodeTable[j].FmtCmd( j, (const EVAL_CMD*) &expr->CmdList[i], pStr);
    pStr += cnt;
    *pStr = ' ';
    pStr++;

    //Should never happen unless the op code table is corrupted!
    ASSERT_MESSAGE(cnt != 0, "EvalExprBinToStr OpCode not valid: %d",expr->CmdList[i].opCode );
  } // for-loop expression entries
  --pStr;
  *pStr = '\0';
}

/******************************************************************************
 * Function:      EvalLoadConstValue
 *
 * Description:   Add the indicated constant value in the context cmd and push
 *                it on the command stack
 *
 * Parameters:    [in] Pointer to the expression context/cmd stack
 *
 *                [out] none
 *
 * Returns:       TRUE if operation was successful, otherwise FALSE
 *
 * Notes:
 *
 *
 *****************************************************************************/
BOOLEAN EvalLoadConstValue( EVAL_EXE_CONTEXT* context )
{
  // A constant is always valid,
  EVAL_RPN_ENTRY rslt;


  rslt.Data = context->cmd->data;
  rslt.DataType = DATATYPE_VALUE;
  rslt.Validity = TRUE;

  RPN_PUSH(rslt);
  return TRUE;
}

/******************************************************************************
 * Function:      EvalLoadConstFalse
 *
 * Description:   Build a const bool "FALSE" obj and push on stack
 *
 * Parameters:    [in] Pointer to the expression context/cmd stack
 *
 *                [out] none
 *
 * Returns:       TRUE if operation was successful, otherwise FALSE
 *
 * Notes:
 *
 *
 *****************************************************************************/
BOOLEAN EvalLoadConstFalse( EVAL_EXE_CONTEXT* context)
{
  // A constant is always valid,
  EVAL_RPN_ENTRY rslt;

  rslt.Data = context->cmd->data;
  rslt.DataType = DATATYPE_BOOL;
  // Const FALSE is always VALID
  rslt.Validity = TRUE;

  RPN_PUSH(rslt);
  return TRUE;
}

/******************************************************************************
 * Function:      EvalLoadInputSrc
 *
 * Description:   Load the value or boolean state from the indicated source
 *                and push in on the command stack.
 *
 * Parameters:    [in] Pointer to the expression context/cmd stack
 *
 *                [out] none
 *
 * Returns:       TRUE if operation was successful, otherwise FALSE
 *
 * Notes:
 *
 *
 *****************************************************************************/
BOOLEAN EvalLoadInputSrc( EVAL_EXE_CONTEXT* context)
{
  EVAL_RPN_ENTRY  rslt;
  const EVAL_DATAACCESS* dataAcc;
  const EVAL_CMD* cmd = context->cmd;

  // Get a pointer to the DataAccess table for the set of
  // function-pointers supporting this opCode.

   dataAcc = EvalGetDataAccess(cmd->opCode);

   // Should never be true! (Table 'coding' error)
   // Every entry tied to this func MUST also have a corr. entry in EVAL_DAI_LIST
   ASSERT_MESSAGE(NULL != dataAcc,
                  "Data Access Interface lookup not configured for OpCode: %d",
                  cmd->opCode );

  // Only 1 'Get' function should be defined in table for each opCode
  // Should never be true. (Table 'coding' error)
  ASSERT_MESSAGE(!((dataAcc->GetSrcByBool == NULL) && (dataAcc->GetSrcByValue == NULL)) &&
                 !((dataAcc->GetSrcByBool != NULL) && (dataAcc->GetSrcByValue != NULL)),
                 "Multiple 'Get' entries configured for DataAccessTable[%d] Cmd: [%d][%f]",
                  cmd->opCode, cmd->opCode, cmd->data);

  // Init the data-type and data here.
  // If the source is not configured we need at least
  // this much info for the stack push.
  rslt.DataType = (NULL != dataAcc->GetSrcByValue) ? DATATYPE_VALUE : DATATYPE_BOOL;
  rslt.Data = 0.f;

  // If the input source is configured, get the data (value or bool) and validity
  if (dataAcc->IsSrcConfigured( (INT32)cmd->data) )
  {
    rslt.Data = (NULL != dataAcc->GetSrcByValue) ?
                  dataAcc->GetSrcByValue( (INT32) cmd->data) :
                  dataAcc->GetSrcByBool ( (INT32) cmd->data);

    rslt.Validity = dataAcc->GetSrcValidity( (INT32)cmd->data);
  }
  else // The indicated data source is not configured.
  {
    rslt.Validity = FALSE;
  }

  // If the input source is an invalid boolean operand (e.g trigger), treat the value as false
  if (DATATYPE_BOOL == rslt.DataType && FALSE == rslt.Validity)
  {
    rslt.Data = FALSE;
  }

  RPN_PUSH(rslt);
  return TRUE;
}

/******************************************************************************
 * Function: EvalLoadFuncCall
 *
 * Description:   Loads the result from the indicated function call and pushes
 *                it on the command stack.
 *
 *
 * Parameters:    [in] Pointer to the expression context/cmd stack
 *
 *                [out] none
 *
 * Returns:       TRUE if operation was successful, otherwise FALSE
 *
 * Notes:
 *
 *
 *****************************************************************************/
BOOLEAN EvalLoadFuncCall ( EVAL_EXE_CONTEXT* context)
{
  EVAL_RPN_ENTRY  rslt;
  const EVAL_DATAACCESS* dataAcc;
  const EVAL_CMD* cmd = context->cmd;

  // Get a pointer to the DataAccess table for the set of
  // function-pointers supporting this opCode.

  dataAcc = EvalGetDataAccess(cmd->opCode);

  // Should never be true! (Table 'coding' error)
  // Every entry tied to this func MUST also have a corr. entry in EVAL_DAI_LIST
  ASSERT_MESSAGE(NULL != dataAcc,
                 "Data Access Interface lookup not configured for OpCode: %d",
                 cmd->opCode );

  // Only 1 'Get' function should be defined in table for any given opCode
  ASSERT_MESSAGE(!((dataAcc->GetSrcByBool == NULL) && (dataAcc->GetSrcByValue == NULL)) &&
                 !((dataAcc->GetSrcByBool != NULL) && (dataAcc->GetSrcByValue != NULL)),
                 "Multiple 'Get' entries configured for DataAccessTable[%d] Cmd: [%d][%f]",
                 cmd->opCode, cmd->opCode, cmd->data);


  // Call the app-supplied function thru the DAI table ptr
  rslt.DataType = (NULL != dataAcc->GetSrcByValue) ? DATATYPE_VALUE : DATATYPE_BOOL;

  rslt.Data     = (NULL != dataAcc->GetSrcByValue) ?
                   dataAcc->GetSrcByValue( (INT32) cmd->data) :
                   dataAcc->GetSrcByBool ( (INT32) cmd->data);

 // A app-supplied function is always considered VALID
 rslt.Validity = TRUE;

  RPN_PUSH(rslt);
  return TRUE;

}

/******************************************************************************
 * Function: EvalCompareOperands
 *
 * Description:   Compare the previous two values from the command stack,
 *                verify that they are the correct type and push the result
 *                back on the stack.
 *
 * Parameters:    [in] Pointer to the expression context/cmd stack
 *
 *                [out] none
 *
 * Returns:       TRUE if operation was successful, otherwise FALSE
 *
 * Notes:
 *
 *
 *****************************************************************************/
BOOLEAN EvalCompareOperands( EVAL_EXE_CONTEXT* context)
{
  EVAL_RPN_ENTRY oprndLeft;
  EVAL_RPN_ENTRY oprndRight;
  EVAL_RPN_ENTRY rslt;
  const EVAL_CMD* cmd = context->cmd;

  // Need two operands off stack
  if(RPN_STACK_CNT < 2)
  {
    rslt.Data     = (INT32) RPN_ERR_TOO_FEW_STACK_VARS;
    rslt.DataType = DATATYPE_RPN_PROC_ERR;
    rslt.Validity = FALSE;
    RPN_STACK_PURGE;
  }
  else
  {
    oprndRight = RPN_POP;
    oprndLeft  = RPN_POP;
    if ( EvalVerifyDataType(DATATYPE_VALUE, &oprndLeft, &oprndRight) )
    {
        // perform the comparison operation, store the result
        switch (cmd->opCode)
        {
          case OP_NE:
            rslt.Data = (FLOAT32)(fabs(oprndLeft.Data - oprndRight.Data) >= FLT_EPSILON);
            break;

          case OP_EQ:
            rslt.Data = (FLOAT32)(fabs(oprndLeft.Data - oprndRight.Data) <= FLT_EPSILON);
            break;

          case OP_GE:
            rslt.Data = (FLOAT32) ((oprndLeft.Data > oprndRight.Data) ||
                        (fabs(oprndLeft.Data - oprndRight.Data) <= FLT_EPSILON));
            break;

          case OP_LE:
            rslt.Data = (FLOAT32) ((oprndLeft.Data < oprndRight.Data) ||
                        (fabs(oprndLeft.Data - oprndRight.Data) <= FLT_EPSILON));
            break;

          case OP_GT:
            rslt.Data = (FLOAT32) (oprndLeft.Data > oprndRight.Data);
            break;

          case OP_LT:
            rslt.Data = (FLOAT32) (oprndLeft.Data < oprndRight.Data);
            break;

          // Should never get here; all Comparison Operators are handled.
          default:
            FATAL("Unrecognized Expression OpCode: %d", cmd->opCode);
        }

      rslt.DataType = DATATYPE_BOOL;

      // SRS-4544  The result of a comparison operation shall be INVALID if
      // the validity of either of the input value operands is invalid.
      rslt.Validity = ( EvalGetValidCnt(&oprndLeft, &oprndRight ) == 2 );

      // SRS-4595 The result of a comparison operation shall be FALSE
      // if the validity of  either of the input values operands is invalid
      if(!rslt.Validity)
      {
        rslt.Data = FALSE;
      }

    }
    else
    {
      // Set RPN response to flag a syntax error
      rslt.Data     = (INT32) RPN_ERR_INV_OPRND_TYPE;
      rslt.DataType = DATATYPE_RPN_PROC_ERR;
      rslt.Validity = FALSE;
      // Ensure our error msg will be the only thing on stack.
      RPN_STACK_PURGE;
    }
  }

  RPN_PUSH(rslt);
  return rslt.DataType != DATATYPE_RPN_PROC_ERR;
}

/******************************************************************************
 * Function: EvalIsNotEqualPrev
 *
 * Description:   Test that the current value of the indicated sensor is not
 *                equal to the previous value
 *
 * Parameters:    [in] Pointer to the expression context/cmd stack
 *
 *                [out] none
 *
 * Returns:       TRUE if operation was successful, otherwise FALSE
 *
 * Notes:
 *
 *
 *****************************************************************************/
BOOLEAN EvalIsNotEqualPrev( EVAL_EXE_CONTEXT* context)
{
  EVAL_RPN_ENTRY oprndCurrent;
  EVAL_RPN_ENTRY oprndPrevious;
  EVAL_RPN_ENTRY rslt;
  UINT32         key;

  // Need one operand off stack
  if(RPN_STACK_CNT < 1)
  {
    rslt.Data     = (INT32) RPN_ERR_TOO_FEW_STACK_VARS;
    rslt.DataType = DATATYPE_RPN_PROC_ERR;
    rslt.Validity = FALSE;

    // Ensure error msg will be the only
    // thing on the stack(push is performed at end of func.)
    RPN_STACK_PURGE;
  }
  else
  {
    oprndCurrent = RPN_POP;
    ASSERT_MESSAGE(EvalVerifyDataType(DATATYPE_VALUE, &oprndCurrent, NULL),
    "Not-Previous Sensor value was not a VALUE: DataType: %d, Data: %d, Validity: %d",
                   oprndCurrent.DataType,
                   oprndCurrent.Data,
                   oprndCurrent.Validity)

    // Lookup/add the previous-sensor for this cmd and make an R-side operand
    oprndPrevious.DataType = DATATYPE_VALUE;

    // Lookup the previous sensor value for this reference.
    // Make the lookup key using caller type, the objectId holding this expression, and the
    // sensor being referenced.
    key = EVAL_MAKE_LOOKUP_KEY( context->objType, context->objId, (UINT8)context->cmd->data);

    EvalGetPrevSensorValue( key, &oprndPrevious.Data, &oprndPrevious.Validity);

    rslt.Data = (FLOAT32)(fabs( (oprndCurrent.Data - oprndPrevious.Data)) >= FLT_EPSILON );
    rslt.DataType = DATATYPE_BOOL;
    rslt.Validity = ( EvalGetValidCnt(&oprndCurrent, &oprndPrevious ) == 2 );

    // Store the current sensor value & validity in table for next processing.
    EvalSetPrevSensorValue( key, context, oprndCurrent.Data ,oprndCurrent.Validity );
  }

  RPN_PUSH(rslt);
  return rslt.DataType != DATATYPE_RPN_PROC_ERR;
}


/******************************************************************************
 * Function: EvalPerformNot
 *
 * Description:   Perform a unary negation on the last entry in the command stack.
 *
 * Parameters:    [in] Pointer to the expression context/cmd stack
 *
 *                [out] none
 *
 * Returns:       TRUE if operation was successful, otherwise FALSE
 *
 * Notes:
 *
 *
 *****************************************************************************/
BOOLEAN EvalPerformNot( EVAL_EXE_CONTEXT* context)
{
  EVAL_RPN_ENTRY oprnd;
  EVAL_RPN_ENTRY rslt;

  // Need one operand off stack
  if(RPN_STACK_CNT < 1)
  {
    rslt.Data     = (INT32)RPN_ERR_TOO_FEW_STACK_VARS;
    rslt.DataType = DATATYPE_RPN_PROC_ERR;
    rslt.Validity = FALSE;
    // Ensure error msg is only thing on the stack
    RPN_STACK_PURGE;
  }
  else
  {
    oprnd = RPN_POP;
    if ( EvalVerifyDataType(DATATYPE_BOOL, &oprnd, NULL) )
    {
      rslt.Validity = oprnd.Validity;
      rslt.DataType = DATATYPE_BOOL;
      rslt.Data     = rslt.Validity ? (FLOAT32) !(BOOLEAN)oprnd.Data : 0.f;
    }
    else
    {
      // Set RPN response to flag a syntax error
      rslt.Data     = (INT32)RPN_ERR_INV_OPRND_TYPE;
      rslt.DataType = DATATYPE_RPN_PROC_ERR;
      rslt.Validity = FALSE;
      // Ensure error msg is only thing on the stack
      RPN_STACK_PURGE;
    }
  }

  RPN_PUSH(rslt);
 return rslt.DataType != DATATYPE_RPN_PROC_ERR;
}

/******************************************************************************
 * Function: EvalPerformAnd
 *
 * Description:   Perform a logical AND on the last 2-entries in the command stack.
 *
 * Parameters:    [in] Pointer to the expression context/cmd stack
 *
 *                [out] none
 *
 * Returns:       TRUE if operation was successful, otherwise FALSE
 *
 * Notes:
 *
 *
 *****************************************************************************/
BOOLEAN EvalPerformAnd( EVAL_EXE_CONTEXT* context )
{
  EVAL_RPN_ENTRY oprndLeft;
  EVAL_RPN_ENTRY oprndRight;
  EVAL_RPN_ENTRY rslt;

  // Need two operands off stack
  if(RPN_STACK_CNT < 2)
  {
    rslt.Data     = (INT32)RPN_ERR_TOO_FEW_STACK_VARS;
    rslt.DataType = DATATYPE_RPN_PROC_ERR;
    rslt.Validity = FALSE;
    // Ensure error msg is only thing on the stack
    RPN_STACK_PURGE;
  }
  else
  {
    oprndRight = RPN_POP;
    oprndLeft  = RPN_POP;
    if ( EvalVerifyDataType(DATATYPE_BOOL, &oprndLeft, &oprndRight) )
    {
      // perform the compare, set the data type
      rslt.Data     = (FLOAT32) (BOOLEAN)(oprndLeft.Data && (BOOLEAN)oprndRight.Data);
      rslt.DataType = DATATYPE_BOOL;
      rslt.Validity = ( EvalGetValidCnt(&oprndLeft, &oprndRight) == 2 );
    }
    else
    {
      // Set RPN response to flag a syntax error
      rslt.Data     = (INT32) RPN_ERR_INV_OPRND_TYPE;
      rslt.DataType = DATATYPE_RPN_PROC_ERR;
      rslt.Validity = FALSE;
      // Ensure error msg is only thing on the stack
      RPN_STACK_PURGE;
    }
  }
  RPN_PUSH(rslt);
  return rslt.DataType != DATATYPE_RPN_PROC_ERR;
}

/******************************************************************************
 * Function: EvalPerformOr
 *
 * Description:   Perform a logical OR on the last 2-entries in the command stack.
 *
 * Parameters:    [in] Pointer to the expression context/cmd stack
 *
 *                [out] none
 *
 * Returns:       TRUE if operation was successful, otherwise FALSE
 *
 * Notes:
 *
 *
 *****************************************************************************/
BOOLEAN EvalPerformOr( EVAL_EXE_CONTEXT* context )
{
  EVAL_RPN_ENTRY oprndLeft;
  EVAL_RPN_ENTRY oprndRight;
  EVAL_RPN_ENTRY rslt;

  // Need two operands off stack
  if(RPN_STACK_CNT < 2)
  {
    rslt.Data     = (INT32) RPN_ERR_TOO_FEW_STACK_VARS;
    rslt.DataType = DATATYPE_RPN_PROC_ERR;
    rslt.Validity = FALSE;
    RPN_STACK_PURGE;
  }
  else
  {
    oprndRight = RPN_POP;
    oprndLeft  = RPN_POP;
    if ( EvalVerifyDataType(DATATYPE_BOOL, &oprndLeft, &oprndRight) )
    {
      if ( (oprndLeft.Validity  && (BOOLEAN)oprndLeft.Data) ||
           (oprndRight.Validity && (BOOLEAN)oprndRight.Data) )
      {
        rslt.Data = 1.0f;
        rslt.DataType = DATATYPE_BOOL;
        rslt.Validity = TRUE;
      }
      else
      {
        rslt.Data = 0.0f;
        rslt.DataType = DATATYPE_BOOL;
        rslt.Validity = (oprndLeft.Validity || oprndRight.Validity);
      }
    }
    else
    {
      // Set RPN response to flag a syntax error
      rslt.Data     = (INT32)RPN_ERR_INV_OPRND_TYPE;
      rslt.DataType = DATATYPE_RPN_PROC_ERR;
      rslt.Validity = FALSE;
      // Ensure error msg is only thing on the stack
      RPN_STACK_PURGE;
    }
  }

  RPN_PUSH(rslt);
  return rslt.DataType != DATATYPE_RPN_PROC_ERR;
}


/******************************************************************************
 * Function: EvalAddConst
 *
 * Description:
 *
 * Parameters:    [in]  tblIdx - table index ( not used in this function)
 *                [in]  str    - pointer  to the string containing the const.
 *                [out] expr   - pointer to expression obj to be updated.
 *
 * Returns:       value < 0    - processing error code.
 *                value >= 0   - the length of the string processed.
 *
 * Notes:
 *
 *
 *****************************************************************************/
INT32 EvalAddConst(INT16 tblIdx, const CHAR* str, EVAL_EXPR* expr)
{
  FLOAT32  temp_float;
  CHAR*    end;
  EVAL_CMD cmd;
  INT32    retval;

  // Have we reached limit count for operands?
  if(expr->OperandCnt == expr->MaxOperands)
  {
    retval = (INT32) RPN_ERR_TOO_MANY_OPRNDS;
  }
  else
  {
    // Try to process as a const value. Treat as floating-point
    temp_float = (FLOAT32)strtod(str, &end);
    if( temp_float <= FLT_MAX && temp_float >= -FLT_MAX)
    {
      cmd.opCode = (UINT8)OP_CONST_VAL;
      cmd.data   = temp_float;

      // Add cmd to expression list
      expr->CmdList[expr->Size++] = cmd;
      expr->OperandCnt++;
      retval = end - str;
    }
    else
    {
      retval = (INT32) RPN_ERR_CONST_VALUE_OTRG;
    }
  }
  return retval;
}

/******************************************************************************
 * Function: EvalAddFuncCall
 *
 * Description: Add a "load const bool FALSE" to the cmd list.
 *
 * Parameters:    [in]  tblIdx - table index ( not used in this function)
 *                [in]  str    - pointer  to the string containing the const.
 *                [out] expr   - pointer to expression obj to be updated.
 *
 * Returns:       value < 0    - processing error code.
 *                value >= 0   - the length of the string processed.
 *
 *
 * Notes: This cmd is used to represent a NO_COMPARE conditions which
 *        to False
 *
 *
 *****************************************************************************/
INT32 EvalAddFuncCall(INT16 tblIdx, const CHAR* str, EVAL_EXPR* expr)
{
  EVAL_CMD cmd;
  INT32    retval;

  // Have we reached limit count for operands?
  if(expr->OperandCnt == expr->MaxOperands)
  {
    retval = (INT32) RPN_ERR_TOO_MANY_OPRNDS;
  }
  else
  {
    // Object is a constant Boolean 'FALSE'
    cmd.opCode = OpCodeTable[tblIdx].OpCode;
    cmd.data   = 0.f;

    // Add cmd to expression list
    expr->CmdList[expr->Size++] = cmd;
    expr->OperandCnt++;

    retval = OpCodeTable[tblIdx].TokenLen;
  }
  return retval;
}

/******************************************************************************
 * Function: EvalAddInputSrc
 *
 * Description:
 *
 * Parameters:    [in]  tblIdx - table index
 *                [in]  str    - pointer  to the string containing the const.
 *                [out] expr   - pointer to expression obj to be updated.
 *
 * Returns:       value < 0    - processing error code.
 *                value >= 0   - the length of the string processed.
 *
 *
 * Notes:
 *
 *
 *****************************************************************************/
INT32 EvalAddInputSrc(INT16 tblIdx, const CHAR* str, EVAL_EXPR* expr)
{
  EVAL_CMD cmd;
  INT32    retval = 0;
  INT32    maxObjects = 0;
  INT16    objIdx;

  if(expr->OperandCnt == expr->MaxOperands)
  {
    retval = (INT32) RPN_ERR_TOO_MANY_OPRNDS;
  }
  else
  {
    // Get the allowable number of objects for this  OpCode
    switch(OpCodeTable[tblIdx].OpCode)
    {
      // Sensors
      case OP_GETSNRVAL:
      case OP_GETSNRVALID:
       maxObjects = MAX_SENSORS;
       break;

      //Triggers
      case OP_GETTRIGVAL:
      case OP_GETTRIGVALID:
        maxObjects = MAX_TRIGGERS;
        break;

      default:
        // CANNOT GET HERE
        FATAL("Unrecognized Opcode: %d",OpCodeTable[tblIdx].OpCode );
    }

    //                                                          |
    //                                                          V
    // Look to end of this fixed-token for the objIdx (e.g. SVLU127)
    objIdx = EvalParseObjectIndex( &str[OpCodeTable[tblIdx].TokenLen], maxObjects);
    if(objIdx >= 0)
    {
      cmd.opCode = (UINT8)OpCodeTable[tblIdx].OpCode;
      cmd.data   = (FLOAT32)objIdx;
      // Add cmd to expression list
      expr->CmdList[expr->Size++] = cmd;
      expr->OperandCnt++;
      retval = EVAL_OPRND_LEN;
    }
    else // The operand index was not valid, return it.
    {
      retval = objIdx;
    }
  }
  return retval;
}

/******************************************************************************
 * Function: EvalAddStdOper
 *
 * Description:
 *
 * Parameters:    [in]  tblIdx - table index ( not used in this function)
 *                [in]  str    - pointer  to the string containing the const.
 *                [out] expr   - pointer to expression obj to be updated.
 *
 * Returns:       value < 0    - processing error code.
 *                value >= 0   - the length of the string processed.
 *
 * Notes:
 *
 *
 *****************************************************************************/
INT32 EvalAddStdOper(INT16 tblIdx, const CHAR* str, EVAL_EXPR* expr)
{
  EVAL_CMD cmd;

  cmd.opCode = OpCodeTable[tblIdx].OpCode;
  cmd.data   = 0.f;

  // Add cmd to expression list
  expr->CmdList[expr->Size++] = cmd;

  // Return the len for this operator
  return OpCodeTable[tblIdx].TokenLen;
}

/******************************************************************************
 * Function: EvalAddNotEqPrev
 *
 * Description:
 *
 *  Parameters:    [in]  tblIdx - table index
 *                 [in]  str    - pointer  to the string containing the const.
 *                 [out] expr   - pointer to expression obj to be updated.
 *
 * Returns:       value < 0    - processing error code.
 *                value >= 0   - the length of the string processed.
 *
 *
 * Notes:
 *
 *
 *****************************************************************************/
INT32 EvalAddNotEqPrev(INT16 tblIdx, const CHAR* str, EVAL_EXPR* expr)
{
  EVAL_CMD cmd;
  INT32    retval;

  // Check the previous operation in list..
  // Verify that it is a sensor-load cmd and get idx-value stored
  // in Data so '!P' can make a call to load prev value at exe time.
  if ( (UINT8)OP_GETSNRVAL == expr->CmdList[expr->Size - 1].opCode)
  {
    cmd.opCode = (UINT8)OP_NOT_EQ_PREV;
    cmd.data   = expr->CmdList[expr->Size - 1].data;
    expr->CmdList[expr->Size++] = cmd;
    retval = OpCodeTable[tblIdx].TokenLen;
  }
  else
  {
    retval = (INT32) RPN_ERR_OP_REQUIRES_SENSOR_OPRND;
  }
  return retval;
}

/******************************************************************************
 * Function: EvalFmtLoadEnumeratedCmdStr
 *
 * Description:
 *
 * Parameters:    [in]  tblIdx - table index
 *                [in]  cmd    - pointer  to the cmd to be coverted to string.
 *                [out] str   - pointer to string to hold conversion.
 *
 * Returns:       value < 0    - processing error code.
 *                value >= 0   - the length of the string processed.
 *
 *
 * Notes:
 *
 *
 *****************************************************************************/
INT32 EvalFmtLoadEnumeratedCmdStr (INT16 tblIdx, const EVAL_CMD* cmd, CHAR* str)
{
  return snprintf(str, EVAL_OPRND_LEN + 1, "%s%03d", OpCodeTable[tblIdx].Token,
                                                     (INT32)cmd->data);
}

/******************************************************************************
 * Function: EvalFmtLoadConstStr
 *
 * Description:
 *
 * Parameters:    [in]  tblIdx - table index
 *                [in]  cmd    - pointer  to the cmd to be coverted to string.
 *                [out] str   - pointer to string to hold conversion.
 *
 * Returns:       value < 0    - processing error code.
 *                value >= 0   - the length of the string processed.
 *
 * Notes:
 *
 *
 *****************************************************************************/
INT32 EvalFmtLoadConstStr(INT16 tblIdx, const EVAL_CMD* cmd, CHAR* str)
{
  return snprintf(str, MAX_FP_STRING, "%f", cmd->data);
}

/******************************************************************************
 * Function: EvalFmtLoadCmdStr
 *
 * Description: Return the token string from the opcode table for this tblIdx.
 *
 * Parameters:    [in]  tblIdx - table index ( not used in this function)
 *                [in]  cmd    - pointer  to the cmd to be formatted as a string.
 *                [out] str   -  pointer to expression string to be updated.
 *
 * Returns:       value < 0    - processing error code.
 *                value >= 0   - the length of the string processed.
 *
 * Notes:
 *
 *
 *****************************************************************************/
INT32 EvalFmtLoadCmdStr(INT16 tblIdx, const EVAL_CMD* cmd, CHAR* str)
{
  return snprintf(str, OpCodeTable[tblIdx].TokenLen + 1, "%s", OpCodeTable[tblIdx].Token);
}

/******************************************************************************
 * Function: EvalFmtOperStr
 *
 * Description:
 *
 * Parameters:    [in]  tblIdx - table index ( not used in this function)
 *                [in]  cmd    - pointer  to the command to be coverted.
 *                [out] expr   - pointer to expression string to be updated.
 *
 * Returns:       value < 0    - processing error code.
 *                value >= 0   - the length of the string processed.
 *
 * Notes:
 *
 *
 *****************************************************************************/
INT32 EvalFmtOperStr(INT16 tblIdx, const EVAL_CMD* cmd, CHAR* str)
{
  return  snprintf(str, OpCodeTable[tblIdx].TokenLen + 1, "%s", OpCodeTable[tblIdx].Token);
}

/******************************************************************************
 * Function: EvalGetPrevSensorValue
 *
 * Description: Get the previous sensor value for the !P command at the passed
 *              address.
 *
 * Parameters:    [in]     key:         The lookup-key of the !P command
 *                [in/out] fPriorValue: Pointer to value to return
 *                [in/out] bPriorValid: pointer to the validity to return
 *
 *
 * Returns:   void
 *
 *
 * Notes:
 *
 *****************************************************************************/
static void  EvalGetPrevSensorValue(UINT32 key,
                                    FLOAT32* fPriorValue,
                                    BOOLEAN* bPriorValid)
{
  INT16 i;
  INT16 idx = -1; // Init index to "not found"

  // Search thru the general table to see if this
  // sensor is already being managed
  for(i = 0; i < m_masterTblCnt; ++i)
  {
    if (m_masterTbl[i].KeyField == key)
    {
      idx = i;
      break;
    }
  }

  // If we found the table entry, return values.
  if (-1 != idx)
  {
    *fPriorValue = m_masterTbl[idx].PriorValue;
    *bPriorValid = m_masterTbl[idx].PriorValid;
  }
  else
  {
    *fPriorValue = 0.f;
    *bPriorValid = FALSE;
  }

}


/******************************************************************************
 * Function: EvalSetPrevSensorValue
 *
 * Description: Stores the passed value as the prior sensor value for the !P
 *              cmd in the temporary table. ( This table's contents will be merged
 *              into the m_masterTbl at the end of processing for this expression.
 *
 * Parameters:    [in] key     - search key for this prev sensor value.
 *                [in] context - to be updated.
 *                [in] fValue  - value to be store
 *                [in] bValid  - valid state of the value.
 *
 *                [out]
 *
 * Returns:       None.
 *
 * Notes:
 *
 *
 *****************************************************************************/
static void EvalSetPrevSensorValue(UINT32 key,
                                   EVAL_EXE_CONTEXT* context,
                                   FLOAT32 fValue,
                                   BOOLEAN bValid)
{
  INT16 i;
  UINT32 objType = EVAL_GET_TYPE(key);
  BOOLEAN status = FALSE;

  if ( (EVAL_CALLER_TYPE)objType > EVAL_CALLER_TYPE_PARSE &&
       (EVAL_CALLER_TYPE)objType < MAX_EVAL_CALLER_TYPE)
  {
    // Lookup entry in temp table and update it.
    for(i = 0; i < context->tempTblCnt; ++i)
    {
      if (context->tempTbl[i].KeyField == key)
      {
        context->tempTbl[i].PriorValue = fValue;
        context->tempTbl[i].PriorValid = bValid;
        status = TRUE;
        break;
      }
    }

    // If entry not found, add it.
    if (!status)
    {
      // THIS CAN NEVER BE TRUE because MAX_TEMP_PRIOR_VALUES is defined as
      // the max # of expression tokens PLUS one. Parsing would fail first!

      ASSERT_MESSAGE(context->tempTblCnt + 1 < MAX_TEMP_PRIOR_VALUES,
                     "Temporary previous-sensor array is FULL",NULL);

      context->tempTbl[context->tempTblCnt].KeyField   = key;
      context->tempTbl[context->tempTblCnt].PriorValue = fValue;
      context->tempTbl[context->tempTblCnt].PriorValid = bValid;
      ++context->tempTblCnt;
    }
  }
}

/******************************************************************************
 * Function: EvalUpdatePrevSensorList
 *
 * Description: Merge the contents of the temp prev-value
 *              cmd in the temporary table. ( This table's contents will be merged
 *              into the m_masterTbl at the end of processing for this expression.
 *
 * Parameters:    [in] execution context
 *
 *                [out]
 *
 * Returns:       Boolean indicating whether the master and context tables
 *
 * Notes:
 *
 *
 *****************************************************************************/
static BOOLEAN EvalUpdatePrevSensorList( const EVAL_EXE_CONTEXT* context)
{
  INT16 idxTempTbl;
  INT16 idxMastTbl;
  BOOLEAN bFound     = FALSE;
  BOOLEAN bTableFull = FALSE;
  INT16   updateCnt = 0;        // updateCnt should equal m_tempTblCnt at end of function

  // Lookup entry in temp table.
  for(idxTempTbl = 0; idxTempTbl < context->tempTblCnt; ++idxTempTbl)
  {
    bFound = FALSE;
    // UPDATE attempt on existing entry
    for (idxMastTbl = 0; idxMastTbl < m_masterTblCnt; ++idxMastTbl)
    {
      if (context->tempTbl[idxTempTbl].KeyField == m_masterTbl[idxMastTbl].KeyField)
      {
        //m_masterTbl[idxMastTbl].PriorValue = context->tempTbl[idxTempTbl].PriorValue;
        //m_masterTbl[idxMastTbl].PriorValid = context->tempTbl[idxTempTbl].PriorValid;
        ++updateCnt;
        bFound = TRUE;
        break;
      }
    } // master lookup

    // INSERT attempt if update failed AND table not reported as full.
    if ( !bFound && !bTableFull )
    {
      if ( m_masterTblCnt < MAX_PRIOR_VALUES )
      {
        m_masterTbl[m_masterTblCnt].KeyField    = context->tempTbl[idxTempTbl].KeyField;
        m_masterTbl[m_masterTblCnt].PriorValue  = context->tempTbl[idxTempTbl].PriorValue;
        m_masterTbl[m_masterTblCnt].PriorValid  = context->tempTbl[idxTempTbl].PriorValid;
        ++m_masterTblCnt;
        ++updateCnt;
      }
      else
      {
        bTableFull = TRUE;
      }
    }

  } // temp processing.

  return (updateCnt == context->tempTblCnt);

}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: Evaluator.c $
 * 
 * *****************  Version 22  *****************
 * User: Contractor V&v Date: 11/13/12   Time: 6:27p
 * Updated in $/software/control processor/code/drivers
 * Code review
 *
 * *****************  Version 21  *****************
 * User: Contractor V&v Date: 11/09/12   Time: 5:35p
 * Updated in $/software/control processor/code/drivers
 * Code Review - coverage gap
 *
 * *****************  Version 20  *****************
 * User: Melanie Jutras Date: 12-10-19   Time: 10:18a
 * Updated in $/software/control processor/code/drivers
 * SCR #1172 PCLint Suspicious use of & Error
 *
 * *****************  Version 19  *****************
 * User: Contractor V&v Date: 12-10-02   Time: 1:23p
 * Updated in $/software/control processor/code/drivers
 * SCR #1107 FAST 2 Coding standard compliance
 *
 * *****************  Version 18  *****************
 * User: Jeff Vahue   Date: 8/31/12    Time: 12:23p
 * Updated in $/software/control processor/code/drivers
 * SCR# 1107 - !P paren position error fix
 *
 * *****************  Version 17  *****************
 * User: Jeff Vahue   Date: 8/29/12    Time: 6:20p
 * Updated in $/software/control processor/code/drivers
 * SCR# 1107 - !P Table Full processing
 *
 * *****************  Version 16  *****************
 * User: Contractor V&v Date: 8/29/12    Time: 2:55p
 * Updated in $/software/control processor/code/drivers
 * SCR #1107 FAST 2 Issue #24 Eval: Not Equal Previous does not work
 *
 * *****************  Version 15  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 14  *****************
 * User: John Omalley Date: 12-08-28   Time: 8:32a
 * Updated in $/software/control processor/code/drivers
 * SCR 1107 - Evaluator Bug Fixes
 *
 * *****************  Version 13  *****************
 * User: Contractor V&v Date: 8/22/12    Time: 5:28p
 * Updated in $/software/control processor/code/drivers
 * FAST 2 Issue #12
 *
 * *****************  Version 12  *****************
 * User: Contractor V&v Date: 8/15/12    Time: 7:21p
 * Updated in $/software/control processor/code/drivers
 * SCR #1107 FAST 2 Fixed Issue #7
 *
 * *****************  Version 11  *****************
 * User: Contractor V&v Date: 8/08/12    Time: 3:12p
 * Updated in $/software/control processor/code/drivers
 * Fixed: Issue #3 eval accepts a 17 token expression
 * Fixed: Issue #4 eval does not end a trigger when a sensor goes invalid
 * Fixed: Issue #7 EVAL OR function and optimized opcode table lookup
 *
 * *****************  Version 10  *****************
 * User: Contractor V&v Date: 7/30/12    Time: 7:29p
 * Updated in $/software/control processor/code/drivers
 * SCR #1107 FAST 2 Fix evaluator
 *
 * *****************  Version 9  *****************
 * User: Contractor V&v Date: 7/18/12    Time: 12:11p
 * Updated in $/software/control processor/code/drivers
 * SCR #1107 FAST 2 fix check for number of tokens
 *
 * *****************  Version 8  *****************
 * User: Contractor V&v Date: 7/11/12    Time: 4:30p
 * Updated in $/software/control processor/code/drivers
 * SCR #1107 FAST 2  Support empty end criteria
 *
 * *****************  Version 7  *****************
 * User: Contractor V&v Date: 6/25/12    Time: 7:12p
 * Updated in $/software/control processor/code/drivers
 * SCR #1107 Add FALSE boolean constant
 *
 * *****************  Version 6  *****************
 * User: Contractor V&v Date: 6/18/12    Time: 4:27p
 * Updated in $/software/control processor/code/drivers
 * FAST 2 reorder RPN operands
 *
 * *****************  Version 5  *****************
 * User: Contractor V&v Date: 4/27/12    Time: 4:03p
 * Updated in $/software/control processor/code/drivers
 ***************************************************************************/
