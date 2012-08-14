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
  $Revision: 11 $  $Date: 8/08/12 3:12p $

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

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define MAX_FP_STRING 32
#define MAX_FALSE_STRING 5


#define RPN_PUSH(obj) (rpn_stack[rpn_stack_pos++] = (obj))
#define RPN_POP       (rpn_stack[--rpn_stack_pos])
#define RPN_STACK_CNT (rpn_stack_pos)


//#define DEBUG_EVALUATOR
/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/


// Keep this list in sync with enum RPN_ERR
const CHAR* EvalRetValEnumString[10] =
{
  "RPN_ERR_UNKNOWN"                  ,
  "RPN_ERR_INDEX_NOT_NUMERIC"        ,
  "RPN_ERR_VALUE_INVALID"            ,
  "RPN_ERR_INDEX_OTRNG"              ,
  "RPN_ERR_TOO_MANY_OPRNDS"          ,
  "RPN_ERR_OP_REQUIRES_SENSOR_OPRND" ,
  "RPN_ERR_CONST_VALUE_OTRG"         ,
  "RPN_ERR_TOO_FEW_STACK_VARS"       ,
  "RPN_ERR_INV_OPRND_TYPE"           ,
  "RPN_ERR_TOO_MANY_TOKENS_IN_EXPR"
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
const EVAL_DATAACCESS DataAccessTable[EVAL_DAI_MAX] = {EVAL_DAI_LIST};

// Evaluation rpn stack used at runtime
EVAL_RPN_ENTRY rpn_stack[EVAL_EXPR_BIN_LEN];
INT32          rpn_stack_pos = 0;

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static const EVAL_DATAACCESS* EvalGetDataAccess(BYTE  OpCode);

static INT32   EvalParseObjectIndex( const CHAR* str, INT32 maxIndex);
static INT8    EvalGetValidCnt     ( const EVAL_RPN_ENTRY* operandA,
                                     const EVAL_RPN_ENTRY* operandB);
static BOOLEAN EvalVerifyDataType  ( DATATYPE expectedType, const EVAL_RPN_ENTRY* opA,
                                     const EVAL_RPN_ENTRY* opB);

/*********************************************************************************************/
/* Local Functions                                                                           */
/*********************************************************************************************/

/******************************************************************************
 * Function:      EvalGetDataAccess
 *
 * Description:
 *
 * Parameters:    [in]
 *
 *                [out]
 *
 * Returns:
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

  for(i = 0; !bFound  && i < EVAL_DAI_MAX; ++i)
  {
    if( OpCode == DataAccessTable[i].OpCode)
    {
      pDataAcc = (const EVAL_DATAACCESS*) &DataAccessTable[i];
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
 * Parameters:    [in]
 *
 *                [out]
 *
 * Returns:
 *
 * Notes:
 *
 *
 *****************************************************************************/
#define BASE_10 10
static INT32 EvalParseObjectIndex( const CHAR* str, INT32 maxIndex)
{
  INT32  temp_int;
  UINT32 len;
  INT32  retVal;
  CHAR*  end;

  // The index must start at str and be left-padded, 3 character wide
  // (000 -> 255)

  if ( !isdigit(*str) )
  {
    retVal = RPN_ERR_INDEX_NOT_NUMERIC;
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
 * Parameters:    [in]
 *
 *                [out]
 *
 * Returns:
 *
 * Notes:
 *
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


/*********************************************************************************************/
/* Public  Function                                                                          */
/*********************************************************************************************/

/******************************************************************************
 * Function:      EvalGetMsgFromErrCode
 *
 * Description:
 *
 * Parameters:    [in]
 *
 *                [out]
 *
 * Returns:
 *
 * Notes:
 *
 *
 *****************************************************************************/
const CHAR* EvalGetMsgFromErrCode(INT32 errNum)
{
  // Check index range.
  // Invert the negative errNum into the positive index into the string table.
  INT32 i = (errNum < RPN_ERR_MAX || errNum >= RPN_ERR_UNKNOWN) ? RPN_ERR_UNKNOWN : -errNum;
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
 * Parameters:  [in] evalType: enum indicating the type of evaluation to perform. *
 *              [in] expr:     The expression object to be parsed.
 *               [in/out]      The EVAL_RESP_INFO structure containing context-sensitive
 *                             user request/response data to be passed from the user to the
 *                             user callback function thru the evaluator function.
 *
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
INT32 EvalExeExpression (const EVAL_EXPR* expr, BOOLEAN* validity)
{

  INT32   i;
  const EVAL_OPCODE_TBL_ENTRY* pOpCodeTbl = NULL;
  const EVAL_CMD* cmd;
  EVAL_RPN_ENTRY  result;
#ifdef DEBUG_EVALUATOR
  /*vcast_dont_instrument_start*/
  CHAR expString[256];
  EvalExprBinToStr(expString,expr);
  /*vcast_dont_instrument_end*/
#endif

  // Init the results
  result.DataType = DATATYPE_BOOL;
  result.Data     = 0.f;
  result.Validity = TRUE;

  // Init the stack to empty
  rpn_stack_pos = 0;


  for( i = 0; i < expr->Size; i++ )
  {
    // Get ptr to next cmd in list.
    cmd = &expr->CmdList[i];

    // The OpCode value is the index into the OpCodeTable for this operation
    // see: EVAL_OPCODE_LIST in EvaluatorInterface.h
    pOpCodeTbl = &OpCodeTable[cmd->OpCode]; 

    // process the command.
    if ( !pOpCodeTbl->ExeCmd(cmd))
    {
      // Something failed, break out.
      // the most recent stack-push entry has details.
      break;
    }
  } // for-loop cmd list

  // Pop the 'final' entry.
  result = RPN_POP;
  *validity = result.Validity;
  return (INT32)result.Data;
}

/******************************************************************************
 * Function:    EvalExprStrToBin
 *
 * Description: Convert a evaluation expression from string
 *              form (i.e that with 4-char names and !,&,| operators) to a
 *              binary form used at runtime.
 *
 * todo DaveB update this
 * Parameters:  [in] str:      String of transition criteria to convert with the
 *                             form of "[6-char TC][Operator][6-char TC]....."
 *              [in/out] expr: Ptr to structure to hold the converted rpn string and
 *                              meta-data needed during runtime to evaluate the expression
 *              [in]     type: Type of object (e.g. TRIGGER  EVENT, etc)
 *              [in]    index: Obj index ( e.g. Trigger[0],  Event[1])
 *
 *              [in] maxOperands: The maximum allowable operands to be allowed for this
 *                                type.
 *              [in] pGetFunc:    Pointer to the callback function to be used at runtime
 *                                to evaluate the validity of the operands in expression.
 *
 * Returns:
 *              EVAL_RETVAL_TRUE                   1
 *              EVAL_RETVAL_FALSE                  0
 *              --------------------------------------------------------------------
 *
 * Notes:
 *
 *****************************************************************************/

INT32 EvalExprStrToBin( CHAR* str, EVAL_EXPR* expr, UINT8 maxOperands)
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
      retval = RPN_ERR_TOO_MANY_TOKENS_IN_EXPR;
      break;
    }

    len = 0;
    // If the token matches a token in table, call the associated
    // function to add a command to the expression's cmd-list.
    for ( i = 0; i < EVAL_OPCODE_MAX; ++i )
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
      len = EvalAddConst(OP_CONST_VAL, str, expr );
    }
    // Still no luck parsing... its an unrecognized operand.
    if(0 == len)
    {
      retval = RPN_ERR_VALUE_INVALID;
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
    retval = EvalExeExpression( expr, &temp);
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
  INT32   i;
  INT32   j;
  INT32   cnt;
  CHAR*   pStr = str;

  //Clear str
  *str = '\0';

  for(i = 0; i < expr->Size; i++)
  {
    cnt  = 0;   
    j = expr->CmdList[i].OpCode;
    cnt = OpCodeTable[j].FmtCmd( j, (const EVAL_CMD*) &expr->CmdList[i], pStr);
    pStr += cnt;
    *pStr = ' ';
    pStr++;

    //Should never happen unless the op code table is corrupted!
    ASSERT_MESSAGE(cnt != 0, "EvalExprBinToStr OpCode not valid: %d",expr->CmdList[i].OpCode );
  } // for-loop expression entries
  --pStr;
  *pStr = '\0';
}

/******************************************************************************
 * Function:      EvalLoadConstValue
 *
 * Description:
 *
 * Parameters:    [in]
 *
 *                [out]
 *
 * Returns:
 *
 * Notes:
 *
 *
 *****************************************************************************/
BOOLEAN EvalLoadConstValue(const EVAL_CMD* cmd)
{
  // A constant is always valid,
  EVAL_RPN_ENTRY rslt;

  rslt.Data = cmd->Data;
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
 * Parameters:    [in]
 *
 *                [out]
 *
 * Returns:
 *
 * Notes:
 *
 *
 *****************************************************************************/
BOOLEAN EvalLoadConstFalse(const EVAL_CMD* cmd)
{
  // A constant is always valid,
  EVAL_RPN_ENTRY rslt;

  rslt.Data = cmd->Data;
  rslt.DataType = DATATYPE_BOOL;
  rslt.Validity = TRUE;

  RPN_PUSH(rslt);
  return TRUE;
}

/******************************************************************************
 * Function:      EvalLoadInputSrc
 *
 * Description:
 *
 * Parameters:    [in]
 *
 *                [out]
 *
 * Returns:
 *
 * Notes:
 *
 *
 *****************************************************************************/
BOOLEAN EvalLoadInputSrc(const EVAL_CMD* cmd)
{
  EVAL_RPN_ENTRY  rslt;
  const EVAL_DATAACCESS* dataAcc;

  // Get a pointer to the DataAccess table for the set of
  // function-pointers supporting this opCode.

  dataAcc = EvalGetDataAccess(cmd->OpCode);
  if (NULL != dataAcc)
  {
    // Only 1 Get function should be defined in table for each opCode
    // Should never be true. (Table 'coding' error)
    ASSERT_MESSAGE(!((dataAcc->GetSrcByBool == NULL) && (dataAcc->GetSrcByValue == NULL)) &&
                   !((dataAcc->GetSrcByBool != NULL) && (dataAcc->GetSrcByValue != NULL)),
                   "Multiple 'Get' entries configured for DataAccessTable[%d] Cmd: [%d][%f]",
                    cmd->OpCode, cmd->OpCode, cmd->Data);

    // Init the data-type and data here.
    // If the source is not configured we need at least
    // this much info for the stack push.
    rslt.DataType = (NULL != dataAcc->GetSrcByValue) ? DATATYPE_VALUE : DATATYPE_BOOL;
    rslt.Data = 0.f;

    // If the input source is configured, get the data (value or bool) and validity
    if (dataAcc->IsSrcConfigured( (INT32)cmd->Data) )
    {
      rslt.Data = (NULL != dataAcc->GetSrcByValue) ?
                    dataAcc->GetSrcByValue( (INT32) cmd->Data) :
                    dataAcc->GetSrcByBool ( (INT32) cmd->Data);

      rslt.Validity = dataAcc->GetSrcValidity( (SENSOR_INDEX)cmd->Data);
    }
    else // The indicated data source is not configured.
    {
      rslt.Validity = FALSE;
    }
  }
  else
  {
    // Should never get here! (Table 'coding' error)
    FATAL("Data Access lookup not configured for OpCode: %d",cmd->OpCode );
  }

  RPN_PUSH(rslt);
  return TRUE;
}

/******************************************************************************
 * Function: EvalCompareOperands
 *
 * Description:
 *
 * Parameters:    [in]
 *
 *                [out]
 *
 * Returns:
 *
 * Notes:
 *
 *
 *****************************************************************************/
BOOLEAN EvalCompareOperands(const EVAL_CMD* cmd)
{
  BOOLEAN opStatus = TRUE;
  EVAL_RPN_ENTRY oprndLeft;
  EVAL_RPN_ENTRY oprndRight;
  EVAL_RPN_ENTRY rslt;

  // Need two operands off stack
  if(RPN_STACK_CNT < 2)
  {
    rslt.Data     = RPN_ERR_TOO_FEW_STACK_VARS;
    rslt.DataType = DATATYPE_RPN_PROC_ERR;
    rslt.Validity = FALSE;
    opStatus      = FALSE;
  }
  else
  {
    oprndRight = RPN_POP;
    oprndLeft  = RPN_POP;
    if ( EvalVerifyDataType(DATATYPE_VALUE, &oprndLeft, &oprndRight) )
    {
        // perform the comparison operation, store the result
        switch (cmd->OpCode)
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
            FATAL("Unrecognized Expression OpCode: %d", cmd->OpCode);
        }

      rslt.DataType = DATATYPE_BOOL;
      rslt.Validity = ( EvalGetValidCnt(&oprndLeft, &oprndRight ) == 2 );
    }
    else
    {
      // Set RPN response to flag a syntax error
      rslt.Data     = RPN_ERR_INV_OPRND_TYPE;
      rslt.DataType = DATATYPE_RPN_PROC_ERR;
      rslt.Validity = FALSE;
      opStatus      = FALSE;
    }
  }
  RPN_PUSH(rslt);
  return opStatus;
}

/******************************************************************************
 * Function: EvalIsNotEqualPrev
 *
 * Description:
 *
 * Parameters:    [in]
 *
 *                [out]
 *
 * Returns:
 *
 * Notes:
 *
 *
 *****************************************************************************/
BOOLEAN EvalIsNotEqualPrev(const EVAL_CMD* cmd)
{
  BOOLEAN opStatus = TRUE;
  EVAL_RPN_ENTRY oprndCurrent;
  EVAL_RPN_ENTRY oprndPrevious;
  EVAL_RPN_ENTRY rslt;

  // Need one operand off stack
  if(RPN_STACK_CNT < 1)
  {
    rslt.Data     = RPN_ERR_TOO_FEW_STACK_VARS;
    rslt.DataType = DATATYPE_RPN_PROC_ERR;
    rslt.Validity = FALSE;
  }
  else
  {
    oprndCurrent = RPN_POP;
    if ( EvalVerifyDataType(DATATYPE_VALUE, &oprndCurrent, NULL) )
    {
      // Fetch the previous sensor and make an R-side operand for the compare
      oprndPrevious.Data     = SensorGetPreviousValue((SENSOR_INDEX) cmd->Data);
      oprndPrevious.DataType = DATATYPE_VALUE;
      oprndPrevious.Validity = SensorGetPreviousValid((SENSOR_INDEX) cmd->Data);

      rslt.Data    = (FLOAT32)(fabs( (oprndCurrent.Data - oprndPrevious.Data) >= FLT_EPSILON ));
      rslt.DataType = DATATYPE_BOOL;
      rslt.Validity = ( EvalGetValidCnt(&oprndCurrent, &oprndPrevious ) == 2 );
    }
    else
    {
      // Set RPN response to flag a syntax error
      rslt.Data     = RPN_ERR_INV_OPRND_TYPE;
      rslt.DataType = DATATYPE_RPN_PROC_ERR;
      rslt.Validity = FALSE;
      opStatus      = FALSE;
    }
  }
  RPN_PUSH(rslt);
  return opStatus;
}


/******************************************************************************
 * Function: EvalPerformNot
 *
 * Description:
 *
 * Parameters:    [in]
 *
 *                [out]
 *
 * Returns:
 *
 * Notes:
 *
 *
 *****************************************************************************/
BOOLEAN EvalPerformNot(const EVAL_CMD* cmd)
{
  BOOLEAN opStatus = TRUE;
  EVAL_RPN_ENTRY oprnd;
  EVAL_RPN_ENTRY rslt;

  // Need one operand off stack
  if(RPN_STACK_CNT < 1)
  {
    rslt.Data     = RPN_ERR_TOO_FEW_STACK_VARS;
    rslt.DataType = DATATYPE_RPN_PROC_ERR;
    rslt.Validity = FALSE;
  }
  else
  {
    oprnd = RPN_POP;
    if ( EvalVerifyDataType(DATATYPE_BOOL, &oprnd, NULL) )
    {
      rslt.Data    = (FLOAT32) !(BOOLEAN)oprnd.Data;
      rslt.DataType = DATATYPE_BOOL;
      rslt.Validity = oprnd.Validity;
    }
    else
    {
      // Set RPN response to flag a syntax error
      rslt.Data     = RPN_ERR_INV_OPRND_TYPE;
      rslt.DataType = DATATYPE_RPN_PROC_ERR;
      rslt.Validity = FALSE;
      opStatus      = FALSE;
    }
  }
  RPN_PUSH(rslt);
  return opStatus;
}

/******************************************************************************
 * Function: EvalPerformAnd
 *
 * Description:
 *
 * Parameters:    [in]
 *
 *                [out]
 *
 * Returns:
 *
 * Notes:
 *
 *
 *****************************************************************************/
BOOLEAN EvalPerformAnd(const EVAL_CMD* cmd)
{
  BOOLEAN opStatus = TRUE;
  EVAL_RPN_ENTRY oprndLeft;
  EVAL_RPN_ENTRY oprndRight;
  EVAL_RPN_ENTRY rslt;

  // Need two operands off stack
  if(RPN_STACK_CNT < 2)
  {
    rslt.Data     = RPN_ERR_TOO_FEW_STACK_VARS;
    rslt.DataType = DATATYPE_RPN_PROC_ERR;
    rslt.Validity = FALSE;
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
      rslt.Data     = RPN_ERR_INV_OPRND_TYPE;
      rslt.DataType = DATATYPE_RPN_PROC_ERR;
      rslt.Validity = FALSE;
      opStatus      = FALSE;
    }
  }
  RPN_PUSH(rslt);
  return opStatus;
}

/******************************************************************************
 * Function: EvalPerformOr
 *
 * Description:
 *
 * Parameters:    [in]
 *
 *                [out]
 *
 * Returns:
 *
 * Notes:
 *
 *
 *****************************************************************************/
BOOLEAN EvalPerformOr(const EVAL_CMD* cmd)
{
  BOOLEAN opStatus = TRUE;
  EVAL_RPN_ENTRY oprndLeft;
  EVAL_RPN_ENTRY oprndRight;
  EVAL_RPN_ENTRY rslt;

  // Need two operands off stack
  if(RPN_STACK_CNT < 2)
  {
    rslt.Data     = RPN_ERR_TOO_FEW_STACK_VARS;
    rslt.DataType = DATATYPE_RPN_PROC_ERR;
    rslt.Validity = FALSE;
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
      rslt.Data     = RPN_ERR_INV_OPRND_TYPE;
      rslt.DataType = DATATYPE_RPN_PROC_ERR;
      rslt.Validity = FALSE;
      opStatus      = FALSE;
    }
  }
  RPN_PUSH(rslt);
  return opStatus;
}


/******************************************************************************
 * Function: EvalAddConst
 *
 * Description:
 *
 * Parameters:    [in]
 *
 *                [out]
 *
 * Returns:
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
    retval = RPN_ERR_TOO_MANY_OPRNDS;
  }
  else
  {
    // Try to process as a const value. Treat as floating-point
    temp_float = (FLOAT32)strtod(str, &end);
    if( temp_float <= FLT_MAX && temp_float >= -FLT_MAX)
    {
      cmd.OpCode = (UINT8)OP_CONST_VAL;
      cmd.Data   = temp_float;

      // Add cmd to expression list
      expr->CmdList[expr->Size++] = cmd;
      expr->OperandCnt++;
      retval = end - str;
    }
    else
    {
      retval = RPN_ERR_CONST_VALUE_OTRG;
    }
  }
  return retval;
}

/******************************************************************************
 * Function: EvalAddFalseStr
 *
 * Description: Add a "load const bool FALSE" to the cmd list.
 *
 * Parameters:    [in]
 *
 *                [out]
 *
 * Returns:
 *
 * Notes: This cmd is used to represent a NO_COMPARE conditions which
 *        to False
 *
 *
 *****************************************************************************/
INT32 EvalAddFalseStr(INT16 tblIdx, const CHAR* str, EVAL_EXPR* expr)
{
  EVAL_CMD cmd;
  INT32    retval;

  // Have we reached limit count for operands?
  if(expr->OperandCnt == expr->MaxOperands)
  {
    retval = RPN_ERR_TOO_MANY_OPRNDS;
  }
  else
  {
    // Object is a constant Boolean 'FALSE'
    cmd.OpCode = (UINT8)OP_CONST_FALSE;
    cmd.Data   = 0.f;
    
    // Add cmd to expression list
    expr->CmdList[expr->Size++] = cmd;
    expr->OperandCnt++;
    retval = MAX_FALSE_STRING;
  }
  return retval;
}

/******************************************************************************
 * Function: EvalAddInputSrc
 *
 * Description:
 *
 * Parameters:    [in]
 *
 *                [out]
 *
 * Returns:
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
    retval = RPN_ERR_TOO_MANY_OPRNDS;
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
    
    objIdx = EvalParseObjectIndex( &str[2], maxObjects);
    if(objIdx >= 0)
    {
      cmd.OpCode = (UINT8)OpCodeTable[tblIdx].OpCode;
      cmd.Data   = (FLOAT32)objIdx;
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
 * Parameters:    [in]
 *
 *                [out]
 *
 * Returns:
 *
 * Notes:
 *
 *
 *****************************************************************************/
INT32 EvalAddStdOper(INT16 tblIdx, const CHAR* str, EVAL_EXPR* expr)
{
  EVAL_CMD cmd;

  cmd.OpCode = OpCodeTable[tblIdx].OpCode;
  cmd.Data   = 0.f;

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
 * Parameters:    [in]
 *
 *                [out]
 *
 * Returns:
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
  if ( OP_GETSNRVAL == expr->CmdList[expr->Size - 1].OpCode)
  {
    cmd.OpCode = OP_NOT_EQ_PREV;
    cmd.Data   = expr->CmdList[expr->Size - 1].Data;
    expr->CmdList[expr->Size++] = cmd;
    retval = OpCodeTable[tblIdx].TokenLen;
  }
  else
  {
    retval = RPN_ERR_OP_REQUIRES_SENSOR_OPRND;
  }
  return retval;
}

/******************************************************************************
 * Function: EvalFmtLoadCmdStr
 *
 * Description:
 *
 * Parameters:    [in]
 *
 *                [out]
 *
 * Returns:
 *
 * Notes:
 *
 *
 *****************************************************************************/
INT16 EvalFmtLoadCmdStr (INT16 tblIdx, const EVAL_CMD* cmd, CHAR* str)
{
  return snprintf(str, EVAL_OPRND_LEN + 1, "%s%03d", &OpCodeTable[tblIdx].Token,
                                                     (INT32)cmd->Data);
}

/******************************************************************************
 * Function: EvalFmtLoadConstStr
 *
 * Description:
 *
 * Parameters:    [in]
 *
 *                [out]
 * Returns:
 *
 * Notes:
 *
 *
 *****************************************************************************/
INT16 EvalFmtLoadConstStr(INT16 tblIdx, const EVAL_CMD* cmd, CHAR* str)
{
  return snprintf(str, MAX_FP_STRING, "%f", cmd->Data);
}

/******************************************************************************
 * Function: EvalFmtLoadFalseStr
 *
 * Description: Return a "FALSE" const string.
 *
 * Parameters:    [in]
 *
 *                [out]
 * Returns:
 *
 * Notes:
 *
 *
 *****************************************************************************/
INT16 EvalFmtLoadFalseStr(INT16 tblIdx, const EVAL_CMD* cmd, CHAR* str)
{  
  return snprintf(str, MAX_FALSE_STRING, "FALSE");
}



/******************************************************************************
 * Function: EvalFmtOperStr
 *
 * Description:
 *
 * Parameters:    [in]
 *
 *                [out]
 *
 * Returns:
 *
 * Notes:
 *
 *
 *****************************************************************************/
INT16 EvalFmtOperStr(INT16 tblIdx, const EVAL_CMD* cmd, CHAR* str)
{
  return  snprintf(str, OpCodeTable[tblIdx].TokenLen + 1, "%s", &OpCodeTable[tblIdx].Token);
}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: Evaluator.c $
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
 ************************************************************************/
