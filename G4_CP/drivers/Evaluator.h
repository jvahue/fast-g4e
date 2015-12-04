#ifndef EVALUATOR_H
#define EVALUATOR_H
/******************************************************************************
              Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
                 All Rights Reserved. Proprietary and Confidential.


    File:        evaluator.h


    Description: Function prototypes and defines for the generic evaluator engine.

  VERSION
  $Revision: 20 $  $Date: 15-10-13 1:43p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_basic.h"
//#include "EvaluatorInterface.h"
/******************************************************************************
                      Package Defines
******************************************************************************/
//  Each expression must also accommodate X operators per input.

#define EVAL_OPRND_LEN             7    // Fixed Len SVLUnnn, SVLDnnn
#define EVAL_OPERAND_DIGIT_LEN     3

#define EVAL_EXPR_BIN_LEN  16

#define MAX_TEMP_PRIOR_VALUES (EVAL_EXPR_BIN_LEN + 1)// Number of temp array items ==
// max expression size + 1 so array
// will always be large enough at runtime

#define EVAL_MAX_EXPR_STR_LEN 256

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

typedef enum
{
  EVAL_CALLER_TYPE_PARSE,    // Do not insert into !P tables during the parse of a Cfg
  EVAL_CALLER_TYPE_TRIGGER,
  EVAL_CALLER_TYPE_EVENT,
  EVAL_CALLER_TYPE_APAC, 
  /*--- Add new types above this line ----*/
  MAX_EVAL_CALLER_TYPE
}EVAL_CALLER_TYPE;


#pragma pack(1)
typedef struct
{
  UINT8   opCode; // Operation to be performed
  FLOAT32 data;   // Data (optional depending on opCode)
}EVAL_CMD;

// Structure/list for holding the binary form of the RPN-encoded logical expression
// and meta-data

typedef struct
{
  UINT8    size;          // # bytes used in the Data array for this expression.
  UINT8    maxOperands;   // Maximum # of arithmetic compares allowed for this expression.
  UINT8    operandCnt;    // Number of operand refs in the list.
  EVAL_CMD cmdList[EVAL_EXPR_BIN_LEN]; // RPN List of binary commands w/ operands
}EVAL_EXPR;
#pragma pack()

// Entry to handle prior sensor values in expressions
// KeyField layout:
// 0x000000FF - Sensor ID 00-255
// 0x0000FF00 - Object ID 00-255
// 0x00FF0000 - Object Type 0- Trigger, 1- Event, etc

typedef struct
{
  UINT32  keyField; // Lookup key - GUID for use of this sensor in a given object(Trig, Event)
  FLOAT32 priorValue;// The previous stored value of this sensor.
  BOOLEAN priorValid;// The validity of the previous stored value.
}PRIOR_SENSOR_ENTRY;

// Structure used during expression execution to
// define context
typedef struct
{
  const EVAL_CMD*    cmd;      // Pointer to currently executing cmd in Expression.
  UINT8              objType;  // Object type of current caller.
  UINT8              objId;    // Object ID   of current caller.
  PRIOR_SENSOR_ENTRY tempTbl[MAX_TEMP_PRIOR_VALUES];
  UINT8              tempTblCnt;
}EVAL_EXE_CONTEXT;

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( EVALUATOR_BODY )
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

// Declare before inclusion of user-table because EvaluatorUserTable.c will
// need to see them for processing the "SC" and "EC" strings.

EXPORT INT32 EvalExprStrToBin  ( EVAL_CALLER_TYPE objType, INT32 objID,
                                 CHAR* str,EVAL_EXPR* expr, UINT8 maxOperands );
EXPORT void  EvalExprBinToStr  ( CHAR* str, const EVAL_EXPR* bin );
EXPORT INT32 EvalExeExpression  ( EVAL_CALLER_TYPE objType, INT32 objID,
                                const EVAL_EXPR* expr, BOOLEAN* validity );

EXPORT const CHAR* EvalGetMsgFromErrCode(INT32 errNum);


#endif // EVALUATOR_H
/*************************************************************************
 *  MODIFICATIONS
 *    $History: Evaluator.h $
 * 
 * *****************  Version 20  *****************
 * User: Peter Lee    Date: 15-10-13   Time: 1:43p
 * Updated in $/software/control processor/code/drivers
 * SCR #1304 APAC Processing Initial Check In
 * 
 * *****************  Version 19  *****************
 * User: Contractor V&v Date: 12/12/12   Time: 6:24p
 * Updated in $/software/control processor/code/drivers
 * SCR #1107 Code Review 
 *
 * *****************  Version 18  *****************
 * User: Contractor V&v Date: 11/16/12   Time: 8:13p
 * Updated in $/software/control processor/code/drivers
 * Code Review updates
 *
 * *****************  Version 17  *****************
 * User: Contractor V&v Date: 11/14/12   Time: 4:01p
 * Updated in $/software/control processor/code/drivers
 * Code Review check in
 *
 * *****************  Version 16  *****************
 * User: Contractor V&v Date: 11/13/12   Time: 6:28p
 * Updated in $/software/control processor/code/drivers
 * Code review
 *
 * *****************  Version 15  *****************
 * User: Melanie Jutras Date: 12-11-07   Time: 1:09p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 File Format Errors
 *
 * *****************  Version 14  *****************
 * User: Contractor V&v Date: 12-10-02   Time: 1:23p
 * Updated in $/software/control processor/code/drivers
 * SCR #1107 FAST 2 Coding standard compliance
 *
 * *****************  Version 13  *****************
 * User: Jeff Vahue   Date: 8/29/12    Time: 6:20p
 * Updated in $/software/control processor/code/drivers
 * SCR# 1107 - !P Table Full processing
 *
 * *****************  Version 12  *****************
 * User: Contractor V&v Date: 8/29/12    Time: 2:55p
 * Updated in $/software/control processor/code/drivers
 * SCR #1107 FAST 2 Issue #24 Eval: Not Equal Previous does not work
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


