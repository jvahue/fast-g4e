#ifndef ACTION_H
#define ACTION_H
/******************************************************************************
                Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
                    All Rights Reserved. Proprietary and Confidential.


    File:        ActionManager.h


    Description: Function prototypes and defines for the action processing.

  VERSION
  $Revision: 17 $  $Date: 13-01-16 4:44p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "EngineRun.h"
#include "FIFO.H"

/******************************************************************************
                                 Package Defines                                           
******************************************************************************/
#define MAX_ACTION_DEFINES           8
#define MAX_ACTION_REQUESTS         100
/* Action Flags                                                          */
#define ACTION0                     0x0000
#define ACTION1                     0x0001
#define ACTION2                     0x0002
#define ACTION3                     0x0004
#define ACTION4                     0x0008
#define ACTION5                     0x0010
#define ACTION6                     0x0020
#define ACTION7                     0x0040
#define ACTION8                     0x0080

#define ACTION_ALL                  ( ACTION1 | ACTION2 | ACTION3 | ACTION4 |\
                                      ACTION5 | ACTION6 | ACTION7 | ACTION8 )
#define ACTION_NONE                 0xFFFF

// OUTPUTS
#define LSS0_INDEX                  0
#define LSS1_INDEX                  1
#define LSS2_INDEX                  2
#define LSS3_INDEX                  3
#define MAX_OUTPUT_LSS              4

#define ACTION_NO_REQ               -1
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
                                      ACTION_OUTPUT_DEFAULT, /* Persistent Action          */\
                                      0x0F                   /* All LSS Active High        */

/******************************************************************************
                                  Package Typedefs                        
******************************************************************************/

/* Action Types */
typedef enum
{
   ACTION_OFF,
   ACTION_ON
} ACTION_TYPE;

typedef enum
{
   ACT_TRANS_TO_ENG_RUN,
   ACT_ACKNOWLEDGED
} ACTION_PERSIST_CLR_TYPE;

/*---------------------------------------- ACTION -------------------------------------------*/
#pragma pack(1)
typedef struct
{
   UINT8   nUsedMask;                                 /* LSS Outputs used by action          */
   UINT8   nLSS_Mask;                                 /* Activate State of the LSS Outputs   */
} ACTION_OUTPUT;

/* Persistent Action Configuration    */
typedef struct
{
   BOOLEAN       bEnabled;                            /* Is the persistent action enabled ?  */
   ACTION_OUTPUT output;                              /* Masks for the persistent output(s)  */
} ACTION_PERSIST_CFG;

/* Action configuration               */
typedef struct
{
   TRIGGER_INDEX      aCKTrigger;                     /* Trigger to Acknowledge Actions      */
   ACTION_OUTPUT      output[MAX_ACTION_DEFINES];     /* Masks for the action outputs        */
   ACTION_PERSIST_CFG persist;                        /* persistent action configuration     */
   UINT8              activeState;
} ACTION_CFG;

/* Dynamic structure to hold the persistent state                  */
typedef struct
{
   BOOLEAN       bState;                              /* Persistent output current state     */
   BOOLEAN       bLatch;                              /* Is persistent action latched?       */
   UINT16        actionNum;
   ACTION_OUTPUT action;                              /* Persistent output state             */
} ACTION_PERSIST;

typedef struct
{
   ACTION_PERSIST_CLR_TYPE clearReason;
   ACTION_PERSIST          persistState;
} ACTION_CLR_PERSIST_LOG;

#pragma pack()

/* Dynamic structure of flags for the state of the action requests */
typedef struct
{
   BITARRAY128 flagACK;                               /* Flags -> request is acknowledable   */
   BITARRAY128 flagActive;                            /* Flags -> request is Active          */
   BITARRAY128 flagLatch;                             /* Flags -> request is Latched         */
   BOOLEAN     bState;                                /* Action output current state         */
} ACTION_FLAGS;

/* Dynamic state of the actions                                    */
typedef struct
{
   ACTION_FLAGS   action[MAX_ACTION_DEFINES];        /* State of all action flags           */
   ER_STATE       prevEngState;                      /* Previous engine state               */
   BOOLEAN        bUpdatePersistOut;
   BOOLEAN        bNVStored;
   ACTION_PERSIST persist;                           /* State of the persistent data        */
   UINT16         nLSS_Priority[MAX_OUTPUT_LSS];     /* Priority Storage for each LSS       */
   INT8           nRequestCounter;                   /* Request Counter for generating IDs  */
   BOOLEAN        bInitialized;                      /* Ready to Accept Requests            */
} ACTION_DATA;

/* Structure that defines the action requests                      */
typedef struct
{
   INT8        nID;                                    /* ID of the request                  */
   UINT16      nAction;                                /* Action flags of the request        */
   ACTION_TYPE state;                                  /* state of the request ON/OFF        */
   BOOLEAN     bACK;                                   /* Is Action acknowledgable           */
   BOOLEAN     bLatch;                                 /* Is Action latched                  */
} ACTION_REQUEST;

/* FIFO used for Action requests                                   */
typedef struct
{
   FIFO    recordFIFO;
   UINT32  nRecordCnt;
} ACTION_REQUEST_FIFO;

/******************************************************************************
                                      Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( ACTION_BODY )
#define EXPORT
#else
#define EXPORT extern
#endif

/******************************************************************************
                                  Package Exports Variables
*******************************************************************************/

/******************************************************************************
                                  Package Exports Functions
******************************************************************************/
EXPORT void ActionsInitialize    ( void );
EXPORT INT8 ActionRequest        ( INT8 nReqNum, UINT8 nAction, ACTION_TYPE state,
                                   BOOLEAN bACK, BOOLEAN bLatch );
EXPORT BOOLEAN ActionAcknowledgable (  INT32 TrigIdx );
EXPORT BOOLEAN ActionEEFileInit     ( void );
EXPORT BOOLEAN ActionRTCFileInit    ( void );

#endif // ACTION_H
/*************************************************************************
 *  MODIFICATIONS
 *    $History: ActionManager.h $
 * 
 * *****************  Version 17  *****************
 * User: John Omalley Date: 13-01-16   Time: 4:44p
 * Updated in $/software/control processor/code/system
 * SCR 1220 - Misc Design Review Issues
 * 
 * *****************  Version 16  *****************
 * User: John Omalley Date: 12-12-08   Time: 11:44a
 * Updated in $/software/control processor/code/system
 * SCR 1162 - NV MGR File Init function
 * 
 * *****************  Version 15  *****************
 * User: John Omalley Date: 12-12-07   Time: 2:48p
 * Updated in $/software/control processor/code/system
 * SCR 1197 - Code Review Update
 * 
 * *****************  Version 14  *****************
 * User: John Omalley Date: 12-12-01   Time: 11:00a
 * Updated in $/software/control processor/code/system
 * SCR 1197 - Code Review Updates
 * 
 * *****************  Version 13  *****************
 * User: John Omalley Date: 12-11-12   Time: 6:26p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review Update
 * 
 * *****************  Version 12  *****************
 * User: John Omalley Date: 12-11-12   Time: 4:30p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review Update
 * 
 * *****************  Version 11  *****************
 * User: John Omalley Date: 12-10-30   Time: 5:48p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Changed Actions to UINT8
 * 
 * *****************  Version 10  *****************
 * User: John Omalley Date: 12-10-18   Time: 1:56p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Design Review Updates
 * 
 * *****************  Version 9  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 8  *****************
 * User: John Omalley Date: 12-08-28   Time: 8:33a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Added Action Acknowledgable function for the evaluator
 *
 * *****************  Version 7  *****************
 * User: John Omalley Date: 12-08-24   Time: 9:30a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - ETM Fault Action Logic
 *
 * *****************  Version 6  *****************
 * User: John Omalley Date: 12-08-16   Time: 4:16p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Fault Action Processing
 *
 * *****************  Version 5  *****************
 * User: John Omalley Date: 12-08-14   Time: 2:59p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review Updates
 *
 * *****************  Version 4  *****************
 * User: John Omalley Date: 12-08-13   Time: 4:21p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Clean up Action Priorities and Latch
 *
 * *****************  Version 3  *****************
 * User: John Omalley Date: 12-07-27   Time: 3:03p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Action Manager Persistent Updates
 *
 * *****************  Version 1  *****************
 * User: John Omalley Date: 12-07-17   Time: 11:32a
 * Created in $/software/control processor/code/system
 * SCR 1107
 *
 *
 ***************************************************************************/

