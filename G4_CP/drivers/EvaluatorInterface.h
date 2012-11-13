#ifndef EVALUATORINTERFACE_H
#define EVALUATORINTERFACE_H

/******************************************************************************
              Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
                 All Rights Reserved. Proprietary and Confidential.

    File:         EvaluatorInterface.h

    Description: Evaluator Interface file

    VERSION
    $Revision: 11 $  $Date: 12-10-02 1:23p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
/******************************************************************************
                                 Package Defines                              *
******************************************************************************/

// Token -> Opcode lookup table
//
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

/******************************************************************************
                                 Package Typedefs                             *
******************************************************************************/


/******************************************************************************
                                 Package Exports                              *
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

#endif // EVALUATORINTERFACE_H


/*************************************************************************
 *  MODIFICATIONS
 *    $History: EvaluatorInterface.h $
 *
 * *****************  Version 11  *****************
 * User: Contractor V&v Date: 12-10-02   Time: 1:23p
 * Updated in $/software/control processor/code/drivers
 * SCR #1107 FAST 2 Coding standard compliance
 *
 * *****************  Version 10  *****************
 * User: Contractor V&v Date: 8/29/12    Time: 2:55p
 * Updated in $/software/control processor/code/drivers
 * SCR #1107 FAST 2 Add Support for Action ACK processing and !P
 *
 * *****************  Version 9  *****************
 * User: John Omalley Date: 12-08-28   Time: 8:32a
 * Updated in $/software/control processor/code/drivers
 * SCR 1107 - Evaluator Bug Fixes
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
 * FAST 2 add operators
 *
 * *****************  Version 5  *****************
 * User: Contractor V&v Date: 5/10/12    Time: 6:50p
 * Updated in $/software/control processor/code/drivers
 * Adding modification banner
 ***************************************************************************/
