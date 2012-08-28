#ifndef EVALUATOR_H
#define EVALUATOR_H
/******************************************************************************
              Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
                 All Rights Reserved. Proprietary and Confidential.


    File:        evaluator.h


    Description: Function prototypes and defines for the generic evaluator engine.

  VERSION
  $Revision: 11 $  $Date: 8/28/12 1:06p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_basic.h"
#include "EvaluatorInterface.h"
/******************************************************************************
                      Package Defines                               
******************************************************************************/
//  Each expression must also accommodate X operators per input.
//  Defined as 2, each operand can have 1 unary ! and two operands have one binary & or |
//  so '2' actual defines one extra operator per input.
#define EVAL_OPS_PER_OPERAND       2
#define EVAL_MAX_OPERANDS_PER_EXPR 8    // Number of operands in an expression

#define EVAL_OPRND_LEN             7    // Fixed Len SVLUnnn, SVLDnnn
#define EVAL_OPERAND_DIGIT_LEN     3
#define EVAL_PREFIX_LEN  (EVAL_OPRND_LEN - EVAL_OPERAND_DIGIT_LEN)

#define EVAL_EXPR_BIN_LEN  16//((1+1+EVAL_OPS_PER_OPERAND) * EVAL_MAX_OPERANDS_PER_EXPR)

#define EVAL_MAX_EXPR_STR_LEN 256
//#define EVAL_MAX_EXPR_STR_LEN ((EVAL_OPS_PER_OPERAND +\
//                                EVAL_OPERAND_LEN)*EVAL_MAX_OPERANDS_PER_EXPR)

// Used by Cfg manager to create default into for EVAL_EXPR definitions.
#define EVAL_CMD_DEFAULT  {0, 0.f}

#define EVAL_EXPR_CFG_DEFAULT 0, 0, 0,\
                              {\
                               EVAL_CMD_DEFAULT,\
                               EVAL_CMD_DEFAULT,\
                               EVAL_CMD_DEFAULT,\
                               EVAL_CMD_DEFAULT,\
                               EVAL_CMD_DEFAULT,\
                               EVAL_CMD_DEFAULT,\
                               EVAL_CMD_DEFAULT,\
                               EVAL_CMD_DEFAULT,\
                               EVAL_CMD_DEFAULT,\
                               EVAL_CMD_DEFAULT,\
                               EVAL_CMD_DEFAULT,\
                               EVAL_CMD_DEFAULT,\
                               EVAL_CMD_DEFAULT,\
                               EVAL_CMD_DEFAULT,\
                               EVAL_CMD_DEFAULT,\
                               EVAL_CMD_DEFAULT\
                              }

/******************************************************************************
                             Package Typedefs                              
******************************************************************************/


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
  DATATYPE_NONE,
  DATATYPE_VALUE,
  DATATYPE_BOOL,
  DATATYPE_RPN_PROC_ERR
}DATATYPE;

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
  RPN_ERR_PRIOR_SENSR_TABLE_FULL   = -11, // The table storing Prior-sensor values is full.
  //-----                                                                                         
  RPN_ERR_MAX                      = -12                                                          
}RPN_ERR;                                                                                         

#pragma pack(1)
typedef struct
{
  UINT8   OpCode; // Operation to be performed
  FLOAT32 Data;   // Data (optional depending on opCode)
}EVAL_CMD;

// Structure/list for holding the binary form of the RPN-encoded logical expression
// and meta-data

typedef struct
{
  UINT8    Size;          // # bytes used in the Data array for this expression.
  UINT8    MaxOperands;   // Maximum # of arithmetic compares allowed for this expression.
  UINT8    OperandCnt;    // Number of operand refs in the list.
  EVAL_CMD CmdList[EVAL_EXPR_BIN_LEN]; // RPN List of binary commands w/ operands
}EVAL_EXPR;
#pragma pack()


typedef struct
{
  DATATYPE DataType;
  FLOAT32  Data;
  BOOLEAN  Validity;
}EVAL_RPN_ENTRY;

// Entry to handle prior sensor values in expressions
typedef struct  
{
  UINT32  CmdAddress; // Lookup key - address of the !P Cmd object.
  FLOAT32 PriorValue; // The previous stored value of this sensor.
  BOOLEAN PriorValid; // The validity of the previous stored value.
}PRIOR_SENSOR_ENTRY;


// Typedef to function pointers.
typedef INT32   ADD_CMD (INT16 tblIdx, const CHAR* str, EVAL_EXPR* expr);
typedef INT16   FMT_CMD (INT16 tblIdx, const EVAL_CMD* cmd, CHAR* str);
typedef BOOLEAN OP_CMD  (const EVAL_CMD* cmd);

typedef struct
{
  BYTE     OpCode;
  CHAR     Token[EVAL_OPRND_LEN+1];
  UINT8    TokenLen;
  ADD_CMD* AddCmd;        // Ptr to function STR -> CMD obj
  FMT_CMD* FmtCmd;        // Ptr to function CMDobj -> STR
  OP_CMD*  ExeCmd;        // Ptr to function to execute CMD
}EVAL_OPCODE_TBL_ENTRY;


// Data Access Interface
typedef FLOAT32 GET_VALUE_FUNC( INT32 objIndex );
typedef BOOLEAN GET_BOOL_FUNC ( INT32 objIndex );

typedef struct
{
  BYTE OpCode;    // Lookup-key from EVAL_OPCODE_TBL_ENTRY
  GET_BOOL_FUNC*  IsSrcConfigured;  // Return is input source configured?
  GET_VALUE_FUNC* GetSrcByValue;    // Return source input data as FP number.
  GET_BOOL_FUNC*  GetSrcByBool;     // Return source input data as boolean.
  GET_BOOL_FUNC*  GetSrcValidity;   // Return source validity 
}EVAL_DATAACCESS;


/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined( EVALUATOR_BODY )
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

// Declare before inclusion of user-table because EvaluatorUserTable.c will need to see them for
// processing the "SC" and "EC" strings.

EXPORT INT32 EvalExprStrToBin  ( CHAR* str, EVAL_EXPR* bin, UINT8 maxOperands );
EXPORT void  EvalExprBinToStr  ( CHAR* str, const EVAL_EXPR* bin );
EXPORT INT32 EvalExeExpression ( const EVAL_EXPR* expr, BOOLEAN* validity );
EXPORT const CHAR* EvalGetMsgFromErrCode(INT32 errNum);

// Functions listed in the function table not really "exported" but
// need to be declared because EvaluatorInterface.h will use them.

BOOLEAN EvalLoadConstValue      (const EVAL_CMD* cmd);
BOOLEAN EvalLoadConstFalse      (const EVAL_CMD* cmd);
BOOLEAN EvalLoadInputSrc        (const EVAL_CMD* cmd);
BOOLEAN EvalLoadFuncCall        (const EVAL_CMD* cmd);

// Comparison Operators
BOOLEAN EvalCompareOperands     (const EVAL_CMD* cmd);
BOOLEAN EvalIsNotEqualPrev      (const EVAL_CMD* cmd);

// Logical Operators
BOOLEAN EvalPerformNot          (const EVAL_CMD* cmd);
BOOLEAN EvalPerformAnd          (const EVAL_CMD* cmd);
BOOLEAN EvalPerformOr           (const EVAL_CMD* cmd);

// String-to-Cmd Converter functions.
INT32 EvalAddConst    (INT16 tblIdx, const CHAR* str, EVAL_EXPR* expr);
INT32 EvalAddFuncCall (INT16 tblIdx, const CHAR* str, EVAL_EXPR* expr);
INT32 EvalAddInputSrc (INT16 tblIdx, const CHAR* str, EVAL_EXPR* expr);

INT32 EvalAddStdOper  (INT16 tblIdx, const CHAR* str, EVAL_EXPR* expr);
INT32 EvalAddNotEqPrev(INT16 tblIdx, const CHAR* str, EVAL_EXPR* expr);

// Cmd-to-String representation converters.
INT16 EvalFmtLoadEnumeratedCmdStr (INT16 tblIdx, const EVAL_CMD* cmd, CHAR* str);
INT16 EvalFmtLoadConstStr         (INT16 tblIdx, const EVAL_CMD* cmd, CHAR* str);
INT16 EvalFmtLoadCmdStr           (INT16 tblIdx, const EVAL_CMD* cmd, CHAR* str);
INT16 EvalFmtOperStr              (INT16 tblIdx, const EVAL_CMD* cmd, CHAR* str);

#endif // EVALUATOR_H
/*************************************************************************
 *  MODIFICATIONS
 *    $History: Evaluator.h $
 * 
 * *****************  Version 11  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 10  *****************
 * User: John Omalley Date: 12-08-28   Time: 8:32a
 * Updated in $/software/control processor/code/drivers
 * SCR 1107 - Evaluator Bug Fixes
 * 
 * *****************  Version 9  *****************
 * User: Contractor V&v Date: 8/22/12    Time: 5:29p
 * Updated in $/software/control processor/code/drivers
 * FAST 2 Issue #14 Coverage: Use assert vs. if/then/else
 * 
 * *****************  Version 8  *****************
 * User: Contractor V&v Date: 8/15/12    Time: 7:22p
 * Updated in $/software/control processor/code/drivers
 * SCR #1107 FAST 2 Issue # 20 Eval operand names like FSM
 * 
 * *****************  Version 7  *****************
 * User: Contractor V&v Date: 6/25/12    Time: 7:12p
 * Updated in $/software/control processor/code/drivers
 * SCR #1107 Add FALSE boolean constant
 * 
 * *****************  Version 6  *****************
 * User: Contractor V&v Date: 6/18/12    Time: 4:27p
 * Updated in $/software/control processor/code/drivers
 * FAST 2 Add accessor for running engs
 * 
 * *****************  Version 5  *****************
 * User: Contractor V&v Date: 4/27/12    Time: 4:03p
 * Updated in $/software/control processor/code/drivers
 ***************************************************************************/


