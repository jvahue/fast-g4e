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
  $Revision: 28 $  $Date: 12/28/12 5:52p $

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


#define EVAL_MAKE_LOOKUP_KEY(type,obj,snsr) (((type) << 16) | ((obj)  <<  8) | (snsr))
#define EVAL_GET_TYPE(key) (((key) & KEY_MASK_TYPE) >> 16)

#define BASE_10 10

/************************************************************************************/
/* SPECIAL FWD DECL for Functions declared in EVAL_OPCODE_LIST&EVAL_DAI_LIST tables */
/************************************************************************************/
static BOOLEAN EvalLoadConstValue (EVAL_EXE_CONTEXT* context);
static BOOLEAN EvalLoadConstFalse (EVAL_EXE_CONTEXT* context);
static BOOLEAN EvalLoadInputSrc   (EVAL_EXE_CONTEXT* context);
static BOOLEAN EvalLoadFuncCall   (EVAL_EXE_CONTEXT* context);
// Comparison Operators
static BOOLEAN EvalCompareOperands (EVAL_EXE_CONTEXT* context);
static BOOLEAN EvalIsNotEqualPrev  (EVAL_EXE_CONTEXT* context);
// Logical Operators
static BOOLEAN EvalPerformNot      (EVAL_EXE_CONTEXT* context);
static BOOLEAN EvalPerformAnd      (EVAL_EXE_CONTEXT* context);
static BOOLEAN EvalPerformOr       (EVAL_EXE_CONTEXT* context);
// String-to-Cmd Converter functions.
static INT32 EvalAddConst    (INT16 tblIdx, const CHAR* str, EVAL_EXPR* expr);
static INT32 EvalAddFuncCall (INT16 tblIdx, const CHAR* str, EVAL_EXPR* expr);
static INT32 EvalAddInputSrc (INT16 tblIdx, const CHAR* str, EVAL_EXPR* expr);
static INT32 EvalAddStdOper  (INT16 tblIdx, const CHAR* str, EVAL_EXPR* expr);
static INT32 EvalAddNotEqPrev(INT16 tblIdx, const CHAR* str, EVAL_EXPR* expr);
// Cmd-to-String representation converters.
static INT32 EvalFmtLoadEnumeratedCmdStr (INT16 tblIdx, const EVAL_CMD* cmd, CHAR* str);
static INT32 EvalFmtLoadConstStr         (INT16 tblIdx, const EVAL_CMD* cmd, CHAR* str);
static INT32 EvalFmtLoadCmdStr           (INT16 tblIdx, const EVAL_CMD* cmd, CHAR* str);
static INT32 EvalFmtOperStr              (INT16 tblIdx, const EVAL_CMD* cmd, CHAR* str);

// Token -> Opcode lookup table
#undef  OPCMD
#define OPCMD(OpCode, Token, TokenLen, AddCmd, FmtString, ExeCmd)

/*              EVAL_OPCODE_LIST is greater than 95 chars for clarity.                       */

#define EVAL_OPCODE_LIST \
  /*       OpCode       Token   Len  AddCmd             FmtString                      ExeCmd */\
  /* Load/Fetch commands                                                                      */\
  /* Operands: KEEP LIST IN SAME ORDER AS EVAL_DAI_LIST BELOW. LOOKUPS USE OPCMD AS INDEXES   */\
  OPCMD(OP_GETSNRVAL,   "SVLU",  4, EvalAddInputSrc,    EvalFmtLoadEnumeratedCmdStr,   EvalLoadInputSrc    ),\
  OPCMD(OP_GETSNRVALID, "SVLD",  4, EvalAddInputSrc,    EvalFmtLoadEnumeratedCmdStr,   EvalLoadInputSrc    ),\
  OPCMD(OP_GETTRIGVAL,  "TACT",  4, EvalAddInputSrc,    EvalFmtLoadEnumeratedCmdStr,   EvalLoadInputSrc    ),\
  OPCMD(OP_GETTRIGVALID,"TVLD",  4, EvalAddInputSrc,    EvalFmtLoadEnumeratedCmdStr,   EvalLoadInputSrc    ),\
  OPCMD(OP_CALL_ACTACK, "AACK",  4, EvalAddFuncCall,    EvalFmtLoadCmdStr,             EvalLoadFuncCall    ),\
  OPCMD(OP_CONST_FALSE, "FALSE", 5, EvalAddFuncCall,    EvalFmtLoadCmdStr,             EvalLoadConstFalse  ),\
  OPCMD(OP_CONST_VAL,   "",      0, EvalAddConst,       EvalFmtLoadConstStr,           EvalLoadConstValue  ),\
  /* Operators: KEEP LIST IN DESCENDING SIZE ORDER ( "!=" MUST BE FOUND BEFORE "!" )                       */\
  OPCMD(OP_NE,          "!=",    2, EvalAddStdOper,     EvalFmtOperStr,                EvalCompareOperands ),\
  OPCMD(OP_EQ,          "==",    2, EvalAddStdOper,     EvalFmtOperStr,                EvalCompareOperands ),\
  OPCMD(OP_NOT_EQ_PREV, "!P",    2, EvalAddNotEqPrev,   EvalFmtOperStr,                EvalIsNotEqualPrev  ),\
  OPCMD(OP_GE,          ">=",    2, EvalAddStdOper,     EvalFmtOperStr,                EvalCompareOperands ),\
  OPCMD(OP_LE,          "<=",    2, EvalAddStdOper,     EvalFmtOperStr,                EvalCompareOperands ),\
  OPCMD(OP_GT,          ">",     1, EvalAddStdOper,     EvalFmtOperStr,                EvalCompareOperands ),\
  OPCMD(OP_LT,          "<",     1, EvalAddStdOper,     EvalFmtOperStr,                EvalCompareOperands ),\
  OPCMD(OP_NOT,         "!",     1, EvalAddStdOper,     EvalFmtOperStr,                EvalPerformNot      ),\
  OPCMD(OP_AND,         "&",     1, EvalAddStdOper,     EvalFmtOperStr,                EvalPerformAnd      ),\
  OPCMD(OP_OR,          "|",     1, EvalAddStdOper,     EvalFmtOperStr,                EvalPerformOr       )


// Data Access Interface table
#undef DAI
#define DAI(OpCode, IsConfiged, RetValueFunc, RetBoolFunc, ValidFunc)

/*              EVAL_DAI_LIST is greater than 95 chars for clarity.                          */

#define EVAL_DAI_LIST \
  /*   NOTE: RetValueFunc and RetBoolFunc are mutually exclusive -DO NOT DECLARE BOTH!                */\
  /*       OpCode      IsConfiged           RetValueFunc    RetBoolFunc           ValidFunc           */\
  DAI(OP_GETSNRVAL,    SensorIsUsed,        SensorGetValue, NULL,                 SensorIsValid       ),\
  DAI(OP_GETSNRVALID,  SensorIsUsed,        NULL,           SensorIsValid,        SensorIsValid       ),\
  DAI(OP_GETTRIGVAL,   TriggerIsConfigured, NULL,           TriggerGetState,      TriggerValidGetState),\
  DAI(OP_GETTRIGVALID, TriggerIsConfigured, NULL,           TriggerValidGetState, TriggerValidGetState),\
  DAI(OP_CALL_ACTACK,  NULL,                NULL,           ActionAcknowledgable, NULL                )

//#define DEBUG_EVALUATOR
/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
// Declare enum from just the list of supported op codes.
#undef  OPCMD
#define OPCMD(OpCode, Token, TokenLen, AddCmd, FmtString, ExeCmd) OpCode
typedef enum
{
  EVAL_OPCODE_LIST,
  EVAL_OPCODE_MAX
} EVAL_OPCODE;

// The following enum is to generate EVAL_DAI_MAX.
// Only a subset of OPCODE need a data access interface so...
// EVAL_DAI_MAX <= EVAL_OPCODE_MAX

#undef DAI
#define DAI(OpCode, IsConfiged, RetValueFunc, RetBoolFunc, ValidFunc) OpCode##_DAI
typedef enum
{
  EVAL_DAI_LIST,
  EVAL_DAI_MAX
}EVAL_DAI;

typedef enum
{
  DATATYPE_VALUE = 1,
  DATATYPE_BOOL,
  DATATYPE_RPN_PROC_ERR
}DATATYPE;

typedef struct
{
  DATATYPE dataType;
  FLOAT32  data;
  BOOLEAN  validity;
}EVAL_RPN_ENTRY;

// Typedef to function pointers.
typedef INT32   ADD_CMD (INT16 tblIdx, const CHAR* str, EVAL_EXPR* expr);
typedef INT32   FMT_CMD (INT16 tblIdx, const EVAL_CMD* cmd, CHAR* str);
typedef BOOLEAN OP_CMD  (EVAL_EXE_CONTEXT* context);

typedef struct
{
  BYTE     opCode;
  CHAR     token[EVAL_OPRND_LEN+1];
  UINT8    tokenLen;
  ADD_CMD* pfAddCmd;        // Ptr to function STR -> CMD obj
  FMT_CMD* pfFmtCmd;        // Ptr to function CMDobj -> STR
  OP_CMD*  pfExeCmd;        // Ptr to function to execute CMD
}EVAL_OPCODE_TBL_ENTRY;

// Data Access Interface
typedef FLOAT32 GET_VALUE_FUNC( INT32 objIndex );
typedef BOOLEAN GET_BOOL_FUNC ( INT32 objIndex );

typedef struct
{
  BYTE opCode;                        // Lookup-key from EVAL_OPCODE_TBL_ENTRY
  GET_BOOL_FUNC*  pfIsSrcConfigured;  // Return is input source configured?
  GET_VALUE_FUNC* pfGetSrcByValue;    // Return source input data as FP number.
  GET_BOOL_FUNC*  pfGetSrcByBool;     // Return source input data as boolean.
  GET_BOOL_FUNC*  pfGetSrcValidity;   // Return source validity
}EVAL_DATAACCESS;

// Note: Keep this enum in sync with EvalRetValEnumString[]
typedef enum
{
  RPN_ERR_UNKNOWN                  =  0,  // place unused.
  RPN_ERR_INDEX_NOT_NUMERIC        = -1,  // Unrecognized operand index
  RPN_ERR_INVALID_TOKEN            = -2,  // Unrecognized operand input name
  RPN_ERR_INDEX_OTRNG              = -3,  // Index out of range
  RPN_ERR_TOO_MANY_OPRNDS          = -4,  // Too many operands in the expression.
  RPN_ERR_OP_REQUIRES_SENSOR_OPRND = -5,  // Operation is invalid on this operand
  RPN_ERR_CONST_VALUE_OTRG         = -6,  // Const value out of range for FLOAT32
  RPN_ERR_TOO_FEW_STACK_VARS       = -7,  // Operation requires more vars on stack than present
  RPN_ERR_INV_OPRND_TYPE           = -8,  // One or more operands are invalid for operation
  RPN_ERR_TOO_MANY_TOKENS_IN_EXPR  = -9,  // Too many tokens to fit on stack
  RPN_ERR_TOO_MANY_STACK_VARS      = -10, // Too many stack vars were present at end of eval
  RPN_ERR_NOT_PREV_TABLE_FULL      = -11, // The table storing Prior-sensor values is full.
  //-----
  RPN_ERR_MAX                      = -12
}RPN_ERR;

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/


// Keep this list in sync with enum RPN_ERR
static const CHAR* m_EvalRetValEnumString[12] =
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
static const EVAL_OPCODE_TBL_ENTRY m_OpCodeTable[EVAL_OPCODE_MAX] = {EVAL_OPCODE_LIST};


//  DataAccessTable[] allocation and initialization
#undef DAI
#define DAI(OpCode, IsConfigured, RetValueFunc, RetBoolFunc, ValidFunc) \
           {OpCode##_DAI, IsConfigured, RetValueFunc, RetBoolFunc, ValidFunc}
static const EVAL_DATAACCESS m_DataAccessTable[EVAL_DAI_MAX] = {EVAL_DAI_LIST};

// Evaluator is not reentrant! see EvalExeExpression

// Evaluation rpn stack used at runtime
static EVAL_RPN_ENTRY rpn_stack[EVAL_EXPR_BIN_LEN];
static INT32          rpn_stack_pos = 0;

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
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:      EvalGetMsgFromErrCode
 *
 * Description:   Returns a pointer to the string equivalent of the enum name
 *                for each ErrCode supported by the Evaluator
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
  return m_EvalRetValEnumString[i];
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
 * Notes:   This function SHOULD ONLY BE INVOKED by DT tasks as it is designed to be
 *          non-reentrant(rpn_stack, rpn_stack_pos integrity,m_masterTbl and m_masterTblCnt)
 *
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
  result.dataType = DATATYPE_BOOL;
  result.data     = 0.f;
  result.validity = TRUE;

  // Init the stack to empty
  rpn_stack_pos = 0;

  // Process the commands in this expression.
  for( i = 0; i < expr->size; i++ )
  {
    // Get a ptr to next cmd in list.
    cmd = &expr->cmdList[i];

    // The OpCode value is the index into the OpCodeTable for this operation
    // see: EVAL_OPCODE_LIST in EvaluatorInterface.h
    pOpCodeTbl = &m_OpCodeTable[cmd->opCode];

    // Set the current command ptr into the context struct
    // and process it.
    context.cmd = cmd;
    if ( !pOpCodeTbl->pfExeCmd( &context ) )
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
      result.data     = (INT32) RPN_ERR_NOT_PREV_TABLE_FULL;
      result.dataType = DATATYPE_RPN_PROC_ERR;
      result.validity = FALSE;
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
    result.data     = (INT32) RPN_ERR_TOO_MANY_STACK_VARS;
    result.dataType = DATATYPE_RPN_PROC_ERR;
    result.validity = FALSE;
    RPN_PUSH(result);

  }

  // Pop the 'final' entry.
  result = RPN_POP;
  *validity = result.validity;
  return (INT32)result.data;
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
  expr->size        = 0;
  expr->maxOperands = maxOperands; // Maximum # operands for this type
  expr->operandCnt = 0;
  memset( expr->cmdList, 0, EVAL_EXPR_BIN_LEN );

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
    if(expr->size >= EVAL_EXPR_BIN_LEN)
    {
      retval = (INT32) RPN_ERR_TOO_MANY_TOKENS_IN_EXPR;
      break;
    }

    len = 0;
    // If the token matches a token in table, call the associated
    // function to add a command to the expression's cmd-list.
    for ( i = 0; i < (INT16) EVAL_OPCODE_MAX; ++i )
    {
      if( 0 != m_OpCodeTable[i].tokenLen  &&
          0 == strncmp(m_OpCodeTable[i].token, str, m_OpCodeTable[i].tokenLen))
      {
        len = m_OpCodeTable[i].pfAddCmd(i, str, expr);
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
  if((retval >= 0 && expr->size != 0))
  {
    retval = EvalExeExpression( objType, objID, expr, &temp);
  }

  // If expression is invalid, disable it
  if (retval < 0)
  {
    expr->size = 0;
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


  for(i = 0; i < expr->size; i++)
  {
    j = expr->cmdList[i].opCode;
    cnt = m_OpCodeTable[j].pfFmtCmd( j, (const EVAL_CMD*) &expr->cmdList[i], pStr);
    pStr += cnt;
    *pStr = ' ';
    pStr++;

    //Should never happen unless the op code table is corrupted!
    ASSERT_MESSAGE(cnt != 0, "EvalExprBinToStr OpCode not valid: %d",expr->cmdList[i].opCode );
  } // for-loop expression entries
  --pStr;
  *pStr = '\0';
}


/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:      EvalGetDataAccess
 *
 * Description:   Returns the pointer to the function used to return the data for
 *                for this type of operation.
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
    if( OpCode == m_DataAccessTable[i].opCode)
    {
      pDataAcc = (const EVAL_DATAACCESS*) &m_DataAccessTable[i];
      bFound = TRUE;
    }
  }
  return pDataAcc;
}

/******************************************************************************
 * Function:      EvalParseObjectIndex
 *
 * Description:   Returns the index value for the passed object.
 *                (e.g. for "TACT023", the function will return the integer value 23
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
 * Description:   Returns the # of passed operands which are in the valid state.
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
    cnt  += operandA->validity ? 1 : 0;
  }
  // If operandB was passed, check it's validity
  if (NULL != operandB)
  {
    cnt  += operandB->validity ? 1 : 0;
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

  if( opA->dataType != expectedType ||
    !(NULL == opB) && opB->dataType != expectedType )
  {
    bState = FALSE;
  }
  // return the result of the check.
  return bState;
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
static BOOLEAN EvalLoadConstValue( EVAL_EXE_CONTEXT* context )
{
  // A constant is always valid,
  EVAL_RPN_ENTRY rslt;


  rslt.data = context->cmd->data;
  rslt.dataType = DATATYPE_VALUE;
  rslt.validity = TRUE;

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
static BOOLEAN EvalLoadConstFalse( EVAL_EXE_CONTEXT* context)
{
  // A constant is always valid,
  EVAL_RPN_ENTRY rslt;

  rslt.data = context->cmd->data;
  rslt.dataType = DATATYPE_BOOL;
  // Const FALSE is always VALID
  rslt.validity = TRUE;

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
static BOOLEAN EvalLoadInputSrc( EVAL_EXE_CONTEXT* context)
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
  ASSERT_MESSAGE(!((dataAcc->pfGetSrcByBool == NULL) && (dataAcc->pfGetSrcByValue == NULL)) &&
                 !((dataAcc->pfGetSrcByBool != NULL) && (dataAcc->pfGetSrcByValue != NULL)),
                 "Multiple 'Get' entries configured for DataAccessTable[%d] Cmd: [%d][%f]",
                  cmd->opCode, cmd->opCode, cmd->data);

  // Init the data-type and data here.
  // If the source is not configured we need at least
  // this much info for the stack push.
  rslt.dataType = (NULL != dataAcc->pfGetSrcByValue) ? DATATYPE_VALUE : DATATYPE_BOOL;
  rslt.data = 0.f;

  // If the input source is configured, get the data (value or bool) and validity
  if (dataAcc->pfIsSrcConfigured( (INT32)cmd->data) )
  {
    rslt.data = (NULL != dataAcc->pfGetSrcByValue) ?
                  dataAcc->pfGetSrcByValue( (INT32) cmd->data) :
                  dataAcc->pfGetSrcByBool ( (INT32) cmd->data);

    rslt.validity = dataAcc->pfGetSrcValidity( (INT32)cmd->data);
  }
  else // The indicated data source is not configured.
  {
    rslt.validity = FALSE;
  }

  // If the input source is an invalid boolean operand (e.g trigger), treat the value as false
  if (DATATYPE_BOOL == rslt.dataType && FALSE == rslt.validity)
  {
    rslt.data = FALSE;
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
static BOOLEAN EvalLoadFuncCall ( EVAL_EXE_CONTEXT* context)
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
  ASSERT_MESSAGE(!((dataAcc->pfGetSrcByBool == NULL) && (dataAcc->pfGetSrcByValue == NULL)) &&
                 !((dataAcc->pfGetSrcByBool != NULL) && (dataAcc->pfGetSrcByValue != NULL)),
                 "Multiple 'Get' entries configured for DataAccessTable[%d] Cmd: [%d][%f]",
                 cmd->opCode, cmd->opCode, cmd->data);


  // Call the app-supplied function thru the DAI table ptr
  rslt.dataType = (NULL != dataAcc->pfGetSrcByValue) ? DATATYPE_VALUE : DATATYPE_BOOL;

  rslt.data     = (NULL != dataAcc->pfGetSrcByValue) ?
                   dataAcc->pfGetSrcByValue( (INT32) cmd->data) :
                   dataAcc->pfGetSrcByBool ( (INT32) cmd->data);

 // A app-supplied function is always considered VALID
 rslt.validity = TRUE;

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
static BOOLEAN EvalCompareOperands( EVAL_EXE_CONTEXT* context)
{
  EVAL_RPN_ENTRY oprndLeft;
  EVAL_RPN_ENTRY oprndRight;
  EVAL_RPN_ENTRY rslt;
  const EVAL_CMD* cmd = context->cmd;

  // Need two operands off stack
  if(RPN_STACK_CNT < 2)
  {
    rslt.data     = (INT32) RPN_ERR_TOO_FEW_STACK_VARS;
    rslt.dataType = DATATYPE_RPN_PROC_ERR;
    rslt.validity = FALSE;
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
            rslt.data = (FLOAT32)(fabs(oprndLeft.data - oprndRight.data) >= FLT_EPSILON);
            break;

          case OP_EQ:
            rslt.data = (FLOAT32)(fabs(oprndLeft.data - oprndRight.data) <= FLT_EPSILON);
            break;

          case OP_GE:
            rslt.data = (FLOAT32) ((oprndLeft.data > oprndRight.data) ||
                        (fabs(oprndLeft.data - oprndRight.data) <= FLT_EPSILON));
            break;

          case OP_LE:
            rslt.data = (FLOAT32) ((oprndLeft.data < oprndRight.data) ||
                        (fabs(oprndLeft.data - oprndRight.data) <= FLT_EPSILON));
            break;

          case OP_GT:
            rslt.data = (FLOAT32) (oprndLeft.data > oprndRight.data);
            break;

          case OP_LT:
            rslt.data = (FLOAT32) (oprndLeft.data < oprndRight.data);
            break;

          // Should never get here; all Comparison Operators are handled.
          default:
            FATAL("Unrecognized Expression OpCode: %d", cmd->opCode);
        }

      rslt.dataType = DATATYPE_BOOL;

      // SRS-4544  The result of a comparison operation shall be INVALID if
      // the validity of either of the input value operands is invalid.
      rslt.validity = ( EvalGetValidCnt(&oprndLeft, &oprndRight ) == 2 );

      // SRS-4595 The result of a comparison operation shall be FALSE
      // if the validity of  either of the input values operands is invalid
      if(!rslt.validity)
      {
        rslt.data = FALSE;
      }

    }
    else
    {
      // Set RPN response to flag a syntax error
      rslt.data     = (INT32) RPN_ERR_INV_OPRND_TYPE;
      rslt.dataType = DATATYPE_RPN_PROC_ERR;
      rslt.validity = FALSE;
      // Ensure our error msg will be the only thing on stack.
      RPN_STACK_PURGE;
    }
  }

  RPN_PUSH(rslt);
  return rslt.dataType != DATATYPE_RPN_PROC_ERR;
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
static BOOLEAN EvalIsNotEqualPrev( EVAL_EXE_CONTEXT* context)
{
  EVAL_RPN_ENTRY oprndCurrent;
  EVAL_RPN_ENTRY oprndPrevious;
  EVAL_RPN_ENTRY rslt;
  UINT32         key;

  // Need one operand off stack
  if(RPN_STACK_CNT < 1)
  {
    rslt.data     = (INT32) RPN_ERR_TOO_FEW_STACK_VARS;
    rslt.dataType = DATATYPE_RPN_PROC_ERR;
    rslt.validity = FALSE;

    // Ensure error msg will be the only
    // thing on the stack(push is performed at end of func.)
    RPN_STACK_PURGE;
  }
  else
  {
    oprndCurrent = RPN_POP;
    ASSERT_MESSAGE(EvalVerifyDataType(DATATYPE_VALUE, &oprndCurrent, NULL),
    "Not-Previous Sensor value was not a VALUE: DataType: %d, Data: %d, Validity: %d",
                   oprndCurrent.dataType,
                   oprndCurrent.data,
                   oprndCurrent.validity)

    // Lookup/add the previous-sensor for this cmd and make an R-side operand
    oprndPrevious.dataType = DATATYPE_VALUE;

    // Lookup the previous sensor value for this reference.
    // Make the lookup key using caller type, the objectId holding this expression, and the
    // sensor being referenced.
    key = EVAL_MAKE_LOOKUP_KEY( context->objType, context->objId, (UINT8)context->cmd->data);

    EvalGetPrevSensorValue( key, &oprndPrevious.data, &oprndPrevious.validity);

    rslt.data = (FLOAT32)(fabs( (oprndCurrent.data - oprndPrevious.data)) >= FLT_EPSILON );
    rslt.dataType = DATATYPE_BOOL;
    rslt.validity = ( EvalGetValidCnt(&oprndCurrent, &oprndPrevious ) == 2 );

    // Store the current sensor value & validity in table for next processing.
    EvalSetPrevSensorValue( key, context, oprndCurrent.data ,oprndCurrent.validity );
  }

  RPN_PUSH(rslt);
  return rslt.dataType != DATATYPE_RPN_PROC_ERR;
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
static BOOLEAN EvalPerformNot( EVAL_EXE_CONTEXT* context)
{
  EVAL_RPN_ENTRY oprnd;
  EVAL_RPN_ENTRY rslt;

  // Need one operand off stack
  if(RPN_STACK_CNT < 1)
  {
    rslt.data     = (INT32)RPN_ERR_TOO_FEW_STACK_VARS;
    rslt.dataType = DATATYPE_RPN_PROC_ERR;
    rslt.validity = FALSE;
    // Ensure error msg is only thing on the stack
    RPN_STACK_PURGE;
  }
  else
  {
    oprnd = RPN_POP;
    if ( EvalVerifyDataType(DATATYPE_BOOL, &oprnd, NULL) )
    {
      rslt.validity = oprnd.validity;
      rslt.dataType = DATATYPE_BOOL;
      rslt.data     = rslt.validity ? (FLOAT32) !(BOOLEAN)oprnd.data : 0.f;
    }
    else
    {
      // Set RPN response to flag a syntax error
      rslt.data     = (INT32)RPN_ERR_INV_OPRND_TYPE;
      rslt.dataType = DATATYPE_RPN_PROC_ERR;
      rslt.validity = FALSE;
      // Ensure error msg is only thing on the stack
      RPN_STACK_PURGE;
    }
  }

  RPN_PUSH(rslt);
 return rslt.dataType != DATATYPE_RPN_PROC_ERR;
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
static BOOLEAN EvalPerformAnd( EVAL_EXE_CONTEXT* context )
{
  EVAL_RPN_ENTRY oprndLeft;
  EVAL_RPN_ENTRY oprndRight;
  EVAL_RPN_ENTRY rslt;

  // Need two operands off stack
  if(RPN_STACK_CNT < 2)
  {
    rslt.data     = (INT32)RPN_ERR_TOO_FEW_STACK_VARS;
    rslt.dataType = DATATYPE_RPN_PROC_ERR;
    rslt.validity = FALSE;
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
      rslt.data     = (FLOAT32) (BOOLEAN)(oprndLeft.data && (BOOLEAN)oprndRight.data);
      rslt.dataType = DATATYPE_BOOL;
      rslt.validity = ( EvalGetValidCnt(&oprndLeft, &oprndRight) == 2 );
    }
    else
    {
      // Set RPN response to flag a syntax error
      rslt.data     = (INT32) RPN_ERR_INV_OPRND_TYPE;
      rslt.dataType = DATATYPE_RPN_PROC_ERR;
      rslt.validity = FALSE;
      // Ensure error msg is only thing on the stack
      RPN_STACK_PURGE;
    }
  }
  RPN_PUSH(rslt);
  return rslt.dataType != DATATYPE_RPN_PROC_ERR;
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
static BOOLEAN EvalPerformOr( EVAL_EXE_CONTEXT* context )
{
  EVAL_RPN_ENTRY oprndLeft;
  EVAL_RPN_ENTRY oprndRight;
  EVAL_RPN_ENTRY rslt;

  // Need two operands off stack
  if(RPN_STACK_CNT < 2)
  {
    rslt.data     = (INT32) RPN_ERR_TOO_FEW_STACK_VARS;
    rslt.dataType = DATATYPE_RPN_PROC_ERR;
    rslt.validity = FALSE;
    RPN_STACK_PURGE;
  }
  else
  {
    oprndRight = RPN_POP;
    oprndLeft  = RPN_POP;
    if ( EvalVerifyDataType(DATATYPE_BOOL, &oprndLeft, &oprndRight) )
    {
      if ( (oprndLeft.validity  && (BOOLEAN)oprndLeft.data) ||
           (oprndRight.validity && (BOOLEAN)oprndRight.data) )
      {
        rslt.data = 1.0f;
        rslt.dataType = DATATYPE_BOOL;
        rslt.validity = TRUE;
      }
      else
      {
        rslt.data = 0.0f;
        rslt.dataType = DATATYPE_BOOL;
        rslt.validity = (oprndLeft.validity || oprndRight.validity);
      }
    }
    else
    {
      // Set RPN response to flag a syntax error
      rslt.data     = (INT32)RPN_ERR_INV_OPRND_TYPE;
      rslt.dataType = DATATYPE_RPN_PROC_ERR;
      rslt.validity = FALSE;
      // Ensure error msg is only thing on the stack
      RPN_STACK_PURGE;
    }
  }

  RPN_PUSH(rslt);
  return rslt.dataType != DATATYPE_RPN_PROC_ERR;
}


/******************************************************************************
 * Function: EvalAddConst
 *
 * Description:   Adds the passed constant float value to the cmd list as a direct value.
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
static INT32 EvalAddConst(INT16 tblIdx, const CHAR* str, EVAL_EXPR* expr)
{
  FLOAT32  temp_float;
  CHAR*    end;
  EVAL_CMD cmd;
  INT32    retval;

  // Have we reached limit count for operands?
  if(expr->operandCnt == expr->maxOperands)
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
      expr->cmdList[expr->size++] = cmd;
      expr->operandCnt++;
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
 * Description: Add a "fetch return value" of the configured function
 *              to the expression cmd list.
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
static INT32 EvalAddFuncCall(INT16 tblIdx, const CHAR* str, EVAL_EXPR* expr)
{
  EVAL_CMD cmd;
  INT32    retval;

  // Have we reached limit count for operands?
  if(expr->operandCnt == expr->maxOperands)
  {
    retval = (INT32) RPN_ERR_TOO_MANY_OPRNDS;
  }
  else
  {
    // Object is a constant Boolean 'FALSE'
    cmd.opCode = m_OpCodeTable[tblIdx].opCode;
    cmd.data   = 0.f;

    // Add cmd to expression list
    expr->cmdList[expr->size++] = cmd;
    expr->operandCnt++;

    retval = m_OpCodeTable[tblIdx].tokenLen;
  }
  return retval;
}

/******************************************************************************
 * Function: EvalAddInputSrc
 *
 * Description:   Add a data-retrieval command to the expression cmd list.
 *
 * Parameters:    [in]  tblIdx - table index
 *                [in]  str    - pointer  to the string whose object value is retrieved.
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
static INT32 EvalAddInputSrc(INT16 tblIdx, const CHAR* str, EVAL_EXPR* expr)
{
  EVAL_CMD cmd;
  INT32    retval = 0;
  INT32    maxObjects = 0;
  INT32    objIdx;

  if(expr->operandCnt == expr->maxOperands)
  {
    retval = (INT32) RPN_ERR_TOO_MANY_OPRNDS;
  }
  else
  {
    // Get the allowable number of objects for this  OpCode
    switch(m_OpCodeTable[tblIdx].opCode)
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
        FATAL("Unrecognized Opcode: %d",m_OpCodeTable[tblIdx].opCode );
    }

    //                                                          |
    //                                                          V
    // Look to end of this fixed-token for the objIdx (e.g. SVLU127)
    objIdx = EvalParseObjectIndex( &str[m_OpCodeTable[tblIdx].tokenLen], maxObjects);
    if(objIdx >= 0)
    {
      cmd.opCode = (UINT8)m_OpCodeTable[tblIdx].opCode;
      cmd.data   = (FLOAT32)objIdx;
      // Add cmd to expression list
      expr->cmdList[expr->size++] = cmd;
      expr->operandCnt++;
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
 * Description:   Add a operation command to the expression cmd list.
 *
 * Parameters:    [in]  tblIdx - table index ( not used in this function)
 *                [in]  str    - pointer  to the string containing the operator.
 *                [out] expr   - pointer to expression obj to be updated.
 *
 * Returns:       value < 0    - processing error code.
 *                value >= 0   - the length of the string processed.
 *
 * Notes:
 *
 *
 *****************************************************************************/
static INT32 EvalAddStdOper(INT16 tblIdx, const CHAR* str, EVAL_EXPR* expr)
{
  EVAL_CMD cmd;

  cmd.opCode = m_OpCodeTable[tblIdx].opCode;
  cmd.data   = 0.f;

  // Add cmd to expression list
  expr->cmdList[expr->size++] = cmd;

  // Return the len for this operator
  return m_OpCodeTable[tblIdx].tokenLen;
}

/******************************************************************************
 * Function: EvalAddNotEqPrev
 *
 * Description:    Add a sensor-data not equal to prev-value
 *                 command to the expression cmd list.
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
static INT32 EvalAddNotEqPrev(INT16 tblIdx, const CHAR* str, EVAL_EXPR* expr)
{
  EVAL_CMD cmd;
  INT32    retval;

  // Check the previous operation in list..
  // Verify that it is a sensor-load cmd and get idx-value stored
  // in Data so '!P' can make a call to load prev value at exe time.
  if ( (UINT8)OP_GETSNRVAL == expr->cmdList[expr->size - 1].opCode)
  {
    cmd.opCode = (UINT8)OP_NOT_EQ_PREV;
    cmd.data   = expr->cmdList[expr->size - 1].data;
    expr->cmdList[expr->size++] = cmd;
    retval = m_OpCodeTable[tblIdx].tokenLen;
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
 * Description:   Create a human-readable string for the indicated command
 *                e.g. the command represents returning he active state of Trigger 23,
 *                the function returns "TACT023"
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
static INT32 EvalFmtLoadEnumeratedCmdStr (INT16 tblIdx, const EVAL_CMD* cmd, CHAR* str)
{
  return snprintf(str, EVAL_OPRND_LEN + 1, "%s%03d", m_OpCodeTable[tblIdx].token,
                                                     (INT32)cmd->data);
}

/******************************************************************************
 * Function: EvalFmtLoadConstStr
 *
 * Description:   Create a human-readable string for the indicated command
 *                e.g. the command represents loading the constant 123.4,
 *                the function returns "123.4"
 *
 * Parameters:    [in]  tblIdx - table index
 *                [in]  cmd    - pointer  to the cmd to be converted to string.
 *                [out] str   - pointer to string to hold conversion.
 *
 * Returns:       value < 0    - processing error code.
 *                value >= 0   - the length of the string processed.
 *
 * Notes:
 *
 *
 *****************************************************************************/
static INT32 EvalFmtLoadConstStr(INT16 tblIdx, const EVAL_CMD* cmd, CHAR* str)
{
  return snprintf(str, MAX_FP_STRING, "%f", cmd->data);
}

/******************************************************************************
 * Function: EvalFmtLoadCmdStr
 *
 * Description:   Return the token string from the opcode table for this tblIdx.
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
static INT32 EvalFmtLoadCmdStr(INT16 tblIdx, const EVAL_CMD* cmd, CHAR* str)
{
  return snprintf(str, m_OpCodeTable[tblIdx].tokenLen + 1, "%s", m_OpCodeTable[tblIdx].token);
}

/******************************************************************************
 * Function: EvalFmtOperStr
 *
 * Description:   Returns the string representation of the indicated operation.
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
static INT32 EvalFmtOperStr(INT16 tblIdx, const EVAL_CMD* cmd, CHAR* str)
{
  return  snprintf(str, m_OpCodeTable[tblIdx].tokenLen + 1, "%s", m_OpCodeTable[tblIdx].token);
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
    if (m_masterTbl[i].keyField == key)
    {
      idx = i;
      break;
    }
  }

  // If we found the table entry, return values.
  if (-1 != idx)
  {
    *fPriorValue = m_masterTbl[idx].priorValue;
    *bPriorValid = m_masterTbl[idx].priorValid;
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
      if (context->tempTbl[i].keyField == key)
      {
        context->tempTbl[i].priorValue = fValue;
        context->tempTbl[i].priorValid = bValid;
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

      context->tempTbl[context->tempTblCnt].keyField   = key;
      context->tempTbl[context->tempTblCnt].priorValue = fValue;
      context->tempTbl[context->tempTblCnt].priorValid = bValid;
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
      if (context->tempTbl[idxTempTbl].keyField == m_masterTbl[idxMastTbl].keyField)
      {
        m_masterTbl[idxMastTbl].priorValue = context->tempTbl[idxTempTbl].priorValue;
        m_masterTbl[idxMastTbl].priorValid = context->tempTbl[idxTempTbl].priorValid;
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
        m_masterTbl[m_masterTblCnt].keyField    = context->tempTbl[idxTempTbl].keyField;
        m_masterTbl[m_masterTblCnt].priorValue  = context->tempTbl[idxTempTbl].priorValue;
        m_masterTbl[m_masterTblCnt].priorValid  = context->tempTbl[idxTempTbl].priorValid;
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
 * *****************  Version 28  *****************
 * User: Contractor V&v Date: 12/28/12   Time: 5:52p
 * Updated in $/software/control processor/code/drivers
 * SCR #1197 CR FIS finding comment
 *
 * *****************  Version 27  *****************
 * User: Contractor V&v Date: 12/14/12   Time: 5:00p
 * Updated in $/software/control processor/code/drivers
 * SCR #1107 Code Review
 *
 * *****************  Version 26  *****************
 * User: Contractor V&v Date: 12/12/12   Time: 6:23p
 * Updated in $/software/control processor/code/drivers
 * SCR #1107 Code Review
 *
 * *****************  Version 25  *****************
 * User: Contractor V&v Date: 12/03/12   Time: 5:34p
 * Updated in $/software/control processor/code/drivers
 * SCR #1107 BitBucket #78
 *
 * *****************  Version 24  *****************
 * User: Contractor V&v Date: 11/16/12   Time: 8:13p
 * Updated in $/software/control processor/code/drivers
 * Code Review updates
 *
 * *****************  Version 23  *****************
 * User: Contractor V&v Date: 11/14/12   Time: 4:01p
 * Updated in $/software/control processor/code/drivers
 * Code Review check in
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
