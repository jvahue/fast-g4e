#ifndef ACTION_H
#define ACTION_H
/**********************************************************************************************
                Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
                    All Rights Reserved. Proprietary and Confidential.


    File:        ActionManager.h


    Description: Function prototypes and defines for the action processing.

  VERSION
  $Revision: 1 $  $Date: 12-07-17 11:32a $

**********************************************************************************************/

/*********************************************************************************************/
/* Compiler Specific Includes                                                                */
/*********************************************************************************************/

/*********************************************************************************************/
/* Software Specific Includes                                                                */
/*********************************************************************************************/
#include "EngineRun.h"
#include "FIFO.H"

/*********************************************************************************************/
/*                                 Package Defines                                           */
/*********************************************************************************************/
#define MAX_ACTION_DEFINES           8
#define MAX_ACTION_REQUESTS         32

#define ACTION1                     0x00000001
#define ACTION2                     0x00000002
#define ACTION3                     0x00000004
#define ACTION4                     0x00000008
#define ACTION5                     0x00000010
#define ACTION6                     0x00000020
#define ACTION7                     0x00000040
#define ACTION8                     0x00000080

#define ACTION_WHEN_MET             0x00080000
#define ACTION_ON_DURATION          0x00040000

// ACKNOWLEDGE AND LATCH
#define ACTION_LATCH                0x00800000
#define ACTION_ACK                  0x80000000

#define ACTION_ANY          ( ACTION_ON_DURATION | ACTION_WHEN_MET )

#define ACTION_MASK         ( ACTION1 | ACTION2 | ACTION3 | ACTION4 |\
                              ACTION5 | ACTION6 | ACTION7 | ACTION8 )

// OUTPUTS
#define LSS0MASK        0x01
#define LSS1MASK        0x02
#define LSS2MASK        0x04
#define LSS3MASK        0x08

#define MAX_OUTPUT_LSS     4

#define ACTION_NO_REQ   -1
//********************************************************************************************
// ACTION CONFIGURATION DEFAULT
//********************************************************************************************
#define ACTION_OUTPUT_DEFAULT         0,                     /* Used Mask                  */\
                                      0                      /* LSS Mask                   */

#define ACTION_DEFAULT                TRIGGER_UNUSED,        /* Trigger ACK Index          */\
                                      ACTION_OUTPUT_DEFAULT, /* Action Output #1           */\
                                      ACTION_OUTPUT_DEFAULT, /* Action Output #2           */\
                                      ACTION_OUTPUT_DEFAULT, /* Action Output #3           */\
                                      ACTION_OUTPUT_DEFAULT, /* Action Output #4           */\
                                      ACTION_OUTPUT_DEFAULT, /* Action Output #5           */\
                                      ACTION_OUTPUT_DEFAULT, /* Action Output #6           */\
                                      ACTION_OUTPUT_DEFAULT, /* Action Output #7           */\
                                      ACTION_OUTPUT_DEFAULT, /* Action Output #8           */\
                                      FALSE,                 /* Persist Enabled            */\
                                      ACTION_OUTPUT_DEFAULT  /* Persistent Action          */

/*********************************************************************************************/
/*                                   Package Typedefs                                        */
/*********************************************************************************************/

typedef enum
{
   ACTION_OFF,
   ON_MET,
   ON_DURATION
} ACTION_TYPE;

///////////////////////////////////////// ACTION //////////////////////////////////////////////
typedef struct
{
   UINT8   UsedMask;
   UINT8   LSS_Mask;
} ACTION_OUTPUT;

typedef struct
{
   BOOLEAN       bEnabled;
   ACTION_OUTPUT Output;
} ACTION_PERSIST_CFG;

typedef struct
{
   TRIGGER_INDEX      ACKTrigger;
   ACTION_OUTPUT      Output[MAX_ACTION_DEFINES];
   ACTION_PERSIST_CFG Persist;
} ACTION_CFG;

typedef struct
{
   BITARRAY128 ACK;
   BITARRAY128 Active;
   BITARRAY128 Latch;
   BOOLEAN     bState;
} ACTION_FLAGS;

typedef struct
{
   BOOLEAN       bState;
   BOOLEAN       bLatch;
   ACTION_OUTPUT Action;
} ACTION_PERSIST;

typedef struct
{
   ACTION_FLAGS    Action[MAX_ACTION_DEFINES];
   ER_STATE        PrevEngState;
   ACTION_PERSIST  Persist;
   UINT8           PriorityMask;
   INT8            RequestCounter;
} ACTION_DATA;

typedef struct
{
   INT8        Index;
   UINT32      Action;
   ACTION_TYPE State;
} ACTION_REQUEST;

typedef struct
{
   FIFO    RecordFIFO;
   UINT32  RecordCnt;
   BOOLEAN bOverFlow;
   UINT16  RecordSize;
} ACTION_REQUEST_FIFO;

/**********************************************************************************************
                                      Package Exports
**********************************************************************************************/
#undef EXPORT

#if defined( ACTION_BODY )
#define EXPORT
#else
#define EXPORT extern
#endif

/**********************************************************************************************
                                  Package Exports Variables
**********************************************************************************************/

/**********************************************************************************************
                                  Package Exports Functions
**********************************************************************************************/
EXPORT void ActionsInitialize ( void );
EXPORT INT8 ActionRequest     ( INT8 ReqNum, UINT32 Action, ACTION_TYPE State );

/**********************************************************************************************
 *  MODIFICATIONS
 *    $History: ActionManager.h $
 * 
 * *****************  Version 1  *****************
 * User: John Omalley Date: 12-07-17   Time: 11:32a
 * Created in $/software/control processor/code/system
 * SCR 1107
 *
 *
 *********************************************************************************************/
#endif  /* ACTION_H */
